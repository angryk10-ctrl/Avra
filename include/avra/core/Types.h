#pragma once

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <span>
#include <string_view>
#include <string>
#include <atomic>
#include <concepts>
#include <algorithm>

#if defined(__cpp_lib_math_constants) && __cpp_lib_math_constants >= 201907L
#include <numbers>
#endif

namespace avra {

// Core sample type: defaults to 32-bit float for audio streaming
using Sample = float;
using ChannelIndex = uint32_t;
using FrameIndex = uint32_t;
using ParameterId = uint32_t;
using StringView = std::string_view;

// Mathematical constants
#if defined(__cpp_lib_math_constants) && __cpp_lib_math_constants >= 201907L
constexpr float kPi = std::numbers::pi_v<float>;
#else
constexpr float kPi = 3.14159265358979323846f;
#endif
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kHalfPi = 0.5f * kPi;
constexpr float kMinDecibels = -120.0f;

// Floating-point concept
template <typename T>
concept FloatType = std::floating_point<T>;

} // namespace avra
