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
 * @brief 1st-order Allpass Filter Stage for analog phaser sweeps.
 */
class AllpassStage {
public:
    AllpassStage() = default;

    void setFrequency(float freqHz, double sampleRate) noexcept {
        const float w = kPi * freqHz / static_cast<float>(sampleRate);
        const float tanW = std::tan(w);
        a1_ = (tanW - 1.0f) / (tanW + 1.0f);
    }

    void reset() noexcept {
        x1_ = 0.0f;
        y1_ = 0.0f;
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        const float out = a1_ * in + x1_ - a1_ * y1_;
        x1_ = in;
        y1_ = math::sanitize(out);
        return out;
    }

private:
    float a1_{0.0f};
    float x1_{0.0f};
    float y1_{0.0f};
};

/**
 * @brief 4-Stage Analog Phaser Engine with Resonant Feedback.
 */
class PhaserEngine {
public:
    PhaserEngine() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        reset();
    }

    void setParameters(float rateHz, float depth, float feedback, float lfoPhaseOffset = 0.0f) noexcept {
        (void)rateHz;
        depth_ = math::clamp(depth, 0.0f, 1.0f);
        feedback_ = math::clamp(feedback, 0.0f, 0.95f);
        lfoPhaseOffset_ = lfoPhaseOffset;
    }

    void reset() noexcept {
        for (auto& st : stagesL_) st.reset();
        for (auto& st : stagesR_) st.reset();
        feedbackSampleL_ = 0.0f;
        feedbackSampleR_ = 0.0f;
    }

    inline void processStereo(float& left, float& right, float lfoValL, float lfoValR) noexcept {
        // Map LFO [-1..1] to sweep frequency range [300 Hz .. 4500 Hz]
        const float freqL = 1200.0f * std::pow(3.5f, lfoValL * depth_);
        const float freqR = 1200.0f * std::pow(3.5f, lfoValR * depth_);

        for (auto& st : stagesL_) st.setFrequency(freqL, sampleRate_);
        for (auto& st : stagesR_) st.setFrequency(freqR, sampleRate_);

        float inL = left + feedbackSampleL_ * feedback_;
        float inR = right + feedbackSampleR_ * feedback_;

        for (auto& st : stagesL_) inL = st.processSample(inL);
        for (auto& st : stagesR_) inR = st.processSample(inR);

        feedbackSampleL_ = inL;
        feedbackSampleR_ = inR;

        // Comb phase notch combination
        left  = 0.5f * (left + inL);
        right = 0.5f * (right + inR);
    }

private:
    double sampleRate_{48000.0};
    AllpassStage stagesL_[4];
    AllpassStage stagesR_[4];
    float depth_{0.75f};
    float feedback_{0.5f};
    float lfoPhaseOffset_{0.0f};
    float feedbackSampleL_{0.0f};
    float feedbackSampleR_{0.0f};
};

} // namespace avra::dsp
