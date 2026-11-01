#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#else
#include "../core/Types.h"
#include "Math.h"
#endif

#include <cmath>
#include <algorithm>

namespace avra::dsp {

namespace math = avra::math;

/**
 * @brief High-fidelity analog saturation and waveshaping DSP building blocks.
 * Zero-allocation, realtime safe, suitable for tape drive, tube warmth, and diode fuzz.
 */
class Saturation {
public:
    /// Analytical 1st-order antiderivative F1(x) = ln(cosh(x)) evaluated without overflow
    [[nodiscard]] static inline double antiderivativeTanh1(double x) noexcept {
        const double ax = std::abs(x);
        if (ax > 9.0) {
            return ax - 0.6931471805599453;
        }
        return ax + std::log1p(std::exp(-2.0 * ax)) - 0.6931471805599453;
    }

    /**
     * @brief 1st-Order Antiderivative Anti-Aliased (ADAA) tanh waveshaper:
     *        (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1]) with L'Hopital fallback when |dx| < 1e-6.
     */
    [[nodiscard]] static inline float adaa1Tanh(float x, float& xPrev) noexcept {
        const double xd = static_cast<double>(x);
        const double xp = static_cast<double>(xPrev);
        const double dx = xd - xp;
        xPrev = x;
        if (std::abs(dx) < 1e-6) {
            return static_cast<float>(std::tanh(0.5 * (xd + xp)));
        }
        return static_cast<float>((antiderivativeTanh1(xd) - antiderivativeTanh1(xp)) / dx);
    }

    /// Smooth hyperbolic tangent saturation with drive and output compensation
    [[nodiscard]] static inline float tanhSat(float in, float driveDb) noexcept {
        float driveGain = 1.0f, comp = 1.0f;
        getDriveAndComp(driveDb, driveGain, comp);
        const float driven = in * driveGain;
        return std::tanh(driven) * comp;
    }

    /// Asymmetric tape saturation curve with warm even harmonics
    [[nodiscard]] static inline float tapeSat(float in, float driveDb, float warmth) noexcept {
        float drive = 1.0f, comp = 1.0f;
        getDriveAndComp(driveDb, drive, comp);
        float x = in * drive;

        // Even harmonic asymmetry injection (tape hysteresis proxy)
        const float bias = warmth * 0.15f;
        x += bias * (x * x);

        // Soft saturation via algebraic curve: x / (1 + |x|)
        const float absX = std::abs(x);
        float saturated = x / (1.0f + absX * 0.7f);

        // Soft top knee
        if (saturated > 1.2f) saturated = 1.2f;
        if (saturated < -1.2f) saturated = -1.2f;

        return saturated * comp;
    }

    /// Tube-style triode saturation with rich 2nd and 3rd harmonic profile
    [[nodiscard]] static inline float tubeSat(float in, float driveDb, float bias) noexcept {
        float drive = 1.0f, comp = 1.0f;
        getDriveAndComp(driveDb, drive, comp);
        float x = in * drive + (bias * 0.2f);

        float out = 0.0f;
        if (x < -1.5f) {
            out = -1.0f;
        } else if (x > 1.5f) {
            out = 1.0f;
        } else {
            out = x - (x * x * x * 0.15f);
        }

        return (out - (bias * 0.2f)) * comp;
    }

private:
    static inline void getDriveAndComp(float driveDb, float& outDrive, float& outComp) noexcept {
        struct Cache {
            float db{-999.0f};
            float drv{1.0f};
            float cmp{1.0f};
        };
        thread_local Cache s_cache;
        if (std::abs(driveDb - s_cache.db) > 1e-5f) {
            s_cache.db  = driveDb;
            s_cache.drv = math::dBToLinear(driveDb);
            s_cache.cmp = 1.0f / std::max(1.0f, std::sqrt(s_cache.drv));
        }
        outDrive = s_cache.drv;
        outComp  = s_cache.cmp;
    }

public:

    /// Hard diode clipping
    [[nodiscard]] static inline float hardClip(float in, float threshold = 1.0f) noexcept {
        return math::clamp(in, -threshold, threshold);
    }

    /// Wave-folding distortion for aggressive modern sound design
    [[nodiscard]] static inline float foldback(float in, float threshold = 1.0f) noexcept {
        if (in > threshold || in < -threshold) {
            in = std::fabs(std::fabs(std::fmod(in - threshold, threshold * 4.0f)) - threshold * 2.0f) - threshold;
        }
        return in;
    }
};

} // namespace avra::dsp
