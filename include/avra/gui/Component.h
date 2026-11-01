#pragma once

#include "avra/gui/Geometry.h"
#include "avra/gui/Graphics.h"
#include <vector>
#include <functional>
#include <algorithm>

namespace avra::gui {

struct MouseEvent {
    Point<float> pos{0.0f, 0.0f};
    Point<float> startPos{0.0f, 0.0f};
    bool isLeftDown{false};
    bool isRightDown{false};
    bool isShiftDown{false};
};

/**
 * @brief Base class for all visual UI components in Avra.
 */
class Component {
public:
    Component() = default;
    virtual ~Component() {
        if (parent_) {
            parent_->removeChild(this);
        }
        for (auto* child : children_) {
            if (child) {
                child->parent_ = nullptr;
            }
        }
        children_.clear();
    }

    [[nodiscard]] Rect<float> getBounds() const noexcept { return bounds_; }
    [[nodiscard]] Rect<float> getLocalBounds() const noexcept { return { 0.0f, 0.0f, bounds_.width, bounds_.height }; }
    [[nodiscard]] float getWidth() const noexcept { return bounds_.width; }
    [[nodiscard]] float getHeight() const noexcept { return bounds_.height; }

    [[nodiscard]] Point<float> toLocal(Point<float> pt) const noexcept {
        if (parent_) {
            pt = parent_->toLocal(pt);
        }
        return { pt.x - bounds_.x, pt.y - bounds_.y };
    }

    [[nodiscard]] Point<float> toGlobal(Point<float> pt) const noexcept {
        pt = { pt.x + bounds_.x, pt.y + bounds_.y };
        if (parent_) {
            pt = parent_->toGlobal(pt);
        }
        return pt;
    }

    [[nodiscard]] bool isVisible() const noexcept { return isVisible_; }
    void setVisible(bool v) noexcept {
        if (isVisible_ != v) {
            isVisible_ = v;
            repaint();
        }
    }

    void setBounds(Rect<float> bounds) noexcept {
        bounds_ = bounds;
        resized();
    }
    void setBounds(float x, float y, float w, float h) noexcept {
        setBounds({ x, y, w, h });
    }

    void addChild(Component* child) {
        if (!child || child == this) return;
        if (child->parent_ && child->parent_ != this) {
            child->parent_->removeChild(child);
        }
        child->parent_ = this;
        if (std::find(children_.begin(), children_.end(), child) == children_.end()) {
            children_.push_back(child);
        }
    }

    void removeChild(Component* child) {
        auto it = std::remove(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            child->parent_ = nullptr;
            children_.erase(it, children_.end());
        }
    }

    [[nodiscard]] const std::vector<Component*>& getChildren() const noexcept { return children_; }
    [[nodiscard]] Component* getParent() const noexcept { return parent_; }

    virtual void paint(Graphics& g) { (void)g; }
    virtual void paintOverlay(Graphics& g) { (void)g; }
    [[nodiscard]] virtual bool interceptsOverlayMouse(Point<float> pt) const noexcept { (void)pt; return false; }
    virtual void resized() {}

    virtual void onMouseDown(const MouseEvent& e) { (void)e; }
    virtual void onDoubleClick(const MouseEvent& e) { (void)e; }
    virtual void onMouseDrag(const MouseEvent& e) { (void)e; }
    virtual void onMouseUp(const MouseEvent& e) { (void)e; }
    virtual void onMouseMove(const MouseEvent& e) { (void)e; }
    virtual void onMouseEnter() {}
    virtual void onMouseLeave() {}
    virtual void onMouseWheel(float delta) {
        if (parent_) {
            parent_->onMouseWheel(delta);
        }
    }
    virtual void onChar(wchar_t c) { (void)c; }
    virtual void onKeyDown(int keyCode) {
        if (parent_) {
            parent_->onKeyDown(keyCode);
        }
    }
    [[nodiscard]] virtual bool isTextInputFocused() const noexcept { return false; }
    [[nodiscard]] virtual bool canCloseWindow() { return true; }
    virtual void onFocusLost() {}
    virtual void onTimerTick() {}

    void tickHierarchy() {
        if (!isVisible_) return;
        onTimerTick();
        for (auto* child : children_) {
            if (child->isVisible()) {
                child->tickHierarchy();
            }
        }
    }

    void setRepaintCallback(std::function<void()> cb) { repaintCallback_ = std::move(cb); }
    void repaint() {
        if (repaintCallback_) {
            repaintCallback_();
        } else if (parent_) {
            parent_->repaint();
        }
    }

    [[nodiscard]] Component* hitTest(Point<float> pt) {
        if (!isVisible_) return nullptr;
        if (interceptsOverlayMouse(pt)) return this;
        // Search children in reverse (topmost child first)
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            Component* child = *it;
            if (child->isVisible() && child->getBounds().contains(pt)) {
                Point<float> localPt = { pt.x - child->getBounds().x, pt.y - child->getBounds().y };
                auto* hit = child->hitTest(localPt);
                if (hit) return hit;
            }
        }
        return this;
    }

    /// Recursively render this component and all its children with proper coordinate translation
    void renderHierarchy(Graphics& g) {
        if (!isVisible_) return;

        paint(g);

        for (auto* child : children_) {
            if (child->isVisible()) {
                g.pushOffset({ child->getBounds().x, child->getBounds().y });
                child->renderHierarchy(g);
                g.popOffset();
            }
        }

        paintOverlay(g);
    }

protected:
    Rect<float> bounds_{0.0f, 0.0f, 100.0f, 100.0f};
    Component* parent_{nullptr};
    std::vector<Component*> children_;
    std::function<void()> repaintCallback_{nullptr};
    bool isVisible_{true};
};


} // namespace avra::gui
