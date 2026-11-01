#pragma once

/**
 * @file StandaloneApp.h
 * @brief Self-contained C++20 Standalone Desktop Application host wrapper for any Avra Processor.
 *
 * Wraps an Avra Processor and its Direct2D Editor inside a native desktop window with a
 * 34px Standalone Host Bar, full Audio & Device Preferences Modal Dialog (live waveIn/waveOut
 * hardware enumeration, Mono In 1 / Mono In 2 / Stereo input routing, Mono / Stereo output mode,
 * 44.1–96 kHz sample rate & 64–512 sample buffer size selectors, 440 Hz -18 dBFS test tone),
 * .avra_preset Save/Load dialogs, and persistent <PluginName>_standalone.cfg settings.
 */

#include "avra/Avra.h"
#include "avra/gui/controls/Button.h"
#include <array>
#include <vector>
#include <cmath>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <format>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#endif

namespace avra::standalone_bridge {

template <typename PluginType>
class StandaloneHostShell final : public gui::Component {
public:
    static constexpr float kHeaderHeight = 34.0f;

    StandaloneHostShell() {
        refreshDeviceLists();
        loadStandaloneConfig();

        processor_.prepare(static_cast<double>(sampleRate_.load(std::memory_order_relaxed)),
                           static_cast<size_t>(bufferSize_.load(std::memory_order_relaxed)));
        editor_ = processor_.createEditor();

        if (editor_) {
            addChild(editor_.get());
        }

        audioEngine_.setDuplexCallback([this](const float* hwInL, const float* hwInR, float* outL, float* outR, size_t numFrames) {
            renderAudioBlock(hwInL, hwInR, outL, outR, numFrames);
        });

        // Default to Live Hardware Input (auditionMode_ = 0: OFF / Live Input) so startup NEVER auto-plays drums!
        auditionMode_.store(0, std::memory_order_relaxed);
        reinitializeAudioStream();
    }

    ~StandaloneHostShell() override {
        audioEngine_.close();
    }

    [[nodiscard]] int getWindowWidth() const noexcept {
        return editor_ ? editor_->getDefaultWidth() : 1020;
    }

    [[nodiscard]] int getWindowHeight() const noexcept {
        return (editor_ ? editor_->getDefaultHeight() : 560) + static_cast<int>(kHeaderHeight);
    }

    void resized() override {
        const auto b = getLocalBounds();
        if (editor_) {
            editor_->setBounds(0.0f, kHeaderHeight, b.width, std::max(0.0f, b.height - kHeaderHeight));
        }
    }

    [[nodiscard]] bool interceptsOverlayMouse(gui::Point<float> pt) const noexcept override {
        if (showAudioSettingsModal_) return true;
        if (pt.y < kHeaderHeight) return true;
        return false;
    }

    void onTimerTick() override {
        const bool shouldRun = !muted_.load(std::memory_order_relaxed) &&
            (testToneActive_.load(std::memory_order_relaxed) ||
             auditionMode_.load(std::memory_order_relaxed) != 0 ||
             inputDeviceId_.load(std::memory_order_relaxed) >= 0);

        if (shouldRun) {
            if (!audioEngine_.isOpen()) {
                (void)audioEngine_.open();
            }
        } else if (audioEngine_.isOpen()) {
            audioEngine_.close();
            inPeakDb_.store(-90.0f, std::memory_order_relaxed);
            outPeakDb_.store(-90.0f, std::memory_order_relaxed);
        }
        repaint();
    }

    void onMouseMove(const gui::MouseEvent& event) override {
        mousePos_ = event.pos;
        repaint();
    }

    void onMouseDown(const gui::MouseEvent& event) override {
        if (showAudioSettingsModal_) {
            handleModalMouseDown(event.pos);
            return;
        }

        if (event.pos.y <= kHeaderHeight) {
            if (event.isRightDown) {
                openAuditionPopupMenu();
                return;
            }
            handleHeaderMouseDown(event.pos);
            return;
        }

        gui::Component::onMouseDown(event);
    }

    void paint(gui::Graphics& g) override {
        const auto b = getLocalBounds();
        // Top 34px Standalone Utility Header Bar
        const gui::Rect<float> barRect = { 0.0f, 0.0f, b.width, kHeaderHeight };
        g.fillLinearGradientRect(barRect, gui::Color::fromHex(0x181C22), gui::Color::fromHex(0x101318));
        g.drawLine({ 0.0f, kHeaderHeight - 0.5f }, { b.width, kHeaderHeight - 0.5f }, gui::Color::fromHex(0x2D3844), 1.0f);

        // Left Group: [ ⚙ AUDIO & DEVICE SETTINGS ] + [ 📂 LOAD PRESET ] + [ 💾 SAVE PRESET ]
        drawHeaderButton(g, rectBtnAudioSettings(), "AUDIO & DEVICE SETTINGS",
                         showAudioSettingsModal_, gui::Color::fromHex(0xF08C28));
        drawHeaderButton(g, rectBtnLoadPreset(), "LOAD PRESET",
                         false, gui::Color::fromHex(0x00F5D4));
        drawHeaderButton(g, rectBtnSavePreset(), "SAVE PRESET",
                         false, gui::Color::fromHex(0x00F5D4));

        // Center Group: [ AUDITION: OFF (LIVE IN) ▾ ] + [ 🔇 MUTE ] + [ BYPASS ]
        const int audMode = auditionMode_.load(std::memory_order_relaxed);
        drawHeaderButton(g, rectBtnAudition(), getAuditionButtonLabel(audMode),
                         audMode != 0, gui::Color::fromHex(0xFFB448));
        drawHeaderButton(g, rectBtnMute(), muted_.load(std::memory_order_relaxed) ? "MUTED" : "MUTE",
                         muted_.load(std::memory_order_relaxed), gui::Color::fromHex(0xEF476F));
        drawHeaderButton(g, rectBtnBypass(), "BYPASS",
                         bypassed_.load(std::memory_order_relaxed), gui::Color::fromHex(0xFFD166));

        // Right Group: Live Status Readout
        const gui::Rect<float> statusRect = rectStatusReadout(b.width);
        if (statusRect.width > 120.0f) {
            g.fillRoundedRect(statusRect, 4.0f, gui::Color::fromHex(0x0B0E12));
            g.drawRoundedRect(statusRect, 4.0f, gui::Color::fromHex(0x26303A), 1.0f);

            const uint32_t sr     = sampleRate_.load(std::memory_order_relaxed);
            const int      bufSmp = bufferSize_.load(std::memory_order_relaxed);
            const double   latMs  = (sr > 0) ? (1000.0 * static_cast<double>(bufSmp) / static_cast<double>(sr)) : 0.0;
            const int      inCh   = inputChannelMode_.load(std::memory_order_relaxed);
            const int      inDev  = inputDeviceId_.load(std::memory_order_relaxed);
            const char*    inTag  = (audMode != 0)
                ? "AUDITION"
                : ((inDev < 0) ? "IN: NONE" : (inCh == 0 ? "IN: MONO CH 1" : (inCh == 1 ? "IN: MONO CH 2" : "IN: STEREO 1+2")));

            const float outDb = outPeakDb_.load(std::memory_order_relaxed);
            const std::string outStr = (outDb <= -75.0f || muted_.load(std::memory_order_relaxed))
                ? "-inf dBFS"
                : std::format("{:+.1f} dBFS", outDb);

            const std::string statusText = std::format(
                "{:.1f} kHz | {} smp ({:.1f} ms) | {} | OUT: {}",
                static_cast<double>(sr) / 1000.0, bufSmp, latMs, inTag, outStr);

            // Live activity LED dot inside status readout
            const bool sigActive = (outDb > -65.0f && !muted_.load(std::memory_order_relaxed));
            g.fillCircle({ statusRect.x + 9.0f, statusRect.y + statusRect.height * 0.5f }, 3.2f,
                         sigActive ? gui::Color::fromHex(0x52B788) : gui::Color::fromHex(0x3B4652));

            g.drawText(statusText,
                       { statusRect.x + 16.0f, statusRect.y, statusRect.width - 20.0f, statusRect.height },
                       gui::Color::fromHex(0xC8D6E2), 9.0f, true, true);
        }
    }

    void paintOverlay(gui::Graphics& g) override {
        if (!showAudioSettingsModal_) return;

        const auto b = getLocalBounds();
        // Translucent dark backdrop
        g.fillRect(b, gui::Color(0.02f, 0.03f, 0.05f, 0.78f));

        const gui::Rect<float> m = getModalRect();
        // Drop shadow
        g.fillRoundedRect({ m.x + 4.0f, m.y + 6.0f, m.width, m.height }, 8.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.65f));
        // Chassis panel
        g.fillLinearGradientRect(m, gui::Color::fromHex(0x1A2028), gui::Color::fromHex(0x11151B), 8.0f);
        g.drawRoundedRect(m, 8.0f, gui::Color::fromHex(0x46586A), 1.6f);

        // Header banner
        const gui::Rect<float> hdr = { m.x, m.y, m.width, 38.0f };
        g.fillLinearGradientRect(hdr, gui::Color::fromHex(0x242C36), gui::Color::fromHex(0x181E26), 8.0f);
        g.drawLine({ m.x, m.y + 38.0f }, { m.right(), m.y + 38.0f }, gui::Color::fromHex(0xF08C28, 0.75f), 1.4f);
        g.drawText("\xE2\x9A\x99  AUDIO & HARDWARE DEVICE PREFERENCES",
                   { m.x + 18.0f, m.y + 4.0f, m.width - 70.0f, 30.0f },
                   gui::Color::fromHex(0xF4F8FB), 12.0f, false, true);

        // Close [ X ] button
        const gui::Rect<float> closeBtn = { m.right() - 36.0f, m.y + 7.0f, 26.0f, 24.0f };
        drawModalPill(g, closeBtn, "\xE2\x9C\x95", false, gui::Color::fromHex(0xEF476F));

        const float lblX = m.x + 20.0f;
        const float lblW = 188.0f;
        const float ctlX = m.x + 214.0f;

        // 1. Audio Driver Mode
        const float r1Y = m.y + 52.0f;
        g.drawText("1. AUDIO DRIVER MODE", { lblX, r1Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        const int drvMode = driverMode_.load(std::memory_order_relaxed);
        drawModalPill(g, { ctlX, r1Y, 208.0f, 28.0f }, "WASAPI Low-Latency (Shared)", drvMode == 0, gui::Color::fromHex(0x00F5D4));
        drawModalPill(g, { ctlX + 216.0f, r1Y, 208.0f, 28.0f }, "WinMM Wave I/O", drvMode == 1, gui::Color::fromHex(0x00F5D4));

        // 2. Audio Input Device (Real Hardware Enumeration)
        const float r2Y = m.y + 94.0f;
        g.drawText("2. AUDIO INPUT DEVICE", { lblX, r2Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        drawModalPill(g, { ctlX, r2Y, 30.0f, 28.0f }, "<", false, gui::Color::fromHex(0xFFB448));
        drawDeviceSelectorBox(g, { ctlX + 35.0f, r2Y, 354.0f, 28.0f }, getActiveInputDeviceName() + "  \xE2\x96\xBE");
        drawModalPill(g, { ctlX + 394.0f, r2Y, 30.0f, 28.0f }, ">", false, gui::Color::fromHex(0xFFB448));

        // 3. Input Channel Routing (Mono-First Support)
        const float r3Y = m.y + 136.0f;
        g.drawText("3. INPUT CHANNEL ROUTING", { lblX, r3Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        const int inCh = inputChannelMode_.load(std::memory_order_relaxed);
        drawModalPill(g, { ctlX, r3Y, 136.0f, 28.0f }, "MONO (IN 1)", inCh == 0, gui::Color::fromHex(0xFFB448));
        drawModalPill(g, { ctlX + 144.0f, r3Y, 136.0f, 28.0f }, "MONO (IN 2)", inCh == 1, gui::Color::fromHex(0xFFB448));
        drawModalPill(g, { ctlX + 288.0f, r3Y, 136.0f, 28.0f }, "STEREO (IN 1+2)", inCh == 2, gui::Color::fromHex(0xFFB448));

        // 4. Audio Output Device (Real Hardware Enumeration)
        const float r4Y = m.y + 178.0f;
        g.drawText("4. AUDIO OUTPUT DEVICE", { lblX, r4Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        drawModalPill(g, { ctlX, r4Y, 30.0f, 28.0f }, "<", false, gui::Color::fromHex(0x00F5D4));
        drawDeviceSelectorBox(g, { ctlX + 35.0f, r4Y, 354.0f, 28.0f }, getActiveOutputDeviceName() + "  \xE2\x96\xBE");
        drawModalPill(g, { ctlX + 394.0f, r4Y, 30.0f, 28.0f }, ">", false, gui::Color::fromHex(0x00F5D4));

        // 5. Output Mode
        const float r5Y = m.y + 220.0f;
        g.drawText("5. OUTPUT MODE", { lblX, r5Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        const int outMode = outputMode_.load(std::memory_order_relaxed);
        drawModalPill(g, { ctlX, r5Y, 208.0f, 28.0f }, "MONO (PURE)", outMode == 0, gui::Color::fromHex(0x00F5D4));
        drawModalPill(g, { ctlX + 216.0f, r5Y, 208.0f, 28.0f }, "STEREO (OUT 1+2)", outMode == 1, gui::Color::fromHex(0x00F5D4));

        // 6. Sample Rate Selector
        const float r6Y = m.y + 262.0f;
        g.drawText("6. SAMPLE RATE", { lblX, r6Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        const uint32_t sr = sampleRate_.load(std::memory_order_relaxed);
        drawModalPill(g, { ctlX, r6Y, 100.0f, 28.0f }, "44.1 kHz", sr == 44100, gui::Color::fromHex(0x52B788));
        drawModalPill(g, { ctlX + 108.0f, r6Y, 100.0f, 28.0f }, "48.0 kHz", sr == 48000, gui::Color::fromHex(0x52B788));
        drawModalPill(g, { ctlX + 216.0f, r6Y, 100.0f, 28.0f }, "88.2 kHz", sr == 88200, gui::Color::fromHex(0x52B788));
        drawModalPill(g, { ctlX + 324.0f, r6Y, 100.0f, 28.0f }, "96.0 kHz", sr == 96000, gui::Color::fromHex(0x52B788));

        // 7. Buffer Size / Latency Selector
        const float r7Y = m.y + 304.0f;
        g.drawText("7. BUFFER SIZE / LATENCY", { lblX, r7Y, lblW, 28.0f }, gui::Color::fromHex(0xB8C6D4), 9.8f, false, true);
        const int bufSmp = bufferSize_.load(std::memory_order_relaxed);
        static constexpr int kBufSizes[4] = { 64, 128, 256, 512 };
        for (int i = 0; i < 4; ++i) {
            const double ms = (sr > 0) ? (1000.0 * static_cast<double>(kBufSizes[i]) / static_cast<double>(sr)) : 0.0;
            const std::string lbl = std::format("{} smp ({:.1f} ms)", kBufSizes[i], ms);
            drawModalPill(g, { ctlX + static_cast<float>(i) * 108.0f, r7Y, 100.0f, 28.0f },
                          lbl, bufSmp == kBufSizes[i], gui::Color::fromHex(0xFFB448));
        }

        // Info / Persistence Status Box
        const gui::Rect<float> infoBox = { m.x + 20.0f, m.y + 346.0f, m.width - 40.0f, 34.0f };
        g.fillRoundedRect(infoBox, 4.0f, gui::Color::fromHex(0x0C1015));
        g.drawRoundedRect(infoBox, 4.0f, gui::Color::fromHex(0x283442), 1.0f);
        const std::string cfgInfo = std::format(
            "CONFIG FILE: {}  |  IN: {} dev(s)  |  OUT: {} dev(s)",
            getStandaloneConfigFilePath().filename().string(),
            std::max<int>(0, static_cast<int>(inputDevices_.size()) - 1),
            std::max<int>(0, static_cast<int>(outputDevices_.size()) - 1));
        g.drawText(cfgInfo, infoBox, gui::Color::fromHex(0x8FA4B8), 9.0f, true, true);

        // 8. Footer Buttons: [ 🔊 TEST TONE (440 Hz) ] + [ ↻ RESCAN HW ] + [ ✓ APPLY & CLOSE ]
        const float r8Y = m.y + 394.0f;
        const bool toneOn = testToneActive_.load(std::memory_order_relaxed);
        drawModalPill(g, { m.x + 20.0f, r8Y, 214.0f, 34.0f },
                      toneOn ? "TEST TONE: ON (-18 dBFS)" : "TEST TONE (440 Hz)",
                      toneOn, gui::Color::fromHex(0xF08C28));
        drawModalPill(g, { m.x + 244.0f, r8Y, 146.0f, 34.0f },
                      "\xE2\x86\xBB RESCAN DEVICES",
                      false, gui::Color::fromHex(0x00F5D4));
        drawModalPill(g, { m.x + 400.0f, r8Y, 238.0f, 34.0f },
                      "\xE2\x9C\x93 APPLY & CLOSE",
                      true, gui::Color::fromHex(0x52B788));
    }

private:
    // Top Header Bar Button Geometry
    [[nodiscard]] static gui::Rect<float> rectBtnAudioSettings() noexcept { return { 8.0f, 5.0f, 174.0f, 24.0f }; }
    [[nodiscard]] static gui::Rect<float> rectBtnLoadPreset() noexcept    { return { 187.0f, 5.0f, 102.0f, 24.0f }; }
    [[nodiscard]] static gui::Rect<float> rectBtnSavePreset() noexcept    { return { 294.0f, 5.0f, 102.0f, 24.0f }; }
    [[nodiscard]] static gui::Rect<float> rectBtnAudition() noexcept      { return { 404.0f, 5.0f, 168.0f, 24.0f }; }
    [[nodiscard]] static gui::Rect<float> rectBtnMute() noexcept          { return { 577.0f, 5.0f, 70.0f, 24.0f }; }
    [[nodiscard]] static gui::Rect<float> rectBtnBypass() noexcept        { return { 652.0f, 5.0f, 66.0f, 24.0f }; }
    [[nodiscard]] static gui::Rect<float> rectStatusReadout(float totalW) noexcept {
        const float x = 724.0f;
        return { x, 5.0f, std::max(0.0f, totalW - x - 8.0f), 24.0f };
    }

    [[nodiscard]] gui::Rect<float> getModalRect() const noexcept {
        const auto b = getLocalBounds();
        const float w = 658.0f;
        const float h = 444.0f;
        return { std::max(10.0f, (b.width - w) * 0.5f), std::max(10.0f, (b.height - h) * 0.5f), w, h };
    }

    void drawHeaderButton(gui::Graphics& g, const gui::Rect<float>& r, const std::string& label, bool active, const gui::Color& accent) const {
        const bool hovered = !showAudioSettingsModal_ && r.contains(mousePos_);
        const gui::Color topBg = active
            ? accent.withAlpha(0.28f)
            : (hovered ? gui::Color::fromHex(0x28303B) : gui::Color::fromHex(0x1E242D));
        const gui::Color botBg = active
            ? accent.withAlpha(0.16f)
            : (hovered ? gui::Color::fromHex(0x1E242D) : gui::Color::fromHex(0x151920));
        const gui::Color border = active
            ? accent
            : (hovered ? accent.withAlpha(0.65f) : gui::Color::fromHex(0x34404E));
        const gui::Color textCol = active
            ? gui::Color::fromHex(0xFFFFFF)
            : (hovered ? gui::Color::fromHex(0xF0F6FA) : gui::Color::fromHex(0xC8D4E0));

        g.fillLinearGradientRect(r, topBg, botBg, 4.0f);
        g.drawRoundedRect(r, 4.0f, border, active ? 1.4f : 1.0f);
        g.drawText(label, r, textCol, 9.2f, true, active || hovered);
    }

    void drawModalPill(gui::Graphics& g, const gui::Rect<float>& r, const std::string& label, bool active, const gui::Color& accent) const {
        const bool hovered = r.contains(mousePos_);
        const gui::Color topBg = active
            ? accent.withAlpha(0.30f)
            : (hovered ? gui::Color::fromHex(0x2A3340) : gui::Color::fromHex(0x1D232C));
        const gui::Color botBg = active
            ? accent.withAlpha(0.16f)
            : (hovered ? gui::Color::fromHex(0x202732) : gui::Color::fromHex(0x141920));
        const gui::Color border = active
            ? accent
            : (hovered ? accent.withAlpha(0.70f) : gui::Color::fromHex(0x364352));
        const gui::Color textCol = active
            ? gui::Color::fromHex(0xFFFFFF)
            : (hovered ? gui::Color::fromHex(0xF2F7FB) : gui::Color::fromHex(0xC4D0DC));

        g.fillLinearGradientRect(r, topBg, botBg, 4.5f);
        g.drawRoundedRect(r, 4.5f, border, active ? 1.5f : 1.0f);
        g.drawText(label, r, textCol, 9.3f, true, active || hovered);
    }

    void drawDeviceSelectorBox(gui::Graphics& g, const gui::Rect<float>& r, const std::string& label) const {
        const bool hovered = r.contains(mousePos_);
        g.fillRoundedRect(r, 4.5f, hovered ? gui::Color::fromHex(0x18202A) : gui::Color::fromHex(0x0E1319));
        g.drawRoundedRect(r, 4.5f, hovered ? gui::Color::fromHex(0x00F5D4, 0.75f) : gui::Color::fromHex(0x364454), 1.2f);
        g.drawText(label, { r.x + 10.0f, r.y, r.width - 20.0f, r.height }, gui::Color::fromHex(0xE8F1F8), 9.5f, true, true);
    }

    [[nodiscard]] static const char* getAuditionButtonLabel(int mode) noexcept {
        switch (mode) {
            case 0: return "AUDITION: OFF (LIVE IN) \xE2\x96\xBE";
            case 1: return "AUDITION: WAV FILE \xE2\x96\xBE";
            default: return "AUDITION: OFF (LIVE IN) \xE2\x96\xBE";
        }
    }

    void handleHeaderMouseDown(gui::Point<float> pt) {
        if (rectBtnAudioSettings().contains(pt)) {
            refreshDeviceLists();
            showAudioSettingsModal_ = !showAudioSettingsModal_;
            if (!showAudioSettingsModal_) {
                testToneActive_.store(false, std::memory_order_relaxed);
            }
            repaint();
            return;
        }
        if (rectBtnLoadPreset().contains(pt)) {
            openLoadPresetDialog();
            return;
        }
        if (rectBtnSavePreset().contains(pt)) {
            openSavePresetDialog();
            return;
        }
        if (rectBtnAudition().contains(pt)) {
            openAuditionPopupMenu();
            return;
        }
        if (rectBtnMute().contains(pt)) {
            const bool nextMute = !muted_.load(std::memory_order_relaxed);
            muted_.store(nextMute, std::memory_order_relaxed);
            repaint();
            return;
        }
        if (rectBtnBypass().contains(pt)) {
            const bool nextByp = !bypassed_.load(std::memory_order_relaxed);
            bypassed_.store(nextByp, std::memory_order_relaxed);
            repaint();
            return;
        }
    }

    void handleModalMouseDown(gui::Point<float> pt) {
        const gui::Rect<float> m = getModalRect();
        const gui::Rect<float> closeBtn = { m.right() - 36.0f, m.y + 7.0f, 26.0f, 24.0f };
        if (!m.contains(pt) || closeBtn.contains(pt)) {
            testToneActive_.store(false, std::memory_order_relaxed);
            showAudioSettingsModal_ = false;
            saveStandaloneConfig();
            repaint();
            return;
        }

        const float ctlX = m.x + 214.0f;

        // 1. Audio Driver Mode
        const float r1Y = m.y + 52.0f;
        if (gui::Rect<float>{ ctlX, r1Y, 208.0f, 28.0f }.contains(pt)) {
            driverMode_.store(0, std::memory_order_relaxed);
            reinitializeAudioStream();
            saveStandaloneConfig();
            return;
        }
        if (gui::Rect<float>{ ctlX + 216.0f, r1Y, 208.0f, 28.0f }.contains(pt)) {
            driverMode_.store(1, std::memory_order_relaxed);
            reinitializeAudioStream();
            saveStandaloneConfig();
            return;
        }

        // 2. Audio Input Device
        const float r2Y = m.y + 94.0f;
        if (gui::Rect<float>{ ctlX, r2Y, 30.0f, 28.0f }.contains(pt)) {
            cycleInputDevice(-1);
            return;
        }
        if (gui::Rect<float>{ ctlX + 35.0f, r2Y, 354.0f, 28.0f }.contains(pt)) {
            openInputDevicePopupMenu();
            return;
        }
        if (gui::Rect<float>{ ctlX + 394.0f, r2Y, 30.0f, 28.0f }.contains(pt)) {
            cycleInputDevice(+1);
            return;
        }

        // 3. Input Channel Routing
        const float r3Y = m.y + 136.0f;
        for (int ch = 0; ch < 3; ++ch) {
            if (gui::Rect<float>{ ctlX + static_cast<float>(ch) * 144.0f, r3Y, 136.0f, 28.0f }.contains(pt)) {
                inputChannelMode_.store(ch, std::memory_order_relaxed);
                saveStandaloneConfig();
                repaint();
                return;
            }
        }

        // 4. Audio Output Device
        const float r4Y = m.y + 178.0f;
        if (gui::Rect<float>{ ctlX, r4Y, 30.0f, 28.0f }.contains(pt)) {
            cycleOutputDevice(-1);
            return;
        }
        if (gui::Rect<float>{ ctlX + 35.0f, r4Y, 354.0f, 28.0f }.contains(pt)) {
            openOutputDevicePopupMenu();
            return;
        }
        if (gui::Rect<float>{ ctlX + 394.0f, r4Y, 30.0f, 28.0f }.contains(pt)) {
            cycleOutputDevice(+1);
            return;
        }

        // 5. Output Mode
        const float r5Y = m.y + 220.0f;
        if (gui::Rect<float>{ ctlX, r5Y, 208.0f, 28.0f }.contains(pt)) {
            outputMode_.store(0, std::memory_order_relaxed);
            saveStandaloneConfig();
            repaint();
            return;
        }
        if (gui::Rect<float>{ ctlX + 216.0f, r5Y, 208.0f, 28.0f }.contains(pt)) {
            outputMode_.store(1, std::memory_order_relaxed);
            saveStandaloneConfig();
            repaint();
            return;
        }

        // 6. Sample Rate Selector
        const float r6Y = m.y + 262.0f;
        static constexpr uint32_t kRates[4] = { 44100, 48000, 88200, 96000 };
        for (int i = 0; i < 4; ++i) {
            if (gui::Rect<float>{ ctlX + static_cast<float>(i) * 108.0f, r6Y, 100.0f, 28.0f }.contains(pt)) {
                sampleRate_.store(kRates[i], std::memory_order_relaxed);
                reinitializeAudioStream();
                saveStandaloneConfig();
                repaint();
                return;
            }
        }

        // 7. Buffer Size / Latency Selector
        const float r7Y = m.y + 304.0f;
        static constexpr int kBufSizes[4] = { 64, 128, 256, 512 };
        for (int i = 0; i < 4; ++i) {
            if (gui::Rect<float>{ ctlX + static_cast<float>(i) * 108.0f, r7Y, 100.0f, 28.0f }.contains(pt)) {
                bufferSize_.store(kBufSizes[i], std::memory_order_relaxed);
                reinitializeAudioStream();
                saveStandaloneConfig();
                repaint();
                return;
            }
        }

        // 8. Footer Buttons
        const float r8Y = m.y + 394.0f;
        if (gui::Rect<float>{ m.x + 20.0f, r8Y, 214.0f, 34.0f }.contains(pt)) {
            const bool nextTone = !testToneActive_.load(std::memory_order_relaxed);
            testToneActive_.store(nextTone, std::memory_order_relaxed);
            if (nextTone) {
                muted_.store(false, std::memory_order_relaxed);
                if (!audioEngine_.isOpen()) (void)audioEngine_.open();
            }
            repaint();
            return;
        }
        if (gui::Rect<float>{ m.x + 244.0f, r8Y, 146.0f, 34.0f }.contains(pt)) {
            refreshDeviceLists();
            reinitializeAudioStream();
            repaint();
            return;
        }
        if (gui::Rect<float>{ m.x + 400.0f, r8Y, 238.0f, 34.0f }.contains(pt)) {
            testToneActive_.store(false, std::memory_order_relaxed);
            showAudioSettingsModal_ = false;
            reinitializeAudioStream();
            saveStandaloneConfig();
            repaint();
            return;
        }
    }

    void refreshDeviceLists() {
        inputDevices_  = audio_io::enumerateInputDevices();
        outputDevices_ = audio_io::enumerateOutputDevices();
    }

    [[nodiscard]] std::string getActiveInputDeviceName() const {
        const int id = inputDeviceId_.load(std::memory_order_relaxed);
        for (const auto& d : inputDevices_) {
            if (d.deviceId == id) return d.name;
        }
        return inputDevices_.empty() ? "None (Muted / Audition Only)" : inputDevices_.front().name;
    }

    [[nodiscard]] std::string getActiveOutputDeviceName() const {
        const int id = outputDeviceId_.load(std::memory_order_relaxed);
        for (const auto& d : outputDevices_) {
            if (d.deviceId == id) return d.name;
        }
        return outputDevices_.empty() ? "System Default Output (WAVE_MAPPER)" : outputDevices_.front().name;
    }

    void cycleInputDevice(int delta) {
        if (inputDevices_.empty()) return;
        const int curId = inputDeviceId_.load(std::memory_order_relaxed);
        int idx = 0;
        for (size_t i = 0; i < inputDevices_.size(); ++i) {
            if (inputDevices_[i].deviceId == curId) {
                idx = static_cast<int>(i);
                break;
            }
        }
        const int n = static_cast<int>(inputDevices_.size());
        idx = (idx + delta % n + n) % n;
        inputDeviceId_.store(inputDevices_[static_cast<size_t>(idx)].deviceId, std::memory_order_relaxed);
        reinitializeAudioStream();
        saveStandaloneConfig();
        repaint();
    }

    void cycleOutputDevice(int delta) {
        if (outputDevices_.empty()) return;
        const int curId = outputDeviceId_.load(std::memory_order_relaxed);
        int idx = 0;
        for (size_t i = 0; i < outputDevices_.size(); ++i) {
            if (outputDevices_[i].deviceId == curId) {
                idx = static_cast<int>(i);
                break;
            }
        }
        const int n = static_cast<int>(outputDevices_.size());
        idx = (idx + delta % n + n) % n;
        outputDeviceId_.store(outputDevices_[static_cast<size_t>(idx)].deviceId, std::memory_order_relaxed);
        reinitializeAudioStream();
        saveStandaloneConfig();
        repaint();
    }

    void openInputDevicePopupMenu() {
#if defined(_WIN32)
        refreshDeviceLists();
        HMENU hMenu = CreatePopupMenu();
        if (!hMenu) return;
        const int curId = inputDeviceId_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < inputDevices_.size(); ++i) {
            const UINT flags = MF_STRING | (inputDevices_[i].deviceId == curId ? MF_CHECKED : 0);
            AppendMenuA(hMenu, flags, static_cast<UINT_PTR>(1000 + i), inputDevices_[i].name.c_str());
        }
        POINT pt{};
        GetCursorPos(&pt);
        HWND hwnd = GetActiveWindow();
        const int cmd = static_cast<int>(TrackPopupMenu(
            hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr));
        DestroyMenu(hMenu);
        if (cmd >= 1000 && static_cast<size_t>(cmd - 1000) < inputDevices_.size()) {
            inputDeviceId_.store(inputDevices_[static_cast<size_t>(cmd - 1000)].deviceId, std::memory_order_relaxed);
            reinitializeAudioStream();
            saveStandaloneConfig();
            repaint();
        }
#endif
    }

    void openOutputDevicePopupMenu() {
#if defined(_WIN32)
        refreshDeviceLists();
        HMENU hMenu = CreatePopupMenu();
        if (!hMenu) return;
        const int curId = outputDeviceId_.load(std::memory_order_relaxed);
        for (size_t i = 0; i < outputDevices_.size(); ++i) {
            const UINT flags = MF_STRING | (outputDevices_[i].deviceId == curId ? MF_CHECKED : 0);
            AppendMenuA(hMenu, flags, static_cast<UINT_PTR>(2000 + i), outputDevices_[i].name.c_str());
        }
        POINT pt{};
        GetCursorPos(&pt);
        HWND hwnd = GetActiveWindow();
        const int cmd = static_cast<int>(TrackPopupMenu(
            hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr));
        DestroyMenu(hMenu);
        if (cmd >= 2000 && static_cast<size_t>(cmd - 2000) < outputDevices_.size()) {
            outputDeviceId_.store(outputDevices_[static_cast<size_t>(cmd - 2000)].deviceId, std::memory_order_relaxed);
            reinitializeAudioStream();
            saveStandaloneConfig();
            repaint();
        }
#endif
    }

    void openAuditionPopupMenu() {
#if defined(_WIN32)
        HMENU hMenu = CreatePopupMenu();
        if (!hMenu) return;
        const int curMode = auditionMode_.load(std::memory_order_relaxed);

        AppendMenuA(hMenu, MF_STRING | (curMode == 0 ? MF_CHECKED : 0), 100, "OFF (Live Hardware Input)");
        AppendMenuA(hMenu, MF_SEPARATOR, 0, nullptr);
        if (wavLoopPlayer_.isLoaded()) {
            const std::string wavItem = "WAV File: " + wavLoopPlayer_.getFileName();
            AppendMenuA(hMenu, MF_STRING | (curMode == 1 ? MF_CHECKED : 0), 102, wavItem.c_str());
        }
        AppendMenuA(hMenu, MF_STRING | ((curMode == 1 && !wavLoopPlayer_.isLoaded()) ? MF_CHECKED : 0), 101, "Load WAV File...");

        POINT pt{};
        GetCursorPos(&pt);
        HWND hwnd = GetActiveWindow();
        const int cmd = static_cast<int>(TrackPopupMenu(
            hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr));
        DestroyMenu(hMenu);

        if (cmd == 100) setAuditionMode(0);
        else if (cmd == 101) openWavLoopDialog();
        else if (cmd == 102) setAuditionMode(1);
#endif
    }

    void setAuditionMode(int mode) {
        auditionMode_.store(std::clamp(mode, 0, 1), std::memory_order_relaxed);
        if (mode != 0) {
            muted_.store(false, std::memory_order_relaxed);
            if (!audioEngine_.isOpen()) (void)audioEngine_.open();
        }
        repaint();
    }

    void reinitializeAudioStream() {
        const uint32_t sr     = sampleRate_.load(std::memory_order_relaxed);
        const int      bufSmp = bufferSize_.load(std::memory_order_relaxed);
        const int      inDev  = inputDeviceId_.load(std::memory_order_relaxed);
        const int      outDev = outputDeviceId_.load(std::memory_order_relaxed);
        const int      drv    = driverMode_.load(std::memory_order_relaxed);

        audioEngine_.close();
        processor_.prepare(static_cast<double>(sr), static_cast<size_t>(std::max(64, bufSmp)));
        audioEngine_.configureDevice(inDev, outDev, sr, static_cast<size_t>( std::max(64, bufSmp)), drv);

        const bool shouldRun = !muted_.load(std::memory_order_relaxed) &&
            (testToneActive_.load(std::memory_order_relaxed) ||
             auditionMode_.load(std::memory_order_relaxed) != 0 ||
             inDev >= 0);
        if (shouldRun) {
            (void)audioEngine_.open();
        }
    }

    void renderAudioBlock(const float* hwInL, const float* hwInR, float* outL, float* outR, size_t numFrames) {
        const size_t nFrames = std::min<size_t>(numFrames, 1024);
        if (muted_.load(std::memory_order_relaxed)) {
            std::fill_n(outL, nFrames, 0.0f);
            std::fill_n(outR, nFrames, 0.0f);
            inPeakDb_.store(-90.0f, std::memory_order_relaxed);
            outPeakDb_.store(-90.0f, std::memory_order_relaxed);
            return;
        }

        std::array<float, 1024> inL{};
        std::array<float, 1024> inR{};
        const int      audMode = auditionMode_.load(std::memory_order_relaxed);
        const int      inCh    = inputChannelMode_.load(std::memory_order_relaxed);
        const int      outMode = outputMode_.load(std::memory_order_relaxed);
        const uint32_t sr      = std::max<uint32_t>(22050, sampleRate_.load(std::memory_order_relaxed));
        const double   invSr   = 1.0 / static_cast<double>(sr);

        for (size_t i = 0; i < nFrames; ++i) {
            float sL = 0.0f, sR = 0.0f;

            if (audMode == 0) {
                // Live Hardware Input routing with Mono-First support
                const float rawL = hwInL ? hwInL[i] : 0.0f;
                const float rawR = hwInR ? hwInR[i] : rawL;
                if (inCh == 0) {
                    // MONO (IN 1) -> centered to both L & R
                    sL = sR = rawL;
                } else if (inCh == 1) {
                    // MONO (IN 2) -> centered to both L & R
                    sL = sR = rawR;
                } else {
                    // STEREO (IN 1+2)
                    sL = rawL;
                    sR = rawR;
                }
            } else if (audMode == 1) {
                if (wavLoopPlayer_.isLoaded()) {
                    wavLoopPlayer_.nextStereoSample(sL, sR, static_cast<double>(sr));
                }
            }

            inL[i] = outL[i] = sL;
            inR[i] = outR[i] = sR;
        }

        float inPL = 0.0f, inRL = 0.0f, inPR = 0.0f, inRR = 0.0f;
        dsp::SimdBlock::computePeakAndRms(inL.data(), nFrames, inPL, inRL);
        dsp::SimdBlock::computePeakAndRms(inR.data(), nFrames, inPR, inRR);
        inPeakDb_.store(math::linearTodB(std::max(1e-5f, std::max(inPL, inPR))), std::memory_order_relaxed);

        if (!bypassed_.load(std::memory_order_relaxed)) {
            const float* inPtrs[2] = { inL.data(), inR.data() };
            float* outPtrs[2]      = { outL, outR };
            ProcessContext ctx{
                .inputs = AudioSpan<const float>(inPtrs, 2, nFrames),
                .outputs = AudioSpan<float>(outPtrs, 2, nFrames),
                .sampleRate = static_cast<double>(sr),
                .numFrames = nFrames
            };
            processor_.process(ctx);
        }

        // Optional 440 Hz -18 dBFS (0.1258925f) Test Tone verification generator
        if (testToneActive_.load(std::memory_order_relaxed)) {
            constexpr float kToneAmp = 0.1258925f; // -18.0 dBFS
            const double phaseInc = 2.0 * 3.14159265358979323846 * 440.0 * invSr;
            for (size_t i = 0; i < nFrames; ++i) {
                const float sine = static_cast<float>(std::sin(testTonePhase_)) * kToneAmp;
                testTonePhase_ += phaseInc;
                if (testTonePhase_ >= 6.28318530717958647692) {
                    testTonePhase_ -= 6.28318530717958647692;
                }
                outL[i] += sine;
                outR[i] += sine;
            }
        }

        // Output Mode: MONO (PURE) vs STEREO (OUT 1+2)
        if (outMode == 0) {
            for (size_t i = 0; i < nFrames; ++i) {
                const float mono = 0.5f * (outL[i] + outR[i]);
                outL[i] = outR[i] = mono;
            }
        }

        float pL = 0.0f, rL = 0.0f, pR = 0.0f, rR = 0.0f;
        dsp::SimdBlock::computePeakAndRms(outL, nFrames, pL, rL);
        dsp::SimdBlock::computePeakAndRms(outR, nFrames, pR, rR);
        outPeakDb_.store(math::linearTodB(std::max(1e-5f, std::max(pL, pR))), std::memory_order_relaxed);
    }

    void openWavLoopDialog() {
#if defined(_WIN32)
        wchar_t fileBuf[MAX_PATH] = {0};
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = GetActiveWindow();
        ofn.lpstrFilter = L"WAV Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = fileBuf;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrTitle  = L"Load Custom .WAV Stem for Standalone Audition";
        if (GetOpenFileNameW(&ofn)) {
            std::string wavErr;
            const std::string utf8Path = std::filesystem::path(fileBuf).string();
            if (wavLoopPlayer_.loadFromFile(utf8Path, wavErr)) {
                setAuditionMode(1);
                return;
            }
        }
#endif
        if (wavLoopPlayer_.isLoaded()) {
            setAuditionMode(1);
        }
    }

    void openSavePresetDialog() {
#if defined(_WIN32)
        wchar_t fileBuf[MAX_PATH] = L"Default.avra_preset";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = GetActiveWindow();
        ofn.lpstrFilter = L"Avra Plugin Preset (*.avra_preset)\0*.avra_preset\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = fileBuf;
        ofn.nMaxFile    = MAX_PATH;
        ofn.lpstrDefExt = L"avra_preset";
        ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrTitle  = L"Save Plugin Preset (.avra_preset)";
        if (GetSaveFileNameW(&ofn)) {
            std::ofstream out(std::filesystem::path(fileBuf), std::ios::binary | std::ios::trunc);
            if (out.is_open()) {
                out << processor_.getParameters().serializeState();
            }
        }
#endif
    }

    void openLoadPresetDialog() {
#if defined(_WIN32)
        wchar_t fileBuf[MAX_PATH] = {0};
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = GetActiveWindow();
        ofn.lpstrFilter = L"Avra Plugin Preset (*.avra_preset)\0*.avra_preset\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = fileBuf;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrTitle  = L"Load Plugin Preset (.avra_preset)";
        if (GetOpenFileNameW(&ofn)) {
            std::ifstream in(std::filesystem::path(fileBuf), std::ios::binary);
            if (in.is_open()) {
                std::ostringstream ss;
                ss << in.rdbuf();
                (void)processor_.getParameters().restoreState(ss.str());
                if (editor_) editor_->repaint();
                repaint();
            }
        }
#endif
    }

    [[nodiscard]] std::filesystem::path getStandaloneConfigFilePath() const {
        namespace fs = std::filesystem;
        std::string plugName = processor_.getName() ? processor_.getName() : "AvraPlugin";
        std::string cleanName;
        for (char c : plugName) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') cleanName.push_back(c);
        }
        if (cleanName.empty()) cleanName = "AvraPlugin";
        const std::string cfgFileName = cleanName + "_standalone.cfg";

#if defined(_WIN32)
        wchar_t exePathBuf[MAX_PATH] = {0};
        if (GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH) > 0) {
            const fs::path exeDir = fs::path(exePathBuf).parent_path();
            std::error_code ec;
            if (fs::exists(exeDir, ec)) {
                return exeDir / cfgFileName;
            }
        }
        wchar_t appDataBuf[MAX_PATH] = {0};
        if (GetEnvironmentVariableW(L"APPDATA", appDataBuf, MAX_PATH) > 0) {
            const fs::path appDir = fs::path(appDataBuf) / "AvraAudio";
            std::error_code ec;
            fs::create_directories(appDir, ec);
            return appDir / cfgFileName;
        }
#endif
        return fs::path(cfgFileName);
    }

    void saveStandaloneConfig() const {
        namespace fs = std::filesystem;
        const fs::path cfgPath = getStandaloneConfigFilePath();
        std::ofstream out(cfgPath, std::ios::trunc);
        if (!out.is_open()) {
#if defined(_WIN32)
            wchar_t appDataBuf[MAX_PATH] = {0};
            if (GetEnvironmentVariableW(L"APPDATA", appDataBuf, MAX_PATH) > 0) {
                std::error_code ec;
                const fs::path fallbackDir = fs::path(appDataBuf) / "AvraAudio";
                fs::create_directories(fallbackDir, ec);
                out.open(fallbackDir / cfgPath.filename(), std::ios::trunc);
            }
#endif
        }
        if (!out.is_open()) return;

        out << "AVRA_STANDALONE_CFG_V1\n"
            << "driver_mode=" << driverMode_.load(std::memory_order_relaxed) << "\n"
            << "input_device_id=" << inputDeviceId_.load(std::memory_order_relaxed) << "\n"
            << "input_device_name=" << getActiveInputDeviceName() << "\n"
            << "input_channel_mode=" << inputChannelMode_.load(std::memory_order_relaxed) << "\n"
            << "output_device_id=" << outputDeviceId_.load(std::memory_order_relaxed) << "\n"
            << "output_device_name=" << getActiveOutputDeviceName() << "\n"
            << "output_mode=" << outputMode_.load(std::memory_order_relaxed) << "\n"
            << "sample_rate=" << sampleRate_.load(std::memory_order_relaxed) << "\n"
            << "buffer_size=" << bufferSize_.load(std::memory_order_relaxed) << "\n";
    }

    void loadStandaloneConfig() {
        namespace fs = std::filesystem;
        fs::path cfgPath = getStandaloneConfigFilePath();
        std::ifstream in(cfgPath);
        if (!in.is_open()) {
#if defined(_WIN32)
            wchar_t appDataBuf[MAX_PATH] = {0};
            if (GetEnvironmentVariableW(L"APPDATA", appDataBuf, MAX_PATH) > 0) {
                in.open(fs::path(appDataBuf) / "AvraAudio" / cfgPath.filename());
            }
#endif
        }
        if (!in.is_open()) return;

        std::string savedInName;
        std::string savedOutName;
        std::string line;
        while (std::getline(in, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
            const auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = line.substr(0, eq);
            const std::string val = line.substr(eq + 1);
            try {
                if (key == "driver_mode") driverMode_.store(std::clamp(std::stoi(val), 0, 1), std::memory_order_relaxed);
                else if (key == "input_device_id") inputDeviceId_.store(std::stoi(val), std::memory_order_relaxed);
                else if (key == "input_device_name") savedInName = val;
                else if (key == "input_channel_mode") inputChannelMode_.store(std::clamp(std::stoi(val), 0, 2), std::memory_order_relaxed);
                else if (key == "output_device_id") outputDeviceId_.store(std::stoi(val), std::memory_order_relaxed);
                else if (key == "output_device_name") savedOutName = val;
                else if (key == "output_mode") outputMode_.store(std::clamp(std::stoi(val), 0, 1), std::memory_order_relaxed);
                else if (key == "sample_rate") {
                    const int sr = std::stoi(val);
                    if (sr == 44100 || sr == 48000 || sr == 88200 || sr == 96000) {
                        sampleRate_.store(static_cast<uint32_t>(sr), std::memory_order_relaxed);
                    }
                } else if (key == "buffer_size") {
                    const int bs = std::stoi(val);
                    if (bs == 64 || bs == 128 || bs == 256 || bs == 512) {
                        bufferSize_.store(bs, std::memory_order_relaxed);
                    }
                }
            } catch (...) {}
        }

        // Match saved device names if hardware enumeration order shifted
        if (!savedInName.empty()) {
            for (const auto& d : inputDevices_) {
                if (d.name == savedInName) {
                    inputDeviceId_.store(d.deviceId, std::memory_order_relaxed);
                    break;
                }
            }
        }
        if (!savedOutName.empty()) {
            for (const auto& d : outputDevices_) {
                if (d.name == savedOutName) {
                    outputDeviceId_.store(d.deviceId, std::memory_order_relaxed);
                    break;
                }
            }
        }
    }

    PluginType processor_{};
    std::unique_ptr<Editor> editor_{nullptr};

    bool showAudioSettingsModal_{false};
    gui::Point<float> mousePos_{-100.0f, -100.0f};

    std::vector<audio_io::AudioDeviceInfo> inputDevices_;
    std::vector<audio_io::AudioDeviceInfo> outputDevices_;

    std::atomic<int>      driverMode_{0};        // 0 = WASAPI Low-Latency (Shared), 1 = WinMM Wave I/O
    std::atomic<int>      inputDeviceId_{-1};    // -1 = None (Muted / Audition Only), 0..N = waveIn device
    std::atomic<int>      inputChannelMode_{0};  // 0 = MONO (IN 1), 1 = MONO (IN 2), 2 = STEREO (IN 1+2)
    std::atomic<int>      outputDeviceId_{-1};   // -1 = WAVE_MAPPER, 0..N = waveOut device
    std::atomic<int>      outputMode_{1};        // 0 = MONO (PURE), 1 = STEREO (OUT 1+2)
    std::atomic<uint32_t> sampleRate_{48000};    // 44100, 48000, 88200, 96000
    std::atomic<int>      bufferSize_{256};      // 64, 128, 256, 512

    std::atomic<int>      auditionMode_{0};      // 0 = OFF (Live Input), 1 = WAV File
    std::atomic<bool>     muted_{false};
    std::atomic<bool>     bypassed_{false};
    std::atomic<bool>     testToneActive_{false};
    double                testTonePhase_{0.0};

    uint64_t sampleCounter_{0};
    uint32_t rng_{0x12345678u};
    float pink0_{0.0f}, pink1_{0.0f};
    std::atomic<float> inPeakDb_{-90.0f};
    std::atomic<float> outPeakDb_{-90.0f};
    audio_io::WavLoopPlayer wavLoopPlayer_{};
    audio_io::RealtimeWaveOutPlayer audioEngine_{};
};

} // namespace avra::standalone_bridge

#if defined(_WIN32)
#define AVRA_EXPORT_STANDALONE(PluginClass, WindowTitleWide) \
    int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { \
        avra::standalone_bridge::StandaloneHostShell<PluginClass> hostShell; \
        avra::gui::Window window(&hostShell); \
        if (!window.createStandalone(WindowTitleWide, hostShell.getWindowWidth(), hostShell.getWindowHeight())) { \
            return 1; \
        } \
        window.show(); \
        return window.runMessageLoop(); \
    } \
    int main() { \
        return WinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOW); \
    }
#else
#define AVRA_EXPORT_STANDALONE(PluginClass, WindowTitleWide) \
    int main() { return 0; }
#endif
