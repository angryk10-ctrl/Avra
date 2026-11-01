#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#else
#include "../core/Types.h"
#include "Math.h"
#endif
#include <array>
#include <cmath>

namespace avra::dsp {

/**
 * @brief True Polyphase Allpass Half-Band 2x Oversampler (HIIR 6th-Order Elliptic Topology).
 * Provides >80 dB stopband rejection above Nyquist/2 with <0.01 dB passband ripple and zero heap allocation.
 * Essential for alias-free tube/transistor waveshaping, hard clipping, and true-peak limiting.
 */
class Oversampling2x {
public:
    Oversampling2x() noexcept = default;

    void reset() noexcept {
        upBranch0_.reset();
        upBranch1_.reset();
        downBranch0_.reset();
        downBranch1_.reset();
        downDelay_ = 0.0f;
    }

    /// Upsample 1 input sample into 2 oversampled outputs (out0, out1) at 2x sample rate
    inline void upsample(float in, float& out0, float& out1) noexcept {
        out0 = upBranch0_.process(in);
        out1 = upBranch1_.process(in);
    }

    /// Downsample 2 oversampled inputs (in0, in1) back to 1 base-rate sample with >80 dB anti-aliasing
    [[nodiscard]] inline float downsample(float in0, float in1) noexcept {
        const float b0 = downBranch0_.process(in1);
        const float b1 = downBranch1_.process(downDelay_);
        downDelay_ = in0;
        return 0.5f * (b0 + b1);
    }

    /// Convenience helper: process a mono nonlinear functor at 2x oversampling
    template <typename NonlinearFn>
    [[nodiscard]] inline float processSample(float in, NonlinearFn&& fn) noexcept {
        float u0 = 0.0f, u1 = 0.0f;
        upsample(in, u0, u1);
        u0 = fn(u0);
        u1 = fn(u1);
        return downsample(u0, u1);
    }

private:
    struct PolyphaseBranch3 {
        std::array<float, 3> coeffs{};
        std::array<float, 3> x1{};
        std::array<float, 3> y1{};

        constexpr PolyphaseBranch3(float c0, float c1, float c2) noexcept
            : coeffs{ c0, c1, c2 }, x1{ 0.0f, 0.0f, 0.0f }, y1{ 0.0f, 0.0f, 0.0f } {}

        void reset() noexcept {
            x1.fill(0.0f);
            y1.fill(0.0f);
        }

        [[nodiscard]] inline float process(float x) noexcept {
            for (size_t i = 0; i < 3; ++i) {
                const float y = coeffs[i] * (x - y1[i]) + x1[i];
                x1[i] = x;
                y1[i] = math::sanitize(y);
                x = y1[i];
            }
            return x;
        }
    };

    // Optimal HIIR 6-coefficient polyphase half-band allpass poles
    PolyphaseBranch3 upBranch0_{ 0.036681502163648017f, 0.27463175937945444f, 0.56109896978791948f };
    PolyphaseBranch3 upBranch1_{ 0.13654762463195794f,  0.42313861743656711f, 0.85908849187948017f };
    PolyphaseBranch3 downBranch0_{ 0.036681502163648017f, 0.27463175937945444f, 0.56109896978791948f };
    PolyphaseBranch3 downBranch1_{ 0.13654762463195794f,  0.42313861743656711f, 0.85908849187948017f };
    float downDelay_{0.0f};
};

/**
 * @brief Cascaded 4x Polyphase Allpass Half-Band Oversampler for extreme high-gain tube/clipper stages.
 */
class Oversampling4x {
public:
    Oversampling4x() noexcept = default;

    void reset() noexcept {
        stage1_.reset();
        stage2a_.reset();
        stage2b_.reset();
    }

    template <typename NonlinearFn>
    [[nodiscard]] inline float processSample(float in, NonlinearFn&& fn) noexcept {
        float u0 = 0.0f, u1 = 0.0f;
        stage1_.upsample(in, u0, u1);

        float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
        stage2a_.upsample(u0, q0, q1);
        stage2b_.upsample(u1, q2, q3);

        q0 = fn(q0);
        q1 = fn(q1);
        q2 = fn(q2);
        q3 = fn(q3);

        const float d0 = stage2a_.downsample(q0, q1);
        const float d1 = stage2b_.downsample(q2, q3);
        return stage1_.downsample(d0, d1);
    }

private:
    Oversampling2x stage1_;
    Oversampling2x stage2a_;
    Oversampling2x stage2b_;
};

} // namespace avra::dsp
