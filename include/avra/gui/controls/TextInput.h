#pragma once

#include "avra/gui/Component.h"
#include <string>
#include <functional>

namespace avra::gui {

/**
 * @brief Sleek hardware-accelerated text input box for plugin parameters & project wizard.
 */
class TextInput : public Component {
public:
    explicit TextInput(std::string placeholder = "", std::string initialText = "")
        : placeholder_(std::move(placeholder)), text_(std::move(initialText)) {}

    void setText(std::string text) {
        text_ = std::move(text);
        repaint();
    }

    [[nodiscard]] const std::string& getText() const noexcept { return text_; }

    void setPlaceholder(std::string placeholder) {
        placeholder_ = std::move(placeholder);
        repaint();
    }

    void setOnTextChanged(std::function<void(const std::string&)> cb) {
        onTextChanged_ = std::move(cb);
    }

    void setOnEnterPressed(std::function<void(const std::string&)> cb) {
        onEnterPressed_ = std::move(cb);
    }

    [[nodiscard]] bool isFocused() const noexcept { return isFocused_; }
    [[nodiscard]] bool isTextInputFocused() const noexcept override { return isFocused_; }
    void setFocused(bool focused) noexcept {
        isFocused_ = focused;
        repaint();
    }

    void paint(Graphics& g) override {
        const auto r = getLocalBounds();
        const Color bg = Color::fromHex(0x181A20);
        const Color border = isFocused_ ? Color::AccentCyan() : Color::Outline();

        g.fillRoundedRect(r, 4.0f, bg);
        g.drawRoundedRect(r, 4.0f, border, isFocused_ ? 1.5f : 1.0f);

        const Rect<float> textBounds = { 10.0f, 0.0f, r.width - 20.0f, r.height };
        if (text_.empty() && !placeholder_.empty() && !isFocused_) {
            g.drawText(placeholder_, textBounds, Color::TextSecondary().withAlpha(0.6f), 12.0f, false);
        } else {
            std::string display = text_;
            if (isFocused_) {
                display += "_"; // Caret indicator
            }
            g.drawText(display, textBounds, Color::TextPrimary(), 12.0f, false);
        }
    }

    void onMouseDown(const MouseEvent&) override {
        setFocused(true);
    }

    void onFocusLost() override {
        setFocused(false);
    }

    void onChar(wchar_t c) override {
        if (!isFocused_) return;

        if (c == 8) { // Backspace
            if (!text_.empty()) {
                text_.pop_back();
                if (onTextChanged_) onTextChanged_(text_);
                repaint();
            }
        } else if (c == 13) { // Enter
            setFocused(false);
            if (onEnterPressed_) onEnterPressed_(text_);
        } else if (c >= 32 && c <= 126) { // Printable ASCII
            text_.push_back(static_cast<char>(c));
            if (onTextChanged_) onTextChanged_(text_);
            repaint();
        }
    }

    void onKeyDown(int keyCode) override {
        if (!isFocused_) {
            if (parent_) {
                parent_->onKeyDown(keyCode);
            }
            return;
        }
        if (keyCode == 27) { // Escape
            setFocused(false);
        } else if (keyCode == 46) { // Delete key while editing text
            if (!text_.empty()) {
                text_.pop_back();
                if (onTextChanged_) onTextChanged_(text_);
                repaint();
            }
        }
    }

private:
    std::string placeholder_{};
    std::string text_{};
    std::function<void(const std::string&)> onTextChanged_{nullptr};
    std::function<void(const std::string&)> onEnterPressed_{nullptr};
    bool isFocused_{false};
};

} // namespace avra::gui
