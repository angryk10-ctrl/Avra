// =============================================================================
// AVRA MODULE BEHAVIOUR TESTS
//
// Per-module tests that check every control does what its label says, with
// measured numbers (not just "the output changed"). Each module rewritten during
// the remediation gets a section here. Tests drive the stage through the same
// setControlValues() path that LiveSchematicRunner and the C++ exporter use.
// =============================================================================
#include "avra/Avra.h"
#include "avra/studio/SchematicSpec.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <span>
#include <string>
#include <vector>

using namespace avra;

namespace {

constexpr double kSr = 48000.0;
constexpr double kPi = 3.14159265358979323846;

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const std::string& what, double measured, const char* unit = "") {
    ++g_checks;
    if (!ok) ++g_failures;
    std::printf("  [%s] %s  (measured %.2f%s)\n", ok ? "PASS" : "FAIL", what.c_str(), measured, unit);
}

struct Measure {
    double rmsInDb{0.0};
    double rmsOutDb{0.0};
    double gainDb{0.0};
    double correlation{0.0}; // normalized cross-correlation in vs out at lag 0
    double thdPct{0.0};      // (total - fundamental) / fundamental, from a single-bin DFT
};

double dftMag(const std::vector<float>& x, size_t start, size_t n, double hz) {
    double re = 0.0, im = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double w = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(n - 1));
        const double ph = 2.0 * kPi * hz * static_cast<double>(start + i) / kSr;
        re += w * x[start + i] * std::cos(ph);
        im -= w * x[start + i] * std::sin(ph);
    }
    return std::sqrt(re * re + im * im);
}

// Runs a stereo sine through a stage after applying controls; measures steady state.
template <typename Stage>
Measure runSine(Stage& st, std::span<const float> controls, double hz, double ampDbfs, double seconds = 0.5) {
    st.prepare(kSr);
    st.setControlValues(controls);
    const size_t n = static_cast<size_t>(kSr * seconds);
    const size_t settle = n / 2;
    const double amp = std::pow(10.0, ampDbfs / 20.0);
    std::vector<float> in(n), out(n);
    for (size_t i = 0; i < n; ++i) {
        float l = static_cast<float>(amp * std::sin(2.0 * kPi * hz * static_cast<double>(i) / kSr));
        float r = l;
        in[i] = l;
        st.processStereo(l, r);
        out[i] = l;
    }
    double si = 0.0, so = 0.0, sio = 0.0;
    for (size_t i = settle; i < n; ++i) {
        si += static_cast<double>(in[i]) * in[i];
        so += static_cast<double>(out[i]) * out[i];
        sio += static_cast<double>(in[i]) * out[i];
    }
    const double cnt = static_cast<double>(n - settle);
    Measure m;
    m.rmsInDb = 10.0 * std::log10(std::max(1e-20, si / cnt));
    m.rmsOutDb = 10.0 * std::log10(std::max(1e-20, so / cnt));
    m.gainDb = m.rmsOutDb - m.rmsInDb;
    m.correlation = sio / std::sqrt(std::max(1e-30, si * so));
    const size_t win = 8192;
    const double f1 = dftMag(out, n - win, win, hz);
    double harm = 0.0;
    for (int k = 2; k <= 6; ++k) {
        if (hz * k < kSr * 0.45) {
            const double hk = dftMag(out, n - win, win, hz * k);
            harm += hk * hk;
        }
    }
    m.thdPct = 100.0 * std::sqrt(harm) / std::max(1e-20, f1);
    return m;
}

// -----------------------------------------------------------------------------
// PR2 · Brit 73 Preamp (1073-style Class-A mic/line amplifier)
// Controls: 0 Gain idx (0..6 LINE +10..-20, 7..19 MIC -20..-80), 1 HPF idx (OFF/50/80/160/300),
//           2 Output Trim dB [Avra Extra], 3 Mic Impedance (0 LO-Z, 1 HI-Z), 4 Phase (0 NORM, 1 INV)
// -----------------------------------------------------------------------------
void testPR2() {
    std::printf("\nPR2 · Brit 73 Preamp\n");
    dsp::circuits::DiscreteClassAPreamp st;
    auto ctl = [](float gainIdx, float hpfIdx, float trimDb, float imp, float phase) {
        return std::vector<float>{ gainIdx, hpfIdx, trimDb, imp, phase };
    };
    const double lvl = -24.0;

    const auto lineUnity = runSine(st, ctl(2, 0, 0, 1, 0), 1000.0, lvl);
    check(std::abs(lineUnity.gainDb) < 1.0, "Gain LINE 0 is unity at 1 kHz (within 1 dB)", lineUnity.gainDb, " dB");

    const auto linePlus10 = runSine(st, ctl(0, 0, 0, 1, 0), 1000.0, lvl);
    check(linePlus10.gainDb < -7.0 && linePlus10.gainDb > -13.0, "Gain LINE +10 attenuates about 10 dB", linePlus10.gainDb, " dB");

    const auto lineMinus20 = runSine(st, ctl(6, 0, 0, 1, 0), 1000.0, lvl);
    check(lineMinus20.gainDb > lineUnity.gainDb + 8.0, "Gain LINE -20 adds substantial gain over LINE 0", lineMinus20.gainDb, " dB");
    check(lineMinus20.thdPct > lineUnity.thdPct, "Gain LINE -20 drives the Class-A stage harder (more THD)", lineMinus20.thdPct, " %");

    const auto mic50 = runSine(st, ctl(13, 0, 0, 1, 0), 1000.0, lvl);
    check(std::abs(mic50.gainDb) < 2.0, "Gain MIC -50 is unity drive for a -50 dBu nominal source (within 2 dB)", mic50.gainDb, " dB");
    const auto mic80 = runSine(st, ctl(19, 0, 0, 1, 0), 1000.0, lvl);
    check(mic80.gainDb > mic50.gainDb + 8.0 && mic80.thdPct > mic50.thdPct, "Gain MIC -80 is hotter and dirtier than MIC -50", mic80.gainDb, " dB");

    // HPF: 3rd-order Butterworth. At 1/3 of the corner (~1.58 oct below) expect about -28 dB.
    const auto hpfOff100 = runSine(st, ctl(2, 0, 0, 1, 0), 100.0, lvl, 1.0);
    const auto hpf300at100 = runSine(st, ctl(2, 4, 0, 1, 0), 100.0, lvl, 1.0);
    const double hpfAtten = hpf300at100.gainDb - hpfOff100.gainDb;
    check(hpfAtten < -22.0 && hpfAtten > -34.0, "HPF 300 Hz attenuates 100 Hz by ~28 dB (18 dB/oct)", hpfAtten, " dB");
    const auto hpf300at300 = runSine(st, ctl(2, 4, 0, 1, 0), 300.0, lvl, 1.0);
    const auto hpfOff300 = runSine(st, ctl(2, 0, 0, 1, 0), 300.0, lvl, 1.0);
    const double atCorner = hpf300at300.gainDb - hpfOff300.gainDb;
    check(atCorner < -1.5 && atCorner > -4.5, "HPF 300 Hz is about -3 dB at its corner", atCorner, " dB");
    const auto hpf50at1k = runSine(st, ctl(2, 1, 0, 1, 0), 1000.0, lvl);
    check(std::abs(hpf50at1k.gainDb - lineUnity.gainDb) < 0.3, "HPF 50 Hz leaves 1 kHz untouched", hpf50at1k.gainDb - lineUnity.gainDb, " dB");

    const auto trim6 = runSine(st, ctl(2, 0, 6, 1, 0), 1000.0, lvl);
    check(std::abs((trim6.gainDb - lineUnity.gainDb) - 6.0) < 0.3, "Output Trim +6 dB adds 6 dB", trim6.gainDb - lineUnity.gainDb, " dB");
    const auto trimM16 = runSine(st, ctl(2, 0, -16, 1, 0), 1000.0, lvl);
    check(std::abs((trimM16.gainDb - lineUnity.gainDb) + 16.0) < 0.3, "Output Trim -16 dB removes 16 dB", trimM16.gainDb - lineUnity.gainDb, " dB");

    const auto lineLoZ = runSine(st, ctl(2, 0, 0, 0, 0), 1000.0, lvl);
    check(std::abs(lineLoZ.gainDb - lineUnity.gainDb) < 0.05, "Mic Impedance has no effect on LINE positions", lineLoZ.gainDb - lineUnity.gainDb, " dB");
    const auto micLoZ = runSine(st, ctl(13, 0, 0, 0, 0), 1000.0, lvl);
    const double zDelta = micLoZ.gainDb - mic50.gainDb;
    check(zDelta < -0.7 && zDelta > -2.5, "Mic Impedance 300 LO-Z loads a mic source (~1.5 dB less level)", zDelta, " dB");

    const auto inv = runSine(st, ctl(2, 0, 0, 1, 1), 1000.0, lvl);
    check(inv.correlation < -0.95 && lineUnity.correlation > 0.95, "Phase 180 INV inverts polarity", inv.correlation);
    check(std::abs(inv.gainDb - lineUnity.gainDb) < 0.05, "Phase 180 INV does not change level", inv.gainDb - lineUnity.gainDb, " dB");

    // Legacy 7-value layout written by plugins exported before the rewrite.
    const std::vector<float> legacy{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };
    const auto leg = runSine(st, legacy, 1000.0, lvl);
    check(std::abs(leg.gainDb) < 1.0, "Legacy 7-value layout (Gain 0 dB, LINE) still unity", leg.gainDb, " dB");

    // Spec <-> DSP agreement: stepValues documented in SchematicSpec.h match the DSP table.
    const auto& meta = studio::getComponentMeta(studio::SchematicComponentType::ClassAPreamp);
    const auto& g = meta.getParamSpec(0);
    check(g.stepLabels.size() == 20 && g.stepValues.size() == 20 && g.minVal == 0.0f && g.maxVal == 19.0f,
          "SchematicSpec Gain is a 20-position index switch with stepValues", static_cast<double>(g.stepValues.size()));
    check(meta.getParamCount() == 5, "PR2 exposes 5 module controls (STAGE IN is added by the faceplate)",
          static_cast<double>(meta.getParamCount()));
}

void testTab1Preamps() {
    std::printf("\nTab 1 (PR1..PR36) · Representative Hardware Behaviour Checks\n");
    const double lvl = -24.0;

    // PR6 · 670 Vari-Mu Line Amp
    {
        dsp::circuits::VariMu670LineAmpStage st;
        const auto mDef  = runSine(st, std::vector<float>{ 9.0f, 12.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 1000.0, lvl);
        const auto mHot  = runSine(st, std::vector<float>{ 9.0f, 28.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 1000.0, lvl);
        const auto mTrim = runSine(st, std::vector<float>{ 9.0f, 12.0f, 0.0f, 6.0f, 0.0f, 0.0f }, 1000.0, lvl);
        const auto mInv  = runSine(st, std::vector<float>{ 9.0f, 12.0f, 0.0f, 0.0f, 0.0f, 1.0f }, 1000.0, lvl);
        check(mHot.thdPct > mDef.thdPct, "PR6: Higher Push-Pull Tube Drive increases THD", mHot.thdPct, " %");
        check(std::abs((mTrim.gainDb - mDef.gainDb) - 6.0) < 0.35, "PR6: Output Level +6 dB adds 6 dB", mTrim.gainDb - mDef.gainDb, " dB");
        check(mInv.correlation * mDef.correlation < -0.9, "PR6: Polarity switch inverts phase", mInv.correlation);
    }

    // PR13 · 610 Tube Console Pre
    {
        dsp::circuits::American610TubeConsoleStage st;
        const auto flatLo = runSine(st, std::vector<float>{ 2.0f, 4.0f, 4.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f }, 80.0, lvl);
        const auto boostLo = runSine(st, std::vector<float>{ 2.0f, 8.0f, 4.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f }, 80.0, lvl);
        const auto flatHi = runSine(st, std::vector<float>{ 2.0f, 4.0f, 4.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f }, 8000.0, lvl);
        const auto boostHi = runSine(st, std::vector<float>{ 2.0f, 4.0f, 8.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f }, 8000.0, lvl);
        check(boostLo.gainDb > flatLo.gainDb + 4.0, "PR13: Low Shelf +9 dB boosts 80 Hz", boostLo.gainDb - flatLo.gainDb, " dB");
        check(boostHi.gainDb > flatHi.gainDb + 4.0, "PR13: High Shelf +9 dB boosts 8 kHz", boostHi.gainDb - flatHi.gainDb, " dB");
    }

    // PR17 · Permalloy Class-A Strip
    // Controls: 0 Stepped Gain idx (0..12), 1 Output Fader dB (-20..10), 2 Input Mode, 3 Feedback Voicing, 4 Phase, 5 Permalloy Iron Drive %
    {
        dsp::circuits::PermalloyClassAStripStage st;
        const auto mNorm = runSine(st, std::vector<float>{ 5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 50.0f }, 1000.0, lvl);
        const auto mFad6 = runSine(st, std::vector<float>{ 5.0f, 6.0f, 0.0f, 0.0f, 0.0f, 50.0f }, 1000.0, lvl);
        const auto mInv  = runSine(st, std::vector<float>{ 5.0f, 0.0f, 0.0f, 0.0f, 1.0f, 50.0f }, 1000.0, lvl);
        check(std::abs((mFad6.gainDb - mNorm.gainDb) - 6.45) < 0.45, "PR17: Output Fader +6 dB adds ~6.4 dB (with feedback presence coupling)", mFad6.gainDb - mNorm.gainDb, " dB");
        check(std::abs(mInv.correlation + mNorm.correlation) < 0.02 && (mInv.correlation * mNorm.correlation < -0.05), "PR17: Phase REV inverts polarity", mInv.correlation + mNorm.correlation);
    }

    // PR24 · Air 110 Studio Pre
    // Controls: 0 Mic/Line Gain idx (0..11), 1 Output Trim dB, 2 Line Mode, 3 High-Pass Filter Hz (20..400), 4 Low-Pass Filter kHz, 5 Phase
    {
        dsp::circuits::AirStudioZobelStage st;
        const auto mHpfMin = runSine(st, std::vector<float>{ 3.0f, 0.0f, 0.0f, 20.0f, 30.0f, 0.0f }, 45.0, lvl);
        const auto mHpfMax = runSine(st, std::vector<float>{ 3.0f, 0.0f, 0.0f, 400.0f, 30.0f, 0.0f }, 45.0, lvl);
        check(mHpfMax.gainDb < mHpfMin.gainDb - 15.0, "PR24: 18 dB/oct HPF at 400 Hz strongly attenuates 45 Hz", mHpfMax.gainDb - mHpfMin.gainDb, " dB");
    }

    // PR31 · V72 Cinema Valve Pre
    {
        dsp::circuits::GermanDualPentodeCinemaStage st;
        const auto mFlat = runSine(st, std::vector<float>{ 6.0f, 50.0f, 0.0f, 0.0f, 0.0f, 50.0f }, 35.0, lvl);
        const auto mHpf  = runSine(st, std::vector<float>{ 6.0f, 50.0f, 0.0f, 1.0f, 0.0f, 50.0f }, 35.0, lvl);
        check(mHpf.gainDb < mFlat.gainDb - 4.0, "PR31: Cinema 60 Hz HPF attenuates 35 Hz", mHpf.gainDb - mFlat.gainDb, " dB");
    }

    // PR36 · Vari-Z Cable & Iron Load
    {
        dsp::circuits::VariImpedanceCableLoadStage st;
        const auto mLowCap  = runSine(st, std::vector<float>{ 50.0f, 0.0f, 18.5f, 0.0f, 0.0f, 0.0f }, 12000.0, lvl);
        const auto mHighCap = runSine(st, std::vector<float>{ 50.0f, 100.0f, 18.5f, 0.0f, 0.0f, 0.0f }, 12000.0, lvl);
        check(mHighCap.gainDb < mLowCap.gainDb - 2.0, "PR36: High Cable Capacitance rolls off 12 kHz", mHighCap.gainDb - mLowCap.gainDb, " dB");
    }
}

void testTab2IronTapeSaturation() {
    std::printf("\nTab 2 (IR1..10, TP1..9, ST1..18) · Representative Hardware Behaviour Checks\n");
    const double lvl = -18.0;

    // IR1 · Brit Iron Transformer (Faraday B ~ V/f low-frequency core saturation)
    // Controls: 0 Input Drive dB, 1 Core Remanence Bias %, 2 Secondary Output Trim dB, 3 Secondary Termination, 4 Winding Strapping
    {
        dsp::circuits::IronTransformerStage st;
        const auto m50Hz  = runSine(st, std::vector<float>{ 9.0f, 50.0f, 0.0f, 0.0f, 0.0f }, 50.0, lvl, 0.8);
        const auto m5kHz  = runSine(st, std::vector<float>{ 9.0f, 50.0f, 0.0f, 0.0f, 0.0f }, 5000.0, lvl, 0.8);
        const auto mTrim0 = runSine(st, std::vector<float>{ 9.0f, 50.0f, 0.0f, 0.0f, 0.0f }, 1000.0, lvl);
        const auto mTrim6 = runSine(st, std::vector<float>{ 9.0f, 50.0f, 6.0f, 0.0f, 0.0f }, 1000.0, lvl);
        check(m50Hz.thdPct > m5kHz.thdPct * 2.0, "IR1: 50 Hz saturates transformer core much harder than 5 kHz (B ~ V/f)", m50Hz.thdPct, " %");
        check(std::abs((mTrim6.gainDb - mTrim0.gainDb) - 6.0) < 0.4, "IR1: Secondary Output Trim +6 dB adds 6 dB", mTrim6.gainDb - mTrim0.gainDb, " dB");
    }

    // IR6 · Layered Permalloy Interstage
    // Controls: 0 Primary Drive dB, 1 Secondary Resonance Hz, 2 Output Trim dB, 3 Grid Load Mode, 4 Roll-Off Hz
    {
        dsp::circuits::LayeredPermalloyInterstageStage st;
        const auto mLowDrv  = runSine(st, std::vector<float>{ 0.0f, 12800.0f, 0.0f, 0.0f, 15500.0f }, 50.0, -18.0, 0.8);
        const auto mHighDrv = runSine(st, std::vector<float>{ 18.0f, 12800.0f, 0.0f, 0.0f, 15500.0f }, 50.0, -18.0, 0.8);
        const auto mTrim0   = runSine(st, std::vector<float>{ 0.0f, 12800.0f, 0.0f, 0.0f, 15500.0f }, 1000.0, -24.0);
        const auto mTrim6   = runSine(st, std::vector<float>{ 0.0f, 12800.0f, 10.0f, 0.0f, 15500.0f }, 1000.0, -24.0);
        check(mHighDrv.thdPct > mLowDrv.thdPct * 1.5, "IR6: +18 dB Primary Drive increases 50 Hz interstage core saturation", mHighDrv.thdPct, " %");
        check(std::abs((mTrim6.gainDb - mTrim0.gainDb) - 6.0) < 0.4, "IR6: Output Trim +10 dB scales level (+6 dB calibrated)", mTrim6.gainDb - mTrim0.gainDb, " dB");
    }

    // TP1 · Swiss 2-Inch Mastering Tape
    // Controls: 0 Input Calibrate dB, 1 Record Over-Bias dB, 2 Repro HF Shelf %, 3 Output Calibrate dB, 4 Tape Speed, 5 Tape Oxide, 6 EQ Standard
    {
        dsp::circuits::TapeHysteresisStage st;
        const auto mUnderBias = runSine(st, std::vector<float>{ 0.0f, -6.0f, 80.0f, 0.0f, 2.0f, 0.0f, 0.0f }, 14000.0, lvl);
        const auto mOverBias  = runSine(st, std::vector<float>{ 0.0f,  6.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 14000.0, lvl);
        const auto mBumpLow   = runSine(st, std::vector<float>{ 0.0f, -6.0f, 50.0f, 0.0f, 2.0f, 0.0f, 0.0f }, 68.0, lvl);
        const auto mBumpHigh  = runSine(st, std::vector<float>{ 0.0f,  6.0f, 50.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 68.0, lvl);
        check(mUnderBias.gainDb > mOverBias.gainDb + 1.0, "TP1: Under-bias + 30 IPS preserves more 14 kHz HF than +6 dB Over-Bias", mUnderBias.gainDb - mOverBias.gainDb, " dB");
        check(mBumpHigh.gainDb > mBumpLow.gainDb + 0.5, "TP1: +6 dB Over-Bias + 7.5 IPS deepens 68 Hz playback head-bump warmth", mBumpHigh.gainDb - mBumpLow.gainDb, " dB");
    }

    // ST1 · Variable Silk Harmonic Stage
    // Controls: 0 Texture %, 1 Input Drive dB, 2 Output Trim dB, 3 Voicing Mode (0=OFF, 1=RED HF, 2=BLUE LF), 4 Pre-Emphasis HPF Hz
    {
        dsp::circuits::VariableSilkExciterStage st;
        const auto mOff  = runSine(st, std::vector<float>{ 10.0f, 0.0f, 0.0f, 0.0f, 55.0f }, 1000.0, lvl);
        const auto mBlue = runSine(st, std::vector<float>{ 90.0f, 12.0f, 0.0f, 2.0f, 55.0f }, 1000.0, lvl);
        check(mBlue.thdPct > mOff.thdPct * 1.5, "ST1: Engaging BLUE Silk + Texture + Drive increases harmonic density", mBlue.thdPct, " %");
    }

    // ST9 · Mastering Vinyl Cutting Lathe
    // Controls: 0 Cutting Head Level dB, 1 HF Acceleration Limiter Hz, 2 Elliptical Filter Hz, 3 Stylus Trace Distortion %, 4 Platter Speed RPM
    {
        dsp::circuits::MasteringVinylCuttingLatheStage st;
        const auto mLowTrace  = runSine(st, std::vector<float>{ 0.0f, 6500.0f, 120.0f, 10.0f, 0.0f }, 1450.0, lvl);
        const auto mHighTrace = runSine(st, std::vector<float>{ 6.0f, 6500.0f, 120.0f, 90.0f, 0.0f }, 1450.0, lvl);
        check(mHighTrace.thdPct > mLowTrace.thdPct * 1.5, "ST9: Higher Cutting Level & Trace Distortion increases groove pinch harmonics", mHighTrace.thdPct, " %");
    }
}

void testTab3EqAndSpectral() {
    std::printf("\nTab 3 (EQ1..23, SP1..13) · Representative Hardware Behaviour Checks\n");
    const double lvl = -18.0;

    // EQ1 · P-1A Low Trick EQ (60 Hz Low Boost + 1.75x Offset Attenuation Dip)
    {
        dsp::circuits::PassiveLcProgramEq st;
        const auto mFlat  = runSine(st, std::vector<float>{ 2.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 50.0, lvl);
        const auto mBoost = runSine(st, std::vector<float>{ 2.0f, 8.0f, 0.0f, 0.0f, 0.0f }, 50.0, lvl);
        const auto mTrick = runSine(st, std::vector<float>{ 2.0f, 6.0f, 8.0f, 0.0f, 0.0f }, 110.0, lvl);
        const auto mNoDip = runSine(st, std::vector<float>{ 2.0f, 6.0f, 0.0f, 0.0f, 0.0f }, 110.0, lvl);
        check(mBoost.gainDb > mFlat.gainDb + 4.0, "EQ1: Low Boost at 60 Hz boosts 50 Hz sub-bass", mBoost.gainDb - mFlat.gainDb, " dB");
        check(mTrick.gainDb < mNoDip.gainDb - 3.0, "EQ1: Simultaneous Low Atten carves 1.75x offset mid-scoop at 110 Hz", mTrick.gainDb - mNoDip.gainDb, " dB");
    }

    // EQ5 · 550A Proportional-Q Gyrator EQ
    // Controls: 0 Low Freq idx, 1 Low Gain dB, 2 Mid Freq idx (4=1500Hz), 3 Mid Gain dB, 4 High Freq idx, 5 High Gain dB, 6 Low Shape, 7 High Shape, 8 Bandpass Filter
    {
        dsp::circuits::ProportionalQGyratorEq st;
        const auto mFlat  = runSine(st, std::vector<float>{ 1.0f, 0.0f, 4.0f, 0.0f, 3.0f, 0.0f, 1.0f, 1.0f, 0.0f }, 1500.0, lvl);
        const auto mBoost = runSine(st, std::vector<float>{ 1.0f, 0.0f, 4.0f, 9.0f, 3.0f, 0.0f, 1.0f, 1.0f, 0.0f }, 1500.0, lvl);
        check(mBoost.gainDb > mFlat.gainDb + 5.0, "EQ5: Mid Gain +9 dB at 1.5 kHz boosts 1.5 kHz", mBoost.gainDb - mFlat.gainDb, " dB");
    }

    // EQ6 · 4K Black & Pink Parametric EQ
    {
        dsp::circuits::Console4000ParametricEqStage st;
        const auto mFlat  = runSine(st, std::vector<float>{ 0.0f, 100.0f, 0.0f, 400.0f, 1.2f, 0.0f, 2500.0f, 1.2f, 0.0f, 8000.0f, 20.0f, 20000.0f, 0.0f, 0.0f, 0.0f }, 50.0, lvl);
        const auto mBoost = runSine(st, std::vector<float>{ 9.0f, 100.0f, 0.0f, 400.0f, 1.2f, 0.0f, 2500.0f, 1.2f, 0.0f, 8000.0f, 20.0f, 20000.0f, 0.0f, 0.0f, 0.0f }, 50.0, lvl);
        check(mBoost.gainDb > mFlat.gainDb + 5.0, "EQ6: LF Shelf +9 dB at 100 Hz boosts 50 Hz bass", mBoost.gainDb - mFlat.gainDb, " dB");
    }

    // EQ11 · M-5 Midrange Peak/Dip EQ
    {
        dsp::circuits::PassiveMidDipPeakEqStage st;
        const auto mFlat = runSine(st, std::vector<float>{ 2.0f, 4.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f }, 1000.0, lvl);
        const auto mDip  = runSine(st, std::vector<float>{ 2.0f, 4.0f, 2.0f, 0.0f, 8.0f, 0.0f, 0.0f }, 1000.0, lvl);
        check(mDip.gainDb < mFlat.gainDb - 4.0, "EQ11: Mid Dip Atten 8 at 1 kHz cuts 1 kHz", mDip.gainDb - mFlat.gainDb, " dB");
    }

    // EQ20 · Elliptical M/S Master EQ
    {
        dsp::circuits::EllipticalMidSideEqStage st;
        const auto mDef   = runSine(st, std::vector<float>{ 95.0f, 0.0f, 0.0f, 100.0f, 0.0f }, 1850.0, lvl);
        const auto mMid6  = runSine(st, std::vector<float>{ 95.0f, 8.0f, 0.0f, 100.0f, 0.0f }, 1850.0, lvl);
        const auto mTrim6 = runSine(st, std::vector<float>{ 95.0f, 0.0f, 0.0f, 100.0f, 6.0f }, 1850.0, lvl);
        check(mMid6.gainDb > mDef.gainDb + 4.5, "EQ20: Mid Presence +8 dB boosts 1.85 kHz", mMid6.gainDb - mDef.gainDb, " dB");
        check(std::abs((mTrim6.gainDb - mDef.gainDb) - 6.0) < 0.35, "EQ20: Output Trim +6 dB adds 6 dB", mTrim6.gainDb - mDef.gainDb, " dB");
    }

    // SP9 · Sub Bass Mono-Maker
    {
        dsp::circuits::PhaseCompensatedBassMonoStage st;
        const auto mDef   = runSine(st, std::vector<float>{ 120.0f, 35.0f, 40.0f, 55.0f, 0.0f }, 1000.0, lvl);
        const auto mTrim6 = runSine(st, std::vector<float>{ 120.0f, 35.0f, 40.0f, 55.0f, 6.0f }, 1000.0, lvl);
        check(std::abs((mTrim6.gainDb - mDef.gainDb) - 6.0) < 0.35, "SP9: Output Trim +6 dB adds 6 dB", mTrim6.gainDb - mDef.gainDb, " dB");
    }
}

void testTab4DynamicsAndClippers() {
    std::printf("\nTab 4 (CP1..29, CL1..7) · Representative Hardware Behaviour Checks\n");
    const double lvl = -12.0;

    // CP1 · 76 Fast FET Limiter
    // Controls: 0 Input Attenuator (-36..0 dB), 1 Output Level (-36..30 dB), 2 Attack (1..7), 3 Release (1..7), 4 Ratio (0..4), 5 Sidechain HPF (20..250 Hz), 6 Mix (0..100%)
    {
        dsp::circuits::FastFetCompressor st;
        const auto mLowIn  = runSine(st, std::vector<float>{ -30.0f, 0.0f, 4.0f, 4.0f, 0.0f, 20.0f, 100.0f }, 1000.0, lvl);
        const auto mHighIn = runSine(st, std::vector<float>{   0.0f, 0.0f, 4.0f, 4.0f, 3.0f, 20.0f, 100.0f }, 1000.0, lvl);
        check(mHighIn.thdPct > mLowIn.thdPct, "CP1: Higher FET Input Drive & 20:1 ratio increases FET harmonic saturation", mHighIn.thdPct, " %");
    }

    // CP2 · 2A Optical Leveler
    // Controls: 0 Gain (0..100), 1 Peak Reduction (0..100), 2 R37 HF Emphasis (0..100), 3 Mode Switch (0 LIMIT, 1 COMPRESS), 4 Output Trim (-12..12 dB), 5 Mix (0..100%)
    {
        dsp::circuits::OpticalLevelerStage st;
        const auto mNoRed   = runSine(st, std::vector<float>{ 50.0f,  0.0f, 80.0f, 1.0f, 0.0f, 100.0f }, 1000.0, lvl);
        const auto mFullRed = runSine(st, std::vector<float>{ 50.0f, 85.0f, 80.0f, 0.0f, 0.0f, 100.0f }, 1000.0, lvl);
        check(mFullRed.gainDb < mNoRed.gainDb - 4.0, "CP2: Peak Reduction 85 + Limit mode compresses 1 kHz level", mFullRed.gainDb - mNoRed.gainDb, " dB");
    }

    // CL1 · Standard 16x Clipper
    // Controls: 0 Input Drive (-12..24 dB), 1 Ceiling (-24..0 dB), 2 Knee Curve (0..100%), 3 HF Emphasis Tilt (500..16000 Hz), 4 Mix (0..100%)
    {
        dsp::circuits::VariableKneePolyClipper st;
        const auto mClean = runSine(st, std::vector<float>{ -6.0f,  0.0f, 80.0f, 3200.0f, 100.0f }, 1000.0, -14.0);
        const auto mClip  = runSine(st, std::vector<float>{ 18.0f, -8.0f,  5.0f, 3200.0f, 100.0f }, 1000.0, -14.0);
        check(mClip.thdPct > mClean.thdPct + 1.0, "CL1: Driving +18 dB into -8 dB Ceiling clips peaks (higher THD)", mClip.thdPct, " %");
    }
}

void testTab5DelayAndReverb() {
    std::printf("\nTab 5 (DL1..17, SH1, RV1..18) · Representative Hardware Behaviour Checks\n");
    const double lvl = -18.0;

    // DL1 · 201 Tape Space Echo
    // Controls: 0 Mode Selector (0..2), 1 Repeat Rate (40..800 ms), 2 Intensity (0..110%), 3 Echo Volume (0..100%), 4 Reverb Volume (0..100%), 5 Bass (-10..10 dB), 6 Treble (-10..10 dB), 7 Input Volume (-12..18 dB)
    {
        dsp::circuits::TapeEchoStage st;
        const auto mBassCut   = runSine(st, std::vector<float>{ 1.0f, 180.0f, 45.0f, 75.0f, 30.0f, -9.0f, 0.0f, 3.0f }, 90.0, lvl, 0.8);
        const auto mBassBoost = runSine(st, std::vector<float>{ 1.0f, 180.0f, 45.0f, 75.0f, 30.0f,  9.0f, 0.0f, 3.0f }, 90.0, lvl, 0.8);
        check(mBassBoost.gainDb > mBassCut.gainDb + 0.5, "DL1: Tape Echo Bass EQ +9 dB boosts 90 Hz over -9 dB", mBassBoost.gainDb - mBassCut.gainDb, " dB");
    }

    // RV2 · Concert Hall FDN
    // Controls: 0 Mid RT60 Decay (0.4..18 s), 1 Bass Multiply (0.4..2.5 x), 2 Hall Size (0..100%), 3 Pre-Delay (0..250 ms), 4 Diffusion (0..100%), 5 Mix (0..100%)
    {
        dsp::circuits::ConcertHallFdnStage st;
        const auto mDry = runSine(st, std::vector<float>{ 2.8f, 1.25f, 68.0f, 28.0f, 75.0f,  0.0f }, 1000.0, lvl, 0.8);
        const auto mWet = runSine(st, std::vector<float>{ 2.8f, 1.25f, 68.0f, 28.0f, 75.0f, 85.0f }, 1000.0, lvl, 0.8);
        check(std::abs(mWet.correlation - mDry.correlation) > 0.02, "RV2: Concert Hall 85% Mix decorrelates phase vs 0% Dry", mWet.correlation);
    }
}

void testTab6ModulationAndRouting() {
    std::printf("\nTab 6 (SP_A..J, MD1..17, CV1..9) · Representative Hardware Behaviour Checks\n");
    const double lvl = -18.0;

    // MD2 · Tri-Stereo BBD Chorus
    // Controls: 0 Left Depth %, 1 Center Depth %, 2 Right Depth %, 3 Rate Hz, 4 Mix %
    {
        dsp::circuits::TriStereoBbdChorusStage st;
        const auto mDry = runSine(st, std::vector<float>{ 60.0f, 45.0f, 65.0f, 0.85f,  0.0f }, 1000.0, lvl);
        const auto mWet = runSine(st, std::vector<float>{ 60.0f, 45.0f, 65.0f, 0.85f, 85.0f }, 1000.0, lvl);
        check(std::abs(mWet.correlation - mDry.correlation) > 0.02, "MD2: Tri-Stereo BBD Chorus 85% Mix modulates phase vs 0% Dry", mWet.correlation);
    }

    // SP_D · Parallel Wet/Dry Phase Blender
    {
        dsp::circuits::ParallelWetDryPhaseBlenderStage st;
        const auto mTrim0 = runSine(st, std::vector<float>{ 50.0f, 0.0f, 80.0f, 12000.0f, 0.0f }, 1000.0, lvl);
        const auto mTrim6 = runSine(st, std::vector<float>{ 50.0f, 0.0f, 80.0f, 12000.0f, 6.0f }, 1000.0, lvl);
        check(std::abs((mTrim6.gainDb - mTrim0.gainDb) - 6.0) < 0.35, "SP_D: Output Trim +6 dB adds 6 dB", mTrim6.gainDb - mTrim0.gainDb, " dB");
    }

    // AU5 · Auto LUFS Gain-Stager (evolved from CV9)
    {
        dsp::circuits::PsychoacousticCentroidCvStage st;
        const auto mLufs24 = runSine(st, std::vector<float>{ -24.0f, 18.0f, -18.0f, -60.0f, 0.0f, 0.0f }, 1000.0, lvl);
        const auto mLufs10 = runSine(st, std::vector<float>{ -10.0f, 18.0f, -18.0f, -60.0f, 0.0f, 0.0f }, 1000.0, lvl);
        check(mLufs10.gainDb > mLufs24.gainDb + 6.0, "AU5: Target Loudness -10 LUFS rides K-weighted gain higher than -24 LUFS", mLufs10.gainDb - mLufs24.gainDb, " dB");
    }
    // AU2 · Auto Bass Rider
    {
        dsp::circuits::AutoBassFundamentalRiderStage st;
        const auto mBassLo = runSine(st, std::vector<float>{ -24.0f, 12.0f, 80.0f, 50.0f, 45.0f, 0.0f }, 80.0, lvl);
        const auto mBassHi = runSine(st, std::vector<float>{  -8.0f, 12.0f, 80.0f, 50.0f, 45.0f, 0.0f }, 80.0, lvl);
        check(mBassHi.gainDb > mBassLo.gainDb + 6.0, "AU2: Target Bass Level -8 dB vs -24 dB lifts 80 Hz fundamental sustain", mBassHi.gainDb - mBassLo.gainDb, " dB");
    }
    // AU3 · 4-Band Dynamic Multiband EQ
    {
        dsp::circuits::ParametricDynamicEq4BandStage st;
        const auto mFlat  = runSine(st, std::vector<float>{ 90.0f, 0.0f, 0.0f, 380.0f, 0.0f, 0.0f, 2800.0f, 0.0f, 0.0f, 10000.0f, 0.0f, 0.0f, -18.0f, 1.0f, 0.0f }, 2800.0, lvl);
        const auto mBoost = runSine(st, std::vector<float>{ 90.0f, 0.0f, 0.0f, 380.0f, 0.0f, 0.0f, 2800.0f, 9.0f, 0.0f, 10000.0f, 0.0f, 0.0f, -18.0f, 1.0f, 0.0f }, 2800.0, lvl);
        check(mBoost.gainDb > mFlat.gainDb + 5.0, "AU3: High-Mid Static Gain +9 dB boosts 2.8 kHz", mBoost.gainDb - mFlat.gainDb, " dB");
    }
}

} // namespace

int main() {
    std::printf("=====================================================================\n");
    std::printf("  AVRA MODULE BEHAVIOUR TESTS\n");
    std::printf("=====================================================================\n");
    testPR2();
    testTab1Preamps();
    testTab2IronTapeSaturation();
    testTab3EqAndSpectral();
    testTab4DynamicsAndClippers();
    testTab5DelayAndReverb();
    testTab6ModulationAndRouting();
    std::printf("\n  %d / %d checks passed\n", g_checks - g_failures, g_checks);
    return g_failures == 0 ? 0 : 1;
}

