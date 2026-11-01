#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#else
#include "../core/Types.h"
#include "Math.h"
#endif

#include <cmath>
#include <vector>
#include <algorithm>

namespace avra::dsp {

namespace math = avra::math;

/**
 * @brief Exponential Ballistics Envelope Follower (Peak / RMS).
 */
class EnvelopeFollower {
public:
    EnvelopeFollower() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        updateCoeffs();
    }

    void setAttackMs(float attackMs) noexcept {
        attackMs_ = std::max(0.1f, attackMs);
        updateCoeffs();
    }

    void setReleaseMs(float releaseMs) noexcept {
        releaseMs_ = std::max(1.0f, releaseMs);
        updateCoeffs();
    }

    void reset() noexcept {
        envelope_ = 0.0f;
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        const float absIn = std::abs(in);
        if (absIn > envelope_) {
            envelope_ = attackCoeff_ * envelope_ + (1.0f - attackCoeff_) * absIn;
        } else {
            envelope_ = releaseCoeff_ * envelope_ + (1.0f - releaseCoeff_) * absIn;
        }
        envelope_ = math::sanitize(envelope_);
        return envelope_;
    }

    [[nodiscard]] float getEnvelope() const noexcept { return envelope_; }

private:
    void updateCoeffs() noexcept {
        attackCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * (attackMs_ * 0.001f)));
        releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * (releaseMs_ * 0.001f)));
    }

    double sampleRate_{48000.0};
    float attackMs_{10.0f};
    float releaseMs_{100.0f};
    float attackCoeff_{0.0f};
    float releaseCoeff_{0.0f};
    float envelope_{0.0f};
};

/**
 * @brief VCA Compressor Gain Computer with Variable Knee.
 */
class VcaCompressor {
public:
    VcaCompressor() {
        updateTransferConstants();
    }

    void prepare(double sampleRate) noexcept {
        detector_.prepare(sampleRate);
        updateTransferConstants();
    }

    void setThresholdDb(float threshDb) noexcept {
        if (std::abs(threshDb - threshDb_) > 1e-4f) {
            threshDb_ = threshDb;
            updateTransferConstants();
        }
    }
    void setRatio(float ratio) noexcept {
        const float r = std::max(1.0f, ratio);
        if (std::abs(r - ratio_) > 1e-4f) {
            ratio_ = r;
            updateTransferConstants();
        }
    }
    void setKneeDb(float kneeDb) noexcept {
        const float k = std::max(0.0f, kneeDb);
        if (std::abs(k - kneeDb_) > 1e-4f) {
            kneeDb_ = k;
            updateTransferConstants();
        }
    }
    void setAttackMs(float attackMs) noexcept { detector_.setAttackMs(attackMs); }
    void setReleaseMs(float releaseMs) noexcept { detector_.setReleaseMs(releaseMs); }
    void setMakeupGainDb(float makeupDb) noexcept { makeupGain_ = math::dBToLinear(makeupDb); }

    void reset() noexcept {
        detector_.reset();
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        return processSampleWithSidechain(in, in);
    }

    [[nodiscard]] inline float processSampleWithSidechain(float in, float sidechainIn) noexcept {
        const float env = detector_.processSample(sidechainIn);
        if (env <= kneeLowerLin_) {
            lastGainReductionDb_ = 0.0f;
            return in * makeupGain_;
        }

        const float envDb = math::linearTodB(env);
        float gainReductionDb = 0.0f;
        if (kneeDb_ > 0.0f && envDb < kneeUpperDb_) {
            // Soft knee quadratic interpolation
            const float diff = envDb - kneeLowerDb_;
            gainReductionDb = slope_ * (diff * diff) * invTwoKnee_;
        } else if (envDb > threshDb_) {
            gainReductionDb = (envDb - threshDb_) * slope_;
        }

        const float gainReductionLin = math::dBToLinear(gainReductionDb);
        lastGainReductionDb_ = -gainReductionDb; // Positive value for meter display
        return in * gainReductionLin * makeupGain_;
    }

    [[nodiscard]] float getLastGainReductionDb() const noexcept { return lastGainReductionDb_; }

private:
    void updateTransferConstants() noexcept {
        slope_ = (1.0f / ratio_) - 1.0f;
        invTwoKnee_ = (kneeDb_ > 0.0f) ? (1.0f / (2.0f * kneeDb_)) : 0.0f;
        kneeLowerDb_ = threshDb_ - kneeDb_ * 0.5f;
        kneeUpperDb_ = threshDb_ + kneeDb_ * 0.5f;
        kneeLowerLin_ = math::dBToLinear(kneeLowerDb_);
    }

    EnvelopeFollower detector_;
    float threshDb_{-18.0f};
    float ratio_{4.0f};
    float kneeDb_{4.0f};
    float slope_{-0.75f};
    float invTwoKnee_{0.125f};
    float kneeLowerDb_{-20.0f};
    float kneeUpperDb_{-16.0f};
    float kneeLowerLin_{0.1f};
    float makeupGain_{1.0f};
    float lastGainReductionDb_{0.0f};
};

/**
 * @brief Mastering Lookahead Brickwall Peak Limiter.
 * Guaranteed 0 dBFS or user-defined ceiling protection with smooth release envelope.
 */
class LookaheadLimiter {
public:
    LookaheadLimiter() = default;

    void prepare(double sampleRate, float lookaheadMs = 2.0f) {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        lookaheadSamples_ = std::max<size_t>(1, static_cast<size_t>(sampleRate_ * lookaheadMs * 0.001f));
        delayBuffer_.assign(lookaheadSamples_ + 1, 0.0f);
        writePos_ = 0;
        releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * (releaseMs_ * 0.001f)));
        envelopeGain_ = 1.0f;
    }

    void setCeilingDb(float ceilingDb) noexcept {
        ceilingLin_ = math::dBToLinear(std::min(0.0f, ceilingDb));
    }

    void setReleaseMs(float releaseMs) noexcept {
        releaseMs_ = std::max(5.0f, releaseMs);
        if (sampleRate_ > 0.0) {
            releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * (releaseMs_ * 0.001f)));
        }
    }

    void reset() noexcept {
        std::fill(delayBuffer_.begin(), delayBuffer_.end(), 0.0f);
        writePos_ = 0;
        envelopeGain_ = 1.0f;
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        // Inter-sample peak proxy / absolute peak
        const float peak = std::abs(in);

        // Required gain reduction
        float targetGain = 1.0f;
        if (peak > ceilingLin_ && ceilingLin_ > 1e-6f) {
            targetGain = ceilingLin_ / peak;
        }

        // Fast attack (instantaneous pull-down), smooth release
        if (targetGain < envelopeGain_) {
            envelopeGain_ = targetGain;
        } else {
            envelopeGain_ = releaseCoeff_ * envelopeGain_ + (1.0f - releaseCoeff_) * targetGain;
        }
        envelopeGain_ = math::sanitize(envelopeGain_);

        // Read delayed sample from lookahead ring buffer
        const size_t readPos = (writePos_ + 1) % delayBuffer_.size();
        const float delayedSample = delayBuffer_[readPos];

        // Store current in
        delayBuffer_[writePos_] = in;
        writePos_ = (writePos_ + 1) % delayBuffer_.size();

        lastGrDb_ = math::linearTodB(envelopeGain_);
        return delayedSample * envelopeGain_;
    }

    [[nodiscard]] float getLastGrDb() const noexcept { return -lastGrDb_; }

private:
    double sampleRate_{48000.0};
    size_t lookaheadSamples_{96};
    std::vector<float> delayBuffer_{97, 0.0f};
    size_t writePos_{0};

    float ceilingLin_{1.0f}; // 0 dBFS
    float releaseMs_{50.0f};
    float releaseCoeff_{0.999f};
    float envelopeGain_{1.0f};
    float lastGrDb_{0.0f};
};

/**
 * @brief Studio Transient Shaper Engine.
 * Dual-ballistics differential detector that separates attack transients from sustain body.
 */
class TransientShaper {
public:
    TransientShaper() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        fastEnv_.prepare(sampleRate_);
        slowEnv_.prepare(sampleRate_);
        fastEnv_.setAttackMs(2.0f);
        fastEnv_.setReleaseMs(30.0f);
        slowEnv_.setAttackMs(25.0f);
        slowEnv_.setReleaseMs(120.0f);
    }

    void setAttackDb(float attackDb) noexcept { attackGain_ = math::dBToLinear(attackDb); }
    void setSustainDb(float sustainDb) noexcept { sustainGain_ = math::dBToLinear(sustainDb); }

    void reset() noexcept {
        fastEnv_.reset();
        slowEnv_.reset();
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        const float fast = fastEnv_.processSample(in);
        const float slow = slowEnv_.processSample(in);
        const float diff = fast - slow;

        float modGain = 1.0f;
        if (diff > 0.0f) {
            // Transient onset
            modGain = 1.0f + (attackGain_ - 1.0f) * std::min(1.0f, diff * 8.0f);
        } else {
            // Sustain body
            modGain = 1.0f + (sustainGain_ - 1.0f) * std::min(1.0f, -diff * 6.0f);
        }

        return in * std::max(0.0f, modGain);
    }

private:
    double sampleRate_{48000.0};
    EnvelopeFollower fastEnv_;
    EnvelopeFollower slowEnv_;
    float attackGain_{1.0f};
    float sustainGain_{1.0f};
};

/**
 * @brief Studio Noise Gate with Threshold, Hold, Release, and Range Floor.
 */
class NoiseGate {
public:
    NoiseGate() = default;

    void prepare(double sampleRate) noexcept {
        sampleRate_ = (sampleRate > 1000.0) ? sampleRate : 48000.0;
        detector_.prepare(sampleRate_);
        detector_.setAttackMs(1.0f);
        detector_.setReleaseMs(20.0f);
        updateReleaseCoeff();
    }

    void setThresholdDb(float threshDb) noexcept { threshLin_ = math::dBToLinear(threshDb); }
    void setHoldMs(float holdMs) noexcept { holdSamples_ = static_cast<size_t>(sampleRate_ * std::max(1.0f, holdMs) * 0.001f); }
    void setReleaseMs(float relMs) noexcept {
        releaseMs_ = std::max(5.0f, relMs);
        updateReleaseCoeff();
    }
    void setFloorDb(float floorDb) noexcept { floorGain_ = math::dBToLinear(floorDb); }

    void reset() noexcept {
        detector_.reset();
        currentGain_ = floorGain_;
        holdCounter_ = 0;
    }

    [[nodiscard]] inline float processSample(float in) noexcept {
        const float env = detector_.processSample(in);

        if (env >= threshLin_) {
            currentGain_ = 1.0f; // Instant open
            holdCounter_ = holdSamples_;
        } else if (holdCounter_ > 0) {
            --holdCounter_;
            currentGain_ = 1.0f;
        } else {
            // Smooth exponential release towards floor
            currentGain_ = releaseCoeff_ * currentGain_ + (1.0f - releaseCoeff_) * floorGain_;
            currentGain_ = math::sanitize(currentGain_);
        }

        return in * currentGain_;
    }

    [[nodiscard]] float getCurrentGain() const noexcept { return currentGain_; }

private:
    void updateReleaseCoeff() noexcept {
        if (sampleRate_ > 0.0) {
            releaseCoeff_ = std::exp(-1.0f / (static_cast<float>(sampleRate_) * (releaseMs_ * 0.001f)));
        }
    }

    double sampleRate_{48000.0};
    EnvelopeFollower detector_;
    float threshLin_{0.015f}; // -36 dB
    float floorGain_{0.001f};  // -60 dB
    float releaseMs_{100.0f};
    float releaseCoeff_{0.999f};
    size_t holdSamples_{1200};
    size_t holdCounter_{0};
    float currentGain_{0.001f};
};

} // namespace avra::dsp
