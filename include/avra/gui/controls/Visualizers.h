#pragma once

#include "avra/gui/Component.h"
#include "avra/core/RingBuffer.h"
#include "avra/dsp/BiquadFilter.h"
#include "avra/dsp/Math.h"
#include <array>
#include <vector>
#include <cmath>
#include <functional>
#include <algorithm>
#include <format>

namespace avra::gui {

/**
 * @brief Hardware-Accelerated Real-Time FFT Spectrum Analyzer (20 Hz - 20 kHz).
 * Features a self-contained 512-point radix-2 Cooley-Tukey FFT, Hann window,
 * logarithmic frequency mapping, smooth ballistics, and peak-hold crest trace.
 */
class SpectrumAnalyzer : public Component {
public:
    static constexpr size_t kFftSize = 512;
    static constexpr size_t kNumBins = kFftSize / 2;
    static constexpr size_t kDisplayPoints = 64;

    SpectrumAnalyzer() noexcept {
        smoothedDb_.fill(-72.0f);
        smoothedInDb_.fill(-72.0f);
        smoothedProbeDb_.fill(-72.0f);
        peakHoldDb_.fill(-72.0f);
        harmonicBinsDb_.fill(-72.0f);
        for (size_t i = 0; i < kFftSize; ++i) {
            hannWindow_[i] = 0.5f * (1.0f - std::cos(kTwoPi * static_cast<float>(i) / static_cast<float>(kFftSize - 1)));
        }
    }

    void setSampleRate(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
    }

    void setAccentColor(Color color) noexcept {
        accentColor_ = color;
    }

    void setTitle(std::string title) {
        title_ = std::move(title);
    }

    void setHarmonicAnalyzerMode(bool enabled) noexcept {
        harmonicAnalyzerMode_ = enabled;
    }

    [[nodiscard]] bool isHarmonicAnalyzerMode() const noexcept { return harmonicAnalyzerMode_; }
    [[nodiscard]] float getThdPercent() const noexcept { return thdPercent_; }
    [[nodiscard]] float getEvenVsOddDb() const noexcept { return evenVsOddDb_; }

    /// Update Pre-Plugin Input (IN) reference trace from a time-domain buffer
    void updateInputFromSamples(std::span<const float> samples) noexcept {
        if (samples.size() < kFftSize) return;

        std::array<float, kFftSize> real{};
        std::array<float, kFftSize> imag{};

        const size_t offset = samples.size() - kFftSize;
        for (size_t i = 0; i < kFftSize; ++i) {
            real[i] = samples[offset + i] * hannWindow_[i];
            imag[i] = 0.0f;
        }

        computeRadix2Fft(real, imag);

        const float binHz = static_cast<float>(sampleRate_) / static_cast<float>(kFftSize);
        for (size_t p = 0; p < kDisplayPoints; ++p) {
            const float normX = static_cast<float>(p) / static_cast<float>(kDisplayPoints - 1);
            const float freq = math::normalizedToLogFreq(normX, 25.0f, 18000.0f);
            const float fBin = math::clamp(freq / binHz, 1.0f, static_cast<float>(kNumBins - 2));

            const size_t b0 = static_cast<size_t>(fBin);
            const size_t b1 = b0 + 1;
            const float frac = fBin - static_cast<float>(b0);

            const float mag0 = std::sqrt(real[b0] * real[b0] + imag[b0] * imag[b0]) / (static_cast<float>(kFftSize) * 0.25f);
            const float mag1 = std::sqrt(real[b1] * real[b1] + imag[b1] * imag[b1]) / (static_cast<float>(kFftSize) * 0.25f);
            const float mag = math::lerp(mag0, mag1, frac);

            const float tiltDb = harmonicAnalyzerMode_ ? 0.0f : (3.0f * std::log2(std::max(1.0f, freq / 250.0f)));
            const float db = math::clamp(math::linearTodB(mag) + tiltDb, -72.0f, 6.0f);

            if (db > smoothedInDb_[p]) {
                smoothedInDb_[p] = 0.45f * smoothedInDb_[p] + 0.55f * db;
            } else {
                smoothedInDb_[p] = 0.86f * smoothedInDb_[p] + 0.14f * db;
            }
        }
        hasInputTrace_ = true;
    }

    template <size_t Cap>
    void updateInputFromRingBuffer(const LockFreeRingBuffer<float, Cap>& ring) noexcept {
        std::array<float, kFftSize> temp{};
        ring.copyLatest(temp);
        updateInputFromSamples(temp);
    }

    void clearProbeTrace() noexcept {
        hasProbeTrace_ = false;
    }

    void updateProbeFromSamples(std::span<const float> samples) noexcept {
        if (samples.size() < kFftSize) return;
        std::array<float, kFftSize> real{};
        std::array<float, kFftSize> imag{};
        const size_t offset = samples.size() - kFftSize;
        for (size_t i = 0; i < kFftSize; ++i) {
            real[i] = samples[offset + i] * hannWindow_[i];
            imag[i] = 0.0f;
        }
        computeRadix2Fft(real, imag);
        const float binHz = static_cast<float>(sampleRate_) / static_cast<float>(kFftSize);
        for (size_t p = 0; p < kDisplayPoints; ++p) {
            const float normX = static_cast<float>(p) / static_cast<float>(kDisplayPoints - 1);
            const float freq = math::normalizedToLogFreq(normX, 25.0f, 18000.0f);
            const float fBin = math::clamp(freq / binHz, 1.0f, static_cast<float>(kNumBins - 2));
            const size_t b0 = static_cast<size_t>(fBin);
            const size_t b1 = b0 + 1;
            const float frac = fBin - static_cast<float>(b0);
            const float mag0 = std::sqrt(real[b0] * real[b0] + imag[b0] * imag[b0]) / (static_cast<float>(kFftSize) * 0.25f);
            const float mag1 = std::sqrt(real[b1] * real[b1] + imag[b1] * imag[b1]) / (static_cast<float>(kFftSize) * 0.25f);
            const float mag = math::lerp(mag0, mag1, frac);
            const float tiltDb = harmonicAnalyzerMode_ ? 0.0f : (3.0f * std::log2(std::max(1.0f, freq / 250.0f)));
            const float db = math::clamp(math::linearTodB(mag) + tiltDb, -72.0f, 6.0f);
            smoothedProbeDb_[p] = 0.72f * smoothedProbeDb_[p] + 0.28f * db;
        }
        hasProbeTrace_ = true;
    }

    template <size_t Cap>
    void updateProbeFromRingBuffer(const LockFreeRingBuffer<float, Cap>& ring) noexcept {
        std::array<float, kFftSize> temp{};
        ring.copyLatest(temp);
        updateProbeFromSamples(temp);
    }

    /// Update FFT from a time-domain buffer of at least kFftSize samples
    void updateFromSamples(std::span<const float> samples) noexcept {
        if (samples.size() < kFftSize) return;

        std::array<float, kFftSize> real{};
        std::array<float, kFftSize> imag{};

        const size_t offset = samples.size() - kFftSize;
        for (size_t i = 0; i < kFftSize; ++i) {
            real[i] = samples[offset + i] * hannWindow_[i];
            imag[i] = 0.0f;
        }

        computeRadix2Fft(real, imag);

        const float binHz = static_cast<float>(sampleRate_) / static_cast<float>(kFftSize);

        // Compute exact harmonic magnitudes for Plugin Doctor THD Analyzer (1 kHz fundamental + 2k..7k harmonics)
        auto sampleMagAtFreq = [&](float targetHz) noexcept -> float {
            const float cBin = math::clamp(targetHz / binHz, 1.0f, static_cast<float>(kNumBins - 3));
            const size_t center = static_cast<size_t>(std::round(cBin));
            float peakMag = 0.0f;
            for (size_t b = (center > 1 ? center - 1 : 1); b <= std::min(kNumBins - 2, center + 1); ++b) {
                const float m = std::sqrt(real[b] * real[b] + imag[b] * imag[b]) / (static_cast<float>(kFftSize) * 0.25f);
                if (m > peakMag) peakMag = m;
            }
            return peakMag;
        };

        const float mag1k = std::max(1e-5f, sampleMagAtFreq(1000.0f));
        float evenPower = 0.0f;
        float oddPower  = 0.0f;
        for (size_t h = 1; h <= 7; ++h) {
            const float m = sampleMagAtFreq(static_cast<float>(h) * 1000.0f);
            const float mDb = math::clamp(math::linearTodB(m), -72.0f, 6.0f);
            harmonicBinsDb_[h - 1] = 0.65f * harmonicBinsDb_[h - 1] + 0.35f * mDb;
            if (h >= 2) {
                if ((h % 2) == 0) evenPower += m * m;
                else              oddPower  += m * m;
            }
        }
        const float rawThd = 100.0f * std::sqrt(evenPower + oddPower) / mag1k;
        thdPercent_  = 0.75f * thdPercent_ + 0.25f * math::clamp(rawThd, 0.0f, 99.9f);
        const float rawEoDb = 10.0f * std::log10(std::max(1e-9f, evenPower) / std::max(1e-9f, oddPower));
        evenVsOddDb_ = 0.75f * evenVsOddDb_ + 0.25f * math::clamp(rawEoDb, -36.0f, 36.0f);

        // Map linear FFT bins onto logarithmic 20 Hz .. 20 kHz display curve
        for (size_t p = 0; p < kDisplayPoints; ++p) {
            const float normX = static_cast<float>(p) / static_cast<float>(kDisplayPoints - 1);
            const float freq = math::normalizedToLogFreq(normX, 25.0f, 18000.0f);
            const float fBin = math::clamp(freq / binHz, 1.0f, static_cast<float>(kNumBins - 2));

            const size_t b0 = static_cast<size_t>(fBin);
            const size_t b1 = b0 + 1;
            const float frac = fBin - static_cast<float>(b0);

            const float mag0 = std::sqrt(real[b0] * real[b0] + imag[b0] * imag[b0]) / (static_cast<float>(kFftSize) * 0.25f);
            const float mag1 = std::sqrt(real[b1] * real[b1] + imag[b1] * imag[b1]) / (static_cast<float>(kFftSize) * 0.25f);
            const float mag = math::lerp(mag0, mag1, frac);

            // Slight +3 dB/oct visual tilt for musical pink-noise balance (flat in THD Harmonic Analyzer mode)
            const float tiltDb = harmonicAnalyzerMode_ ? 0.0f : (3.0f * std::log2( std::max(1.0f, freq / 250.0f) ));
            const float db = math::clamp(math::linearTodB(mag) + tiltDb, -72.0f, 6.0f);

            if (db > smoothedDb_[p]) {
                smoothedDb_[p] = 0.45f * smoothedDb_[p] + 0.55f * db;
            } else {
                smoothedDb_[p] = 0.86f * smoothedDb_[p] + 0.14f * db;
            }

            if (smoothedDb_[p] >= peakHoldDb_[p]) {
                peakHoldDb_[p] = smoothedDb_[p];
            } else {
                peakHoldDb_[p] = std::max(-72.0f, peakHoldDb_[p] - 0.35f);
            }
        }
    }

    template <size_t Cap>
    void updateFromRingBuffer(const LockFreeRingBuffer<float, Cap>& ring) noexcept {
        std::array<float, kFftSize> temp{};
        ring.copyLatest(temp);
        updateFromSamples(temp);
    }

    void paint(Graphics& g) override {
        const Rect<float> r = getLocalBounds();
        g.fillRoundedRect(r, 4.0f, Color::fromHex(0x0E1015));

        const float padX = 8.0f;
        const float padTop = 20.0f;
        const float padBot = 16.0f;
        const float plotW = std::max(10.0f, r.width - padX * 2.0f);
        const float plotH = std::max(10.0f, r.height - padTop - padBot);

        // Frequency grid lines (100 Hz, 1 kHz, 10 kHz)
        constexpr float kGridFreqs[] = { 100.0f, 1000.0f, 10000.0f };
        const char* kGridLabels[] = { "100", "1k", "10k" };
        for (int i = 0; i < 3; ++i) {
            const float nx = math::logFreqToNormalized(kGridFreqs[i], 25.0f, 18000.0f);
            const float x = padX + nx * plotW;
            g.drawLine({ x, padTop }, { x, padTop + plotH }, Color::fromHex(0x1D212B), 1.0f);
            g.drawText(kGridLabels[i], { x - 16.0f, r.height - 15.0f, 32.0f, 14.0f }, Color::TextSecondary().withAlpha(0.7f), 9.0f, true);
        }

        // Horizontal dB grid lines (-12, -36, -60 dB)
        for (float db : { -12.0f, -36.0f, -60.0f }) {
            const float ny = 1.0f - ((db + 72.0f) / 78.0f);
            const float y = padTop + ny * plotH;
            g.drawLine({ padX, y }, { padX + plotW, y }, Color::fromHex(0x1D212B), 1.0f);
        }

        // 1. Pre-Plugin Input (IN) Translucent Grey Reference Shadow Overlay (Phase 1.5)
        if (hasInputTrace_) {
            std::array<Point<float>, kDisplayPoints> inLinePts{};
            std::array<Point<float>, kDisplayPoints + 2> inPolyPts{};
            inPolyPts[0] = { padX, padTop + plotH };
            for (size_t i = 0; i < kDisplayPoints; ++i) {
                const float nx = static_cast<float>(i) / static_cast<float>(kDisplayPoints - 1);
                const float ny = 1.0f - math::clamp((smoothedInDb_[i] + 72.0f) / 78.0f, 0.0f, 1.0f);
                const Point<float> pt{ padX + nx * plotW, padTop + ny * plotH };
                inLinePts[i] = pt;
                inPolyPts[i + 1] = pt;
            }
            inPolyPts[kDisplayPoints + 1] = { padX + plotW, padTop + plotH };
            g.fillPolygon(inPolyPts, Color::fromHex(0x8894A4, 0.14f));
            g.drawPolyline(inLinePts, Color::fromHex(0x9BA8B8, 0.48f), 1.2f);
        }

        // 2. Post-Plugin Output (OUT) Bright Trace & Peak-Hold Crest
        std::array<Point<float>, kDisplayPoints> linePts{};
        std::array<Point<float>, kDisplayPoints> peakPts{};
        std::array<Point<float>, kDisplayPoints + 2> polyPts{};

        polyPts[0] = { padX, padTop + plotH };
        for (size_t i = 0; i < kDisplayPoints; ++i) {
            const float nx = static_cast<float>(i) / static_cast<float>(kDisplayPoints - 1);
            const float ny = 1.0f - math::clamp((smoothedDb_[i] + 72.0f) / 78.0f, 0.0f, 1.0f);
            const float py = 1.0f - math::clamp((peakHoldDb_[i] + 72.0f) / 78.0f, 0.0f, 1.0f);

            const Point<float> pt{ padX + nx * plotW, padTop + ny * plotH };
            linePts[i] = pt;
            polyPts[i + 1] = pt;
            peakPts[i] = { padX + nx * plotW, padTop + py * plotH };
        }
        polyPts[kDisplayPoints + 1] = { padX + plotW, padTop + plotH };

        g.fillPolygon(polyPts, accentColor_.withAlpha(0.18f));
        g.drawPolyline(peakPts, accentColor_.withAlpha(0.35f), 1.0f);
        g.drawPolyline(linePts, accentColor_, 1.8f);

        // 2b. Inter-Stage Schematic Probe Overlay Trace (Gold/Amber)
        if (hasProbeTrace_) {
            std::array<Point<float>, kDisplayPoints> probePts{};
            for (size_t i = 0; i < kDisplayPoints; ++i) {
                const float nx = static_cast<float>(i) / static_cast<float>(kDisplayPoints - 1);
                const float ny = 1.0f - math::clamp((smoothedProbeDb_[i] + 72.0f) / 78.0f, 0.0f, 1.0f);
                probePts[i] = { padX + nx * plotW, padTop + ny * plotH };
            }
            g.drawPolyline(probePts, Color::fromHex(0xE8C547, 0.90f), 1.6f);
        }

        // 3. "Plugin Doctor" THD Harmonic Markers (Phase 1.4: Even 2k/4k/6k in Gold/Cyan, Odd 3k/5k/7k in Orange/Red)
        if (harmonicAnalyzerMode_) {
            static constexpr float kHarmFreqs[7] = { 1000.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f, 6000.0f, 7000.0f };
            static constexpr const char* kHarmLbls[7] = { "1k", "2k", "3k", "4k", "5k", "6k", "7k" };
            for (size_t h = 0; h < 7; ++h) {
                const float nx = math::logFreqToNormalized(kHarmFreqs[h], 25.0f, 18000.0f);
                const float hx = padX + nx * plotW;
                const float hDb = harmonicBinsDb_[h];
                const float hy = padTop + (1.0f - math::clamp((hDb + 72.0f) / 78.0f, 0.0f, 1.0f)) * plotH;

                const bool isFund = (h == 0);
                const bool isEven = ((h + 1) % 2 == 0);
                const Color pinCol = isFund
                    ? Color::fromHex(0xF0F6FA)
                    : (isEven ? Color::fromHex(0xE8C547) : Color::fromHex(0xFF4D5A));

                g.drawLine({ hx, padTop + plotH }, { hx, hy }, pinCol.withAlpha(0.65f), isFund ? 1.6f : 1.3f);
                g.fillCircle({ hx, hy }, isFund ? 3.2f : 2.8f, pinCol);
                g.drawText(kHarmLbls[h], { hx - 10.0f, std::max(padTop, hy - 13.0f), 20.0f, 11.0f }, pinCol, 7.8f, true, true);
            }

            // Live THD Badge in top-right corner
            const char* eoTag = (evenVsOddDb_ > 1.5f) ? "EVEN/TUBE" : ((evenVsOddDb_ < -1.5f) ? "ODD/TAPE" : "BALANCED");
            const std::string thdBadgeText = std::format("THD: {:.2f}% ({} {:+.1f}dB)", thdPercent_, eoTag, evenVsOddDb_);
            Rect<float> badgeR = { r.width - 178.0f, 2.0f, 172.0f, 16.0f };
            g.fillRoundedRect(badgeR, 3.0f, Color::fromHex(0x18202C, 0.92f));
            g.drawRoundedRect(badgeR, 3.0f, Color::fromHex(0xE8C547, 0.75f), 1.0f);
            g.drawText(thdBadgeText, badgeR, Color::fromHex(0xE8C547), 8.2f, true, true);
        } else if (hasProbeTrace_) {
            g.drawText("IN (GREY) / PROBE (GOLD) / OUT (CYAN)", { r.width - 178.0f, 3.0f, 172.0f, 15.0f },
                       Color::fromHex(0xE8C547), 8.0f, true, true);
        } else if (hasInputTrace_) {
            g.drawText("IN (GREY) / OUT (CYAN)", { r.width - 126.0f, 3.0f, 120.0f, 15.0f },
                       Color::fromHex(0x9BA8B8), 8.0f, true, true);
        }

        // Title header inside analyzer
        g.drawText(title_, { 10.0f, 3.0f, r.width - 135.0f, 16.0f }, Color::TextSecondary(), 9.2f, false, true);
        g.drawRoundedRect(r, 4.0f, Color::Outline(), 1.0f);
    }

private:
    static void computeRadix2Fft(std::array<float, kFftSize>& real, std::array<float, kFftSize>& imag) noexcept {
        // Bit-reversal permutation
        size_t j = 0;
        for (size_t i = 0; i < kFftSize; ++i) {
            if (i < j) {
                std::swap(real[i], real[j]);
                std::swap(imag[i], imag[j]);
            }
            size_t m = kFftSize >> 1;
            while (m >= 1 && j >= m) {
                j -= m;
                m >>= 1;
            }
            j += m;
        }

        // Butterfly stages
        for (size_t s = 1; (1ULL << s) <= kFftSize; ++s) {
            const size_t m = 1ULL << s;
            const size_t halfM = m >> 1;
            const float theta = -kTwoPi / static_cast<float>(m);
            const float wStepR = std::cos(theta);
            const float wStepI = std::sin(theta);

            for (size_t k = 0; k < kFftSize; k += m) {
                float wR = 1.0f;
                float wI = 0.0f;
                for (size_t idx = 0; idx < halfM; ++idx) {
                    const size_t i1 = k + idx;
                    const size_t i2 = i1 + halfM;
                    const float tR = wR * real[i2] - wI * imag[i2];
                    const float tI = wR * imag[i2] + wI * real[i2];

                    real[i2] = real[i1] - tR;
                    imag[i2] = imag[i1] - tI;
                    real[i1] += tR;
                    imag[i1] += tI;

                    const float nextWR = wR * wStepR - wI * wStepI;
                    wI = wR * wStepI + wI * wStepR;
                    wR = nextWR;
                }
            }
        }
    }

    double sampleRate_{48000.0};
    Color accentColor_{Color::AccentCyan()};
    std::string title_{"REAL-TIME FFT SPECTRUM (20 Hz - 20 kHz)"};
    std::array<float, kFftSize> hannWindow_{};
    std::array<float, kDisplayPoints> smoothedDb_{};
    std::array<float, kDisplayPoints> smoothedInDb_{};
    std::array<float, kDisplayPoints> smoothedProbeDb_{};
    std::array<float, kDisplayPoints> peakHoldDb_{};
    std::array<float, 8> harmonicBinsDb_{};
    bool harmonicAnalyzerMode_{false};
    bool hasInputTrace_{false};
    bool hasProbeTrace_{false};
    float thdPercent_{0.0f};
    float evenVsOddDb_{0.0f};
};

/**
 * @brief Triggered Real-Time Oscilloscope, Waveshaper Transfer Knee & I/O Phase Scope.
 * Always renders Pre-Plugin Input (IN) as a translucent grey reference shadow behind Post-Plugin Output (OUT),
 * and includes interactive [WAVE] / [KNEE] / [PHASE] mode tabs.
 */
class WaveformScope : public Component {
public:
    static constexpr size_t kScopeSamples = 256;

    enum class ScopeMode {
        Waveform = 0,
        TransferKnee = 1,
        PhaseScope = 2
    };

    WaveformScope() noexcept {
        trace_.fill(0.0f);
        inTrace_.fill(0.0f);
        probeTrace_.fill(0.0f);
    }

    void setAccentColor(Color color) noexcept { accentColor_ = color; }
    void setTitle(std::string title) { title_ = std::move(title); }
    void setScopeMode(ScopeMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] ScopeMode getScopeMode() const noexcept { return mode_; }

    void updateFromSamples(std::span<const float> samples) noexcept {
        if (samples.size() < kScopeSamples) return;

        // Rising zero-crossing trigger search in the first half
        size_t triggerIdx = 0;
        const size_t maxSearch = samples.size() - kScopeSamples;
        for (size_t i = 1; i < maxSearch; ++i) {
            if (samples[i - 1] <= 0.0f && samples[i] > 0.0f) {
                triggerIdx = i;
                break;
            }
        }

        for (size_t i = 0; i < kScopeSamples; ++i) {
            trace_[i] = math::clamp(samples[triggerIdx + i], -1.0f, 1.0f);
        }
    }

    void updateFromSamples(std::span<const float> inSamples, std::span<const float> outSamples) noexcept {
        if (outSamples.size() < kScopeSamples) return;

        // Trigger on input if available and non-silent, otherwise output
        const bool useInTrigger = (inSamples.size() >= outSamples.size());
        const auto& trigSrc = useInTrigger ? inSamples : outSamples;

        size_t triggerIdx = 0;
        const size_t maxSearch = trigSrc.size() - kScopeSamples;
        for (size_t i = 1; i < maxSearch; ++i) {
            if (trigSrc[i - 1] <= 0.0f && trigSrc[i] > 0.0f) {
                triggerIdx = i;
                break;
            }
        }

        hasInputTrace_ = (inSamples.size() >= triggerIdx + kScopeSamples);
        for (size_t i = 0; i < kScopeSamples; ++i) {
            trace_[i] = math::clamp(outSamples[triggerIdx + i], -1.0f, 1.0f);
            if (hasInputTrace_) {
                inTrace_[i] = math::clamp(inSamples[triggerIdx + i], -1.0f, 1.0f);
            }
        }
    }

    template <size_t Cap>
    void updateFromRingBuffer(const LockFreeRingBuffer<float, Cap>& ring) noexcept {
        std::array<float, 512> temp{};
        ring.copyLatest(temp);
        updateFromSamples(temp);
    }

    template <size_t Cap>
    void updateFromRingBuffers(const LockFreeRingBuffer<float, Cap>& inRing,
                               const LockFreeRingBuffer<float, Cap>& outRing) noexcept {
        std::array<float, 512> inTemp{};
        std::array<float, 512> outTemp{};
        inRing.copyLatest(inTemp);
        outRing.copyLatest(outTemp);
        updateFromSamples(inTemp, outTemp);
    }

    void clearProbeTrace() noexcept {
        hasProbeTrace_ = false;
    }

    void updateProbeFromSamples(std::span<const float> probeSamples) noexcept {
        if (probeSamples.size() < kScopeSamples) return;
        size_t triggerIdx = 0;
        const size_t maxSearch = probeSamples.size() - kScopeSamples;
        for (size_t i = 1; i < maxSearch; ++i) {
            if (probeSamples[i - 1] <= 0.0f && probeSamples[i] > 0.0f) {
                triggerIdx = i;
                break;
            }
        }
        for (size_t i = 0; i < kScopeSamples; ++i) {
            probeTrace_[i] = math::clamp(probeSamples[triggerIdx + i], -1.0f, 1.0f);
        }
        hasProbeTrace_ = true;
    }

    template <size_t Cap>
    void updateProbeFromRingBuffer(const LockFreeRingBuffer<float, Cap>& probeRing) noexcept {
        std::array<float, 512> probeTemp{};
        probeRing.copyLatest(probeTemp);
        updateProbeFromSamples(probeTemp);
    }

    void onMouseDown(const MouseEvent& e) override {
        const Rect<float> r = getLocalBounds();
        // Check [WAVE] [KNEE] [PHASE] mode pills in top-right header
        const float pillW = 38.0f;
        const float pillH = 13.0f;
        const float pillY = 3.5f;
        const float rightEdge = r.width - 6.0f;
        const Rect<float> phaseRect{ rightEdge - pillW, pillY, pillW, pillH };
        const Rect<float> kneeRect{ rightEdge - pillW * 2.0f - 3.0f, pillY, pillW, pillH };
        const Rect<float> waveRect{ rightEdge - pillW * 3.0f - 6.0f, pillY, pillW, pillH };

        if (waveRect.contains(e.pos)) {
            mode_ = ScopeMode::Waveform;
        } else if (kneeRect.contains(e.pos)) {
            mode_ = ScopeMode::TransferKnee;
        } else if (phaseRect.contains(e.pos)) {
            mode_ = ScopeMode::PhaseScope;
        }
    }

    void paint(Graphics& g) override {
        const Rect<float> r = getLocalBounds();
        g.fillRoundedRect(r, 4.0f, Color::fromHex(0x0E1015));

        const float padX = 8.0f;
        const float padTop = 20.0f;
        const float padBot = 8.0f;
        const float plotW = std::max(10.0f, r.width - padX * 2.0f);
        const float plotH = std::max(10.0f, r.height - padTop - padBot);
        const float midY = padTop + plotH * 0.5f;

        if (mode_ == ScopeMode::Waveform) {
            // Zero center line
            g.drawLine({ padX, midY }, { padX + plotW, midY }, Color::fromHex(0x222733), 1.0f);

            // 1. Pre-Plugin Input (IN) translucent grey reference shadow
            if (hasInputTrace_) {
                std::array<Point<float>, kScopeSamples> inPts{};
                for (size_t i = 0; i < kScopeSamples; ++i) {
                    const float nx = static_cast<float>(i) / static_cast<float>(kScopeSamples - 1);
                    const float x = padX + nx * plotW;
                    const float y = midY - inTrace_[i] * (plotH * 0.45f);
                    inPts[i] = { x, y };
                }
                g.drawPolyline(inPts, Color::fromHex(0x9BA7B8).withAlpha(0.45f), 1.4f);
            }

            // 2. Post-Plugin Output (OUT) bright trace
            std::array<Point<float>, kScopeSamples> pts{};
            for (size_t i = 0; i < kScopeSamples; ++i) {
                const float nx = static_cast<float>(i) / static_cast<float>(kScopeSamples - 1);
                const float x = padX + nx * plotW;
                const float y = midY - trace_[i] * (plotH * 0.45f);
                pts[i] = { x, y };
            }
            g.drawPolyline(pts, accentColor_, 1.6f);

            // 2b. Inter-Stage Schematic Probe Overlay Trace (Gold/Amber)
            if (hasProbeTrace_) {
                std::array<Point<float>, kScopeSamples> probePts{};
                for (size_t i = 0; i < kScopeSamples; ++i) {
                    const float nx = static_cast<float>(i) / static_cast<float>(kScopeSamples - 1);
                    const float x = padX + nx * plotW;
                    const float y = midY - probeTrace_[i] * (plotH * 0.45f);
                    probePts[i] = { x, y };
                }
                g.drawPolyline(probePts, Color::fromHex(0xE8C547, 0.90f), 1.4f);
            }
        } else if (mode_ == ScopeMode::TransferKnee) {
            // X/Y Waveshaper Transfer Curve (Input X vs Output Y)
            const float boxSize = std::min(plotW * 0.85f, plotH * 0.92f);
            const float cx = padX + plotW * 0.5f;
            const float cy = padTop + plotH * 0.5f;
            const float halfBox = boxSize * 0.5f;
            const Rect<float> plotBox{ cx - halfBox, cy - halfBox, boxSize, boxSize };

            g.fillRoundedRect(plotBox, 2.0f, Color::fromHex(0x090B0E));
            g.drawRoundedRect(plotBox, 2.0f, Color::fromHex(0x222834), 1.0f);
            // Crosshairs & Unity 1:1 linear diagonal reference
            g.drawLine({ cx - halfBox, cy }, { cx + halfBox, cy }, Color::fromHex(0x1E2430), 1.0f);
            g.drawLine({ cx, cy - halfBox }, { cx, cy + halfBox }, Color::fromHex(0x1E2430), 1.0f);
            g.drawLine({ cx - halfBox, cy + halfBox }, { cx + halfBox, cy - halfBox },
                       Color::fromHex(0x8E9BAE).withAlpha(0.38f), 1.2f);

            // Live Input vs Output transfer trajectory
            std::array<Point<float>, kScopeSamples> xyPts{};
            for (size_t i = 0; i < kScopeSamples; ++i) {
                const float inV = hasInputTrace_ ? inTrace_[i] : trace_[i];
                const float outV = trace_[i];
                xyPts[i] = {
                    cx + math::clamp(inV, -1.0f, 1.0f) * (halfBox * 0.92f),
                    cy - math::clamp(outV, -1.0f, 1.0f) * (halfBox * 0.92f)
                };
            }
            g.drawPolyline(xyPts, Color::fromHex(0xFFB703), 1.8f);
            g.drawText("IN (X) -> OUT (Y) TRANSFER KNEE", { padX + 4.0f, padTop + plotH - 13.0f, plotW - 8.0f, 12.0f },
                       Color::fromHex(0x8896AA), 8.0f, false, true);
        } else {
            // I/O Phase Response & Lissajous Vector Scope
            const float cx = padX + plotW * 0.5f;
            const float cy = padTop + plotH * 0.5f;
            const float rad = std::min(plotW, plotH) * 0.44f;

            g.drawCircle({ cx, cy }, rad, Color::fromHex(0x222834), 1.0f);
            g.drawCircle({ cx, cy }, rad * 0.5f, Color::fromHex(0x191E28), 1.0f);
            g.drawLine({ cx - rad, cy }, { cx + rad, cy }, Color::fromHex(0x1E2430), 1.0f);
            g.drawLine({ cx, cy - rad }, { cx, cy + rad }, Color::fromHex(0x1E2430), 1.0f);

            // Compute phase correlation between IN and OUT
            float dot = 0.0f;
            float eIn = 1.0e-7f;
            float eOut = 1.0e-7f;
            std::array<Point<float>, kScopeSamples> phasePts{};
            for (size_t i = 0; i < kScopeSamples; ++i) {
                const float inV = hasInputTrace_ ? inTrace_[i] : trace_[i];
                const float outV = trace_[i];
                dot += inV * outV;
                eIn += inV * inV;
                eOut += outV * outV;
                // Rotate 45 deg so in-phase is vertical
                const float mid = (inV + outV) * 0.7071f;
                const float side = (outV - inV) * 0.7071f;
                phasePts[i] = {
                    cx + math::clamp(side, -1.0f, 1.0f) * rad * 0.9f,
                    cy - math::clamp(mid, -1.0f, 1.0f) * rad * 0.9f
                };
            }
            const float corr = math::clamp(dot / std::sqrt(eIn * eOut), -1.0f, 1.0f);
            const float phaseDeg = std::acos(math::clamp(corr, -1.0f, 1.0f)) * (180.0f / 3.14159265f);

            g.drawPolyline(phasePts, Color::fromHex(0x00F5D4), 1.6f);
            g.drawText(std::format("I/O PHASE SHIFT: {:.1f} DEG (CORR {:+.2f})", phaseDeg, corr),
                       { padX + 4.0f, padTop + plotH - 13.0f, plotW - 8.0f, 12.0f },
                       Color::fromHex(0x00F5D4), 8.0f, false, true);
        }

        // Title + [WAVE] [KNEE] [PHASE] mode selector tabs
        const float pillW = 38.0f;
        const float pillH = 13.0f;
        const float pillY = 3.5f;
        const float rightEdge = r.width - 6.0f;
        g.drawText(title_, { 10.0f, 3.0f, r.width - 138.0f, 16.0f }, Color::TextSecondary(), 9.0f, false, true);

        auto drawTab = [&](float x, const char* label, ScopeMode m) {
            const bool active = (mode_ == m);
            const Rect<float> pr{ x, pillY, pillW, pillH };
            g.fillRoundedRect(pr, 2.5f, active ? Color::fromHex(0x1C3347) : Color::fromHex(0x141820));
            g.drawRoundedRect(pr, 2.5f, active ? Color::AccentCyan() : Color::fromHex(0x2B3342), 1.0f);
            g.drawText(label, pr, active ? Color::AccentCyan() : Color::fromHex(0x8894A6), 7.5f, true, true);
        };
        drawTab(rightEdge - pillW * 3.0f - 6.0f, "WAVE", ScopeMode::Waveform);
        drawTab(rightEdge - pillW * 2.0f - 3.0f, "KNEE", ScopeMode::TransferKnee);
        drawTab(rightEdge - pillW, "PHASE", ScopeMode::PhaseScope);

        g.drawRoundedRect(r, 4.0f, Color::Outline(), 1.0f);
    }

private:
    Color accentColor_{Color::fromHex(0x3A86FF)};
    std::string title_{"WAVEFORM / TRANSFER KNEE SCOPE"};
    ScopeMode mode_{ScopeMode::Waveform};
    bool hasInputTrace_{false};
    bool hasProbeTrace_{false};
    std::array<float, kScopeSamples> trace_{};
    std::array<float, kScopeSamples> inTrace_{};
    std::array<float, kScopeSamples> probeTrace_{};
};

/**
 * @brief Live Parametric EQ Frequency Response Curve Display (-18 dB to +18 dB, 20 Hz - 20 kHz).
 */
class EqCurveDisplay : public Component {
public:
    static constexpr size_t kCurvePoints = 72;

    EqCurveDisplay() = default;

    void setEvaluator(std::function<float(float freqHz)> evalFn) {
        evaluator_ = std::move(evalFn);
    }

    void setAccentColor(Color color) noexcept { accentColor_ = color; }

    void paint(Graphics& g) override {
        const Rect<float> r = getLocalBounds();
        g.fillRoundedRect(r, 4.0f, Color::fromHex(0x0E1015));

        const float padX = 8.0f;
        const float padY = 16.0f;
        const float plotW = std::max(10.0f, r.width - padX * 2.0f);
        const float plotH = std::max(10.0f, r.height - padY * 2.0f);
        const float midY = padY + plotH * 0.5f;

        // 0 dB center line and +/-12 dB lines
        g.drawLine({ padX, midY }, { padX + plotW, midY }, Color::fromHex(0x282E3D), 1.0f);
        g.drawLine({ padX, padY + plotH * 0.166f }, { padX + plotW, padY + plotH * 0.166f }, Color::fromHex(0x191D26), 1.0f);
        g.drawLine({ padX, padY + plotH * 0.833f }, { padX + plotW, padY + plotH * 0.833f }, Color::fromHex(0x191D26), 1.0f);

        std::array<Point<float>, kCurvePoints> linePts{};
        std::array<Point<float>, kCurvePoints + 2> polyPts{};
        polyPts[0] = { padX, midY };

        for (size_t i = 0; i < kCurvePoints; ++i) {
            const float nx = static_cast<float>(i) / static_cast<float>(kCurvePoints - 1);
            const float freq = math::normalizedToLogFreq(nx, 20.0f, 20000.0f);
            const float db = evaluator_ ? math::clamp(evaluator_(freq), -18.0f, 18.0f) : 0.0f;
            const float y = midY - (db / 18.0f) * (plotH * 0.5f);
            const Point<float> pt{ padX + nx * plotW, y };
            linePts[i] = pt;
            polyPts[i + 1] = pt;
        }
        polyPts[kCurvePoints + 1] = { padX + plotW, midY };

        g.fillPolygon(polyPts, accentColor_.withAlpha(0.18f));
        g.drawPolyline(linePts, accentColor_, 2.0f);
        g.drawText("EQ FREQUENCY RESPONSE (20 Hz - 20 kHz)", { 10.0f, 2.0f, r.width - 20.0f, 14.0f }, Color::TextSecondary(), 9.0f, false, true);
        g.drawRoundedRect(r, 4.0f, Color::Outline(), 1.0f);
    }

private:
    std::function<float(float)> evaluator_{nullptr};
    Color accentColor_{Color::AccentCyan()};
};

/**
 * @brief Scrolling Real-Time Gain Reduction History Graph (0 dB to -24 dB).
 */
class GainReductionHistory : public Component {
public:
    static constexpr size_t kHistoryPoints = 96;

    GainReductionHistory() noexcept {
        historyDb_.fill(0.0f);
    }

    void pushGainReductionDb(float grDb) noexcept {
        // grDb is positive for attenuation (e.g. 6.0f means 6 dB of GR)
        currentGrDb_ = math::clamp(grDb, 0.0f, 24.0f);
        for (size_t i = 0; i + 1 < kHistoryPoints; ++i) {
            historyDb_[i] = historyDb_[i + 1];
        }
        historyDb_[kHistoryPoints - 1] = currentGrDb_;
    }

    void paint(Graphics& g) override {
        const Rect<float> r = getLocalBounds();
        g.fillRoundedRect(r, 4.0f, Color::fromHex(0x0E1015));

        const float padX = 8.0f;
        const float padTop = 20.0f;
        const float padBot = 8.0f;
        const float plotW = std::max(10.0f, r.width - padX * 2.0f);
        const float plotH = std::max(10.0f, r.height - padTop - padBot);

        std::array<Point<float>, kHistoryPoints> linePts{};
        std::array<Point<float>, kHistoryPoints + 2> polyPts{};
        polyPts[0] = { padX, padTop };

        for (size_t i = 0; i < kHistoryPoints; ++i) {
            const float nx = static_cast<float>(i) / static_cast<float>(kHistoryPoints - 1);
            const float ny = math::clamp(historyDb_[i] / 24.0f, 0.0f, 1.0f);
            const Point<float> pt{ padX + nx * plotW, padTop + ny * plotH };
            linePts[i] = pt;
            polyPts[i + 1] = pt;
        }
        polyPts[kHistoryPoints + 1] = { padX + plotW, padTop };

        g.fillPolygon(polyPts, Color::AccentAmber().withAlpha(0.22f));
        g.drawPolyline(linePts, Color::AccentAmber(), 1.8f);

        g.drawText(std::format("GAIN REDUCTION: -{:.1f} dB", currentGrDb_), { 10.0f, 3.0f, r.width - 20.0f, 16.0f }, Color::AccentAmber(), 9.5f, false, true);
        g.drawRoundedRect(r, 4.0f, Color::Outline(), 1.0f);
    }

private:
    float currentGrDb_{0.0f};
    std::array<float, kHistoryPoints> historyDb_{};
};

} // namespace avra::gui
