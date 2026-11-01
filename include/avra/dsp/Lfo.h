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

enum class LfoWaveform {
    Sine,
    Triangle,
    Sawtooth,
    Square
};

/**
 * @brief Multi-waveform Low-Frequency Oscillator for modulation (Chorus, Phaser, Tremolo).
 * Real-time safe, supports quadrature (90 deg) and anti-phase (180 deg) stereo output.
 */
class Lfo {
public:
    Lfo() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        updatePhaseInc();
    }

    void setFrequency(float freqHz) noexcept {
        freqHz_ = math::clamp(freqHz, 0.01f, 40.0f);
        updatePhaseInc();
    }

    void setWaveform(LfoWaveform wave) noexcept { waveform_ = wave; }

    void reset(float initialPhase = 0.0f) noexcept {
        phase_ = initialPhase;
    }

    [[nodiscard]] inline float processSample() noexcept {
        const float out = computeWave(phase_);
        advancePhase(phaseInc_);
        return out;
    }

    /// Process stereo pair with configurable phase offset (default 180 deg for anti-phase stereo width)
    void processStereo(float& outL, float& outR, float phaseOffsetRad = kPi) noexcept {
        outL = computeWave(phase_);

        if (waveform_ == LfoWaveform::Sine && std::abs(phaseOffsetRad - kPi) < 1e-4f) {
            outR = -outL;
        } else {
            float phaseR = phase_ + phaseOffsetRad;
            while (phaseR >= kTwoPi) phaseR -= kTwoPi;
            outR = computeWave(phaseR);
        }

        advancePhase(phaseInc_);
    }

private:
    void updatePhaseInc() noexcept {
        phaseInc_ = (kTwoPi * freqHz_) / static_cast<float>(sampleRate_);
    }

    inline void advancePhase(float inc) noexcept {
        phase_ += inc;
        if (phase_ >= kTwoPi) phase_ -= kTwoPi;
    }

    [[nodiscard]] inline float computeWave(float ph) const noexcept {
        switch (waveform_) {
            case LfoWaveform::Sine:
                return std::sin(ph);

            case LfoWaveform::Triangle: {
                // Normalized [0..1]
                const float norm = ph / kTwoPi;
                return 4.0f * std::abs(norm - 0.5f) - 1.0f;
            }

            case LfoWaveform::Sawtooth: {
                const float norm = ph / kTwoPi;
                return 2.0f * norm - 1.0f;
            }

            case LfoWaveform::Square:
                return (ph < kPi) ? 1.0f : -1.0f;
        }
        return 0.0f;
    }

    double sampleRate_{48000.0};
    float freqHz_{1.0f};
    LfoWaveform waveform_{LfoWaveform::Sine};
    float phase_{0.0f};
    float phaseInc_{0.0f};
};

} // namespace avra::dsp
