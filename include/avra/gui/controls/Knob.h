#pragma once

#include "avra/gui/Component.h"
#include "avra/core/Parameter.h"
#include <cmath>
#include <format>

namespace avra::gui {

/**
 * @brief High-precision studio rotary knob with bipolar center-detent support,
 * double-click reset to default, hover glow ring, and fine-tuning.
 */
class Knob : public Component {
public:
    explicit Knob(Parameter* param = nullptr, Color accentColor = Color::AccentBlue())
        : param_(param), accentColor_(accentColor) {
        detectBipolar();
    }

    void setParameter(Parameter* param) noexcept {
        param_ = param;
        detectBipolar();
        repaint();
    }

    void setAccentColor(Color color) noexcept {
        accentColor_ = color;
        repaint();
    }

    void setBipolar(bool bipolar) noexcept {
        isBipolar_ = bipolar;
        repaint();
    }

    void paint(Graphics& g) override {
        const float w = bounds_.width;
        const float h = bounds_.height;
        const float radius = std::min(w, h) * 0.30f;
        const Point<float> center = { w * 0.5f, h * 0.40f };

        // Knob Arc geometry: 7 o'clock to 5 o'clock (-140 deg to +140 deg)
        constexpr float kStartAngle = -2.44346f;
        constexpr float kEndAngle   =  2.44346f;
        constexpr float kTotalAngle = kEndAngle - kStartAngle;

        const float norm = param_ ? param_->getNormalized() : 0.5f;
        const float currentAngle = kStartAngle + norm * kTotalAngle;

        // 1. Subtle outer tick marks around skirt
        constexpr int kNumTicks = 11;
        for (int i = 0; i < kNumTicks; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(kNumTicks - 1);
            const float ang = kStartAngle + t * kTotalAngle;
            const float rIn = radius + 7.5f;
            const float rOut = radius + ((i == 0 || i == 5 || i == 10) ? 11.0f : 9.5f);
            const Point<float> t1{ center.x + rIn * std::sin(ang), center.y - rIn * std::cos(ang) };
            const Point<float> t2{ center.x + rOut * std::sin(ang), center.y - rOut * std::cos(ang) };
            g.drawLine(t1, t2, Color::Outline().withAlpha(0.6f), 1.0f);
        }

        // 2. Outer background track arc
        g.drawArc(center, radius + 4.5f, kStartAngle, kEndAngle, Color::fromHex(0x16181E), 4.0f);

        // 3. Active value arc (Bipolar from 12 o'clock or Unipolar from 7 o'clock)
        if (isBipolar_) {
            if (std::abs(currentAngle) > 0.01f) {
                g.drawArc(center, radius + 4.5f, 0.0f, currentAngle, accentColor_, 4.0f);
            }
        } else {
            if (norm > 0.002f) {
                g.drawArc(center, radius + 4.5f, kStartAngle, currentAngle, accentColor_, 4.0f);
            }
        }

        // 4. Hover / Drag Glow Halo
        if (isHovered_ || isDragging_) {
            g.drawCircle(center, radius + 1.5f, accentColor_.withAlpha(isDragging_ ? 0.55f : 0.30f), 2.0f);
        }

        // 5. Knob metallic body shadow + cap
        g.fillCircle({ center.x, center.y + 2.0f }, radius, Color::fromHex(0x0D0E12, 0.8f));
        g.fillCircle(center, radius, Color::fromHex(0x23262E));
        g.fillCircle(center, radius * 0.82f, Color::fromHex(0x1B1E25));
        g.drawCircle(center, radius, isDragging_ ? accentColor_ : Color::Outline(), 1.5f);

        // 6. Pointer indicator notch
        const float pointerInner = radius * 0.32f;
        const float pointerOuter = radius * 0.86f;
        const Point<float> p1 = {
            center.x + pointerInner * std::sin(currentAngle),
            center.y - pointerInner * std::cos(currentAngle)
        };
        const Point<float> p2 = {
            center.x + pointerOuter * std::sin(currentAngle),
            center.y - pointerOuter * std::cos(currentAngle)
        };
        g.drawLine(p1, p2, isDragging_ ? accentColor_ : Color::TextPrimary(), 2.5f);

        // 7. Parameter Name Label & Value Readout Pill
        if (param_) {
            Rect<float> labelBounds = { 0.0f, h * 0.70f, w, 16.0f };
            g.drawText(param_->getName(), labelBounds, Color::TextSecondary(), 10.5f, true, true);

            Rect<float> pillBounds = { w * 0.10f, h * 0.83f, w * 0.80f, 18.0f };
            g.fillRoundedRect(pillBounds, 3.0f, Color::fromHex(0x14161B));
            if (isDragging_ || isHovered_) {
                g.drawRoundedRect(pillBounds, 3.0f, accentColor_.withAlpha(0.45f), 1.0f);
            }
            g.drawText(param_->getCurrentText(), pillBounds, isDragging_ ? accentColor_ : Color::TextPrimary(), 11.0f, true);
        }
    }

    static void setGlobalMouseMode(int mode) noexcept {
        globalMouseMode_ = (mode < 0 || mode > 2) ? 0 : mode;
    }

    [[nodiscard]] static int getGlobalMouseMode() noexcept {
        return globalMouseMode_;
    }

    void onMouseDown(const MouseEvent& e) override {
        isDragging_ = true;
        dragStartX_ = e.pos.x;
        dragStartY_ = e.pos.y;
        dragStartNorm_ = param_ ? param_->getNormalized() : 0.0f;
        repaint();
    }

    void onDoubleClick(const MouseEvent&) override {
        if (param_) {
            param_->setPlain(param_->getDefaultValue());
            isDragging_ = false;
            repaint();
        }
    }

    void onMouseDrag(const MouseEvent& e) override {
        if (!isDragging_ || !param_) return;

        // Sensitivity: 150 pixels vertical drag for full 0..1 range (fine tuning with Shift or Mode 2)
        const bool fineTune = e.isShiftDown || (globalMouseMode_ == 2);
        const float sensitivity = fineTune ? 500.0f : 150.0f;
        const float deltaY = dragStartY_ - e.pos.y;
        const float deltaX = e.pos.x - dragStartX_;
        const float deltaNorm = (globalMouseMode_ == 1)
            ? ((deltaY + deltaX * 0.65f) / sensitivity)
            : (deltaY / sensitivity);

        const float newNorm = math::clamp(dragStartNorm_ + deltaNorm, 0.0f, 1.0f);
        param_->setNormalized(newNorm);
        repaint();
    }

    void onMouseUp(const MouseEvent&) override {
        isDragging_ = false;
        repaint();
    }

    void onMouseEnter() override {
        isHovered_ = true;
        repaint();
    }

    void onMouseLeave() override {
        isHovered_ = false;
        repaint();
    }

    void onMouseWheel(float delta) override {
        if (!param_) return;
        const float step = 0.02f * (delta > 0 ? 1.0f : -1.0f);
        param_->setNormalized(param_->getNormalized() + step);
        repaint();
    }

private:
    void detectBipolar() noexcept {
        if (param_) {
            isBipolar_ = (std::abs(param_->getMinValue() + param_->getMaxValue()) < 0.01f) &&
                         (std::abs(param_->getDefaultValue()) < 0.01f);
        }
    }

    Parameter* param_{nullptr};
    Color accentColor_{Color::AccentBlue()};
    bool isBipolar_{false};
    bool isDragging_{false};
    bool isHovered_{false};
    float dragStartX_{0.0f};
    float dragStartY_{0.0f};
    float dragStartNorm_{0.0f};
    inline static int globalMouseMode_{0};
};

} // namespace avra::gui
