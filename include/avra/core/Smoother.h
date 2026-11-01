#pragma once

#include "avra/core/Types.h"
#include <cmath>
#include <algorithm>

namespace avra {

/**
 * @brief Sample-accurate linear parameter ramp smoother.
 * Traverses from current value to target value over a specified sample duration.
 */
template <FloatType T>
class LinearSmoother {
public:
    constexpr LinearSmoother() noexcept = default;

    void reset(T initialValue) noexcept {
        current_ = initialValue;
        target_ = initialValue;
        step_ = static_cast<T>(0);
        remainingSamples_ = 0;
    }

    void setTarget(T target, size_t durationInSamples) noexcept {
        target_ = target;
        if (durationInSamples == 0) {
            current_ = target;
            step_ = static_cast<T>(0);
            remainingSamples_ = 0;
            return;
        }

        remainingSamples_ = durationInSamples;
        step_ = (target_ - current_) / static_cast<T>(durationInSamples);
    }

    [[nodiscard]] inline T next() noexcept {
        if (remainingSamples_ > 0) {
            current_ += step_;
            --remainingSamples_;
            if (remainingSamples_ == 0) {
                current_ = target_;
            }
        }
        return current_;
    }

    [[nodiscard]] constexpr T getCurrent() const noexcept { return current_; }
    [[nodiscard]] constexpr T getTarget() const noexcept { return target_; }
    [[nodiscard]] constexpr bool isSmoothing() const noexcept { return remainingSamples_ > 0; }

private:
    T current_{static_cast<T>(0)};
    T target_{static_cast<T>(0)};
    T step_{static_cast<T>(0)};
    size_t remainingSamples_{0};
};

/**
 * @brief One-pole exponential smoother (analog-style parameter filter).
 * Provides zipper-free, continuous musical parameter smoothing.
 */
template <FloatType T>
class ExponentialSmoother {
public:
    constexpr ExponentialSmoother() noexcept = default;

    void setSampleRate(double sampleRate) noexcept {
        sampleRate_ = sampleRate;
        updateCoefficient();
    }

    void setTimeMs(double timeMs) noexcept {
        timeMs_ = timeMs;
        updateCoefficient();
    }

    void reset(T initialValue) noexcept {
        current_ = initialValue;
        target_ = initialValue;
    }

    void setTarget(T target) noexcept {
        target_ = target;
    }

    [[nodiscard]] inline T next() noexcept {
        // y[n] = y[n-1] + alpha * (target - y[n-1])
        current_ += alpha_ * (target_ - current_);
        return current_;
    }

    [[nodiscard]] constexpr T getCurrent() const noexcept { return current_; }
    [[nodiscard]] constexpr T getTarget() const noexcept { return target_; }

private:
    void updateCoefficient() noexcept {
        if (sampleRate_ <= 0.0 || timeMs_ <= 0.0) {
            alpha_ = static_cast<T>(1);
            return;
        }
        // Time constant tau = timeMs / 1000. alpha = 1 - exp(-1 / (sampleRate * tau))
        const double tau = (timeMs_ * 0.001);
        alpha_ = static_cast<T>(1.0 - std::exp(-1.0 / (sampleRate_ * tau)));
    }

    double sampleRate_{48000.0};
    double timeMs_{20.0}; // default 20ms smoothing
    T alpha_{static_cast<T>(0.05)};
    T current_{static_cast<T>(0)};
    T target_{static_cast<T>(0)};
};

} // namespace avra
