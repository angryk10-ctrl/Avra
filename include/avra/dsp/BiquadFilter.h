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

enum class FilterType {
    Lowpass,
    Highpass,
    Bandpass,
    Notch,
    Peaking,
    LowShelf,
    HighShelf
};

/**
 * @brief High-performance Transposed Direct-Form II Biquad Filter.
 * Real-time safe, zero-allocation, numerically stable for audio equalization and tone shaping.
 */
class BiquadFilter {
public:
    BiquadFilter() = default;

    void setSampleRate(double sampleRate) noexcept {
        const double sr = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        if (std::abs(sr - sampleRate_) > 0.5 || !initialized_) {
            sampleRate_ = sr;
            recalculate();
        }
    }

    void setType(FilterType type) noexcept {
        if (type != type_ || !initialized_) {
            type_ = type;
            recalculate();
        }
    }

    void setFrequency(float freqHz) noexcept {
        const float clamped = math::clamp(freqHz, 10.0f, static_cast<float>(sampleRate_ * 0.49));
        if (!initialized_ || std::abs(clamped - freqHz_) > std::max(0.02f, freqHz_ * 0.0002f)) {
            freqHz_ = clamped;
            recalculate();
        }
    }

    void setQ(float q) noexcept {
        const float clamped = math::clamp(q, 0.1f, 18.0f);
        if (!initialized_ || std::abs(clamped - q_) > 2e-4f) {
            q_ = clamped;
            recalculate();
        }
    }

    void setGainDb(float gainDb) noexcept {
        const float clamped = math::clamp(gainDb, -36.0f, 36.0f);
        if (!initialized_ || std::abs(clamped - gainDb_) > 0.005f) {
            gainDb_ = clamped;
            recalculate();
        }
    }

    void setParameters(FilterType type, float freqHz, float q, float gainDb = 0.0f) noexcept {
        const float clampedFreq = math::clamp(freqHz, 10.0f, static_cast<float>(sampleRate_ * 0.49));
        const float clampedQ    = math::clamp(q, 0.1f, 18.0f);
        const float clampedGain = math::clamp(gainDb, -36.0f, 36.0f);
        if (initialized_ && type == type_ &&
            std::abs(clampedFreq - freqHz_) <= std::max(0.02f, freqHz_ * 0.0002f) &&
            std::abs(clampedQ - q_) <= 2e-4f &&
            std::abs(clampedGain - gainDb_) <= 0.005f) {
            return;
        }
        type_   = type;
        freqHz_ = clampedFreq;
        q_      = clampedQ;
        gainDb_ = clampedGain;
        recalculate();
    }

    void reset() noexcept {
        s1_ = 0.0f;
        s2_ = 0.0f;
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        // Direct Form II Transposed
        const float out = b0_ * in + s1_;
        s1_ = b1_ * in - a1_ * out + s2_;
        s2_ = b2_ * in - a2_ * out;
        s1_ = math::sanitize(s1_);
        s2_ = math::sanitize(s2_);
        return out;
    }

    void processBlock(float* data, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            data[i] = processSample(data[i]);
        }
    }

    /**
     * @brief Evaluate exact magnitude response in dB at a given frequency (20 Hz - 20 kHz)
     * using the digital transfer function |H(e^(jw))| for live EQ curve rendering.
     */
    [[nodiscard]] float getMagnitudeDbAtFrequency(float freqHz) const noexcept {
        const float w = kTwoPi * (math::clamp(freqHz, 10.0f, static_cast<float>(sampleRate_ * 0.495)) / static_cast<float>(sampleRate_));
        const float cosW = std::cos(w);
        const float cos2W = std::cos(2.0f * w);
        const float sinW = std::sin(w);
        const float sin2W = std::sin(2.0f * w);

        const float numReal = b0_ + b1_ * cosW + b2_ * cos2W;
        const float numImag = -(b1_ * sinW + b2_ * sin2W);
        const float denReal = 1.0f + a1_ * cosW + a2_ * cos2W;
        const float denImag = -(a1_ * sinW + a2_ * sin2W);

        const float numMagSq = numReal * numReal + numImag * numImag;
        const float denMagSq = std::max(1e-12f, denReal * denReal + denImag * denImag);
        const float mag = std::sqrt(numMagSq / denMagSq);
        return math::linearTodB(mag);
    }

    [[nodiscard]] float getPhaseDegAtFrequency(float freqHz) const noexcept {
        const float w = kTwoPi * (math::clamp(freqHz, 10.0f, static_cast<float>(sampleRate_ * 0.495)) / static_cast<float>(sampleRate_));
        const float cosW = std::cos(w);
        const float cos2W = std::cos(2.0f * w);
        const float sinW = std::sin(w);
        const float sin2W = std::sin(2.0f * w);

        const float numReal = b0_ + b1_ * cosW + b2_ * cos2W;
        const float numImag = -(b1_ * sinW + b2_ * sin2W);
        const float denReal = 1.0f + a1_ * cosW + a2_ * cos2W;
        const float denImag = -(a1_ * sinW + a2_ * sin2W);

        const float phaseRad = std::atan2(numImag, numReal) - std::atan2(denImag, denReal);
        return phaseRad * (180.0f / kPi);
    }

private:
    /**
     * @brief TPT / Cytomic ZDF bilinear state-variable equivalent coefficient synthesis
     * with Orfanidis/Vicanek Nyquist bandwidth decramping so 8 kHz - 20 kHz bells and shelves
     * never cramp to 0 dB at Nyquist at 44.1 kHz or 48 kHz.
     */
    void recalculate() noexcept {
        initialized_ = true;
        const float sr = static_cast<float>((sampleRate_ > 1000.0) ? sampleRate_ : 48000.0);
        const float fc = math::clamp(freqHz_, 10.0f, sr * 0.485f);
        const float g0 = std::tan(kPi * fc / sr);
        float g = g0;
        float k = 1.0f / std::max(0.10f, q_);
        float m0 = 0.0f, m1 = 0.0f, m2 = 1.0f;

        switch (type_) {
            case FilterType::Lowpass:
                m0 = 0.0f; m1 = 0.0f; m2 = 1.0f;
                break;
            case FilterType::Highpass:
                m0 = 1.0f; m1 = -k;   m2 = -1.0f;
                break;
            case FilterType::Bandpass:
                m0 = 0.0f; m1 = k;    m2 = 0.0f;
                break;
            case FilterType::Notch:
                m0 = 1.0f; m1 = -k;   m2 = 0.0f;
                break;
            case FilterType::Peaking: {
                const float nyqRatio = fc / (0.5f * sr);
                const float decramp  = std::sqrt(std::max(0.22f, 1.0f - 0.72f * nyqRatio * nyqRatio));
                const float qEff     = std::max(0.10f, q_ * decramp);
                const float A        = std::pow(10.0f, gainDb_ / 40.0f);
                k = 1.0f / std::max(0.05f, qEff * A);
                // Orfanidis prescribed Nyquist-frequency gain matching analog prototype at f_N = 0.5 * fs
                const float fNyq = 0.5f * sr;
                const float rNyq = (fNyq / fc) - (fc / fNyq);
                const float uSq  = (rNyq * q_) * (rNyq * q_);
                const float a2   = A * A;
                const float a4   = a2 * a2;
                const float gNyq = std::sqrt((a4 + a2 * uSq) / (1.0f + a2 * uSq));
                m0 = gNyq;
                m2 = 1.0f - m0;
                const float m2OverK = m2 / std::max(0.05f, k);
                const float rad     = std::max(0.0f, a4 - m2OverK * m2OverK);
                m1 = k * (std::sqrt(rad) - m0);
                break;
            }
            case FilterType::LowShelf: {
                const float A = std::pow(10.0f, gainDb_ / 40.0f);
                g  = g0 / std::sqrt(std::max(0.01f, A));
                m0 = 1.0f; m1 = k * (A - 1.0f); m2 = (A * A - 1.0f);
                break;
            }
            case FilterType::HighShelf: {
                const float A = std::pow(10.0f, gainDb_ / 40.0f);
                g  = g0 * std::sqrt(std::max(0.01f, A));
                m0 = A * A; m1 = k * (1.0f - A) * A; m2 = (1.0f - A * A);
                break;
            }
        }

        // Exact TPT ZDF SVF to Direct-Form II Transposed transfer function mapping:
        // H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
        const float gSq = g * g;
        const float gk  = g * k;
        const float D   = 1.0f / (1.0f + gSq + gk);
        b0_ = (m0 * (1.0f + gSq + gk) + m1 * g + m2 * gSq) * D;
        b1_ = (2.0f * m0 * (gSq - 1.0f) + 2.0f * m2 * gSq) * D;
        b2_ = (m0 * (1.0f + gSq - gk) - m1 * g + m2 * gSq) * D;
        a1_ = 2.0f * (gSq - 1.0f) * D;
        a2_ = (1.0f + gSq - gk) * D;
    }

    double sampleRate_{48000.0};
    bool initialized_{false};
    FilterType type_{FilterType::Lowpass};
    float freqHz_{1000.0f};
    float q_{0.7071f};
    float gainDb_{0.0f};

    float b0_{1.0f}, b1_{0.0f}, b2_{0.0f};
    float a1_{0.0f}, a2_{0.0f};
    float s1_{0.0f}, s2_{0.0f};
};

} // namespace avra::dsp
