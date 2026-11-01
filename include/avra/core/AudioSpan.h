#pragma once

#include "avra/core/Types.h"
#include <cassert>
#include <cstring>
#include <span>
#include <algorithm>

namespace avra {

/**
 * @brief Zero-allocation multi-channel audio buffer view.
 * 
 * Non-owning view wrapping non-interleaved channel buffers (T* const*).
 * Guaranteed zero heap allocations on construction and slicing.
 */
template <FloatType T>
class AudioSpan {
public:
    constexpr AudioSpan() noexcept
        : channels_(nullptr), numChannels_(0), numFrames_(0) {}

    constexpr AudioSpan(T* const* channels, size_t numChannels, size_t numFrames) noexcept
        : channels_(channels), numChannels_(numChannels), numFrames_(numFrames) {}

    [[nodiscard]] constexpr size_t numChannels() const noexcept { return numChannels_; }
    [[nodiscard]] constexpr size_t numFrames() const noexcept { return numFrames_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return numChannels_ == 0 || numFrames_ == 0; }

    /// Returns a direct std::span for a specific channel
    [[nodiscard]] std::span<T> getChannel(size_t ch) noexcept {
        assert(ch < numChannels_ && channels_ != nullptr);
        return std::span<T>(channels_[ch], numFrames_);
    }

    /// Returns a read-only std::span for a specific channel
    [[nodiscard]] std::span<const T> getChannel(size_t ch) const noexcept {
        assert(ch < numChannels_ && channels_ != nullptr);
        return std::span<const T>(channels_[ch], numFrames_);
    }

    /// Access raw sample: span(channel, frame)
    [[nodiscard]] constexpr T& operator()(size_t ch, size_t frame) noexcept {
        assert(ch < numChannels_ && frame < numFrames_ && channels_ != nullptr);
        return channels_[ch][frame];
    }

    [[nodiscard]] constexpr const T& operator()(size_t ch, size_t frame) const noexcept {
        assert(ch < numChannels_ && frame < numFrames_ && channels_ != nullptr);
        return channels_[ch][frame];
    }

    /// Zero out all samples across all channels
    void clear() noexcept {
        for (size_t ch = 0; ch < numChannels_; ++ch) {
            if (channels_[ch] != nullptr) {
                std::fill_n(channels_[ch], numFrames_, static_cast<T>(0));
            }
        }
    }

    /// Set all samples across all channels to a specific value
    void fill(T value) noexcept {
        for (size_t ch = 0; ch < numChannels_; ++ch) {
            if (channels_[ch] != nullptr) {
                std::fill_n(channels_[ch], numFrames_, value);
            }
        }
    }

    /// Apply linear gain multiplier across all channels
    void applyGain(T gain) noexcept {
        if (gain == static_cast<T>(1)) return;
        if (gain == static_cast<T>(0)) {
            clear();
            return;
        }

        for (size_t ch = 0; ch < numChannels_; ++ch) {
            auto span = getChannel(ch);
            for (auto& s : span) {
                s *= gain;
            }
        }
    }

    /// Copy audio from another AudioSpan
    template <typename OtherT>
    void copyFrom(const AudioSpan<OtherT>& source) noexcept {
        const size_t channelsToCopy = std::min(numChannels_, source.numChannels());
        const size_t framesToCopy = std::min(numFrames_, source.numFrames());

        for (size_t ch = 0; ch < channelsToCopy; ++ch) {
            auto dst = getChannel(ch);
            auto src = source.getChannel(ch);
            std::copy_n(src.data(), framesToCopy, dst.data());
        }
    }

    /// Add audio from another AudioSpan with optional gain (mixing)
    template <typename OtherT>
    void addFrom(const AudioSpan<OtherT>& source, T gain = static_cast<T>(1)) noexcept {
        if (gain == static_cast<T>(0)) return;

        const size_t channelsToMix = std::min(numChannels_, source.numChannels());
        const size_t framesToMix = std::min(numFrames_, source.numFrames());

        for (size_t ch = 0; ch < channelsToMix; ++ch) {
            auto dst = getChannel(ch);
            auto src = source.getChannel(ch);
            for (size_t i = 0; i < framesToMix; ++i) {
                dst[i] += static_cast<T>(src[i]) * gain;
            }
        }
    }

private:
    T* const* channels_{nullptr};
    size_t numChannels_{0};
    size_t numFrames_{0};
};

} // namespace avra
