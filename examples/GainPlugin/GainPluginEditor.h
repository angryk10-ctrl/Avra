#pragma once

#include "avra/Avra.h"

namespace avra::examples {

/**
 * @brief Hardware-accelerated Direct2D studio rack interface for GainPlugin.
 */
class GainPluginEditor : public Editor {
public:
    explicit GainPluginEditor(GainPlugin& processor)
        : Editor(processor, 520, 320)
        , gainProcessor_(processor)
        , gainKnob_(processor.getParameters().getById(1), gui::Color::AccentCyan())
        , muteButton_("MUTE", processor.getParameters().getById(2)) {

        addChild(&gainKnob_);
        addChild(&muteButton_);
        addChild(&meter_);

        resized();
    }

    void resized() override {
        // Layout:
        // Left: Gain Knob (center)
        // Middle: Mute Toggle Button
        // Right: Stereo Peak/VU Meter
        gainKnob_.setBounds(40.0f, 70.0f, 180.0f, 210.0f);
        muteButton_.setBounds(255.0f, 155.0f, 95.0f, 38.0f);
        meter_.setBounds(395.0f, 65.0f, 65.0f, 220.0f);
    }

    void paint(gui::Graphics& g) override {
        const auto localBounds = getLocalBounds();

        // 1. Dark sleek rack faceplate
        g.fillRect(localBounds, gui::Color::Background());

        // 2. Top Header Bar
        gui::Rect<float> headerBar = { 0.0f, 0.0f, localBounds.width, 45.0f };
        g.fillRect(headerBar, gui::Color::Surface());
        g.drawLine({ 0.0f, 45.0f }, { localBounds.width, 45.0f }, gui::Color::Outline(), 1.0f);

        // Header Title
        g.drawText("AVRA GAIN", { 20.0f, 0.0f, 200.0f, 45.0f }, gui::Color::TextPrimary(), 14.0f, false);
        g.drawText("DIRECT2D 60FPS", { localBounds.width - 150.0f, 0.0f, 130.0f, 45.0f }, gui::Color::AccentCyan(), 10.0f, false);

        meter_.setLevels(gainProcessor_.getPeakL(), gainProcessor_.getPeakR());

        // Outer chassis border
        g.drawRect(localBounds, gui::Color::Outline(), 1.0f);
    }

private:
    GainPlugin& gainProcessor_;
    gui::Knob gainKnob_;
    gui::Button muteButton_;
    gui::Meter meter_;
};

} // namespace avra::examples
