#pragma once

#include "avra/core/Smoother.h"
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#include <atomic>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace avra {

enum class ParameterTaper {
  Linear,
  Logarithmic, // Ideal for frequencies (20 Hz - 20 kHz)
  Exponential,
  Stepped // Integer steps / enums
};

/**
 * @brief Thread-safe, lock-free audio plugin parameter.
 *
 * Supports normalized [0..1] and plain unit representation,
 * non-linear tapers, step quantization, custom formatting,
 * and real-time DSP smoothing with zero allocation.
 */
class Parameter {
public:
  struct Config {
    ParameterId id{0};
    std::string name{};
    std::string unit{};
    float minValue{0.0f};
    float maxValue{1.0f};
    float defaultValue{0.0f};
    ParameterTaper taper{ParameterTaper::Linear};
    uint32_t numSteps{0}; // 0 = continuous float
    double smoothingTimeMs{
        15.0}; // Default 15ms smoothing for zipper noise elimination
    std::function<std::string(float plainValue)> textFormatter{nullptr};
  };

  explicit Parameter(Config config)
      : config_(std::move(config)),
        normalizedValue_(plainToNormalized(config_.defaultValue)),
        plainValue_(normalizedToPlain(
            normalizedValue_.load(std::memory_order_relaxed))) {
    smoother_.setTimeMs(config_.smoothingTimeMs);
    smoother_.reset(config_.defaultValue);
  }

  [[nodiscard]] ParameterId getId() const noexcept { return config_.id; }
  [[nodiscard]] const std::string &getName() const noexcept {
    return config_.name;
  }
  [[nodiscard]] const std::string &getUnit() const noexcept {
    return config_.unit;
  }
  [[nodiscard]] float getMinValue() const noexcept { return config_.minValue; }
  [[nodiscard]] float getMaxValue() const noexcept { return config_.maxValue; }
  [[nodiscard]] float getDefaultValue() const noexcept {
    return config_.defaultValue;
  }
  [[nodiscard]] float getDefaultNormalized() const noexcept {
    return plainToNormalized(config_.defaultValue);
  }
  [[nodiscard]] ParameterTaper getTaper() const noexcept {
    return config_.taper;
  }
  [[nodiscard]] uint32_t getNumSteps() const noexcept {
    return config_.numSteps;
  }

  /// Prepare smoother for a given sample rate (called in prepare())
  void prepare(double sampleRate) noexcept {
    smoother_.setSampleRate(sampleRate);
    smoother_.reset(getModulatedPlain());
  }

  /// Read normalized [0..1] value (Lock-free, real-time safe)
  [[nodiscard]] float getNormalized() const noexcept {
    return normalizedValue_.load(std::memory_order_relaxed);
  }

  /// Set normalized [0..1] value (Thread-safe, called by host or GUI)
  void setNormalized(float norm) noexcept {
    norm = math::clamp(norm, 0.0f, 1.0f);
    if (config_.numSteps > 1) {
      const float stepSize = 1.0f / static_cast<float>(config_.numSteps - 1);
      norm = std::round(norm / stepSize) * stepSize;
    }
    normalizedValue_.store(norm, std::memory_order_release);
    plainValue_.store(normalizedToPlain(norm), std::memory_order_release);
    smoother_.setTarget(getModulatedPlain());
  }

  /// Read current base plain value (e.g. +6.0 dB, 1200 Hz)
  [[nodiscard]] float getPlain() const noexcept {
    return plainValue_.load(std::memory_order_relaxed);
  }

  /// Set base plain value
  void setPlain(float plain) noexcept {
    setNormalized(plainToNormalized(plain));
  }

  /// Set non-destructive CLAP / CV parameter modulation offset in plain units
  /// (Lock-free)
  void setModulationOffset(float deltaPlain) noexcept {
    modulationOffset_.store(deltaPlain, std::memory_order_release);
    smoother_.setTarget(getModulatedPlain());
  }

  /// Read current non-destructive modulation offset in plain units
  [[nodiscard]] float getModulationOffset() const noexcept {
    return modulationOffset_.load(std::memory_order_relaxed);
  }

  /// Read effective modulated plain value clamped to [minValue, maxValue]
  [[nodiscard]] float getModulatedPlain() const noexcept {
    const float mod = modulationOffset_.load(std::memory_order_relaxed);
    const float base = plainValue_.load(std::memory_order_relaxed);
    if (mod == 0.0f) {
      return base;
    }
    return math::clamp(base + mod, config_.minValue, config_.maxValue);
  }

  /// Audio thread DSP evaluation: returns next smoothed sample value (including
  /// non-destructive modulation)
  [[nodiscard]] inline float getSmoothed() noexcept {
    // Update smoother target with the latest atomic base + modulation parameter
    // value
    smoother_.setTarget(getModulatedPlain());
    return smoother_.next();
  }

  /// Convert plain value to formatted display string
  [[nodiscard]] std::string getText(float plainValue) const {
    if (config_.textFormatter) {
      return config_.textFormatter(plainValue);
    }

    if (config_.unit.empty()) {
      return std::format("{:.2f}", plainValue);
    }
    return std::format("{:.2f} {}", plainValue, config_.unit);
  }

  [[nodiscard]] std::string getCurrentText() const {
    return getText(getPlain());
  }

  /// Convert normalized [0..1] to plain real-world value
  [[nodiscard]] float normalizedToPlain(float norm) const noexcept {
    norm = math::clamp(norm, 0.0f, 1.0f);

    switch (config_.taper) {
    case ParameterTaper::Linear:
      if (config_.numSteps <= 1 && config_.minValue < -1e-4f &&
          config_.maxValue > 1e-4f &&
          (std::abs(config_.defaultValue) < 1e-4f ||
           std::abs(config_.minValue + config_.maxValue) < 0.01f)) {
        if (std::abs(norm - 0.5f) < 0.004f) {
          return 0.0f;
        }
        if (norm < 0.5f) {
          return math::clamp(config_.minValue * (1.0f - 2.0f * norm),
                             config_.minValue, 0.0f);
        }
        return math::clamp(config_.maxValue * (2.0f * norm - 1.0f), 0.0f,
                           config_.maxValue);
      }
      return math::lerp(config_.minValue, config_.maxValue, norm);

    case ParameterTaper::Logarithmic:
      return math::normalizedToLogFreq(norm, config_.minValue,
                                       config_.maxValue);

    case ParameterTaper::Exponential: {
      // Quadratic curve (y = x^2) for gain/faders
      const float curved = norm * norm;
      return math::lerp(config_.minValue, config_.maxValue, curved);
    }

    case ParameterTaper::Stepped: {
      if (config_.numSteps <= 1)
        return config_.minValue;
      const float stepSize = 1.0f / static_cast<float>(config_.numSteps - 1);
      const auto stepIndex = static_cast<uint32_t>(std::round(norm / stepSize));
      const float stepT = static_cast<float>(stepIndex) /
                          static_cast<float>(config_.numSteps - 1);
      return math::lerp(config_.minValue, config_.maxValue, stepT);
    }
    }
    return config_.minValue;
  }

  /// Convert plain real-world value to normalized [0..1]
  [[nodiscard]] float plainToNormalized(float plain) const noexcept {
    plain = math::clamp(plain, config_.minValue, config_.maxValue);

    switch (config_.taper) {
    case ParameterTaper::Linear:
      if (config_.maxValue == config_.minValue)
        return 0.0f;
      if (config_.numSteps <= 1 && config_.minValue < -1e-4f &&
          config_.maxValue > 1e-4f &&
          (std::abs(config_.defaultValue) < 1e-4f ||
           std::abs(config_.minValue + config_.maxValue) < 0.01f)) {
        if (plain < 0.0f) {
          return math::clamp(0.5f * (1.0f - (plain / config_.minValue)), 0.0f,
                             0.5f);
        }
        return math::clamp(0.5f + 0.5f * (plain / config_.maxValue), 0.5f,
                           1.0f);
      }
      return (plain - config_.minValue) / (config_.maxValue - config_.minValue);

    case ParameterTaper::Logarithmic:
      return math::logFreqToNormalized(plain, config_.minValue,
                                       config_.maxValue);

    case ParameterTaper::Exponential: {
      if (config_.maxValue == config_.minValue)
        return 0.0f;
      const float lin =
          (plain - config_.minValue) / (config_.maxValue - config_.minValue);
      return std::sqrt(std::max(0.0f, lin));
    }

    case ParameterTaper::Stepped: {
      if (config_.maxValue == config_.minValue)
        return 0.0f;
      return (plain - config_.minValue) / (config_.maxValue - config_.minValue);
    }
    }
    return 0.0f;
  }

private:
  Config config_;
  std::atomic<float> normalizedValue_{0.0f};
  std::atomic<float> plainValue_{0.0f};
  std::atomic<float> modulationOffset_{0.0f};
  ExponentialSmoother<float> smoother_;
};

} // namespace avra
