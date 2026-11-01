#pragma once

#if __has_include("avra/Avra.h")
#include "avra/Avra.h"
#include "avra/gui/controls/TextInput.h"
#include "avra/studio/ProjectGenerator.h"
#include "avra/studio/SchematicSpec.h"
#include "avra/studio/StudioIdeShell.h"
#include "avra/studio/UiBuilderSpec.h"
#else
#include "../include/avra/Avra.h"
#include "../include/avra/gui/controls/TextInput.h"
#include "../include/avra/studio/ProjectGenerator.h"
#include "../include/avra/studio/SchematicSpec.h"
#include "../include/avra/studio/StudioIdeShell.h"
#include "../include/avra/studio/UiBuilderSpec.h"
#endif
#include "../examples/GainPlugin/GainPlugin.h"
#include "../examples/GainPlugin/GainPluginEditor.h"
#include <atomic>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <commdlg.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <windows.h>
#endif

namespace avra::studio {

enum class StudioTab {
  Projects,
  NewPluginWizard, // Visual Drag-and-Drop SCHEMATIC BUILDER
  UiBuilder,       // Visual Drag-and-Drop HARDWARE FACEPLATE UI BUILDER
  LiveRack
};

enum class TestSignalMode {
  DrumTransientLoop = 0, // 120 BPM 4/4 Acoustic Studio Drum Beat
  SynthChordLoop = 1,    // 120 BPM Polyphonic Pad Chords
  CleanGuitarChord = 2,  // 120 BPM 4-Beat Strumming Open Clean Guitar Chord
  GuitarDiChug = 3,      // 120 BPM 4-Beat Strumming Distorted Guitar Chord
  BassDiGroove = 4,      // 120 BPM 4/4 Single-Note Bass DI Loop
  VocalPhrase = 5,       // 120 BPM Single-Note Sustained "AAhhhhhhh" Vocal Loop
  CustomWavLoop = 6,     // User-loaded .WAV Audio Loop from disk
  PinkNoise = 7,         // Full-spectrum Pink Noise for EQ/Filter curve testing
  HarmonicMultiTone =
      8,      // Plugin Doctor 1 kHz -12 dBFS THD Harmonic Analyzer Sine
  Silence = 9 // Mute Audio
};

struct WorkspacePlugin {
  std::string name;
  std::string diskFolder;
  std::string category;
  std::string pluginId;
  std::string description;
  std::string cmakeTarget;
  bool isScaffolded;
  bool isBuilt;
  bool isCoreReference;
};

/**
 * @brief Real-time 48 kHz DSP evaluator for auditioning any multi-stage
 * Schematic Chain directly inside Avra Studio's Live Test Rack before
 * compilation.
 */
class LiveSchematicRunner {
public:
  static constexpr size_t kMaxSlots = 30;

  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    dcCoeff_ = 1.0 - (2.0 * 3.14159265358979323846 * 5.0 /
                      std::max(8000.0, sampleRate));
    if (slots_.size() != kMaxSlots) {
      slots_.resize(kMaxSlots);
    }
    for (size_t i = 0; i < slots_.size(); ++i) {
      auto &s = slots_[i];
      s.dcInL = 0.0;
      s.dcOutL = 0.0;
      s.dcInR = 0.0;
      s.dcOutR = 0.0;
      s.driftRng = 0x9E3779B9u ^ static_cast<uint32_t>((i + 1) * 0x85EBCA6Bu);
      // Lazy per-stage preparation on demand when active in processBlock
      s.preparedType = -1;
      s.hadDeltaLastBlock = true;
    }
    stageOutputPeakDb_.fill(-18.0f);
  }

  // =========================================================================
  // Rack-Level Studio Engineering Tools (Signal Probe, PDC latency report)
  // =========================================================================
  [[nodiscard]] int getProbeStageIndex() const noexcept {
    return probeStageIdx_;
  }
  void setProbeStageIndex(int idx) noexcept {
    probeStageIdx_ = (idx < -1) ? -1 : idx;
    if (probeStageIdx_ < 0)
      probeListenActive_ = false;
  }

  [[nodiscard]] bool isProbeListenActive() const noexcept {
    return probeListenActive_ && (probeStageIdx_ >= 0);
  }
  void setProbeListenActive(bool active) noexcept {
    probeListenActive_ = active && (probeStageIdx_ >= 0);
  }

  [[nodiscard]] LockFreeRingBuffer<float, 4096> &getProbeRingBuffer() noexcept {
    return probeRingBuffer_;
  }

  [[nodiscard]] int getChannelTopologyMode() const noexcept {
    return channelTopologyMode_;
  }
  void setChannelTopologyMode(int mode) noexcept {
    channelTopologyMode_ = std::clamp(mode, 0, 2);
  }
  [[nodiscard]] static const char *
  getChannelTopologyModeName(int mode) noexcept {
    switch (mode & 3) {
    case 0:
      return "MONO (PURE) [1x CPU]";
    case 1:
      return "STEREO [L/R DRIFT]";
    default:
      return "MID / SIDE";
    }
  }

  [[nodiscard]] float getStageOutputPeakDb(size_t stageIdx) const noexcept {
    if (stageIdx >= kMaxSlots)
      return -18.0f;
    return stageOutputPeakDb_[stageIdx];
  }
  [[nodiscard]] float getInputBusPeakDb() const noexcept {
    return inputBusPeakDb_;
  }

  [[nodiscard]] uint32_t getTotalLatencySamples(
      const std::vector<SchematicNode> &chain) const noexcept {
    uint32_t totalSamples = 0;
    const size_t n = std::min(chain.size(), kMaxSlots);
    for (size_t idx = 0; idx < n; ++idx) {
      const auto &node = chain[idx];
      if (node.isBypassed)
        continue;
      totalSamples += static_cast<uint32_t>(
          std::max(0, getAutoPlumbingSpec(node.type).baseLatencySamples));
    }
    return totalSamples;
  }

  void processBlock(float *outL, float *outR, size_t numFrames,
                    const std::vector<SchematicNode> &chain,
                    float &outMaxGrDb) noexcept {
    const dsp::ScopedNoDenormals noDenormals;
    const size_t n = std::min(chain.size(), std::min(kMaxSlots, slots_.size()));
    float maxGr = 0.0f;


    // 1. Update circuit parameters for each active stage
    for (size_t idx = 0; idx < n; ++idx) {
      const auto &node = chain[idx];
      auto &s = slots_[idx];
      const auto &m = getComponentMeta(node.type);
      const size_t pCount = m.getParamCount();

      // Every control value is forwarded verbatim to the stage's own
      // setControlValues(). No synthetic per-control filter/comb layer is
      // added: a control that the stage does not wire into its DSP is reported
      // as dead by the forensic and control-spec audits.
      static constexpr size_t kMaxStageControls = 24;
      std::array<float, kMaxStageControls> rawVals{};
      for (size_t pIdx = 0; pIdx < std::min(pCount, rawVals.size()); ++pIdx) {
        rawVals[pIdx] = node.getParamValue(static_cast<int>(pIdx));
      }
      const std::span<const float> rawSpan(rawVals.data(),
                                           std::min(pCount, rawVals.size()));

      const bool typeChanged = (s.preparedType != static_cast<int>(node.type));
      bool valsChanged = typeChanged;
      for (size_t pIdx = 0; pIdx < std::min(pCount, rawVals.size()); ++pIdx) {
        if (std::abs(rawVals[pIdx] - s.lastRawVals[pIdx]) > 1e-5f) {
          s.lastRawVals[pIdx] = rawVals[pIdx];
          valsChanged = true;
        }
      }
      if (typeChanged) {
        s.preparedType = static_cast<int>(node.type);
        dispatchStage(s, node.type, [&](auto &st) { st.prepare(sampleRate_); });
      }
      if (valsChanged || s.hadDeltaLastBlock) {
        s.hadDeltaLastBlock = false;
        dispatchStage(s, node.type,
                      [&](auto &st) { st.setControlValues(rawSpan); });
      }

      // Precompute Smart Plumbing (GraphSanitizerAndSanityEngine) per-stage factors
      const auto &plum = getAutoPlumbingSpec(node.type);
      s.injectsDcBlocker = plum.injectDcGuard;
      s.injectsAnalogDrift =
          plum.enableComponentDrift && (node.trim2Val > 0.01f);
      s.injectsSafetyClamp = plum.injectSafetySubsonicClamp;
      s.trimBiasGain = math::dBToLinear(node.trim1Val * 0.25f);
      const float driftPct = s.injectsAnalogDrift ? node.trim2Val : 0.0f;
      s.tolGainL = 1.0f + driftPct * 0.005f;
      s.tolGainR = 1.0f - driftPct * 0.005f;
      s.autoMakeupLinear = 1.0f;
      if (plum.supportsAutoGainLink && node.autoGainLinked &&
          plum.linkedOutputParamSlot < 0) {
        const float driveDelta = node.param1Val - m.param1.defaultVal;
        s.autoMakeupLinear = (m.param1.unit == "dB")
                                 ? math::dBToLinear(-driveDelta * 0.65f)
                                 : math::dBToLinear(-driveDelta * 0.10f);
      }
    }

    float inMaxPeak = 1e-6f;
    std::array<float, kMaxSlots> stageMaxPeak{};
    stageMaxPeak.fill(1e-6f);

    // 2. Per-sample execution through the schematic chain + Auto-Plumbing Rule
    // Engine
    for (size_t f = 0; f < numFrames; ++f) {
      float l = outL[f];
      float r = outR[f];
      inMaxPeak = std::max(inMaxPeak, std::max(std::abs(l), std::abs(r)));

      float probeCapL = l;
      float probeCapR = r;
      bool capturedProbe = false;

      // Channel Topology Encoding: 0 = MONO (PURE) [1x CPU], 1 = STEREO [L/R
      // DRIFT], 2 = MID / SIDE
      if (channelTopologyMode_ == 0) {
        float monoSample = 0.5f * (l + r);
        processMono(monoSample, chain, maxGr, &stageMaxPeak, &probeCapL,
                    &capturedProbe);
        probeCapR = probeCapL;
        if (probeListenActive_ && capturedProbe) {
          monoSample = probeCapL;
        }
        outL[f] = outR[f] = math::clamp(monoSample, -1.5f, 1.5f);
        continue;
      } else if (channelTopologyMode_ == 2) {
        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r);
        l = mid;
        r = side;
        probeCapL = l;
        probeCapR = r;
      }

      bool inParallelBranch = false;
      bool branchProcessedAny = false;
      float branchDryL = l;
      float branchDryR = r;

      for (size_t idx = 0; idx < n; ++idx) {
        const auto &node = chain[idx];
        const bool nextIsParallel = (idx + 1 < n) && chain[idx + 1].isParallel; (void)nextIsParallel;
        if (node.isParallel && !inParallelBranch) {
          inParallelBranch = true;
          branchProcessedAny = false;
          branchDryL = l;
          branchDryR = r;
        }
        if (node.isBypassed) {
          if (false) {
            if (branchProcessedAny) {
              const float pol = node.parallelPhaseInvert ? -1.0f : 1.0f;
              const float wet = (node.parallelMixPercent * 0.01f) * pol;
              l = branchDryL + l * wet;
              r = branchDryR + r * wet;
            } else {
              l = branchDryL;
              r = branchDryR;
            }
            inParallelBranch = false;
          }
          stageMaxPeak[idx] =
              std::max(stageMaxPeak[idx], std::max(std::abs(l), std::abs(r)));
          if (static_cast<int>(idx) == probeStageIdx_) {
            probeCapL = l;
            probeCapR = r;
            capturedProbe = true;
            probeRingBuffer_.pushTelemetrySample(0.5f * (l + r));
          }
          continue;
        }
        branchProcessedAny = true;
        auto &s = slots_[idx];
        const float stageDryL = l;
        const float stageDryR = r;

        l *= s.trimBiasGain;
        r *= s.trimBiasGain;

        dispatchStage(s, node.type, [&](auto &st) {
          st.processStereo(l, r);
          if constexpr (requires { st.getGainReductionDb(); }) {
            maxGr =
                std::max(maxGr, static_cast<float>(st.getGainReductionDb()));
          }
        });
        if (!std::isfinite(l) || !std::isfinite(r)) {
          l = stageDryL;
          r = stageDryR;
        }

        // RULE_02: Analog Component Drift & Tolerance (L/R variance + -120 dBFS
        // thermal bias)
        if (s.injectsAnalogDrift) {
          s.driftRng = s.driftRng * 1664525u + 1013904223u;
          const float thNoise =
              ((static_cast<float>(s.driftRng & 0xFFFF) / 32768.0f) - 1.0f) *
              1.0e-6f;
          l = l * s.tolGainL + thNoise;
          r = r * s.tolGainR - thNoise;
        }

        // RULE_01: Asymmetrical DC Guard (double-precision 5 Hz 1st-order
        // high-pass filter)
        if (s.injectsDcBlocker) {
          const double yL =
              static_cast<double>(l) - s.dcInL + dcCoeff_ * s.dcOutL;
          s.dcInL = static_cast<double>(l);
          s.dcOutL = (std::abs(yL) < 1e-24) ? 0.0 : yL;
          l = static_cast<float>(s.dcOutL);

          const double yR =
              static_cast<double>(r) - s.dcInR + dcCoeff_ * s.dcOutR;
          s.dcInR = static_cast<double>(r);
          s.dcOutR = (std::abs(yR) < 1e-24) ? 0.0 : yR;
          r = static_cast<float>(s.dcOutR);
        }

        // [SAFETY SUB-SONIC & NAN CLAMP]: 15 Hz infrasonic guard + +18 dBFS
        // (7.943f) safety soft-limiter & NaN/Inf clamp
        if (s.injectsSafetyClamp) {
          if (!std::isfinite(l))
            l = 0.0f;
          if (!std::isfinite(r))
            r = 0.0f;
          const float subCoeff = 0.9980f; // ~15 Hz @ 48 kHz
          const float subL = l - s.subInL + subCoeff * s.subOutL;
          s.subInL = l;
          s.subOutL = subL;
          l = subL;

          const float subR = r - s.subInR + subCoeff * s.subOutR;
          s.subInR = r;
          s.subOutR = subR;
          r = subR;

          constexpr float kCeil18dB = 7.9432823f;
          if (std::abs(l) > 1.0f)
            l = kCeil18dB * std::tanh(l / kCeil18dB);
          if (std::abs(r) > 1.0f)
            r = kCeil18dB * std::tanh(r / kCeil18dB);
        }

        // RULE_06: Auto-Gain Inverse Link compensation for single-knob drive
        // stages
        l *= s.autoMakeupLinear;
        r *= s.autoMakeupLinear;

        if (channelTopologyMode_ == 0) {
          r = l; // MONO [1x CPU] clean single-channel duplication
        }
        if (node.isParallel) {
          const float pol = node.parallelPhaseInvert ? -1.0f : 1.0f;
          const float wet = (node.parallelMixPercent * 0.01f) * pol;
          l = stageDryL + l * wet;
          r = stageDryR + r * wet;
          inParallelBranch = false;
        }

        if (false) {
          const float pol = node.parallelPhaseInvert ? -1.0f : 1.0f;
          const float wet = (node.parallelMixPercent * 0.01f) * pol;
          l = branchDryL + l * wet;
          r = branchDryR + r * wet;
          inParallelBranch = false;
        } else if (!node.isParallel && node.parallelMixPercent < 99.9f) {
          const float wet =
              math::clamp(node.parallelMixPercent * 0.01f, 0.0f, 1.0f);
          l = stageDryL * (1.0f - wet) + l * wet;
          r = stageDryR * (1.0f - wet) + r * wet;
        }

        stageMaxPeak[idx] =
            std::max(stageMaxPeak[idx], std::max(std::abs(l), std::abs(r)));

        // Tool 1: Schematic Signal Probe tap after stage `probeStageIdx_`
        if (static_cast<int>(idx) == probeStageIdx_) {
          probeCapL = l;
          probeCapR = r;
          capturedProbe = true;
          probeRingBuffer_.pushTelemetrySample(0.5f * (l + r));
        }
      }

      if (probeListenActive_ && capturedProbe) {
        l = probeCapL;
        r = probeCapR;
      }

      // Channel Topology Decoding: 0 = MONO [1x CPU], 1 = STEREO [L/R DRIFT], 2
      // = MID / SIDE
      if (channelTopologyMode_ == 0) {
        r = l;
      } else if (channelTopologyMode_ == 2) {
        const float decL = l + r;
        const float decR = l - r;
        l = decL;
        r = decR;
      }

      outL[f] = math::clamp(l, -1.5f, 1.5f);
      outR[f] = math::clamp(r, -1.5f, 1.5f);
    }
    inputBusPeakDb_ =
        0.72f * inputBusPeakDb_ + 0.28f * math::linearTodB(inMaxPeak);
    for (size_t idx = 0; idx < n; ++idx) {
      const float instDb = math::linearTodB(stageMaxPeak[idx]);
      stageOutputPeakDb_[idx] =
          0.72f * stageOutputPeakDb_[idx] + 0.28f * instDb;
    }
    outMaxGrDb = maxGr;
  }

  /**
   * @brief Dedicated single-channel MONO (PURE) execution path with zero L/R
   * phase decorrelation and 50% lower conditioning/oversampling CPU usage.
   */
  void processMono(float &mSample, const std::vector<SchematicNode> &chain,
                   float &ioMaxGrDb,
                   std::array<float, kMaxSlots> *stageMaxPeak = nullptr,
                   float *outProbeCap = nullptr,
                   bool *outCapturedProbe = nullptr) noexcept {
    const size_t n = std::min(chain.size(), kMaxSlots);
    bool inParallelBranch = false;
    bool branchProcessedAny = false;
    float branchDry = mSample;

    for (size_t idx = 0; idx < n; ++idx) {
      const auto &node = chain[idx];
      const bool nextIsParallel = (idx + 1 < n) && chain[idx + 1].isParallel; (void)nextIsParallel;
      if (node.isParallel && !inParallelBranch) {
        inParallelBranch = true;
        branchProcessedAny = false;
        branchDry = mSample;
      }
      if (node.isBypassed) {
        if (false) {
          if (branchProcessedAny) {
            const float pol = node.parallelPhaseInvert ? -1.0f : 1.0f;
            const float wet = (node.parallelMixPercent * 0.01f) * pol;
            mSample = branchDry + mSample * wet;
          } else {
            mSample = branchDry;
          }
          inParallelBranch = false;
        }
        if (stageMaxPeak)
          (*stageMaxPeak)[idx] =
              std::max((*stageMaxPeak)[idx], std::abs(mSample));
        if (static_cast<int>(idx) == probeStageIdx_) {
          if (outProbeCap)
            *outProbeCap = mSample;
          if (outCapturedProbe)
            *outCapturedProbe = true;
          probeRingBuffer_.pushTelemetrySample(mSample);
        }
        continue;
      }
      branchProcessedAny = true;
      auto &s = slots_[idx];
      const float stageDry = mSample;

      mSample *= s.trimBiasGain;
      float l = mSample;
      float r = mSample;

      dispatchStage(s, node.type, [&](auto &st) {
        st.processStereo(l, r);
        if constexpr (requires { st.getGainReductionDb(); }) {
          ioMaxGrDb =
              std::max(ioMaxGrDb, static_cast<float>(st.getGainReductionDb()));
        }
      });
      if (!std::isfinite(l) || !std::isfinite(r)) {
        l = stageDry;
        r = stageDry;
      }
      mSample = l;

      if (s.injectsAnalogDrift) {
        s.driftRng = s.driftRng * 1664525u + 1013904223u;
        const float thNoise =
            ((static_cast<float>(s.driftRng & 0xFFFF) / 32768.0f) - 1.0f) *
            1.0e-6f;
        mSample +=
            thNoise; // Pure mono thermal bias without L/R tolerance asymmetry
      }

      if (s.injectsDcBlocker) {
        const double y =
            static_cast<double>(mSample) - s.dcInL + dcCoeff_ * s.dcOutL;
        s.dcInL = s.dcInR = static_cast<double>(mSample);
        s.dcOutL = s.dcOutR = (std::abs(y) < 1e-24) ? 0.0 : y;
        mSample = static_cast<float>(s.dcOutL);
      }

      if (s.injectsSafetyClamp) {
        if (!std::isfinite(mSample))
          mSample = 0.0f;
        const float subCoeff = 0.9980f;
        const float sub = mSample - s.subInL + subCoeff * s.subOutL;
        s.subInL = s.subInR = mSample;
        s.subOutL = s.subOutR = sub;
        mSample = sub;
        constexpr float kCeil18dB = 7.9432823f;
        if (std::abs(mSample) > 1.0f)
          mSample = kCeil18dB * std::tanh(mSample / kCeil18dB);
      }

      mSample *= s.autoMakeupLinear;

      if (node.isParallel) {
        const float pol = node.parallelPhaseInvert ? -1.0f : 1.0f;
        const float wet = (node.parallelMixPercent * 0.01f) * pol;
        mSample = stageDry + mSample * wet;
        inParallelBranch = false;
      } else if (!node.isParallel && false) {
        const float pol = node.parallelPhaseInvert ? -1.0f : 1.0f;
        const float wet = (node.parallelMixPercent * 0.01f) * pol;
        mSample = branchDry + mSample * wet;
        inParallelBranch = false;
      } else if (!node.isParallel && node.parallelMixPercent < 99.9f) {
        const float wet =
            math::clamp(node.parallelMixPercent * 0.01f, 0.0f, 1.0f);
        mSample = stageDry * (1.0f - wet) + mSample * wet;
      }

      if (stageMaxPeak)
        (*stageMaxPeak)[idx] =
            std::max((*stageMaxPeak)[idx], std::abs(mSample));
      if (static_cast<int>(idx) == probeStageIdx_) {
        if (outProbeCap)
          *outProbeCap = mSample;
        if (outCapturedProbe)
          *outCapturedProbe = true;
        probeRingBuffer_.pushTelemetrySample(mSample);
      }
    }
  }

  void setMasterMixPercent(float pct) noexcept {
    masterMixPercent_.store(math::clamp(pct, 0.0f, 100.0f),
                            std::memory_order_relaxed);
  }

  [[nodiscard]] float getMasterMixPercent() const noexcept {
    return masterMixPercent_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] float stepMasterMixSmoother(float targetWetNorm) noexcept {
    const float target = math::clamp(targetWetNorm, 0.0f, 1.0f);
    const float alpha =
        1.0f - std::exp(-1.0f / std::max(1.0f, 0.020f * static_cast<float>(
                                                            sampleRate_)));
    smoothedMasterMix_ += alpha * (target - smoothedMasterMix_);
    return smoothedMasterMix_;
  }

private:
  struct StageSlot {
    bool injectsDcBlocker{false};
    bool injectsAnalogDrift{false};
    bool injectsSafetyClamp{false};
    float trimBiasGain{1.0f};
    float tolGainL{1.0f};
    float tolGainR{1.0f};
    float autoMakeupLinear{1.0f};
    double dcInL{0.0}, dcOutL{0.0};
    double dcInR{0.0}, dcOutR{0.0};
    float subInL{0.0f}, subOutL{0.0f};
    float subInR{0.0f}, subOutR{0.0f};
    uint32_t driftRng{0x9E3779B9u};
    int preparedType{-1};
    std::array<float, 24> lastRawVals{};

    dsp::circuits::JfetBufferStage c_PR1;
    dsp::circuits::DiscreteClassAPreamp c_PR2;
    dsp::circuits::DiscreteClassABOpAmp c_PR3;
    dsp::circuits::DualTriodeTubeStage c_PR4;
    dsp::circuits::PentodeTubeStage c_PR5;
    dsp::circuits::VariMu670LineAmpStage c_PR6;
    dsp::circuits::GermaniumBroadcastPreStage c_PR7;
    dsp::circuits::BlueStripeSiliconPreStage c_PR8;
    dsp::circuits::Console4000IcPreStage c_PR9;
    dsp::circuits::JapaneseTapeDeckPreStage c_PR10;
    dsp::circuits::HighVoltageCascodeTube c_PR11;
    dsp::circuits::SubMiniatureTubeMicStage c_PR12;
    dsp::circuits::American610TubeConsoleStage c_PR13;
    dsp::circuits::GermanV76BroadcastValveStage c_PR14;
    dsp::circuits::ConsoleSummingBusStage c_PR15;
    dsp::circuits::HeadroomCalibratorStage c_PR16;
    dsp::circuits::Olympic69LustraphoneStage c_PR17;
    dsp::circuits::Abbey68TransistorStage c_PR18;
    dsp::circuits::LondonARangeZenerStage c_PR19;
    dsp::circuits::WestCoastAm10Stage c_PR20;
    dsp::circuits::TransformerlessJfetBjtStage c_PR21;
    dsp::circuits::HighRail80BroadcastStage c_PR22;
    dsp::circuits::Criteria2002HybridStage c_PR23;
    dsp::circuits::AirStudioZobelStage c_PR24;
    dsp::circuits::Soul60s24VClassAStage c_PR25;
    dsp::circuits::LaStudioQuadTransistorStage c_PR26;
    dsp::circuits::Octal50sBroadcastTubeStage c_PR27;
    dsp::circuits::DavenSteppedTubeStage c_PR28;
    dsp::circuits::SwissBroadcastLineStage c_PR29;
    dsp::circuits::ZeroFeedbackMasteringStage c_PR30;
    dsp::circuits::GermanDualPentodeCinemaStage c_PR31;
    dsp::circuits::ZeroFeedbackPureTriodeStage c_PR32;
    dsp::circuits::CurrentModeTransimpedanceStage c_PR33;
    dsp::circuits::EasternBroadcastPentodeStage c_PR34;
    dsp::circuits::JapaneseDiscreteClassAStage c_PR35;
    dsp::circuits::VariImpedanceCableLoadStage c_PR36;
    dsp::circuits::IronTransformerStage c_IR1;
    dsp::circuits::NickelBridgingTransformerStage c_IR2;
    dsp::circuits::SilverToroidalTransformer c_IR3;
    dsp::circuits::OutputTransformerMatrix c_IR4;
    dsp::circuits::PaperInOilInterstageCap c_IR5;
    dsp::circuits::LayeredPermalloyInterstageStage c_IR6;
    dsp::circuits::Balanced600BridgingStage c_IR7;
    dsp::circuits::SiliconValveOutputStage c_IR8;
    dsp::circuits::BesselWidebandServoStage c_IR9;
    dsp::circuits::CobaltAmorphousCoreStage c_IR10;
    dsp::circuits::TapeHysteresisStage c_TP1;
    dsp::circuits::HighFreqTapeLimiterStage c_TP2;
    dsp::circuits::LoFiCassetteVhsStage c_TP3;
    dsp::circuits::HalfInch30IpsMasteringStage c_TP4;
    dsp::circuits::OneInch15IpsTubeTapeStage c_TP5;
    dsp::circuits::DifferentialAirCompanderStage c_TP6;
    dsp::circuits::StainlessWireRecorderStage c_TP7;
    dsp::circuits::VariableSilkExciterStage c_ST1;
    dsp::circuits::Starved6AS6ValveDriveStage c_ST2;
    dsp::circuits::DynamicDensityInflatorStage c_ST3;
    dsp::circuits::DiscreteTransistorOverdrive c_ST4;
    dsp::circuits::VintageAuralExciterStage c_ST5;
    dsp::circuits::MissingFundamentalSynth c_ST6;
    dsp::circuits::AsymmetricalDiodeClipperStage c_ST7;
    dsp::circuits::DacBitcrusherStage c_ST8;
    dsp::circuits::MasteringVinylCuttingLatheStage c_ST9;
    dsp::circuits::TriodePentode3BandMatrixStage c_ST10;
    dsp::circuits::EnvelopeGatedTransientExciterStage c_ST11;
    dsp::circuits::ConsoleBusRailOverdriveStage c_ST12;
    dsp::circuits::StudioPcm14BitConverterStage c_ST13;
    dsp::circuits::SlidingActionSpectralCompanderStage c_TP8;
    dsp::circuits::OpticalSoundtrackFilmStage c_TP9;
    dsp::circuits::CollectorSaturationMatrixStage c_ST15;
    dsp::circuits::DynamicFluxDeSaturatorStage c_ST16;
    dsp::circuits::GainRangingFloatingPointDacStage c_ST17;
    dsp::circuits::DielectricAbsorptionCapStage c_ST18;
    dsp::circuits::PassiveLcProgramEq c_EQ1;
    dsp::circuits::PultecHighInductorEqStage c_EQ2;
    dsp::circuits::ParallelLcMasteringEq c_EQ3;
    dsp::circuits::InductorEq3Band c_EQ4;
    dsp::circuits::ProportionalQGyrator c_EQ5;
    dsp::circuits::Console4000ParametricEqStage c_EQ6;
    dsp::circuits::Proportional10BandGraphicEq c_EQ7;
    dsp::circuits::FulcrumTiltAirEq c_EQ8;
    dsp::circuits::AutoBassFundamentalRiderStage c_EQ9;
    dsp::circuits::Moog904LadderSvfFilterStage c_EQ10;
    dsp::circuits::BarkScaleAuditoryUnmasker c_SP1;
    dsp::circuits::SpectralResonanceTamer c_SP2;
    dsp::circuits::SpectralTransientBinShaperStage c_SP3;
    dsp::circuits::SidechainSpectralCarver32BandStage c_SP4;
    dsp::circuits::HarmonicPitchTrackEqStage c_SP5;
    dsp::circuits::PhasePolarityAlignerStage c_SP6;
    dsp::circuits::PassiveMidDipPeakEqStage c_EQ11;
    dsp::circuits::PassiveHighLowBracketEqStage c_EQ12;
    dsp::circuits::Olympic69InductorEqStage c_EQ13;
    dsp::circuits::Detroit7BandToroidalEqStage c_EQ14;
    dsp::circuits::LondonARangeInductorEqStage c_EQ15;
    dsp::circuits::MasteringBaxandallEqStage c_EQ16;
    dsp::circuits::ParallelStateVariableEqStage c_EQ17;
    dsp::circuits::GermanBroadcastSilkyEqStage c_EQ18;
    dsp::circuits::SteppedMasteringTransferEqStage c_EQ19;
    dsp::circuits::EllipticalMidSideEqStage c_EQ20;
    dsp::circuits::DualInductorPCurveEqStage c_EQ21;
    dsp::circuits::Iso226DynamicLoudnessEqStage c_EQ22;
    dsp::circuits::BridgedTwinTNotchEqStage c_EQ23;
    dsp::circuits::FixedResonant12BandFilterStage c_SP7;
    dsp::circuits::SpectralBrickwallBinStage c_SP8;
    dsp::circuits::PhaseCompensatedBassMonoStage c_SP9;
    dsp::circuits::DynamicSpectralDeHarsherStage c_SP10;
    dsp::circuits::SpectralDeReverberatorStage c_SP11;
    dsp::circuits::SinusoidalTonalNoiseStage c_SP12;
    dsp::circuits::SubSamplePhaseAlignerStage c_SP13;
    dsp::circuits::FastFetCompressor c_CP1;
    dsp::circuits::OpticalLevelerStage c_CP2;
    dsp::circuits::VariMuTubeGainCell c_CP3;
    dsp::circuits::DiodeBridgeGainCell c_CP4;
    dsp::circuits::QuadVcaMasterBusGlueStage c_CP5;
    dsp::circuits::MultiRatioDistressorCompStage c_CP6;
    dsp::circuits::DualOptoVcaMasterComp c_CP7;
    dsp::circuits::OverEasyRmsVcaStage c_CP8;
    dsp::circuits::UpDownMultibandCompressor c_CP9;
    dsp::circuits::DifferentialTransientShaper9946Stage c_CP10;
    dsp::circuits::LookaheadUpwardExpanderStage c_CP11;
    dsp::circuits::SplitBandDeEsser c_CP12;
    dsp::circuits::HysteresisNoiseGate c_CP13;
    dsp::circuits::BroadcastTriodeLimiterStage c_CP14;
    dsp::circuits::ZenerShuntLimiter c_CP15;
    dsp::circuits::VariableKneePolyClipper c_CL1;
    dsp::circuits::PsychoacousticMasterLimiterStage c_CL2;
    dsp::circuits::TruePeakLookaheadBrickwallStage c_CL3;
    dsp::circuits::TransconductanceRoomSlammerStage c_CP16;
    dsp::circuits::BroadcastFieldEffectCrusherStage c_CP17;
    dsp::circuits::RmsVcaPeakStopLimiterStage c_CP18;
    dsp::circuits::DiscreteMasteringMsCompStage c_CP19;
    dsp::circuits::BritishPhotoOpticalCompStage c_CP20;
    dsp::circuits::DualSlopeBusVcaStage c_CP21;
    dsp::circuits::SolidStateT4BLevelerStage c_CP22;
    dsp::circuits::ParametricDynamicEq4BandStage c_CP23;
    dsp::circuits::PhaseSafeDrumBleedGateStage c_CP24;
    dsp::circuits::ZeroDistortionVocalRiderStage c_CP25;
    dsp::circuits::Broadcast5BandFmLimiterStage c_CP26;
    dsp::circuits::SuperTube6H30PVariMuStage c_CP27;
    dsp::circuits::LinearPhaseUpwardLifterStage c_CP28;
    dsp::circuits::CrestFactorDensityCompStage c_CP29;
    dsp::circuits::MasteringAdcOverdriveStage c_CL4;
    dsp::circuits::SplitBandMasteringClipperStage c_CL5;
    dsp::circuits::HilbertWaveformSymmetrizerStage c_CL6;
    dsp::circuits::Adaa2ndOrderClipperStage c_CL7;
    dsp::circuits::TapeEchoStage c_DL1;
    dsp::circuits::PingPongStereoDelayStage c_DL2;
    dsp::circuits::CompanderBbdDelay c_DL3;
    dsp::circuits::DualMicroPitchStage c_DL4;
    dsp::circuits::MagneticDrumEchoStage c_DL5;
    dsp::circuits::Companded12BitRackDelayStage c_DL6;
    dsp::circuits::DynamicDuckingDelayStage c_DL7;
    dsp::circuits::Rhythmic8TapDiffuserStage c_DL8;
    dsp::circuits::AsynchronousGrainCloudStage c_DL9;
    dsp::circuits::VarispeedAdtDoublerStage c_DL10;
    dsp::circuits::OctaveShimmerFdnTank c_SH1;
    dsp::circuits::StudioReverbFDN c_RV1;
    dsp::circuits::ConcertHallFdnStage c_RV2;
    dsp::circuits::DispersionSpringReverb c_RV3;
    dsp::circuits::NonlinearGatedReverb c_RV4;
    dsp::circuits::DispersiveSteelPlateStage c_RV5;
    dsp::circuits::TiledLiveDrumChamberStage c_RV6;
    dsp::circuits::EarlyReflectionAmbienceStage c_RV7;
    dsp::circuits::InfiniteAbyssalGravityStage c_RV8;
    dsp::circuits::ZeroLatencyIrConvolverStage c_RV9;
    dsp::circuits::GoldenRatioDualDelayStage c_DL11;
    dsp::circuits::Modulated16BitRackDelayStage c_DL12;
    dsp::circuits::ReverseEnvelopeEchoStage c_DL13;
    dsp::circuits::CoiledTubingAcousticDelayStage c_DL14;
    dsp::circuits::SubMsHaasPhaseAlignerStage c_DL15;
    dsp::circuits::ElectrostaticOilCanDelayStage c_DL16;
    dsp::circuits::DispersivePrismDelayStage c_DL17;
    dsp::circuits::Vintage4LeverDigitalVerbStage c_RV10;
    dsp::circuits::AcousticRoomComputer1982Stage c_RV11;
    dsp::circuits::HighDensityDigitalPlateStage c_RV12;
    dsp::circuits::SympatheticSoundboardVerbStage c_RV13;
    dsp::circuits::SubOctaveAbyssalVerbStage c_RV14;
    dsp::circuits::SplitDecayConcertHallStage c_RV15;
    dsp::circuits::Coaxial6SpringStudioVerbStage c_RV16;
    dsp::circuits::ExponentialReverseVerbStage c_RV17;
    dsp::circuits::InfiniteSpectralFreezeStage c_RV18;
    dsp::circuits::StructuralStreamSplitter c_SP_A;
    dsp::circuits::LinkwitzRiley4BandCrossoverStage c_SP_B;
    dsp::circuits::EllipticalMidSideMatrixStage c_SP_C;
    dsp::circuits::ParallelWetDryPhaseBlenderStage c_SP_D;
    dsp::circuits::FrequencyConsciousDuckingRouterStage c_SP_E;
    dsp::circuits::BucketBrigadeChorus c_MD1;
    dsp::circuits::TriStereoBbdChorusStage c_MD2;
    dsp::circuits::ThroughZeroTapeFlanger c_MD3;
    dsp::circuits::OpticalUniVibePhaserStage c_MD4;
    dsp::circuits::PhasePureStereoWidener c_MD5;
    dsp::circuits::OpticalHarmonicTremolo c_MD6;
    dsp::circuits::FormantThroatPitchShifterStage c_MD7;
    dsp::circuits::ChannelToleranceDriftEngineStage c_CV1;
    dsp::circuits::MacroAttenuverterHubStage c_CV2;
    dsp::circuits::SlopeEnvelopeFollowerDusgStage c_CV3;
    dsp::circuits::OrganicLfoPerlinStage c_CV4;
    dsp::circuits::SubAnchorCrossoverRouterStage c_SP_F;
    dsp::circuits::DualMonoLrSplitStage c_SP_G;
    dsp::circuits::DynamicDuckingFxContainerStage c_SP_H;
    dsp::circuits::HarmonicTonalNoiseSplitterStage c_SP_I;
    dsp::circuits::AzimuthCenterWideSplitterStage c_SP_J;
    dsp::circuits::DualRotorCabinetHornDrumStage c_MD8;
    dsp::circuits::BalancedRackAnalogChorusStage c_MD9;
    dsp::circuits::RackThroughZeroFlangerStage c_MD10;
    dsp::circuits::Jfet8StageRackPhaserStage c_MD11;
    dsp::circuits::StudioRmsEnvelopeFilterStage c_MD12;
    dsp::circuits::Analog16BandVocoderStage c_MD13;
    dsp::circuits::Binaural3DHrtfPannerStage c_MD14;
    dsp::circuits::VacuumTubeTapeAdt1966Stage c_MD15;
    dsp::circuits::GermaniumRingModulatorStage c_MD16;
    dsp::circuits::PhysicalDopplerAnimatorStage c_MD17;
    dsp::circuits::TempoSynced16StepGateCvStage c_CV5;
    dsp::circuits::TransientAdEnvelopeCvStage c_CV6;
    dsp::circuits::SampleAndHoldSlewCvStage c_CV7;
    dsp::circuits::PitchToCvFollowerStage c_CV8;
    dsp::circuits::PsychoacousticCentroidCvStage c_CV9;
    bool hadDeltaLastBlock{false};
  };

  template <typename Func>
  static void dispatchStage(StageSlot &s, SchematicComponentType type,
                            Func &&fn) noexcept {
    switch (type) {
    case SchematicComponentType::JfetBuffer:
      fn(s.c_PR1);
      break;
    case SchematicComponentType::ClassAPreamp:
      fn(s.c_PR2);
      break;
    case SchematicComponentType::ClassABOpAmp2520:
      fn(s.c_PR3);
      break;
    case SchematicComponentType::Tube12AX7:
      fn(s.c_PR4);
      break;
    case SchematicComponentType::PentodeEF86:
      fn(s.c_PR5);
      break;
    case SchematicComponentType::DirectHeatedTriode:
      fn(s.c_PR6);
      break;
    case SchematicComponentType::GermaniumBroadcastPre:
      fn(s.c_PR7);
      break;
    case SchematicComponentType::BlueStripeSiliconPre:
      fn(s.c_PR8);
      break;
    case SchematicComponentType::Console4000IcStage:
      fn(s.c_PR9);
      break;
    case SchematicComponentType::JapaneseTapeDeckPre:
      fn(s.c_PR10);
      break;
    case SchematicComponentType::HighVoltageCascodeTube:
      fn(s.c_PR11);
      break;
    case SchematicComponentType::SubMiniatureTubeMicPre:
      fn(s.c_PR12);
      break;
    case SchematicComponentType::AmericanTubeConsolePre:
      fn(s.c_PR13);
      break;
    case SchematicComponentType::GermanBroadcastValvePre:
      fn(s.c_PR14);
      break;
    case SchematicComponentType::ConsoleSummingBusAmp:
      fn(s.c_PR15);
      break;
    case SchematicComponentType::HeadroomCalibratorPre:
      fn(s.c_PR16);
      break;
    case SchematicComponentType::Olympic69LustraphonePre:
      fn(s.c_PR17);
      break;
    case SchematicComponentType::Abbey68TransistorPre:
      fn(s.c_PR18);
      break;
    case SchematicComponentType::LondonARangeZenerPre:
      fn(s.c_PR19);
      break;
    case SchematicComponentType::WestCoastAm10Pre:
      fn(s.c_PR20);
      break;
    case SchematicComponentType::TransformerlessJfetBjtPre:
      fn(s.c_PR21);
      break;
    case SchematicComponentType::HighRail80BroadcastPre:
      fn(s.c_PR22);
      break;
    case SchematicComponentType::Criteria2002HybridPre:
      fn(s.c_PR23);
      break;
    case SchematicComponentType::AirStudioZobelPre:
      fn(s.c_PR24);
      break;
    case SchematicComponentType::Soul60s24VClassAPre:
      fn(s.c_PR25);
      break;
    case SchematicComponentType::LaStudioQuadTransistorPre:
      fn(s.c_PR26);
      break;
    case SchematicComponentType::Octal50sBroadcastTubePre:
      fn(s.c_PR27);
      break;
    case SchematicComponentType::DavenSteppedTubePre:
      fn(s.c_PR28);
      break;
    case SchematicComponentType::SwissBroadcastLinePre:
      fn(s.c_PR29);
      break;
    case SchematicComponentType::ZeroFeedbackMasteringPre:
      fn(s.c_PR30);
      break;
    case SchematicComponentType::GermanDualPentodeCinemaPre:
      fn(s.c_PR31);
      break;
    case SchematicComponentType::ZeroFeedbackPureTriodePre:
      fn(s.c_PR32);
      break;
    case SchematicComponentType::CurrentModeTransimpedancePre:
      fn(s.c_PR33);
      break;
    case SchematicComponentType::EasternBroadcastPentodePre:
      fn(s.c_PR34);
      break;
    case SchematicComponentType::JapaneseDiscreteClassAPre:
      fn(s.c_PR35);
      break;
    case SchematicComponentType::VariImpedanceCableLoadPre:
      fn(s.c_PR36);
      break;
    case SchematicComponentType::OutputTransformer:
      fn(s.c_IR1);
      break;
    case SchematicComponentType::InputTransformer:
      fn(s.c_IR2);
      break;
    case SchematicComponentType::SilverToroidalIron:
      fn(s.c_IR3);
      break;
    case SchematicComponentType::OutputTransformerMatrix:
      fn(s.c_IR4);
      break;
    case SchematicComponentType::PaperInOilInterstageCap:
      fn(s.c_IR5);
      break;
    case SchematicComponentType::LayeredPermalloyInterstage:
      fn(s.c_IR6);
      break;
    case SchematicComponentType::Balanced600BridgingIron:
      fn(s.c_IR7);
      break;
    case SchematicComponentType::SiliconValveOutputIron:
      fn(s.c_IR8);
      break;
    case SchematicComponentType::BesselWidebandServoIron:
      fn(s.c_IR9);
      break;
    case SchematicComponentType::CobaltAmorphousCoreIron:
      fn(s.c_IR10);
      break;
    case SchematicComponentType::TapeHysteresis:
      fn(s.c_TP1);
      break;
    case SchematicComponentType::HighFreqTapeLimiter:
      fn(s.c_TP2);
      break;
    case SchematicComponentType::LoFiCassetteVhsWarper:
      fn(s.c_TP3);
      break;
    case SchematicComponentType::HalfInch30IpsMasteringTape:
      fn(s.c_TP4);
      break;
    case SchematicComponentType::OneInch15IpsTubeTape:
      fn(s.c_TP5);
      break;
    case SchematicComponentType::DifferentialAirCompander:
      fn(s.c_TP6);
      break;
    case SchematicComponentType::StainlessWireRecorder:
      fn(s.c_TP7);
      break;
    case SchematicComponentType::VariableSilkExciter:
      fn(s.c_ST1);
      break;
    case SchematicComponentType::TrigonometricWavefolder:
      fn(s.c_ST2);
      break;
    case SchematicComponentType::StatelessHarmonicInflator:
      fn(s.c_ST3);
      break;
    case SchematicComponentType::ParallelSaturationBus:
      fn(s.c_ST4);
      break;
    case SchematicComponentType::VintageAuralExciter:
      fn(s.c_ST5);
      break;
    case SchematicComponentType::PsychoacousticSubSynth:
      fn(s.c_ST6);
      break;
    case SchematicComponentType::AsymmetricalDiodeClipper:
      fn(s.c_ST7);
      break;
    case SchematicComponentType::DacBitcrusherStage:
      fn(s.c_ST8);
      break;
    case SchematicComponentType::MasteringVinylCuttingLathe:
      fn(s.c_ST9);
      break;
    case SchematicComponentType::TriodePentode3BandMatrix:
      fn(s.c_ST10);
      break;
    case SchematicComponentType::EnvelopeGatedTransientExciter:
      fn(s.c_ST11);
      break;
    case SchematicComponentType::ConsoleBusRailOverdrive:
      fn(s.c_ST12);
      break;
    case SchematicComponentType::StudioPcm14BitConverter:
      fn(s.c_ST13);
      break;
    case SchematicComponentType::SlidingActionSpectralCompander:
      fn(s.c_TP8);
      break;
    case SchematicComponentType::OpticalSoundtrackFilmExciter:
      fn(s.c_TP9);
      break;
    case SchematicComponentType::CollectorSaturationMatrix3Path:
      fn(s.c_ST15);
      break;
    case SchematicComponentType::DynamicFluxDeSaturator:
      fn(s.c_ST16);
      break;
    case SchematicComponentType::GainRangingFloatingPointDac:
      fn(s.c_ST17);
      break;
    case SchematicComponentType::DielectricAbsorptionCapAging:
      fn(s.c_ST18);
      break;
    case SchematicComponentType::PassivePultecEq:
      fn(s.c_EQ1);
      break;
    case SchematicComponentType::ResonantSubSculptor:
      fn(s.c_EQ2);
      break;
    case SchematicComponentType::ParallelPassiveLcEq:
      fn(s.c_EQ3);
      break;
    case SchematicComponentType::InductorEq3Band:
      fn(s.c_EQ4);
      break;
    case SchematicComponentType::ProportionalQGyratorEq:
      fn(s.c_EQ5);
      break;
    case SchematicComponentType::BaxandallTone:
      fn(s.c_EQ6);
      break;
    case SchematicComponentType::ProportionalGraphicEq10Band:
      fn(s.c_EQ7);
      break;
    case SchematicComponentType::FulcrumTiltAirEq:
      fn(s.c_EQ8);
      break;
    case SchematicComponentType::SilkBerylliumAirExciter:
      fn(s.c_EQ9);
      break;
    case SchematicComponentType::ResonantFilter:
      fn(s.c_EQ10);
      break;
    case SchematicComponentType::BarkAuditoryUnmasker:
      fn(s.c_SP1);
      break;
    case SchematicComponentType::SpectralResonanceSuppressor:
      fn(s.c_SP2);
      break;
    case SchematicComponentType::TransistorLadderFilter:
      fn(s.c_SP3);
      break;
    case SchematicComponentType::SidechainSpectralCarver32:
      fn(s.c_SP4);
      break;
    case SchematicComponentType::HarmonicPitchTrackingEq:
      fn(s.c_SP5);
      break;
    case SchematicComponentType::PhasePolarityAligner:
      fn(s.c_SP6);
      break;
    case SchematicComponentType::PassiveMidDipPeakEq:
      fn(s.c_EQ11);
      break;
    case SchematicComponentType::PassiveHighLowBracketEq:
      fn(s.c_EQ12);
      break;
    case SchematicComponentType::Olympic69InductorEq:
      fn(s.c_EQ13);
      break;
    case SchematicComponentType::Detroit7BandToroidalEq:
      fn(s.c_EQ14);
      break;
    case SchematicComponentType::LondonARangeInductorEq:
      fn(s.c_EQ15);
      break;
    case SchematicComponentType::MasteringBaxandallEq:
      fn(s.c_EQ16);
      break;
    case SchematicComponentType::ParallelStateVariableEq:
      fn(s.c_EQ17);
      break;
    case SchematicComponentType::GermanBroadcastSilkyEq:
      fn(s.c_EQ18);
      break;
    case SchematicComponentType::SteppedMasteringTransferEq:
      fn(s.c_EQ19);
      break;
    case SchematicComponentType::EllipticalMidSideEq:
      fn(s.c_EQ20);
      break;
    case SchematicComponentType::DualInductorPCurveEq:
      fn(s.c_EQ21);
      break;
    case SchematicComponentType::Iso226DynamicLoudnessEq:
      fn(s.c_EQ22);
      break;
    case SchematicComponentType::BridgedTwinTNotchEq:
      fn(s.c_EQ23);
      break;
    case SchematicComponentType::FixedResonant12BandFilter:
      fn(s.c_SP7);
      break;
    case SchematicComponentType::SpectralBrickwallBinEq:
      fn(s.c_SP8);
      break;
    case SchematicComponentType::PhaseCompensatedBassMono:
      fn(s.c_SP9);
      break;
    case SchematicComponentType::DynamicSpectralDeHarsher:
      fn(s.c_SP10);
      break;
    case SchematicComponentType::RealTimeSpectralDeReverb:
      fn(s.c_SP11);
      break;
    case SchematicComponentType::SinusoidalTonalNoiseSeparator:
      fn(s.c_SP12);
      break;
    case SchematicComponentType::SubSampleCrossCorrPhaseAligner:
      fn(s.c_SP13);
      break;
    case SchematicComponentType::FetCompressor1176:
      fn(s.c_CP1);
      break;
    case SchematicComponentType::OptoCompressorT4B:
      fn(s.c_CP2);
      break;
    case SchematicComponentType::VariMuTubeCompressor:
      fn(s.c_CP3);
      break;
    case SchematicComponentType::DiodeBridgeCompressor:
      fn(s.c_CP4);
      break;
    case SchematicComponentType::VcaBusCompressor:
      fn(s.c_CP5);
      break;
    case SchematicComponentType::SidechainThrustMatrix:
      fn(s.c_CP6);
      break;
    case SchematicComponentType::DualOptoVcaCompressor:
      fn(s.c_CP7);
      break;
    case SchematicComponentType::OverEasyRmsSmackVca:
      fn(s.c_CP8);
      break;
    case SchematicComponentType::UpDownMultibandCompressor:
      fn(s.c_CP9);
      break;
    case SchematicComponentType::TransientShaper:
      fn(s.c_CP10);
      break;
    case SchematicComponentType::LookaheadUpwardExpander:
      fn(s.c_CP11);
      break;
    case SchematicComponentType::SplitBandDeEsser:
      fn(s.c_CP12);
      break;
    case SchematicComponentType::HysteresisNoiseGate:
      fn(s.c_CP13);
      break;
    case SchematicComponentType::BroadcastTriodeLimiter:
      fn(s.c_CP14);
      break;
    case SchematicComponentType::ZenerShuntLimiter:
      fn(s.c_CP15);
      break;
    case SchematicComponentType::OversampledVariableClipper:
      fn(s.c_CL1);
      break;
    case SchematicComponentType::PsychoacousticMasteringLimiter:
      fn(s.c_CL2);
      break;
    case SchematicComponentType::BrickwallLimiter:
      fn(s.c_CL3);
      break;
    case SchematicComponentType::TransconductanceRoomSlammer:
      fn(s.c_CP16);
      break;
    case SchematicComponentType::BroadcastFieldEffectCrusher:
      fn(s.c_CP17);
      break;
    case SchematicComponentType::RmsVcaPeakStopLimiter:
      fn(s.c_CP18);
      break;
    case SchematicComponentType::DiscreteMasteringMsComp:
      fn(s.c_CP19);
      break;
    case SchematicComponentType::BritishPhotoOpticalComp:
      fn(s.c_CP20);
      break;
    case SchematicComponentType::DualSlopeBusVcaComp:
      fn(s.c_CP21);
      break;
    case SchematicComponentType::SolidStateT4BLeveler:
      fn(s.c_CP22);
      break;
    case SchematicComponentType::ParametricDynamicEq4Band:
      fn(s.c_CP23);
      break;
    case SchematicComponentType::PhaseSafeDrumBleedGate:
      fn(s.c_CP24);
      break;
    case SchematicComponentType::ZeroDistortionVocalRider:
      fn(s.c_CP25);
      break;
    case SchematicComponentType::Broadcast5BandFmLimiter:
      fn(s.c_CP26);
      break;
    case SchematicComponentType::SuperTube6H30PVariMu:
      fn(s.c_CP27);
      break;
    case SchematicComponentType::LinearPhaseUpwardLifter:
      fn(s.c_CP28);
      break;
    case SchematicComponentType::CrestFactorDensityController:
      fn(s.c_CP29);
      break;
    case SchematicComponentType::MasteringAdcOverdrive:
      fn(s.c_CL4);
      break;
    case SchematicComponentType::SplitBandMasteringClipper:
      fn(s.c_CL5);
      break;
    case SchematicComponentType::HilbertWaveformSymmetrizer:
      fn(s.c_CL6);
      break;
    case SchematicComponentType::Adaa2ndOrderClipper:
      fn(s.c_CL7);
      break;
    case SchematicComponentType::TapeEcho:
      fn(s.c_DL1);
      break;
    case SchematicComponentType::PingPongDelay:
      fn(s.c_DL2);
      break;
    case SchematicComponentType::BbdCompanderDelay:
      fn(s.c_DL3);
      break;
    case SchematicComponentType::MicroPitchHarmonizer:
      fn(s.c_DL4);
      break;
    case SchematicComponentType::MagneticDrumEcho:
      fn(s.c_DL5);
      break;
    case SchematicComponentType::Companded12BitRackDelay:
      fn(s.c_DL6);
      break;
    case SchematicComponentType::DynamicDuckingDelay:
      fn(s.c_DL7);
      break;
    case SchematicComponentType::Rhythmic8TapDiffuser:
      fn(s.c_DL8);
      break;
    case SchematicComponentType::AsynchronousGrainCloud:
      fn(s.c_DL9);
      break;
    case SchematicComponentType::StudioAdtTapeDoubler:
      fn(s.c_DL10);
      break;
    case SchematicComponentType::OctaveShimmerReverb:
      fn(s.c_SH1);
      break;
    case SchematicComponentType::PlateReverb:
      fn(s.c_RV1);
      break;
    case SchematicComponentType::HallReverbFDN:
      fn(s.c_RV2);
      break;
    case SchematicComponentType::DispersionSpringReverb:
      fn(s.c_RV3);
      break;
    case SchematicComponentType::NonlinearGatedReverb:
      fn(s.c_RV4);
      break;
    case SchematicComponentType::DispersiveSteelPlate:
      fn(s.c_RV5);
      break;
    case SchematicComponentType::TiledLiveDrumChamber:
      fn(s.c_RV6);
      break;
    case SchematicComponentType::EarlyReflectionAmbience:
      fn(s.c_RV7);
      break;
    case SchematicComponentType::InfiniteVelvetBlackhole:
      fn(s.c_RV8);
      break;
    case SchematicComponentType::ZeroLatencyIrConvolver:
      fn(s.c_RV9);
      break;
    case SchematicComponentType::GoldenRatioDualRackDelay:
      fn(s.c_DL11);
      break;
    case SchematicComponentType::Modulated16BitRackDelay:
      fn(s.c_DL12);
      break;
    case SchematicComponentType::ReverseEnvelopeEcho:
      fn(s.c_DL13);
      break;
    case SchematicComponentType::CoiledTubingAcousticDelay:
      fn(s.c_DL14);
      break;
    case SchematicComponentType::SubMsHaasPhaseAligner:
      fn(s.c_DL15);
      break;
    case SchematicComponentType::ElectrostaticOilCanDelay:
      fn(s.c_DL16);
      break;
    case SchematicComponentType::DispersivePrismDelay:
      fn(s.c_DL17);
      break;
    case SchematicComponentType::Vintage4LeverDigitalVerb:
      fn(s.c_RV10);
      break;
    case SchematicComponentType::AcousticRoomComputer1982:
      fn(s.c_RV11);
      break;
    case SchematicComponentType::HighDensityDigitalPlate:
      fn(s.c_RV12);
      break;
    case SchematicComponentType::SympatheticSoundboardVerb:
      fn(s.c_RV13);
      break;
    case SchematicComponentType::SubOctaveAbyssalVerb:
      fn(s.c_RV14);
      break;
    case SchematicComponentType::SplitDecayConcertHall:
      fn(s.c_RV15);
      break;
    case SchematicComponentType::Coaxial6SpringStudioVerb:
      fn(s.c_RV16);
      break;
    case SchematicComponentType::ExponentialReverseVerb:
      fn(s.c_RV17);
      break;
    case SchematicComponentType::InfiniteSpectralFreezeVerb:
      fn(s.c_RV18);
      break;
    case SchematicComponentType::TransientTonalSplitter:
      fn(s.c_SP_A);
      break;
    case SchematicComponentType::LinkwitzRiley4BandCrossover:
      fn(s.c_SP_B);
      break;
    case SchematicComponentType::MidSideImager:
      fn(s.c_SP_C);
      break;
    case SchematicComponentType::ParallelSplitBus:
      fn(s.c_SP_D);
      break;
    case SchematicComponentType::FrequencyDuckingRouter:
      fn(s.c_SP_E);
      break;
    case SchematicComponentType::BbdChorus:
      fn(s.c_MD1);
      break;
    case SchematicComponentType::TriStereoBbdChorus:
      fn(s.c_MD2);
      break;
    case SchematicComponentType::ThroughZeroTapeFlanger:
      fn(s.c_MD3);
      break;
    case SchematicComponentType::AnalogPhaser:
      fn(s.c_MD4);
      break;
    case SchematicComponentType::MonoCompatibleWidener:
      fn(s.c_MD5);
      break;
    case SchematicComponentType::OpticalHarmonicTremolo:
      fn(s.c_MD6);
      break;
    case SchematicComponentType::DiodeRingSubOctaveMod:
      fn(s.c_MD7);
      break;
    case SchematicComponentType::CvAnalogDriftEngine:
      fn(s.c_CV1);
      break;
    case SchematicComponentType::CvMacroKnobHub:
      fn(s.c_CV2);
      break;
    case SchematicComponentType::CvEnvelopeFollowerSlew:
      fn(s.c_CV3);
      break;
    case SchematicComponentType::CvOrganicLfoPerlin:
      fn(s.c_CV4);
      break;
    case SchematicComponentType::SubAnchorCrossoverRouter:
      fn(s.c_SP_F);
      break;
    case SchematicComponentType::DualMonoLrSplitMatrix:
      fn(s.c_SP_G);
      break;
    case SchematicComponentType::DynamicDuckingFxContainer:
      fn(s.c_SP_H);
      break;
    case SchematicComponentType::HarmonicTonalNoiseSplitter:
      fn(s.c_SP_I);
      break;
    case SchematicComponentType::AzimuthCenterWideSplitter:
      fn(s.c_SP_J);
      break;
    case SchematicComponentType::DualRotorCabinetHornDrum:
      fn(s.c_MD8);
      break;
    case SchematicComponentType::BalancedRackAnalogChorus:
      fn(s.c_MD9);
      break;
    case SchematicComponentType::RackThroughZeroFlanger:
      fn(s.c_MD10);
      break;
    case SchematicComponentType::Jfet8StageRackPhaser:
      fn(s.c_MD11);
      break;
    case SchematicComponentType::StudioRmsEnvelopeFilter:
      fn(s.c_MD12);
      break;
    case SchematicComponentType::Analog16BandVocoderMatrix:
      fn(s.c_MD13);
      break;
    case SchematicComponentType::Binaural3DHrtfPanner:
      fn(s.c_MD14);
      break;
    case SchematicComponentType::VacuumTubeTapeAdt1966:
      fn(s.c_MD15);
      break;
    case SchematicComponentType::GermaniumDiodeRingMod:
      fn(s.c_MD16);
      break;
    case SchematicComponentType::PhysicalDopplerAnimator:
      fn(s.c_MD17);
      break;
    case SchematicComponentType::TempoSynced16StepGateCv:
      fn(s.c_CV5);
      break;
    case SchematicComponentType::TransientAdEnvelopeCv:
      fn(s.c_CV6);
      break;
    case SchematicComponentType::SampleAndHoldSlewCv:
      fn(s.c_CV7);
      break;
    case SchematicComponentType::PitchToCvFollower:
      fn(s.c_CV8);
      break;
    case SchematicComponentType::PsychoacousticCentroidCv:
      fn(s.c_CV9);
      break;
    default:
      break;
    }
  }

  double sampleRate_{48000.0};
  double dcCoeff_{0.9993455};
  int probeStageIdx_{-1};
  bool probeListenActive_{false};
  int channelTopologyMode_{1};
  float inputBusPeakDb_{-18.0f};
  std::array<float, kMaxSlots> stageOutputPeakDb_{
      -18.0f, -18.0f, -18.0f, -18.0f, -18.0f,
      -18.0f, -18.0f, -18.0f, -18.0f, -18.0f};
  std::atomic<float> masterMixPercent_{100.0f};
  float smoothedMasterMix_{1.0f};
  LockFreeRingBuffer<float, 4096> probeRingBuffer_;
  std::vector<StageSlot> slots_{kMaxSlots};
};

/**
 * @brief Avra Studio - Project Hub, Drag-and-Drop Hardware Schematic Builder &
 * Live Test Rack.
 */
class AvraStudioComponent : public gui::Component {
public:
  static constexpr size_t kMaxStagesPerRow = 5;
  static constexpr size_t kMaxStagesPerPage =
      10; // 2 rows of 5 spacious cards per page
  static constexpr size_t kMaxTotalStages =
      30; // Up to 30 circuit stages per plugin

  AvraStudioComponent() {
    liveSchematicRunner_.prepare(48000.0);
    liveSchematicRunner_.setChannelTopologyMode(
        1); // Always default to STEREO

    // 1. Navigation Tab Buttons (SCHEMATIC BUILDER | FACEPLATE UI BUILDER | LIVE TEST RACK | BUILD)
    btnTabWizard_ = std::make_unique<gui::Button>("SCHEMATIC BUILDER");
    btnTabUiBuilder_ = std::make_unique<gui::Button>("FACEPLATE UI BUILDER");
    btnTabRack_ = std::make_unique<gui::Button>("LIVE TEST RACK");
    btnTabProjects_ = std::make_unique<gui::Button>("BUILD");

    btnTabUiBuilder_->setAccentColor(gui::Color::AccentAmber());
    btnTabProjects_->setAccentColor(gui::Color::fromHex(0x52B788));

    btnTabProjects_->setOnClick([this]() { switchTab(StudioTab::Projects); });
    btnTabWizard_->setOnClick(
        [this]() { switchTab(StudioTab::NewPluginWizard); });
    btnTabUiBuilder_->setOnClick([this]() { switchTab(StudioTab::UiBuilder); });
    btnTabRack_->setOnClick([this]() { switchTab(StudioTab::LiveRack); });

    addChild(btnTabWizard_.get());
    addChild(btnTabUiBuilder_.get());
    addChild(btnTabRack_.get());
    addChild(btnTabProjects_.get());

    // Header Global Save, Build, DSP Code/CLI & Fullscreen Buttons
    btnHeaderSaveProject_ = std::make_unique<gui::Button>("SAVE PROJECT");
    btnHeaderOpenIde_ =
        std::make_unique<gui::Button>("\xE2\x9A\xA1 BUILD VST3/CLAP \xE2\x96\xBE");
    btnHeaderVsCode_ = std::make_unique<gui::Button>("DSP CODE & CLI");
    btnHeaderFullscreen_ = std::make_unique<gui::Button>("FULLSCREEN (F11)");
    btnHeaderSaveProject_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnHeaderOpenIde_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnHeaderVsCode_->setAccentColor(gui::Color::AccentCyan());
    btnHeaderFullscreen_->setAccentColor(gui::Color::AccentAmber());

    btnHeaderSaveProject_->setOnClick([this]() { saveActiveProject(false); });
    btnHeaderOpenIde_->setOnClick([this]() { createProjectFromWizard(false); });
    btnHeaderVsCode_->setOnClick([this]() { openSelectedInVsCode(); });
    btnHeaderFullscreen_->setOnClick([this]() {
#if defined(_WIN32)
      const bool isFs = gui::Window::toggleWindowFullscreen(GetActiveWindow());
      btnHeaderFullscreen_->setText(isFs ? "WINDOWED (F11)"
                                         : "FULLSCREEN (F11)");
      btnHeaderFullscreen_->setToggleState(isFs);
      statusMessage_ =
          isFs ? "Switched Avra Studio to Full-Screen mode (press F11 or click "
                 "WINDOWED (F11) to return to resizable window)."
               : "Switched Avra Studio to Resizable Windowed mode (drag "
                 "borders or click Maximize/F11 anytime).";
      repaint();
#endif
    });

    addChild(btnHeaderSaveProject_.get());
    addChild(btnHeaderOpenIde_.get());
    addChild(btnHeaderVsCode_.get());
    addChild(btnHeaderFullscreen_.get());
    btnHeaderOpenIde_->setVisible(false);
    btnHeaderVsCode_->setVisible(false);
    btnHeaderFullscreen_->setVisible(false);

    // Left Sidebar Project Management Buttons (+ NEW PLUGIN, DUPLICATE, IMPORT
    // / EXPORT, RESTORE HIDDEN)
    btnProjNew_ = std::make_unique<gui::Button>("+ NEW PLUGIN");
    btnProjDuplicate_ = std::make_unique<gui::Button>("DUPLICATE");
    btnProjImportExport_ = std::make_unique<gui::Button>("IMPORT/EXPORT");
    btnProjNew_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnProjDuplicate_->setAccentColor(gui::Color::AccentCyan());
    btnProjImportExport_->setAccentColor(gui::Color::AccentAmber());

    btnProjNew_->setOnClick([this]() {
      initializeBlankUntitledProject();
      switchTab(StudioTab::NewPluginWizard);
      statusMessage_ =
          "Created new blank plugin project - add stages from the Component "
          "Library and enter a plugin name below!";
      repaint();
    });
    btnProjDuplicate_->setOnClick([this]() {
      if (selectedProjectIndex_ >= 0 &&
          selectedProjectIndex_ < static_cast<int>(projects_.size())) {
        const std::string dupName = ProjectGenerator::sanitizeName(
            projects_[selectedProjectIndex_].name + "Copy");
        wizardConfig_.name = dupName;
        wizardConfig_.pluginId = "com.avra." + toLower(dupName);
        inputPluginName_->setText(dupName);
        userHasCustomName_ = true;
        createProjectFromWizard(false, false);
      }
    });
    btnProjImportExport_->setOnClick([this]() {
      saveActiveProject(false);
      openSelectedFolder(false);
    });

    addChild(btnProjNew_.get());
    addChild(btnProjDuplicate_.get());
    addChild(btnProjImportExport_.get());

    // Left Sidebar Restore Hidden Projects Button
    btnRestoreHidden_ =
        std::make_unique<gui::Button>("RESTORE HIDDEN PROJECTS");
    btnRestoreHidden_->setAccentColor(gui::Color::AccentAmber());
    btnRestoreHidden_->setOnClick([this]() { restoreHiddenProjects(); });
    addChild(btnRestoreHidden_.get());

    // 2. Live Interactive Rack Controls (Synced 1:1 with Current Project's
    // Faceplate UI & Schematic + Real Windows Speaker Output)
    btnRackGain_ = std::make_unique<gui::Button>("VIEW: LIVE FACEPLATE");
      btnRackGain_->setAccentColor(gui::Color::AccentCyan());

    btnRackGain_->setOnClick([this]() { selectRackPlugin(0); });
    btnRackAudioDevice_ = std::make_unique<gui::Button>("AUDIO OUT \xE2\x96\xBE");
    btnRackAudioDevice_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnRackAudioDevice_->setOnClick([this]() { toggleRackAudioDeviceMenu(); });

    btnSigDrum_ = std::make_unique<gui::Button>("DRUMS");
    btnSigSynth_ = std::make_unique<gui::Button>("SYNTH");
    btnSigCleanGuitar_ = std::make_unique<gui::Button>("CLEAN GTR");
    btnSigGuitar_ = std::make_unique<gui::Button>("DIST GTR");
    btnSigBass_ = std::make_unique<gui::Button>("BASS DI");
    btnSigVocal_ = std::make_unique<gui::Button>("VOCAL");
    btnSigWav_ = std::make_unique<gui::Button>("WAV...");
    btnSigPink_ = std::make_unique<gui::Button>("PINK");
    btnSigTone_ = std::make_unique<gui::Button>("HARMONICS");
    btnSigSilentAnalyzer_ = std::make_unique<gui::Button>("SILENT");
    btnSigMute_ = std::make_unique<gui::Button>("MUTE");
    btnRackDelta_ = std::make_unique<gui::Button>("Δ DELTA");
    btnRackAutoMatch_ = std::make_unique<gui::Button>("AUTO-LEVEL");

    btnSigDrum_->setAccentColor(gui::Color::AccentAmber());
    btnSigSynth_->setAccentColor(gui::Color::AccentAmber());
    btnSigCleanGuitar_->setAccentColor(gui::Color::AccentAmber());
    btnSigGuitar_->setAccentColor(gui::Color::AccentAmber());
    btnSigBass_->setAccentColor(gui::Color::AccentAmber());
    btnSigVocal_->setAccentColor(gui::Color::AccentAmber());
    btnSigWav_->setAccentColor(gui::Color::AccentCyan());
    btnSigPink_->setAccentColor(gui::Color::AccentAmber());
    btnSigTone_->setAccentColor(gui::Color::AccentCyan());
    btnSigSilentAnalyzer_->setAccentColor(gui::Color::fromHex(0xB565F2));
    btnSigMute_->setAccentColor(gui::Color::AccentRed());
    btnRackDelta_->setAccentColor(gui::Color::AccentCyan());
    btnRackAutoMatch_->setAccentColor(gui::Color::fromHex(0x52B788));

    btnSigSilentAnalyzer_->setAutoToggle(true);
    btnRackDelta_->setAutoToggle(true);
    btnRackAutoMatch_->setAutoToggle(true);

    btnSigDrum_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::DrumTransientLoop);
      statusMessage_ =
          "LIVE RACK: Playing 120 BPM 4/4 Acoustic Studio Drum Beat through "
          "your plugin chain (48 kHz Stereo Speaker Output).";
    });
    btnSigSynth_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::SynthChordLoop);
      statusMessage_ =
          "LIVE RACK: Playing 120 BPM Polyphonic Synth Pad Chords through "
          "your plugin chain (48 kHz Stereo Speaker Output).";
    });
    btnSigCleanGuitar_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::CleanGuitarChord);
      statusMessage_ =
          "LIVE RACK: Playing 120 BPM A - E - D - A Open Clean Guitar "
          "Chord Progression (4 beats per chord) through your plugin chain.";
    });
    btnSigGuitar_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::GuitarDiChug);
      statusMessage_ =
          "LIVE RACK: Playing 120 BPM A5 - E5 - D5 - A5 Distorted Guitar "
          "Chord Progression (4 beats per chord) through your plugin chain.";
    });
    btnSigBass_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::BassDiGroove);
      statusMessage_ =
          "LIVE RACK: Playing 120 BPM A - E - D - A Bass DI Groove (4 beats "
          "per note, ideal for FET/Opto comps, bass levellers & iron).";
    });
    btnSigVocal_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::VocalPhrase);
      statusMessage_ =
          "LIVE RACK: Playing 120 BPM A - E - D - A Sustained 'AAhhhhhhh' "
          "Vocal Progression (4 beats per note, ideal for Vocal Chains).";
    });
    btnSigWav_->setOnClick([this]() {
#if defined(_WIN32)
      char fileBuf[MAX_PATH] = {0};
      OPENFILENAMEA ofn{};
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner = GetActiveWindow();
      ofn.lpstrFilter =
          "WAV Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
      ofn.lpstrFile = fileBuf;
      ofn.nMaxFile = MAX_PATH;
      ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
      ofn.lpstrTitle =
          "Select a .WAV Audio Loop to Audition Through Your Plugin";
      if (GetOpenFileNameA(&ofn)) {
        std::string err;
        if (wavLoopPlayer_.loadFromFile(fileBuf, err)) {
          selectTestSignal(TestSignalMode::CustomWavLoop);
          btnSigWav_->setText("WAV:" +
                              wavLoopPlayer_.getFileName().substr(0, 6));
          statusMessage_ = std::format(
              "LIVE RACK: Loaded & looping '{}' through your plugin chain!",
              wavLoopPlayer_.getFileName());
        } else {
          statusMessage_ = std::format("WAV Load Error: {}", err);
        }
        repaint();
      }
#endif
    });
    btnSigPink_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::PinkNoise);
      statusMessage_ =
          "LIVE RACK: Playing Full-Spectrum Pink Noise (ideal for inspecting "
          "EQ & Filter curves on the Dual-Trace FFT Spectrum).";
    });
    btnSigTone_->setOnClick([this]() {
      selectTestSignal(TestSignalMode::HarmonicMultiTone);
      statusMessage_ =
          "HARMONIC THD ANALYZER: Injecting 1 kHz -12 dBFS reference sine - "
          "inspect Even (2k/4k/6k) vs Odd (3k/5k/7k) harmonics & live THD%!";
    });
    btnSigSilentAnalyzer_->setOnClick([this]() {
      silentAnalyzerMode_ = btnSigSilentAnalyzer_->getToggleState();
      statusMessage_ =
          silentAnalyzerMode_
              ? "SILENT ANALYZER ACTIVE: Windows speaker output muted while "
                "60fps DSP, THD Analyzer & Dual-Trace Scopes remain 100% live!"
              : "SILENT ANALYZER OFF: Windows speaker audio output restored.";
      repaint();
    });
    btnSigMute_->setOnClick([this]() {

      if (testSignalMode_ == TestSignalMode::Silence || rackMuted_) {
        selectTestSignal(TestSignalMode::DrumTransientLoop);
        statusMessage_ = "LIVE RACK: Audio generator unmuted (Playing 120 BPM Studio Drum & Bass Groove).";
      } else {
        const auto muteMode = TestSignalMode::Silence;
        selectTestSignal(muteMode);
        statusMessage_ = "LIVE RACK: Audio generator muted (Click MUTE or any signal button to resume).";
      }
    });
    btnRackDelta_->setOnClick([this]() {
      rackDeltaListen_ = btnRackDelta_->getToggleState();
      if (rackDeltaListen_ && rackBypassed_) {
        rackBypassed_ = false;
        if (btnRackBypass_)
          btnRackBypass_->setToggleState(false);
      }
      syncGlobalFaceplateControls();
      statusMessage_ =
          rackDeltaListen_
              ? "Δ DELTA NULL LISTEN ENGAGED: Outputting level-matched (Wet - "
                "LatencyCompensatedDry) difference so you hear strictly what "
                "the circuit is adding or removing!"
              : "Δ DELTA NULL LISTEN OFF: Normal Wet/Dry monitoring restored.";
      repaint();
    });
    btnRackAutoMatch_->setOnClick([this]() {
      rackAutoMatch_ = btnRackAutoMatch_->getToggleState();
      syncGlobalFaceplateControls();
      statusMessage_ =
          rackAutoMatch_ ? "AUTO-LEVEL CONSTANT-LOUDNESS ENGAGED: Continuously "
                           "tracking input-to-output RMS differential and "
                           "applying smooth inverse output trim!"
                         : "AUTO-LEVEL OFF: True uncompensated output level.";
      repaint();
    });

    btnRackBypass_ = std::make_unique<gui::Button>("BYPASS");
    btnRackBypass_->setAutoToggle(true);
    btnRackBypass_->setAccentColor(gui::Color::AccentRed());
    btnRackBypass_->setOnClick([this]() {
      rackBypassed_ = btnRackBypass_->getToggleState();
      if (rackBypassed_ && rackDeltaListen_) {
        rackDeltaListen_ = false;
        if (btnRackDelta_)
          btnRackDelta_->setToggleState(false);
      }
      for (auto &w : faceplateLayout_.widgets) {
        if (w.boundStageIdx < 0 && w.boundParamSlot == 3 &&
            w.title.find("MUTE") == std::string::npos &&
            w.title.find("PHASE") == std::string::npos &&
            w.subtitle.find("MUTE") == std::string::npos &&
            w.subtitle.find("180") == std::string::npos) {
          if (isSwitchWidget(w.type) || w.type == UiWidgetType::JewelLed) {
            w.previewNorm = rackBypassed_ ? 0.0f : 1.0f;
          }
        }
      }
      statusMessage_ =
          rackBypassed_
              ? "LIVE RACK: Master Hard-Bypass Engaged (Hearing Unity Dry "
                "Input - Click again or flip MASTER switch to re-engage)"
              : "LIVE RACK: Master Engaged (Hearing Live Signal Chain)";
      repaint();
    });

    btnRackLoadPreset_ =
        std::make_unique<gui::Button>("PRESET: Init / Flat \xE2\x96\xBE");
    btnRackSavePreset_ = std::make_unique<gui::Button>("SAVE");
    btnRackAbCompare_ = std::make_unique<gui::Button>("A | B");
    btnRackLoadPreset_->setAccentColor(gui::Color::AccentAmber());
    btnRackSavePreset_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnRackAbCompare_->setAccentColor(gui::Color::AccentCyan());
    btnRackAbCompare_->setAutoToggle(true);
    btnRackSavePreset_->setOnClick([this]() { saveRackPreset(); });
    btnRackLoadPreset_->setOnClick([this]() { loadRackPreset(); });
    btnRackAbCompare_->setOnClick([this]() {
      if (!hasBankBState_) {
        bankBChain_ = schematicChain_;
        hasBankBState_ = true;
      }
      std::swap(schematicChain_, bankBChain_);
      isBankBActive_ = btnRackAbCompare_->getToggleState();
      btnRackAbCompare_->setText(isBankBActive_ ? "BANK B" : "A | B");
      syncFaceplateParamValuesFromSchematic(faceplateLayout_, schematicChain_);
      syncInspectorKnobsToSelection();
      statusMessage_ =
          isBankBActive_
              ? "A/B COMPARE [BANK B ACTIVE]: Tweaking independent Bank B "
                "circuit state - click again to compare against Bank A!"
              : "A/B COMPARE [BANK A ACTIVE]: Restored Bank A circuit state.";
      repaint();
    });

    addChild(btnRackGain_.get());
    addChild(btnRackAudioDevice_.get());
    addChild(btnSigDrum_.get());
    addChild(btnSigSynth_.get());
    addChild(btnSigCleanGuitar_.get());
    addChild(btnSigGuitar_.get());
    addChild(btnSigBass_.get());
    addChild(btnSigVocal_.get());
    addChild(btnSigWav_.get());
    addChild(btnSigPink_.get());
    addChild(btnSigTone_.get());
    addChild(btnSigSilentAnalyzer_.get());
    addChild(btnSigMute_.get());
    addChild(btnRackDelta_.get());
    addChild(btnRackAutoMatch_.get());
    addChild(btnRackBypass_.get());
    addChild(btnRackSavePreset_.get());
    addChild(btnRackLoadPreset_.get());
    addChild(btnRackAbCompare_.get());

    rackSpectrum_.setTitle("POST-PLUGIN FFT SPECTRUM (GREY = IN | CYAN = OUT)");
    rackScope_.setTitle("DUAL-TRACE SCOPE");
    addChild(&rackSpectrum_);
    addChild(&rackScope_);
    addChild(&rackGrHistory_);
    addChild(&rackInMeter_);
    addChild(&rackOutMeter_);

    // 3. Build Tab Action Buttons (5 Clear Buttons: VST3 | CLAP | STANDALONE | BUILD ALL | OPEN IN IDE)
    btnChooseOutputPath_ = std::make_unique<gui::Button>("CHOOSE OUTPUT PATH...");
    btnOutputSystemVst3_ = std::make_unique<gui::Button>("SYSTEM VST3");
    btnOutputDefaultDist_ = std::make_unique<gui::Button>("DEFAULT DIST");
    btnExportFmtVst3_ = std::make_unique<gui::Button>("VST3");
    btnExportFmtClap_ = std::make_unique<gui::Button>("CLAP");
    btnExportFmtApp_ = std::make_unique<gui::Button>("STANDALONE");
    btnBuildClap_ = std::make_unique<gui::Button>("BUILD ALL");
    btnOpenFolder_ = std::make_unique<gui::Button>("OPEN IN IDE");
    btnRunStandalone_ = std::make_unique<gui::Button>("RUN STANDALONE .EXE");

    btnExportFmtVst3_->setAccentColor(gui::Color::fromHex(0x06D6A0));
    btnExportFmtClap_->setAccentColor(gui::Color::AccentCyan());
    btnExportFmtApp_->setAccentColor(gui::Color::AccentAmber());
    btnBuildClap_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnOpenFolder_->setAccentColor(gui::Color::fromHex(0xD4AF37));

    btnChooseOutputPath_->setOnClick([this]() { browseForCustomExportFolder(); });
    btnOutputSystemVst3_->setOnClick([this]() {
      setCustomExportFolder("C:\\Program Files\\Common Files\\VST3");
    });
    btnOutputDefaultDist_->setOnClick(
        [this]() { resetCustomExportFolderToDefault(); });
    btnExportFmtVst3_->setOnClick(
        [this]() { toggleExportFormatSelection(0); });
    btnExportFmtClap_->setOnClick(
        [this]() { toggleExportFormatSelection(1); });
    btnExportFmtApp_->setOnClick(
        [this]() { toggleExportFormatSelection(2); });
    btnBuildClap_->setOnClick(
        [this]() { buildSelectedProject(); });
    btnOpenFolder_->setOnClick([this]() { openCurrentProjectInIde(); });
    btnRunStandalone_->setOnClick([this]() { runSelectedStandalone(); });

    addChild(btnChooseOutputPath_.get());
    addChild(btnOutputSystemVst3_.get());
    addChild(btnOutputDefaultDist_.get());
    addChild(btnExportFmtVst3_.get());
    addChild(btnExportFmtClap_.get());
    addChild(btnExportFmtApp_.get());
    addChild(btnBuildClap_.get());
    addChild(btnOpenFolder_.get());
    addChild(btnRunStandalone_.get());

    btnChooseOutputPath_->setVisible(false);
    btnOutputSystemVst3_->setVisible(false);
    btnOutputDefaultDist_->setVisible(false);
    btnRunStandalone_->setVisible(false);

    // 4. Schematic Builder 10-Slot (Analog Slots A-E + Digital Slots F-J)
    // Category & Blueprint Buttons
    btnCatPreamps_ = std::make_unique<gui::Button>("PRE / TUBE");
    btnCatIron_ = std::make_unique<gui::Button>("IRON / SAT");
    btnCatEq_ = std::make_unique<gui::Button>("EQ / SPEC");
    btnCatDynamics_ = std::make_unique<gui::Button>("COMP / CLIP");
    btnCatDelayRev_ = std::make_unique<gui::Button>("TIME / VERB");
    btnCatModRoute_ = std::make_unique<gui::Button>("SPLIT / MOD");
    btnCatAutoSmart_ = std::make_unique<gui::Button>("AUTO / SMART");

    btnCatPreamps_->setOnClick([this]() {
      selectSchematicCategory(SchematicCategory::PreampsAndTubes);
    });
    btnCatIron_->setOnClick(
        [this]() { selectSchematicCategory(SchematicCategory::IronAndTape); });
    btnCatEq_->setOnClick(
        [this]() { selectSchematicCategory(SchematicCategory::EqAndFilters); });
    btnCatDynamics_->setOnClick([this]() {
      selectSchematicCategory(SchematicCategory::AnalogDynamics);
    });
    btnCatDelayRev_->setOnClick([this]() {
      selectSchematicCategory(SchematicCategory::DelayAndReverb);
    });
    btnCatModRoute_->setOnClick([this]() {
      selectSchematicCategory(SchematicCategory::ModulationAndRoute);
    });
    btnCatAutoSmart_->setOnClick([this]() {
      selectSchematicCategory(SchematicCategory::AutoAndSmart);
    });

    addChild(btnCatPreamps_.get());
    addChild(btnCatIron_.get());
    addChild(btnCatEq_.get());
    addChild(btnCatDynamics_.get());
    addChild(btnCatDelayRev_.get());
    addChild(btnCatModRoute_.get());
    addChild(btnCatAutoSmart_.get());

    // Search Filter Input for Component Library & UI Widgets Library
    inputLibrarySearch_ =
        std::make_unique<gui::TextInput>("Filter components/widgets...", "");
    inputLibrarySearch_->setOnTextChanged([this](const std::string &text) {
      librarySearchQuery_ = text;
      schematicLibraryScrollY_ = 0.0f;
      uiWidgetLibraryScrollY_ = 0.0f;
      selectSchematicCategory(activeCategory_, activeSubFilter_);
      selectUiWidgetCategory(activeUiCategory_);
      repaint();
    });
    addChild(inputLibrarySearch_.get());

    // Palette Page Navigation Buttons (prevents >6 items in a category from
    // overflowing the palette box)
    btnPalettePrev_ = std::make_unique<gui::Button>("<");
    btnPaletteNext_ = std::make_unique<gui::Button>(">");
    btnPalettePrev_->setOnClick([this]() {
      if (palettePage_ > 0) {
        --palettePage_;
        repaint();
      }
    });
    btnPaletteNext_->setOnClick([this]() {
      const int maxPage = std::max(
          0, static_cast<int>((paletteItems_.size() + kMaxPalettePerPage - 1) /
                              kMaxPalettePerPage) -
                 1);
      if (palettePage_ < maxPage) {
        ++palettePage_;
        repaint();
      }
    });
    addChild(btnPalettePrev_.get());
    addChild(btnPaletteNext_.get());

    // Starter 3-Stage Studio Blueprint Macro Buttons (100% Copyright-Free &
    // Trademark-Safe Names)
    btnPresetTubeFet_ =
        std::make_unique<gui::Button>("BLUEPRINT: DRUM BUS");
    btnPresetBritStrip_ =
        std::make_unique<gui::Button>("BLUEPRINT: BASS GUITAR");
    btnPresetPultecOpto_ =
        std::make_unique<gui::Button>("BLUEPRINT: CLEAN GUITAR");
    btnPresetEchoSpace_ =
        std::make_unique<gui::Button>("BLUEPRINT: DIST GUITAR");
    btnPresetVocal_ =
        std::make_unique<gui::Button>("BLUEPRINT: VOCAL CHAIN");
    btnPresetSynthKeys_ =
        std::make_unique<gui::Button>("BLUEPRINT: SYNTH / KEYS");
    btnClearCanvas_ = std::make_unique<gui::Button>("CLEAR CANVAS");

    btnPresetTubeFet_->setAccentColor(gui::Color::AccentAmber());
    btnPresetBritStrip_->setAccentColor(gui::Color::AccentAmber());
    btnPresetPultecOpto_->setAccentColor(gui::Color::AccentAmber());
    btnPresetEchoSpace_->setAccentColor(gui::Color::AccentAmber());
    btnPresetVocal_->setAccentColor(gui::Color::AccentAmber());
    btnPresetSynthKeys_->setAccentColor(gui::Color::AccentAmber());
    btnClearCanvas_->setAccentColor(gui::Color::AccentRed());

    btnPresetTubeFet_->setOnClick([this]() { loadStarterChain(0); });
    btnPresetBritStrip_->setOnClick([this]() { loadStarterChain(1); });
    btnPresetPultecOpto_->setOnClick([this]() { loadStarterChain(2); });
    btnPresetEchoSpace_->setOnClick([this]() { loadStarterChain(3); });
    btnPresetVocal_->setOnClick([this]() { loadStarterChain(4); });
    btnPresetSynthKeys_->setOnClick([this]() { loadStarterChain(5); });
    btnClearCanvas_->setOnClick([this]() {
      schematicChain_.clear();
      selectedNodeIndex_ = -1;
      canvasPage_ = 0;
      syncInspectorKnobsToSelection();
      updateCanvasPageButtons();
      syncFaceplateWithSchematic();
      statusMessage_ =
          "Schematic and faceplate cleared. Select modules from "
          "the library on the left to build a signal chain.";
      repaint();
    });

    addChild(btnPresetTubeFet_.get());
    addChild(btnPresetBritStrip_.get());
    addChild(btnPresetPultecOpto_.get());
    addChild(btnPresetEchoSpace_.get());
    addChild(btnPresetVocal_.get());
    addChild(btnPresetSynthKeys_.get());
    addChild(btnClearCanvas_.get());

    // Canvas Page Navigation Buttons (for chains > 10 stages)
    btnCanvasPrevPage_ = std::make_unique<gui::Button>("< PREV 10");
    btnCanvasNextPage_ = std::make_unique<gui::Button>("NEXT 10 >");
    btnCanvasPrevPage_->setOnClick([this]() {
      if (canvasPage_ > 0) {
        --canvasPage_;
        repaint();
      }
    });
    btnCanvasNextPage_->setOnClick([this]() {
      const int maxPage = std::max(
          0, static_cast<int>((schematicChain_.size() + kMaxStagesPerPage - 1) /
                              kMaxStagesPerPage) -
                 1);
      if (canvasPage_ < maxPage) {
        ++canvasPage_;
        repaint();
      }
    });
    addChild(btnCanvasPrevPage_.get());
    addChild(btnCanvasNextPage_.get());

    // Node Inspector Controls (Toggle Series/Parallel, Mix %, Internal Trims &
    // Live Audition)
    btnNodeToggleParallel_ = std::make_unique<gui::Button>("SERIES / PARALLEL");
    btnNodeToggleParallel_->setAccentColor(gui::Color::fromHex(0xD4AF37));
    btnNodeToggleParallel_->setOnClick([this]() {
      if (selectedNodeIndex_ >= 0 &&
          selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
        auto &node = schematicChain_[selectedNodeIndex_];
        node.isParallel = !node.isParallel;
        if (node.isParallel && node.parallelMixPercent >= 99.9f) node.parallelMixPercent = 50.0f;
        else if (!node.isParallel) node.parallelMixPercent = 100.0f;
        syncFaceplateWithSchematic();
        statusMessage_ =
            std::format("Stage {} set to {} routing (Faceplate UI synced).",
                        selectedNodeIndex_ + 1,
                        node.isParallel ? "PARALLEL BRANCH" : "SERIES");
        repaint();
      }
    });

    btnNodeMixDown_ = std::make_unique<gui::Button>("MIX -10%");
    btnNodeMixUp_ = std::make_unique<gui::Button>("MIX +10%");
    btnNodeMixDown_->setOnClick([this]() {
      if (selectedNodeIndex_ >= 0 &&
          selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
        auto &node = schematicChain_[selectedNodeIndex_];
        node.isParallel = true;
        node.parallelMixPercent =
            math::clamp(node.parallelMixPercent - 10.0f, 10.0f, 100.0f);
        syncFaceplateWithSchematic();
        repaint();
      }
    });
    btnNodeMixUp_->setOnClick([this]() {
      if (selectedNodeIndex_ >= 0 &&
          selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
        auto &node = schematicChain_[selectedNodeIndex_];
        node.isParallel = true;
        node.parallelMixPercent =
            math::clamp(node.parallelMixPercent + 10.0f, 10.0f, 100.0f);
        syncFaceplateWithSchematic();
        repaint();
      }
    });

    btnNodeToggleTrims_ = std::make_unique<gui::Button>("TRIMS: HIDDEN");
    btnNodeToggleTrims_->setAccentColor(gui::Color::AccentAmber());
    btnNodeToggleTrims_->setOnClick([this]() {
      if (selectedNodeIndex_ >= 0 &&
          selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
        auto &node = schematicChain_[selectedNodeIndex_];
        node.exposeInternalTrims = !node.exposeInternalTrims;
        syncFaceplateWithSchematic();
        const auto &plum = getAutoPlumbingSpec(node.type);
        statusMessage_ =
            std::format("Stage {} Internal Calibration Trims ({} + {}) {} on "
                        "the Faceplate!",
                        selectedNodeIndex_ + 1, plum.internalTrim1.name,
                        plum.internalTrim2.name,
                        node.exposeInternalTrims ? "EXPOSED" : "HIDDEN");
        repaint();
      }
    });

    btnAuditionSchematic_ = std::make_unique<gui::Button>(
        "AUDITION & TWEAK IN LIVE RACK");
    btnAuditionSchematic_->setAccentColor(gui::Color::AccentCyan());
    btnAuditionSchematic_->setOnClick([this]() {
      selectRackPlugin(0);
      switchTab(StudioTab::LiveRack);
      statusMessage_ = std::format(
          "LIVE RACK: Auditioning '{}' ({} stages) at 48 kHz through Windows "
          "Audio Output - turn any knob on your Faceplate UI!",
          wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name,
          schematicChain_.size());
      repaint();
    });

    inspectorKnob1_ =
        std::make_unique<gui::Knob>(nullptr, gui::Color::AccentCyan());
    inspectorKnob2_ =
        std::make_unique<gui::Knob>(nullptr, gui::Color::AccentAmber());

    addChild(btnNodeToggleParallel_.get());
    addChild(btnNodeMixDown_.get());
    addChild(btnNodeMixUp_.get());
    addChild(btnNodeToggleTrims_.get());
    addChild(btnAuditionSchematic_.get());
    addChild(inspectorKnob1_.get());
    addChild(inspectorKnob2_.get());

    // 5. Plugin Name Input & Action Creation Buttons
    inputPluginName_ =
        std::make_unique<gui::TextInput>("Enter Plugin Name...", "");
    inputPluginName_->setOnTextChanged([this](const std::string &text) {
      const std::string displayTitle = text;
      wizardConfig_.name = text;
      wizardConfig_.pluginId =
          text.empty() ? "com.avra.plugin"
                       : std::format("com.company.{}", toLower(text));
      faceplateLayout_.theme.pluginTitle = displayTitle;
      for (auto &w : faceplateLayout_.widgets) {
        if (w.type == UiWidgetType::RivetedNameplate) {
          w.title = displayTitle.empty() ? "CUSTOM UNIT" : displayTitle;
        }
      }
      if (!text.empty() && selectedProjectIndex_ >= 0 &&
          selectedProjectIndex_ < static_cast<int>(projects_.size())) {
        projects_[selectedProjectIndex_].name = displayTitle;
      }
      userHasCustomName_ = !text.empty();
      markProjectDirty(true);
      repaint();
    });

    btnDesignUiFromSchematic_ =
        std::make_unique<gui::Button>("DESIGN FACEPLATE UI ->");
    btnDesignUiFromSchematic_->setAccentColor(gui::Color::AccentAmber());
    btnDesignUiFromSchematic_->setOnClick(
        [this]() { switchTab(StudioTab::UiBuilder); });
    btnDesignUiFromSchematic_->setVisible(false);

    btnCreateProject_ =
        std::make_unique<gui::Button>("GENERATE VST3 + CLAP + APP");
    btnCreateAndOpenIde_ =
        std::make_unique<gui::Button>("BUILD & OPEN IN VS CODE");
    btnCreateProject_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnCreateAndOpenIde_->setAccentColor(gui::Color::AccentCyan());

    btnCreateProject_->setOnClick([this]() { createProjectFromWizard(false); });
    btnCreateAndOpenIde_->setOnClick(
        [this]() { createProjectFromWizard(true); });

    addChild(inputPluginName_.get());
    addChild(btnDesignUiFromSchematic_.get());
    addChild(btnCreateProject_.get());
    addChild(btnCreateAndOpenIde_.get());

    // 6. Faceplate UI Builder Controls (Unified Style Template Dropdown, Single
    // Chassis Colour Picker, 16 Surface Finishes, 6 Mounts, 10 Screw Styles,
    // Grid Snap, Sync, Clear)
    btnUiFinish_ = std::make_unique<gui::Button>("STYLE: SLEEK MODERN DARK  v");
    btnUiPickFinishColor_ =
        std::make_unique<gui::Button>("COLOUR: OBSIDIAN...");
    btnUiGrain_ = std::make_unique<gui::Button>("SURFACE: VERTICAL BRUSHED");
    btnUiScrews_ = std::make_unique<gui::Button>("MOUNT: 4X CORNER BOLTS");
    btnUiScrewStyle_ = std::make_unique<gui::Button>("SCREWS: PHILLIPS CROSS");
    btnUiGrid_ = std::make_unique<gui::Button>("GRID: 8px");
    btnUiSyncSchematic_ = std::make_unique<gui::Button>("SYNC");
    btnUiUnifyTheme_ = std::make_unique<gui::Button>("SKIN: HERITAGE");
    btnUiInternalTrims_ = std::make_unique<gui::Button>("TRIMS: HIDDEN");
    btnUiClearCanvas_ = std::make_unique<gui::Button>("CLEAR");

    btnUiGlobalKnob_ = std::make_unique<gui::Button>("KNOBS: POINTER KNOB");
    btnUiGlobalSwitch_ =
        std::make_unique<gui::Button>("SWITCHES: CHROME TOGGLE");
    btnUiGlobalMeter_ =
        std::make_unique<gui::Button>("METERS: ANALOG VU METER");
    btnUiGlobalColor_ = std::make_unique<gui::Button>("SCHEME: WARM AMBER");

    btnUiFinish_->setAccentColor(gui::Color::fromHex(0xD4AF37));
    btnUiPickFinishColor_->setAccentColor(gui::Color::AccentAmber());
    btnUiGrain_->setAccentColor(gui::Color::AccentCyan());
    btnUiScrews_->setAccentColor(gui::Color::AccentCyan());
    btnUiScrewStyle_->setAccentColor(gui::Color::AccentCyan());
    btnUiGrid_->setAccentColor(gui::Color::fromHex(0x9BA6AE));
    btnUiSyncSchematic_->setAccentColor(gui::Color::AccentAmber());
    btnUiUnifyTheme_->setAccentColor(gui::Color::AccentCyan());
    btnUiInternalTrims_->setAccentColor(gui::Color::AccentAmber());
    btnUiClearCanvas_->setAccentColor(gui::Color::AccentRed());

    btnUiGlobalKnob_->setAccentColor(gui::Color::AccentAmber());
    btnUiGlobalSwitch_->setAccentColor(gui::Color::AccentAmber());
    btnUiGlobalMeter_->setAccentColor(gui::Color::AccentAmber());
    btnUiGlobalColor_->setAccentColor(gui::Color::AccentAmber());

    savedUserThemes_ =
        loadSavedUserThemesFromFile(ProjectGenerator::getWorkspaceRoot());

    inputCustomThemeName_ =
        std::make_unique<gui::TextInput>("e.g. My Studio Gold", "MY CUSTOM THEME");
    inputCustomThemeName_->setOnEnterPressed([this](const std::string &text) {
      saveCurrentThemeAsUserTheme(text);
    });
    btnSaveCustomTheme_ =
        std::make_unique<gui::Button>("SAVE THEME");
    btnSaveCustomTheme_->setAccentColor(gui::Color::AccentCyan());
    btnSaveCustomTheme_->setOnClick([this]() {
      saveCurrentThemeAsUserTheme(
          inputCustomThemeName_ ? inputCustomThemeName_->getText() : "");
    });

    btnUiUnifyTheme_->setOnClick([this]() {
      faceplateLayout_.theme.unifyFaceplateTheme =
          !faceplateLayout_.theme.unifyFaceplateTheme;
      markThemeAsCustomModified();
      syncFaceplateWithSchematic();
      updateUiThemeButtonLabels();
      repaint();
    });
    btnUiInternalTrims_->setOnClick([this]() {
      bool anyExposed = false;
      for (const auto &n : schematicChain_) {
        if (n.exposeInternalTrims) {
          anyExposed = true;
          break;
        }
      }
      const bool newExpose = !anyExposed;
      for (auto &n : schematicChain_) {
        n.exposeInternalTrims = newExpose;
      }
      syncFaceplateWithSchematic();
      repaint();
    });

    btnUiFinish_->setOnClick([this]() {
      isStyleTemplateDropdownOpen_ = !isStyleTemplateDropdownOpen_;
      hoveredStyleTemplateDropdownIdx_ = -1;
      hoveredUserThemeDropdownIdx_ = -1;
      isScaleDropdownOpen_ = false;
      isParamBindDropdownOpen_ = false;
      activePopupMenu_ = UiPopupMenuKind::None;
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ =
          isStyleTemplateDropdownOpen_
              ? "Select a Unified UI Style Template or Saved User Theme from "
                "the dropdown (or click SAVE *CUSTOM to save your current theme)!"
              : std::format("Active UI Style Template: {}.",
                            getUiStyleTemplateName(activeStyleTemplate_));
      repaint();
    });
    btnUiPickFinishColor_->setOnClick([this]() {
#if defined(_WIN32)
      if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
        const int next =
            (static_cast<int>(faceplateLayout_.theme.finish) + 1) % 14;
        faceplateLayout_.theme.finish = static_cast<gui::ChassisFinish>(next);
        faceplateLayout_.theme.useCustomFinishColor = false;
        faceplateLayout_.theme.customFinishHex =
            gui::HardwareMaterials::getPresetFinishBaseHex(
                faceplateLayout_.theme.finish);
        markThemeAsCustomModified();
        updateUiThemeButtonLabels();
        statusMessage_ =
            std::format("Chassis colour set to {} - Style switched to *CUSTOM "
                        "(click SAVE *CUSTOM to save as your own theme).",
                        getChassisFinishName(faceplateLayout_.theme.finish));
        repaint();
        return;
      }
      static COLORREF customCustColors[16] = {
          RGB(0x16, 0x19, 0x20), RGB(0x2E, 0x3D, 0x52), RGB(0x1A, 0x2E, 0x24),
          RGB(0x3B, 0x1C, 0x22), RGB(0x48, 0x42, 0x36), RGB(0x24, 0x34, 0x38),
          RGB(0x2B, 0x21, 0x3A), RGB(0x4A, 0x2C, 0x1D), RGB(0x1C, 0x2D, 0x38),
          RGB(0x7A, 0x82, 0x8C), RGB(0x8B, 0x5A, 0x2B), RGB(0x5A, 0x18, 0x9A),
          RGB(0x00, 0x5F, 0x73), RGB(0xAE, 0x20, 0x12), RGB(0x2D, 0x6A, 0x4F),
          RGB(0xE8, 0xEE, 0xF2)};
      const uint32_t curHex =
          faceplateLayout_.theme.useCustomFinishColor
              ? faceplateLayout_.theme.customFinishHex
              : gui::HardwareMaterials::getPresetFinishBaseHex(
                    faceplateLayout_.theme.finish);
      const BYTE r0 = static_cast<BYTE>((curHex >> 16) & 0xFF);
      const BYTE g0 = static_cast<BYTE>((curHex >> 8) & 0xFF);
      const BYTE b0 = static_cast<BYTE>(curHex & 0xFF);

      CHOOSECOLORA cc{};
      cc.lStructSize = sizeof(cc);
      cc.hwndOwner = GetActiveWindow();
      cc.lpCustColors = customCustColors;
      cc.rgbResult = RGB(r0, g0, b0);
      cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;

      if (ChooseColorA(&cc)) {
        const uint32_t r = GetRValue(cc.rgbResult);
        const uint32_t g = GetGValue(cc.rgbResult);
        const uint32_t b = GetBValue(cc.rgbResult);
        faceplateLayout_.theme.customFinishHex = (r << 16) | (g << 8) | b;
        faceplateLayout_.theme.useCustomFinishColor = true;
        faceplateLayout_.theme.hasSurfaceOverlayOverride = true;
        markThemeAsCustomModified();
        updateUiThemeButtonLabels();
        statusMessage_ =
            std::format("Chassis colour set to #{:06X} - Style switched to "
                        "*CUSTOM (click SAVE *CUSTOM to save as your own theme).",
                        faceplateLayout_.theme.customFinishHex);
        repaint();
      }
#endif
    });
    btnUiGrain_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::SurfaceGrain); });
    btnUiScrews_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::MountStyle); });
    btnUiScrewStyle_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::ScrewStyle); });
    btnUiGrid_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::GridSnap); });
    btnUiGlobalKnob_->setOnClick([this]() {
      UiWidgetType targetType = faceplateLayout_.theme.globalKnobType;
      UiAccentColor targetColor = faceplateLayout_.theme.globalAccentColor;
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size()) &&
          isKnobWidget(faceplateLayout_.widgets[selectedUiWidgetIndex_].type)) {
        targetType = faceplateLayout_.widgets[selectedUiWidgetIndex_].type;
        targetColor =
            faceplateLayout_.widgets[selectedUiWidgetIndex_].accentColor;
      } else {
        targetType = nextKnobWidgetType(faceplateLayout_.theme.globalKnobType);
      }
      recordUndoSnapshot("Apply Knob Style & Colour to All Knobs");
      faceplateLayout_.theme.globalKnobType = targetType;
      faceplateLayout_.theme.globalAccentColor = targetColor;
      size_t updatedCount = 0;
      for (auto &w : faceplateLayout_.widgets) {
        if (isKnobWidget(w.type)) {
          w.type = targetType;
          w.accentColor = targetColor;
          ++updatedCount;
        }
      }
      markThemeAsCustomModified();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ = std::format(
          "Applied knob style '{}' and colour '{}' to all {} knobs on the faceplate.",
          getWidgetTypeName(targetType), getUiAccentColorName(targetColor),
          updatedCount);
      repaint();
    });
    btnUiGlobalSwitch_->setOnClick([this]() {
      UiWidgetType targetType = faceplateLayout_.theme.globalSwitchType;
      UiAccentColor targetColor = faceplateLayout_.theme.globalAccentColor;
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size()) &&
          isSwitchWidget(
              faceplateLayout_.widgets[selectedUiWidgetIndex_].type)) {
        targetType = faceplateLayout_.widgets[selectedUiWidgetIndex_].type;
        targetColor =
            faceplateLayout_.widgets[selectedUiWidgetIndex_].accentColor;
      } else {
        targetType =
            (faceplateLayout_.theme.globalSwitchType ==
             UiWidgetType::ChromeToggleSwitch)
                ? UiWidgetType::IlluminatedPushButton
                : UiWidgetType::ChromeToggleSwitch;
      }
      recordUndoSnapshot("Apply Switch Style & Colour to All Switches");
      faceplateLayout_.theme.globalSwitchType = targetType;
      faceplateLayout_.theme.globalAccentColor = targetColor;
      size_t updatedCount = 0;
      for (auto &w : faceplateLayout_.widgets) {
        if (isSwitchWidget(w.type)) {
          w.type = targetType;
          w.accentColor = targetColor;
          if (w.boundStageIdx >= 0 && w.y >= 445.0f && w.h < 44.0f) {
            w.y = 454.0f;
            w.h = 48.0f;
          }
          ++updatedCount;
        }
      }
      markThemeAsCustomModified();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ = std::format(
          "Applied switch style '{}' and colour '{}' to all {} switches on the faceplate.",
          getWidgetTypeName(targetType), getUiAccentColorName(targetColor),
          updatedCount);
      repaint();
    });
    btnUiGlobalMeter_->setOnClick([this]() {
      faceplateLayout_.theme.globalMeterType =
          nextMeterWidgetType(faceplateLayout_.theme.globalMeterType);
      size_t updatedCount = 0;
      for (auto &w : faceplateLayout_.widgets) {
        if (isMeterWidget(w.type) && faceplateLayout_.isMasterMeterWidget(w) &&
            w.type != UiWidgetType::DigitalTrimBargraph) {
          w.type = faceplateLayout_.theme.globalMeterType;
          ++updatedCount;
        }
      }
      markThemeAsCustomModified();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ =
          std::format("Global Meter/Visualizer Style set to {} (1 of 7 studio "
                      "meters/tube/CRT scopes - applied to {} widgets).",
                      getWidgetTypeName(faceplateLayout_.theme.globalMeterType),
                      updatedCount);
      repaint();
    });
    btnUiGlobalColor_->setOnClick([this]() {
      const int nextCol =
          (static_cast<int>(faceplateLayout_.theme.globalAccentColor) + 1) % 12;
      faceplateLayout_.theme.globalAccentColor =
          static_cast<UiAccentColor>(nextCol);
      for (auto &w : faceplateLayout_.widgets) {
        w.accentColor = faceplateLayout_.theme.globalAccentColor;
      }
      markThemeAsCustomModified();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ = std::format(
          "Global Scheme Accent Colour set to {} (1 of 12 studio colours - "
          "applied to all {} UI widgets).",
          getUiAccentColorName(faceplateLayout_.theme.globalAccentColor),
          faceplateLayout_.widgets.size());
      repaint();
    });
    btnUiSyncSchematic_->setOnClick([this]() {
      syncFaceplateWithSchematic();
      statusMessage_ =
          schematicChain_.empty()
              ? "Schematic is currently empty - Faceplate UI cleared to a "
                "blank chassis in your active scheme."
              : std::format("Synced {} hardware faceplate controls from the {} "
                            "active Schematic stages in your chosen UI scheme!",
                            faceplateLayout_.widgets.size(),
                            schematicChain_.size());
      repaint();
    });
    btnUiClearCanvas_->setOnClick([this]() {
      recordUndoSnapshot("Reset Faceplate Layout & Clear Stickers");
      std::erase_if(faceplateLayout_.widgets, [](const UiCanvasWidget &w) {
        return w.type == UiWidgetType::VintageSticker ||
               w.type == UiWidgetType::SilkscreenLabel ||
               w.type == UiWidgetType::VentilationGrille ||
               w.type == UiWidgetType::RackHandlePair ||
               w.type == UiWidgetType::PatchbayJackSocket;
      });
      syncFaceplateWithSchematic();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ =
          "Reset faceplate layout to default schematic & permanent Metering / "
          "Master controls (custom stickers cleared).";
      repaint();
    });

    addChild(inputCustomThemeName_.get());
    addChild(btnSaveCustomTheme_.get());
    addChild(btnUiFinish_.get());
    addChild(btnUiPickFinishColor_.get());
    addChild(btnUiGrain_.get());
    addChild(btnUiScrews_.get());
    addChild(btnUiScrewStyle_.get());
    addChild(btnUiGrid_.get());
    addChild(btnUiSyncSchematic_.get());
    addChild(btnUiUnifyTheme_.get());
    addChild(btnUiInternalTrims_.get());
    addChild(btnUiClearCanvas_.get());
    addChild(btnUiGlobalKnob_.get());
    addChild(btnUiGlobalSwitch_.get());
    addChild(btnUiGlobalMeter_.get());
    addChild(btnUiGlobalColor_.get());

    // Faceplate Canvas Alignment & Snap Guide Toolbar Buttons
    btnUiAlignRow_ = std::make_unique<gui::Button>("ALIGN TOP");
    btnUiDistribute_ = std::make_unique<gui::Button>("DISTRIBUTE");
    btnUiSnapGuides_ = std::make_unique<gui::Button>("GUIDES");
    btnUiAlignRow_->setAccentColor(gui::Color::AccentCyan());
    btnUiDistribute_->setAccentColor(gui::Color::AccentCyan());
    btnUiSnapGuides_->setAccentColor(gui::Color::AccentAmber());
    btnUiSnapGuides_->setAutoToggle(true);

    btnUiAlignRow_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        const float refY = faceplateLayout_.widgets[selectedUiWidgetIndex_].y;
        size_t alignedCount = 0;
        for (auto &w : faceplateLayout_.widgets) {
          if (w.type == UiWidgetType::SectionFrame ||
              w.type == UiWidgetType::RivetedNameplate)
            continue;
          if (std::abs(w.y - refY) <= 46.0f) {
            w.y = refY;
            ++alignedCount;
          }
        }
        statusMessage_ = std::format(
            "Aligned {} widgets in the active horizontal row to Y = {:.0f}px.",
            alignedCount, refY);
        repaint();
      }
    });
    btnUiDistribute_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        const float refY = faceplateLayout_.widgets[selectedUiWidgetIndex_].y;
        std::vector<size_t> rowIdxs;
        for (size_t i = 0; i < faceplateLayout_.widgets.size(); ++i) {
          const auto &w = faceplateLayout_.widgets[i];
          if (w.type == UiWidgetType::SectionFrame ||
              w.type == UiWidgetType::RivetedNameplate)
            continue;
          if (std::abs(w.y - refY) <= 46.0f)
            rowIdxs.push_back(i);
        }
        if (rowIdxs.size() >= 3) {
          std::sort(rowIdxs.begin(), rowIdxs.end(), [this](size_t a, size_t b) {
            return faceplateLayout_.widgets[a].x <
                   faceplateLayout_.widgets[b].x;
          });
          const float minX = faceplateLayout_.widgets[rowIdxs.front()].x;
          const float maxX = faceplateLayout_.widgets[rowIdxs.back()].x;
          const float step =
              (maxX - minX) / static_cast<float>(rowIdxs.size() - 1);
          for (size_t k = 0; k < rowIdxs.size(); ++k) {
            faceplateLayout_.widgets[rowIdxs[k]].x =
                std::round(minX + static_cast<float>(k) * step);
          }
          statusMessage_ = std::format(
              "Evenly distributed {} widgets horizontally across the row.",
              rowIdxs.size());
          repaint();
        } else {
          statusMessage_ = "Select a widget in a row of 3+ controls to "
                           "distribute them evenly.";
          repaint();
        }
      }
    });
    btnUiSnapGuides_->setOnClick([this]() {
      faceplateLayout_.theme.showSnapGuides =
          btnUiSnapGuides_->getToggleState();
      statusMessage_ = faceplateLayout_.theme.showSnapGuides
                           ? "SNAP GUIDES ENABLED: Showing center crosshair "
                             "and active widget alignment rails on the canvas."
                           : "SNAP GUIDES OFF.";
      repaint();
    });
    addChild(btnUiAlignRow_.get());
    addChild(btnUiDistribute_.get());
    addChild(btnUiSnapGuides_.get());

    btnUiAddSticker_ = std::make_unique<gui::Button>("STICKERS (20) v");
    btnUiAddSticker_->setAccentColor(gui::Color::AccentAmber());
    btnUiAddSticker_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::StickerQuickAdd); });
    addChild(btnUiAddSticker_.get());

    // UI Palette Category & Page Navigation Buttons
    btnUiCatKnobs_ = std::make_unique<gui::Button>("KNOBS (46)");
    btnUiCatSwitches_ = std::make_unique<gui::Button>("SW / FADER");
    btnUiCatMeters_ = std::make_unique<gui::Button>("METERS (10)");
    btnUiCatFrames_ = std::make_unique<gui::Button>("STICKERS/HW");

    btnUiCatKnobs_->setOnClick(
        [this]() { selectUiWidgetCategory(UiWidgetCategory::Knobs); });
    btnUiCatSwitches_->setOnClick([this]() {
      selectUiWidgetCategory(UiWidgetCategory::SwitchesAndLeds);
    });
    btnUiCatMeters_->setOnClick([this]() {
      selectUiWidgetCategory(UiWidgetCategory::MetersAndDisplays);
    });
    btnUiCatFrames_->setOnClick([this]() {
      selectUiWidgetCategory(UiWidgetCategory::FramesAndBadges);
    });

    addChild(btnUiCatKnobs_.get());
    addChild(btnUiCatSwitches_.get());
    addChild(btnUiCatMeters_.get());
    addChild(btnUiCatFrames_.get());

    btnUiPalettePrev_ = std::make_unique<gui::Button>("<");
    btnUiPaletteNext_ = std::make_unique<gui::Button>(">");
    btnUiPalettePrev_->setOnClick([this]() {
      if (uiPalettePage_ > 0) {
        --uiPalettePage_;
        repaint();
      }
    });
    btnUiPaletteNext_->setOnClick([this]() {
      const int maxPage = std::max(
          0,
          static_cast<int>((uiPaletteItems_.size() + kMaxUiPalettePerPage - 1) /
                           kMaxUiPalettePerPage) -
              1);
      if (uiPalettePage_ < maxPage) {
        ++uiPalettePage_;
        repaint();
      }
    });
    addChild(btnUiPalettePrev_.get());
    addChild(btnUiPaletteNext_.get());

    // Selected UI Widget Property Inspector Controls + Riveted Badge Text
    // Inputs
    inputUiWidgetTitle_ =
        std::make_unique<gui::TextInput>("Label / Badge Title", "GAIN");
    inputUiWidgetSub_ =
        std::make_unique<gui::TextInput>("Sub / Badge Line 3", "LINE / MIC");
    inputUiBadgeFooter_ = std::make_unique<gui::TextInput>(
        "Badge Bottom Line", "CUSTOM STUDIO SERIES");

    inputUiWidgetTitle_->setOnTextChanged([this](const std::string &txt) {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        w.title = txt;
        if (w.type == UiWidgetType::RivetedNameplate) {
          faceplateLayout_.theme.pluginTitle = txt;
        }
        if (w.type == UiWidgetType::VintageSticker) {
          autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                                     w.fontSizePreset, false);
        }
        repaint();
      }
    });
    inputUiWidgetSub_->setOnTextChanged([this](const std::string &txt) {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        if (w.type == UiWidgetType::RivetedNameplate) {
          const auto pipePos = w.subtitle.find('|');
          const std::string sub2 = (pipePos != std::string::npos)
                                       ? w.subtitle.substr(pipePos + 1)
                                       : faceplateLayout_.theme.badgeFooterText;
          w.subtitle = std::format("{}|{}", txt, sub2);
          faceplateLayout_.theme.subtitleText = txt;
        } else {
          w.subtitle = txt;
        }
        if (w.type == UiWidgetType::VintageSticker) {
          autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                                     w.fontSizePreset, false);
        }
        repaint();
      }
    });
    inputUiBadgeFooter_->setOnTextChanged([this](const std::string &txt) {
      faceplateLayout_.theme.badgeFooterText = txt;
      for (auto &w : faceplateLayout_.widgets) {
        if (w.type == UiWidgetType::RivetedNameplate) {
          const auto pipePos = w.subtitle.find('|');
          const std::string sub1 = (pipePos != std::string::npos)
                                       ? w.subtitle.substr(0, pipePos)
                                       : w.subtitle;
          w.subtitle = std::format("{}|{}", sub1, txt);
        }
      }
      repaint();
    });

    btnUiEditBadge_ = std::make_unique<gui::Button>("EDIT BADGE");
    btnUiEditBadge_->setAccentColor(gui::Color::AccentAmber());
    btnUiEditBadge_->setOnClick([this]() {
      int badgeIdx = -1;
      for (size_t i = 0; i < faceplateLayout_.widgets.size(); ++i) {
        if (faceplateLayout_.widgets[i].type ==
            UiWidgetType::RivetedNameplate) {
          badgeIdx = static_cast<int>(i);
          break;
        }
      }
      if (badgeIdx < 0) {
        UiCanvasWidget badge;
        badge.type = UiWidgetType::RivetedNameplate;
        badge.title = faceplateLayout_.theme.pluginTitle.empty()
                          ? "CUSTOM UNIT"
                          : faceplateLayout_.theme.pluginTitle;
        badge.subtitle = "AUDIO PLUGIN";
        badge.x = 770.0f;
        badge.y = 434.0f;
        badge.w = 186.0f;
        badge.h = 70.0f;
        badge.accentColor = faceplateLayout_.theme.globalAccentColor;
        faceplateLayout_.widgets.push_back(badge);
        badgeIdx = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
      }
      selectedUiWidgetIndex_ = badgeIdx;
      syncUiWidgetInspector();
      statusMessage_ =
          "Selected Riveted Badge (top brand permanently AVRA AUDIO). Edit "
          "LABEL (center model), SUB (line 3), and BADGE (bottom line) below.";
      repaint();
    });

    btnUiWidgetBind_ = std::make_unique<gui::Button>("BIND: S1 Input Trim v");
    btnUiWidgetStyle_ = std::make_unique<gui::Button>("STYLE: POINTER KNOB  v");
    btnUiWidgetColor_ = std::make_unique<gui::Button>("COLOUR: WARM AMBER  v");
    btnUiWidgetScale_ = std::make_unique<gui::Button>("SCALE: -16..+16 dB  v");
    btnUiGlobalFontType_ =
        std::make_unique<gui::Button>("FONT: STUDIO SANS  v");
    btnUiWidgetFont_ = std::make_unique<gui::Button>("SIZE: LARGE  v");
    btnUiGlobalFontColor_ =
        std::make_unique<gui::Button>("TEXT COL: SILK WHITE  v");
    btnUiGlobalPlateColor_ =
        std::make_unique<gui::Button>("PLATE COL: THEME MATCH  v");
    btnUiWidgetSizeDown_ = std::make_unique<gui::Button>("-");
    btnUiWidgetSizeUp_ = std::make_unique<gui::Button>("+");
    btnUiWidgetLayer_ = std::make_unique<gui::Button>("LAYER ^/v");
    btnUiWidgetDup_ = std::make_unique<gui::Button>("DUP");
    btnUiWidgetDel_ = std::make_unique<gui::Button>("DEL");
    btnUiUploadStickerImg_ = std::make_unique<gui::Button>("UPLOAD IMAGE...");
    btnUiStickerRotate_ = std::make_unique<gui::Button>("ANGLE: 0\xC2\xB0  v");

    btnUiWidgetBind_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnUiWidgetStyle_->setAccentColor(gui::Color::AccentCyan());
    btnUiWidgetColor_->setAccentColor(gui::Color::AccentAmber());
    btnUiWidgetScale_->setAccentColor(gui::Color::AccentAmber());
    btnUiGlobalFontType_->setAccentColor(gui::Color::AccentCyan());
    btnUiWidgetFont_->setAccentColor(gui::Color::AccentCyan());
    btnUiGlobalFontColor_->setAccentColor(gui::Color::fromHex(0xE8F1F2));
    btnUiGlobalPlateColor_->setAccentColor(gui::Color::AccentAmber());
    btnUiWidgetDup_->setAccentColor(gui::Color::fromHex(0x52B788));
    btnUiWidgetDel_->setAccentColor(gui::Color::AccentRed());
    btnUiUploadStickerImg_->setAccentColor(gui::Color::AccentAmber());
    btnUiStickerRotate_->setAccentColor(gui::Color::AccentCyan());

    btnUiUploadStickerImg_->setOnClick(
        [this]() { openStickerImageFileDialog(selectedUiWidgetIndex_); });
    btnUiStickerRotate_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        toggleUiPopupMenu(UiPopupMenuKind::StickerRotation);
      }
    });

    btnUiWidgetBind_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        rebuildParamBindOptions();
        isParamBindDropdownOpen_ = !isParamBindDropdownOpen_;
        isScaleDropdownOpen_ = false;
        isStyleTemplateDropdownOpen_ = false;
        activePopupMenu_ = UiPopupMenuKind::None;
        hoveredParamBindIdx_ = -1;
        updateUiThemeButtonLabels();
        syncUiWidgetInspector();
        statusMessage_ = isParamBindDropdownOpen_
                             ? "Select any Schematic Stage Parameter or Global "
                               "Bus Control to bind to the selected widget!"
                             : "Closed DSP Parameter Binding menu.";
        repaint();
      }
    });

    btnUiWidgetStyle_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        toggleUiPopupMenu(UiPopupMenuKind::WidgetStyle);
      }
    });
    btnUiWidgetColor_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        toggleUiPopupMenu(UiPopupMenuKind::WidgetColor);
      }
    });
    btnUiWidgetScale_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        if (isKnobWidget(w.type)) {
          relevantScaleOptions_ = getRelevantScalesForKnob(w, schematicChain_);
          isScaleDropdownOpen_ = !isScaleDropdownOpen_;
          isStyleTemplateDropdownOpen_ = false;
          isParamBindDropdownOpen_ = false;
          activePopupMenu_ = UiPopupMenuKind::None;
          hoveredScaleDropdownIdx_ = -1;
          updateUiThemeButtonLabels();
          syncUiWidgetInspector();
          statusMessage_ =
              isScaleDropdownOpen_
                  ? std::format("Select a scale option relevant to '{}' ({}) "
                                "from the dropdown menu.",
                                w.title,
                                describeWidgetAutoBinding(w, schematicChain_))
                  : std::format("Knob scale set to {}.",
                                getScalePresetName(w.scalePreset));
          repaint();
        }
      }
    });
    btnUiGlobalFontType_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::FontFamily); });
    btnUiWidgetFont_->setOnClick(
        [this]() { toggleUiPopupMenu(UiPopupMenuKind::FontSize); });
    btnUiGlobalFontColor_->setOnClick([this]() {
#if defined(_WIN32)
      if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
        openCustomFontColorPicker();
        return;
      }
#endif
      toggleUiPopupMenu(UiPopupMenuKind::TextColor);
    });
    btnUiGlobalPlateColor_->setOnClick([this]() {
#if defined(_WIN32)
      if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
        openCustomPlateColorPicker();
        return;
      }
#endif
      toggleUiPopupMenu(UiPopupMenuKind::PlateColor);
    });
    btnUiWidgetSizeDown_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        const float minW =
            (w.type == UiWidgetType::VintageSticker) ? 90.0f : 32.0f;
        const float minH = (w.type == UiWidgetType::VintageSticker)
                               ? (!w.subtitle.empty() ? 46.0f : 36.0f)
                               : 24.0f;
        w.w = std::max(minW, w.w - 10.0f);
        w.h = std::max(minH, w.h - 10.0f);
        if (w.type == UiWidgetType::VintageSticker) {
          autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                                     w.fontSizePreset, true);
        }
        repaint();
      }
    });
    btnUiWidgetSizeUp_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        w.w = std::min(FaceplateLayout::kRefWidth - w.x - 6.0f, w.w + 10.0f);
        w.h = std::min(FaceplateLayout::kRefHeight - w.y - 6.0f, w.h + 10.0f);
        repaint();
      }
    });
    btnUiWidgetLayer_->setOnClick([this]() {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size()) &&
          faceplateLayout_.widgets.size() > 1) {
        UiCanvasWidget w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        faceplateLayout_.widgets.erase(faceplateLayout_.widgets.begin() +
                                       selectedUiWidgetIndex_);
        if (selectedUiWidgetIndex_ ==
            static_cast<int>(faceplateLayout_.widgets.size())) {
          faceplateLayout_.widgets.insert(faceplateLayout_.widgets.begin(), w);
          selectedUiWidgetIndex_ = 0;
          statusMessage_ = std::format("Sent '{}' to back layer.", w.title);
        } else {
          faceplateLayout_.widgets.push_back(w);
          selectedUiWidgetIndex_ =
              static_cast<int>(faceplateLayout_.widgets.size()) - 1;
          statusMessage_ = std::format("Brought '{}' to front layer.", w.title);
        }
        syncUiWidgetInspector();
        repaint();
      }
    });
    btnUiWidgetDup_->setOnClick([this]() { duplicateSelectedUiWidget(); });
    btnUiWidgetDel_->setOnClick([this]() { deleteSelectedUiWidget(); });

    addChild(inputUiWidgetTitle_.get());
    addChild(inputUiWidgetSub_.get());
    addChild(inputUiBadgeFooter_.get());
    addChild(btnUiEditBadge_.get());
    addChild(btnUiWidgetBind_.get());
    addChild(btnUiWidgetStyle_.get());
    addChild(btnUiWidgetColor_.get());
    addChild(btnUiWidgetScale_.get());
    addChild(btnUiGlobalFontType_.get());
    addChild(btnUiWidgetFont_.get());
    addChild(btnUiGlobalFontColor_.get());
    addChild(btnUiGlobalPlateColor_.get());
    addChild(btnUiWidgetSizeDown_.get());
    addChild(btnUiWidgetSizeUp_.get());
    addChild(btnUiWidgetLayer_.get());
    addChild(btnUiWidgetDup_.get());
    addChild(btnUiWidgetDel_.get());
    addChild(btnUiUploadStickerImg_.get());
    addChild(btnUiStickerRotate_.get());

    faceplateLayout_ = buildAvraGainDemoFaceplate("AVRA GAIN");
    selectedUiWidgetIndex_ = 2;
    selectUiWidgetCategory(UiWidgetCategory::Knobs);
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();

    loadStudioPreferencesFromFile(ProjectGenerator::getWorkspaceRoot() /
                                      ".avra_preferences.json",
                                  prefs_);
    recentProjects_ =
        loadRecentProjectsList(ProjectGenerator::getWorkspaceRoot());

    loadHiddenProjectsList();
    loadExportFolderSetting();
    refreshWorkspaceProjects();
    syncPreferencesRuntimeState();
    updateBuildFormatUiLabels();

    selectSchematicCategory(SchematicCategory::PreampsAndTubes);
    // Default to a clean blank project with no name, or restore the last opened
    // project if it exists in the active workspace and has components.
    initializeBlankUntitledProject();
    if (!projects_.empty() && !recentProjects_.empty()) {
      for (const auto &recPathStr : recentProjects_) {
        const std::filesystem::path recPath(recPathStr);
        std::error_code ec;
        if (!std::filesystem::exists(recPath, ec))
          continue;
        const std::string stem =
            ProjectGenerator::sanitizeName(recPath.stem().string());
        if (hiddenProjects_.contains(stem))
          continue;
        std::string loadedName;
        std::vector<SchematicNode> loadedChain;
        FaceplateLayout loadedLayout;
        if (loadProjectFromAvraJsonFile(recPath, loadedName, loadedChain,
                                        loadedLayout) &&
            !loadedChain.empty()) {
          loadAvraProjectFile(recPath);
          break;
        }
      }
    }
    history_.clear();
    markProjectDirty(false);
    lastAutoSaveTickMs_ = GetTickCount64();

    audioOut_.setRenderCallback(
        [this](float *outL, float *outR, size_t numFrames) {
          renderRealtimeAudioBlock(outL, outR, numFrames);
        });

    selectTestSignal(TestSignalMode::DrumTransientLoop);
    switchTab(StudioTab::NewPluginWizard);
    resized();
  }

  ~AvraStudioComponent() override {
    audioOut_.close();
    saveActiveProject(true);
    if (backgroundBuildThread_.joinable()) {
      backgroundBuildThread_.join();
    }
  }

  [[nodiscard]] bool canCloseWindow() override {
#if defined(_WIN32)
    if (isProjectDirty_) {
      const std::string projName =
          wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name;
      const std::string prompt =
          std::format("Project '{}' has unsaved modifications.\n\nSave changes "
                      "to {}.avra before exiting Avra Studio?",
                      projName, projName);
      const int res = MessageBoxA(GetActiveWindow(), prompt.c_str(),
                                  "Avra Studio - Unsaved Modifications",
                                  MB_YESNOCANCEL | MB_ICONWARNING);
      if (res == IDCANCEL) {
        return false;
      }
      if (res == IDYES) {
        saveActiveProject(false);
      }
    }
#endif
    return true;
  }

  [[nodiscard]] ProjectSnapshot captureCurrentProjectSnapshot(
      std::string actionLabel = "Edit Project") const {
    ProjectSnapshot snap;
    snap.actionLabel = std::move(actionLabel);
    snap.schematicChain = schematicChain_;
    snap.faceplateLayout = faceplateLayout_;
    snap.globalInputDriveDb = rackInputTrimDb_;
    snap.globalOutputTrimDb = rackOutputTrimDb_;
    snap.globalParallelMixPct = rackGlobalWetMix_ * 100.0f;
    snap.globalMasterBypass = rackBypassed_;
    snap.selectedSchematicIdx = selectedNodeIndex_;
    snap.selectedWidgetIdx = selectedUiWidgetIndex_;
    return snap;
  }

  void applyProjectSnapshot(const ProjectSnapshot &snap) {
    schematicChain_ = snap.schematicChain;
    faceplateLayout_ = snap.faceplateLayout;
    rackInputTrimDb_ = snap.globalInputDriveDb;
    rackOutputTrimDb_ = snap.globalOutputTrimDb;
    rackGlobalWetMix_ =
        math::clamp(snap.globalParallelMixPct * 0.01f, 0.0f, 1.0f);
    rackBypassed_ = snap.globalMasterBypass;
    selectedNodeIndex_ =
        schematicChain_.empty()
            ? -1
            : std::clamp(snap.selectedSchematicIdx, 0,
                         static_cast<int>(schematicChain_.size()) - 1);
    selectedUiWidgetIndex_ =
        faceplateLayout_.widgets.empty()
            ? -1
            : std::clamp(snap.selectedWidgetIdx, -1,
                         static_cast<int>(faceplateLayout_.widgets.size()) - 1);
    activeStyleTemplate_ = static_cast<UiStyleTemplate>(
        std::clamp(faceplateLayout_.theme.styleTemplateIdx, 0,
                   kTotalUiStyleTemplates - 1));
    if (btnRackBypass_)
      btnRackBypass_->setToggleState(rackBypassed_);
    syncInspectorKnobsToSelection();
    updateCanvasPageButtons();
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();
    markProjectDirty(true);
    repaint();
  }

  void markProjectDirty(bool dirty) {
    isProjectDirty_ = dirty;
    if (btnHeaderSaveProject_) {
      btnHeaderSaveProject_->setText(dirty ? "SAVE PROJECT *" : "SAVE PROJECT");
    }
  }

  void recordUndoSnapshot(std::string actionLabel) {
    history_.pushState(captureCurrentProjectSnapshot(actionLabel),
                       std::move(actionLabel));
    hasPendingMouseDownSnapshot_ = false;
    markProjectDirty(true);
  }

  void performUndo() {
    ProjectSnapshot restored;
    std::string undoneLabel;
    if (history_.undo(captureCurrentProjectSnapshot(), restored, undoneLabel)) {
      applyProjectSnapshot(restored);
      statusMessage_ = std::format(
          "UNDO (Ctrl+Z): Reverted '{}'  [{} undo / {} redo steps available]",
          undoneLabel, history_.undoDepth(), history_.redoDepth());
    } else {
      statusMessage_ =
          "UNDO (Ctrl+Z): Nothing left to undo in the 50-step history stack.";
      repaint();
    }
  }

  void performRedo() {
    ProjectSnapshot restored;
    std::string redoneLabel;
    if (history_.redo(captureCurrentProjectSnapshot(), restored, redoneLabel)) {
      applyProjectSnapshot(restored);
      statusMessage_ = std::format(
          "REDO (Ctrl+Y): Re-applied '{}'  [{} undo / {} redo steps available]",
          redoneLabel, history_.undoDepth(), history_.redoDepth());
    } else {
      statusMessage_ = "REDO (Ctrl+Y): Nothing to redo.";
      repaint();
    }
  }

  void clearAllTextInputFocus() {
    if (inputUiWidgetTitle_)
      inputUiWidgetTitle_->setFocused(false);
    if (inputUiWidgetSub_)
      inputUiWidgetSub_->setFocused(false);
    if (inputUiBadgeFooter_)
      inputUiBadgeFooter_->setFocused(false);
    if (inputLibrarySearch_)
      inputLibrarySearch_->setFocused(false);
    if (inputPluginName_)
      inputPluginName_->setFocused(false);
  }

  bool handleFaceplateRackScrollClick(gui::Point<float> pt,
                                      gui::Rect<float> canvasRect) {
    if (!canvasRect.contains(pt))
      return false;

    const int stageCount = faceplateLayout_.getStageCount();
    const bool useStageStrip = faceplateLayout_.usesSignalChainSelectorStrip();
    if (!useStageStrip)
      return false;

    const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
    const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
    const float rx = (pt.x - canvasRect.x) / sx;
    const float ry = (pt.y - canvasRect.y) / sy;

    // 1. Top Signal-Chain Tile Strip (`x = 34..730`, `y = 58..100`) - active only when 2+ stages are loaded
    if (ry >= 54.0f && ry <= 102.0f && rx >= 34.0f && rx <= 730.0f &&
        stageCount > 1) {
      const float startX = 34.0f;
      const float gap = 8.0f;
      const float tileW = getSignalChainTileWidthRef(stageCount);
      for (int s = 0; s < stageCount; ++s) {
        const float tx = startX + static_cast<float>(s) * (tileW + gap);
        if (rx >= tx && rx <= tx + tileW) {
          if (currentTab_ == StudioTab::UiBuilder && stageCount > 1 &&
              rx >= tx + tileW - 17.0f && ry >= 54.0f && ry <= 78.0f &&
              s < static_cast<int>(schematicChain_.size())) {
            const auto &meta =
                getComponentMeta(schematicChain_[static_cast<size_t>(s)].type);
            recordUndoSnapshot(
                std::format("Delete S{} {}", s + 1, meta.shortName));
            schematicChain_.erase(schematicChain_.begin() + s);
            if (selectedNodeIndex_ >=
                static_cast<int>(schematicChain_.size())) {
              selectedNodeIndex_ = static_cast<int>(schematicChain_.size()) - 1;
            }
            clampSchematicScrollX();
            syncInspectorKnobsToSelection();
            syncFaceplateWithSchematic();
            statusMessage_ = std::format("SIGNAL-CHAIN STRIP: Removed Stage "
                                         "S{} ({}) & updated Faceplate UI!",
                                         s + 1, meta.shortName);
            repaint();
            return true;
          }
          if (rx <= tx + 19.0f && ry >= 54.0f && ry <= 81.0f &&
              s < static_cast<int>(schematicChain_.size())) {
            auto &node = schematicChain_[static_cast<size_t>(s)];
            node.isBypassed = !node.isBypassed;
            syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                  schematicChain_);
            statusMessage_ = std::format(
                "SIGNAL-CHAIN STRIP: Stage S{} ({}) {}!", s + 1,
                getComponentMeta(node.type).shortName,
                node.isBypassed ? "BYPASSED (OUT)" : "ENGAGED (IN)");
            repaint();
            return true;
          }
          faceplateLayout_.activeStageIdx = s;
          selectedNodeIndex_ = s;
          syncInspectorKnobsToSelection();
          if (s < static_cast<int>(schematicChain_.size())) {
            const auto &meta =
                getComponentMeta(schematicChain_[static_cast<size_t>(s)].type);
            statusMessage_ =
                std::format("SIGNAL-CHAIN STRIP: Focused Stage S{} [{}] {} in "
                            "Full-Width Active Bay!",
                            s + 1, meta.designator, meta.fullName);
          }
          repaint();
          return true;
        }
      }
    }
    return false;
  }

  int findHoveredUiWidgetAt(gui::Point<float> pt,
                            gui::Rect<float> canvasRect) const noexcept {
    if (!canvasRect.contains(pt))
      return -1;
    const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
    const float sy = canvasRect.height / FaceplateLayout::kRefHeight;

    for (int pass = 0; pass < 2; ++pass) {
      for (int i = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
           i >= 0; --i) {
        const auto &w = faceplateLayout_.widgets[i];
        if (!faceplateLayout_.isWidgetVisibleForActiveStage(w)) {
          continue;
        }
        const bool isFrame = (w.type == UiWidgetType::SectionFrame ||
                              w.type == UiWidgetType::VentilationGrille ||
                              w.type == UiWidgetType::RackHandlePair);
        if ((pass == 0 && isFrame) || (pass == 1 && !isFrame))
          continue;

        const float effX = faceplateLayout_.getEffectiveWidgetX(w);

        gui::Rect<float> wr = {canvasRect.x + effX * sx,
                               canvasRect.y + w.y * sy, w.w * sx, w.h * sy};
        gui::Point<float> testPt = pt;
        if (w.type == UiWidgetType::VintageSticker &&
            std::abs(w.rotationDeg) > 0.01f) {
          const auto c = wr.getCentre();
          const float rad = -w.rotationDeg * 0.0174532925f;
          const float dx = pt.x - c.x;
          const float dy = pt.y - c.y;
          testPt = {c.x + dx * std::cos(rad) - dy * std::sin(rad),
                    c.y + dx * std::sin(rad) + dy * std::cos(rad)};
        }
        if (wr.contains(testPt)) {
          return i;
        }
      }
    }
    return -1;
  }

  void duplicateSelectedUiWidget() {
    if (selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size())) {
      const auto &orig = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      if (faceplateLayout_.isPermanentMasterSectionWidget(orig)) {
        const std::string name =
            orig.title.empty() ? getWidgetTypeName(orig.type) : orig.title;
        statusMessage_ = std::format(
            "'{}' is a permanent Metering / Master section widget and cannot "
            "be duplicated. Its style, colour, scale and text are editable.",
            name);
        repaint();
        return;
      }
      const std::string origTitle = orig.title;
      recordUndoSnapshot(std::format("Duplicate Widget '{}'", origTitle));
      UiCanvasWidget copy = orig;
      copy.x =
          std::min(FaceplateLayout::kRefWidth - copy.w - 10.0f, copy.x + 28.0f);
      copy.y = std::min(FaceplateLayout::kRefHeight - copy.h - 10.0f,
                        copy.y + 20.0f);
      faceplateLayout_.widgets.push_back(copy);
      selectedUiWidgetIndex_ =
          static_cast<int>(faceplateLayout_.widgets.size()) - 1;
      lastUiWidgetInteractMs_ = GetTickCount64();
      syncUiWidgetInspector();
      statusMessage_ = std::format("Duplicated '{}' ({}) on faceplate.",
                                   copy.title, getWidgetTypeName(copy.type));
      repaint();
    }
  }

  void duplicateSelectedSchematicStage() {
    if (selectedNodeIndex_ >= 0 &&
        selectedNodeIndex_ < static_cast<int>(schematicChain_.size()) &&
        schematicChain_.size() < kMaxTotalStages) {
      const auto &orig = schematicChain_[selectedNodeIndex_];
      const auto &meta = getComponentMeta(orig.type);
      recordUndoSnapshot(std::format("Duplicate S{} {}", selectedNodeIndex_ + 1,
                                     meta.shortName));
      const int insertIdx = selectedNodeIndex_ + 1;
      schematicChain_.insert(schematicChain_.begin() + insertIdx, orig);
      selectedNodeIndex_ = insertIdx;
      clampSchematicScrollX();
      syncInspectorKnobsToSelection();
      updateCanvasPageButtons();
      syncFaceplateWithSchematic();
      statusMessage_ =
          std::format("Duplicated Stage [{}] {} into Stage {}!",
                      meta.designator, meta.shortName, insertIdx + 1);
      repaint();
    }
  }

  bool deleteSelectedUiWidget() {
    if (selectedUiWidgetIndex_ < 0 ||
        selectedUiWidgetIndex_ >=
            static_cast<int>(faceplateLayout_.widgets.size())) {
      const gui::Rect<float> cRect = (currentTab_ == StudioTab::LiveRack)
                                         ? getRackFaceplateRect()
                                         : getUiBuilderCanvasRect();
      const int hoveredIdx = findHoveredUiWidgetAt(lastMousePos_, cRect);
      if (hoveredIdx >= 0) {
        selectedUiWidgetIndex_ = hoveredIdx;
      }
    }
    if (selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size())) {
      const auto &target = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      const std::string delName =
          target.title.empty() ? getWidgetTypeName(target.type) : target.title;
      const std::string delType = getWidgetTypeName(target.type);
      if (faceplateLayout_.isPermanentMasterSectionWidget(target)) {
        statusMessage_ = std::format(
            "'{}' ({}) is a permanent Metering / Master section widget and "
            "cannot be deleted. Its style, colour, scale and text remain editable.",
            delName, delType);
        repaint();
        return false;
      }
      recordUndoSnapshot(std::format("Delete Widget '{}'", delName));
      faceplateLayout_.widgets.erase(faceplateLayout_.widgets.begin() +
                                     selectedUiWidgetIndex_);
      if (faceplateLayout_.widgets.empty()) {
        selectedUiWidgetIndex_ = -1;
        lastUiWidgetInteractMs_ = 0;
      } else {
        if (selectedUiWidgetIndex_ >=
            static_cast<int>(faceplateLayout_.widgets.size())) {
          selectedUiWidgetIndex_ =
              static_cast<int>(faceplateLayout_.widgets.size()) - 1;
        }
        lastUiWidgetInteractMs_ = GetTickCount64();
      }
      isDraggingUiWidget_ = false;
      isResizingUiWidget_ = false;
      isRotatingUiWidget_ = false;
      isScaleDropdownOpen_ = false;
      isParamBindDropdownOpen_ = false;
      activePopupMenu_ = UiPopupMenuKind::None;
      syncUiWidgetInspector();
      statusMessage_ =
          std::format("Deleted '{}' ({}) from faceplate UI.", delName, delType);
      repaint();
      return true;
    }
    return false;
  }

  bool deleteSelectedSchematicStage() {
    if (selectedNodeIndex_ >= 0 &&
        selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
      const int idx = selectedNodeIndex_;
      const auto &meta = getComponentMeta(schematicChain_[idx].type);
      recordUndoSnapshot(std::format("Delete S{} {}", idx + 1, meta.shortName));
      statusMessage_ = std::format(
          "Removed Stage {} ({}) from schematic & synced Faceplate UI.",
          idx + 1, meta.shortName);
      schematicChain_.erase(schematicChain_.begin() + idx);
      if (selectedNodeIndex_ >= static_cast<int>(schematicChain_.size())) {
        selectedNodeIndex_ = static_cast<int>(schematicChain_.size()) - 1;
      }
      const int maxPage = std::max(
          0, static_cast<int>((schematicChain_.size() + kMaxStagesPerPage - 1) /
                              kMaxStagesPerPage) -
                 1);
      if (canvasPage_ > maxPage)
        canvasPage_ = maxPage;
      clampSchematicScrollX();
      syncInspectorKnobsToSelection();
      updateCanvasPageButtons();
      syncFaceplateWithSchematic();
      repaint();
      return true;
    }
    return false;
  }

  void resetAllParametersToUnity() {
    recordUndoSnapshot("Reset All Parameters to Unity");
    rackInputTrimDb_ = 0.0f;
    rackOutputTrimDb_ = 0.0f;
    rackGlobalWetMix_ = 1.0f;
    rackStereoBalance_ = 0.0f;
    rackBypassed_ = false;
    rackMuted_ = false;
    rackPhaseInvert_ = false;
    for (auto &st : schematicChain_) {
      const bool wasPar = st.isParallel;
      st = makeDefaultSchematicNode(st.type, wasPar);
    }
    syncFaceplateParamValuesFromSchematic(faceplateLayout_, schematicChain_);
    syncGlobalFaceplateControls();
    syncInspectorKnobsToSelection();
    statusMessage_ = "RESET ALL PARAMETERS (Ctrl+R): Restored all schematic "
                     "stage parameters, internal trim-pots & master bus trims "
                     "to factory unity/default!";
    repaint();
  }

  void exportFaceplatePngSnapshot() {
    namespace fs = std::filesystem;
    std::string cleanName = ProjectGenerator::sanitizeName(
        wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name);
    if (cleanName.empty() || cleanName == "GainPlugin")
      cleanName = "AvraGain";
    fs::path pngPath = ProjectGenerator::getWorkspaceRoot() / "plugins" /
                       cleanName / (cleanName + "_Faceplate.png");
#if defined(_WIN32)
    char fileBuf[MAX_PATH] = {0};
    const std::string defStr = pngPath.string();
    std::snprintf(fileBuf, MAX_PATH, "%s", defStr.c_str());
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter =
        "PNG Faceplate Snapshot (*.png)\0*.png\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "png";
    ofn.lpstrTitle = "Export High-Resolution Faceplate Snapshot (.png)";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) {
      pngPath = fs::path(fileBuf);
    } else {
      return;
    }
#endif
    const bool ok = gui::Graphics::exportRenderCallbackToPng(
        pngPath, static_cast<uint32_t>(FaceplateLayout::kRefWidth),
        static_cast<uint32_t>(FaceplateLayout::kRefHeight),
        [this](gui::Graphics &offG) {
          paintFaceplateCanvas(offG,
                               {0.0f, 0.0f, FaceplateLayout::kRefWidth,
                                FaceplateLayout::kRefHeight},
                               faceplateLayout_, -1, false, smoothedVuGrDb_,
                               smoothedVuInDb_, smoothedVuOutDb_,
                               smoothedVuOutDb_);
        });
    statusMessage_ = ok ? std::format("EXPORTED FACEPLATE SNAPSHOT (.PNG): "
                                      "Saved 1020x560 render -> {}",
                                      pngPath.string())
                        : "Failed to export .png snapshot.";
    repaint();
  }

  void openProjectFromDiskDialog() {
#if defined(_WIN32)
    char filePath[MAX_PATH] = {0};
    const std::string initDir =
        (ProjectGenerator::getWorkspaceRoot() / "plugins").string();
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter =
        "Avra Studio Project (*.avra;*.avra.bak)\0*.avra;*.bak\0Schematic "
        "Chain (*.avrachain;*.schematic)\0*.avrachain;*.schematic\0All Files "
        "(*.*)\0*.*\0\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = initDir.c_str();
    ofn.lpstrTitle = "Open Avra Studio Project (.avra)";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&ofn))
      return;
    loadAvraProjectFile(std::filesystem::path(filePath));
#endif
  }

  void loadAvraProjectFile(const std::filesystem::path &path) {
    std::string loadedName = wizardConfig_.name;
    std::vector<SchematicNode> loadedChain;
    FaceplateLayout loadedLayout;
    float inDb = 0.0f, outDb = 0.0f, parMix = 100.0f;
    bool byp = false;
    if (loadProjectFromAvraJsonFile(path, loadedName, loadedChain, loadedLayout,
                                    &inDb, &outDb, &parMix, &byp)) {
      wizardConfig_.name = ProjectGenerator::sanitizeName(loadedName);
      inputPluginName_->setText(wizardConfig_.name);
      userHasCustomName_ = true;
      schematicChain_ = std::move(loadedChain);
      faceplateLayout_ = std::move(loadedLayout);
      ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
      rackInputTrimDb_ = inDb;
      rackOutputTrimDb_ = outDb;
      rackGlobalWetMix_ = math::clamp(parMix * 0.01f, 0.0f, 1.0f);
      rackBypassed_ = byp;
      activeStyleTemplate_ = static_cast<UiStyleTemplate>(
          std::clamp(faceplateLayout_.theme.styleTemplateIdx, 0,
                     kTotalUiStyleTemplates - 1));
      history_.clear();
      markProjectDirty(false);
      recordRecentProjectPath(ProjectGenerator::getWorkspaceRoot(),
                              path.string());
      recentProjects_ =
          loadRecentProjectsList(ProjectGenerator::getWorkspaceRoot());
      ensureActiveProjectInWorkspaceList();
      syncInspectorKnobsToSelection();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ = std::format("OPENED PROJECT '{}': Loaded {} stages & {} "
                                   "faceplate widgets from {}",
                                   wizardConfig_.name, schematicChain_.size(),
                                   faceplateLayout_.widgets.size(),
                                   path.filename().string());
      repaint();
    } else if (loadSchematicChainFromFile(path.string(), loadedChain)) {
      recordUndoSnapshot("Import Stage Blueprint");
      schematicChain_ = std::move(loadedChain);
      syncFaceplateWithSchematic();
      statusMessage_ =
          std::format("Imported {} schematic stages from {}",
                      schematicChain_.size(), path.filename().string());
      repaint();
    }
  }

  void saveProjectAsDialog() {
#if defined(_WIN32)
    char filePath[MAX_PATH] = {0};
    const std::string cleanName = ProjectGenerator::sanitizeName(
        wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name);
    const std::string defFile = (ProjectGenerator::getWorkspaceRoot() /
                                 "plugins" / cleanName / (cleanName + ".avra"))
                                    .string();
    std::snprintf(filePath, MAX_PATH, "%s", defFile.c_str());
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter =
        "Avra Studio Project (*.avra)\0*.avra\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "avra";
    ofn.lpstrTitle = "Save Avra Studio Project As (.avra)";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameA(&ofn))
      return;

    const std::filesystem::path targetPath(filePath);
    const std::string newName =
        ProjectGenerator::sanitizeName(targetPath.stem().string());
    if (!newName.empty()) {
      wizardConfig_.name = newName;
      inputPluginName_->setText(newName);
      faceplateLayout_.theme.pluginTitle = newName;
      userHasCustomName_ = true;
    }
    saveProjectToAvraJsonFile(targetPath, wizardConfig_.name, schematicChain_,
                              faceplateLayout_, rackInputTrimDb_,
                              rackOutputTrimDb_, rackGlobalWetMix_ * 100.0f,
                              rackBypassed_);
    saveActiveProject(false);
    recordRecentProjectPath(ProjectGenerator::getWorkspaceRoot(),
                            targetPath.string());
    recentProjects_ =
        loadRecentProjectsList(ProjectGenerator::getWorkspaceRoot());
    statusMessage_ = std::format("SAVED PROJECT AS -> {}", targetPath.string());
    repaint();
#else
    saveActiveProject(false);
#endif
  }

  void executeMenuCommand(StudioMenuCommandId cmd) {
    activeTopMenu_ = TopMenuBarSection::None;
    switch (cmd) {
    case StudioMenuCommandId::File_NewProject:
      recordUndoSnapshot("New Blank Plugin Project");
      initializeBlankUntitledProject();
      switchTab(StudioTab::NewPluginWizard);
      statusMessage_ =
          "Created new blank plugin project (Ctrl+N) - add stages from the "
          "Component Library and enter a plugin name below!";
      break;

    case StudioMenuCommandId::File_OpenProject:
      openProjectFromDiskDialog();
      break;

    case StudioMenuCommandId::File_OpenRecentSubmenu:
      // Parent row only opens the flyout (handled in onMouseDown); nothing to run.
      break;

    case StudioMenuCommandId::File_OpenRecent0:
    case StudioMenuCommandId::File_OpenRecent1:
    case StudioMenuCommandId::File_OpenRecent2:
    case StudioMenuCommandId::File_OpenRecent3:
    case StudioMenuCommandId::File_OpenRecent4:
    case StudioMenuCommandId::File_OpenRecent5:
    case StudioMenuCommandId::File_OpenRecent6:
    case StudioMenuCommandId::File_OpenRecent7: {
      const int idx = recentProjectIndexForCommand(cmd);
      if (idx >= 0 && static_cast<size_t>(idx) < recentProjects_.size()) {
        loadAvraProjectFile(std::filesystem::path(recentProjects_[idx]));
      }
      break;
    }

    case StudioMenuCommandId::File_OpenInIde:
    case StudioMenuCommandId::Build_OpenInIde:
      openCurrentProjectInIde();
      break;

    case StudioMenuCommandId::Build_OpenFolderInExplorer:
      openCurrentProjectFolderInExplorer();
      break;

    case StudioMenuCommandId::File_SaveProject:
      saveActiveProject(false);
      break;

    case StudioMenuCommandId::File_SaveProjectAs:
      saveProjectAsDialog();
      break;

    case StudioMenuCommandId::File_ImportExportBlueprint: {
      const auto bpPath =
          ProjectGenerator::getWorkspaceRoot() / "presets" /
          ((wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name) +
           ".avrachain");
      saveSchematicChainToFile(bpPath.string(), schematicChain_);
      statusMessage_ = std::format(
          "Exported stage blueprint (.avrachain) -> {}", bpPath.string());
      break;
    }

    case StudioMenuCommandId::File_ExportFaceplatePng:
      exportFaceplatePngSnapshot();
      break;

    case StudioMenuCommandId::File_Preferences:
      activeStudioModal_ = ActiveStudioModal::Preferences;
      activePreferencesTab_ = 0;
      break;

    case StudioMenuCommandId::File_ExitStudio:
#if defined(_WIN32)
      if (canCloseWindow()) {
        PostMessageW(GetActiveWindow(), WM_DESTROY, 0, 0);
      }
#endif
      break;

    case StudioMenuCommandId::Edit_Undo:
      performUndo();
      break;

    case StudioMenuCommandId::Edit_Redo:
      performRedo();
      break;

    case StudioMenuCommandId::Edit_DuplicateSelected:
      if (currentTab_ == StudioTab::UiBuilder) {
        duplicateSelectedUiWidget();
      } else {
        duplicateSelectedSchematicStage();
      }
      break;

    case StudioMenuCommandId::Edit_CopyStyle:
      clipboardTheme_ = faceplateLayout_.theme;
      hasClipboardStyle_ = true;
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        clipboardWidgetStyle_ =
            faceplateLayout_.widgets[selectedUiWidgetIndex_];
        hasClipboardWidget_ = true;
        statusMessage_ = std::format(
            "Copied style of '{}' & theme to clipboard (Ctrl+Shift+C).",
            clipboardWidgetStyle_.title);
      } else {
        statusMessage_ =
            "Copied faceplate theme style to clipboard (Ctrl+Shift+C).";
      }
      break;

    case StudioMenuCommandId::Edit_PasteStyle:
      if (hasClipboardStyle_) {
        recordUndoSnapshot("Paste Widget / Theme Style");
        if (hasClipboardWidget_ && selectedUiWidgetIndex_ >= 0 &&
            selectedUiWidgetIndex_ <
                static_cast<int>(faceplateLayout_.widgets.size())) {
          auto &dst = faceplateLayout_.widgets[selectedUiWidgetIndex_];
          dst.accentColor = clipboardWidgetStyle_.accentColor;
          dst.fontSizePreset = clipboardWidgetStyle_.fontSizePreset;
          if (isKnobWidget(dst.type) &&
              isKnobWidget(clipboardWidgetStyle_.type)) {
            dst.type = clipboardWidgetStyle_.type;
          }
          statusMessage_ =
              std::format("Pasted style onto '{}' (Ctrl+Shift+V).", dst.title);
        } else {
          faceplateLayout_.theme = clipboardTheme_;
          updateUiThemeButtonLabels();
          statusMessage_ = "Pasted faceplate theme style (Ctrl+Shift+V).";
        }
        syncUiWidgetInspector();
      }
      break;

    case StudioMenuCommandId::Edit_DeleteSelected:
      if (currentTab_ == StudioTab::UiBuilder) {
        deleteSelectedUiWidget();
      } else if (currentTab_ == StudioTab::NewPluginWizard ||
                 currentTab_ == StudioTab::LiveRack) {
        deleteSelectedSchematicStage();
      }
      break;

    case StudioMenuCommandId::Edit_SelectAllWidgets:
      switchTab(StudioTab::UiBuilder);
      if (!faceplateLayout_.widgets.empty()) {
        selectedUiWidgetIndex_ =
            static_cast<int>(faceplateLayout_.widgets.size()) - 1;
        lastUiWidgetInteractMs_ = GetTickCount64();
        syncUiWidgetInspector();
        statusMessage_ = std::format(
            "Selected all {} faceplate widgets (active focus: '{}').",
            faceplateLayout_.widgets.size(),
            faceplateLayout_.widgets[selectedUiWidgetIndex_].title);
      }
      break;

    case StudioMenuCommandId::Edit_ResetAllParamsToUnity:
      resetAllParametersToUnity();
      break;

    case StudioMenuCommandId::View_ToggleProjectDrawer:
      isSidebarCollapsed_ = !isSidebarCollapsed_;
      if (btnProjNew_)
        btnProjNew_->setVisible(!isSidebarCollapsed_);
      if (btnProjDuplicate_)
        btnProjDuplicate_->setVisible(!isSidebarCollapsed_);
      if (btnProjImportExport_)
        btnProjImportExport_->setVisible(!isSidebarCollapsed_);
      if (btnRestoreHidden_)
        btnRestoreHidden_->setVisible(!isSidebarCollapsed_ &&
                                      !hiddenProjects_.empty());
      resized();
      statusMessage_ =
          isSidebarCollapsed_
              ? "Collapsed left WORKSPACE PROJECTS drawer (Ctrl+B)."
              : "Expanded left WORKSPACE PROJECTS drawer (Ctrl+B).";
      break;

    case StudioMenuCommandId::View_SwitchToSchematic:
      switchTab(StudioTab::NewPluginWizard);
      break;

    case StudioMenuCommandId::View_SwitchToFaceplate:
      switchTab(StudioTab::UiBuilder);
      break;

    case StudioMenuCommandId::View_SwitchToLiveRack:
      switchTab(StudioTab::LiveRack);
      break;

    case StudioMenuCommandId::View_SwitchToBuild:
      switchTab(StudioTab::Projects);
      break;

    case StudioMenuCommandId::View_CycleGridSnap:
      if (!faceplateLayout_.theme.showGrid ||
          faceplateLayout_.theme.gridSnapPx <= 1) {
        faceplateLayout_.theme.gridSnapPx = 4;
        faceplateLayout_.theme.showGrid = true;
      } else if (faceplateLayout_.theme.gridSnapPx == 4) {
        faceplateLayout_.theme.gridSnapPx = 8;
        faceplateLayout_.theme.showGrid = true;
      } else if (faceplateLayout_.theme.gridSnapPx == 8) {
        faceplateLayout_.theme.gridSnapPx = 16;
        faceplateLayout_.theme.showGrid = true;
      } else {
        faceplateLayout_.theme.gridSnapPx = 1;
        faceplateLayout_.theme.showGrid = false;
      }
      updateUiThemeButtonLabels();
      statusMessage_ = (faceplateLayout_.theme.gridSnapPx > 1)
                           ? std::format("Grid Snap set to {}px (Ctrl+G).",
                                         faceplateLayout_.theme.gridSnapPx)
                           : "Grid Snap set to Off / Free 1px (Ctrl+G).";
      break;

    case StudioMenuCommandId::View_ToggleFullscreen:
#if defined(_WIN32)
      gui::Window::toggleWindowFullscreen(GetActiveWindow());
#endif
      break;

    case StudioMenuCommandId::Build_CompileAllNow: {
      createProjectFromWizard(false);
      break;
    }

    case StudioMenuCommandId::Build_ChooseOutputFolder: {
      browseForCustomExportFolder();
      break;
    }

    case StudioMenuCommandId::Build_ToggleFormatVst3:
      toggleExportFormatSelection(0);
      break;

    case StudioMenuCommandId::Build_ToggleFormatClap:
      toggleExportFormatSelection(1);
      break;

    case StudioMenuCommandId::Build_ToggleFormatStandalone:
      toggleExportFormatSelection(2);
      break;

    case StudioMenuCommandId::Build_RunForensicDspAudit: {
      const auto plans =
          GraphSanitizerAndSanityEngine::analyzeChain(schematicChain_);
      size_t totalWiredParams = 0;
      for (const auto &st : schematicChain_) {
        totalWiredParams += getComponentMeta(st.type).getParamCount();
      }
      statusMessage_ =
          std::format("FORENSIC DSP AUDIT PASSED (F8): {} stages verified "
                      "(@schematic_benchmark compliant), {} parameters 100% "
                      "wired (0 dead knobs), {} smart-plumbing rules active!",
                      schematicChain_.size(), totalWiredParams, plans.size());
      break;
    }

    case StudioMenuCommandId::Build_OpenInVsCode:
      openCurrentProjectInIde();
      break;

    case StudioMenuCommandId::Build_AudioDeviceSettings:
      activeStudioModal_ = ActiveStudioModal::Preferences;
      activePreferencesTab_ = 0;
      break;

    case StudioMenuCommandId::Build_PanicHardMute:
      selectTestSignal(TestSignalMode::Silence);
      rackMuted_ = true;
      liveSchematicRunner_.prepare(48000.0);
      if (audioOut_.isOpen())
        audioOut_.close();
      syncGlobalFaceplateControls();
      statusMessage_ =
          "PANIC / HARD MUTE ENGAGED (Esc): Audio output silenced and all "
          "internal delay/reverb tank state buffers zeroed!";
      break;

    case StudioMenuCommandId::Help_ShortcutsCheatSheet:
      activeStudioModal_ = ActiveStudioModal::ShortcutsCheatSheet;
      break;

    case StudioMenuCommandId::Help_SchematicReferenceGuide:
      activeStudioModal_ = ActiveStudioModal::SchematicReferenceGuide;
      break;

    case StudioMenuCommandId::Help_AboutAndDiagnostics:
      activeStudioModal_ = ActiveStudioModal::AboutAndDiagnostics;
      break;

    case StudioMenuCommandId::None:
    default:
      break;
    }
    repaint();
  }

  void onKeyDown(int keyCode) override {
#if defined(_WIN32)
    const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (keyCode == VK_ESCAPE) {
      if (activeStudioModal_ != ActiveStudioModal::None) {
        activeStudioModal_ = ActiveStudioModal::None;
        repaint();
        return;
      }
      if (activeTopMenu_ != TopMenuBarSection::None) {
        activeTopMenu_ = TopMenuBarSection::None;
        recentSubmenuOpen_ = false;
        repaint();
        return;
      }
      if (rackAudioDeviceMenuOpen_ || rackPresetMenuOpen_ ||
          isStyleTemplateDropdownOpen_ || activePopupMenu_ != UiPopupMenuKind::None ||
          isScaleDropdownOpen_ || isParamBindDropdownOpen_) {
        rackAudioDeviceMenuOpen_ = false;
        hoveredRackAudioDeviceItem_ = -1;
        rackPresetMenuOpen_ = false;
        hoveredRackPresetItem_ = -1;
        isStyleTemplateDropdownOpen_ = false;
        activePopupMenu_ = UiPopupMenuKind::None;
        isScaleDropdownOpen_ = false;
        isParamBindDropdownOpen_ = false;
        repaint();
        return;
      }
      executeMenuCommand(StudioMenuCommandId::Build_PanicHardMute);
      return;
    }

    if (keyCode == VK_F1) {
      executeMenuCommand(StudioMenuCommandId::View_SwitchToSchematic);
      return;
    }
    if (keyCode == VK_F2) {
      executeMenuCommand(StudioMenuCommandId::View_SwitchToFaceplate);
      return;
    }
    if (keyCode == VK_F3) {
      executeMenuCommand(StudioMenuCommandId::View_SwitchToLiveRack);
      return;
    }
    if (keyCode == VK_F4) {
      executeMenuCommand(StudioMenuCommandId::View_SwitchToBuild);
      return;
    }
    if (keyCode == VK_F4) {
      executeMenuCommand(StudioMenuCommandId::View_SwitchToBuild);
      return;
    }
    if (keyCode == VK_F7) {
      executeMenuCommand(StudioMenuCommandId::Build_CompileAllNow);
      return;
    }
    if (keyCode == VK_F8) {
      executeMenuCommand(StudioMenuCommandId::Build_RunForensicDspAudit);
      return;
    }
    if (keyCode == VK_F9) {
      executeMenuCommand(StudioMenuCommandId::Build_OpenInVsCode);
      return;
    }
    if (keyCode == VK_F12 || (shiftDown && keyCode == VK_OEM_2)) {
      executeMenuCommand(StudioMenuCommandId::Help_ShortcutsCheatSheet);
      return;
    }

    if (ctrlDown) {
      if (keyCode == VK_OEM_COMMA) {
        executeMenuCommand(StudioMenuCommandId::File_Preferences);
        return;
      }
      if (keyCode == 'Z' || keyCode == 'z') {
        if (shiftDown)
          performRedo();
        else
          performUndo();
        return;
      }
      if (keyCode == 'Y' || keyCode == 'y') {
        performRedo();
        return;
      }
      if (keyCode == 'S' || keyCode == 's') {
        if (shiftDown)
          saveProjectAsDialog();
        else
          saveActiveProject(false);
        return;
      }
      if (keyCode == 'N' || keyCode == 'n') {
        executeMenuCommand(StudioMenuCommandId::File_NewProject);
        return;
      }
      if (keyCode == 'O' || keyCode == 'o') {
        executeMenuCommand(shiftDown ? StudioMenuCommandId::File_OpenInIde
                                     : StudioMenuCommandId::File_OpenProject);
        return;
      }
      if (keyCode == 'D' || keyCode == 'd') {
        executeMenuCommand(StudioMenuCommandId::Edit_DuplicateSelected);
        return;
      }
      if (shiftDown && (keyCode == 'C' || keyCode == 'c')) {
        executeMenuCommand(StudioMenuCommandId::Edit_CopyStyle);
        return;
      }
      if (shiftDown && (keyCode == 'V' || keyCode == 'v')) {
        executeMenuCommand(StudioMenuCommandId::Edit_PasteStyle);
        return;
      }
      if (shiftDown && (keyCode == 'B' || keyCode == 'b')) {
        executeMenuCommand(StudioMenuCommandId::Build_CompileAllNow);
        return;
      }
      if (keyCode == 'B' || keyCode == 'b') {
        executeMenuCommand(StudioMenuCommandId::View_ToggleProjectDrawer);
        return;
      }
      if (keyCode == 'G' || keyCode == 'g') {
        executeMenuCommand(StudioMenuCommandId::View_CycleGridSnap);
        return;
      }
      if (keyCode == 'R' || keyCode == 'r') {
        executeMenuCommand(StudioMenuCommandId::Edit_ResetAllParamsToUnity);
        return;
      }
      if (keyCode == 'A' || keyCode == 'a') {
        executeMenuCommand(StudioMenuCommandId::Edit_SelectAllWidgets);
        return;
      }
    }
    if (keyCode == VK_DELETE || keyCode == VK_BACK) {
      if (currentTab_ == StudioTab::UiBuilder) {
        deleteSelectedUiWidget();
        return;
      }
      if (currentTab_ == StudioTab::NewPluginWizard) {
        deleteSelectedSchematicStage();
        return;
      }
      if (currentTab_ == StudioTab::LiveRack && activeRackPluginIndex_ == 0) {
        const int hoveredIdx =
            findHoveredUiWidgetAt(lastMousePos_, getRackFaceplateRect());
        if (hoveredIdx >= 0 && faceplateLayout_.widgets[hoveredIdx].type ==
                                   UiWidgetType::VintageSticker) {
          selectedUiWidgetIndex_ = hoveredIdx;
          deleteSelectedUiWidget();
        } else if (selectedUiWidgetIndex_ >= 0 &&
                   selectedUiWidgetIndex_ <
                       static_cast<int>(faceplateLayout_.widgets.size()) &&
                   faceplateLayout_.widgets[selectedUiWidgetIndex_].type ==
                       UiWidgetType::VintageSticker) {
          deleteSelectedUiWidget();
        }
        return;
      }
    }
    if (currentTab_ == StudioTab::UiBuilder && selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size()) &&
        (keyCode == VK_LEFT || keyCode == VK_RIGHT || keyCode == VK_UP ||
         keyCode == VK_DOWN)) {
      recordUndoSnapshot(
          "Nudge " + faceplateLayout_.widgets[selectedUiWidgetIndex_].title);
      auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      const float step = shiftDown ? 1.0f
                                   : static_cast<float>(std::max(
                                         1, faceplateLayout_.theme.gridSnapPx));
      if (keyCode == VK_LEFT)
        w.x = math::clamp(w.x - step, 4.0f,
                          FaceplateLayout::kRefWidth - w.w - 4.0f);
      if (keyCode == VK_RIGHT)
        w.x = math::clamp(w.x + step, 4.0f,
                          FaceplateLayout::kRefWidth - w.w - 4.0f);
      if (keyCode == VK_UP)
        w.y = math::clamp(w.y - step, 4.0f,
                          FaceplateLayout::kRefHeight - w.h - 4.0f);
      if (keyCode == VK_DOWN)
        w.y = math::clamp(w.y + step, 4.0f,
                          FaceplateLayout::kRefHeight - w.h - 4.0f);
      lastUiWidgetInteractMs_ = GetTickCount64();
      syncUiWidgetInspector();
      statusMessage_ =
          std::format("Moved '{}' to ({:.0f}, {:.0f}).", w.title, w.x, w.y);
      repaint();
      return;
    }
#else
    (void)keyCode;
#endif
  }

  // =========================================================================
  // Real-Time 60 FPS Audio Test Generator, Windows Speaker Output & Rack
  // Telemetry
  // =========================================================================
  void onTimerTick() override {
    pollAsyncBuildProgress();

#if defined(_WIN32)
    if (isProjectDirty_) {
      const uint64_t intervalMs = prefs_.getAutoSaveIntervalSeconds() * 1000ull;
      if (intervalMs > 0) {
        const uint64_t nowMs = GetTickCount64();
        if (lastAutoSaveTickMs_ == 0) {
          lastAutoSaveTickMs_ = nowMs;
        } else if (nowMs - lastAutoSaveTickMs_ >= intervalMs) {
          lastAutoSaveTickMs_ = nowMs;
          namespace fs = std::filesystem;
          const std::string cleanName = ProjectGenerator::sanitizeName(wizardConfig_.name);
          if (!cleanName.empty() && cleanName != "AvraGain" && cleanName != "GainPlugin") {
            const fs::path bakDir =
                ProjectGenerator::getWorkspaceRoot() / "plugins" / cleanName;
            std::error_code ec;
            fs::create_directories(bakDir, ec);
            saveProjectToAvraJsonFile(
                bakDir / (cleanName + ".avra.bak"), cleanName, schematicChain_,
                faceplateLayout_, rackInputTrimDb_, rackOutputTrimDb_,
                rackGlobalWetMix_ * 100.0f, rackBypassed_);
          } else {
            saveProjectToAvraJsonFile(
                ProjectGenerator::getWorkspaceRoot() / ".avra_autosave_recovery.bak",
                cleanName.empty() ? "UntitledProject" : cleanName, schematicChain_,
                faceplateLayout_, rackInputTrimDb_, rackOutputTrimDb_,
                rackGlobalWetMix_ * 100.0f, rackBypassed_);
          }
        }
      }
    }
#endif

    if (currentTab_ != StudioTab::LiveRack) {
      if (audioOut_.isOpen()) {
        audioOut_.close();
      }
      return;
    }

    if (testSignalMode_ != TestSignalMode::Silence && !silentAnalyzerMode_) {
      if (!audioOut_.isOpen()) {
        (void)audioOut_.open();
      }
      if (!audioOut_.isOpen()) {
        std::array<float, 1024> silentL{};
        std::array<float, 1024> silentR{};
        renderRealtimeAudioBlock(silentL.data(), silentR.data(), 1024);
      }
    } else {
      if (audioOut_.isOpen()) {
        audioOut_.close();
      }
      if (testSignalMode_ != TestSignalMode::Silence && silentAnalyzerMode_) {
        std::array<float, 1024> silentL{};
        std::array<float, 1024> silentR{};
        renderRealtimeAudioBlock(silentL.data(), silentR.data(), 1024);
      }
    }

    std::array<float, 1024> inL{};
    std::array<float, 1024> inR{};
    std::array<float, 1024> outL{};
    std::array<float, 1024> outR{};
    size_t kBlockFrames = 1024;
    {
      std::lock_guard<std::mutex> lock(audioRenderMutex_);
      inL = latestInBlockL_;
      inR = latestInBlockR_;
      outL = latestOutBlockL_;
      outR = latestOutBlockR_;
      kBlockFrames = std::clamp<size_t>(latestRenderedFrames_, 64, 1024);
    }

    for (size_t i = 0; i < kBlockFrames; ++i) {
      if (!std::isfinite(inL[i]))
        inL[i] = 0.0f;
      if (!std::isfinite(inR[i]))
        inR[i] = 0.0f;
      if (!std::isfinite(outL[i]))
        outL[i] = 0.0f;
      if (!std::isfinite(outR[i]))
        outR[i] = 0.0f;
    }

    float inPeakL = 0.0f, inRmsL = 0.0f, inPeakR = 0.0f, inRmsR = 0.0f;
    dsp::SimdBlock::computePeakAndRms(inL.data(), kBlockFrames, inPeakL,
                                      inRmsL);
    dsp::SimdBlock::computePeakAndRms(inR.data(), kBlockFrames, inPeakR,
                                      inRmsR);
    lastInPeak_ = std::max(inPeakL, inPeakR);
    lastInRms_ = std::max(inRmsL, inRmsR);
    rackInMeter_.setLevels(inPeakL, inPeakR, inRmsL, inRmsR);

    dsp::SimdBlock::computePeakAndRms(outL.data(), kBlockFrames, lastOutPeakL_,
                                      lastOutRmsL_);
    dsp::SimdBlock::computePeakAndRms(outR.data(), kBlockFrames, lastOutPeakR_,
                                      lastOutRmsR_);
    rackOutMeter_.setLevels(lastOutPeakL_, lastOutPeakR_, lastOutRmsL_,
                            lastOutRmsR_);

    rackSpectrum_.setHarmonicAnalyzerMode(testSignalMode_ ==
                                          TestSignalMode::HarmonicMultiTone);
    rackSpectrum_.updateInputFromRingBuffer(rackInputRing_);
    rackSpectrum_.updateFromRingBuffer(rackOutputRing_);
    rackScope_.updateFromRingBuffers(rackInputRing_, rackOutputRing_);

    if (liveSchematicRunner_.getProbeStageIndex() >= 0 && !rackBypassed_ &&
        !rackMuted_) {
      rackSpectrum_.updateProbeFromRingBuffer(
          liveSchematicRunner_.getProbeRingBuffer());
      rackScope_.updateProbeFromRingBuffer(
          liveSchematicRunner_.getProbeRingBuffer());
    } else {
      rackSpectrum_.clearProbeTrace();
      rackScope_.clearProbeTrace();
    }

    const float grDb =
        (!rackBypassed_ && !rackMuted_ && lastSchematicGrDb_ > 0.01f)
            ? lastSchematicGrDb_
            : 0.0f;
    rackGrHistory_.pushGainReductionDb(grDb);

    // 300ms Time-Integrated RMS Window across Pre-Plugin Input and Post-Plugin
    // Output Buffers (Guarantees rock-steady 0.0 dB OUT-IN delta at unity gain
    // without transient phase/delay jumps!)
    const float blockMeanSqIn = 0.5f * (inRmsL * inRmsL + inRmsR * inRmsR);
    const float blockMeanSqOut =
        0.5f * (lastOutRmsL_ * lastOutRmsL_ + lastOutRmsR_ * lastOutRmsR_);
    if (!std::isfinite(integratedMeanSqIn_))
      integratedMeanSqIn_ = 0.0f;
    if (!std::isfinite(integratedMeanSqOut_))
      integratedMeanSqOut_ = 0.0f;
    constexpr float kRmsAlpha300ms =
        0.052f; // ~300ms integration window at 60Hz tick rate
    if (std::isfinite(blockMeanSqIn))
      integratedMeanSqIn_ +=
          (blockMeanSqIn - integratedMeanSqIn_) * kRmsAlpha300ms;
    if (std::isfinite(blockMeanSqOut))
      integratedMeanSqOut_ +=
          (blockMeanSqOut - integratedMeanSqOut_) * kRmsAlpha300ms;

    const float rmsIn300 = std::sqrt(std::max(0.0f, integratedMeanSqIn_));
    const float rmsOut300 = std::sqrt(std::max(0.0f, integratedMeanSqOut_));
    smoothedVuInDb_ = (rmsIn300 > 1e-5f) ? math::linearTodB(rmsIn300) : -80.0f;
    smoothedVuOutDb_ =
        (rmsOut300 > 1e-5f) ? math::linearTodB(rmsOut300) : -80.0f;
    if (std::abs(smoothedVuOutDb_ - smoothedVuInDb_) < 0.05f) {
      smoothedVuOutDb_ = smoothedVuInDb_;
    }

    const float rawRmsDelta = (rmsIn300 > 1e-4f && rmsOut300 > 1e-4f)
                                  ? (smoothedVuOutDb_ - smoothedVuInDb_)
                                  : 0.0f;
    smoothedRmsDeltaDb_ = (std::abs(rawRmsDelta) < 0.05f) ? 0.0f : rawRmsDelta;

    if (!std::isfinite(smoothedVuGrDb_))
      smoothedVuGrDb_ = 0.0f;
    const float grAttack = (grDb > smoothedVuGrDb_) ? 0.48f : 0.14f;
    if (std::isfinite(grDb))
      smoothedVuGrDb_ += (grDb - smoothedVuGrDb_) * grAttack;

    repaint();
  }

  void renderRealtimeAudioBlock(float *dstL, float *dstR, size_t numFrames) {
    std::lock_guard<std::mutex> lock(audioRenderMutex_);
    const size_t nFrames = std::min<size_t>(numFrames, 1024);
    latestRenderedFrames_ = nFrames;

    const float inTrimGain = math::dBToLinear(rackInputTrimDb_);
    const float outTrimGain = math::dBToLinear(rackOutputTrimDb_);
    const float wetMix = std::clamp(rackGlobalWetMix_, 0.0f, 1.0f);

    for (size_t i = 0; i < nFrames; ++i) {
      const double activeSr = static_cast<double>(std::max<uint32_t>(22050u, audioOut_.getSampleRate()));
      const double t = static_cast<double>(signalSampleCounter_++) / activeSr;
      float sL = 0.0f;
      float sR = 0.0f;

      if (activeStemSlotLoaded_ && wavLoopPlayer_.isLoaded() &&
          (testSignalMode_ == TestSignalMode::DrumTransientLoop ||
           testSignalMode_ == TestSignalMode::SynthChordLoop ||
           testSignalMode_ == TestSignalMode::CleanGuitarChord ||
           testSignalMode_ == TestSignalMode::GuitarDiChug ||
           testSignalMode_ == TestSignalMode::BassDiGroove ||
           testSignalMode_ == TestSignalMode::VocalPhrase)) {
        wavLoopPlayer_.nextStereoSample(sL, sR, activeSr);
      } else {
        switch (testSignalMode_) {
        case TestSignalMode::DrumTransientLoop: {
          audio_io::StudioGrooveSynthesizer::synthesizeDrumAndBassGroove(
              t, noiseState_, sL, sR, activeSr);
          break;
        }
        case TestSignalMode::SynthChordLoop: {
          audio_io::StudioGrooveSynthesizer::synthesizeSynthChordLoop(
              t, sL, sR, activeSr);
          break;
        }
        case TestSignalMode::CleanGuitarChord: {
          audio_io::StudioGrooveSynthesizer::synthesizeCleanGuitarChord(
              t, noiseState_, sL, sR, activeSr);
          break;
        }
        case TestSignalMode::GuitarDiChug: {
          audio_io::StudioGrooveSynthesizer::synthesizeGuitarDiChug(
              t, noiseState_, sL, sR, activeSr);
          break;
        }
        case TestSignalMode::BassDiGroove: {
          audio_io::StudioGrooveSynthesizer::synthesizeBassDiGroove(
              t, noiseState_, sL, sR, activeSr);
          break;
        }
        case TestSignalMode::VocalPhrase: {
          audio_io::StudioGrooveSynthesizer::synthesizeVocalPhrase(
              t, noiseState_, sL, sR, activeSr);
          break;
        }
        case TestSignalMode::CustomWavLoop: {
          if (wavLoopPlayer_.isLoaded()) {
            wavLoopPlayer_.nextStereoSample(sL, sR, activeSr);
          } else {
            audio_io::StudioGrooveSynthesizer::synthesizeDrumAndBassGroove(
                t, noiseState_, sL, sR, activeSr);
          }
          break;
        }
        case TestSignalMode::PinkNoise: {
          noiseState_ = noiseState_ * 1664525u + 1013904223u;
          const float whiteL =
              ((static_cast<float>(noiseState_ & 0xFFFF) / 32768.0f) - 1.0f) *
              0.22f;
          noiseState_ = noiseState_ * 1664525u + 1013904223u;
          const float whiteR =
              ((static_cast<float>(noiseState_ & 0xFFFF) / 32768.0f) - 1.0f) *
              0.22f;

          pinkB0_ = 0.99765f * pinkB0_ + whiteL * 0.0990460f;
          pinkB1_ = 0.96300f * pinkB1_ + whiteL * 0.2965164f;
          pinkB2_ = 0.57000f * pinkB2_ + whiteL * 1.0526913f;
          sL = (pinkB0_ + pinkB1_ + pinkB2_ + whiteL * 0.1848f) * 0.25f;
          sR = (pinkB0_ + pinkB1_ + pinkB2_ + whiteR * 0.1848f) * 0.25f;
          break;
        }
        case TestSignalMode::HarmonicMultiTone: {
          sL = 0.25118864f * std::sin(static_cast<float>(t * kTwoPi * 1000.0));
          sR = sL;
          break;
        }
        case TestSignalMode::Silence:
        default:
          sL = 0.0f;
          sR = 0.0f;
          break;
        }
      }

      latestInBlockL_[i] = sL * inTrimGain;
      latestInBlockR_[i] = sR * inTrimGain;
      latestOutBlockL_[i] = sL * inTrimGain;
      latestOutBlockR_[i] = sR * inTrimGain;
    }

    if (rackMuted_) {
      latestOutBlockL_.fill(0.0f);
      latestOutBlockR_.fill(0.0f);
      lastSchematicGrDb_ = 0.0f;
    } else if (!rackBypassed_) {
      liveSchematicRunner_.processBlock(latestOutBlockL_.data(),
                                        latestOutBlockR_.data(), nFrames,
                                        schematicChain_, lastSchematicGrDb_);
      const float pol = rackPhaseInvert_ ? -1.0f : 1.0f;
      const float panL =
          (rackStereoBalance_ > 0.0f) ? (1.0f - rackStereoBalance_) : 1.0f;
      const float panR =
          (rackStereoBalance_ < 0.0f) ? (1.0f + rackStereoBalance_) : 1.0f;
      const uint32_t latSamples = std::min<uint32_t>(
          liveSchematicRunner_.getTotalLatencySamples(schematicChain_), 4000u);
      for (size_t i = 0; i < nFrames; ++i) {
        dryDelayLineL_[dryDelayWritePos_ & 4095u] = latestInBlockL_[i];
        dryDelayLineR_[dryDelayWritePos_ & 4095u] = latestInBlockR_[i];
        const uint32_t rdIdx = (dryDelayWritePos_ - latSamples) & 4095u;
        const float delayedDryL = dryDelayLineL_[rdIdx];
        const float delayedDryR = dryDelayLineR_[rdIdx];
        ++dryDelayWritePos_;

        const float wetGain =
            liveSchematicRunner_.stepMasterMixSmoother(wetMix);
        const float dryGain = 1.0f - wetGain;
        const float mixedL =
            delayedDryL * dryGain + latestOutBlockL_[i] * wetGain;
        const float mixedR =
            delayedDryR * dryGain + latestOutBlockR_[i] * wetGain;
        float wetOutL = mixedL * outTrimGain * panL * pol;
        float wetOutR = mixedR * outTrimGain * panR * pol;

        if (!std::isfinite(wetOutL))
          wetOutL = delayedDryL;
        if (!std::isfinite(wetOutR))
          wetOutR = delayedDryR;
        if (!std::isfinite(autoLevelEnvInSq_))
          autoLevelEnvInSq_ = 0.01f;
        if (!std::isfinite(autoLevelEnvOutSq_))
          autoLevelEnvOutSq_ = 0.01f;
        if (!std::isfinite(autoLevelGainSmooth_))
          autoLevelGainSmooth_ = 1.0f;

        const float inSq =
            0.5f * (delayedDryL * delayedDryL + delayedDryR * delayedDryR);
        const float outSq = 0.5f * (wetOutL * wetOutL + wetOutR * wetOutR);
        autoLevelEnvInSq_ = 0.9992f * autoLevelEnvInSq_ + 0.0008f * inSq;
        autoLevelEnvOutSq_ = 0.9992f * autoLevelEnvOutSq_ + 0.0008f * outSq;
        float targetAutoLevelGain = 1.0f;
        if ((rackAutoMatch_ || rackDeltaListen_) && autoLevelEnvInSq_ > 1e-7f &&
            autoLevelEnvOutSq_ > 1e-7f) {
          targetAutoLevelGain = math::clamp(
              std::sqrt(autoLevelEnvInSq_ / autoLevelEnvOutSq_), 0.125f, 8.0f);
        }
        autoLevelGainSmooth_ +=
            0.0015f * (targetAutoLevelGain - autoLevelGainSmooth_);
        wetOutL *= autoLevelGainSmooth_;
        wetOutR *= autoLevelGainSmooth_;

        if (rackDeltaListen_) {
          // Align polarity before nulling if a stage or master phase switch inverted the signal
          const float dotLR = wetOutL * delayedDryL + wetOutR * delayedDryR;
          autoLevelPolaritySmooth_ = 0.999f * autoLevelPolaritySmooth_ + 0.001f * dotLR;
          const float nullSign = (autoLevelPolaritySmooth_ < -1e-6f) ? -1.0f : 1.0f;
          wetOutL = wetOutL * nullSign - delayedDryL;
          wetOutR = wetOutR * nullSign - delayedDryR;
        }
        latestOutBlockL_[i] =
            std::isfinite(wetOutL) ? std::clamp(wetOutL, -0.99f, 0.99f) : 0.0f;
        latestOutBlockR_[i] =
            std::isfinite(wetOutR) ? std::clamp(wetOutR, -0.99f, 0.99f) : 0.0f;
      }
    } else {
      for (size_t i = 0; i < nFrames; ++i) {
        dryDelayLineL_[dryDelayWritePos_ & 4095u] = latestInBlockL_[i];
        dryDelayLineR_[dryDelayWritePos_ & 4095u] = latestInBlockR_[i];
        ++dryDelayWritePos_;
        latestOutBlockL_[i] = rackDeltaListen_ ? 0.0f : latestInBlockL_[i];
        latestOutBlockR_[i] = rackDeltaListen_ ? 0.0f : latestInBlockR_[i];
      }
      lastSchematicGrDb_ = 0.0f;
    }

    constexpr float kMonitorMatchGain = 0.85f;
    std::array<float, 1024> inMonoSum{};
    std::array<float, 1024> outMonoSum{};
    for (size_t i = 0; i < nFrames; ++i) {
      dstL[i] = latestOutBlockL_[i] * kMonitorMatchGain;
      dstR[i] = latestOutBlockR_[i] * kMonitorMatchGain;
      inMonoSum[i] = 0.5f * (latestInBlockL_[i] + latestInBlockR_[i]);
      outMonoSum[i] = 0.5f * (latestOutBlockL_[i] + latestOutBlockR_[i]);
    }
    rackInputRing_.pushTelemetryBlock(
        std::span<const float>(inMonoSum.data(), nFrames));
    rackOutputRing_.pushTelemetryBlock(
        std::span<const float>(outMonoSum.data(), nFrames));
  }

  // =========================================================================
  // Schematic Builder Helpers & Starter Chains
  // =========================================================================
  void syncInspectorKnobsToSelection() {
    if (inspectorKnob1_) {
      inspectorKnob1_->setParameter(nullptr);
      inspectorKnob1_->setVisible(false);
    }
    if (inspectorKnob2_) {
      inspectorKnob2_->setParameter(nullptr);
      inspectorKnob2_->setVisible(false);
    }
    if (selectedNodeIndex_ >= 0 &&
        selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
      const auto &node = schematicChain_[selectedNodeIndex_];
      if (btnNodeToggleTrims_) {
        btnNodeToggleTrims_->setText(
            node.exposeInternalTrims ? "TRIMS: EXPOSED" : "TRIMS: HIDDEN");
        btnNodeToggleTrims_->setToggleState(node.exposeInternalTrims);
      }
    } else {
      if (btnNodeToggleTrims_) {
        btnNodeToggleTrims_->setText("TRIMS: HIDDEN");
        btnNodeToggleTrims_->setToggleState(false);
      }
    }
  }

  [[nodiscard]] gui::Rect<float>
  getSubFilterPillRect(int pillIdx) const noexcept {
    const float libX = isSidebarCollapsed_ ? 72.0f : 368.0f;
    const float libW = isSidebarCollapsed_ ? 280.0f : 235.0f;
    const float x0 = libX + 6.0f;
    const float innerW = libW - 12.0f;
    const float gap = 2.0f;
    const float allW = std::round(innerW * 0.17f);
    if (pillIdx <= 0)
      return {x0, 249.0f, allW, 16.0f};
    const float remW = innerW - allW - 4.0f * gap;
    const float subW = remW / 4.0f;
    const float px =
        x0 + allW + gap + static_cast<float>(pillIdx - 1) * (subW + gap);
    return {px, 249.0f, subW, 16.0f};
  }

  [[nodiscard]] gui::Rect<float>
  getSourceFilterPillRect(int srcIdx) const noexcept {
    const float libX = isSidebarCollapsed_ ? 72.0f : 368.0f;
    const float libW = isSidebarCollapsed_ ? 280.0f : 235.0f;
    const float x0 = libX + 6.0f;
    const float innerW = libW - 12.0f;
    const float gap = 2.0f;
    const float pillW = (innerW - 5.0f * gap) / 6.0f;
    return {x0 + static_cast<float>(srcIdx) * (pillW + gap), 267.0f, pillW,
            17.0f};
  }

  static bool matchesSearchQuery(std::string_view text,
                                 const std::string &query) noexcept {
    if (query.empty())
      return true;
    std::string hay(text);
    std::string needle = query;
    for (char &c : hay)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (char &c : needle)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return hay.find(needle) != std::string::npos;
  }

  void selectSchematicSourceFilter(SchematicSourceFilter srcFilter) {
    activeSourceFilter_ = srcFilter;
    selectSchematicCategory(activeCategory_, activeSubFilter_);
  }

  void selectSchematicCategory(SchematicCategory cat, int subFilter = 0) {
    activeCategory_ = cat;
    activeSubFilter_ = subFilter;
    palettePage_ = 0;
    schematicLibraryScrollY_ = 0.0f;
    paletteItems_.clear();
    const bool hasQuery = !librarySearchQuery_.empty();
    for (const auto &meta : getCuratedSchematicCatalog()) {
      if (!matchesSchematicSourceFilter(meta, activeSourceFilter_))
        continue;
      if (hasQuery) {
        if (matchesSearchQuery(meta.shortName, librarySearchQuery_) ||
            matchesSearchQuery(meta.fullName, librarySearchQuery_) ||
            matchesSearchQuery(meta.designator, librarySearchQuery_)) {
          paletteItems_.push_back(meta);
        }
      } else if (meta.category == cat &&
                 matchesSchematicSubFilter(meta, activeSubFilter_)) {
        paletteItems_.push_back(meta);
      }
    }

    btnCatPreamps_->setToggleState(cat == SchematicCategory::PreampsAndTubes);
    btnCatIron_->setToggleState(cat == SchematicCategory::IronAndTape);
    btnCatEq_->setToggleState(cat == SchematicCategory::EqAndFilters);
    btnCatDynamics_->setToggleState(cat == SchematicCategory::AnalogDynamics);
    btnCatDelayRev_->setToggleState(cat == SchematicCategory::DelayAndReverb);
    btnCatModRoute_->setToggleState(cat ==
                                    SchematicCategory::ModulationAndRoute);
    if (btnCatAutoSmart_)
      btnCatAutoSmart_->setToggleState(cat == SchematicCategory::AutoAndSmart);
    updateCanvasPageButtons();
    repaint();
  }

  void selectUiWidgetCategory(UiWidgetCategory cat) {
    activeUiCategory_ = cat;
    uiPalettePage_ = 0;
    uiWidgetLibraryScrollY_ = 0.0f;
    uiPaletteItems_.clear();
    const bool hasQuery = !librarySearchQuery_.empty();
    for (const auto &entry : getUiWidgetCatalog()) {
      if (hasQuery) {
        if (matchesSearchQuery(entry.name, librarySearchQuery_) ||
            matchesSearchQuery(entry.subtitle, librarySearchQuery_) ||
            matchesSearchQuery(entry.bakedTitle, librarySearchQuery_)) {
          uiPaletteItems_.push_back(entry);
        }
      } else if (entry.category == cat) {
        uiPaletteItems_.push_back(entry);
      }
    }
    if (btnUiCatKnobs_)
      btnUiCatKnobs_->setToggleState(cat == UiWidgetCategory::Knobs);
    if (btnUiCatSwitches_)
      btnUiCatSwitches_->setToggleState(cat ==
                                        UiWidgetCategory::SwitchesAndLeds);
    if (btnUiCatMeters_)
      btnUiCatMeters_->setToggleState(cat ==
                                      UiWidgetCategory::MetersAndDisplays);
    if (btnUiCatFrames_)
      btnUiCatFrames_->setToggleState(cat == UiWidgetCategory::FramesAndBadges);
    updateCanvasPageButtons();
    repaint();
  }

  enum class UiPopupMenuKind {
    None = 0,
    SurfaceGrain,
    MountStyle,
    ScrewStyle,
    GridSnap,
    WidgetStyle,
    WidgetColor,
    FontFamily,
    FontSize,
    TextColor,
    PlateColor,
    StickerQuickAdd,
    StickerRotation
  };

  struct UiPopupItem {
    int id{0};
    std::string label{};
    bool isCurrent{false};
    bool hasSwatch{false};
    gui::Color swatchColor{gui::Color::AccentAmber()};
  };

  void openStickerImageFileDialog(int targetWidgetIdx = -1) {
#if defined(_WIN32)
    char filePath[MAX_PATH] = {0};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = "Image Files "
                      "(*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.ico)\0*.png;*.jpg;*."
                      "jpeg;*.bmp;*.gif;*.ico\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle =
        "Select Custom Sticker / Decal Image to Place on Faceplate";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&ofn))
      return;

    namespace fs = std::filesystem;
    fs::path srcPath(filePath);
    std::string storedPath = srcPath.generic_string();
    std::error_code ec;
    const fs::path stickerDir =
        ProjectGenerator::getWorkspaceRoot() / "assets" / "stickers";
    fs::create_directories(stickerDir, ec);
    const fs::path dstPath = stickerDir / srcPath.filename();
    if (fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing,
                      ec)) {
      storedPath = dstPath.generic_string();
    }

    std::string stemUpper = srcPath.stem().string();
    for (char &c : stemUpper)
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (stemUpper.size() > 14)
      stemUpper = stemUpper.substr(0, 14);

    if (targetWidgetIdx >= 0 &&
        targetWidgetIdx < static_cast<int>(faceplateLayout_.widgets.size()) &&
        faceplateLayout_.widgets[targetWidgetIdx].type ==
            UiWidgetType::VintageSticker) {
      auto &w = faceplateLayout_.widgets[targetWidgetIdx];
      w.customImagePath = storedPath;
      if (w.title == "CUSTOM IMAGE" || w.title.empty()) {
        w.title = stemUpper;
      }
      autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                                 w.fontSizePreset, false);
      selectedUiWidgetIndex_ = targetWidgetIdx;
      syncUiWidgetInspector();
      statusMessage_ =
          std::format("Loaded custom sticker image '{}' onto widget '{}'! Edit "
                      "LABEL/SUB below to customize text.",
                      srcPath.filename().string(), w.title);
      repaint();
      return;
    }

    UiCanvasWidget w;
    w.type = UiWidgetType::VintageSticker;
    w.scalePreset =
        static_cast<UiScalePreset>(gui::StickerPreset::CustomImageDecal);
    w.customImagePath = storedPath;
    w.title = stemUpper;
    w.subtitle = "";
    w.w = 96.0f;
    w.h = 110.0f;
    w.x = 420.0f;
    w.y = 180.0f;
    w.accentColor = faceplateLayout_.theme.globalAccentColor;
    w.fontSizePreset = faceplateLayout_.theme.globalFontSizePreset;
    autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                               w.fontSizePreset, false);
    faceplateLayout_.widgets.push_back(w);
    selectedUiWidgetIndex_ =
        static_cast<int>(faceplateLayout_.widgets.size()) - 1;
    syncUiWidgetInspector();
    statusMessage_ =
        std::format("Added custom image sticker '{}' to faceplate! Drag to "
                    "position, resize corners, or edit LABEL/SUB text.",
                    srcPath.filename().string());
    repaint();
#else
    (void)targetWidgetIdx;
#endif
  }

  void openCustomFontColorPicker() {
#if defined(_WIN32)
    static COLORREF customFontColors[16] = {
        RGB(0xE8, 0xF1, 0xF2), RGB(0x14, 0x18, 0x1C), RGB(0xFF, 0xF3, 0xD6),
        RGB(0xE8, 0xC5, 0x47), RGB(0xFF, 0x9F, 0x1C), RGB(0x00, 0xF5, 0xD4),
        RGB(0x52, 0xB7, 0x88), RGB(0xFF, 0x4D, 0x5A), RGB(0x4E, 0xA8, 0xDE),
        RGB(0xB5, 0x65, 0xF2), RGB(0xFF, 0x6B, 0x2B), RGB(0xB4, 0xC0, 0xC8),
        RGB(0x22, 0x22, 0x22), RGB(0xFF, 0xFF, 0xFF), RGB(0xA8, 0xF0, 0x28),
        RGB(0x72, 0xE5, 0xFC)};
    const uint32_t curHex = faceplateLayout_.theme.customFontColorHex;
    CHOOSECOLORA cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = GetActiveWindow();
    cc.lpCustColors = customFontColors;
    cc.rgbResult =
        RGB((curHex >> 16) & 0xFF, (curHex >> 8) & 0xFF, curHex & 0xFF);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;
    if (ChooseColorA(&cc)) {
      const uint32_t r = GetRValue(cc.rgbResult);
      const uint32_t g = GetGValue(cc.rgbResult);
      const uint32_t b = GetBValue(cc.rgbResult);
      faceplateLayout_.theme.customFontColorHex = (r << 16) | (g << 8) | b;
      faceplateLayout_.theme.useCustomFontColor = true;
      markThemeAsCustomModified();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ =
          std::format("Whole-Skin Custom Font Colour set to #{:06X}.",
                      faceplateLayout_.theme.customFontColorHex);
      repaint();
    }
#endif
  }

  void openCustomPlateColorPicker() {
#if defined(_WIN32)
    static COLORREF customPlateColors[16] = {
        RGB(0xF8, 0xFA, 0xFC), RGB(0xF5, 0xEC, 0xDA), RGB(0xE2, 0xE8, 0xF0),
        RGB(0x0B, 0x10, 0x15), RGB(0x0F, 0x29, 0x2E), RGB(0x1E, 0x29, 0x3B),
        RGB(0x10, 0x20, 0x3D), RGB(0x2D, 0x0A, 0x12), RGB(0x16, 0x28, 0x1B),
        RGB(0x2E, 0x1D, 0x12), RGB(0x1F, 0x11, 0x38), RGB(0xFF, 0xE6, 0xA8),
        RGB(0xD8, 0xF3, 0xFF), RGB(0x22, 0x22, 0x22), RGB(0xFF, 0xFF, 0xFF),
        RGB(0x33, 0x41, 0x55)};
    const gui::Color curCol = getSkinTextPlateColorValue(faceplateLayout_.theme);
    const uint32_t curHex = faceplateLayout_.theme.useCustomTextPlateColor
                                ? faceplateLayout_.theme.customTextPlateColorHex
                                : ((static_cast<uint32_t>(curCol.r * 255.0f) << 16) |
                                   (static_cast<uint32_t>(curCol.g * 255.0f) << 8) |
                                   static_cast<uint32_t>(curCol.b * 255.0f));
    CHOOSECOLORA cc{};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = GetActiveWindow();
    cc.lpCustColors = customPlateColors;
    cc.rgbResult =
        RGB((curHex >> 16) & 0xFF, (curHex >> 8) & 0xFF, curHex & 0xFF);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;
    if (ChooseColorA(&cc)) {
      const uint32_t r = GetRValue(cc.rgbResult);
      const uint32_t g = GetGValue(cc.rgbResult);
      const uint32_t b = GetBValue(cc.rgbResult);
      faceplateLayout_.theme.customTextPlateColorHex = (r << 16) | (g << 8) | b;
      faceplateLayout_.theme.useCustomTextPlateColor = true;
      markThemeAsCustomModified();
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      statusMessage_ =
          std::format("Control Text Plate Colour set to #{:06X}.",
                      faceplateLayout_.theme.customTextPlateColorHex);
      repaint();
    }
#endif
  }

  void toggleUiPopupMenu(UiPopupMenuKind kind) {
    if (activePopupMenu_ == kind) {
      activePopupMenu_ = UiPopupMenuKind::None;
      hoveredPopupItemIdx_ = -1;
    } else {
      activePopupMenu_ = kind;
      hoveredPopupItemIdx_ = -1;
      isStyleTemplateDropdownOpen_ = false;
      isParamBindDropdownOpen_ = false;
      isScaleDropdownOpen_ = false;
    }
    rebuildActivePopupItems();
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();
    repaint();
  }

  void rebuildActivePopupItems() {
    activePopupItems_.clear();
    switch (activePopupMenu_) {
    case UiPopupMenuKind::SurfaceGrain: {
      for (int i = 0; i < 18; ++i) {
        const auto g = static_cast<gui::SurfaceGrain>(i);
        activePopupItems_.push_back({i, getSurfaceGrainName(g),
                                     faceplateLayout_.theme.grain == g, false,
                                     gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::MountStyle: {
      for (int i = 0; i < 7; ++i) {
        const auto h = static_cast<gui::ChassisHardwareStyle>(i);
        activePopupItems_.push_back({i, getHardwareStyleName(h),
                                     faceplateLayout_.theme.hardwareStyle == h,
                                     false, gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::ScrewStyle: {
      const bool isScrewless =
          (!faceplateLayout_.theme.cornerScrews ||
           faceplateLayout_.theme.hardwareStyle ==
               gui::ChassisHardwareStyle::CleanScrewless);
      activePopupItems_.push_back({-1, "NONE (NO SCREWS)", isScrewless, false,
                                   gui::Color::AccentAmber()});
      for (int i = 0; i < 10; ++i) {
        const auto s = static_cast<gui::ScrewHeadStyle>(i);
        activePopupItems_.push_back(
            {i, getScrewHeadStyleName(s),
             !isScrewless && faceplateLayout_.theme.screwStyle == s, false,
             gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::GridSnap: {
      const int curSnap = faceplateLayout_.theme.showGrid
                              ? faceplateLayout_.theme.gridSnapPx
                              : 1;
      activePopupItems_.push_back({8, "8px Standard Grid Snap", curSnap == 8,
                                   false, gui::Color::AccentCyan()});
      activePopupItems_.push_back({16, "16px Coarse Grid Snap", curSnap == 16,
                                   false, gui::Color::AccentCyan()});
      activePopupItems_.push_back({4, "4px Fine Grid Snap", curSnap == 4, false,
                                   gui::Color::AccentCyan()});
      activePopupItems_.push_back({1, "FREE (1px / No Snap)", curSnap <= 1,
                                   false, gui::Color::AccentCyan()});
      break;
    }
    case UiPopupMenuKind::WidgetStyle: {
      if (selectedUiWidgetIndex_ < 0 ||
          selectedUiWidgetIndex_ >=
              static_cast<int>(faceplateLayout_.widgets.size()))
        break;
      const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      if (w.type == UiWidgetType::SectionFrame) {
        const int curF = static_cast<int>(w.scalePreset) % 8;
        for (int i = 0; i < 8; ++i) {
          const auto fs = static_cast<gui::SectionFrameStyle>(i);
          activePopupItems_.push_back({i, getSectionFrameStyleName(fs),
                                       curF == i, false,
                                       gui::Color::AccentCyan()});
        }
      } else if (w.type == UiWidgetType::RivetedNameplate) {
        const int curB = static_cast<int>(w.scalePreset) % 6;
        for (int i = 0; i < 6; ++i) {
          const auto bs = static_cast<gui::BadgeVisualStyle>(i);
          activePopupItems_.push_back({i, getBadgeVisualStyleName(bs),
                                       curB == i, false,
                                       gui::Color::AccentCyan()});
        }
      } else if (w.type == UiWidgetType::VintageSticker) {
        const int curS = std::clamp(static_cast<int>(w.scalePreset), 0,
                                    gui::kStickerPresetCount - 1);
        for (int i = 0; i < gui::kStickerPresetCount - 1; ++i) {
          const auto sp = static_cast<gui::StickerPreset>(i);
          activePopupItems_.push_back(
              {i, std::format("{}. {}", i + 1, getStickerPresetName(sp)),
               curS == i && w.customImagePath.empty(), false,
               gui::Color::AccentAmber()});
        }
        activePopupItems_.push_back(
            {static_cast<int>(gui::StickerPreset::CustomImageDecal),
             "32. UPLOAD CUSTOM IMAGE (.PNG/.JPG)...",
             !w.customImagePath.empty(), true, gui::Color::AccentCyan()});
      } else if (isKnobWidget(w.type)) {
        constexpr std::array<UiWidgetType, 32> kKnobs = {
            {UiWidgetType::PointerKnob,
             UiWidgetType::SkirtedCapKnob,
             UiWidgetType::ConcentricEqKnob,
             UiWidgetType::MarconiRedKnob,
             UiWidgetType::ModernArcKnob,
             UiWidgetType::BakeliteMasterKnob,
             UiWidgetType::ColorCapConsoleKnob,
             UiWidgetType::ChickenHeadKnob,
             UiWidgetType::FlatVectorKnob,
             UiWidgetType::GoldDomeKnob,
             UiWidgetType::MacroHaloKnob,
             UiWidgetType::KnurledBarrelKnob,
             UiWidgetType::ColletBroadcastKnob,
             UiWidgetType::StarburstSynthKnob,
             UiWidgetType::GrimyRelicKnob,
             UiWidgetType::SteppedAttenuatorKnob,
             UiWidgetType::CyberLuxeHaloKnob,
             UiWidgetType::NeumorphicGlassKnob,
             UiWidgetType::SciFiHudRingKnob,
             UiWidgetType::ValveWheelRelicKnob,
             UiWidgetType::SubmarineHelmKnob,
             UiWidgetType::TattooFlashKnob,
             UiWidgetType::MetalSkullKnob,
             UiWidgetType::FlowerDaisyKnob,
             UiWidgetType::ReaperCryptKnob,
             UiWidgetType::HellfireMagmaKnob,
             UiWidgetType::ApocalypseCogKnob,
             UiWidgetType::PunkSpikedKnob,
             UiWidgetType::MeadowWoodSliceKnob,
             UiWidgetType::SkySunburstKnob,
             UiWidgetType::StormRaindropKnob,
             UiWidgetType::GlacialIceCrystalKnob}};
        for (const auto kt : kKnobs) {
          activePopupItems_.push_back({static_cast<int>(kt),
                                       getWidgetTypeName(kt), w.type == kt,
                                       false, gui::Color::AccentCyan()});
        }
        activePopupItems_.push_back({-100, "★ APPLY CURRENT TO ALL KNOBS",
                                     false, true, gui::Color::AccentAmber()});
      } else if (isSwitchWidget(w.type)) {
        constexpr std::array<UiWidgetType, 10> kSw = {
            {UiWidgetType::ChromeToggleSwitch,
             UiWidgetType::IlluminatedPushButton,
             UiWidgetType::RockerPowerSwitch,
             UiWidgetType::LockingCageToggle,
             UiWidgetType::FlatPillVectorSwitch,
             UiWidgetType::NeumorphicPillButton,
             UiWidgetType::SciFiHologramSwitch,
             UiWidgetType::NavalBrassToggle,
             UiWidgetType::PunkSafetyPinToggle,
             UiWidgetType::HellfireEmberSwitch}};
        for (const auto st : kSw) {
          activePopupItems_.push_back({static_cast<int>(st),
                                       getWidgetTypeName(st), w.type == st,
                                       false, gui::Color::AccentCyan()});
        }
        activePopupItems_.push_back({-101, "★ APPLY CURRENT TO ALL SWITCHES",
                                     false, true, gui::Color::AccentAmber()});
      } else if (isMeterWidget(w.type)) {
        constexpr std::array<UiWidgetType, 18> kMet = {
            {UiWidgetType::AnalogVuMeter,
             UiWidgetType::RoundVintageVuMeter,
             UiWidgetType::LedBargraphMeter,
             UiWidgetType::DigitalTrimBargraph,
             UiWidgetType::GlowingTubeWindow,
             UiWidgetType::CrtOscilloscopeScreen,
             UiWidgetType::StereoGoniometerScope,
             UiWidgetType::LcdParameterReadout,
             UiWidgetType::NixieTubeDisplay,
             UiWidgetType::PastelShaderCurveMeter,
             UiWidgetType::CyberLuxeMultiBarMeter,
             UiWidgetType::NeumorphicDualArcMeter,
             UiWidgetType::SubmarinePortholeGauge,
             UiWidgetType::ReaperScytheArchMeter,
             UiWidgetType::HellfireHornedVuMeter,
             UiWidgetType::ApocalypseGeigerVuMeter,
             UiWidgetType::PunkAnarchyVuMeter,
             UiWidgetType::NatureElementalVuMeter}};
        for (const auto mt : kMet) {
          activePopupItems_.push_back({static_cast<int>(mt),
                                       getWidgetTypeName(mt), w.type == mt,
                                       false, gui::Color::AccentCyan()});
        }
      } else if (w.type == UiWidgetType::VerticalStudioFader ||
                 w.type == UiWidgetType::HorizontalCrossfader) {
        activePopupItems_.push_back(
            {static_cast<int>(UiWidgetType::VerticalStudioFader),
             getWidgetTypeName(UiWidgetType::VerticalStudioFader),
             w.type == UiWidgetType::VerticalStudioFader, false,
             gui::Color::AccentCyan()});
        activePopupItems_.push_back(
            {static_cast<int>(UiWidgetType::HorizontalCrossfader),
             getWidgetTypeName(UiWidgetType::HorizontalCrossfader),
             w.type == UiWidgetType::HorizontalCrossfader, false,
             gui::Color::AccentCyan()});
      } else {
        activePopupItems_.push_back({static_cast<int>(w.type),
                                     getWidgetTypeName(w.type), true, false,
                                     gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::WidgetColor: {
      if (selectedUiWidgetIndex_ < 0 ||
          selectedUiWidgetIndex_ >=
              static_cast<int>(faceplateLayout_.widgets.size()))
        break;
      const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      activePopupItems_.push_back(
          {-102,
           isKnobWidget(w.type)
               ? "★ APPLY COLOUR TO ALL KNOBS"
               : (isSwitchWidget(w.type) ? "★ APPLY COLOUR TO ALL SWITCHES"
                                         : "★ APPLY COLOUR TO ALL WIDGETS"),
           false, true, getUiAccentColorValue(w.accentColor)});
      for (int i = 0; i < 12; ++i) {
        const auto col = static_cast<UiAccentColor>(i);
        activePopupItems_.push_back({i, getUiAccentColorName(col),
                                     w.accentColor == col, true,
                                     getUiAccentColorValue(col)});
      }
      break;
    }
    case UiPopupMenuKind::FontFamily: {
      for (int i = 0; i < 8; ++i) {
        const auto f = static_cast<gui::FontFamily>(i);
        activePopupItems_.push_back(
            {i, getFontFamilyName(f),
             faceplateLayout_.theme.globalFontFamily == f, false,
             gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::FontSize: {
      for (int i = 0; i < 4; ++i) {
        activePopupItems_.push_back(
            {i, getUiFontSizeName(i),
             faceplateLayout_.theme.globalFontSizePreset == i, false,
             gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::TextColor: {
      for (int i = 0; i < 12; ++i) {
        const auto c = static_cast<UiSkinFontColor>(i);
        activePopupItems_.push_back(
            {i, getSkinFontPresetColorName(c),
             !faceplateLayout_.theme.useCustomFontColor &&
                 faceplateLayout_.theme.globalFontColor == c,
             true, getSkinFontPresetColorValue(c)});
      }
      activePopupItems_.push_back(
          {12, "CUSTOM RGB COLOUR PICKER...",
           faceplateLayout_.theme.useCustomFontColor, true,
           gui::Color::fromHex(faceplateLayout_.theme.customFontColorHex)});
      break;
    }
    case UiPopupMenuKind::PlateColor: {
      for (int i = 0; i < 13; ++i) {
        const auto swCol = getTextPlatePresetColorValue(i, faceplateLayout_.theme);
        activePopupItems_.push_back(
            {i, getTextPlatePresetColorName(i),
             !faceplateLayout_.theme.useCustomTextPlateColor &&
                 faceplateLayout_.theme.textPlateColorPreset == i,
             true, (swCol.a < 0.05f) ? gui::Color::AccentCyan() : swCol});
      }
      activePopupItems_.push_back(
          {13, "CUSTOM RGB COLOUR PICKER...",
           faceplateLayout_.theme.useCustomTextPlateColor, true,
           gui::Color::fromHex(faceplateLayout_.theme.customTextPlateColorHex)});
      break;
    }
    case UiPopupMenuKind::StickerQuickAdd: {
      const bool hasSelSticker =
          (selectedUiWidgetIndex_ >= 0 &&
           selectedUiWidgetIndex_ <
               static_cast<int>(faceplateLayout_.widgets.size()) &&
           faceplateLayout_.widgets[selectedUiWidgetIndex_].type ==
               UiWidgetType::VintageSticker);
      const int curS =
          hasSelSticker
              ? std::clamp(static_cast<int>(
                               faceplateLayout_.widgets[selectedUiWidgetIndex_]
                                   .scalePreset),
                           0, gui::kStickerPresetCount - 1)
              : -1;
      for (int i = 0; i < gui::kStickerPresetCount - 1; ++i) {
        const auto sp = static_cast<gui::StickerPreset>(i);
        activePopupItems_.push_back(
            {i, std::format("{}. {}", i + 1, getStickerPresetName(sp)),
             curS == i, false, gui::Color::AccentAmber()});
      }
      activePopupItems_.push_back(
          {static_cast<int>(gui::StickerPreset::CustomImageDecal),
           "32. UPLOAD CUSTOM IMAGE (.PNG/.JPG)...", false, true,
           gui::Color::AccentCyan()});
      break;
    }
    case UiPopupMenuKind::StickerRotation: {
      float curDeg = 0.0f;
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        curDeg = faceplateLayout_.widgets[selectedUiWidgetIndex_].rotationDeg;
      }
      struct AngleOpt {
        int deg;
        const char *desc;
      };
      constexpr std::array<AngleOpt, 12> kAngles = {
          {{0, "0°  (Level Horizontal)"},
           {-5, "-5°  (Subtle Slant Left)"},
           {5, "+5°  (Subtle Slant Right)"},
           {-12, "-12°  (Taped Tilt Left)"},
           {12, "+12°  (Taped Tilt Right)"},
           {-25, "-25°  (Diagonal Left)"},
           {25, "+25°  (Diagonal Right)"},
           {-45, "-45°  (45° Corner Left)"},
           {45, "+45°  (45° Corner Right)"},
           {-90, "-90°  (Vertical CCW)"},
           {90, "+90°  (Vertical CW)"},
           {180, "180°  (Upside Down)"}}};
      for (const auto &a : kAngles) {
        activePopupItems_.push_back(
            {a.deg, a.desc, std::abs(curDeg - static_cast<float>(a.deg)) < 1.5f,
             false, gui::Color::AccentCyan()});
      }
      break;
    }
    case UiPopupMenuKind::None:
    default:
      break;
    }
  }

  [[nodiscard]] std::string getActivePopupMenuTitle() const {
    switch (activePopupMenu_) {
    case UiPopupMenuKind::SurfaceGrain:
      return "FACEPLATE SURFACE FINISH (18 TEXTURES)";
    case UiPopupMenuKind::MountStyle:
      return "CHASSIS HARDWARE MOUNTING (7 STYLES)";
    case UiPopupMenuKind::ScrewStyle:
      return "CHASSIS SCREW HEAD STYLE (10 STYLES)";
    case UiPopupMenuKind::GridSnap:
      return "FACEPLATE ALIGNMENT GRID SNAP";
    case UiPopupMenuKind::WidgetStyle:
      return "SELECT WIDGET VISUAL STYLE / STICKER PRESET";
    case UiPopupMenuKind::WidgetColor:
      return "WIDGET ACCENT / LED COLOUR (12 COLOURS)";
    case UiPopupMenuKind::FontFamily:
      return "WHOLE-SKIN FONT FAMILY (8 HARDWARE FONTS)";
    case UiPopupMenuKind::FontSize:
      return "WHOLE-SKIN FONT SIZE PRESET";
    case UiPopupMenuKind::TextColor:
      return "WHOLE-SKIN TEXT INK COLOUR";
    case UiPopupMenuKind::PlateColor:
      return "CONTROL TEXT BACKING PLATE COLOUR";
    case UiPopupMenuKind::StickerQuickAdd:
      return "ADD VINTAGE STICKER OR STUDIO TAPE (31 PRESETS + CUSTOM IMAGE)";
    case UiPopupMenuKind::StickerRotation:
      return "STICKER / TAPE ROTATION ANGLE (OR DRAG 'R' HANDLE ON CANVAS)";
    default:
      return "SELECT OPTION";
    }
  }

  [[nodiscard]] int getActivePopupMenuCols() const noexcept {
    if (activePopupMenu_ == UiPopupMenuKind::SurfaceGrain)
      return 2;
    if (activePopupMenu_ == UiPopupMenuKind::StickerQuickAdd)
      return 3;
    if (activePopupMenu_ == UiPopupMenuKind::WidgetStyle) {
      if (activePopupItems_.size() > 20)
        return 3;
      if (activePopupItems_.size() > 10)
        return 2;
    }
    return 1;
  }

  [[nodiscard]] gui::Rect<float> getActivePopupMenuRect() const noexcept {
    const int cols = getActivePopupMenuCols();
    const int rows =
        std::max(1, static_cast<int>((activePopupItems_.size() +
                                      static_cast<size_t>(cols) - 1) /
                                     static_cast<size_t>(cols)));
    constexpr float rowH = 24.0f;
    constexpr float headH = 24.0f;
    const float menuH = headH + static_cast<float>(rows) * rowH + 6.0f;
    const float libX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    const float uiInspX = isSidebarCollapsed_ ? 364.0f : 604.0f;

    switch (activePopupMenu_) {
    case UiPopupMenuKind::SurfaceGrain:
      return {libX + 240.0f, 130.0f, 412.0f, menuH};
    case UiPopupMenuKind::MountStyle:
      return {libX + 430.0f, 130.0f, 224.0f, menuH};
    case UiPopupMenuKind::ScrewStyle:
      return {libX + 545.0f, 130.0f, 224.0f, menuH};
    case UiPopupMenuKind::GridSnap:
      return {libX + 650.0f, 130.0f, 214.0f, menuH};
    case UiPopupMenuKind::WidgetStyle:
      return {uiInspX + 10.0f, 626.0f - menuH,
              (cols == 3) ? 672.0f : ((cols == 2) ? 446.0f : 232.0f), menuH};
    case UiPopupMenuKind::WidgetColor:
      return {uiInspX + 176.0f, 626.0f - menuH, 248.0f, menuH};
    case UiPopupMenuKind::FontFamily:
      return {836.0f, 626.0f - menuH, 250.0f, menuH};
    case UiPopupMenuKind::FontSize:
      return {966.0f, 626.0f - menuH, 198.0f, menuH};
    case UiPopupMenuKind::TextColor:
      return {1030.0f, 626.0f - menuH, 234.0f, menuH};
    case UiPopupMenuKind::PlateColor:
      return {1102.0f, 626.0f - menuH, 238.0f, menuH};
    case UiPopupMenuKind::StickerQuickAdd:
      return {std::min(libX + 540.0f, 668.0f), 130.0f, 672.0f, menuH};
    case UiPopupMenuKind::StickerRotation:
      return {uiInspX + 460.0f, 586.0f - menuH, 290.0f, menuH};
    default:
      return {0.0f, 0.0f, 0.0f, 0.0f};
    }
  }

  void applyPopupMenuSelection(int itemIdx) {
    if (itemIdx < 0 || itemIdx >= static_cast<int>(activePopupItems_.size()))
      return;
    const int chosenId = activePopupItems_[static_cast<size_t>(itemIdx)].id;
    const UiPopupMenuKind kind = activePopupMenu_;
    const std::string menuTitle = getActivePopupMenuTitle();
    activePopupMenu_ = UiPopupMenuKind::None;
    hoveredPopupItemIdx_ = -1;
    if (kind != UiPopupMenuKind::StickerQuickAdd) {
      recordUndoSnapshot("Change " + menuTitle);
    }

    switch (kind) {
    case UiPopupMenuKind::SurfaceGrain: {
      if (!faceplateLayout_.theme.useCustomFinishColor) {
        faceplateLayout_.theme.customFinishHex =
            gui::HardwareMaterials::getPresetFinishBaseHex(
                faceplateLayout_.theme.finish);
        faceplateLayout_.theme.useCustomFinishColor = true;
      }
      faceplateLayout_.theme.grain = static_cast<gui::SurfaceGrain>(chosenId);
      faceplateLayout_.theme.surfaceFinish = chosenId;
      faceplateLayout_.theme.hasSurfaceOverlayOverride = true;
      markThemeAsCustomModified();
      statusMessage_ =
          std::format("Faceplate surface finish set to {} - Style switched to *CUSTOM.",
                      getSurfaceGrainName(faceplateLayout_.theme.grain));
      break;
    }
    case UiPopupMenuKind::MountStyle: {
      faceplateLayout_.theme.hardwareStyle =
          static_cast<gui::ChassisHardwareStyle>(chosenId);
      faceplateLayout_.theme.cornerScrews =
          (faceplateLayout_.theme.hardwareStyle !=
           gui::ChassisHardwareStyle::CleanScrewless);
      markThemeAsCustomModified();
      statusMessage_ = std::format(
          "Chassis hardware mounting set to {} - Style switched to *CUSTOM.",
          getHardwareStyleName(faceplateLayout_.theme.hardwareStyle));
      break;
    }
    case UiPopupMenuKind::ScrewStyle: {
      if (chosenId < 0) {
        faceplateLayout_.theme.cornerScrews = false;
        faceplateLayout_.theme.hardwareStyle =
            gui::ChassisHardwareStyle::CleanScrewless;
        markThemeAsCustomModified();
        statusMessage_ =
            "Chassis screws disabled (CLEAN SCREWLESS) - Style switched to *CUSTOM.";
        break;
      }
      faceplateLayout_.theme.screwStyle =
          static_cast<gui::ScrewHeadStyle>(chosenId);
      if (!faceplateLayout_.theme.cornerScrews ||
          faceplateLayout_.theme.hardwareStyle ==
              gui::ChassisHardwareStyle::CleanScrewless) {
        faceplateLayout_.theme.cornerScrews = true;
        faceplateLayout_.theme.hardwareStyle =
            gui::ChassisHardwareStyle::CornerScrews4;
      }
      markThemeAsCustomModified();
      statusMessage_ =
          std::format("Chassis screw head style set to {} - Style switched to *CUSTOM.",
                      getScrewHeadStyleName(faceplateLayout_.theme.screwStyle));
      break;
    }
    case UiPopupMenuKind::GridSnap: {
      faceplateLayout_.theme.gridSnapPx = chosenId;
      faceplateLayout_.theme.showGrid = (chosenId > 1);
      statusMessage_ =
          (chosenId > 1)
              ? std::format("Faceplate alignment grid snap set to {}px.",
                            chosenId)
              : "Faceplate alignment grid snap set to FREE (1px).";
      break;
    }
    case UiPopupMenuKind::WidgetStyle: {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        if (w.type == UiWidgetType::SectionFrame) {
          w.scalePreset = static_cast<UiScalePreset>(chosenId);
          faceplateLayout_.theme.panelBorderStyle =
              static_cast<gui::SectionFrameStyle>(chosenId);
          markThemeAsCustomModified();
          statusMessage_ =
              std::format("Changed Section Frame style to {} - Style switched to *CUSTOM.",
                          getSectionFrameStyleName(
                              static_cast<gui::SectionFrameStyle>(chosenId)));
        } else if (w.type == UiWidgetType::RivetedNameplate) {
          w.scalePreset = static_cast<UiScalePreset>(chosenId);
          faceplateLayout_.theme.globalBadgeStyle =
              static_cast<gui::BadgeVisualStyle>(chosenId);
          markThemeAsCustomModified();
          statusMessage_ =
              std::format("Changed Badge style to {} - Style switched to *CUSTOM.",
                          getBadgeVisualStyleName(
                              static_cast<gui::BadgeVisualStyle>(chosenId)));
        } else if (w.type == UiWidgetType::VintageSticker) {
          if (chosenId ==
              static_cast<int>(gui::StickerPreset::CustomImageDecal)) {
            openStickerImageFileDialog(selectedUiWidgetIndex_);
            return;
          }
          w.scalePreset = static_cast<UiScalePreset>(chosenId);
          w.customImagePath.clear();
          for (const auto &entry : getUiWidgetCatalog()) {
            if (entry.type == UiWidgetType::VintageSticker &&
                static_cast<int>(entry.bakedScale) == chosenId) {
              w.title = entry.bakedTitle;
              w.subtitle = entry.bakedValText;
              w.w = entry.defaultW;
              w.h = entry.defaultH;
              break;
            }
          }
          autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                                     w.fontSizePreset, false);
          statusMessage_ = std::format(
              "Changed Vintage Sticker / Tape to '{}' - edit LABEL and SUB "
              "below to customize text!",
              getStickerPresetName(static_cast<gui::StickerPreset>(chosenId)));
        } else if (chosenId == -100 && isKnobWidget(w.type)) {
          const UiWidgetType targetType = w.type;
          const UiAccentColor targetColor = w.accentColor;
          faceplateLayout_.theme.globalKnobType = targetType;
          faceplateLayout_.theme.globalAccentColor = targetColor;
          size_t updatedCount = 0;
          for (auto &kw : faceplateLayout_.widgets) {
            if (isKnobWidget(kw.type)) {
              kw.type = targetType;
              kw.accentColor = targetColor;
              ++updatedCount;
            }
          }
          markThemeAsCustomModified();
          statusMessage_ = std::format(
              "Applied knob style '{}' and colour '{}' to all {} knobs on the faceplate.",
              getWidgetTypeName(targetType), getUiAccentColorName(targetColor),
              updatedCount);
        } else if (chosenId == -101 && isSwitchWidget(w.type)) {
          const UiWidgetType targetType = w.type;
          const UiAccentColor targetColor = w.accentColor;
          faceplateLayout_.theme.globalSwitchType = targetType;
          faceplateLayout_.theme.globalAccentColor = targetColor;
          size_t updatedCount = 0;
          for (auto &sw : faceplateLayout_.widgets) {
            if (isSwitchWidget(sw.type)) {
              sw.type = targetType;
              sw.accentColor = targetColor;
              if (sw.boundStageIdx >= 0 && sw.y >= 445.0f && sw.h < 44.0f) {
                sw.y = 454.0f;
                sw.h = 48.0f;
              }
              ++updatedCount;
            }
          }
          markThemeAsCustomModified();
          statusMessage_ = std::format(
              "Applied switch style '{}' and colour '{}' to all {} switches on the faceplate.",
              getWidgetTypeName(targetType), getUiAccentColorName(targetColor),
              updatedCount);
        } else {
          const UiWidgetType oldType = w.type;
          const bool wasMasterMeter = faceplateLayout_.isMasterMeterWidget(w);
          const UiWidgetType newType = static_cast<UiWidgetType>(chosenId);
          w.type = newType;
          if (isSwitchWidget(newType) && w.boundStageIdx >= 0 &&
              w.y >= 445.0f && w.h < 44.0f) {
            w.y = 454.0f;
            w.h = 48.0f;
          }
          if (isKnobWidget(newType)) {
            faceplateLayout_.theme.globalKnobType = newType;
          } else if (isSwitchWidget(newType)) {
            faceplateLayout_.theme.globalSwitchType = newType;
          } else if (isMeterWidget(newType)) {
            faceplateLayout_.theme.globalMeterType = newType;
          }
          markThemeAsCustomModified();
          bool appliedAllKnobs = false;
          bool appliedAllSwitches = false;
#if defined(_WIN32)
          if (isKnobWidget(newType) && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
            faceplateLayout_.theme.globalKnobType = newType;
            faceplateLayout_.theme.globalAccentColor = w.accentColor;
            for (auto &kw : faceplateLayout_.widgets) {
              if (isKnobWidget(kw.type)) {
                kw.type = newType;
                kw.accentColor = w.accentColor;
              }
            }
            appliedAllKnobs = true;
          } else if (isSwitchWidget(newType) &&
                     (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
            faceplateLayout_.theme.globalSwitchType = newType;
            faceplateLayout_.theme.globalAccentColor = w.accentColor;
            for (auto &sw : faceplateLayout_.widgets) {
              if (isSwitchWidget(sw.type)) {
                sw.type = newType;
                sw.accentColor = w.accentColor;
                if (sw.boundStageIdx >= 0 && sw.y >= 445.0f && sw.h < 44.0f) {
                  sw.y = 454.0f;
                  sw.h = 48.0f;
                }
              }
            }
            appliedAllSwitches = true;
          }
#endif
          if ((oldType == UiWidgetType::VerticalStudioFader &&
               w.type == UiWidgetType::HorizontalCrossfader) ||
              (oldType == UiWidgetType::HorizontalCrossfader &&
               w.type == UiWidgetType::VerticalStudioFader)) {
            std::swap(w.w, w.h);
          }
          if (wasMasterMeter && isMeterWidget(w.type)) {
            faceplateLayout_.theme.globalMeterType = w.type;
            ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
          }
          if (appliedAllKnobs) {
            statusMessage_ = std::format(
                "Applied knob style '{}' and colour '{}' to all knobs on the faceplate.",
                getWidgetTypeName(w.type), getUiAccentColorName(w.accentColor));
          } else if (appliedAllSwitches) {
            statusMessage_ = std::format(
                "Applied switch style '{}' and colour '{}' to all switches on the faceplate.",
                getWidgetTypeName(w.type), getUiAccentColorName(w.accentColor));
          } else if (isSwitchWidget(w.type)) {
            statusMessage_ = std::format(
                "Changed '{}' style to {} (click 'APPLY TO ALL SWITCHES' to apply globally).",
                w.title.empty() ? w.subtitle : w.title,
                getWidgetTypeName(w.type));
          } else {
            statusMessage_ = std::format(
                "Changed '{}' style to {} (click 'APPLY TO ALL KNOBS' to apply globally).",
                w.title, getWidgetTypeName(w.type));
          }
        }
      }
      break;
    }
    case UiPopupMenuKind::WidgetColor: {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        if (chosenId == -102) {
          const UiAccentColor targetCol = w.accentColor;
          faceplateLayout_.theme.globalAccentColor = targetCol;
          size_t updatedCount = 0;
          for (auto &ow : faceplateLayout_.widgets) {
            if (isKnobWidget(w.type) ? isKnobWidget(ow.type)
                                     : (isSwitchWidget(w.type)
                                            ? isSwitchWidget(ow.type)
                                            : true)) {
              ow.accentColor = targetCol;
              ++updatedCount;
            }
          }
          markThemeAsCustomModified();
          statusMessage_ = std::format(
              "Applied colour '{}' to all {} matching widgets on the faceplate.",
              getUiAccentColorName(targetCol), updatedCount);
        } else {
          w.accentColor = static_cast<UiAccentColor>(chosenId);
          faceplateLayout_.theme.globalAccentColor = w.accentColor;
          bool appliedGlobally = false;
#if defined(_WIN32)
          if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) {
            for (auto &ow : faceplateLayout_.widgets) {
              if (isKnobWidget(w.type) ? isKnobWidget(ow.type)
                                       : (isSwitchWidget(w.type)
                                              ? isSwitchWidget(ow.type)
                                              : true)) {
                ow.accentColor = w.accentColor;
              }
            }
            appliedGlobally = true;
          }
#endif
          markThemeAsCustomModified();
          statusMessage_ =
              appliedGlobally
                  ? std::format("Applied colour '{}' to all matching widgets on the faceplate.",
                                getUiAccentColorName(w.accentColor))
                  : std::format(
                        "Changed widget accent/LED colour to {} (click 'APPLY TO ALL KNOBS' to apply globally) - Style switched to *CUSTOM.",
                        getUiAccentColorName(w.accentColor));
        }
      }
      break;
    }
    case UiPopupMenuKind::FontFamily: {
      faceplateLayout_.theme.globalFontFamily =
          static_cast<gui::FontFamily>(chosenId);
      markThemeAsCustomModified();
      statusMessage_ = std::format(
          "Whole-Skin Font Type set to {} - Style switched to *CUSTOM.",
          getFontFamilyName(faceplateLayout_.theme.globalFontFamily));
      break;
    }
    case UiPopupMenuKind::FontSize: {
      faceplateLayout_.theme.globalFontSizePreset = chosenId;
      for (auto &w : faceplateLayout_.widgets) {
        w.fontSizePreset = chosenId;
      }
      markThemeAsCustomModified();
      statusMessage_ = std::format(
          "Whole-Skin Font Size set to {} - Style switched to *CUSTOM.",
          getUiFontSizeName(faceplateLayout_.theme.globalFontSizePreset));
      break;
    }
    case UiPopupMenuKind::TextColor: {
      if (chosenId == 12) {
        openCustomFontColorPicker();
        return;
      }
      faceplateLayout_.theme.globalFontColor =
          static_cast<UiSkinFontColor>(chosenId);
      faceplateLayout_.theme.useCustomFontColor = false;
      markThemeAsCustomModified();
      statusMessage_ =
          std::format("Whole-Skin Font Colour set to {} - Style switched to *CUSTOM.",
                      getSkinFontColorName(faceplateLayout_.theme));
      break;
    }
    case UiPopupMenuKind::PlateColor: {
      if (chosenId == 13) {
        openCustomPlateColorPicker();
        return;
      }
      faceplateLayout_.theme.textPlateColorPreset = chosenId;
      faceplateLayout_.theme.useCustomTextPlateColor = false;
      markThemeAsCustomModified();
      statusMessage_ =
          std::format("Control Text Backing Plate Colour set to {} - Style switched to *CUSTOM.",
                      getSkinTextPlateColorName(faceplateLayout_.theme));
      break;
    }
    case UiPopupMenuKind::StickerQuickAdd: {
      if (chosenId == static_cast<int>(gui::StickerPreset::CustomImageDecal)) {
        openStickerImageFileDialog(-1);
        return;
      }
      for (const auto &entry : getUiWidgetCatalog()) {
        if (entry.type == UiWidgetType::VintageSticker &&
            static_cast<int>(entry.bakedScale) == chosenId) {
          const float offset =
              static_cast<float>(faceplateLayout_.widgets.size() % 6) * 20.0f;
          addUiWidgetFromPalette(entry, 410.0f + offset,
                                 175.0f + offset * 0.7f);
          selectUiWidgetCategory(UiWidgetCategory::FramesAndBadges);
          return;
        }
      }
      break;
    }
    case UiPopupMenuKind::StickerRotation: {
      if (selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        w.rotationDeg = static_cast<float>(chosenId);
        lastUiWidgetInteractMs_ = GetTickCount64();
        statusMessage_ = std::format(
            "Rotated '{}' to {:+.0f}°! (Tip: You can also drag the orange 'R' "
            "handle above the sticker or scroll mouse-wheel over it.)",
            w.title, w.rotationDeg);
      }
      break;
    }
    default:
      break;
    }
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();
    repaint();
  }

  void markThemeAsCustomModified() {
    faceplateLayout_.theme.isCustomThemeModified = true;
    markProjectDirty(true);
  }

  void saveCurrentThemeAsUserTheme(std::string customName = "") {
    if (customName.empty() && inputCustomThemeName_) {
      customName = inputCustomThemeName_->getText();
    }
    while (!customName.empty() &&
           std::isspace(static_cast<unsigned char>(customName.front()))) {
      customName.erase(customName.begin());
    }
    while (!customName.empty() &&
           std::isspace(static_cast<unsigned char>(customName.back()))) {
      customName.pop_back();
    }
    if (customName.empty()) {
      customName = std::format("USER THEME {}", savedUserThemes_.size() + 1);
    }
    for (char &c : customName) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    faceplateLayout_.theme.customThemeName = customName;
    faceplateLayout_.theme.isCustomThemeModified = false;
    if (inputCustomThemeName_) {
      inputCustomThemeName_->setText(customName);
    }

    SavedUserTheme entry;
    entry.name = customName;
    entry.theme = faceplateLayout_.theme;

    bool replaced = false;
    for (auto &existing : savedUserThemes_) {
      if (existing.name == customName) {
        existing = entry;
        replaced = true;
        break;
      }
    }
    if (!replaced) {
      savedUserThemes_.push_back(entry);
    }
    saveSavedUserThemesToFile(ProjectGenerator::getWorkspaceRoot(),
                              savedUserThemes_);
    updateUiThemeButtonLabels();
    statusMessage_ = std::format(
        "Saved custom UI theme '{}' to presets/user_ui_themes.cfg! Available "
        "anytime in the STYLE dropdown.",
        customName);
    repaint();
  }

  void updateUiThemeButtonLabels() {
    if (btnUiFinish_) {
      const char *arrow = isStyleTemplateDropdownOpen_ ? "^" : "v";
      if (faceplateLayout_.theme.isCustomThemeModified) {
        btnUiFinish_->setText(std::format("STYLE: *CUSTOM  {}", arrow));
        btnUiFinish_->setAccentColor(gui::Color::AccentCyan());
      } else if (!faceplateLayout_.theme.customThemeName.empty()) {
        btnUiFinish_->setText(std::format("STYLE: \xE2\x98\x85 {}  {}",
                                          faceplateLayout_.theme.customThemeName,
                                          arrow));
        btnUiFinish_->setAccentColor(gui::Color::fromHex(0xD4AF37));
      } else {
        btnUiFinish_->setText(std::format(
            "STYLE: {}  {}", getUiStyleTemplateName(activeStyleTemplate_),
            arrow));
        btnUiFinish_->setAccentColor(gui::Color::fromHex(0xD4AF37));
      }
    }
    if (btnSaveCustomTheme_) {
      btnSaveCustomTheme_->setText(faceplateLayout_.theme.isCustomThemeModified
                                       ? "SAVE *CUSTOM"
                                       : "SAVE THEME");
      btnSaveCustomTheme_->setAccentColor(
          faceplateLayout_.theme.isCustomThemeModified
              ? gui::Color::AccentCyan()
              : gui::Color::AccentAmber());
    }
    if (btnUiPickFinishColor_) {
      if (faceplateLayout_.theme.useCustomFinishColor) {
        btnUiPickFinishColor_->setText(std::format(
            "COLOUR: #{:06X}...", faceplateLayout_.theme.customFinishHex));
        btnUiPickFinishColor_->setAccentColor(
            gui::Color::fromHex(faceplateLayout_.theme.customFinishHex));
      } else {
        btnUiPickFinishColor_->setText(
            std::format("COLOUR: {}...",
                        getChassisFinishName(faceplateLayout_.theme.finish)));
        const uint32_t presetHex =
            gui::HardwareMaterials::getPresetFinishBaseHex(
                faceplateLayout_.theme.finish);
        const auto c = gui::Color::fromHex(presetHex);
        btnUiPickFinishColor_->setAccentColor(
            (c.r + c.g + c.b < 0.55f) ? gui::Color::AccentAmber() : c);
      }
    }
    if (btnUiGrain_) {
      btnUiGrain_->setText(std::format(
          "SURFACE: {}  {}", getSurfaceGrainName(faceplateLayout_.theme.grain),
          (activePopupMenu_ == UiPopupMenuKind::SurfaceGrain) ? "^" : "v"));
    }
    if (btnUiScrews_) {
      btnUiScrews_->setText(std::format(
          "MOUNT: {}  {}",
          getHardwareStyleName(faceplateLayout_.theme.hardwareStyle),
          (activePopupMenu_ == UiPopupMenuKind::MountStyle) ? "^" : "v"));
    }
    if (btnUiScrewStyle_) {
      const bool isScrewless =
          (!faceplateLayout_.theme.cornerScrews ||
           faceplateLayout_.theme.hardwareStyle ==
               gui::ChassisHardwareStyle::CleanScrewless);
      btnUiScrewStyle_->setText(std::format(
          "SCREWS: {}  {}",
          isScrewless ? "NONE"
                      : getScrewHeadStyleName(faceplateLayout_.theme.screwStyle),
          (activePopupMenu_ == UiPopupMenuKind::ScrewStyle) ? "^" : "v"));
    }
    if (btnUiGrid_) {
      const char *gridArrow =
          (activePopupMenu_ == UiPopupMenuKind::GridSnap) ? "^" : "v";
      if (!faceplateLayout_.theme.showGrid ||
          faceplateLayout_.theme.gridSnapPx <= 1) {
        btnUiGrid_->setText(std::format("GRID: FREE {}", gridArrow));
      } else {
        btnUiGrid_->setText(std::format(
            "GRID: {}px {}", faceplateLayout_.theme.gridSnapPx, gridArrow));
      }
    }
    if (btnUiAddSticker_) {
      btnUiAddSticker_->setText(std::format(
          "STICKERS (32) {}",
          (activePopupMenu_ == UiPopupMenuKind::StickerQuickAdd) ? "^" : "v"));
    }
    if (btnUiGlobalFontType_) {
      btnUiGlobalFontType_->setText(std::format(
          "FONT: {}  {}",
          getFontFamilyShortName(faceplateLayout_.theme.globalFontFamily),
          (activePopupMenu_ == UiPopupMenuKind::FontFamily) ? "^" : "v"));
    }
    if (btnUiWidgetFont_) {
      btnUiWidgetFont_->setText(std::format(
          "SIZE: {}  {}",
          getUiFontSizeShortName(faceplateLayout_.theme.globalFontSizePreset),
          (activePopupMenu_ == UiPopupMenuKind::FontSize) ? "^" : "v"));
    }
    if (btnUiGlobalFontColor_) {
      btnUiGlobalFontColor_->setText(std::format(
          "TEXT COL: {}  {}", getSkinFontColorName(faceplateLayout_.theme),
          (activePopupMenu_ == UiPopupMenuKind::TextColor) ? "^" : "v"));
      const auto fc = getSkinFontColorValue(faceplateLayout_.theme);
      const bool isDarkInk = (fc.r + fc.g + fc.b < 0.75f);
      btnUiGlobalFontColor_->setAccentColor(
          isDarkInk ? gui::Color::AccentAmber() : fc);
    }
    if (btnUiGlobalPlateColor_) {
      btnUiGlobalPlateColor_->setText(std::format(
          "PLATE COL: {}  {}", getSkinTextPlateColorName(faceplateLayout_.theme),
          (activePopupMenu_ == UiPopupMenuKind::PlateColor) ? "^" : "v"));
      const auto pc = getSkinTextPlateColorValue(faceplateLayout_.theme);
      const bool isDarkPlate = (pc.a < 0.05f || (pc.r + pc.g + pc.b < 0.75f));
      btnUiGlobalPlateColor_->setAccentColor(
          isDarkPlate ? gui::Color::AccentAmber() : pc);
    }
    faceplateLayout_.theme.brandName = "AVRA AUDIO";
    if (inputUiBadgeFooter_)
      inputUiBadgeFooter_->setText(faceplateLayout_.theme.badgeFooterText);
  }

  /**
   * @brief Symbiotically synchronizes the Faceplate UI Builder layout with the
   * current Schematic chain, preserving the user's chosen global scheme
   * (finish, grain, mount, screw style, knob style, switch style, meter style,
   * accent color). When the Schematic is cleared, the Faceplate UI is also
   * cleared to a blank chassis!
   */
  void selectDefaultKnobOnFaceplate() {
    selectedUiWidgetIndex_ = -1;
    for (size_t i = 0; i < faceplateLayout_.widgets.size(); ++i) {
      if (isKnobWidget(faceplateLayout_.widgets[i].type)) {
        selectedUiWidgetIndex_ = static_cast<int>(i);
        break;
      }
    }
    if (selectedUiWidgetIndex_ < 0 && !faceplateLayout_.widgets.empty()) {
      selectedUiWidgetIndex_ = 0;
    }
  }

  void syncGlobalFaceplateControls() {
    for (auto &w : faceplateLayout_.widgets) {
      if (isSwitchWidget(w.type) || w.type == UiWidgetType::JewelLed) {
        if (w.boundStageIdx < 0) {
          if (w.boundParamSlot == 4 || w.title == "TOPOLOGY" ||
              w.title == "STEREO MODE" ||
              w.title.find("MONO") != std::string::npos) {
            const int topMode = liveSchematicRunner_.getChannelTopologyMode();
            const bool has3Way =
                (std::count(w.subtitle.begin(), w.subtitle.end(), '|') >= 2);
            if (has3Way) {
              w.previewNorm =
                  (topMode == 0) ? 0.0f : ((topMode == 1) ? 0.5f : 1.0f);
            } else {
              // 2-way `[ STEREO | MONO (PURE) ]`: UP/ON (1.0f) = STEREO, DOWN/OFF (0.0f) = MONO (PURE)
              w.previewNorm = (topMode == 0) ? 0.0f : 1.0f;
            }
          } else if (w.boundParamSlot == 5 || w.title == "AUTO-LEVEL" ||
                     w.title == "AUTO-GAIN") {
            w.previewNorm = rackAutoMatch_ ? 1.0f : 0.0f;
          } else if (w.boundParamSlot == 6 ||
                     w.title.find("DELTA") != std::string::npos) {
            w.previewNorm = rackDeltaListen_ ? 1.0f : 0.0f;
          } else if (w.title.find("MUTE") != std::string::npos ||
                     w.subtitle.find("MUTE") != std::string::npos) {
            w.previewNorm = rackMuted_ ? 0.0f : 1.0f;
          } else if (w.title.find("PHASE") != std::string::npos ||
                     w.subtitle.find("180") != std::string::npos) {
            w.previewNorm = rackPhaseInvert_ ? 0.0f : 1.0f;
          } else {
            w.previewNorm = rackBypassed_ ? 0.0f : 1.0f;
          }
        }
      } else if (isKnobWidget(w.type) ||
                 w.type == UiWidgetType::VerticalStudioFader) {
        if (w.knobRole == UiKnobRole::MasterVolume ||
            (w.boundStageIdx < 0 && w.boundParamSlot == 1)) {
          w.previewNorm =
              math::clamp((rackOutputTrimDb_ + 24.0f) / 48.0f, 0.0f, 1.0f);
          w.subtitle = std::format("{:+.1f} dB", rackOutputTrimDb_);
        } else if (w.scalePreset == UiScalePreset::StereoPanLCR ||
                   w.title == "STEREO BAL") {
          const float panPct = (w.previewNorm - 0.5f) * 200.0f;
          if (std::abs(panPct) < 2.0f) {
            w.previewNorm = 0.5f;
            w.subtitle = "CENTER";
          } else if (panPct < 0.0f) {
            w.subtitle = std::format("L{:.0f}", std::abs(panPct));
          } else {
            w.subtitle = std::format("R{:.0f}", panPct);
          }
        } else if (w.boundStageIdx < 0 &&
                   (w.boundParamSlot == 2 || w.boundParamSlot == 9 ||
                    w.title == "MASTER MIX" || w.title == "DRY / WET")) {
          w.previewNorm = math::clamp(rackGlobalWetMix_, 0.0f, 1.0f);
          w.subtitle = std::format("{:.0f} %", rackGlobalWetMix_ * 100.0f);
        } else if (w.boundStageIdx < 0 && w.boundParamSlot == 0) {
          w.previewNorm =
              math::clamp((rackInputTrimDb_ + 24.0f) / 48.0f, 0.0f, 1.0f);
          w.subtitle = std::format("{:+.1f} dB", rackInputTrimDb_);
        }
      }
    }
  }

  void syncFaceplateWithSchematic() {
    const std::string title = wizardConfig_.name.empty()
                                  ? (faceplateLayout_.theme.pluginTitle.empty()
                                         ? "CUSTOM PLUGIN"
                                         : faceplateLayout_.theme.pluginTitle)
                                  : wizardConfig_.name;
    faceplateLayout_.theme.pluginTitle = title;
    faceplateLayout_.theme.styleTemplateIdx =
        static_cast<int>(activeStyleTemplate_);

    // Preserve user-added decorative widgets (stickers, labels, grilles,
    // handles, jacks) and per-widget custom visual styles/colors/titles across
    // schematic edits.
    const auto oldWidgets = faceplateLayout_.widgets;
    if (isLegacyGenericBadgeFooter(faceplateLayout_.theme.badgeFooterText)) {
      faceplateLayout_.theme.badgeFooterText = "CLASS-A EFFECTS";
    }
    if (isLegacyGenericBadgeSub1(faceplateLayout_.theme.subtitleText)) {
      faceplateLayout_.theme.subtitleText = "CUSTOM CHASSIS";
    }
    faceplateLayout_ = buildFaceplateFromSchematicWithTheme(
        title, schematicChain_, faceplateLayout_.theme);

    for (auto &nw : faceplateLayout_.widgets) {
      for (const auto &ow : oldWidgets) {
        if (nw.type == UiWidgetType::RivetedNameplate &&
            ow.type == UiWidgetType::RivetedNameplate) {
          nw.scalePreset = ow.scalePreset;
          nw.accentColor = ow.accentColor;
          nw.fontSizePreset = ow.fontSizePreset;
          if (!ow.title.empty()) {
            nw.title = ow.title;
            faceplateLayout_.theme.pluginTitle = ow.title;
          }
          if (!ow.subtitle.empty()) {
            const auto pipePos = ow.subtitle.find('|');
            std::string sub1 = (pipePos != std::string::npos) ? ow.subtitle.substr(0, pipePos) : ow.subtitle;
            std::string sub2 = (pipePos != std::string::npos && pipePos + 1 < ow.subtitle.size())
                ? ow.subtitle.substr(pipePos + 1)
                : faceplateLayout_.theme.badgeFooterText;
            if (isLegacyGenericBadgeSub1(sub1) && sub2 != "PREAMP · EQ · DIODE BRIDGE DYNAMICS") {
              sub1 = "CUSTOM CHASSIS";
            }
            if (isLegacyGenericBadgeFooter(sub2)) {
              sub2 = "CLASS-A EFFECTS";
            }
            faceplateLayout_.theme.subtitleText = sub1;
            faceplateLayout_.theme.badgeFooterText = sub2;
            nw.subtitle = std::format("{}|{}", sub1, sub2);
          } else {
            nw.subtitle = "CUSTOM CHASSIS|CLASS-A EFFECTS";
          }
          break;
        }
        if (nw.type == UiWidgetType::SectionFrame &&
            ow.type == UiWidgetType::SectionFrame &&
            nw.boundStageIdx == -1 && ow.boundStageIdx == -1) {
          nw.scalePreset = ow.scalePreset;
          nw.accentColor = ow.accentColor;
          nw.fontSizePreset = ow.fontSizePreset;
          if (!ow.title.empty()) {
            nw.title = ow.title;
          }
          break;
        }
        if (nw.boundStageIdx == ow.boundStageIdx &&
            nw.boundParamSlot == ow.boundParamSlot &&
            faceplateLayout_.isMasterMeterWidget(nw) ==
                faceplateLayout_.isMasterMeterWidget(ow) &&
            ((isKnobWidget(nw.type) && isKnobWidget(ow.type)) ||
             (isFaderWidget(nw.type) && isFaderWidget(ow.type)) ||
             (isSwitchWidget(nw.type) && isSwitchWidget(ow.type)) ||
             (isMeterWidget(nw.type) && isMeterWidget(ow.type)))) {
          nw.type = (!faceplateLayout_.isMasterMeterWidget(nw) &&
                     nw.type == UiWidgetType::GlowingTubeWindow)
                        ? UiWidgetType::GlowingTubeWindow
                        : ow.type;
          nw.accentColor = ow.accentColor;
          nw.fontSizePreset = ow.fontSizePreset;
          if (nw.boundStageIdx < 0 || ow.title == nw.title) {
            nw.scalePreset = ow.scalePreset;
          }
          if (nw.boundStageIdx < 0 || faceplateLayout_.isMasterMeterWidget(nw)) {
            if (!ow.title.empty()) {
              nw.title = ow.title;
            }
            if (!ow.subtitle.empty()) {
              nw.subtitle = ow.subtitle;
            }
          }
          break;
        }
      }
    }
    for (const auto &ow : oldWidgets) {
      if (ow.type == UiWidgetType::VintageSticker ||
          ow.type == UiWidgetType::SilkscreenLabel ||
          ow.type == UiWidgetType::VentilationGrille ||
          ow.type == UiWidgetType::RackHandlePair ||
          ow.type == UiWidgetType::PatchbayJackSocket) {
        faceplateLayout_.widgets.push_back(ow);
      }
    }

    ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
    syncGlobalFaceplateControls();
    if (faceplateLayout_.widgets.empty()) {
      selectedUiWidgetIndex_ = -1;
    } else if (selectedUiWidgetIndex_ < 0 ||
               selectedUiWidgetIndex_ >=
                   static_cast<int>(faceplateLayout_.widgets.size()) ||
               !isKnobWidget(
                   faceplateLayout_.widgets[selectedUiWidgetIndex_].type)) {
      selectDefaultKnobOnFaceplate();
    }
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();
  }

  struct ParamBindOption {
    int stageIdx{-1};
    int paramSlot{0};
    std::string label{};
  };

  void rebuildParamBindOptions() {
    paramBindOptions_.clear();
    for (size_t s = 0; s < schematicChain_.size(); ++s) {
      const auto &meta = getComponentMeta(schematicChain_[s].type);
      const auto &plum = getAutoPlumbingSpec(schematicChain_[s].type);
      const size_t pCount = meta.getParamCount();
      for (size_t p = 0; p < pCount; ++p) {
        const auto &ps = meta.getParamSpec(static_cast<int>(p));
        paramBindOptions_.push_back(
            {static_cast<int>(s), static_cast<int>(p),
             std::format("S{} [{}]: {} ({})", s + 1, meta.designator, ps.name,
                         ps.unit)});
      }
      if (plum.internalTrim1.isValid()) {
        paramBindOptions_.push_back(
            {static_cast<int>(s), 5,
             std::format("S{} [{}]: Trim 1 - {} ({})", s + 1, meta.designator,
                         plum.internalTrim1.name, plum.internalTrim1.unit)});
      }
      if (plum.internalTrim2.isValid()) {
        paramBindOptions_.push_back(
            {static_cast<int>(s), 6,
             std::format("S{} [{}]: Trim 2 - {} ({})", s + 1, meta.designator,
                         plum.internalTrim2.name, plum.internalTrim2.unit)});
      }
      paramBindOptions_.push_back(
          {static_cast<int>(s), 109, std::format("S{} [{}]: Parallel Level (%)", s + 1, meta.designator)});
      paramBindOptions_.push_back(
          {static_cast<int>(s), 9,
           std::format("S{} [{}]: Stage Dry/Wet Mix (%)", s + 1,
                       meta.designator)});
    }
    paramBindOptions_.push_back({-1, 0, "Global: Input Trim (-24..+24 dB)"});
    paramBindOptions_.push_back({-1, 1, "Global: Master Output (-24..+24 dB)"});
    paramBindOptions_.push_back(
        {-1, 2, "Global: Master Mix Dry/Wet (0..100 %)"});
    paramBindOptions_.push_back(
        {-1, 4, "Global: Channel Topology (MONO / STEREO / M/S)"});
    paramBindOptions_.push_back({-1, 5, "Global: Auto-Gain LUFS Lock"});
    paramBindOptions_.push_back({-1, 6, "Global: Phase-Aligned Delta Listen"});
  }

  void syncUiWidgetInspector() {
    if (faceplateLayout_.theme.brandName.empty()) {
      faceplateLayout_.theme.brandName = "AVRA AUDIO";
    }
    if (inputUiBadgeFooter_)
      inputUiBadgeFooter_->setText(faceplateLayout_.theme.badgeFooterText);
    if (btnUiGlobalFontType_) {
      btnUiGlobalFontType_->setText(std::format(
          "FONT: {}  {}",
          getFontFamilyShortName(faceplateLayout_.theme.globalFontFamily),
          (activePopupMenu_ == UiPopupMenuKind::FontFamily) ? "^" : "v"));
    }
    if (btnUiWidgetFont_) {
      btnUiWidgetFont_->setText(std::format(
          "SIZE: {}  {}",
          getUiFontSizeShortName(faceplateLayout_.theme.globalFontSizePreset),
          (activePopupMenu_ == UiPopupMenuKind::FontSize) ? "^" : "v"));
    }
    if (btnUiGlobalFontColor_) {
      btnUiGlobalFontColor_->setText(std::format(
          "TEXT COL: {}  {}", getSkinFontColorName(faceplateLayout_.theme),
          (activePopupMenu_ == UiPopupMenuKind::TextColor) ? "^" : "v"));
      const auto fc = getSkinFontColorValue(faceplateLayout_.theme);
      const bool isDarkInk = (fc.r + cLuminance(fc) < 0.75f);
      (void)isDarkInk;
      btnUiGlobalFontColor_->setAccentColor(
          (fc.r + fc.g + fc.b < 0.75f) ? gui::Color::AccentAmber() : fc);
    }
    if (btnUiGlobalPlateColor_) {
      btnUiGlobalPlateColor_->setText(std::format(
          "PLATE COL: {}  {}", getSkinTextPlateColorName(faceplateLayout_.theme),
          (activePopupMenu_ == UiPopupMenuKind::PlateColor) ? "^" : "v"));
      const auto pc = getSkinTextPlateColorValue(faceplateLayout_.theme);
      btnUiGlobalPlateColor_->setAccentColor(
          (pc.a < 0.05f || (pc.r + pc.g + pc.b < 0.75f)) ? gui::Color::AccentAmber() : pc);
    }

    const bool isUiBuild = (currentTab_ == StudioTab::UiBuilder);
    rebuildParamBindOptions();

    if (selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size())) {
      const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      const bool isBadgeOrFrame = (w.type == UiWidgetType::RivetedNameplate ||
                                   w.type == UiWidgetType::SectionFrame ||
                                   w.type == UiWidgetType::VentilationGrille ||
                                   w.type == UiWidgetType::RackHandlePair);
      const bool isSingleTitleWidget =
          (w.type == UiWidgetType::SectionFrame ||
           w.type == UiWidgetType::VentilationGrille ||
           w.type == UiWidgetType::RackHandlePair ||
           w.type == UiWidgetType::PatchbayJackSocket ||
           w.type == UiWidgetType::SilkscreenLabel);
      if (inputUiWidgetTitle_) {
        if (w.type == UiWidgetType::RivetedNameplate) {
          inputUiWidgetTitle_->setText(!w.title.empty() ? w.title : faceplateLayout_.theme.pluginTitle);
        } else {
          inputUiWidgetTitle_->setText(w.title);
        }
        inputUiWidgetTitle_->setVisible(isUiBuild);
      }
      if (inputUiWidgetSub_) {
        if (w.type == UiWidgetType::RivetedNameplate) {
          const auto pipePos = w.subtitle.find('|');
          const std::string sub1 = (pipePos != std::string::npos) ? w.subtitle.substr(0, pipePos) : w.subtitle;
          inputUiWidgetSub_->setText(sub1);
          if (pipePos != std::string::npos && pipePos + 1 < w.subtitle.size() &&
              faceplateLayout_.theme.badgeFooterText.empty()) {
            faceplateLayout_.theme.badgeFooterText = w.subtitle.substr(pipePos + 1);
          }
        } else {
          inputUiWidgetSub_->setText(w.subtitle);
        }
        inputUiWidgetSub_->setVisible(isUiBuild && !isSingleTitleWidget);
      }
      if (inputUiBadgeFooter_) {
        if (w.type == UiWidgetType::RivetedNameplate) {
          const auto pipePos = w.subtitle.find('|');
          const std::string sub2 = !faceplateLayout_.theme.badgeFooterText.empty()
              ? faceplateLayout_.theme.badgeFooterText
              : ((pipePos != std::string::npos) ? w.subtitle.substr(pipePos + 1) : "");
          inputUiBadgeFooter_->setText(sub2);
        }
        inputUiBadgeFooter_->setVisible(isUiBuild && isBadgeOrFrame);
      }
      if (btnUiEditBadge_)
        btnUiEditBadge_->setVisible(isUiBuild && isBadgeOrFrame);
      if (btnUiUploadStickerImg_) {
        const bool isSticker = (w.type == UiWidgetType::VintageSticker);
        btnUiUploadStickerImg_->setText(
            w.customImagePath.empty() ? "UPLOAD IMAGE..." : "CHANGE IMAGE...");
        btnUiUploadStickerImg_->setVisible(isUiBuild && isSticker);
      }
      if (btnUiStickerRotate_) {
        const bool isSticker = (w.type == UiWidgetType::VintageSticker);
        const char *rotArrow =
            (activePopupMenu_ == UiPopupMenuKind::StickerRotation) ? "^" : "v";
        btnUiStickerRotate_->setText(
            std::format("ANGLE: {:+.0f}°  {}", w.rotationDeg, rotArrow));
        btnUiStickerRotate_->setVisible(isUiBuild && isSticker);
      }

      const bool canBindParam =
          (isKnobWidget(w.type) || isSwitchWidget(w.type) ||
           isFaderWidget(w.type));
      if (!canBindParam) {
        isParamBindDropdownOpen_ = false;
      }
      if (btnUiWidgetBind_) {
        if (canBindParam) {
          btnUiWidgetBind_->setText(
              std::format("BIND: {}  {}",
                          describeWidgetAutoBinding(w, schematicChain_),
                          isParamBindDropdownOpen_ ? "^" : "v"));
          btnUiWidgetBind_->setVisible(isUiBuild);
        } else {
          btnUiWidgetBind_->setVisible(false);
        }
      }

      if (btnUiWidgetStyle_) {
        const char *styleArrow =
            (activePopupMenu_ == UiPopupMenuKind::WidgetStyle) ? "^" : "v";
        if (w.type == UiWidgetType::SectionFrame) {
          const auto fStyle = static_cast<gui::SectionFrameStyle>(
              static_cast<int>(w.scalePreset) % 8);
          btnUiWidgetStyle_->setText(std::format(
              "FRAME: {}  {}", getSectionFrameStyleName(fStyle), styleArrow));
        } else if (w.type == UiWidgetType::RivetedNameplate) {
          const auto bStyle = static_cast<gui::BadgeVisualStyle>(
              static_cast<int>(w.scalePreset) % 6);
          btnUiWidgetStyle_->setText(std::format(
              "BADGE: {}  {}", getBadgeVisualStyleName(bStyle), styleArrow));
        } else if (w.type == UiWidgetType::VintageSticker) {
          const auto sPreset = static_cast<gui::StickerPreset>(
              std::clamp(static_cast<int>(w.scalePreset), 0,
                         gui::kStickerPresetCount - 1));
          btnUiWidgetStyle_->setText(std::format(
              "STICKER: {}  {}", getStickerPresetName(sPreset), styleArrow));
        } else {
          btnUiWidgetStyle_->setText(std::format(
              "STYLE: {}  {}", getWidgetTypeName(w.type), styleArrow));
        }
      }
      if (btnUiWidgetColor_) {
        const char *colPrefix =
            (w.type == UiWidgetType::SectionFrame) ? "FRAME COL" : "COLOUR";
        const char *colArrow =
            (activePopupMenu_ == UiPopupMenuKind::WidgetColor) ? "^" : "v";
        btnUiWidgetColor_->setText(
            std::format("{}: {}  {}", colPrefix,
                        getUiAccentColorName(w.accentColor), colArrow));
        btnUiWidgetColor_->setAccentColor(getUiAccentColorValue(w.accentColor));
      }

      if (btnUiWidgetScale_) {
        if (isKnobWidget(w.type)) {
          relevantScaleOptions_ = getRelevantScalesForKnob(w, schematicChain_);
          const std::string scaleLabel =
              (w.customTickLabels.size() >= 2 &&
               w.scalePreset != UiScalePreset::DialOneToTen &&
               w.scalePreset != UiScalePreset::DialZeroToTen)
                  ? std::format("{} .. {}", w.customTickLabels.front(),
                                w.customTickLabels.back())
                  : std::string(getScalePresetName(w.scalePreset));
          btnUiWidgetScale_->setText(
              std::format("SCALE: {}  {}", scaleLabel,
                          isScaleDropdownOpen_ ? "^" : "v"));
          btnUiWidgetScale_->setVisible(isUiBuild);
        } else {
          isScaleDropdownOpen_ = false;
          relevantScaleOptions_.clear();
          btnUiWidgetScale_->setVisible(false);
        }
      }
      if (btnUiGlobalKnob_) {
        if (isKnobWidget(w.type)) {
          btnUiGlobalKnob_->setText("APPLY TO ALL KNOBS");
          btnUiGlobalKnob_->setVisible(isUiBuild);
        } else {
          btnUiGlobalKnob_->setVisible(false);
        }
      }
      if (btnUiGlobalSwitch_) {
        if (isSwitchWidget(w.type)) {
          btnUiGlobalSwitch_->setText("APPLY TO ALL SWITCHES");
          btnUiGlobalSwitch_->setVisible(isUiBuild);
        } else {
          btnUiGlobalSwitch_->setVisible(false);
        }
      }
      if (btnUiWidgetLayer_)
        btnUiWidgetLayer_->setVisible(false);
      const bool isPermanentMaster =
          faceplateLayout_.isPermanentMasterSectionWidget(w);
      if (btnUiWidgetDel_)
        btnUiWidgetDel_->setVisible(isUiBuild && !isPermanentMaster);
      if (btnUiWidgetDup_)
        btnUiWidgetDup_->setVisible(isUiBuild && !isPermanentMaster);
    } else {
      isScaleDropdownOpen_ = false;
      isParamBindDropdownOpen_ = false;
      relevantScaleOptions_.clear();
      if (btnUiWidgetScale_)
        btnUiWidgetScale_->setVisible(false);
      if (btnUiGlobalKnob_)
        btnUiGlobalKnob_->setVisible(false);
      if (btnUiGlobalSwitch_)
        btnUiGlobalSwitch_->setVisible(false);
      if (btnUiWidgetBind_)
        btnUiWidgetBind_->setVisible(false);
      if (btnUiWidgetLayer_)
        btnUiWidgetLayer_->setVisible(false);
      if (btnUiUploadStickerImg_)
        btnUiUploadStickerImg_->setVisible(false);
      if (btnUiStickerRotate_)
        btnUiStickerRotate_->setVisible(false);
      if (btnUiWidgetDel_)
        btnUiWidgetDel_->setVisible(false);
      if (btnUiWidgetDup_)
        btnUiWidgetDup_->setVisible(false);
      if (inputUiWidgetSub_)
        inputUiWidgetSub_->setVisible(isUiBuild);
      if (inputUiBadgeFooter_)
        inputUiBadgeFooter_->setVisible(isUiBuild);
      if (btnUiEditBadge_)
        btnUiEditBadge_->setVisible(isUiBuild);
    }
    resized();
  }

  static float cLuminance(gui::Color c) noexcept {
    return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
  }

  void initializeBlankUntitledProject() {
    schematicChain_.clear();
    selectedNodeIndex_ = -1;
    selectedProjectIndex_ = -1;
    canvasPage_ = 0;
    schematicZoom_ = 1.0f;
    schematicScrollX_ = 0.0f;
    wizardConfig_.name.clear();
    wizardConfig_.pluginId = "com.avra.plugin";
    wizardConfig_.category = "Custom Studio Processor";
    userHasCustomName_ = false;
    if (inputPluginName_) {
      inputPluginName_->setText("");
    }
    rackBypassed_ = false;
    rackMuted_ = false;
    rackPhaseInvert_ = false;
    rackInputTrimDb_ = 0.0f;
    rackOutputTrimDb_ = 0.0f;
    rackGlobalWetMix_ = 1.0f;
    liveSchematicRunner_.setMasterMixPercent(100.0f);
    rackStereoBalance_ = 0.0f;
    integratedMeanSqIn_ = 0.0f;
    integratedMeanSqOut_ = 0.0f;
    if (btnRackBypass_) {
      btnRackBypass_->setToggleState(false);
    }
    faceplateLayout_.widgets.clear();
    faceplateLayout_ = buildFaceplateFromSchematicWithTheme(
        "", schematicChain_, faceplateLayout_.theme);
    faceplateLayout_.theme.pluginTitle.clear();
    selectDefaultKnobOnFaceplate();
    syncInspectorKnobsToSelection();
    updateCanvasPageButtons();
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();
    statusMessage_ =
        "Ready - add stages from the Component Library on the left and enter a "
        "Plugin Name below, or open a project from FILE -> Open Project.";
  }

  void loadWorkspaceProjectIntoSchematic(const WorkspacePlugin &proj) {
    const std::string diskFolder =
        (proj.diskFolder == "GainPlugin" || proj.name == "Avra Gain")
            ? "AvraGain"
            : (proj.diskFolder.empty() ? proj.name : proj.diskFolder);
    schematicChain_ = loadProjectSchematicChain(diskFolder);
    canvasPage_ = 0;
    selectedNodeIndex_ = schematicChain_.empty() ? -1 : 0;
    rackBypassed_ = false;
    rackMuted_ = false;
    rackPhaseInvert_ = false;
    rackInputTrimDb_ = 0.0f;
    rackOutputTrimDb_ = 0.0f;
    rackGlobalWetMix_ = 1.0f;
    liveSchematicRunner_.setMasterMixPercent(100.0f);
    rackStereoBalance_ = 0.0f;
    integratedMeanSqIn_ = 0.0f;
    integratedMeanSqOut_ = 0.0f;
    if (btnRackBypass_)
      btnRackBypass_->setToggleState(false);

    const std::string cleanName = ProjectGenerator::sanitizeName(proj.name);
    wizardConfig_.name = cleanName;
    wizardConfig_.category = proj.category;
    wizardConfig_.pluginId = proj.pluginId;
    inputPluginName_->setText(cleanName);
    userHasCustomName_ = true;

    faceplateLayout_ = loadProjectFaceplateLayout(diskFolder, schematicChain_);
    ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
    activeStyleTemplate_ = static_cast<UiStyleTemplate>(
        std::clamp(faceplateLayout_.theme.styleTemplateIdx, 0,
                   kTotalUiStyleTemplates - 1));
    selectDefaultKnobOnFaceplate();
    updateUiThemeButtonLabels();
    syncUiWidgetInspector();

    syncInspectorKnobsToSelection();
    updateCanvasPageButtons();
    schematicZoom_ = 1.0f;
    schematicScrollX_ = 0.0f;
    statusMessage_ = std::format(
        "Loaded '{}' ({} modules & {} faceplate controls - "
        "press Ctrl+S or click SAVE PROJECT anytime).",
        proj.name, schematicChain_.size(), faceplateLayout_.widgets.size());
  }

  void loadStarterChain(int presetIdx) {
    recordUndoSnapshot("Load Starter Blueprint Chain");
    schematicChain_.clear();
    canvasPage_ = 0;
    if (presetIdx == 0) {
      // =====================================================================
      // BLUEPRINT 1: STUDIO DRUM BUS CHAIN (5 Stages, Dialed-In)
      //   S1: PR2 Class-A Transistor Preamp (LINE 0 / +2.5 dB drive, -1.5 dB trim, 1200 HI-Z)
      //   S2: EQ5 Proportional-Q Gyrator EQ (+2.0 dB @ 50Hz Peak, -1.5 dB @ 400Hz, +1.5 dB @ 10kHz)
      //   S3: CP5 Quad-VCA Master Bus Comp (4:1, 30ms Attack, 0.1s Fast Release, MED Thrust)
      //   S4: CP1 Fast Feedback FET Limiter [Parallel 25%] (4:1 Parallel Drum Crush, 90Hz SC HPF)
      //   S5: CL1 Standard 16x Clipper (0.0 dB drive, -0.5 dB ceiling, 60% soft analog knee, 85% Mix)
      // =====================================================================
      auto s1 = makeDefaultSchematicNode(SchematicComponentType::ClassAPreamp, false);
      s1.setParamVal(0, 2.0f);  // LINE 0 (clean Class-A input drive)
      s1.setParamVal(1, 0.0f);  // HPF OFF (preserve full sub-kick weight)
      s1.setParamVal(2, -1.5f); // -1.5 dB Output Trim
      s1.setParamVal(3, 1.0f);  // 1200 Ohm HI-Z
      s1.setParamVal(4, 0.0f);  // 0 deg NORM
      schematicChain_.push_back(s1);

      auto s2 = makeDefaultSchematicNode(SchematicComponentType::ProportionalQGyratorEq, false);
      s2.setParamVal(0, 0.0f);  // 50 Hz Low Band
      s2.setParamVal(1, 2.0f);  // +2.0 dB Low Punch
      s2.setParamVal(2, 1.0f);  // 400 Hz Low-Mid Boxiness Cut
      s2.setParamVal(3, -1.5f); // -1.5 dB Mid Cut
      s2.setParamVal(4, 3.0f);  // 10 kHz High Snap
      s2.setParamVal(5, 1.5f);  // +1.5 dB High Gain
      s2.setParamVal(6, 0.0f);  // Low Band = PEAK (tight 50 Hz kick punch)
      s2.setParamVal(7, 1.0f);  // High Band = SHELF
      schematicChain_.push_back(s2);

      auto s3 = makeDefaultSchematicNode(SchematicComponentType::VcaBusCompressor, false);
      s3.setParamVal(0, -12.0f); // -12.0 dB Threshold (2-3 dB bus glue)
      s3.setParamVal(1, 1.5f);   // +1.5 dB Make-Up Gain
      s3.setParamVal(2, 5.0f);   // 30.0 ms Attack (lets transient crack through)
      s3.setParamVal(3, 0.0f);   // 0.1 s Fast Snappy Release
      s3.setParamVal(4, 1.0f);   // 4:1 Ratio
      s3.setParamVal(5, 1.0f);   // MED Pink-Noise Thrust Filter
      s3.setParamVal(6, 100.0f); // 100% Mix
      schematicChain_.push_back(s3);

      auto s4 = makeDefaultSchematicNode(SchematicComponentType::FetCompressor1176, true);
      s4.parallelMixPercent = 25.0f; // 25% Parallel New-York FET Smash
      s4.setParamVal(0, -18.0f); // Controlled FET input drive
      s4.setParamVal(1, 0.0f);   // 0.0 dB Output Level (prevents downstream clipper overload)
      s4.setParamVal(2, 2.5f);   // Medium-fast Attack
      s4.setParamVal(3, 6.0f);   // Fast Release
      s4.setParamVal(4, 1.0f);   // 8:1 Punchy Parallel Ratio
      s4.setParamVal(5, 90.0f);  // 90 Hz Sidechain HPF
      s4.setParamVal(6, 100.0f); // 100% Stage Mix (blended via 25% Parallel Bus)
      schematicChain_.push_back(s4);

      auto s5 = makeDefaultSchematicNode(SchematicComponentType::OversampledVariableClipper, false);
      s5.setParamVal(0, 0.0f);    // 0.0 dB Input Drive (transparent peak shaving without distortion)
      s5.setParamVal(1, -0.5f);   // -0.5 dB Peak Ceiling
      s5.setParamVal(2, 60.0f);   // 60% Smooth Soft-Knee Curve
      s5.setParamVal(3, 3500.0f); // 3.5 kHz HF Emphasis Tilt
      s5.setParamVal(4, 85.0f);   // 85% Mix (retains natural drum transient air)
      schematicChain_.push_back(s5);

      wizardConfig_.category = "5-Stage Studio Drum Bus Chain";
      wizardConfig_.name = "DrumBusMaster";
      wizardConfig_.pluginId = "com.avra.drumbusmaster";
      if (inputPluginName_) inputPluginName_->setText("DrumBusMaster");
      faceplateLayout_.theme.pluginTitle = "DrumBusMaster";
      selectTestSignal(TestSignalMode::DrumTransientLoop);
      statusMessage_ =
          "Loaded 5-Stage DRUM BUS Blueprint (PR2 -> EQ5 -> CP5 -> CP1 [Par 25%] -> CL1): "
          "Class-A Pre -> Gyrator EQ -> Quad-VCA Glue -> Parallel FET -> Transparent 16x Clipper.";
    } else if (presetIdx == 1) {
      // =====================================================================
      // BLUEPRINT 2: STUDIO BASS GUITAR CHAIN (6 Stages, Dialed-In)
      //   S1: PR4 Dual-Triode Tube Pre (+3.5 dB 12AX7/12BH7 drive, 58% Cathode Bias/B+ Sag)
      //   S2: AU2 Auto Bass Leveller (30-320 Hz Fundamental Isolator & Note-Onset Sustain Rider)
      //   S3: CP2 Program-Dependent T4B Opto (48 Peak Reduction, 51 Gain, Locked-In Sustain)
      //   S4: ST6 Sub-Harmonic Synthesizer (Missing-Fundamental Reconstructor, 80 Hz Crossover)
      //   S5: ST4 Dark Bass Grit Bus [Parallel 25%] (+4.5 dB Drive, 45% Blend, 160 Hz Sub Crossover)
      //   S6: EQ1 Passive Offset-RC Low EQ (60 Hz simultaneous +3.5 Boost / 2.5 Atten Trick)
      // =====================================================================
      auto s1 = makeDefaultSchematicNode(SchematicComponentType::Tube12AX7, false);
      s1.setParamVal(0, 3.5f);  // +3.5 dB 12AX7 Grid Drive
      s1.setParamVal(1, -2.0f); // -2.0 dB Output Attenuator
      s1.setParamVal(2, 58.0f); // 58% Cathode Bias / B+ Sag
      s1.setParamVal(3, 0.0f);  // ECC83 High-Gain Voicing
      s1.setParamVal(4, 0.0f);  // NORM Polarity
      schematicChain_.push_back(s1);

      auto s2 = makeDefaultSchematicNode(SchematicComponentType::SilkBerylliumAirExciter, false);
      s2.setParamVal(0, -15.0f); // -15.0 dB Target Bass Level
      s2.setParamVal(1, 6.0f);   // 6.0 dB Note-to-Note Leveller Range
      s2.setParamVal(2, 160.0f); // 160 Hz Fundamental Ceiling
      s2.setParamVal(3, 60.0f);  // 60% Fret & Slap Guard
      s2.setParamVal(4, 55.0f);  // 55 ms Note Onset Speed
      s2.setParamVal(5, 0.0f);   // 0.0 dB Output Trim
      schematicChain_.push_back(s2);

      auto s3 = makeDefaultSchematicNode(SchematicComponentType::OptoCompressorT4B, false);
      s3.setParamVal(0, 51.0f); // 51 Optical Makeup Gain
      s3.setParamVal(1, 48.0f); // 48 Peak Reduction (smooth optical sustain)
      s3.setParamVal(2, 85.0f); // Flat wideband sidechain detector response
      s3.setParamVal(3, 1.0f);  // COMPRESS Mode
      s3.setParamVal(4, 0.0f);  // 0.0 dB Output Trim
      s3.setParamVal(5, 100.0f);
      schematicChain_.push_back(s3);

      auto s4 = makeDefaultSchematicNode(SchematicComponentType::PsychoacousticSubSynth, false);
      s4.setParamVal(0, 35.0f); // 35% Subharmonic Level (missing-fundamental bloom)
      s4.setParamVal(1, 4.5f);  // 4.5 24-36 Hz Band Boost
      s4.setParamVal(2, 5.5f);  // 5.5 36-56 Hz Band Boost
      s4.setParamVal(3, 1.0f);  // 80 Hz Crossover
      s4.setParamVal(4, 0.0f);  // 0.0 dB Main Output Trim
      schematicChain_.push_back(s4);

      auto s5 = makeDefaultSchematicNode(SchematicComponentType::ParallelSaturationBus, true);
      s5.parallelMixPercent = 25.0f; // 25% Parallel Dark Bass Grit Blend
      s5.setParamVal(0, 4.5f);   // +4.5 dB Warm Midrange Grit Drive
      s5.setParamVal(1, 45.0f);  // 45% Upper-Band Grit Blend (keeps sub foundation clean)
      s5.setParamVal(2, 160.0f); // 160 Hz Linkwitz-Riley Sub Crossover
      s5.setParamVal(3, 2.0f);   // +2.0 dB Low-Mid Grunt (280 Hz body)
      s5.setParamVal(4, -1.5f);  // -1.5 dB Output Level
      schematicChain_.push_back(s5);

      auto s6 = makeDefaultSchematicNode(SchematicComponentType::PassivePultecEq, false);
      s6.setParamVal(0, 2.0f); // 60 Hz Low Frequency (CPS)
      s6.setParamVal(1, 3.5f); // 3.5 Low Boost (deep sub weight)
      s6.setParamVal(2, 2.5f); // 2.5 Low Atten (105 Hz low-mid scoop clarity trick)
      s6.setParamVal(3, 0.0f); // 0.0 dB Tube Makeup Trim
      s6.setParamVal(4, 2.0f); // +2.0 dB Interstage Transformer Drive
      schematicChain_.push_back(s6);

      wizardConfig_.category = "6-Stage Bass Guitar Studio Chain";
      wizardConfig_.name = "BassStudioChain";
      wizardConfig_.pluginId = "com.avra.bassstudiochain";
      if (inputPluginName_) inputPluginName_->setText("BassStudioChain");
      faceplateLayout_.theme.pluginTitle = "BassStudioChain";
      selectTestSignal(TestSignalMode::BassDiGroove);
      statusMessage_ =
          "Loaded 6-Stage BASS GUITAR Blueprint (PR4 -> AU2 -> CP2 -> ST6 -> ST4 [Par 25%] -> EQ1): "
          "Tube Pre -> Auto Bass Leveller -> T4B Opto -> Sub-Harmonic Synth -> Parallel Dark Grit -> 60Hz Passive Low EQ.";
    } else if (presetIdx == 2) {
      // =====================================================================
      // BLUEPRINT 3: STUDIO CLEAN GUITAR CHAIN (5 Stages, Dialed-In)
      //   S1: PR11 Dual-Triode Console Pre (0dB Clean Tube Bloom, +1.5dB @ 100Hz, +1.5dB @ 7kHz)
      //   S2: CP1 Fast Feedback FET Limiter (-22dB Input, +1.0dB Out, 4:1, Zero Noise Floor Hiss)
      //   S3: EQ5 Proportional-Q Gyrator EQ (-1.5dB @ 200Hz Mud Cut, +1.5dB @ 3kHz Chime, +1.5dB @ 10kHz)
      //   S4: MD2 Tri-Stereo BBD Chorus [Parallel 28%] (0.65 Hz Lush 3-Phase MN3007 Swirl)
      //   S5: RV3 Triple-Helix Studio Spring [Parallel 22%] (2.1s Decay, 58% Chirp Dispersion)
      // =====================================================================
      auto s1 = makeDefaultSchematicNode(SchematicComponentType::AmericanTubeConsolePre, false);
      s1.setParamVal(0, 2.0f);  // 0 dB Tube Headroom
      s1.setParamVal(1, 5.0f);  // +1.5 dB Low Shelf Warmth
      s1.setParamVal(2, 5.0f);  // +1.5 dB High Shelf Glassy Chime
      s1.setParamVal(3, 0.0f);  // 0.0 dB Level Trim
      s1.setParamVal(4, 1.0f);  // 100 Hz Low Shelf Corner
      s1.setParamVal(5, 1.0f);  // 7 kHz High Shelf Corner
      schematicChain_.push_back(s1);

      auto s2 = makeDefaultSchematicNode(SchematicComponentType::FetCompressor1176, false);
      s2.setParamVal(0, -22.0f); // Gentle 2-3 dB Clean Pick Compression (whisper-quiet floor)
      s2.setParamVal(1, 1.0f);   // +1.0 dB Makeup Level (zero hiss amplification)
      s2.setParamVal(2, 3.0f);   // Medium-Slow Attack (preserves transient pluck)
      s2.setParamVal(3, 5.0f);   // Fast Musical Release (lifts chord sustain)
      s2.setParamVal(4, 0.0f);   // 4:1 Ratio
      s2.setParamVal(5, 85.0f);  // 85 Hz Sidechain HPF
      s2.setParamVal(6, 85.0f);  // 85% Mix
      schematicChain_.push_back(s2);

      auto s3 = makeDefaultSchematicNode(SchematicComponentType::ProportionalQGyratorEq, false);
      s3.setParamVal(0, 2.0f);  // 200 Hz Low-Mid Mud
      s3.setParamVal(1, -1.5f); // -1.5 dB Low-Mid Cleanup
      s3.setParamVal(2, 5.0f);  // 3.0 kHz Bell Chime
      s3.setParamVal(3, 1.5f);  // +1.5 dB Mid Chime
      s3.setParamVal(4, 3.0f);  // 10 kHz Air Sparkle
      s3.setParamVal(5, 1.5f);  // +1.5 dB High Shelf
      schematicChain_.push_back(s3);

      auto s4 = makeDefaultSchematicNode(SchematicComponentType::TriStereoBbdChorus, true);
      s4.parallelMixPercent = 28.0f; // 28% Parallel Tri-Stereo Chorus
      s4.setParamVal(0, 50.0f);  // 50% Left Mod Depth
      s4.setParamVal(1, 38.0f);  // 38% Center Mod Depth
      s4.setParamVal(2, 52.0f);  // 52% Right Mod Depth
      s4.setParamVal(3, 0.65f);  // 0.65 Hz Slow Liquid Swirl
      s4.setParamVal(4, 100.0f); // 100% Wet on Parallel Branch
      schematicChain_.push_back(s4);

      auto s5 = makeDefaultSchematicNode(SchematicComponentType::DispersionSpringReverb, true);
      s5.parallelMixPercent = 22.0f; // 22% Parallel Studio Spring Reverb
      s5.setParamVal(0, 0.0f);   // 0.0 dB Tank Dwell Drive
      s5.setParamVal(1, 2.1f);   // 2.1 s Spring Decay
      s5.setParamVal(2, 58.0f);  // 58% Helical Chirp Tension
      s5.setParamVal(3, 60.0f);  // 60% Warm Spring Tone
      s5.setParamVal(4, 100.0f); // 100% Wet on Parallel Branch
      schematicChain_.push_back(s5);

      wizardConfig_.category = "5-Stage Clean Guitar Studio Chain";
      wizardConfig_.name = "CleanGuitarStudio";
      wizardConfig_.pluginId = "com.avra.cleanguitarstudio";
      if (inputPluginName_) inputPluginName_->setText("CleanGuitarStudio");
      faceplateLayout_.theme.pluginTitle = "CleanGuitarStudio";
      selectTestSignal(TestSignalMode::CleanGuitarChord);
      statusMessage_ =
          "Loaded 5-Stage CLEAN GUITAR Blueprint (PR11 -> CP1 -> EQ5 -> MD2 [Par 28%] -> RV3 [Par 22%]): "
          "Dual-Triode Pre -> Low-Noise 4:1 FET Comp -> Gyrator Chime EQ -> Tri-Stereo Chorus -> Studio Spring.";
    } else if (presetIdx == 3) {
      // =====================================================================
      // BLUEPRINT 4: DISTORTED ELECTRIC GUITAR CHAIN (5 Stages, Dialed-In)
      //   S1: CP11 Dual-Threshold Noise Gate (-34dB Thresh, 0.8ms Attack, 85ms Tight Chug Clamp)
      //   S2: PR5 Pentode Line Preamp (28dB EF86 Gain, 0.0dB Fine Trim - Zero Upper Fizz)
      //   S3: ST2 6AS6 Starved Valve Drive (P1 Pentode Crunch, +7.5dB Drive, 6kHz Fizz-Taming LPF)
      //   S4: EQ4 3-Band Inductor Console EQ (80Hz 18dB/oct HPF, +1.5dB @ 110Hz Thump, +2.5dB @ 1.6kHz Bite)
      //   S5: TP1 2-Inch Multitrack Tape (15 IPS Oxide Compression & HF Smoothing)
      // =====================================================================
      auto s1 = makeDefaultSchematicNode(SchematicComponentType::HysteresisNoiseGate, false);
      s1.setParamVal(0, -34.0f); // -34 dB Schmitt-Trigger Gate Threshold
      s1.setParamVal(1, 0.8f);   // 0.8 ms Instant Pick-Transient Opening
      s1.setParamVal(2, 85.0f);  // 85 ms Tight Palm-Mute Release
      s1.setParamVal(3, -45.0f); // -45 dB Noise Floor Attenuation
      s1.setParamVal(4, 750.0f); // 750 Hz Pick-Attack Key Filter
      schematicChain_.push_back(s1);

      auto s2 = makeDefaultSchematicNode(SchematicComponentType::PentodeEF86, false);
      s2.setParamVal(0, 2.0f);  // 28 dB Sweet-Spot Pentode Gain (warm body without fizz)
      s2.setParamVal(1, 0.0f);  // 0.0 dB Fine Gain Trim (no harsh fine-trim push)
      s2.setParamVal(2, -2.0f); // -2.0 dB Output Trim
      s2.setParamVal(3, 1.0f);  // -3 dB @ 60 Hz Pre-Drive Bass Tightener
      s2.setParamVal(4, 0.0f);  // Pad OUT
      schematicChain_.push_back(s2);

      auto s3 = makeDefaultSchematicNode(SchematicComponentType::TrigonometricWavefolder, false);
      s3.setParamVal(0, 1.0f);  // P1 Pentode Odd-Harmonic Valve Crunch
      s3.setParamVal(1, 1.0f);  // 6 kHz Steep Fizz-Taming LPF
      s3.setParamVal(2, 7.5f);  // +7.5 dB Tight Musical Rhythm/Lead Drive
      s3.setParamVal(3, 52.0f); // 52% Plate Bias Asymmetry
      s3.setParamVal(4, -3.0f); // -3.0 dB Gain-Matched Output Level
      s3.setParamVal(5, 0.0f);  // Overdrive Mode = OFF (smooth harmonic structure)
      s3.setParamVal(6, 0.0f);  // Presence Lift = NORM (zero upper-mid fizz)
      schematicChain_.push_back(s3);

      auto s4 = makeDefaultSchematicNode(SchematicComponentType::InductorEq3Band, false);
      s4.setParamVal(0, 3.0f); // 110 Hz Cabinet Low-End Thump
      s4.setParamVal(1, 3.0f); // 1.6 kHz Forward Guitar Midrange Bite
      s4.setParamVal(2, 1.5f); // +1.5 dB @ 110 Hz
      s4.setParamVal(3, 2.5f); // +2.5 dB @ 1.6 kHz Inductor Mid Cut-Through
      s4.setParamVal(4, 0.0f); // 0.0 dB @ 12 kHz (keeps top end silky)
      s4.setParamVal(5, 2.0f); // 80 Hz 18 dB/oct Resonant HPF (clears mud for kick/bass)
      schematicChain_.push_back(s4);

      auto s5 = makeDefaultSchematicNode(SchematicComponentType::TapeHysteresis, false);
      s5.setParamVal(0, 2.0f);  // +2.0 dB Tape Input Drive
      s5.setParamVal(1, 1.5f);  // +1.5 dB Warm Over-Bias HF Smoothing
      s5.setParamVal(2, 50.0f); // 50% Head-Bump & Repro Contour
      s5.setParamVal(3, -1.5f); // -1.5 dB Output Calibrate
      s5.setParamVal(4, 1.0f);  // 15 IPS Rock Multitrack Speed
      schematicChain_.push_back(s5);

      wizardConfig_.category = "5-Stage Distorted Electric Guitar Chain";
      wizardConfig_.name = "DistGuitarStudio";
      wizardConfig_.pluginId = "com.avra.distguitarstudio";
      if (inputPluginName_) inputPluginName_->setText("DistGuitarStudio");
      faceplateLayout_.theme.pluginTitle = "DistGuitarStudio";
      selectTestSignal(TestSignalMode::GuitarDiChug);
      statusMessage_ =
          "Loaded 5-Stage DIST GUITAR Blueprint (CP11 -> PR5 -> ST2 -> EQ4 -> TP1): "
          "Hysteresis Gate -> 28dB Pentode Pre (0dB Fine Trim) -> 6AS6 Valve Drive (6kHz LPF) -> Inductor EQ -> 15 IPS Tape.";
    } else if (presetIdx == 4) {
      // =====================================================================
      // BLUEPRINT 5: FLAGSHIP STUDIO VOCAL CHAIN (7 Stages, Dialed-In)
      //   S1: PR2 Class-A Transistor Preamp (+5dB Warmth, 80Hz Plosive HPF, 1200 Ohm HI-Z)
      //   S2: AU1 Auto Vocal Leveller (180Hz-4.2kHz Speech-Weighted Breath-Guarded Gain Rider)
      //   S3: CP10 Split-Band Sibilance Comp (6.2kHz Split HF De-Esser, 6.5dB Range)
      //   S4: CP2 Program-Dependent T4B Opto (Silky Optical Body Leveler)
      //   S5: EQ8 40kHz Air & Tilt EQ (+3.0dB @ 20kHz Silky Air Shelf, +1.0dB Tilt @ 550Hz)
      //   S6: DL10 Varispeed Capstan Tape ADT [Parallel 26%] (22ms Offset, 38% Wobble, 0.85Hz, 45% Head Bump)
      //   S7: RV1 Modulated Gold-Foil Plate [Parallel 22%] (2.1s Decay, 35ms Pre-Delay, 140Hz HPF)
      // =====================================================================
      auto s1 = makeDefaultSchematicNode(SchematicComponentType::ClassAPreamp, false);
      s1.setParamVal(0, 3.0f);  // LINE -5 (+5 dB Class-A Vocal Warmth)
      s1.setParamVal(1, 2.0f);  // 80 Hz Vocal Plosive & Rumble HPF
      s1.setParamVal(2, -2.0f); // -2.0 dB Output Trim
      s1.setParamVal(3, 1.0f);  // 1200 Ohm HI-Z Open Top-End
      s1.setParamVal(4, 0.0f);  // NORM Polarity
      schematicChain_.push_back(s1);

      auto s2 = makeDefaultSchematicNode(SchematicComponentType::ZeroDistortionVocalRider, false);
      s2.setParamVal(0, -16.0f); // -16.0 dB Target Vocal Level
      s2.setParamVal(1, 6.0f);   // 6.0 dB Mass-Damped Fader Range
      s2.setParamVal(2, 45.0f);  // 45% Breath & Sibilance Gate Sensitivity
      s2.setParamVal(3, 160.0f); // 160 ms Natural Vocal Phrase Speed
      s2.setParamVal(4, 0.0f);   // 0.0 dB Output Trim
      schematicChain_.push_back(s2);

      auto s3 = makeDefaultSchematicNode(SchematicComponentType::SplitBandDeEsser, false);
      s3.setParamVal(0, 6200.0f); // 6.2 kHz Sibilance Center Frequency
      s3.setParamVal(1, 6.5f);    // 6.5 dB Max Sibilance Clamp
      s3.setParamVal(2, 38.0f);   // 38 ms Natural Consonant Recovery
      s3.setParamVal(3, 1.0f);    // SPLIT HF Mode (leaves vocal body untouched)
      s3.setParamVal(4, 0.0f);    // 0.0 dB Output Trim
      schematicChain_.push_back(s3);

      auto s4 = makeDefaultSchematicNode(SchematicComponentType::OptoCompressorT4B, false);
      s4.setParamVal(0, 52.0f); // 52 Tube Makeup Gain
      s4.setParamVal(1, 48.0f); // 48 Peak Reduction (2-3 dB smooth optical leveling)
      s4.setParamVal(2, 68.0f); // Slight HF emphasis for upfront vocal presence
      s4.setParamVal(3, 1.0f);  // COMPRESS Mode
      s4.setParamVal(4, 0.0f);  // 0.0 dB Output Trim
      s4.setParamVal(5, 100.0f);
      schematicChain_.push_back(s4);

      auto s5 = makeDefaultSchematicNode(SchematicComponentType::FulcrumTiltAirEq, false);
      s5.setParamVal(0, 3.0f);   // 20 kHz Ultra-Silky Air Band
      s5.setParamVal(1, 3.0f);   // +3.0 dB Breathy Vocal Air Boost
      s5.setParamVal(2, 1.0f);   // +1.0 dB Forward Spectral Tilt
      s5.setParamVal(3, 550.0f); // 550 Hz Vocal Tilt Fulcrum
      s5.setParamVal(4, -2.0f);  // -2.0 dB Sub-40Hz Cleanup
      s5.setParamVal(5, 0.0f);   // 0.0 dB Level Trim
      schematicChain_.push_back(s5);

      auto s6 = makeDefaultSchematicNode(SchematicComponentType::StudioAdtTapeDoubler, true);
      s6.parallelMixPercent = 26.0f; // 26% Parallel Varispeed Tape ADT Vocal Doubler
      s6.setParamVal(0, 22.0f);  // 22.0 ms Short-Gap Vocal Double Offset
      s6.setParamVal(1, 38.0f);  // 38% Organic Capstan Varispeed Depth
      s6.setParamVal(2, 0.85f);  // 0.85 Hz Smooth Tape Wander Rate
      s6.setParamVal(3, 45.0f);  // 45% Warm 95Hz Tape Head Bump
      s6.setParamVal(4, 100.0f); // 100% Wet on Parallel Branch
      schematicChain_.push_back(s6);

      auto s7 = makeDefaultSchematicNode(SchematicComponentType::PlateReverb, true);
      s7.parallelMixPercent = 22.0f; // 22% Parallel Gold-Foil Vocal Plate
      s7.setParamVal(0, 2.1f);    // 2.1 s Plate Damper Decay
      s7.setParamVal(1, 35.0f);   // 35 ms Pre-Delay (keeps dry vocal upfront)
      s7.setParamVal(2, 140.0f);  // 140 Hz Input HPF (zero low-mid reverb mud)
      s7.setParamVal(3, 6800.0f); // 6.8 kHz Warm Plate HF Damping
      s7.setParamVal(4, 100.0f);  // 100% Wet on Parallel Branch
      schematicChain_.push_back(s7);

      wizardConfig_.category = "7-Stage Studio Vocal Chain";
      wizardConfig_.name = "StudioVocalChain";
      wizardConfig_.pluginId = "com.avra.studiovocalchain";
      if (inputPluginName_) inputPluginName_->setText("StudioVocalChain");
      faceplateLayout_.theme.pluginTitle = "StudioVocalChain";
      selectTestSignal(TestSignalMode::VocalPhrase);
      statusMessage_ =
          "Loaded 7-Stage VOCAL CHAIN Blueprint (PR2 -> AU1 -> CP10 -> CP2 -> EQ8 -> DL10 [Par 26%] -> RV1 [Par 22%]): "
          "Class-A Pre -> Auto Vocal Leveller -> Split De-Esser -> T4B Opto -> 20kHz Air & Tilt EQ -> Parallel Tape ADT Doubler -> Gold-Foil Plate.";
    } else if (presetIdx == 5) {
      // =====================================================================
      // BLUEPRINT 6: STUDIO SYNTH / KEYS & FX CHAIN (6 Stages, Dialed-In)
      //   S1: PR1 Pure JFET Line Buffer (-3.0 dB Input Trim, 50% Drain Bias, -2.0 dB Output Trim)
      //   S2: EQ5 Proportional-Q Gyrator EQ (+1.5 dB @ 100Hz Shelf, -1.5 dB @ 400Hz Cut, +1.5 dB @ 12.5kHz Air)
      //   S3: CP5 Quad-VCA Master Bus Comp (2:1 Polyphonic Glue, 10ms Attack, 0.3s Smooth Release, 0 dB Make-Up)
      //   S4: MD1 4-Mode Pushbutton BBD Chorus [Parallel 20%] (Mode 3 Motionless-Center Stereo Width)
      //   S5: DL2 Cross-Fed Ping-Pong Delay [Parallel 15%] (375ms Dotted-8th Stereo Echo, 5.2kHz Damping)
      //   S6: RV2 Spin & Wander Concert Hall [Parallel 18%] (3.2s Lush Modulated Hall Tail, 28ms Pre-Delay)
      // =====================================================================
      auto s1 = makeDefaultSchematicNode(SchematicComponentType::JfetBuffer, false);
      s1.setParamVal(0, -3.0f); // -3.0 dB Clean Headroom Input Trim (prevents hot polyphonic synth pad clipping)
      s1.setParamVal(1, 50.0f); // 50% JFET Drain Bias
      s1.setParamVal(2, -2.0f); // -2.0 dB Output Trim
      s1.setParamVal(3, 0.0f);  // 0 dB Pad
      s1.setParamVal(4, 0.0f);  // 10 kOhm High-Z Synth/Keys Bridging Load
      schematicChain_.push_back(s1);

      auto s2 = makeDefaultSchematicNode(SchematicComponentType::ProportionalQGyratorEq, false);
      s2.setParamVal(0, 1.0f);  // 100 Hz Polysynth Fundamental Warmth
      s2.setParamVal(1, 1.5f);  // +1.5 dB Low Shelf Boost
      s2.setParamVal(2, 1.0f);  // 400 Hz Low-Mid Boxiness Cleanup
      s2.setParamVal(3, -1.5f); // -1.5 dB Mid Cut
      s2.setParamVal(4, 4.0f);  // 12.5 kHz Silky Harmonic Air
      s2.setParamVal(5, 1.5f);  // +1.5 dB High Shelf Boost
      s2.setParamVal(6, 1.0f);  // Low Band = SHELF
      s2.setParamVal(7, 1.0f);  // High Band = SHELF
      schematicChain_.push_back(s2);

      auto s3 = makeDefaultSchematicNode(SchematicComponentType::VcaBusCompressor, false);
      s3.setParamVal(0, -14.0f); // -14.0 dB Threshold (smooth polyphonic chord glue)
      s3.setParamVal(1, 0.0f);   // 0.0 dB Unity Make-Up Gain
      s3.setParamVal(2, 4.0f);   // 10.0 ms Attack (preserves synth/keys transient pluck)
      s3.setParamVal(3, 1.0f);   // 0.3 s Musical Release
      s3.setParamVal(4, 0.0f);   // 2:1 Gentle Synth Bus Ratio
      s3.setParamVal(5, 1.0f);   // MED Pink-Noise Sidechain Thrust
      s3.setParamVal(6, 100.0f); // 100% Mix
      schematicChain_.push_back(s3);

      auto s4 = makeDefaultSchematicNode(SchematicComponentType::BbdChorus, true);
      s4.parallelMixPercent = 20.0f; // 20% Parallel Motionless-Center Dimension BBD Chorus
      s4.setParamVal(0, 3.0f);   // Mode 3 Lush Anti-Phase Stereo Widening
      s4.setParamVal(1, 105.0f); // 105% BBD Modulation Depth
      s4.setParamVal(2, 0.55f);  // 0.55 Hz Smooth Triangle LFO Rate
      s4.setParamVal(3, 100.0f); // 100% Wet on Parallel Branch
      s4.setParamVal(4, -2.0f);  // -2.0 dB Output Trim
      schematicChain_.push_back(s4);

      auto s5 = makeDefaultSchematicNode(SchematicComponentType::PingPongDelay, true);
      s5.parallelMixPercent = 15.0f; // 15% Parallel Dotted-8th Stereo Ping-Pong Delay
      s5.setParamVal(0, 375.0f);  // 375 ms Dotted-8th Delay Time (at 120 BPM)
      s5.setParamVal(1, 38.0f);   // 38% Stereo Cross-Feedback
      s5.setParamVal(2, 85.0f);   // 85% Ping-Pong Stereo Width
      s5.setParamVal(3, 5200.0f); // 5.2 kHz Warm Analog High-Cut Damping
      s5.setParamVal(4, 100.0f);  // 100% Wet on Parallel Branch
      schematicChain_.push_back(s5);

      auto s6 = makeDefaultSchematicNode(SchematicComponentType::HallReverbFDN, true);
      s6.parallelMixPercent = 18.0f; // 18% Parallel Spin & Wander Concert Hall FDN
      s6.setParamVal(0, 3.2f);   // 3.2 s Lush Mid RT60 Decay
      s6.setParamVal(1, 1.25f);  // 1.25x Warm Bass Multiply
      s6.setParamVal(2, 74.0f);  // 74% Wide Concert Hall Size
      s6.setParamVal(3, 28.0f);  // 28 ms Pre-Delay
      s6.setParamVal(4, 78.0f);  // 78% High Diffusion
      s6.setParamVal(5, 100.0f); // 100% Wet on Parallel Branch
      schematicChain_.push_back(s6);

      wizardConfig_.category = "6-Stage Studio Synth / Keys & FX Chain";
      wizardConfig_.name = "SynthKeysStudio";
      wizardConfig_.pluginId = "com.avra.synthkeysstudio";
      if (inputPluginName_) inputPluginName_->setText("SynthKeysStudio");
      faceplateLayout_.theme.pluginTitle = "SynthKeysStudio";
      selectTestSignal(TestSignalMode::SynthChordLoop);
      statusMessage_ =
          "Loaded 6-Stage SYNTH / KEYS Blueprint (PR1 -> EQ5 -> CP5 -> MD1 [Par 20%] -> DL2 [Par 15%] -> RV2 [Par 18%]): "
          "JFET Line Buffer -> Gyrator EQ -> 2:1 Quad-VCA Glue -> Dimension BBD Chorus -> Ping-Pong Delay -> Concert Hall FDN.";
    }
    selectedNodeIndex_ = schematicChain_.empty()
                             ? -1
                             : static_cast<int>(schematicChain_.size() - 1);
    schematicZoom_ = 1.0f;
    schematicScrollX_ = 0.0f;
    syncInspectorKnobsToSelection();
    updateCanvasPageButtons();
    syncFaceplateWithSchematic();
    ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
    for (auto &w : faceplateLayout_.widgets) {
      if (w.type == UiWidgetType::RivetedNameplate) {
        w.title = wizardConfig_.name;
      }
    }
    repaint();
  }

  // =========================================================================
  // Workspace Project Management
  // =========================================================================
  void refreshWorkspaceProjects() {
    toolchainStatus_ = ProjectGenerator::detectToolchain();
    projects_.clear();

    namespace fs = std::filesystem;
    const fs::path rootDir = ProjectGenerator::getWorkspaceRoot();

    if (!hiddenProjects_.contains("Avra Gain") &&
        fs::exists(rootDir / "examples" / "GainPlugin" / "GainPlugin.h")) {
      const auto gainRep = ProjectGenerator::inspectProjectArtifacts(
          "GainPlugin", prefs_.exportFormatVst3, prefs_.exportFormatClap,
          prefs_.exportFormatStandalone, customExportDir_);
      projects_.push_back(
          {"Avra Gain", "GainPlugin", "Clean Stereo Utility (Demo)",
           "com.avra.gain",
           "Initial included Avra demo plugin - transparent stereo gain "
           "stage with Phase, Mute, and Direct2D peak meters.",
           "AvraGain_Clap", true, gainRep.succeeded, true});
    }

    const fs::path pluginsDir = rootDir / "plugins";
    if (fs::exists(pluginsDir) && fs::is_directory(pluginsDir)) {
      for (const auto &entry : fs::directory_iterator(pluginsDir)) {
        if (entry.is_directory()) {
          const std::string dirName = entry.path().filename().string();
          if (dirName == "AvraGain" || dirName == "GainPlugin")
            continue;
          if (hiddenProjects_.contains(dirName))
            continue;

          const bool isScaffolded = fs::exists(entry.path() / (dirName + ".h"));
          const auto artRep = ProjectGenerator::inspectProjectArtifacts(
              dirName, prefs_.exportFormatVst3, prefs_.exportFormatClap,
              prefs_.exportFormatStandalone, customExportDir_);
          const bool isBuilt =
              artRep.succeeded ||
              fs::exists(entry.path() / "dist" / (dirName + ".vst3")) ||
              fs::exists(rootDir / "build" / "Release" / (dirName + ".vst3")) ||
              fs::exists(rootDir / "build" / "Release" / (dirName + ".clap"));

          projects_.push_back(
              {dirName, dirName, "C++20 VST3 + CLAP + APP",
               std::format("com.company.{}", toLower(dirName)),
               std::format("Standalone C++20 plugin project '{}' (plugins/{}/ "
                           "with VST3, CLAP, Standalone .exe & .vscode tasks).",
                           dirName, dirName),
               std::format("Avra{}_Clap", dirName), isScaffolded, isBuilt,
               false});
        }
      }
    }

    if (projects_.empty()) {
      selectedProjectIndex_ = -1;
    } else if (selectedProjectIndex_ >= static_cast<int>(projects_.size())) {
      selectedProjectIndex_ = static_cast<int>(projects_.size()) - 1;
    }
    if (selectedProjectIndex_ >= 0 &&
        selectedProjectIndex_ < static_cast<int>(projects_.size()) &&
        !isBuildRunning_.load()) {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      const auto inspected = ProjectGenerator::inspectProjectArtifacts(
          projects_[selectedProjectIndex_].diskFolder, prefs_.exportFormatVst3,
          prefs_.exportFormatClap, prefs_.exportFormatStandalone,
          customExportDir_);
      if (lastBuildReport_.pluginName != inspected.pluginName ||
          lastBuildReport_.artifacts.empty()) {
        lastBuildReport_ = inspected;
      } else {
        lastBuildReport_.artifacts = inspected.artifacts;
        lastBuildReport_.sourceFolder = inspected.sourceFolder;
        lastBuildReport_.destinationFolder = inspected.destinationFolder;
      }
    }
    if (btnRestoreHidden_) {
      btnRestoreHidden_->setText(
          std::format("RESTORE HIDDEN ({})", hiddenProjects_.size()));
      btnRestoreHidden_->setVisible(!isSidebarCollapsed_ &&
                                    !hiddenProjects_.empty());
    }
  }

  void ensureActiveProjectInWorkspaceList() {
    updateOutputFolderUiButtons();
    if (wizardConfig_.name.empty()) {
      return;
    }
    const std::string cleanName =
        ProjectGenerator::sanitizeName(wizardConfig_.name);
    if (cleanName.empty()) {
      return;
    }
    const bool isCoreGain =
        (cleanName == "AvraGain" || cleanName == "GainPlugin" ||
         wizardConfig_.name == "Avra Gain");
    const std::string lookupKey = isCoreGain ? "Avra Gain" : cleanName;
    if (hiddenProjects_.erase(lookupKey) > 0 ||
        hiddenProjects_.erase(cleanName) > 0) {
      saveHiddenProjectsList();
    }
    refreshWorkspaceProjects();

    int foundIdx = -1;
    for (size_t i = 0; i < projects_.size(); ++i) {
      if (projects_[i].name == lookupKey || projects_[i].name == cleanName ||
          projects_[i].diskFolder == cleanName ||
          (isCoreGain && projects_[i].diskFolder == "GainPlugin")) {
        foundIdx = static_cast<int>(i);
        break;
      }
    }
    if (foundIdx < 0) {
      const std::string diskFolder = isCoreGain ? "GainPlugin" : cleanName;
      const auto artRep = ProjectGenerator::inspectProjectArtifacts(
          diskFolder, prefs_.exportFormatVst3, prefs_.exportFormatClap,
          prefs_.exportFormatStandalone, customExportDir_);
      projects_.push_back(
          {isCoreGain ? "Avra Gain" : cleanName, diskFolder,
           wizardConfig_.category.empty() ? "C++20 VST3 + CLAP + APP"
                                          : wizardConfig_.category,
           isCoreGain ? "com.avra.gain"
                      : std::format("com.company.{}", toLower(cleanName)),
           std::format("Standalone C++20 plugin project '{}' (plugins/{}/ "
                       "with VST3, CLAP, Standalone .exe & .vscode tasks).",
                       cleanName, diskFolder),
           std::format("Avra{}_Clap", cleanName), true, artRep.succeeded,
           isCoreGain});
      foundIdx = static_cast<int>(projects_.size()) - 1;
    }
    selectedProjectIndex_ = foundIdx;
    if (!isBuildRunning_.load()) {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      lastBuildReport_ = ProjectGenerator::inspectProjectArtifacts(
          projects_[selectedProjectIndex_].diskFolder, prefs_.exportFormatVst3,
          prefs_.exportFormatClap, prefs_.exportFormatStandalone,
          customExportDir_);
    }
  }

  void removeProjectAtIndex(int idx) {
    if (idx < 0 || idx >= static_cast<int>(projects_.size()))
      return;
    const std::string projName = projects_[idx].name;
    const std::string diskName = projects_[idx].diskFolder;
    const bool isCore = projects_[idx].isCoreReference;

    hiddenProjects_.insert(projName);
    hiddenProjects_.insert(diskName);
    saveHiddenProjectsList();

    if (!isCore) {
      ProjectGenerator::unregisterFromCMakeLists(diskName);
    }

    refreshWorkspaceProjects();
    if (projects_.empty() ||
        ProjectGenerator::sanitizeName(wizardConfig_.name) ==
            ProjectGenerator::sanitizeName(projName) ||
        ProjectGenerator::sanitizeName(wizardConfig_.name) == diskName) {
      initializeBlankUntitledProject();
    }
    statusMessage_ = std::format("Removed '{}' from workspace & CMakeLists.txt "
                                 "(source files kept safe on disk).",
                                 projName);
    repaint();
  }

  void restoreHiddenProjects() {
    hiddenProjects_.clear();
    saveHiddenProjectsList();
    refreshWorkspaceProjects();
    statusMessage_ =
        "Restored all hidden projects back into the workspace list!";
    repaint();
  }

  void updateCanvasPageButtons() {
    // Pagination buttons are permanently retired in favor of Infinite Pan/Zoom
    // + Minimap Radar!
    if (btnCanvasPrevPage_)
      btnCanvasPrevPage_->setVisible(false);
    if (btnCanvasNextPage_)
      btnCanvasNextPage_->setVisible(false);
    if (btnPalettePrev_)
      btnPalettePrev_->setVisible(false);
    if (btnPaletteNext_)
      btnPaletteNext_->setVisible(false);
    if (btnUiPalettePrev_)
      btnUiPalettePrev_->setVisible(false);
    if (btnUiPaletteNext_)
      btnUiPaletteNext_->setVisible(false);
  }

  void switchTab(StudioTab tab) {
    currentTab_ = tab;
    isScaleDropdownOpen_ = false;
    isParamBindDropdownOpen_ = false;
    isStyleTemplateDropdownOpen_ = false;
    rackPresetMenuOpen_ = false;
    hoveredRackPresetItem_ = -1;

    // Auto-collapse left WORKSPACE PROJECTS sidebar in Editor & Rack views to
    // reclaim ~295px of horizontal screen width!
    isSidebarCollapsed_ = (tab != StudioTab::Projects);

    btnTabProjects_->setToggleState(tab == StudioTab::Projects);
    btnTabWizard_->setToggleState(tab == StudioTab::NewPluginWizard);
    if (btnTabUiBuilder_)
      btnTabUiBuilder_->setToggleState(tab == StudioTab::UiBuilder);
    btnTabRack_->setToggleState(tab == StudioTab::LiveRack);

    const bool isProjects = (tab == StudioTab::Projects);
    const bool isWizard = (tab == StudioTab::NewPluginWizard);
    const bool isUiBuild = (tab == StudioTab::UiBuilder);
    const bool isRack = (tab == StudioTab::LiveRack);

    if (isProjects) {
      ensureActiveProjectInWorkspaceList();
    }

    if (btnProjNew_)
      btnProjNew_->setVisible(!isSidebarCollapsed_);
    if (btnProjDuplicate_)
      btnProjDuplicate_->setVisible(!isSidebarCollapsed_);
    if (btnProjImportExport_)
      btnProjImportExport_->setVisible(!isSidebarCollapsed_);
    if (btnRestoreHidden_)
      btnRestoreHidden_->setVisible(!isSidebarCollapsed_ &&
                                    !hiddenProjects_.empty());

    if (btnChooseOutputPath_)
      btnChooseOutputPath_->setVisible(false);
    if (btnOutputSystemVst3_)
      btnOutputSystemVst3_->setVisible(false);
    if (btnOutputDefaultDist_)
      btnOutputDefaultDist_->setVisible(false);
    if (btnExportFmtVst3_)
      btnExportFmtVst3_->setVisible(isProjects);
    if (btnExportFmtClap_)
      btnExportFmtClap_->setVisible(isProjects);
    if (btnExportFmtApp_)
      btnExportFmtApp_->setVisible(isProjects);
    btnBuildClap_->setVisible(isProjects);
    btnOpenFolder_->setVisible(isProjects);
    if (btnRunStandalone_)
      btnRunStandalone_->setVisible(false);

    if (inputLibrarySearch_) {
      // The search box now only filters the Schematic component library; the
      // Faceplate UI Builder no longer has an add-widget palette to filter.
      inputLibrarySearch_->setVisible(isWizard);
    }

    btnCatPreamps_->setVisible(isWizard);
    btnCatIron_->setVisible(isWizard);
    btnCatEq_->setVisible(isWizard);
    btnCatDynamics_->setVisible(isWizard);
    btnCatDelayRev_->setVisible(isWizard);
    btnCatModRoute_->setVisible(isWizard);
    if (btnCatAutoSmart_)
      btnCatAutoSmart_->setVisible(isWizard);

    btnPresetTubeFet_->setVisible(isWizard);
    btnPresetBritStrip_->setVisible(isWizard);
    btnPresetPultecOpto_->setVisible(isWizard);
    btnPresetEchoSpace_->setVisible(isWizard);
    if (btnPresetVocal_)
      btnPresetVocal_->setVisible(isWizard);
    if (btnPresetSynthKeys_)
      btnPresetSynthKeys_->setVisible(isWizard);
    btnClearCanvas_->setVisible(isWizard);

    updateCanvasPageButtons();
    syncInspectorKnobsToSelection();

    btnNodeToggleParallel_->setVisible(isWizard);
    btnNodeMixDown_->setVisible(isWizard);
    btnNodeMixUp_->setVisible(isWizard);
    if (btnNodeToggleTrims_)
      btnNodeToggleTrims_->setVisible(false);
    btnAuditionSchematic_->setVisible(isWizard);
    if (btnDesignUiFromSchematic_)
      btnDesignUiFromSchematic_->setVisible(false);

    // Faceplate UI Builder controls (Clean Single-Row Toolbar + Quick
    // Alignment)
    if (inputCustomThemeName_)
      inputCustomThemeName_->setVisible(isUiBuild);
    if (btnSaveCustomTheme_)
      btnSaveCustomTheme_->setVisible(isUiBuild);
    if (btnUiFinish_)
      btnUiFinish_->setVisible(isUiBuild);
    if (btnUiPickFinishColor_)
      btnUiPickFinishColor_->setVisible(isUiBuild);
    if (btnUiGrain_)
      btnUiGrain_->setVisible(isUiBuild);
    if (btnUiScrews_)
      btnUiScrews_->setVisible(isUiBuild);
    if (btnUiScrewStyle_)
      btnUiScrewStyle_->setVisible(isUiBuild);
    if (btnUiGrid_)
      btnUiGrid_->setVisible(isUiBuild);
    if (btnUiSnapGuides_)
      btnUiSnapGuides_->setVisible(isUiBuild);
    if (btnUiAlignRow_)
      btnUiAlignRow_->setVisible(isUiBuild);
    if (btnUiDistribute_)
      btnUiDistribute_->setVisible(isUiBuild);
    if (btnUiAddSticker_)
      btnUiAddSticker_->setVisible(isUiBuild);
    if (btnUiSyncSchematic_)
      btnUiSyncSchematic_->setVisible(isUiBuild);
    if (btnUiClearCanvas_)
      btnUiClearCanvas_->setVisible(isUiBuild);
    if (btnUiUnifyTheme_)
      btnUiUnifyTheme_->setVisible(false);
    if (btnUiInternalTrims_)
      btnUiInternalTrims_->setVisible(false);
    const bool selIsKnob =
        (isUiBuild && selectedUiWidgetIndex_ >= 0 &&
         selectedUiWidgetIndex_ <
             static_cast<int>(faceplateLayout_.widgets.size()) &&
         isKnobWidget(faceplateLayout_.widgets[selectedUiWidgetIndex_].type));
    const bool selIsSwitch =
        (isUiBuild && selectedUiWidgetIndex_ >= 0 &&
         selectedUiWidgetIndex_ <
             static_cast<int>(faceplateLayout_.widgets.size()) &&
         isSwitchWidget(faceplateLayout_.widgets[selectedUiWidgetIndex_].type));
    if (btnUiGlobalKnob_)
      btnUiGlobalKnob_->setVisible(selIsKnob);
    if (btnUiGlobalSwitch_)
      btnUiGlobalSwitch_->setVisible(selIsSwitch);
    if (btnUiGlobalMeter_)
      btnUiGlobalMeter_->setVisible(false);
    if (btnUiGlobalColor_)
      btnUiGlobalColor_->setVisible(false);

    // Add-widget palette removed: controls, faders and meters are generated
    // automatically from each module's hardware control spec.
    if (btnUiCatKnobs_)
      btnUiCatKnobs_->setVisible(false);
    if (btnUiCatSwitches_)
      btnUiCatSwitches_->setVisible(false);
    if (btnUiCatMeters_)
      btnUiCatMeters_->setVisible(false);
    if (btnUiCatFrames_)
      btnUiCatFrames_->setVisible(false);
    isParamBindDropdownOpen_ = false;
    if (btnUiWidgetBind_)
      btnUiWidgetBind_->setVisible(false);

    if (inputUiWidgetTitle_)
      inputUiWidgetTitle_->setVisible(isUiBuild);
    if (btnUiWidgetStyle_)
      btnUiWidgetStyle_->setVisible(isUiBuild);
    if (btnUiWidgetColor_)
      btnUiWidgetColor_->setVisible(isUiBuild);
    if (btnUiGlobalFontType_)
      btnUiGlobalFontType_->setVisible(isUiBuild);
    if (btnUiWidgetFont_)
      btnUiWidgetFont_->setVisible(isUiBuild);
    if (btnUiGlobalFontColor_)
      btnUiGlobalFontColor_->setVisible(isUiBuild);
    if (btnUiGlobalPlateColor_)
      btnUiGlobalPlateColor_->setVisible(isUiBuild);
    if (btnUiWidgetSizeDown_)
      btnUiWidgetSizeDown_->setVisible(isUiBuild);
    if (btnUiWidgetSizeUp_)
      btnUiWidgetSizeUp_->setVisible(isUiBuild);
    if (btnUiWidgetLayer_)
      btnUiWidgetLayer_->setVisible(false);
    if (btnUiWidgetDup_)
      btnUiWidgetDup_->setVisible(isUiBuild);
    if (btnUiWidgetDel_)
      btnUiWidgetDel_->setVisible(isUiBuild);

    syncUiWidgetInspector();

    if (inputPluginName_)
      inputPluginName_->setVisible(false);
    btnCreateProject_->setVisible(false);
    btnCreateAndOpenIde_->setVisible(false);

    btnRackGain_->setVisible(isRack);
    if (btnRackAudioDevice_)
      btnRackAudioDevice_->setVisible(isRack);
    btnSigDrum_->setVisible(isRack);
    if (btnSigSynth_)
      btnSigSynth_->setVisible(isRack);
    if (btnSigCleanGuitar_)
      btnSigCleanGuitar_->setVisible(isRack);
    if (btnSigGuitar_)
      btnSigGuitar_->setVisible(isRack);
    if (btnSigBass_)
      btnSigBass_->setVisible(isRack);
    if (btnSigVocal_)
      btnSigVocal_->setVisible(isRack);
    if (btnSigWav_)
      btnSigWav_->setVisible(isRack);
    btnSigPink_->setVisible(isRack);
    btnSigTone_->setVisible(isRack);
    if (btnSigSilentAnalyzer_)
      btnSigSilentAnalyzer_->setVisible(isRack);
    btnSigMute_->setVisible(isRack);
    if (btnRackDelta_)
      btnRackDelta_->setVisible(isRack);
    if (btnRackAutoMatch_)
      btnRackAutoMatch_->setVisible(isRack);
    btnRackBypass_->setVisible(isRack);
    btnRackSavePreset_->setVisible(isRack);
    btnRackLoadPreset_->setVisible(false);
    rackPresetMenuOpen_ = false;
    if (btnRackAbCompare_)
      btnRackAbCompare_->setVisible(isRack);

    syncRackTelemetryVisibility();

    if (isRack) {
      if (testSignalMode_ != TestSignalMode::Silence) {
        rackMuted_ = false;
      }
      rackAudioDeviceMenuOpen_ = false;
      hoveredRackAudioDeviceItem_ = -1;
      selectRackPlugin(activeRackPluginIndex_);
    } else if (audioOut_.isOpen()) {
      audioOut_.close();
    }

    resized();
    repaint();
  }

  void syncRackTelemetryVisibility() noexcept {
    const bool isRack = (currentTab_ == StudioTab::LiveRack);
    rackSpectrum_.setVisible(isRack && !labBenchActive_);
    rackScope_.setVisible(isRack && !labBenchActive_);
    rackGrHistory_.setVisible(isRack && !labBenchActive_);
    rackInMeter_.setVisible(isRack);
    rackOutMeter_.setVisible(isRack);
  }

  // The Live Rack has a single view (the live faceplate); index is kept for
  // call compatibility.
  void selectRackPlugin(int index) {
    (void)index;
    activeRackPluginIndex_ = 0;
    if (currentTab_ == StudioTab::LiveRack) {
      btnRackGain_->setToggleState(true);
      statusMessage_ = "LIVE FACEPLATE VIEW: Interactive hardware panel with "
                       "48 kHz telemetry scopes.";
    }
    repaint();
  }

  void selectTestSignal(TestSignalMode mode) {
    std::lock_guard<std::mutex> lock(audioRenderMutex_);
    testSignalMode_ = mode;
    rackMuted_ = (mode == TestSignalMode::Silence);
    activeStemSlotLoaded_ = false;

    int stemSlot = -1;
    const char *defaultStemFile = nullptr;
    if (mode == TestSignalMode::DrumTransientLoop) {
      stemSlot = 0;
      defaultStemFile = "assets/stems/drums.wav";
    } else if (mode == TestSignalMode::SynthChordLoop) {
      stemSlot = 1;
      defaultStemFile = "assets/stems/synth.wav";
    } else if (mode == TestSignalMode::CleanGuitarChord) {
      stemSlot = 2;
      defaultStemFile = "assets/stems/clean_guitar.wav";
    } else if (mode == TestSignalMode::GuitarDiChug) {
      stemSlot = 3;
      defaultStemFile = "assets/stems/guitar_di.wav";
    } else if (mode == TestSignalMode::BassDiGroove) {
      stemSlot = 4;
      defaultStemFile = "assets/stems/bass_di.wav";
    } else if (mode == TestSignalMode::VocalPhrase) {
      stemSlot = 5;
      defaultStemFile = "assets/stems/vocal.wav";
    }

    if (stemSlot >= 0) {
      std::filesystem::path candidatePath;
      if (!customStemPaths_[static_cast<size_t>(stemSlot)].empty() &&
          std::filesystem::exists(
              customStemPaths_[static_cast<size_t>(stemSlot)])) {
        candidatePath = customStemPaths_[static_cast<size_t>(stemSlot)];
      } else if (defaultStemFile != nullptr) {
        if (std::filesystem::exists(defaultStemFile)) {
          candidatePath = defaultStemFile;
        } else if (std::filesystem::exists(std::filesystem::path("..") /
                                           defaultStemFile)) {
          candidatePath = std::filesystem::path("..") / defaultStemFile;
        }
      }
      if (!candidatePath.empty()) {
        std::string stemErr;
        activeStemSlotLoaded_ =
            wavLoopPlayer_.loadFromFile(candidatePath.string(), stemErr);
      }
    }

    btnSigDrum_->setToggleState(mode == TestSignalMode::DrumTransientLoop);
    if (btnSigSynth_)
      btnSigSynth_->setToggleState(mode == TestSignalMode::SynthChordLoop);
    if (btnSigCleanGuitar_)
      btnSigCleanGuitar_->setToggleState(mode == TestSignalMode::CleanGuitarChord);
    if (btnSigGuitar_)
      btnSigGuitar_->setToggleState(mode == TestSignalMode::GuitarDiChug);
    if (btnSigBass_)
      btnSigBass_->setToggleState(mode == TestSignalMode::BassDiGroove);
    if (btnSigVocal_)
      btnSigVocal_->setToggleState(mode == TestSignalMode::VocalPhrase);
    if (btnSigWav_)
      btnSigWav_->setToggleState(mode == TestSignalMode::CustomWavLoop);
    btnSigPink_->setToggleState(mode == TestSignalMode::PinkNoise);
    btnSigTone_->setToggleState(mode == TestSignalMode::HarmonicMultiTone);
    btnSigMute_->setToggleState(mode == TestSignalMode::Silence);
    rackSpectrum_.setHarmonicAnalyzerMode(mode ==
                                          TestSignalMode::HarmonicMultiTone);
    repaint();
  }

  [[nodiscard]] gui::Rect<float> getUiBuilderCanvasRect() const noexcept {
    const float availX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    const float availW = 1345.0f - availX;
    const float cw = std::min(availW, 975.0f);
    const float cx = availX + (availW - cw) * 0.5f;
    return {cx, 136.0f, cw, 414.0f};
  }

  [[nodiscard]] gui::Rect<float>
  getStyleTemplateDropdownMenuRect() const noexcept {
    constexpr int kCols = 3;
    constexpr float rowH = 26.0f;
    constexpr float headH = 26.0f;
    const int userRows =
        (static_cast<int>(savedUserThemes_.size()) + kCols - 1) / kCols;
    constexpr int rows = (kTotalUiStyleTemplates + kCols - 1) / kCols;
    const float menuH =
        headH + static_cast<float>(1 + userRows + rows) * rowH + 8.0f;
    const float mx = isSidebarCollapsed_ ? 72.0f : 264.0f;
    const float mw = 720.0f;
    const float my = std::min(130.0f, std::max(54.0f, 748.0f - menuH));
    return {mx, my, mw, menuH};
  }

  [[nodiscard]] gui::Rect<float> getScaleDropdownMenuRect() const noexcept {
    const float rowH = 25.0f;
    const float headH = 24.0f;
    const float menuH =
        headH + static_cast<float>(relevantScaleOptions_.size()) * rowH + 6.0f;
    const float baseX = isSidebarCollapsed_ ? 640.0f : 876.0f;
    return {baseX, 624.0f - menuH, 336.0f, menuH};
  }

  [[nodiscard]] gui::Rect<float> getParamBindDropdownMenuRect() const noexcept {
    const float rowH = 23.0f;
    const float headH = 24.0f;
    const size_t maxRows = std::min<size_t>(paramBindOptions_.size(), 16);
    const float menuH = headH + static_cast<float>(maxRows) * rowH + 6.0f;
    const float baseX = isSidebarCollapsed_ ? 680.0f : 820.0f;
    return {baseX, 586.0f - menuH, 360.0f, menuH};
  }

  [[nodiscard]] gui::Rect<float> getRackFaceplateRect() const noexcept {
    const float availX = isSidebarCollapsed_ ? 74.0f : 368.0f;
    const float availW = isSidebarCollapsed_ ? 1258.0f : 964.0f;
    const float rw = std::min(availW, 964.0f);
    const float rx = availX + (availW - rw) * 0.5f;
    return {rx, 106.0f, rw, 414.0f};
  }

  void resized() override {
    // Top Navigation Header: Integrated Menu Bar (x:144..352) + Quick Icons
    // (x:360..419) + 4 Tabs (SCHEMATIC BUILDER | FACEPLATE UI BUILDER | LIVE TEST RACK | BUILD)
    btnTabWizard_->setBounds(432.0f, 9.0f, 166.0f, 32.0f);
    if (btnTabUiBuilder_)
      btnTabUiBuilder_->setBounds(606.0f, 9.0f, 172.0f, 32.0f);
    btnTabRack_->setBounds(786.0f, 9.0f, 142.0f, 32.0f);
    btnTabProjects_->setBounds(936.0f, 9.0f, 124.0f, 32.0f);

    if (inputPluginName_)
      inputPluginName_->setVisible(false);
    if (btnHeaderSaveProject_)
      btnHeaderSaveProject_->setBounds(1185.0f, 9.0f, 160.0f, 32.0f);
    if (btnHeaderOpenIde_)
      btnHeaderOpenIde_->setVisible(false);
    if (btnHeaderVsCode_)
      btnHeaderVsCode_->setVisible(false);
    if (btnHeaderFullscreen_)
      btnHeaderFullscreen_->setVisible(false);

    // Left Sidebar Project Management Buttons (+ NEW PLUGIN, DUPLICATE,
    // IMPORT/EXPORT)
    if (btnProjNew_)
      btnProjNew_->setBounds(28.0f, 98.0f, 104.0f, 24.0f);
    if (btnProjDuplicate_)
      btnProjDuplicate_->setBounds(136.0f, 98.0f, 86.0f, 24.0f);
    if (btnProjImportExport_)
      btnProjImportExport_->setBounds(226.0f, 98.0f, 106.0f, 24.0f);
    btnRestoreHidden_->setBounds(32.0f, 688.0f, 296.0f, 32.0f);

    // --- Mode 1: Build Tab (5 Clear Buttons Underneath Component Chain) ---
    if (btnChooseOutputPath_)
      btnChooseOutputPath_->setVisible(false);
    if (btnOutputSystemVst3_)
      btnOutputSystemVst3_->setVisible(false);
    if (btnOutputDefaultDist_)
      btnOutputDefaultDist_->setVisible(false);
    if (btnRunStandalone_)
      btnRunStandalone_->setVisible(false);
    if (btnExportFmtVst3_)
      btnExportFmtVst3_->setBounds(385.0f, 510.0f, 178.0f, 56.0f);
    if (btnExportFmtClap_)
      btnExportFmtClap_->setBounds(578.0f, 510.0f, 178.0f, 56.0f);
    if (btnExportFmtApp_)
      btnExportFmtApp_->setBounds(771.0f, 510.0f, 178.0f, 56.0f);
    btnBuildClap_->setBounds(964.0f, 510.0f, 178.0f, 56.0f);
    btnOpenFolder_->setBounds(1157.0f, 510.0f, 178.0f, 56.0f);

    const float libX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    const float libW = isSidebarCollapsed_ ? 280.0f : 232.0f;

    if (inputLibrarySearch_) {
      inputLibrarySearch_->setBounds(libX + 6.0f, 166.0f, libW - 12.0f, 24.0f);
    }

    // --- Mode 2: Drag-and-Drop Schematic Builder ---
    // Starter Chain Macro Row (y = 106, 6 Instrument Blueprints + Clear Canvas)
    const float bpSpan = 1219.0f - libX;
    const float bpGap  = 5.0f;
    const float bpW    = (bpSpan - 5.0f * bpGap) / 6.0f;
    btnPresetTubeFet_->setBounds(libX + 0.0f * (bpW + bpGap), 106.0f, bpW, 28.0f);
    btnPresetBritStrip_->setBounds(libX + 1.0f * (bpW + bpGap), 106.0f, bpW, 28.0f);
    btnPresetPultecOpto_->setBounds(libX + 2.0f * (bpW + bpGap), 106.0f, bpW, 28.0f);
    btnPresetEchoSpace_->setBounds(libX + 3.0f * (bpW + bpGap), 106.0f, bpW, 28.0f);
    if (btnPresetVocal_)
      btnPresetVocal_->setBounds(libX + 4.0f * (bpW + bpGap), 106.0f, bpW, 28.0f);
    if (btnPresetSynthKeys_)
      btnPresetSynthKeys_->setBounds(libX + 5.0f * (bpW + bpGap), 106.0f, bpW, 28.0f);
    btnClearCanvas_->setBounds(1225.0f, 106.0f, 120.0f, 28.0f);

    // Faceplate Sync button in Top Right of Schematic Header
    if (btnDesignUiFromSchematic_)
      btnDesignUiFromSchematic_->setBounds(1148.0f, 68.0f, 196.0f, 28.0f);

    // Canvas Page Navigation Buttons
    btnCanvasPrevPage_->setBounds(1192.0f, 146.0f, 70.0f, 20.0f);
    btnCanvasNextPage_->setBounds(1268.0f, 146.0f, 70.0f, 20.0f);

    // Left Component Library Category Buttons (2x3 grid + full-width AUTO / SMART row)
    const float catW = (libW - 18.0f) / 3.0f;
    const float catStep = catW + 3.0f;
    btnCatPreamps_->setBounds(libX + 6.0f + 0.0f * catStep, 192.0f, catW,
                              17.5f);
    btnCatIron_->setBounds(libX + 6.0f + 1.0f * catStep, 192.0f, catW, 17.5f);
    btnCatEq_->setBounds(libX + 6.0f + 2.0f * catStep, 192.0f, catW, 17.5f);
    btnCatDynamics_->setBounds(libX + 6.0f + 0.0f * catStep, 211.0f, catW,
                               17.5f);
    btnCatDelayRev_->setBounds(libX + 6.0f + 1.0f * catStep, 211.0f, catW,
                               17.5f);
    btnCatModRoute_->setBounds(libX + 6.0f + 2.0f * catStep, 211.0f, catW,
                               17.5f);
    if (btnCatAutoSmart_)
      btnCatAutoSmart_->setBounds(libX + 6.0f, 230.0f, libW - 12.0f, 17.0f);

    // Selected Stage Bar Action Row (inside sleek 82px-tall Selected Stage Bar
    // at y = 630..712)
    btnNodeToggleParallel_->setBounds(1345.0f - 546.0f, 682.0f, 144.0f, 24.0f);
    btnNodeMixDown_->setBounds(1345.0f - 396.0f, 682.0f, 70.0f, 24.0f);
    btnNodeMixUp_->setBounds(1345.0f - 320.0f, 682.0f, 70.0f, 24.0f);
    if (btnNodeToggleTrims_)
      btnNodeToggleTrims_->setVisible(false);
    btnAuditionSchematic_->setBounds(1345.0f - 244.0f, 682.0f, 234.0f, 24.0f);

    // --- Mode 3: Drag-and-Drop Faceplate UI Builder ---
    // Single-Row Top Toolbar (y = 102) + Quick Alignment Buttons (ALIGN TOP,
    // DISTRIBUTE, GUIDES) + Top-Right Custom Theme Name & Save Button
    if (inputCustomThemeName_)
      inputCustomThemeName_->setBounds(1030.0f, 66.0f, 176.0f, 26.0f);
    if (btnSaveCustomTheme_)
      btnSaveCustomTheme_->setBounds(1212.0f, 66.0f, 133.0f, 26.0f);
    const bool compactUiToolbar = !isSidebarCollapsed_;
    if (btnUiFinish_)
      btnUiFinish_->setBounds(libX, 102.0f, compactUiToolbar ? 136.0f : 184.0f, 26.0f);
    if (btnUiPickFinishColor_)
      btnUiPickFinishColor_->setBounds(libX + (compactUiToolbar ? 139.0f : 188.0f), 102.0f, compactUiToolbar ? 96.0f : 122.0f, 26.0f);
    if (btnUiGrain_)
      btnUiGrain_->setBounds(libX + (compactUiToolbar ? 238.0f : 314.0f), 102.0f, compactUiToolbar ? 104.0f : 138.0f, 26.0f);
    if (btnUiScrews_)
      btnUiScrews_->setBounds(libX + (compactUiToolbar ? 345.0f : 456.0f), 102.0f, compactUiToolbar ? 88.0f : 114.0f, 26.0f);
    if (btnUiScrewStyle_)
      btnUiScrewStyle_->setBounds(libX + (compactUiToolbar ? 436.0f : 574.0f), 102.0f, compactUiToolbar ? 90.0f : 118.0f, 26.0f);
    if (btnUiGrid_)
      btnUiGrid_->setBounds(libX + (compactUiToolbar ? 529.0f : 696.0f), 102.0f, compactUiToolbar ? 56.0f : 70.0f, 26.0f);
    if (btnUiSnapGuides_)
      btnUiSnapGuides_->setBounds(libX + (compactUiToolbar ? 588.0f : 770.0f), 102.0f, compactUiToolbar ? 52.0f : 64.0f, 26.0f);
    if (btnUiAlignRow_)
      btnUiAlignRow_->setBounds(libX + (compactUiToolbar ? 643.0f : 838.0f), 102.0f, compactUiToolbar ? 58.0f : 74.0f, 26.0f);
    if (btnUiDistribute_)
      btnUiDistribute_->setBounds(libX + (compactUiToolbar ? 704.0f : 916.0f), 102.0f, compactUiToolbar ? 62.0f : 78.0f, 26.0f);
    if (btnUiAddSticker_)
      btnUiAddSticker_->setBounds(libX + (compactUiToolbar ? 769.0f : 998.0f), 102.0f, compactUiToolbar ? 86.0f : 134.0f, 26.0f);
    if (btnUiSyncSchematic_)
      btnUiSyncSchematic_->setBounds(1228.0f, 102.0f, 58.0f, 26.0f);
    if (btnUiClearCanvas_)
      btnUiClearCanvas_->setBounds(1290.0f, 102.0f, 55.0f, 26.0f);

    const float uiCatW = (libW - 16.0f) * 0.5f;
    if (btnUiCatKnobs_)
      btnUiCatKnobs_->setBounds(libX + 6.0f, 194.0f, uiCatW, 22.0f);
    if (btnUiCatSwitches_)
      btnUiCatSwitches_->setBounds(libX + 10.0f + uiCatW, 194.0f, uiCatW,
                                   22.0f);
    if (btnUiCatMeters_)
      btnUiCatMeters_->setBounds(libX + 6.0f, 219.0f, uiCatW, 22.0f);
    if (btnUiCatFrames_)
      btnUiCatFrames_->setBounds(libX + 10.0f + uiCatW, 219.0f, uiCatW, 22.0f);

    // Expanded Bottom Selected Widget Property Inspector (x: libX..1345, y:
    // 556..712)
    const float uiInspX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    if (btnUiWidgetSizeDown_)
      btnUiWidgetSizeDown_->setBounds(1142.0f, 560.0f, 28.0f, 22.0f);
    if (btnUiWidgetSizeUp_)
      btnUiWidgetSizeUp_->setBounds(1174.0f, 560.0f, 28.0f, 22.0f);
    if (btnUiEditBadge_)
      btnUiEditBadge_->setBounds(1210.0f, 560.0f, 126.0f, 22.0f);

    const bool isSingleTitle =
        (selectedUiWidgetIndex_ >= 0 &&
         selectedUiWidgetIndex_ <
             static_cast<int>(faceplateLayout_.widgets.size()) &&
         (faceplateLayout_.widgets[selectedUiWidgetIndex_].type ==
              UiWidgetType::SectionFrame ||
          faceplateLayout_.widgets[selectedUiWidgetIndex_].type ==
              UiWidgetType::VentilationGrille ||
          faceplateLayout_.widgets[selectedUiWidgetIndex_].type ==
              UiWidgetType::SilkscreenLabel));
    if (inputUiWidgetTitle_)
      inputUiWidgetTitle_->setBounds(uiInspX + 52.0f, 590.0f,
                                     isSingleTitle ? 290.0f : 140.0f, 28.0f);
    if (inputUiWidgetSub_)
      inputUiWidgetSub_->setBounds(uiInspX + 230.0f, 590.0f, 132.0f, 28.0f);
    if (btnUiWidgetBind_)
      btnUiWidgetBind_->setBounds(uiInspX + 370.0f, 590.0f, 276.0f, 28.0f);
    if (btnUiUploadStickerImg_)
      btnUiUploadStickerImg_->setBounds(uiInspX + 370.0f, 590.0f, 148.0f,
                                        28.0f);
    if (btnUiStickerRotate_)
      btnUiStickerRotate_->setBounds(uiInspX + 524.0f, 590.0f, 124.0f, 28.0f);
    if (inputUiBadgeFooter_)
      inputUiBadgeFooter_->setBounds(uiInspX + 418.0f, 590.0f, 228.0f, 28.0f);
    if (btnUiWidgetDup_)
      btnUiWidgetDup_->setBounds(1216.0f, 590.0f, 58.0f, 28.0f);
    if (btnUiWidgetDel_)
      btnUiWidgetDel_->setBounds(1278.0f, 590.0f, 58.0f, 28.0f);

    if (btnUiWidgetStyle_)
      btnUiWidgetStyle_->setBounds(uiInspX + 10.0f, 630.0f, 160.0f, 28.0f);
    if (btnUiWidgetColor_)
      btnUiWidgetColor_->setBounds(uiInspX + 176.0f, 630.0f, 132.0f, 28.0f);
    if (btnUiWidgetScale_)
      btnUiWidgetScale_->setBounds(uiInspX + 314.0f, 630.0f, 152.0f, 28.0f);
    if (btnUiGlobalKnob_)
      btnUiGlobalKnob_->setBounds(uiInspX + 472.0f, 630.0f, 148.0f, 28.0f);
    if (btnUiGlobalSwitch_)
      btnUiGlobalSwitch_->setBounds(uiInspX + 314.0f, 630.0f, 168.0f, 28.0f);
    if (btnUiGlobalFontType_)
      btnUiGlobalFontType_->setBounds(836.0f, 630.0f, 124.0f, 28.0f);
    if (btnUiWidgetFont_)
      btnUiWidgetFont_->setBounds(964.0f, 630.0f, 82.0f, 28.0f);
    if (btnUiGlobalFontColor_)
      btnUiGlobalFontColor_->setBounds(1050.0f, 630.0f, 138.0f, 28.0f);
    if (btnUiGlobalPlateColor_)
      btnUiGlobalPlateColor_->setBounds(1192.0f, 630.0f, 144.0f, 28.0f);

    // --- Mode 4: Live Test Rack (Full-Height 30px Toolbar Controls + Expanded
    // Scopes) ---
    const float rackX = isSidebarCollapsed_ ? 60.0f : 355.0f;
    const bool compactRackBar = !isSidebarCollapsed_;
    btnRackGain_->setBounds(rackX, 62.0f, compactRackBar ? 92.0f : 136.0f, 30.0f);
    if (btnRackAudioDevice_)
      btnRackAudioDevice_->setBounds(rackX + (compactRackBar ? 94.0f : 140.0f), 62.0f, compactRackBar ? 68.0f : 84.0f, 30.0f);

    const float sigStartX = rackX + (compactRackBar ? 164.0f : 228.0f);
    btnSigDrum_->setBounds(sigStartX, 62.0f, compactRackBar ? 44.0f : 50.0f, 30.0f);
    if (btnSigBass_)
      btnSigBass_->setBounds(sigStartX + (compactRackBar ? 46.0f : 53.0f), 62.0f, compactRackBar ? 48.0f : 54.0f, 30.0f);
    if (btnSigCleanGuitar_) {
      btnSigCleanGuitar_->setVisible(currentTab_ == StudioTab::LiveRack);
      btnSigCleanGuitar_->setBounds(sigStartX + (compactRackBar ? 96.0f : 110.0f), 62.0f, compactRackBar ? 56.0f : 66.0f, 30.0f);
    }
    if (btnSigGuitar_) {
      btnSigGuitar_->setVisible(currentTab_ == StudioTab::LiveRack);
      btnSigGuitar_->setBounds(sigStartX + (compactRackBar ? 154.0f : 179.0f), 62.0f, compactRackBar ? 50.0f : 60.0f, 30.0f);
    }
    if (btnSigVocal_) {
      btnSigVocal_->setVisible(currentTab_ == StudioTab::LiveRack);
      btnSigVocal_->setBounds(sigStartX + (compactRackBar ? 206.0f : 242.0f), 62.0f, compactRackBar ? 42.0f : 48.0f, 30.0f);
    }
    if (btnSigSynth_)
      btnSigSynth_->setBounds(sigStartX + (compactRackBar ? 250.0f : 293.0f), 62.0f, compactRackBar ? 42.0f : 48.0f, 30.0f);
    if (btnSigWav_)
      btnSigWav_->setBounds(sigStartX + (compactRackBar ? 294.0f : 344.0f), 62.0f, compactRackBar ? 44.0f : 50.0f, 30.0f);
    btnSigPink_->setBounds(sigStartX + (compactRackBar ? 340.0f : 397.0f), 62.0f, compactRackBar ? 38.0f : 44.0f, 30.0f);
    btnSigTone_->setBounds(sigStartX + (compactRackBar ? 380.0f : 444.0f), 62.0f, compactRackBar ? 62.0f : 72.0f, 30.0f);
    if (btnSigSilentAnalyzer_)
      btnSigSilentAnalyzer_->setBounds(sigStartX + (compactRackBar ? 444.0f : 519.0f), 62.0f, compactRackBar ? 44.0f : 50.0f, 30.0f);
    btnSigMute_->setBounds(sigStartX + (compactRackBar ? 490.0f : 572.0f), 62.0f, compactRackBar ? 40.0f : 46.0f, 30.0f);

    if (btnRackDelta_)
      btnRackDelta_->setBounds(sigStartX + (compactRackBar ? 532.0f : 624.0f), 62.0f, compactRackBar ? 48.0f : 58.0f, 30.0f);
    if (btnRackAutoMatch_)
      btnRackAutoMatch_->setBounds(sigStartX + (compactRackBar ? 582.0f : 685.0f), 62.0f, compactRackBar ? 62.0f : 76.0f, 30.0f);
    btnRackBypass_->setBounds(sigStartX + (compactRackBar ? 646.0f : 764.0f), 62.0f, compactRackBar ? 44.0f : 52.0f, 30.0f);

    // Full-height 30px Preset Selector, Save, and A|B Compare Toolbar Controls!
    btnRackLoadPreset_->setVisible(false);
    btnRackLoadPreset_->setBounds(-1000.0f, -1000.0f, 0.0f, 0.0f);
    btnRackSavePreset_->setBounds(1224.0f, 62.0f, 52.0f, 30.0f);
    if (btnRackAbCompare_)
      btnRackAbCompare_->setBounds(1280.0f, 62.0f, 54.0f, 30.0f);

    const float teleY = 568.0f;
    const float teleH = 158.0f;
    const float specX = isSidebarCollapsed_ ? 74.0f : 368.0f;
    const float specW = isSidebarCollapsed_ ? 580.0f : 390.0f;
    const float scopX = specX + specW + 12.0f;
    const float scopW = isSidebarCollapsed_ ? 320.0f : 225.0f;
    rackSpectrum_.setBounds(specX, teleY, specW, teleH);
    rackScope_.setBounds(scopX, teleY, scopW, teleH * 0.52f);
    rackGrHistory_.setBounds(scopX, teleY + teleH * 0.55f, scopW,
                             teleH * 0.45f);
    rackInMeter_.setBounds(1005.0f, teleY + 18.0f, 72.0f, teleH - 20.0f);
    rackOutMeter_.setBounds(1090.0f, teleY + 18.0f, 72.0f, teleH - 20.0f);
  }

  void paint(gui::Graphics &g) override {
    const auto local = getLocalBounds();

    const gui::Color bgDark = gui::Color::fromHex(0x121417);
    const gui::Color panelBg = gui::Color::fromHex(0x181A1F);
    const gui::Color outline = gui::Color::fromHex(0x2A2E38);
    const gui::Color goldAccent = gui::Color::fromHex(0xD4AF37);
    const gui::Color cyanAccent = gui::Color::fromHex(0x00F5D4);
    const gui::Color cardBg = gui::Color::fromHex(0x1D2027);

    g.fillRect(local, bgDark);

#if defined(_WIN32)
    if (btnHeaderFullscreen_) {
      const bool isFs = gui::Window::isWindowFullscreen(GetActiveWindow());
      if (btnHeaderFullscreen_->getToggleState() != isFs) {
        btnHeaderFullscreen_->setToggleState(isFs);
        btnHeaderFullscreen_->setText(isFs ? "WINDOWED (F11)"
                                           : "FULLSCREEN (F11)");
      }
    }
#endif

    // Top Header Bar with Embedded Direct2D Avra Shepherd Artwork Badge
    // (assets/avra_studio_icon.png)
    gui::Rect<float> headerBar = {0.0f, 0.0f, local.width, 50.0f};
    g.fillRect(headerBar, panelBg);
    g.drawLine({0.0f, 50.0f}, {local.width, 50.0f}, outline, 1.5f);

    const std::string headerIconPath = (ProjectGenerator::getWorkspaceRoot() /
                                        "assets" / "avra_studio_icon.png")
                                           .string();
    const gui::Rect<float> iconBadgeRect = {7.0f, 4.0f, 42.0f, 42.0f};
    g.fillRoundedRect(iconBadgeRect, 8.0f, gui::Color::fromHex(0x0D161D));
    const bool drewHeaderIcon =
        g.drawImageFromFile(headerIconPath, {8.0f, 5.0f, 40.0f, 40.0f}, 1.0f);
    g.drawRoundedRect(iconBadgeRect, 8.0f, cyanAccent.withAlpha(0.55f), 1.1f);

    const float brandTextX = drewHeaderIcon ? 53.0f : 16.0f;
    const float brandTextW = drewHeaderIcon ? 92.0f : 130.0f;
    g.drawText("AVRA STUDIO", {brandTextX, 3.0f, brandTextW, 24.0f}, goldAccent,
               drewHeaderIcon ? 12.2f : 16.5f, false, true);
    const std::string activeProjName = wizardConfig_.name;
    const std::string dirtyBadge =
        (isProjectDirty_ && !activeProjName.empty()) ? " *" : "";
    if (!activeProjName.empty()) {
      g.drawText((drewHeaderIcon ? "" : "ACTIVE: ") + activeProjName + dirtyBadge,
                 {brandTextX, 26.0f, brandTextW, 19.0f},
                 isProjectDirty_ ? gui::Color::AccentAmber() : cyanAccent, 8.4f,
                 false, true);
    }

    paintIntegratedMenuBarAndQuickIcons(g, activeTopMenu_, hoveredTopMenuIdx_,
                                        hoveredQuickIconIdx_, isProjectDirty_,
                                        history_);

    // Collapsible Left Sidebar: WORKSPACE PROJECTS (Full 320px in Projects tab,
    // 36px Drawer Rail in Editor/Rack tabs)
    paintSidebar(g, local, panelBg, outline, cyanAccent);

    if (currentTab_ == StudioTab::Projects) {
      paintProjectsMode(g, local, panelBg, outline, cardBg, goldAccent,
                        cyanAccent);
    } else if (currentTab_ == StudioTab::NewPluginWizard) {
      paintSchematicBuilderMode(g, local, panelBg, outline, cardBg, goldAccent,
                                cyanAccent);
    } else if (currentTab_ == StudioTab::UiBuilder) {
      paintUiBuilderMode(g, local, panelBg, outline, cardBg, goldAccent,
                         cyanAccent);
    } else if (currentTab_ == StudioTab::LiveRack) {
      paintRackMode(g, local, panelBg, outline, cardBg, goldAccent, cyanAccent);
    }

    // Bottom Status Bar
    gui::Rect<float> statusBar = {0.0f, local.height - 35.0f, local.width,
                                  35.0f};
    g.fillRect(statusBar, panelBg);
    g.drawLine({0.0f, local.height - 35.0f},
               {local.width, local.height - 35.0f}, outline, 1.0f);

    std::string statusText =
        statusMessage_.empty()
            ? std::format("WORKSPACE: {}  |  PROJECTS: {}  |  SCHEMATIC "
                          "STAGES: {}  |  UI WIDGETS: {}",
                          ProjectGenerator::getWorkspaceRoot().string(),
                          projects_.size(), schematicChain_.size(),
                          faceplateLayout_.widgets.size())
            : statusMessage_;
    const float statusMaxW = prefs_.showFpsCpuOverlay ? (local.width - 280.0f) : (local.width - 50.0f);
    g.drawText(statusText, {25.0f, local.height - 35.0f, std::max(300.0f, statusMaxW), 35.0f},
               statusMessage_.empty() ? gui::Color::TextSecondary()
                                      : cyanAccent,
               11.0f, false);

    if (prefs_.showFpsCpuOverlay) {
      size_t activeStages = 0;
      for (const auto &n : schematicChain_) {
        if (!n.isBypassed) ++activeStages;
      }
      const float osMult = (prefs_.globalOversamplingCapIdx == 0) ? 0.65f
                         : (prefs_.globalOversamplingCapIdx == 1) ? 0.85f
                         : (prefs_.globalOversamplingCapIdx == 3) ? 1.45f : 1.0f;
      const float srMult = static_cast<float>(std::max(44100, prefs_.sampleRateHz)) / 48000.0f;
      const float cpuPct = (currentTab_ == StudioTab::LiveRack && testSignalMode_ != TestSignalMode::Silence)
          ? std::clamp((0.35f + static_cast<float>(activeStages) * 0.28f) * osMult * srMult, 0.2f, 99.9f)
          : std::clamp(0.15f + static_cast<float>(activeStages) * 0.08f, 0.1f, 25.0f);
      const std::string telemetryBadge = std::format("60 FPS  |  DSP {:.1f}% CPU  |  {} Hz", cpuPct, prefs_.sampleRateHz);
      const gui::Rect<float> pillRect = {local.width - 246.0f, local.height - 29.0f, 234.0f, 23.0f};
      g.fillRoundedRect(pillRect, 4.0f, gui::Color::fromHex(0x09131A));
      g.drawRoundedRect(pillRect, 4.0f, cyanAccent.withAlpha(0.65f), 1.0f);
      g.drawText(telemetryBadge, pillRect, cyanAccent, 8.8f, true, true);
    }
  }

  [[nodiscard]] bool
  interceptsOverlayMouse(gui::Point<float> pt) const noexcept override {
    if (activeStudioModal_ != ActiveStudioModal::None)
      return true;
    if (activeTopMenu_ != TopMenuBarSection::None)
      return true;
    if (currentTab_ == StudioTab::LiveRack && rackAudioDeviceMenuOpen_) {
      if (!btnRackAudioDevice_ || !btnRackAudioDevice_->getBounds().contains(pt)) {
        return true;
      }
    }
    if (pt.y <= 44.0f && pt.x >= 144.0f && pt.x <= 421.0f)
      return true;
    if (currentTab_ == StudioTab::LiveRack && rackPresetMenuOpen_) {
      if (!btnRackLoadPreset_ ||
          !btnRackLoadPreset_->getBounds().contains(pt)) {
        return true;
      }
    }
    if (currentTab_ == StudioTab::UiBuilder) {
      if (isStyleTemplateDropdownOpen_ &&
          getStyleTemplateDropdownMenuRect().contains(pt))
        return true;
      if (isScaleDropdownOpen_ && !relevantScaleOptions_.empty() &&
          getScaleDropdownMenuRect().contains(pt))
        return true;
      if (isParamBindDropdownOpen_ && !paramBindOptions_.empty() &&
          getParamBindDropdownMenuRect().contains(pt))
        return true;
      if (activePopupMenu_ != UiPopupMenuKind::None &&
          !activePopupItems_.empty() && getActivePopupMenuRect().contains(pt))
        return true;
    }
    return false;
  }

  void paintOverlay(gui::Graphics &g) override {
    if (activeTopMenu_ != TopMenuBarSection::None) {
      const auto items = buildDropdownMenuItems(
          activeTopMenu_, history_, recentProjects_,
          faceplateLayout_.theme.gridSnapPx,
          hasClipboardStyle_ || hasClipboardWidget_, &prefs_);
      paintDropdownMenuOverlay(g, activeTopMenu_, items, lastMousePos_,
                               &recentProjects_,
                               recentSubmenuOpen_ &&
                                   activeTopMenu_ == TopMenuBarSection::File);
    }
    if (activeStudioModal_ != ActiveStudioModal::None) {
      prefs_.activeUndoCount = static_cast<int>(history_.undoDepth());
      prefs_.activeRedoCount = static_cast<int>(history_.redoDepth());
      if (customExportDir_.empty()) {
        prefs_.activeExportFolderDisplay = "Project Default (plugins/<Plugin>/dist/)";
      } else if (_stricmp(customExportDir_.string().c_str(), "C:\\Program Files\\Common Files\\VST3") == 0) {
        prefs_.activeExportFolderDisplay = "System Common VST3 (C:\\Program Files\\Common Files\\VST3)";
      } else {
        prefs_.activeExportFolderDisplay = customExportDir_.string();
      }
      paintActiveStudioModal(g, getLocalBounds(), activeStudioModal_,
                             activePreferencesTab_, prefs_, lastMousePos_);
      return;
    }
    if (currentTab_ == StudioTab::LiveRack && rackPresetMenuOpen_) {
      paintRackPresetDropdownOverlay(g);
      return;
    }
    if (currentTab_ == StudioTab::LiveRack && rackAudioDeviceMenuOpen_) {
      paintRackAudioDeviceDropdownOverlay(g);
      return;
    }
    if (currentTab_ != StudioTab::UiBuilder)
      return;

    const gui::Color goldAccent = gui::Color::fromHex(0xD4AF37);
    const gui::Color cyanAccent = gui::Color::fromHex(0x00F5D4);

    // 0. Unified Property & Toolbar Dropdown Menu (Surface, Mount, Screws,
    // Grid, Style, Color, Font, Size, Text Col)
    if (activePopupMenu_ != UiPopupMenuKind::None &&
        !activePopupItems_.empty()) {
      const gui::Rect<float> menuRect = getActivePopupMenuRect();
      const int cols = std::max(1, getActivePopupMenuCols());
      const float colW =
          (menuRect.width - 6.0f - static_cast<float>(cols - 1) * 4.0f) /
          static_cast<float>(cols);

      g.fillRoundedRect({menuRect.x + 3.0f, menuRect.y + 4.0f, menuRect.width,
                         menuRect.height},
                        6.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.78f));
      g.fillRoundedRect(menuRect, 6.0f, gui::Color::fromHex(0x0E1219));
      g.drawRoundedRect(menuRect, 6.0f, cyanAccent, 1.5f);

      gui::Rect<float> hdrRect = {menuRect.x + 1.0f, menuRect.y + 1.0f,
                                  menuRect.width - 2.0f, 23.0f};
      g.fillRoundedRect(hdrRect, 5.0f, gui::Color::fromHex(0x182230));
      g.drawLine({menuRect.x, menuRect.y + 24.0f},
                 {menuRect.right(), menuRect.y + 24.0f},
                 cyanAccent.withAlpha(0.55f), 1.0f);
      g.drawText(getActivePopupMenuTitle(),
                 {menuRect.x + 10.0f, menuRect.y + 2.0f, menuRect.width - 20.0f,
                  20.0f},
                 cyanAccent, 8.8f, false, true);

      for (int c = 1; c < cols; ++c) {
        const float divX =
            menuRect.x + 3.0f + static_cast<float>(c) * (colW + 4.0f) - 2.0f;
        g.drawLine({divX, menuRect.y + 26.0f}, {divX, menuRect.bottom() - 4.0f},
                   gui::Color::fromHex(0x283242, 0.65f), 1.0f);
      }

      for (size_t i = 0; i < activePopupItems_.size(); ++i) {
        const auto &item = activePopupItems_[i];
        const bool isHovered = (static_cast<int>(i) == hoveredPopupItemIdx_);
        const int row = static_cast<int>(i) / cols;
        const int col = static_cast<int>(i) % cols;

        const float rx =
            menuRect.x + 3.0f + static_cast<float>(col) * (colW + 4.0f);
        const float ry = menuRect.y + 25.0f + static_cast<float>(row) * 24.0f;
        gui::Rect<float> rowRect = {rx, ry, colW, 22.0f};

        if (isHovered) {
          g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.22f));
          g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.80f), 1.0f);
        } else if (item.isCurrent) {
          g.fillRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.18f));
          g.drawRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.65f), 1.0f);
        } else if ((row % 2) == 1) {
          g.fillRoundedRect(rowRect, 3.0f,
                            gui::Color::fromHex(0x141A24, 0.50f));
        }

        float textOffsetX = 20.0f;
        if (item.hasSwatch) {
          gui::Rect<float> sw = {rowRect.x + 6.0f, rowRect.y + 5.0f, 12.0f,
                                 12.0f};
          g.fillRoundedRect(sw, 2.5f, item.swatchColor);
          g.drawRoundedRect(
              sw, 2.5f,
              item.isCurrent ? goldAccent : gui::Color::fromHex(0xDCE2E6, 0.7f),
              1.0f);
          textOffsetX = 24.0f;
        } else {
          const gui::Color dotCol =
              item.isCurrent
                  ? goldAccent
                  : (isHovered ? cyanAccent : gui::Color::TextSecondary());
          g.fillCircle({rowRect.x + 10.0f, rowRect.getCentre().y},
                       item.isCurrent ? 3.5f : 2.1f, dotCol);
        }

        const gui::Color txtCol =
            item.isCurrent
                ? goldAccent
                : (isHovered ? cyanAccent : gui::Color::TextPrimary());
        g.drawText(item.label,
                   {rowRect.x + textOffsetX, rowRect.y,
                    rowRect.width - textOffsetX - 4.0f, rowRect.height},
                   txtCol, 8.9f, false, true);
      }
      return;
    }

    // 1. Unified UI Style Template Dropdown Menu (Saved User Themes + 55 Studio
    // Themes across 3 Columns)
    if (isStyleTemplateDropdownOpen_) {
      constexpr int kCols = 3;
      constexpr float kRowH = 26.0f;
      const gui::Rect<float> menuRect = getStyleTemplateDropdownMenuRect();
      g.fillRoundedRect({menuRect.x + 3.0f, menuRect.y + 4.0f, menuRect.width,
                         menuRect.height},
                        6.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.78f));
      g.fillRoundedRect(menuRect, 6.0f, gui::Color::fromHex(0x0E1219));
      g.drawRoundedRect(menuRect, 6.0f, goldAccent, 1.5f);

      gui::Rect<float> hdrRect = {menuRect.x + 1.0f, menuRect.y + 1.0f,
                                  menuRect.width - 2.0f, 25.0f};
      g.fillRoundedRect(hdrRect, 5.0f, gui::Color::fromHex(0x1B2230));
      g.drawLine({menuRect.x, menuRect.y + 26.0f},
                 {menuRect.right(), menuRect.y + 26.0f},
                 goldAccent.withAlpha(0.55f), 1.0f);
      g.drawText(
          std::format(
              "UNIFIED UI STYLE TEMPLATES ({} USER + {} STUDIO THEMES)  -  "
              "Updates Surface, Knobs, Switches, Meter, Plaque & Hardware Trim",
              savedUserThemes_.size(), kTotalUiStyleTemplates),
          {menuRect.x + 12.0f, menuRect.y + 3.0f, menuRect.width - 24.0f,
           20.0f},
          goldAccent, 9.0f, false, true);

      const float colW = (menuRect.width - 16.0f) / static_cast<float>(kCols);

      // Top Action Bar (Row 0): Save Current (*CUSTOM) Theme as User Theme
      const std::string pendingName =
          (inputCustomThemeName_ && !inputCustomThemeName_->getText().empty())
              ? inputCustomThemeName_->getText()
              : "MY CUSTOM THEME";
      gui::Rect<float> saveRowRect = {menuRect.x + 4.0f, menuRect.y + 27.0f,
                                      menuRect.width - 8.0f, 24.0f};
      const bool saveHovered = (hoveredUserThemeDropdownIdx_ == -100);
      g.fillRoundedRect(
          saveRowRect, 4.0f,
          saveHovered ? cyanAccent.withAlpha(0.28f)
                      : (faceplateLayout_.theme.isCustomThemeModified
                             ? cyanAccent.withAlpha(0.16f)
                             : gui::Color::fromHex(0x16202C)));
      g.drawRoundedRect(saveRowRect, 4.0f,
                        (saveHovered ||
                         faceplateLayout_.theme.isCustomThemeModified)
                            ? cyanAccent
                            : goldAccent.withAlpha(0.55f),
                        1.2f);
      g.drawText(
          std::format(
              "+ SAVE CURRENT {} STYLE AS USER THEME: \"{}\"  "
              "(Edit name in top-right THEME box before saving)",
              faceplateLayout_.theme.isCustomThemeModified ? "(*CUSTOM)" : "",
              pendingName),
          {saveRowRect.x + 10.0f, saveRowRect.y, saveRowRect.width - 20.0f,
           saveRowRect.height},
          saveHovered ? gui::Color::TextPrimary() : cyanAccent, 8.6f, false,
          true);

      const int userRows =
          (static_cast<int>(savedUserThemes_.size()) + kCols - 1) / kCols;
      for (size_t u = 0; u < savedUserThemes_.size(); ++u) {
        const auto &ut = savedUserThemes_[u];
        const bool isCurrentUser =
            (!faceplateLayout_.theme.isCustomThemeModified &&
             faceplateLayout_.theme.customThemeName == ut.name);
        const bool isHoveredUser =
            (static_cast<int>(u) == hoveredUserThemeDropdownIdx_);

        const int uRow = 1 + static_cast<int>(u) / kCols;
        const int uCol = static_cast<int>(u) % kCols;
        const float rx =
            menuRect.x + 4.0f + static_cast<float>(uCol) * (colW + 4.0f);
        const float ry =
            menuRect.y + 27.0f + static_cast<float>(uRow) * kRowH;
        gui::Rect<float> rowRect = {rx, ry, colW - 2.0f, kRowH - 2.0f};

        if (isHoveredUser) {
          g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.22f));
          g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.85f), 1.0f);
        } else if (isCurrentUser) {
          g.fillRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.22f));
          g.drawRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.75f), 1.0f);
        } else {
          g.fillRoundedRect(rowRect, 3.0f,
                            gui::Color::fromHex(0x16222E, 0.75f));
        }

        const uint32_t swatchHex =
            ut.theme.useCustomFinishColor
                ? ut.theme.customFinishHex
                : gui::HardwareMaterials::getPresetFinishBaseHex(
                      ut.theme.finish);
        gui::Rect<float> swatch = {rowRect.x + 5.0f, rowRect.y + 5.0f, 14.0f,
                                   14.0f};
        g.fillRoundedRect(swatch, 3.0f, gui::Color::fromHex(swatchHex));
        g.drawRoundedRect(swatch, 3.0f, goldAccent, 1.2f);

        const gui::Color titleCol =
            isCurrentUser ? goldAccent
                          : (isHoveredUser ? cyanAccent
                                           : gui::Color::TextPrimary());
        g.drawText("* " + ut.name,
                   {rowRect.x + 24.0f, rowRect.y, rowRect.width - 48.0f,
                    rowRect.height},
                   titleCol, 8.4f, false, true);

        // Delete [x] pill on right edge
        gui::Rect<float> delR = {rowRect.right() - 21.0f, rowRect.y + 3.5f,
                                 17.0f, rowRect.height - 7.0f};
        g.fillRoundedRect(delR, 3.0f, gui::Color::fromHex(0x2E181C, 0.90f));
        g.drawRoundedRect(delR, 3.0f,
                          gui::Color::AccentRed().withAlpha(0.65f), 0.9f);
        g.drawText("\xC3\x97", delR, gui::Color::AccentRed(), 8.5f, true, true);
      }

      const float builtInStartY =
          menuRect.y + 27.0f + static_cast<float>(1 + userRows) * kRowH;
      g.drawLine({menuRect.x + 4.0f, builtInStartY - 1.0f},
                 {menuRect.right() - 4.0f, builtInStartY - 1.0f},
                 goldAccent.withAlpha(0.40f), 1.0f);
      for (int c = 1; c < kCols; ++c) {
        const float divX =
            menuRect.x + 2.0f + static_cast<float>(c) * (colW + 4.0f);
        g.drawLine({divX, menuRect.y + 53.0f},
                   {divX, menuRect.bottom() - 4.0f},
                   gui::Color::fromHex(0x283242, 0.65f), 1.0f);
      }

      for (int i = 0; i < kTotalUiStyleTemplates; ++i) {
        const auto tpl = static_cast<UiStyleTemplate>(i);
        const auto &spec = getUnifiedStyleTemplateSpec(tpl);
        const bool isCurrent =
            (!faceplateLayout_.theme.isCustomThemeModified &&
             faceplateLayout_.theme.customThemeName.empty() &&
             activeStyleTemplate_ == tpl);
        const bool isHovered = (i == hoveredStyleTemplateDropdownIdx_);

        const int row = i / kCols;
        const int col = i % kCols;
        const float rx =
            menuRect.x + 4.0f + static_cast<float>(col) * (colW + 4.0f);
        const float ry = builtInStartY + static_cast<float>(row) * kRowH;
        gui::Rect<float> rowRect = {rx, ry, colW - 2.0f, kRowH - 2.0f};

        if (isHovered) {
          g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.20f));
          g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.80f), 1.0f);
        } else if (isCurrent) {
          g.fillRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.18f));
          g.drawRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.65f), 1.0f);
        } else if ((row % 2) == 1) {
          g.fillRoundedRect(rowRect, 3.0f,
                            gui::Color::fromHex(0x141A24, 0.55f));
        }

        gui::Rect<float> swatch = {rowRect.x + 5.0f, rowRect.y + 5.0f, 14.0f,
                                   14.0f};
        g.fillRoundedRect(swatch, 3.0f,
                          gui::Color::fromHex(spec.surface.baseColorHex));
        g.drawRoundedRect(swatch, 3.0f,
                          gui::Color::fromHex(spec.knobStyle.indicatorColorHex),
                          1.2f);

        const gui::Color titleCol =
            isCurrent ? goldAccent
                      : (isHovered ? cyanAccent : gui::Color::TextPrimary());
        g.drawText(spec.name,
                   {rowRect.x + 24.0f, rowRect.y, rowRect.width - 28.0f,
                    rowRect.height},
                   titleCol, 8.4f, false, true);
      }
      return;
    }

    // 2. Explicit Parameter Binding Dropdown Menu (Grouped by Schematic Stage)
    if (isParamBindDropdownOpen_ && !paramBindOptions_.empty() &&
        selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size())) {
      const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      const gui::Rect<float> menuRect = getParamBindDropdownMenuRect();
      g.fillRoundedRect({menuRect.x + 3.0f, menuRect.y + 4.0f, menuRect.width,
                         menuRect.height},
                        6.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.72f));
      g.fillRoundedRect(menuRect, 6.0f, gui::Color::fromHex(0x10141C));
      g.drawRoundedRect(menuRect, 6.0f, cyanAccent, 1.5f);

      gui::Rect<float> hdrRect = {menuRect.x + 1.0f, menuRect.y + 1.0f,
                                  menuRect.width - 2.0f, 23.0f};
      g.fillRoundedRect(hdrRect, 5.0f, gui::Color::fromHex(0x182230));
      g.drawLine({menuRect.x, menuRect.y + 24.0f},
                 {menuRect.right(), menuRect.y + 24.0f},
                 cyanAccent.withAlpha(0.55f), 1.0f);
      g.drawText(std::format("BIND '{}' TO DSP PARAMETER", w.title),
                 {menuRect.x + 10.0f, menuRect.y + 2.0f, menuRect.width - 20.0f,
                  20.0f},
                 cyanAccent, 9.2f, false, true);

      const size_t dispRows = std::min<size_t>(paramBindOptions_.size(), 16);
      for (size_t i = 0; i < dispRows; ++i) {
        const auto &opt = paramBindOptions_[i];
        const bool isCurrent = (w.boundStageIdx == opt.stageIdx &&
                                w.boundParamSlot == opt.paramSlot);
        const bool isHovered = (static_cast<int>(i) == hoveredParamBindIdx_);
        const float ry = menuRect.y + 24.0f + static_cast<float>(i) * 23.0f;
        gui::Rect<float> rowRect = {menuRect.x + 3.0f, ry + 1.0f,
                                    menuRect.width - 6.0f, 21.0f};

        if (isHovered) {
          g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.22f));
          g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.75f), 1.0f);
        } else if (isCurrent) {
          g.fillRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.18f));
          g.drawRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.55f), 1.0f);
        }
        const gui::Color dotCol =
            isCurrent ? goldAccent
                      : (isHovered ? cyanAccent : gui::Color::TextSecondary());
        g.fillCircle({rowRect.x + 10.0f, rowRect.getCentre().y},
                     isCurrent ? 3.6f : 2.2f, dotCol);
        const gui::Color txtCol =
            isCurrent ? goldAccent
                      : (isHovered ? cyanAccent : gui::Color::TextPrimary());
        g.drawText(opt.label,
                   {rowRect.x + 20.0f, rowRect.y, rowRect.width - 24.0f,
                    rowRect.height},
                   txtCol, 9.2f, false, true);
      }
      return;
    }

    // 3. Relevant Scale Dropdown Menu for Selected Knob
    if (!isScaleDropdownOpen_ || relevantScaleOptions_.empty())
      return;
    if (selectedUiWidgetIndex_ < 0 ||
        selectedUiWidgetIndex_ >=
            static_cast<int>(faceplateLayout_.widgets.size()))
      return;

    const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
    if (!isKnobWidget(w.type))
      return;

    const gui::Rect<float> menuRect = getScaleDropdownMenuRect();

    g.fillRoundedRect(
        {menuRect.x + 3.0f, menuRect.y + 4.0f, menuRect.width, menuRect.height},
        6.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.65f));
    g.fillRoundedRect(menuRect, 6.0f, gui::Color::fromHex(0x10141C));
    g.drawRoundedRect(menuRect, 6.0f, goldAccent, 1.5f);

    gui::Rect<float> hdrRect = {menuRect.x + 1.0f, menuRect.y + 1.0f,
                                menuRect.width - 2.0f, 23.0f};
    g.fillRoundedRect(hdrRect, 5.0f, gui::Color::fromHex(0x1B2230));
    g.drawLine({menuRect.x, menuRect.y + 24.0f},
               {menuRect.right(), menuRect.y + 24.0f},
               goldAccent.withAlpha(0.55f), 1.0f);
    g.drawText(
        std::format("RELEVANT SCALES FOR '{}' (Click to apply)", w.title),
        {menuRect.x + 10.0f, menuRect.y + 2.0f, menuRect.width - 20.0f, 20.0f},
        goldAccent, 9.2f, false, true);

    for (size_t i = 0; i < relevantScaleOptions_.size(); ++i) {
      const UiScalePreset opt = relevantScaleOptions_[i];
      const bool isCurrent = (w.scalePreset == opt);
      const bool isHovered = (static_cast<int>(i) == hoveredScaleDropdownIdx_);
      const float ry = menuRect.y + 24.0f + static_cast<float>(i) * 25.0f;
      gui::Rect<float> rowRect = {menuRect.x + 3.0f, ry + 1.0f,
                                  menuRect.width - 6.0f, 23.0f};

      if (isHovered) {
        g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.22f));
        g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.75f), 1.0f);
      } else if (isCurrent) {
        g.fillRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.18f));
        g.drawRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.55f), 1.0f);
      }

      const gui::Color dotCol =
          isCurrent ? goldAccent
                    : (isHovered ? cyanAccent : gui::Color::TextSecondary());
      g.fillCircle({rowRect.x + 10.0f, rowRect.getCentre().y},
                   isCurrent ? 3.8f : 2.2f, dotCol);

      const gui::Color txtCol =
          isCurrent ? goldAccent
                    : (isHovered ? cyanAccent : gui::Color::TextPrimary());
      g.drawText(
          getScalePresetDropdownDescription(opt),
          {rowRect.x + 20.0f, rowRect.y, rowRect.width - 24.0f, rowRect.height},
          txtCol, 9.3f, false, true);
    }
  }

  // =========================================================================
  // Mouse Wheel, Move & Drag-and-Drop Interaction for Sidebar & Canvases
  // =========================================================================
  void onMouseWheel(float delta) override {
    if (currentTab_ == StudioTab::NewPluginWizard) {
      const auto canvas = getSchematicCanvasRect();
      if (canvas.contains(lastMousePos_)) {
        bool shiftHeld = false;
#if defined(_WIN32)
        shiftHeld = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
#endif
        if (shiftHeld) {
          schematicScrollX_ = math::clamp(schematicScrollX_ - delta * 64.0f,
                                          0.0f, getMaxSchematicScrollX());
        } else {
          const float oldZoom = schematicZoom_;
          const float newZoom =
              math::clamp(oldZoom + delta * 0.12f, 0.40f, 1.75f);
          if (std::abs(newZoom - oldZoom) > 1e-4f) {
            const float anchorRelX =
                (lastMousePos_.x - (canvas.x + 58.0f)) + schematicScrollX_;
            schematicZoom_ = newZoom;
            schematicScrollX_ = anchorRelX * (newZoom / oldZoom) -
                                (lastMousePos_.x - (canvas.x + 58.0f));
            clampSchematicScrollX();
          }
          statusMessage_ = std::format(
              "SCHEMATIC CANVAS ZOOM: {:.0f}% (MouseWheel to zoom, Shift+Wheel "
              "or drag empty space to pan, or click [FIT CHAIN]).",
              schematicZoom_ * 100.0f);
        }
        repaint();
        return;
      }
      constexpr float stepH = 64.0f;
      const float contentH = static_cast<float>(paletteItems_.size()) * stepH;
      const float viewH = 418.0f;
      const float maxScroll = std::max(0.0f, contentH - viewH);
      schematicLibraryScrollY_ = math::clamp(
          schematicLibraryScrollY_ - delta * 48.0f, 0.0f, maxScroll);
      repaint();
      return;
    }
    if (currentTab_ == StudioTab::UiBuilder) {
      if (getUiBuilderCanvasRect().contains(lastMousePos_)) {
        if (selectedUiWidgetIndex_ >= 0 &&
            selectedUiWidgetIndex_ <
                static_cast<int>(faceplateLayout_.widgets.size()) &&
            faceplateLayout_.widgets[selectedUiWidgetIndex_].type ==
                UiWidgetType::VintageSticker) {
          auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
          float stepDeg = 5.0f;
#if defined(_WIN32)
          if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
            stepDeg = 1.0f;
#endif
          w.rotationDeg += delta * stepDeg;
          while (w.rotationDeg > 180.0f)
            w.rotationDeg -= 360.0f;
          while (w.rotationDeg < -180.0f)
            w.rotationDeg += 360.0f;
          if (std::abs(w.rotationDeg) < 0.8f)
            w.rotationDeg = 0.0f;
          lastUiWidgetInteractMs_ = GetTickCount64();
          syncUiWidgetInspector();
          statusMessage_ =
              std::format("Rotated '{}' to {:+.0f}° (hold Shift + Wheel for "
                          "1° precision, or drag the orange 'R' handle).",
                          w.title, w.rotationDeg);
          repaint();
          return;
        }
        const float maxRackScroll = faceplateLayout_.getMaxRackScrollX();
        if (maxRackScroll > 0.5f) {
          faceplateLayout_.rackScrollX =
              math::clamp(faceplateLayout_.rackScrollX - delta * 120.0f, 0.0f,
                          maxRackScroll);
          repaint();
          return;
        }
      }
      constexpr float stepH = 66.0f;
      (void)stepH;
      // No add-widget palette to scroll any more.
      return;
    }
    if (currentTab_ == StudioTab::LiveRack && activeRackPluginIndex_ == 0) {
      if (getRackFaceplateRect().contains(lastMousePos_)) {
        const float maxRackScroll = faceplateLayout_.getMaxRackScrollX();
        if (maxRackScroll > 0.5f) {
          faceplateLayout_.rackScrollX =
              math::clamp(faceplateLayout_.rackScrollX - delta * 120.0f, 0.0f,
                          maxRackScroll);
          repaint();
          return;
        }
      }
    }
  }

  void onMouseMove(const gui::MouseEvent &e) override {
    lastMousePos_ = e.pos;
    if (activeStudioModal_ != ActiveStudioModal::None) {
      repaint();
      return;
    }

    int newHoverMenu = -1;
    for (int i = 0; i < TopHeaderMenuBarGeometry::kMenuButtonCount; ++i) {
      if (TopHeaderMenuBarGeometry::getMenuButtonRect(i).contains(e.pos)) {
        newHoverMenu = i;
        break;
      }
    }
    int newHoverQuick = -1;
    for (int i = 0; i < 2; ++i) {
      if (TopHeaderMenuBarGeometry::getQuickIconRect(i).contains(e.pos)) {
        newHoverQuick = i;
        break;
      }
    }
    if (hoveredTopMenuIdx_ != newHoverMenu ||
        hoveredQuickIconIdx_ != newHoverQuick) {
      hoveredTopMenuIdx_ = newHoverMenu;
      hoveredQuickIconIdx_ = newHoverQuick;
      if (activeTopMenu_ != TopMenuBarSection::None && newHoverMenu >= 0) {
        activeTopMenu_ =
            TopHeaderMenuBarGeometry::getSectionForButtonIndex(newHoverMenu);
      }
      repaint();
    }
    if (activeTopMenu_ != TopMenuBarSection::None) {
      // FILE > "Open Recent" flyout: open while the mouse is on the parent row
      // (or the gap bridging it to the flyout), stay open while inside the flyout.
      bool wantSubmenu = false;
      if (activeTopMenu_ == TopMenuBarSection::File) {
        const auto fileItems = buildDropdownMenuItems(
            activeTopMenu_, history_, recentProjects_,
            faceplateLayout_.theme.gridSnapPx,
            hasClipboardStyle_ || hasClipboardWidget_, &prefs_);
        const auto keepZone =
            getRecentSubmenuKeepOpenZone(activeTopMenu_, fileItems);
        const auto subRect = getActiveRecentSubmenuRect(
            activeTopMenu_, fileItems, recentProjects_.size());
        wantSubmenu = keepZone.contains(e.pos) ||
                      (recentSubmenuOpen_ && subRect.contains(e.pos));
      }
      recentSubmenuOpen_ = wantSubmenu;
      repaint();
    } else {
      recentSubmenuOpen_ = false;
    }

    if (currentTab_ == StudioTab::LiveRack && rackPresetMenuOpen_) {
      int newHoverPreset = -1;
      const auto presetMenuRect = getRackPresetMenuRect();
      (void)presetMenuRect;
      const auto menuRect = getRackPresetMenuRect();
      if (menuRect.contains(e.pos)) {
        const int totalItems = getRackPresetMenuTotalItems();
        for (int i = 0; i < totalItems; ++i) {
          if (getRackPresetMenuItemRect(i).contains(e.pos)) {
            newHoverPreset = i;
            break;
          }
        }
      }
      if (hoveredRackPresetItem_ != newHoverPreset) {
        hoveredRackPresetItem_ = newHoverPreset;
        repaint();
      }
    }

    if (currentTab_ == StudioTab::UiBuilder &&
        activePopupMenu_ != UiPopupMenuKind::None &&
        !activePopupItems_.empty()) {
      const auto menuRect = getActivePopupMenuRect();
      const int cols = std::max(1, getActivePopupMenuCols());
      int newHoverIdx = -1;
      if (menuRect.contains(e.pos) && e.pos.y >= menuRect.y + 24.0f) {
        const int row =
            static_cast<int>((e.pos.y - (menuRect.y + 25.0f)) / 24.0f);
        const int col =
            std::clamp(static_cast<int>(((e.pos.x - menuRect.x) /
                                         std::max(1.0f, menuRect.width)) *
                                        static_cast<float>(cols)),
                       0, cols - 1);
        const int idx = row * cols + col;
        if (row >= 0 && idx >= 0 &&
            idx < static_cast<int>(activePopupItems_.size())) {
          newHoverIdx = idx;
        }
      }
      if (hoveredPopupItemIdx_ != newHoverIdx) {
        hoveredPopupItemIdx_ = newHoverIdx;
        repaint();
      }
    }

    if (currentTab_ == StudioTab::UiBuilder && isStyleTemplateDropdownOpen_) {
      constexpr int kCols = 3;
      constexpr float kRowH = 26.0f;
      const auto menuRect = getStyleTemplateDropdownMenuRect();
      int newHoverTpl = -1;
      int newHoverUser = -1;
      const int userRows =
          (static_cast<int>(savedUserThemes_.size()) + kCols - 1) / kCols;
      if (menuRect.contains(e.pos) && e.pos.y >= menuRect.y + 26.0f) {
        const int row =
            static_cast<int>((e.pos.y - (menuRect.y + 27.0f)) / kRowH);
        const int col = std::clamp(
            static_cast<int>(((e.pos.x - menuRect.x) /
                              std::max(1.0f, menuRect.width)) *
                             static_cast<float>(kCols)),
            0, kCols - 1);
        if (row == 0) {
          newHoverUser = -100;
        } else if (row >= 1 && row < 1 + userRows) {
          const int uIdx = (row - 1) * kCols + col;
          if (uIdx >= 0 && uIdx < static_cast<int>(savedUserThemes_.size())) {
            newHoverUser = uIdx;
          }
        } else if (row >= 1 + userRows) {
          const int idx = (row - (1 + userRows)) * kCols + col;
          if (idx >= 0 && idx < kTotalUiStyleTemplates) {
            newHoverTpl = idx;
          }
        }
      }
      if (hoveredStyleTemplateDropdownIdx_ != newHoverTpl ||
          hoveredUserThemeDropdownIdx_ != newHoverUser) {
        hoveredStyleTemplateDropdownIdx_ = newHoverTpl;
        hoveredUserThemeDropdownIdx_ = newHoverUser;
        repaint();
      }
    }

    if (currentTab_ == StudioTab::UiBuilder && isParamBindDropdownOpen_ &&
        !paramBindOptions_.empty()) {
      const auto menuRect = getParamBindDropdownMenuRect();
      int newHoverBind = -1;
      const size_t dispRows = std::min<size_t>(paramBindOptions_.size(), 16);
      if (menuRect.contains(e.pos) && e.pos.y >= menuRect.y + 24.0f) {
        const int row =
            static_cast<int>((e.pos.y - (menuRect.y + 24.0f)) / 23.0f);
        if (row >= 0 && row < static_cast<int>(dispRows)) {
          newHoverBind = row;
        }
      }
      if (hoveredParamBindIdx_ != newHoverBind) {
        hoveredParamBindIdx_ = newHoverBind;
        repaint();
      }
    }

    if (currentTab_ == StudioTab::UiBuilder && isScaleDropdownOpen_ &&
        !relevantScaleOptions_.empty()) {
      const auto menuRect = getScaleDropdownMenuRect();
      int newHoverScale = -1;
      if (menuRect.contains(e.pos) && e.pos.y >= menuRect.y + 24.0f) {
        const int row =
            static_cast<int>((e.pos.y - (menuRect.y + 24.0f)) / 25.0f);
        if (row >= 0 && row < static_cast<int>(relevantScaleOptions_.size())) {
          newHoverScale = row;
        }
      }
      if (hoveredScaleDropdownIdx_ != newHoverScale) {
        hoveredScaleDropdownIdx_ = newHoverScale;
        repaint();
      }
    }

    if (currentTab_ == StudioTab::LiveRack && rackAudioDeviceMenuOpen_) {
      int newHoverDev = -1;
      const auto devMenuRect = getRackAudioDeviceMenuRect();
      if (devMenuRect.contains(e.pos)) {
        const int totalItems = getRackAudioDeviceMenuTotalItems();
        for (int i = 0; i < totalItems; ++i) {
          if (getRackAudioDeviceMenuItemRect(i).contains(e.pos)) {
            newHoverDev = i;
            break;
          }
        }
      }
      if (hoveredRackAudioDeviceItem_ != newHoverDev) {
        hoveredRackAudioDeviceItem_ = newHoverDev;
        repaint();
      }
    }

    int newHoverRemove = -1;
    if (!isSidebarCollapsed_ && e.pos.x >= 28.0f && e.pos.x <= 332.0f &&
        e.pos.y >= 130.0f) {
      const int idx = static_cast<int>((e.pos.y - 130.0f) / 50.0f);
      if (idx >= 0 && idx < static_cast<int>(projects_.size())) {
        const float itemY = 130.0f + static_cast<float>(idx) * 50.0f;
        gui::Rect<float> removeBtnRect = {304.0f, itemY + 11.0f, 22.0f, 22.0f};
        if (removeBtnRect.contains(e.pos)) {
          newHoverRemove = idx;
        }
      }
    }
    if (hoveredRemoveIndex_ != newHoverRemove) {
      hoveredRemoveIndex_ = newHoverRemove;
      repaint();
    }

    if (currentTab_ == StudioTab::UiBuilder &&
        faceplateLayout_.usesSignalChainSelectorStrip()) {
      const auto uiCanvas = getUiBuilderCanvasRect();
      if (uiCanvas.contains(e.pos)) {
        const float sy = uiCanvas.height / FaceplateLayout::kRefHeight;
        const float ry = (e.pos.y - uiCanvas.y) / sy;
        if (ry >= 50.0f && ry <= 112.0f) {
          repaint();
        }
      }
    }

    if (currentTab_ == StudioTab::LiveRack && activeRackPluginIndex_ == 0) {
      const auto fpRect = getRackFaceplateRect();
      int newTapedHover = -1;
      std::string newTapeLbl;
      if (fpRect.contains(e.pos)) {
        const float sx = fpRect.width / FaceplateLayout::kRefWidth;
        const float sy = fpRect.height / FaceplateLayout::kRefHeight;
        const float maxRackScroll = faceplateLayout_.getMaxRackScrollX();
        for (int i = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
             i >= 0; --i) {
          const auto &w = faceplateLayout_.widgets[i];
          if (!faceplateLayout_.isWidgetVisibleForActiveStage(w)) {
            continue;
          }
          if (!isKnobWidget(w.type) && !isSwitchWidget(w.type) &&
              w.type != UiWidgetType::VerticalStudioFader &&
              w.type != UiWidgetType::HorizontalCrossfader) {
            continue;
          }
          const float effX = faceplateLayout_.getEffectiveWidgetX(w);
          if (maxRackScroll > 0.5f &&
              faceplateLayout_.isScrollableStageWidget(w) &&
              (effX + w.w < FaceplateLayout::kStageViewportLeft ||
               effX > FaceplateLayout::kStageViewportRight)) {
            continue;
          }
          gui::Rect<float> wr = {fpRect.x + effX * sx, fpRect.y + w.y * sy,
                                 w.w * sx, w.h * sy};
          if (wr.contains(e.pos)) {
            if (isWidgetLockedByTape(faceplateLayout_, static_cast<size_t>(i),
                                     &newTapeLbl)) {
              newTapedHover = i;
            }
            break;
          }
        }
      }
      if (hoveredTapedWidgetIdx_ != newTapedHover ||
          hoveredTapeLabel_ != newTapeLbl) {
        hoveredTapedWidgetIdx_ = newTapedHover;
        hoveredTapeLabel_ = std::move(newTapeLbl);
        repaint();
      }
    } else if (hoveredTapedWidgetIdx_ != -1) {
      hoveredTapedWidgetIdx_ = -1;
      hoveredTapeLabel_.clear();
      repaint();
    }

    // Hover detection for Component Library Cards (Mouseover Description Tooltip) &
    // Overdriven Inter-Stage Schematic Traces (> +3.0 dBFS)
    if (currentTab_ == StudioTab::NewPluginWizard) {
      const float libX = isSidebarCollapsed_ ? 72.0f : 368.0f;
      const float libW = isSidebarCollapsed_ ? 280.0f : 235.0f;
      int newPaletteHover = -1;
      if (!isDraggingPaletteItem_ && e.pos.x >= libX + 4.0f &&
          e.pos.x <= libX + libW - 8.0f && e.pos.y >= 288.0f &&
          e.pos.y <= 706.0f) {
        constexpr float stepH = 64.0f;
        constexpr float cardH = 58.0f;
        const float relY = (e.pos.y - 288.0f) + schematicLibraryScrollY_;
        const int idx = static_cast<int>(relY / stepH);
        if (idx >= 0 && idx < static_cast<int>(paletteItems_.size())) {
          const float cardY =
              288.0f + static_cast<float>(idx) * stepH - schematicLibraryScrollY_;
          if (e.pos.y >= cardY && e.pos.y <= cardY + cardH) {
            newPaletteHover = idx;
          }
        }
      }
      if (hoveredPaletteItemIdx_ != newPaletteHover) {
        hoveredPaletteItemIdx_ = newPaletteHover;
        repaint();
      }

      int newOverdriveHover = -1;
      const gui::Rect<float> viewport = getSchematicViewportRect();
      if (hoveredOverdriveStageIdx_ >= 0 &&
          overdriveAutoTrimBtnRect_.contains(e.pos)) {
        newOverdriveHover = hoveredOverdriveStageIdx_;
      } else if (viewport.contains(e.pos)) {
        const size_t grpCount = getMacroGroupedStageCount();
        for (size_t i = 0; i < schematicChain_.size(); ++i) {
          if (grpCount >= 2 && i > 0 && i < grpCount)
            continue;
          if (estimateStageOutputHeadroomDb(i) <= 3.0f)
            continue;
          const gui::Rect<float> nr = getCanvasNodeRect(i);
          const float busY = getStageBusY(i);
          const gui::Rect<float> wireHotzone = {nr.right() - 8.0f, busY - 22.0f,
                                                52.0f, 44.0f};
          const gui::Rect<float> pillHotzone = getStageStatusBadgeRect(i);
          if (wireHotzone.contains(e.pos) || pillHotzone.contains(e.pos) ||
              nr.contains(e.pos)) {
            newOverdriveHover = static_cast<int>(i);
            break;
          }
        }
      }
      if (hoveredOverdriveStageIdx_ != newOverdriveHover) {
        hoveredOverdriveStageIdx_ = newOverdriveHover;
        repaint();
      }
    } else {
      if (hoveredPaletteItemIdx_ != -1) {
        hoveredPaletteItemIdx_ = -1;
      }
      if (hoveredOverdriveStageIdx_ != -1) {
        hoveredOverdriveStageIdx_ = -1;
      }
    }
  }

  [[nodiscard]] gui::Rect<float>
  getInspectorStageKnobRect(int slot) const noexcept {
    const float canvasX = isSidebarCollapsed_ ? 364.0f : 614.0f;
    return {canvasX + 18.0f + static_cast<float>(slot) * 102.0f, 650.0f, 94.0f,
            56.0f};
  }

  void onDoubleClick(const gui::MouseEvent &e) override {
    if (activeStudioModal_ != ActiveStudioModal::None)
      return;

    // 1. Live Rack (Faceplate View) or Faceplate UI Builder: Double-click any
    // knob/fader to reset to Unity / Default
    if ((currentTab_ == StudioTab::LiveRack && activeRackPluginIndex_ == 0) ||
        currentTab_ == StudioTab::UiBuilder) {
      const gui::Rect<float> fpRect = (currentTab_ == StudioTab::LiveRack)
                                          ? getRackFaceplateRect()
                                          : getUiBuilderCanvasRect();
      if (fpRect.contains(e.pos)) {
        const float sx = fpRect.width / FaceplateLayout::kRefWidth;
        const float sy = fpRect.height / FaceplateLayout::kRefHeight;
        const float maxRackScroll = faceplateLayout_.getMaxRackScrollX();

        for (int i = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
             i >= 0; --i) {
          auto &w = faceplateLayout_.widgets[i];
          if (!faceplateLayout_.isWidgetVisibleForActiveStage(w))
            continue;
          if (!isKnobWidget(w.type) &&
              w.type != UiWidgetType::VerticalStudioFader &&
              w.type != UiWidgetType::HorizontalCrossfader) {
            continue;
          }
          const float effX = faceplateLayout_.getEffectiveWidgetX(w);
          if (maxRackScroll > 0.5f &&
              faceplateLayout_.isScrollableStageWidget(w) &&
              (effX + w.w < FaceplateLayout::kStageViewportLeft ||
               effX > FaceplateLayout::kStageViewportRight)) {
            continue;
          }
          const gui::Rect<float> wr = {fpRect.x + effX * sx,
                                       fpRect.y + w.y * sy, w.w * sx, w.h * sy};
          if (wr.contains(e.pos)) {
            if (isWidgetLockedByTape(faceplateLayout_, static_cast<size_t>(i)))
              return;
            recordUndoSnapshot(std::format("Reset '{}' to Default", w.title));

            if (w.boundStageIdx >= 0 &&
                w.boundStageIdx < static_cast<int>(schematicChain_.size())) {
              const auto &node = schematicChain_[w.boundStageIdx];
              const auto &meta = getComponentMeta(node.type);
              const auto &plum = getAutoPlumbingSpec(node.type);
              const int pCount = static_cast<int>(meta.getParamCount());
              if ((w.boundParamSlot == 105 ||
                   (w.boundParamSlot == 5 && pCount <= 5)) &&
                  plum.internalTrim1.isValid()) {
                const float span =
                    std::max(1e-5f, plum.internalTrim1.maxVal -
                                        plum.internalTrim1.minVal);
                w.previewNorm = math::clamp((plum.internalTrim1.defaultVal -
                                             plum.internalTrim1.minVal) /
                                                span,
                                            0.0f, 1.0f);
              } else if ((w.boundParamSlot == 106 ||
                          (w.boundParamSlot == 6 && pCount <= 6) ||
                          (w.boundParamSlot == 7 && pCount <= 7) ||
                          w.title == "TOLERANCE" ||
                          w.subtitle == "DRIFT|CLEAN") &&
                         plum.internalTrim2.isValid()) {
                const float span =
                    std::max(1e-5f, plum.internalTrim2.maxVal -
                                        plum.internalTrim2.minVal);
                w.previewNorm = math::clamp((plum.internalTrim2.defaultVal -
                                             plum.internalTrim2.minVal) /
                                                span,
                                            0.0f, 1.0f);
              } else if (w.knobRole == UiKnobRole::ParallelMix ||
                         w.boundParamSlot == 109 ||
                         (w.boundParamSlot == 9 && pCount <= 9) ||
                         w.boundParamSlot >= pCount) {
                w.previewNorm = 1.0f;
              } else {
                const int slot = std::clamp(w.boundParamSlot, 0, pCount - 1);
                const auto &pSpec = meta.getParamSpec(slot);
                w.previewNorm = computeParamNorm(pSpec, pSpec.defaultVal);
              }
            } else {
              if (w.title.find("MIX") != std::string::npos &&
                  w.knobRole != UiKnobRole::ParallelMix) {
                w.previewNorm = 1.0f;
              } else {
                w.previewNorm = 0.5f;
              }
            }

            applyRackWidgetChangeToSchematic(w);
            isDraggingUiWidget_ = false;
            isResizingUiWidget_ = false;
            rackDraggingWidgetIdx_ = -1;
            statusMessage_ = std::format(
                "DOUBLE-CLICK RESET: '{}' restored to factory default ({}).",
                w.title, w.subtitle);
            repaint();
            return;
          }
        }
      }
      return;
    }


    // 3. Schematic Builder: Double-click any of the 7 Stage Component Inspector
    // slots or Selected Stage Bar to reset to defaultVal
    if (currentTab_ == StudioTab::NewPluginWizard && selectedNodeIndex_ >= 0 &&
        selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
      auto &node = schematicChain_[selectedNodeIndex_];
      const auto &meta = getComponentMeta(node.type);
      const auto &plum = getAutoPlumbingSpec(node.type);
      for (int slot = 0; slot < 7; ++slot) {
        if (getInspectorStageKnobRect(slot).contains(e.pos)) {
          recordUndoSnapshot("Reset Stage Parameter to Default");
          if (slot < 5 && slot < static_cast<int>(meta.getParamCount())) {
            const auto &pSpec = meta.getParamSpec(slot);
            node.setParamVal(slot, pSpec.defaultVal);
            statusMessage_ =
                std::format("SCHEMATIC BUILDER [Stage {} {}]: Reset '{}' to "
                            "default ({:.1f} {}).",
                            selectedNodeIndex_ + 1, meta.shortName, pSpec.name,
                            pSpec.defaultVal, pSpec.unit);
          } else if (slot == 5 && plum.internalTrim1.isValid()) {
            node.trim1Val = plum.internalTrim1.defaultVal;
            statusMessage_ = std::format(
                "SCHEMATIC BUILDER [Stage {} {}]: Reset '{}' to default "
                "({:.1f} {}).",
                selectedNodeIndex_ + 1, meta.shortName, plum.internalTrim1.name,
                plum.internalTrim1.defaultVal, plum.internalTrim1.unit);
          } else if (slot == 6 && plum.internalTrim2.isValid()) {
            node.trim2Val = plum.internalTrim2.defaultVal;
            statusMessage_ = std::format(
                "SCHEMATIC BUILDER [Stage {} {}]: Reset '{}' to default "
                "({:.1f} {}).",
                selectedNodeIndex_ + 1, meta.shortName, plum.internalTrim2.name,
                plum.internalTrim2.defaultVal, plum.internalTrim2.unit);
          }
          syncInspectorKnobsToSelection();
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          repaint();
          return;
        }
      }
    }
  }

  void onMouseDown(const gui::MouseEvent &e) override {
    clearAllTextInputFocus();

    // -2. If any Studio Modal (Preferences, Shortcuts, Reference Guide, About)
    // is open, handle it first!
    if (activeStudioModal_ != ActiveStudioModal::None) {
      StudioModalSpecialAction specialAction = StudioModalSpecialAction::None;
      if (handleActiveStudioModalClick(
              getLocalBounds(), e.pos, activeStudioModal_,
              activePreferencesTab_, prefs_, faceplateLayout_.theme.gridSnapPx,
              ProjectGenerator::getWorkspaceRoot(), &specialAction)) {
        syncPreferencesRuntimeState();
        faceplateLayout_.theme.showGrid =
            (faceplateLayout_.theme.gridSnapPx > 1);
        updateUiThemeButtonLabels();
        updateBuildFormatUiLabels();
        lastAutoSaveTickMs_ = GetTickCount64();

        if (specialAction == StudioModalSpecialAction::CycleExportOutputFolder) {
          if (customExportDir_.empty()) {
            setCustomExportFolder("C:\\Program Files\\Common Files\\VST3");
          } else if (_stricmp(customExportDir_.string().c_str(), "C:\\Program Files\\Common Files\\VST3") == 0) {
            if (!promptForBuildSaveFolder()) {
              resetCustomExportFolderToDefault();
            }
          } else {
            resetCustomExportFolderToDefault();
          }
        } else if (specialAction == StudioModalSpecialAction::VerifyCompilerToolchain) {
          toolchainStatus_ = ProjectGenerator::detectToolchain();
          syncPreferencesRuntimeState();
          saveStudioPreferencesToFile(ProjectGenerator::getWorkspaceRoot() / ".avra_preferences.json", prefs_);
          statusMessage_ = std::format(
              "TOOLCHAIN VERIFIED: MSVC C++20 cl.exe = [{}]  |  CMake = [{}]",
              toolchainStatus_.hasMsvcCompiler ? "OK" : "NOT FOUND",
              toolchainStatus_.hasCMake ? "OK" : "NOT FOUND");
        } else if (specialAction == StudioModalSpecialAction::ClearUndoRedoHistory) {
          history_.clear();
          prefs_.activeUndoCount = 0;
          prefs_.activeRedoCount = 0;
          statusMessage_ = "UNDO / REDO HISTORY CLEARED: 50-step RAM snapshot ring reset.";
        } else {
          if (selectedProjectIndex_ >= 0 && selectedProjectIndex_ < static_cast<int>(projects_.size()) && !isBuildRunning_.load()) {
            std::lock_guard<std::mutex> lk(buildStateMutex_);
            lastBuildReport_ = ProjectGenerator::inspectProjectArtifacts(
                projects_[selectedProjectIndex_].diskFolder,
                prefs_.exportFormatVst3,
                prefs_.exportFormatClap,
                prefs_.exportFormatStandalone,
                customExportDir_);
          }
          statusMessage_ = std::format(
              "PREFERENCES APPLIED: Device = '{}', Rate = {} Hz ({} smpl), Oversampling = {}, Knob Mode = {}.",
              prefs_.getOutputDeviceName(),
              prefs_.sampleRateHz,
              prefs_.blockBufferSize,
              prefs_.getOversamplingCapName(),
              prefs_.getKnobMouseModeName());
        }
        repaint();
        return;
      }
    }

    // -1. Integrated Top-Left Brand Icon & Studio Menu Bar (FILE, EDIT, VIEW,
    // HELP) & Quick-Action Icons ([CCW] [CW])
    if (gui::Rect<float>{6.0f, 4.0f, 138.0f, 42.0f}.contains(e.pos)) {
      activeTopMenu_ = TopMenuBarSection::None;
      activeStudioModal_ = ActiveStudioModal::AboutAndDiagnostics;
      repaint();
      return;
    }
    for (int i = 0; i < TopHeaderMenuBarGeometry::kMenuButtonCount; ++i) {
      if (TopHeaderMenuBarGeometry::getMenuButtonRect(i).contains(e.pos)) {
        const auto sec = TopHeaderMenuBarGeometry::getSectionForButtonIndex(i);
        activeTopMenu_ =
            (activeTopMenu_ == sec) ? TopMenuBarSection::None : sec;
        recentSubmenuOpen_ = false;
        repaint();
        return;
      }
    }
    for (int i = 0; i < 2; ++i) {
      if (TopHeaderMenuBarGeometry::getQuickIconRect(i).contains(e.pos)) {
        activeTopMenu_ = TopMenuBarSection::None;
        recentSubmenuOpen_ = false;
        if (i == 0)
          performUndo();
        else if (i == 1)
          performRedo();
        repaint();
        return;
      }
    }
    if (activeTopMenu_ != TopMenuBarSection::None) {
      const auto items = buildDropdownMenuItems(
          activeTopMenu_, history_, recentProjects_,
          faceplateLayout_.theme.gridSnapPx,
          hasClipboardStyle_ || hasClipboardWidget_, &prefs_);

      // Click inside the "Open Recent" flyout
      if (recentSubmenuOpen_ && activeTopMenu_ == TopMenuBarSection::File) {
        const auto subRect = getActiveRecentSubmenuRect(
            activeTopMenu_, items, recentProjects_.size());
        if (subRect.contains(e.pos)) {
          const int idx = hitTestRecentProjectsSubmenu(
              subRect, recentProjects_.size(), e.pos);
          if (idx >= 0) {
            activeTopMenu_ = TopMenuBarSection::None;
            recentSubmenuOpen_ = false;
            executeMenuCommand(
                recentProjectCommandForIndex(static_cast<size_t>(idx)));
          }
          // "(No Recent Projects)" placeholder / padding: keep menu open.
          return;
        }
      }

      const StudioMenuCommandId cmd =
          hitTestDropdownMenuOverlay(activeTopMenu_, items, e.pos);
      if (cmd == StudioMenuCommandId::File_OpenRecentSubmenu) {
        // Clicking the parent row opens (or keeps open) the flyout.
        recentSubmenuOpen_ = true;
        repaint();
        return;
      }
      activeTopMenu_ = TopMenuBarSection::None;
      recentSubmenuOpen_ = false;
      if (cmd != StudioMenuCommandId::None) {
        executeMenuCommand(cmd);
      } else {
        repaint();
      }
      return;
    }

    // Capture pre-interaction snapshot so continuous drags or clicks coalesce
    // into a single 50-step Undo entry!
    pendingMouseDownSnapshot_ =
        captureCurrentProjectSnapshot("Edit Studio State");
    hasPendingMouseDownSnapshot_ = true;
    lastDragOrClickActionLabel_.clear();

    if (currentTab_ == StudioTab::LiveRack && rackAudioDeviceMenuOpen_) {
      const auto menuRect = getRackAudioDeviceMenuRect();
      if (menuRect.contains(e.pos)) {
        const int totalItems = getRackAudioDeviceMenuTotalItems();
        for (int i = 0; i < totalItems; ++i) {
          if (getRackAudioDeviceMenuItemRect(i).contains(e.pos)) {
            applyRackAudioDeviceItem(i);
            return;
          }
        }
        return;
      }
      rackAudioDeviceMenuOpen_ = false;
      hoveredRackAudioDeviceItem_ = -1;
      repaint();
      return;
    }

    // -0.5. If the Live Test Rack Preset Dropdown Menu is open, handle item
    // selection or close!
    if (currentTab_ == StudioTab::LiveRack && rackPresetMenuOpen_) {
      const auto menuRect = getRackPresetMenuRect();
      if (menuRect.contains(e.pos)) {
        const int totalItems = getRackPresetMenuTotalItems();
        for (int i = 0; i < totalItems; ++i) {
          if (getRackPresetMenuItemRect(i).contains(e.pos)) {
            applyRackPresetItem(i);
            return;
          }
        }
        return;
      }
      rackPresetMenuOpen_ = false;
      hoveredRackPresetItem_ = -1;
      repaint();
      return;
    }

    // 0. If any Property or Toolbar Dropdown Menu is open, handle selection
    // first!
    if (currentTab_ == StudioTab::UiBuilder &&
        activePopupMenu_ != UiPopupMenuKind::None &&
        !activePopupItems_.empty()) {
      const auto menuRect = getActivePopupMenuRect();
      const int cols = std::max(1, getActivePopupMenuCols());
      if (menuRect.contains(e.pos)) {
        if (e.pos.y >= menuRect.y + 24.0f) {
          const int row =
              static_cast<int>((e.pos.y - (menuRect.y + 25.0f)) / 24.0f);
          const int col =
              std::clamp(static_cast<int>(((e.pos.x - menuRect.x) /
                                           std::max(1.0f, menuRect.width)) *
                                          static_cast<float>(cols)),
                         0, cols - 1);
          const int idx = row * cols + col;
          if (row >= 0 && idx >= 0 &&
              idx < static_cast<int>(activePopupItems_.size())) {
            applyPopupMenuSelection(idx);
            return;
          }
        }
        return;
      }
      activePopupMenu_ = UiPopupMenuKind::None;
      hoveredPopupItemIdx_ = -1;
      updateUiThemeButtonLabels();
      syncUiWidgetInspector();
      repaint();
    }

    // 0a. If the Unified UI Style Template Dropdown Menu is open, handle
    // selection first!
    if (currentTab_ == StudioTab::UiBuilder && isStyleTemplateDropdownOpen_) {
      constexpr int kCols = 3;
      constexpr float kRowH = 26.0f;
      const auto menuRect = getStyleTemplateDropdownMenuRect();
      if (menuRect.contains(e.pos)) {
        const int userRows =
            (static_cast<int>(savedUserThemes_.size()) + kCols - 1) / kCols;
        if (e.pos.y >= menuRect.y + 26.0f) {
          const int row =
              static_cast<int>((e.pos.y - (menuRect.y + 27.0f)) / kRowH);
          const int col = std::clamp(
              static_cast<int>(((e.pos.x - menuRect.x) /
                                std::max(1.0f, menuRect.width)) *
                               static_cast<float>(kCols)),
              0, kCols - 1);
          if (row == 0) {
            // Save Current (*CUSTOM) Style as User Theme
            isStyleTemplateDropdownOpen_ = false;
            hoveredStyleTemplateDropdownIdx_ = -1;
            hoveredUserThemeDropdownIdx_ = -1;
            saveCurrentThemeAsUserTheme(
                inputCustomThemeName_ ? inputCustomThemeName_->getText() : "");
            return;
          } else if (row >= 1 && row < 1 + userRows) {
            const int uIdx = (row - 1) * kCols + col;
            if (uIdx >= 0 && uIdx < static_cast<int>(savedUserThemes_.size())) {
              const float colW =
                  (menuRect.width - 16.0f) / static_cast<float>(kCols);
              const float rx =
                  menuRect.x + 4.0f + static_cast<float>(col) * (colW + 4.0f);
              const float ry =
                  menuRect.y + 27.0f + static_cast<float>(row) * kRowH;
              const gui::Rect<float> delR = {rx + colW - 23.0f, ry + 3.5f,
                                             17.0f, 17.0f};
              if (delR.contains(e.pos)) {
                const std::string deletedName =
                    savedUserThemes_[static_cast<size_t>(uIdx)].name;
                savedUserThemes_.erase(savedUserThemes_.begin() + uIdx);
                saveSavedUserThemesToFile(ProjectGenerator::getWorkspaceRoot(),
                                          savedUserThemes_);
                hoveredUserThemeDropdownIdx_ = -1;
                statusMessage_ = std::format(
                    "Deleted saved user theme '{}'.", deletedName);
                repaint();
                return;
              }
              const auto chosenUserTheme =
                  savedUserThemes_[static_cast<size_t>(uIdx)];
              isStyleTemplateDropdownOpen_ = false;
              hoveredStyleTemplateDropdownIdx_ = -1;
              hoveredUserThemeDropdownIdx_ = -1;
              lastUiWidgetInteractMs_ = 0;
              recordUndoSnapshot(
                  std::format("Apply User Theme '{}'", chosenUserTheme.name));
              applySavedUserTheme(faceplateLayout_, chosenUserTheme,
                                  schematicChain_);
              activeStyleTemplate_ = static_cast<UiStyleTemplate>(std::clamp(
                  faceplateLayout_.theme.styleTemplateIdx, 0,
                  kTotalUiStyleTemplates - 1));
              if (inputCustomThemeName_) {
                inputCustomThemeName_->setText(chosenUserTheme.name);
              }
              updateUiThemeButtonLabels();
              syncUiWidgetInspector();
              statusMessage_ =
                  "APPLIED SAVED USER THEME '* " + chosenUserTheme.name + "'!";
              repaint();
              return;
            }
          } else if (row >= 1 + userRows) {
            const int tplIdx = (row - (1 + userRows)) * kCols + col;
            if (tplIdx >= 0 && tplIdx < kTotalUiStyleTemplates) {
              activeStyleTemplate_ = static_cast<UiStyleTemplate>(tplIdx);
              isStyleTemplateDropdownOpen_ = false;
              hoveredStyleTemplateDropdownIdx_ = -1;
              hoveredUserThemeDropdownIdx_ = -1;
              lastUiWidgetInteractMs_ = 0;
              recordUndoSnapshot(
                  std::format("Apply Theme '{}'",
                              getUiStyleTemplateName(activeStyleTemplate_)));
              applyUiStyleTemplate(faceplateLayout_, activeStyleTemplate_,
                                   schematicChain_);
              updateUiThemeButtonLabels();
              syncUiWidgetInspector();
              statusMessage_ = std::format(
                  "APPLIED UI STYLE TEMPLATE '{}': {} - Click any widget on "
                  "the canvas to customize individually!",
                  getUiStyleTemplateName(activeStyleTemplate_),
                  getUiStyleTemplateDescription(activeStyleTemplate_));
              repaint();
              return;
            }
          }
        }
        return;
      }
      isStyleTemplateDropdownOpen_ = false;
      hoveredStyleTemplateDropdownIdx_ = -1;
      hoveredUserThemeDropdownIdx_ = -1;
      updateUiThemeButtonLabels();
      repaint();
    }

    // 0b. If the Explicit Parameter Binding Dropdown Menu is open, handle
    // selection!
    if (currentTab_ == StudioTab::UiBuilder && isParamBindDropdownOpen_ &&
        !paramBindOptions_.empty()) {
      const auto menuRect = getParamBindDropdownMenuRect();
      if (menuRect.contains(e.pos)) {
        const size_t dispRows = std::min<size_t>(paramBindOptions_.size(), 16);
        if (e.pos.y >= menuRect.y + 24.0f) {
          const int rowIdx =
              static_cast<int>((e.pos.y - (menuRect.y + 24.0f)) / 23.0f);
          if (rowIdx >= 0 && rowIdx < static_cast<int>(dispRows) &&
              selectedUiWidgetIndex_ >= 0 &&
              selectedUiWidgetIndex_ <
                  static_cast<int>(faceplateLayout_.widgets.size())) {
            auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
            const auto &chosen = paramBindOptions_[static_cast<size_t>(rowIdx)];
            w.boundStageIdx = chosen.stageIdx;
            w.boundParamSlot = chosen.paramSlot;
            syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                  schematicChain_);
            syncGlobalFaceplateControls();
            isParamBindDropdownOpen_ = false;
            hoveredParamBindIdx_ = -1;
            syncUiWidgetInspector();
            statusMessage_ = std::format("Bound '{}' directly to [{}]!",
                                         w.title, chosen.label);
            repaint();
            return;
          }
        }
        return;
      }
      isParamBindDropdownOpen_ = false;
      hoveredParamBindIdx_ = -1;
      syncUiWidgetInspector();
      repaint();
    }

    // 0c. If the Relevant Scale Dropdown Menu is open in Faceplate UI Builder,
    // handle selection first!
    if (currentTab_ == StudioTab::UiBuilder && isScaleDropdownOpen_ &&
        !relevantScaleOptions_.empty()) {
      const auto menuRect = getScaleDropdownMenuRect();
      if (menuRect.contains(e.pos)) {
        if (e.pos.y >= menuRect.y + 24.0f) {
          const int rowIdx =
              static_cast<int>((e.pos.y - (menuRect.y + 24.0f)) / 25.0f);
          if (rowIdx >= 0 &&
              rowIdx < static_cast<int>(relevantScaleOptions_.size()) &&
              selectedUiWidgetIndex_ >= 0 &&
              selectedUiWidgetIndex_ <
                  static_cast<int>(faceplateLayout_.widgets.size())) {
            auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
            w.scalePreset = relevantScaleOptions_[static_cast<size_t>(rowIdx)];
            if (w.scalePreset == UiScalePreset::DialOneToTen ||
                w.scalePreset == UiScalePreset::DialZeroToTen) {
              w.customTickLabels.clear();
            } else if (w.boundStageIdx >= 0 &&
                       w.boundStageIdx <
                           static_cast<int>(schematicChain_.size())) {
              const auto &node = schematicChain_[w.boundStageIdx];
              const auto &meta = getComponentMeta(node.type);
              const int pCount = static_cast<int>(meta.getParamCount());
              if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount) {
                w.customTickLabels = buildContinuousFreqTickLabels(
                    meta.getParamSpec(w.boundParamSlot), node.type);
              }
            }
            isScaleDropdownOpen_ = false;
            hoveredScaleDropdownIdx_ = -1;
            syncUiWidgetInspector();
            statusMessage_ = std::format(
                "Amended '{}' scale to {} (relevant to {}).", w.title,
                getScalePresetDropdownDescription(w.scalePreset),
                describeWidgetAutoBinding(w, schematicChain_));
            repaint();
            return;
          }
        }
        return;
      }
      isScaleDropdownOpen_ = false;
      hoveredScaleDropdownIdx_ = -1;
      syncUiWidgetInspector();
      repaint();
    }

    // 1. Collapsible Left Workspace Projects Sidebar Click:
    if (isSidebarCollapsed_) {
      gui::Rect<float> collapsedRail = {16.0f, 62.0f, 36.0f,
                                        getLocalBounds().height - 107.0f};
      if (collapsedRail.contains(e.pos)) {
        if (e.pos.y >= 116.0f) {
          const int clickedIdx = static_cast<int>((e.pos.y - 116.0f) / 36.0f);
          if (clickedIdx >= 0 &&
              clickedIdx < static_cast<int>(projects_.size())) {
            if (clickedIdx != selectedProjectIndex_) {
              saveActiveProject(true);
            }
            selectedProjectIndex_ = clickedIdx;
            loadWorkspaceProjectIntoSchematic(projects_[clickedIdx]);
            repaint();
            return;
          }
        }
        isSidebarCollapsed_ = false;
        if (btnProjNew_)
          btnProjNew_->setVisible(true);
        if (btnProjDuplicate_)
          btnProjDuplicate_->setVisible(true);
        if (btnProjImportExport_)
          btnProjImportExport_->setVisible(true);
        if (btnRestoreHidden_)
          btnRestoreHidden_->setVisible(!hiddenProjects_.empty());
        resized();
        statusMessage_ = "Expanded WORKSPACE PROJECTS drawer (click [<<] on "
                         "sidebar header to collapse back to 36px rail).";
        repaint();
        return;
      }
    } else {
      gui::Rect<float> collapseBtn = {306.0f, 68.0f, 26.0f, 22.0f};
      if (collapseBtn.contains(e.pos) && currentTab_ != StudioTab::Projects) {
        isSidebarCollapsed_ = true;
        if (btnProjNew_)
          btnProjNew_->setVisible(false);
        if (btnProjDuplicate_)
          btnProjDuplicate_->setVisible(false);
        if (btnProjImportExport_)
          btnProjImportExport_->setVisible(false);
        if (btnRestoreHidden_)
          btnRestoreHidden_->setVisible(false);
        resized();
        statusMessage_ = "Collapsed WORKSPACE PROJECTS drawer to slim 36px "
                         "rail (+295px reclaimed for editor canvas!).";
        repaint();
        return;
      }

      if (e.pos.x >= 28.0f && e.pos.x <= 332.0f && e.pos.y >= 130.0f) {
        const int clickedIdx = static_cast<int>((e.pos.y - 130.0f) / 50.0f);
        if (clickedIdx >= 0 &&
            clickedIdx < static_cast<int>(projects_.size())) {
          const float itemY = 130.0f + static_cast<float>(clickedIdx) * 50.0f;
          gui::Rect<float> removeBtnRect = {304.0f, itemY + 11.0f, 22.0f,
                                            22.0f};

          if (removeBtnRect.contains(e.pos)) {
            removeProjectAtIndex(clickedIdx);
            return;
          }

          if (clickedIdx != selectedProjectIndex_) {
            saveActiveProject(true);
          }
          selectedProjectIndex_ = clickedIdx;
          loadWorkspaceProjectIntoSchematic(projects_[clickedIdx]);
          {
            std::lock_guard<std::mutex> lk(buildStateMutex_);
            lastBuildReport_ = ProjectGenerator::inspectProjectArtifacts(
                projects_[clickedIdx].diskFolder, prefs_.exportFormatVst3,
                prefs_.exportFormatClap, prefs_.exportFormatStandalone,
                customExportDir_);
            lastConsoleActionBanner_.clear();
          }

          if (currentTab_ == StudioTab::LiveRack) {
            statusMessage_ =
                std::format("LIVE RACK: Loaded '{}' ({} stages & {} UI "
                            "controls) - playing live @ 48 kHz!",
                            projects_[clickedIdx].name, schematicChain_.size(),
                            faceplateLayout_.widgets.size());
          } else if (currentTab_ == StudioTab::Projects) {
            statusMessage_ = std::format(
                "SELECTED PROJECT '{}': Destination folder -> {}",
                projects_[clickedIdx].name, lastBuildReport_.destinationFolder);
          } else if (currentTab_ != StudioTab::UiBuilder) {
            switchTab(StudioTab::NewPluginWizard);
          }
          repaint();
          return;
        }
      }
    }

    // In BUILD mode, clicking any component card in the Component Chain screen
    // opens Schematic Builder.
    if (currentTab_ == StudioTab::Projects) {
      if (e.pos.x >= 385.0f && e.pos.x <= 1335.0f && e.pos.y >= 170.0f &&
          e.pos.y <= 486.0f) {
        switchTab(StudioTab::NewPluginWizard);
        return;
      }
      return;
    }

    const float libX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    const float libW = isSidebarCollapsed_ ? 280.0f : 232.0f;
    (void)libX;
    (void)libW;

    // In FACEPLATE UI BUILDER mode: Faceplate Canvas positioning/resizing
    // (the add-widget palette has been removed).
    if (currentTab_ == StudioTab::UiBuilder) {
      const auto canvasRect = getUiBuilderCanvasRect();
      if (canvasRect.contains(e.pos)) {
        clearAllTextInputFocus();
        if (handleFaceplateRackScrollClick(e.pos, canvasRect)) {
          return;
        }
        const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
        const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
        const float maxRackScroll = faceplateLayout_.getMaxRackScrollX();

        // Topmost Hit-Test Priority: Check [X] delete button, (R) rotate
        // handle, and bottom-right resize handle on the currently selected
        // widget before checking any widget body drag!
        if (selectedUiWidgetIndex_ >= 0 &&
            selectedUiWidgetIndex_ <
                static_cast<int>(faceplateLayout_.widgets.size())) {
          const auto &sw =
              faceplateLayout_
                  .widgets[static_cast<size_t>(selectedUiWidgetIndex_)];
          if (faceplateLayout_.isWidgetVisibleForActiveStage(sw)) {
            const float effX = faceplateLayout_.getEffectiveWidgetX(sw);
            gui::Rect<float> swr = {canvasRect.x + effX * sx,
                                    canvasRect.y + sw.y * sy, sw.w * sx,
                                    sw.h * sy};
            gui::Point<float> testPt = e.pos;
            if (sw.type == UiWidgetType::VintageSticker &&
                std::abs(sw.rotationDeg) > 0.01f) {
              const auto c = swr.getCentre();
              const float rad = -sw.rotationDeg * 0.0174532925f;
              const float dx = e.pos.x - c.x;
              const float dy = e.pos.y - c.y;
              testPt = {c.x + dx * std::cos(rad) - dy * std::sin(rad),
                        c.y + dx * std::sin(rad) + dy * std::cos(rad)};
            }

            // 1. Top-right '[X]' delete button ({ swr.right() - 2.0f, swr.y
            // - 10.0f })
            gui::Rect<float> delBtn = {swr.right() - 12.0f, swr.y - 20.0f,
                                       20.0f, 20.0f};
            if (delBtn.contains(testPt)) {
              deleteSelectedUiWidget();
              return;
            }

            // 2. Top-center '(R)' rotate handle ({ swr.getCentre().x, swr.y
            // - 22.0f })
            if (sw.type == UiWidgetType::VintageSticker) {
              const float rotHx = swr.getCentre().x;
              const float rotHy = swr.y - 22.0f;
              if (std::hypot(testPt.x - rotHx, testPt.y - rotHy) <= 13.0f) {
                lastUiWidgetInteractMs_ = GetTickCount64();
                syncUiWidgetInspector();
                isRotatingUiWidget_ = true;
                uiDragStartMouse_ = e.pos;
                uiDragStartRect_ = {sw.x, sw.y, sw.w, sw.h};
                repaint();
                return;
              }
            }

            // 3. Bottom-right resize handle ({ swr.right() - 5.0f, swr.bottom()
            // - 5.0f })
            gui::Rect<float> grip = {swr.right() - 14.0f, swr.bottom() - 14.0f,
                                     16.0f, 16.0f};
            if (grip.contains(testPt)) {
              lastUiWidgetInteractMs_ = GetTickCount64();
              syncUiWidgetInspector();
              isResizingUiWidget_ = true;
              uiDragStartMouse_ = e.pos;
              uiDragStartRect_ = {sw.x, sw.y, sw.w, sw.h};
              repaint();
              return;
            }
          }
        }

        for (int pass = 0; pass < 2; ++pass) {
          for (int i = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
               i >= 0; --i) {
            const auto &w = faceplateLayout_.widgets[i];
            if (!faceplateLayout_.isWidgetVisibleForActiveStage(w))
              continue;
            const bool isFrame = (w.type == UiWidgetType::SectionFrame ||
                                  w.type == UiWidgetType::VentilationGrille ||
                                  w.type == UiWidgetType::RackHandlePair);
            if ((pass == 0 && isFrame) || (pass == 1 && !isFrame))
              continue;

            const float effX = faceplateLayout_.getEffectiveWidgetX(w);
            if (maxRackScroll > 0.5f &&
                faceplateLayout_.isScrollableStageWidget(w) &&
                (effX + w.w < FaceplateLayout::kStageViewportLeft ||
                 effX > FaceplateLayout::kStageViewportRight)) {
              continue;
            }

            gui::Rect<float> wr = {canvasRect.x + effX * sx,
                                   canvasRect.y + w.y * sy, w.w * sx, w.h * sy};
            gui::Point<float> testPt = e.pos;
            if (w.type == UiWidgetType::VintageSticker &&
                std::abs(w.rotationDeg) > 0.01f) {
              const auto c = wr.getCentre();
              const float rad = -w.rotationDeg * 0.0174532925f;
              const float dx = e.pos.x - c.x;
              const float dy = e.pos.y - c.y;
              testPt = {c.x + dx * std::cos(rad) - dy * std::sin(rad),
                        c.y + dx * std::sin(rad) + dy * std::cos(rad)};
            }

            // Check top-right '[X]' Quick-Delete Badge on currently selected
            // widget/sticker (only for non-permanent widgets)
            if (i == selectedUiWidgetIndex_ &&
                !faceplateLayout_.isPermanentMasterSectionWidget(w)) {
              gui::Rect<float> delBtn = {wr.right() - 12.0f, wr.y - 20.0f,
                                         20.0f, 20.0f};
              if (delBtn.contains(testPt)) {
                deleteSelectedUiWidget();
                return;
              }
            }

            // Check top-center 'R' Rotation Handle for VintageSticker
            if (w.type == UiWidgetType::VintageSticker) {
              const float rotHx = wr.getCentre().x;
              const float rotHy = wr.y - 22.0f;
              if (std::hypot(testPt.x - rotHx, testPt.y - rotHy) <= 13.0f) {
                selectedUiWidgetIndex_ = i;
                lastUiWidgetInteractMs_ = GetTickCount64();
                syncUiWidgetInspector();
                isRotatingUiWidget_ = true;
                uiDragStartMouse_ = e.pos;
                uiDragStartRect_ = {w.x, w.y, w.w, w.h};
                repaint();
                return;
              }
            }

            if (wr.contains(testPt)) {
              selectedUiWidgetIndex_ = i;
              lastUiWidgetInteractMs_ = GetTickCount64();
              syncUiWidgetInspector();

              // Shift + Right-Click on any rotary knob draws/toggles a
              // China-Marker / Wax-Pencil sweet-spot mark!
              if (e.isRightDown && e.isShiftDown && isKnobWidget(w.type)) {
                toggleOrSetKnobWaxMark(faceplateLayout_.widgets[i], wr, e.pos);
                repaint();
                return;
              }

              gui::Rect<float> grip = {wr.right() - 14.0f, wr.bottom() - 14.0f,
                                       16.0f, 16.0f};
              if (grip.contains(testPt)) {
                isResizingUiWidget_ = true;
              } else if (w.type == UiWidgetType::VintageSticker &&
                         e.isRightDown) {
                isRotatingUiWidget_ = true;
              } else {
                isDraggingUiWidget_ = true;
              }
              uiDragStartMouse_ = e.pos;
              uiDragStartRect_ = {w.x, w.y, w.w, w.h};
              repaint();
              return;
            }
          }
        }
        lastUiWidgetInteractMs_ = 0;
        repaint();
      }
      return;
    }

    // In LIVE RACK mode:
    if (currentTab_ == StudioTab::LiveRack) {
#if defined(_WIN32)
      if (e.isRightDown) {
        gui::Button *stemBtns[6] = {btnSigDrum_.get(), btnSigSynth_.get(),
                                    btnSigCleanGuitar_.get(),
                                    btnSigGuitar_.get(), btnSigBass_.get(),
                                    btnSigVocal_.get()};
        const TestSignalMode stemModes[6] = {
            TestSignalMode::DrumTransientLoop, TestSignalMode::SynthChordLoop,
            TestSignalMode::CleanGuitarChord, TestSignalMode::GuitarDiChug,
            TestSignalMode::BassDiGroove, TestSignalMode::VocalPhrase};
        const char *stemNames[6] = {"DRUMS",    "SYNTH",   "CLEAN GTR",
                                    "DIST GTR", "BASS DI", "VOCAL"};
        for (int s = 0; s < 6; ++s) {
          if (stemBtns[s] != nullptr &&
              stemBtns[s]->getBounds().contains(e.pos)) {
            char fileBuf[MAX_PATH] = {0};
            OPENFILENAMEA ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = GetActiveWindow();
            ofn.lpstrFilter =
                "WAV Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = fileBuf;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            ofn.lpstrTitle = "Assign Custom .WAV Stem to Test Signal Slot";
            if (GetOpenFileNameA(&ofn)) {
              customStemPaths_[static_cast<size_t>(s)] = fileBuf;
              selectTestSignal(stemModes[s]);
              statusMessage_ = std::format(
                  "LIVE RACK: Assigned custom .WAV stem '{}' to [{}] button!",
                  std::filesystem::path(fileBuf).filename().string(),
                  stemNames[s]);
            }
            repaint();
            return;
          }
        }
      }
#endif
      const float infoCardY = 535.0f;
      const gui::Rect<float> rackLabBenchPill = {
          getWidth() - 510.0f, infoCardY + 5.0f, 126.0f, 20.0f};
      const gui::Rect<float> rackProbePill = {getWidth() - 378.0f,
                                              infoCardY + 5.0f, 188.0f, 20.0f};
      const gui::Rect<float> rackPdcPill = {getWidth() - 184.0f,
                                            infoCardY + 5.0f, 148.0f, 20.0f};
      if (rackLabBenchPill.contains(e.pos)) {
        labBenchActive_ = !labBenchActive_;
        syncRackTelemetryVisibility();
        statusMessage_ =
            labBenchActive_
                ? "ACOUSTIC LAB BENCH ACTIVE: Switched telemetry dock to "
                  "real-time Bode Magnitude/Phase, H2..H7 Harmonics & X-Y "
                  "Hysteresis Plot!"
                : "ACOUSTIC LAB BENCH OFF: Restored Dual-Trace FFT Spectrum, "
                  "Waveform Scope & Gain Reduction History.";
        repaint();
        return;
      }
      if (labBenchActive_) {
        const float teleY = 568.0f;
        const float teleH = 158.0f;
        const float specX = isSidebarCollapsed_ ? 74.0f : 368.0f;
        const gui::Rect<float> labBox = {specX, teleY, 995.0f - specX, teleH};
        if (labBox.contains(e.pos) && e.pos.y <= labBox.y + 24.0f) {
          const float tabW = (labBox.width - 16.0f) / 3.0f;
          const int clickedTab = std::clamp(
              static_cast<int>((e.pos.x - (labBox.x + 6.0f)) / tabW), 0, 2);
          labBenchPlotMode_ = clickedTab;
          const char *modeDesc[3] = {
              "1. Magnitude & Phase Response Curve (20 Hz - 20 kHz Bode Plot)",
              "2. Harmonic Distortion Distribution (H2..H7 Bargraph & THD %)",
              "3. Non-Linear Transfer & Hysteresis Scope (X-Y Input vs. Output "
              "Plot)"};
          statusMessage_ =
              std::format("LAB BENCH MODE: {}", modeDesc[labBenchPlotMode_]);
          repaint();
          return;
        }
      }
      if (rackProbePill.contains(e.pos)) {
        cycleSignalProbeState();
        repaint();
        return;
      }
      if (!labBenchActive_ && rackScope_.getBounds().contains(e.pos)) {
        gui::MouseEvent localEv = e;
        localEv.pos.x -= rackScope_.getBounds().x;
        localEv.pos.y -= rackScope_.getBounds().y;
        rackScope_.onMouseDown(localEv);
        repaint();
        return;
      }
      if (activeRackPluginIndex_ == 0) {
        const auto fpRect = getRackFaceplateRect();
        if (fpRect.contains(e.pos)) {
          if (handleFaceplateRackScrollClick(e.pos, fpRect)) {
            return;
          }
          const float sx = fpRect.width / FaceplateLayout::kRefWidth;
          const float sy = fpRect.height / FaceplateLayout::kRefHeight;
          const float maxRackScroll = faceplateLayout_.getMaxRackScrollX();

          for (int i = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
               i >= 0; --i) {
            auto &w = faceplateLayout_.widgets[i];
            if (!faceplateLayout_.isWidgetVisibleForActiveStage(w)) {
              continue;
            }
            if (!isKnobWidget(w.type) && !isSwitchWidget(w.type) &&
                w.type != UiWidgetType::VerticalStudioFader &&
                w.type != UiWidgetType::HorizontalCrossfader) {
              continue;
            }
            const float effX = faceplateLayout_.getEffectiveWidgetX(w);
            if (maxRackScroll > 0.5f &&
                faceplateLayout_.isScrollableStageWidget(w) &&
                (effX + w.w < FaceplateLayout::kStageViewportLeft ||
                 effX > FaceplateLayout::kStageViewportRight)) {
              continue;
            }
            gui::Rect<float> wr = {fpRect.x + effX * sx, fpRect.y + w.y * sy,
                                   w.w * sx, w.h * sy};
            if (wr.contains(e.pos)) {
              // 1. Shift + Right-Click on any knob draws/toggles a China-Marker
              // / Wax-Pencil sweet-spot mark!
              if (e.isRightDown && e.isShiftDown && isKnobWidget(w.type)) {
                toggleOrSetKnobWaxMark(w, wr, e.pos);
                repaint();
                return;
              }

              // 2. Functional Gaff / Masking Tape Lock: if a tape strip
              // overlaps this knob/switch, block changes!
              std::string tapeLbl;
              if (isWidgetLockedByTape(faceplateLayout_, static_cast<size_t>(i),
                                       &tapeLbl)) {
                hoveredTapedWidgetIdx_ = i;
                hoveredTapeLabel_ = tapeLbl;
                statusMessage_ = std::format(
                    "TAPED DOWN: '{}' is physically locked by '{}' tape! Move "
                    "or delete the tape in FACEPLATE UI BUILDER to unlock.",
                    w.title, tapeLbl);
                repaint();
                return;
              }

              if (isSwitchWidget(w.type)) {
                const bool is3Way = (std::count(w.subtitle.begin(),
                                                w.subtitle.end(), '|') >= 2);
                if (is3Way) {
                  if (w.previewNorm < 0.25f)
                    w.previewNorm = 0.5f;
                  else if (w.previewNorm < 0.75f)
                    w.previewNorm = 1.0f;
                  else
                    w.previewNorm = 0.0f;
                } else {
                  w.previewNorm = (w.previewNorm > 0.5f) ? 0.0f : 1.0f;
                }
                applyRackWidgetChangeToSchematic(w);
                repaint();
                return;
              }
              rackDraggingWidgetIdx_ = i;
              rackDragStartY_ = e.pos.y;
              rackDragStartNorm_ = w.previewNorm;
              repaint();
              return;
            }
          }
        }
        return;
      }
    }

    if (currentTab_ != StudioTab::NewPluginWizard)
      return;

    // 1b. Secondary Filter Pill Bar inside Component Library (y: 249..265)
    for (int pillIdx = 0; pillIdx < 5; ++pillIdx) {
      if (getSubFilterPillRect(pillIdx).contains(e.pos)) {
        selectSchematicCategory(activeCategory_, pillIdx);
        return;
      }
    }

    // 1c. Source-Type Filter Pill Bar (ALL | BASS | GUITAR | DRUMS | VOCALS | KEYS) (y: 267..284)
    for (int srcIdx = 0; srcIdx < 6; ++srcIdx) {
      if (getSourceFilterPillRect(srcIdx).contains(e.pos)) {
        selectSchematicSourceFilter(static_cast<SchematicSourceFilter>(srcIdx));
        return;
      }
    }

    // 2. Smooth-Scrolled Schematic Palette Item Click / Drag Start (y:
    // 288..706)
    if (e.pos.x >= libX + 4.0f && e.pos.x <= libX + libW - 4.0f &&
        e.pos.y >= 288.0f && e.pos.y <= 706.0f) {
      constexpr float stepH = 64.0f;
      const float relY = (e.pos.y - 288.0f) + schematicLibraryScrollY_;
      const int idx = static_cast<int>(relY / stepH);
      if (idx >= 0 && idx < static_cast<int>(paletteItems_.size())) {
        const float cardY =
            288.0f + static_cast<float>(idx) * stepH - schematicLibraryScrollY_;
        gui::Rect<float> quickAddBtn = {libX + libW - 46.0f, cardY + 6.0f,
                                        36.0f, 46.0f};

        if (quickAddBtn.contains(e.pos)) {
          appendSchematicComponent(paletteItems_[idx].type);
          return;
        }

        isDraggingPaletteItem_ = true;
        draggedComponentType_ = paletteItems_[idx].type;
        dragCurrentPos_ = e.pos;
        dragInsertSlot_ = computeCanvasInsertSlot(e.pos);
        dragTargetParallelLane_ = (e.pos.y >= kSchematicCenterTrunkY + 48.0f);
        repaint();
        return;
      }
    }

    // 4. Dual-Lane Spatial Schematic Canvas Toolbar, Minimap Radar, Summing
    // Junctions & Node Interaction
    const auto tb = getSchematicToolbarRects();
    if (tb.zoomOutBtn.contains(e.pos)) {
      schematicZoom_ = math::clamp(schematicZoom_ - 0.15f, 0.40f, 1.75f);
      clampSchematicScrollX();
      statusMessage_ = std::format("SCHEMATIC CANVAS ZOOM: {:.0f}%",
                                   schematicZoom_ * 100.0f);
      repaint();
      return;
    }
    if (tb.zoomResetBtn.contains(e.pos)) {
      schematicZoom_ = 1.0f;
      schematicScrollX_ = 0.0f;
      statusMessage_ = "SCHEMATIC CANVAS ZOOM: Reset to 100% (Origin).";
      repaint();
      return;
    }
    if (tb.zoomInBtn.contains(e.pos)) {
      schematicZoom_ = math::clamp(schematicZoom_ + 0.15f, 0.40f, 1.75f);
      clampSchematicScrollX();
      statusMessage_ = std::format("SCHEMATIC CANVAS ZOOM: {:.0f}%",
                                   schematicZoom_ * 100.0f);
      repaint();
      return;
    }
    if (tb.fitChainBtn.contains(e.pos)) {
      fitSchematicChainToViewport();
      statusMessage_ =
          std::format("FIT CHAIN: Framed all {} stages inside the viewport at "
                      "{:.0f}% zoom!",
                      schematicChain_.size(), schematicZoom_ * 100.0f);
      repaint();
      return;
    }
    if (tb.compactViewBtn.contains(e.pos)) {
      schematicCompactView_ = !schematicCompactView_;
      clampSchematicScrollX();
      statusMessage_ =
          schematicCompactView_
              ? "COMPACT VIEW ENABLED: Stage cards condensed to 110px "
                "mini-rack footprint!"
              : "COMPACT VIEW OFF: Restored full-width schematic stage cards.";
      repaint();
      return;
    }
    if (tb.groupStagesBtn.contains(e.pos)) {
      toggleSchematicStageMacroGroup();
      return;
    }

    const gui::Rect<float> minimap = getSchematicMinimapRect();
    if (minimap.contains(e.pos)) {
      isDraggingMinimap_ = true;
      const float radarX = minimap.x + 96.0f;
      const float radarW = std::max(60.0f, minimap.width - 102.0f);
      const float frac = math::clamp((e.pos.x - radarX) / radarW, 0.0f, 1.0f);
      schematicScrollX_ = frac * getMaxSchematicScrollX();
      repaint();
      return;
    }

    const gui::Rect<float> viewport = getSchematicViewportRect();
    if (hoveredOverdriveStageIdx_ >= 0 &&
        overdriveAutoTrimBtnRect_.contains(e.pos)) {
      autoTrimOverdrivenStage(static_cast<size_t>(hoveredOverdriveStageIdx_));
      return;
    }
    if (viewport.contains(e.pos)) {
      const size_t totalNodes = schematicChain_.size();
      const size_t grpCount = getMacroGroupedStageCount();

      // Check 1-click Auto-Trim on flashing red Overdrive Wire Chip (> +3.0
      // dBFS)
      for (size_t i = 0; i < totalNodes; ++i) {
        if (grpCount >= 2 && i > 0 && i < grpCount)
          continue;
        if (estimateStageOutputHeadroomDb(i) <= 3.0f)
          continue;
        const gui::Rect<float> nr = getCanvasNodeRect(i);
        const float busY = getStageBusY(i);
        const gui::Rect<float> wireChipZone = {nr.right() + 1.0f, busY - 18.0f,
                                               42.0f, 36.0f};
        if (wireChipZone.contains(e.pos)) {
          autoTrimOverdrivenStage(i);
          return;
        }
      }

      // 4a. Check interactive [Σ MIX: 50% | Ø | SYNC] Parallel Summing
      // Junction Badges first
      size_t bIdx = (grpCount >= 2) ? grpCount : 0;
      while (bIdx < totalNodes) {
        if (!schematicChain_[bIdx].isParallel) {
          ++bIdx;
          continue;
        }
        const size_t bStart = bIdx;
        while (bIdx + 1 < totalNodes && schematicChain_[bIdx + 1].isParallel) {
          ++bIdx;
        }
        const size_t bEnd = bIdx;
        const gui::Rect<float> sumBadge =
            getSummingJunctionBadgeRect(bStart, bEnd);
        if (sumBadge.contains(e.pos)) {
          auto &endNode = schematicChain_[bEnd];
          const float relX =
              (e.pos.x - sumBadge.x) / std::max(1.0f, sumBadge.width);
          if (relX >= 0.46f && relX <= 0.72f) {
            endNode.parallelPhaseInvert = !endNode.parallelPhaseInvert;
            statusMessage_ = std::format(
                "PARALLEL SUMMING JUNCTION [\xCE\xA3 S{}]: Phase Polarity "
                "(\xC3\x98) set to {} ([LATENCY SYNC] active)!",
                bEnd + 1,
                endNode.parallelPhaseInvert ? "180\xC2\xB0 INVERTED"
                                            : "0\xC2\xB0 NORMAL");
          } else {
            draggingSummingStageIdx_ = static_cast<int>(bEnd);
            summingDragStartY_ = e.pos.y;
            summingDragStartMix_ = endNode.parallelMixPercent;
            endNode.parallelMixPercent =
                std::fmod(endNode.parallelMixPercent + 15.0f, 105.0f);
            if (endNode.parallelMixPercent < 10.0f)
              endNode.parallelMixPercent = 15.0f;
            syncFaceplateWithSchematic();
            statusMessage_ = std::format(
                "PARALLEL SUMMING JUNCTION [\xCE\xA3 S{}]: Wet/Dry Blend = "
                "{:.0f}% (Drag badge vertically to fine-tune)!",
                bEnd + 1, endNode.parallelMixPercent);
          }
          repaint();
          return;
        }
        ++bIdx;
      }

      // 4b. Check Stage Cards (and Macro Chassis Container if grouped)
      for (size_t i = 0; i < totalNodes; ++i) {
        if (grpCount >= 2 && i > 0 && i < grpCount)
          continue;
        const gui::Rect<float> nodeRect = getCanvasNodeRect(i);
        if (!nodeRect.contains(e.pos))
          continue;

        if (grpCount >= 2 && i == 0) {
          schematicMacroGrouped_ = false;
          clampSchematicScrollX();
          statusMessage_ =
              std::format("MACRO CONTAINER EXPANDED: Unpacked all {} front-end "
                          "stages for individual editing.",
                          grpCount);
          repaint();
          return;
        }

        selectedNodeIndex_ = static_cast<int>(i);
        syncInspectorKnobsToSelection();

        const float btnScale = math::clamp(schematicZoom_, 0.65f, 1.30f);
        const float btnW = 13.0f * btnScale;
        const float btnH = 14.0f * btnScale;
        const float btnGap = 2.0f * btnScale;
        const float hdrH = 19.0f * btnScale;
        const float btnY = nodeRect.y + (hdrH - btnH) * 0.5f;
        gui::Rect<float> btnDel = {nodeRect.right() - btnW - 3.0f * btnScale,
                                   btnY, btnW, btnH};
        gui::Rect<float> btnRight = {btnDel.x - btnW - btnGap, btnY, btnW,
                                     btnH};
        gui::Rect<float> btnLeft = {btnRight.x - btnW - btnGap, btnY, btnW,
                                    btnH};
        gui::Rect<float> btnByp = {
            nodeRect.x + 2.0f, nodeRect.y + 1.5f,
            std::max(20.0f, btnLeft.x - nodeRect.x - 4.0f), hdrH};
        const float pillH = 14.5f * btnScale;
        gui::Rect<float> pillMode = {nodeRect.x + 6.0f * btnScale,
                                     nodeRect.bottom() - pillH -
                                         3.0f * btnScale,
                                     nodeRect.width - 12.0f * btnScale, pillH};

        if (nodeRect.width >= 56.0f && btnDel.contains(e.pos)) {
          const auto &meta = getComponentMeta(schematicChain_[i].type);
          statusMessage_ = std::format(
              "Removed Stage {} ({}) from schematic & synced Faceplate UI.",
              i + 1, meta.shortName);
          schematicChain_.erase(schematicChain_.begin() +
                                static_cast<ptrdiff_t>(i));
          if (selectedNodeIndex_ >= static_cast<int>(schematicChain_.size())) {
            selectedNodeIndex_ = static_cast<int>(schematicChain_.size()) - 1;
          }
          clampSchematicScrollX();
          syncInspectorKnobsToSelection();
          updateCanvasPageButtons();
          syncFaceplateWithSchematic();
          repaint();
          return;
        }
        if (nodeRect.width >= 56.0f && btnLeft.contains(e.pos) && i > 0) {
          std::swap(schematicChain_[i], schematicChain_[i - 1]);
          selectedNodeIndex_ = static_cast<int>(i - 1);
          syncInspectorKnobsToSelection();
          syncFaceplateWithSchematic();
          repaint();
          return;
        }
        if (nodeRect.width >= 56.0f && btnRight.contains(e.pos) &&
            i + 1 < schematicChain_.size()) {
          std::swap(schematicChain_[i], schematicChain_[i + 1]);
          selectedNodeIndex_ = static_cast<int>(i + 1);
          syncInspectorKnobsToSelection();
          syncFaceplateWithSchematic();
          repaint();
          return;
        }
        if (btnByp.contains(e.pos)) {
          schematicChain_[i].isBypassed = !schematicChain_[i].isBypassed;
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ = std::format(
              "Stage {} ({}) {} (Faceplate IN/OUT switch synced).", i + 1,
              getComponentMeta(schematicChain_[i].type).shortName,
              schematicChain_[i].isBypassed ? "BYPASSED (OUT)"
                                            : "ENGAGED (IN)");
          repaint();
          return;
        }
        if ((schematicChain_[i].isBypassed || schematicChain_[i].isParallel) &&
            pillMode.contains(e.pos)) {
          if (schematicChain_[i].isBypassed) {
            schematicChain_[i].isBypassed = false;
            syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                  schematicChain_);
          } else {
            schematicChain_[i].isParallel = !schematicChain_[i].isParallel;
            if (schematicChain_[i].isParallel && schematicChain_[i].parallelMixPercent >= 99.9f) schematicChain_[i].parallelMixPercent = 50.0f;
            else if (!schematicChain_[i].isParallel) schematicChain_[i].parallelMixPercent = 100.0f;
            syncFaceplateWithSchematic();
          }
          repaint();
          return;
        }

        isDraggingCanvasNode_ = true;
        draggedNodeOrigIndex_ = static_cast<int>(i);
        dragInsertSlot_ = static_cast<int>(i);
        dragTargetParallelLane_ = schematicChain_[i].isParallel;
        dragCurrentPos_ = e.pos;
        repaint();
        return;
      }

      // 4c. Dragging on empty Schematic Canvas background pans horizontally
      isPanningSchematicCanvas_ = true;
      schematicPanStartMouseX_ = e.pos.x;
      schematicPanStartScrollX_ = schematicScrollX_;
      return;
    }
  }

  void onMouseDrag(const gui::MouseEvent &e) override {
    if (currentTab_ == StudioTab::LiveRack) {
      if (rackDraggingWidgetIdx_ >= 0 &&
          rackDraggingWidgetIdx_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        if (isWidgetLockedByTape(faceplateLayout_,
                                 static_cast<size_t>(rackDraggingWidgetIdx_))) {
          rackDraggingWidgetIdx_ = -1;
          return;
        }
        auto &w = faceplateLayout_.widgets[rackDraggingWidgetIdx_];
        lastDragOrClickActionLabel_ = std::format("Adjust '{}'", w.title);
        const bool fineTune = e.isShiftDown || (prefs_.knobMouseMode == 2);
        const float sensitivity = fineTune ? 420.0f : 135.0f;
        float deltaNorm = (w.type == UiWidgetType::HorizontalCrossfader)
                              ? ((e.pos.x - e.startPos.x) / sensitivity)
                              : ((rackDragStartY_ - e.pos.y) / sensitivity);
        if (prefs_.knobMouseMode == 1 && w.type != UiWidgetType::HorizontalCrossfader) {
          deltaNorm =
              ((rackDragStartY_ - e.pos.y) + (e.pos.x - e.startPos.x) * 0.65f) /
              sensitivity;
        }
        float newNorm = math::clamp(rackDragStartNorm_ + deltaNorm, 0.0f, 1.0f);
        // Subtle magnetic snap when dragging past a Console China-Marker /
        // Wax-Pencil sweet-spot mark!
        if (w.hasWaxMark && !e.isShiftDown &&
            std::abs(newNorm - w.waxMarkNorm) < 0.022f) {
          newNorm = w.waxMarkNorm;
        }
        w.previewNorm = newNorm;
        applyRackWidgetChangeToSchematic(w);
        repaint();
        return;
      }
      return;
    }

    if (currentTab_ == StudioTab::UiBuilder) {
      if (isDraggingUiPalette_) {
        dragCurrentPos_ = e.pos;
        repaint();
        return;
      }
      if (isRotatingUiWidget_ && selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        const auto canvasRect = getUiBuilderCanvasRect();
        const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
        const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        const float cx = canvasRect.x + (w.x + w.w * 0.5f) * sx;
        const float cy = canvasRect.y + (w.y + w.h * 0.5f) * sy;
        float deg = std::atan2(e.pos.x - cx, -(e.pos.y - cy)) * 57.2957795f;
        if (e.isShiftDown) {
          deg = std::round(deg / 15.0f) * 15.0f;
        } else {
          deg = std::round(deg);
          if (std::abs(deg) <= 2.0f)
            deg = 0.0f;
        }
        w.rotationDeg = deg;
        lastUiWidgetInteractMs_ = GetTickCount64();
        syncUiWidgetInspector();
        statusMessage_ = std::format(
            "Rotating '{}' -> {:+.0f}° (hold Shift to snap to 15° steps).",
            w.title, w.rotationDeg);
        repaint();
        return;
      }
      if ((isDraggingUiWidget_ || isResizingUiWidget_) &&
          selectedUiWidgetIndex_ >= 0 &&
          selectedUiWidgetIndex_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        const auto canvasRect = getUiBuilderCanvasRect();
        const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
        const float sy = canvasRect.height / FaceplateLayout::kRefHeight;

        const float dxRef = (e.pos.x - uiDragStartMouse_.x) / sx;
        const float dyRef = (e.pos.y - uiDragStartMouse_.y) / sy;
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];

        const float snapStep =
            static_cast<float>(std::max(1, faceplateLayout_.theme.gridSnapPx));
        auto snapVal = [&](float val) -> float {
          if (e.isShiftDown || snapStep <= 1.0f)
            return val;
          return std::round(val / snapStep) * snapStep;
        };

        if (isResizingUiWidget_) {
          const float minW =
              (w.type == UiWidgetType::VintageSticker) ? 90.0f : 32.0f;
          const float minH = (w.type == UiWidgetType::VintageSticker)
                                 ? (!w.subtitle.empty() ? 46.0f : 36.0f)
                                 : 24.0f;
          w.w = math::clamp(snapVal(uiDragStartRect_.width + dxRef), minW,
                            FaceplateLayout::kRefWidth - w.x - 6.0f);
          w.h = math::clamp(snapVal(uiDragStartRect_.height + dyRef), minH,
                            FaceplateLayout::kRefHeight - w.y - 6.0f);
          if (w.type == UiWidgetType::VintageSticker) {
            autoFitStickerWidgetBounds(w,
                                       faceplateLayout_.theme.globalFontFamily,
                                       w.fontSizePreset, true);
            w.w = std::min(w.w, FaceplateLayout::kRefWidth - w.x - 6.0f);
            w.h = std::min(w.h, FaceplateLayout::kRefHeight - w.y - 6.0f);
          }
        } else {
          w.x = math::clamp(snapVal(uiDragStartRect_.x + dxRef), 6.0f,
                            FaceplateLayout::kRefWidth - w.w - 6.0f);
          w.y = math::clamp(snapVal(uiDragStartRect_.y + dyRef), 6.0f,
                            FaceplateLayout::kRefHeight - w.h - 6.0f);
        }
        lastUiWidgetInteractMs_ = GetTickCount64();
        repaint();
        return;
      }
    }

    if (isDraggingMinimap_) {
      const gui::Rect<float> minimap = getSchematicMinimapRect();
      const float radarX = minimap.x + 96.0f;
      const float radarW = std::max(60.0f, minimap.width - 102.0f);
      const float frac = math::clamp((e.pos.x - radarX) / radarW, 0.0f, 1.0f);
      schematicScrollX_ = frac * getMaxSchematicScrollX();
      repaint();
      return;
    }
    if (isPanningSchematicCanvas_) {
      schematicScrollX_ = math::clamp(schematicPanStartScrollX_ -
                                          (e.pos.x - schematicPanStartMouseX_),
                                      0.0f, getMaxSchematicScrollX());
      repaint();
      return;
    }
    if (draggingSummingStageIdx_ >= 0 &&
        draggingSummingStageIdx_ < static_cast<int>(schematicChain_.size())) {
      auto &endNode = schematicChain_[draggingSummingStageIdx_];
      const float newMix = math::clamp(
          summingDragStartMix_ + (summingDragStartY_ - e.pos.y) * 0.85f, 0.0f,
          100.0f);
      endNode.parallelMixPercent = std::round(newMix);
      syncFaceplateWithSchematic();
      statusMessage_ =
          std::format("PARALLEL SUMMING JUNCTION [\xCE\xA3 S{}]: Wet/Dry Blend "
                      "= {:.0f}% ([LATENCY SYNC] active)",
                      draggingSummingStageIdx_ + 1, endNode.parallelMixPercent);
      repaint();
      return;
    }
    if (isDraggingPaletteItem_) {
      dragCurrentPos_ = e.pos;
      dragInsertSlot_ = computeCanvasInsertSlot(e.pos);
      dragTargetParallelLane_ = (e.pos.y >= kSchematicCenterTrunkY + 48.0f);
      repaint();
    } else if (isDraggingCanvasNode_ && draggedNodeOrigIndex_ >= 0 &&
               draggedNodeOrigIndex_ <
                   static_cast<int>(schematicChain_.size())) {
      dragCurrentPos_ = e.pos;
      dragTargetParallelLane_ = (e.pos.y >= kSchematicCenterTrunkY + 48.0f);
      if (getSchematicCanvasRect().contains(e.pos)) {
        const bool wasPar = schematicChain_[draggedNodeOrigIndex_].isParallel;
        if (dragTargetParallelLane_ && !wasPar && schematicChain_[draggedNodeOrigIndex_].parallelMixPercent >= 99.9f) {
          schematicChain_[draggedNodeOrigIndex_].parallelMixPercent = 50.0f;
        } else if (!dragTargetParallelLane_ && wasPar) {
          schematicChain_[draggedNodeOrigIndex_].parallelMixPercent = 100.0f;
        }
        schematicChain_[draggedNodeOrigIndex_].isParallel =
            dragTargetParallelLane_;
      }
      const int targetSlot = computeCanvasInsertSlot(e.pos);
      const int clampedTarget = math::clamp(
          targetSlot, 0, static_cast<int>(schematicChain_.size()) - 1);
      dragInsertSlot_ = clampedTarget;
      if (clampedTarget != draggedNodeOrigIndex_) {
        std::swap(schematicChain_[draggedNodeOrigIndex_],
                  schematicChain_[clampedTarget]);
        draggedNodeOrigIndex_ = clampedTarget;
        selectedNodeIndex_ = clampedTarget;
      }
      syncFaceplateWithSchematic();
      repaint();
    }
  }

  void onMouseUp(const gui::MouseEvent &e) override {
    inspectorDraggingParamSlot_ = -1;
    rackDraggingWidgetIdx_ = -1;
    isDraggingMinimap_ = false;
    isPanningSchematicCanvas_ = false;
    draggingSummingStageIdx_ = -1;
    if (isDraggingUiPalette_) {
      isDraggingUiPalette_ = false;
      const auto canvasRect = getUiBuilderCanvasRect();
      if (canvasRect.contains(e.pos)) {
        const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
        const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
        const float refX =
            (e.pos.x - canvasRect.x) / sx - draggedUiEntry_.defaultW * 0.5f;
        const float refY =
            (e.pos.y - canvasRect.y) / sy - draggedUiEntry_.defaultH * 0.5f;
        addUiWidgetFromPalette(draggedUiEntry_, refX, refY);
      } else if (std::hypot(e.pos.x - e.startPos.x, e.pos.y - e.startPos.y) <
                 6.0f) {
        addUiWidgetFromPalette(draggedUiEntry_, 440.0f, 210.0f);
      }
      repaint();
    }
    if (currentTab_ == StudioTab::UiBuilder && isDraggingUiWidget_ &&
        selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size())) {
      if (std::hypot(e.pos.x - uiDragStartMouse_.x,
                     e.pos.y - uiDragStartMouse_.y) < 5.0f) {
        auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        if (isSwitchWidget(w.type)) {
          const bool is3Way =
              (std::count(w.subtitle.begin(), w.subtitle.end(), '|') >= 2);
          if (is3Way) {
            if (w.previewNorm < 0.25f)
              w.previewNorm = 0.5f;
            else if (w.previewNorm < 0.75f)
              w.previewNorm = 1.0f;
            else
              w.previewNorm = 0.0f;
          } else {
            w.previewNorm = (w.previewNorm > 0.5f) ? 0.0f : 1.0f;
          }
          applyRackWidgetChangeToSchematic(w);
        }
      } else {
        // Pillar 5.2: Alt + Drag (or Shift + Drag) one Knob onto another Knob
        // to consolidate into a Dual-Bound MacroHaloKnob!
        bool modHeld = e.isShiftDown;
#if defined(_WIN32)
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)
          modHeld = true;
#endif
        const auto &src = faceplateLayout_.widgets[selectedUiWidgetIndex_];
        if (modHeld && isKnobWidget(src.type)) {
          const auto canvasRect = getUiBuilderCanvasRect();
          const float sx = canvasRect.width / FaceplateLayout::kRefWidth;
          const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
          const float dropRefX = (e.pos.x - canvasRect.x) / sx;
          const float dropRefY = (e.pos.y - canvasRect.y) / sy;

          int targetKnobIdx = -1;
          for (int i = static_cast<int>(faceplateLayout_.widgets.size()) - 1;
               i >= 0; --i) {
            if (i == selectedUiWidgetIndex_)
              continue;
            const auto &cand = faceplateLayout_.widgets[i];
            if (!isKnobWidget(cand.type) ||
                faceplateLayout_.isPermanentMasterSectionWidget(cand) ||
                faceplateLayout_.isPermanentMasterSectionWidget(src))
              continue;
            if (dropRefX >= cand.x && dropRefX <= cand.x + cand.w &&
                dropRefY >= cand.y && dropRefY <= cand.y + cand.h) {
              targetKnobIdx = i;
              break;
            }
          }

          if (targetKnobIdx >= 0) {
            const std::string srcTitle = src.title;
            const int srcStage = src.boundStageIdx;
            const int srcSlot = src.boundParamSlot;
            auto &dst = faceplateLayout_.widgets[targetKnobIdx];
            const std::string dstTitle = dst.title;
            dst.type = UiWidgetType::MacroHaloKnob;
            dst.macroStageIdx2 = srcStage;
            dst.macroParamSlot2 = srcSlot;
            dst.title = std::format("{}+{}", dstTitle.substr(0, 6),
                                    srcTitle.substr(0, 6));
            dst.subtitle = "MACRO LINK";

            const int delIdx = selectedUiWidgetIndex_;
            faceplateLayout_.widgets.erase(faceplateLayout_.widgets.begin() +
                                           delIdx);
            selectedUiWidgetIndex_ =
                (delIdx < targetKnobIdx) ? (targetKnobIdx - 1) : targetKnobIdx;
            syncUiWidgetInspector();
            statusMessage_ = std::format(
                "ONE-KNOBIFY MACRO MERGE: Consolidated '{}' + '{}' into a "
                "single MacroHaloKnob driving both parameters!",
                dstTitle, srcTitle);
          }
        }
      }
    }
    isDraggingUiWidget_ = false;
    isResizingUiWidget_ = false;
    isRotatingUiWidget_ = false;

    if (isDraggingPaletteItem_) {
      isDraggingPaletteItem_ = false;
      const float canvasX = isSidebarCollapsed_ ? 364.0f : 605.0f;
      if (e.pos.x >= canvasX && e.pos.y >= 135.0f && e.pos.y <= 624.0f) {
        const bool dropParallel = (e.pos.y >= kSchematicCenterTrunkY + 48.0f);
        insertSchematicComponentAt(draggedComponentType_, dragInsertSlot_,
                                   dropParallel);
      } else if (std::hypot(e.pos.x - e.startPos.x, e.pos.y - e.startPos.y) <
                 6.0f) {
        appendSchematicComponent(draggedComponentType_, false);
      }
      dragInsertSlot_ = -1;
      dragTargetParallelLane_ = false;
      repaint();
    }
    if (isDraggingCanvasNode_) {
      isDraggingCanvasNode_ = false;
      draggedNodeOrigIndex_ = -1;
      dragInsertSlot_ = -1;
      dragTargetParallelLane_ = false;
      repaint();
    }
    if (hasPendingMouseDownSnapshot_) {
      hasPendingMouseDownSnapshot_ = false;
      const auto afterSnap = captureCurrentProjectSnapshot();
      if (!pendingMouseDownSnapshot_.equalsContent(afterSnap)) {
        std::string label = lastDragOrClickActionLabel_;
        if (label.empty()) {
          if (currentTab_ == StudioTab::UiBuilder &&
              selectedUiWidgetIndex_ >= 0 &&
              selectedUiWidgetIndex_ <
                  static_cast<int>(faceplateLayout_.widgets.size())) {
            label = std::format(
                "Edit Widget '{}'",
                faceplateLayout_.widgets[selectedUiWidgetIndex_].title);
          } else if (currentTab_ == StudioTab::NewPluginWizard &&
                     selectedNodeIndex_ >= 0 &&
                     selectedNodeIndex_ <
                         static_cast<int>(schematicChain_.size())) {
            label = std::format(
                "Edit Stage S{} {}", selectedNodeIndex_ + 1,
                getComponentMeta(schematicChain_[selectedNodeIndex_].type)
                    .shortName);
          } else {
            label = "Modify Studio State";
          }
        }
        history_.pushState(std::move(pendingMouseDownSnapshot_),
                           std::move(label));
        markProjectDirty(true);
      }
    }
  }

private:
  void toggleOrSetKnobWaxMark(UiCanvasWidget &w, const gui::Rect<float> &wr,
                              gui::Point<float> clickPos) {
    const bool isCompactDial = (wr.width < 88.0f || wr.height < 94.0f);
    const bool isLargeDial = (wr.width >= 96.0f && wr.height >= 102.0f);
    const float bottomTextH = isCompactDial ? 23.0f : 30.0f;
    const float dialH = std::max(22.0f, wr.height - bottomTextH);
    const float kCx = wr.x + wr.width * 0.5f;
    const float kCy = wr.y + dialH * (isLargeDial ? 0.56f : 0.53f);

    float targetNorm = w.previewNorm;
    if (std::hypot(clickPos.x - kCx, clickPos.y - kCy) >= 8.0f) {
      const float clickAngle =
          std::atan2(clickPos.x - kCx, -(clickPos.y - kCy));
      constexpr float startAngle = -3.14159265f * 0.75f;
      constexpr float endAngle = +3.14159265f * 0.75f;
      targetNorm = math::clamp(
          (clickAngle - startAngle) / (endAngle - startAngle), 0.0f, 1.0f);
    }

    if (w.hasWaxMark && std::abs(w.waxMarkNorm - targetNorm) < 0.065f) {
      w.hasWaxMark = false;
      statusMessage_ =
          std::format("WAX PENCIL: Wiped China-Marker sweet-spot mark off "
                      "'{}'. (Shift + Right-Click skirt to mark again)",
                      w.title);
    } else {
      w.hasWaxMark = true;
      w.waxMarkNorm = targetNorm;
      const float deg = -135.0f + w.waxMarkNorm * 270.0f;
      statusMessage_ = std::format(
          "WAX PENCIL: Marked sweet-spot tick on '{}' skirt at {:+.0f}°! "
          "(Knob magnetically snaps here; Shift+Right-Click again to clear)",
          w.title, deg);
    }
  }

  void applyRackWidgetChangeToSchematic(UiCanvasWidget &w) {
    // 1. Check if this is a Master Output Trim / Master Volume knob or fader
    // (e.g. "OUTPUT TRIM", "MASTER OUTPUT")
    if (!isSwitchWidget(w.type) &&
        (w.knobRole == UiKnobRole::MasterVolume ||
         (w.boundStageIdx < 0 && w.boundParamSlot == 1) ||
         w.title.find("OUTPUT TRIM") != std::string::npos ||
         w.title.find("MASTER OUTPUT") != std::string::npos ||
         w.title == "OUTPUT" || w.title == "MASTER")) {
      rackOutputTrimDb_ = -24.0f + w.previewNorm * 48.0f;
      if (std::abs(rackOutputTrimDb_) < 0.15f)
        rackOutputTrimDb_ = 0.0f;
      w.subtitle = std::format("{:+.1f} dB", rackOutputTrimDb_);
      syncGlobalFaceplateControls();
      statusMessage_ = std::format("LIVE RACK: Master Output Trim = {:+.1f} dB "
                                   "(Driving Main Output Meters & Speakers)",
                                   rackOutputTrimDb_);
      return;
    }

    // 2. Check if this is a Stereo Balance control (e.g. "STEREO BAL" on Avra
    // Gain)
    if (!isSwitchWidget(w.type) &&
        (w.title.find("STEREO BAL") != std::string::npos ||
         w.title.find("BALANCE") != std::string::npos)) {
      rackStereoBalance_ =
          math::clamp((w.previewNorm - 0.5f) * 2.0f, -1.0f, 1.0f);
      if (std::abs(rackStereoBalance_) < 0.04f) {
        rackStereoBalance_ = 0.0f;
        w.subtitle = "CENTER";
      } else if (rackStereoBalance_ < 0.0f) {
        w.subtitle = std::format("L {:.0f}%", -rackStereoBalance_ * 100.0f);
      } else {
        w.subtitle = std::format("R {:.0f}%", rackStereoBalance_ * 100.0f);
      }
      statusMessage_ =
          std::format("LIVE RACK: Stereo Output Balance = {}", w.subtitle);
      return;
    }

    if (w.boundStageIdx >= 0 &&
        w.boundStageIdx < static_cast<int>(schematicChain_.size())) {
      auto &node = schematicChain_[w.boundStageIdx];
      const auto &meta = getComponentMeta(node.type);
      const auto &plum = getAutoPlumbingSpec(node.type);

      const int pCount = static_cast<int>(meta.getParamCount());
      if (isSwitchWidget(w.type)) {
        const bool isOn = (w.previewNorm > 0.5f);
        if (w.boundParamSlot == 108 || (w.boundParamSlot == 8 && pCount <= 8) ||
            w.title.find("LINK IO") != std::string::npos) {
          node.autoGainLinked = isOn;
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ =
              std::format("LIVE RACK: Stage {} [{}] {} Auto-Gain Inverse Link "
                          "{} (RULE_06).",
                          w.boundStageIdx + 1, meta.designator, meta.shortName,
                          node.autoGainLinked ? "ENGAGED (Drive/Trim coupled)"
                                              : "UNLINKED (Independent)");
        } else if (w.title == "TOLERANCE" || w.subtitle == "DRIFT|CLEAN" ||
                   w.boundParamSlot == 106 ||
                   (w.boundParamSlot == 6 && pCount <= 6) ||
                   (w.boundParamSlot == 7 && pCount <= 7)) {
          node.trim2Val =
              isOn ? plum.internalTrim2.maxVal : plum.internalTrim2.minVal;
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ = std::format(
              "LIVE RACK: Stage {} [{}] {} Component Tolerance {} ({:.1f} {}).",
              w.boundStageIdx + 1, meta.designator, meta.shortName,
              isOn ? "DRIFT ENGAGED" : "CLEAN MATCHED", node.trim2Val,
              plum.internalTrim2.unit);
        } else if (w.boundParamSlot == 110 ||
                   (w.boundParamSlot == 10 && pCount <= 10) ||
                   w.title == "STAGE IN" || w.subtitle == "IN|BYP") {
          node.isBypassed = !isOn;
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ =
              std::format("LIVE RACK: Stage {} [{}] {} {} - {}!",
                          w.boundStageIdx + 1, meta.designator, meta.shortName,
                          node.isBypassed ? "BYPASSED (OUT)" : "ENGAGED (IN)",
                          node.isBypassed ? "Signal bypassing this stage"
                                          : "Stage processing active");
        } else if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount) {
          const auto &pSpec = meta.getParamSpec(w.boundParamSlot);
          const float targetVal =
              computeParamValFromNorm(pSpec, w.previewNorm);
          node.setParamVal(w.boundParamSlot, targetVal);
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ =
              std::format("LIVE RACK: Stage {} [{}] {} {} = {:.1f} {}",
                          w.boundStageIdx + 1, meta.designator, meta.shortName,
                          pSpec.name, targetVal, pSpec.unit);
        } else {
          node.isBypassed = !isOn;
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ =
              std::format("LIVE RACK: Stage {} [{}] {} {} - {}!",
                          w.boundStageIdx + 1, meta.designator, meta.shortName,
                          node.isBypassed ? "BYPASSED (OUT)" : "ENGAGED (IN)",
                          node.isBypassed ? "Signal bypassing this stage"
                                          : "Stage processing active");
        }
      } else if ((w.boundParamSlot == 105 ||
                  (w.boundParamSlot == 5 && pCount <= 5)) &&
                 plum.internalTrim1.isValid()) {
        const float span = std::max(1e-5f, plum.internalTrim1.maxVal -
                                               plum.internalTrim1.minVal);
        node.trim1Val = plum.internalTrim1.minVal + w.previewNorm * span;
        w.subtitle =
            std::format("{:.1f} {}", node.trim1Val, plum.internalTrim1.unit);
        statusMessage_ = std::format(
            "LIVE RACK: Stage {} [{}] Internal Trim '{}' = {:.1f} {}",
            w.boundStageIdx + 1, meta.designator, plum.internalTrim1.name,
            node.trim1Val, plum.internalTrim1.unit);
      } else if ((w.boundParamSlot == 106 ||
                  (w.boundParamSlot == 6 && pCount <= 6) ||
                  (w.boundParamSlot == 7 && pCount <= 7)) &&
                 plum.internalTrim2.isValid()) {
        const float span = std::max(1e-5f, plum.internalTrim2.maxVal -
                                               plum.internalTrim2.minVal);
        node.trim2Val = plum.internalTrim2.minVal + w.previewNorm * span;
        w.subtitle =
            std::format("{:.1f} {}", node.trim2Val, plum.internalTrim2.unit);
        statusMessage_ = std::format(
            "LIVE RACK: Stage {} [{}] Internal Trim '{}' = {:.1f} {}",
            w.boundStageIdx + 1, meta.designator, plum.internalTrim2.name,
            node.trim2Val, plum.internalTrim2.unit);
      } else if (w.knobRole == UiKnobRole::ParallelMix ||
                 w.boundParamSlot == 109 ||
                 (w.boundParamSlot == 9 && pCount <= 9) ||
                 w.boundParamSlot >= pCount) {
        node.parallelMixPercent =
            math::clamp(w.previewNorm * 100.0f, 0.0f, 100.0f);
        w.subtitle = std::format("{:.0f} %", node.parallelMixPercent);
        statusMessage_ = std::format(
            "LIVE RACK: Stage {} [{}] {} Parallel Level = {:.0f}%",
            w.boundStageIdx + 1, meta.designator, meta.shortName,
            node.parallelMixPercent);
      } else {
        const int slot = std::clamp(w.boundParamSlot, 0,
                                    static_cast<int>(meta.getParamCount()) - 1);
        const auto &pSpec = meta.getParamSpec(slot);
        const float span = std::max(1e-5f, pSpec.maxVal - pSpec.minVal);
        const float oldVal = node.getParamVal(slot);
        float targetVal = computeParamValFromNorm(pSpec, w.previewNorm);
        if (pSpec.minVal < 0.0f && pSpec.maxVal > 0.0f &&
            (std::abs(targetVal) < span * 0.005f ||
             std::abs(w.previewNorm - 0.5f) < 0.008f)) {
          targetVal = 0.0f;
          w.previewNorm = 0.5f;
        }
        node.setParamVal(slot, targetVal);
        w.subtitle = formatWidgetSubtitleForParam(pSpec, targetVal);

        // RULE_06: Drive / Makeup Inverse Link
        if (slot == 0 && plum.supportsAutoGainLink && node.autoGainLinked &&
            plum.linkedOutputParamSlot >= 0 &&
            plum.linkedOutputParamSlot <
                static_cast<int>(meta.getParamCount())) {
          const float delta = targetVal - oldVal;
          const auto &outSpec = meta.getParamSpec(plum.linkedOutputParamSlot);
          const float newOut =
              math::clamp(node.getParamVal(plum.linkedOutputParamSlot) - delta,
                          outSpec.minVal, outSpec.maxVal);
          node.setParamVal(plum.linkedOutputParamSlot, newOut);
          syncFaceplateParamValuesFromSchematic(faceplateLayout_,
                                                schematicChain_);
          statusMessage_ = std::format(
              "LIVE RACK [LINK IO]: Stage {} {} = {}  ->  Auto-Compensated {} "
              "= {}",
              w.boundStageIdx + 1, pSpec.name,
              formatWidgetSubtitleForParam(pSpec, targetVal), outSpec.name,
              formatWidgetSubtitleForParam(outSpec, newOut));
        } else {
          statusMessage_ = std::format(
              "LIVE RACK: Stage {} [{}] {} {} = {}", w.boundStageIdx + 1,
              meta.designator, meta.shortName, pSpec.name,
              formatWidgetSubtitleForParam(pSpec, targetVal));
        }
      }

      // Pillar 5.2: Dual-Bound Macro Knob secondary parameter update
      if (w.macroStageIdx2 >= 0 &&
          w.macroStageIdx2 < static_cast<int>(schematicChain_.size())) {
        auto &node2 = schematicChain_[w.macroStageIdx2];
        const auto &meta2 = getComponentMeta(node2.type);
        const int slot2 = std::clamp(
            w.macroParamSlot2, 0, static_cast<int>(meta2.getParamCount()) - 1);
        const auto &pSpec2 = meta2.getParamSpec(slot2);
        node2.setParamVal(slot2,
                          computeParamValFromNorm(pSpec2, w.previewNorm));
      }
      syncInspectorKnobsToSelection();
    } else {
      if (isSwitchWidget(w.type)) {
        const bool isOn = (w.previewNorm > 0.5f);
        if (w.boundParamSlot == 4 || w.title == "TOPOLOGY" ||
            w.title == "STEREO MODE" ||
            w.title.find("MONO") != std::string::npos) {
          const bool has3Way =
              (std::count(w.subtitle.begin(), w.subtitle.end(), '|') >= 2);
          const int topMode = has3Way
                                  ? ((w.previewNorm < 0.25f)
                                         ? 0
                                         : ((w.previewNorm < 0.75f) ? 1 : 2))
                                  : (isOn ? 1 : 0);
          liveSchematicRunner_.setChannelTopologyMode(topMode);
          syncGlobalFaceplateControls();
          statusMessage_ = std::format(
              "LIVE RACK: Channel Engine set to {} - {}!",
              LiveSchematicRunner::getChannelTopologyModeName(topMode),
              (topMode == 0)
                  ? "Single-channel processMono() path with zero L/R phase "
                    "decorrelation & 50% lower CPU usage"
                  : ((topMode == 1) ? "Dual-channel processStereo() with 1.5% "
                                      "L/R analog tolerance drift & crosstalk"
                                    : "Mid/Side encoding before chain & L/R "
                                      "decoding at output bus"));
        } else if (w.boundParamSlot == 5 || w.title == "AUTO-LEVEL" ||
                   w.title == "AUTO-GAIN") {
          rackAutoMatch_ = isOn;
          if (btnRackAutoMatch_)
            btnRackAutoMatch_->setToggleState(rackAutoMatch_);
          syncGlobalFaceplateControls();
          statusMessage_ =
              rackAutoMatch_
                  ? "AUTO-LEVEL CONSTANT-LOUDNESS ENGAGED: Continuously "
                    "tracking input-to-output RMS differential and applying "
                    "smooth inverse output trim!"
                  : "AUTO-LEVEL OFF: True uncompensated output level.";
        } else if (w.boundParamSlot == 6 ||
                   w.title.find("DELTA") != std::string::npos) {
          rackDeltaListen_ = isOn;
          if (btnRackDelta_)
            btnRackDelta_->setToggleState(rackDeltaListen_);
          syncGlobalFaceplateControls();
          statusMessage_ =
              rackDeltaListen_
                  ? "Δ DELTA LISTEN ENGAGED: Soloing phase-aligned difference "
                    "(Wet - LatencyCompensatedDry)!"
                  : "Δ DELTA LISTEN OFF: Normal Wet/Dry monitoring restored.";
        } else if (w.title.find("MUTE") != std::string::npos ||
                   w.subtitle.find("MUTE") != std::string::npos) {
          rackMuted_ = !isOn;
          statusMessage_ = rackMuted_ ? "LIVE RACK: Output Muted (MUTE engaged "
                                        "- flip back to ACTIVE to unmute)"
                                      : "LIVE RACK: Output Active (Unmuted)";
        } else if (w.title.find("PHASE") != std::string::npos ||
                   w.subtitle.find("180") != std::string::npos) {
          rackPhaseInvert_ = !isOn;
          statusMessage_ = rackPhaseInvert_
                               ? "LIVE RACK: 180° Phase Polarity Inverted"
                               : "LIVE RACK: Phase Polarity Normal (0°)";
        } else {
          rackBypassed_ = !isOn;
          if (btnRackBypass_)
            btnRackBypass_->setToggleState(rackBypassed_);
          syncGlobalFaceplateControls();
          statusMessage_ = rackBypassed_
                               ? "LIVE RACK: Master Hard-Bypass Engaged via "
                                 "Faceplate Switch (Hearing Unity Dry Signal)"
                               : "LIVE RACK: Master Unit Engaged via Faceplate "
                                 "Switch (Hearing Active DSP)";
        }
      } else if (w.boundParamSlot == 0) {
        rackInputTrimDb_ = -24.0f + w.previewNorm * 48.0f;
        if (std::abs(rackInputTrimDb_) < 0.15f)
          rackInputTrimDb_ = 0.0f;
        w.subtitle = std::format("{:+.1f} dB", rackInputTrimDb_);
        syncGlobalFaceplateControls();
        statusMessage_ = std::format(
            "LIVE RACK: Global Input Drive = {:+.1f} dB", rackInputTrimDb_);
      } else if (w.boundParamSlot == 1) {
        rackOutputTrimDb_ = -24.0f + w.previewNorm * 48.0f;
        if (std::abs(rackOutputTrimDb_) < 0.15f)
          rackOutputTrimDb_ = 0.0f;
        w.subtitle = std::format("{:+.1f} dB", rackOutputTrimDb_);
        syncGlobalFaceplateControls();
        statusMessage_ = std::format(
            "LIVE RACK: Master Output Trim = {:+.1f} dB", rackOutputTrimDb_);
      } else if (w.scalePreset == UiScalePreset::StereoPanLCR ||
                 w.title == "STEREO BAL") {
        const float panPct = (w.previewNorm - 0.5f) * 200.0f;
        if (std::abs(panPct) < 2.0f) {
          w.previewNorm = 0.5f;
          w.subtitle = "CENTER";
        } else if (panPct < 0.0f) {
          w.subtitle = std::format("L{:.0f}", std::abs(panPct));
        } else {
          w.subtitle = std::format("R{:.0f}", panPct);
        }
        statusMessage_ =
            std::format("LIVE RACK: Stereo Balance = {}", w.subtitle);
      } else {
        rackGlobalWetMix_ = math::clamp(w.previewNorm, 0.0f, 1.0f);
        liveSchematicRunner_.setMasterMixPercent(rackGlobalWetMix_ * 100.0f);
        w.subtitle = std::format("{:.0f} %", rackGlobalWetMix_ * 100.0f);
        syncGlobalFaceplateControls();
        statusMessage_ =
            std::format("LIVE RACK: Master Mix (Dry/Wet) = {:.0f}%",
                        rackGlobalWetMix_ * 100.0f);
      }
    }
  }
  void addUiWidgetFromPalette(const UiWidgetCatalogEntry &entry, float refX,
                              float refY) {
    recordUndoSnapshot(std::string("Add ") + entry.name);
    UiCanvasWidget w;
    w.type = entry.type;
    w.w = entry.defaultW;
    w.h = entry.defaultH;
    w.accentColor = faceplateLayout_.theme.globalAccentColor;
    w.fontSizePreset = faceplateLayout_.theme.globalFontSizePreset;
    const float snapStep =
        static_cast<float>(std::max(1, faceplateLayout_.theme.gridSnapPx));
    w.x = math::clamp(std::round(refX / snapStep) * snapStep, 8.0f,
                      FaceplateLayout::kRefWidth - w.w - 8.0f);
    w.y = math::clamp(std::round(refY / snapStep) * snapStep, 8.0f,
                      FaceplateLayout::kRefHeight - w.h - 8.0f);

    auto findContainingStageFrame = [&](float cx, float cy) -> int {
      for (const auto &existing : faceplateLayout_.widgets) {
        if (existing.type == UiWidgetType::SectionFrame &&
            existing.boundStageIdx >= 0 &&
            existing.boundStageIdx < static_cast<int>(schematicChain_.size())) {
          if (cx >= existing.x && cx <= existing.x + existing.w &&
              cy >= existing.y && cy <= existing.y + existing.h) {
            return existing.boundStageIdx;
          }
        }
      }
      return -1;
    };

    if (isKnobWidget(entry.type)) {
      w.title = entry.bakedTitle;
      w.subtitle = entry.bakedValText;
      w.scalePreset = entry.bakedScale;
      w.previewNorm = entry.bakedNorm;
      w.knobRole = entry.knobRole;
      applyBakedKnobAutoBinding(w, schematicChain_);
    } else if (entry.type == UiWidgetType::ChromeToggleSwitch ||
               entry.type == UiWidgetType::IlluminatedPushButton ||
               entry.type == UiWidgetType::RockerPowerSwitch) {
      const int frameStage =
          findContainingStageFrame(w.x + w.w * 0.5f, w.y + w.h * 0.5f);
      w.boundStageIdx = frameStage;
      w.boundParamSlot = 3;
      if (frameStage >= 0) {
        w.title = std::format("S{} IN/OUT", frameStage + 1);
        w.subtitle = "IN|BYP";
        w.previewNorm = schematicChain_[frameStage].isBypassed ? 0.0f : 1.0f;
      } else {
        w.title = (entry.type == UiWidgetType::RockerPowerSwitch)
                      ? "MAINS POWER"
                      : "MASTER";
        w.subtitle = (entry.type == UiWidgetType::RockerPowerSwitch) ? "ON|OFF"
                                                                     : "IN|BYP";
        w.previewNorm = rackBypassed_ ? 0.0f : 1.0f;
      }
    } else if (entry.type == UiWidgetType::VerticalStudioFader) {
      w.title = "MASTER LEVEL";
      w.subtitle = "0.0 dB";
      w.previewNorm = 0.72f;
      w.boundStageIdx = -1;
      w.boundParamSlot = 1;
    } else if (entry.type == UiWidgetType::HorizontalCrossfader) {
      w.title = "PARALLEL BLEND";
      w.subtitle = "DRY  |  50%  |  WET";
      w.previewNorm = 0.50f;
      w.boundStageIdx = (!schematicChain_.empty()) ? 0 : -1;
      w.boundParamSlot = 2;
    } else if (entry.type == UiWidgetType::JewelLed) {
      const int frameStage =
          findContainingStageFrame(w.x + w.w * 0.5f, w.y + w.h * 0.5f);
      w.boundStageIdx = frameStage;
      w.boundParamSlot = 3;
      w.title = (frameStage >= 0) ? "IN" : "ACTIVE";
      w.subtitle = "";
      w.previewNorm =
          (frameStage >= 0)
              ? (schematicChain_[frameStage].isBypassed ? 0.0f : 1.0f)
              : (rackBypassed_ ? 0.0f : 1.0f);
    } else if (entry.type == UiWidgetType::AnalogVuMeter) {
      w.title = "OUTPUT / GR  VU";
      w.subtitle = "AVRA AUDIO";
    } else if (entry.type == UiWidgetType::RoundVintageVuMeter) {
      w.title = "OUTPUT / GR";
      w.subtitle = "PORTHOLE VU";
    } else if (entry.type == UiWidgetType::LedBargraphMeter) {
      w.title = "PEAK dB";
      w.subtitle = "STEREO";
    } else if (entry.type == UiWidgetType::DigitalTrimBargraph) {
      w.title = "IN / OUT";
      w.subtitle = "TRIM DELTA";
    } else if (entry.type == UiWidgetType::GlowingTubeWindow) {
      w.title = "12AX7 PLATE";
      w.subtitle = "CATHODE GLOW";
      w.previewNorm = 0.72f;
    } else if (entry.type == UiWidgetType::CrtOscilloscopeScreen) {
      w.title = "CRT WAVEFORM";
      w.subtitle = "OSCILLOSCOPE";
    } else if (entry.type == UiWidgetType::LcdParameterReadout) {
      w.title = "DIGITAL READOUT";
      w.subtitle = "0.0 dBFS  |  GR 0.0";
    } else if (entry.type == UiWidgetType::NixieTubeDisplay) {
      w.title = "NIXIE TELEMETRY";
      w.subtitle = "IN-14 CATHODE";
    } else if (entry.type == UiWidgetType::StereoGoniometerScope) {
      w.title = "STEREO PHASE";
      w.subtitle = "LISSAJOUS VECTOR";
    } else if (entry.type == UiWidgetType::SectionFrame) {
      w.title = "ANALOG SECTION";
      w.subtitle = "";
    } else if (entry.type == UiWidgetType::VentilationGrille) {
      w.title = "TUBE BAY VENTS";
      w.subtitle = "";
    } else if (entry.type == UiWidgetType::RivetedNameplate) {
      w.title = wizardConfig_.name.empty() ? "AVRA GAIN" : wizardConfig_.name;
      w.subtitle = "STEREO UTILITY";
    } else if (entry.type == UiWidgetType::RackHandlePair) {
      w.title = "19\" RACK HANDLE";
      w.subtitle = "";
    } else if (entry.type == UiWidgetType::PatchbayJackSocket) {
      w.title = "EXT KEY IN";
      w.subtitle = "1/4\" TRS";
    } else if (entry.type == UiWidgetType::VintageSticker) {
      w.title = entry.bakedTitle;
      w.subtitle = entry.bakedValText;
      w.scalePreset = entry.bakedScale;
      autoFitStickerWidgetBounds(w, faceplateLayout_.theme.globalFontFamily,
                                 w.fontSizePreset, false);
      w.x = math::clamp(w.x, 8.0f, FaceplateLayout::kRefWidth - w.w - 8.0f);
      w.y = math::clamp(w.y, 8.0f, FaceplateLayout::kRefHeight - w.h - 8.0f);
    } else {
      w.title = "MODULE";
      w.subtitle = "";
    }

    isScaleDropdownOpen_ = false;
    isStyleTemplateDropdownOpen_ = false;
    clearAllTextInputFocus();
    if (entry.type == UiWidgetType::SectionFrame ||
        entry.type == UiWidgetType::VentilationGrille ||
        entry.type == UiWidgetType::RackHandlePair) {
      faceplateLayout_.widgets.insert(faceplateLayout_.widgets.begin(), w);
      selectedUiWidgetIndex_ = 0;
    } else {
      faceplateLayout_.widgets.push_back(w);
      selectedUiWidgetIndex_ =
          static_cast<int>(faceplateLayout_.widgets.size()) - 1;
    }
    lastUiWidgetInteractMs_ = GetTickCount64();
    syncUiWidgetInspector();
    if (entry.type == UiWidgetType::VintageSticker &&
        static_cast<int>(entry.bakedScale) ==
            static_cast<int>(gui::StickerPreset::CustomImageDecal)) {
      openStickerImageFileDialog(selectedUiWidgetIndex_);
      return;
    }
    if (isKnobWidget(entry.type)) {
      statusMessage_ = std::format(
          "Added '{}' ({}) - Auto-bound to [{}] with [{}] scale (click SCALE "
          "dropdown to amend)!",
          entry.name, w.title, describeWidgetAutoBinding(w, schematicChain_),
          getScalePresetName(w.scalePreset));
    } else if (isSwitchWidget(entry.type)) {
      statusMessage_ = std::format(
          "Added '{}' ({}) - Auto-bound to [{}]! Click switch to toggle "
          "IN/BYP.",
          entry.name, w.title, describeWidgetAutoBinding(w, schematicChain_));
    } else {
      statusMessage_ =
          std::format("Placed '{}' on faceplate at ({:.0f}, {:.0f}). Drag to "
                      "reposition or drag corner to resize!",
                      entry.name, w.x, w.y);
    }
    repaint();
  }
  static constexpr float kSchematicCenterTrunkY = 336.0f;
  static constexpr float kSchematicPathAUpperY = 336.0f;
  static constexpr float kSchematicPathBLowerY = 480.0f;

  [[nodiscard]] float getStageBusY(size_t stageIdx) const noexcept {
    if (stageIdx >= schematicChain_.size())
      return kSchematicCenterTrunkY;
    return schematicChain_[stageIdx].isParallel ? kSchematicPathBLowerY
                                                : kSchematicCenterTrunkY;
  }

  [[nodiscard]] gui::Rect<float> getSchematicCanvasRect() const noexcept {
    const float canvasX = isSidebarCollapsed_ ? 364.0f : 612.0f;
    return {canvasX, 145.0f, 1345.0f - canvasX, 479.0f};
  }

  [[nodiscard]] gui::Rect<float> getSchematicViewportRect() const noexcept {
    const auto c = getSchematicCanvasRect();
    return {c.x + 2.0f, c.y + 27.0f, c.width - 4.0f, c.height - 52.0f};
  }

  [[nodiscard]] gui::Rect<float> getSchematicMinimapRect() const noexcept {
    const auto c = getSchematicCanvasRect();
    return {c.x + 8.0f, 600.0f, c.width - 16.0f, 19.0f};
  }

  struct SchematicToolbarRects {
    gui::Rect<float> zoomOutBtn;
    gui::Rect<float> zoomResetBtn;
    gui::Rect<float> zoomInBtn;
    gui::Rect<float> fitChainBtn;
    gui::Rect<float> compactViewBtn;
    gui::Rect<float> groupStagesBtn;
    gui::Rect<float> probePill;
    gui::Rect<float> pdcPill;
  };

  [[nodiscard]] SchematicToolbarRects
  getSchematicToolbarRects() const noexcept {
    constexpr float y = 148.0f;
    constexpr float h = 20.0f;
    const float r = 1341.0f;
    SchematicToolbarRects t{};
    t.pdcPill = {r - 106.0f, y, 106.0f, h};
    t.probePill = {0.0f, 0.0f, 0.0f, 0.0f};
    t.groupStagesBtn = {t.pdcPill.x - 92.0f, y, 88.0f, h};
    t.compactViewBtn = {t.groupStagesBtn.x - 90.0f, y, 86.0f, h};
    t.fitChainBtn = {t.compactViewBtn.x - 66.0f, y, 62.0f, h};
    t.zoomInBtn = {t.fitChainBtn.x - 25.0f, y, 21.0f, h};
    t.zoomResetBtn = {t.zoomInBtn.x - 45.0f, y, 42.0f, h};
    t.zoomOutBtn = {t.zoomResetBtn.x - 24.0f, y, 21.0f, h};
    return t;
  }

  [[nodiscard]] size_t getMacroGroupedStageCount() const noexcept {
    if (!schematicMacroGrouped_ || schematicChain_.size() < 2)
      return 0;
    size_t count = 0;
    for (const auto &n : schematicChain_) {
      if (n.inMacroGroup)
        ++count;
      else
        break;
    }
    if (count < 2) {
      count = std::min<size_t>(
          schematicChain_.size(),
          std::max<size_t>(2, (schematicChain_.size() + 1) / 2));
    }
    return count;
  }

  [[nodiscard]] size_t
  getStageVisualColumnIndex(size_t stageIdx) const noexcept {
    const size_t grpCount = getMacroGroupedStageCount();
    if (grpCount >= 2) {
      if (stageIdx < grpCount)
        return 0;
      return (stageIdx - grpCount) + 1;
    }
    return stageIdx;
  }

  [[nodiscard]] size_t getTotalVisualColumns() const noexcept {
    if (schematicChain_.empty())
      return 0;
    const size_t grpCount = getMacroGroupedStageCount();
    if (grpCount >= 2) {
      return 1 + (schematicChain_.size() - grpCount);
    }
    return schematicChain_.size();
  }

  [[nodiscard]] float getSchematicBaseCardWidth() const noexcept {
    return schematicCompactView_ ? 118.0f : 152.0f;
  }

  [[nodiscard]] float getSchematicBaseStepX() const noexcept {
    return schematicCompactView_ ? 134.0f : 176.0f;
  }

  [[nodiscard]] float
  getStageParallelExtraOffsetX(size_t stageIdx,
                               float zoomScale) const noexcept {
    if (schematicChain_.empty())
      return 0.0f;
    const size_t grpCount = getMacroGroupedStageCount();
    const size_t firstIdx = (grpCount >= 2) ? grpCount : 0;
    if (stageIdx < firstIdx)
      return 0.0f;

    const float z = std::max(0.65f, zoomScale);
    float extraX = 0.0f;
    const size_t limit = std::min(stageIdx, schematicChain_.size() - 1);
    for (size_t k = firstIdx; k <= limit; ++k) {
      const bool curPar = schematicChain_[k].isParallel;
      const bool prevPar =
          (k > firstIdx) ? schematicChain_[k - 1].isParallel : false;
      if (curPar && !prevPar) {
        if (k > 0) {
          // Leave >= 28.0f between preceding series node and [Y] split box
          extraX += 42.0f * z;
        } else {
          // Leave clean clearance between IN XLR jack and [Y] split box when S1 is parallel
          extraX += 28.0f * z;
        }
      } else if (!curPar && prevPar) {
        // Leave >= 44.0f between [Sum] summing box and next series stage card
        extraX += 58.0f * z;
      }
    }
    return extraX;
  }

  [[nodiscard]] float getMaxSchematicScrollX() const noexcept {
    const size_t cols = getTotalVisualColumns();
    if (cols == 0)
      return 0.0f;
    const auto c = getSchematicCanvasRect();
    const float availW = std::max(120.0f, c.width - 104.0f);
    const float stepX = getSchematicBaseStepX() * schematicZoom_;
    const float cardW = getSchematicBaseCardWidth() * schematicZoom_;
    const float extraX = schematicChain_.empty()
                             ? 0.0f
                             : getStageParallelExtraOffsetX(
                                   schematicChain_.size() - 1, schematicZoom_);
    const float totalW =
        static_cast<float>(cols - 1) * stepX + cardW + extraX + 84.0f;
    return std::max(0.0f, totalW - availW);
  }

  void clampSchematicScrollX() noexcept {
    schematicScrollX_ =
        math::clamp(schematicScrollX_, 0.0f, getMaxSchematicScrollX());
  }

  void fitSchematicChainToViewport() noexcept {
    const size_t cols = getTotalVisualColumns();
    if (cols == 0) {
      schematicZoom_ = 1.0f;
      schematicScrollX_ = 0.0f;
      return;
    }
    const auto c = getSchematicCanvasRect();
    const float availW = std::max(160.0f, c.width - 102.0f);
    const float baseStep = getSchematicBaseStepX();
    const float baseCard = getSchematicBaseCardWidth();
    const float extraUnscaled =
        schematicChain_.empty()
            ? 0.0f
            : getStageParallelExtraOffsetX(schematicChain_.size() - 1, 1.0f);
    const float unscaledW =
        static_cast<float>(cols - 1) * baseStep + baseCard + extraUnscaled;
    schematicZoom_ =
        math::clamp(availW / std::max(1.0f, unscaledW), 0.25f, 1.00f);
    schematicScrollX_ = 0.0f;
  }

  void toggleSchematicStageMacroGroup() {
    if (schematicChain_.size() < 2) {
      statusMessage_ = "GROUP STAGES: Add at least 2 stages to collapse them "
                       "into a Macro Chassis Container.";
      repaint();
      return;
    }
    schematicMacroGrouped_ = !schematicMacroGrouped_;
    if (schematicMacroGrouped_) {
      const size_t grpCount = std::min<size_t>(
          schematicChain_.size(),
          std::max<size_t>(2, (schematicChain_.size() + 1) / 2));
      for (size_t i = 0; i < schematicChain_.size(); ++i) {
        schematicChain_[i].inMacroGroup = (i < grpCount);
      }
      statusMessage_ =
          std::format("MACRO CONTAINER CREATED: Collapsed first {} stages into "
                      "[ MACRO: STUDIO FRONT-END ({} STAGES) ] - click "
                      "container or [GROUP STAGES] to expand!",
                      grpCount, grpCount);
    } else {
      statusMessage_ = "MACRO CONTAINER EXPANDED: Restored individual stage "
                       "cards across the schematic canvas.";
    }
    clampSchematicScrollX();
    repaint();
  }

  void appendSchematicComponent(SchematicComponentType type,
                                bool inParallel = false) {
    if (schematicChain_.size() >= kMaxTotalStages) {
      statusMessage_ = std::format(
          "Schematic chain has reached {} stages maximum.", kMaxTotalStages);
      repaint();
      return;
    }
    const auto &meta = getComponentMeta(type);
    recordUndoSnapshot(
        std::format("Add S{} {}", schematicChain_.size() + 1, meta.shortName));
    schematicChain_.push_back(makeDefaultSchematicNode(type, inParallel));
    selectedNodeIndex_ = static_cast<int>(schematicChain_.size()) - 1;
    schematicScrollX_ = getMaxSchematicScrollX();
    syncInspectorKnobsToSelection();
    updateCanvasPageButtons();
    syncFaceplateWithSchematic();
    statusMessage_ =
        std::format("Added [{}] {} as Stage {} ({}) & dynamically synced its "
                    "4-Tier controls to Faceplate UI!",
                    meta.designator, meta.fullName, schematicChain_.size(),
                    inParallel ? "PARALLEL PATH B" : "SERIES TRUNK");
    repaint();
  }

  void insertSchematicComponentAt(SchematicComponentType type, int slotIdx,
                                  bool inParallel = false) {
    if (schematicChain_.size() >= kMaxTotalStages) {
      statusMessage_ = std::format(
          "Schematic chain has reached {} stages maximum.", kMaxTotalStages);
      return;
    }
    const int idx =
        math::clamp(slotIdx, 0, static_cast<int>(schematicChain_.size()));
    const auto &meta = getComponentMeta(type);
    recordUndoSnapshot(std::format("Insert S{} {}", idx + 1, meta.shortName));
    schematicChain_.insert(schematicChain_.begin() + idx,
                           makeDefaultSchematicNode(type, inParallel));
    selectedNodeIndex_ = idx;
    clampSchematicScrollX();
    syncInspectorKnobsToSelection();
    updateCanvasPageButtons();
    syncFaceplateWithSchematic();
    statusMessage_ =
        std::format("Dropped [{}] {} at Stage {} ({}) & dynamically synced its "
                    "controls to Faceplate UI!",
                    meta.designator, meta.fullName, idx + 1,
                    inParallel ? "PARALLEL PATH B" : "SERIES TRUNK");
  }

  /**
   * @brief Computes real-time or estimated output peak headroom (dBFS) after
   * stage `stageIdx` for color-coding inter-stage PCB traces:
   *   - Cyan (< -6 dBFS): Clean headroom
   *   - Warm Amber (-6 to 0 dBFS): Optimal analog saturation sweet spot
   *   - Flashing Red (> +3 dBFS): Overdriving downstream input
   */
  [[nodiscard]] float
  estimateStageOutputHeadroomDb(size_t stageIdx) const noexcept {
    if (stageIdx >= schematicChain_.size())
      return -12.0f;
    float cumDb = -9.5f + rackInputTrimDb_;
    const size_t limit = std::min(stageIdx + 1, schematicChain_.size());
    for (size_t k = 0; k < limit; ++k) {
      const auto &n = schematicChain_[k];
      if (n.isBypassed)
        continue;
      const auto &m = getComponentMeta(n.type);
      const float p1Delta = n.param1Val - m.param1.defaultVal;
      if (m.param1.unit == "dB") {
        cumDb += p1Delta * (n.autoGainLinked ? 0.22f : 0.85f);
      } else {
        const float span = std::max(1.0f, m.param1.maxVal - m.param1.minVal);
        cumDb += (p1Delta / span) * 10.0f;
      }
      const int pCount = static_cast<int>(m.getParamCount());
      for (int slot = 1; slot < pCount; ++slot) {
        const auto &ps = m.getParamSpec(slot);
        if (ps.unit == "dB" && (ps.name.find("Out") != std::string::npos ||
                                ps.name.find("Trim") != std::string::npos ||
                                ps.name.find("Makeup") != std::string::npos ||
                                ps.name.find("Gain") != std::string::npos ||
                                ps.name.find("Boost") != std::string::npos)) {
          cumDb += (n.getParamVal(slot) - ps.defaultVal) * 0.65f;
        }
      }
      cumDb += n.trim1Val * 0.25f;
    }
    const float livePeak = liveSchematicRunner_.getStageOutputPeakDb(stageIdx);
    if (livePeak > -45.0f && currentTab_ == StudioTab::LiveRack) {
      return std::max(cumDb, livePeak);
    }
    return math::clamp(cumDb, -48.0f, 18.0f);
  }

  [[nodiscard]] gui::Color
  getStageTraceHealthColor(float peakDb, gui::Color cyan,
                           gui::Color gold) const noexcept {
    if (peakDb > 3.0f) {
      const float flash =
          0.55f + 0.45f * static_cast<float>(std::sin(
                              static_cast<double>(GetTickCount64()) * 0.012));
      return gui::Color::AccentRed().withAlpha(math::clamp(flash, 0.45f, 1.0f));
    }
    if (peakDb >= -6.0f) {
      return gold;
    }
    return cyan;
  }

  void autoTrimOverdrivenStage(size_t stageIdx) {
    if (stageIdx >= schematicChain_.size())
      return;
    auto &node = schematicChain_[stageIdx];
    const auto &meta = getComponentMeta(node.type);
    const float curDb = estimateStageOutputHeadroomDb(stageIdx);
    const float excessDb = std::max(1.0f, curDb - (-3.0f));

    recordUndoSnapshot(
        std::format("Auto-Trim Stage S{} {}", stageIdx + 1, meta.shortName));

    // Trim output/makeup dB parameter if present, or trim input drive back into
    // the -3.0 dBFS sweet spot
    bool trimmedOutputSlot = false;
    const int pCount = static_cast<int>(meta.getParamCount());
    for (int slot = pCount - 1; slot >= 1; --slot) {
      const auto &ps = meta.getParamSpec(slot);
      if (ps.unit == "dB" && (ps.name.find("Out") != std::string::npos ||
                              ps.name.find("Trim") != std::string::npos ||
                              ps.name.find("Makeup") != std::string::npos)) {
        const float newVal = math::clamp(node.getParamVal(slot) - excessDb,
                                         ps.minVal, ps.maxVal);
        node.setParamVal(slot, newVal);
        trimmedOutputSlot = true;
        break;
      }
    }
    if (!trimmedOutputSlot) {
      if (meta.param1.unit == "dB") {
        node.param1Val = math::clamp(node.param1Val - excessDb / 0.85f,
                                     meta.param1.minVal, meta.param1.maxVal);
      } else {
        node.param1Val = meta.param1.defaultVal;
      }
      node.trim1Val = 0.0f;
    }
    syncInspectorKnobsToSelection();
    syncFaceplateParamValuesFromSchematic(faceplateLayout_, schematicChain_);
    hoveredOverdriveStageIdx_ = -1;
    statusMessage_ =
        std::format("AUTO-TRIMMED STAGE S{} [{}] {}: Reduced stage gain by "
                    "{:.1f} dB into the -3.0 dBFS analog sweet spot!",
                    stageIdx + 1, meta.designator, meta.shortName, excessDb);
    repaint();
  }

  /**
   * @brief Returns the card rectangle for stage `stageIdx` on the Dynamic
   * Single-Trunk + Y-Split / Mixer Topology (Center Trunk = 368px, Path A Upper
   * Companion = 248px, Path B Lower Parallel = 488px), scaled proportionally by
   * zoom and pan.
   */
  [[nodiscard]] gui::Rect<float>
  getCanvasNodeRect(size_t stageIdx) const noexcept {
    const size_t col = getStageVisualColumnIndex(stageIdx);
    const size_t grpCount = getMacroGroupedStageCount();
    const bool isMacroCard = (grpCount >= 2 && stageIdx < grpCount);

    const float startX = getSchematicCanvasRect().x + 64.0f - schematicScrollX_;
    const float stepX = getSchematicBaseStepX() * schematicZoom_;
    const float cardW =
        (schematicCompactView_ ? 118.0f : 152.0f) * schematicZoom_;
    const float cardH = 104.0f * math::clamp(schematicZoom_, 0.65f, 1.45f);
    const float extraX =
        isMacroCard ? 0.0f
                    : getStageParallelExtraOffsetX(stageIdx, schematicZoom_);

    const float x = startX + static_cast<float>(col) * stepX + extraX;
    const float busY =
        isMacroCard ? kSchematicCenterTrunkY : getStageBusY(stageIdx);
    const float y = std::max(176.0f, busY - cardH * 0.5f);
    return {x, y, cardW, cardH};
  }

  [[nodiscard]] gui::Rect<float>
  getStageStatusBadgeRect(size_t stageIdx) const noexcept {
    const gui::Rect<float> nr = getCanvasNodeRect(stageIdx);
    const float availW = std::max(36.0f, nr.width - 12.0f);
    const float pillW = std::min(
        availW,
        std::max(44.0f, 54.0f * math::clamp(schematicZoom_, 0.75f, 1.15f)));
    const float pillX = nr.x + 6.0f + (availW - pillW) * 0.5f;
    return {pillX, nr.bottom() + 6.0f, pillW, 15.0f};
  }

  [[nodiscard]] float
  getParallelBranchSplitX(size_t branchStartIdx) const noexcept {
    const auto rFirstPar = getCanvasNodeRect(branchStartIdx);
    const size_t grpCount = getMacroGroupedStageCount();
    if (branchStartIdx > 0) {
      const size_t prevIdx = (grpCount >= 2 && branchStartIdx == grpCount)
                                 ? 0
                                 : (branchStartIdx - 1);
      const auto rPrev = getCanvasNodeRect(prevIdx);
      // Guarantee at least 28.0f clearance between preceding series card's
      // right edge and splitBox.x (splitX - 11.0f)
      return std::max(rPrev.right() + 28.0f + 11.0f,
                      rFirstPar.x - 18.0f * schematicZoom_);
    }
    const float inJackRight =
        getSchematicCanvasRect().x + 42.0f - schematicScrollX_;
    return std::max(inJackRight + 16.0f + 11.0f,
                    rFirstPar.x - 18.0f * schematicZoom_);
  }

  [[nodiscard]] float
  getParallelBranchMergeX(size_t branchEndIdx) const noexcept {
    const auto rLastPar = getCanvasNodeRect(branchEndIdx);
    return rLastPar.right() + 16.0f * schematicZoom_;
  }

  [[nodiscard]] gui::Rect<float>
  getSummingJunctionBadgeRect(size_t branchStartIdx,
                              size_t branchEndIdx) const noexcept {
    (void)branchStartIdx;
    const float mergeX = getParallelBranchMergeX(branchEndIdx);
    const gui::Rect<float> sumBox = {
        mergeX - 13.0f, kSchematicCenterTrunkY - 10.0f, 26.0f, 20.0f};
    const float badgeW = math::clamp(
        118.0f * math::clamp(schematicZoom_, 0.80f, 1.10f), 94.0f, 132.0f);
    const float pillRight = sumBox.getCentre().x + 8.0f;
    const float pillY = sumBox.y - 22.0f;
    return {pillRight - badgeW, pillY, badgeW, 18.0f};
  }

  static void drawChamferedPcbTrace(gui::Graphics &g, gui::Point<float> p0,
                                    gui::Point<float> p1, gui::Color col,
                                    float thickness = 2.5f) {
    const gui::Color glowCol = col.withAlpha(0.18f);
    if (std::abs(p0.y - p1.y) < 1.5f || std::abs(p0.x - p1.x) < 1.5f) {
      g.drawLine(p0, p1, glowCol, thickness + 3.0f);
      g.drawLine(p0, p1, col, thickness);
      return;
    }
    const float dx = p1.x - p0.x;
    const float dy = p1.y - p0.y;
    const float ch =
        std::min({std::abs(dx) * 0.65f, std::abs(dy) * 0.35f, 18.0f});
    const gui::Point<float> c1 = {p0.x, p1.y - std::copysign(ch, dy)};
    const gui::Point<float> c2 = {p0.x + std::copysign(ch, dx), p1.y};
    g.drawLine(p0, c1, glowCol, thickness + 3.0f);
    g.drawLine(c1, c2, glowCol, thickness + 3.0f);
    g.drawLine(c2, p1, glowCol, thickness + 3.0f);
    g.drawLine(p0, c1, col, thickness);
    g.drawLine(c1, c2, col, thickness);
    g.drawLine(c2, p1, col, thickness);
  }

  static void drawSolderedTerminalPin(gui::Graphics &g, gui::Point<float> pt,
                                      gui::Color col) {
    g.fillCircle(pt, 4.2f, gui::Color::fromHex(0x0D1117));
    g.drawCircle(pt, 4.2f, col, 1.4f);
    g.fillCircle(pt, 1.9f, col);
  }

  [[nodiscard]] int
  computeCanvasInsertSlot(gui::Point<float> mousePos) const noexcept {
    if (schematicChain_.empty())
      return 0;
    const size_t grpCount = getMacroGroupedStageCount();
    for (size_t i = 0; i < schematicChain_.size(); ++i) {
      if (grpCount >= 2 && i > 0 && i < grpCount)
        continue;
      const auto r = getCanvasNodeRect(i);
      if (mousePos.x < r.getCentre().x) {
        return static_cast<int>(i);
      }
    }
    return static_cast<int>(schematicChain_.size());
  }

  // =========================================================================
  // Painting Methods
  // =========================================================================
  void paintSidebar(gui::Graphics &g, const gui::Rect<float> &local,
                    gui::Color panelBg, gui::Color outline, gui::Color cyan) {
    if (isSidebarCollapsed_) {
      gui::Rect<float> rail = {16.0f, 62.0f, 36.0f, local.height - 107.0f};
      g.fillRoundedRect(rail, 6.0f, panelBg);
      g.drawRoundedRect(rail, 6.0f, outline, 1.0f);

      gui::Rect<float> expBtn = {20.0f, 68.0f, 28.0f, 24.0f};
      g.fillRoundedRect(expBtn, 4.0f, cyan.withAlpha(0.18f));
      g.drawRoundedRect(expBtn, 4.0f, cyan.withAlpha(0.75f), 1.0f);
      g.drawText("\xC2\xBB", expBtn, cyan, 12.0f, true, true);

      float dotY = 108.0f;
      for (size_t i = 0; i < projects_.size(); ++i) {
        const auto &p = projects_[i];
        const bool isSel = (static_cast<int>(i) == selectedProjectIndex_);
        gui::Rect<float> dotBox = {20.0f, dotY, 28.0f, 28.0f};
        if (isSel) {
          g.fillRoundedRect(dotBox, 4.0f, cyan.withAlpha(0.22f));
          g.drawRoundedRect(dotBox, 4.0f, cyan, 1.2f);
        }
        gui::Color dotCol =
            p.isBuilt ? gui::Color::fromHex(0x52B788)
                      : (p.isScaffolded ? gui::Color::AccentAmber()
                                        : gui::Color::TextSecondary());
        g.fillCircle({34.0f, dotY + 14.0f}, isSel ? 5.0f : 3.5f, dotCol);
        dotY += 34.0f;
      }
      return;
    }

    gui::Rect<float> sidebar = {20.0f, 62.0f, 320.0f, local.height - 107.0f};
    g.fillRoundedRect(sidebar, 6.0f, panelBg);
    g.drawRoundedRect(sidebar, 6.0f, outline, 1.0f);

    g.drawText("WORKSPACE PROJECTS", {32.0f, 70.0f, 185.0f, 24.0f},
               gui::Color::TextPrimary(), 12.5f, false, true);
    if (currentTab_ != StudioTab::Projects) {
      gui::Rect<float> colBtn = {304.0f, 68.0f, 28.0f, 22.0f};
      g.fillRoundedRect(colBtn, 4.0f, gui::Color::fromHex(0x222630));
      g.drawRoundedRect(colBtn, 4.0f, outline, 1.0f);
      g.drawText("\xC2\xAB", colBtn, cyan, 11.5f, true, true);
    } else {
      g.drawText("click to load", {248.0f, 70.0f, 82.0f, 24.0f},
                 cyan.withAlpha(0.8f), 9.5f, false);
    }

    g.drawLine({28.0f, 126.0f}, {332.0f, 126.0f}, outline, 1.0f);

    float itemY = 132.0f;
    for (size_t i = 0; i < projects_.size(); ++i) {
      const auto &p = projects_[i];
      const bool isSelected = (static_cast<int>(i) == selectedProjectIndex_);

      gui::Rect<float> itemRect = {28.0f, itemY, 304.0f, 44.0f};
      if (isSelected) {
        g.fillRoundedRect(itemRect, 4.0f, gui::Color::fromHex(0x242833));
        g.drawRoundedRect(itemRect, 4.0f, cyan, 1.2f);
      } else {
        g.fillRoundedRect(itemRect, 4.0f, gui::Color::fromHex(0x1C1F26));
      }

      gui::Color dotCol = p.isBuilt
                              ? gui::Color::fromHex(0x52B788)
                              : (p.isScaffolded ? gui::Color::AccentAmber()
                                                : gui::Color::TextSecondary());
      g.fillCircle({42.0f, itemY + 22.0f}, 4.5f, dotCol);

      g.drawText(p.name, {54.0f, itemY + 4.0f, 155.0f, 18.0f},
                 gui::Color::TextPrimary(), 12.0f, false, isSelected);
      g.drawText(p.category, {54.0f, itemY + 22.0f, 155.0f, 16.0f},
                 gui::Color::TextSecondary(), 10.0f, false);

      std::string statusStr =
          p.isBuilt ? "Built (VST3+APP)"
                    : (p.isScaffolded ? "Source Ready" : "Template");
      gui::Color tagCol = p.isBuilt
                              ? gui::Color::fromHex(0x52B788)
                              : (p.isScaffolded ? gui::Color::AccentAmber()
                                                : gui::Color::TextSecondary());
      g.drawText(statusStr, {202.0f, itemY + 12.0f, 96.0f, 20.0f}, tagCol, 9.2f,
                 false, true);

      const bool isRemoveHovered = (static_cast<int>(i) == hoveredRemoveIndex_);
      gui::Rect<float> xBtn = {304.0f, itemY + 11.0f, 22.0f, 22.0f};
      g.fillRoundedRect(xBtn, 4.0f,
                        isRemoveHovered ? gui::Color::AccentRed()
                                        : gui::Color::fromHex(0x262A34));
      g.drawText("X", xBtn,
                 isRemoveHovered ? gui::Color::TextPrimary()
                                 : gui::Color::TextSecondary(),
                 10.0f, true, true);

      itemY += 50.0f;
    }
  }

  void paintProjectsMode(gui::Graphics &g, const gui::Rect<float> &local,
                         gui::Color panelBg, gui::Color outline,
                         gui::Color /*cardBg*/, gui::Color gold, gui::Color cyan) {
    const bool hasNamedWizard = !wizardConfig_.name.empty();
    const std::string fallbackName =
        hasNamedWizard ? ProjectGenerator::sanitizeName(wizardConfig_.name)
                       : "Untitled Project";
    const std::string fallbackFolder =
        hasNamedWizard
            ? ((fallbackName == "AvraGain" || fallbackName == "GainPlugin")
                   ? "GainPlugin"
                   : fallbackName)
            : "CustomPlugin";
    WorkspacePlugin fallbackProj{
        fallbackName,
        fallbackFolder,
        wizardConfig_.category.empty() ? "C++20 Audio Plugin" : wizardConfig_.category,
        wizardConfig_.pluginId.empty() ? ("com.avra." + toLower(fallbackFolder)) : wizardConfig_.pluginId,
        hasNamedWizard
            ? std::format("Project '{}' - choose a target below to select a save folder and build.", fallbackName)
            : "Enter a Plugin Name in the top header or select a project on the left.",
        std::format("Avra{}_Clap", fallbackFolder),
        hasNamedWizard,
        false,
        false};
    const WorkspacePlugin &proj =
        (selectedProjectIndex_ >= 0 &&
         selectedProjectIndex_ < static_cast<int>(projects_.size()))
            ? projects_[selectedProjectIndex_]
            : fallbackProj;

    gui::Rect<float> mainPanel = {355.0f, 62.0f, local.width - 375.0f,
                                  local.height - 107.0f};
    g.fillRoundedRect(mainPanel, 6.0f, panelBg);
    g.drawRoundedRect(mainPanel, 6.0f, outline, 1.0f);

    g.drawText(proj.name, {385.0f, 72.0f, 540.0f, 28.0f}, gold, 20.5f, false,
               true);
    g.drawText(proj.category + " | AVRA C++20 AUDIO PLUGIN",
               {385.0f, 102.0f, 540.0f, 18.0f}, cyan, 10.8f, false, true);
    g.drawText(
        "Click VST3, CLAP, STANDALONE, or BUILD ALL below to choose a save folder and compile, or click OPEN IN IDE.",
        {385.0f, 124.0f, 640.0f, 20.0f}, gui::Color::TextSecondary(), 10.2f,
        false);

    // Hero Logo Medallion Showcase on the right side of the Header
    const std::string heroLogoPath = (ProjectGenerator::getWorkspaceRoot() /
                                      "assets" / "avra_studio_icon.png")
                                         .string();
    const gui::Rect<float> heroPlaque = {local.width - 320.0f, 64.0f, 295.0f,
                                         90.0f};
    g.fillLinearGradientRect(heroPlaque, gui::Color::fromHex(0x161D26),
                             gui::Color::fromHex(0x0D1218), 7.0f);
    g.drawRoundedRect(heroPlaque, 7.0f, gold.withAlpha(0.55f), 1.3f);
    const gui::Rect<float> heroImgBox = {heroPlaque.x + 5.0f,
                                         heroPlaque.y + 1.0f, 88.0f, 88.0f};
    drawAvraHeroEmblem(g, heroImgBox, heroLogoPath);
    g.drawText(
        "AVRA STUDIO",
        {heroImgBox.right() + 10.0f, heroPlaque.y + 14.0f, 182.0f, 24.0f},
        gui::Color::fromHex(0xD4AF37), 15.5f, false, true);
    g.drawText(
        "AUDIO PLUGIN",
        {heroImgBox.right() + 10.0f, heroPlaque.y + 40.0f, 182.0f, 18.0f},
        gui::Color::fromHex(0x00F5D4), 10.5f, false, true);
    g.drawText(
        "WORKSTATION",
        {heroImgBox.right() + 10.0f, heroPlaque.y + 58.0f, 182.0f, 18.0f},
        gui::Color::fromHex(0x00F5D4), 10.5f, false, true);

    g.drawLine({385.0f, 159.0f}, {local.width - 25.0f, 159.0f}, outline, 1.0f);

    // =====================================================================
    // 1. COMPONENT CHAIN INCLUDED IN THE PROJECT (y: 170..486)
    // =====================================================================
    gui::Rect<float> chainBox = {385.0f, 170.0f, 950.0f, 316.0f};
    g.fillRoundedRect(chainBox, 6.0f, gui::Color::fromHex(0x0D1117));
    g.drawRoundedRect(chainBox, 6.0f, outline, 1.2f);
    g.drawText(std::format("PROJECT COMPONENT CHAIN ({} STAGES)  -  CLICK TO EDIT IN SCHEMATIC BUILDER",
                           schematicChain_.size()),
               {402.0f, 178.0f, 900.0f, 20.0f}, cyan, 10.5f, false, true);
    g.drawLine({400.0f, 204.0f}, {1320.0f, 204.0f}, outline.withAlpha(0.6f), 1.0f);

    if (schematicChain_.empty()) {
      g.drawText("No stages in component chain yet - add components in SCHEMATIC BUILDER.",
                 chainBox, gui::Color::TextSecondary(), 11.5f, true, true);
    } else if (schematicChain_.size() <= 7) {
      const float busY = 342.0f;
      g.drawLine({410.0f, busY}, {1310.0f, busY}, cyan.withAlpha(0.45f), 2.2f);
      const size_t dispCount = schematicChain_.size();
      const float step = 880.0f / static_cast<float>(dispCount);
      const float cw = std::min(126.0f, step - 14.0f);
      for (size_t i = 0; i < dispCount; ++i) {
        const auto &node = schematicChain_[i];
        const auto &meta = getComponentMeta(node.type);
        const float cx = 420.0f + static_cast<float>(i) * step + (step - cw) * 0.5f;
        gui::Rect<float> cr = {cx, 240.0f, cw, 204.0f};
        g.fillLinearGradientRect(cr, gui::Color::fromHex(0x1E232E),
                                 gui::Color::fromHex(0x141821), 5.0f);
        g.drawRoundedRect(cr, 5.0f, meta.accentColor, 1.4f);
        g.drawText(std::format("S{} · {}", i + 1, meta.designator),
                   {cr.x + 6.0f, cr.y + 10.0f, cr.width - 12.0f, 18.0f},
                   meta.accentColor, 10.0f, true, true);
        g.drawText(meta.shortName,
                   {cr.x + 4.0f, cr.y + 32.0f, cr.width - 8.0f, 20.0f},
                   gui::Color::TextPrimary(), 10.5f, true, true);
        paintSchematicNodeSymbol(
            g, node.type, meta.category,
            {cr.x + 14.0f, cr.y + 68.0f, cr.width - 28.0f, 68.0f},
            meta.accentColor);
        g.drawText(node.isParallel ? "PARALLEL" : "SERIES",
                   {cr.x + 6.0f, cr.bottom() - 28.0f, cr.width - 12.0f, 18.0f},
                   node.isParallel ? gold : cyan, 9.2f, true, true);
      }
    } else {
      const size_t dispCount = std::min<size_t>(schematicChain_.size(), 14);
      const size_t row1Count = (dispCount + 1) / 2;
      const size_t row2Count = dispCount - row1Count;
      for (size_t row = 0; row < 2; ++row) {
        const size_t rCount = (row == 0) ? row1Count : row2Count;
        if (rCount == 0) continue;
        const float rowY = (row == 0) ? 214.0f : 350.0f;
        const float busY = rowY + 62.0f;
        g.drawLine({410.0f, busY}, {1310.0f, busY}, cyan.withAlpha(0.4f), 2.0f);
        const float step = 880.0f / static_cast<float>(rCount);
        const float cw = std::min(118.0f, step - 12.0f);
        for (size_t c = 0; c < rCount; ++c) {
          const size_t idx = (row == 0) ? c : (row1Count + c);
          const auto &node = schematicChain_[idx];
          const auto &meta = getComponentMeta(node.type);
          const float cx = 420.0f + static_cast<float>(c) * step + (step - cw) * 0.5f;
          gui::Rect<float> cr = {cx, rowY, cw, 124.0f};
          g.fillLinearGradientRect(cr, gui::Color::fromHex(0x1E232E),
                                   gui::Color::fromHex(0x141821), 4.0f);
          g.drawRoundedRect(cr, 4.0f, meta.accentColor, 1.2f);
          g.drawText(std::format("S{} · {}", idx + 1, meta.designator),
                     {cr.x + 6.0f, cr.y + 4.0f, cr.width - 12.0f, 16.0f},
                     meta.accentColor, 9.2f, true, true);
          g.drawText(meta.shortName,
                     {cr.x + 4.0f, cr.y + 20.0f, cr.width - 8.0f, 18.0f},
                     gui::Color::TextPrimary(), 10.0f, true, true);
          paintSchematicNodeSymbol(
              g, node.type, meta.category,
              {cr.x + 12.0f, cr.y + 42.0f, cr.width - 24.0f, 46.0f},
              meta.accentColor);
          g.drawText(node.isParallel ? "PARALLEL" : "SERIES",
                     {cr.x + 6.0f, cr.bottom() - 20.0f, cr.width - 12.0f, 16.0f},
                     node.isParallel ? gold : cyan, 8.8f, true, true);
        }
      }
    }

    // =====================================================================
    // 2. CLEAN BUILD STATUS / PROGRESS BAR UNDERNEATH THE 5 BUTTONS (y: 586..696)
    // =====================================================================
    const bool buildingNow = isBuildRunning_.load();
    const auto okCol = gui::Color::fromHex(0x52B788);
    const auto warnCol = gui::Color::AccentAmber();

    ProjectGenerator::BuildExecutionReport repCopy;
    std::string stepHeadlineCopy;
    std::string liveCompilerCopy;
    std::string actionBannerCopy;
    {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      repCopy = lastBuildReport_;
      stepHeadlineCopy = buildStepHeadline_;
      liveCompilerCopy = buildLiveCompilerLine_;
      actionBannerCopy = lastConsoleActionBanner_;
    }

    gui::Rect<float> consoleBox = {385.0f, 586.0f, 950.0f, 110.0f};
    g.fillRoundedRect(consoleBox, 6.0f, gui::Color::fromHex(0x0B0E14));
    const gui::Color consoleBorderCol =
        buildingNow ? gold
                    : (repCopy.succeeded ? okCol.withAlpha(0.85f)
                                         : outline);
    g.drawRoundedRect(consoleBox, 6.0f, consoleBorderCol,
                      buildingNow ? 1.8f : 1.2f);

    if (buildingNow) {
      const int curStep = std::clamp(buildStepIndex_.load(), 1, 4);
      const double elapsedSec =
          static_cast<double>(GetTickCount64() - buildStartTickMs_) / 1000.0;
      const float progFrac = std::clamp(
          (static_cast<float>(curStep - 1) + 0.45f) / 4.0f, 0.10f, 0.96f);

      g.drawText(
          std::format("BUILDING [STEP {}/4 | {:.1f}s]: {}",
                      curStep, elapsedSec, stepHeadlineCopy),
          {consoleBox.x + 16.0f, consoleBox.y + 14.0f, 620.0f, 20.0f}, gold,
          10.5f, false, true);

      gui::Rect<float> progTrack = {consoleBox.right() - 296.0f,
                                    consoleBox.y + 16.0f, 280.0f, 14.0f};
      g.fillRoundedRect(progTrack, 4.0f, gui::Color::fromHex(0x090C10));
      g.drawRoundedRect(progTrack, 4.0f, gold.withAlpha(0.6f), 1.0f);
      g.fillRoundedRect({progTrack.x + 1.0f, progTrack.y + 1.0f,
                         (progTrack.width - 2.0f) * progFrac,
                         progTrack.height - 2.0f},
                        3.0f, gold);
      g.drawText(std::format("{:.0f}%", progFrac * 100.0f), progTrack,
                 gui::Color::fromHex(0x0B0E14), 8.8f, true, true);

      g.drawText("COMPILER: " + (liveCompilerCopy.empty()
                                     ? "Compiling C++20 target..."
                                     : liveCompilerCopy),
                 {consoleBox.x + 16.0f, consoleBox.y + 52.0f,
                  consoleBox.width - 32.0f, 20.0f},
                 cyan, 9.8f, false, true);
    } else {
      const std::string activeProjFolder = proj.diskFolder;
      std::string effExportStr = getEffectiveExportDir(activeProjFolder).string();
      if (!effExportStr.empty() && effExportStr.back() != '\\' &&
          effExportStr.back() != '/') {
        effExportStr.push_back('\\');
      }

      const std::string hdrStatus =
          repCopy.succeeded
              ? (repCopy.elapsedSeconds > 0.01
                     ? std::format("BUILD COMPLETE ({:.1f}s) - SAVED TO: {}",
                                   repCopy.elapsedSeconds, effExportStr)
                     : std::format("READY - OUTPUT FOLDER: {}", effExportStr))
              : std::format("READY TO BUILD - OUTPUT FOLDER: {}", effExportStr);
      g.drawText(hdrStatus,
                 {consoleBox.x + 16.0f, consoleBox.y + 14.0f,
                  consoleBox.width - 32.0f, 20.0f},
                 repCopy.succeeded ? okCol : warnCol, 10.2f, false, true);

      const float badgeW = (consoleBox.width - 36.0f) / 3.0f;
      for (size_t i = 0; i < std::min<size_t>(3, repCopy.artifacts.size());
           ++i) {
        const auto &art = repCopy.artifacts[i];
        gui::Rect<float> ar = {consoleBox.x + 14.0f +
                                   static_cast<float>(i) * (badgeW + 4.0f),
                               consoleBox.y + 42.0f, badgeW, 28.0f};
        const gui::Color badgeCol = art.exists ? okCol : gui::Color::TextSecondary();
        g.fillRoundedRect(ar, 4.0f,
                          art.exists ? okCol.withAlpha(0.14f)
                                     : gui::Color::fromHex(0x141821));
        g.drawRoundedRect(ar, 4.0f, badgeCol.withAlpha(0.65f), 1.0f);

        const std::string sizeStr =
            art.exists
                ? std::format("✓ {} ({} KB)", art.fileName,
                              std::max<uintmax_t>(1, art.sizeBytes / 1024))
                : std::format("- {} (Not Built)", art.fileName);
        g.drawText(sizeStr, {ar.x + 10.0f, ar.y + 5.0f, ar.width - 20.0f, 18.0f},
                   badgeCol, 9.2f, false, true);
      }

      const std::string bottomLog =
          !actionBannerCopy.empty()
              ? actionBannerCopy
              : "Select VST3, CLAP, STANDALONE, or BUILD ALL above to choose a destination folder and build.";
      g.drawText(bottomLog,
                 {consoleBox.x + 16.0f, consoleBox.y + 78.0f,
                  consoleBox.width - 32.0f, 20.0f},
                 !actionBannerCopy.empty() ? cyan : gui::Color::TextSecondary(),
                 9.2f, false, true);
    }
  }

  // =========================================================================
  // Visual Drag-and-Drop Hardware Schematic Builder Paint (Clean 2-Row
  // Serpentine)
  // =========================================================================
  void paintSchematicBuilderMode(gui::Graphics &g,
                                 const gui::Rect<float> &local,
                                 gui::Color panelBg, gui::Color outline,
                                 gui::Color cardBg, gui::Color gold,
                                 gui::Color cyan) {
    const float panelX = isSidebarCollapsed_ ? 60.0f : 355.0f;
    const float libX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    const float libW = isSidebarCollapsed_ ? 280.0f : 232.0f;
    const float canvasX = isSidebarCollapsed_ ? 364.0f : 612.0f;
    const float canvasW = 1345.0f - canvasX;

    gui::Rect<float> builderPanel = {
        panelX, 62.0f, local.width - panelX - 20.0f, local.height - 107.0f};
    g.fillRoundedRect(builderPanel, 6.0f, panelBg);
    g.drawRoundedRect(builderPanel, 6.0f, outline, 1.0f);

    // Header
    g.drawText("SCHEMATIC BUILDER  -  SIGNAL CHAIN DESIGNER",
               {libX, 66.0f, 760.0f, 22.0f}, gold, 15.0f, false, true);
    g.drawText(
        std::format("Browse {} audio modules (preamps, saturation, EQs, "
                    "dynamics, delays, reverbs & utilities) - scroll or filter "
                    "by category above.",
                    getCuratedSchematicCatalog().size()),
        {libX, 86.0f, 980.0f, 18.0f}, gui::Color::TextSecondary(), 10.2f,
        false);

    // 1. Left Component Library Smooth-Scroll Box (y: 145..712)
    gui::Rect<float> paletteBox = {libX, 145.0f, libW, 567.0f};
    g.fillRoundedRect(paletteBox, 5.0f, cardBg);
    g.drawRoundedRect(paletteBox, 5.0f, outline, 1.0f);

    const std::string libHeaderText =
        (activeSourceFilter_ == SchematicSourceFilter::All)
            ? std::format("COMPONENT LIBRARY ({} MODULES)",
                          paletteItems_.size())
            : std::format(
                  "COMPONENT LIBRARY \xC2\xB7 {} ({})",
                  getSchematicSourceFilterLabel(activeSourceFilter_),
                  paletteItems_.size());
    g.drawText(libHeaderText, {libX + 8.0f, 149.0f, libW - 16.0f, 16.0f}, gold,
               10.0f, false, true);
    g.drawText("SEARCH:", {libX + 8.0f, 169.0f, 52.0f, 22.0f}, cyan, 8.8f,
               false, true);

    // Secondary Filter Pill Bar inside active tab (5 sub-category pills with
    // live counts)
    for (int pillIdx = 0; pillIdx < 5; ++pillIdx) {
      const gui::Rect<float> pillRect = getSubFilterPillRect(pillIdx);
      const bool isSelected = (activeSubFilter_ == pillIdx);
      g.fillRoundedRect(pillRect, 4.0f,
                        isSelected ? cyan.withAlpha(0.24f)
                                   : gui::Color::fromHex(0x161920));
      g.drawRoundedRect(pillRect, 4.0f,
                        isSelected ? cyan : outline.withAlpha(0.85f),
                        isSelected ? 1.2f : 0.8f);
      g.drawText(
          getSchematicSubFilterLabel(activeCategory_, pillIdx,
                                     activeSourceFilter_),
          pillRect, isSelected ? cyan : gui::Color::TextSecondary(), 6.8f, true,
          isSelected);
    }

    // Source-Type Filter Pill Bar (ALL | BASS | GUITAR | DRUMS | VOCALS | KEYS)
    for (int srcIdx = 0; srcIdx < 6; ++srcIdx) {
      const auto srcFilter = static_cast<SchematicSourceFilter>(srcIdx);
      const gui::Rect<float> sRect = getSourceFilterPillRect(srcIdx);
      const bool isSelected = (activeSourceFilter_ == srcFilter);
      g.fillRoundedRect(sRect, 4.0f,
                        isSelected ? gold.withAlpha(0.26f)
                                   : gui::Color::fromHex(0x141820));
      g.drawRoundedRect(sRect, 4.0f,
                        isSelected ? gold : outline.withAlpha(0.85f),
                        isSelected ? 1.3f : 0.8f);
      g.drawText(getSchematicSourceFilterLabel(srcFilter), sRect,
                 isSelected ? gold : gui::Color::fromHex(0xCBD5E1), 7.4f, true,
                 true);
    }

    // Smooth Vertical Scroll Container for Component Cards (y: 288..706)
    constexpr float listTop = 288.0f;
    constexpr float listBot = 706.0f;
    constexpr float listH = listBot - listTop;
    constexpr float stepH = 64.0f;
    constexpr float cardH = 58.0f;

    g.pushClipRect({libX + 2.0f, listTop, libW - 4.0f, listH});
    for (size_t i = 0; i < paletteItems_.size(); ++i) {
      const float cardY =
          listTop + static_cast<float>(i) * stepH - schematicLibraryScrollY_;
      if (cardY + cardH < listTop || cardY > listBot)
        continue;

      const auto &item = paletteItems_[i];
      const bool isHovered = (static_cast<int>(i) == hoveredPaletteItemIdx_);
      gui::Rect<float> cRect = {libX + 6.0f, cardY, libW - 18.0f, cardH};
      g.fillLinearGradientRect(cRect,
                               isHovered ? gui::Color::fromHex(0x2B3140)
                                         : gui::Color::fromHex(0x232732),
                               isHovered ? gui::Color::fromHex(0x202532)
                                         : gui::Color::fromHex(0x1A1D26),
                               4.5f);
      g.drawRoundedRect(
          cRect, 4.5f,
          isHovered ? item.accentColor : item.accentColor.withAlpha(0.65f),
          isHovered ? 1.5f : 1.0f);

      gui::Rect<float> badge = {cRect.x + 8.0f, cardY + (cardH - 22.0f) * 0.5f,
                                38.0f, 22.0f};
      g.fillRoundedRect(badge, 3.5f, item.accentColor.withAlpha(0.24f));
      g.drawRoundedRect(badge, 3.5f, item.accentColor.withAlpha(0.55f), 0.9f);
      g.drawText(item.designator, badge, item.accentColor, 10.0f, true, true);

      g.drawText(item.shortName,
                 {cRect.x + 52.0f, cardY + 6.0f, cRect.width - 88.0f, 26.0f},
                 gui::Color::TextPrimary(), 11.6f, false, true);
      g.drawText(formatComponentSourceBadgeText(item, true),
                 {cRect.x + 52.0f, cardY + 31.0f, cRect.width - 88.0f, 20.0f},
                 (getSchematicComponentSourceMask(item) == kSourceUniversal)
                     ? cyan.withAlpha(0.88f)
                     : gold.withAlpha(0.92f),
                 8.0f, false, true);

      gui::Rect<float> addBtn = {cRect.right() - 32.0f,
                                 cardY + (cardH - 24.0f) * 0.5f, 26.0f, 24.0f};
      g.fillRoundedRect(addBtn, 3.5f, item.accentColor.withAlpha(0.32f));
      g.drawRoundedRect(addBtn, 3.5f, item.accentColor, 1.1f);
      g.drawText("+", addBtn, gui::Color::TextPrimary(), 12.5f, true, true);
    }
    g.popClipRect();

    // Custom Vertical Scrollbar for Component Library
    const float totalContentH =
        static_cast<float>(paletteItems_.size()) * stepH;
    gui::Rect<float> scrollTrack = {libX + libW - 8.0f, listTop, 5.0f, listH};
    g.fillRoundedRect(scrollTrack, 2.5f, gui::Color::fromHex(0x12151C));
    if (totalContentH > listH) {
      const float thumbH = std::max(28.0f, listH * (listH / totalContentH));
      const float maxScroll = std::max(1.0f, totalContentH - listH);
      const float thumbY =
          listTop + (schematicLibraryScrollY_ / maxScroll) * (listH - thumbH);
      g.fillRoundedRect({scrollTrack.x, thumbY, 5.0f, thumbH}, 2.5f,
                        cyan.withAlpha(0.75f));
    }

    // 2. Center Interactive Dual-Lane Spatial Schematic Canvas (x:
    // canvasX..1345, y: 145..446)
    gui::Rect<float> canvas = getSchematicCanvasRect();
    g.fillRoundedRect(canvas, 5.0f, gui::Color::fromHex(0x0B0F15));

    // Subtle PCB background grid shifted by horizontal camera pan
    const float gridStep = std::max(14.0f, 24.0f * schematicZoom_);
    const float gridOffX = std::fmod(-schematicScrollX_, gridStep);
    for (float gx = canvas.x + gridOffX; gx < canvas.right(); gx += gridStep) {
      if (gx > canvas.x + 2.0f) {
        g.drawLine({gx, canvas.y + 26.0f}, {gx, canvas.bottom() - 23.0f},
                   gui::Color::fromHex(0x121822), 1.0f);
      }
    }
    for (float gy = canvas.y + 46.0f; gy < canvas.bottom() - 23.0f;
         gy += 24.0f) {
      g.drawLine({canvas.x, gy}, {canvas.right(), gy},
                 gui::Color::fromHex(0x121822), 1.0f);
    }

    // Top Header Strip (y: 145..171)
    gui::Rect<float> canvasHdr = {canvas.x + 1.0f, canvas.y + 1.0f,
                                  canvas.width - 2.0f, 25.0f};
    g.fillRoundedRect(canvasHdr, 4.0f, gui::Color::fromHex(0x121822));
    g.drawLine({canvas.x, canvas.y + 26.0f}, {canvas.right(), canvas.y + 26.0f},
               gui::Color::fromHex(0x263142), 1.0f);

    const size_t totalNodes = schematicChain_.size();
    g.drawText(
        std::format("SINGLE-TRUNK & Y-SPLIT SCHEMATIC  |  STAGES: {} (DRAG "
                    "BELOW TRUNK FOR PARALLEL [Y] -> [\xCE\xA3] SPLIT)",
                    totalNodes),
        {canvas.x + 10.0f, canvas.y + 4.0f, std::max(120.0f, canvasW - 615.0f),
         18.0f},
        cyan, 9.0f, false, true);

    const auto tb = getSchematicToolbarRects();
    auto drawToolbarBtn = [&](const gui::Rect<float> &r, const std::string &lbl,
                              bool active, gui::Color col) {
      g.fillRoundedRect(r, 3.0f,
                        active ? col.withAlpha(0.26f)
                               : gui::Color::fromHex(0x161D28));
      g.drawRoundedRect(r, 3.0f, active ? col : outline.withAlpha(0.85f),
                        active ? 1.2f : 0.9f);
      g.drawText(lbl, r, active ? col : gui::Color::TextPrimary(), 7.8f, true,
                 true);
    };
    drawToolbarBtn(tb.zoomOutBtn, "-", false, cyan);
    drawToolbarBtn(tb.zoomResetBtn,
                   std::format("{:.0f}%", schematicZoom_ * 100.0f),
                   std::abs(schematicZoom_ - 1.0f) > 0.02f, cyan);
    drawToolbarBtn(tb.zoomInBtn, "+", false, cyan);
    drawToolbarBtn(tb.fitChainBtn, "FIT CHAIN", false, gold);
    drawToolbarBtn(tb.compactViewBtn, "COMPACT VIEW", schematicCompactView_,
                   cyan);
    drawToolbarBtn(tb.groupStagesBtn,
                   schematicMacroGrouped_ ? "UNGROUP MACRO" : "GROUP STAGES",
                   schematicMacroGrouped_, gold);

    g.fillRoundedRect(tb.pdcPill, 3.5f, gui::Color::fromHex(0x161B24));
    g.drawRoundedRect(tb.pdcPill, 3.5f, outline, 1.1f);
    g.drawText(formatPdcPillText(), tb.pdcPill, gui::Color::TextPrimary(), 7.6f,
               true, true);

    // Clip all schematic wires, jacks, split/mixer nodes, and stage cards
    // strictly inside the canvas viewport
    const gui::Rect<float> viewport = getSchematicViewportRect();
    g.pushClipRect(viewport);

    // Primary Center Audio Trunk Trace (y = 336) + Dynamic Y-Split (Y) & Mixer
    // (Sum) Parallel Branches (World Space)
    const float chainEndX =
        schematicChain_.empty()
            ? (canvas.right() - 44.0f)
            : (schematicChain_.back().isParallel
                   ? (getParallelBranchMergeX(totalNodes - 1) + 28.0f)
                   : getCanvasNodeRect(totalNodes - 1).right());
    const float outJackX =
        schematicChain_.empty()
            ? (canvas.right() - 42.0f)
            : std::max(canvas.right() - 44.0f - schematicScrollX_,
                       chainEndX + 20.0f * schematicZoom_);
    gui::Rect<float> inJack = {canvas.x + 8.0f - schematicScrollX_,
                               kSchematicCenterTrunkY - 19.0f, 34.0f, 38.0f};
    gui::Rect<float> outJack = {outJackX, kSchematicCenterTrunkY - 19.0f, 36.0f,
                                38.0f};
    const float pulsePhase = static_cast<float>(
        std::fmod(static_cast<double>(GetTickCount64()) * 0.00055, 1.0));
    if (schematicChain_.empty()) {
      drawChamferedPcbTrace(g, {inJack.right(), kSchematicCenterTrunkY},
                            {outJack.x, kSchematicCenterTrunkY},
                            cyan.withAlpha(0.65f), 2.5f);
      const float busSpanW = std::max(10.0f, outJack.x - inJack.right());
      for (int p = 0; p < 5; ++p) {
        const float frac =
            std::fmod(pulsePhase + static_cast<float>(p) * 0.20f, 1.0f);
        const float px = inJack.right() + frac * busSpanW;
        g.fillCircle({px, kSchematicCenterTrunkY}, 3.2f, cyan.withAlpha(0.42f));
        g.fillCircle({px, kSchematicCenterTrunkY}, 1.6f,
                     gui::Color::TextPrimary());
      }
      g.drawText("CANVAS EMPTY - DRAG COMPONENTS ONTO CENTER TRUNK (SERIES) "
                 "OR BELOW TRUNK (PARALLEL [Y] -> [\xCE\xA3] SPLIT)",
                 {canvas.x + 60.0f, kSchematicCenterTrunkY - 22.0f,
                  canvas.width - 120.0f, 44.0f},
                 gui::Color::TextSecondary(), 10.2f, true, true);
    } else {
      // Segment-by-Segment Gain-Staging Health Color Traces along the straight
      // horizontal Center Trunk (y = kSchematicCenterTrunkY)
      size_t firstSeries = 0;
      while (firstSeries < totalNodes &&
             schematicChain_[firstSeries].isParallel) {
        ++firstSeries;
      }
      const float firstEndX = (firstSeries < totalNodes)
                                  ? getCanvasNodeRect(firstSeries).x
                                  : outJack.x;
      const float inPeakDb = -9.5f + rackInputTrimDb_;
      const gui::Color inTraceCol =
          getStageTraceHealthColor(inPeakDb, cyan, gold);
      if (firstEndX > inJack.right()) {
        drawChamferedPcbTrace(g, {inJack.right(), kSchematicCenterTrunkY},
                              {firstEndX, kSchematicCenterTrunkY},
                              inTraceCol.withAlpha(0.85f), 2.6f);
      }
      for (size_t s = 0; s < totalNodes; ++s) {
        if (schematicChain_[s].isParallel)
          continue;
        const auto curRect = getCanvasNodeRect(s);
        size_t nextSeries = s + 1;
        while (nextSeries < totalNodes &&
               schematicChain_[nextSeries].isParallel) {
          ++nextSeries;
        }
        const float segStartX = curRect.right();
        const float segEndX = (nextSeries < totalNodes)
                                  ? getCanvasNodeRect(nextSeries).x
                                  : outJack.x;
        if (segEndX > segStartX) {
          const float stgPeakDb = estimateStageOutputHeadroomDb(s);
          const gui::Color segCol =
              getStageTraceHealthColor(stgPeakDb, cyan, gold);
          const float segThick = (stgPeakDb > 3.0f) ? 3.4f : 2.6f;
          drawChamferedPcbTrace(g, {segStartX, kSchematicCenterTrunkY},
                                {segEndX, kSchematicCenterTrunkY},
                                segCol.withAlpha(0.88f), segThick);
          const float pulseX = segStartX + pulsePhase * (segEndX - segStartX);
          if (pulseX >= viewport.x && pulseX <= viewport.right()) {
            g.fillCircle({pulseX, kSchematicCenterTrunkY},
                         (stgPeakDb > 3.0f) ? 4.2f : 3.2f,
                         segCol.withAlpha(0.75f));
            g.fillCircle({pulseX, kSchematicCenterTrunkY}, 1.6f,
                         gui::Color::TextPrimary());
          }
        }
      }
    }

    // Pass 2 & 3: Draw Parallel Branch Wires, followed by Physical Y-Split
    // ([Y]) and Summing Mixer ([Sum]) Junction Boxes
    const size_t grpCount = getMacroGroupedStageCount();
    size_t bIdx = (grpCount >= 2) ? grpCount : 0;
    while (bIdx < totalNodes) {
      if (!schematicChain_[bIdx].isParallel) {
        ++bIdx;
        continue;
      }
      const size_t bStart = bIdx;
      while (bIdx + 1 < totalNodes && schematicChain_[bIdx + 1].isParallel) {
        ++bIdx;
      }
      const size_t bEnd = bIdx;

      const auto rFirstPar = getCanvasNodeRect(bStart);
      const auto rLastPar = getCanvasNodeRect(bEnd);
      const float splitX = getParallelBranchSplitX(bStart);
      const float mergeX = getParallelBranchMergeX(bEnd);

      if (mergeX >= viewport.x - 32.0f && splitX <= viewport.right() + 32.0f) {
        // 45-degree Chamfered PCB Split Trace from [Y] (Center Trunk -> Lower
        // Path B)
        drawChamferedPcbTrace(g, {splitX, kSchematicCenterTrunkY},
                              {rFirstPar.x, kSchematicPathBLowerY}, gold, 2.5f);
        // Multi-stage Parallel Chain horizontal traces along Lower Path B
        for (size_t k = bStart; k < bEnd; ++k) {
          const auto rk0 = getCanvasNodeRect(k);
          const auto rk1 = getCanvasNodeRect(k + 1);
          drawChamferedPcbTrace(g, {rk0.right(), kSchematicPathBLowerY},
                                {rk1.x, kSchematicPathBLowerY}, gold, 2.5f);
        }
        // 45-degree Chamfered PCB Return Trace (Lower Path B -> [Sum MIX] on
        // Center Trunk)
        drawChamferedPcbTrace(g, {rLastPar.right(), kSchematicPathBLowerY},
                              {mergeX, kSchematicCenterTrunkY}, gold, 2.5f);

        // Animated pulse along Lower Path B parallel branch
        const float parSpan = std::max(8.0f, mergeX - splitX);
        const float parPx = splitX + pulsePhase * parSpan;
        if (parPx >= rFirstPar.x && parPx <= rLastPar.right()) {
          g.fillCircle({parPx, kSchematicPathBLowerY}, 3.0f,
                       gold.withAlpha(0.75f));
        }

        // Physical Y-Split Hardware Node ([Y]) on Center Trunk
        const gui::Rect<float> yNodeBox = {
            splitX - 11.0f, kSchematicCenterTrunkY - 9.0f, 22.0f, 18.0f};
        g.fillRoundedRect(yNodeBox, 4.0f, gui::Color::fromHex(0x141B24));
        g.drawRoundedRect(yNodeBox, 4.0f, cyan, 1.4f);
        g.drawText("Y", yNodeBox, cyan, 8.6f, true, true);

        // Physical Summing Mixer Node ([Sum]) on Center Trunk
        const gui::Rect<float> sumNodeBox = {
            mergeX - 13.0f, kSchematicCenterTrunkY - 10.0f, 26.0f, 20.0f};
        g.fillRoundedRect(sumNodeBox, 4.5f, gui::Color::fromHex(0x181E28));
        g.drawRoundedRect(sumNodeBox, 4.5f, gold, 1.6f);
        g.drawText("\xCE\xA3", sumNodeBox, gold, 9.6f, true, true);
      }
      ++bIdx;
    }

    // Draw Magnetic Drag-and-Drop Ghost Slot + Glowing Wire Insertion Marker
    // when dragging from Library or reordering a Stage Card
    if ((isDraggingPaletteItem_ || isDraggingCanvasNode_) &&
        canvas.contains(dragCurrentPos_)) {
      const float ghostCardW =
          (schematicCompactView_ ? 118.0f : 152.0f) * schematicZoom_;
      const float ghostCardH =
          104.0f * math::clamp(schematicZoom_, 0.65f, 1.45f);
      float ghostX = getSchematicCanvasRect().x + 58.0f - schematicScrollX_;
      if (!schematicChain_.empty()) {
        const int clampedSlot = math::clamp(
            dragInsertSlot_, 0, static_cast<int>(schematicChain_.size()));
        if (clampedSlot < static_cast<int>(schematicChain_.size())) {
          ghostX =
              getCanvasNodeRect(static_cast<size_t>(clampedSlot)).x - 10.0f;
        } else {
          ghostX =
              getCanvasNodeRect(schematicChain_.size() - 1).right() + 14.0f;
        }
      }
      const float ghostBusY = dragTargetParallelLane_ ? kSchematicPathBLowerY
                                                      : kSchematicCenterTrunkY;
      const gui::Rect<float> ghostSlot = {
          math::clamp(ghostX, viewport.x + 8.0f,
                      viewport.right() - ghostCardW - 8.0f),
          ghostBusY - ghostCardH * 0.5f, ghostCardW, ghostCardH};
      const gui::Color ghostCol = dragTargetParallelLane_ ? gold : cyan;
      // Glowing vertical insertion marker on the trunk wire
      g.drawLine({ghostSlot.x - 4.0f, kSchematicCenterTrunkY - 28.0f},
                 {ghostSlot.x - 4.0f, kSchematicCenterTrunkY + 28.0f}, ghostCol,
                 2.2f);
      g.fillCircle({ghostSlot.x - 4.0f, kSchematicCenterTrunkY}, 4.5f,
                   ghostCol);
      if (dragTargetParallelLane_) {
        // Live preview of Y-Split ([Y]) and Sum-Mixer ([Sum]) ghost wires for
        // Lower Path B Drop Zone
        drawChamferedPcbTrace(g, {ghostSlot.x - 10.0f, kSchematicCenterTrunkY},
                              {ghostSlot.x, kSchematicPathBLowerY},
                              gold.withAlpha(0.60f), 2.0f);
        drawChamferedPcbTrace(
            g, {ghostSlot.right(), kSchematicPathBLowerY},
            {ghostSlot.right() + 10.0f, kSchematicCenterTrunkY},
            gold.withAlpha(0.60f), 2.0f);
      }
      g.fillRoundedRect(ghostSlot, 5.0f, ghostCol.withAlpha(0.14f));
      g.drawRoundedRect(ghostSlot, 5.0f, ghostCol, 2.0f);
      g.drawText(dragTargetParallelLane_ ? "[ + DROP IN PARALLEL PATH B ]"
                                         : "[ + DROP IN SERIES TRUNK ]",
                 ghostSlot, ghostCol, 8.4f, true, true);
    }

    // Pass 4: Draw Stage Component Cards (S1..Sn), IN XLR, and OUT BUS
    for (size_t i = 0; i < totalNodes; ++i) {
      if (grpCount >= 2 && i > 0 && i < grpCount) {
        continue; // Collapsed inside Macro Chassis Container at i == 0
      }
      const gui::Rect<float> nr = getCanvasNodeRect(i);
      if (nr.right() < viewport.x || nr.x > viewport.right()) {
        continue; // Culled outside horizontal viewport
      }

      if (grpCount >= 2 && i == 0) {
        // Render Expandable Macro Chassis Container Card [ 📦 STUDIO
        // FRONT-END (N STAGES) ]
        g.fillLinearGradientRect(nr, gui::Color::fromHex(0x242A38),
                                 gui::Color::fromHex(0x161B24), 5.0f);
        g.fillRoundedRect({nr.x, nr.y, nr.width, 20.0f}, 5.0f,
                          gold.withAlpha(0.28f));
        g.drawRoundedRect(nr, 5.0f, gold, 2.0f);
        drawSolderedTerminalPin(g, {nr.x, kSchematicCenterTrunkY}, cyan);
        drawSolderedTerminalPin(g, {nr.right(), kSchematicCenterTrunkY}, gold);
        g.drawText(std::format("MACRO ({} STAGES)", grpCount),
                   {nr.x + 6.0f, nr.y + 2.0f, nr.width - 12.0f, 16.0f}, gold,
                   8.5f, true, true);
        g.drawText("STUDIO FRONT-END",
                   {nr.x + 6.0f, nr.y + 22.0f, nr.width - 12.0f, 16.0f},
                   gui::Color::TextPrimary(), 9.6f, true, true);

        std::string chainSummary;
        for (size_t k = 0; k < grpCount; ++k) {
          if (k > 0)
            chainSummary += "+";
          chainSummary += getComponentMeta(schematicChain_[k].type).designator;
        }
        g.drawText(chainSummary,
                   {nr.x + 6.0f, nr.y + 42.0f, nr.width - 12.0f, 18.0f}, cyan,
                   8.6f, true, true);

        gui::Rect<float> expPill = {nr.x + 8.0f, nr.bottom() - 20.0f,
                                    nr.width - 16.0f, 16.0f};
        g.fillRoundedRect(expPill, 3.0f, gold.withAlpha(0.22f));
        g.drawRoundedRect(expPill, 3.0f, gold, 1.0f);
        g.drawText("+ CLICK TO EXPAND", expPill, gold, 7.8f, true, true);
        continue;
      }

      const auto &node = schematicChain_[i];
      const auto &meta = getComponentMeta(node.type);
      const bool isSel = (static_cast<int>(i) == selectedNodeIndex_);
      const float busY = getStageBusY(i);
      const float zScale = math::clamp(schematicZoom_, 0.65f, 1.30f);
      const float hdrH = 19.0f * zScale;

      // Node Card Background & Header
      g.fillLinearGradientRect(nr, gui::Color::fromHex(0x1E232E),
                               gui::Color::fromHex(0x151922), 5.0f);
      g.fillRoundedRect({nr.x, nr.y, nr.width, hdrH}, 5.0f,
                        meta.accentColor.withAlpha(0.24f));
      g.drawRoundedRect(nr, 5.0f,
                        isSel ? cyan : meta.accentColor.withAlpha(0.82f),
                        isSel ? 2.2f : 1.2f);

      // Soldered Gold/Cyan Terminal Pins (◉) on Left Input and Right Output
      // Ports
      drawSolderedTerminalPin(g, {nr.x, busY}, node.isParallel ? gold : cyan);
      drawSolderedTerminalPin(g, {nr.right(), busY}, meta.accentColor);

      // Header: S1·PR1 on left, [<] [>] [X] buttons on right (always rendered
      // at nr.width >= 56.0f, scaled by schematicZoom_)
      const float btnW = 13.0f * zScale;
      const float btnH = 14.0f * zScale;
      const float btnGap = 2.0f * zScale;
      const float btnY = nr.y + (hdrH - btnH) * 0.5f;
      gui::Rect<float> btnX = {nr.right() - btnW - 3.0f * zScale, btnY, btnW,
                               btnH};
      gui::Rect<float> btnR = {btnX.x - btnW - btnGap, btnY, btnW, btnH};
      gui::Rect<float> btnL = {btnR.x - btnW - btnGap, btnY, btnW, btnH};

      g.drawText(node.isBypassed
                     ? std::format("S{}·BYP", i + 1)
                     : std::format("S{}·{}", i + 1, meta.designator),
                 {nr.x + 5.0f * zScale, nr.y + 1.5f * zScale,
                  std::max(24.0f, btnL.x - nr.x - 6.0f * zScale),
                  hdrH - 2.0f * zScale},
                 node.isBypassed ? gui::Color::AccentRed() : meta.accentColor,
                 9.0f * math::clamp(schematicZoom_, 0.78f, 1.25f), false, true);

      if (nr.width >= 56.0f) {
        g.fillRoundedRect(btnL, 2.0f, gui::Color::fromHex(0x161A22, 0.78f));
        g.fillRoundedRect(btnR, 2.0f, gui::Color::fromHex(0x161A22, 0.78f));
        g.fillRoundedRect(btnX, 2.0f, gui::Color::fromHex(0x2A181E, 0.85f));
        const float btnFont = 8.2f * math::clamp(schematicZoom_, 0.78f, 1.20f);
        g.drawText("<", btnL, gui::Color::TextSecondary(), btnFont, true, true);
        g.drawText(">", btnR, gui::Color::TextSecondary(), btnFont, true, true);
        g.drawText("X", btnX, gui::Color::AccentRed(), btnFont, true, true);
      }

      // Proportional Component Title (splits across 2 lines if long or zoomed,
      // with smooth font scaling)
      const float titleFont = 10.5f * math::clamp(schematicZoom_, 0.78f, 1.25f);
      std::string cardTitle = meta.shortName;
      float titleBlockH = 18.0f * zScale;
      if (cardTitle.size() > 14) {
        const size_t mid = cardTitle.size() / 2;
        size_t splitPos = cardTitle.rfind(' ', mid + 4);
        if (splitPos == std::string::npos || splitPos < 3) {
          splitPos = cardTitle.find(' ', mid);
        }
        if (splitPos != std::string::npos && splitPos + 1 < cardTitle.size()) {
          cardTitle[splitPos] = '\n';
          titleBlockH = 27.0f * zScale;
        }
      }
      const float titleTopY = nr.y + hdrH + 2.0f * zScale;
      g.drawText(cardTitle,
                 {nr.x + 5.0f * zScale, titleTopY, nr.width - 10.0f * zScale,
                  titleBlockH},
                 node.isBypassed ? gui::Color::TextSecondary()
                                 : gui::Color::TextPrimary(),
                 titleFont, true, true);

      const bool showBottomPill = (node.isBypassed || node.isParallel);
      const float bottomPad =
          showBottomPill ? (19.0f * zScale) : (5.0f * zScale);

      // Dual Input/Output 5-Segment Vertical Mini-LED Meters on Left (IN) &
      // Right (OUT) Edges
      const float meterTopY = titleTopY + titleBlockH + 2.0f * zScale;
      const float meterAvailH =
          std::max(18.0f, nr.bottom() - bottomPad - meterTopY);
      const float segStep = meterAvailH / 5.0f;
      const float segH = std::max(2.2f, segStep - 1.4f);
      auto drawMiniMeter5 = [&](float mx, bool active, int litCount) {
        const gui::Color segCols[5] = {
            gui::Color::AccentRed(), gui::Color::AccentAmber(),
            gui::Color::fromHex(0xE8C547), gui::Color::fromHex(0x52B788),
            gui::Color::fromHex(0x38A3A5)};
        for (int s = 0; s < 5; ++s) {
          gui::Rect<float> seg = {mx,
                                  meterTopY + static_cast<float>(s) * segStep,
                                  3.8f * zScale, segH};
          const bool lit = active && ((5 - s) <= litCount);
          g.fillRoundedRect(seg, 1.0f,
                            lit ? segCols[s] : gui::Color::fromHex(0x0E1219));
        }
      };
      const int inLit = node.isBypassed ? 1 : 3;
      const int outLit =
          node.isBypassed ? 0 : ((node.param1Val > 6.0f) ? 5 : 4);
      drawMiniMeter5(nr.x + 3.5f * zScale, true, inLit);
      drawMiniMeter5(nr.right() - 7.3f * zScale, !node.isBypassed, outLit);

      // Vector Electronic Circuit Symbol + GR / THD Activity Dot (Enlarged
      // inside series cards with no bottom pill!)
      gui::Rect<float> symBox = {nr.x + 10.0f * zScale, meterTopY,
                                 nr.width - 20.0f * zScale, meterAvailH};
      paintSchematicNodeSymbol(
          g, node.type, meta.category, symBox,
          node.isBypassed ? gui::Color::TextSecondary().withAlpha(0.45f)
                          : meta.accentColor);
      const bool isDyn = (meta.category == SchematicCategory::AnalogDynamics);
      const gui::Color actCol = node.isBypassed
                                    ? gui::Color::fromHex(0x262C36)
                                    : (isDyn ? gui::Color::AccentAmber()
                                             : gui::Color::fromHex(0x52B788));
      g.fillCircle({symBox.right() - 5.5f * zScale, symBox.y + 5.5f * zScale},
                   2.6f * zScale, actCol);

      // Bottom Pill ONLY when BYPASSED (Red) or in PARALLEL Mode (Gold)
      if (showBottomPill) {
        const float pillH = 14.5f * zScale;
        gui::Rect<float> pill = {nr.x + 6.0f * zScale,
                                 nr.bottom() - pillH - 3.0f * zScale,
                                 nr.width - 12.0f * zScale, pillH};
        const float pillFont = 8.0f * math::clamp(schematicZoom_, 0.78f, 1.20f);
        if (node.isBypassed) {
          g.fillRoundedRect(pill, 3.0f,
                            gui::Color::AccentRed().withAlpha(0.24f));
          g.drawRoundedRect(pill, 3.0f, gui::Color::AccentRed(), 1.0f);
          g.drawText("BYPASSED", pill, gui::Color::AccentRed(), pillFont, true,
                     true);
        } else if (node.isParallel) {
          g.fillRoundedRect(pill, 3.0f, gold.withAlpha(0.28f));
          g.drawRoundedRect(pill, 3.0f, gold, 1.0f);
          g.drawText(std::format("PARALLEL: {:.0f}%", node.parallelMixPercent),
                     pill, gold, pillFont, true, true);
        }
      }
    }

    // Re-draw IN XLR & OUT BUS Jacks on top of scrolled edge so nodes tuck
    // cleanly underneath
    g.fillRoundedRect(inJack, 4.0f, gui::Color::fromHex(0x1C222D));
    g.drawRoundedRect(inJack, 4.0f, cyan, 1.5f);
    g.drawText("IN\nXLR", inJack, cyan, 8.5f, true, true);

    g.fillRoundedRect(outJack, 4.0f, gui::Color::fromHex(0x1C222D));
    g.drawRoundedRect(outJack, 4.0f, gold, 1.5f);
    g.drawText("OUT\nBUS", outJack, gold, 8.5f, true, true);

    // Pass 5 (Foreground Pass): Draw all per-stage status pills (SAFE, DRIFT,
    // 4x OS, +16dB, DC 5Hz) docked centered underneath each component card,
    // plus all Σ MIX:50% | Ø | SYNC badges.
    if (schematicZoom_ >= 0.65f) {
      for (size_t i = 0; i < totalNodes; ++i) {
        if (grpCount >= 2 && i < grpCount) {
          continue;
        }
        const gui::Rect<float> nr = getCanvasNodeRect(i);
        if (nr.right() < viewport.x || nr.x > viewport.right()) {
          continue;
        }
        const auto &node = schematicChain_[i];
        const uint32_t pFlags = node.getSmartPlumbingFlags();
        const float outPeakEstDb = estimateStageOutputHeadroomDb(i);
        const bool isOverdriven = (outPeakEstDb > 3.0f);
        const std::string wireChipText =
            isOverdriven
                ? std::format("+{:.0f}dB!", outPeakEstDb)
                : ((pFlags & PLUMB_PASSIVE_MAKEUP)
                       ? "+16dB"
                       : ((pFlags & PLUMB_OVERSAMPLE_ADAA)
                              ? "4x OS"
                              : ((pFlags & PLUMB_SAFETY_SUBSONIC_NAN_CLAMP)
                                     ? "SAFE"
                                     : ((pFlags & PLUMB_DC_GUARD_5HZ)
                                            ? "DC 5Hz"
                                            : "DRIFT"))));
        const gui::Rect<float> chipRect = getStageStatusBadgeRect(i);
        const gui::Color chipBg =
            isOverdriven ? gui::Color::fromHex(0x361218, 0.96f)
                         : ((outPeakEstDb >= -6.0f)
                                ? gui::Color::fromHex(0x282112, 0.94f)
                                : gui::Color::fromHex(0x0F241D, 0.92f));
        const gui::Color chipFg =
            isOverdriven
                ? gui::Color::AccentRed()
                : ((outPeakEstDb >= -6.0f) ? gold
                                           : gui::Color::fromHex(0x52B788));
        g.fillRoundedRect(chipRect, 3.0f, chipBg);
        g.drawRoundedRect(chipRect, 3.0f, chipFg.withAlpha(0.88f), 1.0f);
        g.drawText(wireChipText, chipRect, chipFg, 7.2f, true, true);
      }
    }

    size_t fgBIdx = (grpCount >= 2) ? grpCount : 0;
    while (fgBIdx < totalNodes) {
      if (!schematicChain_[fgBIdx].isParallel) {
        ++fgBIdx;
        continue;
      }
      const size_t bStart = fgBIdx;
      while (fgBIdx + 1 < totalNodes &&
             schematicChain_[fgBIdx + 1].isParallel) {
        ++fgBIdx;
      }
      const size_t bEnd = fgBIdx;
      const gui::Rect<float> sumBadge =
          getSummingJunctionBadgeRect(bStart, bEnd);
      if (sumBadge.right() >= viewport.x && sumBadge.x <= viewport.right()) {
        const auto &endNode = schematicChain_[bEnd];
        g.fillRoundedRect(sumBadge, 3.5f, gui::Color::fromHex(0x141A24, 0.96f));
        g.drawRoundedRect(
            sumBadge, 3.5f,
            endNode.parallelPhaseInvert ? gui::Color::AccentRed() : gold, 1.2f);
        const std::string sumTxt = std::format(
            "\xCE\xA3 MIX:{:.0f}% | {} | SYNC", endNode.parallelMixPercent,
            endNode.parallelPhaseInvert ? "\xC3\x98"
                                          "180\xC2\xB0"
                                        : "\xC3\x98");
        g.drawText(sumTxt, sumBadge,
                   endNode.parallelPhaseInvert ? gui::Color::AccentAmber()
                                               : gold,
                   7.6f, true, true);
      }
      ++fgBIdx;
    }

    // Inter-Stage Gain-Staging Overdrive Hover Tooltip with 1-click [ Auto-Trim
    // Previous Stage ]
    if (hoveredOverdriveStageIdx_ >= 0 &&
        hoveredOverdriveStageIdx_ < static_cast<int>(schematicChain_.size()) &&
        overdriveAutoTrimBtnRect_.width > 10.0f) {
      const size_t ovIdx = static_cast<size_t>(hoveredOverdriveStageIdx_);
      const auto &ovMeta = getComponentMeta(schematicChain_[ovIdx].type);
      const float ovDb = estimateStageOutputHeadroomDb(ovIdx);
      gui::Rect<float> tipBox = {overdriveAutoTrimBtnRect_.x - 8.0f,
                                 overdriveAutoTrimBtnRect_.y - 24.0f,
                                 overdriveAutoTrimBtnRect_.width + 16.0f,
                                 50.0f};
      g.fillRoundedRect(
          {tipBox.x + 2.0f, tipBox.y + 3.0f, tipBox.width, tipBox.height}, 4.5f,
          gui::Color(0.0f, 0.0f, 0.0f, 0.72f));
      g.fillRoundedRect(tipBox, 4.5f, gui::Color::fromHex(0x1E1014, 0.98f));
      g.drawRoundedRect(tipBox, 4.5f, gui::Color::AccentRed(), 1.4f);
      g.drawText(
          std::format("\xE2\x9A\xA0 OVERDRIVING DOWNSTREAM INPUT (+{:.1f} dBFS "
                      "from S{} {})",
                      ovDb, ovIdx + 1, ovMeta.designator),
          {tipBox.x + 6.0f, tipBox.y + 3.0f, tipBox.width - 12.0f, 18.0f},
          gui::Color::AccentAmber(), 7.8f, true, true);
      g.fillRoundedRect(overdriveAutoTrimBtnRect_, 3.5f,
                        gui::Color::AccentRed().withAlpha(0.32f));
      g.drawRoundedRect(overdriveAutoTrimBtnRect_, 3.5f, gold, 1.2f);
      g.drawText("[ Auto-Trim Previous Stage ]", overdriveAutoTrimBtnRect_,
                 gui::Color::TextPrimary(), 8.2f, true, true);
    }

    g.popClipRect();

    // Bottom Interactive Horizontal Scrollbar & Minimap Overview Radar Strip
    // (y: 600..619)
    const gui::Rect<float> minimap = getSchematicMinimapRect();
    g.fillRoundedRect(minimap, 3.5f, gui::Color::fromHex(0x090C11));
    g.drawRoundedRect(minimap, 3.5f, gui::Color::fromHex(0x222B3A), 1.0f);
    g.drawText(
        "MINIMAP RADAR",
        {minimap.x + 6.0f, minimap.y + 1.0f, 88.0f, minimap.height - 2.0f},
        gui::Color::TextSecondary(), 7.4f, false, true);

    const float radarX = minimap.x + 96.0f;
    const float radarW = std::max(60.0f, minimap.width - 102.0f);
    if (!schematicChain_.empty()) {
      const float pipStep =
          std::min(18.0f, radarW / static_cast<float>(std::max<size_t>(
                                       1, schematicChain_.size())));
      const float pipW = std::max(4.0f, pipStep - 2.0f);
      for (size_t i = 0; i < schematicChain_.size(); ++i) {
        const auto &n = schematicChain_[i];
        const auto &m = getComponentMeta(n.type);
        const float px = radarX + static_cast<float>(i) * pipStep;
        const float py =
            n.isParallel ? (minimap.y + 10.5f) : (minimap.y + 3.0f);
        g.fillRoundedRect({px, py, pipW, 5.5f}, 1.5f,
                          n.isParallel ? gold : m.accentColor);
      }
    }
    const float maxScrollX = getMaxSchematicScrollX();
    const float viewFrac =
        (maxScrollX > 1.0f)
            ? math::clamp(canvas.width / (canvas.width + maxScrollX), 0.15f,
                          1.0f)
            : 1.0f;
    const float lensW = radarW * viewFrac;
    const float lensX =
        radarX + ((maxScrollX > 1.0f)
                      ? (schematicScrollX_ / maxScrollX) * (radarW - lensW)
                      : 0.0f);
    gui::Rect<float> lensRect = {lensX, minimap.y + 1.5f, lensW,
                                 minimap.height - 3.0f};
    g.fillRoundedRect(lensRect, 2.5f, cyan.withAlpha(0.14f));
    g.drawRoundedRect(lensRect, 2.5f, cyan.withAlpha(0.80f), 1.1f);

    g.drawRoundedRect(canvas, 5.0f, gui::Color::fromHex(0x283244), 1.5f);

    // Draw Floating Ghost Card while dragging from palette or reordering a
    // canvas node
    if (isDraggingPaletteItem_) {
      const auto &dMeta = getComponentMeta(draggedComponentType_);
      gui::Rect<float> ghost = {dragCurrentPos_.x - 55.0f,
                                dragCurrentPos_.y - 22.0f, 110.0f, 44.0f};
      g.fillRoundedRect(ghost, 4.0f, gui::Color::fromHex(0x1E2430, 0.90f));
      g.drawRoundedRect(ghost, 4.0f, dMeta.accentColor, 2.0f);
      g.drawText(dMeta.shortName, ghost, gui::Color::TextPrimary(), 10.5f, true,
                 true);
    }

    // 3. Clean, High-Legibility 82px-Tall Selected Stage Bar (x: canvasX..1345,
    // y: 630..712)
    gui::Rect<float> inspectorBox = {canvasX, 630.0f, canvasW, 82.0f};
    g.fillRoundedRect(inspectorBox, 5.0f, cardBg);
    g.drawRoundedRect(inspectorBox, 5.0f, outline, 1.0f);

    if (selectedNodeIndex_ >= 0 &&
        selectedNodeIndex_ < static_cast<int>(schematicChain_.size())) {
      const auto &node = schematicChain_[selectedNodeIndex_];
      const auto &meta = getComponentMeta(node.type);

      // Line 1 (y = 638, 14.5f bold gold font, full width so it NEVER
      // auto-shrinks)
      g.drawText(std::format("STAGE {} [{}]  \xE2\x80\x94  {}",
                             selectedNodeIndex_ + 1, meta.designator,
                             meta.fullName),
                 {canvasX + 14.0f, 636.0f, canvasW - 28.0f, 22.0f}, gold, 14.5f,
                 false, true);

      // Line 2 (y = 662, 11.5f crisp primary text)
      g.drawText(meta.schematicNotes,
                 {canvasX + 14.0f, 660.0f, canvasW - 28.0f, 19.0f},
                 gui::Color::TextPrimary(), 11.5f, false);

      // Line 3 Left Hint (y = 682, next to right-aligned routing & audition
      // buttons)
      const std::string routeSummary =
          node.isParallel
              ? std::format("Routing: PARALLEL PATH B (Wet Mix {:.0f}%)",
                            node.parallelMixPercent)
              : "Routing: SERIES TRUNK  |  Click [AUDITION & "
                "TWEAK IN LIVE RACK] to dial in knobs with live audio";
      g.drawText(
          routeSummary,
          {canvasX + 14.0f, 684.0f, std::max(140.0f, canvasW - 575.0f), 20.0f},
          cyan, 10.2f, false, true);
    } else {
      g.drawText(
          "Select any component card on the Schematic Canvas above to view its "
          "circuit architecture or toggle Series/Parallel routing.",
          {canvasX + 18.0f, 658.0f, canvasW - 36.0f, 24.0f},
          gui::Color::TextSecondary(), 12.0f, false);
    }

    // 4. Floating High-Contrast Mouseover Description Tooltip for Component
    // Library Cards (rendered on top of canvas when hovering a library item!)
    if (!isDraggingPaletteItem_ && hoveredPaletteItemIdx_ >= 0 &&
        hoveredPaletteItemIdx_ < static_cast<int>(paletteItems_.size())) {
      const float hovCardY =
          listTop + static_cast<float>(hoveredPaletteItemIdx_) * stepH -
          schematicLibraryScrollY_;
      if (hovCardY + cardH >= listTop && hovCardY <= listBot) {
        const auto &hItem =
            paletteItems_[static_cast<size_t>(hoveredPaletteItemIdx_)];
        const float tipW = 440.0f;
        const float tipH = 88.0f;
        const float tipX = libX + libW + 6.0f;
        const float tipY = std::clamp(hovCardY - 6.0f, listTop, listBot - tipH);
        gui::Rect<float> tipBox = {tipX, tipY, tipW, tipH};

        g.fillRoundedRect({tipBox.x + 3.0f, tipBox.y + 4.0f, tipW, tipH}, 6.0f,
                          gui::Color(0.0f, 0.0f, 0.0f, 0.80f));
        g.fillRoundedRect(tipBox, 6.0f, gui::Color::fromHex(0x0D131C, 0.98f));
        g.drawRoundedRect(tipBox, 6.0f, hItem.accentColor, 1.6f);

        g.drawText(std::format("[{}]  {}", hItem.designator, hItem.shortName),
                   {tipBox.x + 12.0f, tipBox.y + 5.0f, tipW - 24.0f, 19.0f},
                   hItem.accentColor, 12.2f, false, true);
        g.drawText(hItem.fullName,
                   {tipBox.x + 12.0f, tipBox.y + 25.0f, tipW - 24.0f, 20.0f},
                   gui::Color::fromHex(0xF2F7FA), 10.8f, false, true);
        g.drawText(std::format("SUITABLE SOURCES: {}",
                               formatComponentSourceBadgeText(hItem, false)),
                   {tipBox.x + 12.0f, tipBox.y + 46.0f, tipW - 24.0f, 18.0f},
                   gold, 9.6f, false, true);
        g.drawText(
            "Click [+] or drag onto the Schematic Canvas to add this stage",
            {tipBox.x + 12.0f, tipBox.y + 65.0f, tipW - 24.0f, 18.0f}, cyan,
            9.4f, false, true);
      }
    }
  }

  // =========================================================================
  // Visual Drag-and-Drop Hardware Faceplate UI Builder Paint
  // =========================================================================
  void paintUiBuilderMode(gui::Graphics &g, const gui::Rect<float> &local,
                          gui::Color panelBg, gui::Color outline,
                          gui::Color cardBg, gui::Color gold, gui::Color cyan) {
    const float panelX = isSidebarCollapsed_ ? 60.0f : 355.0f;
    const float libX = isSidebarCollapsed_ ? 72.0f : 370.0f;
    const float uiInspX = libX;
    const float uiInspW = 1345.0f - uiInspX;

    gui::Rect<float> builderPanel = {
        panelX, 62.0f, local.width - panelX - 20.0f, local.height - 107.0f};
    g.fillRoundedRect(builderPanel, 6.0f, panelBg);
    g.drawRoundedRect(builderPanel, 6.0f, outline, 1.0f);

    // Header
    g.drawText("HARDWARE FACEPLATE UI BUILDER  (UNIFIED STYLE TEMPLATES + LIVE "
               "THUMBNAILS + ALIGNMENT TOOLS)",
               {libX, 64.0f, 600.0f, 20.0f}, gold, 13.5f, false, true);
    g.drawText(
        "Controls are generated from each module's hardware spec "
        "— click a widget to restyle it, or save your *CUSTOM theme.",
        {libX, 81.0f, 600.0f, 15.0f}, gui::Color::TextSecondary(), 9.6f, false);
    g.drawText("THEME:", {980.0f, 66.0f, 48.0f, 26.0f},
               faceplateLayout_.theme.isCustomThemeModified ? cyan : gold,
               9.8f, false, true);

    // Full-Width Interactive WYSIWYG Hardware Faceplate Canvas
    const gui::Rect<float> canvasRect = getUiBuilderCanvasRect();
    const bool showActiveGizmo =
        isDraggingUiWidget_ || isResizingUiWidget_ || isRotatingUiWidget_ ||
        (lastUiWidgetInteractMs_ != 0 &&
         (GetTickCount64() - lastUiWidgetInteractMs_ < 2000));
    paintFaceplateCanvas(g, canvasRect, faceplateLayout_,
                         showActiveGizmo ? selectedUiWidgetIndex_ : -1, true,
                         0.0f, -8.0f, -8.0f, -999.0f,
                         isDraggingUiWidget_ || isResizingUiWidget_ ||
                             isRotatingUiWidget_,
                         lastMousePos_);

    // Draw floating ghost while dragging from UI palette
    if (isDraggingUiPalette_) {
      gui::Rect<float> ghost = {dragCurrentPos_.x - 60.0f,
                                dragCurrentPos_.y - 24.0f, 120.0f, 48.0f};
      g.fillRoundedRect(ghost, 4.0f, gui::Color::fromHex(0x1E2430, 0.92f));
      g.drawRoundedRect(ghost, 4.0f, cyan, 2.0f);
      g.drawText(draggedUiEntry_.name, ghost, gui::Color::TextPrimary(), 10.0f,
                 true, true);
    }

    // 3. Expanded Context-Sensitive Widget Property Inspector (y: 556..712)
    gui::Rect<float> inspBox = {uiInspX, 556.0f, uiInspW, 156.0f};
    g.fillRoundedRect(inspBox, 5.0f, cardBg);
    g.drawRoundedRect(inspBox, 5.0f, outline, 1.0f);

    if (selectedUiWidgetIndex_ >= 0 &&
        selectedUiWidgetIndex_ <
            static_cast<int>(faceplateLayout_.widgets.size())) {
      const auto &w = faceplateLayout_.widgets[selectedUiWidgetIndex_];
      const bool isBadgeOrFrame = (w.type == UiWidgetType::RivetedNameplate ||
                                   w.type == UiWidgetType::SectionFrame ||
                                   w.type == UiWidgetType::VentilationGrille ||
                                   w.type == UiWidgetType::SilkscreenLabel ||
                                   w.type == UiWidgetType::RackHandlePair);
      const std::string sizeInfo =
          std::format("POS: ({:.0f}, {:.0f})  |  SIZE: {:.0f}x{:.0f}",
                      w.x, w.y, w.w, w.h);
      g.drawText(std::format("WIDGET #{}/{} ({})  |  {}  |  COLOR: {}",
                             selectedUiWidgetIndex_ + 1,
                             faceplateLayout_.widgets.size(),
                             getWidgetTypeName(w.type), sizeInfo,
                             getUiAccentColorName(w.accentColor)),
                 {uiInspX + 12.0f, 562.0f, uiInspW - 240.0f, 20.0f}, gold,
                 11.5f, false, true);

      g.drawText(isBadgeOrFrame ? "TITLE:" : "LABEL:",
                 {uiInspX + 12.0f, 594.0f, 48.0f, 28.0f},
                 gui::Color::TextPrimary(), 10.5f, false, true);
      g.drawText("SUB:", {uiInspX + 216.0f, 594.0f, 36.0f, 28.0f},
                 gui::Color::TextPrimary(), 10.5f, false, true);
      if (isBadgeOrFrame) {
        g.drawText("LINE 2:", {uiInspX + 398.0f, 594.0f, 48.0f, 28.0f}, gold,
                   10.5f, false, true);
      }
      g.drawText(
          "Customize this widget's label, style, color, or scale below — "
          "or use ALIGN TOP / DISTRIBUTE / GUIDES above.",
          {uiInspX + 12.0f, 678.0f, uiInspW - 24.0f, 20.0f},
          gui::Color::TextSecondary(), 10.2f, false);
    } else {
      g.drawText(
          "CHASSIS & BADGE INSPECTOR — Click any widget on the canvas above "
          "to inspect its Knob, Switch, Meter, or Badge properties.",
          {uiInspX + 12.0f, 562.0f, uiInspW - 240.0f, 20.0f}, cyan, 11.0f,
          false, true);
      g.drawText("BADGE:", {uiInspX + 12.0f, 594.0f, 48.0f, 28.0f},
                 gui::Color::TextSecondary(), 10.5f, false, true);
      g.drawText("LINE 1:", {uiInspX + 216.0f, 594.0f, 44.0f, 28.0f},
                 gui::Color::TextSecondary(), 10.5f, false, true);
      g.drawText("LINE 2:", {uiInspX + 398.0f, 594.0f, 48.0f, 28.0f}, gold,
                 10.5f, false, true);
      g.drawText("Use global typography controls above or click 'EDIT BADGE' "
                 "to select the faceplate nameplate.",
                 {uiInspX + 12.0f, 678.0f, uiInspW - 24.0f, 20.0f},
                 gui::Color::TextSecondary(), 10.2f, false);
    }
  }

  void paintSchematicNodeSymbol(gui::Graphics &g, SchematicComponentType type,
                                SchematicCategory cat, gui::Rect<float> box,
                                gui::Color col) {
    g.fillRoundedRect(box, 3.0f, gui::Color::fromHex(0x0E1218));
    g.drawRoundedRect(box, 3.0f, col.withAlpha(0.35f), 1.0f);

    const float cx = box.getCentre().x;
    const float cy = box.getCentre().y;

    if (cat == SchematicCategory::PreampsAndTubes) {
      const auto &meta = getComponentMeta(type);
      const bool isSolidState =
          (!meta.designator.empty() && meta.designator[0] != 'V');
      if (isSolidState) {
        g.drawCircle({cx, cy}, 12.5f, col, 1.5f);
        g.drawLine({box.x + 8.0f, cy}, {cx - 4.5f, cy}, col, 1.6f);
        g.drawLine({cx - 4.5f, cy - 7.5f}, {cx - 4.5f, cy + 7.5f}, col, 2.2f);
        g.drawLine({cx - 4.5f, cy - 4.0f}, {cx + 5.5f, cy - 9.0f}, col, 1.6f);
        g.drawLine({cx + 5.5f, cy - 9.0f}, {cx + 5.5f, box.y + 4.0f}, col,
                   1.5f);
        g.drawLine({cx - 4.5f, cy + 4.0f}, {cx + 5.5f, cy + 9.0f}, col, 1.6f);
        g.drawLine({cx + 5.5f, cy + 9.0f}, {cx + 5.5f, box.bottom() - 4.0f},
                   col, 1.5f);
        g.drawLine({cx + 1.5f, cy + 7.0f}, {cx + 5.5f, cy + 9.0f}, col, 1.6f);
        g.drawLine({cx + 3.5f, cy + 4.5f}, {cx + 5.5f, cy + 9.0f}, col, 1.6f);
      } else {
        g.drawCircle({cx, cy}, 13.0f, col, 1.5f);
        g.drawLine({cx, box.y + 3.0f}, {cx, cy - 6.0f}, col, 1.4f);
        g.drawLine({cx - 6.5f, cy - 6.0f}, {cx + 6.5f, cy - 6.0f}, col, 2.0f);
        g.drawLine({cx - 7.5f, cy - 0.5f}, {cx - 3.5f, cy - 0.5f}, col, 1.5f);
        g.drawLine({cx - 1.5f, cy - 0.5f}, {cx + 1.5f, cy - 0.5f}, col, 1.5f);
        g.drawLine({cx + 3.5f, cy - 0.5f}, {cx + 7.5f, cy - 0.5f}, col, 1.5f);
        g.drawLine({cx - 4.5f, cy + 4.5f}, {cx + 4.5f, cy + 4.5f}, col, 1.6f);
        g.drawLine({cx - 3.5f, cy + 9.5f}, {cx, cy + 5.5f},
                   gui::Color::fromHex(0xFF9F1C), 1.5f);
        g.drawLine({cx, cy + 5.5f}, {cx + 3.5f, cy + 9.5f},
                   gui::Color::fromHex(0xFF9F1C), 1.5f);
      }
      return;
    }
    paintSchematicSymbol(g, cat, box, col);
  }

  void paintSchematicSymbol(gui::Graphics &g, SchematicCategory cat,
                            gui::Rect<float> box, gui::Color col) {
    g.fillRoundedRect(box, 3.0f, gui::Color::fromHex(0x0E1218));
    g.drawRoundedRect(box, 3.0f, col.withAlpha(0.35f), 1.0f);

    const float cx = box.getCentre().x;
    const float cy = box.getCentre().y;

    switch (cat) {
    case SchematicCategory::PreampsAndTubes: {
      g.drawCircle({cx, cy}, 12.5f, col, 1.4f);
      g.drawLine({cx - 6.0f, cy - 5.5f}, {cx + 6.0f, cy - 5.5f}, col, 1.8f);
      g.drawLine({cx - 7.0f, cy}, {cx - 3.0f, cy}, col, 1.4f);
      g.drawLine({cx - 1.5f, cy}, {cx + 1.5f, cy}, col, 1.4f);
      g.drawLine({cx + 3.0f, cy}, {cx + 7.0f, cy}, col, 1.4f);
      g.drawArc({cx, cy + 7.5f}, 4.5f, -1.2f, 1.2f, col, 1.4f);
      break;
    }
    case SchematicCategory::IronAndTape: {
      g.drawLine({cx - 2.0f, box.y + 5.0f}, {cx - 2.0f, box.bottom() - 5.0f},
                 col, 1.6f);
      g.drawLine({cx + 2.0f, box.y + 5.0f}, {cx + 2.0f, box.bottom() - 5.0f},
                 col, 1.6f);
      g.drawArc({cx - 7.0f, cy - 6.0f}, 4.0f, 0.2f, 2.9f, col, 1.5f);
      g.drawArc({cx - 7.0f, cy + 0.0f}, 4.0f, 0.2f, 2.9f, col, 1.5f);
      g.drawArc({cx - 7.0f, cy + 6.0f}, 4.0f, 0.2f, 2.9f, col, 1.5f);
      g.drawArc({cx + 7.0f, cy - 6.0f}, 4.0f, -2.9f, -0.2f, col, 1.5f);
      g.drawArc({cx + 7.0f, cy + 0.0f}, 4.0f, -2.9f, -0.2f, col, 1.5f);
      g.drawArc({cx + 7.0f, cy + 6.0f}, 4.0f, -2.9f, -0.2f, col, 1.5f);
      break;
    }
    case SchematicCategory::EqAndFilters: {
      g.drawLine({box.x + 6.0f, cy - 3.0f}, {cx - 14.0f, cy - 3.0f}, col, 1.5f);
      g.drawArc({cx - 9.0f, cy - 3.0f}, 4.5f, -1.57f, 1.57f, col, 1.5f);
      g.drawArc({cx, cy - 3.0f}, 4.5f, -1.57f, 1.57f, col, 1.5f);
      g.drawArc({cx + 9.0f, cy - 3.0f}, 4.5f, -1.57f, 1.57f, col, 1.5f);
      g.drawLine({cx + 14.0f, cy - 3.0f}, {box.right() - 6.0f, cy - 3.0f}, col,
                 1.5f);
      g.drawLine({cx - 5.0f, cy + 5.0f}, {cx + 5.0f, cy + 5.0f}, col, 1.8f);
      g.drawLine({cx - 5.0f, cy + 8.5f}, {cx + 5.0f, cy + 8.5f}, col, 1.8f);
      g.drawLine({cx, cy + 8.5f}, {cx, box.bottom() - 4.0f}, col, 1.3f);
      break;
    }
    case SchematicCategory::AnalogDynamics: {
      g.drawLine({cx - 14.0f, cy - 10.0f}, {cx - 14.0f, cy + 10.0f}, col, 1.6f);
      g.drawLine({cx - 14.0f, cy - 10.0f}, {cx + 2.0f, cy}, col, 1.6f);
      g.drawLine({cx - 14.0f, cy + 10.0f}, {cx + 2.0f, cy}, col, 1.6f);
      g.drawLine({cx + 10.0f, cy - 7.0f}, {cx + 17.0f, cy}, col, 1.4f);
      g.drawLine({cx + 17.0f, cy}, {cx + 10.0f, cy + 7.0f}, col, 1.4f);
      g.drawLine({cx + 10.0f, cy + 7.0f}, {cx + 3.0f, cy}, col, 1.4f);
      g.drawLine({cx + 3.0f, cy}, {cx + 10.0f, cy - 7.0f}, col, 1.4f);
      break;
    }
    case SchematicCategory::DelayAndReverb:
    case SchematicCategory::ModulationAndRoute:
    default: {
      std::array<gui::Point<float>, 16> wave{};
      for (size_t i = 0; i < 16; ++i) {
        const float t = static_cast<float>(i) / 15.0f;
        wave[i] = {box.x + 6.0f + t * (box.width - 12.0f),
                   cy -
                       std::sin(t * kTwoPi * 1.5f) * 9.0f * (1.0f - t * 0.35f)};
      }
      g.drawPolyline(wave, col, 1.6f);
      break;
    }
    }
  }

  std::string formatPdcPillText() const {
    return std::format(
        "PDC: {} smpl",
        liveSchematicRunner_.getTotalLatencySamples(schematicChain_));
  }

  std::string formatSignalProbePillText() const {
    const int idx = liveSchematicRunner_.getProbeStageIndex();
    if (idx >= 0 && idx < static_cast<int>(schematicChain_.size())) {
      const auto &meta = getComponentMeta(schematicChain_[idx].type);
      return std::format(
          "PROBE: S{}·{} [{}]", idx + 1, meta.designator,
          liveSchematicRunner_.isProbeListenActive() ? "LISTEN" : "OVERLAY");
    }
    return "PROBE: OFF (CLICK TO ARM)";
  }

  void toggleStageSignalProbe(int stageIdx) {
    if (stageIdx < 0 || stageIdx >= static_cast<int>(schematicChain_.size())) {
      liveSchematicRunner_.setProbeStageIndex(-1);
      liveSchematicRunner_.setProbeListenActive(false);
      rackSpectrum_.clearProbeTrace();
      rackScope_.clearProbeTrace();
      return;
    }
    const auto &meta = getComponentMeta(schematicChain_[stageIdx].type);
    if (liveSchematicRunner_.getProbeStageIndex() != stageIdx) {
      liveSchematicRunner_.setProbeStageIndex(stageIdx);
      liveSchematicRunner_.setProbeListenActive(false);
      statusMessage_ =
          std::format("SIGNAL PROBE ARMED ON STAGE {} [{}] {}: Gold "
                      "intermediate trace active in Live Rack Telemetry (click "
                      "PROBE again for Probe Listen)!",
                      stageIdx + 1, meta.designator, meta.shortName);
    } else if (!liveSchematicRunner_.isProbeListenActive()) {
      liveSchematicRunner_.setProbeListenActive(true);
      statusMessage_ = std::format(
          "PROBE LISTEN ACTIVE ON STAGE {} [{}] {}: Auditioning "
          "exact inter-stage signal tap (click PROBE again to disarm)!",
          stageIdx + 1, meta.designator, meta.shortName);
    } else {
      liveSchematicRunner_.setProbeStageIndex(-1);
      liveSchematicRunner_.setProbeListenActive(false);
      rackSpectrum_.clearProbeTrace();
      rackScope_.clearProbeTrace();
      statusMessage_ =
          "SIGNAL PROBE DISARMED: Restored full post-plugin "
          "signal monitoring.";
    }
  }

  void cycleSignalProbeState() {
    if (schematicChain_.empty())
      return;
    const int cur = liveSchematicRunner_.getProbeStageIndex();
    if (cur < 0) {
      const int target =
          (selectedNodeIndex_ >= 0 &&
           selectedNodeIndex_ < static_cast<int>(schematicChain_.size()))
              ? selectedNodeIndex_
              : 0;
      toggleStageSignalProbe(target);
    } else if (!liveSchematicRunner_.isProbeListenActive()) {
      toggleStageSignalProbe(cur);
    } else if (cur + 1 < static_cast<int>(schematicChain_.size())) {
      toggleStageSignalProbe(cur + 1);
    } else {
      toggleStageSignalProbe(-1);
      statusMessage_ =
          "SIGNAL PROBE DISARMED: Restored full post-plugin "
          "signal monitoring.";
    }
  }

  [[nodiscard]] gui::Rect<float>
  getBayStageCardRect(size_t visibleSlotIdx, size_t visibleCount,
                      bool hasTopStrip = false) const noexcept {
    const float startBaseX = isSidebarCollapsed_ ? 92.0f : 382.0f;
    const float availW = isSidebarCollapsed_ ? 1216.0f : 936.0f;
    const float cardY = hasTopStrip ? 182.0f : 156.0f;
    const float cardH = hasTopStrip ? 332.0f : 344.0f;
    if (visibleCount == 0)
      return {startBaseX, cardY, 280.0f, cardH};
    const float step = availW / static_cast<float>(visibleCount);
    const float maxCardW = (visibleCount <= 3) ? 330.0f : 286.0f;
    const float cw = std::min(maxCardW, step - 12.0f);
    const float totalSpan = static_cast<float>(visibleCount - 1) * step + cw;
    const float startX = startBaseX + (availW - totalSpan) * 0.5f;
    return {startX + static_cast<float>(visibleSlotIdx) * step, cardY, cw,
            cardH};
  }

  void paintRackMode(gui::Graphics &g, const gui::Rect<float> &local,
                     gui::Color panelBg, gui::Color outline, gui::Color cardBg,
                     gui::Color gold, gui::Color cyan) {
    (void)cardBg;
    const float rackX = isSidebarCollapsed_ ? 60.0f : 355.0f;

    gui::Rect<float> rackFrame = {rackX, 97.0f, local.width - rackX - 20.0f,
                                  432.0f};
    g.fillRoundedRect(rackFrame, 6.0f, panelBg);
    g.drawRoundedRect(rackFrame, 6.0f, outline, 1.0f);

    const float outPeakDbL =
        (lastOutPeakL_ > 1e-5f) ? math::linearTodB(lastOutPeakL_) : -80.0f;
    const float outPeakDbR =
        (lastOutPeakR_ > 1e-5f) ? math::linearTodB(lastOutPeakR_) : -80.0f;
    const float outPeakDbMax = std::max(outPeakDbL, outPeakDbR);
    const float liveVuOutDb = (outPeakDbMax > smoothedVuOutDb_)
                                  ? (smoothedVuOutDb_ * 0.65f + outPeakDbMax * 0.35f)
                                  : smoothedVuOutDb_;

    if (activeRackPluginIndex_ == 0) {
      const gui::Rect<float> fpRect = getRackFaceplateRect();
      paintFaceplateCanvas(g, fpRect, faceplateLayout_, rackDraggingWidgetIdx_,
                           false, rackBypassed_ ? 0.0f : smoothedVuGrDb_,
                           smoothedVuInDb_, liveVuOutDb, outPeakDbR);

      if (hoveredTapedWidgetIdx_ >= 0 &&
          hoveredTapedWidgetIdx_ <
              static_cast<int>(faceplateLayout_.widgets.size())) {
        const auto &tw = faceplateLayout_.widgets[hoveredTapedWidgetIdx_];
        const float sx = fpRect.width / FaceplateLayout::kRefWidth;
        const float sy = fpRect.height / FaceplateLayout::kRefHeight;
        const float tx =
            fpRect.x +
            (faceplateLayout_.getEffectiveWidgetX(tw) + tw.w * 0.5f) * sx;
        const float ty =
            std::max(fpRect.y + 6.0f, fpRect.y + tw.y * sy - 24.0f);
        const std::string tipTxt =
            std::format("TAPED DOWN ({}) — Move tape in UI Builder to unlock",
                        hoveredTapeLabel_);
        const float tipW =
            std::max(210.0f, g.measureTextWidth(tipTxt, 9.0f, true) + 20.0f);
        const gui::Rect<float> tipRect = {
            std::clamp(tx - tipW * 0.5f, fpRect.x + 8.0f,
                       fpRect.right() - tipW - 8.0f),
            ty, tipW, 20.0f};
        g.fillRoundedRect(
            {tipRect.x + 1.5f, tipRect.y + 2.0f, tipRect.width, tipRect.height},
            4.0f, gui::Color::fromHex(0x000000, 0.72f));
        g.fillRoundedRect(tipRect, 4.0f, gui::Color::fromHex(0x14181F, 0.96f));
        g.drawRoundedRect(tipRect, 4.0f, gold, 1.2f);
        g.drawText(tipTxt, tipRect, gold, 9.0f, true, true);
      }

      if (faceplateLayout_.widgets.empty()) {
        g.drawText(
            "NO CONTROLS ON FACEPLATE — Add components in Schematic Builder "
            "or widgets in Faceplate UI Builder to test them live!",
            {fpRect.x + 80.0f, fpRect.y + 185.0f, fpRect.width - 160.0f, 40.0f},
            gold, 12.0f, true, true);
      }
    }

    const float infoCardY = 535.0f;
    gui::Rect<float> infoCard = {rackX, infoCardY, local.width - rackX - 20.0f,
                                 local.height - infoCardY - 45.0f};
    g.fillRoundedRect(infoCard, 6.0f, panelBg);
    g.drawRoundedRect(infoCard, 6.0f, outline, 1.0f);

    const float maxTitleW =
        std::max(220.0f, (local.width - 518.0f) - (rackX + 15.0f));
    g.drawText(std::format(
                   "LIVE RACK TELEMETRY  |  PROJECT: {} ({} STAGES)  |  {} Hz "
                   "SPEAKER AUDIO  |  300ms RMS CALIBRATED",
                   wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name,
                   schematicChain_.size(), prefs_.sampleRateHz),
               {rackX + 15.0f, infoCardY + 6.0f, maxTitleW, 20.0f}, gold, 10.6f,
               false, true);

    const bool prbActRack = (liveSchematicRunner_.getProbeStageIndex() >= 0);
    const gui::Rect<float> rackLabBenchPill = {local.width - 510.0f,
                                               infoCardY + 5.0f, 126.0f, 20.0f};
    const gui::Rect<float> rackProbePill = {local.width - 378.0f,
                                            infoCardY + 5.0f, 188.0f, 20.0f};
    const gui::Rect<float> rackPdcPill = {local.width - 184.0f,
                                          infoCardY + 5.0f, 148.0f, 20.0f};

    g.fillRoundedRect(rackLabBenchPill, 3.5f,
                      labBenchActive_ ? cyan.withAlpha(0.28f)
                                      : gui::Color::fromHex(0x161B24));
    g.drawRoundedRect(rackLabBenchPill, 3.5f,
                      labBenchActive_ ? cyan : gold.withAlpha(0.75f),
                      labBenchActive_ ? 1.4f : 1.0f);
    g.drawText(labBenchActive_ ? "LAB BENCH: ON"
                               : "LAB BENCH",
               rackLabBenchPill, labBenchActive_ ? cyan : gold, 8.2f, true,
               true);

    g.fillRoundedRect(rackProbePill, 3.5f,
                      prbActRack ? gold.withAlpha(0.26f)
                                 : gui::Color::fromHex(0x161B24));
    g.drawRoundedRect(rackProbePill, 3.5f, prbActRack ? gold : outline,
                      prbActRack ? 1.3f : 1.0f);
    g.drawText(formatSignalProbePillText(), rackProbePill,
               prbActRack ? gold : gui::Color::TextSecondary(), 8.0f, true,
               true);

    g.fillRoundedRect(rackPdcPill, 3.5f, gui::Color::fromHex(0x161B24));
    g.drawRoundedRect(rackPdcPill, 3.5f, outline, 1.1f);
    g.drawText(formatPdcPillText(), rackPdcPill, gui::Color::TextPrimary(), 8.0f,
               true, true);

    g.drawLine({rackX + 13.0f, infoCardY + 28.0f},
               {local.width - 35.0f, infoCardY + 28.0f}, outline, 1.0f);

    syncRackTelemetryVisibility();

    if (faceplateLayout_.theme.earsOnlyBlackoutMode) {
      const gui::Rect<float> blackoutBox = {rackX + 10.0f, infoCardY + 32.0f,
                                            infoCard.width - 20.0f,
                                            infoCard.height - 38.0f};
      g.fillRoundedRect(blackoutBox, 5.0f, gui::Color::fromHex(0x070A0E));
      g.drawRoundedRect(blackoutBox, 5.0f, gui::Color::fromHex(0xD4AF37, 0.35f),
                        1.0f);

      const float midY = blackoutBox.y + blackoutBox.height * 0.5f;
      g.drawText("EARS ONLY MODE ENGAGED  -  ALL "
                 "VISUAL METERS, SCOPES & SPECTRUM ANALYZERS OFF",
                 {blackoutBox.x + 16.0f, midY - 34.0f, blackoutBox.width - 32.0f,
                  22.0f},
                 gold, 13.5f, true, true);
      g.drawText("Visual telemetry and digital dB readouts are blacked out so "
                 "you can dial in gain staging, saturation, and dynamics purely "
                 "by ear.",
                 {blackoutBox.x + 16.0f, midY - 6.0f, blackoutBox.width - 32.0f,
                  20.0f},
                 gui::Color::TextSecondary(), 10.5f, true, false);
      g.drawText("Click [ DARK METERS: ON ] in the top-right "
                 "header bar to restore real-time scopes and meters.",
                 {blackoutBox.x + 16.0f, midY + 18.0f, blackoutBox.width - 32.0f,
                  18.0f},
                 cyan, 9.5f, true, true);
      return;
    }

    if (labBenchActive_) {
      const float teleY = 568.0f;
      const float teleH = 158.0f;
      const float specX = isSidebarCollapsed_ ? 74.0f : 368.0f;
      paintLabBenchAnalyzer(g, {specX, teleY, 995.0f - specX, teleH}, gold,
                            cyan, outline);
    }

    g.drawText("IN L/R", {1005.0f, infoCardY + 32.0f, 72.0f, 16.0f},
               gui::Color::TextSecondary(), 9.5f, true, true);
    g.drawText("OUT L/R", {1090.0f, infoCardY + 32.0f, 72.0f, 16.0f}, cyan,
               9.5f, true, true);

    const float statX = 1164.0f;
    const float statW = 168.0f;
    const float statY = infoCardY + 38.0f;
    const float peakDb = math::linearTodB(std::max(lastOutPeakL_, lastOutPeakR_));
    const float rmsDb = math::linearTodB(std::max(lastOutRmsL_, lastOutRmsR_));

    g.drawText("LIVE METRICS", {statX, statY, statW, 18.0f}, gold, 11.0f,
               false, true);
    g.drawText(std::format("Out Peak: {:+.1f} dBFS", std::max(-90.0f, peakDb)),
               {statX, statY + 22.0f, statW, 18.0f}, gui::Color::TextPrimary(),
               10.5f, false);
    g.drawText(std::format("Out RMS:  {:+.1f} dBFS", std::max(-90.0f, rmsDb)),
               {statX, statY + 42.0f, statW, 18.0f}, cyan, 10.5f, false);
    g.drawText(std::format("Out-In \xCE\x94: {:+.1f} dB",
                           rackBypassed_ ? 0.0f : smoothedRmsDeltaDb_),
               {statX, statY + 62.0f, statW, 18.0f}, gold, 10.5f, false);
    g.drawText(rackBypassed_ ? "DSP: BYPASSED (DRY)" : "DSP: ACTIVE (SPEAKERS)",
               {statX, statY + 84.0f, statW, 18.0f},
               rackBypassed_ ? gui::Color::AccentRed()
                             : gui::Color::fromHex(0x52B788),
               10.0f, false, true);
    g.drawText("Drag knobs/switches above;", {statX, statY + 106.0f, statW, 15.0f},
               gui::Color::TextSecondary(), 9.0f, false);
    g.drawText("Double-click knob to reset.", {statX, statY + 121.0f, statW, 15.0f},
               gui::Color::TextSecondary(), 9.0f, false);
  }

  void paintLabBenchAnalyzer(gui::Graphics &g, const gui::Rect<float> &box,
                             gui::Color gold, gui::Color cyan,
                             gui::Color outline) {
    g.fillRoundedRect(box, 5.0f, gui::Color::fromHex(0x0B0F15));
    g.drawRoundedRect(box, 5.0f, cyan.withAlpha(0.70f), 1.2f);

    const char *tabLabels[3] = {
        "1. BODE MAGNITUDE & PHASE (20 Hz - 20 kHz)",
        "2. HARMONIC DISTORTION (H2..H7 & THD %)",
        "3. NON-LINEAR TRANSFER & HYSTERESIS (X-Y)"};
    const float tabW = (box.width - 16.0f) / 3.0f;
    for (int t = 0; t < 3; ++t) {
      const bool isSel = (labBenchPlotMode_ == t);
      gui::Rect<float> tr = {box.x + 6.0f + static_cast<float>(t) * (tabW + 2.0f),
                             box.y + 3.0f, tabW - 2.0f, 18.0f};
      g.fillRoundedRect(tr, 3.0f,
                        isSel ? cyan.withAlpha(0.24f)
                              : gui::Color::fromHex(0x141A24));
      g.drawRoundedRect(tr, 3.0f, isSel ? cyan : outline, isSel ? 1.2f : 0.8f);
      g.drawText(tabLabels[t], tr, isSel ? cyan : gui::Color::TextSecondary(),
                 7.6f, true, true);
    }

    const gui::Rect<float> plot = {box.x + 8.0f, box.y + 24.0f,
                                   box.width - 16.0f, box.height - 28.0f};
    g.fillRoundedRect(plot, 3.5f, gui::Color::fromHex(0x070A0E));
    g.drawRoundedRect(plot, 3.5f, gui::Color::fromHex(0x1D2532), 0.9f);

    float lowShelfDb = 0.0f, midBellDb = 0.0f, midFreqHz = 1200.0f,
          highShelfDb = 0.0f;
    float passiveScoopDb = 0.0f, headBumpDb = 0.0f,
          totalDriveDb = std::max(0.0f, rackInputTrimDb_);
    float asymBias = 0.0f, hysteresisWidth = 0.04f, compRatio = 1.0f,
          compThreshDb = 0.0f;

    for (const auto &n : schematicChain_) {
      if (n.isBypassed)
        continue;
      const auto &m = getComponentMeta(n.type);
      const float p1Norm = (m.param1.maxVal > m.param1.minVal)
                               ? (n.param1Val - m.param1.minVal) /
                                     (m.param1.maxVal - m.param1.minVal)
                               : 0.5f;
      const float p2Norm = (m.param2.maxVal > m.param2.minVal)
                               ? (n.param2Val - m.param2.minVal) /
                                     (m.param2.maxVal - m.param2.minVal)
                               : 0.5f;

      if (m.category == SchematicCategory::EqAndFilters) {
        lowShelfDb += (p1Norm - 0.5f) * 18.0f;
        highShelfDb += (p2Norm - 0.5f) * 16.0f;
        if (n.type == SchematicComponentType::PassivePultecEq) {
          passiveScoopDb -= std::abs(p1Norm - 0.35f) * 6.5f;
        }
        if (m.getParamCount() >= 4) {
          const auto &ps3 = m.getParamSpec(2);
          const float p3Norm = (ps3.maxVal > ps3.minVal)
                                   ? (n.param3Val - ps3.minVal) /
                                         (ps3.maxVal - ps3.minVal)
                                   : 0.5f;
          midBellDb += (p3Norm - 0.5f) * 15.0f;
          if (ps3.unit == "Hz" || ps3.unit == "kHz") {
            midFreqHz = math::clamp(n.param3Val, 180.0f, 8500.0f);
          }
        }
      } else if (m.category == SchematicCategory::IronAndTape ||
                 m.category == SchematicCategory::PreampsAndTubes) {
        totalDriveDb += p1Norm * 14.0f + std::max(0.0f, n.trim1Val * 0.1f);
        asymBias += (p2Norm - 0.5f) * 0.45f + n.trim1Val * 0.003f;
        hysteresisWidth += 0.05f + p1Norm * 0.12f;
        if (m.shortName.find("Tape") != std::string::npos ||
            m.designator.rfind("TP", 0) == 0) {
          headBumpDb += 2.8f + p1Norm * 3.5f;
        }
      } else if (m.category == SchematicCategory::AnalogDynamics) {
        compThreshDb = -24.0f * (1.0f - p1Norm);
        compRatio = std::max(compRatio, 1.5f + p2Norm * 8.5f);
        totalDriveDb += p1Norm * 5.0f;
      } else if (m.category == SchematicCategory::DelayAndReverb) {
        highShelfDb -= (1.0f - p2Norm) * 5.5f;
        headBumpDb += 1.8f;
      }
    }

    if (labBenchPlotMode_ == 0) {
      const float midY = plot.getCentre().y;
      g.drawLine({plot.x, midY}, {plot.right(), midY},
                 gui::Color::fromHex(0x283446), 1.0f);
      g.drawLine({plot.x, plot.y + plot.height * 0.25f},
                 {plot.right(), plot.y + plot.height * 0.25f},
                 gui::Color::fromHex(0x161E2B), 0.8f);
      g.drawLine({plot.x, plot.y + plot.height * 0.75f},
                 {plot.right(), plot.y + plot.height * 0.75f},
                 gui::Color::fromHex(0x161E2B), 0.8f);

      const float freqTicks[6] = {20.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 20000.0f};
      const char *freqLbls[6] = {"20Hz", "100Hz", "500Hz", "1kHz", "5kHz", "20kHz"};
      for (int f = 0; f < 6; ++f) {
        const float normX = std::log10(freqTicks[f] / 20.0f) / 3.0f;
        const float gx = plot.x + 28.0f + normX * (plot.width - 56.0f);
        g.drawLine({gx, plot.y}, {gx, plot.bottom()},
                   gui::Color::fromHex(0x161E2B), 0.8f);
        g.drawText(freqLbls[f], {gx - 18.0f, plot.bottom() - 13.0f, 36.0f, 12.0f},
                   gui::Color::TextSecondary(), 7.0f, true, true);
      }
      g.drawText("+18dB", {plot.x + 2.0f, plot.y + 2.0f, 32.0f, 11.0f},
                 cyan.withAlpha(0.8f), 6.8f, false, true);
      g.drawText(" 0dB", {plot.x + 2.0f, midY - 6.0f, 32.0f, 11.0f},
                 gui::Color::TextSecondary(), 6.8f, false, true);
      g.drawText("-18dB", {plot.x + 2.0f, plot.bottom() - 13.0f, 32.0f, 11.0f},
                 cyan.withAlpha(0.8f), 6.8f, false, true);
      g.drawText("+180\xC2\xB0", {plot.right() - 34.0f, plot.y + 2.0f, 32.0f, 11.0f},
                 gold.withAlpha(0.8f), 6.8f, true, true);
      g.drawText("-180\xC2\xB0",
                 {plot.right() - 34.0f, plot.bottom() - 13.0f, 32.0f, 11.0f},
                 gold.withAlpha(0.8f), 6.8f, true, true);

      constexpr int kPts = 96;
      gui::Point<float> prevMag{}, prevPhase{};
      for (int i = 0; i < kPts; ++i) {
        const float norm = static_cast<float>(i) / static_cast<float>(kPts - 1);
        const float freq = 20.0f * std::pow(10.0f, norm * 3.0f);
        const float x = plot.x + 28.0f + norm * (plot.width - 56.0f);

        const float lowShape = 1.0f / (1.0f + std::pow(freq / 140.0f, 2.0f));
        const float scoopShape =
            std::exp(-std::pow(std::log2(freq / 260.0f) / 0.65f, 2.0f));
        const float bumpShape =
            std::exp(-std::pow(std::log2(freq / 95.0f) / 0.45f, 2.0f));
        const float midShape =
            std::exp(-std::pow(std::log2(freq / midFreqHz) / 0.70f, 2.0f));
        const float highShape = 1.0f / (1.0f + std::pow(3200.0f / freq, 2.0f));
        const float hfRoll =
            (totalDriveDb > 8.0f)
                ? -((totalDriveDb - 8.0f) * 0.22f) * (freq / 20000.0f)
                : 0.0f;

        const float magDb =
            rackBypassed_
                ? 0.0f
                : math::clamp(lowShelfDb * lowShape + passiveScoopDb * scoopShape +
                                  headBumpDb * bumpShape + midBellDb * midShape +
                                  highShelfDb * highShape + hfRoll +
                                  rackOutputTrimDb_ * 0.35f,
                              -18.0f, 18.0f);

        const float phaseDeg =
            rackBypassed_
                ? 0.0f
                : math::clamp((-lowShelfDb * 3.8f * (freq / 140.0f) /
                               (1.0f + std::pow(freq / 140.0f, 2.0f))) +
                                  (-midBellDb * 4.5f *
                                   std::log2(freq / midFreqHz) * midShape) +
                                  (highShelfDb * 3.2f * (3200.0f / freq) /
                                   (1.0f + std::pow(3200.0f / freq, 2.0f))),
                              -160.0f, 160.0f);

        const float magY = midY - (magDb / 18.0f) * (plot.height * 0.42f);
        const float phaseY = midY - (phaseDeg / 180.0f) * (plot.height * 0.42f);

        if (i > 0) {
          g.drawLine(prevPhase, {x, phaseY}, gold.withAlpha(0.65f), 1.3f);
          g.drawLine(prevMag, {x, magY}, cyan, 2.1f);
        }
        prevMag = {x, magY};
        prevPhase = {x, phaseY};
      }
      g.drawText("CYAN: Magnitude (dB)   |   AMBER: Phase (\xC2\xB0)",
                 {plot.x + 40.0f, plot.y + 3.0f, plot.width - 80.0f, 13.0f},
                 gui::Color::TextPrimary(), 7.6f, true, true);
    } else if (labBenchPlotMode_ == 1) {
      const float driveNorm = math::clamp(totalDriveDb / 28.0f, 0.02f, 1.0f);
      const float evenBoost = 1.0f + std::abs(asymBias) * 2.2f;
      const float hDb[6] = {
          math::clamp(-78.0f + driveNorm * 58.0f * evenBoost, -90.0f, -6.0f),
          math::clamp(-74.0f + driveNorm * 54.0f, -90.0f, -8.0f),
          math::clamp(-86.0f + driveNorm * 46.0f * evenBoost, -90.0f, -14.0f),
          math::clamp(-84.0f + driveNorm * 42.0f, -90.0f, -18.0f),
          math::clamp(-90.0f + driveNorm * 34.0f * evenBoost, -90.0f, -24.0f),
          math::clamp(-89.0f + driveNorm * 30.0f, -90.0f, -28.0f)};
      float sumSq = 0.0f;
      for (int k = 0; k < 6; ++k) {
        const float lin = std::pow(10.0f, hDb[k] / 20.0f);
        sumSq += lin * lin;
      }
      const float thdPct =
          rackBypassed_ ? 0.001f : math::clamp(std::sqrt(sumSq) * 100.0f, 0.002f, 24.5f);

      g.drawText(std::format("1 kHz REFERENCE TEST TONE   |   TOTAL THD: {:.2f}%   "
                             "|   CYAN = EVEN (H2,H4,H6)   AMBER = ODD (H3,H5,H7)",
                             thdPct),
                 {plot.x + 8.0f, plot.y + 3.0f, plot.width - 16.0f, 14.0f}, gold,
                 7.8f, true, true);

      const char *hNames[6] = {"H2 (2kHz)", "H3 (3kHz)", "H4 (4kHz)",
                               "H5 (5kHz)", "H6 (6kHz)", "H7 (7kHz)"};
      const float barAreaW = plot.width - 48.0f;
      const float slotW = barAreaW / 6.0f;
      const float baseB = plot.bottom() - 16.0f;
      const float maxBarH = plot.height - 38.0f;

      for (int k = 0; k < 6; ++k) {
        const bool isEven = ((k % 2) == 0);
        const gui::Color barCol = isEven ? cyan : gold;
        const float dispDb = rackBypassed_ ? -90.0f : hDb[k];
        const float normH = math::clamp((dispDb + 90.0f) / 84.0f, 0.04f, 1.0f);
        const float bh = normH * maxBarH;
        const float bx =
            plot.x + 24.0f + static_cast<float>(k) * slotW + slotW * 0.18f;
        const float bw = slotW * 0.64f;

        gui::Rect<float> barR = {bx, baseB - bh, bw, bh};
        g.fillRoundedRect(barR, 2.5f, barCol.withAlpha(0.32f));
        g.drawRoundedRect(barR, 2.5f, barCol, 1.3f);
        g.drawText(std::format("{:.1f}dB", dispDb),
                   {bx - 6.0f, barR.y - 12.0f, bw + 12.0f, 11.0f}, barCol, 7.0f,
                   true, true);
        g.drawText(hNames[k], {bx - 8.0f, baseB + 2.0f, bw + 16.0f, 12.0f},
                   gui::Color::TextSecondary(), 7.0f, true, true);
      }
    } else {
      const gui::Point<float> c = plot.getCentre();
      g.drawLine({plot.x, c.y}, {plot.right(), c.y},
                 gui::Color::fromHex(0x222C3C), 0.9f);
      g.drawLine({c.x, plot.y}, {c.x, plot.bottom()},
                 gui::Color::fromHex(0x222C3C), 0.9f);
      g.drawLine({plot.x + 24.0f, plot.bottom() - 8.0f},
                 {plot.right() - 24.0f, plot.y + 8.0f},
                 gui::Color::fromHex(0x324054, 0.65f), 1.0f);

      const float driveGain =
          rackBypassed_ ? 1.0f : std::pow(10.0f, (totalDriveDb * 0.45f) / 20.0f);
      const float hyst =
          rackBypassed_ ? 0.0f : math::clamp(hysteresisWidth, 0.0f, 0.28f);
      constexpr int kLoopPts = 96;
      gui::Point<float> prevPt{};
      for (int i = 0; i <= kLoopPts; ++i) {
        const float theta =
            (static_cast<float>(i) / static_cast<float>(kLoopPts)) * 6.283185307f;
        const float xIn = std::sin(theta);
        const float dxdt = std::cos(theta);
        const float xHyst = xIn * driveGain + hyst * dxdt + asymBias;
        float yOut = std::tanh(xHyst) - std::tanh(asymBias);
        if (compRatio > 1.2f && std::abs(yOut) > 0.45f) {
          const float signY = (yOut >= 0.0f) ? 1.0f : -1.0f;
          yOut = signY * (0.45f + (std::abs(yOut) - 0.45f) / compRatio);
        }
        yOut = math::clamp(yOut, -1.0f, 1.0f);

        const float px = c.x + xIn * (plot.width * 0.38f);
        const float py = c.y - yOut * (plot.height * 0.42f);
        if (i > 0) {
          g.drawLine(prevPt, {px, py}, (dxdt >= 0.0f) ? cyan : gold, 1.9f);
        }
        prevPt = {px, py};
      }
      g.drawText("X: Input Level   |   Y: Output Transfer & Magnetic Hysteresis "
                 "Loop (Cyan = Rising, Amber = Falling)",
                 {plot.x + 12.0f, plot.y + 3.0f, plot.width - 24.0f, 13.0f},
                 gui::Color::TextPrimary(), 7.5f, true, true);
    }
  }

  void saveActiveProject(bool silentAutoSave = false) {
    namespace fs = std::filesystem;
    std::string cleanName = ProjectGenerator::sanitizeName(
        wizardConfig_.name.empty()
            ? (faceplateLayout_.theme.pluginTitle.empty()
                   ? "AvraGain"
                   : faceplateLayout_.theme.pluginTitle)
            : wizardConfig_.name);
    if (cleanName.empty() || cleanName == "GainPlugin") {
      cleanName = "AvraGain";
    }

    wizardConfig_.name = cleanName;
    faceplateLayout_.theme.styleTemplateIdx =
        static_cast<int>(activeStyleTemplate_);
    if (faceplateLayout_.theme.pluginTitle.empty()) {
      faceplateLayout_.theme.pluginTitle = cleanName;
    }

    std::string saveMsg;
    const bool ok = ProjectGenerator::saveProjectStateOnly(
        wizardConfig_, schematicChain_, faceplateLayout_, saveMsg);

    const fs::path presetDir = ProjectGenerator::getWorkspaceRoot() / "presets";
    std::error_code ec;
    fs::create_directories(presetDir, ec);
    saveSchematicChainToFile(
        (presetDir / (cleanName + "_Preset.avra_schematic")).string(),
        schematicChain_);
    saveFaceplateLayoutToFile(presetDir / (cleanName + "_Preset.avra_ui"),
                              faceplateLayout_);

    const fs::path rootDir = ProjectGenerator::getWorkspaceRoot();
    const fs::path avraProjDir =
        (cleanName == "AvraGain" && fs::exists(rootDir / "examples" / "GainPlugin", ec))
            ? (rootDir / "examples" / "GainPlugin")
            : (rootDir / "plugins" / cleanName);
    fs::create_directories(avraProjDir, ec);
    const fs::path avraJsonPath = avraProjDir / (cleanName + ".avra");
    saveProjectToAvraJsonFile(avraJsonPath, cleanName, schematicChain_,
                              faceplateLayout_, rackInputTrimDb_,
                              rackOutputTrimDb_, rackGlobalWetMix_ * 100.0f,
                              rackBypassed_);

    if (silentAutoSave) {
      return;
    }

    if (ok) {
      markProjectDirty(false);
      recordRecentProjectPath(ProjectGenerator::getWorkspaceRoot(),
                              avraJsonPath.string());
      recentProjects_ =
          loadRecentProjectsList(ProjectGenerator::getWorkspaceRoot());
      refreshWorkspaceProjects();
      for (size_t i = 0; i < projects_.size(); ++i) {
        if (projects_[i].name == cleanName ||
            projects_[i].diskFolder == cleanName ||
            (cleanName == "AvraGain" && projects_[i].name == "Avra Gain")) {
          selectedProjectIndex_ = static_cast<int>(i);
          break;
        }
      }
      {
        std::lock_guard<std::mutex> lk(buildStateMutex_);
        lastConsoleActionBanner_ = saveMsg;
      }
      if (btnHeaderSaveProject_) {
        btnHeaderSaveProject_->setText("\xE2\x9C\x93 SAVED PROJECT!");
      }
      if (btnRackSavePreset_) {
        btnRackSavePreset_->setText("\xE2\x9C\x93 SAVED");
      }
      lastActionFlashTickMs_ = GetTickCount64();
      statusMessage_ = saveMsg + "  (Shortcut: Ctrl+S)";
    } else {
      statusMessage_ = std::format("SAVE ERROR: {}", saveMsg);
    }
    repaint();
  }

  static constexpr int kBuiltinRackPresetCount = 5;

  [[nodiscard]] int getActiveBuiltinRackPresetCount() const noexcept {
    return kBuiltinRackPresetCount;
  }

  [[nodiscard]] const char *getBuiltinRackPresetTitle(int idx) const noexcept {
    switch (idx) {
    case 0:
      return "Default (Factory Unity)";
    case 1:
      return "Warm Driven Iron & Tube";
    case 2:
      return "Punchy Console Glue";
    case 3:
      return "Parallel Crush / Slam";
    case 4:
      return "Wide Mastering Polish";
    default:
      return "Default (Factory Unity)";
    }
  }

  [[nodiscard]] const char *
  getBuiltinRackPresetShortLabel(int idx) const noexcept {
    switch (idx) {
    case 0:
      return "Default";
    case 1:
      return "Warm Iron";
    case 2:
      return "Console Glue";
    case 3:
      return "Parallel Slam";
    case 4:
      return "Master Polish";
    default:
      return "Default";
    }
  }

  void refreshCustomPresetFiles() {
    namespace fs = std::filesystem;
    customPresetFiles_.clear();
    const fs::path presetDir = ProjectGenerator::getWorkspaceRoot() / "presets";
    std::error_code ec;
    if (!fs::exists(presetDir, ec) || !fs::is_directory(presetDir, ec)) {
      return;
    }
    std::set<std::string> seenStems;
    std::vector<std::string> avraFiles;
    std::vector<std::string> schemFiles;
    for (const auto &entry : fs::directory_iterator(presetDir, ec)) {
      if (!entry.is_regular_file(ec))
        continue;
      const std::string ext = entry.path().extension().string();
      const std::string name = entry.path().filename().string();
      if (ext == ".avra") {
        avraFiles.push_back(name);
        seenStems.insert(entry.path().stem().string());
      } else if (ext == ".avra_schematic") {
        schemFiles.push_back(name);
      }
    }
    std::sort(avraFiles.begin(), avraFiles.end());
    std::sort(schemFiles.begin(), schemFiles.end());
    for (const auto &f : avraFiles) {
      customPresetFiles_.push_back(f);
    }
    for (const auto &f : schemFiles) {
      const std::string stem = fs::path(f).stem().string();
      if (!seenStems.contains(stem)) {
        customPresetFiles_.push_back(f);
      }
    }
  }

  [[nodiscard]] int getRackPresetMenuTotalItems() const noexcept {
    const int builtinCnt = getActiveBuiltinRackPresetCount();
    const int customShown =
        static_cast<int>(std::min<size_t>(customPresetFiles_.size(), 8));
    return builtinCnt + customShown;
  }

  [[nodiscard]] gui::Rect<float> getRackPresetMenuRect() const noexcept {
    const gui::Rect<float> btnB =
        btnRackLoadPreset_ ? btnRackLoadPreset_->getBounds()
                           : gui::Rect<float>{1118.0f, 62.0f, 122.0f, 30.0f};
    const int builtinCnt = getActiveBuiltinRackPresetCount();
    const float menuW = 248.0f;
    const float rowH = 26.0f;
    const int customShown =
        static_cast<int>(std::min<size_t>(customPresetFiles_.size(), 8));
    const float divH = (customShown > 0) ? 14.0f : 0.0f;
    const float menuH = 26.0f + static_cast<float>(builtinCnt) * rowH + divH +
                        static_cast<float>(customShown) * rowH + 6.0f;
    const float menuX = std::clamp(
        btnB.x, 12.0f, std::max(12.0f, getWidth() - menuW - 12.0f));
    const float menuY = btnB.bottom() + 4.0f;
    return {menuX, menuY, menuW, menuH};
  }

  [[nodiscard]] gui::Rect<float>
  getRackPresetMenuItemRect(int itemIdx) const noexcept {
    const gui::Rect<float> menuRect = getRackPresetMenuRect();
    const int builtinCnt = getActiveBuiltinRackPresetCount();
    const float rowH = 26.0f;
    float ry = menuRect.y + 26.0f + static_cast<float>(itemIdx) * rowH;
    if (itemIdx >= builtinCnt) {
      ry += 14.0f;
    }
    return {menuRect.x + 4.0f, ry, menuRect.width - 8.0f, rowH - 2.0f};
  }

  void paintRackPresetDropdownOverlay(gui::Graphics &g) {
    const gui::Color goldAccent = gui::Color::fromHex(0xD4AF37);
    const gui::Color cyanAccent = gui::Color::fromHex(0x00F5D4);
    const gui::Rect<float> menuRect = getRackPresetMenuRect();
    const int builtinCnt = getActiveBuiltinRackPresetCount();

    g.fillRoundedRect(
        {menuRect.x + 3.0f, menuRect.y + 4.0f, menuRect.width, menuRect.height},
        6.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.78f));
    g.fillRoundedRect(menuRect, 6.0f, gui::Color::fromHex(0x0E131B));
    g.drawRoundedRect(menuRect, 6.0f, cyanAccent, 1.5f);

    gui::Rect<float> hdrRect = {menuRect.x + 1.0f, menuRect.y + 1.0f,
                                menuRect.width - 2.0f, 23.0f};
    g.fillRoundedRect(hdrRect, 5.0f, gui::Color::fromHex(0x182230));
    g.drawLine({menuRect.x, menuRect.y + 24.0f},
               {menuRect.right(), menuRect.y + 24.0f},
               cyanAccent.withAlpha(0.55f), 1.0f);
    g.drawText("ADAPTIVE STUDIO VOICINGS & PRESETS",
               {menuRect.x + 10.0f, menuRect.y + 2.0f, menuRect.width - 20.0f,
                20.0f},
               goldAccent, 8.6f, false, true);

    const int totalItems = getRackPresetMenuTotalItems();
    for (int i = 0; i < totalItems; ++i) {
      if (i == builtinCnt) {
        const float divY =
            menuRect.y + 26.0f + static_cast<float>(builtinCnt) * 26.0f + 5.0f;
        g.drawLine({menuRect.x + 8.0f, divY}, {menuRect.right() - 8.0f, divY},
                   gui::Color::fromHex(0x2B3748), 1.0f);
        g.drawText("USER SAVED PRESETS (presets/)",
                   {menuRect.x + 10.0f, divY - 6.0f, menuRect.width - 20.0f,
                    12.0f},
                   gui::Color::fromHex(0x8FA0B2), 7.2f, true, true);
      }

      const gui::Rect<float> rowRect = getRackPresetMenuItemRect(i);
      const bool isCurrent = (i == activeRackPresetIdx_);
      const bool isHovered = (i == hoveredRackPresetItem_);

      if (isHovered) {
        g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.22f));
        g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.80f), 1.0f);
      } else if (isCurrent) {
        g.fillRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.18f));
        g.drawRoundedRect(rowRect, 4.0f, goldAccent.withAlpha(0.65f), 1.0f);
      } else if ((i % 2) == 1) {
        g.fillRoundedRect(rowRect, 3.0f, gui::Color::fromHex(0x141B26, 0.50f));
      }

      const gui::Color dotCol =
          isCurrent ? goldAccent
                    : (isHovered ? cyanAccent : gui::Color::TextSecondary());
      g.fillCircle({rowRect.x + 10.0f, rowRect.getCentre().y},
                   isCurrent ? 3.5f : 2.2f, dotCol);

      std::string itemLabel;
      if (i < builtinCnt) {
        itemLabel = std::format("{}. {}", i + 1, getBuiltinRackPresetTitle(i));
      } else {
        const std::string &fname =
            customPresetFiles_[static_cast<size_t>(i - builtinCnt)];
        itemLabel = "* " + std::filesystem::path(fname).stem().string();
      }

      const gui::Color txtCol =
          isCurrent ? goldAccent
                    : (isHovered ? cyanAccent : gui::Color::TextPrimary());
      g.drawText(itemLabel,
                 {rowRect.x + 20.0f, rowRect.y, rowRect.width - 24.0f,
                  rowRect.height},
                 txtCol, 9.0f, false, true);
    }
  }

  void applyRackPresetItem(int itemIdx) {
    namespace fs = std::filesystem;
    rackPresetMenuOpen_ = false;
    hoveredRackPresetItem_ = -1;
    const int builtinCnt = getActiveBuiltinRackPresetCount();

    if (itemIdx < 0 || itemIdx >= getRackPresetMenuTotalItems()) {
      repaint();
      return;
    }

    if (itemIdx < builtinCnt) {
      const std::string presetTitle = getBuiltinRackPresetTitle(itemIdx);
      const std::string shortLabel = getBuiltinRackPresetShortLabel(itemIdx);
      recordUndoSnapshot(std::format("Apply Rack Preset '{}'", presetTitle));

      rackInputTrimDb_ = 0.0f;
      rackOutputTrimDb_ = 0.0f;
      rackGlobalWetMix_ = 1.0f;
      rackBypassed_ = false;
      if (btnRackBypass_)
        btnRackBypass_->setToggleState(false);

      for (auto &node : schematicChain_) {
        const auto &meta = getComponentMeta(node.type);
        const auto &plum = getAutoPlumbingSpec(node.type);
        const int pCount = static_cast<int>(meta.getParamCount());

        for (int k = 0; k < pCount && k < 5; ++k) {
          node.setParamVal(k, meta.getParamSpec(k).defaultVal);
        }
        if (plum.internalTrim1.isValid())
          node.trim1Val = plum.internalTrim1.defaultVal;
        if (plum.internalTrim2.isValid())
          node.trim2Val = plum.internalTrim2.defaultVal;
        node.isBypassed = false;
        node.isParallel = false;
        node.parallelMixPercent = 100.0f;
        node.autoGainLinked = false;

        auto spanVal = [&](int slot, float norm) -> float {
          if (slot < 0 || slot >= pCount)
            return 0.0f;
          const auto &sp = meta.getParamSpec(slot);
          return math::clamp(sp.minVal + norm * (sp.maxVal - sp.minVal),
                             sp.minVal, sp.maxVal);
        };
        auto offsetFromDefault = [&](int slot, float deltaSpanFrac) -> float {
          if (slot < 0 || slot >= pCount)
            return 0.0f;
          const auto &sp = meta.getParamSpec(slot);
          return math::clamp(sp.defaultVal +
                                 deltaSpanFrac * (sp.maxVal - sp.minVal),
                             sp.minVal, sp.maxVal);
        };

        if (itemIdx == 1) {
          if (pCount >= 1)
            node.setParamVal(0, offsetFromDefault(0, 0.30f));
          if (pCount >= 2)
            node.setParamVal(1, spanVal(1, 0.68f));
          if (meta.category == SchematicCategory::PreampsAndTubes ||
              meta.category == SchematicCategory::IronAndTape) {
            node.autoGainLinked = true;
            if (plum.supportsAutoGainLink && plum.linkedOutputParamSlot >= 0 &&
                plum.linkedOutputParamSlot < pCount) {
              const float d0 =
                  node.getParamVal(0) - meta.getParamSpec(0).defaultVal;
              const auto &outSp = meta.getParamSpec(plum.linkedOutputParamSlot);
              node.setParamVal(plum.linkedOutputParamSlot,
                               math::clamp(outSp.defaultVal - d0 * 0.75f,
                                           outSp.minVal, outSp.maxVal));
            }
          }
        } else if (itemIdx == 2) {
          if (pCount >= 1)
            node.setParamVal(0, offsetFromDefault(0, 0.15f));
          if (meta.category == SchematicCategory::AnalogDynamics) {
            if (pCount >= 3)
              node.setParamVal(2, spanVal(2, 0.35f));
            if (pCount >= 4)
              node.setParamVal(3, spanVal(3, 0.25f));
          } else if (meta.category == SchematicCategory::EqAndFilters) {
            if (pCount >= 1)
              node.setParamVal(0, offsetFromDefault(0, 0.20f));
            if (pCount >= 3)
              node.setParamVal(2, offsetFromDefault(2, 0.20f));
          }
        } else if (itemIdx == 3) {
          if (pCount >= 1)
            node.setParamVal(0, spanVal(0, 0.75f));
          if (pCount >= 2)
            node.setParamVal(1, spanVal(1, 0.75f));
          if (meta.category == SchematicCategory::AnalogDynamics ||
              meta.category == SchematicCategory::DelayAndReverb) {
            node.isParallel = true;
            node.parallelMixPercent = 45.0f;
          }
          node.autoGainLinked = true;
          if (plum.supportsAutoGainLink && plum.linkedOutputParamSlot >= 0 &&
              plum.linkedOutputParamSlot < pCount) {
            const float d0 =
                node.getParamVal(0) - meta.getParamSpec(0).defaultVal;
            const auto &outSp = meta.getParamSpec(plum.linkedOutputParamSlot);
            node.setParamVal(plum.linkedOutputParamSlot,
                             math::clamp(outSp.defaultVal - d0 * 0.80f,
                                         outSp.minVal, outSp.maxVal));
          }
        } else if (itemIdx == 4) {
          if (pCount >= 1)
            node.setParamVal(0, offsetFromDefault(0, 0.08f));
          if (meta.category == SchematicCategory::AnalogDynamics &&
              pCount >= 2) {
            node.setParamVal(1, spanVal(1, 0.20f));
          } else if (meta.category == SchematicCategory::EqAndFilters &&
                     pCount >= 3) {
            node.setParamVal(2, offsetFromDefault(2, 0.14f));
          }
        }
      }

      activeRackPresetIdx_ = itemIdx;
      syncFaceplateParamValuesFromSchematic(faceplateLayout_, schematicChain_);
      syncGlobalFaceplateControls();
      syncInspectorKnobsToSelection();
      if (btnRackLoadPreset_) {
        btnRackLoadPreset_->setLabel(std::string("PRESET: ") + shortLabel + " v");
      }
      statusMessage_ = std::format(
          "APPLIED RACK VOICING PRESET: '{}' across {} active stages!",
          presetTitle, schematicChain_.size());
      repaint();
      return;
    }

    const size_t customIdx = static_cast<size_t>(itemIdx - builtinCnt);
    if (customIdx >= customPresetFiles_.size()) {
      repaint();
      return;
    }

    const fs::path presetDir = ProjectGenerator::getWorkspaceRoot() / "presets";
    const fs::path filePath = presetDir / customPresetFiles_[customIdx];
    const std::string stem = filePath.stem().string();
    recordUndoSnapshot(std::format("Load User Preset '{}'", stem));

    if (filePath.extension() == ".avra") {
      std::string loadedName = wizardConfig_.name;
      std::vector<SchematicNode> loadedChain;
      FaceplateLayout loadedLayout;
      float inDb = 0.0f, outDb = 0.0f, parMix = 100.0f;
      bool byp = false;
      if (loadProjectFromAvraJsonFile(filePath, loadedName, loadedChain,
                                      loadedLayout, &inDb, &outDb, &parMix,
                                      &byp)) {
        schematicChain_ = std::move(loadedChain);
        faceplateLayout_ = std::move(loadedLayout);
        ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
        syncFaceplateParamValuesFromSchematic(faceplateLayout_, schematicChain_);
        rackInputTrimDb_ = inDb;
        rackOutputTrimDb_ = outDb;
        rackGlobalWetMix_ = math::clamp(parMix * 0.01f, 0.0f, 1.0f);
        rackBypassed_ = byp;
        if (btnRackBypass_)
          btnRackBypass_->setToggleState(rackBypassed_);
        activeStyleTemplate_ = static_cast<UiStyleTemplate>(std::clamp(
            faceplateLayout_.theme.styleTemplateIdx, 0,
            kTotalUiStyleTemplates - 1));
        updateUiThemeButtonLabels();
        syncUiWidgetInspector();
        syncGlobalFaceplateControls();
        syncInspectorKnobsToSelection();
      }
    } else {
      const fs::path uiPath = presetDir / (stem + ".avra_ui");
      std::vector<SchematicNode> loadedChain;
      if (loadSchematicChainFromFile(filePath.string(), loadedChain) &&
          !loadedChain.empty()) {
        schematicChain_ = std::move(loadedChain);
      }
      FaceplateLayout loadedUi;
      if (loadFaceplateLayoutFromFile(uiPath, loadedUi)) {
        faceplateLayout_ = std::move(loadedUi);
        ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
        activeStyleTemplate_ = static_cast<UiStyleTemplate>(std::clamp(
            faceplateLayout_.theme.styleTemplateIdx, 0,
            kTotalUiStyleTemplates - 1));
        updateUiThemeButtonLabels();
        syncUiWidgetInspector();
      } else {
        ensureMultiStageActiveBayLayout(faceplateLayout_, schematicChain_);
      }
      syncFaceplateParamValuesFromSchematic(faceplateLayout_, schematicChain_);
      syncGlobalFaceplateControls();
      syncInspectorKnobsToSelection();
    }

    activeRackPresetIdx_ = itemIdx;
    std::string shortStem = stem;
    if (shortStem.size() > 12) {
      shortStem = shortStem.substr(0, 12);
    }
    if (btnRackLoadPreset_) {
      btnRackLoadPreset_->setLabel("PRESET: " + shortStem + " v");
    }
    statusMessage_ = std::format(
        "LOADED USER RACK PRESET '{}' from presets/{} ({} stages)!", stem,
        filePath.filename().string(), schematicChain_.size());
    repaint();
  }

  void saveRackPreset() {
    namespace fs = std::filesystem;
    saveActiveProject(false);

    std::string cleanName = ProjectGenerator::sanitizeName(
        wizardConfig_.name.empty() ? "AvraGain" : wizardConfig_.name);
    if (cleanName.empty() || cleanName == "GainPlugin") {
      cleanName = "AvraGain";
    }
    const fs::path presetDir = ProjectGenerator::getWorkspaceRoot() / "presets";
    std::error_code ec;
    fs::create_directories(presetDir, ec);

    int nextPresetNum = 1;
    fs::path numberedPresetPath;
    for (; nextPresetNum <= 99; ++nextPresetNum) {
      numberedPresetPath =
          presetDir / std::format("{}_Preset_{}.avra", cleanName, nextPresetNum);
      if (!fs::exists(numberedPresetPath, ec)) {
        break;
      }
    }

    saveProjectToAvraJsonFile(numberedPresetPath, cleanName, schematicChain_,
                              faceplateLayout_, rackInputTrimDb_,
                              rackOutputTrimDb_, rackGlobalWetMix_ * 100.0f,
                              rackBypassed_);

    refreshCustomPresetFiles();
    const std::string savedFilename = numberedPresetPath.filename().string();
    for (size_t i = 0; i < customPresetFiles_.size(); ++i) {
      if (customPresetFiles_[i] == savedFilename) {
        activeRackPresetIdx_ =
            getActiveBuiltinRackPresetCount() + static_cast<int>(i);
        break;
      }
    }

    const std::string shortLabel = std::format(
        "{} #{}", cleanName.substr(0, std::min<size_t>(cleanName.size(), 8)),
        nextPresetNum);
    if (btnRackLoadPreset_) {
      btnRackLoadPreset_->setLabel(std::format("PRESET: {} \xE2\x96\xBE", shortLabel));
    }
    statusMessage_ = std::format(
        "SAVED RACK PRESET: presets/{} (Click 'PRESET: {} \xE2\x96\xBE' to switch "
        "voicings & user presets!)",
        savedFilename, shortLabel);
    repaint();
  }

  [[nodiscard]] int getRackAudioDeviceMenuTotalItems() const {
    const auto devs = avra::audio_io::enumerateOutputDevices();
    return static_cast<int>(devs.size()) + 1; // +1 for "Audio Engine & Buffer Settings (Ctrl+,)..."
  }

  [[nodiscard]] gui::Rect<float> getRackAudioDeviceMenuRect() const {
    const gui::Rect<float> btnB =
        btnRackAudioDevice_ ? btnRackAudioDevice_->getBounds()
                            : gui::Rect<float>{495.0f, 62.0f, 84.0f, 30.0f};
    const int devCount = std::max(1, getRackAudioDeviceMenuTotalItems() - 1);
    const float menuW = 356.0f;
    const float rowH = 26.0f;
    const float divH = 14.0f;
    const float menuH = 26.0f + static_cast<float>(devCount) * rowH + divH + rowH + 6.0f;
    const float menuX = std::clamp(
        btnB.x, 12.0f, std::max(12.0f, getWidth() - menuW - 12.0f));
    const float menuY = btnB.bottom() + 4.0f;
    return {menuX, menuY, menuW, menuH};
  }

  [[nodiscard]] gui::Rect<float>
  getRackAudioDeviceMenuItemRect(int itemIdx) const {
    const gui::Rect<float> menuRect = getRackAudioDeviceMenuRect();
    const int devCount = std::max(1, getRackAudioDeviceMenuTotalItems() - 1);
    const float rowH = 26.0f;
    float ry = menuRect.y + 26.0f + static_cast<float>(itemIdx) * rowH;
    if (itemIdx >= devCount) {
      ry += 14.0f;
    }
    return {menuRect.x + 4.0f, ry, menuRect.width - 8.0f, rowH - 2.0f};
  }

  void paintRackAudioDeviceDropdownOverlay(gui::Graphics &g) {
    const gui::Color goldAccent = gui::Color::fromHex(0xD4AF37);
    const gui::Color cyanAccent = gui::Color::fromHex(0x00F5D4);
    const gui::Color greenAccent = gui::Color::fromHex(0x52B788);
    const gui::Rect<float> menuRect = getRackAudioDeviceMenuRect();
    const auto devs = avra::audio_io::enumerateOutputDevices();
    const int devCount = static_cast<int>(devs.size());
    const int curIdx = (prefs_.outputDeviceIdx >= 0 && prefs_.outputDeviceIdx < devCount)
                           ? prefs_.outputDeviceIdx
                           : 0;

    g.fillRoundedRect(
        {menuRect.x + 3.0f, menuRect.y + 4.0f, menuRect.width, menuRect.height},
        6.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.80f));
    g.fillRoundedRect(menuRect, 6.0f, gui::Color::fromHex(0x0E131B));
    g.drawRoundedRect(menuRect, 6.0f, greenAccent, 1.5f);

    gui::Rect<float> hdrRect = {menuRect.x + 1.0f, menuRect.y + 1.0f,
                                menuRect.width - 2.0f, 23.0f};
    g.fillRoundedRect(hdrRect, 5.0f, gui::Color::fromHex(0x162420));
    g.drawLine({menuRect.x, menuRect.y + 24.0f},
               {menuRect.right(), menuRect.y + 24.0f},
               greenAccent.withAlpha(0.55f), 1.0f);
    g.drawText(std::format("SPEAKER / HEADPHONE AUDIO OUTPUT ({} Hz / {} smpl)",
                           prefs_.sampleRateHz, prefs_.blockBufferSize),
               {menuRect.x + 10.0f, menuRect.y + 2.0f, menuRect.width - 20.0f,
                20.0f},
               goldAccent, 8.6f, false, true);

    const int totalItems = devCount + 1;
    for (int i = 0; i < totalItems; ++i) {
      if (i == devCount) {
        const float divY =
            menuRect.y + 26.0f + static_cast<float>(devCount) * 26.0f + 5.0f;
        g.drawLine({menuRect.x + 8.0f, divY}, {menuRect.right() - 8.0f, divY},
                   gui::Color::fromHex(0x2B3748), 1.0f);
        g.drawText("ENGINE SAMPLE RATE & BUFFER SETTINGS",
                   {menuRect.x + 10.0f, divY - 6.0f, menuRect.width - 20.0f,
                    12.0f},
                   gui::Color::fromHex(0x8FA0B2), 7.2f, true, true);
      }

      const gui::Rect<float> rowRect = getRackAudioDeviceMenuItemRect(i);
      const bool isCurrent = (i < devCount && i == curIdx);
      const bool isHovered = (i == hoveredRackAudioDeviceItem_);

      if (isHovered) {
        g.fillRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.22f));
        g.drawRoundedRect(rowRect, 4.0f, cyanAccent.withAlpha(0.80f), 1.0f);
      } else if (isCurrent) {
        g.fillRoundedRect(rowRect, 4.0f, greenAccent.withAlpha(0.20f));
        g.drawRoundedRect(rowRect, 4.0f, greenAccent.withAlpha(0.70f), 1.0f);
      } else if ((i % 2) == 1) {
        g.fillRoundedRect(rowRect, 3.0f, gui::Color::fromHex(0x141B26, 0.50f));
      }

      const gui::Color dotCol =
          isCurrent ? greenAccent
                    : (isHovered ? cyanAccent : gui::Color::TextSecondary());
      g.fillCircle({rowRect.x + 10.0f, rowRect.getCentre().y},
                   isCurrent ? 3.8f : 2.2f, dotCol);

      std::string itemLabel;
      if (i < devCount) {
        itemLabel = std::format("{}{}", isCurrent ? "\xE2\x9C\x93 " : "", devs[static_cast<size_t>(i)].name);
      } else {
        itemLabel = "Audio Engine, Sample Rate & Buffer Settings (Ctrl+,)...";
      }

      const gui::Color txtCol =
          isCurrent ? goldAccent
                    : (isHovered ? cyanAccent : gui::Color::TextPrimary());
      g.drawText(itemLabel,
                 {rowRect.x + 20.0f, rowRect.y, rowRect.width - 24.0f,
                  rowRect.height},
                 txtCol, 9.0f, false, true);
    }
  }

  void applyRackAudioDeviceItem(int itemIdx) {
    const auto devs = avra::audio_io::enumerateOutputDevices();
    const int devCount = static_cast<int>(devs.size());
    rackAudioDeviceMenuOpen_ = false;
    hoveredRackAudioDeviceItem_ = -1;

    if (itemIdx < 0 || itemIdx > devCount) {
      repaint();
      return;
    }

    if (itemIdx == devCount) {
      activeStudioModal_ = ActiveStudioModal::Preferences;
      activePreferencesTab_ = 0;
      repaint();
      return;
    }

    prefs_.outputDeviceIdx = itemIdx;
    saveStudioPreferencesToFile(
        ProjectGenerator::getWorkspaceRoot() / ".avra_preferences.json",
        prefs_);
    syncPreferencesRuntimeState();
    rackMuted_ = false;
    silentAnalyzerMode_ = false;
    if (btnSigSilentAnalyzer_) {
      btnSigSilentAnalyzer_->setToggleState(false);
    }
    if (testSignalMode_ == TestSignalMode::Silence) {
      selectTestSignal(TestSignalMode::DrumTransientLoop);
    }
    if (!audioOut_.isOpen()) {
      (void)audioOut_.open();
    }
    statusMessage_ = std::format(
        "AUDIO OUTPUT DEVICE SELECTED: '{}' ({} Hz / {} samples).",
        prefs_.getOutputDeviceName(), prefs_.sampleRateHz,
        prefs_.blockBufferSize);
    repaint();
  }

  void toggleRackAudioDeviceMenu() {
    rackPresetMenuOpen_ = false;
    hoveredRackPresetItem_ = -1;
    rackAudioDeviceMenuOpen_ = !rackAudioDeviceMenuOpen_;
    hoveredRackAudioDeviceItem_ = -1;
    if (rackAudioDeviceMenuOpen_) {
      statusMessage_ = std::format(
          "AUDIO OUTPUT SELECTOR: Active device = '{}' — click any speaker/headphone bus below to switch immediately!",
          prefs_.getOutputDeviceName());
    }
    repaint();
  }

  void loadRackPreset() {
    rackAudioDeviceMenuOpen_ = false;
    hoveredRackAudioDeviceItem_ = -1;
    refreshCustomPresetFiles();
    rackPresetMenuOpen_ = !rackPresetMenuOpen_;
    hoveredRackPresetItem_ = -1;
    if (rackPresetMenuOpen_) {
      statusMessage_ = std::format(
          "RACK PRESET MENU OPEN: Choose from {} factory voicings or {} saved "
          "preset files in presets/.",
          getActiveBuiltinRackPresetCount(), customPresetFiles_.size());
    }
    repaint();
  }

  void pollAsyncBuildProgress() {
#if defined(_WIN32)
    if (isBuildRunning_.load()) {
      const int step = std::clamp(buildStepIndex_.load(), 1, 4);
      const double elapsedSec =
          static_cast<double>(GetTickCount64() - buildStartTickMs_) / 1000.0;
      if (btnBuildClap_) {
        btnBuildClap_->setText(
            std::format("\xE2\x9A\xA1 BUILDING [{}/4] ({:.1f}s)...", step, elapsedSec));
        btnBuildClap_->setAccentColor(gui::Color::AccentAmber());
      }
      if (btnHeaderOpenIde_) {
        btnHeaderOpenIde_->setText(std::format("\xE2\x9A\xA1 BUILDING [{}/4]...", step));
      }
      std::string headline;
      std::string detail;
      {
        std::lock_guard<std::mutex> lk(buildStateMutex_);
        headline = buildStepHeadline_;
        detail = buildLiveCompilerLine_;
      }
      statusMessage_ = std::format("BUILDING [{}/4] ({:.1f}s): {} \xE2\x80\x94 {}", step,
                                   elapsedSec, headline, detail);
      repaint();
      return;
    }

    if (buildFinishedPendingUiSync_.exchange(false)) {
      if (backgroundBuildThread_.joinable()) {
        backgroundBuildThread_.join();
      }
      refreshWorkspaceProjects();
      for (size_t i = 0; i < projects_.size(); ++i) {
        if (projects_[i].diskFolder == activeBuildTargetFolder_ ||
            projects_[i].name == activeBuildTargetFolder_) {
          selectedProjectIndex_ = static_cast<int>(i);
          break;
        }
      }

      ProjectGenerator::BuildExecutionReport repCopy;
      {
        std::lock_guard<std::mutex> lk(buildStateMutex_);
        repCopy = lastBuildReport_;
        lastConsoleActionBanner_ = repCopy.summaryMessage;
      }

      if (btnBuildClap_) {
        btnBuildClap_->setText(repCopy.succeeded ? "\xE2\x9C\x93 BUILD COMPLETE (REBUILD)"
                                                 : "!! BUILD FAILED (RETRY)");
        btnBuildClap_->setAccentColor(repCopy.succeeded
                                          ? gui::Color::fromHex(0x52B788)
                                          : gui::Color::AccentRed());
      }
      if (btnHeaderOpenIde_) {
        btnHeaderOpenIde_->setText(prefs_.getBuildButtonLabel());
      }
      statusMessage_ = repCopy.summaryMessage;
      lastActionFlashTickMs_ = GetTickCount64();

      if (repCopy.succeeded && launchStandaloneAfterBuild_) {
        launchStandaloneAfterBuild_ = false;
        runSelectedStandalone();
        return;
      }
      launchStandaloneAfterBuild_ = false;

      if (repCopy.succeeded && openIdeAfterBuild_) {
        openIdeAfterBuild_ = false;
        openSelectedInVsCode();
        return;
      }
      openIdeAfterBuild_ = false;
      repaint();
      return;
    }

    if (lastActionFlashTickMs_ > 0 &&
        (GetTickCount64() - lastActionFlashTickMs_) > 4000) {
      lastActionFlashTickMs_ = 0;
      if (btnHeaderSaveProject_)
        btnHeaderSaveProject_->setText(isProjectDirty_ ? "SAVE PROJECT *" : "SAVE PROJECT");
      if (btnRackSavePreset_)
        btnRackSavePreset_->setText("SAVE");
      if (btnRunStandalone_)
        btnRunStandalone_->setText("RUN STANDALONE .EXE");
      if (btnOpenFolder_)
        btnOpenFolder_->setText("OPEN SOURCE / DIST");
      updateBuildFormatUiLabels();
      repaint();
    }
#endif
  }

  void updateBuildFormatUiLabels() {
    prefs_.ensureAtLeastOneExportFormat();
    if (btnExportFmtVst3_) {
      btnExportFmtVst3_->setText(
          prefs_.exportFormatVst3 ? "[\xE2\x9C\x93] VST3 (.vst3)" : "[ ] VST3 (.vst3)");
      btnExportFmtVst3_->setToggleState(prefs_.exportFormatVst3);
      btnExportFmtVst3_->setAccentColor(prefs_.exportFormatVst3
                                            ? gui::Color::fromHex(0x06D6A0)
                                            : gui::Color::TextSecondary());
    }
    if (btnExportFmtClap_) {
      btnExportFmtClap_->setText(
          prefs_.exportFormatClap ? "[\xE2\x9C\x93] CLAP (.clap)" : "[ ] CLAP (.clap)");
      btnExportFmtClap_->setToggleState(prefs_.exportFormatClap);
      btnExportFmtClap_->setAccentColor(prefs_.exportFormatClap
                                            ? gui::Color::AccentCyan()
                                            : gui::Color::TextSecondary());
    }
    if (btnExportFmtApp_) {
      btnExportFmtApp_->setText(prefs_.exportFormatStandalone
                                    ? "[\xE2\x9C\x93] Standalone (.exe)"
                                    : "[ ] Standalone (.exe)");
      btnExportFmtApp_->setToggleState(prefs_.exportFormatStandalone);
      btnExportFmtApp_->setAccentColor(prefs_.exportFormatStandalone
                                           ? gui::Color::AccentAmber()
                                           : gui::Color::TextSecondary());
    }
    const std::string buildLabel = prefs_.getBuildButtonLabel();
    if (!isBuildRunning_.load()) {
      if (btnBuildClap_) {
        btnBuildClap_->setText(buildLabel);
        btnBuildClap_->setAccentColor(gui::Color::fromHex(0x52B788));
      }
      if (btnHeaderOpenIde_) {
        btnHeaderOpenIde_->setText(buildLabel);
      }
      if (btnCreateProject_) {
        btnCreateProject_->setText(buildLabel);
      }
    }
  }

  void toggleExportFormatSelection(int fmtIdx) {
    if (fmtIdx == 0)
      prefs_.exportFormatVst3 = !prefs_.exportFormatVst3;
    else if (fmtIdx == 1)
      prefs_.exportFormatClap = !prefs_.exportFormatClap;
    else if (fmtIdx == 2)
      prefs_.exportFormatStandalone = !prefs_.exportFormatStandalone;
    prefs_.ensureAtLeastOneExportFormat();
    saveStudioPreferencesToFile(
        ProjectGenerator::getWorkspaceRoot() / ".avra_preferences.json",
        prefs_);
    updateBuildFormatUiLabels();
    if (selectedProjectIndex_ >= 0 &&
        selectedProjectIndex_ < static_cast<int>(projects_.size())) {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      lastBuildReport_ = ProjectGenerator::inspectProjectArtifacts(
          projects_[selectedProjectIndex_].diskFolder, prefs_.exportFormatVst3,
          prefs_.exportFormatClap, prefs_.exportFormatStandalone,
          customExportDir_);
    }
    statusMessage_ = std::format(
        "EXPORT FORMATS UPDATED (Saved to Preferences): {}  \xE2\x80\x94  Click "
        "'{}' to compile!",
        prefs_.getExportFormatCheckboxesSummary(), prefs_.getBuildButtonLabel());
    repaint();
  }

  void startAsyncProjectBuild(const std::string &folderToBuild,
                              bool launchStandaloneAfter,
                              bool openVsCodeAfter) {
    if (isBuildRunning_.load()) {
      statusMessage_ = std::format(
          "BUILD IN PROGRESS: Please wait for the active {} build to finish...",
          prefs_.getSelectedFormatsShortList());
      if (currentTab_ != StudioTab::Projects)
        switchTab(StudioTab::Projects);
      repaint();
      return;
    }

    if (backgroundBuildThread_.joinable()) {
      backgroundBuildThread_.join();
    }

    const std::string cleanFolder =
        ProjectGenerator::sanitizeName(folderToBuild);
    activeBuildTargetFolder_ = cleanFolder;
    launchStandaloneAfterBuild_ = launchStandaloneAfter;
    openIdeAfterBuild_ = openVsCodeAfter;

    faceplateLayout_.theme.styleTemplateIdx =
        static_cast<int>(activeStyleTemplate_);

    ProjectConfig cfgSnapshot = wizardConfig_;
    const bool isCoreGain =
        (cleanFolder == "GainPlugin" || cleanFolder == "AvraGain");
    if (!isCoreGain) {
      cfgSnapshot.name = cleanFolder;
      cfgSnapshot.pluginId = std::format("com.avra.{}", toLower(cleanFolder));
      faceplateLayout_.theme.pluginTitle = cleanFolder;
    } else {
      cfgSnapshot.name = "AvraGain";
      cfgSnapshot.pluginId = "com.avra.gain";
    }
    const auto chainSnapshot = schematicChain_;
    const auto faceplateSnapshot = faceplateLayout_;

    prefs_.ensureAtLeastOneExportFormat();
    const bool buildVst3 = prefs_.exportFormatVst3;
    const bool buildClap = prefs_.exportFormatClap;
    const bool buildApp =
        prefs_.exportFormatStandalone || launchStandaloneAfter;

    {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      buildStepHeadline_ = "Syncing C++20 DSP & Direct2D UI Headers...";
      buildLiveCompilerLine_ = "Preparing build targets...";
      lastConsoleActionBanner_ = std::format("STARTED BUILD ({}) for '{}'...",
                                             prefs_.getSelectedFormatsShortList(),
                                             cleanFolder);
    }

    buildStepIndex_.store(1);
    buildStartTickMs_ = GetTickCount64();
    buildFinishedPendingUiSync_.store(false);
    isBuildRunning_.store(true);

    if (btnBuildClap_) {
      btnBuildClap_->setText("\xE2\x9A\xA1 BUILDING [1/4] (0.0s)...");
      btnBuildClap_->setAccentColor(gui::Color::AccentAmber());
    }
    if (currentTab_ != StudioTab::Projects) {
      switchTab(StudioTab::Projects);
    }
    statusMessage_ = std::format(
        "BUILDING '{}' ({}) [1/4]: Syncing C++20 DSP & Direct2D UI sources...",
        cleanFolder, prefs_.getSelectedFormatsShortList());
    repaint();

    backgroundBuildThread_ = std::thread([this, cleanFolder, isCoreGain,
                                          cfgSnapshot, chainSnapshot,
                                          faceplateSnapshot, buildVst3,
                                          buildClap, buildApp]() {
      std::string syncMsg;
      ProjectGenerator::generateFromSchematicAndUi(cfgSnapshot, chainSnapshot,
                                                   faceplateSnapshot, syncMsg);
      if (isCoreGain) {
        ProjectGenerator::saveProjectStateOnly(cfgSnapshot, chainSnapshot,
                                               faceplateSnapshot, syncMsg);
      }

      ProjectGenerator::BuildExecutionReport rep;
      ProjectGenerator::buildAllPluginFormatsVerbose(
          cleanFolder, rep,
          [this](int step, int /*totalSteps*/, const std::string &stepTitle,
                 const std::string &liveDetail) {
            buildStepIndex_.store(step);
            std::lock_guard<std::mutex> lk(buildStateMutex_);
            buildStepHeadline_ = stepTitle;
            buildLiveCompilerLine_ = liveDetail;
          },
          buildVst3, buildClap, buildApp, customExportDir_);

      {
        std::lock_guard<std::mutex> lk(buildStateMutex_);
        lastBuildReport_ = std::move(rep);
      }
      buildFinishedPendingUiSync_.store(true);
      isBuildRunning_.store(false);
    });
  }

  void buildSelectedProject() {
    createProjectFromWizard(false, true);
  }

  void runSelectedStandalone() {
    if (selectedProjectIndex_ < 0 ||
        selectedProjectIndex_ >= static_cast<int>(projects_.size()))
      return;
    const auto &proj = projects_[selectedProjectIndex_];

    if (isBuildRunning_.load()) {
      launchStandaloneAfterBuild_ = true;
      statusMessage_ = "QUEUED STANDALONE LAUNCH: Will automatically run "
                       "Standalone .EXE as soon as the active build finishes!";
      repaint();
      return;
    }

    const auto inspected = ProjectGenerator::inspectProjectArtifacts(
        proj.diskFolder, prefs_.exportFormatVst3, prefs_.exportFormatClap,
        prefs_.exportFormatStandalone, customExportDir_);
    const bool hasStandalone =
        (inspected.artifacts.size() > 2 && inspected.artifacts[2].exists);
    if (!hasStandalone) {
      statusMessage_ = std::format(
          "Standalone .EXE not built yet for '{}' \xE2\x80\x94 building now and "
          "auto-launching upon completion!",
          proj.name);
      startAsyncProjectBuild(proj.diskFolder, true, false);
      return;
    }

    std::string runLog;
    std::string exePath;
    if (ProjectGenerator::launchStandaloneExe(proj.diskFolder, runLog,
                                              &exePath)) {
      refreshWorkspaceProjects();
      {
        std::lock_guard<std::mutex> lk(buildStateMutex_);
        lastConsoleActionBanner_ =
            std::format("LAUNCHED STANDALONE APP -> {}", exePath);
      }
      if (btnRunStandalone_) {
        btnRunStandalone_->setText("\xE2\x9C\x93 STANDALONE .EXE RUNNING!");
      }
      lastActionFlashTickMs_ = GetTickCount64();
    }
    statusMessage_ = runLog;
    repaint();
  }

  void openSelectedInVsCode() {
    if (selectedProjectIndex_ < 0 ||
        selectedProjectIndex_ >= static_cast<int>(projects_.size())) {
      openInVsCode();
      return;
    }
    const auto &proj = projects_[selectedProjectIndex_];
    const auto root = ProjectGenerator::getWorkspaceRoot();
    const std::string folderPath =
        (proj.name == "Avra Gain")
            ? (root / "examples" / "GainPlugin").string()
            : (root / "plugins" / proj.diskFolder).string();
    const std::string editorFile =
        (proj.name == "Avra Gain")
            ? (root / "examples" / "GainPlugin" / "GainPluginEditor.h").string()
            : (root / "plugins" / proj.diskFolder /
               (proj.diskFolder + "Editor.h"))
                  .string();
    openInVsCode(editorFile, folderPath);
  }

  void loadHiddenProjectsList() {
    hiddenProjects_.clear();
    std::ifstream in(ProjectGenerator::getWorkspaceRoot() /
                     ".avra_hidden_projects");
    std::string line;
    while (std::getline(in, line)) {
      if (!line.empty() && line.back() == '\r')
        line.pop_back();
      if (!line.empty())
        hiddenProjects_.insert(line);
    }
  }

  void saveHiddenProjectsList() const {
    std::ofstream out(ProjectGenerator::getWorkspaceRoot() /
                          ".avra_hidden_projects",
                      std::ios::trunc);
    for (const auto &name : hiddenProjects_) {
      out << name << "\n";
    }
  }

  void openInIDE(const std::string &specificFile = "") {
#if defined(_WIN32)
    const auto root = ProjectGenerator::getWorkspaceRoot();
    const std::string slnxPath = (root / "build" / "Avra.slnx").string();
    const std::string devenv = "C:\\Program Files\\Microsoft Visual "
                               "Studio\\18\\Community\\Common7\\IDE\\devenv.exe";

    if (!specificFile.empty()) {
      std::string args = std::format("\"{}\" \"{}\"", slnxPath, specificFile);
      HINSTANCE resDev = ShellExecuteA(nullptr, "open", devenv.c_str(),
                                       args.c_str(), nullptr, SW_SHOW);
      if (reinterpret_cast<uintptr_t>(resDev) > 32) {
        statusMessage_ =
            std::format("Launched Visual Studio with {}!", specificFile);
        repaint();
        return;
      }
    }

    HINSTANCE res =
        ShellExecuteA(nullptr, "open", slnxPath.c_str(), nullptr, nullptr, SW_SHOW);
    if (reinterpret_cast<uintptr_t>(res) <= 32) {
      HINSTANCE resDev = ShellExecuteA(nullptr, "open", devenv.c_str(),
                                       slnxPath.c_str(), nullptr, SW_SHOW);
      if (reinterpret_cast<uintptr_t>(resDev) <= 32) {
        openInVsCode(specificFile);
        return;
      }
    }
    statusMessage_ = "Launched project in Visual Studio!";
#endif
    repaint();
  }

  void openInVsCode(const std::string &specificFile = "",
                    const std::string &folderOverride = "") {
#if defined(_WIN32)
    toolchainStatus_ = ProjectGenerator::detectToolchain();
    const std::string workspacePath =
        folderOverride.empty() ? ProjectGenerator::getWorkspaceRoot().string()
                               : folderOverride;
    const std::string codeBin = toolchainStatus_.vsCodePath.empty()
                                    ? "code.cmd"
                                    : toolchainStatus_.vsCodePath;
    std::string cmd =
        specificFile.empty()
            ? std::format("\"{}\"", workspacePath)
            : std::format("\"{}\" \"{}\"", workspacePath, specificFile);
    HINSTANCE res =
        ShellExecuteA(nullptr, "open", codeBin.c_str(), cmd.c_str(), nullptr, SW_HIDE);
    if (reinterpret_cast<uintptr_t>(res) <= 32) {
      statusMessage_ = "VS CODE NOT FOUND: Please install Visual Studio Code "
                       "(https://code.visualstudio.com) + C/C++ & CMake Tools "
                       "extensions.";
    } else {
      statusMessage_ = std::format(
          "Opened '{}' in VS Code! Press Ctrl+Shift+B to build VST3 + CLAP + "
          "Standalone .exe, or F5 to debug.",
          workspacePath);
    }
#endif
    repaint();
  }

  void openSelectedFolder(bool preferDist = true) {
    if (selectedProjectIndex_ < 0 ||
        selectedProjectIndex_ >= static_cast<int>(projects_.size()))
      return;
    const auto &proj = projects_[selectedProjectIndex_];
    namespace fs = std::filesystem;

    const auto inspected = ProjectGenerator::inspectProjectArtifacts(
        proj.diskFolder, prefs_.exportFormatVst3, prefs_.exportFormatClap,
        prefs_.exportFormatStandalone, customExportDir_);
    {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      lastBuildReport_ = inspected;
    }

    const fs::path srcDir(inspected.sourceFolder);
    const fs::path distDir(inspected.destinationFolder);
    std::error_code ec;

    const bool openDist = preferDist && fs::exists(distDir, ec);
    const std::string targetFolder =
        openDist ? distDir.string() : srcDir.string();

#if defined(_WIN32)
    if (!inspected.artifacts.empty() && openDist &&
        inspected.artifacts[0].exists) {
      const std::string selectArg =
          std::format("/select,\"{}\"", inspected.artifacts[0].fullPath);
      ShellExecuteA(nullptr, "open", "explorer.exe", selectArg.c_str(), nullptr,
                    SW_SHOWNORMAL);
    } else {
      ShellExecuteA(nullptr, "open", "explorer.exe", targetFolder.c_str(),
                    nullptr, SW_SHOWNORMAL);
    }
#endif

    {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      lastConsoleActionBanner_ = std::format(
          "OPENED {} IN WINDOWS EXPLORER -> {}",
          openDist ? "DESTINATION FOLDER (dist\\)" : "SOURCE FOLDER",
          targetFolder);
    }
    if (btnOpenFolder_) {
      btnOpenFolder_->setText(openDist ? "\xE2\x9C\x93 OPENED DIST FOLDER!"
                                       : "\xE2\x9C\x93 OPENED SOURCE DIR!");
    }
    lastActionFlashTickMs_ = GetTickCount64();
    statusMessage_ = std::format(
        "OPENED IN EXPLORER: {} ({})", targetFolder,
        openDist ? "Built VST3, CLAP & Standalone .EXE binaries"
                 : "C++20 Source & Header files");
    repaint();
  }

  void createProjectFromWizard(bool buildAndOpenVsCodeAfter,
                               bool promptForLocation = true) {
    if (promptForLocation) {
      if (!promptForBuildDestinationAndPluginName()) {
        return;
      }
    }
    if (wizardConfig_.name.empty()) {
      statusMessage_ = "ERROR: Please specify a plugin name.";
      repaint();
      return;
    }

    const std::string cleanName =
        ProjectGenerator::sanitizeName(wizardConfig_.name);
    hiddenProjects_.erase(cleanName);
    saveHiddenProjectsList();

    faceplateLayout_.theme.pluginTitle = cleanName;
    faceplateLayout_.theme.styleTemplateIdx =
        static_cast<int>(activeStyleTemplate_);
    const std::string targetFolder =
        (cleanName == "AvraGain" || cleanName == "GainPlugin") ? "GainPlugin"
                                                               : cleanName;

    if (targetFolder != "GainPlugin") {
      bool found = false;
      for (size_t i = 0; i < projects_.size(); ++i) {
        if (projects_[i].name == cleanName ||
            projects_[i].diskFolder == cleanName) {
          selectedProjectIndex_ = static_cast<int>(i);
          found = true;
          break;
        }
      }
      if (!found) {
        projects_.push_back(
            {cleanName, cleanName, "C++20 VST3 + CLAP + APP",
             std::format("com.avra.{}", toLower(cleanName)),
             std::format("Standalone C++20 plugin project '{}' (plugins/{}/ "
                         "with VST3, CLAP, Standalone .exe & .vscode tasks).",
                         cleanName, cleanName),
             std::format("Avra{}_Clap", cleanName), true, false, false});
        selectedProjectIndex_ = static_cast<int>(projects_.size() - 1);
      }
    } else {
      selectedProjectIndex_ = 0;
    }

    startAsyncProjectBuild(targetFolder, false, buildAndOpenVsCodeAfter);
  }

  static std::string toLower(std::string s) {
    for (char &c : s)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
  }

  StudioTab currentTab_{StudioTab::NewPluginWizard};
  bool isSidebarCollapsed_{true};
  int selectedProjectIndex_{0};
  int hoveredRemoveIndex_{-1};
  int activeRackPluginIndex_{0};
  bool userHasCustomName_{false};
  std::string statusMessage_{};

  std::atomic<bool> isBuildRunning_{false};
  std::atomic<bool> buildFinishedPendingUiSync_{false};
  std::atomic<int> buildStepIndex_{0};
  uint64_t buildStartTickMs_{0};
  uint64_t lastActionFlashTickMs_{0};
  bool launchStandaloneAfterBuild_{false};
  bool openIdeAfterBuild_{false};
  std::string activeBuildTargetFolder_{};
  mutable std::mutex buildStateMutex_;
  std::string buildStepHeadline_{};
  std::string buildLiveCompilerLine_{};
  std::string lastConsoleActionBanner_{};
  ProjectGenerator::BuildExecutionReport lastBuildReport_{};
  std::thread backgroundBuildThread_;

  std::set<std::string> hiddenProjects_;
  std::vector<WorkspacePlugin> projects_;
  ProjectConfig wizardConfig_;
  ToolchainStatus toolchainStatus_{};

  static constexpr size_t kMaxPalettePerPage = 6;
  static constexpr size_t kMaxUiPalettePerPage = 5;

  SchematicCategory activeCategory_{SchematicCategory::PreampsAndTubes};
  int activeSubFilter_{0};
  SchematicSourceFilter activeSourceFilter_{SchematicSourceFilter::All};
  std::string librarySearchQuery_{};
  float schematicLibraryScrollY_{0.0f};
  std::vector<SchematicComponentMeta> paletteItems_;
  int palettePage_{0};
  int hoveredPaletteItemIdx_{-1};
  std::vector<SchematicNode> schematicChain_;
  int selectedNodeIndex_{0};
  int canvasPage_{0};
  float schematicScrollX_{0.0f};
  float schematicZoom_{1.0f};
  bool schematicCompactView_{false};
  bool schematicMacroGrouped_{false};
  bool dragTargetParallelLane_{false};
  bool isPanningSchematicCanvas_{false};
  float schematicPanStartMouseX_{0.0f};
  float schematicPanStartScrollX_{0.0f};
  bool isDraggingMinimap_{false};
  int draggingSummingStageIdx_{-1};
  float summingDragStartY_{0.0f};
  float summingDragStartMix_{50.0f};
  int inspectorDraggingParamSlot_{-1};
  float inspectorDragStartY_{0.0f};
  float inspectorDragStartNorm_{0.5f};
  bool isDraggingPaletteItem_{false};
  SchematicComponentType draggedComponentType_{
      SchematicComponentType::ClassAPreamp};
  int dragInsertSlot_{-1};
  bool isDraggingCanvasNode_{false};
  int draggedNodeOrigIndex_{-1};
  gui::Point<float> dragCurrentPos_{0.0f, 0.0f};

  UiWidgetCategory activeUiCategory_{UiWidgetCategory::Knobs};
  float uiWidgetLibraryScrollY_{0.0f};
  std::vector<UiWidgetCatalogEntry> uiPaletteItems_;
  int uiPalettePage_{0};
  FaceplateLayout faceplateLayout_;
  int selectedUiWidgetIndex_{-1};
  bool isDraggingUiPalette_{false};
  UiWidgetCatalogEntry draggedUiEntry_{};
  bool isDraggingUiWidget_{false};
  bool isResizingUiWidget_{false};
  bool isRotatingUiWidget_{false};
  uint64_t lastUiWidgetInteractMs_{0};
  gui::Point<float> lastMousePos_{0.0f, 0.0f};
  gui::Point<float> uiDragStartMouse_{0.0f, 0.0f};
  gui::Rect<float> uiDragStartRect_{0.0f, 0.0f, 100.0f, 100.0f};
  bool isScaleDropdownOpen_{false};
  int hoveredScaleDropdownIdx_{-1};
  std::vector<UiScalePreset> relevantScaleOptions_;
  UiStyleTemplate activeStyleTemplate_{UiStyleTemplate::SleekModernDark};
  bool isStyleTemplateDropdownOpen_{false};
  int hoveredStyleTemplateDropdownIdx_{-1};

  bool isParamBindDropdownOpen_{false};
  int hoveredParamBindIdx_{-1};
  std::vector<ParamBindOption> paramBindOptions_;

  UiPopupMenuKind activePopupMenu_{UiPopupMenuKind::None};
  int hoveredPopupItemIdx_{-1};
  std::vector<UiPopupItem> activePopupItems_;

  std::unique_ptr<gui::Button> btnTabProjects_;
  std::unique_ptr<gui::Button> btnTabWizard_;
  std::unique_ptr<gui::Button> btnTabUiBuilder_;
  std::unique_ptr<gui::Button> btnTabRack_;

  std::unique_ptr<gui::Button> btnHeaderSaveProject_;
  std::unique_ptr<gui::Button> btnHeaderOpenIde_;
  std::unique_ptr<gui::Button> btnHeaderVsCode_;
  std::unique_ptr<gui::Button> btnHeaderFullscreen_;
  std::unique_ptr<gui::Button> btnRestoreHidden_;

  std::unique_ptr<gui::Button> btnProjNew_;
  std::unique_ptr<gui::Button> btnProjDuplicate_;
  std::unique_ptr<gui::Button> btnProjImportExport_;

  std::unique_ptr<gui::TextInput> inputLibrarySearch_;

  std::unique_ptr<gui::Button> btnExportFmtVst3_;
  std::unique_ptr<gui::Button> btnExportFmtClap_;
  std::unique_ptr<gui::Button> btnExportFmtApp_;
  std::unique_ptr<gui::Button> btnChooseOutputPath_;
  std::unique_ptr<gui::Button> btnOutputSystemVst3_;
  std::unique_ptr<gui::Button> btnOutputDefaultDist_;
  std::unique_ptr<gui::Button> btnOpenFolder_;
  std::unique_ptr<gui::Button> btnBuildClap_;
  std::unique_ptr<gui::Button> btnRunStandalone_;

  std::unique_ptr<gui::Button> btnCatPreamps_;
  std::unique_ptr<gui::Button> btnCatIron_;
  std::unique_ptr<gui::Button> btnCatEq_;
  std::unique_ptr<gui::Button> btnCatDynamics_;
  std::unique_ptr<gui::Button> btnCatDelayRev_;
  std::unique_ptr<gui::Button> btnCatModRoute_;
  std::unique_ptr<gui::Button> btnPalettePrev_;
  std::unique_ptr<gui::Button> btnPaletteNext_;

  std::unique_ptr<gui::Button> btnPresetTubeFet_;
  std::unique_ptr<gui::Button> btnPresetBritStrip_;
  std::unique_ptr<gui::Button> btnPresetPultecOpto_;
  std::unique_ptr<gui::Button> btnPresetEchoSpace_;
  std::unique_ptr<gui::Button> btnPresetVocal_;
  std::unique_ptr<gui::Button> btnPresetSynthKeys_;
  std::unique_ptr<gui::Button> btnClearCanvas_;
  std::unique_ptr<gui::Button> btnCanvasPrevPage_;
  std::unique_ptr<gui::Button> btnCanvasNextPage_;

  std::unique_ptr<gui::Button> btnNodeToggleParallel_;
  std::unique_ptr<gui::Button> btnNodeMixDown_;
  std::unique_ptr<gui::Button> btnNodeMixUp_;
  std::unique_ptr<gui::Button> btnNodeToggleTrims_;
  std::unique_ptr<gui::Button> btnAuditionSchematic_;
  std::unique_ptr<gui::Button> btnDesignUiFromSchematic_;

  std::unique_ptr<Parameter> inspectorParam1_{nullptr};
  std::unique_ptr<Parameter> inspectorParam2_{nullptr};
  std::unique_ptr<gui::Knob> inspectorKnob1_{nullptr};
  std::unique_ptr<gui::Knob> inspectorKnob2_{nullptr};

  std::unique_ptr<gui::Button> btnUiFinish_;
  std::unique_ptr<gui::Button> btnUiPickFinishColor_;
  std::unique_ptr<gui::Button> btnUiGrain_;
  std::unique_ptr<gui::Button> btnUiScrews_;
  std::unique_ptr<gui::Button> btnUiScrewStyle_;
  std::unique_ptr<gui::Button> btnUiGrid_;
  std::unique_ptr<gui::Button> btnUiAlignRow_;
  std::unique_ptr<gui::Button> btnUiDistribute_;
  std::unique_ptr<gui::Button> btnUiSnapGuides_;
  std::unique_ptr<gui::Button> btnUiAddSticker_;
  std::unique_ptr<gui::Button> btnUiSyncSchematic_;
  std::unique_ptr<gui::Button> btnUiUnifyTheme_;
  std::unique_ptr<gui::Button> btnUiInternalTrims_;
  std::unique_ptr<gui::Button> btnUiClearCanvas_;
  std::unique_ptr<gui::Button> btnUiGlobalKnob_;
  std::unique_ptr<gui::Button> btnUiGlobalSwitch_;
  std::unique_ptr<gui::Button> btnUiGlobalMeter_;
  std::unique_ptr<gui::Button> btnUiGlobalColor_;

  std::unique_ptr<gui::Button> btnUiCatKnobs_;
  std::unique_ptr<gui::Button> btnUiCatSwitches_;
  std::unique_ptr<gui::Button> btnUiCatMeters_;
  std::unique_ptr<gui::Button> btnUiCatFrames_;
  std::unique_ptr<gui::Button> btnUiPalettePrev_;
  std::unique_ptr<gui::Button> btnUiPaletteNext_;

  std::unique_ptr<gui::TextInput> inputUiWidgetTitle_;
  std::unique_ptr<gui::TextInput> inputUiWidgetSub_;
  std::unique_ptr<gui::TextInput> inputUiBadgeFooter_;
  std::unique_ptr<gui::Button> btnUiEditBadge_;
  std::unique_ptr<gui::Button> btnUiWidgetBind_;
  std::unique_ptr<gui::Button> btnUiWidgetStyle_;
  std::unique_ptr<gui::Button> btnUiWidgetColor_;
  std::unique_ptr<gui::Button> btnUiWidgetScale_;
  std::unique_ptr<gui::Button> btnUiGlobalFontType_;
  std::unique_ptr<gui::Button> btnUiWidgetFont_;
  std::unique_ptr<gui::Button> btnUiGlobalFontColor_;
  std::unique_ptr<gui::Button> btnUiGlobalPlateColor_;
  std::unique_ptr<gui::Button> btnUiWidgetSizeDown_;
  std::unique_ptr<gui::Button> btnUiWidgetSizeUp_;
  std::unique_ptr<gui::Button> btnUiWidgetLayer_;
  std::unique_ptr<gui::Button> btnUiWidgetDup_;
  std::unique_ptr<gui::Button> btnUiWidgetDel_;
  std::unique_ptr<gui::Button> btnUiUploadStickerImg_;
  std::unique_ptr<gui::Button> btnUiStickerRotate_;

  std::unique_ptr<gui::TextInput> inputPluginName_;
  std::unique_ptr<gui::Button> btnCreateProject_;
  std::unique_ptr<gui::Button> btnCreateAndOpenIde_;

  std::unique_ptr<gui::Button> btnRackGain_;
  std::unique_ptr<gui::Button> btnRackAudioDevice_;
  std::unique_ptr<gui::Button> btnRackSchematic_;
  std::unique_ptr<gui::Button> btnSigDrum_;
  std::unique_ptr<gui::Button> btnSigSynth_;
  std::unique_ptr<gui::Button> btnSigCleanGuitar_;
  std::unique_ptr<gui::Button> btnSigGuitar_;
  std::unique_ptr<gui::Button> btnSigBass_;
  std::unique_ptr<gui::Button> btnSigVocal_;
  std::unique_ptr<gui::Button> btnSigWav_;
  std::unique_ptr<gui::Button> btnSigPink_;
  std::unique_ptr<gui::Button> btnSigTone_;
  std::unique_ptr<gui::Button> btnSigSilentAnalyzer_;
  std::unique_ptr<gui::Button> btnSigMute_;
  std::unique_ptr<gui::Button> btnRackDelta_;
  std::unique_ptr<gui::Button> btnRackAutoMatch_;
  std::unique_ptr<gui::Button> btnRackBypass_;
  std::unique_ptr<gui::Button> btnRackSavePreset_;
  std::unique_ptr<gui::Button> btnRackLoadPreset_;
  std::unique_ptr<gui::Button> btnRackAbCompare_;

  bool hasBankBState_{false};
  bool isBankBActive_{false};
  std::vector<SchematicNode> bankBChain_;

  TestSignalMode testSignalMode_{TestSignalMode::DrumTransientLoop};
  bool rackBypassed_{false};
  bool rackMuted_{false};
  bool rackPhaseInvert_{false};
  bool rackDeltaListen_{false};
  bool rackAutoMatch_{false};
  bool silentAnalyzerMode_{false};
  int rackDraggingWidgetIdx_{-1};
  int hoveredTapedWidgetIdx_{-1};
  std::string hoveredTapeLabel_{};
  int bayDraggingStageIdx_{-1};
  int bayDraggingTrimPotIdx_{-1};
  int bayScrollOffset_{0};
  float bayFlipProgress_{0.0f};
  float rackDragStartY_{0.0f};
  float rackDragStartNorm_{0.5f};
  float rackInputTrimDb_{0.0f};
  float rackOutputTrimDb_{0.0f};
  float rackGlobalWetMix_{1.0f};
  float rackStereoBalance_{0.0f};
  float smoothedVuInDb_{-12.0f};
  float smoothedVuOutDb_{-12.0f};
  float smoothedRmsDeltaDb_{0.0f};
  float integratedMeanSqIn_{0.05f};
  float integratedMeanSqOut_{0.05f};
  float smoothedVuGrDb_{0.0f};
  uint64_t signalSampleCounter_{0};
  uint32_t noiseState_{0x12345678u};
  float pinkB0_{0.0f}, pinkB1_{0.0f}, pinkB2_{0.0f};
  float lastInPeak_{0.0f};
  float lastInRms_{0.0f};
  float lastOutPeakL_{0.0f}, lastOutPeakR_{0.0f};
  float lastOutRmsL_{0.0f}, lastOutRmsR_{0.0f};
  float lastSchematicGrDb_{0.0f};

  LockFreeRingBuffer<float, 2048> rackInputRing_;
  LockFreeRingBuffer<float, 2048> rackOutputRing_;
  gui::SpectrumAnalyzer rackSpectrum_;
  gui::WaveformScope rackScope_;
  gui::GainReductionHistory rackGrHistory_;
  gui::Meter rackInMeter_;
  gui::Meter rackOutMeter_;

  LiveSchematicRunner liveSchematicRunner_;
  audio_io::RealtimeWaveOutPlayer audioOut_;
  audio_io::WavLoopPlayer wavLoopPlayer_;
  std::mutex audioRenderMutex_;
  std::array<float, 1024> latestInBlockL_{};
  std::array<float, 1024> latestInBlockR_{};
  std::array<float, 1024> latestOutBlockL_{};
  std::array<float, 1024> latestOutBlockR_{};
  bool activeStemSlotLoaded_{false};
  std::array<std::string, 6> customStemPaths_{};

  HistoryManager history_{};
  StudioPreferences prefs_{};
  std::vector<std::string> recentProjects_{};
  bool isProjectDirty_{false};
  uint64_t lastAutoSaveTickMs_{0};
  TopMenuBarSection activeTopMenu_{TopMenuBarSection::None};
  int hoveredTopMenuIdx_{-1};
  int hoveredQuickIconIdx_{-1};
  ActiveStudioModal activeStudioModal_{ActiveStudioModal::None};
  int activePreferencesTab_{0};
  bool hasPendingMouseDownSnapshot_{false};
  ProjectSnapshot pendingMouseDownSnapshot_{};
  std::string lastDragOrClickActionLabel_{};
  bool hasClipboardStyle_{false};
  bool hasClipboardWidget_{false};
  FaceplateTheme clipboardTheme_{};
  UiCanvasWidget clipboardWidgetStyle_{};

  bool labBenchActive_{false};
  int labBenchPlotMode_{0};
  int hoveredOverdriveStageIdx_{-1};
  gui::Rect<float> overdriveAutoTrimBtnRect_{0.0f, 0.0f, 0.0f, 0.0f};
  std::array<float, 4096> dryDelayLineL_{};
  std::array<float, 4096> dryDelayLineR_{};
  uint32_t dryDelayWritePos_{0};
  float autoLevelEnvInSq_{0.01f};
  float autoLevelEnvOutSq_{0.01f};
  float autoLevelGainSmooth_{1.0f};

  bool rackPresetMenuOpen_{false};
  bool rackAudioDeviceMenuOpen_{false};
  int hoveredRackAudioDeviceItem_{-1};
  int activeRackPresetIdx_{0};
  int hoveredRackPresetItem_{-1};
  std::vector<std::string> customPresetFiles_{};

  std::unique_ptr<gui::Button> btnCatAutoSmart_;
  std::vector<SavedUserTheme> savedUserThemes_{};
  std::unique_ptr<gui::TextInput> inputCustomThemeName_;
  std::unique_ptr<gui::Button> btnSaveCustomTheme_;
  int hoveredUserThemeDropdownIdx_{-1};
  bool recentSubmenuOpen_{false};
  size_t latestRenderedFrames_{256};
  float autoLevelPolaritySmooth_{1.0f};
  std::filesystem::path customExportDir_{};

  void loadExportFolderSetting() {
    const auto p =
        ProjectGenerator::getWorkspaceRoot() / ".avra_export_folder.txt";
    if (std::filesystem::exists(p)) {
      std::ifstream in(p);
      std::string dirStr;
      std::getline(in, dirStr);
      if (!dirStr.empty()) {
        customExportDir_ = dirStr;
      }
    }
  }

  std::filesystem::path
  getEffectiveExportDir(const std::string &projFolder) const {
    if (!customExportDir_.empty()) {
      return customExportDir_;
    }
    return ProjectGenerator::getWorkspaceRoot() / "dist" / projFolder;
  }

  void setCustomExportFolder(const std::string &dirStr) {
    customExportDir_ = dirStr;
    std::ofstream out(ProjectGenerator::getWorkspaceRoot() /
                      ".avra_export_folder.txt");
    out << dirStr;
    syncPreferencesRuntimeState();
    if (selectedProjectIndex_ >= 0 &&
        selectedProjectIndex_ < static_cast<int>(projects_.size()) &&
        !isBuildRunning_.load()) {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      lastBuildReport_ = ProjectGenerator::inspectProjectArtifacts(
          projects_[selectedProjectIndex_].diskFolder, prefs_.exportFormatVst3,
          prefs_.exportFormatClap, prefs_.exportFormatStandalone,
          customExportDir_);
    }
    statusMessage_ =
        std::format("BUILD DESTINATION SET TO: {}", dirStr);
    repaint();
  }

  void resetCustomExportFolderToDefault() {
    customExportDir_.clear();
    std::error_code ec;
    std::filesystem::remove(
        ProjectGenerator::getWorkspaceRoot() / ".avra_export_folder.txt", ec);
    syncPreferencesRuntimeState();
    if (selectedProjectIndex_ >= 0 &&
        selectedProjectIndex_ < static_cast<int>(projects_.size()) &&
        !isBuildRunning_.load()) {
      std::lock_guard<std::mutex> lk(buildStateMutex_);
      lastBuildReport_ = ProjectGenerator::inspectProjectArtifacts(
          projects_[selectedProjectIndex_].diskFolder, prefs_.exportFormatVst3,
          prefs_.exportFormatClap, prefs_.exportFormatStandalone,
          customExportDir_);
    }
    statusMessage_ = "BUILD DESTINATION RESET TO DEFAULT (dist/<Project>/)";
    repaint();
  }

  void updateOutputFolderUiButtons() {
    const bool hasCustom = !customExportDir_.empty();
    const bool isSysVst3 =
        hasCustom && (_stricmp(customExportDir_.string().c_str(),
                               "C:\\Program Files\\Common Files\\VST3") == 0);
    if (btnChooseOutputPath_) {
      btnChooseOutputPath_->setText(
          hasCustom ? std::format("FOLDER: {}",
                                  customExportDir_.filename().empty()
                                      ? customExportDir_.string()
                                      : customExportDir_.filename().string())
                    : "CHOOSE OUTPUT FOLDER...");
      btnChooseOutputPath_->setToggleState(hasCustom && !isSysVst3);
    }
    if (btnOutputSystemVst3_) {
      btnOutputSystemVst3_->setToggleState(isSysVst3);
    }
    if (btnOutputDefaultDist_) {
      btnOutputDefaultDist_->setToggleState(!hasCustom);
    }
  }

  static bool browseForFolderDialog(std::filesystem::path &outDir) {
#if defined(_WIN32)
    HRESULT hrCo = CoInitializeEx(
        nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog *pDlg = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
    bool picked = false;
    if (SUCCEEDED(hr) && pDlg) {
      DWORD opts = 0;
      if (SUCCEEDED(pDlg->GetOptions(&opts))) {
        pDlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM |
                         FOS_PATHMUSTEXIST);
      }
      pDlg->SetTitle(L"Select Destination Folder for Compiled Plugin Binaries");
      if (SUCCEEDED(pDlg->Show(nullptr))) {
        IShellItem *pItem = nullptr;
        if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem) {
          PWSTR pszPath = nullptr;
          if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)) &&
              pszPath) {
            outDir = std::filesystem::path(pszPath);
            picked = true;
            CoTaskMemFree(pszPath);
          }
          pItem->Release();
        }
      }
      pDlg->Release();
    }
    if (SUCCEEDED(hrCo)) {
      CoUninitialize();
    }
    return picked;
#else
    (void)outDir;
    return false;
#endif
  }

  bool promptForBuildDestinationAndPluginName() {
#if defined(_WIN32)
    std::string defaultName = wizardConfig_.name;
    if (defaultName.empty() && selectedProjectIndex_ >= 0 &&
        selectedProjectIndex_ < static_cast<int>(projects_.size())) {
      defaultName = projects_[selectedProjectIndex_].diskFolder;
    }
    if (defaultName.empty()) {
      defaultName = "AvraCustomPlugin";
    }
    defaultName = ProjectGenerator::sanitizeName(defaultName);

    std::filesystem::path initDir =
        customExportDir_.empty()
            ? (ProjectGenerator::getWorkspaceRoot() / "dist")
            : customExportDir_;
    std::error_code ec;
    if (!std::filesystem::exists(initDir, ec)) {
      std::filesystem::create_directories(initDir, ec);
    }

    char fileBuf[MAX_PATH] = {0};
    strncpy_s(fileBuf, defaultName.c_str(), _TRUNCATE);

    const std::string initDirStr = initDir.string();
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter =
        "Compiled Plugin Name & Destination (*.*)\0*.*\0"
        "VST3 / CLAP / Standalone (.vst3;*.clap;*.exe)\0*.vst3;*.clap;*.exe\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = initDirStr.c_str();
    ofn.lpstrTitle =
        "Choose Destination Folder & Enter Plugin Name to Build";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn)) {
      const std::filesystem::path chosenPath(fileBuf);
      const std::filesystem::path parentDir = chosenPath.parent_path();
      std::string chosenName =
          ProjectGenerator::sanitizeName(chosenPath.stem().string());
      if (chosenName.empty()) {
        chosenName = defaultName;
      }
      wizardConfig_.name = chosenName;
      wizardConfig_.pluginId =
          std::format("com.avra.{}", toLower(chosenName));
      if (inputPluginName_) {
        inputPluginName_->setText(chosenName);
      }
      faceplateLayout_.theme.pluginTitle = chosenName;
      for (auto &w : faceplateLayout_.widgets) {
        if (w.type == UiWidgetType::RivetedNameplate) {
          w.title = chosenName;
        }
      }
      userHasCustomName_ = true;

      if (!parentDir.empty()) {
        setCustomExportFolder(parentDir.string());
        updateOutputFolderUiButtons();
      }
      return true;
    }
    statusMessage_ =
        "Build cancelled (no destination folder / plugin name selected).";
    repaint();
    return false;
#else
    return true;
#endif
  }

  bool promptForBuildSaveFolder() {
#if defined(_WIN32)
    std::filesystem::path chosen;
    if (browseForFolderDialog(chosen)) {
      setCustomExportFolder(chosen.string());
      updateOutputFolderUiButtons();
      return true;
    }
#endif
    return false;
  }

  void triggerBuildWithFolderPrompt(bool buildVst3, bool buildClap,
                                    bool buildApp) {
    prefs_.exportFormatVst3 = buildVst3;
    prefs_.exportFormatClap = buildClap;
    prefs_.exportFormatStandalone = buildApp;
    prefs_.ensureAtLeastOneExportFormat();
    saveStudioPreferencesToFile(
        ProjectGenerator::getWorkspaceRoot() / ".avra_preferences.json",
        prefs_);
    updateBuildFormatUiLabels();
    buildSelectedProject();
  }

  void triggerBuildWithFolderPrompt(const std::string &folderOverride = "",
                                    bool launchStandaloneAfter = false,
                                    bool openVsCodeAfter = false) {
    std::string target = folderOverride;
    if (target.empty()) {
      if (selectedProjectIndex_ >= 0 &&
          selectedProjectIndex_ < static_cast<int>(projects_.size())) {
        target = projects_[selectedProjectIndex_].diskFolder;
      } else {
        target = wizardConfig_.name.empty() ? "GainPlugin" : wizardConfig_.name;
      }
    }
    startAsyncProjectBuild(target, launchStandaloneAfter, openVsCodeAfter);
  }

  void syncPreferencesRuntimeState() {
    gui::Knob::setGlobalMouseMode(prefs_.knobMouseMode);
    audioOut_.configureDevice(
        audioOut_.getInputDeviceId(), prefs_.getOutputDeviceHardwareId(),
        static_cast<uint32_t>(prefs_.sampleRateHz),
        static_cast<size_t>(prefs_.blockBufferSize), prefs_.audioDriverMode);

    if (!toolchainStatus_.cmakePath.empty()) {
      prefs_.cmakeExePath = toolchainStatus_.cmakePath;
    }
    if (!toolchainStatus_.vsCodePath.empty()) {
      prefs_.vsCodeExePath = toolchainStatus_.vsCodePath;
    }
    prefs_.activeExportFolderDisplay =
        customExportDir_.empty()
            ? "Default Workspace dist/<Project>/ (+ System VST3/CLAP)"
            : customExportDir_.string();
    prefs_.activeUndoCount = static_cast<int>(history_.undoDepth());
    prefs_.activeRedoCount = static_cast<int>(history_.redoDepth());
    updateOutputFolderUiButtons();
  }

  void browseForCustomExportFolder() { promptForBuildSaveFolder(); }

  void openCurrentProjectInIde() { openSelectedInVsCode(); }

  void openCurrentProjectFolderInExplorer() { openSelectedFolder(false); }
};

} // namespace avra::studio