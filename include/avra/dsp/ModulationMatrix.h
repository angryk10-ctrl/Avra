#pragma once

#if __has_include("avra/core/ParameterTree.h")
#include "avra/core/ParameterTree.h"
#else
#include "../core/ParameterTree.h"
#endif

#include <array>
#include <atomic>

namespace avra::dsp {

/**
 * @brief Lock-Free Real-Time Parameter Modulation Matrix.
 * 
 * Allows routing modulation sources (LFOs, Envelope Followers, Macros, MIDI)
 * to any ParameterId with bipolar modulation depth [-1.0 .. +1.0] without
 * mutating the base parameter value set by the user or DAW automation.
 */
class ModulationMatrix {
public:
    static constexpr size_t kMaxSources = 8;
    static constexpr size_t kMaxRoutes  = 32;

    struct Route {
        uint32_t sourceIndex{0};
        ParameterId targetParamId{0};
        float depth{0.0f}; // Bipolar [-1.0 .. +1.0] in normalized parameter space
        bool enabled{false};
    };

    ModulationMatrix() = default;

    void clearRoutes() noexcept {
        numRoutes_ = 0;
        for (auto& r : routes_) r = Route{};
    }

    bool addRoute(uint32_t sourceIndex, ParameterId targetParamId, float depth) noexcept {
        if (sourceIndex >= kMaxSources || numRoutes_ >= kMaxRoutes) return false;
        routes_[numRoutes_++] = Route{
            .sourceIndex = sourceIndex,
            .targetParamId = targetParamId,
            .depth = math::clamp(depth, -1.0f, 1.0f),
            .enabled = true
        };
        return true;
    }

    /// Set the current output value [-1.0 .. +1.0] of a modulation source on the audio thread
    inline void setSourceValue(uint32_t sourceIndex, float value) noexcept {
        if (sourceIndex < kMaxSources) {
            sourceValues_[sourceIndex].store(math::clamp(value, -1.0f, 1.0f), std::memory_order_relaxed);
        }
    }

    [[nodiscard]] inline float getSourceValue(uint32_t sourceIndex) const noexcept {
        if (sourceIndex < kMaxSources) {
            return sourceValues_[sourceIndex].load(std::memory_order_relaxed);
        }
        return 0.0f;
    }

    /// Evaluate the modulated plain value of a parameter on the audio thread
    [[nodiscard]] inline float getModulatedPlain(const Parameter& param) const noexcept {
        float norm = param.getNormalized();
        const ParameterId pid = param.getId();

        for (size_t i = 0; i < numRoutes_; ++i) {
            const auto& r = routes_[i];
            if (r.enabled && r.targetParamId == pid) {
                norm += sourceValues_[r.sourceIndex].load(std::memory_order_relaxed) * r.depth;
            }
        }

        return param.normalizedToPlain(math::clamp(norm, 0.0f, 1.0f));
    }

private:
    std::array<std::atomic<float>, kMaxSources> sourceValues_{};
    std::array<Route, kMaxRoutes> routes_{};
    size_t numRoutes_{0};
};

} // namespace avra::dsp
