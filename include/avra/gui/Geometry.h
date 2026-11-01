#pragma once

#include "avra/core/Types.h"
#include <algorithm>
#include <cmath>

namespace avra::gui {

template <typename T = float>
struct Point {
    T x{0};
    T y{0};

    constexpr Point() noexcept = default;
    constexpr Point(T px, T py) noexcept : x(px), y(py) {}

    constexpr Point operator+(const Point& other) const noexcept { return { x + other.x, y + other.y }; }
    constexpr Point operator-(const Point& other) const noexcept { return { x - other.x, y - other.y }; }
};

template <typename T = float>
struct Size {
    T width{0};
    T height{0};

    constexpr Size() noexcept = default;
    constexpr Size(T w, T h) noexcept : width(w), height(h) {}
};

template <typename T = float>
struct Rect {
    T x{0};
    T y{0};
    T width{0};
    T height{0};

    constexpr Rect() noexcept = default;
    constexpr Rect(T rx, T ry, T rw, T rh) noexcept : x(rx), y(ry), width(rw), height(rh) {}

    [[nodiscard]] constexpr T right() const noexcept { return x + width; }
    [[nodiscard]] constexpr T bottom() const noexcept { return y + height; }
    [[nodiscard]] constexpr Point<T> getCentre() const noexcept { return { x + width * static_cast<T>(0.5), y + height * static_cast<T>(0.5) }; }

    [[nodiscard]] constexpr bool contains(Point<T> pt) const noexcept {
        return (pt.x >= x && pt.x < right() && pt.y >= y && pt.y < bottom());
    }

    [[nodiscard]] constexpr Rect reduced(T amount) const noexcept {
        return { x + amount, y + amount, std::max<T>(0, width - amount * 2), std::max<T>(0, height - amount * 2) };
    }

    [[nodiscard]] constexpr Rect reduced(T dx, T dy) const noexcept {
        return { x + dx, y + dy, std::max<T>(0, width - dx * 2), std::max<T>(0, height - dy * 2) };
    }
};

/**
 * @brief High-precision RGBA color with sleek predefined studio palette colors.
 */
struct Color {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    constexpr Color() noexcept = default;
    constexpr Color(float red, float green, float blue, float alpha = 1.0f) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    static constexpr Color fromHex(uint32_t hex, float alpha = 1.0f) noexcept {
        return {
            static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
            static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
            static_cast<float>(hex & 0xFF) / 255.0f,
            alpha
        };
    }

    [[nodiscard]] constexpr Color withAlpha(float newAlpha) const noexcept {
        return { r, g, b, newAlpha };
    }

    // Professional Modern Studio Theme Palette
    static constexpr Color Background() noexcept { return fromHex(0x18191C); }
    static constexpr Color Surface() noexcept    { return fromHex(0x23252A); }
    static constexpr Color SurfaceHighlight() noexcept { return fromHex(0x2E3038); }
    static constexpr Color Outline() noexcept    { return fromHex(0x3B3E48); }
    static constexpr Color TextPrimary() noexcept { return fromHex(0xF0F1F3); }
    static constexpr Color TextSecondary() noexcept { return fromHex(0x8C909C); }
    static constexpr Color AccentBlue() noexcept { return fromHex(0x3A86FF); }
    static constexpr Color AccentCyan() noexcept { return fromHex(0x00F5D4); }
    static constexpr Color AccentAmber() noexcept { return fromHex(0xFF9E00); }
    static constexpr Color AccentRed() noexcept  { return fromHex(0xFF006E); }
};

} // namespace avra::gui
