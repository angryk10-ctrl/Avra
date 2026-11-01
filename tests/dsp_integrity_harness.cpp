#include "avra/Avra.h"
#include "../studio/AvraStudio.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <string>
#include <array>

using namespace avra;
using namespace avra::studio;

namespace {

constexpr double kSampleRate = 48000.0;
constexpr size_t kNumFrames  = 48000; // 1.0 second buffer

struct TestSignalBuffer {
    std::vector<float> inL;
    std::vector<float> inR;
};

// Generates a 1-second deterministic composite test signal:
//   - Periodic transient impulses + drum bursts (triggers gates, hold timers, compressors & envelopes)
//   - Broadband deterministic pink/white noise (excites all spectral & EQ filter bands from 20 Hz to 20 kHz)
//   - Low + Mid + High sine components (80 Hz, 1000 Hz, 6000 Hz)
TestSignalBuffer generateCompositeSignal() {
    TestSignalBuffer buf;
    buf.inL.resize(kNumFrames, 0.0f);
    buf.inR.resize(kNumFrames, 0.0f);

    uint32_t rng = 0x13579BDFu;
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    for (size_t i = 0; i < kNumFrames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);

        // Deterministic PRNG noise -> 3-stage pinking filter
        rng = rng * 1664525u + 1013904223u;
        const float white = (static_cast<float>(rng & 0xFFFF) / 32768.0f) - 1.0f;
        b0 = 0.997f * b0 + white * 0.029f;
        b1 = 0.985f * b1 + white * 0.032f;
        b2 = 0.950f * b2 + white * 0.048f;
        const float pink = (b0 + b1 + b2 + white * 0.18f);

        // Multi-tone sines (60 Hz sub, 440 Hz body, 1200 Hz mid, 8500 Hz air)
        const float sines = 0.22f * std::sin(2.0f * 3.1415926535f * 60.0f * t)
                          + 0.20f * std::sin(2.0f * 3.1415926535f * 440.0f * t)
                          + 0.18f * std::sin(2.0f * 3.1415926535f * 1200.0f * t)
                          + 0.15f * std::sin(2.0f * 3.1415926535f * 8500.0f * t);

        // Pulsed envelope (burst every 0.25s with sharp impulse attack + silence gap for gate/reverb tail testing)
        const size_t beatPhase = i % 12000;
        float env = 0.15f;
        if (beatPhase < 3000) {
            env = std::exp(-static_cast<float>(beatPhase) / 1100.0f);
        }
        const float impulse = (beatPhase == 0) ? 0.95f : 0.0f;

        const float sigL = (sines + 0.35f * pink) * env + impulse;
        const float sigR = (sines * 0.96f - 0.35f * pink) * env + impulse;
        buf.inL[i] = sigL;
        buf.inR[i] = sigR;
    }
    return buf;
}

// Computes single-bin DFT magnitude at targetFreqHz over a steady-state window
double measureFrequencyMagnitude(const std::vector<float>& sig, size_t startSample, size_t count, double freqHz) {
    double re = 0.0;
    double im = 0.0;
    const double omega = 2.0 * 3.14159265358979323846 * freqHz / kSampleRate;
    for (size_t n = 0; n < count; ++n) {
        // Hann window to prevent spectral leakage
        const double w = 0.5 * (1.0 - std::cos(2.0 * 3.14159265358979323846 * static_cast<double>(n) / static_cast<double>(count - 1)));
        const double s = static_cast<double>(sig[startSample + n]) * w;
        re += s * std::cos(omega * static_cast<double>(n));
        im -= s * std::sin(omega * static_cast<double>(n));
    }
    return (2.0 * std::sqrt(re * re + im * im)) / static_cast<double>(count);
}

// GATE 1: Control Density (>= 6 controls + 1 live telemetry meter + Link IO Input/Output pairing)
bool checkControlDensity(const SchematicComponentMeta& meta, std::string& failReason) {
    SchematicNode node = makeDefaultSchematicNode(meta.type);
    FaceplateTheme theme{};
    FaceplateLayout layout = buildFaceplateFromSchematicWithTheme(meta.shortName, { node }, theme);

    int interactiveControls = 0;
    bool hasTelemetryMeter  = false;
    bool hasLinkIo          = false;
    bool hasInputKnob       = false;
    bool hasOutputKnob      = false;

    for (const auto& w : layout.widgets) {
        if (isMeterWidget(w.type)) {
            hasTelemetryMeter = true;
        }
        if ((w.boundStageIdx == 0 || w.boundStageIdx == -1) &&
            (isKnobWidget(w.type) || isSwitchWidget(w.type) || w.type == UiWidgetType::VerticalStudioFader)) {
            if (w.boundStageIdx == 0) {
                ++interactiveControls;
            }
            if (w.boundStageIdx == 0 && (w.boundParamSlot == 8 || w.boundParamSlot == 108) &&
                isSwitchWidget(w.type) && w.title == "LINK IO") {
                hasLinkIo = true;
            }
            if (isKnobWidget(w.type) || w.type == UiWidgetType::VerticalStudioFader) {
                std::string lower = w.title;
                for (char& c : lower) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
                if (lower.find("in") != std::string::npos || lower.find("drive") != std::string::npos ||
                    lower.find("gain") != std::string::npos || lower.find("trim") != std::string::npos ||
                    (w.boundStageIdx == 0 && w.boundParamSlot == 0)) {
                    hasInputKnob = true;
                }
                if (lower.find("out") != std::string::npos || lower.find("makeup") != std::string::npos) {
                    hasOutputKnob = true;
                }
            }
        }
    }

    if (interactiveControls < static_cast<int>(meta.getParamCount()) + 1) {
        failReason = "Fewer than expected interactive controls (" + std::to_string(interactiveControls) + ")";
        return false;
    }
    if (!hasTelemetryMeter) {
        failReason = "Missing live telemetry meter widget";
        return false;
    }
    if (hasLinkIo && (!hasInputKnob || !hasOutputKnob)) {
        failReason = "LINK IO present without paired Input & Output Trim knobs";
        return false;
    }
    return true;
}

// GATE 2: Unit Sanity Check
bool checkUnitSanity(const SchematicComponentMeta& meta, std::string& failReason) {
    for (int p = 0; p < static_cast<int>(meta.getParamCount()); ++p) {
        const auto& spec = meta.getParamSpec(p);
        if (spec.name.empty()) {
            failReason = "Empty parameter name at slot " + std::to_string(p);
            return false;
        }
        if (!(spec.maxVal > spec.minVal)) {
            failReason = "Invalid min/max range on '" + spec.name + "'";
            return false;
        }
    }
    return true;
}

// GATE 3: Perturbation / Dead-Knob Check (toggling each parameter 0..paramCount-1 from min to max must produce > 0 RMS diff)
bool checkNoDeadKnobs(
    const SchematicComponentMeta& meta,
    const TestSignalBuffer& testSig,
    std::vector<double>& outKnobRmsDiffs,
    std::string& failReason) {
    LiveSchematicRunner runner;
    const size_t paramCount = meta.getParamCount();
    outKnobRmsDiffs.assign(paramCount, 0.0);

    for (size_t p = 0; p < paramCount; ++p) {
        const auto& spec = meta.getParamSpec(static_cast<int>(p));

        // Run with parameter p at minVal
        SchematicNode nodeMin = makeDefaultSchematicNode(meta.type);
        nodeMin.autoGainLinked = false; // Test each parameter independently
        nodeMin.trim2Val = 0.0f;        // Deterministic zero drift
        nodeMin.setParamVal(static_cast<int>(p), spec.minVal);

        std::vector<float> minL = testSig.inL;
        std::vector<float> minR = testSig.inR;
        float maxGrMin = 0.0f;
        runner.prepare(kSampleRate);
        runner.processBlock(minL.data(), minR.data(), kNumFrames, { nodeMin }, maxGrMin);

        // Run with parameter p at maxVal
        SchematicNode nodeMax = makeDefaultSchematicNode(meta.type);
        nodeMax.autoGainLinked = false;
        nodeMax.trim2Val = 0.0f;
        nodeMax.setParamVal(static_cast<int>(p), spec.maxVal);

        std::vector<float> maxL = testSig.inL;
        std::vector<float> maxR = testSig.inR;
        float maxGrMax = 0.0f;
        runner.prepare(kSampleRate);
        runner.processBlock(maxL.data(), maxR.data(), kNumFrames, { nodeMax }, maxGrMax);

        double sumSqDiff = 0.0;
        for (size_t i = 0; i < kNumFrames; ++i) {
            const double dL = static_cast<double>(maxL[i]) - static_cast<double>(minL[i]);
            const double dR = static_cast<double>(maxR[i]) - static_cast<double>(minR[i]);
            sumSqDiff += 0.5 * (dL * dL + dR * dR);
        }
        const double rmsDiff = std::sqrt(sumSqDiff / static_cast<double>(kNumFrames));
        outKnobRmsDiffs[p] = rmsDiff;

        if (!(rmsDiff > 1e-6)) {
            failReason = "DEAD KNOB on Slot " + std::to_string(p) + " ('" + spec.name + "'): RMS diff = 0.0";
            return false;
        }
    }
    return true;
}

// GATE 4: Harmonic, Stability & DC Offset Check
bool checkHarmonicsAndStability(const SchematicComponentMeta& meta, double& outMetricDb, std::string& failReason) {
    LiveSchematicRunner runner;
    SchematicNode node = makeDefaultSchematicNode(meta.type);
    node.trim2Val = 0.0f;

    // 1. Stability & Residual DC Offset Check (0.2s active signal followed by 2.8s digital silence)
    const size_t stabFrames = static_cast<size_t>(kSampleRate) * 3;
    std::vector<float> stabL(stabFrames, 0.0f);
    std::vector<float> stabR(stabFrames, 0.0f);
    for (size_t i = 0; i < 9600; ++i) {
        const float s = 0.5f * std::sin(2.0f * 3.1415926535f * 250.0f * (static_cast<float>(i) / static_cast<float>(kSampleRate)));
        stabL[i] = s;
        stabR[i] = s;
    }
    float gr = 0.0f;
    runner.prepare(kSampleRate);
    runner.processBlock(stabL.data(), stabR.data(), stabFrames, { node }, gr);

    for (size_t i = 0; i < stabFrames; ++i) {
        if (!std::isfinite(stabL[i]) || !std::isfinite(stabR[i])) {
            failReason = "NaN or Inf detected at sample " + std::to_string(i);
            return false;
        }
    }

    // Measure DC offset over final 4608 samples (24 exact periods of 250 Hz) of silence
    double dcSum = 0.0;
    for (size_t i = stabFrames - 4608; i < stabFrames; ++i) {
        dcSum += 0.5 * (static_cast<double>(stabL[i]) + static_cast<double>(stabR[i]));
    }
    const double dcMean = std::abs(dcSum / 4608.0);
    if (dcMean > 1e-5) { // -100 dBFS = 1e-5
        failReason = "Residual DC offset after silence exceeds -100 dBFS (" + std::to_string(dcMean) + ")";
        return false;
    }

    // 2. Asymmetrical 2nd-Order Harmonic Check (2000 Hz on 1000 Hz sine) for Preamps/JFET/Tubes
    if (meta.category == SchematicCategory::PreampsAndTubes) {
        std::vector<float> sineL(24000, 0.0f);
        std::vector<float> sineR(24000, 0.0f);
        for (size_t i = 0; i < sineL.size(); ++i) {
            const float s = 0.55f * std::sin(2.0f * 3.1415926535f * 1000.0f * (static_cast<float>(i) / static_cast<float>(kSampleRate)));
            sineL[i] = s;
            sineR[i] = s;
        }
        runner.prepare(kSampleRate);
        runner.processBlock(sineL.data(), sineR.data(), sineL.size(), { node }, gr);

        const double mag1k = measureFrequencyMagnitude(sineL, 8000, 12000, 1000.0);
        const double mag2k = measureFrequencyMagnitude(sineL, 8000, 12000, 2000.0);
        const double h2RelDb = 20.0 * std::log10(std::max(1e-12, mag2k) / std::max(1e-12, mag1k));
        outMetricDb = h2RelDb;
        if (h2RelDb < -65.0) {
            failReason = "Insufficient 2nd-order harmonic (2000 Hz) on 1000 Hz sine (" + std::to_string(h2RelDb) + " dB)";
            return false;
        }
    } else {
        outMetricDb = 20.0 * std::log10(std::max(1e-12, dcMean));
    }

    return true;
}

// Global Transformer Frequency-Dependent THD Verification (50 Hz THD > 5000 Hz THD)
bool checkTransformerFrequencyDependentThd(double& outThd50Db, double& outThd5kDb) {
    dsp::circuits::IronTransformerStage iron;
    iron.prepare(kSampleRate);
    iron.setParameters(0.75f, 2.0f, 20.0f, 20000.0f, 0.0f);

    auto measureThdAtFreq = [&](double freqHz) -> double {
        iron.reset();
        std::vector<float> sigL(24000, 0.0f);
        std::vector<float> sigR(24000, 0.0f);
        for (size_t i = 0; i < sigL.size(); ++i) {
            const float s = 0.65f * std::sin(2.0f * 3.1415926535f * static_cast<float>(freqHz) * (static_cast<float>(i) / static_cast<float>(kSampleRate)));
            sigL[i] = s;
            sigR[i] = s;
            iron.processStereo(sigL[i], sigR[i]);
        }
        const double f1 = measureFrequencyMagnitude(sigL, 8000, 14000, freqHz);
        const double f2 = measureFrequencyMagnitude(sigL, 8000, 14000, freqHz * 2.0);
        const double f3 = measureFrequencyMagnitude(sigL, 8000, 14000, freqHz * 3.0);
        const double harmRms = std::sqrt(f2 * f2 + f3 * f3);
        return 20.0 * std::log10(std::max(1e-12, harmRms) / std::max(1e-12, f1));
    };

    outThd50Db = measureThdAtFreq(50.0);
    outThd5kDb = measureThdAtFreq(5000.0);
    return (outThd50Db > outThd5kDb + 6.0);
}

} // namespace

int main(int argc, char** argv) {
    std::cout << "================================================================================\n";
    std::cout << "  AVRA COMMERCIAL DSP & 4-TIER UI INTEGRITY VERIFICATION HARNESS (C++20 @ 48kHz)\n";
    std::cout << "================================================================================\n";

    std::vector<SchematicComponentType> targetTypes;
    bool runAll = false;
    int targetTab = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--all") {
            runAll = true;
        } else if (arg == "--tab" && i + 1 < argc) {
            targetTab = std::stoi(argv[++i]);
        }
    }

    if (runAll) {
        for (const auto& m : getSchematicCatalog()) {
            targetTypes.push_back(m.type);
        }
    } else if (targetTab >= 0 && targetTab <= 5) {
        for (const auto& m : getSchematicCatalog()) {
            if (static_cast<int>(m.category) == targetTab) {
                targetTypes.push_back(m.type);
            }
        }
    } else {
        // Default Step 4 Vertical-Slice: The 5 currently loaded target modules
        targetTypes = {
            SchematicComponentType::JfetBuffer,            // PR1: Matched JFET Buffer
            SchematicComponentType::ParallelPassiveLcEq,   // EQ3: Parallel 4-Band LC Network
            SchematicComponentType::BarkAuditoryUnmasker,  // SP1: Cochlear Entropy Unmasker
            SchematicComponentType::NonlinearGatedReverb,  // RV4: Nonlinear Room Gate
            SchematicComponentType::DispersionSpringReverb // RV3: Triple-Helix Spring
        };
    }

    const TestSignalBuffer testSig = generateCompositeSignal();
    int passedCount = 0;
    int failedCount = 0;

    std::cout << std::left
              << std::setw(6)  << "CODE"
              << std::setw(29) << "MODULE NAME"
              << std::setw(10) << "DENSITY"
              << std::setw(10) << "UNITS"
              << std::setw(36) << "5-KNOB PERTURBATION RMS (P1..P5)"
              << std::setw(12) << "STABILITY"
              << "STATUS\n";
    std::cout << "------------------------------------------------------------------------------------------------------------\n";

    for (SchematicComponentType type : targetTypes) {
        const auto& meta = getComponentMeta(type);
        std::string failReason;

        const bool okDensity = checkControlDensity(meta, failReason);
        const bool okUnits   = okDensity ? checkUnitSanity(meta, failReason) : false;

        std::vector<double> knobDiffs;
        const bool okKnobs   = (okDensity && okUnits) ? checkNoDeadKnobs(meta, testSig, knobDiffs, failReason) : false;

        double metricDb = 0.0;
        const bool okStab    = (okDensity && okUnits && okKnobs) ? checkHarmonicsAndStability(meta, metricDb, failReason) : false;

        const bool allOk = okDensity && okUnits && okKnobs && okStab;
        if (allOk) {
            ++passedCount;
        } else {
            ++failedCount;
        }

        std::ostringstream rmsStream;
        rmsStream << std::fixed << std::setprecision(3);
        for (size_t k = 0; k < std::min<size_t>(5u, knobDiffs.size()); ++k) {
            if (k > 0) rmsStream << "/";
            rmsStream << knobDiffs[k];
        }

        std::cout << std::left
                  << std::setw(6)  << meta.designator
                  << std::setw(29) << meta.shortName.substr(0, 27)
                  << std::setw(10) << (okDensity ? "PASS(>=6)" : "FAIL")
                  << std::setw(10) << (okUnits   ? "PASS"      : "FAIL")
                  << std::setw(36) << rmsStream.str()
                  << std::setw(12) << (okStab    ? "PASS"      : "FAIL")
                  << (allOk ? "[PASS]" : ("[FAIL: " + failReason + "]")) << "\n";
    }

    double thd50Db = 0.0, thd5kDb = 0.0;
    const bool okIronThd = checkTransformerFrequencyDependentThd(thd50Db, thd5kDb);
    std::cout << "------------------------------------------------------------------------------------------------------------\n";
    std::cout << std::fixed << std::setprecision(2)
              << "Transformer Physics Check (B ~ V/f): THD @ 50 Hz = " << thd50Db << " dB  vs  THD @ 5000 Hz = " << thd5kDb << " dB  ->  "
              << (okIronThd ? "[PASS]" : "[FAIL]") << "\n";
    if (!okIronThd) {
        ++failedCount;
    }

    std::cout << "================================================================================\n";
    std::cout << "  SUMMARY: " << passedCount << " / " << targetTypes.size() << " MODULES PASSED  |  FAILURES: " << failedCount << "\n";
    std::cout << "================================================================================\n";

    return (failedCount == 0) ? 0 : 1;
}
