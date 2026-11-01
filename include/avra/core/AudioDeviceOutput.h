#pragma once

/**
 * @file AudioDeviceOutput.h
 * @brief Zero-dependency Windows real-time stereo audio output engine (winmm waveOut),
 *        RIFF .WAV loop loader, and 48 kHz studio drum/music groove generator.
 */

#include <array>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

#if defined(_WIN32)
#include <windows.h>
#include <mmsystem.h>
#endif

namespace avra::audio_io {

struct AudioDeviceInfo {
    int         deviceId{-1};
    std::string name{};
    int         channels{2};
};

inline std::string wideToUtf8DeviceString(const wchar_t* wstr) {
#if defined(_WIN32)
    if (!wstr || !wstr[0]) return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out.data(), len, nullptr, nullptr);
    return out;
#else
    (void)wstr;
    return {};
#endif
}

inline std::vector<AudioDeviceInfo> enumerateInputDevices() {
    static std::vector<AudioDeviceInfo> s_cachedIn;
    static uint64_t s_lastInTick = 0;
#if defined(_WIN32)
    const uint64_t nowIn = GetTickCount64();
    if (!s_cachedIn.empty() && (nowIn - s_lastInTick) < 1500) {
        return s_cachedIn;
    }
    s_lastInTick = nowIn;
#endif
    std::vector<AudioDeviceInfo> list;
    list.push_back({ -1, "None (Muted / Audition Only)", 0 });
#if defined(_WIN32)
    const UINT numDevs = waveInGetNumDevs();
    for (UINT i = 0; i < numDevs; ++i) {
        WAVEINCAPSW caps{};
        if (waveInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            std::string devName = wideToUtf8DeviceString(caps.szPname);
            if (devName.empty()) devName = "Recording Input " + std::to_string(i + 1);
            list.push_back({ static_cast<int>(i), devName, std::max<int>(1, caps.wChannels) });
        }
    }
#endif
    s_cachedIn = list;
    return list;
}

inline std::vector<AudioDeviceInfo> enumerateOutputDevices() {
    static std::vector<AudioDeviceInfo> s_cachedOut;
    static uint64_t s_lastOutTick = 0;
#if defined(_WIN32)
    const uint64_t nowOut = GetTickCount64();
    if (!s_cachedOut.empty() && (nowOut - s_lastOutTick) < 1500) {
        return s_cachedOut;
    }
    s_lastOutTick = nowOut;
#endif
    std::vector<AudioDeviceInfo> list;
    list.push_back({ -1, "System Default Output (WAVE_MAPPER)", 2 });
#if defined(_WIN32)
    const UINT numDevs = waveOutGetNumDevs();
    for (UINT i = 0; i < numDevs; ++i) {
        WAVEOUTCAPSW caps{};
        if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            std::string devName = wideToUtf8DeviceString(caps.szPname);
            while (!devName.empty() && devName.back() == ' ') {
                devName.pop_back();
            }
            if (devName.empty()) devName = "Playback Output " + std::to_string(i + 1);
            list.push_back({ static_cast<int>(i), devName, std::max<int>(2, caps.wChannels) });
        }
    }
    for (size_t i = 1; i < list.size(); ++i) {
        int totalSame = 0;
        for (size_t j = 1; j < list.size(); ++j) {
            if (list[j].name == list[i].name) ++totalSame;
        }
        if (totalSame > 1) {
            const std::string baseName = list[i].name;
            int pairIdx = 0;
            for (size_t j = i; j < list.size(); ++j) {
                if (list[j].name == baseName) {
                    const int chStart = pairIdx * 2 + 1;
                    const int chEnd   = chStart + 1;
                    list[j].name = baseName + " (Out " + std::to_string(chStart) + "-" + std::to_string(chEnd) + ")";
                    ++pairIdx;
                }
            }
        }
    }
#endif
    s_cachedOut = list;
    return list;
}

/**
 * @brief Low-latency multi-buffered stereo Windows waveIn/waveOut full-duplex audio engine
 *        with a dedicated high-priority background rendering thread (CALLBACK_EVENT).
 */
class RealtimeWaveOutPlayer {
public:
    static constexpr uint32_t kSampleRate        = 48000;
    static constexpr uint32_t kChannels          = 2;
    static constexpr size_t   kNumBuffers        = 8;
    static constexpr size_t   kMaxFramesPerBlock = 2048;
    static constexpr size_t   kInputRingCapacity = 32768;

    [[nodiscard]] static size_t computeHardwarePacketFrames(size_t dspFrames, uint32_t sr, int driverMode = 0) noexcept {
        // Windows winmm (waveOutWrite / audiodg.exe) mixes on a 10ms kernel clock.
        // Submitting tiny (<512-sample) WAVEHDR blocks starves the winmm queue and causes
        // slowed-down playback and severe underrun distortion. Scale hardware packet size
        // according to the selected driver queue mode while slicing DSP callbacks into dspFrames!
        const size_t minHwFrames = (driverMode == 1)
            ? ((sr >= 88200) ? 1024 : 512)
            : ((driverMode == 2) ? ((sr >= 88200) ? 2048 : 1536) : ((sr >= 88200) ? 2048 : 1024));
        const size_t step = std::clamp<size_t>(dspFrames, 64, kMaxFramesPerBlock);
        size_t hw = std::max(step, minHwFrames);
        hw = ((hw + step - 1) / step) * step;
        return std::clamp<size_t>(hw, step, kMaxFramesPerBlock);
    }

    using RenderCallback = std::function<void(float* outL, float* outR, size_t numFrames)>;
    using DuplexCallback = std::function<void(const float* inL, const float* inR, float* outL, float* outR, size_t numFrames)>;

    RealtimeWaveOutPlayer() = default;

    ~RealtimeWaveOutPlayer() {
        close();
    }

    RealtimeWaveOutPlayer(const RealtimeWaveOutPlayer&) = delete;
    RealtimeWaveOutPlayer& operator=(const RealtimeWaveOutPlayer&) = delete;

    void setRenderCallback(RenderCallback cb) {
        std::lock_guard<std::mutex> lk(callbackMutex_);
        renderCallback_ = std::move(cb);
    }

    void setDuplexCallback(DuplexCallback cb) {
        std::lock_guard<std::mutex> lk(callbackMutex_);
        duplexCallback_ = std::move(cb);
    }

    void configureDevice(int inputDeviceId, int outputDeviceId, uint32_t sampleRate, size_t bufferFrames, int driverMode = 0) {
        const uint32_t clampedSr     = (sampleRate >= 22050 && sampleRate <= 192000) ? sampleRate : 48000u;
        const size_t   clampedFrames = std::clamp<size_t>(bufferFrames, 64, kMaxFramesPerBlock);
        if (isOpen_.load(std::memory_order_acquire) &&
            inputDeviceId_  == inputDeviceId &&
            outputDeviceId_ == outputDeviceId &&
            sampleRate_     == clampedSr &&
            bufferFrames_   == clampedFrames &&
            driverMode_     == driverMode) {
            return;
        }
        const bool wasOpen = isOpen_.load(std::memory_order_acquire);
        if (wasOpen) {
            close();
        }
        inputDeviceId_  = inputDeviceId;
        outputDeviceId_ = outputDeviceId;
        sampleRate_     = (sampleRate >= 22050 && sampleRate <= 192000) ? sampleRate : 48000u;
        bufferFrames_   = std::clamp<size_t>(bufferFrames, 64, kMaxFramesPerBlock);
        driverMode_     = driverMode;
        if (wasOpen) {
            (void)open();
        }
    }

    [[nodiscard]] uint32_t getSampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] size_t   getBufferFrames() const noexcept { return bufferFrames_; }
    [[nodiscard]] int      getInputDeviceId() const noexcept { return inputDeviceId_; }
    [[nodiscard]] int      getOutputDeviceId() const noexcept { return outputDeviceId_; }
    [[nodiscard]] int      getDriverMode() const noexcept { return driverMode_; }
    [[nodiscard]] bool     hasActiveInput() const noexcept { return isInputOpen_.load(std::memory_order_acquire); }

    bool open() {
#if defined(_WIN32)
        if (isOpen_.load(std::memory_order_acquire)) return true;

        if (!hBufferEvent_) {
            hBufferEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        }

        const uint32_t activeSr     = (sampleRate_ >= 22050 && sampleRate_ <= 192000) ? sampleRate_ : kSampleRate;
        const size_t   activeFrames = std::clamp<size_t>(bufferFrames_, 64, kMaxFramesPerBlock);
        const size_t   hwFrames     = computeHardwarePacketFrames(activeFrames, activeSr, driverMode_);

        WAVEFORMATEX wfx{};
        wfx.wFormatTag      = WAVE_FORMAT_PCM;
        wfx.nChannels       = static_cast<WORD>(kChannels);
        wfx.nSamplesPerSec  = activeSr;
        wfx.wBitsPerSample  = 16;
        wfx.nBlockAlign     = static_cast<WORD>((wfx.nChannels * wfx.wBitsPerSample) / 8);
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.cbSize          = 0;

        const DWORD_PTR cbTarget = hBufferEvent_ ? reinterpret_cast<DWORD_PTR>(hBufferEvent_) : 0;
        const DWORD     cbFlags  = hBufferEvent_ ? CALLBACK_EVENT : CALLBACK_NULL;
        const UINT      outDev   = (outputDeviceId_ >= 0) ? static_cast<UINT>(outputDeviceId_) : WAVE_MAPPER;

        MMRESULT res = waveOutOpen(&hWaveOut_, outDev, &wfx, cbTarget, 0, cbFlags);
        if (res != MMSYSERR_NOERROR && wfx.nSamplesPerSec != kSampleRate) {
            wfx.nSamplesPerSec  = kSampleRate;
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
            sampleRate_         = kSampleRate;
            res = waveOutOpen(&hWaveOut_, outDev, &wfx, cbTarget, 0, cbFlags);
        }
        if (res != MMSYSERR_NOERROR && outDev != WAVE_MAPPER) {
            res = waveOutOpen(&hWaveOut_, WAVE_MAPPER, &wfx, cbTarget, 0, cbFlags);
        }
        if (res != MMSYSERR_NOERROR || !hWaveOut_) {
            hWaveOut_ = nullptr;
            isOpen_.store(false, std::memory_order_release);
            return false;
        }

        for (size_t i = 0; i < kNumBuffers; ++i) {
            buffers_[i].assign(kMaxFramesPerBlock * kChannels, 0);
            std::memset(&headers_[i], 0, sizeof(WAVEHDR));
            headers_[i].lpData         = reinterpret_cast<LPSTR>(buffers_[i].data());
            headers_[i].dwBufferLength = static_cast<DWORD>(hwFrames * kChannels * sizeof(int16_t));
            headers_[i].dwFlags        = WHDR_DONE;
        }

        // Optional Hardware Audio Input (waveIn)
        isInputOpen_.store(false, std::memory_order_release);
        inRingRead_.store(0, std::memory_order_relaxed);
        inRingWrite_.store(0, std::memory_order_relaxed);
        inRingL_.fill(0.0f);
        inRingR_.fill(0.0f);

        if (inputDeviceId_ >= 0) {
            const UINT inDev = static_cast<UINT>(inputDeviceId_);
            WAVEFORMATEX wfxIn = wfx;
            inputChannels_ = 2;
            MMRESULT inRes = waveInOpen(&hWaveIn_, inDev, &wfxIn, cbTarget, 0, cbFlags);
            if (inRes != MMSYSERR_NOERROR) {
                wfxIn.nChannels       = 1;
                wfxIn.nBlockAlign     = 2;
                wfxIn.nAvgBytesPerSec = wfxIn.nSamplesPerSec * 2;
                inputChannels_        = 1;
                inRes = waveInOpen(&hWaveIn_, inDev, &wfxIn, cbTarget, 0, cbFlags);
            }
            if (inRes == MMSYSERR_NOERROR && hWaveIn_) {
                for (size_t i = 0; i < kNumBuffers; ++i) {
                    inBuffers_[i].assign(kMaxFramesPerBlock * 2, 0);
                    std::memset(&inHeaders_[i], 0, sizeof(WAVEHDR));
                    inHeaders_[i].lpData         = reinterpret_cast<LPSTR>(inBuffers_[i].data());
                    inHeaders_[i].dwBufferLength = static_cast<DWORD>(hwFrames * static_cast<size_t>(inputChannels_) * sizeof(int16_t));
                    if (waveInPrepareHeader(hWaveIn_, &inHeaders_[i], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                        waveInAddBuffer(hWaveIn_, &inHeaders_[i], sizeof(WAVEHDR));
                    }
                }
                waveInStart(hWaveIn_);
                isInputOpen_.store(true, std::memory_order_release);
            } else {
                hWaveIn_ = nullptr;
            }
        }

        writeCursor_ = 0;
        isOpen_.store(true, std::memory_order_release);

        bool hasCb = false;
        {
            std::lock_guard<std::mutex> lk(callbackMutex_);
            hasCb = static_cast<bool>(renderCallback_) || static_cast<bool>(duplexCallback_);
        }
        if (hasCb && !threadRunning_.load(std::memory_order_acquire)) {
            threadRunning_.store(true, std::memory_order_release);
            audioThread_ = std::thread([this]() { audioThreadLoop(); });
        }
        return true;
#else
        return false;
#endif
    }

    void close() {
#if defined(_WIN32)
        threadRunning_.store(false, std::memory_order_release);
        if (hBufferEvent_) {
            SetEvent(hBufferEvent_);
        }
        if (audioThread_.joinable()) {
            audioThread_.join();
        }
        if (hWaveIn_) {
            waveInStop(hWaveIn_);
            waveInReset(hWaveIn_);
            for (size_t i = 0; i < kNumBuffers; ++i) {
                if (inHeaders_[i].dwFlags & WHDR_PREPARED) {
                    waveInUnprepareHeader(hWaveIn_, &inHeaders_[i], sizeof(WAVEHDR));
                }
            }
            waveInClose(hWaveIn_);
            hWaveIn_ = nullptr;
        }
        isInputOpen_.store(false, std::memory_order_release);

        if (!isOpen_.load(std::memory_order_acquire) || !hWaveOut_) {
            if (hBufferEvent_) {
                CloseHandle(hBufferEvent_);
                hBufferEvent_ = nullptr;
            }
            isOpen_.store(false, std::memory_order_release);
            return;
        }
        waveOutReset(hWaveOut_);
        for (size_t i = 0; i < kNumBuffers; ++i) {
            if (headers_[i].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(hWaveOut_, &headers_[i], sizeof(WAVEHDR));
            }
        }
        waveOutClose(hWaveOut_);
        hWaveOut_ = nullptr;
        if (hBufferEvent_) {
            CloseHandle(hBufferEvent_);
            hBufferEvent_ = nullptr;
        }
        isOpen_.store(false, std::memory_order_release);
#endif
    }

    [[nodiscard]] bool isOpen() const noexcept { return isOpen_.load(std::memory_order_acquire); }

    [[nodiscard]] bool hasFreeBuffer() noexcept {
#if defined(_WIN32)
        if (!isOpen() && !open()) return false;
        const size_t activeSlots = (driverMode_ == 1) ? 4 : ((driverMode_ == 2) ? 8 : 6);
        for (size_t i = 0; i < activeSlots; ++i) {
            if ((headers_[i].dwFlags & WHDR_INQUEUE) == 0) {
                return true;
            }
        }
        return false;
#else
        return false;
#endif
    }

    /**
     * @brief Submits a block of stereo float samples [-1.0, +1.0] to the soundcard.
     * Drops gracefully if all hardware buffers are currently queued so the caller never blocks.
     */
    void submitStereoBlock(const float* left, const float* right, size_t numFrames, float masterGain = 0.85f) {
#if defined(_WIN32)
        if (!isOpen() && !open()) return;
        if (!hWaveOut_ || numFrames == 0) return;

        const size_t frames = std::min(numFrames, kMaxFramesPerBlock);
        const size_t activeSlots = (driverMode_ == 1) ? 4 : ((driverMode_ == 2) ? 8 : 6);

        // Find the next completed buffer slot
        size_t chosen = activeSlots;
        for (size_t attempt = 0; attempt < activeSlots; ++attempt) {
            const size_t idx = (writeCursor_ + attempt) % activeSlots;
            if ((headers_[idx].dwFlags & WHDR_INQUEUE) == 0) {
                chosen = idx;
                writeCursor_ = (idx + 1) % activeSlots;
                break;
            }
        }
        if (chosen >= activeSlots) {
            return; // Queue is full; skip without blocking
        }

        writeBufferSlot(chosen, left, right, frames, masterGain);
#else
        (void)left; (void)right; (void)numFrames; (void)masterGain;
#endif
    }

private:
#if defined(_WIN32)
    void harvestWaveInBuffers(size_t activeFrames) noexcept {
        if (!hWaveIn_ || !isInputOpen_.load(std::memory_order_relaxed)) return;
        for (size_t i = 0; i < kNumBuffers; ++i) {
            WAVEHDR& hdr = inHeaders_[i];
            if ((hdr.dwFlags & WHDR_DONE) != 0) {
                const size_t bytesRecorded = hdr.dwBytesRecorded;
                const size_t ch = static_cast<size_t>(std::max(1, inputChannels_));
                const size_t framesRecorded = std::min(activeFrames, bytesRecorded / (ch * sizeof(int16_t)));
                const int16_t* src = inBuffers_[i].data();
                size_t wPos = inRingWrite_.load(std::memory_order_relaxed);
                for (size_t f = 0; f < framesRecorded; ++f) {
                    const float sL = static_cast<float>(src[f * ch + 0]) * (1.0f / 32768.0f);
                    const float sR = (ch > 1) ? (static_cast<float>(src[f * ch + 1]) * (1.0f / 32768.0f)) : sL;
                    inRingL_[wPos % kInputRingCapacity] = sL;
                    inRingR_[wPos % kInputRingCapacity] = sR;
                    ++wPos;
                }
                inRingWrite_.store(wPos, std::memory_order_release);

                waveInUnprepareHeader(hWaveIn_, &hdr, sizeof(WAVEHDR));
                hdr.lpData         = reinterpret_cast<LPSTR>(inBuffers_[i].data());
                hdr.dwBufferLength = static_cast<DWORD>(activeFrames * ch * sizeof(int16_t));
                hdr.dwFlags        = 0;
                if (waveInPrepareHeader(hWaveIn_, &hdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                    waveInAddBuffer(hWaveIn_, &hdr, sizeof(WAVEHDR));
                }
            }
        }
    }

    void popInputFrames(float* dstL, float* dstR, size_t frames) noexcept {
        if (!isInputOpen_.load(std::memory_order_relaxed)) {
            std::fill_n(dstL, frames, 0.0f);
            std::fill_n(dstR, frames, 0.0f);
            return;
        }
        size_t rPos = inRingRead_.load(std::memory_order_relaxed);
        const size_t wPos = inRingWrite_.load(std::memory_order_acquire);
        if (wPos > rPos + kInputRingCapacity / 2) {
            rPos = wPos - frames;
        }
        for (size_t i = 0; i < frames; ++i) {
            if (rPos < wPos) {
                dstL[i] = inRingL_[rPos % kInputRingCapacity];
                dstR[i] = inRingR_[rPos % kInputRingCapacity];
                ++rPos;
            } else {
                dstL[i] = 0.0f;
                dstR[i] = 0.0f;
            }
        }
        inRingRead_.store(rPos, std::memory_order_release);
    }

    void writeBufferSlot(size_t slotIdx, const float* left, const float* right, size_t frames, float masterGain) noexcept {
        WAVEHDR& hdr = headers_[slotIdx];
        if (hdr.dwFlags & WHDR_PREPARED) {
            waveOutUnprepareHeader(hWaveOut_, &hdr, sizeof(WAVEHDR));
        }

        int16_t* dst = buffers_[slotIdx].data();
        for (size_t i = 0; i < frames; ++i) {
            const float sL = std::clamp(left[i] * masterGain, -0.99f, 0.99f);
            const float sR = std::clamp(right[i] * masterGain, -0.99f, 0.99f);
            dst[i * 2 + 0] = static_cast<int16_t>(sL * 32760.0f);
            dst[i * 2 + 1] = static_cast<int16_t>(sR * 32760.0f);
        }

        hdr.lpData         = reinterpret_cast<LPSTR>(dst);
        hdr.dwBufferLength = static_cast<DWORD>(frames * kChannels * sizeof(int16_t));
        hdr.dwFlags        = 0;

        if (waveOutPrepareHeader(hWaveOut_, &hdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
            waveOutWrite(hWaveOut_, &hdr, sizeof(WAVEHDR));
        }
    }

    void audioThreadLoop() {
        const int mode = driverMode_;
        timeBeginPeriod(1);
        SetThreadPriority(GetCurrentThread(),
                          (mode == 1) ? THREAD_PRIORITY_TIME_CRITICAL
                                      : ((mode == 2) ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_HIGHEST));
        std::array<float, kMaxFramesPerBlock> captureL{};
        std::array<float, kMaxFramesPerBlock> captureR{};
        std::array<float, kMaxFramesPerBlock> renderL{};
        std::array<float, kMaxFramesPerBlock> renderR{};

        const uint32_t activeSr     = (sampleRate_ >= 22050 && sampleRate_ <= 192000) ? sampleRate_ : kSampleRate;
        const size_t   dspFrames    = std::clamp<size_t>(bufferFrames_, 64, kMaxFramesPerBlock);
        const size_t   hwFrames     = computeHardwarePacketFrames(dspFrames, activeSr, mode);
        const size_t   activeQueueSlots = (mode == 1) ? 4 : ((mode == 2) ? 8 : 6);
        const DWORD    waitTimeoutMs = (mode == 1) ? 1 : ((mode == 2) ? 4 : 2);

        while (threadRunning_.load(std::memory_order_acquire)) {
            harvestWaveInBuffers(hwFrames);
            bool filledAny = false;
            for (size_t attempt = 0; attempt < activeQueueSlots; ++attempt) {
                if (!threadRunning_.load(std::memory_order_acquire) || !hWaveOut_) break;
                const size_t idx = (writeCursor_ + attempt) % activeQueueSlots;
                if ((headers_[idx].dwFlags & WHDR_INQUEUE) == 0) {
                    writeCursor_ = (idx + 1) % activeQueueSlots;
                    harvestWaveInBuffers(hwFrames);
                    popInputFrames(captureL.data(), captureR.data(), hwFrames);
                    renderL.fill(0.0f);
                    renderR.fill(0.0f);

                    RenderCallback cbCopy;
                    DuplexCallback duplexCopy;
                    {
                        std::lock_guard<std::mutex> lk(callbackMutex_);
                        cbCopy     = renderCallback_;
                        duplexCopy = duplexCallback_;
                    }
                    // Slice hardware packet into dspFrames sub-blocks so the plugin DSP
                    // processes at the exact user-selected buffer size while waveOutWrite
                    // receives smooth >=1024-sample packets with zero kernel underruns.
                    for (size_t offset = 0; offset < hwFrames; offset += dspFrames) {
                        const size_t chunk = std::min(dspFrames, hwFrames - offset);
                        if (duplexCopy) {
                            duplexCopy(captureL.data() + offset, captureR.data() + offset,
                                       renderL.data() + offset, renderR.data() + offset, chunk);
                        } else if (cbCopy) {
                            cbCopy(renderL.data() + offset, renderR.data() + offset, chunk);
                        }
                    }
                    writeBufferSlot(idx, renderL.data(), renderR.data(), hwFrames, 0.85f);
                    filledAny = true;
                }
            }
            if (!threadRunning_.load(std::memory_order_acquire)) break;
            if (!filledAny && hBufferEvent_) {
                WaitForSingleObject(hBufferEvent_, waitTimeoutMs);
            }
        }
        timeEndPeriod(1);
    }

    HWAVEOUT hWaveOut_{nullptr};
    HWAVEIN  hWaveIn_{nullptr};
    HANDLE   hBufferEvent_{nullptr};
    std::thread audioThread_;
    std::atomic<bool> threadRunning_{false};
    std::array<WAVEHDR, kNumBuffers> headers_{};
    std::array<std::vector<int16_t>, kNumBuffers> buffers_{};
    std::array<WAVEHDR, kNumBuffers> inHeaders_{};
    std::array<std::vector<int16_t>, kNumBuffers> inBuffers_{};
    std::array<float, kInputRingCapacity> inRingL_{};
    std::array<float, kInputRingCapacity> inRingR_{};
    std::atomic<size_t> inRingRead_{0};
    std::atomic<size_t> inRingWrite_{0};
    int inputChannels_{2};
#endif
    std::mutex callbackMutex_;
    RenderCallback renderCallback_{};
    DuplexCallback duplexCallback_{};
    std::atomic<bool> isOpen_{false};
    std::atomic<bool> isInputOpen_{false};
    int inputDeviceId_{-1};
    int outputDeviceId_{-1};
    uint32_t sampleRate_{kSampleRate};
    size_t bufferFrames_{kMaxFramesPerBlock};
    int driverMode_{0};
    size_t writeCursor_{0};
};

/**
 * @brief Zero-dependency RIFF .WAV audio file reader & seamless stereo loop player.
 * Supports 16-bit PCM, 24-bit PCM, and 32-bit IEEE Float WAV files (mono or stereo).
 */
class WavLoopPlayer {
public:
    bool loadFromFile(const std::string& filePath, std::string& outError) {
        std::ifstream f(filePath, std::ios::binary);
        if (!f.is_open()) {
            outError = "Could not open WAV file.";
            return false;
        }

        char riffId[4]{};
        uint32_t riffSize = 0;
        char waveId[4]{};
        f.read(riffId, 4);
        f.read(reinterpret_cast<char*>(&riffSize), 4);
        f.read(waveId, 4);
        if (std::strncmp(riffId, "RIFF", 4) != 0 || std::strncmp(waveId, "WAVE", 4) != 0) {
            outError = "Not a valid RIFF WAVE file.";
            return false;
        }

        uint16_t audioFormat = 0;
        uint16_t numChannels = 0;
        uint32_t sampleRate  = 48000;
        uint16_t bitsPerSample = 16;
        std::vector<uint8_t> rawData;

        while (f.good() && !f.eof()) {
            char chunkId[4]{};
            uint32_t chunkSize = 0;
            if (!f.read(chunkId, 4)) break;
            if (!f.read(reinterpret_cast<char*>(&chunkSize), 4)) break;

            if (std::strncmp(chunkId, "fmt ", 4) == 0) {
                f.read(reinterpret_cast<char*>(&audioFormat), 2);
                f.read(reinterpret_cast<char*>(&numChannels), 2);
                f.read(reinterpret_cast<char*>(&sampleRate), 4);
                uint32_t byteRate = 0;
                uint16_t blockAlign = 0;
                f.read(reinterpret_cast<char*>(&byteRate), 4);
                f.read(reinterpret_cast<char*>(&blockAlign), 2);
                f.read(reinterpret_cast<char*>(&bitsPerSample), 2);
                if (chunkSize > 16) {
                    const uint32_t extraBytes = chunkSize - 16;
                    if (audioFormat == 0xFFFEu && extraBytes >= 24) {
                        // WAVE_FORMAT_EXTENSIBLE: read SubFormat GUID tag at offset +8 of extension
                        uint8_t extBuf[24]{};
                        f.read(reinterpret_cast<char*>(extBuf), 24);
                        uint16_t subFormat = 0;
                        std::memcpy(&subFormat, extBuf + 8, sizeof(uint16_t));
                        if (subFormat == 1 || subFormat == 3) {
                            audioFormat = subFormat;
                        }
                        if (extraBytes > 24) {
                            f.seekg(extraBytes - 24, std::ios::cur);
                        }
                    } else {
                        f.seekg(extraBytes, std::ios::cur);
                    }
                }
            } else if (std::strncmp(chunkId, "data", 4) == 0) {
                // Limit to 120 seconds max to keep memory footprint reasonable
                const uint32_t maxBytes = 96000u * 120u * 8u;
                const uint32_t readBytes = std::min(chunkSize, maxBytes);
                rawData.resize(readBytes);
                f.read(reinterpret_cast<char*>(rawData.data()), readBytes);
                break;
            } else {
                f.seekg(chunkSize + (chunkSize & 1u), std::ios::cur);
            }
        }

        if (rawData.empty() || numChannels == 0) {
            outError = "WAV file contains no valid audio data.";
            return false;
        }

        const size_t bytesPerSample = bitsPerSample / 8;
        const size_t frameStride = bytesPerSample * numChannels;
        if (frameStride == 0) {
            outError = "Unsupported WAV bit depth.";
            return false;
        }

        const size_t numRawFrames = rawData.size() / frameStride;
        if (numRawFrames == 0) {
            outError = "WAV file is empty.";
            return false;
        }

        left_.resize(numRawFrames, 0.0f);
        right_.resize(numRawFrames, 0.0f);

        for (size_t i = 0; i < numRawFrames; ++i) {
            const uint8_t* framePtr = rawData.data() + i * frameStride;
            auto decodeSample = [&](size_t ch) -> float {
                const uint8_t* p = framePtr + ch * bytesPerSample;
                if (audioFormat == 3 && bitsPerSample == 32) {
                    float v = 0.0f;
                    std::memcpy(&v, p, sizeof(float));
                    return std::isfinite(v) ? std::clamp(v, -1.0f, 1.0f) : 0.0f;
                } else if (audioFormat == 3 && bitsPerSample == 64) {
                    double v = 0.0;
                    std::memcpy(&v, p, sizeof(double));
                    return std::isfinite(v) ? static_cast<float>(std::clamp(v, -1.0, 1.0)) : 0.0f;
                } else if (bitsPerSample == 16) {
                    int16_t v = 0;
                    std::memcpy(&v, p, sizeof(int16_t));
                    return static_cast<float>(v) / 32768.0f;
                } else if (bitsPerSample == 24) {
                    int32_t v = (static_cast<int32_t>(p[0]) << 8) |
                                (static_cast<int32_t>(p[1]) << 16) |
                                (static_cast<int32_t>(static_cast<int8_t>(p[2])) << 24);
                    return static_cast<float>(v >> 8) / 8388608.0f;
                } else if (bitsPerSample == 32) {
                    int32_t v = 0;
                    std::memcpy(&v, p, sizeof(int32_t));
                    return static_cast<float>(static_cast<double>(v) / 2147483648.0);
                }
                return 0.0f;
            };

            left_[i]  = decodeSample(0);
            right_[i] = (numChannels > 1) ? decodeSample(1) : left_[i];
        }

        wavSampleRate_ = std::clamp<uint32_t>(sampleRate, 8000u, 384000u);
        playheadPos_   = 0.0;
        fileName_      = filePath;
        const auto slashPos = fileName_.find_last_of("\\/");
        if (slashPos != std::string::npos) {
            fileName_ = fileName_.substr(slashPos + 1);
        }
        return true;
    }

    [[nodiscard]] bool isLoaded() const noexcept { return !left_.empty(); }
    [[nodiscard]] const std::string& getFileName() const noexcept { return fileName_; }
    [[nodiscard]] uint32_t getSampleRate() const noexcept { return wavSampleRate_; }

    void nextStereoSample(float& outL, float& outR, double playbackSampleRate = 48000.0) noexcept {
        if (left_.empty()) {
            outL = 0.0f;
            outR = 0.0f;
            return;
        }
        const size_t N = left_.size();
        while (playheadPos_ >= static_cast<double>(N)) {
            playheadPos_ -= static_cast<double>(N);
        }
        if (playheadPos_ < 0.0) {
            playheadPos_ = 0.0;
        }
        const size_t idx0 = static_cast<size_t>(playheadPos_) % N;
        const size_t idx1 = (idx0 + 1) % N;
        const float  frac = static_cast<float>(playheadPos_ - std::floor(playheadPos_));
        outL = left_[idx0]  + frac * (left_[idx1]  - left_[idx0]);
        outR = right_[idx0] + frac * (right_[idx1] - right_[idx0]);

        const double step = static_cast<double>(wavSampleRate_) / std::max(8000.0, playbackSampleRate);
        playheadPos_ += step;
    }

private:
    std::vector<float> left_;
    std::vector<float> right_;
    uint32_t wavSampleRate_{48000};
    double playheadPos_{0.0};
    std::string fileName_;
};

/**
 * @brief Built-in 48 kHz Studio Drum & Bass Groove + Synth Chord Loop generator
 *        so users can immediately hear compression, EQ, saturation,transient shaping,
 *        reverb, delay, and stereo modulation on realistic musical transients.
 */
struct StudioGrooveSynthesizer {
    static constexpr double kTwoPi = 6.28318530717958647692;

    /**
     * @brief 120 BPM Straightforward 4/4 Acoustic Studio Drum Kit (4 beats = 2.0s bar).
     * Kick on Beats 1 & 3, Acoustic Snare on Beats 2 & 4, and natural Bronze Hi-Hats on straight 8th notes.
     */
    static void synthesizeDrumAndBassGroove(double t, uint32_t& rng, float& outL, float& outR, double sampleRate = 48000.0) noexcept {
        struct DrumKitState {
            int    lastStep{-1};
            double kickPhase{0.0};
            double kickMode2Phase{0.0};
            double snarePhase1{0.0};
            double snarePhase2{0.0};
            float  beaterLp{0.0f};
            float  snareBpIc1L{0.0f}, snareBpIc2L{0.0f};
            float  snareBpIc1R{0.0f}, snareBpIc2R{0.0f};
            float  hatBpIc1L{0.0f}, hatBpIc2L{0.0f};
            float  hatBpIc1R{0.0f}, hatBpIc2R{0.0f};
            std::array<float, 1024> roomBufL{};
            std::array<float, 1024> roomBufR{};
            float  roomLpL{0.0f}, roomLpR{0.0f};
            size_t roomIdx{0};
        };
        static thread_local DrumKitState st{};

        const double kSr = std::max(8000.0, sampleRate);
        // 120 BPM = 2 beats/sec = 4 eighth-notes/sec -> 8 eighth-notes per 4/4 bar (2.0s loop)
        const double stepFloat = std::fmod(t * 4.0, 8.0);
        const int step8 = static_cast<int>(stepFloat) & 7;
        const double stepTime = (stepFloat - static_cast<double>(step8)) * 0.25; // 0..0.25s per 8th note

        const bool isKickStep  = (step8 == 0 || step8 == 4); // Beats 1 and 3
        const bool isSnareStep = (step8 == 2 || step8 == 6); // Beats 2 and 4

        if (step8 != st.lastStep) {
            st.lastStep = step8;
            if (isKickStep) {
                st.kickPhase      = 0.0;
                st.kickMode2Phase = 0.0;
            }
            if (isSnareStep) {
                st.snarePhase1 = 0.0;
                st.snarePhase2 = 0.0;
            }
        }

        // 1. Acoustic 22" Maple Kick Drum on Beats 1 & 3 (natural batter-head tension drop + warm wood shell overtones + felt beater thud)
        float kick = 0.0f;
        if (isKickStep) {
            const double kickFreq = 52.0 + 46.0 * std::exp(-stepTime * 36.0) + 38.0 * std::exp(-stepTime * 185.0);
            st.kickPhase += kickFreq / kSr;
            st.kickPhase -= std::floor(st.kickPhase);
            st.kickMode2Phase += (kickFreq * 1.92) / kSr;
            st.kickMode2Phase -= std::floor(st.kickMode2Phase);

            const float airEnv   = static_cast<float>(std::exp(-stepTime * 9.8));
            const float woodEnv  = static_cast<float>(std::exp(-stepTime * 28.0));
            const float thudEnv  = static_cast<float>(std::exp(-stepTime * 165.0));

            rng = rng * 1664525u + 1013904223u;
            const float wn = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);
            st.beaterLp += 0.18f * (wn - st.beaterLp);
            const float feltBeater = (st.beaterLp * 0.55f + std::sin(static_cast<float>(stepTime * kTwoPi * 1450.0)) * 0.45f) * thudEnv * 0.26f;

            const float fundamental = std::sin(static_cast<float>(st.kickPhase * kTwoPi)) * airEnv;
            const float shellWood   = std::sin(static_cast<float>(st.kickMode2Phase * kTwoPi)) * woodEnv * 0.22f;
            kick = std::tanh((fundamental + shellWood + feltBeater) * 1.18f) * 0.78f;
        }

        // 2. Acoustic 14" Studio Snare Drum on Beats 2 & 4 (dual membrane modes + wood stick transient + natural snare-bed wire decay)
        float snareL = 0.0f, snareR = 0.0f;
        if (isSnareStep) {
            const double f1 = 182.0 + 34.0 * std::exp(-stepTime * 105.0);
            const double f2 = 294.0 + 48.0 * std::exp(-stepTime * 120.0);
            st.snarePhase1 += f1 / kSr;
            st.snarePhase1 -= std::floor(st.snarePhase1);
            st.snarePhase2 += f2 / kSr;
            st.snarePhase2 -= std::floor(st.snarePhase2);

            const float headEnv  = static_cast<float>(std::exp(-stepTime * 22.0));
            const float wireSnap = static_cast<float>(std::exp(-stepTime * 28.0));
            const float wireTail = static_cast<float>(std::exp(-stepTime * 11.5));
            const float wireEnv  = 0.58f * wireSnap + 0.42f * wireTail;
            const float stickEnv = static_cast<float>(std::exp(-stepTime * 175.0));

            const float shell = (std::sin(static_cast<float>(st.snarePhase1 * kTwoPi)) * 0.65f +
                                 std::sin(static_cast<float>(st.snarePhase2 * kTwoPi)) * 0.35f) * headEnv;
            const float woodStick = (std::sin(static_cast<float>(stepTime * kTwoPi * 860.0)) * 0.6f +
                                     std::sin(static_cast<float>(stepTime * kTwoPi * 1520.0)) * 0.4f) * stickEnv * 0.32f;

            rng = rng * 1664525u + 1013904223u;
            const float rawNL = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
            rng = rng * 1664525u + 1013904223u;
            const float rawNR = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;

            constexpr float gBp = 0.36f; // ~2.9 kHz natural acoustic snare wire bandpass
            constexpr float kBp = 0.92f;
            const float v3L = rawNL - st.snareBpIc2L;
            const float v1L = (st.snareBpIc1L + gBp * v3L) / (1.0f + gBp * (gBp + kBp));
            const float v2L = st.snareBpIc2L + gBp * v1L;
            st.snareBpIc1L = 2.0f * v1L - st.snareBpIc1L;
            st.snareBpIc2L = 2.0f * v2L - st.snareBpIc2L;
            const float wireMod = 1.0f + 0.25f * shell;
            const float wireL = (v1L * 0.78f + (rawNL - v2L) * 0.22f) * wireEnv * wireMod;

            const float v3R = rawNR - st.snareBpIc2R;
            const float v1R = (st.snareBpIc1R + gBp * v3R) / (1.0f + gBp * (gBp + kBp));
            const float v2R = st.snareBpIc2R + gBp * v1R;
            st.snareBpIc1R = 2.0f * v1R - st.snareBpIc1R;
            st.snareBpIc2R = 2.0f * v2R - st.snareBpIc2R;
            const float wireR = (v1R * 0.78f + (rawNR - v2R) * 0.22f) * wireEnv * wireMod;

            snareL = std::tanh((shell * 0.68f + woodStick + wireL * 0.58f) * 1.15f) * 0.70f;
            snareR = std::tanh((shell * 0.68f + woodStick + wireR * 0.58f) * 1.15f) * 0.70f;
        }

        // 3. Acoustic Closed Bronze Hi-Hat on straight 8th notes (8 inharmonic bronze modes + stick tip + SVF resonator)
        static constexpr std::array<double, 8> kBronzeModes = {
            3280.0, 4195.0, 5340.0, 6485.0, 7820.0, 9155.0, 10680.0, 12410.0
        };
        float bronzeModal = 0.0f;
        const float modIndex = 0.35f * static_cast<float>(std::exp(-stepTime * 55.0));
        float prevSin = 0.0f;
        for (double cf : kBronzeModes) {
            const float s = std::sin(static_cast<float>(t * kTwoPi * cf) + prevSin * modIndex);
            bronzeModal += s * 0.125f;
            prevSin = s;
        }
        rng = rng * 1664525u + 1013904223u;
        const float hatNoiseL = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);
        rng = rng * 1664525u + 1013904223u;
        const float hatNoiseR = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);

        const float rawHatL = bronzeModal * 0.48f + hatNoiseL * 0.52f;
        const float rawHatR = bronzeModal * 0.44f + hatNoiseR * 0.56f;

        constexpr float gHat = 0.68f;
        constexpr float kHat = 0.85f;
        const float hv3L = rawHatL - st.hatBpIc2L;
        const float hv1L = (st.hatBpIc1L + gHat * hv3L) / (1.0f + gHat * (gHat + kHat));
        const float hv2L = st.hatBpIc2L + gHat * hv1L;
        st.hatBpIc1L = 2.0f * hv1L - st.hatBpIc1L;
        st.hatBpIc2L = 2.0f * hv2L - st.hatBpIc2L;
        const float crispHatL = (rawHatL - hv2L) * 0.65f + hv1L * 0.35f;

        const float hv3R = rawHatR - st.hatBpIc2R;
        const float hv1R = (st.hatBpIc1R + gHat * hv3R) / (1.0f + gHat * (gHat + kHat));
        const float hv2R = st.hatBpIc2R + gHat * hv1R;
        st.hatBpIc1R = 2.0f * hv1R - st.hatBpIc1R;
        st.hatBpIc2R = 2.0f * hv2R - st.hatBpIc2R;
        const float crispHatR = (rawHatR - hv2R) * 0.65f + hv1R * 0.35f;

        const bool isDownbeatHat = ((step8 & 1) == 0);
        const float hatVel    = isDownbeatHat ? 0.20f : 0.13f;
        const float hatDecay  = isDownbeatHat ? 38.0f : 48.0f;
        const float hatAttack = static_cast<float>(std::min(1.0, stepTime / 0.002));
        const float hatEnv    = hatAttack * static_cast<float>(std::exp(-stepTime * hatDecay)) * hatVel;

        const float dryL = kick + snareL + crispHatL * hatEnv * 1.12f;
        const float dryR = kick + snareR + crispHatR * hatEnv * 0.90f;

        // Warm wood studio room early reflections (12 ms / 17 ms with HF damping)
        const float rawRoomL = st.roomBufL[(st.roomIdx + 1024 - 576) & 1023];
        const float rawRoomR = st.roomBufR[(st.roomIdx + 1024 - 816) & 1023];
        st.roomLpL += 0.32f * (rawRoomL - st.roomLpL);
        st.roomLpR += 0.32f * (rawRoomR - st.roomLpR);
        st.roomBufL[st.roomIdx] = snareR * 0.26f + kick * 0.09f + crispHatR * hatEnv * 0.12f - st.roomLpR * 0.16f;
        st.roomBufR[st.roomIdx] = snareL * 0.26f + kick * 0.09f + crispHatL * hatEnv * 0.12f + st.roomLpL * 0.16f;
        st.roomIdx = (st.roomIdx + 1) & 1023;

        outL = std::clamp(dryL + st.roomLpL * 0.22f, -0.95f, 0.95f);
        outR = std::clamp(dryR + st.roomLpR * 0.22f, -0.95f, 0.95f);
    }

    /**
     * @brief 120 BPM Warm Polyphonic Synth Pad Chords (4 beats = 2.0s per chord, 4-chord progression = 8.0s loop).
     * Pure stereo analog pad chords with lead/arpeggio synth removed.
     */
    static void synthesizeSynthChordLoop(double t, float& outL, float& outR, double sampleRate = 48000.0) noexcept {
        struct SynthChordState {
            double voicePhaseL[4]{ 0.0, 0.0, 0.0, 0.0 };
            double voicePhaseR[4]{ 0.0, 0.0, 0.0, 0.0 };
            double subVoicePhase[4]{ 0.0, 0.0, 0.0, 0.0 };
            float  padLpL{0.0f}, padLpR{0.0f};
        };
        static thread_local SynthChordState st{};

        const double kSr = std::max(8000.0, sampleRate);
        // 120 BPM: 4 beats = 2.0 seconds per chord, 4 chords = 8.0s loop
        const double chordPos = std::fmod(t * 0.5, 4.0);
        const int chordIdx = static_cast<int>(chordPos) & 3;
        const double chordTime = (chordPos - static_cast<double>(chordIdx)) * 2.0;

        static constexpr double kChords[4][4] = {
            { 220.00, 261.63, 329.63, 392.00 }, // Am7
            { 174.61, 220.00, 261.63, 329.63 }, // Fmaj7
            { 130.81, 196.00, 246.94, 329.63 }, // Cmaj7
            { 196.00, 246.94, 293.66, 392.00 }  // G6
        };

        // Smooth 90ms raised-cosine pad swell at chord boundaries so transitions are seamless
        float padEnv = 1.0f;
        if (chordTime < 0.090) {
            padEnv = static_cast<float>(0.5 * (1.0 - std::cos(3.14159265358979323846 * (chordTime / 0.090))));
        } else if (chordTime > 1.910) {
            padEnv = static_cast<float>(0.5 * (1.0 - std::cos(3.14159265358979323846 * ((2.0 - chordTime) / 0.090))));
        }

        float sumL = 0.0f;
        float sumR = 0.0f;
        for (int v = 0; v < 4; ++v) {
            const double f = kChords[chordIdx][v];
            st.voicePhaseL[v] += (f * 0.9988) / kSr;
            st.voicePhaseL[v] -= std::floor(st.voicePhaseL[v]);
            st.voicePhaseR[v] += (f * 1.0012) / kSr;
            st.voicePhaseR[v] -= std::floor(st.voicePhaseR[v]);
            st.subVoicePhase[v] += (f * 0.5) / kSr;
            st.subVoicePhase[v] -= std::floor(st.subVoicePhase[v]);

            const float pL   = static_cast<float>(st.voicePhaseL[v] * kTwoPi);
            const float pR   = static_cast<float>(st.voicePhaseR[v] * kTwoPi);
            const float pSub = static_cast<float>(st.subVoicePhase[v] * kTwoPi);

            const float vL = std::sin(pL) * 0.58f + std::sin(pL * 2.0f) * 0.22f + std::sin(pL * 3.0f) * 0.08f + std::sin(pSub) * 0.18f;
            const float vR = std::sin(pR) * 0.58f + std::sin(pR * 2.0f) * 0.22f + std::sin(pR * 3.0f) * 0.08f + std::sin(pSub) * 0.18f;
            sumL += vL * 0.14f;
            sumR += vR * 0.14f;
        }

        st.padLpL += 0.18f * (sumL - st.padLpL);
        st.padLpR += 0.18f * (sumR - st.padLpR);

        outL = std::clamp(st.padLpL * padEnv, -0.75f, 0.75f);
        outR = std::clamp(st.padLpR * padEnv, -0.75f, 0.75f);
    }

    /**
     * @brief 120 BPM 6-String Open Clean Guitar Progression (A - E - D - A, 4 beats = 2.0s per chord, 8.0s loop).
     */
    static void synthesizeCleanGuitarChord(double t, uint32_t& rng, float& outL, float& outR, double sampleRate = 48000.0) noexcept {
        struct CleanGuitarState {
            int  lastBar{-1};
            bool stringPlucked[6]{ false, false, false, false, false, false };
            std::array<std::array<float, 1024>, 6> strBuf{};
            int   pos[6]{ 0, 0, 0, 0, 0, 0 };
            int   len[6]{ 436, 291, 218, 173, 146, 109 };
            float lp[6]{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
            float pickupIc1{0.0f}, pickupIc2{0.0f};
            std::array<float, 1024> stereoAirBuf{};
            size_t airIdx{0};
        };
        static thread_local CleanGuitarState st{};

        const double kSr = std::max(8000.0, sampleRate);
        const int barIdx = static_cast<int>(std::floor(t * 0.5));
        const int chordIdx = barIdx & 3;
        const double barTime = std::fmod(t, 2.0);

        // 4-Bar Open Guitar Progression (A Major -> E Major -> D Major -> A Major, 4 beats per chord)
        static constexpr double kOpenChords[4][6] = {
            { 110.00, 164.81, 220.00, 277.18, 329.63, 440.00 }, // A Major (A2, E3, A3, C#4, E4, A4)
            {  82.41, 123.47, 164.81, 207.65, 246.94, 329.63 }, // E Major (E2, B2, E3, G#3, B3, E4)
            {  73.42, 110.00, 146.83, 220.00, 293.66, 369.99 }, // D Major (D2, A2, D3, A3, D4, F#4)
            { 110.00, 164.81, 220.00, 277.18, 329.63, 440.00 }  // A Major (A2, E3, A3, C#4, E4, A4)
        };
        static constexpr std::array<double, 6> kStrumDelays = {
            0.000, 0.018, 0.036, 0.054, 0.072, 0.090
        };

        if (barIdx != st.lastBar) {
            st.lastBar = barIdx;
            for (int s = 0; s < 6; ++s) {
                st.stringPlucked[s] = false;
            }
        }

        for (int s = 0; s < 6; ++s) {
            if (!st.stringPlucked[s] && barTime >= kStrumDelays[static_cast<size_t>(s)]) {
                st.stringPlucked[s] = true;
                st.len[s] = std::clamp(static_cast<int>(std::round(kSr / kOpenChords[chordIdx][s])), 60, 1000);
                const int L = st.len[s];
                const int pickPos = std::max(4, L / 5);
                float smoothNoise = 0.0f;
                const float strAmp = 0.85f - 0.04f * static_cast<float>(s);
                for (int i = 0; i < L; ++i) {
                    const float tri = (i < pickPos)
                        ? (static_cast<float>(i) / static_cast<float>(pickPos))
                        : (static_cast<float>(L - i) / static_cast<float>(std::max(1, L - pickPos)));
                    rng = rng * 1664525u + 1013904223u;
                    const float wn = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);
                    smoothNoise = 0.62f * smoothNoise + 0.38f * wn;
                    const float newExcite = (tri * 0.68f + smoothNoise * 0.32f) * strAmp;
                    st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i)] =
                        st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i)] * 0.15f + newExcite * 0.85f;
                }
                for (int i = L - 1; i >= pickPos; --i) {
                    st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i)] -=
                        0.35f * st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i - pickPos)];
                }
                st.pos[s] = 0;
            }
        }

        float chordSum = 0.0f;
        for (int s = 0; s < 6; ++s) {
            auto& buf = st.strBuf[static_cast<size_t>(s)];
            const int L = st.len[s];
            const int p = st.pos[s];
            const float cur = buf[static_cast<size_t>(p)];
            const int nextIdx = (p + 1 < L) ? (p + 1) : 0;
            const float next = buf[static_cast<size_t>(nextIdx)];
            const float avg  = 0.5f * (cur + next);
            const float damp = 0.075f + 0.010f * static_cast<float>(s);
            const float fb   = (barTime > 1.92) ? 0.965f : (0.9990f - 0.0001f * static_cast<float>(s));
            st.lp[s] += damp * (avg - st.lp[s]);
            buf[static_cast<size_t>(p)] = (avg * (1.0f - damp) + st.lp[s] * damp) * fb;
            st.pos[s] = nextIdx;
            chordSum += cur * 0.24f;
        }

        float barEnv = 1.0f;
        if (barTime > 1.93) {
            barEnv = static_cast<float>(0.5 * (1.0 + std::cos(3.14159265358979323846 * ((barTime - 1.93) / 0.07))));
        }

        constexpr float gPick = 0.48f;
        constexpr float kPick = 0.88f;
        const float v3 = chordSum - st.pickupIc2;
        const float v1 = (st.pickupIc1 + gPick * v3) / (1.0f + gPick * (gPick + kPick));
        const float v2 = st.pickupIc2 + gPick * v1;
        st.pickupIc1 = 2.0f * v1 - st.pickupIc1;
        st.pickupIc2 = 2.0f * v2 - st.pickupIc2;

        const float cleanOut = std::tanh((v2 * 0.78f + v1 * 0.36f) * 1.12f) * 0.76f * barEnv;
        const float air = st.stereoAirBuf[(st.airIdx + 1024 - 640) & 1023];
        st.stereoAirBuf[st.airIdx] = cleanOut;
        st.airIdx = (st.airIdx + 1) & 1023;

        outL = std::clamp(cleanOut, -0.92f, 0.92f);
        outR = std::clamp(cleanOut * 0.84f + air * 0.22f, -0.92f, 0.92f);
    }

    /**
     * @brief 120 BPM Distorted Open Guitar Progression (A5 - E5 - D5 - A5, 4 beats = 2.0s per chord, 8.0s loop).
     * Models a 6-string rock power-chord progression driven through a multi-stage tube overdrive and 4x12 speaker cabinet resonator.
     */
    static void synthesizeGuitarDiChug(double t, uint32_t& rng, float& outL, float& outR, double sampleRate = 48000.0) noexcept {
        struct DistGuitarState {
            int  lastBar{-1};
            bool stringPlucked[6]{ false, false, false, false, false, false };
            std::array<std::array<float, 1024>, 6> strBuf{};
            int   pos[6]{ 0, 0, 0, 0, 0, 0 };
            int   len[6]{ 436, 291, 218, 146, 145, 109 };
            float lp[6]{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
            float preHpLp{0.0f};
            float cabIc1{0.0f}, cabIc2{0.0f};
            float cabLp2{0.0f};
            std::array<float, 1024> cabDoubleBuf{};
            size_t cabIdx{0};
        };
        static thread_local DistGuitarState st{};

        const double kSr = std::max(8000.0, sampleRate);
        const int barIdx = static_cast<int>(std::floor(t * 0.5));
        const int chordIdx = barIdx & 3;
        const double barTime = std::fmod(t, 2.0);

        // 4-Bar Power-Chord Progression (A5 -> E5 -> D5 -> A5, 4 beats per chord)
        static constexpr double kPowerChords[4][6] = {
            { 110.00, 164.81, 220.00, 329.63, 329.85, 440.00 }, // A5
            {  82.41, 123.47, 164.81, 246.94, 247.15, 329.63 }, // E5
            {  73.42, 110.00, 146.83, 220.00, 293.66, 440.00 }, // D5
            { 110.00, 164.81, 220.00, 329.63, 329.85, 440.00 }  // A5
        };
        static constexpr std::array<double, 6> kStrumDelays = {
            0.000, 0.014, 0.028, 0.042, 0.056, 0.070
        };

        if (barIdx != st.lastBar) {
            st.lastBar = barIdx;
            for (int s = 0; s < 6; ++s) {
                st.stringPlucked[s] = false;
            }
        }

        for (int s = 0; s < 6; ++s) {
            if (!st.stringPlucked[s] && barTime >= kStrumDelays[static_cast<size_t>(s)]) {
                st.stringPlucked[s] = true;
                st.len[s] = std::clamp(static_cast<int>(std::round(kSr / kPowerChords[chordIdx][s])), 60, 1000);
                const int L = st.len[s];
                const int pickPos = std::max(4, L / 6);
                float smoothNoise = 0.0f;
                const float strAmp = 0.90f - 0.04f * static_cast<float>(s);
                for (int i = 0; i < L; ++i) {
                    const float tri = (i < pickPos)
                        ? (static_cast<float>(i) / static_cast<float>(pickPos))
                        : (static_cast<float>(L - i) / static_cast<float>(std::max(1, L - pickPos)));
                    rng = rng * 1664525u + 1013904223u;
                    const float wn = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);
                    smoothNoise = 0.58f * smoothNoise + 0.42f * wn;
                    const float saw = 1.0f - 2.0f * (static_cast<float>(i) / static_cast<float>(L));
                    const float newExcite = (tri * 0.55f + saw * 0.25f + smoothNoise * 0.30f) * strAmp;
                    st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i)] =
                        st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i)] * 0.15f + newExcite * 0.85f;
                }
                for (int i = L - 1; i >= pickPos; --i) {
                    st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i)] -=
                        0.40f * st.strBuf[static_cast<size_t>(s)][static_cast<size_t>(i - pickPos)];
                }
                st.pos[s] = 0;
            }
        }

        float rawChord = 0.0f;
        for (int s = 0; s < 6; ++s) {
            auto& buf = st.strBuf[static_cast<size_t>(s)];
            const int L = st.len[s];
            const int p = st.pos[s];
            const float cur = buf[static_cast<size_t>(p)];
            const int nextIdx = (p + 1 < L) ? (p + 1) : 0;
            const float next = buf[static_cast<size_t>(nextIdx)];
            const float avg  = 0.5f * (cur + next);
            const float damp = 0.09f;
            const float fb   = (barTime > 1.92) ? 0.960f : 0.9992f;
            st.lp[s] += damp * (avg - st.lp[s]);
            buf[static_cast<size_t>(p)] = (avg * (1.0f - damp) + st.lp[s] * damp) * fb;
            st.pos[s] = nextIdx;
            rawChord += cur * 0.24f;
        }

        float barEnv = 1.0f;
        if (barTime > 1.93) {
            barEnv = static_cast<float>(0.5 * (1.0 + std::cos(3.14159265358979323846 * ((barTime - 1.93) / 0.07))));
        }

        st.preHpLp += 0.018f * (rawChord - st.preHpLp);
        const float preDrive = (rawChord - st.preHpLp * 0.72f) * 3.8f;
        const float stage1   = std::tanh(preDrive + 0.14f * preDrive * preDrive) - 0.08f;
        const float stage2   = std::tanh(stage1 * 2.4f);

        constexpr float gCab = 0.31f;
        constexpr float kCab = 0.76f;
        const float v3 = stage2 - st.cabIc2;
        const float v1 = (st.cabIc1 + gCab * v3) / (1.0f + gCab * (gCab + kCab));
        const float v2 = st.cabIc2 + gCab * v1;
        st.cabIc1 = 2.0f * v1 - st.cabIc1;
        st.cabIc2 = 2.0f * v2 - st.cabIc2;
        const float cabVoiced = v2 * 0.76f + v1 * 0.42f;
        st.cabLp2 += 0.44f * (cabVoiced - st.cabLp2);

        const float distOut = std::clamp(st.cabLp2 * 0.68f * barEnv, -0.92f, 0.92f);
        const float dbl = st.cabDoubleBuf[(st.cabIdx + 1024 - 720) & 1023];
        st.cabDoubleBuf[st.cabIdx] = distOut;
        st.cabIdx = (st.cabIdx + 1) & 1023;

        outL = std::clamp(distOut, -0.92f, 0.92f);
        outR = std::clamp(distOut * 0.78f + dbl * 0.28f, -0.92f, 0.92f);
    }

    /**
     * @brief 120 BPM Karplus-Strong Physical Waveguide P-Bass DI Groove Generator.
     * Plays an A - E - D - A bass progression (4 quarter-note beats per bar, 8.0s loop) at 120 BPM.
     */
    static void synthesizeBassDiGroove(double t, uint32_t& rng, float& outL, float& outR, double sampleRate = 48000.0) noexcept {
        struct BassWaveguideState {
            int lastStep{-1};
            std::array<float, 2048> strBuf{};
            int pos{0};
            int len{873};
            float loopLp{0.0f};
            float feedback{0.9978f};
            float dampCoeff{0.16f};
            bool  reexcitedStep{false};
            float prevWaveOut{0.0f};
            double subPhase{0.0};
            double curFreq{55.0};
            float svfIc1{0.0f}, svfIc2{0.0f};
        };
        static thread_local BassWaveguideState st{};

        const double kSr = std::max(8000.0, sampleRate);
        // 120 BPM: 16 quarter-note beats across 4 bars (8.0s total, 4 beats = 2.0s per note: A1 -> E1 -> D2 -> A1)
        const double stepFloat = std::fmod(t * 2.0, 16.0);
        const int step = static_cast<int>(stepFloat) & 15;
        const int beatInBar = step & 3;
        const int chordIdx = (step >> 2) & 3;
        const double stepTime = (stepFloat - static_cast<double>(step)) * 0.5;

        static constexpr double kBassProgressionHz[4] = {
            55.00, // Bar 1: A1 (4 beats)
            41.20, // Bar 2: E1 (4 beats)
            73.42, // Bar 3: D2 (4 beats)
            55.00  // Bar 4: A1 (4 beats)
        };

        const bool isDownbeat = (beatInBar == 0);

        if (step != st.lastStep) {
            st.lastStep      = step;
            st.reexcitedStep = true;
            st.curFreq       = kBassProgressionHz[chordIdx];
            st.len           = std::clamp(static_cast<int>(std::round(kSr / st.curFreq)), 200, 2000);
            st.feedback      = 0.9978f;
            st.dampCoeff     = 0.16f;

            const int pluckPos = std::max(8, st.len / 4);
            float lpNoise = 0.0f;
            const float vel = isDownbeat ? 1.0f : 0.88f;
            for (int i = 0; i < st.len; ++i) {
                const float slope = (i < pluckPos)
                    ? (static_cast<float>(i) / static_cast<float>(pluckPos))
                    : (static_cast<float>(st.len - i) / static_cast<float>(std::max(1, st.len - pluckPos)));
                rng = rng * 1664525u + 1013904223u;
                const float wn = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);
                lpNoise = 0.68f * lpNoise + 0.32f * wn;
                const float saw = 1.0f - 2.0f * (static_cast<float>(i) / static_cast<float>(st.len));
                const float newExcite = (slope * 0.58f + saw * 0.34f + lpNoise * 0.24f) * vel;
                st.strBuf[static_cast<size_t>(i)] = st.strBuf[static_cast<size_t>(i)] * 0.25f + newExcite * 0.75f;
            }
            const int combOffset = std::max(4, st.len / 7);
            for (int i = st.len - 1; i >= combOffset; --i) {
                st.strBuf[static_cast<size_t>(i)] -= 0.35f * st.strBuf[static_cast<size_t>(i - combOffset)];
            }
            st.pos = 0;
        }

        float cur = st.strBuf[static_cast<size_t>(st.pos)];
        const int nextIdx = (st.pos + 1 < st.len) ? (st.pos + 1) : 0;
        const float next = st.strBuf[static_cast<size_t>(nextIdx)];
        const float avg  = 0.5f * (cur + next);
        st.loopLp += st.dampCoeff * (avg - st.loopLp);
        st.strBuf[static_cast<size_t>(st.pos)] = (avg * (1.0f - st.dampCoeff) + st.loopLp * st.dampCoeff) * st.feedback;
        st.pos = nextIdx;

        if (st.reexcitedStep && stepTime < 0.004) {
            const float ramp = static_cast<float>(0.5 * (1.0 - std::cos(3.14159265358979323846 * (stepTime / 0.004))));
            cur = st.prevWaveOut * (1.0f - ramp) + cur * ramp;
        } else {
            st.prevWaveOut = cur;
        }

        st.subPhase += st.curFreq / kSr;
        st.subPhase -= std::floor(st.subPhase);
        const float subEnv = static_cast<float>(std::exp(-stepTime * 5.5));
        const float subBody = std::sin(static_cast<float>(st.subPhase * kTwoPi)) * subEnv * 0.45f;

        const float clankEnv = static_cast<float>(std::exp(-stepTime * 115.0));
        const float fretClank = std::sin(static_cast<float>(stepTime * kTwoPi * 1180.0)) * clankEnv * (isDownbeat ? 0.16f : 0.11f);

        const float envBright = static_cast<float>(std::exp(-stepTime * 18.0));
        const float gLp = 0.065f + 0.185f * envBright;
        constexpr float kLp = 1.15f;
        const float rawBass = cur * 0.85f + subBody + fretClank;
        const float v3 = rawBass - st.svfIc2;
        const float v1 = (st.svfIc1 + gLp * v3) / (1.0f + gLp * (gLp + kLp));
        const float v2 = st.svfIc2 + gLp * v1;
        st.svfIc1 = 2.0f * v1 - st.svfIc1;
        st.svfIc2 = 2.0f * v2 - st.svfIc2;

        const float bassOut = std::tanh((v2 * 0.88f + v1 * 0.25f) * 1.45f) * 0.78f;
        outL = std::clamp(bassOut, -0.92f, 0.92f);
        outR = std::clamp(bassOut, -0.92f, 0.92f);
    }

    /**
     * @brief 120 BPM Sustained Clean "AAhhhhhhh" Vocal Progression (A - E - D - A).
     * Sings a 4-bar A3 -> E3 -> D3 -> A3 progression (4 beats = 2.0s per note, 8.0s loop) on an open "AAhhhhhhh" (/a/) vowel.
     */
    static void synthesizeVocalPhrase(double t, uint32_t& rng, float& outL, float& outR, double sampleRate = 48000.0) noexcept {
        struct VocalTractState {
            double glottalPhase{0.0};
            double vibPhase{0.0};
            double smoothedPitch{220.0};
            float  ic1[4]{}, ic2[4]{};
            float  breathIc1{0.0f}, breathIc2{0.0f};
            std::array<float, 2048> adtBuf{};
            size_t adtIdx{0};
        };
        static thread_local VocalTractState st{};

        const double kSr = std::max(8000.0, sampleRate);
        const int barIdx = static_cast<int>(std::floor(t * 0.5));
        const int chordIdx = barIdx & 3;
        const double barTime = std::fmod(t, 2.0);

        // 4-Bar Vocal Melody Progression: A3 -> E3 -> D3 -> A3 (4 beats = 2.0s per note)
        static constexpr double kVocalProgressionHz[4] = {
            220.00, // Bar 1: A3
            164.81, // Bar 2: E3
            146.83, // Bar 3: D3
            220.00  // Bar 4: A3
        };
        const double targetPitchHz = kVocalProgressionHz[chordIdx];
        st.smoothedPitch += 0.0025 * (targetPitchHz - st.smoothedPitch);
        const double kPitchHz = st.smoothedPitch;
        static constexpr float kAhFormants[4] = { 730.0f, 1150.0f, 2540.0f, 3320.0f };

        float sylEnv = 0.0f;
        if (barTime < 0.065) {
            sylEnv = static_cast<float>(0.5 * (1.0 - std::cos(3.14159265358979323846 * (barTime / 0.065))));
        } else if (barTime < 1.700) {
            sylEnv = 1.0f;
        } else if (barTime < 1.840) {
            sylEnv = static_cast<float>(0.5 * (1.0 + std::cos(3.14159265358979323846 * ((barTime - 1.700) / 0.140))));
        } else {
            sylEnv = 0.0f;
        }

        st.vibPhase += 5.4 / kSr;
        st.vibPhase -= std::floor(st.vibPhase);
        const double vibBloom = std::clamp((barTime - 0.25) * 1.5, 0.0, 1.0);
        const double instFreq = kPitchHz * (1.0 + vibBloom * 0.0065 * std::sin(st.vibPhase * kTwoPi));

        st.glottalPhase += instFreq / kSr;
        st.glottalPhase -= std::floor(st.glottalPhase);

        const float p = static_cast<float>(st.glottalPhase);
        constexpr float kOq = 0.62f;
        constexpr float kTp = 0.44f;
        float glottalPulse = 0.0f;
        if (p < kTp) {
            const float x = p / kTp;
            glottalPulse = 3.0f * x * x - 2.0f * x * x * x;
        } else if (p < kOq) {
            const float x = (p - kTp) / (kOq - kTp);
            glottalPulse = 1.0f - x * x;
        } else {
            glottalPulse = 0.0f;
        }
        const float lipRad = (glottalPulse - 0.36f) + ((p >= kTp && p < kOq) ? -0.85f * ((p - kTp) / (kOq - kTp)) : 0.15f);

        rng = rng * 1664525u + 1013904223u;
        const float wn = ((static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f);
        constexpr float gBr = 0.26f;
        constexpr float kBr = 0.90f;
        const float bv3 = wn - st.breathIc2;
        const float bv1 = (st.breathIc1 + gBr * bv3) / (1.0f + gBr * (gBr + kBr));
        const float bv2 = st.breathIc2 + gBr * bv1;
        st.breathIc1 = 2.0f * bv1 - st.breathIc1;
        st.breathIc2 = 2.0f * bv2 - st.breathIc2;
        (void)bv2;

        const float sourceSig = lipRad * 0.92f + bv1 * (0.020f + 0.025f * glottalPulse);

        static constexpr float kBandwidths[4] = { 85.0f, 115.0f, 145.0f, 190.0f };
        static constexpr float kFormantGains[4] = { 1.00f, 0.74f, 0.42f, 0.26f };
        float tractOut = sourceSig * 0.14f;
        for (int b = 0; b < 4; ++b) {
            const float fc = kAhFormants[b];
            const float g  = std::tan(3.14159265f * fc / static_cast<float>(kSr));
            const float k  = kBandwidths[b] / std::max(100.0f, fc);
            const float v3 = sourceSig - st.ic2[b];
            const float v1 = (st.ic1[b] + g * v3) / (1.0f + g * (g + k));
            const float v2 = st.ic2[b] + g * v1;
            st.ic1[b] = 2.0f * v1 - st.ic1[b];
            st.ic2[b] = 2.0f * v2 - st.ic2[b];
            tractOut += v1 * kFormantGains[b];
        }

        const float monoVocal = std::tanh(tractOut * 1.12f) * 0.58f * sylEnv;

        const float lfoMod = 980.0f + 22.0f * std::sin(static_cast<float>(t * 1.8));
        const size_t dInt  = static_cast<size_t>(lfoMod);
        const float  dFrac = lfoMod - static_cast<float>(dInt);
        const float  sA    = st.adtBuf[(st.adtIdx + 2048 - dInt) & 2047];
        const float  sB    = st.adtBuf[(st.adtIdx + 2048 - dInt - 1) & 2047];
        const float  adtVocal = sA + dFrac * (sB - sA);
        st.adtBuf[st.adtIdx] = monoVocal;
        st.adtIdx = (st.adtIdx + 1) & 2047;

        outL = std::clamp(monoVocal * 0.85f + adtVocal * 0.25f, -0.92f, 0.92f);
        outR = std::clamp(monoVocal * 0.72f - adtVocal * 0.35f, -0.92f, 0.92f);
    }
};

} // namespace avra::audio_io
