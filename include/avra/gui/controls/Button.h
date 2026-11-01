#pragma once

#include "avra/gui/Component.h"
#include "avra/core/Parameter.h"
#include <string>

namespace avra::gui {

/**
 * @brief Pro studio push/toggle button with parameter binding, hover states, and custom accents.
 */
class Button : public Component {
public:
    explicit Button(std::string text = "", Parameter* param = nullptr, Color accentColor = Color::AccentCyan())
        : text_(std::move(text))
        , param_(param)
        , accentColor_(param ? Color::AccentRed() : accentColor)
        , autoToggle_(param != nullptr) {}

    void setText(std::string text) {
        text_ = std::move(text);
        repaint();
    }

    void setLabel(std::string text) {
        setText(std::move(text));
    }

    [[nodiscard]] const std::string& getText() const noexcept { return text_; }

    void setParameter(Parameter* param) {
        param_ = param;
        autoToggle_ = (param != nullptr);
        repaint();
    }

    void setAccentColor(Color color) noexcept {
        accentColor_ = color;
        repaint();
    }

    void setAutoToggle(bool autoToggle) noexcept {
        autoToggle_ = autoToggle;
    }

    [[nodiscard]] bool getToggleState() const noexcept {
        if (param_) return param_->getPlain() > 0.5f;
        return isToggled_;
    }

    void setToggleState(bool toggled) noexcept {
        isToggled_ = toggled;
        if (param_) param_->setPlain(toggled ? 1.0f : 0.0f);
        repaint();
    }

    void paint(Graphics& g) override {
        const bool active = getToggleState();
        const Rect<float> r = getLocalBounds();

        Color topBg = Color::fromHex(0x252830);
        Color botBg = Color::fromHex(0x1C1E24);
        Color outline = Color::Outline();
        Color textCol = Color::TextSecondary();

        if (active) {
            topBg = accentColor_.withAlpha(0.28f);
            botBg = accentColor_.withAlpha(0.16f);
            outline = accentColor_;
            textCol = Color::TextPrimary();
        } else if (isPressed_) {
            topBg = Color::fromHex(0x181A20);
            botBg = Color::fromHex(0x181A20);
            outline = accentColor_.withAlpha(0.8f);
            textCol = Color::TextPrimary();
        } else if (isHovered_) {
            topBg = Color::fromHex(0x2E323D);
            botBg = Color::fromHex(0x23262F);
            outline = accentColor_.withAlpha(0.55f);
            textCol = Color::TextPrimary();
        }

        g.fillLinearGradientRect(r, topBg, botBg, 5.0f);
        g.drawRoundedRect(r, 5.0f, outline, active ? 1.5f : 1.0f);

        if (active && param_) {
            // Subtle LED indicator dot for hardware bypass/engage switches
            g.fillCircle({ 11.0f, r.height * 0.5f }, 3.0f, accentColor_);
        }

        std::string displayText = text_.empty() && param_ ? param_->getName() : text_;
        g.drawText(displayText, r, textCol, 11.5f, true, active || isHovered_);
    }

    void setOnClick(std::function<void()> onClick) {
        onClick_ = std::move(onClick);
    }

    void onMouseDown(const MouseEvent&) override {
        isPressed_ = true;
        if (autoToggle_) {
            setToggleState(!getToggleState());
        }
        if (onClick_) {
            onClick_();
        }
        repaint();
    }

    void onMouseUp(const MouseEvent&) override {
        if (isPressed_) {
            isPressed_ = false;
            repaint();
        }
    }

    void onMouseEnter() override {
        isHovered_ = true;
        repaint();
    }

    void onMouseLeave() override {
        isHovered_ = false;
        isPressed_ = false;
        repaint();
    }

    void onMouseMove(const MouseEvent& e) override {
        const bool nowHovered = getLocalBounds().contains(e.pos);
        if (isHovered_ != nowHovered) {
            isHovered_ = nowHovered;
            repaint();
        }
    }

private:
    std::string text_{};
    Parameter* param_{nullptr};
    Color accentColor_{Color::AccentCyan()};
    std::function<void()> onClick_{nullptr};
    bool isToggled_{false};
    bool autoToggle_{false};
    bool isHovered_{false};
    bool isPressed_{false};
};

} // namespace avra::gui
