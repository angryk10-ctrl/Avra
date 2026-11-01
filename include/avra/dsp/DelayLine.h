#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#else
#include "../core/Types.h"
#include "Math.h"
#endif

#include <vector>
#include <cmath>
#include <algorithm>

namespace avra::dsp {

namespace math = avra::math;

/**
 * @brief High-precision Fractional Audio Delay Line.
 * Supports linear and cubic Hermite interpolation for smooth, click-free pitch and chorus modulation.
 */
class DelayLine {
public:
    DelayLine() = default;

    explicit DelayLine(size_t maxDelaySamples) {
        init(maxDelaySamples);
    }

    void init(size_t maxDelaySamples) {
        size_t cap = 64;
        const size_t needed = maxDelaySamples + 8;
        while (cap < needed) {
            cap <<= 1;
        }
        buffer_.assign(cap, 0.0f);
        writeIndex_ = 0;
        mask_ = cap - 1;
    }

    void reset() noexcept {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writeIndex_ = 0;
    }

    inline void write(float sample) noexcept {
        buffer_[writeIndex_] = sample;
        writeIndex_ = (writeIndex_ + 1) & mask_;
    }

    [[nodiscard]] inline float readLinear(float delaySamples) const noexcept {
        const size_t m = mask_;
        const float maxD = static_cast<float>(m - 2);
        const float d = math::clamp(delaySamples, 0.0f, maxD);
        const float readPos = static_cast<float>(writeIndex_ + m + 1) - d;

        const size_t idx0 = static_cast<size_t>(readPos);
        const float frac = readPos - static_cast<float>(idx0);

        return math::lerp(buffer_[idx0 & m], buffer_[(idx0 + 1) & m], frac);
    }

    [[nodiscard]] inline float readHermite(float delaySamples) const noexcept {
        const size_t m = mask_;
        const float maxD = static_cast<float>(m - 4);
        const float d = math::clamp(delaySamples, 1.0f, maxD);
        const float readPos = static_cast<float>(writeIndex_ + m + 1) - d;

        const size_t i1 = static_cast<size_t>(readPos);
        const float frac = readPos - static_cast<float>(i1);

        const float y0 = buffer_[(i1 - 1) & m];
        const float y1 = buffer_[i1 & m];
        const float y2 = buffer_[(i1 + 1) & m];
        const float y3 = buffer_[(i1 + 2) & m];

        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

private:
    std::vector<float> buffer_{64, 0.0f};
    size_t writeIndex_{0};
    size_t mask_{63};
};

} // namespace avra::dsp
