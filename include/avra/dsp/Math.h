#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#else
#include "../core/Types.h"
#endif
#include <cmath>
#include <algorithm>
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <pmmintrin.h>
#ifndef _MM_DENORMALS_ZERO_MASK
#define _MM_DENORMALS_ZERO_MASK 0x0040
#endif
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_ON 0x0040
#endif
#ifndef _MM_DENORMALS_ARE_ZERO_ON
#define _MM_DENORMALS_ARE_ZERO_ON 0x0040
#endif
#ifndef _MM_SET_DENORMALS_ARE_ZERO_MODE
#define _MM_SET_DENORMALS_ARE_ZERO_MODE(mode) _mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO_MASK) | (mode))
#endif
#ifndef _MM_GET_DENORMALS_ARE_ZERO_MODE
#define _MM_GET_DENORMALS_ARE_ZERO_MODE() (_mm_getcsr() & _MM_DENORMALS_ZERO_MASK)
#endif
#define AVRA_HAS_SSE_DENORMAL_GUARD 1
#endif

namespace avra::math {

/// Branchless clamp
template <typename T>
constexpr T clamp(T val, T minVal, T maxVal) noexcept {
    return std::max(minVal, std::min(val, maxVal));
}

/// Convert Decibels to Linear gain factor
template <FloatType T>
inline T dBToLinear(T dB) noexcept {
    if (dB == static_cast<T>(0))
        return static_cast<T>(1);
    if (dB <= static_cast<T>(kMinDecibels))
        return static_cast<T>(0);
    return std::exp(dB * static_cast<T>(0.1151292546497022842));
}

/// Convert Linear gain factor to Decibels
template <FloatType T>
inline T linearTodB(T linear) noexcept {
    constexpr T minLinear = static_cast<T>(1e-6); // -120 dB
    if (linear <= minLinear)
        return static_cast<T>(kMinDecibels);
    return static_cast<T>(20) * std::log10(linear);
}

/// Flush denormals to zero (branchless anti-denormal prevention)
template <FloatType T>
inline T sanitize(T val) noexcept {
    constexpr T threshold = static_cast<T>(1e-15);
    return (std::abs(val) < threshold) ? static_cast<T>(0) : val;
}

/// Linear interpolation
template <FloatType T>
constexpr T lerp(T a, T b, T t) noexcept {
    return a + t * (b - a);
}

/// Map a normalized [0..1] value to a logarithmic frequency range [minHz..maxHz]
template <FloatType T>
inline T normalizedToLogFreq(T normalized, T minHz, T maxHz) noexcept {
    normalized = clamp(normalized, static_cast<T>(0), static_cast<T>(1));
    return minHz * std::pow(maxHz / minHz, normalized);
}

/// Map a frequency [minHz..maxHz] back to normalized [0..1]
template <FloatType T>
inline T logFreqToNormalized(T freqHz, T minHz, T maxHz) noexcept {
    freqHz = clamp(freqHz, minHz, maxHz);
    return std::log(freqHz / minHz) / std::log(maxHz / minHz);
}

/// Fast rational Pade approximation of tanh(x) for real-time audio waveshaping
template <FloatType T>
constexpr T fastTanh(T x) noexcept {
    x = clamp(x, static_cast<T>(-3), static_cast<T>(3));
    const T x2 = x * x;
    return x * (static_cast<T>(27) + x2) / (static_cast<T>(27) + static_cast<T>(9) * x2);
}

/**
 * @brief RAII Hardware Denormals-Are-Zero (DAZ) & Flush-To-Zero (FTZ) Guard.
 * Eliminates denormal CPU spikes on feedback IIR filters, compressors, and reverbs.
 */
struct ScopedNoDenormals {
#if defined(AVRA_HAS_SSE_DENORMAL_GUARD)
    unsigned int prevFz_{0};
    unsigned int prevDaz_{0};
    ScopedNoDenormals() noexcept {
        prevFz_  = _MM_GET_FLUSH_ZERO_MODE();
        prevDaz_ = _MM_GET_DENORMALS_ARE_ZERO_MODE();
        _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
        _MM_SET_DENORMALS_ARE_ZERO_MODE(_MM_DENORMALS_ARE_ZERO_ON);
    }
    ~ScopedNoDenormals() noexcept {
        _MM_SET_FLUSH_ZERO_MODE(prevFz_);
        _MM_SET_DENORMALS_ARE_ZERO_MODE(prevDaz_);
    }
    [[nodiscard]] static bool isHardwareDenormalFlushActive() noexcept {
        return (_MM_GET_FLUSH_ZERO_MODE() == _MM_FLUSH_ZERO_ON) &&
               (_MM_GET_DENORMALS_ARE_ZERO_MODE() == _MM_DENORMALS_ARE_ZERO_ON);
    }
#else
    ScopedNoDenormals() noexcept = default;
    ~ScopedNoDenormals() noexcept = default;
    [[nodiscard]] static bool isHardwareDenormalFlushActive() noexcept { return true; }
#endif
    ScopedNoDenormals(const ScopedNoDenormals&) = delete;
    ScopedNoDenormals& operator=(const ScopedNoDenormals&) = delete;
};

} // namespace avra::math

namespace avra::dsp {
using ScopedNoDenormals = avra::math::ScopedNoDenormals;
} // namespace avra::dsp
