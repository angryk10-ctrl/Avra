#pragma once

#include "avra/core/Types.h"
#include <array>
#include <atomic>
#include <span>
#include <algorithm>
#include <cstddef>

namespace avra {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324) // structure padded due to cache-line alignas(64)
#endif

/**
 * @brief Lock-Free Single-Producer Single-Consumer (SPSC) Ring Buffer.
 * 
 * Designed for real-time audio-to-GUI streaming (FFT spectrum analyzers,
 * oscilloscopes, metering) and lock-free inter-thread sample queues.
 * Capacity must be a power of two for branchless bitmask wrapping.
 */
template <typename T, size_t Capacity = 4096>
class LockFreeRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    LockFreeRingBuffer() noexcept {
        buffer_.fill(T{});
    }

    void clear() noexcept {
        writePos_.store(0, std::memory_order_relaxed);
        readPos_.store(0, std::memory_order_relaxed);
        buffer_.fill(T{});
    }

    /// Push a single element (Audio Producer Thread)
    bool push(const T& item) noexcept {
        const size_t w = writePos_.load(std::memory_order_relaxed);
        const size_t r = readPos_.load(std::memory_order_acquire);

        if (w - r >= Capacity) {
            return false; // Buffer full
        }

        buffer_[w & kMask] = item;
        writePos_.store(w + 1, std::memory_order_release);
        return true;
    }

    /// Push a single telemetry sample, overwriting oldest unread data if needed for scope/FFT telemetry
    void pushTelemetrySample(const T& sample) noexcept {
        const size_t w = writePos_.load(std::memory_order_relaxed);
        buffer_[w & kMask] = sample;
        writePos_.store(w + 1, std::memory_order_release);
    }

    /// Push a block of elements, overwriting oldest unread data if needed for scope/FFT telemetry
    void pushTelemetryBlock(std::span<const T> data) noexcept {
        size_t w = writePos_.load(std::memory_order_relaxed);
        for (const T& sample : data) {
            buffer_[w & kMask] = sample;
            ++w;
        }
        writePos_.store(w, std::memory_order_release);
    }

    /// Pop a single element (GUI Consumer Thread)
    bool pop(T& outItem) noexcept {
        const size_t r = readPos_.load(std::memory_order_relaxed);
        const size_t w = writePos_.load(std::memory_order_acquire);

        if (r == w) {
            return false; // Buffer empty
        }

        outItem = buffer_[r & kMask];
        readPos_.store(r + 1, std::memory_order_release);
        return true;
    }

    /// Copy the latest N written elements in chronological order without advancing readPos (ideal for FFT windows & scopes)
    void copyLatest(std::span<T> dst) const noexcept {
        const size_t count = std::min(dst.size(), Capacity);
        const size_t w = writePos_.load(std::memory_order_acquire);
        const size_t start = w - count;

        for (size_t i = 0; i < count; ++i) {
            dst[i] = buffer_[(start + i) & kMask];
        }
    }

    [[nodiscard]] size_t availableToRead() const noexcept {
        const size_t w = writePos_.load(std::memory_order_acquire);
        const size_t r = readPos_.load(std::memory_order_relaxed);
        return (w >= r) ? std::min(w - r, Capacity) : 0;
    }

    [[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }

private:
    static constexpr size_t kMask = Capacity - 1;
    alignas(64) std::atomic<size_t> writePos_{0};
    alignas(64) std::atomic<size_t> readPos_{0};
    std::array<T, Capacity> buffer_{};
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace avra
