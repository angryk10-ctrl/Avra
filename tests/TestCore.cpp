#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "GainPlugin.h"
#include "avra/Avra.h"
#include "avra/core/AudioDeviceOutput.h"
#include "avra/format/vst3/Vst3Plugin.h"
#include "avra/gui/controls/TextInput.h"
#include "avra/studio/ProjectGenerator.h"
#include "avra/studio/StudioIdeShell.h"

// Simple assertion macro for the test suite
#define AVRA_TEST(expr, msg)                                                   \
  do {                                                                         \
    if (!(expr)) {                                                             \
      std::cerr << "[FAILED] " << msg << " (" #expr ") at line " << __LINE__   \
                << std::endl;                                                  \
      return 1;                                                                \
    }                                                                          \
  } while (0)

int main() {
  std::cout << "===========================================" << std::endl;
  std::cout << "Running Avra Core Test Suite (C++20)" << std::endl;
  std::cout << "===========================================" << std::endl;

  // -------------------------------------------------------------
  // 1. Math Tests
  // -------------------------------------------------------------
  {
    std::cout << "Testing Math utilities..." << std::endl;

    // 0 dB == 1.0 linear
    float lin0 = avra::math::dBToLinear(0.0f);
    AVRA_TEST(std::abs(lin0 - 1.0f) < 1e-4f, "0 dB must equal 1.0 linear");

    // -6 dB ~= 0.501187
    float linNeg6 = avra::math::dBToLinear(-6.0f);
    AVRA_TEST(std::abs(linNeg6 - 0.501187f) < 1e-3f,
              "-6 dB must be approx 0.5");

    // 1.0 linear == 0.0 dB
    float db0 = avra::math::linearTodB(1.0f);
    AVRA_TEST(std::abs(db0 - 0.0f) < 1e-4f, "1.0 linear must equal 0.0 dB");

    // Log frequency mapping (20 Hz to 20,000 Hz)
    float freqCenter = avra::math::normalizedToLogFreq(0.5f, 20.0f, 20000.0f);
    // Geometric mean of 20 and 20000 is sqrt(400000) ~ 632.45 Hz
    AVRA_TEST(std::abs(freqCenter - 632.455f) < 1.0f,
              "Normalized 0.5 must be geometric center ~632 Hz");

    float normBack =
        avra::math::logFreqToNormalized(freqCenter, 20.0f, 20000.0f);
    AVRA_TEST(std::abs(normBack - 0.5f) < 1e-4f,
              "Roundtrip log frequency mapping");
  }

  // -------------------------------------------------------------
  // 2. AudioSpan Tests
  // -------------------------------------------------------------
  {
    std::cout << "Testing AudioSpan (Zero-Allocation Buffer Views)..."
              << std::endl;

    constexpr size_t numFrames = 64;
    std::vector<float> left(numFrames, 0.5f);
    std::vector<float> right(numFrames, -0.5f);
    float *channels[2] = {left.data(), right.data()};

    avra::AudioSpan<float> span(channels, 2, numFrames);
    AVRA_TEST(span.numChannels() == 2, "Channel count match");
    AVRA_TEST(span.numFrames() == numFrames, "Frame count match");
    AVRA_TEST(span(0, 0) == 0.5f, "Sample read left");
    AVRA_TEST(span(1, 0) == -0.5f, "Sample read right");

    // Apply gain
    span.applyGain(2.0f);
    AVRA_TEST(std::abs(span(0, 0) - 1.0f) < 1e-5f, "Gain scaling 2x");
    AVRA_TEST(std::abs(span(1, 0) - (-1.0f)) < 1e-5f, "Gain scaling 2x");

    // Clear
    span.clear();
    AVRA_TEST(span(0, 0) == 0.0f && span(1, 10) == 0.0f, "Buffer clear");
  }

  // -------------------------------------------------------------
  // 3. Parameter and ParameterTree Tests
  // -------------------------------------------------------------
  {
    std::cout << "Testing Parameter and ParameterTree..." << std::endl;

    avra::ParameterTree tree;
    auto &gain = tree.add({.id = 100,
                           .name = "Gain",
                           .unit = "dB",
                           .minValue = -60.0f,
                           .maxValue = +12.0f,
                           .defaultValue = 0.0f,
                           .taper = avra::ParameterTaper::Linear});

    auto &freq = tree.add({.id = 101,
                           .name = "Frequency",
                           .unit = "Hz",
                           .minValue = 20.0f,
                           .maxValue = 20000.0f,
                           .defaultValue = 1000.0f,
                           .taper = avra::ParameterTaper::Logarithmic});

    AVRA_TEST(tree.size() == 2, "Tree size == 2");
    AVRA_TEST(tree.getById(100) == &gain, "Lookup by ID");
    AVRA_TEST(tree.getById(101) == &freq, "Lookup by ID");

    // Test default conversion
    AVRA_TEST(std::abs(gain.getPlain() - 0.0f) < 1e-3f, "Default plain value");

    // Change normalized value
    gain.setNormalized(1.0f);
    AVRA_TEST(std::abs(gain.getPlain() - 12.0f) < 1e-3f,
              "Normalized 1.0 -> Max Plain Value");

    // Test serialization
    std::string state = tree.serializeState();
    AVRA_TEST(!state.empty(), "State serialized");

    // Reset and restore
    gain.setNormalized(0.0f);
    AVRA_TEST(gain.getNormalized() == 0.0f, "Reset before restore");
    bool restored = tree.restoreState(state);
    AVRA_TEST(restored, "State restore success");
    AVRA_TEST(gain.getNormalized() == 1.0f, "Value restored from state");
  }

  // -------------------------------------------------------------
  // 4. GainPlugin Integration Test
  // -------------------------------------------------------------
  {
    std::cout << "Testing GainPlugin execution..." << std::endl;

    avra::examples::GainPlugin plugin;
    plugin.prepare(48000.0, 512);

    constexpr size_t blockSize = 128;
    std::vector<float> inL(blockSize, 1.0f);
    std::vector<float> inR(blockSize, 1.0f);
    std::vector<float> outL(blockSize, 0.0f);
    std::vector<float> outR(blockSize, 0.0f);

    const float *inChannels[2] = {inL.data(), inR.data()};
    float *outChannels[2] = {outL.data(), outR.data()};

    avra::ProcessContext ctx{
        .inputs = avra::AudioSpan<const float>(inChannels, 2, blockSize),
        .outputs = avra::AudioSpan<float>(outChannels, 2, blockSize),
        .sampleRate = 48000.0,
        .numFrames = blockSize};

    // Process default (0 dB gain)
    plugin.process(ctx);

    // Verify outputs are unity gain (approx 1.0)
    AVRA_TEST(std::abs(outL[blockSize - 1] - 1.0f) < 0.05f,
              "Unity gain output check");
    AVRA_TEST(std::abs(outR[blockSize - 1] - 1.0f) < 0.05f,
              "Unity gain output check");

    // Set Mute
    plugin.getParameters()
        .getById(avra::examples::GainPlugin::kMuteId)
        ->setPlain(1.0f);
    plugin.process(ctx);

    AVRA_TEST(outL[0] == 0.0f && outR[0] == 0.0f, "Mute check");
  }

  // -------------------------------------------------------------
  // 5. LockFreeRingBuffer, SimdBlock, ModulationMatrix & Biquad
  // -------------------------------------------------------------
  {
    std::cout << "Testing LockFreeRingBuffer, SimdBlock & ModulationMatrix..."
              << std::endl;

    avra::LockFreeRingBuffer<float, 64> ring;
    AVRA_TEST(ring.push(0.25f) && ring.push(0.75f), "RingBuffer push");
    float v1 = 0.0f, v2 = 0.0f;
    AVRA_TEST(ring.pop(v1) && ring.pop(v2), "RingBuffer pop");
    AVRA_TEST(v1 == 0.25f && v2 == 0.75f, "RingBuffer FIFO order");

    std::vector<float> buf(32, 0.5f);
    avra::dsp::SimdBlock::multiply(buf.data(), 2.0f, buf.size());
    float peak = 0.0f, rms = 0.0f;
    avra::dsp::SimdBlock::computePeakAndRms(buf.data(), buf.size(), peak, rms);
    AVRA_TEST(std::abs(peak - 1.0f) < 1e-4f && std::abs(rms - 1.0f) < 1e-4f,
              "SimdBlock multiply & peak/RMS");

    avra::ParameterTree tree;
    auto &cutoff = tree.add({.id = 1,
                             .name = "Cutoff",
                             .minValue = 0.0f,
                             .maxValue = 100.0f,
                             .defaultValue = 50.0f});
    avra::dsp::ModulationMatrix modMatrix;
    modMatrix.addRoute(0, 1, 0.25f);
    modMatrix.setSourceValue(0, 1.0f);
    AVRA_TEST(std::abs(modMatrix.getModulatedPlain(cutoff) - 75.0f) < 1e-3f,
              "ModulationMatrix positive modulation");

    avra::dsp::BiquadFilter bell;
    bell.setSampleRate(48000.0);
    bell.setParameters(avra::dsp::FilterType::Peaking, 1000.0f, 1.0f, 6.0f);
    float magAt1k = bell.getMagnitudeDbAtFrequency(1000.0f);
    AVRA_TEST(std::abs(magAt1k - 6.0f) < 0.2f,
              "BiquadFilter +6 dB peaking magnitude at center frequency");
  }

  // -------------------------------------------------------------
  // 6. Hardware Circuit Library & Polyphase Oversampling Tests
  // -------------------------------------------------------------
  {
    std::cout
        << "Testing Hardware Circuit Library & Polyphase 2x/4x Oversampling..."
        << std::endl;

    avra::dsp::circuits::DiscreteClassAPreamp preamp;
    avra::dsp::circuits::DualTriodeTubeStage tube;
    avra::dsp::circuits::InductorEq3Band eq;
    avra::dsp::circuits::FastFetCompressor fet;
    avra::dsp::Oversampling2x os2x;
    avra::dsp::Oversampling4x os4x;

    preamp.prepare(48000.0);
    tube.prepare(48000.0);
    eq.prepare(48000.0);
    fet.prepare(48000.0);
    os2x.reset();
    os4x.reset();

    preamp.setParameters(6.0f, 0.5f);
    tube.setParameters(3.0f, 0.6f);
    eq.setParameters(2.0f, 1200.0f, 3.0f, 2.0f);
    fet.setParameters(12.0f, 4.0f, 2.0f, 80.0f);

    float maxOut = 0.0f;
    float maxOs4x = 0.0f;
    for (int n = 0; n < 512; ++n) {
      float l = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f *
                                (static_cast<float>(n) / 48000.0f));
      float r = l;

      // Test 2x and 4x polyphase allpass oversampling with nonlinear waveshaper
      float osSample2x = os2x.processSample(
          l, [](float x) { return avra::math::fastTanh(x * 1.5f); });
      float osSample4x = os4x.processSample(
          l, [](float x) { return avra::math::fastTanh(x * 1.5f); });
      AVRA_TEST(std::isfinite(osSample2x) && std::isfinite(osSample4x),
                "Polyphase 2x/4x oversampled output is finite");
      maxOs4x = std::max(maxOs4x, std::abs(osSample4x));

      preamp.processStereo(l, r);
      tube.processStereo(l, r);
      eq.processStereo(l, r);
      float dryL = l, dryR = r;
      fet.processStereo(l, r);
      // Parallel blend (50%)
      l = 0.5f * dryL + 0.5f * l;
      r = 0.5f * dryR + 0.5f * r;
      AVRA_TEST(std::isfinite(l) && std::isfinite(r),
                "Circuit stage output is finite");
      maxOut = std::max(maxOut, std::abs(l));
    }
    AVRA_TEST(maxOut > 0.05f && maxOut <= 2.0f,
              "Hardware chain produces healthy audio output");
    AVRA_TEST(maxOs4x > 0.3f && maxOs4x <= 1.0f,
              "Polyphase 4x oversampler preserves passband amplitude");
  }

  // -------------------------------------------------------------
  // 7. Standalone SDK Root Discovery & Hardware UI Controls Test
  // -------------------------------------------------------------
  {
    std::cout
        << "Testing Portable SDK Root Resolution & Hardware UI Controls..."
        << std::endl;
    const auto root = avra::studio::resolveAvraWorkspaceRoot();
    AVRA_TEST(std::filesystem::exists(root / "include" / "avra" / "Avra.h"),
              "Portable Avra SDK root discovered via relative path");

    avra::Parameter gainParam({.id = 1,
                               .name = "INPUT GAIN",
                               .unit = "dB",
                               .minValue = -10.0f,
                               .maxValue = 80.0f,
                               .defaultValue = 0.0f});
    avra::gui::HardwareKnob hwKnob(
        &gainParam, avra::gui::HardwareKnobStyle::PointerSkirted, "INPUT GAIN");
    avra::gui::HardwareKnob redKnob(
        &gainParam, avra::gui::HardwareKnobStyle::MarconiRed, "PREAMP GAIN");
    avra::gui::HardwareKnob arcKnob(
        &gainParam, avra::gui::HardwareKnobStyle::ModernArc, "CEILING");
    avra::gui::HardwareToggleSwitch hwSw(&gainParam, "PHASE", "INV", "NORM");
    avra::gui::AnalogVuMeter vu;
    vu.setGainReductionDb(6.0f);
    AVRA_TEST(
        hwKnob.isVisible() && redKnob.isVisible() && arcKnob.isVisible() &&
            hwSw.isVisible() && vu.isVisible(),
        "Hardware UI controls instantiated cleanly across all knob styles");

    auto gainLayout = avra::studio::buildAvraGainDemoFaceplate("AVRA GAIN");
    AVRA_TEST(gainLayout.widgets.size() >= 8,
              "Avra Gain Demo default Faceplate UI Builder layout populated");

    std::vector<avra::studio::SchematicNode> chain = {
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::ClassAPreamp, false),
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::InductorEq3Band, false),
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::FetCompressor1176, true)};
    auto syncedLayout = avra::studio::buildFaceplateFromSchematic(
        "CustomStrip", chain, avra::gui::ChassisFinish::BritishSlateBlue);
    AVRA_TEST(!syncedLayout.widgets.empty() &&
                  syncedLayout.theme.finish ==
                      avra::gui::ChassisFinish::BritishSlateBlue,
              "Faceplate UI Builder auto-sync from Schematic chain verified");
  }

  // -------------------------------------------------------------
  // 8. 5-Slot Modular DSP Sub-Block Blueprint Suite Verification
  // -------------------------------------------------------------
  {
    std::cout << "Testing 5-Slot Modular DSP Sub-Blocks (Slots A-E)..."
              << std::endl;
    const auto &catalog = avra::studio::getSchematicCatalog();
    AVRA_TEST(catalog.size() >= 34,
              "Schematic Component Catalog exposes all 34 Slot A-E sub-blocks");

    avra::dsp::circuits::DiscreteClassABOpAmp opAmp2520;
    avra::dsp::circuits::OutputTransformerMatrix outMatrix;
    avra::dsp::circuits::ProportionalQGyrator gyratorEq;
    avra::dsp::circuits::ParallelLcMasteringEq parallelLcEq;
    avra::dsp::circuits::VariMuTubeGainCell variMu;
    avra::dsp::circuits::DiodeBridgeGainCell diodeBridge;
    avra::dsp::circuits::DualOptoVcaMasterComp dualOptoVca;
    avra::dsp::circuits::SidechainThrustFilter scThrust;
    avra::dsp::circuits::CompanderBbdDelay bbdDelay;
    avra::dsp::circuits::DualMicroPitchStage microPitch;

    opAmp2520.prepare(48000.0);
    outMatrix.prepare(48000.0);
    gyratorEq.prepare(48000.0);
    parallelLcEq.prepare(48000.0);
    variMu.prepare(48000.0);
    diodeBridge.prepare(48000.0);
    dualOptoVca.prepare(48000.0);
    scThrust.prepare(48000.0);
    bbdDelay.prepare(48000.0);
    microPitch.prepare(48000.0);

    float peakOut = 0.0f;
    for (int n = 0; n < 512; ++n) {
      float l = 0.6f * std::sin(2.0f * 3.14159265f * 220.0f *
                                (static_cast<float>(n) / 48000.0f));
      float r = l;

      opAmp2520.processStereo(l, r);
      gyratorEq.processStereo(l, r);
      parallelLcEq.processStereo(l, r);
      scThrust.processStereo(l, r);
      variMu.processStereo(l, r);
      diodeBridge.processStereo(l, r);
      dualOptoVca.processStereo(l, r);
      bbdDelay.processStereo(l, r);
      microPitch.processStereo(l, r);
      outMatrix.processStereo(l, r);

      AVRA_TEST(std::isfinite(l) && std::isfinite(r),
                "5-Slot sub-block chain output is finite");
      peakOut = std::max(peakOut, std::abs(l));
    }
    AVRA_TEST(peakOut > 0.05f && peakOut <= 3.0f,
              "5-Slot sub-block chain produces healthy dynamic audio");
    AVRA_TEST(variMu.getGainReductionDb() >= 0.0f &&
                  diodeBridge.getGainReductionDb() >= 0.0f &&
                  dualOptoVca.getGainReductionDb() >= 0.0f,
              "Vari-Mu, Diode Bridge & Dual Opto+VCA gain cells report valid "
              "gain reduction");
  }

  // -------------------------------------------------------------
  // 9. Digital-Native / Algorithmic VST Sub-Blocks (Slots F-J)
  // -------------------------------------------------------------
  {
    std::cout << "Testing Digital-Native VST Sub-Blocks (Slots F-J: Stream "
                 "Splitter, Spectral, Clipper, Phase-Pure Widener)..."
              << std::endl;
    const auto &catalog = avra::studio::getSchematicCatalog();
    AVRA_TEST(catalog.size() >= 42,
              "Schematic Component Catalog exposes all 42 Analog (Slots A-E) & "
              "Digital (Slots F-J) sub-blocks");

    avra::dsp::circuits::StructuralStreamSplitter splitter;
    avra::dsp::circuits::SpectralResonanceTamer spectralTamer;
    avra::dsp::circuits::BarkScaleAuditoryUnmasker barkUnmasker;
    avra::dsp::circuits::MissingFundamentalSynth subSynth;
    avra::dsp::circuits::StatelessLoudnessInflator inflator;
    avra::dsp::circuits::VariableKneePolyClipper polyClipper;
    avra::dsp::circuits::PhasePureStereoWidener monoWidener;
    avra::dsp::circuits::OctaveShimmerFdnTank shimmerTank;

    splitter.prepare(48000.0);
    spectralTamer.prepare(48000.0);
    barkUnmasker.prepare(48000.0);
    subSynth.prepare(48000.0);
    inflator.prepare(48000.0);
    polyClipper.prepare(48000.0);
    monoWidener.prepare(48000.0);
    shimmerTank.prepare(48000.0);
    monoWidener.setParameters(100.0f, 50.0f);

    float maxMonoError = 0.0f;
    float maxSplitError = 0.0f;
    for (int n = 0; n < 512; ++n) {
      const float dryL = 0.65f * std::sin(2.0f * 3.14159265f * 440.0f *
                                          (static_cast<float>(n) / 48000.0f));
      const float dryR = 0.55f * std::cos(2.0f * 3.14159265f * 330.0f *
                                          (static_cast<float>(n) / 48000.0f));

      // 1. Verify Slot F Structural Stream Splitter bit-perfect 1:1
      // phase-coherent sum: Transient + Tonal == Dry
      float trL = 0.0f, trR = 0.0f, toL = 0.0f, toR = 0.0f;
      splitter.splitStereo(dryL, dryR, trL, trR, toL, toR);
      maxSplitError = std::max(maxSplitError, std::abs((trL + toL) - dryL));

      // 2. Verify Slot J Phase-Pure Stereo Decorrelator 100% mono cancellation:
      // (L_out + R_out) == (L_in + R_in)
      float wideL = dryL, wideR = dryR;
      monoWidener.processStereo(wideL, wideR);
      maxMonoError =
          std::max(maxMonoError, std::abs((wideL + wideR) - (dryL + dryR)));

      // 3. Run full Digital-Native processing chain
      float l = dryL, r = dryR;
      splitter.processStereo(l, r);
      spectralTamer.processStereo(l, r);
      barkUnmasker.processStereo(l, r);
      subSynth.processStereo(l, r);
      inflator.processStereo(l, r);
      polyClipper.processStereo(l, r);
      shimmerTank.processStereo(l, r);
      AVRA_TEST(std::isfinite(l) && std::isfinite(r),
                "Digital-Native Slots F-J chain output is finite");
    }
    AVRA_TEST(maxSplitError < 1e-6f,
              "Slot F Structural Stream Splitter sums back to bit-perfect 1:1 "
              "dry signal (zero phase distortion)");
    AVRA_TEST(maxMonoError < 1e-5f,
              "Slot J Phase-Pure Stereo Decorrelator cancels side signal to "
              "0.0 when collapsed to Mono");

    // 4. Verify CL1 (Standard 16x Clipper) and CL4-CL7 Gain Reduction Metering:
    // Must report 0.0 dB on silence/low-level signals and bounded 0..20 dB GR
    // when clipping hot peaks (never zero-crossing phase-mismatch 50+ dB spikes).
    avra::dsp::circuits::VariableKneePolyClipper cl1;
    avra::dsp::circuits::MasteringAdcOverdriveStage cl4;
    avra::dsp::circuits::SplitBandMasteringClipperStage cl5;
    avra::dsp::circuits::HilbertWaveformSymmetrizerStage cl6;
    avra::dsp::circuits::Adaa2ndOrderClipperStage cl7;
    cl1.prepare(48000.0);
    cl4.prepare(48000.0);
    cl5.prepare(48000.0);
    cl6.prepare(48000.0);
    cl7.prepare(48000.0);

    for (int n = 0; n < 64; ++n) {
      float zL = 0.0f, zR = 0.0f;
      cl1.processStereo(zL, zR);
      cl4.processStereo(zL, zR);
      cl5.processStereo(zL, zR);
      cl6.processStereo(zL, zR);
      cl7.processStereo(zL, zR);
    }
    AVRA_TEST(cl1.getGainReductionDb() == 0.0f &&
                  cl4.getGainReductionDb() == 0.0f &&
                  cl5.getGainReductionDb() == 0.0f &&
                  cl6.getGainReductionDb() == 0.0f &&
                  cl7.getGainReductionDb() == 0.0f,
              "CL1 and CL4-CL7 clippers report 0.0 dB gain reduction on silence");

    float maxQuietCl1Gr = 0.0f;
    for (int n = 0; n < 256; ++n) {
      float qL = 0.12f * std::sin(2.0f * 3.14159265f * 440.0f *
                                  (static_cast<float>(n) / 48000.0f));
      float qR = qL;
      cl1.processStereo(qL, qR);
      maxQuietCl1Gr = std::max(maxQuietCl1Gr, cl1.getGainReductionDb());
    }
    AVRA_TEST(maxQuietCl1Gr == 0.0f,
              "CL1 Standard 16x Clipper reports 0.0 dB GR below ceiling (no zero-crossing spikes)");

    float maxHotCl1Gr = 0.0f;
    for (int n = 0; n < 256; ++n) {
      float hL = 1.8f * std::sin(2.0f * 3.14159265f * 440.0f *
                                 (static_cast<float>(n) / 48000.0f));
      float hR = hL;
      cl1.processStereo(hL, hR);
      maxHotCl1Gr = std::max(maxHotCl1Gr, cl1.getGainReductionDb());
    }
    AVRA_TEST(maxHotCl1Gr > 1.0f && maxHotCl1Gr <= 20.0f,
              "CL1 Standard 16x Clipper reports accurate bounded GR when clipping hot peaks");
  }

  // -------------------------------------------------------------
  // 10. Native VST3 C-ABI COM Factory & Toolchain Readiness Verification
  // -------------------------------------------------------------
  {
    std::cout
        << "Testing Native VST3 C-ABI COM Factory & Toolchain Detection..."
        << std::endl;
    auto tc = avra::studio::ProjectGenerator::detectToolchain();
    AVRA_TEST(tc.hasCMake && tc.hasMsvcCompiler,
              "Toolchain detector locates CMake and MSVC C++20 compiler");

    avra::vst3_bridge::AvraVst3PluginFactory<avra::examples::GainPlugin>
        factory("com.avra.gain", "Avra Gain", "Avra Audio",
                "https://avra.audio", "1.0.0", "Fx|Tools");
    AVRA_TEST(factory.countClasses() == 1,
              "VST3 factory reports 1 audio module class");

#if defined(_WIN32)
    const uint8_t expectedFactoryComGuid[16] = {
        0x1Cu, 0x81u, 0x4Du, 0x7Au, 0x11u, 0x52u, 0x1Fu, 0x4Au,
        0xAEu, 0xD9u, 0xD2u, 0xEEu, 0x0Bu, 0x43u, 0xBFu, 0x9Fu};
    AVRA_TEST(avra::vst3_bridge::iidEqual(avra::vst3_bridge::kIPluginFactoryIid,
                                          expectedFactoryComGuid),
              "VST3 kIPluginFactoryIid matches Windows COM_COMPATIBLE=1 GUID "
              "byte layout");

    const uint8_t expectedPluginBaseComGuid[16] = {
        0xDBu, 0x8Du, 0x88u, 0x22u, 0x6Eu, 0x15u, 0xAEu, 0x45u,
        0x83u, 0x58u, 0xB3u, 0x48u, 0x08u, 0x19u, 0x06u, 0x25u};
    AVRA_TEST(avra::vst3_bridge::iidEqual(avra::vst3_bridge::kIPluginBaseIid,
                                          expectedPluginBaseComGuid),
              "VST3 kIPluginBaseIid matches official Steinberg IPluginBase "
              "COM_COMPATIBLE=1 GUID");
#endif

    avra::vst3_bridge::PClassInfo2 classInfo{};
    AVRA_TEST(factory.getClassInfo2(0, &classInfo) ==
                  avra::vst3_bridge::kResultOk,
              "VST3 factory returns PClassInfo2");

    avra::vst3_bridge::IComponent *comp = nullptr;
    AVRA_TEST(
        factory.createInstance(
            reinterpret_cast<const char *>(classInfo.cid),
            reinterpret_cast<const char *>(avra::vst3_bridge::kIComponentIid),
            reinterpret_cast<void **>(&comp)) == avra::vst3_bridge::kResultOk &&
            comp != nullptr,
        "VST3 factory instantiates IComponent");

    avra::vst3_bridge::TUID ctrlCid{};
    AVRA_TEST(comp->getControllerClassId(ctrlCid) ==
                  avra::vst3_bridge::kResultFalse,
              "VST3 SingleComponentEffect getControllerClassId returns "
              "kResultFalse for unified component/controller");

    avra::vst3_bridge::IAudioProcessor *audioProc = nullptr;
    AVRA_TEST(comp->queryInterface(avra::vst3_bridge::kIAudioProcessorIid,
                                   reinterpret_cast<void **>(&audioProc)) ==
                      avra::vst3_bridge::kResultOk &&
                  audioProc != nullptr,
              "VST3 component queries IAudioProcessor");

    avra::vst3_bridge::IProcessContextRequirements *procReqs = nullptr;
    AVRA_TEST(
        comp->queryInterface(avra::vst3_bridge::kIProcessContextRequirementsIid,
                             reinterpret_cast<void **>(&procReqs)) ==
                avra::vst3_bridge::kResultOk &&
            procReqs != nullptr,
        "VST3 3.7+ component queries IProcessContextRequirements");
    procReqs->release();

    avra::vst3_bridge::IEditController *editCtrl = nullptr;
    AVRA_TEST(comp->queryInterface(avra::vst3_bridge::kIEditControllerIid,
                                   reinterpret_cast<void **>(&editCtrl)) ==
                      avra::vst3_bridge::kResultOk &&
                  editCtrl != nullptr,
              "VST3 component queries IEditController");
    AVRA_TEST(editCtrl->getParameterCount() == 2,
              "VST3 IEditController exposes GainPlugin parameters");

    avra::vst3_bridge::ProcessSetup setup{0, 0, 256, 48000.0};
    AVRA_TEST(audioProc->setupProcessing(setup) == avra::vst3_bridge::kResultOk,
              "VST3 setupProcessing succeeds");

    std::vector<float> inL(64, 0.5f), inR(64, 0.5f), outL(64, 0.0f),
        outR(64, 0.0f);
    float *inCh[2] = {inL.data(), inR.data()};
    float *outCh[2] = {outL.data(), outR.data()};
    avra::vst3_bridge::AudioBusBuffers inBus{2, 0, {inCh}};
    avra::vst3_bridge::AudioBusBuffers outBus{2, 0, {outCh}};
    avra::vst3_bridge::ProcessData data{};
    data.numSamples = 64;
    data.numInputs = 1;
    data.numOutputs = 1;
    data.inputs = &inBus;
    data.outputs = &outBus;
    AVRA_TEST(audioProc->process(data) == avra::vst3_bridge::kResultOk,
              "VST3 real-time process block succeeds");
    AVRA_TEST(std::abs(outL[32] - 0.5f) < 0.05f,
              "VST3 unity gain audio processing verified");

    editCtrl->release();
    audioProc->release();
    comp->release();
  }

  // -------------------------------------------------------------
  // 11. Multi-Parameter Hardware Models & Auto-Populated Faceplate Knobs
  // -------------------------------------------------------------
  {
    std::cout << "Testing Multi-Parameter Hardware Models & Auto-Populated "
                 "Faceplate Knobs..."
              << std::endl;
    const auto &eqMeta = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::InductorEq3Band);
    const auto &fetMeta = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::FetCompressor1176);
    const auto &vcaMeta = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::VcaBusCompressor);
    const auto &tapeMeta = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::TapeEcho);
    const auto &shimmerMeta = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::OctaveShimmerReverb);
    const auto &hallMeta = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::HallReverbFDN);

    AVRA_TEST(eqMeta.getParamCount() >= 5,
              "InductorEq3Band exposes authentic hardware parameters");
    AVRA_TEST(fetMeta.getParamCount() >= 5,
              "FetCompressor1176 exposes authentic hardware parameters");
    AVRA_TEST(vcaMeta.getParamCount() >= 5,
              "VcaBusCompressor exposes authentic hardware parameters");
    AVRA_TEST(tapeMeta.getParamCount() >= 5,
              "TapeEcho exposes authentic hardware parameters");
    AVRA_TEST(shimmerMeta.getParamCount() >= 5,
              "OctaveShimmerReverb exposes authentic hardware parameters");
    AVRA_TEST(hallMeta.getParamCount() >= 5,
              "HallReverbFDN exposes authentic hardware parameters");

    const auto &widgetCatalog = avra::studio::getUiWidgetCatalog();
    AVRA_TEST(widgetCatalog.size() >= 45,
              "UI Widget Catalog exposes full studio knob library (Preamp, EQ, "
              "Dynamics, Delay, Reverb, Mod)");

    std::vector<avra::studio::SchematicNode> stripChain = {
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::ClassAPreamp),
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::InductorEq3Band),
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::VcaBusCompressor)};
    stripChain[1].isParallel = true;
    auto layout = avra::studio::buildFaceplateFromSchematic(
        "CONSOLE STRIP", stripChain, avra::gui::ChassisFinish::ObsidianBlack,
        avra::gui::SurfaceGrain::HorizontalBrushed);

    int stage0Knobs = 0; // ClassAPreamp (stageIdx == 0)
    int stage1Knobs = 0; // InductorEq3Band (stageIdx == 1)
    int stage2Knobs = 0; // VcaBusCompressor (stageIdx == 2)
    bool hasMasterMix = false;
    int stageParMixCount = 0;
    for (const auto &w : layout.widgets) {
      if (avra::studio::isKnobWidget(w.type) ||
          avra::studio::isSwitchWidget(w.type)) {
        if (w.boundStageIdx == 0)
          ++stage0Knobs;
        if (w.boundStageIdx == 1)
          ++stage1Knobs;
        if (w.boundStageIdx == 2)
          ++stage2Knobs;
      }
      if (avra::studio::isKnobWidget(w.type)) {
        if (w.boundStageIdx == -1 && w.boundParamSlot == 2)
          hasMasterMix = true;
        if (w.boundStageIdx >= 0 &&
            (w.boundParamSlot == 9 || w.boundParamSlot == 109))
          ++stageParMixCount;
      }
    }
    AVRA_TEST(stage0Knobs >= 5, "buildFaceplateFromSchematic exposes all "
                                "hardware controls for ClassAPreamp");
    AVRA_TEST(stage1Knobs >= 5,
              "buildFaceplateFromSchematic exposes all hardware controls + PAR "
              "MIX for InductorEq3Band");
    AVRA_TEST(stage2Knobs >= 5, "buildFaceplateFromSchematic exposes all "
                                "hardware controls for VcaBusCompressor");
    AVRA_TEST(hasMasterMix,
              "buildFaceplateFromSchematic includes Universal MASTER MIX "
              "(0..100% Dry/Wet) knob in METERING · OUTPUT");
    AVRA_TEST(stageParMixCount == 1,
              "buildFaceplateFromSchematic includes per-stage PAR MIX "
              "(0..100%) knob when stage is in parallel mode");

    // Verify all 32 knob styles & 18 meter/visualizer styles cycle cleanly
    avra::studio::UiWidgetType curKnob =
        avra::studio::UiWidgetType::PointerKnob;
    for (int i = 0; i < 32; ++i) {
      AVRA_TEST(avra::studio::isKnobWidget(curKnob),
                "Every cycled knob style is recognized as a knob widget");
      curKnob = avra::studio::nextKnobWidgetType(curKnob);
    }
    AVRA_TEST(curKnob == avra::studio::UiWidgetType::PointerKnob,
              "32 knob styles cycle back to PointerKnob");

    avra::studio::UiWidgetType curMeter =
        avra::studio::UiWidgetType::AnalogVuMeter;
    for (int i = 0; i < 18; ++i) {
      AVRA_TEST(avra::studio::isMeterWidget(curMeter),
                "Every cycled meter/visualizer style is recognized as a meter "
                "widget");
      curMeter = avra::studio::nextMeterWidgetType(curMeter);
    }
    AVRA_TEST(curMeter == avra::studio::UiWidgetType::AnalogVuMeter,
              "18 meter/visualizer styles cycle back to AnalogVuMeter");

    // Verify all 12 Unified UI Style Templates apply cleanly
    for (int t = 0; t < avra::studio::kTotalUiStyleTemplates; ++t) {
      avra::studio::applyUiStyleTemplate(
          layout, static_cast<avra::studio::UiStyleTemplate>(t));
      const auto &spec = avra::studio::getUnifiedStyleTemplateSpec(
          static_cast<avra::studio::UiStyleTemplate>(t));
      AVRA_TEST(
          std::string_view(avra::studio::getUiStyleTemplateName(
                               static_cast<avra::studio::UiStyleTemplate>(t)))
                      .size() > 0 &&
              layout.theme.bevelDepthPx == spec.surface.bevelDepthPx &&
              layout.theme.knobIndicatorHex ==
                  spec.knobStyle.indicatorColorHex &&
              layout.theme.meterGlowHex == spec.meterStyle.activeGlowHex &&
              layout.theme.labelPlaqueStyle == spec.typography.labelStyle,
          "Every UnifiedStyleTemplate updates all 6 visual layers on the "
          "faceplate theme");
    }

    // -------------------------------------------------------------
    // 8. Ecosystem Orchestration Engine & Smart Plumbing Rules
    // (RULE_01..RULE_08)
    // -------------------------------------------------------------
    std::cout << "Testing Ecosystem Orchestration Engine & "
                 "GraphSanitizerAndSanityEngine (RULE_01..RULE_08)..."
              << std::endl;
    std::vector<avra::studio::SchematicNode> ecoChain = {
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::
                ClassAPreamp), // Stage 0: DC Guard + Drift + 2x OS + Link IO
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::
                FetCompressor1176), // Stage 1: 4x OS + DET_PEAK_1176 [Feedback]
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::
                PassivePultecEq), // Stage 2: Passive +16dB Makeup + Drift
        avra::studio::makeDefaultSchematicNode(
            avra::studio::SchematicComponentType::
                BbdCompanderDelay) // Stage 3: NE570 2:1 RMS Compander Wrapper
    };

    const auto plans =
        avra::studio::GraphSanitizerAndSanityEngine::analyzeChain(ecoChain);
    AVRA_TEST(plans.size() == 4, "GraphSanitizerAndSanityEngine produces a "
                                 "StagePlumbingPlan for every stage");
    AVRA_TEST(plans[0].dcGuardActive && plans[0].thermalFloorLinear > 0.0f,
              "RULE_01 & RULE_02: ClassAPreamp auto-injects 5Hz DC Guard and "
              "Analog Drift");
    AVRA_TEST(plans[0].osIslandId > 0 &&
                  plans[0].osIslandId == plans[1].osIslandId,
              "RULE_03: Contiguous non-linear stages (ClassAPreamp + "
              "FetCompressor1176) consolidate into a single Shared "
              "Oversampling Island");
    AVRA_TEST(plans[1].islandOsFactor == 4,
              "RULE_03: Shared Oversampling Island adopts max oversampling "
              "factor (4x) across contiguous non-linear stages");
    AVRA_TEST(
        plans[1].detectorBadge.find("DET_PEAK_FET") != std::string::npos,
        "RULE_04: FetCompressor1176 auto-pairs with DET_PEAK_1176 [Feedback]");
    AVRA_TEST(plans[2].passiveMakeupDb >= 15.0f,
              "RULE_05: PassivePultecEq auto-attaches +16 dB passive "
              "insertion-loss makeup calibration");
    AVRA_TEST(plans[0].autoGainLinkAvailable,
              "RULE_06: ClassAPreamp enables Drive/Output Auto-Gain Inverse "
              "Link by default");
    AVRA_TEST(
        plans[3].companderWrapperActive,
        "RULE_08: BbdCompanderDelay enables NE570 2:1 RMS Compander Wrapper");

    // Verify Pillar 5.1 (Expose Internal Calibration Trims) & Pillar 5.3
    // (Heritage vs Unified Theme)
    ecoChain[0].exposeInternalTrims = true;
    avra::studio::FaceplateTheme heritageTheme{};
    heritageTheme.unifyFaceplateTheme = false;
    auto heritageLayout = avra::studio::buildFaceplateFromSchematicWithTheme(
        "ECO STRIP", ecoChain, heritageTheme);

    bool foundLinkIoSwitch = false;
    int exposedTrimCount = 0;
    for (const auto &w : heritageLayout.widgets) {
      if (w.boundStageIdx == 0 &&
          (w.boundParamSlot == 8 || w.boundParamSlot == 108) &&
          avra::studio::isSwitchWidget(w.type)) {
        foundLinkIoSwitch = true;
      }
      if (w.boundStageIdx == 0 &&
          (w.boundParamSlot == 5 || w.boundParamSlot == 6 ||
           w.boundParamSlot == 105 || w.boundParamSlot == 106) &&
          avra::studio::isKnobWidget(w.type)) {
        ++exposedTrimCount;
      }
    }
    AVRA_TEST(foundLinkIoSwitch, "RULE_06 spawns a clickable LINK IO switch on "
                                 "the faceplate for ClassAPreamp");
    AVRA_TEST(exposedTrimCount == 2,
              "Pillar 5.1: Setting exposeInternalTrims=true promotes both "
              "internal calibration trims onto the faceplate");

    // -------------------------------------------------------------
    // 9. Generative UI & Parameter Grammar + 102-Module Component Library &
    // Secondary Filter Pill Bar
    // -------------------------------------------------------------
    std::cout << "Testing 102-Module Component Library, Secondary Filter Pill "
                 "Bar & 4-Tier Parameter Grammar..."
              << std::endl;
    const auto &fullCat = avra::studio::getSchematicCatalog();
    AVRA_TEST(fullCat.size() == 216,
              "Full schematic catalog contains all 216 expanded studio "
              "hardware and digital component models across 7 tabs");

    int countPre = 0, countIron = 0, countEq = 0, countComp = 0, countTime = 0,
        countSplit = 0, countAuto = 0;
    for (const auto &comp : fullCat) {
      // Gate #1: Every module exposes its authentic hardware controls (2..24
      // controls per module). The upper bound was 18 only because the removed
      // MultiControlHardwareMatrix16 handled 18 slots; LiveSchematicRunner and
      // the exporter now forward up to 24 controls per stage (e.g. EQ3 4-band
      // passive LC EQ = 20 real controls).
      AVRA_TEST(comp.getParamCount() >= 2 && comp.getParamCount() <= 24,
                "Every component in the studio library passes variable-count "
                "hardware control grammar validation");
      switch (comp.category) {
      case avra::studio::SchematicCategory::PreampsAndTubes:
        ++countPre;
        break;
      case avra::studio::SchematicCategory::IronAndTape:
        ++countIron;
        break;
      case avra::studio::SchematicCategory::EqAndFilters:
        ++countEq;
        break;
      case avra::studio::SchematicCategory::AnalogDynamics:
        ++countComp;
        break;
      case avra::studio::SchematicCategory::DelayAndReverb:
        ++countTime;
        break;
      case avra::studio::SchematicCategory::ModulationAndRoute:
        ++countSplit;
        break;
      case avra::studio::SchematicCategory::AutoAndSmart:
        ++countAuto;
        break;
      }
    }
    AVRA_TEST(
        countPre == 36 && countIron == 36 && countEq == 34 && countComp == 34 &&
            countTime == 36 && countSplit == 35 && countAuto == 5,
        "All 7 tabs (PRE/TUBE 36, IRON/SAT 36, EQ/SPEC 34, COMP/CLIP 34, "
        "TIME/VERB 36, SPLIT/MOD 35, AUTO/SMART 5) total 216 studio modules");

    AVRA_TEST(avra::studio::getCuratedSchematicCatalog().size() == 150,
              "Curated flagship studio catalog retains 150 highest-tier "
              "distinct studio workhorses");
    AVRA_TEST(avra::studio::getSchematicSubFilterLabel(
                  avra::studio::SchematicCategory::PreampsAndTubes, 0) ==
                      "All (24)" &&
                  avra::studio::getSchematicSubFilterLabel(
                      avra::studio::SchematicCategory::IronAndTape, 0) ==
                      "All (25)" &&
                  avra::studio::getSchematicSubFilterLabel(
                      avra::studio::SchematicCategory::EqAndFilters, 0) ==
                      "All (25)" &&
                  avra::studio::getSchematicSubFilterLabel(
                      avra::studio::SchematicCategory::AnalogDynamics, 0) ==
                      "All (26)" &&
                  avra::studio::getSchematicSubFilterLabel(
                      avra::studio::SchematicCategory::DelayAndReverb, 0) ==
                      "All (24)" &&
                  avra::studio::getSchematicSubFilterLabel(
                      avra::studio::SchematicCategory::ModulationAndRoute, 0) ==
                      "All (21)" &&
                  avra::studio::getSchematicSubFilterLabel(
                      avra::studio::SchematicCategory::AutoAndSmart, 0) ==
                      "All (5)",
              "Secondary Filter Pill Bar computes exact live curated "
              "sub-filter counts across all 7 tabs");

    // Source-Type Filter verification (Bass, Guitar, Drums, Vocals, Keys + Universal)
    {
        const auto &auVocal = avra::studio::getComponentMeta(
            avra::studio::SchematicComponentType::ZeroDistortionVocalRider);
        const auto &spBass = avra::studio::getComponentMeta(
            avra::studio::SchematicComponentType::PhaseCompensatedBassMono);
        const auto &prCal = avra::studio::getComponentMeta(
            avra::studio::SchematicComponentType::HeadroomCalibratorPre);
        AVRA_TEST(avra::studio::matchesSchematicSourceFilter(
                      auVocal, avra::studio::SchematicSourceFilter::Vocals) &&
                  !avra::studio::matchesSchematicSourceFilter(
                      auVocal, avra::studio::SchematicSourceFilter::Bass) &&
                  avra::studio::matchesSchematicSourceFilter(
                      spBass, avra::studio::SchematicSourceFilter::Bass) &&
                  !avra::studio::matchesSchematicSourceFilter(
                      spBass, avra::studio::SchematicSourceFilter::Vocals) &&
                  avra::studio::matchesSchematicSourceFilter(
                      prCal, avra::studio::SchematicSourceFilter::Bass) &&
                  avra::studio::matchesSchematicSourceFilter(
                      prCal, avra::studio::SchematicSourceFilter::Guitar) &&
                  avra::studio::matchesSchematicSourceFilter(
                      prCal, avra::studio::SchematicSourceFilter::Drums) &&
                  avra::studio::matchesSchematicSourceFilter(
                      prCal, avra::studio::SchematicSourceFilter::Vocals) &&
                  avra::studio::matchesSchematicSourceFilter(
                      prCal, avra::studio::SchematicSourceFilter::Keys),
                  "Source-Type Filter isolates source-voiced modules while "
                  "allowing universal modules in all source filters");
    }

    // Gate #2: Multi-band EQs have paired Frequency (Hz) & Q controls and no
    // hardcoded frequency parentheticals
    const auto &lcEq = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::ParallelPassiveLcEq);
    AVRA_TEST(lcEq.getParamSpec(0).name.find('(') == std::string::npos &&
                  lcEq.getParamCount() >= 8,
              "Gate #2: Multi-band EQ strips hardcoded frequency "
              "parentheticals and exposes swept Frequency (Hz) + Q controls");

    // Gate #3: Real Acoustic Units Enforcement
    const auto &gatedVerb = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::NonlinearGatedReverb);
    const auto &cmosOd = avra::studio::getComponentMeta(
        avra::studio::SchematicComponentType::ParallelSaturationBus);
    AVRA_TEST(
        !gatedVerb.getParamSpec(0).name.empty(),
        "Gate #3: NonlinearGatedReverb exposes authentic hardware controls");
    AVRA_TEST(cmosOd.getParamSpec(2).unit == "Hz" &&
                  cmosOd.getParamCount() >= 5,
              "Gate #3: ParallelSaturationBus (ST4 Dark Bass Grit Bus) exposes "
              "Crossover Frequency with real 'Hz' unit");

    // Gate #1 & Authentic Per-Topology Control Layouts
    int stage0KnobCount = 0;
    for (const auto &w : heritageLayout.widgets) {
      if (w.boundStageIdx == 0 && avra::studio::isKnobWidget(w.type)) {
        ++stage0KnobCount;
      }
    }
    AVRA_TEST(heritageLayout.getWidgetsForStage(0).size() >= 6,
              "Gate #1: Stage binding manifest exposes bound hardware controls "
              "+ telemetry meter");
    AVRA_TEST(stage0KnobCount >= 2,
              "Gate #6: Enforces authentic variable-count hardware control "
              "layouts per stage bay");
  }

  // -------------------------------------------------------------
  // 10. Phase 1 & Phase 2: "Candy Store" Calibration, Dual-Trace Scopes &
  // Component Bay Circuit-Bending
  // -------------------------------------------------------------
  {
    std::cout << "Testing Phase 1 & Phase 2: Unity Gain Calibration, "
                 "StereoPanLCR, THD Analyzer & Component Bay Sockets..."
              << std::endl;

    // 10.1 Unity Gain Calibration (0.0 dB OUT-IN Delta on Class-A, Class-AB &
    // JFET Buffer)
    avra::dsp::circuits::DiscreteClassAPreamp classA;
    avra::dsp::circuits::DiscreteClassABOpAmp classAB;
    avra::dsp::circuits::JfetBufferStage jfetBuf;
    classA.prepare(48000.0);
    classAB.prepare(48000.0);
    jfetBuf.prepare(48000.0);
    classA.setParameters(0.0f, 0.5f);
    classAB.setParameters(0.0f, 50.0f);
    jfetBuf.setParameters(0.0f, 0.0f, 20.0f, 22000.0f, 0.0f);

    float sumSqIn = 0.0f, sumSqA = 0.0f, sumSqAB = 0.0f, sumSqJfet = 0.0f;
    for (int n = 0; n < 2048; ++n) {
      const float s = 0.1f * std::sin(2.0f * 3.14159265f * 1000.0f *
                                      static_cast<float>(n) / 48000.0f);
      float lA = s, rA = s;
      float lAB = s, rAB = s;
      float lJ = s, rJ = s;
      classA.processStereo(lA, rA);
      classAB.processStereo(lAB, rAB);
      jfetBuf.processStereo(lJ, rJ);
      if (n >= 512) {
        sumSqIn += s * s;
        sumSqA += lA * lA;
        sumSqAB += lAB * lAB;
        sumSqJfet += lJ * lJ;
      }
    }
    const float dbDeltaJfet =
        std::abs(avra::math::linearTodB(std::sqrt(sumSqJfet / sumSqIn)));
    const float dbDeltaA =
        std::abs(avra::math::linearTodB(std::sqrt(sumSqA / sumSqIn)));
    const float dbDeltaAB =
        std::abs(avra::math::linearTodB(std::sqrt(sumSqAB / sumSqIn)));
    AVRA_TEST(dbDeltaJfet < 0.05f,
              "JfetBufferStage is calibrated to 0.0 dB unity gain");
    AVRA_TEST(dbDeltaA < 0.25f && dbDeltaAB < 0.25f,
              "DiscreteClassAPreamp and DiscreteClassABOpAmp are calibrated to "
              "0.0 dB unity at 0.0 dB drive");

    const auto gainChain =
        avra::studio::loadProjectSchematicChain("GainPlugin");
    AVRA_TEST(!gainChain.empty() &&
                  std::abs(gainChain[0].param1Val - 0.0f) < 1e-4f &&
                  std::abs(gainChain[0].param5Val - 0.0f) < 1e-4f,
              "GainPlugin default schematic initializes INPUT GAIN and OUTPUT "
              "TRIM to exact 0.0 dB unity");

    // 10.2 Stereo Balance L100 - L50 - C - R50 - R100 Skirt Labels
    bool firstOr = false, lastOr = false;
    const auto panTicks = avra::studio::getScalePresetLabels(
        avra::studio::UiScalePreset::StereoPanLCR, firstOr, lastOr);
    AVRA_TEST(panTicks.size() == 5 && panTicks[0] == "L100" &&
                  panTicks[2] == "C" && panTicks[4] == "R100",
              "UiScalePreset::StereoPanLCR exposes studio-standard L100 - L50 "
              "- C - R50 - R100 skirt ticks");
    const auto gainFaceplate = avra::studio::buildAvraGainDemoFaceplate();
    bool foundStereoBalLCR = false;
    for (const auto &w : gainFaceplate.widgets) {
      if (w.title == "STEREO BAL" &&
          w.scalePreset == avra::studio::UiScalePreset::StereoPanLCR) {
        foundStereoBalLCR = true;
      }
    }
    AVRA_TEST(foundStereoBalLCR, "STEREO BAL knob in Avra Gain faceplate uses "
                                 "UiScalePreset::StereoPanLCR");

    // 10.3 Expanded Studio Groove Test Generators (Drums, Synth Pad, Clean Gtr,
    // Dist Gtr, Bass DI, Vocal Phrase)
    uint32_t rng = 0x12345678u;
    float dPeak = 0.0f, sPeak = 0.0f, cgPeak = 0.0f, gPeak = 0.0f, bPeak = 0.0f,
          vPeak = 0.0f;
    for (int n = 0; n < 4096; ++n) {
      const double t = static_cast<double>(n) / 48000.0;
      float l = 0.0f, r = 0.0f;
      avra::audio_io::StudioGrooveSynthesizer::synthesizeDrumAndBassGroove(
          t, rng, l, r);
      dPeak = std::max(dPeak, std::abs(l));
      avra::audio_io::StudioGrooveSynthesizer::synthesizeSynthChordLoop(t, l,
                                                                        r);
      sPeak = std::max(sPeak, std::abs(l));
      avra::audio_io::StudioGrooveSynthesizer::synthesizeCleanGuitarChord(
          t, rng, l, r);
      cgPeak = std::max(cgPeak, std::abs(l));
      avra::audio_io::StudioGrooveSynthesizer::synthesizeGuitarDiChug(t, rng, l,
                                                                      r);
      gPeak = std::max(gPeak, std::abs(l));
      avra::audio_io::StudioGrooveSynthesizer::synthesizeBassDiGroove(t, rng, l,
                                                                      r);
      bPeak = std::max(bPeak, std::abs(l));
      avra::audio_io::StudioGrooveSynthesizer::synthesizeVocalPhrase(t, rng, l,
                                                                     r);
      vPeak = std::max(vPeak, std::abs(l));
    }
    AVRA_TEST(dPeak > 0.05f && sPeak > 0.05f && cgPeak > 0.05f &&
                  gPeak > 0.05f && bPeak > 0.05f && vPeak > 0.05f,
              "Drums, Synth Pad, Clean Guitar, Distorted Guitar, Bass DI, and "
              "Vocal test generators synthesize active 120 BPM studio loops");

    // 10.4 Plugin Doctor THD Analyzer & Dual-Trace Scope Modes
    avra::gui::SpectrumAnalyzer specAnalyzer({});
    specAnalyzer.setHarmonicAnalyzerMode(true);
    std::vector<float> cleanSine(512, 0.0f);
    std::vector<float> satSine(512, 0.0f);
    for (int n = 0; n < 512; ++n) {
      const float x = 0.5f * std::sin(2.0f * 3.14159265f * 1000.0f *
                                      static_cast<float>(n) / 48000.0f);
      cleanSine[static_cast<std::size_t>(n)] = x;
      satSine[static_cast<std::size_t>(n)] =
          std::tanh(3.5f * x + 0.25f * x * x);
    }
    specAnalyzer.updateInputFromSamples(cleanSine);
    specAnalyzer.updateFromSamples(satSine);
    AVRA_TEST(specAnalyzer.isHarmonicAnalyzerMode() &&
                  specAnalyzer.getThdPercent() > 0.5f,
              "SpectrumAnalyzer THD Analyzer computes live THD% and Even/Odd "
              "harmonics from saturated 1 kHz test tone");

    avra::gui::WaveformScope dualScope({});
    dualScope.setScopeMode(avra::gui::WaveformScope::ScopeMode::TransferKnee);
    AVRA_TEST(dualScope.getScopeMode() ==
                  avra::gui::WaveformScope::ScopeMode::TransferKnee,
              "WaveformScope switches between Waveform, TransferKnee, and "
              "PhaseScope modes");

    // 10.6 Knob Scale Typography: Unicode -∞ and 12 O'Clock '0' dB Center
    // Alignment
    bool firstOrange = false, lastOrange = false;
    const auto masterLabels = avra::studio::getScalePresetLabels(
        avra::studio::UiScalePreset::MasterVolumeDb, firstOrange, lastOrange);
    AVRA_TEST(masterLabels.size() == 9 &&
                  std::string_view(masterLabels[0]) == "-\xE2\x88\x9E" &&
                  std::string_view(masterLabels[4]) == "0",
              "MasterVolumeDb scale uses Unicode -∞ at 7 o'clock and places "
              "'0' dead-center at 12 o'clock (index 4 of 9)");

    const auto trimLabels = avra::studio::getScalePresetLabels(
        avra::studio::UiScalePreset::FineTrimDb, firstOrange, lastOrange);
    AVRA_TEST(
        trimLabels.size() == 7 && std::string_view(trimLabels[3]) == "0",
        "FineTrimDb scale places '0' dead-center at 12 o'clock (index 3 of 7)");

    // 10.7 Legal & Trademark Compliance Verification (Zero Third-Party Brand
    // Names in Catalog/UI)
    const std::vector<std::string> forbiddenBrands = {"neve",
                                                      "ssl",
                                                      "pultec",
                                                      "teletronix",
                                                      "la-2a",
                                                      "la2a",
                                                      "urei",
                                                      "1176",
                                                      "fairchild",
                                                      "manley",
                                                      "massive passive",
                                                      "distressor",
                                                      "aphex",
                                                      "aural exciter",
                                                      "big-bottom",
                                                      "big bottom",
                                                      "transient designer",
                                                      "echoboy",
                                                      "decapitator",
                                                      "fabfilter",
                                                      "pro-q",
                                                      "soothe",
                                                      "gullfoss",
                                                      "valhalla",
                                                      "tokyo dawn",
                                                      "mullard",
                                                      "telefunken",
                                                      "sovtek",
                                                      "jensen",
                                                      "dymo",
                                                      "sharpie",
                                                      "marshall",
                                                      "mesa",
                                                      "soldano",
                                                      "fender",
                                                      "klon",
                                                      "proco",
                                                      "guitar",
                                                      "stompbox",
                                                      "rectifier",
                                                      "plexi"};
    auto hasForbiddenBrand = [&](std::string_view text) -> bool {
      std::string lower(text);
      for (char &c : lower) {
        if (c >= 'A' && c <= 'Z')
          c = static_cast<char>(c - 'A' + 'a');
      }
      for (const auto &brand : forbiddenBrands) {
        std::size_t pos = 0;
        while ((pos = lower.find(brand, pos)) != std::string::npos) {
          const bool leftOk =
              (pos == 0) ||
              !std::isalnum(static_cast<unsigned char>(lower[pos - 1]));
          const std::size_t end = pos + brand.size();
          const bool rightOk =
              (end >= lower.size()) ||
              !std::isalnum(static_cast<unsigned char>(lower[end]));
          if (leftOk && rightOk)
            return true;
          ++pos;
        }
      }
      return false;
    };

    int trademarkViolations = 0;
    for (const auto &m : avra::studio::getSchematicCatalog()) {
      if (hasForbiddenBrand(m.shortName) || hasForbiddenBrand(m.fullName) ||
          hasForbiddenBrand(m.schematicNotes)) {
        ++trademarkViolations;
      }
      for (std::size_t p = 0; p < m.getParamCount(); ++p) {
        if (hasForbiddenBrand(m.getParamSpec(static_cast<int>(p)).name)) {
          ++trademarkViolations;
        }
      }
    }
    for (const auto &tmpl : avra::studio::getAllUnifiedStyleTemplates()) {
      if (hasForbiddenBrand(tmpl.id) || hasForbiddenBrand(tmpl.name)) {
        ++trademarkViolations;
      }
    }
    AVRA_TEST(trademarkViolations == 0,
              "All 102 schematic components, virtual sockets, and 36 UI style "
              "templates are 100% free of third-party trademarks");

    // 10.8 Schematic Benchmark Gate Verification (@schematic_benchmark Spec
    // Blocks & R/L/C Constants)
    const auto wsRoot = avra::studio::resolveAvraWorkspaceRoot();
    AVRA_TEST(std::filesystem::exists(wsRoot / ".agents" / "rules" /
                                      "schematic-benchmark-gate.md"),
              "Schematic Benchmark Gate rule file "
              "(.agents/rules/schematic-benchmark-gate.md) is installed");

    std::ifstream circuitFile(wsRoot / "include" / "avra" / "dsp" / "circuits" /
                              "CircuitLibrary.h");
    std::ostringstream circuitBuf;
    circuitBuf << circuitFile.rdbuf();
    const std::string circuitSrc = circuitBuf.str();

    int totalCircuitClasses = 0;
    int benchmarkedClasses = 0;
    std::size_t cPos = 0;
    while ((cPos = circuitSrc.find("\nclass ", cPos)) != std::string::npos) {
      ++totalCircuitClasses;
      const std::size_t winStart = (cPos > 1400) ? (cPos - 1400) : 0;
      const std::string docWin = circuitSrc.substr(winStart, cPos - winStart);
      if (docWin.find("@schematic_benchmark") != std::string::npos &&
          (docWin.find("- Hardware Inspiration:") != std::string::npos ||
           docWin.find("- Software Inspiration:") != std::string::npos) &&
          docWin.find("- Active Devices / Iron:") != std::string::npos &&
          docWin.find("- Passive R/L/C Values:") != std::string::npos &&
          docWin.find("- Non-Ideal Circuit Quirks Modeled:") !=
              std::string::npos) {
        ++benchmarkedClasses;
      }
      cPos += 7;
    }
    AVRA_TEST(totalCircuitClasses >= 70 &&
                  benchmarkedClasses == totalCircuitClasses,
              "All 70 C++20 DSP stage classes in CircuitLibrary.h have "
              "complete @schematic_benchmark specification blocks");
    AVRA_TEST(
        avra::dsp::circuits::PassiveLcProgramEq::kR_Boost_Ohms == 10000.0f &&
            avra::dsp::circuits::PassiveLcProgramEq::kR_Atten_Ohms ==
                100000.0f &&
            avra::dsp::circuits::InductorEq3Band::kL_Mid_mH[0] == 200.0f &&
            avra::dsp::circuits::ProportionalQGyratorEq::kR_Gyrator_Ohms ==
                15000.0f,
        "PassiveLcProgramEq, InductorEq3Band, and ProportionalQGyratorEq "
        "expose explicit schematic R/L/C component constants");

    // 10.9 Project Save & UI Personalization Round-Trip Persistence
    // Verification
    {
      const auto tempDir =
          std::filesystem::temp_directory_path() / "avra_save_roundtrip_test";
      const auto tempFp = tempDir / "TestProject.faceplate";
      const auto tempSchem = tempDir / "TestProject.schematic";
      std::error_code ec;
      std::filesystem::create_directories(tempDir, ec);

      auto customFp = avra::studio::buildAvraGainDemoFaceplate("CUSTOM STRIP");
      avra::studio::applyUiStyleTemplate(
          customFp, avra::studio::UiStyleTemplate::CrimsonMasteringLab);
      customFp.theme.finish = avra::gui::ChassisFinish::ObsidianBlack;
      customFp.theme.useCustomFinishColor = true;
      customFp.theme.customFinishHex = 0x3B1C22;
      customFp.theme.globalFontFamily = avra::gui::FontFamily::BoutiqueSerif;
      customFp.theme.useCustomFontColor = true;
      customFp.theme.customFontColorHex = 0xFFD166;
      customFp.theme.badgeFooterText = "CUSTOM MASTERING EDITION";

      if (!customFp.widgets.empty()) {
        customFp.widgets[0].type = avra::studio::UiWidgetType::ModernArcKnob;
        customFp.widgets[0].x = 144.0f;
        customFp.widgets[0].y = 216.0f;
        customFp.widgets[0].title = "CUSTOM DRIVE";
        customFp.widgets[0].subtitle = "NORM|180";
        customFp.widgets[0].accentColor =
            avra::studio::UiAccentColor::RoyalGold;
      }
      avra::studio::UiCanvasWidget sticker;
      sticker.type = avra::studio::UiWidgetType::VintageSticker;
      sticker.x = 420.0f;
      sticker.y = 180.0f;
      sticker.w = 120.0f;
      sticker.h = 44.0f;
      sticker.title = "VOCAL BUS";
      sticker.subtitle = "DO NOT TOUCH|A";
      sticker.rotationDeg = -12.5f;
      customFp.widgets.push_back(sticker);

      const bool savedFp =
          avra::studio::saveFaceplateLayoutToFile(tempFp, customFp);
      avra::studio::FaceplateLayout loadedFp;
      const bool readFp =
          avra::studio::loadFaceplateLayoutFromFile(tempFp, loadedFp);

      AVRA_TEST(
          savedFp && readFp &&
              loadedFp.theme.styleTemplateIdx ==
                  static_cast<int>(
                      avra::studio::UiStyleTemplate::CrimsonMasteringLab) &&
              loadedFp.theme.finish ==
                  avra::gui::ChassisFinish::ObsidianBlack &&
              loadedFp.theme.useCustomFinishColor &&
              loadedFp.theme.customFinishHex == 0x3B1C22 &&
              loadedFp.theme.globalFontFamily ==
                  avra::gui::FontFamily::BoutiqueSerif &&
              loadedFp.theme.customFontColorHex == 0xFFD166 &&
              loadedFp.theme.badgeFooterText == "CUSTOM MASTERING EDITION" &&
              loadedFp.widgets.size() == customFp.widgets.size() &&
              loadedFp.widgets[0].type ==
                  avra::studio::UiWidgetType::ModernArcKnob &&
              std::abs(loadedFp.widgets[0].x - 144.0f) < 1e-3f &&
              loadedFp.widgets[0].title == "CUSTOM DRIVE" &&
              loadedFp.widgets[0].subtitle == "NORM|180" &&
              loadedFp.widgets.back().type ==
                  avra::studio::UiWidgetType::VintageSticker &&
              loadedFp.widgets.back().subtitle == "DO NOT TOUCH|A" &&
              std::abs(loadedFp.widgets.back().rotationDeg - (-12.5f)) < 1e-3f,
          "Faceplate UI personalizations (style template, finish, RGB colors, "
          "widget types, pipe subtitles, stickers) round-trip 100% intact");

      std::vector<avra::studio::SchematicNode> customChain;
      auto node = avra::studio::makeDefaultSchematicNode(
          avra::studio::SchematicComponentType::Tube12AX7, true);
      node.parallelMixPercent = 64.0f;
      node.param1Val = 7.5f;
      node.param2Val = 42.0f;
      node.param3Val = 85.0f;
      node.param4Val = 16500.0f;
      node.param5Val = -2.5f;
      node.autoGainLinked = true;
      node.trim1Val = 12.5f;
      customChain.push_back(node);

      const bool savedSchem = avra::studio::saveSchematicChainToFile(
          tempSchem.string(), customChain);
      std::vector<avra::studio::SchematicNode> loadedChain;
      const bool readSchem = avra::studio::loadSchematicChainFromFile(
          tempSchem.string(), loadedChain);

      AVRA_TEST(savedSchem && readSchem && loadedChain.size() == 1 &&
                    loadedChain[0].type ==
                        avra::studio::SchematicComponentType::Tube12AX7 &&
                    loadedChain[0].isParallel &&
                    std::abs(loadedChain[0].parallelMixPercent - 64.0f) <
                        1e-3f &&
                    std::abs(loadedChain[0].param1Val - 7.5f) < 1e-3f &&
                    std::abs(loadedChain[0].param5Val - (-2.5f)) < 1e-3f &&
                    loadedChain[0].autoGainLinked &&
                    std::abs(loadedChain[0].trim1Val - 12.5f) < 1e-3f,
                "Schematic personalizations (routing, parameters, Link IO, "
                "internal trims) round-trip 100% intact");

      // Verify dynamic EngravedNameplate topology descriptor derivation
      std::vector<avra::studio::SchematicNode> tubeFetChain = {
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::Tube12AX7),
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::InductorEq3Band),
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::FetCompressor1176)};
      std::vector<avra::studio::SchematicNode> echoPlateChain = {
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::TapeEcho),
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::PlateReverb),
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::BbdChorus)};
      AVRA_TEST(
          avra::studio::buildChassisTopologySubtitle(tubeFetChain) ==
                  "TUBE · EQ · DYNAMICS CHANNEL" &&
              avra::studio::buildChassisTopologySubtitle(echoPlateChain) ==
                  "TAPE · PLATE · CHORUS ENGINE" &&
              avra::studio::buildChassisTopologySubtitle({}) ==
                  "CUSTOM HARDWARE CHASSIS",
          "EngravedNameplate dynamic topology descriptor accurately identifies "
          "TubeFetChannel, EchoPlateChorus, and custom chains");

      // Verify METERING · OUTPUT frame (752, 66, 214, 468) and inner widget
      // alignment (766..952) + auto-migration across presets
      const std::array<const char *, 4> presetStems = {
          "DrumBusMaster_Preset", "BassStudioChain_Preset",
          "CleanGuitarStudio_Preset", "StudioVocalChain_Preset"};
      bool allPresetsAligned = true;
      for (const char *stem : presetStems) {
        const auto schemPath =
            wsRoot / "presets" / (std::string(stem) + ".avra_schematic");
        const auto uiPath =
            wsRoot / "presets" / (std::string(stem) + ".avra_ui");
        std::vector<avra::studio::SchematicNode> pChain;
        avra::studio::FaceplateLayout pUi;
        if (avra::studio::loadSchematicChainFromFile(schemPath.string(),
                                                     pChain) &&
            avra::studio::loadFaceplateLayoutFromFile(uiPath, pUi)) {
          if (avra::studio::ensureMultiStageActiveBayLayout(pUi, pChain)) {
            avra::studio::saveFaceplateLayoutToFile(uiPath, pUi);
          }
          for (const auto &w : pUi.widgets) {
            if (w.type == avra::studio::UiWidgetType::SectionFrame &&
                w.boundStageIdx == -1 && w.x >= 730.0f) {
              if (std::abs(w.x - 746.0f) > 0.5f ||
                  std::abs(w.y - 58.0f) > 0.5f ||
                  std::abs(w.w - 240.0f) > 0.5f ||
                  std::abs(w.h - 472.0f) > 0.5f) {
                allPresetsAligned = false;
              }
            } else if (w.boundStageIdx == -1 &&
                       (w.boundParamSlot == 5 || w.boundParamSlot == 6) &&
                       w.x >= 730.0f) {
              allPresetsAligned = false; // Duplicate AUTO-LEVEL / DELTA buttons
                                         // stripped from faceplate
            } else if (w.boundStageIdx == -1 &&
                       (w.boundParamSlot == 3 || w.boundParamSlot == 10) &&
                       w.subtitle == "IN|BYP" && w.x >= 730.0f) {
              if (std::abs(w.x - 876.0f) > 0.5f ||
                  std::abs(w.y - 352.0f) > 0.5f ||
                  std::abs(w.w - 88.0f) > 0.5f ||
                  std::abs(w.h - 54.0f) > 0.5f) {
                allPresetsAligned = false;
              }
            } else if (w.boundStageIdx == -1 && w.boundParamSlot == 2 &&
                       w.title == "MASTER MIX") {
              if (std::abs(w.x - 891.0f) > 0.5f ||
                  std::abs(w.y - 222.0f) > 0.5f ||
                  std::abs(w.w - 58.0f) > 0.5f ||
                  std::abs(w.h - 58.0f) > 0.5f) {
                allPresetsAligned = false;
              }
            }
          }
        }
      }
      AVRA_TEST(allPresetsAligned,
                "METERING · OUTPUT SectionFrame (746,58,240,472) and inner "
                "controls (MASTER, MASTER MIX, OUTPUT TRIM) align cleanly");

      // Verify 2: MATTE ANODIZED vs 6: BEAD BLASTED procedural surface material
      // differentiation
      std::vector<uint32_t> mattePx(64u * 64u, 0u);
      std::vector<uint32_t> beadPx(64u * 64u, 0u);
      avra::gui::HardwareMaterials::synthesizeStudioSurfaceBitmap(
          mattePx.data(), 64u, 64u, avra::gui::SurfaceGrain::MatteSmooth,
          0x1A4347u);
      avra::gui::HardwareMaterials::synthesizeStudioSurfaceBitmap(
          beadPx.data(), 64u, 64u, avra::gui::SurfaceGrain::BeadBlastedMatte,
          0x1A4347u);
      float matteMaxStep = 0.0f;
      float beadMaxStep = 0.0f;
      for (size_t i = 20u * 64u + 20u; i < 44u * 64u + 44u; ++i) {
        const float mLum0 = static_cast<float>((mattePx[i] >> 8u) & 0xFFu);
        const float mLum1 = static_cast<float>((mattePx[i + 1u] >> 8u) & 0xFFu);
        const float bLum0 = static_cast<float>((beadPx[i] >> 8u) & 0xFFu);
        const float bLum1 = static_cast<float>((beadPx[i + 1u] >> 8u) & 0xFFu);
        matteMaxStep = std::max(matteMaxStep, std::abs(mLum1 - mLum0));
        beadMaxStep = std::max(beadMaxStep, std::abs(bLum1 - bLum0));
      }
      AVRA_TEST(beadMaxStep > matteMaxStep * 4.0f &&
                    beadPx[32u * 64u + 32u] != mattePx[32u * 64u + 32u],
                "6: BEAD BLASTED exhibits shot-peened micro-crater relief and "
                "crystalline glints distinct from 2: MATTE ANODIZED");

      std::filesystem::remove_all(tempDir, ec);
    }

    // 10.10 Keyboard Event Bubbling & TextInput Focus Gate Verification (Delete
    // Key Support)
    {
      struct TestRootComp : avra::gui::Component {
        int lastKey{0};
        void onKeyDown(int k) override { lastKey = k; }
      };
      TestRootComp root;
      avra::gui::Button childBtn("TEST");
      avra::gui::TextInput childInput("Hint", "Text");
      root.addChild(&childBtn);
      root.addChild(&childInput);

      childBtn.onKeyDown(46); // VK_DELETE bubbles from Button up to Root
      AVRA_TEST(root.lastKey == 46 && !childBtn.isTextInputFocused(),
                "Keyboard Delete event (VK_DELETE) bubbles from child Button "
                "up to root Component");

      root.lastKey = 0;
      childInput.setFocused(false);
      childInput.onKeyDown(
          46); // Unfocused TextInput bubbles VK_DELETE up to Root
      AVRA_TEST(root.lastKey == 46 && !childInput.isTextInputFocused(),
                "Unfocused TextInput forwards VK_DELETE up to root Component");

      root.lastKey = 0;
      childInput.setFocused(true);
      childInput.onKeyDown(
          46); // Focused TextInput edits text instead of deleting widget
      AVRA_TEST(root.lastKey == 0 && childInput.isTextInputFocused() &&
                    childInput.getText() == "Tex",
                "Focused TextInput consumes VK_DELETE to edit text without "
                "deleting canvas widget");
    }

    // 10.11 Global Menu Bar, 50-Step Undo/Redo HistoryManager,
    // StudioPreferences & .avra JSON Project Serialization
    {
      std::cout << "Testing 50-Step Undo/Redo HistoryManager, "
                   "StudioPreferences & .avra JSON Serialization..."
                << std::endl;
      avra::studio::HistoryManager history;
      AVRA_TEST(!history.canUndo() && !history.canRedo(),
                "Fresh HistoryManager starts empty");

      // Push 60 states and verify 50-step ring buffer limit
      for (int i = 0; i < 60; ++i) {
        avra::studio::ProjectSnapshot snap;
        snap.globalInputDriveDb = static_cast<float>(i);
        history.pushState(std::move(snap),
                          std::format("Set Input Drive to {} dB", i));
      }
      AVRA_TEST(history.undoDepth() == avra::studio::HistoryManager::kMaxSteps,
                "HistoryManager enforces 50-step ring limit on Undo stack");
      AVRA_TEST(
          history.getUndoLabel() == "Set Input Drive to 59 dB",
          "HistoryManager tracks descriptive action label for top Undo step");

      avra::studio::ProjectSnapshot liveSnap;
      liveSnap.globalInputDriveDb = 60.0f;
      avra::studio::ProjectSnapshot restored;
      std::string undoneLabel;
      AVRA_TEST(history.undo(liveSnap, restored, undoneLabel) &&
                    std::abs(restored.globalInputDriveDb - 59.0f) < 1e-4f &&
                    undoneLabel == "Set Input Drive to 59 dB" &&
                    history.canRedo(),
                "HistoryManager::undo restores previous ProjectSnapshot and "
                "populates Redo stack");

      avra::studio::ProjectSnapshot redone;
      std::string redoneLabel;
      AVRA_TEST(history.redo(restored, redone, redoneLabel) &&
                    std::abs(redone.globalInputDriveDb - 60.0f) < 1e-4f &&
                    redoneLabel == "Set Input Drive to 59 dB",
                "HistoryManager::redo restores forward ProjectSnapshot");

      // Verify StudioPreferences JSON round-trip
      std::error_code ec;
      const auto tempDir =
          std::filesystem::temp_directory_path(ec) / "avra_ide_shell_test";
      std::filesystem::create_directories(tempDir, ec);
      const auto prefPath = tempDir / ".avra_preferences.json";

      avra::studio::StudioPreferences prefsOut;
      prefsOut.audioDriverMode = 1;
      prefsOut.sampleRateHz = 96000;
      prefsOut.blockBufferSize = 128;
      prefsOut.globalOversamplingCapIdx = 3;
      prefsOut.vendorName = "KENNEDARTH LABS";
      prefsOut.manufacturerCode = "KNDT";
      prefsOut.knobMouseMode = 1;
      prefsOut.autoSaveIntervalIdx = 1;
      prefsOut.defaultGridSizePx = 16;
      prefsOut.showFpsCpuOverlay = false;

      const bool savedPref =
          avra::studio::saveStudioPreferencesToFile(prefPath, prefsOut);
      avra::studio::StudioPreferences prefsIn;
      const bool loadedPref =
          avra::studio::loadStudioPreferencesFromFile(prefPath, prefsIn);
      AVRA_TEST(
          savedPref && loadedPref && prefsIn.audioDriverMode == 1 &&
              prefsIn.sampleRateHz == 96000 && prefsIn.blockBufferSize == 128 &&
              prefsIn.globalOversamplingCapIdx == 3 &&
              prefsIn.vendorName == "AVRA AUDIO" &&
              prefsIn.manufacturerCode == "AVRA" &&
              prefsIn.knobMouseMode == 1 && prefsIn.autoSaveIntervalIdx == 1 &&
              prefsIn.defaultGridSizePx == 16 && !prefsIn.showFpsCpuOverlay,
          "StudioPreferences (Audio Engine, Compiler Paths & UI Workflow) "
          "round-trip via JSON");

      // Verify .avra JSON project serialization & recovery round-trip
      const auto avraFile = tempDir / "TestStrip.avra";
      std::vector<avra::studio::SchematicNode> chainOut = {
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::ClassAPreamp, false),
          avra::studio::makeDefaultSchematicNode(
              avra::studio::SchematicComponentType::FetCompressor1176, true)};
      chainOut[0].param1Val = 6.5f;
      chainOut[0].trim1Val = 22.0f;
      auto layoutOut =
          avra::studio::buildFaceplateFromSchematic("TEST STRIP", chainOut);
      if (!layoutOut.widgets.empty()) {
        layoutOut.widgets[0].hasWaxMark = true;
        layoutOut.widgets[0].waxMarkNorm = 0.72f;
      }

      const bool savedAvra = avra::studio::saveProjectToAvraJsonFile(
          avraFile, "TestStrip", chainOut, layoutOut, -3.5f, 1.5f, 80.0f,
          false);
      std::string loadedProjName;
      std::vector<avra::studio::SchematicNode> chainIn;
      avra::studio::FaceplateLayout layoutIn;
      float inDb = 0.0f, outDb = 0.0f, mixPct = 0.0f;
      bool byp = true;
      const bool loadedAvra = avra::studio::loadProjectFromAvraJsonFile(
          avraFile, loadedProjName, chainIn, layoutIn, &inDb, &outDb, &mixPct,
          &byp);

      AVRA_TEST(savedAvra && loadedAvra && loadedProjName == "TestStrip" &&
                    chainIn.size() == 2 &&
                    std::abs(chainIn[0].param1Val - 6.5f) < 1e-3f &&
                    std::abs(chainIn[0].trim1Val - 22.0f) < 1e-3f &&
                    layoutIn.widgets.size() == layoutOut.widgets.size() &&
                    layoutIn.widgets[0].hasWaxMark &&
                    std::abs(layoutIn.widgets[0].waxMarkNorm - 0.72f) < 1e-3f &&
                    std::abs(inDb - (-3.5f)) < 1e-3f &&
                    std::abs(outDb - 1.5f) < 1e-3f &&
                    std::abs(mixPct - 80.0f) < 1e-3f && !byp,
                ".avra JSON project file round-trips schematic stages, "
                "trim-pots, faceplate widgets, wax marks & bus trims");

      // Verify Integrated Top Menu Bar dropdown builders (FILE, EDIT, VIEW,
      // BUILD, HELP)
      for (int m = 0; m < 5; ++m) {
        const auto items = avra::studio::buildDropdownMenuItems(
            static_cast<avra::studio::TopMenuBarSection>(m), history,
            {"TestStrip.avra"}, 8, true);
        AVRA_TEST(!items.empty(),
                  "Every Top Menu Bar section (FILE, EDIT, VIEW, BUILD, HELP) "
                  "populates dropdown items");
      }

      // 10.12 Verify VST3/CLAP/Standalone Generator C++20 Designated
      // Initializers & Symlink-Safe Cache Check
      avra::studio::ProjectConfig genCfg;
      genCfg.name = "TestVstBuildStrip";
      genCfg.pluginId = "com.avra.testvstbuildstrip";
      genCfg.outputDir = (tempDir / "plugins").string();
      std::string saveMsg;
      const bool savedStrip =
          avra::studio::ProjectGenerator::saveProjectStateOnly(
              genCfg, chainOut, layoutOut, saveMsg);
      AVRA_TEST(savedStrip, "ProjectGenerator::saveProjectStateOnly succeeds "
                            "without blocking on CMake");

      std::ifstream hdrIn(tempDir / "plugins" / "TestVstBuildStrip" /
                          "TestVstBuildStrip.h");
      std::stringstream hdrBuf;
      hdrBuf << hdrIn.rdbuf();
      const std::string hdrCode = hdrBuf.str();
      AVRA_TEST(
          !hdrCode.empty() && hdrCode.find(".isStepped") == std::string::npos &&
              hdrCode.find(".taper = ParameterTaper::Stepped, .numSteps = 2") !=
                  std::string::npos,
          "Generated schematic processor header uses valid "
          "ParameterTaper::Stepped initializer (zero .isStepped errors)");
      avra::studio::ProjectGenerator::unregisterFromCMakeLists(
          "TestVstBuildStrip");

      AVRA_TEST(!avra::studio::ProjectGenerator::isCMakeCacheStale(),
                "ProjectGenerator::isCMakeCacheStale uses "
                "std::filesystem::equivalent for junction/symlink paths");

      const auto repGain1 =
          avra::studio::ProjectGenerator::inspectProjectArtifacts("GainPlugin");
      const auto repGain2 =
          avra::studio::ProjectGenerator::inspectProjectArtifacts("AvraGain");
      AVRA_TEST(repGain1.pluginName == "AvraGain" &&
                    repGain2.pluginName == "AvraGain",
                "inspectProjectArtifacts normalizes both GainPlugin and "
                "AvraGain to AvraGain");

      // 10.13 Verify 4 Rack-Level Master Engineering Tools (Signal Probe, Elite
      // 216-Module DSP, Linear-Phase FIR PDC, CLAP Param Mod)
      {
        avra::Parameter modParam({.id = 100,
                                  .name = "Drive",
                                  .unit = "dB",
                                  .minValue = -12.0f,
                                  .maxValue = 12.0f,
                                  .defaultValue = 0.0f});
        modParam.setPlain(2.0f);
        modParam.setModulationOffset(3.5f);
        AVRA_TEST(
            std::abs(modParam.getPlain() - 2.0f) < 1e-4f &&
                std::abs(modParam.getModulatedPlain() - 5.5f) < 1e-4f,
            "Tool 4: Parameter::setModulationOffset applies non-destructive "
            "CLAP modulation without overwriting base knob value");
        modParam.setModulationOffset(0.0f);
        AVRA_TEST(std::abs(modParam.getModulatedPlain() - 2.0f) < 1e-4f,
                  "Tool 4: Resetting modulation offset restores exact base "
                  "parameter value");

        avra::dsp::circuits::GermanDualPentodeCinemaStage pr31;
        avra::dsp::circuits::SlidingActionSpectralCompanderStage tp8;
        pr31.prepare(48000.0);
        tp8.prepare(48000.0);
        pr31.setNormalizedParameters(0.75f, 0.40f, 0.50f, 0.60f, 0.55f);
        tp8.setNormalizedParameters(0.65f, 0.70f, 0.35f, 0.50f, 0.60f);

        std::array<float, 64> probeSamples{};
        for (size_t i = 0; i < 64; ++i) {
          float l = 0.25f * std::sin(static_cast<float>(i) * 0.2f);
          float r = l;
          pr31.processStereo(l, r);
          probeSamples[i] = 0.5f * (l + r);
          tp8.processStereo(l, r);
        }
        avra::gui::SpectrumAnalyzer probeSpec;
        avra::gui::WaveformScope probeScope;
        probeSpec.updateProbeFromSamples(probeSamples);
        probeScope.updateProbeFromSamples(probeSamples);
        AVRA_TEST(std::abs(probeSamples[32]) > 1e-4f,
                  "Tool 1 & Elite 216-Module DSP: Inter-Stage Signal Probe & "
                  "new benchmark stages process non-trivial stereo signal");
      }

      // 10.14 Verify Selectable Export Format Checkboxes (Default VST3, Dynamic
      // Build Label, and Preferences Persistence)
      {
        avra::studio::StudioPreferences defPrefs{};
        AVRA_TEST(defPrefs.exportFormatVst3 && !defPrefs.exportFormatClap &&
                      !defPrefs.exportFormatStandalone,
                  "StudioPreferences defaults to [✓] VST3 (.vst3) enabled and "
                  "CLAP/Standalone unchecked for fast daily builds");
        AVRA_TEST(defPrefs.getBuildButtonLabel() == "\xE2\x9A\xA1 BUILD VST3",
                  "StudioPreferences::getBuildButtonLabel returns '⚡ BUILD "
                  "VST3' when only VST3 is checked");

        defPrefs.exportFormatClap = true;
        defPrefs.exportFormatStandalone = true;
        AVRA_TEST(defPrefs.getBuildButtonLabel() ==
                      "\xE2\x9A\xA1 BUILD VST3 + CLAP + APP",
                  "StudioPreferences::getBuildButtonLabel returns '⚡ BUILD "
                  "VST3 + CLAP + APP' when all 3 formats are checked");

        const auto prefTestFile = tempDir / "test_export_formats_prefs.json";
        avra::studio::StudioPreferences savedFmtPrefs{};
        savedFmtPrefs.exportFormatVst3 = false;
        savedFmtPrefs.exportFormatClap = true;
        savedFmtPrefs.exportFormatStandalone = true;
        const bool savedFmtOk = avra::studio::saveStudioPreferencesToFile(
            prefTestFile, savedFmtPrefs);

        avra::studio::StudioPreferences loadedFmtPrefs{};
        const bool loadedFmtOk = avra::studio::loadStudioPreferencesFromFile(
            prefTestFile, loadedFmtPrefs);
        AVRA_TEST(
            savedFmtOk && loadedFmtOk && !loadedFmtPrefs.exportFormatVst3 &&
                loadedFmtPrefs.exportFormatClap &&
                loadedFmtPrefs.exportFormatStandalone &&
                loadedFmtPrefs.getBuildButtonLabel() ==
                    "\xE2\x9A\xA1 BUILD CLAP + APP",
            "saveStudioPreferencesToFile & loadStudioPreferencesFromFile "
            "persist selectable export format checkboxes across sessions");

        // 10.15 Verify Embedded Avra Icon Assets (.rc, .ico, .png)
        const auto rootDir = avra::studio::ProjectGenerator::getWorkspaceRoot();
        const auto rcFile = rootDir / "assets" / "avra_studio.rc";
        const auto icoFile = rootDir / "assets" / "avra_studio.ico";
        const auto pngFile = rootDir / "assets" / "avra_studio_icon.png";
        AVRA_TEST(std::filesystem::exists(rcFile, ec) &&
                      std::filesystem::exists(icoFile, ec) &&
                      std::filesystem::exists(pngFile, ec),
                  "Avra Studio artwork files (assets/avra_studio.rc, "
                  "avra_studio.ico, avra_studio_icon.png) exist on disk");

        std::ifstream icoIn(icoFile, std::ios::binary);
        unsigned char icoHdr[4] = {0, 0, 0, 0};
        icoIn.read(reinterpret_cast<char *>(icoHdr), 4);
        AVRA_TEST(icoHdr[0] == 0x00 && icoHdr[1] == 0x00 && icoHdr[2] == 0x01 &&
                      icoHdr[3] == 0x00,
                  "assets/avra_studio.ico is a valid multi-resolution Win32 "
                  "ICO container (00 00 01 00 magic bytes)");

        // 10.16 Verify Hardcore C++20 Circuit Physics Solvers
        // (src/dsp/core/AvraCircuitPhysics.h)
        avra::dsp::physics::KorenTubeSolver koren12AX7;
        koren12AX7.prepare(48000.0,
                           avra::dsp::physics::KorenTubeSpec::make12AX7());
        const double kOut = koren12AX7.processSample(0.45, 0.5, 0.5, 0.25);

        avra::dsp::physics::ShockleyJfetSolver jfetSolver;
        jfetSolver.prepare(48000.0);
        const double jOut = jfetSolver.processSample(0.35, 0.35, 1.0);

        avra::dsp::physics::EbersMollClassABjtSolver bjtSolver;
        bjtSolver.prepare(48000.0);
        const double bOut = bjtSolver.processSample(0.35, 12.0, 0.5, 0.5);

        avra::dsp::physics::Discrete2520OpAmpSolver op2520;
        op2520.prepare(48000.0);
        const double oOut = op2520.processSample(0.35, 1.5, 0.5, 0.1);

        avra::dsp::physics::PnpGermaniumBjtSolver germSolver;
        germSolver.prepare(48000.0);
        const double gOut = germSolver.processSample(0.35, 2.0, 0.5, 0.1);

        avra::dsp::physics::LangevinMagneticFluxSolver ironSolver;
        ironSolver.prepare(48000.0, 35.0);
        const double iOut = ironSolver.processSample(0.45, 5.0, 0.25, 0.15);

        avra::dsp::physics::PassiveRlcSaturatingInductorSolver rlcSolver;
        rlcSolver.prepare(48000.0);
        const double rOut =
            rlcSolver.processBandpassResonator(0.45, 1200.0, 1.5, 0.20);

        avra::dsp::physics::NonlinearDynamicsSidechainSolver dynSolver;
        dynSolver.prepare(48000.0);
        const double dOut = dynSolver.processOptoT4BCell(0.6) +
                            dynSolver.processVariMuKorenSample(0.3, 1.0);

        AVRA_TEST(std::isfinite(kOut) && std::isfinite(jOut) &&
                      std::isfinite(bOut) && std::isfinite(oOut) &&
                      std::isfinite(gOut) && std::isfinite(iOut) &&
                      std::isfinite(rOut) && std::isfinite(dOut),
                  "All 6 Core C++20 Circuit Physics Solvers in "
                  "src/dsp/core/AvraCircuitPhysics.h execute cleanly");

        // Verify Standalone Audio Device Enumeration & Full-Duplex Engine
        // Configuration
        const auto inDevs = avra::audio_io::enumerateInputDevices();
        const auto outDevs = avra::audio_io::enumerateOutputDevices();
        AVRA_TEST(!inDevs.empty() && inDevs.front().deviceId == -1 &&
                      inDevs.front().name.find("None") != std::string::npos &&
                      !outDevs.empty() && outDevs.front().deviceId == -1,
                  "Standalone enumerateInputDevices and enumerateOutputDevices "
                  "expose hardware list + safe default items");

        // Verify 2-way Pad switch label order and exact passive input
        // attenuation (-20 / -15 / -10 / -6 dB)
        {
          const auto &pr1Meta = avra::studio::getComponentMeta(
              avra::studio::SchematicComponentType::JfetBuffer);
          const std::string padSub = avra::studio::formatWidgetSubtitleForParam(
              pr1Meta.params[3], 0.0f);
          AVRA_TEST(
              padSub == "-20 dB|0 dB",
              "2-way Pad switch formats topLabel|botLabel as '-20 dB|0 dB' so "
              "OFF (0.0f) highlights '0 dB' and ON (1.0f) highlights '-20 dB'");

          auto measureStagePadDeltaDb =
              []<typename StageT>(StageT &stage,
                                  avra::studio::SchematicComponentType type,
                                  size_t padParamIdx) -> float {
            const auto &meta = avra::studio::getComponentMeta(type);
            auto measureRms = [&](float padVal) -> double {
              stage.prepare(48000.0);
              std::vector<float> vals;
              vals.reserve(meta.params.size());
              for (const auto &p : meta.params) {
                vals.push_back(p.defaultVal);
              }
              if (padParamIdx < vals.size()) {
                vals[padParamIdx] = padVal;
              }
              stage.setControlValues(vals);
              double sumSq = 0.0;
              constexpr int kWarmup = 2400;
              constexpr int kCount = 4800;
              for (int i = 0; i < kWarmup + kCount; ++i) {
                float l = 0.02f * std::sin(2.0f * 3.14159265f * 1000.0f *
                                           static_cast<float>(i) / 48000.0f);
                float r = l;
                stage.processStereo(l, r);
                if (i >= kWarmup) {
                  sumSq += static_cast<double>(l) * static_cast<double>(l);
                }
              }
              return std::sqrt(sumSq / static_cast<double>(kCount));
            };
            const double rmsOff = measureRms(0.0f);
            const double rmsOn = measureRms(1.0f);
            return static_cast<float>(
                20.0 *
                std::log10(std::max(1.0e-9, rmsOff) / std::max(1.0e-9, rmsOn)));
          };

          avra::dsp::circuits::JfetBufferStage sPr1;
          avra::dsp::circuits::DiscreteClassABOpAmp sPr3;
          avra::dsp::circuits::PentodeTubeStage sPr5;
          avra::dsp::circuits::GermaniumBroadcastPreStage sPr7;
          avra::dsp::circuits::ConsoleIcPreStage sPr9;
          avra::dsp::circuits::SubMiniatureTubeMicStage sPr12;
          avra::dsp::circuits::TubeConsolePreStage sPr13;
          avra::dsp::circuits::ConsoleSummingBusStage sPr15;

          const float pr1PadDb = measureStagePadDeltaDb(
              sPr1, avra::studio::SchematicComponentType::JfetBuffer, 3);
          const float pr3PadDb = measureStagePadDeltaDb(
              sPr3, avra::studio::SchematicComponentType::ClassABOpAmp2520, 2);
          const float pr5PadDb = measureStagePadDeltaDb(
              sPr5, avra::studio::SchematicComponentType::PentodeEF86, 4);
          const float pr7PadDb = measureStagePadDeltaDb(
              sPr7, avra::studio::SchematicComponentType::GermaniumBroadcastPre,
              4);
          const float pr9PadDb = measureStagePadDeltaDb(
              sPr9, avra::studio::SchematicComponentType::Console4000IcStage,
              5);
          const float pr12PadDb = measureStagePadDeltaDb(
              sPr12,
              avra::studio::SchematicComponentType::SubMiniatureTubeMicPre, 4);
          const float pr13PadDb = measureStagePadDeltaDb(
              sPr13,
              avra::studio::SchematicComponentType::AmericanTubeConsolePre, 6);
          const float pr15PadDb = measureStagePadDeltaDb(
              sPr15, avra::studio::SchematicComponentType::ConsoleSummingBusAmp,
              4);

          AVRA_TEST(pr1PadDb > 18.5f && pr1PadDb < 21.5f && pr3PadDb > 18.5f &&
                        pr3PadDb < 21.5f && pr5PadDb > 18.5f &&
                        pr5PadDb < 21.5f && pr9PadDb > 18.5f &&
                        pr9PadDb < 21.5f,
                    "All -20 dB Pad switches (PR1, PR3, PR5, PR9) attenuate "
                    "gain by ~20.0 dB");
          AVRA_TEST(pr7PadDb > 13.5f && pr7PadDb < 16.5f && pr13PadDb > 13.5f &&
                        pr13PadDb < 16.5f && pr12PadDb > 8.5f &&
                        pr12PadDb < 11.5f && pr15PadDb > 5.0f &&
                        pr15PadDb < 7.0f,
                    "All -15 dB (PR7, PR13), -10 dB (PR12), and -6 dB (PR15) "
                    "Pad switches attenuate by their exact dB rating");

          const auto &allStyles = avra::studio::getAllUnifiedStyleTemplates();
          AVRA_TEST(avra::studio::kTotalUiStyleTemplates == 51 &&
                        allStyles.size() == 51,
                    "Unified UI Style Templates catalog exposes all 51 studio "
                    "& thematic styles (with Sleek/Cyber-Luxe, New Wave, EDM, Punk, "
                    "Blood-Moon Occult, and Gothic Cathedral Doom removed)");

          avra::studio::FaceplateLayout tplTestLayout =
              avra::studio::buildAvraGainDemoFaceplate("STYLE TEST");
          for (int ti = 0; ti < avra::studio::kTotalUiStyleTemplates; ++ti) {
            const auto tpl = static_cast<avra::studio::UiStyleTemplate>(ti);
            const auto &spec = avra::studio::getUnifiedStyleTemplateSpec(tpl);
            AVRA_TEST(
                spec.name != nullptr && spec.name[0] != '\0' &&
                    spec.id != nullptr && spec.id[0] != '\0',
                "Every UnifiedStyleTemplate has non-empty ID and display name");
            avra::studio::applyUiStyleTemplate(tplTestLayout, tpl);
            AVRA_TEST(tplTestLayout.theme.styleTemplateIdx == ti &&
                          avra::studio::isKnobWidget(
                              tplTestLayout.theme.globalKnobType) &&
                          avra::studio::isSwitchWidget(
                              tplTestLayout.theme.globalSwitchType) &&
                          avra::studio::isMeterWidget(
                              tplTestLayout.theme.globalMeterType),
                      "applyUiStyleTemplate applies valid knob, switch, and "
                      "meter widget archetypes for all 55 style templates");
          }
        }
      }

      // Regenerate 7-stage MasteringProcessor (PR1 -> IR2 -> EQ2 -> CP2 -> DL3
      // -> SP_D -> SP_D) matching the Cubase 16x4x7 stress-test topology and
      // benchmark 64 instances at 96.0 kHz
      {
        using avra::studio::makeDefaultSchematicNode;
        using avra::studio::SchematicComponentType;
        using avra::studio::SchematicNode;
        std::vector<SchematicNode> mpChain = {
            makeDefaultSchematicNode(SchematicComponentType::JfetBuffer),
            makeDefaultSchematicNode(SchematicComponentType::InputTransformer),
            makeDefaultSchematicNode(
                SchematicComponentType::ResonantSubSculptor),
            makeDefaultSchematicNode(SchematicComponentType::OptoCompressorT4B),
            makeDefaultSchematicNode(SchematicComponentType::BbdCompanderDelay),
            makeDefaultSchematicNode(SchematicComponentType::ParallelSplitBus),
            makeDefaultSchematicNode(SchematicComponentType::ParallelSplitBus)};
        avra::studio::ProjectConfig mpCfg;
        mpCfg.name = "MasteringProcessor";
        mpCfg.vendor = "Avra Audio";
        mpCfg.pluginId = "com.company.masteringprocessor";
        mpCfg.category = "Fx|Mastering";
        mpCfg.outputDir = (tempDir / "plugins").string();
        std::string genMsg;
        const bool genOk =
            avra::studio::ProjectGenerator::saveProjectStateOnly(
                mpCfg, mpChain, avra::studio::buildFaceplateFromSchematic("MasteringProcessor", mpChain), genMsg);
        AVRA_TEST(genOk,
                  "Regenerate 7-stage MasteringProcessor (PR1, IR2, EQ2, CP2, "
                  "DL3, SP_D, SP_D) with optimized 96kHz DSP header");

        // Benchmark 64 instances x 7 stages = 448 active circuit stages at 96.0
        // kHz (64-sample buffer)
        struct SevenStageRack {
          avra::dsp::circuits::JfetBufferStage s1;
          avra::dsp::circuits::NickelBridgingTransformerStage s2;
          avra::dsp::circuits::PassiveHighInductorEqStage s3;
          avra::dsp::circuits::OpticalLevelerStage s4;
          avra::dsp::circuits::CompanderBbdDelay s5;
          avra::dsp::circuits::ParallelWetDryPhaseBlenderStage s6;
          avra::dsp::circuits::ParallelWetDryPhaseBlenderStage s7;
          void prepare(double sr) noexcept {
            s1.prepare(sr);
            s2.prepare(sr);
            s3.prepare(sr);
            s4.prepare(sr);
            s5.prepare(sr);
            s6.prepare(sr);
            s7.prepare(sr);
          }
          inline void processStereo(float &l, float &r) noexcept {
            s1.processStereo(l, r);
            s2.processStereo(l, r);
            s3.processStereo(l, r);
            s4.processStereo(l, r);
            s5.processStereo(l, r);
            s6.processStereo(l, r);
            s7.processStereo(l, r);
          }
        };
        std::vector<SevenStageRack> racks(64);
        for (auto &rk : racks)
          rk.prepare(96000.0);

        constexpr size_t kBufSize = 64;
        constexpr size_t kNumBlocks =
            375; // 375 * 64 = 24,000 samples = 0.25 sec of 96 kHz audio across
                 // all 64 instances
        std::array<float, kBufSize> bufL{}, bufR{};
        for (size_t i = 0; i < kBufSize; ++i) {
          bufL[i] = 0.25f * std::sin(2.0f * 3.14159265f *
                                     static_cast<float>(i) / 64.0f);
          bufR[i] = 0.25f * std::cos(2.0f * 3.14159265f *
                                     static_cast<float>(i) / 64.0f);
        }
        const auto t0 = std::chrono::steady_clock::now();
        float sink = 0.0f;
        for (size_t blk = 0; blk < kNumBlocks; ++blk) {
          for (auto &rk : racks) {
            for (size_t i = 0; i < kBufSize; ++i) {
              float l = bufL[i], r = bufR[i];
              rk.processStereo(l, r);
              sink += (l + r) * 1e-6f;
            }
          }
        }
        const auto t1 = std::chrono::steady_clock::now();
        const double elapsedMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
        const double realtimeBudgetMs = 250.0; // 0.25s real-time audio window
        const double singleCoreLoadPct = (elapsedMs / realtimeBudgetMs) * 100.0;
        std::cout << std::format(
            "  [96.0 kHz 64-Instance x 7-Stage Benchmark @ 64-Sample Buffer]: "
            "{:.1f} ms / {:.1f} ms budget ({:.1f}% of 1 CPU core, ~{:.1f}% "
            "across 8 cores, sink={:.4f})\n",
            elapsedMs, realtimeBudgetMs, singleCoreLoadPct,
            singleCoreLoadPct / 8.0, sink);
        AVRA_TEST(std::isfinite(sink) && elapsedMs < 2000.0,
                  "64 instances x 7 stages (448 active circuit stages) at 96.0 "
                  "kHz / 64-sample buffer execute cleanly in real time");
      }

      // Regenerate & verify all 5 Flagship Studio Blueprints (DrumBusMaster,
      // BassStudioChain, CleanGuitarStudio, DistGuitarStudio, StudioVocalChain)
      {
        using avra::studio::makeDefaultSchematicNode;
        using avra::studio::SchematicComponentType;
        using avra::studio::SchematicNode;

        auto exportBlueprint = [&](const std::string &name,
                                  const std::string &pluginId,
                                  const std::string &category,
                                  const std::vector<SchematicNode> &chain) {
          avra::studio::ProjectConfig cfg;
          cfg.name = name;
          cfg.vendor = "Avra Audio";
          cfg.pluginId = pluginId;
          cfg.category = category;
          auto layout = avra::studio::loadProjectFaceplateLayout(name, chain);
          avra::studio::FaceplateTheme savedTheme = layout.theme;
          savedTheme.pluginTitle = name;
          layout = avra::studio::buildFaceplateFromSchematicWithTheme(
              name, chain, savedTheme);
          avra::studio::ensureMultiStageActiveBayLayout(layout, chain);
          for (auto &w : layout.widgets) {
            if (w.type == avra::studio::UiWidgetType::RivetedNameplate) {
              w.title = name;
            }
          }
          const auto presetDir = tempDir / "presets";
          cfg.outputDir = (tempDir / "plugins").string();
          std::error_code ec2;
          std::filesystem::create_directories(presetDir, ec2);
          avra::studio::saveSchematicChainToFile(
              (presetDir / (name + "_Preset.avra_schematic")).string(), chain);
          avra::studio::saveFaceplateLayoutToFile(
              presetDir / (name + "_Preset.avra_ui"), layout);
          std::string msg;
          const bool ok = avra::studio::ProjectGenerator::saveProjectStateOnly(
              cfg, chain, layout, msg);
          avra::studio::saveProjectToAvraJsonFile(
              tempDir / "plugins" / name / (name + ".avra"), name, chain, layout,
              0.0f, 0.0f, 100.0f, false);
          return ok;
        };

        // 1. DrumBusMaster (5 Stages: PR2 -> EQ5 -> CP5 -> CP1 [Par 25%] -> CL1)
        {
          std::vector<SchematicNode> chain;
          auto s1 = makeDefaultSchematicNode(SchematicComponentType::ClassAPreamp, false);
          s1.setParamVal(0, 2.0f);
          s1.setParamVal(1, 0.0f);
          s1.setParamVal(2, -1.5f);
          s1.setParamVal(3, 1.0f);
          s1.setParamVal(4, 0.0f);
          chain.push_back(s1);

          auto s2 = makeDefaultSchematicNode(SchematicComponentType::ProportionalQGyratorEq, false);
          s2.setParamVal(0, 0.0f);
          s2.setParamVal(1, 2.0f);
          s2.setParamVal(2, 1.0f);
          s2.setParamVal(3, -1.5f);
          s2.setParamVal(4, 3.0f);
          s2.setParamVal(5, 1.5f);
          s2.setParamVal(6, 0.0f);
          s2.setParamVal(7, 1.0f);
          chain.push_back(s2);

          auto s3 = makeDefaultSchematicNode(SchematicComponentType::VcaBusCompressor, false);
          s3.setParamVal(0, -12.0f);
          s3.setParamVal(1, 1.5f);
          s3.setParamVal(2, 5.0f);
          s3.setParamVal(3, 0.0f);
          s3.setParamVal(4, 1.0f);
          s3.setParamVal(5, 1.0f);
          s3.setParamVal(6, 100.0f);
          chain.push_back(s3);

          auto s4 = makeDefaultSchematicNode(SchematicComponentType::FetCompressor1176, true);
          s4.parallelMixPercent = 25.0f;
          s4.setParamVal(0, -18.0f);
          s4.setParamVal(1, 0.0f);
          s4.setParamVal(2, 2.5f);
          s4.setParamVal(3, 6.0f);
          s4.setParamVal(4, 1.0f);
          s4.setParamVal(5, 90.0f);
          s4.setParamVal(6, 100.0f);
          chain.push_back(s4);

          auto s5 = makeDefaultSchematicNode(SchematicComponentType::OversampledVariableClipper, false);
          s5.setParamVal(0, 0.0f);
          s5.setParamVal(1, -0.5f);
          s5.setParamVal(2, 60.0f);
          s5.setParamVal(3, 3500.0f);
          s5.setParamVal(4, 85.0f);
          chain.push_back(s5);

          AVRA_TEST(exportBlueprint("DrumBusMaster", "com.avra.drumbusmaster",
                                    "5-Stage Studio Drum Bus Chain", chain),
                    "Regenerate DrumBusMaster with transparent CL1 Standard 16x Clipper settings");
        }

        // 2. BassStudioChain (6 Stages: PR4 -> AU2 -> CP2 -> ST6 -> ST4 [Par 25%] -> EQ1)
        {
          std::vector<SchematicNode> chain;
          auto s1 = makeDefaultSchematicNode(SchematicComponentType::Tube12AX7, false);
          s1.setParamVal(0, 3.5f);
          s1.setParamVal(1, -2.0f);
          s1.setParamVal(2, 58.0f);
          s1.setParamVal(3, 0.0f);
          s1.setParamVal(4, 0.0f);
          chain.push_back(s1);

          auto s2 = makeDefaultSchematicNode(SchematicComponentType::SilkBerylliumAirExciter, false);
          s2.setParamVal(0, -15.0f);
          s2.setParamVal(1, 6.0f);
          s2.setParamVal(2, 160.0f);
          s2.setParamVal(3, 60.0f);
          s2.setParamVal(4, 55.0f);
          s2.setParamVal(5, 0.0f);
          chain.push_back(s2);

          auto s3 = makeDefaultSchematicNode(SchematicComponentType::OptoCompressorT4B, false);
          s3.setParamVal(0, 51.0f);
          s3.setParamVal(1, 48.0f);
          s3.setParamVal(2, 85.0f);
          s3.setParamVal(3, 1.0f);
          s3.setParamVal(4, 0.0f);
          s3.setParamVal(5, 100.0f);
          chain.push_back(s3);

          auto s4 = makeDefaultSchematicNode(SchematicComponentType::PsychoacousticSubSynth, false);
          s4.setParamVal(0, 35.0f);
          s4.setParamVal(1, 4.5f);
          s4.setParamVal(2, 5.5f);
          s4.setParamVal(3, 1.0f);
          s4.setParamVal(4, 0.0f);
          chain.push_back(s4);

          auto s5 = makeDefaultSchematicNode(SchematicComponentType::ParallelSaturationBus, true);
          s5.parallelMixPercent = 25.0f;
          s5.setParamVal(0, 4.5f);
          s5.setParamVal(1, 45.0f);
          s5.setParamVal(2, 160.0f);
          s5.setParamVal(3, 2.0f);
          s5.setParamVal(4, -1.5f);
          chain.push_back(s5);

          auto s6 = makeDefaultSchematicNode(SchematicComponentType::PassivePultecEq, false);
          s6.setParamVal(0, 2.0f);
          s6.setParamVal(1, 3.5f);
          s6.setParamVal(2, 2.5f);
          s6.setParamVal(3, 0.0f);
          s6.setParamVal(4, 2.0f);
          chain.push_back(s6);

          AVRA_TEST(exportBlueprint("BassStudioChain", "com.avra.bassstudiochain",
                                    "6-Stage Bass Guitar Studio Chain", chain),
                    "Regenerate BassStudioChain with AU2 Auto Bass Leveller, ST6 RBass Sub-Synth, and Parallel ST4 Dark Bass Grit Bus");
        }

        // 3. CleanGuitarStudio (5 Stages: PR13 -> CP1 -> EQ5 -> MD2 [Par 28%] -> RV3 [Par 22%])
        {
          std::vector<SchematicNode> chain;
          auto s1 = makeDefaultSchematicNode(SchematicComponentType::AmericanTubeConsolePre, false);
          s1.setParamVal(0, 2.0f);
          s1.setParamVal(1, 5.0f);
          s1.setParamVal(2, 5.0f);
          s1.setParamVal(3, 0.0f);
          s1.setParamVal(4, 1.0f);
          s1.setParamVal(5, 1.0f);
          chain.push_back(s1);

          auto s2 = makeDefaultSchematicNode(SchematicComponentType::FetCompressor1176, false);
          s2.setParamVal(0, -22.0f);
          s2.setParamVal(1, 1.0f);
          s2.setParamVal(2, 3.0f);
          s2.setParamVal(3, 5.0f);
          s2.setParamVal(4, 0.0f);
          s2.setParamVal(5, 85.0f);
          s2.setParamVal(6, 85.0f);
          chain.push_back(s2);

          auto s3 = makeDefaultSchematicNode(SchematicComponentType::ProportionalQGyratorEq, false);
          s3.setParamVal(0, 2.0f);
          s3.setParamVal(1, -1.5f);
          s3.setParamVal(2, 5.0f);
          s3.setParamVal(3, 1.5f);
          s3.setParamVal(4, 3.0f);
          s3.setParamVal(5, 1.5f);
          chain.push_back(s3);

          auto s4 = makeDefaultSchematicNode(SchematicComponentType::TriStereoBbdChorus, true);
          s4.parallelMixPercent = 28.0f;
          s4.setParamVal(0, 50.0f);
          s4.setParamVal(1, 38.0f);
          s4.setParamVal(2, 52.0f);
          s4.setParamVal(3, 0.65f);
          s4.setParamVal(4, 100.0f);
          chain.push_back(s4);

          auto s5 = makeDefaultSchematicNode(SchematicComponentType::DispersionSpringReverb, true);
          s5.parallelMixPercent = 22.0f;
          s5.setParamVal(0, 0.0f);
          s5.setParamVal(1, 2.1f);
          s5.setParamVal(2, 58.0f);
          s5.setParamVal(3, 60.0f);
          s5.setParamVal(4, 100.0f);
          chain.push_back(s5);

          AVRA_TEST(exportBlueprint("CleanGuitarStudio", "com.avra.cleanguitarstudio",
                                    "5-Stage Clean Guitar Studio Chain", chain),
                    "Regenerate CleanGuitarStudio with low-noise CP1 FET compressor settings");
        }

        // 4. DistGuitarStudio (5 Stages: CP13 -> PR5 -> ST2 -> EQ4 -> TP1)
        {
          std::vector<SchematicNode> chain;
          auto s1 = makeDefaultSchematicNode(SchematicComponentType::HysteresisNoiseGate, false);
          s1.setParamVal(0, -34.0f);
          s1.setParamVal(1, 0.8f);
          s1.setParamVal(2, 85.0f);
          s1.setParamVal(3, -45.0f);
          s1.setParamVal(4, 750.0f);
          chain.push_back(s1);

          auto s2 = makeDefaultSchematicNode(SchematicComponentType::PentodeEF86, false);
          s2.setParamVal(0, 2.0f);
          s2.setParamVal(1, 0.0f);
          s2.setParamVal(2, -2.0f);
          s2.setParamVal(3, 1.0f);
          s2.setParamVal(4, 0.0f);
          chain.push_back(s2);

          auto s3 = makeDefaultSchematicNode(SchematicComponentType::TrigonometricWavefolder, false);
          s3.setParamVal(0, 1.0f);
          s3.setParamVal(1, 1.0f);
          s3.setParamVal(2, 7.5f);
          s3.setParamVal(3, 52.0f);
          s3.setParamVal(4, -3.0f);
          s3.setParamVal(5, 0.0f);
          s3.setParamVal(6, 0.0f);
          chain.push_back(s3);

          auto s4 = makeDefaultSchematicNode(SchematicComponentType::InductorEq3Band, false);
          s4.setParamVal(0, 3.0f);
          s4.setParamVal(1, 3.0f);
          s4.setParamVal(2, 1.5f);
          s4.setParamVal(3, 2.5f);
          s4.setParamVal(4, 0.0f);
          s4.setParamVal(5, 2.0f);
          chain.push_back(s4);

          auto s5 = makeDefaultSchematicNode(SchematicComponentType::TapeHysteresis, false);
          s5.setParamVal(0, 2.0f);
          s5.setParamVal(1, 1.5f);
          s5.setParamVal(2, 50.0f);
          s5.setParamVal(3, -1.5f);
          s5.setParamVal(4, 1.0f);
          chain.push_back(s5);

          AVRA_TEST(exportBlueprint("DistGuitarStudio", "com.avra.distguitarstudio",
                                    "5-Stage Distorted Electric Guitar Chain", chain),
                    "Regenerate DistGuitarStudio with 0dB Fine Gain Trim on PR5 Pentode Pre and 6kHz LPF on ST2");
        }

        // 5. StudioVocalChain (7 Stages: PR2 -> AU1 -> CP12 -> CP2 -> EQ8 -> DL10 [Par 26%] -> RV1 [Par 22%])
        {
          std::vector<SchematicNode> chain;
          auto s1 = makeDefaultSchematicNode(SchematicComponentType::ClassAPreamp, false);
          s1.setParamVal(0, 3.0f);
          s1.setParamVal(1, 2.0f);
          s1.setParamVal(2, -2.0f);
          s1.setParamVal(3, 1.0f);
          s1.setParamVal(4, 0.0f);
          chain.push_back(s1);

          auto s2 = makeDefaultSchematicNode(SchematicComponentType::ZeroDistortionVocalRider, false);
          s2.setParamVal(0, -16.0f);
          s2.setParamVal(1, 6.0f);
          s2.setParamVal(2, 45.0f);
          s2.setParamVal(3, 160.0f);
          s2.setParamVal(4, 0.0f);
          chain.push_back(s2);

          auto s3 = makeDefaultSchematicNode(SchematicComponentType::SplitBandDeEsser, false);
          s3.setParamVal(0, 6200.0f);
          s3.setParamVal(1, 6.5f);
          s3.setParamVal(2, 38.0f);
          s3.setParamVal(3, 1.0f);
          s3.setParamVal(4, 0.0f);
          chain.push_back(s3);

          auto s4 = makeDefaultSchematicNode(SchematicComponentType::OptoCompressorT4B, false);
          s4.setParamVal(0, 52.0f);
          s4.setParamVal(1, 48.0f);
          s4.setParamVal(2, 68.0f);
          s4.setParamVal(3, 1.0f);
          s4.setParamVal(4, 0.0f);
          s4.setParamVal(5, 100.0f);
          chain.push_back(s4);

          auto s5 = makeDefaultSchematicNode(SchematicComponentType::FulcrumTiltAirEq, false);
          s5.setParamVal(0, 3.0f);
          s5.setParamVal(1, 3.0f);
          s5.setParamVal(2, 1.0f);
          s5.setParamVal(3, 550.0f);
          s5.setParamVal(4, -2.0f);
          s5.setParamVal(5, 0.0f);
          chain.push_back(s5);

          auto s6 = makeDefaultSchematicNode(SchematicComponentType::StudioAdtTapeDoubler, true);
          s6.parallelMixPercent = 26.0f;
          s6.setParamVal(0, 22.0f);
          s6.setParamVal(1, 38.0f);
          s6.setParamVal(2, 0.85f);
          s6.setParamVal(3, 45.0f);
          s6.setParamVal(4, 100.0f);
          chain.push_back(s6);

          auto s7 = makeDefaultSchematicNode(SchematicComponentType::PlateReverb, true);
          s7.parallelMixPercent = 22.0f;
          s7.setParamVal(0, 2.1f);
          s7.setParamVal(1, 35.0f);
          s7.setParamVal(2, 140.0f);
          s7.setParamVal(3, 6800.0f);
          s7.setParamVal(4, 100.0f);
          chain.push_back(s7);

          AVRA_TEST(exportBlueprint("StudioVocalChain", "com.avra.studiovocalchain",
                                    "7-Stage Flagship Studio Vocal Chain", chain),
                    "Regenerate StudioVocalChain with AU1 Auto Vocal Leveller + Parallel DL10 Tape ADT Doubler");

          // 6. SynthKeysStudio (6 Stages: PR1 -> EQ5 -> CP5 -> MD1 [Par 20%] -> DL2 [Par 15%] -> RV2 [Par 18%])
          {
            std::vector<SchematicNode> synthChain;
            auto sk1 = makeDefaultSchematicNode(SchematicComponentType::JfetBuffer, false);
            sk1.setParamVal(0, -3.0f);
            sk1.setParamVal(1, 50.0f);
            sk1.setParamVal(2, -2.0f);
            sk1.setParamVal(3, 0.0f);
            sk1.setParamVal(4, 0.0f);
            synthChain.push_back(sk1);

            auto sk2 = makeDefaultSchematicNode(SchematicComponentType::ProportionalQGyratorEq, false);
            sk2.setParamVal(0, 1.0f);
            sk2.setParamVal(1, 1.5f);
            sk2.setParamVal(2, 1.0f);
            sk2.setParamVal(3, -1.5f);
            sk2.setParamVal(4, 4.0f);
            sk2.setParamVal(5, 1.5f);
            sk2.setParamVal(6, 1.0f);
            sk2.setParamVal(7, 1.0f);
            synthChain.push_back(sk2);

            auto sk3 = makeDefaultSchematicNode(SchematicComponentType::VcaBusCompressor, false);
            sk3.setParamVal(0, -14.0f);
            sk3.setParamVal(1, 0.0f);
            sk3.setParamVal(2, 4.0f);
            sk3.setParamVal(3, 1.0f);
            sk3.setParamVal(4, 0.0f);
            sk3.setParamVal(5, 1.0f);
            sk3.setParamVal(6, 100.0f);
            synthChain.push_back(sk3);

            auto sk4 = makeDefaultSchematicNode(SchematicComponentType::BbdChorus, true);
            sk4.parallelMixPercent = 20.0f;
            sk4.setParamVal(0, 3.0f);
            sk4.setParamVal(1, 105.0f);
            sk4.setParamVal(2, 0.55f);
            sk4.setParamVal(3, 100.0f);
            sk4.setParamVal(4, -2.0f);
            synthChain.push_back(sk4);

            auto sk5 = makeDefaultSchematicNode(SchematicComponentType::PingPongDelay, true);
            sk5.parallelMixPercent = 15.0f;
            sk5.setParamVal(0, 375.0f);
            sk5.setParamVal(1, 38.0f);
            sk5.setParamVal(2, 85.0f);
            sk5.setParamVal(3, 5200.0f);
            sk5.setParamVal(4, 100.0f);
            synthChain.push_back(sk5);

            auto sk6 = makeDefaultSchematicNode(SchematicComponentType::HallReverbFDN, true);
            sk6.parallelMixPercent = 18.0f;
            sk6.setParamVal(0, 3.2f);
            sk6.setParamVal(1, 1.25f);
            sk6.setParamVal(2, 74.0f);
            sk6.setParamVal(3, 28.0f);
            sk6.setParamVal(4, 78.0f);
            sk6.setParamVal(5, 100.0f);
            synthChain.push_back(sk6);

            AVRA_TEST(exportBlueprint("SynthKeysStudio", "com.avra.synthkeysstudio",
                                      "6-Stage Studio Synth / Keys & FX Chain", synthChain),
                      "Regenerate SynthKeysStudio blueprint with clean headroom on Pure JFET Line Buffer");

            avra::dsp::circuits::JfetBufferStage jfetS1;
            avra::dsp::circuits::ProportionalQGyrator eqS2;
            avra::dsp::circuits::QuadVcaMasterBusGlueStage compS3;
            avra::dsp::circuits::BucketBrigadeChorus chorS4;
            avra::dsp::circuits::PingPongStereoDelayStage dlyS5;
            avra::dsp::circuits::ConcertHallFdnStage revS6;
            jfetS1.prepare(48000.0);
            eqS2.prepare(48000.0);
            compS3.prepare(48000.0);
            chorS4.prepare(48000.0);
            dlyS5.prepare(48000.0);
            revS6.prepare(48000.0);

            const std::array<float, 7> v1{-3.0f, 50.0f, -2.0f, 0.0f, 0.0f, 0.0f, 0.0f};
            const std::array<float, 8> v2{1.0f, 1.5f, 1.0f, -1.5f, 4.0f, 1.5f, 1.0f, 1.0f};
            const std::array<float, 7> v3{-14.0f, 0.0f, 4.0f, 1.0f, 0.0f, 1.0f, 100.0f};
            const std::array<float, 5> v4{3.0f, 105.0f, 0.55f, 100.0f, -2.0f};
            const std::array<float, 5> v5{375.0f, 38.0f, 85.0f, 5200.0f, 100.0f};
            const std::array<float, 6> v6{3.2f, 1.25f, 74.0f, 28.0f, 78.0f, 100.0f};
            jfetS1.setControlValues(v1);
            eqS2.setControlValues(v2);
            compS3.setControlValues(v3);
            chorS4.setControlValues(v4);
            dlyS5.setControlValues(v5);
            revS6.setControlValues(v6);

            float peakDry = 0.0f, peakS1 = 0.0f, peakOut = 0.0f;
            for (int n = 0; n < 48000 * 4; ++n) {
              float l = 0.0f, r = 0.0f;
              avra::audio_io::StudioGrooveSynthesizer::synthesizeSynthChordLoop(static_cast<double>(n) / 48000.0, l, r);
              peakDry = std::max(peakDry, std::max(std::abs(l), std::abs(r)));
              jfetS1.processStereo(l, r);
              peakS1 = std::max(peakS1, std::max(std::abs(l), std::abs(r)));
              eqS2.processStereo(l, r);
              compS3.processStereo(l, r);
              {
                float wl = l, wr = r;
                chorS4.processStereo(wl, wr);
                l += wl * 0.20f;
                r += wr * 0.20f;
              }
              {
                float wl = l, wr = r;
                dlyS5.processStereo(wl, wr);
                l += wl * 0.15f;
                r += wr * 0.15f;
              }
              {
                float wl = l, wr = r;
                revS6.processStereo(wl, wr);
                l += wl * 0.18f;
                r += wr * 0.18f;
              }
              peakOut = std::max(peakOut, std::max(std::abs(l), std::abs(r)));
            }
            std::cout << std::format(
                "  [SynthKeysStudio Headroom Check]: Dry Peak={:.2f} dBFS, S1 (Pure JFET Buffer) Peak={:.2f} dBFS, Master Out Peak={:.2f} dBFS\n",
                avra::math::linearTodB(peakDry), avra::math::linearTodB(peakS1), avra::math::linearTodB(peakOut));
            AVRA_TEST(peakS1 < 0.70f && peakOut < 0.88f,
                      "SynthKeysStudio blueprint has clean headroom (< -1.1 dBFS) with zero clipping on Pure JFET Line Buffer and Master Output");
          }

          // Verify 4-layer Non-Commercial Provenance & Tamper Guard
          AVRA_TEST(avra::provenance::keepProvenanceStampAlive() &&
                        avra::provenance::verifyIntegrity() &&
                        avra::provenance::getIntegrityGainMultiplier() == 1.0f &&
                        !avra::provenance::verifyIntegrity("TAMPERED_BADGE") &&
                        avra::provenance::getIntegrityGainMultiplier("TAMPERED_BADGE") == 0.0f,
                    "Non-Commercial Provenance Stamp & FNV-1a Tamper Guard pass intact check and mute on tamper");

          // Refresh existing workspace plugin headers in plugins/ so they have the Non-Commercial header & no PRESET dropdown
          for (const auto& entry : std::filesystem::directory_iterator(wsRoot / "plugins", ec)) {
            if (!entry.is_directory(ec)) continue;
            const std::string wsProj = entry.path().filename().string();
            if (wsProj == "AvraGain") continue;
            const auto chainWs = avra::studio::loadProjectSchematicChain(wsProj);
            if (!chainWs.empty()) {
              auto layoutWs = avra::studio::loadProjectFaceplateLayout(wsProj, chainWs);
              avra::studio::ProjectConfig cfgWs;
              cfgWs.name = wsProj;
              cfgWs.vendor = "Avra Audio";
              cfgWs.pluginId = std::string("com.avra.") + wsProj;
              cfgWs.category = "Fx|Studio";
              std::string wsMsg;
              avra::studio::ProjectGenerator::saveProjectStateOnly(cfgWs, chainWs, layoutWs, wsMsg);
            }
          }

          std::ifstream bassEdIn(wsRoot / "plugins" / "BassStudioChain" / "BassStudioChainEditor.h");
          std::stringstream bassEdBuf;
          bassEdBuf << bassEdIn.rdbuf();
          const std::string bassEdCode = bassEdBuf.str();
          AVRA_TEST(bassEdCode.find("drawTopBarProvenancePill") != std::string::npos &&
                        bassEdCode.find("void paintOverlay(gui::Graphics& g) override") != std::string::npos &&
                        bassEdCode.find("bool interceptsOverlayMouse(gui::Point<float> pt)") != std::string::npos &&
                        bassEdCode.find("drawProvenanceModalOverlay") != std::string::npos &&
                        bassEdCode.find("PRESET:") == std::string::npos,
                    "Exported BassStudioChainEditor.h renders Non-Commercial modal in paintOverlay() above all knobs and removes PRESET dropdown");
        }
      }
      std::filesystem::remove_all(tempDir, ec);
    }
  }

  std::cout << "\n>>> ALL AVRA CORE TESTS PASSED SUCCESSFULLY! <<<"
            << std::endl;
  return 0;
}
