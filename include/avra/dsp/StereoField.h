#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#else
#include "../core/Types.h"
#include "Math.h"
#endif

#include <cmath>

namespace avra::dsp {

namespace math = avra::math;

/**
 * @brief High-precision Mid/Side, Stereo Width, and Phase manipulation processor.
 * Ideal for mastering consoles, spatial wideners, and clean stereo utility tools.
 */
class StereoField {
public:
    StereoField() = default;

    /// Convert Left/Right into Mid/Side
    static inline void encodeMidSide(float left, float right, float& mid, float& side) noexcept {
        constexpr float invSqrt2 = 0.70710678f;
        mid  = (left + right) * invSqrt2;
        side = (left - right) * invSqrt2;
    }

    /// Convert Mid/Side back into Left/Right
    static inline void decodeMidSide(float mid, float side, float& left, float& right) noexcept {
        constexpr float invSqrt2 = 0.70710678f;
        left  = (mid + side) * invSqrt2;
        right = (mid - side) * invSqrt2;
    }

    /// Adjust stereo width (0.0 = mono, 1.0 = normal, 2.0 = ultra-wide)
    static inline void applyWidth(float& left, float& right, float width) noexcept {
        float mid = 0.0f, side = 0.0f;
        encodeMidSide(left, right, mid, side);
        side *= width;
        decodeMidSide(mid, side, left, right);
    }

    /// Apply constant-power (-3 dB) stereo panning [-1.0 .. +1.0]
    static inline void applyPan(float& left, float& right, float pan) noexcept {
        pan = math::clamp(pan, -1.0f, 1.0f);
        constexpr float piOver4 = kPi * 0.25f;
        const float angle = (pan + 1.0f) * piOver4; // 0 to pi/2
        const float gainL = std::cos(angle);
        const float gainR = std::sin(angle);
        left  *= gainL;
        right *= gainR;
    }

    /// Invert phase on left, right, or both channels
    static inline void applyPhase(float& left, float& right, bool invertLeft, bool invertRight) noexcept {
        if (invertLeft)  left  = -left;
        if (invertRight) right = -right;
    }
};

/**
 * @brief Mono Bass Crossover & Spatial Imager Engine.
 * Sums low frequencies below crossover frequency to 100% solid mono while widening mids/highs.
 */
class MonoBassProcessor {
public:
    MonoBassProcessor() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        updateFilters();
    }

    void setCrossoverFreq(float freqHz) noexcept {
        crossoverHz_ = math::clamp(freqHz, 20.0f, 600.0f);
        updateFilters();
    }

    void reset() noexcept {
        lpL_ = 0.0f;
        lpR_ = 0.0f;
    }

    /// Split low and high frequencies, sum lows to mono, apply width to highs
    inline void processStereo(float& inL, float& inR, float width) noexcept {
        // 1st order Butterworth complementary lowpass / highpass
        lpL_ = alpha_ * inL + (1.0f - alpha_) * lpL_;
        lpR_ = alpha_ * inR + (1.0f - alpha_) * lpR_;
        lpL_ = math::sanitize(lpL_);
        lpR_ = math::sanitize(lpR_);

        const float lowL = lpL_;
        const float lowR = lpR_;
        float highL = inL - lowL;
        float highR = inR - lowR;

        // Bass mono sum
        const float monoBass = (lowL + lowR) * 0.5f;

        // Apply width to mids/highs
        StereoField::applyWidth(highL, highR, width);

        // Recombine
        inL = monoBass + highL;
        inR = monoBass + highR;
    }

private:
    void updateFilters() noexcept {
        constexpr float twoPi = 2.0f * kPi;
        const float dt = 1.0f / static_cast<float>(sampleRate_);
        const float rc = 1.0f / (twoPi * crossoverHz_);
        alpha_ = dt / (rc + dt);
    }

    double sampleRate_{48000.0};
    float crossoverHz_{140.0f};
    float alpha_{0.02f};
    float lpL_{0.0f};
    float lpR_{0.0f};
};

/**
 * @brief Real-time Stereo Phase Correlation Computer [-1.0 = anti-phase .. +1.0 = pure mono].
 */
class PhaseCorrelationDetector {
public:
    PhaseCorrelationDetector() = default;

    void prepare(double sampleRate) noexcept {
        decay_ = std::exp(-1.0f / (static_cast<float>(sampleRate > 1000.0 ? sampleRate : 48000.0) * 0.05f));
    }

    inline void processSample(float left, float right) noexcept {
        dotProduct_ = decay_ * dotProduct_ + (1.0f - decay_) * (left * right);
        normL_      = decay_ * normL_      + (1.0f - decay_) * (left * left);
        normR_      = decay_ * normR_      + (1.0f - decay_) * (right * right);
    }

    [[nodiscard]] float getCorrelation() const noexcept {
        const float denom = std::sqrt(normL_ * normR_);
        if (denom <= 1e-6f) return 1.0f;
        return math::clamp(dotProduct_ / denom, -1.0f, 1.0f);
    }

private:
    float decay_{0.99f};
    float dotProduct_{0.0f};
    float normL_{0.0f};
    float normR_{0.0f};
};

} // namespace avra::dsp
