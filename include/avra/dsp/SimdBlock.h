#pragma once

#if __has_include("avra/core/Types.h")
#include "avra/core/Types.h"
#include "avra/dsp/Math.h"
#else
#include "../core/Types.h"
#include "Math.h"
#endif

#include <cmath>
#include <cstring>
#include <algorithm>

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#define AVRA_HAS_SSE2 1
#else
#define AVRA_HAS_SSE2 0
#endif

namespace avra::dsp {

/**
 * @brief High-throughput SIMD-accelerated audio block operations.
 * Uses hardware SSE2 vector instructions on x86_64 with scalar tail fallbacks.
 */
struct SimdBlock {
    static inline void clear(float* dst, size_t numSamples) noexcept {
        if (!dst || numSamples == 0) return;
        std::memset(dst, 0, numSamples * sizeof(float));
    }

    static inline void copy(float* dst, const float* src, size_t numSamples) noexcept {
        if (!dst || !src || numSamples == 0 || dst == src) return;
        std::memcpy(dst, src, numSamples * sizeof(float));
    }

    static inline void multiply(float* dst, float gain, size_t numSamples) noexcept {
        size_t i = 0;
#if AVRA_HAS_SSE2
        const __m128 vGain = _mm_set1_ps(gain);
        for (; i + 4 <= numSamples; i += 4) {
            __m128 v = _mm_loadu_ps(dst + i);
            _mm_storeu_ps(dst + i, _mm_mul_ps(v, vGain));
        }
#endif
        for (; i < numSamples; ++i) {
            dst[i] *= gain;
        }
    }

    static inline void add(float* dst, const float* src, size_t numSamples) noexcept {
        size_t i = 0;
#if AVRA_HAS_SSE2
        for (; i + 4 <= numSamples; i += 4) {
            __m128 vDst = _mm_loadu_ps(dst + i);
            __m128 vSrc = _mm_loadu_ps(src + i);
            _mm_storeu_ps(dst + i, _mm_add_ps(vDst, vSrc));
        }
#endif
        for (; i < numSamples; ++i) {
            dst[i] += src[i];
        }
    }

    static inline void multiplyAdd(float* dst, const float* src, float gain, size_t numSamples) noexcept {
        size_t i = 0;
#if AVRA_HAS_SSE2
        const __m128 vGain = _mm_set1_ps(gain);
        for (; i + 4 <= numSamples; i += 4) {
            __m128 vDst = _mm_loadu_ps(dst + i);
            __m128 vSrc = _mm_loadu_ps(src + i);
            _mm_storeu_ps(dst + i, _mm_add_ps(vDst, _mm_mul_ps(vSrc, vGain)));
        }
#endif
        for (; i < numSamples; ++i) {
            dst[i] += src[i] * gain;
        }
    }

    static inline void computePeakAndRms(const float* src, size_t numSamples, float& outPeak, float& outRms) noexcept {
        if (!src || numSamples == 0) {
            outPeak = 0.0f;
            outRms = 0.0f;
            return;
        }

        float peak = 0.0f;
        float sumSq = 0.0f;
        size_t i = 0;

#if AVRA_HAS_SSE2
        __m128 vPeak = _mm_setzero_ps();
        __m128 vSumSq = _mm_setzero_ps();
        const __m128 vAbsMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));

        for (; i + 4 <= numSamples; i += 4) {
            __m128 v = _mm_loadu_ps(src + i);
            __m128 vAbs = _mm_and_ps(v, vAbsMask);
            vPeak = _mm_max_ps(vPeak, vAbs);
            vSumSq = _mm_add_ps(vSumSq, _mm_mul_ps(v, v));
        }

        alignas(16) float pArr[4];
        alignas(16) float sArr[4];
        _mm_store_ps(pArr, vPeak);
        _mm_store_ps(sArr, vSumSq);
        peak = std::max({ pArr[0], pArr[1], pArr[2], pArr[3] });
        sumSq = sArr[0] + sArr[1] + sArr[2] + sArr[3];
#endif

        for (; i < numSamples; ++i) {
            const float s = src[i];
            const float a = std::abs(s);
            if (a > peak) peak = a;
            sumSq += s * s;
        }

        outPeak = peak;
        outRms = std::sqrt(sumSq / static_cast<float>(numSamples));
    }
};

} // namespace avra::dsp
