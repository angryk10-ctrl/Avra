#pragma once

#include "avra/gui/Component.h"
#include "avra/dsp/Math.h"
#include <algorithm>
#include <format>

namespace avra::gui {

/**
 * @brief Real-time stereo Peak + RMS + Peak-Hold studio meter with dB scale markings.
 */
class Meter : public Component {
public:
    Meter() = default;

    void setLevels(float leftPeak, float rightPeak, float leftRms = -1.0f, float rightRms = -1.0f) noexcept {
        leftLevel_ = std::max(leftLevel_ * decayFactor_, leftPeak);
        rightLevel_ = std::max(rightLevel_ * decayFactor_, rightPeak);

        const float effRmsL = (leftRms >= 0.0f) ? leftRms : (leftPeak * 0.707f);
        const float effRmsR = (rightRms >= 0.0f) ? rightRms : (rightPeak * 0.707f);
        leftRms_ = std::max(leftRms_ * 0.92f, effRmsL);
        rightRms_ = std::max(rightRms_ * 0.92f, effRmsR);

        if (leftPeak >= leftHold_) {
            leftHold_ = leftPeak;
            leftHoldTimer_ = 45;
        } else if (leftHoldTimer_ > 0) {
            --leftHoldTimer_;
        } else {
            leftHold_ *= 0.96f;
        }

        if (rightPeak >= rightHold_) {
            rightHold_ = rightPeak;
            rightHoldTimer_ = 45;
        } else if (rightHoldTimer_ > 0) {
            --rightHoldTimer_;
        } else {
            rightHold_ *= 0.96f;
        }

        repaint();
    }

    void paint(Graphics& g) override {
        const Rect<float> r = getLocalBounds();
        g.fillRoundedRect(r, 4.0f, Color::fromHex(0x16181E));
        g.drawRoundedRect(r, 4.0f, Color::Outline(), 1.0f);

        const float pad = 6.0f;
        const float scaleW = (r.width >= 58.0f) ? 20.0f : 0.0f;
        const float availW = r.width - pad * 3.0f - scaleW;
        const float meterW = std::max(6.0f, availW * 0.5f);
        const float meterH = r.height - pad * 2.0f;

        drawChannel(g, { pad, pad, meterW, meterH }, leftLevel_, leftRms_, leftHold_);
        drawChannel(g, { pad * 2.0f + meterW, pad, meterW, meterH }, rightLevel_, rightRms_, rightHold_);

        if (scaleW > 0.0f) {
            const float scaleX = pad * 2.5f + meterW * 2.0f;
            constexpr float kMarks[] = { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f };
            for (float db : kMarks) {
                const float norm = dbToNorm(db);
                const float y = pad + meterH * (1.0f - norm);
                g.drawLine({ scaleX - 3.0f, y }, { scaleX, y }, Color::Outline(), 1.0f);
                g.drawText(std::format("{:.0f}", std::abs(db)), { scaleX + 1.0f, y - 6.0f, scaleW, 12.0f }, Color::TextSecondary(), 8.5f, false);
            }
        }
    }

private:
    static float dbToNorm(float db) noexcept {
        return math::clamp((db + 60.0f) / 66.0f, 0.0f, 1.0f);
    }

    void drawChannel(Graphics& g, Rect<float> r, float peak, float rms, float hold) {
        g.fillRoundedRect(r, 2.0f, Color::fromHex(0x0E1014));

        const float peakDb = math::linearTodB(peak);
        const float rmsDb  = math::linearTodB(rms);
        const float holdDb = math::linearTodB(hold);

        const float peakNorm = dbToNorm(peakDb);
        const float rmsNorm  = dbToNorm(rmsDb);
        const float holdNorm = dbToNorm(holdDb);

        // Peak bar
        const float fillH = r.height * peakNorm;
        if (fillH > 1.0f) {
            Rect<float> fillRect = { r.x, r.bottom() - fillH, r.width, fillH };
            Color topCol = (peakDb >= 0.0f) ? Color::AccentRed() : (peakDb >= -6.0f ? Color::AccentAmber() : Color::AccentCyan());
            Color botCol = Color::fromHex(0x0077B6);
            g.fillLinearGradientRect(fillRect, topCol, botCol, 2.0f);
        }

        // Brighter inner RMS core
        const float rmsH = r.height * rmsNorm;
        if (rmsH > 1.0f) {
            Rect<float> rmsRect = { r.x + 1.5f, r.bottom() - rmsH, std::max(1.0f, r.width - 3.0f), rmsH };
            g.fillRect(rmsRect, Color::AccentCyan().withAlpha(0.45f));
        }

        // Peak-hold horizontal tick
        if (holdNorm > 0.02f) {
            const float holdY = r.bottom() - r.height * holdNorm;
            Color holdCol = (holdDb >= 0.0f) ? Color::AccentRed() : Color::TextPrimary();
            g.drawLine({ r.x, holdY }, { r.right(), holdY }, holdCol, 1.5f);
        }

        // 0 dBFS clip LED at top
        if (peakDb >= -0.05f) {
            g.fillRect({ r.x, r.y, r.width, 3.5f }, Color::AccentRed());
        }
    }

    float leftLevel_{0.0f};
    float rightLevel_{0.0f};
    float leftRms_{0.0f};
    float rightRms_{0.0f};
    float leftHold_{0.0f};
    float rightHold_{0.0f};
    int leftHoldTimer_{0};
    int rightHoldTimer_{0};
    float decayFactor_{0.86f};
};

} // namespace avra::gui
