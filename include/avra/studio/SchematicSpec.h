#pragma once

#include "avra/dsp/Math.h"
#include "avra/gui/Geometry.h"
#include <array>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <format>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace avra::studio {

/**
 * @brief Dynamically resolves the portable Avra SDK / Workspace root directory relative to the running executable.
 * Ensures Avra works from any installation folder without hardcoded paths.
 */
inline const std::filesystem::path& resolveAvraWorkspaceRoot() {
    namespace fs = std::filesystem;
    static const fs::path s_cachedRoot = []() -> fs::path {
#if defined(_WIN32)
        char exePathBuf[MAX_PATH]{};
        if (GetModuleFileNameA(nullptr, exePathBuf, MAX_PATH) > 0) {
            fs::path cur = fs::path(exePathBuf).parent_path();
            for (int depth = 0; depth < 5; ++depth) {
                if (fs::exists(cur / "include" / "avra" / "Avra.h")) {
                    return cur;
                }
                if (!cur.has_parent_path() || cur.parent_path() == cur) break;
                cur = cur.parent_path();
            }
        }
#endif
        fs::path cwd = fs::current_path();
        for (int depth = 0; depth < 5; ++depth) {
            if (fs::exists(cwd / "include" / "avra" / "Avra.h")) {
                return cwd;
            }
            if (!cwd.has_parent_path() || cwd.parent_path() == cwd) break;
            cwd = cwd.parent_path();
        }
        return fs::current_path();
    }();
    return s_cachedRoot;
}

enum class SchematicCategory : int {
    PreampsAndTubes     = 0, // SLOT B: Preamp, Op-Amp & Tube Topologies
    IronAndTape         = 1, // SLOT A: Input/Output Iron & Magnetic Tape
    EqAndFilters        = 2, // SLOT D: Passive LC, Inductor & Gyrator Filter Networks
    AnalogDynamics      = 3, // SLOT C/D: Sidechain Detectors & Analog Gain Cells
    DelayAndReverb      = 4, // SLOT E: BBD Companders, Tape Echo, MicroPitch & Figure-8 Tanks
    ModulationAndRoute  = 5, // MODULATION, M/S MATRIX & PARALLEL BUS ROUTING
    AutoAndSmart        = 6  // MODERN AUTOMATION, LEVEL RIDERS, DYNAMIC EQ & SMART GAIN-STAGING
};

enum class SchematicComponentType {
    // 0: Preamps & Tubes (Slot B)
    ClassAPreamp,
    British73Preamp = ClassAPreamp,
    Tube12AX7,
    PentodeEF86,
    JfetBuffer,

    // 1: Transformers & Tape (Slot A)
    InputTransformer,
    OutputTransformer,
    TapeHysteresis,

    // 2: EQ & Filters (Slot D)
    InductorEq3Band,
    PassivePultecEq,
    BaxandallTone,
    ResonantFilter,

    // 3: Analog Dynamics (Slot C & D)
    FetCompressor1176,
    OptoCompressorT4B,
    VcaBusCompressor,
    TransientShaper,
    BrickwallLimiter,

    // 4: Delay & Reverb (Slot E)
    TapeEcho,
    PingPongDelay,
    PlateReverb,
    HallReverbFDN,

    // 5: Modulation, Stereo & Routing
    BbdChorus,
    AnalogPhaser,
    MidSideImager,
    ParallelSplitBus,

    // --- Extended 5-Slot Modular Sub-Blocks (Appended for 100% file compatibility) ---
    ClassABOpAmp2520,        // Slot B: Discrete Class-AB Op-Amp with Slew Limiting
    OutputTransformerMatrix, // Slot A: Switchable Nickel / Iron / Steel Output Matrix
    ProportionalQGyratorEq,  // Slot D: Proportional-Q Op-Amp Gyrator EQ (Q = Q0 * 2^(|G|/4))
    ParallelPassiveLcEq,     // Slot D: 4-Band Parallel Passive LC Tank + Tube Makeup
    VariMuTubeCompressor,    // Slot C/D: Remote-Cutoff Push-Pull Vari-Mu Tube Cell
    DiodeBridgeCompressor,   // Slot C/D: Balanced 4-Diode Bridge Feedback Attenuator + Class-A Makeup
    DualOptoVcaCompressor,   // Slot C/D: Serial Optical + Class-A VCA + 3-Core Iron Matrix
    SidechainThrustMatrix,   // Slot C: Variable HPF + 10dB/dec Pink-Noise Tilt + Dual-Cap Auto
    BbdCompanderDelay,       // Slot E: 2:1 RMS Compander + 4096-Stage Clock-LPF BBD
    MicroPitchHarmonizer,    // Slot E: 16-Bit PCM + Deglitched Dual-Voice Detuned Pitch Shifter

    // --- Extended Slots F-J: Digital-Native / Algorithmic VST Sub-Blocks ---
    TransientTonalSplitter,      // Slot F: Complementary Transient / Tonal Stream Splitter (M + (1-M) = 1)
    SpectralResonanceSuppressor, // Slot G: Local Spectral Median Dynamic Notch Array
    BarkAuditoryUnmasker,        // Slot G: Bark-Scale Auditory Scene Tame & Recover (CASA Mode)
    PsychoacousticSubSynth,      // Slot H: Missing-Fundamental Full-Wave Rectifier Sub Synthesizer
    StatelessHarmonicInflator,   // Slot H: Stateless Polynomial Loudness & Density Inflator (1.5x - 0.5x^3)
    OversampledVariableClipper,  // Slot I: 4x Polyphase Lp-Norm Variable-Knee Soft/Hard Clipper
    MonoCompatibleWidener,       // Slot J: 6-Stage All-Pass Phase-Pure Mono-Compatible Stereo Widener
    OctaveShimmerReverb,         // Slot J: +12st Pitch-Shifted Feedback Loop Shimmer FDN Tank

    // --- Extended Studio & Creative Component Blocks ---
    GermaniumBroadcastPre,         // Asymmetric Germanium Transistor Fuzz with Thermal Bias Drift
    ParallelSaturationBus,       // Cascaded CMOS Hex-Inverter Soft-Square Overdrive
    DacBitcrusherStage,          // R-2R Ladder Bitcrusher & Sample-Rate Decimator
    TrigonometricWavefolder,     // Multi-Stage Trigonometric Sine Wavefolder
    TransistorLadderFilter,      // 24 dB/oct 4-Pole Resonant Transistor Ladder VCF
    SidechainSpectralCarver32,          // Dual-Formant Vocal Vowel & Envelope Auto-Wah
    ProportionalGraphicEq10Band,     // Multi-Resonant Speaker Cabinet & Mic Axis Simulator
    FulcrumTiltAirEq,            // 650 Hz Fulcrum Spectral Tilt + 18 kHz Silk Air Shelf
    HysteresisNoiseGate,         // Schmitt-Trigger Dual-Threshold Hysteresis Noise Gate
    SplitBandDeEsser,            // Split-Band High-Frequency Sibilance Suppressor
    UpDownMultibandCompressor,   // 3-Band Upward + Downward Multiband Dynamics
    DispersionSpringReverb,      // Chirp-Dispersive All-Pass Helical Spring Reverb Tank
    NonlinearGatedReverb,        // Dense Early-Reflection Cluster + RMS Hold Gated Reverb
    ThroughZeroTapeFlanger,      // Through-Zero Dual-Tap Tape Flanger
    OpticalHarmonicTremolo,      // Optical Photoresistor Bias & Harmonic Auto-Tremolo
    DiodeRingSubOctaveMod,       // Balanced 4-Diode Ring Modulator & Flip-Flop Sub-Octave

    // --- Crown-Jewel Unobtainium Boutique Audio Electrics Components ---
    DirectHeatedTriode,          // NOS Direct-Heated Single-Ended Triode Valve (Zero-Feedback Filament)
    HighVoltageCascodeTube,      // 400V Dual-Triode Cascode Mastering Preamp
    SilverToroidalIron,          // 99.99% Pure Silver-Wound Amorphous Core Mastering Transformer
    PaperInOilInterstageCap,     // NOS Wax Paper-In-Oil & Silver-Mica Interstage Capacitor Coupler
    SilkBerylliumAirExciter,     // Toroidal Air Inductor & Beryllium-Voiced High-Sheen Exciter
    ResonantSubSculptor,         // Passive Resonant Sub-Bass Focus & Elliptical Tightener
    ZenerShuntLimiter,           // Matched Zener-Diode Shunt Mastering Limiter

    // --- Expanded 102-Module Library (Appended for 100% file compatibility) ---
    // Tab 2: PRE / TUBE additions (PR8..PR10, PR12..PR16)
    BlueStripeSiliconPre,        // PR8: Blue-Stripe Silicon BJT Pre
    Console4000IcStage,          // PR9: Console 4000 Bipolar IC Stage
    JapaneseTapeDeckPre,         // PR10: Japanese Tape-Deck Transistor
    SubMiniatureTubeMicPre,      // PR12: Sub-Miniature Tube Microphone Stage
    AmericanTubeConsolePre,      // PR13: Cathode-Biased Guitar Preamp
    GermanBroadcastValvePre,      // PR14: Optical FET Instrument DI
    ConsoleSummingBusAmp,        // PR15: Console Summing Bus Amp
    HeadroomCalibratorPre,       // PR16: dBFS-to-dBu Headroom Calibrator

    // Tab 3: IRON / SAT additions (TP2..TP3, ST1, ST5, ST7)
    HighFreqTapeLimiter,         // TP2: High-Frequency Tape Limiter
    LoFiCassetteVhsWarper,       // TP3: Lo-Fi Cassette & VHS Warper
    VariableSilkExciter,         // ST1: Variable Silk Harmonic Exciter
    VintageAuralExciter,         // ST5: Phase-Harmonic Air Exciter
    AsymmetricalDiodeClipper,    // ST7: Asymmetrical Diode Clipper

    // Tab 4: EQ / SPEC additions (SP5..SP6)
    HarmonicPitchTrackingEq,     // SP5: Harmonic Pitch-Tracking EQ
    PhasePolarityAligner,        // SP6: Continuous Phase & Polarity Aligner

    // Tab 5: COMP / CLIP additions (CP8, CP11, CP14, CL2)
    OverEasyRmsSmackVca,         // CP8: Vari-Knee RMS Smack VCA
    LookaheadUpwardExpander,     // CP11: Lookahead Upward Expander
    BroadcastTriodeLimiter,          // CP14: Bass-Compensated Guitar Sustainer
    PsychoacousticMasteringLimiter, // CL2: 26-Band Psychoacoustic Limiter

    // Tab 1: TIME / VERB additions (DL5..DL10, RV5..RV9)
    MagneticDrumEcho,            // DL5: Magnetic Drum Echo
    Companded12BitRackDelay,     // DL6: 12-Bit Companded Rack Delay
    DynamicDuckingDelay,         // DL7: Dynamic Ducking Delay
    Rhythmic8TapDiffuser,        // DL8: 8-Tap Rhythmic Diffuser
    AsynchronousGrainCloud,      // DL9: Asynchronous Grain Cloud
    StudioAdtTapeDoubler,        // DL10: Studio ADT Tape Doubler
    DispersiveSteelPlate,        // RV5: Dispersive Steel Plate
    TiledLiveDrumChamber,        // RV6: Tiled Live Drum Chamber
    EarlyReflectionAmbience,     // RV7: Early-Reflection Ambience
    InfiniteVelvetBlackhole,     // RV8: Infinite Abyssal Gravity Tank
    ZeroLatencyIrConvolver,      // RV9: Zero-Latency IR Convolver

    // Tab 6: SPLIT / MOD additions (SP_B, SP_E, MD2, CV1..CV4)
    LinkwitzRiley4BandCrossover, // SP_B: 4-Band Linkwitz-Riley Crossover
    FrequencyDuckingRouter,      // SP_E: Frequency-Conscious Ducking Router
    TriStereoBbdChorus,          // MD2: Tri-Stereo Bucket-Brigade Chorus
    CvAnalogDriftEngine,         // CV1: Analog Drift & Tolerance Engine
    CvMacroKnobHub,              // CV2: Multi-Target Macro Knob Hub
    CvEnvelopeFollowerSlew,      // CV3: Envelope Follower & Slew Shaper
    CvOrganicLfoPerlin,          // CV4: Organic LFO & Perlin Wander Generator

    // --- 180-Module Studio Engineering Catalog: Tab 1 (PR17..PR30) & Tab 2 (IR6..IR10, TP4..TP7, ST9..ST13) ---
    Olympic69LustraphonePre,     // PR17: Permalloy Class-A Console Strip
    Abbey68TransistorPre,        // PR18: Dual-Transistor Presence Strip
    LondonARangeZenerPre,        // PR19: Zener-Biased Console Strip
    WestCoastAm10Pre,            // PR20: West-Coast Discrete AM-10 Stage
    TransformerlessJfetBjtPre,   // PR21: Transformerless JFET-BJT Hybrid
    HighRail80BroadcastPre,      // PR22: High-Rail 80-Series Broadcast Pre
    Criteria2002HybridPre,       // PR23: Hybrid IC-Transistor Console Stage
    AirStudioZobelPre,           // PR24: Air-Studio Zobel Transformer Stage
    Soul60s24VClassAPre,         // PR25: 1960s Soul 24V Discrete Class-A
    LaStudioQuadTransistorPre,   // PR26: LA-Studio Discrete Quad-Transistor
    Octal50sBroadcastTubePre,    // PR27: 1950s Octal-Tube Broadcast Amp
    DavenSteppedTubePre,         // PR28: Dual-Triode Stepped Ladder Tube Pre
    SwissBroadcastLinePre,       // PR29: Swiss Precision Broadcast Line
    ZeroFeedbackMasteringPre,    // PR30: Zero-Feedback Mastering Transfer

    LayeredPermalloyInterstage,  // IR6: 1950s Layered-Permalloy Interstage
    Balanced600BridgingIron,     // IR7: Balanced 600-Ohm Bridging Transformer
    SiliconValveOutputIron,      // IR8: Grain-Oriented Silicon Valve Iron
    BesselWidebandServoIron,     // IR9: Bessel-Tuned Wideband Servo Iron
    CobaltAmorphousCoreIron,     // IR10: Uncut Cobalt Amorphous Core
    HalfInch30IpsMasteringTape,  // TP4: 1/2-Inch 30 IPS Mastering Deck
    OneInch15IpsTubeTape,        // TP5: 1-Inch 15 IPS Vacuum-Tube Deck
    DifferentialAirCompander,    // TP6: 4-Band Differential Air Compander
    StainlessWireRecorder,       // TP7: 1940s Stainless Wire Recorder
    MasteringVinylCuttingLathe,  // ST9: Mastering Vinyl Cutting Lathe
    TriodePentode3BandMatrix,    // ST10: 3-Band Split Triode/Pentode Matrix
    EnvelopeGatedTransientExciter, // ST11: Envelope-Gated Transient Exciter
    ConsoleBusRailOverdrive,       // ST12: +/-15V Console Bus Rail Overdrive
    StudioPcm14BitConverter,       // ST13: 1979 14-Bit Studio PCM Converter

    // --- 216-Module Master Catalog Expansion: Tab 1 (PR31..PR36) & Tab 2 (TP8..TP9, ST15..ST18) ---
    GermanDualPentodeCinemaPre,      // PR31: 1950s German Dual-Pentode Cinema Pre
    ZeroFeedbackPureTriodePre,       // PR32: Zero-Negative-Feedback Pure Triode
    CurrentModeTransimpedancePre,    // PR33: Current-Mode Transimpedance Line Amp
    EasternBroadcastPentodePre,      // PR34: 1960s Eastern Broadcast Pentode Strip
    JapaneseDiscreteClassAPre,       // PR35: 1970s Japanese Discrete Class-A Stage
    VariImpedanceCableLoadPre,       // PR36: Vari-Impedance Cable & Transformer Load
    SlidingActionSpectralCompander,  // TP8:  3-Band Sliding-Action Spectral Compander
    OpticalSoundtrackFilmExciter,    // TP9:  35mm Optical Soundtrack Film Exciter
    CollectorSaturationMatrix3Path,  // ST15: 3-Path Transistor Collector-Saturation Matrix
    DynamicFluxDeSaturator,          // ST16: Dynamic Flux De-Saturator
    GainRangingFloatingPointDac,     // ST17: 1978 Gain-Ranging Floating-Point DAC
    DielectricAbsorptionCapAging,    // ST18: Dielectric Absorption & Capacitor Aging Stage

    // --- Tab 3: EQ / SPEC additions (EQ11..EQ23, SP7..SP13) ---
    PassiveMidDipPeakEq,             // EQ11: Passive 3-Band Midrange Dip & Peak
    PassiveHighLowBracketEq,         // EQ12: Passive 12dB/Oct High/Low Bracket
    Olympic69InductorEq,             // EQ13: Tapped-Inductor Console EQ
    Detroit7BandToroidalEq,          // EQ14: 7-Band Detroit Toroidal Graphic
    LondonARangeInductorEq,          // EQ15: 4-Band Multi-Tap Inductor EQ
    MasteringBaxandallEq,            // EQ16: Mastering Reciprocal Shelf EQ
    ParallelStateVariableEq,         // EQ17: Parallel State-Variable Mastering EQ
    GermanBroadcastSilkyEq,          // EQ18: German Broadcast Class-A Silky EQ
    SteppedMasteringTransferEq,      // EQ19: 51-Step Mastering Transfer EQ
    EllipticalMidSideEq,             // EQ20: Elliptical Mid/Side Mastering EQ
    DualInductorPCurveEq,            // EQ21: Dual-Inductor Resonant P-Curve Shelf
    Iso226DynamicLoudnessEq,         // EQ22: ISO-226 Dynamic Loudness Contour EQ
    BridgedTwinTNotchEq,             // EQ23: Bridged Twin-T Passive Notch Array
    FixedResonant12BandFilter,       // SP7:  12-Band Fixed Resonant Filterbank
    SpectralBrickwallBinEq,          // SP8:  FFT Per-Bin Spectral Brickwall
    PhaseCompensatedBassMono,        // SP9:  Phase-Compensated Bass Mono-Maker
    DynamicSpectralDeHarsher,        // SP10: Dynamic Spectral De-Harsher
    RealTimeSpectralDeReverb,        // SP11: Real-Time Spectral De-Reverberator
    SinusoidalTonalNoiseSeparator,   // SP12: Sinusoidal Tonal vs. Noise Separator
    SubSampleCrossCorrPhaseAligner,  // SP13: Sub-Sample Cross-Correlation Phase Aligner

    // --- Tab 4: COMP / CLIP additions (CP16..CP29, CL4..CL7) ---
    TransconductanceRoomSlammer,     // CP16: Transconductance Room Slammer
    BroadcastFieldEffectCrusher,     // CP17: Broadcast PA Field-Effect Crusher
    RmsVcaPeakStopLimiter,           // CP18: RMS VCA + Diode PeakStop Limiter
    DiscreteMasteringMsComp,         // CP19: Discrete Class-A Mastering M/S Comp
    BritishPhotoOpticalComp,         // CP20: British Dark-Room Photo-Optical
    DualSlopeBusVcaComp,             // CP21: Dual-Slope Crush Stereo Bus VCA
    SolidStateT4BLeveler,            // CP22: Solid-State T4B Optical Leveler
    ParametricDynamicEq4Band,        // CP23: 4-Band Parametric Dynamic EQ
    PhaseSafeDrumBleedGate,          // CP24: Phase-Safe Drum Bleed Suppressor
    ZeroDistortionVocalRider,        // CP25: Zero-Distortion Vocal Gain Rider
    Broadcast5BandFmLimiter,         // CP26: 5-Band Broadcast FM Limiter Chain
    SuperTube6H30PVariMu,            // CP27: 6H30P High-Current Super-Tube Vari-Mu
    LinearPhaseUpwardLifter,         // CP28: Linear-Phase Upward Micro-Lifter
    CrestFactorDensityController,    // CP29: Crest-Factor Dynamic Density Controller
    MasteringAdcOverdrive,           // CL4:  Mastering A/D Converter Overdrive
    SplitBandMasteringClipper,       // CL5:  2-Band Split Sub/Transient Clipper
    HilbertWaveformSymmetrizer,      // CL6:  Hilbert-Transform Waveform Symmetrizer
    Adaa2ndOrderClipper,             // CL7:  2nd-Order ADAA Anti-Aliased Clipper

    // --- Tab 5: TIME / VERB additions (DL11..DL17, RV10..RV18) ---
    GoldenRatioDualRackDelay,        // DL11: Dual-Line Golden-Ratio Rack Delay
    Modulated16BitRackDelay,         // DL12: 1983 16-Bit Modulated Rack Delay
    ReverseEnvelopeEcho,             // DL13: Tempo-Synced Reverse Envelope Echo
    CoiledTubingAcousticDelay,       // DL14: Mechanical Coiled-Tubing Acoustic Delay
    SubMsHaasPhaseAligner,           // DL15: Sub-Millisecond Haas & Phase Aligner
    ElectrostaticOilCanDelay,        // DL16: Electrostatic Flywheel Oil-Can Delay
    DispersivePrismDelay,            // DL17: Dispersive All-Pass Acoustic Prism Delay
    Vintage4LeverDigitalVerb,        // RV10: 1976 4-Lever First Digital Reverb
    AcousticRoomComputer1982,        // RV11: 1982 3D Acoustic Room Computer
    HighDensityDigitalPlate,         // RV12: 1980s High-Density Digital Snare Plate
    SympatheticSoundboardVerb,       // RV13: Sympathetic Chordal Soundboard
    SubOctaveAbyssalVerb,            // RV14: Sub-Octave (-12st) Dark Abyssal Tank
    SplitDecayConcertHall,           // RV15: 1979 Split-Decay Concert Hall Lattice
    Coaxial6SpringStudioVerb,        // RV16: Coaxial 6-Spring Balanced Studio Tank
    ExponentialReverseVerb,          // RV17: Exponential Reverse-Envelope Room Verb
    InfiniteSpectralFreezeVerb,      // RV18: Infinite Spectral Freeze & Drone Pad

    // --- Tab 6: SPLIT / MOD additions (SP_F..SP_J, MD8..MD17, CV5..CV9) ---
    SubAnchorCrossoverRouter,        // SP_F: 2-Band Sub-Anchor Crossover Router
    DualMonoLrSplitMatrix,           // SP_G: Independent Left / Right Dual-Mono Split
    DynamicDuckingFxContainer,       // SP_H: Dynamic Ducking FX Loop Container
    HarmonicTonalNoiseSplitter,      // SP_I: Harmonic Tonal / Unvoiced Noise Splitter
    AzimuthCenterWideSplitter,       // SP_J: Center-Channel vs. Wide-Panned Azimuth Splitter
    DualRotorCabinetHornDrum,        // MD8:  Dual-Rotor Wooden Cabinet Horn & Drum
    BalancedRackAnalogChorus,        // MD9:  1979 Balanced Studio Rack Analog Chorus
    RackThroughZeroFlanger,          // MD10: 1975 Studio Rack Through-Zero Flanger
    Jfet8StageRackPhaser,            // MD11: 1971 8-Stage JFET Studio Rack Phaser
    StudioRmsEnvelopeFilter,         // MD12: Studio RMS Dynamic Envelope Filter
    Analog16BandVocoderMatrix,       // MD13: 16-Band Analog Studio Vocoder Matrix
    Binaural3DHrtfPanner,            // MD14: Binaural 3D HRTF & Micro-ITD Panner
    VacuumTubeTapeAdt1966,           // MD15: 1966 Varispeed Vacuum-Tube Tape ADT
    GermaniumDiodeRingMod,           // MD16: Balanced Germanium Diode Ring Modulator
    PhysicalDopplerAnimator,         // MD17: Physical Doppler & Air-Absorption Animator
    TempoSynced16StepGateCv,         // CV5:  16-Step Tempo-Synced Rhythmic Gate/Mod
    TransientAdEnvelopeCv,           // CV6:  Transient-Triggered AD Percussive Envelope
    SampleAndHoldSlewCv,             // CV7:  Clocked Sample-and-Hold Slew Randomizer
    PitchToCvFollower,               // CV8:  Autocorrelation Pitch-to-CV Follower
    PsychoacousticCentroidCv         // CV9:  Psychoacoustic Loudness / Crest / Centroid CV
};

// =========================================================================
// Generative UI & Parameter Grammar:
//   1. Universal 4-Tier Parameter Grammar
//   2. Primary Archetype Matrix (A..I)
//   3. Trait-Tag Modifier Decorators (TAG_TUBE, TAG_TRANSFORMER, etc.)
// =========================================================================
enum class ParameterControlTier : uint8_t {
    Tier1_InputConditioning    = 1, // Entry/Exit: Input Drive, Pad/Impedance, Pre-Delay, Low/High Scope, SC HPF
    Tier2_CorePhysicsEngine    = 2, // The Heart: Threshold/Ratio/Attack/Release, Gain/Freq/Q, Decay/Size, Recover/Tame
    Tier3_CircuitVoiceModifier = 3, // The Soul: Bias/Asymmetry, Tube/Transformer Mode, SC Tilt, Damping/Spin + Trait Tags
    Tier4_OutputBlendTelemetry = 4  // The Polish: Output Trim, Link IO, Dry/Wet Mix, Mandatory Live Telemetry Probe
};

enum class PrimaryModuleArchetype : uint8_t {
    A_PreampsBuffersTransformers    = 0,
    B_CompressorsLimitersGainCells  = 1,
    C_MultiBandParametricEqs        = 2,
    D_PassiveSpecialtyToneEqs       = 3,
    E_SpectralSmartProcessors       = 4,
    F_TransientShapersAndGates      = 5,
    G_SaturatorsWaveShapersClippers = 6,
    H_DelaysTapeBbdPitchShifters    = 7,
    I_ReverbsAndPhysicalSpaces      = 8
};

enum ComponentTraitTag : uint32_t {
    TAG_NONE                   = 0u,
    TAG_TUBE                   = 1u << 0, // Injects Bias/Plate Starve (-100..+100%) & Sag/Warmth (0..100%)
    TAG_TRANSFORMER            = 1u << 1, // Injects Core Material (Ni/Fe/St) & LF Hysteresis (0..100%)
    TAG_INDUCTOR               = 1u << 2, // Injects Coil Saturation (0..100%) & Stepped Frequency Detents
    TAG_TAPE                   = 1u << 3, // Injects Tape Speed (7.5/15/30 IPS), Head Bump (40-180 Hz) & Wow/Flutter
    TAG_BBD_OR_VINTAGE_DIGITAL = 1u << 4, // Injects Compander Crush (0..100%), Clock/Bitrate & Anti-Alias LPF
    TAG_SIDECHAIN_CAPABLE      = 1u << 5  // Injects SC Filter HPF (20-500 Hz), SC Tilt & Ext SC
};

enum class SchematicElectronicSymbol : uint8_t {
    Transistor            = 0,
    DualTriodeValve       = 1,
    TransformerCoils      = 2,
    InductorCapacitorTank = 3,
    DiodeBridgeVca        = 4,
    TapeDelayHead         = 5,
    CrossoverMatrix       = 6
};

enum SmartPlumbingFlags : uint32_t {
    PLUMB_NONE                      = 0u,
    PLUMB_DC_GUARD_5HZ              = 1u << 0, // [DC-GUARD 5Hz]: Asymmetrical 5 Hz DC blocker
    PLUMB_OVERSAMPLE_ADAA           = 1u << 1, // [OVERSAMPLE 4x/8x ADAA]: Local Anti-Aliasing Island
    PLUMB_PASSIVE_MAKEUP            = 1u << 2, // [PASSIVE MAKEUP +14dB..+18dB]: Insertion Loss Restoration
    PLUMB_DRIFT_AND_THERMAL         = 1u << 3, // [DRIFT 1.5%-2.5% + THERMAL -95dB]: Analog Tolerance & Bias Floor
    PLUMB_PHASE_LATENCY_SYNC        = 1u << 4, // [PHASE / LATENCY SYNC]: Parallel / Multi-Band Branch Delay Comp
    PLUMB_SAFETY_SUBSONIC_NAN_CLAMP = 1u << 5  // [SAFETY SUB-SONIC & NAN CLAMP]: 15 Hz guard + +18 dBFS loop clamp
};

enum class HardwareControlKind : uint8_t {
    ContinuousKnob      = 0, // Smooth potentiometer (e.g., 1073 Output Trim, LA-2A Gain/Peak Reduction)
    SteppedRotarySwitch = 1, // Detented rotary switch (e.g., 24-step Gain, Passive Freq Selector, Time Constant)
    ConcentricDualKnob  = 2, // Dual-ring concentric switch/pot (e.g., Proportional-Q Gain/Freq)
    VerticalFader       = 3, // Linear graphic/console fader (e.g., 10-Band Graphic, 7-Band Toroidal, Density Inflator)
    LeverSlider         = 4, // Vertical lever (e.g., 4-Lever Digital Reverb Controls)
    ToggleSwitch2Way    = 5, // 2-position bat switch (e.g., Pad -20dB, Phase 180, Comp/Limit, Mic/Line, Lo-Z/Hi-Z)
    ToggleSwitch3Way    = 6, // 3-position bat switch (e.g., UA 610 Shelf Freq, RE-201 Input Level, BA-6A Recovery)
    PushButtonLatching  = 7, // Illuminated latching button (e.g., Curve Mode, Harmonic Voicing, Filter Engage)
    PushButtonBank      = 8, // Multi-button radio/slam bank (e.g., 1176 4/8/12/20/ALL, Dimension D Modes 1-4)
    IndicatorMeter      = 9  // VU / Gain Reduction meter or Peak LED indicator
};

struct ComponentParamSpec {
    std::string name;
    std::string unit;
    float minVal{0.0f};
    float maxVal{100.0f};
    float defaultVal{0.0f};
    bool isLogarithmic{false};
    ParameterControlTier tier{ParameterControlTier::Tier2_CorePhysicsEngine};
    HardwareControlKind controlKind{HardwareControlKind::ContinuousKnob};
    std::vector<std::string> stepLabels{};

    // Physical value of each detent (same length as stepLabels, in `unit`).
    // Convention: stepped controls are stored and delivered to the DSP as a position
    // index 0..N-1; the stage class maps the index to its physical value. stepValues
    // documents that mapping for the UI, the C++ exporter and the control-spec audit.
    std::vector<float> stepValues{};

    // true  = control does not exist on the real hardware/software being modelled
    //         (Avra addition such as Input/Output trim, Mix, Link IO, extra HPF).
    //         Rendered in the separate "AVRA EXTRAS" faceplate tier.
    // false = authentic front-panel (or documented internal trim) control.
    bool isAvraExtra{false};

    [[nodiscard]] bool isValid() const noexcept {
        return !name.empty();
    }
};

struct SchematicComponentMeta {
    SchematicComponentType type;
    SchematicCategory category;
    std::string shortName;
    std::string fullName;
    std::string designator;       // e.g. "Q1", "V1", "T1", "EQ1", "CP1"
    std::string cppClassName;     // e.g. "dsp::circuits::DualTriodeTubeStage"
    std::string schematicNotes;   // Under-the-hood hardware component specs (bias, valves, alloys)
    gui::Color accentColor;
    ComponentParamSpec param1{};
    ComponentParamSpec param2{};
    std::vector<ComponentParamSpec> extraParams{};
    std::vector<ComponentParamSpec> params{};
    std::string rawDesignator{};

    [[nodiscard]] size_t getParamCount() const noexcept {
        if (!params.empty()) return params.size();
        size_t count = 0;
        if (param1.isValid()) ++count;
        if (param2.isValid()) ++count;
        return count + extraParams.size();
    }

    [[nodiscard]] const ComponentParamSpec& getParamSpec(int slot) const noexcept {
        if (!params.empty()) {
            if (slot <= 0) return params.front();
            if (static_cast<size_t>(slot) < params.size()) return params[static_cast<size_t>(slot)];
            return params.back();
        }
        if (slot <= 0) return param1;
        if (slot == 1) return param2;
        const size_t extIdx = static_cast<size_t>(slot - 2);
        if (extIdx < extraParams.size()) return extraParams[extIdx];
        return param2.isValid() ? param2 : param1;
    }
};

using ComponentMeta = SchematicComponentMeta;

inline const SchematicComponentMeta& getComponentMeta(SchematicComponentType type);

struct SchematicNode {
    SchematicComponentType type{SchematicComponentType::ClassAPreamp};
    bool isParallel{false};           // True = runs in parallel wet/dry path around this node (or inside ParallelSplitBus)
    bool isBypassed{false};           // True = stage is switched OUT (hard-bypassed in signal chain)
    float parallelMixPercent{100.0f}; // Parallel wet blend (0..100%), defaults to 100% Series Wet
    float param1Val{0.0f};
    float param2Val{0.0f};
    float param3Val{0.0f};
    float param4Val{0.0f};
    float param5Val{0.0f};
    std::vector<float> paramValues{}; // Dynamic hardware control values (indices 0..N-1)
    // Ecosystem Orchestration Engine extensions:
    bool exposeInternalTrims{false}; // Pillar 5.1: Promotes hidden calibration trims (Slots 5 & 6) onto the Faceplate
    bool autoGainLinked{false};      // RULE_06: Inverse Drive/Output coupling (crank Drive without blowing up monitors)
    float trim1Val{0.0f};            // Internal circuit bias / Q-Bias / Cathode Sag trim (-50..+50%)
    float trim2Val{1.5f};            // Analog Component Tolerance Drift % (0.0..5.0% L/R variance)

    bool parallelPhaseInvert{false}; // True = 180-degree polarity invert (Ø) on parallel branch summing junction (Σ)
    bool inMacroGroup{false};        // True = stage is part of a collapsible Macro Chassis Container group

    void syncLegacyToVector(size_t minCount = 5) {
        if (paramValues.size() < minCount) paramValues.resize(minCount, 0.0f);
        if (paramValues.size() >= 1) paramValues[0] = param1Val;
        if (paramValues.size() >= 2) paramValues[1] = param2Val;
        if (paramValues.size() >= 3) paramValues[2] = param3Val;
        if (paramValues.size() >= 4) paramValues[3] = param4Val;
        if (paramValues.size() >= 5) paramValues[4] = param5Val;
    }

    [[nodiscard]] float getParamVal(int slot) const noexcept {
        if (slot == 105) return trim1Val;
        if (slot == 106) return trim2Val;
        if (slot >= 0 && static_cast<size_t>(slot) < paramValues.size()) {
            if (slot == 0) return param1Val;
            if (slot == 1) return param2Val;
            if (slot == 2) return param3Val;
            if (slot == 3) return param4Val;
            if (slot == 4) return param5Val;
            return paramValues[static_cast<size_t>(slot)];
        }
        switch (slot) {
            case 0:  return param1Val;
            case 1:  return param2Val;
            case 2:  return param3Val;
            case 3:  return param4Val;
            case 4:  return param5Val;
            default: {
                const auto& meta = getComponentMeta(type);
                if (slot >= 0 && static_cast<size_t>(slot) < meta.getParamCount()) {
                    return meta.getParamSpec(slot).defaultVal;
                }
                if (slot == 5) return trim1Val;
                if (slot == 6) return trim2Val;
                return 0.0f;
            }
        }
    }

    void setParamVal(int slot, float val) {
        if (slot == 105) { trim1Val = val; return; }
        if (slot == 106) { trim2Val = val; return; }
        if (slot >= 0 && slot < 32) {
            if (paramValues.size() <= static_cast<size_t>(slot)) {
                paramValues.resize(static_cast<size_t>(slot) + 1, 0.0f);
            }
            paramValues[static_cast<size_t>(slot)] = val;
        }
        switch (slot) {
            case 0: param1Val = val; break;
            case 1: param2Val = val; break;
            case 2: param3Val = val; break;
            case 3: param4Val = val; break;
            case 4: param5Val = val; break;
            case 5: if (paramValues.size() <= 5) trim1Val = val; break;
            case 6: if (paramValues.size() <= 6) trim2Val = val; break;
            default: break;
        }
    }

    [[nodiscard]] float getParamValue(int slot) const noexcept { return getParamVal(slot); }
    void setParamValue(int slot, float val) { setParamVal(slot, val); }

    [[nodiscard]] uint32_t getSmartPlumbingFlags() const noexcept;
};

inline const std::vector<SchematicComponentMeta>& getRawSchematicCatalog() {
    static const std::vector<SchematicComponentMeta> s_rawCatalog = {
        // =====================================================================
        // TAB 1: PRE / TUBE (30 Studio Console, Broadcast & Mastering Line Stages: PR1..PR30)
        // =====================================================================
        {
            SchematicComponentType::JfetBuffer,
            SchematicCategory::PreampsAndTubes,
            "Pure JFET Line Buffer",
            "High-Impedance Class-A JFET Studio Line Buffer with 2nd-Order Harmonic Chime",
            "PR1",
            "dsp::circuits::JfetBufferStage",
            "TIS58 / 2N5457 Class-A JFET studio line buffer topology with square-law 2nd-order harmonic enhancement.",
            gui::Color::fromHex(0xFFD166),
            { "Input Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "JFET Drain Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "0 dB", "-20 dB" }, { 0.0f, -20.0f } },
                { "Input Load", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "10 kOhm Bridging", "600 Ohm Loaded" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" } },
                { "Low-Cut Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "45 Hz Sub-Cut" }, {}, true }
            }
        },
        {
            SchematicComponentType::ClassAPreamp,
            SchematicCategory::PreampsAndTubes,
            "Class-A Transistor Preamp",
            "Single-Ended 2N3055 Power Transistor @ 75mA DC Bias with Dual-Stage Cascade",
            "PR2",
            "dsp::circuits::DiscreteClassAPreamp",
            "Discrete Class-A dual-stage gain law + gapped steel output loading with asymmetric 2nd-order collector saturation.",
            gui::Color::fromHex(0xFF4D6D),
            { "Gain", "dB", 0.0f, 19.0f, 13.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "LINE +10", "LINE +5", "LINE 0", "LINE -5", "LINE -10", "LINE -15", "LINE -20",
                "MIC -20", "MIC -25", "MIC -30", "MIC -35", "MIC -40", "MIC -45", "MIC -50", "MIC -55", "MIC -60",
                "MIC -65", "MIC -70", "MIC -75", "MIC -80" },
              { -10.0f, -5.0f, 0.0f, 5.0f, 10.0f, 15.0f, 20.0f, 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f, 65.0f, 70.0f, 75.0f, 80.0f } },
            { "High-Pass Filter", "Hz", 0.0f, 4.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
              { "OFF", "50", "80", "160", "300" }, { 0.0f, 50.0f, 80.0f, 160.0f, 300.0f } },
            {
                // Not on the original module (level was set by the console fader): Avra Extra.
                { "Output Trim", "dB", -16.0f, 16.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                // Internal/rear strap on the original module, front-panel switch on later reissues.
                { "Mic Impedance", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "300 LO-Z", "1200 HI-Z" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "180 INV" } }
            }
        },
        {
            SchematicComponentType::ClassABOpAmp2520,
            SchematicCategory::PreampsAndTubes,
            "Discrete Bipolar Preamp",
            "Fast-Slew Encapsulated Bipolar Op-Amp with Hard +/-16V Rail Punch",
            "PR3",
            "dsp::circuits::DiscreteClassABOpAmp",
            "Class-AB discrete op-amp with explicit slew-rate limiting (|dV/dt|<=Slew_max) for forward midrange punch.",
            gui::Color::fromHex(0xFF758F),
            { "Mic/Line Gain", "dB", 10.0f, 65.0f, 34.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
            {
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-20 dB" }, { 0.0f, -20.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" } },
                { "Input Select", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "MIC", "LINE" } },
                { "Quadfilar Output Tap", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "+18 dBu (1:1)", "+28 dBu (1:3)" }, { 18.0f, 28.0f } }
            }
        },
        {
            SchematicComponentType::Tube12AX7,
            SchematicCategory::PreampsAndTubes,
            "Dual-Triode Tube Pre",
            "12AX7 High-Mu Voltage Amp into 12BH7 Cathode Follower with Miller HF Smoothing",
            "PR4",
            "dsp::circuits::DualTriodeTubeStage",
            "Classic optical/passive-EQ tube makeup topology with dynamic Miller HF smoothing and RC B+ rail sag.",
            gui::Color::fromHex(0xFF9E00),
            { "Input Drive / Grid Gain", "dB", -12.0f, 36.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Attenuator", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Cathode Bias / B+ Sag", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Tube Voicing", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "ECC83 High-Gain", "12AY7 Low-Gain" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Negative Feedback", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "OPEN LOOP", "6 dB NFB" }, {}, true }
            }
        },
        {
            SchematicComponentType::PentodeEF86,
            SchematicCategory::PreampsAndTubes,
            "Pentode Line Preamp",
            "Sharp-Cutoff Pentode Line Amp with Odd-Order Grit & Screen-Grid Sag",
            "PR5",
            "dsp::circuits::PentodeTubeStage",
            "EF86/E88CC broadcast valve amplifier with switched negative-feedback gain and odd-order harmonic density.",
            gui::Color::fromHex(0xFB8500),
            { "Coarse Voltage Gain", "dB", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "16 dB", "22 dB", "28 dB", "34 dB", "40 dB", "46 dB" },
              { 16.0f, 22.0f, 28.0f, 34.0f, 40.0f, 46.0f } },
            { "Fine Gain Trim", "dB", -5.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Drive / Pad", "dB", -20.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Bass Roll-Off", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "-3 dB @ 60 Hz Pop Filter" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OUT", "-20 dB IN" }, { 0.0f, -20.0f } },
                { "Phase Reverse", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } }
            }
        },
        {
            SchematicComponentType::DirectHeatedTriode,
            SchematicCategory::PreampsAndTubes,
            "Vari-Mu Twin-Triode Pre",
            "Balanced Parallel Twin-Triode Amplifier with Even-Harmonic Cancellation & B+ Sag",
            "PR6",
            "dsp::circuits::VariMuLineAmpStage",
            "Push-pull vari-mu remote-cutoff twin-triode amplifier topology with even-harmonic cancellation and B+ sag.",
            gui::Color::fromHex(0xFFB703),
            { "Input Attenuator", "dB", 0.0f, 9.0f, 9.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
              { "-18 dB", "-16 dB", "-14 dB", "-12 dB", "-10 dB", "-8 dB", "-6 dB", "-4 dB", "-2 dB", "0 dB" },
              { -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f } },
            { "Push-Pull Tube Drive", "dB", 0.0f, 30.0f, 12.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "DC Balance", "%", -100.0f, 100.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Center-Tap Iron Loading", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "600 Ohm", "Bridging" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" }, {}, true }
            }
        },
        {
            SchematicComponentType::GermaniumBroadcastPre,
            SchematicCategory::PreampsAndTubes,
            "PNP Germanium Line Pre",
            "Leaky-Bias PNP Germanium Transistor Stage with Woolly Low-Mids & Soft Clipping",
            "PR7",
            "dsp::circuits::GermaniumBroadcastPreStage",
            "OC71/AC128 vintage broadcast germanium transistor topology with leaky collector bias and soft clipping.",
            gui::Color::fromHex(0xFF595E),
            { "Coarse Gain", "dB", 0.0f, 9.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "20 dB", "25 dB", "30 dB", "35 dB", "40 dB", "45 dB", "50 dB", "55 dB", "60 dB", "65 dB" },
              { 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f, 65.0f } },
            { "Feedback Control", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Pad", "dB", -20.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Thick", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "OFF", "THICK" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-15 dB" }, { 0.0f, -15.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } }
            }
        },
        {
            SchematicComponentType::BlueStripeSiliconPre,
            SchematicCategory::PreampsAndTubes,
            "High-Gain Silicon BJT Pre",
            "High-Gain 2N3707 Silicon Bipolar Stage with Aggressive Upper-Mid Bite",
            "PR8",
            "dsp::circuits::BlueStripeSiliconPreStage",
            "Early FET-limiter discrete 2N3707 silicon transistor output stage with aggressive upper-midrange bite.",
            gui::Color::fromHex(0x4EA8DE),
            { "Input Drive", "dB", -24.0f, 36.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Level", "dB", -24.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "FET / BJT Bias Point", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Preamp Clip Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Soft Saturation", "Hard Rail Clip" } },
                { "Low-Cut Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "80 Hz Low-Cut" }, {}, true },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" }, {}, true }
            }
        },
        {
            SchematicComponentType::Console4000IcStage,
            SchematicCategory::PreampsAndTubes,
            "Balanced IC Console Strip",
            "Electronically-Balanced NE5534 Operational Amplifier with Tight Transient Clip",
            "PR9",
            "dsp::circuits::ConsoleIcPreStage",
            "1980s large-format console line input using NE5534 operational amplifier with tight transient clip.",
            gui::Color::fromHex(0x90E0EF),
            { "Line Trim", "dB", -20.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Mic Gain", "dB", 10.0f, 70.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High-Pass Filter", "Hz", 16.0f, 350.0f, 20.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Low-Pass Filter", "Hz", 3000.0f, 22000.0f, 20000.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Mic / Line Flip", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonLatching, { "LINE", "MIC" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonLatching, { "OFF", "-20 dB" }, { 0.0f, -20.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "0 deg NORM", "180 deg INV" } }
            }
        },
        {
            SchematicComponentType::JapaneseTapeDeckPre,
            SchematicCategory::PreampsAndTubes,
            "Tape-Deck Silicon Preamp",
            "Mid-Forward 2SC1000 Silicon Preamp with 40Hz Sub-Cut & Soft Diode Clipping",
            "PR10",
            "dsp::circuits::JapaneseTapeDeckPreStage",
            "1970s Japanese tape echo analog preamp path with 2SC1000 silicon transistor and 40Hz sub-cut.",
            gui::Color::fromHex(0x52B788),
            { "Input Volume", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Level Trim", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
            {
                { "Input Level Sensitivity", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch3Way,
                  { "LOW -20 dBm", "MID -10 dBm", "HIGH +4 dBm" }, { 20.0f, 10.0f, 0.0f } },
                { "Feedback Diode Clip", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Sub-Cut Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "48 Hz Coupled", "100 Hz Speech" }, {}, true },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" }, {}, true }
            }
        },
        {
            SchematicComponentType::HighVoltageCascodeTube,
            SchematicCategory::PreampsAndTubes,
            "HV Mastering Pre",
            "Discrete FET/Bipolar High-Rail Mastering Stage for Clinical Slew & Headroom",
            "PR11",
            "dsp::circuits::HighVoltageCascodeTube",
            "High-voltage transformerless mastering line amp eliminating Miller HF loss for clinical slew and headroom.",
            gui::Color::fromHex(0xFCA311),
            { "Gain", "dB", 0.0f, 11.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "6 dB", "10 dB", "15 dB", "20 dB", "25 dB", "30 dB", "35 dB", "40 dB", "45 dB", "50 dB", "55 dB", "60 dB" },
              { 6.0f, 10.0f, 15.0f, 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f } },
            { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
            {
                { "Ribbon / High-Gain Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "+10 dB DC-Coupled" } },
                { "High-Pass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "30 Hz 12 dB/oct" } },
                { "Phase Invert", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } },
                { "Output Stage Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::SubMiniatureTubeMicPre,
            SchematicCategory::PreampsAndTubes,
            "6AU6 Pentode Vocal Pre",
            "Sub-Miniature 6AU6 Pentode Vocal Preamp for Ultra-Silky High-Frequency Air",
            "PR12",
            "dsp::circuits::SubMiniatureTubeMicStage",
            "Cooled modern vocal tube circuit topology delivering silky 2nd-harmonic vocal air and zero harshness.",
            gui::Color::fromHex(0xFFD166),
            { "Tube Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Peltier Thermal Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "High-Pass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonLatching, { "FLAT", "80 Hz Low-Cut" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonLatching, { "OFF", "-10 dB" }, { 0.0f, -10.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" }, {}, true }
            }
        },
        {
            SchematicComponentType::AmericanTubeConsolePre,
            SchematicCategory::PreampsAndTubes,
            "Dual-Triode Console Pre",
            "12AX7 + 6072A Variable-Negative-Feedback Tube Amp with Thick Low-Mid Bloom",
            "PR13",
            "dsp::circuits::TubeConsolePreStage",
            "Variable feedback loop gain-staging topology with 12AX7 + 6072A tubes and thick low-mid bloom.",
            gui::Color::fromHex(0xF77F00),
            { "Gain", "dB", 0.0f, 4.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "-10 dB", "-5 dB", "0 dB", "+5 dB", "+10 dB" },
              { -10.0f, -5.0f, 0.0f, 5.0f, 10.0f } },
            { "Low Shelf Boost/Cut", "dB", 0.0f, 8.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "-9 dB", "-6 dB", "-3 dB", "-1.5 dB", "0 dB", "+1.5 dB", "+3 dB", "+6 dB", "+9 dB" },
              { -9.0f, -6.0f, -3.0f, -1.5f, 0.0f, 1.5f, 3.0f, 6.0f, 9.0f } },
            {
                { "High Shelf Boost/Cut", "dB", 0.0f, 8.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "-9 dB", "-6 dB", "-3 dB", "-1.5 dB", "0 dB", "+1.5 dB", "+3 dB", "+6 dB", "+9 dB" },
                  { -9.0f, -6.0f, -3.0f, -1.5f, 0.0f, 1.5f, 3.0f, 6.0f, 9.0f } },
                { "Level", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Low Shelf Corner", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way,
                  { "70 Hz", "100 Hz", "200 Hz" }, { 70.0f, 100.0f, 200.0f } },
                { "High Shelf Corner", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way,
                  { "4.5 kHz", "7 kHz", "10 kHz" }, { 4500.0f, 7000.0f, 10000.0f } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-15 dB" }, { 0.0f, -15.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" } }
            }
        },
        {
            SchematicComponentType::GermanBroadcastValvePre,
            SchematicCategory::PreampsAndTubes,
            "Quad-Pentode Broadcast Pre",
            "4-Stage EF804S Pentode/Triode Broadcast Amplifier with Switchable Choke Filter",
            "PR14",
            "dsp::circuits::BroadcastValvePreStage",
            "300Hz-15kHz choke/feedback 4-stage EF804S broadcast valve amplifier topology with deep low-end authority.",
            gui::Color::fromHex(0x06D6A0),
            { "Gain", "dB", 0.0f, 14.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "0 dB", "+3 dB", "+6 dB", "+12 dB", "+18 dB", "+24 dB", "+30 dB", "+36 dB", "+42 dB", "+48 dB", "+54 dB", "+60 dB", "+66 dB", "+72 dB", "+76 dB" },
              { 0.0f, 3.0f, 6.0f, 12.0f, 18.0f, 24.0f, 30.0f, 36.0f, 42.0f, 48.0f, 54.0f, 60.0f, 66.0f, 72.0f, 76.0f } },
            { "Output Attenuator", "dB", -30.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
            {
                { "Filter Switch", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch3Way, { "FLAT", "80 Hz HPF", "300 Hz + 10 kHz Speech" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } },
                { "19.5 kHz Broadcast LPF", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "OFF", "19.5 kHz IN" }, {}, true },
                { "Anode Screen Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ConsoleSummingBusAmp,
            SchematicCategory::PreampsAndTubes,
            "Active Virtual-Earth Bus",
            "Shunt-Feedback Active Bus Receiver with Inter-Channel Crosstalk & Bus Compression",
            "PR15",
            "dsp::circuits::ConsoleSummingBusStage",
            "Active current-summing console master bus with inter-channel crosstalk and bus transformer glue.",
            gui::Color::fromHex(0xFF4D6D),
            { "Bus Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Crosstalk & Bus Bleed", "dB", -90.0f, -40.0f, -65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Master Fader Trim", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} },
                { "Bus Topology", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Class-A British", "Discrete Console" } },
                { "-6 dB Bus Shunt Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "NORM", "-6 dB Pad" }, { 0.0f, -6.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" }, {}, true }
            }
        },
        {
            SchematicComponentType::HeadroomCalibratorPre,
            SchematicCategory::PreampsAndTubes,
            "VU Headroom Calibrator",
            "Reference Operating Level Calibrator (-18 dBFS = 0 VU) with Inverse Link-IO Trim",
            "PR16",
            "dsp::circuits::HeadroomCalibratorStage",
            "Precision studio gain-staging calibration utility mapping -18 dBFS to 0 VU with inverse Link-IO trim.",
            gui::Color::fromHex(0x4CC9F0),
            { "Reference Calibration", "dB", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "-24 dBFS", "-20 dBFS", "-18 dBFS", "-16 dBFS", "-14 dBFS", "-12 dBFS" },
              { -24.0f, -20.0f, -18.0f, -16.0f, -14.0f, -12.0f } },
            { "Input Trim", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Gain Compensation Link", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ToggleSwitch2Way, { "FREE", "LINK IO" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "VU Meter Bridge Load", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Olympic69LustraphonePre,
            SchematicCategory::PreampsAndTubes,
            "Permalloy Class-A Strip",
            "Input Transformer Coupled into Single-Ended Silicon Class-A Transistor Stage",
            "PR17",
            "dsp::circuits::PermalloyClassAStripStage",
            "Late-60s British rock console channel strip with permalloy input iron and BC109C Class-A stage.",
            gui::Color::fromHex(0xFF6B6B),
            { "Mic Gain", "dB", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "20 dB", "25 dB", "30 dB", "35 dB", "40 dB", "45 dB", "50 dB", "55 dB", "60 dB", "65 dB", "70 dB" },
              { 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f, 65.0f, 70.0f } },
            { "Output Fader Level", "dB", -24.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} },
            {
                { "Mic / Line Switch", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "MIC", "LINE" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-20 dB" }, { 0.0f, -20.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "REV" } },
                { "Input Iron Flux", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Abbey68TransistorPre,
            SchematicCategory::PreampsAndTubes,
            "Dual-BJT Presence Strip",
            "Dual-Transistor Class-A/AB Console Strip with Forward 1.8kHz Presence Peak",
            "PR18",
            "dsp::circuits::DualBjtPresenceStripStage",
            "1968 solid-state mastering/tracking desk topology with 1.8kHz presence contour and gapped steel iron.",
            gui::Color::fromHex(0xF9844A),
            { "Input Cassette Gain", "dB", 0.0f, 11.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "15 dB", "20 dB", "25 dB", "30 dB", "35 dB", "40 dB", "45 dB", "50 dB", "55 dB", "60 dB", "65 dB", "70 dB" },
              { 15.0f, 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f, 65.0f, 70.0f } },
            { "Output Level Trim", "dB", -20.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Bass Cut", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "-3 dB @ 60 Hz" } },
                { "Phase Invert", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } },
                { "Presence Peak Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Collector Rail Headroom", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::LondonARangeZenerPre,
            SchematicCategory::PreampsAndTubes,
            "Zener-Biased Console Pre",
            "Discrete Bipolar Line Stage with Zener-Biased Asymmetric Soft Overdrive",
            "PR19",
            "dsp::circuits::ZenerBiasedConsoleStage",
            "1970s discrete transistor console saturation with 5.6V Zener-biased asymmetric soft overdrive.",
            gui::Color::fromHex(0xFFD166),
            { "Coarse Input Gain", "dB", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "-15 dB", "-10 dB", "-5 dB", "0 dB", "+5 dB", "+10 dB", "+15 dB" },
              { -15.0f, -10.0f, -5.0f, 0.0f, 5.0f, 10.0f, 15.0f } },
            { "Fine Gain Trim", "dB", -5.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Fader", "dB", -24.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} },
                { "High-Pass Filter", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch3Way,
                  { "25 Hz", "50 Hz", "100 Hz" }, { 25.0f, 50.0f, 100.0f } },
                { "Low-Pass Filter", "", 0.0f, 2.0f, 2.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way,
                  { "9 kHz", "12 kHz", "15 kHz" }, { 9000.0f, 12000.0f, 15000.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "NORM", "INV" } }
            }
        },
        {
            SchematicComponentType::WestCoastAm10Pre,
            SchematicCategory::PreampsAndTubes,
            "High-Current Discrete Pre",
            "Encapsulated Discrete Class-AB Op-Amp with High-Current 80Hz Low-End Authority",
            "PR20",
            "dsp::circuits::WestCoastAm10Stage",
            "1970s American film/music desk topology with discrete Class-AB op-amp and 80Hz low-end authority.",
            gui::Color::fromHex(0x4EA8DE),
            { "Input Drive", "dB", -12.0f, 48.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Level", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Impedance / Gain Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "40 dB Strap", "48 dB Strap" }, { 40.0f, 48.0f } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-20 dB" }, { 0.0f, -20.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Nickel Core Saturation", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::TransformerlessJfetBjtPre,
            SchematicCategory::PreampsAndTubes,
            "Twin-Path Hybrid Pre",
            "Ultra-Fast Hybrid Console Line Input with Phase-Coherent Ultrasonic Response",
            "PR21",
            "dsp::circuits::TransformerlessJfetBjtStage",
            "Late-70s American automated console input with 2N5457 JFET differential pair into bipolar buffer.",
            gui::Color::fromHex(0x00F5D4),
            { "Gain", "dB", 0.0f, 11.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "8.5 dB", "14 dB", "19 dB", "24 dB", "29 dB", "34 dB", "39 dB", "44 dB", "49 dB", "54 dB", "60 dB", "65 dB" },
              { 8.5f, 14.0f, 19.0f, 24.0f, 29.0f, 34.0f, 39.0f, 44.0f, 49.0f, 54.0f, 60.0f, 65.0f } },
            { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "DC-Coupled Ribbon Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "+10 dB DC Path" }, { 0.0f, 10.0f } },
                { "High-Pass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "80 Hz 6 dB/oct" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } },
                { "JFET Pair Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::HighRail80BroadcastPre,
            SchematicCategory::PreampsAndTubes,
            "High-Rail Class-A Pre",
            "High-Supply-Voltage Class-A Transistor Stage with Extended HF Slew Rate",
            "PR22",
            "dsp::circuits::HighRail80BroadcastStage",
            "28V broadcast console line driver with dual-transistor feedback cards and extended HF slew rate.",
            gui::Color::fromHex(0xFF4D6D),
            { "Input Gain", "dB", 0.0f, 10.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "20 dB", "25 dB", "30 dB", "35 dB", "40 dB", "45 dB", "50 dB", "55 dB", "60 dB", "65 dB", "70 dB" },
              { 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f, 65.0f, 70.0f } },
            { "Fine Trim", "dB", -6.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High-Pass Filter", "Hz", 20.0f, 250.0f, 45.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Mic / Line", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "MIC", "LINE" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Criteria2002HybridPre,
            SchematicCategory::PreampsAndTubes,
            "Hybrid IC-Transistor Strip",
            "High-Headroom IC/Transistor Hybrid Stage with Warm Coupling-Cap Sub Roll-Off",
            "PR23",
            "dsp::circuits::HybridConsoleStripStage",
            "1970s American rock tracking console hybrid stage with tantalum coupling-cap warmth.",
            gui::Color::fromHex(0xFFB703),
            { "Gain", "dB", 6.0f, 60.0f, 24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High-Pass Filter", "Hz", 30.0f, 250.0f, 45.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Transformer", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "IRON", "IC DIRECT" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-20 dB" }, { 0.0f, -20.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } }
            }
        },
        {
            SchematicComponentType::AirStudioZobelPre,
            SchematicCategory::PreampsAndTubes,
            "Zobel-Damped Studio Pre",
            "Amorphous Input Iron into NE5534 + Zobel Network Class-A/B Line Amplifier",
            "PR24",
            "dsp::circuits::AirStudioZobelStage",
            "Wideband custom console channel topology with amorphous input iron and RC Zobel damping network.",
            gui::Color::fromHex(0x4CC9F0),
            { "Mic Gain", "dB", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "0 dB", "+6 dB", "+12 dB", "+18 dB", "+24 dB", "+30 dB", "+36 dB", "+42 dB", "+48 dB", "+54 dB", "+60 dB" },
              { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f, 30.0f, 36.0f, 42.0f, 48.0f, 54.0f, 60.0f } },
            { "Line Gain", "dB", 0.0f, 5.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "-20 dB", "-14 dB", "-8 dB", "-2 dB", "+4 dB", "+10 dB" },
              { -20.0f, -14.0f, -8.0f, -2.0f, 4.0f, 10.0f } },
            {
                { "Fine Trim", "dB", 0.0f, 14.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "High-Pass Filter", "Hz", 20.0f, 400.0f, 40.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Input Impedance", "", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::SteppedRotarySwitch,
                  { "Low 600", "Std 1400", "Med 2400", "High 6800" }, { 600.0f, 1400.0f, 2400.0f, 6800.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "NORM", "INV" } }
            }
        },
        {
            SchematicComponentType::Soul60s24VClassAPre,
            SchematicCategory::PreampsAndTubes,
            "24V Dual-Iron Line Pre",
            "24V Silicon Transistor Line Amplifier with Paired Input/Output Steel Iron",
            "PR25",
            "dsp::circuits::Soul60s24VClassAStage",
            "1960s Detroit/Memphis discrete solid-state preamp with paired 600:600 steel transformers.",
            gui::Color::fromHex(0xF77F00),
            { "Input Gain", "dB", -10.0f, 55.0f, 12.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Attenuator", "dB", -24.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "600-Ohm Output Termination", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "600 Ohm LOADED", "UNLOADED" } },
                { "Pad", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "-18 dB" }, { 0.0f, -18.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Steel Core Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::LaStudioQuadTransistorPre,
            SchematicCategory::PreampsAndTubes,
            "Quad-Transistor Line Pre",
            "Early Silicon 4-Transistor Feedback Amplifier with Deep Sub-Bass Weight",
            "PR26",
            "dsp::circuits::LaStudioQuadTransistorStage",
            "Late-60s West-Coast 4-transistor feedback module with deep sub-bass weight and midrange punch.",
            gui::Color::fromHex(0x52B788),
            { "Gain", "dB", 0.0f, 8.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "+20 dB", "+25 dB", "+30 dB", "+35 dB", "+40 dB", "+45 dB", "+50 dB", "+55 dB", "+60 dB" },
              { 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f } },
            { "Output Level", "dB", -20.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pad", "", 0.0f, 2.0f, 2.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch3Way,
                  { "-12 dB", "-24 dB", "OFF" }, { -12.0f, -24.0f, 0.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } },
                { "Feedback Loop Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Sub-Bass Iron Contour", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Octal50sBroadcastTubePre,
            SchematicCategory::PreampsAndTubes,
            "Octal Tube Broadcast Pre",
            "6J7 + 6SN7 Octal-Base Vacuum Tube Line Stage with Lush Even-Order Harmonics",
            "PR27",
            "dsp::circuits::Octal50sBroadcastTubeStage",
            "American 1950s tube broadcast console using 6J7 pentode and 6SN7 dual-triode octal valves.",
            gui::Color::fromHex(0xFF9E00),
            { "Interstage Gain", "dB", 0.0f, 50.0f, 20.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output T-Pad Attenuator", "dB", -24.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Input Strapping", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "150 Ohm", "600 Ohm" }, { 150.0f, 600.0f } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "5Y3 B+ Rail Sag", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Octal Cathode Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::DavenSteppedTubePre,
            SchematicCategory::PreampsAndTubes,
            "Stepped Ladder Tube Pre",
            "5879 Pentode into 12AU7 Cathode Follower with Stepped Attenuator Impedance Law",
            "PR28",
            "dsp::circuits::SteppedLadderTubeStage",
            "Vintage broadcast stepped tube pre with 5879 pentode into 12AU7 cathode follower and stepped T-pad ladder.",
            gui::Color::fromHex(0xFCA311),
            { "Stepped Gain", "dB", 0.0f, 8.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "+10 dB", "+15 dB", "+20 dB", "+25 dB", "+30 dB", "+35 dB", "+40 dB", "+45 dB", "+50 dB" },
              { 10.0f, 15.0f, 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f } },
            { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Feedback Loop Contour", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "BROADCAST PRESENCE" } },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Screen Grid Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "T-Pad Loading", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::SwissBroadcastLinePre,
            SchematicCategory::PreampsAndTubes,
            "Precision Broadcast Line Pre",
            "Transformer-Balanced Precision Solid-State Stage with Ultra-Tight Low End",
            "PR29",
            "dsp::circuits::SwissBroadcastLineStage",
            "European broadcast console line receiver with toroidal permalloy iron and ultra-tight low end.",
            gui::Color::fromHex(0x90E0EF),
            { "Input Sensitivity", "dB", -60.0f, 16.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "High-Pass Filter", "Hz", 40.0f, 320.0f, 55.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Calibrate Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Limiter Guard", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "OFF", "+18 dBu Peak Guard" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "180 deg" } },
                { "Toroidal Q-Factor", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ZeroFeedbackMasteringPre,
            SchematicCategory::PreampsAndTubes,
            "Zero-FB Master Transfer",
            "Discrete JFET/Bipolar Zero-Global-Feedback Driver with Triode/Pentode Emulation",
            "PR30",
            "dsp::circuits::ZeroFeedbackMasteringStage",
            "Modern mastering console transfer stage with zero global negative feedback and triode/pentode voicing.",
            gui::Color::fromHex(0xFFD166),
            { "Input Trim", "dB", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
              { "-5 dB", "-4 dB", "-3 dB", "-2 dB", "-1 dB", "0 dB", "+1 dB", "+2 dB", "+3 dB", "+4 dB", "+5 dB" },
              { -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f } },
            { "Output Trim", "dB", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::SteppedRotarySwitch,
              { "-5 dB", "-4 dB", "-3 dB", "-2 dB", "-1 dB", "0 dB", "+1 dB", "+2 dB", "+3 dB", "+4 dB", "+5 dB" },
              { -5.0f, -4.0f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f } },
            {
                { "Class-A Current Bias", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Standard", "High-Current Class-A" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" } },
                { "Triode / Pentode Morph", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "JFET Drain Symmetry", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::GermanDualPentodeCinemaPre,
            SchematicCategory::PreampsAndTubes,
            "Dual-Pentode Cinema Pre",
            "Cascaded EF12k/EF40 Glass Pentodes with High-Z Input Iron & Screen-Grid Sag",
            "PR31",
            "dsp::circuits::GermanDualPentodeCinemaStage",
            "1950s German cinema console dual-pentode cascade with high-inductance input iron and thick midrange authority.",
            gui::Color::fromHex(0xFCA311),
            { "Input Gain Pad", "dB", -20.0f, 34.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pentode Screen Grid Voltage", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Cinema High-Pass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "60 Hz 12 dB/oct" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Choke Inductance Load", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ZeroFeedbackPureTriodePre,
            SchematicCategory::PreampsAndTubes,
            "Pure 12AX7 Triode",
            "Loop-Free Single-Ended 12AX7 Triode Stage for Unrestrained 2nd/3rd Harmonics",
            "PR32",
            "dsp::circuits::ZeroFeedbackPureTriodeStage",
            "Open-loop single-ended 12AX7 triode amplifier with zero global feedback for liquid 2nd-order vocal bloom.",
            gui::Color::fromHex(0xFF9F1C),
            { "Input Drive", "dB", 0.0f, 40.0f, 16.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Plate Voltage", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Level", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Cathode Capacitor", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "TIGHT", "FULL LOWS" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Grid Conduction Clamp", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::CurrentModeTransimpedancePre,
            SchematicCategory::PreampsAndTubes,
            "Transimpedance Line Pre",
            "Zero-TIM Instrumentation Current-Feedback Topology with Ultra-Fast Transient Slew",
            "PR33",
            "dsp::circuits::CurrentModeTransimpedanceStage",
            "Current-feedback transimpedance instrumentation amplifier with gain-independent bandwidth and zero TIM distortion.",
            gui::Color::fromHex(0x00F5D4),
            { "Transimpedance Gain", "dB", 6.0f, 66.0f, 24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Ultrasonic Bandwidth", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "100 kHz Std", "300 kHz Wide" } },
                { "High-Pass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "75 Hz" } },
                { "Current Mirror Slew", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Polarity", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "0 deg", "180 deg" }, {}, true }
            }
        },
        {
            SchematicComponentType::EasternBroadcastPentodePre,
            SchematicCategory::PreampsAndTubes,
            "Permalloy Pentode Strip",
            "6J32P Pentode into 6N3P Cathode Follower with 80%-Nickel Permalloy Iron",
            "PR34",
            "dsp::circuits::EasternBroadcastPentodeStage",
            "1960s state broadcast console valve strip pairing a 6J32P pentode with a 6N3P cathode follower and permalloy iron.",
            gui::Color::fromHex(0xE76F51),
            { "Input Gain", "dB", 0.0f, 8.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "+20 dB", "+25 dB", "+30 dB", "+35 dB", "+40 dB", "+45 dB", "+50 dB", "+55 dB", "+60 dB" },
              { 20.0f, 25.0f, 30.0f, 35.0f, 40.0f, 45.0f, 50.0f, 55.0f, 60.0f } },
            { "Output Level", "dB", -20.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Rumble Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "80 Hz" } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "INV" } },
                { "Permalloy Core Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Screen Grid Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::JapaneseDiscreteClassAPre,
            SchematicCategory::PreampsAndTubes,
            "Complementary Class-A Pre",
            "Complementary 2SA/2SC Silicon Differential Pair with Gapped Steel Output Iron",
            "PR35",
            "dsp::circuits::JapaneseDiscreteClassAStage",
            "1970s Tokyo discrete Class-A console amplifier delivering forward 2kHz presence punch and tight sub-bass.",
            gui::Color::fromHex(0x4CC9F0),
            { "Input Sensitivity", "dB", 0.0f, 7.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "-60 dB", "-50 dB", "-40 dB", "-30 dB", "-20 dB", "-10 dB", "0 dB", "+4 dB" },
              { -60.0f, -50.0f, -40.0f, -30.0f, -20.0f, -10.0f, 0.0f, 4.0f } },
            { "Continuous Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Fader", "dB", -24.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} },
                { "High-Pass Filter", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch3Way,
                  { "OFF", "40 Hz", "80 Hz" }, { 18.0f, 40.0f, 80.0f } },
                { "Phase", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "REV" } },
                { "Emitter Degeneration", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::VariImpedanceCableLoadPre,
            SchematicCategory::PreampsAndTubes,
            "Vari-Z Cable & Iron Load",
            "Variable 150-Ohm to 10k-Ohm Secondary Termination with Reactive Cable Resonance",
            "PR36",
            "dsp::circuits::VariImpedanceCableLoadStage",
            "Simulates variable 150-Ohm to 10k-Ohm transformer secondary loading, cable capacitance roll-off, and damped resonance.",
            gui::Color::fromHex(0x90BE6D),
            { "Load Impedance", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Cable Capacitance", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Clean JFET Gain", "dB", 12.0f, 25.0f, 18.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Transformer Primary Inductance", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Low-H", "High-H" } },
                { "High-Pass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OFF", "80 Hz" } },
                { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },

        // =====================================================================
        // TAB 2: IRON / SAT (30 Studio Transformers, Mastering Tape, Lathes & Exciters: IR1..10, TP1..7, ST1..13)
        // =====================================================================
        {
            SchematicComponentType::OutputTransformer,
            SchematicCategory::IronAndTape,
            "Gapped-Steel Class-A Iron",
            "DC-Biased Gapped Steel Core with 25Hz Inductance Bump & Low-Mid Weight",
            "IR1",
            "dsp::circuits::IronTransformerStage",
            "British 1:2 step-up gapped steel core with 75mA DC asymmetric bias, 25Hz bump, and low-mid hysteresis.",
            gui::Color::fromHex(0x3A86FF),
            { "Primary Drive", "dB", -18.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "DC Standing Bias Current", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Secondary Output Trim", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Secondary Termination", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "600 Ohm Terminated", "Unterminated Resonant Peak" } },
                { "Gain Link", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "LINK IO", "FREE" } }
            }
        },
        {
            SchematicComponentType::InputTransformer,
            SchematicCategory::IronAndTape,
            "Nickel Mu-Metal Iron",
            "80% Nickel Core with Silky Top-End Sheen & Warm 50Hz Bloom",
            "IR2",
            "dsp::circuits::NickelBridgingTransformerStage",
            "80% Permalloy high-nickel input bridging hysteresis curve with silky top-end sheen and 50Hz bloom.",
            gui::Color::fromHex(0x4CC9F0),
            { "Input Drive", "dB", -18.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Low-Frequency Core Saturation Contour", "Hz", 20.0f, 120.0f, 49.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Winding Ratio", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "1 Line", "1:2 Step-Up" } },
                { "Bessel Zobel Network", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "DAMPED", "OPEN RINGING" } }
            }
        },
        {
            SchematicComponentType::SilverToroidalIron,
            SchematicCategory::IronAndTape,
            "Quad-Filar Output Iron",
            "Trifilar/Quadfilar Winding for Fast Slew & Aggressive 1.5kHz Midrange Snap",
            "IR3",
            "dsp::circuits::SilverToroidalTransformer",
            "Quad-filar 1:3 step-up output transformer model voiced for fast slew and aggressive 1.5kHz midrange snap.",
            gui::Color::fromHex(0x72EFDD),
            { "Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Winding Tap Ratio", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Core Alloy", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "50% Nickel", "Silicon Steel" } },
                { "Link IO", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "LINK", "FREE" } }
            }
        },
        {
            SchematicComponentType::OutputTransformerMatrix,
            SchematicCategory::IronAndTape,
            "Triple-Core Master Iron",
            "Selectable Output Transformer Core (Glossy Nickel, Class-A Iron, High-THD Steel)",
            "IR4",
            "dsp::circuits::OutputTransformerMatrix",
            "3-core switchable mastering transformer matrix: 0-33% Nickel, 34-66% Class-A Iron, 67-100% High-THD Steel.",
            gui::Color::fromHex(0x4361EE),
            { "Transformer Core", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "NICKEL", "IRON", "STEEL" } },
            { "Matrix Drive", "dB", -12.0f, 18.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Sub-Bass Head Bump", "dB", 0.0f, 6.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Zobel HF Roll-Off", "Hz", 6000.0f, 22000.0f, 17500.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::PaperInOilInterstageCap,
            SchematicCategory::IronAndTape,
            "PIO & Ferrite Coil",
            "Mid-Band Magnetic Inductor Core Hysteresis Without EQ Frequency Boosting",
            "IR5",
            "dsp::circuits::PaperInOilInterstageCap",
            "Pure LC tank magnetic core saturation and toroidal ferrite coil hysteresis without EQ frequency lift.",
            gui::Color::fromHex(0x48CAE4),
            { "Toroidal Coil Flux", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Midrange Ferrite Focus Frequency", "Hz", 400.0f, 4000.0f, 1264.9f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "PIO Capacitor Dielectric Lag", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "CLEAN", "VINTAGE PIO" } },
                { "PIO Foil Harmonics", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::LayeredPermalloyInterstage,
            SchematicCategory::IronAndTape,
            "Permalloy Interstage Iron",
            "1:3.16 Step-Up Tube Interstage Iron with Sweet 12kHz Winding Resonance",
            "IR6",
            "dsp::circuits::LayeredPermalloyInterstageStage",
            "Passive-EQ-to-tube 1:3.16 step-up high-permalloy interstage model with 12kHz secondary winding resonance.",
            gui::Color::fromHex(0x4EA8DE),
            { "Interstage Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Secondary Winding Resonance", "Hz", 8000.0f, 18000.0f, 13000.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Grid Load", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Loaded", "Unloaded" } },
                { "Inter-Winding HF Roll", "Hz", 6000.0f, 20000.0f, 15500.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Balanced600BridgingIron,
            SchematicCategory::IronAndTape,
            "Vintage 600-Ohm Line Iron",
            "Symmetrical High-Permalloy Input Iron with Smooth Sub-Sonic Phase Shift",
            "IR7",
            "dsp::circuits::Balanced600BridgingStage",
            "Symmetrical 600:600 high-permalloy broadcast bridging transformer with smooth sub-sonic phase dispersion.",
            gui::Color::fromHex(0x00BBF9),
            { "Input Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "600-Ohm Termination Load", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Impedance Strapping", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "150 Ohm", "600 Ohm" } },
                { "Long-Line HF Roll-Off", "Hz", 5000.0f, 20000.0f, 15000.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::SiliconValveOutputIron,
            SchematicCategory::IronAndTape,
            "Silicon-Steel Valve Iron",
            "Heavy Mu-Metal/Silicon-Steel Core Designed for High Tube Plate Current",
            "IR8",
            "dsp::circuits::SiliconValveOutputStage",
            "British valve output transformer hysteresis with grain-oriented silicon steel laminations.",
            gui::Color::fromHex(0x3A86FF),
            { "Push-Pull Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Core Hysteresis Depth", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Laminations", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "M6 Silicon Steel", "RadioMetal" } },
                { "Secondary Leakage Roll", "Hz", 6000.0f, 22000.0f, 16500.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::BesselWidebandServoIron,
            SchematicCategory::IronAndTape,
            "Twin-Servo Wide Iron",
            "Ultra-Linear Output Transformer with Zero Phase Rotation Down to 10Hz",
            "IR9",
            "dsp::circuits::BesselWidebandServoStage",
            "Wideband discrete twin-servo bifilar iron with Bessel ultrasonic roll-off and deep sub authority.",
            gui::Color::fromHex(0x00F5D4),
            { "Input Drive", "dB", -12.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Ultrasonic Phase Linearizer", "Hz", 20000.0f, 80000.0f, 40000.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "DC Servo", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "ON", "OFF" } },
                { "Sub-Sonic Servo Cutoff", "Hz", 15.0f, 120.0f, 28.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::CobaltAmorphousCoreIron,
            SchematicCategory::IronAndTape,
            "Amorphous Master Iron",
            "Zero-Magnetostriction Amorphous Strip Core for Mastering Harmonic Polish",
            "IR10",
            "dsp::circuits::CobaltAmorphousCoreStage",
            "Modern uncut cobalt amorphous core transformer model delivering mastering-grade harmonic polish.",
            gui::Color::fromHex(0x72EFDD),
            { "Mastering Flux Drive", "dB", -12.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Zero-Magnetostriction Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Core Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Cobalt Amorphous", "Mu-Metal" } },
                { "Ultrasonic Bandwidth", "Hz", 8000.0f, 24000.0f, 19500.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::TapeHysteresis,
            SchematicCategory::IronAndTape,
            "2-Inch Multitrack Tape",
            "Magnetic Tape Hysteresis with Bias Calibration & 60Hz/120Hz Head-Bump",
            "TP1",
            "dsp::circuits::TapeHysteresisStage",
            "Jiles-Atherton hysteresis + gap-loss + scrape flutter with 68Hz playback head bump and self-erasure.",
            gui::Color::fromHex(0xF72585),
            { "Input Calibrate", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Bias", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Repro HF EQ & Head Bump", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Calibrate Level", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tape Speed", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "7.5", "15", "30 IPS" } },
                { "Tape Oxide Formula", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "250", "456", "900" } },
                { "Equalization Curve", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "NAB", "CCIR" } }
            }
        },
        {
            SchematicComponentType::HighFreqTapeLimiter,
            SchematicCategory::IronAndTape,
            "Fat Tape Warmth",
            "Pre-Emphasized HF Soft Clipper Taming Harsh Cymbals & Vocal Sibilance",
            "TP2",
            "dsp::circuits::HighFreqTapeLimiterStage",
            "Dynamic high-frequency magnetic saturation curve taming harsh cymbal, brass, and vocal sibilance.",
            gui::Color::fromHex(0xFF5D8F),
            { "Warmth", "", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "1", "2", "3", "4", "5", "6", "7" }, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f } },
            { "Input Drive", "dB", -18.0f, 18.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Level", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Iron Saturation", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "LF", "HF IRON IN", "OUT PUSH" } },
                { "Dynamic Mode", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "BUSS", "TRACK", "SLAM PUSH" } }
            }
        },
        {
            SchematicComponentType::LoFiCassetteVhsWarper,
            SchematicCategory::IronAndTape,
            "Ferric Cassette Warper",
            "3.75 IPS Ferric Tape Saturation with Azimuth Phase Drift & Wow/Flutter",
            "TP3",
            "dsp::circuits::LoFiCassetteVhsStage",
            "Lo-fi magnetic oxide compression and transport azimuth drift with capstan scrape flutter.",
            gui::Color::fromHex(0xE0AAFF),
            { "Input Trim", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pitch / Varispeed", "%", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Wow, Flutter & Scrape Modulation", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tape Formulation", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "Type I Ferric", "Type II CrO2", "VHS Hi-Fi" } },
                { "Compander NR Artifact", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "OFF", "ON" } }
            }
        },
        {
            SchematicComponentType::HalfInch30IpsMasteringTape,
            SchematicCategory::IronAndTape,
            "Half-Inch 30 IPS Tape",
            "Transformerless 2-Track Mastering Head with Tight 55Hz Bump & Extended Air",
            "TP4",
            "dsp::circuits::HalfInch30IpsMasteringStage",
            "High-speed 1/2-inch 30 IPS mastering deck electronics and playback head physics with 55Hz bump.",
            gui::Color::fromHex(0xFF4D6D),
            { "Record Input Level", "dB", -12.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Repro Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Record Bias Calibration", "dB", -6.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Head Gap & Azimuth HF Polish", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tape Speed", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "7.5", "15", "30 IPS" } },
                { "Head Stack Width", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "1/4-Inch", "1/2-Inch", "1-Inch" } },
                { "Emphasis Curve", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "NAB", "CCIR", "AES" } }
            }
        },
        {
            SchematicComponentType::OneInch15IpsTubeTape,
            SchematicCategory::IronAndTape,
            "1-Inch All-Valve Tape",
            "All-Tube Tape Record/Repro Electronics with Thick 110Hz Bump & Oxide Squash",
            "TP5",
            "dsp::circuits::OneInch15IpsTubeTapeStage",
            "1960s 4-track/8-track all-tube tape machine with 110Hz head resonance and warm oxide squash.",
            gui::Color::fromHex(0xFF9E00),
            { "Valve Record Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Tube Bias & Tape Flux", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "55 Hz Tube Head-Bump Resonance", "dB", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Repro Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Speed", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "7.5 IPS", "15 IPS" } },
                { "Equalization", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "CCIR", "NAB" } }
            }
        },
        {
            SchematicComponentType::DifferentialAirCompander,
            SchematicCategory::IronAndTape,
            "High-Band Air Compander",
            "Encode-Only High-Band Differential Compander Trick for Classic Vocal Shimmer",
            "TP6",
            "dsp::circuits::DifferentialAirCompanderStage",
            "3kHz/9kHz upward low-level high-frequency differential compander encode stage for vocal shimmer.",
            gui::Color::fromHex(0x00F5D4),
            { "Input Threshold Drive", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Band 3 Air Gain", "dB", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Band 4 Sheen Gain", "dB", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mid Band Enable", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "OFF", "ON" } },
                { "Compander Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "ENCODE AIR TRICK", "FULL COMPAND" } }
            }
        },
        {
            SchematicComponentType::StainlessWireRecorder,
            SchematicCategory::IronAndTape,
            "40s Wire Recorder",
            "Magnetic Steel Wire Hysteresis with Band-Limited Vintage Midrange Bark",
            "TP7",
            "dsp::circuits::StainlessWireRecorderStage",
            "Early magnetic stainless-steel wire non-linear transfer curve with vintage midrange bark.",
            gui::Color::fromHex(0xFCA311),
            { "6J7 Tube Input Drive", "dB", -12.0f, 24.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Spool Speed & Transport Flutter", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Wire Spool High-Cut", "Hz", 1800.0f, 8500.0f, 4200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Wire Age", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "New Spool", "Oxidized 1948 Wire" } }
            }
        },
        {
            SchematicComponentType::VariableSilkExciter,
            SchematicCategory::IronAndTape,
            "Red & Blue Harmonic Silk",
            "Negative-Feedback Transformer Exciter (Blue Low-Weight vs. Red High-Mid Sheen)",
            "ST1",
            "dsp::circuits::VariableSilkExciterStage",
            "Frequency-weighted transformer feedback exciter morphing between Blue (low-mid weight) and Red (air sheen).",
            gui::Color::fromHex(0xFF4D6D),
            { "Texture", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Input Drive", "dB", -12.0f, 18.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Voicing Mode", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "OFF", "RED HF", "BLUE LF" } },
                { "Pre-Emphasis HPF", "Hz", 20.0f, 300.0f, 55.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::TrigonometricWavefolder,
            SchematicCategory::IronAndTape,
            "6AS6 Starved Valve Drive",
            "All-Valve Harmonic Generator with Starved-Plate Bias & Odd/Even Morphing",
            "ST2",
            "dsp::circuits::Starved6AS6ValveDriveStage",
            "Switchable triode 2nd-order vs. pentode 3rd-order all-valve harmonic core with starved-plate bias.",
            gui::Color::fromHex(0xF15BB5),
            { "Function Switch", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "T Triode", "P1 Pentode", "P2 Starved Pentode" } },
            { "Low-Pass Filter", "Hz", 0.0f, 4.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
              { "4 kHz", "6 kHz", "9 kHz", "20 kHz", "OFF" }, { 4000.0f, 6000.0f, 9000.0f, 20000.0f, 22000.0f } },
            {
                { "Drive", "dB", -12.0f, 30.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Bias", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Overdrive", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "OFF", "ON" } },
                { "Presence Lift", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "NORM", "PRESENCE" } }
            }
        },
        {
            SchematicComponentType::StatelessHarmonicInflator,
            SchematicCategory::IronAndTape,
            "Sonic Density Inflator",
            "Zero-Time-Constant Polynomial Waveshaper Boosting Perceived RMS Loudness",
            "ST3",
            "dsp::circuits::DynamicDensityInflatorStage",
            "Band-split polynomial shaper y=1.5x-0.5x^3 boosting perceived RMS loudness and harmonic density.",
            gui::Color::fromHex(0xFFBE0B),
            { "Input", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "Effect", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Curve", "", -50.0f, 50.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "Output", "dB", -12.0f, 0.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} },
                { "Band Split", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "OFF", "ON" } },
                { "Clip 0 dB", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "OFF", "ON" } }
            }
        },
        {
            SchematicComponentType::ParallelSaturationBus,
            SchematicCategory::IronAndTape,
            "Dark Bass Grit Bus",
            "Keeps Sub-Bass (<140Hz) Phase-Pure While Saturating Midrange Transients",
            "ST4",
            "dsp::circuits::DiscreteTransistorOverdrive",
            "2-band Linkwitz-Riley parallel saturation bus keeping sub-bass phase-pure while saturating midrange transients.",
            gui::Color::fromHex(0xFF8500),
            { "Drive", "dB", -12.0f, 24.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Blend", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Crossover Frequency", "Hz", 80.0f, 500.0f, 150.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Grunt", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::VintageAuralExciter,
            SchematicCategory::IronAndTape,
            "Harmonic Air & Sub Exciter",
            "High-Passed Harmonic Synthesizer Injecting Even/Odd Air Above 3.5kHz",
            "ST5",
            "dsp::circuits::HarmonicAirExciterStage",
            "Classic studio vocal and mix harmonic exciter topology injecting phase-shifted upper-octave air.",
            gui::Color::fromHex(0x00F5D4),
            { "High Tune Frequency", "Hz", 700.0f, 7000.0f, 2213.6f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "High Harmonics", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Sub Tune", "Hz", 50.0f, 200.0f, 100.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Sub-Resonance Optical Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PsychoacousticSubSynth,
            SchematicCategory::IronAndTape,
            "Sub-Harmonic Synthesizer",
            "Generates Precise 2nd, 3rd & 4th Harmonics from 30-90Hz Bass Fundamentals",
            "ST6",
            "dsp::circuits::MissingFundamentalSynth",
            "Psychoacoustic sub-bass harmonic reconstructor generating 2nd/3rd/4th harmonics from 30-90Hz fundamentals.",
            gui::Color::fromHex(0xFF007F),
            { "Subharmonic Level", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "24-36 Hz Band Boost", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "36-56 Hz Band Boost", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Crossover", "Hz", 0.0f, 3.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "50 Hz", "80 Hz", "100 Hz", "120 Hz" }, { 50.0f, 80.0f, 100.0f, 120.0f } },
                { "Main Output Trim", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::AsymmetricalDiodeClipper,
            SchematicCategory::IronAndTape,
            "Multi-Diode Shunt Clipper",
            "Selectable Silicon, Germanium & LED Forward-Voltage Shunt Clipping Array",
            "ST7",
            "dsp::circuits::AsymmetricalDiodeClipperStage",
            "Multi-diode forward-voltage wave-shaping array with variable Silicon, Germanium, and LED symmetry skew.",
            gui::Color::fromHex(0xFF0055),
            { "Gain / Drive", "dB", -12.0f, 24.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Diode Asymmetry", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Treble / Tone", "Hz", 400.0f, 12000.0f, 2190.9f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Diode Type", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "Germanium 0.3V", "Silicon 0.7V", "Red LED 1.8V" } }
            }
        },
        {
            SchematicComponentType::DacBitcrusherStage,
            SchematicCategory::IronAndTape,
            "12-Bit Sample & Clock Crunch",
            "Variable 8-to-16 Bit Fixed-Point Quantizer with Zero-Order-Hold Aliasing",
            "ST8",
            "dsp::circuits::DacBitcrusherStage",
            "Vintage studio digital converter quantization model with R-2R ladder bit-depth and sample-clock decimation.",
            gui::Color::fromHex(0xB5179E),
            { "Bit Depth", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sample Rate Clock", "Hz", 2000.0f, 48000.0f, 9798.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Resonant Filter Cutoff", "Hz", 800.0f, 18000.0f, 3794.7f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Input Drive", "dB", -12.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Dither", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "OFF", "ON" } },
                { "Filter Bump", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "OFF", "ON" } }
            }
        },
        {
            SchematicComponentType::MasteringVinylCuttingLathe,
            SchematicCategory::IronAndTape,
            "Mastering Vinyl Lathe",
            "RIAA Pre/De-Emphasis, HF Slew/Acceleration Limiter & Elliptical Groove Sum",
            "ST9",
            "dsp::circuits::MasteringVinylCuttingLatheStage",
            "Physical cutterhead feedback and high-frequency acceleration limiting with RIAA groove emphasis.",
            gui::Color::fromHex(0xFFD166),
            { "Cutting Level", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "HF Acceleration Limiter Threshold", "Hz", 2000.0f, 16000.0f, 5656.9f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Elliptical Bass Mono-Maker Cutoff", "Hz", 40.0f, 300.0f, 109.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Groove Tracing Distortion", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Lathe Speed", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "33.3 RPM", "45 RPM" } }
            }
        },
        {
            SchematicComponentType::TriodePentode3BandMatrix,
            SchematicCategory::IronAndTape,
            "Tri-Band Harmonic Matrix",
            "Applies 2nd-Order Triode Warmth to Lows/Mids and 3rd-Order Pentode Sheen to Highs",
            "ST10",
            "dsp::circuits::TriodePentode3BandMatrixStage",
            "Mastering multi-band harmonic generator applying triode warmth to lows/mids and pentode sheen to highs.",
            gui::Color::fromHex(0xFF9E00),
            { "2nd Harmonic Triode Blend", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "3rd Harmonic Pentode Blend", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Low/Mid Crossover", "Hz", 60.0f, 500.0f, 173.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mid/High Crossover", "Hz", 1000.0f, 8000.0f, 2828.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Shape / THD Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::EnvelopeGatedTransientExciter,
            SchematicCategory::IronAndTape,
            "Transient Harmonic Exciter",
            "Synthesizes Harmonics Strictly on Drum/Vocal Transients Without Muddy Sustain",
            "ST11",
            "dsp::circuits::EnvelopeGatedTransientExciterStage",
            "Differential envelope-controlled harmonic exciter saturating attacks while keeping sustain tails clean.",
            gui::Color::fromHex(0x06D6A0),
            { "Transient Sensitivity Threshold", "dB", -36.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Transient Harmonic Saturation", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Focus Frequency", "Hz", 800.0f, 10000.0f, 2828.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Dry / Wet Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ConsoleBusRailOverdrive,
            SchematicCategory::IronAndTape,
            "Console Rail Overdrive",
            "Simulates Driving an Analog Console Master Bus Past its Power Supply Rails",
            "ST12",
            "dsp::circuits::ConsoleBusRailOverdriveStage",
            "Asymmetrical slew-rate and hard +/-15V bipolar power-supply rail clipping on console master bus.",
            gui::Color::fromHex(0xFF4D6D),
            { "Bus Input Push", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Power Supply Rail Sag", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Slew-Rate Limiting", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Pad", "dB", -24.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Sub-Bass HPF", "Hz", 20.0f, 250.0f, 35.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::StudioPcm14BitConverter,
            SchematicCategory::IronAndTape,
            "14-Bit Studio PCM DAC",
            "Early Digital Mastering Processor with Clock Jitter, Dither & Steep Bessel LPF",
            "ST13",
            "dsp::circuits::StudioPcm14BitConverterStage",
            "Early digital master recorder 14-bit linear PCM AD/DA chain with clock jitter and steep Bessel LPF.",
            gui::Color::fromHex(0x4CC9F0),
            { "ADC Input Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Wordlength", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Anti-Aliasing Brickwall Ringing", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "J-17 Pre-Emphasis", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "ON", "OFF" } }
            }
        },
        {
            SchematicComponentType::SlidingActionSpectralCompander,
            SchematicCategory::IronAndTape,
            "3-Band Spectral Compander",
            "Multi-Tier Variable-Turnover Encode Enhancer for High/Mid Density & Air",
            "TP8",
            "dsp::circuits::SlidingActionSpectralCompanderStage",
            "3-band sliding-filter spectral compander encode stage lifting low-level midrange density and vocal air.",
            gui::Color::fromHex(0x00BBF9),
            { "Input Drive", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Low-Level Density Lift", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Spectral Modulation Skew", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Action Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Encode Enhancer", "Symmetric Spectral" } }
            }
        },
        {
            SchematicComponentType::OpticalSoundtrackFilmExciter,
            SchematicCategory::IronAndTape,
            "35mm Optical Film Track",
            "Incandescent Light-Valve Slit Diffraction with Photocell Soft Saturation",
            "TP9",
            "dsp::circuits::OpticalSoundtrackFilmStage",
            "35mm variable-area optical film soundtrack modulator with slit diffraction roll-off and photocell warmth.",
            gui::Color::fromHex(0xF72585),
            { "Light-Valve Ribbon Drive", "dB", -12.0f, 24.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Photocell Slit Aperture HF Roll-Off", "Hz", 5000.0f, 16000.0f, 10500.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Academy Curve Contour", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Film Track Type", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Variable-Area Duplex", "Variable-Density" } }
            }
        },
        {
            SchematicComponentType::CollectorSaturationMatrix3Path,
            SchematicCategory::IronAndTape,
            "Tri-Path Harmonic Matrix",
            "Continuous JFET/BJT Current-Mirror Triode, Pentode & Tape Harmonic Synthesizer",
            "ST15",
            "dsp::circuits::CollectorSaturationMatrixStage",
            "Parallel transistor collector-saturation matrix blending Triode 2nd-order, Pentode 3rd-order, and Tape curves.",
            gui::Color::fromHex(0xFF9F1C),
            { "Triode", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pentode", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Tape", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Collector Bias Symmetry", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::DynamicFluxDeSaturator,
            SchematicCategory::IronAndTape,
            "Contour Flux De-Clipper",
            "Inverse Low-Frequency Polynomial Counter-Waveshaper Tightening Muddy Sub-Bass",
            "ST16",
            "dsp::circuits::DynamicFluxDeSaturatorStage",
            "Concave inverse polynomial counter-waveshaper restoring transient crest factor to over-saturated bass and mixes.",
            gui::Color::fromHex(0x06D6A0),
            { "Clip Detection Threshold", "dB", -18.0f, 0.0f, -9.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Counter-Curvature Restoration", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Crest-Factor Recovery", "dB", 0.0f, 12.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Envelope Release", "ms", 20.0f, 400.0f, 95.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::GainRangingFloatingPointDac,
            SchematicCategory::IronAndTape,
            "Gain-Ranging Floating DAC",
            "Stepped Mantissa/Exponent Converter with Envelope-Modulated Quantization",
            "ST17",
            "dsp::circuits::GainRangingFloatingPointDacStage",
            "1978 10-bit mantissa + 3-bit exponent gain-ranging floating-point converter with Chebyshev reconstruction filter.",
            gui::Color::fromHex(0x9B5DE5),
            { "Input Drive", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Mantissa Resolution", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Exponent Step Speed", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -18.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Dry / Wet Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::DielectricAbsorptionCapAging,
            SchematicCategory::IronAndTape,
            "Paper-In-Oil Cap Aging",
            "Non-Linear Voltage-Coefficient Capacitance & Multi-Pole Memory Relaxation",
            "ST18",
            "dsp::circuits::DielectricAbsorptionCapStage",
            "Models vintage wax-paper and electrolytic coupling capacitor voltage nonlinearity and dielectric memory soak.",
            gui::Color::fromHex(0xFFD166),
            { "Dielectric Memory", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "ESR High-Frequency Loss", "Hz", 8000.0f, 20000.0f, 14000.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Coupling Cap Low-Cut Drift", "Hz", 10.0f, 90.0f, 30.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Leakage DC Bias", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },

        // =====================================================================
        // TAB 3: EQ / SPEC (16 Analog Filters, Console EQs & Spectral FFT Processors: EQ1..10, SP1..6)
        // =====================================================================
        {
            SchematicComponentType::PassivePultecEq,
            SchematicCategory::EqAndFilters,
            "Passive Offset-RC Low EQ",
            "Offset-Corner Passive RC Boost & Cut Shelf with Resonant Low-Mid Dip",
            "EQ1",
            "dsp::circuits::PassiveLcProgramEq",
            "-16dB passive RLC insertion loss + offset Low Boost/Atten scoop (f_cut=1.8*f_boost) into +16dB push-pull dual-triode amp.",
            gui::Color::fromHex(0x00BBF9),
            { "Low Frequency (CPS)", "Hz", 0.0f, 3.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "20", "30", "60", "100" }, { 20.0f, 30.0f, 60.0f, 100.0f } },
            { "Low Boost", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Low Atten", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tube Makeup Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Transformer Drive", "dB", 0.0f, 12.0f, 2.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ResonantSubSculptor,
            SchematicCategory::EqAndFilters,
            "Passive High Inductor EQ",
            "Multi-Tap Toroidal Inductor Bell Boost (3k-16kHz) with Passive High Roll-Off",
            "EQ2",
            "dsp::circuits::PassiveHighInductorEqStage",
            "Multi-tap toroidal inductor bell boost with passive high-frequency roll-off and resonant foundation sculpting.",
            gui::Color::fromHex(0x00F5D4),
            { "High Boost Freq (KCS)", "kHz", 0.0f, 6.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "3", "4", "5", "8", "10", "12", "16" }, { 3.0f, 4.0f, 5.0f, 8.0f, 10.0f, 12.0f, 16.0f } },
            { "Atten Sel", "kHz", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "5", "10", "20" }, { 5.0f, 10.0f, 20.0f } },
            {
                { "High Boost", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Bandwidth", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High Atten", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ParallelPassiveLcEq,
            SchematicCategory::EqAndFilters,
            "Parallel 4-Band LC Tube EQ",
            "Non-Additive Parallel Inductor-Capacitor Filter Bank with Tube Makeup",
            "EQ3",
            "dsp::circuits::ParallelLcMasteringEq",
            "4 parallel LC tank bands whose impedances sum & self-limit organically into a warm push-pull tube makeup stage.",
            gui::Color::fromHex(0x2EC4B6),
            { "Band 1 Low Gain", "dB", 0.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Band 1 Low Frequency", "Hz", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "22", "33", "47", "68", "100", "150", "220", "330", "470", "680", "1000" },
              { 22.0f, 33.0f, 47.0f, 68.0f, 100.0f, 150.0f, 220.0f, 330.0f, 470.0f, 680.0f, 1000.0f } },
            {
                { "Band 1 Low Bandwidth", "", 1.0f, 3.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 1 Mode & Bell", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "BOOST", "OUT", "CUT" } },
                { "Band 2 Low-Mid Gain", "dB", 0.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 2 Low-Mid Frequency", "Hz", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "100", "150", "220", "330", "470", "680", "1k", "1.5k", "2.2k", "2.7k", "3.3k" },
                  { 100.0f, 150.0f, 220.0f, 330.0f, 470.0f, 680.0f, 1000.0f, 1500.0f, 2200.0f, 2700.0f, 3300.0f } },
                { "Band 2 Low-Mid Bandwidth", "", 1.0f, 3.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 2 Mode & Bell", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "BOOST", "OUT", "CUT" } },
                { "Band 3 High-Mid Gain", "dB", 0.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 3 High-Mid Frequency", "Hz", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "470", "680", "1k", "1.5k", "2.2k", "3.3k", "4.7k", "6.8k", "10k", "12k", "16k" },
                  { 470.0f, 680.0f, 1000.0f, 1500.0f, 2200.0f, 3300.0f, 4700.0f, 6800.0f, 10000.0f, 12000.0f, 16000.0f } },
                { "Band 3 High-Mid Bandwidth", "", 1.0f, 3.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 3 Mode & Bell", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "BOOST", "OUT", "CUT" } },
                { "Band 4 High Gain", "dB", 0.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 4 High Frequency", "Hz", 0.0f, 10.0f, 6.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "1.5k", "2.2k", "3.3k", "4.7k", "6.8k", "8.2k", "10k", "12k", "16k", "18k", "27k" },
                  { 1500.0f, 2200.0f, 3300.0f, 4700.0f, 6800.0f, 8200.0f, 10000.0f, 12000.0f, 16000.0f, 18000.0f, 27000.0f } },
                { "Band 4 High Bandwidth", "", 1.0f, 3.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 4 Mode & Bell", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "BOOST", "OUT", "CUT" } },
                { "High-Pass Filter", "Hz", 0.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
                  { "OUT", "22", "39", "68", "120", "220" }, { 0.0f, 22.0f, 39.0f, 68.0f, 120.0f, 220.0f } },
                { "Low-Pass Filter", "Hz", 0.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
                  { "OUT", "50k", "27k", "18k", "12k", "9k" }, { 0.0f, 50000.0f, 27000.0f, 18000.0f, 12000.0f, 9000.0f } },
                { "Tube Makeup Trim", "dB", -6.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::InductorEq3Band,
            SchematicCategory::EqAndFilters,
            "3-Band Inductor Console EQ",
            "Fixed 12kHz Capacitor Shelf, Saturating Inductor Mids & 18dB/oct Resonant HPF",
            "EQ4",
            "dsp::circuits::InductorEq3Band",
            "Proportional-Q tapped inductor mid bell with magnetic core saturation + 110Hz low shelf + 12kHz high shelf + 18dB/oct HPF.",
            gui::Color::fromHex(0x00F5D4),
            { "Low Frequency Selector", "Hz", 0.0f, 4.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "OFF", "35", "60", "110", "220" }, { 0.0f, 35.0f, 60.0f, 110.0f, 220.0f } },
            { "Mid Frequency Selector", "kHz", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "OFF", "0.36", "0.70", "1.6", "3.2", "4.8", "7.2" }, { 0.0f, 0.36f, 0.70f, 1.6f, 3.2f, 4.8f, 7.2f } },
            {
                { "Low Shelf Gain", "dB", -16.0f, 16.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mid Bell Gain", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High 12kHz Shelf Gain", "dB", -16.0f, 16.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High-Pass Filter", "Hz", 0.0f, 4.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
                  { "OFF", "50", "80", "160", "300" }, { 0.0f, 50.0f, 80.0f, 160.0f, 300.0f } }
            }
        },
        {
            SchematicComponentType::ProportionalQGyratorEq,
            SchematicCategory::EqAndFilters,
            "Proportional-Q Gyrator EQ",
            "Active Op-Amp Gyrator Filter Bank (Wide Q at Small Boosts, Narrow Q at Extreme Boosts)",
            "EQ5",
            "dsp::circuits::ProportionalQGyrator",
            "Active gyrator peaking network where Q = Q0 * 2^(|Gain|/4.0), driven by a 24V slew-limited discrete op-amp.",
            gui::Color::fromHex(0x00E5FF),
            { "Low Frequency", "Hz", 0.0f, 4.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ConcentricDualKnob,
              { "50", "100", "200", "300", "400" }, { 50.0f, 100.0f, 200.0f, 300.0f, 400.0f } },
            { "Low Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ConcentricDualKnob, {} },
            {
                { "Mid Frequency", "Hz", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ConcentricDualKnob,
                  { "200", "400", "600", "800", "1.5k", "3.0k", "5.0k" }, { 200.0f, 400.0f, 600.0f, 800.0f, 1500.0f, 3000.0f, 5000.0f } },
                { "Mid Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ConcentricDualKnob, {} },
                { "High Frequency", "Hz", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ConcentricDualKnob,
                  { "2.5k", "5k", "7k", "10k", "12.5k", "15k", "20k" }, { 2500.0f, 5000.0f, 7000.0f, 10000.0f, 12500.0f, 15000.0f, 20000.0f } },
                { "High Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ConcentricDualKnob, {} },
                { "Low Band Shape", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "PEAK", "SHELF" } },
                { "High Band Shape", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "PEAK", "SHELF" } },
                { "Bandpass Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonLatching, { "OUT", "50Hz-15kHz IN" } }
            }
        },
        {
            SchematicComponentType::BaxandallTone,
            SchematicCategory::EqAndFilters,
            "Constant-Q Console EQ",
            "Aggressive Constant-Q State-Variable Console EQ for Surgical Drum Sculpting",
            "EQ6",
            "dsp::circuits::ConstantQConsoleEqStage",
            "Aggressive constant-Q state-variable console equalizer stack voiced for punchy drum and mix bus sculpting.",
            gui::Color::fromHex(0x48CAE4),
            { "LF Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "LF Frequency", "Hz", 30.0f, 450.0f, 116.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "LMF Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "LMF Frequency", "Hz", 200.0f, 2500.0f, 707.1f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "LMF Q", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "HMF Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "HMF Frequency", "Hz", 600.0f, 7000.0f, 2049.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "HMF Q", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "HF Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "HF Frequency", "Hz", 1500.0f, 16000.0f, 4899.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High-Pass Filter", "Hz", 16.0f, 350.0f, 20.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Low-Pass Filter", "Hz", 3000.0f, 22000.0f, 20000.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "LF Bell", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonLatching, { "Shelf", "Bell" } },
                { "HF Bell", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonLatching, { "Shelf", "Bell" } },
                { "Q Topology Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "Constant-Q Black", "Proportional-Q Brown" } }
            }
        },
        {
            SchematicComponentType::ProportionalGraphicEq10Band,
            SchematicCategory::EqAndFilters,
            "10-Band Proportional Graphic",
            "1-Octave Center-Detent Op-Amp Graphic Equalizer for Fast Midrange Shaping",
            "EQ7",
            "dsp::circuits::Proportional10BandGraphicEq",
            "Active gyrator 10-band octave graphic bank voiced for fast snare, mix bus, and midrange shaping.",
            gui::Color::fromHex(0x80ED99),
            { "31 Hz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "63 Hz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "125 Hz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "250 Hz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "500 Hz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "1 kHz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "2 kHz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "4 kHz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "8 kHz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "16 kHz Band Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "Pol", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonLatching, { "0 deg NORM", "180 deg Polarity Invert" } },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::FulcrumTiltAirEq,
            SchematicCategory::EqAndFilters,
            "40kHz Air & Tilt EQ",
            "Phase-Coherent Sub-to-40kHz Air Shelf & Full-Spectrum Fulcrum Tilt Filter",
            "EQ8",
            "dsp::circuits::FulcrumTiltAirEq",
            "Tilts the entire spectrum around a phase-coherent 650Hz center fulcrum with an ultra-high 40kHz air shelf.",
            gui::Color::fromHex(0x56CFE1),
            { "Air Band Frequency", "kHz", 0.0f, 4.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "2.5", "5", "10", "20", "40" }, { 2.5f, 5.0f, 10.0f, 20.0f, 40.0f } },
            { "Air Gain", "dB", 0.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Spectral Tilt", "dB", -6.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tilt Fulcrum Frequency", "Hz", 250.0f, 1200.0f, 547.7f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Sub 40 Hz Shelf", "dB", -10.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Level Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SilkBerylliumAirExciter,
            SchematicCategory::AutoAndSmart,
            "Auto Bass Leveller",
            "30-320 Hz Fundamental Isolator with Fret/Slap Blanking & Note-Onset Sustain Leveller",
            "AU2",
            "dsp::circuits::AutoBassFundamentalRiderStage",
            "Isolates 30-320 Hz bass fundamentals while ignoring fret clank and slap transients, resetting on each note onset to level sustain note-by-note.",
            gui::Color::fromHex(0x90E0EF),
            { "Target Bass Level", "dB", -36.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "Leveller Range", "dB", 0.0f, 12.0f, 8.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Fundamental Ceiling", "Hz", 80.0f, 320.0f, 180.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Fret & Slap Guard", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Note Onset Speed", "ms", 10.0f, 400.0f, 65.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} }
            }
        },
        {
            SchematicComponentType::ResonantFilter,
            SchematicCategory::EqAndFilters,
            "4-Pole Ladder & SVF Filter",
            "Self-Oscillating Analog Low/Band/High-Pass Filter with Envelope & CV Drive",
            "EQ10",
            "dsp::circuits::TransistorLadderSvfFilterStage",
            "Self-oscillating zero-delay-voiced state-variable and ladder resonant filter (60Hz - 18kHz).",
            gui::Color::fromHex(0x52B788),
            { "Cutoff Frequency", "Hz", 20.0f, 20000.0f, 632.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Resonance / Emphasis", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Filter Input Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Envelope", "%", -100.0f, 100.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "LP -> BP -> HP Mode Morph", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BarkAuditoryUnmasker,
            SchematicCategory::EqAndFilters,
            "Auto Clarity Unmasker",
            "Bark-Scale Auditory Filterbank Balancing Masked Detail vs. Dominant Peaks",
            "SP1",
            "dsp::circuits::BarkScaleAuditoryUnmasker",
            "Models cochlear Bark critical bands to dynamically unmask buried air/detail (Recover) while taming dominant masking peaks (Tame).",
            gui::Color::fromHex(0x64DFDF),
            { "Recover", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Tame", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Bias", "%", -100.0f, 100.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Brighten", "%", -100.0f, 100.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Boost", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SpectralResonanceSuppressor,
            SchematicCategory::AutoAndSmart,
            "Auto Resonance Suppressor",
            "Local Spectral Median Comparator Dynamically Notching Whistling Peaks",
            "AU4",
            "dsp::circuits::SpectralResonanceTamer",
            "Compares narrowband energy against the local spectral median to dynamically notch whistling peaks without dulling the mix.",
            gui::Color::fromHex(0x72EFDD),
            { "Depth", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sharpness", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Selectivity", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Attack/Release Speed", "ms", 0.1f, 100.0f, 3.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mix/Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::TransistorLadderFilter,
            SchematicCategory::EqAndFilters,
            "Spectral Pick & Click",
            "Crossover-Free FFT Flux Analyzer Boosting or Cutting Pick Clicks & Drum Thump",
            "SP3",
            "dsp::circuits::SpectralTransientBinShaperStage",
            "Frequency-selective transient shaper sculpting pick clicks, vocal mouth clicks, and drum thump across the spectrum.",
            gui::Color::fromHex(0x06D6A0),
            { "Depth", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sensitivity", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Sharpness", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Decay", "ms", 20.0f, 2000.0f, 200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Focus Freq", "Hz", 60.0f, 16000.0f, 979.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SidechainSpectralCarver32,
            SchematicCategory::EqAndFilters,
            "32-Band Space Carver",
            "Inverts External Sidechain Spectrum to Carve Real-Time Vocal/Lead Pocket",
            "SP4",
            "dsp::circuits::SidechainSpectralCarver32BandStage",
            "32-band dynamic spectral carver creating a real-time frequency pocket for lead vocals, kicks, or solo instruments.",
            gui::Color::fromHex(0x38B000),
            { "Amount", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Low Cut", "Hz", 20.0f, 600.0f, 100.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High Cut", "Hz", 1500.0f, 18000.0f, 8000.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Attack/Release", "ms", 0.1f, 100.0f, 3.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "M/S Focus", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::HarmonicPitchTrackingEq,
            SchematicCategory::EqAndFilters,
            "Pitch-Track Harmonic EQ",
            "Autocorrelation Pitch Tracker Locking EQ Filter Frequencies to Played Note Harmonics",
            "SP5",
            "dsp::circuits::HarmonicPitchTrackEqStage",
            "Autocorrelation fundamental pitch tracker locking parametric filter bands to the played note's harmonic series.",
            gui::Color::fromHex(0x00BBF9),
            { "Pitch Range", "Hz", 35.0f, 1200.0f, 110.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Fundamental Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Harmonic 2/3 Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Comb Q", "", 0.5f, 10.0f, 5.25f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Glide Speed", "Hz", 0.1f, 12.0f, 1.1f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PhasePolarityAligner,
            SchematicCategory::EqAndFilters,
            "180 Phase & Delay Aligner",
            "0-360 Deg All-Pass Phase Rotator & Sub-Sample Time Delay for Multi-Mic Alignment",
            "SP6",
            "dsp::circuits::PhasePolarityAlignerStage",
            "Continuous 0-360 degree all-pass phase rotator and sub-sample time alignment utility for multi-mic drums and bass.",
            gui::Color::fromHex(0x4CC9F0),
            { "Phase Sweep", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Micro-Delay", "ms", 0.02f, 25.0f, 1.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "All-Pass Center Freq", "Hz", 60.0f, 16000.0f, 979.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Polarity Invert", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "0 deg NORM", "180 deg INV" }, {}, true }
            }
        },
        {
            SchematicComponentType::PassiveMidDipPeakEq,
            SchematicCategory::EqAndFilters,
            "Passive Mid Dip & Peak EQ",
            "Dual-Dip & Single-Peak Passive Inductor Midrange Sculptor with Tube Makeup",
            "EQ11",
            "dsp::circuits::PassiveMidDipPeakEqStage",
            "Passive 3-band midrange LC sculptor pairing low-mid and high-mid dip tanks with a central presence peak and tube makeup.",
            gui::Color::fromHex(0x00BBF9),
            { "Low Peak Frequency", "Hz", 0.0f, 4.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "200", "300", "500", "700", "1000" }, { 200.0f, 300.0f, 500.0f, 700.0f, 1000.0f } },
            { "Mid Dip Frequency", "Hz", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "200", "300", "500", "700", "1k", "1.5k", "2k", "3k", "4k", "5k", "7k" },
              { 200.0f, 300.0f, 500.0f, 700.0f, 1000.0f, 1500.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f, 7000.0f } },
            {
                { "High Peak Frequency", "kHz", 0.0f, 4.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "1.5", "2.0", "3.0", "4.0", "5.0" }, { 1.5f, 2.0f, 3.0f, 4.0f, 5.0f } },
                { "Low Peak Boost", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mid Dip Atten", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High Peak Boost", "", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tube Makeup Gain", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::PassiveHighLowBracketEq,
            SchematicCategory::EqAndFilters,
            "Passive LC Bracket Filter",
            "Constant-K Passive LC High-Pass & Low-Pass Steep Bandwidth Bracket Filter",
            "EQ12",
            "dsp::circuits::PassiveHighLowBracketEqStage",
            "Constant-k passive LC ladder high/low passband bracket filter with 600-Ohm transformer passband knee.",
            gui::Color::fromHex(0x48CAE4),
            { "Low Cut", "Hz", 0.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "OFF", "50", "80", "100", "150", "250", "500", "750", "1k", "1.5k", "2k" },
              { 0.0f, 50.0f, 80.0f, 100.0f, 150.0f, 250.0f, 500.0f, 750.0f, 1000.0f, 1500.0f, 2000.0f } },
            { "High Cut", "Hz", 0.0f, 10.0f, 10.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "1.5k", "2k", "3k", "4k", "5k", "6k", "8k", "10k", "12k", "15k", "OFF" },
              { 1500.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f, 6000.0f, 8000.0f, 10000.0f, 12000.0f, 15000.0f, 22000.0f } },
            {
                { "Makeup Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Passband Resonance", "dB", 0.0f, 6.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Transformer Load", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "600 OHM", "HI-Z" }, {}, true }
            }
        },
        {
            SchematicComponentType::Olympic69InductorEq,
            SchematicCategory::EqAndFilters,
            "Tapped-Inductor Console EQ",
            "Tapped Permalloy Mid Inductor & Silver-Mica Shelf with Class-A Transistor Makeup",
            "EQ13",
            "dsp::circuits::TappedInductorConsoleEqStage",
            "1960s British console passive LC equalizer with tapped permalloy mid inductor and discrete silicon Class-A makeup.",
            gui::Color::fromHex(0x00F5D4),
            { "Bass Frequency Selector", "Hz", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "60", "100", "200", "300" }, { 60.0f, 100.0f, 200.0f, 300.0f } },
            { "Mid Frequency Selector", "kHz", 0.0f, 7.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "0.7", "1.0", "1.4", "2.0", "2.8", "3.5", "4.5", "6.0" }, { 0.7f, 1.0f, 1.4f, 2.0f, 2.8f, 3.5f, 4.5f, 6.0f } },
            {
                { "Treble 10 kHz Shelf Boost", "dB", -6.0f, 16.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Bass Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mid Gain", "dB", 0.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mid Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "PK Peak Boost", "TRGH Trough Cut" } },
                { "Bass Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "Boost Shelf", "Trough Cut" } },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Detroit7BandToroidalEq,
            SchematicCategory::EqAndFilters,
            "7-Band Toroidal Graphic EQ",
            "Hand-Wound Toroidal LC Tank Resonators (50Hz to 12.5kHz) for Soul/Funk Punch",
            "EQ14",
            "dsp::circuits::Detroit7BandToroidalEqStage",
            "7-band passive toroidal inductor graphic equalizer delivering classic 1960s soul/funk bass and midrange punch.",
            gui::Color::fromHex(0xFFB703),
            { "50 Hz Sub-Bass Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "130 Hz Bass Punch Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "320 Hz Low-Mid Warmth Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "800 Hz Midrange Bark Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "2.0 kHz Vocal Presence Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "5.0 kHz Presence Bite Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "12.5 kHz Tambourine Air Tank", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "Master Makeup Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::LondonARangeInductorEq,
            SchematicCategory::EqAndFilters,
            "4-Band Multi-Tap Inductor EQ",
            "4 Multi-Tap Inductor Bands with Steep 18dB/Oct Elliptical High/Low Cut",
            "EQ15",
            "dsp::circuits::MultiTap4BandInductorEqStage",
            "1970s London studio 4-band inductor channel equalizer with inductor-voiced high bell and discrete Class-A buffers.",
            gui::Color::fromHex(0xF72585),
            { "Low Shelf Frequency", "Hz", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "50", "80", "100", "150" }, { 50.0f, 80.0f, 100.0f, 150.0f } },
            { "Low Shelf Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Low-Mid Inductor Frequency", "Hz", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "250", "500", "1k", "2k" }, { 250.0f, 500.0f, 1000.0f, 2000.0f } },
                { "Low-Mid Bell Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "High-Mid Inductor Frequency", "Hz", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "3k", "5k", "7k", "9k" }, { 3000.0f, 5000.0f, 7000.0f, 9000.0f } },
                { "High-Mid Bell Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "High Shelf Frequency", "Hz", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "8k", "10k", "12k", "15k" }, { 8000.0f, 10000.0f, 12000.0f, 15000.0f } },
                { "High Shelf Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "High-Pass Filter Buttons", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonBank,
                  { "25 Hz", "50 Hz", "100 Hz" }, { 25.0f, 50.0f, 100.0f } },
                { "Low-Pass Filter Buttons", "", 0.0f, 2.0f, 2.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::PushButtonBank,
                  { "9 kHz", "12 kHz", "15 kHz" }, { 9000.0f, 12000.0f, 15000.0f } }
            }
        },
        {
            SchematicComponentType::MasteringBaxandallEq,
            SchematicCategory::EqAndFilters,
            "Reciprocal Mastering Shelf",
            "Wide-Bandwidth 1st-Order Reciprocal Shelving Network with +/-28V Discrete Op-Amps",
            "EQ16",
            "dsp::circuits::ReciprocalMasteringShelfStage",
            "Ultra-low-phase-shift reciprocal gentle-slope bass and treble shelving equalizer voiced for stereo mastering.",
            gui::Color::fromHex(0x72EFDD),
            { "Low Cut Filter", "Hz", 0.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
              { "OUT", "12", "18", "24", "30", "36", "43", "54", "80" }, { 0.0f, 12.0f, 18.0f, 24.0f, 30.0f, 36.0f, 43.0f, 54.0f, 80.0f } },
            { "High Cut Filter", "kHz", 0.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch,
              { "OUT", "70", "50", "40", "32", "28", "24", "18", "12" }, { 0.0f, 70.0f, 50.0f, 40.0f, 32.0f, 28.0f, 24.0f, 18.0f, 12.0f } },
            {
                { "Low Shelf Frequency", "Hz", 0.0f, 7.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "74", "84", "98", "116", "131", "166", "230", "361" }, { 74.0f, 84.0f, 98.0f, 116.0f, 131.0f, 166.0f, 230.0f, 361.0f } },
                { "High Shelf Frequency", "kHz", 0.0f, 7.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "1.6", "1.8", "2.2", "2.7", "3.4", "4.3", "6.2", "18.0" }, { 1.6f, 1.8f, 2.2f, 2.7f, 3.4f, 4.3f, 6.2f, 18.0f } },
                { "Low Shelf Gain", "dB", -5.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High Shelf Gain", "dB", -5.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ParallelStateVariableEq,
            SchematicCategory::EqAndFilters,
            "Parallel SVF Mastering EQ",
            "Parallel-Summed TPT State-Variable Biquad Cells Avoiding Cumulative Phase Smear",
            "EQ17",
            "dsp::circuits::ParallelStateVariableEqStage",
            "Parallel-summed precision state-variable mastering equalizer eliminating series phase accumulation.",
            gui::Color::fromHex(0x06D6A0),
            { "Band 1 Low Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Band 1 Low Frequency", "Hz", 15.0f, 800.0f, 109.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Band 1 Low Q / Shelf", "", 0.4f, 4.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 2 Low-Mid Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 2 Low-Mid Frequency", "Hz", 120.0f, 2500.0f, 547.7f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 2 Low-Mid Q", "", 0.4f, 4.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 3 High-Mid Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 3 High-Mid Frequency", "Hz", 400.0f, 8000.0f, 1788.9f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 3 High-Mid Q", "", 0.4f, 4.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 4 High Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Band 4 High Frequency", "Hz", 1200.0f, 26000.0f, 5585.7f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level Trim", "dB", -12.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Band 1 Shelf/Peak Toggle", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "SHELF", "PEAK" } },
                { "Band 4 Shelf/Peak Toggle", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "SHELF", "PEAK" } }
            }
        },
        {
            SchematicComponentType::GermanBroadcastSilkyEq,
            SchematicCategory::EqAndFilters,
            "Wien-Bridge Broadcast EQ",
            "Wien-Bridge 28kHz Ultrasonic Air Shelf with Transformer-Balanced Class-A Stage",
            "EQ18",
            "dsp::circuits::GermanBroadcastSilkyEqStage",
            "German broadcast mastering equalizer featuring a 28kHz Wien-bridge air shelf and balanced output iron.",
            gui::Color::fromHex(0x4EA8DE),
            { "Low 60 Hz Shelf Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Presence Frequency", "kHz", 0.0f, 6.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "0.7", "1.0", "1.4", "2.0", "2.8", "4.0", "5.6" }, { 0.7f, 1.0f, 1.4f, 2.0f, 2.8f, 4.0f, 5.6f } },
            {
                { "Presence Bell Gain", "dB", -8.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High 15 kHz Shelf Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "28 kHz Air Trim", "dB", -6.0f, 6.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::SteppedMasteringTransferEq,
            SchematicCategory::EqAndFilters,
            "Stepped Master Transfer EQ",
            "Gold-Contact 0.5dB Bridged-T Rotary Ladder with Hybrid JFET/Bipolar Buffer",
            "EQ19",
            "dsp::circuits::SteppedMasteringTransferEqStage",
            "51-step precision bridged-T resistor ladder mastering equalizer with clinical transient recall.",
            gui::Color::fromHex(0xFFD166),
            { "Low Band Gain", "dB", -8.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Low Band Frequency", "Hz", 0.0f, 7.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
              { "20", "30", "40", "60", "85", "120", "170", "240" }, { 20.0f, 30.0f, 40.0f, 60.0f, 85.0f, 120.0f, 170.0f, 240.0f } },
            {
                { "Low-Mid Band Gain", "dB", -8.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Low-Mid Band Frequency", "Hz", 0.0f, 7.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "100", "140", "200", "280", "400", "560", "800", "1100" }, { 100.0f, 140.0f, 200.0f, 280.0f, 400.0f, 560.0f, 800.0f, 1100.0f } },
                { "Low-Mid Band Q", "", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "0.5", "0.7", "1.0", "1.4", "2.0", "3.0" }, { 0.5f, 0.7f, 1.0f, 1.4f, 2.0f, 3.0f } },
                { "High-Mid Band Gain", "dB", -8.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High-Mid Band Frequency", "kHz", 0.0f, 7.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "0.8", "1.1", "1.6", "2.2", "3.2", "4.5", "6.4", "9.0" }, { 0.8f, 1.1f, 1.6f, 2.2f, 3.2f, 4.5f, 6.4f, 9.0f } },
                { "High-Mid Band Q", "", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "0.5", "0.7", "1.0", "1.4", "2.0", "3.0" }, { 0.5f, 0.7f, 1.0f, 1.4f, 2.0f, 3.0f } },
                { "High Band Gain", "dB", -8.0f, 8.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High Band Frequency", "kHz", 0.0f, 7.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch,
                  { "2.5", "3.6", "5.0", "7.0", "10.0", "14.0", "20.0", "27.0" }, { 2.5f, 3.6f, 5.0f, 7.0f, 10.0f, 14.0f, 20.0f, 27.0f } },
                { "Output Trim", "dB", -5.5f, 5.5f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Low Shelf/Bell", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonLatching, { "SHELF", "BELL" } },
                { "High Shelf/Bell", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonLatching, { "SHELF", "BELL" } }
            }
        },
        {
            SchematicComponentType::EllipticalMidSideEq,
            SchematicCategory::EqAndFilters,
            "Elliptical M/S Master EQ",
            "Sum/Difference Matrix EQ with Sub-120Hz Elliptical Mono Fold & Side Air Lift",
            "EQ20",
            "dsp::circuits::EllipticalMidSideEqStage",
            "Mastering Mid/Side equalizer combining an elliptical sub-bass mono filter with Side-channel air expansion.",
            gui::Color::fromHex(0x9B5DE5),
            { "Bass Mono-Maker Cutoff", "Hz", 20.0f, 350.0f, 83.7f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Mid Presence Bell", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Side Air Shelf", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Stereo Width", "%", 0.0f, 200.0f, 100.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DualInductorPCurveEq,
            SchematicCategory::EqAndFilters,
            "Dual-Inductor Sub & Scoop",
            "Simultaneous Low Shelf Boost with Automatic Complementary Infrasonic Mud Cut",
            "EQ21",
            "dsp::circuits::DualInductorPCurveEqStage",
            "Opposing dual-inductor P-curve shelving network boosting sub-bass weight while scooping 220Hz mud.",
            gui::Color::fromHex(0x00F5D4),
            { "Sub Boost", "dB", 0.0f, 14.0f, 7.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Mud Scoop", "dB", -14.0f, 0.0f, -7.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Corner Freq", "Hz", 30.0f, 320.0f, 95.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Q", "", 0.5f, 10.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Iso226DynamicLoudnessEq,
            SchematicCategory::EqAndFilters,
            "Equal-Loudness Contour EQ",
            "K-Weighted Loudness Tracker Scaling Sub-Bass & 12kHz Air Shelves in Real Time",
            "EQ22",
            "dsp::circuits::Iso226DynamicLoudnessEqStage",
            "Dynamic Fletcher-Munson / ISO-226 loudness contour equalizer maintaining perceived bass and air balance.",
            gui::Color::fromHex(0x3A86FF),
            { "Target Phon Level", "", 50.0f, 90.0f, 70.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Dynamic Tracking", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Sub Contour", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "3.5kHz Guard", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BridgedTwinTNotchEq,
            SchematicCategory::EqAndFilters,
            "Bridged Twin-T Notch Array",
            "Passive RC Null Network with JFET Bootstrap Q for Ringing-Free Resonance Removal",
            "EQ23",
            "dsp::circuits::BridgedTwinTNotchEqStage",
            "Passive bridged Twin-T RC null network removing harsh whistling resonances without active filter ringing.",
            gui::Color::fromHex(0x52B788),
            { "Notch 1 Frequency", "Hz", 20.0f, 2000.0f, 200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Notch 1 Bandwidth Q", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Notch 1 Depth", "dB", -50.0f, 0.0f, -25.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Notch 2 Frequency", "Hz", 200.0f, 20000.0f, 2000.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Notch 2 Bandwidth Q", "", 0.4f, 6.0f, 1.2f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Notch 2 Depth", "dB", -50.0f, 0.0f, -25.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Low-Cut Filter", "Hz", 20.0f, 500.0f, 100.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Filter", "Hz", 2000.0f, 20000.0f, 16000.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Notch 1 In/Out", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "OUT", "IN" } },
                { "Notch 2 In/Out", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "OUT", "IN" } }
            }
        },
        {
            SchematicComponentType::FixedResonant12BandFilter,
            SchematicCategory::EqAndFilters,
            "12-Band Fixed Filterbank",
            "12 Parallel Multiple-Feedback Bandpass Resonators with Odd/Even Stereo Spread",
            "SP7",
            "dsp::circuits::FixedResonant12BandFilterStage",
            "12-band parallel analog resonant filterbank sculpting vocal formants and stereo spectral width.",
            gui::Color::fromHex(0x80ED99),
            { "Spectral Tilt", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Odd/Even Band Offset", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Formant Shift", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Filterbank Q", "", 0.5f, 10.0f, 2.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SpectralBrickwallBinEq,
            SchematicCategory::EqAndFilters,
            "Spectral Brickwall Cut",
            "Ultra-Steep Elliptic-Equivalent Spectral Gate & Harmonic Aliasing Suppressor",
            "SP8",
            "dsp::circuits::SpectralBrickwallBinStage",
            "High-order steep spectral brickwall and per-bin resonance clamp eliminating ultrasonic fizz and rumble.",
            gui::Color::fromHex(0x64DFDF),
            { "HP Brickwall", "Hz", 15.0f, 800.0f, 45.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "LP Brickwall", "Hz", 2500.0f, 22000.0f, 14000.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Gate Floor", "dB", -48.0f, 0.0f, -24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Edge Roll-Off", "dB", 0.0f, 12.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PhaseCompensatedBassMono,
            SchematicCategory::EqAndFilters,
            "Sub Bass Mono-Maker",
            "All-Pass Phase-Aligned Sub-Bass Mono Merger Preventing L+R Cancellation",
            "SP9",
            "dsp::circuits::PhaseCompensatedBassMonoStage",
            "Collapses stereo sub-bass to phase-aligned mono while preserving upper-spectrum width and punch.",
            gui::Color::fromHex(0x4CC9F0),
            { "Mono-Maker Freq", "Hz", 20.0f, 400.0f, 110.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sub Phase", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Sub Punch", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High Stereo Width", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DynamicSpectralDeHarsher,
            SchematicCategory::EqAndFilters,
            "Smooth Glare Tamer",
            "Dual-Band 2.6kHz/5.4kHz Fast Envelope-Tracked Glare & Harshness Suppressor",
            "SP10",
            "dsp::circuits::DynamicSpectralDeHarsherStage",
            "Level-independent differential spectral detector dynamically clamping 2.5kHz-6kHz ear-fatiguing glare.",
            gui::Color::fromHex(0x72EFDD),
            { "Threshold", "dB", -48.0f, 0.0f, -24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "2.6 kHz Glare Cut", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "5.4 kHz Sibilance Cut", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tube Warmth Recovery", "ms", 20.0f, 2000.0f, 200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::RealTimeSpectralDeReverb,
            SchematicCategory::EqAndFilters,
            "Room Tail De-Verb",
            "Inter-Frame Direct-to-Diffuse Coherence Analyzer Suppressing Late Room Tails",
            "SP11",
            "dsp::circuits::SpectralDeReverberatorStage",
            "Separates direct transient onsets from diffuse late room reflections to tighten boxy or reverberant recordings.",
            gui::Color::fromHex(0x06D6A0),
            { "Tail Reduction", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Transient Threshold", "dB", -48.0f, 0.0f, -24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Release", "ms", 20.0f, 2000.0f, 200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Low/High Damping Tilt", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SinusoidalTonalNoiseSeparator,
            SchematicCategory::EqAndFilters,
            "Tonal & Noise Separator",
            "Autocorrelation Partial Tracker Separating Pure Harmonic Notes from Breath/Noise",
            "SP12",
            "dsp::circuits::SinusoidalTonalNoiseStage",
            "Separates periodic harmonic partials from stochastic breath, bow, and cymbal noise for independent sculpting.",
            gui::Color::fromHex(0x56CFE1),
            { "Tonal/Noise Balance", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Coherence Threshold", "dB", -48.0f, 0.0f, -24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Tonal Drive", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Noise Air", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SubSampleCrossCorrPhaseAligner,
            SchematicCategory::EqAndFilters,
            "Auto Phase Aligner",
            "Continuous Fractional Hermite Micro-Delay & Quadrature Phase Solver",
            "SP13",
            "dsp::circuits::SubSamplePhaseAlignerStage",
            "Sub-sample fractional delay and quadrature all-pass phase solver locking multi-mic drums and DI bass in phase.",
            gui::Color::fromHex(0x90E0EF),
            { "Sub-Sample Delay", "ms", -5.0f, 5.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Phase Angle", "", -180.0f, 180.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Track Speed", "Hz", 0.1f, 12.0f, 1.1f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "LF Anchor", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },

        // =====================================================================
        // TAB 4: COMP / CLIP (36 Compressors, Limiters, Gates & Clippers: CP1..29, CL1..7)
        // =====================================================================
        {
            SchematicComponentType::FetCompressor1176,
            SchematicCategory::AnalogDynamics,
            "Fast Feedback FET Limiter",
            "20us Feedback JFET Attenuator with All-Buttons-In Bias Slam",
            "CP1",
            "dsp::circuits::FastFetCompressor",
            "1-sample ZDF feedback FET divider, GR-proportional 2nd/3rd harmonics, All-Buttons bias above +26dB & auto-calibrated output.",
            gui::Color::fromHex(0xFFD166),
            { "Input Attenuator", "dB", -36.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Output Level", "dB", -36.0f, 30.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Attack", "", 1.0f, 7.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "", 1.0f, 7.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Ratio Pushbuttons", "", 0.0f, 4.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonBank, { "4:1", "8:1", "12:1", "20:1", "ALL-BUTTONS-IN" } },
                { "Sidechain HPF", "Hz", 20.0f, 250.0f, 20.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::OptoCompressorT4B,
            SchematicCategory::AnalogDynamics,
            "Program-Dependent T4B Opto",
            "Dual-Stage Memory Electroluminescent CdS Cell for Effortless Vocal Leveling",
            "CP2",
            "dsp::circuits::OpticalLevelerStage",
            "Dual parallel leaky integrators: 60% fast 65ms release + 40% slow 1.6s CdS light-memory tail into warm tube makeup.",
            gui::Color::fromHex(0xF4A261),
            { "Gain", "", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            { "Peak Reduction", "", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Sidechain HF Emphasis Trimmer", "", 0.0f, 100.0f, 80.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mode Switch", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "LIMIT", "COMPRESS" } },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::VariMuTubeCompressor,
            SchematicCategory::AnalogDynamics,
            "Push-Pull Vari-Mu Limiter",
            "Remote-Cutoff Tube Bias Compressor with Continuous Knee & Lat/Vert Mode",
            "CP3",
            "dsp::circuits::VariMuTubeGainCell",
            "Variable transconductance (g_m) tube cell: instantaneous ratio R=1+k*max(0,Env-Thr)^2 scales smoothly from 1.5:1 to 20:1.",
            gui::Color::fromHex(0xFF9F1C),
            { "Left/Lat Input Gain", "dB", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch, { "-20", "-18", "-16", "-14", "-12", "-10", "-8", "-6", "-4", "-2", "0" }, { -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f } },
            { "Right/Vert Input Gain", "dB", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::SteppedRotarySwitch, { "-20", "-18", "-16", "-14", "-12", "-10", "-8", "-6", "-4", "-2", "0" }, { -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f } },
            {
                { "Time Constant Selector", "", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "Pos 1 (0.2ms/300ms)", "Pos 2 (0.2ms/800ms)", "Pos 3 (0.4ms/2s)", "Pos 4 (0.8ms/5s)", "Pos 5 (Auto Program)", "Pos 6 (Dual Auto)" } },
                { "Left/Lat Threshold", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Right/Vert Threshold", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "DC Threshold / Knee Bias", "", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mode Selector", "", 0.0f, 2.0f, 2.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::SteppedRotarySwitch, { "L/R IND", "LAT/VERT M/S", "L/R LINKED" } },
                { "Output Makeup Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::DiodeBridgeCompressor,
            SchematicCategory::AnalogDynamics,
            "Balanced Diode-Bridge Comp",
            "Padded Balanced 4-Diode Bridge with High Harmonic Density & Fast Recovery",
            "CP4",
            "dsp::circuits::DiodeBridgeGainCell",
            "Pads signal down into balanced 4-diode asinh bridge clipper and recovers gain via a discrete Class-A preamp card.",
            gui::Color::fromHex(0xE76F51),
            { "Threshold", "dB", 0.0f, 15.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "-20", "-18", "-16", "-14", "-12", "-10", "-8", "-6", "-4", "-2", "0", "+2", "+4", "+6", "+8", "+10" }, { -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f, -6.0f, -4.0f, -2.0f, 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f } },
            { "Ratio", ":1", 0.0f, 4.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "1.5:1", "2:1", "3:1", "4:1", "6:1" } },
            {
                { "Recovery", "ms", 0.0f, 5.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "100 ms", "400 ms", "800 ms", "1500 ms", "AUTO 1", "AUTO 2" } },
                { "Make-Up Gain", "dB", 0.0f, 10.0f, 2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::SteppedRotarySwitch, { "0", "+2", "+4", "+6", "+8", "+10", "+12", "+14", "+16", "+18", "+20" }, { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f } },
                { "Sidechain HPF", "Hz", 20.0f, 250.0f, 40.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Attack Speed", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "FAST 1.5 ms", "SLOW 5.0 ms" } },
                { "Stereo Link", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "DUAL MONO", "STEREO LINK" } }
            }
        },
        {
            SchematicComponentType::VcaBusCompressor,
            SchematicCategory::AnalogDynamics,
            "Quad-VCA Master Bus Comp",
            "Feed-Forward Solid-State VCA with Dual-Cap Auto-Release & Pink-Noise Thrust",
            "CP5",
            "dsp::circuits::QuadVcaMasterBusGlueStage",
            "Precision feed-forward exponential current-in/current-out VCA bus compressor for mix glue and punchy transients.",
            gui::Color::fromHex(0xE9C46A),
            { "Threshold", "dB", -20.0f, 20.0f, -10.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Make-Up Gain", "dB", 0.0f, 15.0f, 3.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Attack", "ms", 0.0f, 5.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "0.1", "0.3", "1.0", "3.0", "10.0", "30.0" }, { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f } },
                { "Release", "s", 0.0f, 4.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "0.1", "0.3", "0.6", "1.2", "AUTO Dual-RC" }, { 0.1f, 0.3f, 0.6f, 1.2f, 2.0f } },
                { "Ratio", ":1", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "2:1", "4:1", "10:1" }, { 2.0f, 4.0f, 10.0f } },
                { "Thrust Sidechain Filter", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "FLAT", "MED", "LOUD" } },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::SidechainThrustMatrix,
            SchematicCategory::AnalogDynamics,
            "Multi-Ratio Harmonic VCA",
            "Digitally Controlled VCA with 20:1 Brickwall Ratio & 2nd/3rd Harmonic Modes",
            "CP6",
            "dsp::circuits::MultiRatioHarmonicVcaStage",
            "High-headroom VCA gain cell with variable detector HPF, Pink-Noise Thrust tilt, and 2nd/3rd harmonic injection.",
            gui::Color::fromHex(0x90BE6D),
            { "Input Drive", "", 0.0f, 10.0f, 5.5f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Attack", "", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Release", "", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "", 0.0f, 10.0f, 5.5f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Ratio Selector", "", 0.0f, 7.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonBank, { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "20:1", "BRICKWALL" } },
                { "Detector Mode", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "FLAT", "HPF 80Hz", "6kHz EMPHASIS" } },
                { "Audio Mode", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch3Way, { "Clean", "Dist 2 Tube", "Dist 3 Tape" } }
            }
        },
        {
            SchematicComponentType::DualOptoVcaCompressor,
            SchematicCategory::AnalogDynamics,
            "Dual Opto-VCA Master",
            "Serial Optical Macro-Leveler Feeding Fast Discrete VCA Micro-Catcher",
            "CP7",
            "dsp::circuits::DualOptoVcaMasterComp",
            "Slow Optical stage shaves macro-dynamics into fast Class-A VCA catching micro-transients + Nickel/Iron/Steel matrix.",
            gui::Color::fromHex(0xD4AF37),
            { "Optical Threshold", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Optical Gain", "dB", 0.0f, 20.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Discrete VCA Threshold", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Discrete Ratio", ":1", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "1.2:1", "2:1", "3:1", "4:1", "6:1", "10:1" }, { 1.2f, 2.0f, 3.0f, 4.0f, 6.0f, 10.0f } },
                { "Discrete Attack", "ms", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "0.1", "0.5", "1.0", "5.0", "10.0", "30.0" }, { 0.1f, 0.5f, 1.0f, 5.0f, 10.0f, 30.0f } },
                { "Discrete Release", "s", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "0.1", "0.25", "0.5", "0.8", "1.2", "Dual-Auto" }, { 0.1f, 0.25f, 0.5f, 0.8f, 1.2f, 2.0f } },
                { "Discrete Make-Up Gain", "dB", -10.0f, 10.0f, 2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Output Transformer Matrix", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::SteppedRotarySwitch, { "NICKEL", "IRON", "STEEL" } },
                { "Sidechain 90 Hz HPF", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OUT", "IN" } }
            }
        },
        {
            SchematicComponentType::OverEasyRmsSmackVca,
            SchematicCategory::AnalogDynamics,
            "Soft-Knee RMS VCA Comp",
            "Soft-Knee RMS Detector VCA for Explosive Kick, Snare & Bass Thwack",
            "CP8",
            "dsp::circuits::OverEasyRmsVcaStage",
            "True log-RMS detector with parabolic soft knee voiced for explosive kick, snare, and bass bus thwack.",
            gui::Color::fromHex(0xF4A261),
            { "Threshold", "dB", -40.0f, 20.0f, -16.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Compression Ratio", ":1", 1.0f, 20.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Gain", "dB", -20.0f, 20.0f, 4.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Soft Knee", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "HARD KNEE", "SOFT KNEE" } },
                { "Sidechain HPF", "Hz", 20.0f, 250.0f, 40.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::UpDownMultibandCompressor,
            SchematicCategory::AnalogDynamics,
            "Up/Down Multiband Comp",
            "3-Band Aggressive Upward & Downward Compressor Pulling Up Micro-Details",
            "CP9",
            "dsp::circuits::UpDownMultibandCompressor",
            "3-band crossover (180Hz / 3.2kHz) applying simultaneous downward peak compression and upward low-level detail lift.",
            gui::Color::fromHex(0xF3722C),
            { "Depth", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Time Scale", "ms", 5.0f, 400.0f, 45.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Upward Gain", "dB", -15.0f, 15.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Downward Gain", "dB", -15.0f, 15.0f, -2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::TransientShaper,
            SchematicCategory::AnalogDynamics,
            "Differential Transient Shaper",
            "Threshold-Independent Dual-Envelope Shaper for Attack Snap & Room Sustain",
            "CP10",
            "dsp::circuits::DifferentialTransientShaperStage",
            "Level-independent differential envelope detector (dE = Env_fast - Env_slow) sculpting Attack punch & Sustain body.",
            gui::Color::fromHex(0x06D6A0),
            { "Attack", "dB", -15.0f, 15.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sustain", "dB", -24.0f, 24.0f, -3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Output Gain", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Stereo Link", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "DUAL MONO", "STEREO LINK" } },
                { "Detector Speed", "ms", 2.0f, 150.0f, 25.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::LookaheadUpwardExpander,
            SchematicCategory::AnalogDynamics,
            "Upward Crest Lifter",
            "Un-Compresses Squashed Stems by Boosting Peaks Above Threshold",
            "CP11",
            "dsp::circuits::LookaheadUpwardExpanderStage",
            "Lookahead upward dynamic expander restoring crest factor and punch to over-compressed stems and drum buses.",
            gui::Color::fromHex(0x52B788),
            { "Threshold", "dB", -48.0f, 0.0f, -20.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Upward Range", "dB", 0.0f, 12.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Attack", "ms", 0.1f, 100.0f, 3.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 20.0f, 2000.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SplitBandDeEsser,
            SchematicCategory::AnalogDynamics,
            "Split-Band Sibilance Comp",
            "Level-Independent High-Frequency Comparator Clamping 5k-12kHz Vocal Sibilance",
            "CP12",
            "dsp::circuits::SplitBandDeEsser",
            "Zero-latency high-band dynamic sibilance extractor (3.2kHz-11kHz) taming harsh vocal Esses and cymbal glare.",
            gui::Color::fromHex(0xF8961E),
            { "Frequency", "Hz", 800.0f, 10000.0f, 4200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Range", "dB", 0.0f, 20.0f, 10.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Detector Release", "ms", 10.0f, 250.0f, 45.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Split/Wideband Mode", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "WIDEBAND", "SPLIT HF" } },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::HysteresisNoiseGate,
            SchematicCategory::AnalogDynamics,
            "Dual-Threshold Noise Gate",
            "Chatter-Free Noise Gate with Independent Open/Close Thresholds & Filter Key",
            "CP13",
            "dsp::circuits::HysteresisNoiseGate",
            "Dual-threshold Schmitt-trigger noise gate & downward expander with 4.5dB hysteresis window for chatter-free gating.",
            gui::Color::fromHex(0xF9844A),
            { "Threshold", "dB", -60.0f, 10.0f, -22.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Attack", "ms", 0.1f, 100.0f, 1.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Hold/Release", "ms", 20.0f, 2000.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Range", "dB", -80.0f, 0.0f, -36.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Key Filter", "Hz", 80.0f, 6000.0f, 600.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BroadcastTriodeLimiter,
            SchematicCategory::AnalogDynamics,
            "Balanced Broadcast Tube Limiter",
            "Balanced 6V6/6386 Tube Broadcast Limiter with Single/Double/Triple Recovery Law",
            "CP14",
            "dsp::circuits::BroadcastTriodeLimiterStage",
            "Multi-time-constant balanced push-pull tube broadcast limiter with Single, Double, and Triple recovery laws.",
            gui::Color::fromHex(0x4CC9F0),
            { "Input Attenuator", "dB", 0.0f, 40.0f, 18.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Output Attenuator", "dB", -24.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Recovery Mode Switch", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "SINGLE", "DOUBLE", "TRIPLE" } },
                { "Mode", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "COMP", "LIMIT" } },
                { "Cathode Bias Bal", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Sidechain HPF", "Hz", 20.0f, 200.0f, 45.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ZenerShuntLimiter,
            SchematicCategory::AnalogDynamics,
            "Matched Zener Shunt Limiter",
            "Matched Zener-Diode Shunt Console Limiter with 85Hz THD Compensation",
            "CP15",
            "dsp::circuits::ZenerShuntLimiter",
            "2x oversampled reverse-breakdown Zener diode shunt limiter with 85Hz bass THD compensation for explosive drum & bus glue.",
            gui::Color::fromHex(0xFFBA08),
            { "Input Drive / Hold", "dB", -12.0f, 24.0f, 8.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Recovery", "ms", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "50 ms", "100 ms", "250 ms", "500 ms", "1000 ms", "2000 ms" }, { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f } },
            {
                { "Make-Up / Output Level", "dB", -18.0f, 18.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Mode Selector", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "COMP 2:1", "LIMIT ZENER" } },
                { "Sidechain 90 Hz HPF", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ToggleSwitch2Way, { "OUT", "IN" } },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::OversampledVariableClipper,
            SchematicCategory::AnalogDynamics,
            "Standard 16x Clipper",
            "Continuous Hard-to-Soft Polynomial Wave-Shaper with Linear/Min-Phase Anti-Aliasing",
            "CL1",
            "dsp::circuits::VariableKneePolyClipper",
            "4x/16x polyphase Lp-norm clipper y = sign(x)*T*(1+(|x|/T)^-p)^(-1/p) morphing from Soft Sigmoid to Razor Hard Clip.",
            gui::Color::fromHex(0xFF0055),
            { "Input Drive", "dB", -12.0f, 24.0f, 0.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Ceiling", "dB", -24.0f, 0.0f, -1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Knee Curve", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "HF Emphasis Tilt", "Hz", 500.0f, 16000.0f, 3200.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PsychoacousticMasteringLimiter,
            SchematicCategory::AnalogDynamics,
            "26-Band Bark Limiter",
            "Critical-Band Linear-Phase Mastering Limiter Preserving Transient Punch",
            "CL2",
            "dsp::circuits::PsychoacousticMasterLimiterStage",
            "Critical-band psychoacoustic mastering limiter distributing peak attenuation across Bark bands to preserve transient punch.",
            gui::Color::fromHex(0xFF4D6D),
            { "Input Gain", "dB", 0.0f, 20.0f, 6.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::VerticalFader, {} },
            { "Ceiling", "dB", -18.0f, 0.0f, -3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Release Speed", "ms", 5.0f, 450.0f, 65.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Transient Emphasis", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Spectral Balance", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BrickwallLimiter,
            SchematicCategory::AnalogDynamics,
            "True-Peak Brickwall",
            "Inter-Sample Peak (ISP) Compliant Mastering Ceiling Limiter",
            "CL3",
            "dsp::circuits::TruePeakLookaheadBrickwallStage",
            "2ms lookahead true-peak mastering brickwall ceiling limiter with exponential release recovery.",
            gui::Color::fromHex(0xEF476F),
            { "Gain / Threshold", "dB", -15.0f, 15.0f, 6.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::VerticalFader, {} },
            { "Out Ceiling", "dB", -18.0f, 0.0f, -3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Lookahead", "ms", 0.2f, 20.0f, 2.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 15.0f, 1000.0f, 90.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "True-Peak ISP Mode", "", 0.0f, 1.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "SAMPLE PEAK", "TRUE PEAK 4x" } }
            }
        },
        {
            SchematicComponentType::TransconductanceRoomSlammer,
            SchematicCategory::AnalogDynamics,
            "OTA Transconductance Slammer",
            "CA3080 Operational Transconductance Limiter for Explosive Drum Room Pumping",
            "CP16",
            "dsp::circuits::TransconductanceRoomSlammerStage",
            "Vintage broadcast operational transconductance limiter delivering explosive room-mic smash and gritty harmonic punch.",
            gui::Color::fromHex(0xFF9F1C),
            { "Crush", "", 0.0f, 10.0f, 5.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Crunch", "", 0.0f, 10.0f, 4.5f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Darkness", "", 0.0f, 10.0f, 4.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Release Speed", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch2Way, { "FAST", "SLOW" } },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::BroadcastFieldEffectCrusher,
            SchematicCategory::AnalogDynamics,
            "Shunt-JFET Iron Crusher",
            "Depletion-Mode JFET Shunt Attenuator with Steel PA Transformer Bite",
            "CP17",
            "dsp::circuits::BroadcastFieldEffectCrusherStage",
            "Depletion-mode JFET shunt limiter with 120Hz-6kHz broadcast presence contour and steel transformer grit.",
            gui::Color::fromHex(0xFCA311),
            { "Input/Threshold", "dB", -48.0f, 0.0f, -22.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Ratio", ":1", 2.0f, 20.0f, 10.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Steel Iron Saturation", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Level", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Release Time", "ms", 25.0f, 1200.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Voice Contour", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "FLAT", "VOICE PRESENCE" }, {}, true }
            }
        },
        {
            SchematicComponentType::RmsVcaPeakStopLimiter,
            SchematicCategory::AnalogDynamics,
            "RMS VCA & Diode Peak Limiter",
            "Feed-Forward Log-RMS VCA Leveler Cascaded into Silicon-Diode Peak-Clamp Ceiling",
            "CP18",
            "dsp::circuits::RmsVcaPeakStopLimiterStage",
            "Two-tier dynamics processor pairing smooth log-RMS VCA body compression with a hard-knee silicon-diode Peak-Clamp ceiling.",
            gui::Color::fromHex(0xF4A261),
            { "Threshold", "dB", -48.0f, 0.0f, -20.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Ratio", ":1", 1.5f, 20.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Attack", "ms", 0.5f, 120.0f, 12.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 30.0f, 1500.0f, 220.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Diode Peak Ceiling", "dB", -18.0f, 0.0f, -4.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Gain", "dB", -15.0f, 15.0f, 2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DiscreteMasteringMsComp,
            SchematicCategory::AnalogDynamics,
            "Crest-Factor M/S Bus Comp",
            "Sum/Difference Matrix VCA Compressor with Crest-Factor Auto-Release",
            "CP19",
            "dsp::circuits::DiscreteMasteringMsCompStage",
            "Discrete Class-A Mid/Side mastering compressor tightening center punch while lifting wide stereo sustain.",
            gui::Color::fromHex(0xFFD166),
            { "Mid Threshold", "dB", -48.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Side Threshold", "dB", -48.0f, 0.0f, -20.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Ratio", ":1", 1.2f, 10.0f, 2.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Attack", "ms", 0.5f, 120.0f, 15.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 25.0f, 1800.0f, 250.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Make-Up Gain", "dB", -15.0f, 15.0f, 2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BritishPhotoOpticalComp,
            SchematicCategory::AnalogDynamics,
            "Photo-Cell & Steel-Iron Opto",
            "Filament Lamp & CdSe Photo-Cell Leveler with British Steel Output Iron",
            "CP20",
            "dsp::circuits::BritishPhotoOpticalCompStage",
            "Warm incandescent lamp and CdSe photoresistor compressor with natural attack lag and British steel transformer weight.",
            gui::Color::fromHex(0xE76F51),
            { "Input Compression Drive", "dB", -12.0f, 24.0f, 8.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Slope/Ratio", ":1", 1.5f, 20.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Lamp Attack", "ms", 0.5f, 100.0f, 8.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "CdSe Dark Release", "ms", 30.0f, 2000.0f, 280.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Steel Iron Harmonics", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Make-Up Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DualSlopeBusVcaComp,
            SchematicCategory::AnalogDynamics,
            "Dual-Slope Harmonic Bus VCA",
            "Twin VCA Cells with Fast/Slow Compound Release & Odd-Harmonic Saturation",
            "CP21",
            "dsp::circuits::DualSlopeBusVcaStage",
            "Stereo bus VCA compressor riding a fast 45ms transient recovery atop a 650ms macro-glue platform.",
            gui::Color::fromHex(0xE9C46A),
            { "Threshold", "dB", -48.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Attack", "ms", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "0.1 ms", "0.3 ms", "1.0 ms", "3.0 ms", "10 ms", "30 ms" }, { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f } },
            {
                { "Release", "ms", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "50 ms", "150 ms", "300 ms", "600 ms", "1200 ms", "Compound Auto" }, { 50.0f, 150.0f, 300.0f, 600.0f, 1200.0f, 450.0f } },
                { "Ratio", ":1", 1.5f, 10.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Crush Saturation", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Make-Up Gain", "dB", -15.0f, 15.0f, 2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SolidStateT4BLeveler,
            SchematicCategory::AnalogDynamics,
            "Solid-State T4B Autoformer",
            "Electroluminescent T4B Cell into Discrete Class-AB Solid-State & Autoformer",
            "CP22",
            "dsp::circuits::SolidStateT4BLevelerStage",
            "Pairs an electroluminescent T4B optical attenuator with a punchy solid-state output stage and tapped autoformer.",
            gui::Color::fromHex(0x90BE6D),
            { "Gain", "", 0.0f, 10.0f, 5.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            { "Peak Reduction", "", 0.0f, 10.0f, 5.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "HF Contour", "Hz", 600.0f, 10000.0f, 2500.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "COMP", "LIMIT" } },
                { "Autoformer Saturation", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ParametricDynamicEq4Band,
            SchematicCategory::AutoAndSmart,
            "4-Band Dynamic Multiband EQ",
            "Low, Low-Mid, High-Mid & Air Static/Dynamic Equalizer Clamping Threshold Buildup",
            "AU3",
            "dsp::circuits::ParametricDynamicEq4BandStage",
            "Gives Low, Low-Mid, High-Mid, and Air bands independent static gain and dynamic range to clamp buildup only when energy crosses threshold.",
            gui::Color::fromHex(0x00BBF9),
            { "Low Dynamic Range", "dB", -15.0f, 12.0f, -4.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Low-Mid Dynamic Range", "dB", -15.0f, 12.0f, -5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High-Mid Dynamic Range", "dB", -15.0f, 12.0f, -5.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Air Dynamic Range", "dB", -15.0f, 12.0f, -4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Dynamic Threshold", "dB", -48.0f, 0.0f, -20.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Static Tilt Balance", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Band Selectivity Q", "", 0.5f, 8.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::PhaseSafeDrumBleedGate,
            SchematicCategory::AnalogDynamics,
            "Drum Bleed Gate",
            "Resonant Key Detector Attenuating Hi-Hat/Cymbal Bleed While Preserving Low Ring",
            "CP24",
            "dsp::circuits::PhaseSafeDrumBleedGateStage",
            "Phase-coherent high-shelf drum gate suppressing cymbal bleed between snare/tom hits without choking fundamental sustain.",
            gui::Color::fromHex(0x52B788),
            { "Hit Threshold", "dB", -48.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Cymbal Bleed Cut", "dB", -24.0f, 0.0f, -12.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Decay Time", "ms", 20.0f, 2000.0f, 160.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Bleed Cutoff Freq", "Hz", 400.0f, 8000.0f, 1800.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Low Ring Floor", "dB", -48.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ZeroDistortionVocalRider,
            SchematicCategory::AutoAndSmart,
            "Auto Vocal Leveller",
            "180 Hz-4.2 kHz Speech-Weighted Detector & Breath-Guarded Mass-Damped Leveller",
            "AU1",
            "dsp::circuits::ZeroDistortionVocalRiderStage",
            "Uses a 180 Hz-4.2 kHz speech-weighted detector and breath/sibilance guard to level a mass-damped fader (+/-12 dB) with zero compression distortion.",
            gui::Color::fromHex(0x06D6A0),
            { "Target Vocal Level", "dB", -36.0f, 0.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "Leveller Range", "dB", 0.0f, 12.0f, 8.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Breath Gate Sensitivity", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Fader Speed", "ms", 20.0f, 1200.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} }
            }
        },
        {
            SchematicComponentType::Broadcast5BandFmLimiter,
            SchematicCategory::AnalogDynamics,
            "5-Band Broadcast FM Limiter",
            "All-Pass Phase Rotator + Slow AGC + 5-Band Limiter + 75us Pre-Emphasis Clipper",
            "CP26",
            "dsp::circuits::Broadcast5BandFmLimiterStage",
            "Complete FM broadcast transmission processor with waveform symmetrizer, 5-band limiter, and 75us HF clipper.",
            gui::Color::fromHex(0xF72585),
            { "AGC Drive", "dB", -6.0f, 18.0f, 6.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "5-Band Limiter Density", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "75us HF Clipper", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "100Hz Bass Punch", "dB", 0.0f, 12.0f, 4.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Ceiling", "dB", -18.0f, 0.0f, -2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SuperTube6H30PVariMu,
            SchematicCategory::AnalogDynamics,
            "Super-Tube Vari-Mu",
            "High-Transconductance Push-Pull 6H30P Supertriode with Fast Zener Attack Clamp",
            "CP27",
            "dsp::circuits::SuperTube6H30PVariMuStage",
            "Modern mastering Vari-Mu compressor using high-current 6H30P twin-triodes and a Zener-accelerated sidechain.",
            gui::Color::fromHex(0xFFB703),
            { "Input Attenuator", "dB", -18.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Threshold", "dB", -36.0f, 0.0f, -16.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Attack", "ms", 0.5f, 100.0f, 8.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Recovery", "ms", 0.0f, 4.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "100 ms", "250 ms", "500 ms", "1200 ms", "2500 ms" }, { 100.0f, 250.0f, 500.0f, 1200.0f, 2500.0f } },
                { "Output Level", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Comp/Limit Toggle", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "COMP 1.5:1", "ZENER LIMIT" } }
            }
        },
        {
            SchematicComponentType::LinearPhaseUpwardLifter,
            SchematicCategory::AnalogDynamics,
            "Low-Level Detail Lifter",
            "Zero-Distortion Upward Compressor Lifting Low-Level Detail (-60 to -24 dBFS)",
            "CP28",
            "dsp::circuits::LinearPhaseUpwardLifterStage",
            "Upward micro-dynamics lifter raising quiet room air and low-level detail while leaving loud peaks untouched.",
            gui::Color::fromHex(0x4CC9F0),
            { "Floor Threshold", "dB", -60.0f, -6.0f, -24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Upward Lift", "dB", 0.0f, 12.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Spectral Tilt", "dB", -6.0f, 6.0f, 1.5f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 20.0f, 2000.0f, 220.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::CrestFactorDensityController,
            SchematicCategory::AnalogDynamics,
            "Crest Density Comp",
            "Triggers Gain Reduction Strictly on Peak-to-RMS Decibel Differential",
            "CP29",
            "dsp::circuits::CrestFactorDensityCompStage",
            "Level-independent crest-factor compressor taming spiky transients identically at any volume while ignoring sustained pads.",
            gui::Color::fromHex(0x9B5DE5),
            { "Target Crest Factor", "dB", 2.0f, 18.0f, 8.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Transient Preserve", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "RMS Window", "ms", 5.0f, 750.0f, 60.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 20.0f, 2000.0f, 200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::MasteringAdcOverdrive,
            SchematicCategory::AnalogDynamics,
            "Gold ADC Soft Clipper",
            "High-Current Differential ADC Analog Driver Soft-Clip & Sigma-Delta Clamp",
            "CL4",
            "dsp::circuits::MasteringAdcOverdriveStage",
            "Models the sought-after analog front-end soft-clipping of reference mastering A/D converters driven +3dB hot.",
            gui::Color::fromHex(0xFF0055),
            { "ADC Input Push", "dB", -6.0f, 18.0f, 6.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Soft Sat Knee", "dB", -9.0f, -0.5f, -3.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Iron Weight", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Ceiling", "dB", -18.0f, 0.0f, -2.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Anti-Alias Filter", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "MIN PHASE", "STEEP BRICKWALL" }, {}, true }
            }
        },
        {
            SchematicComponentType::SplitBandMasteringClipper,
            SchematicCategory::AnalogDynamics,
            "Split Sub/Top Clipper",
            "160Hz Phase-Compensated Crossover Isolating Sub Waveshaping from HF Clip",
            "CL5",
            "dsp::circuits::SplitBandMasteringClipperStage",
            "Splits sub-bass (<160Hz) and upper transients into separate polynomial clippers so heavy kicks never crackle vocals.",
            gui::Color::fromHex(0xFF4D6D),
            { "Sub/Top Crossover", "Hz", 60.0f, 400.0f, 160.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Sub Drive", "dB", -6.0f, 18.0f, 5.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Top Transient Drive", "dB", -6.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Knee Softness", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Ceiling", "dB", -18.0f, 0.0f, -3.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::HilbertWaveformSymmetrizer,
            SchematicCategory::AnalogDynamics,
            "90-Deg Phase Clipper",
            "6-Pole 90-Deg All-Pass Phase Rotator Aligning Asymmetric Peaks Before Clipping",
            "CL6",
            "dsp::circuits::HilbertWaveformSymmetrizerStage",
            "Rotates phase by 90 degrees via a 6-pole Hilbert network to symmetrize lopsided brass/vocal peaks for +3dB clean headroom.",
            gui::Color::fromHex(0xF15BB5),
            { "Rotator Stages", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "All-Pass Corner Freq", "Hz", 80.0f, 1200.0f, 240.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Post-Rotator Clipper Drive", "dB", -6.0f, 18.0f, 6.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Ceiling", "dB", -18.0f, 0.0f, -3.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Adaa2ndOrderClipper,
            SchematicCategory::AnalogDynamics,
            "ADAA Smooth Clipper",
            "Continuous-Time Antiderivative Analytic Waveshaper for Alias-Free Clipping",
            "CL7",
            "dsp::circuits::Adaa2ndOrderClipperStage",
            "Evaluates the 2nd-order continuous-time antiderivative F2(x) between samples to suppress harmonic aliasing without FIR pre-ring.",
            gui::Color::fromHex(0xEF476F),
            { "Input Drive", "dB", -6.0f, 24.0f, 7.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Curve Morph", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Asymmetry", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Ceiling", "dB", -18.0f, 0.0f, -3.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 100.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },

        // =====================================================================
        // TAB 5: TIME / VERB (20 Delays, Pitch/Grain & Reverbs: DL1..10, SH1, RV1..9)
        // =====================================================================
        {
            SchematicComponentType::TapeEcho,
            SchematicCategory::DelayAndReverb,
            "Multi-Head Magnetic Tape Echo",
            "Free-Running Multi-Head Magnetic Tape Echo (150Hz-3.5kHz)",
            "DL1",
            "dsp::circuits::TapeEchoStage",
            "Hermite fractional tape delay with capstan wow/flutter LFO, 150Hz head bump & 3.5kHz 6dB/oct tape loss per repeat.",
            gui::Color::fromHex(0x9B5DE5),
            { "Mode Selector", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "1-4 Echo+Verb", "5-11 Echo Only", "12 Reverb Only" }, { 0.0f, 1.0f, 2.0f } },
            { "Repeat Rate", "ms", 40.0f, 800.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Intensity", "%", 0.0f, 110.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Echo Volume", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Reverb Volume", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Bass", "dB", -10.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Treble", "dB", -10.0f, 10.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mic/Inst Input Volume", "dB", -12.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PingPongDelay,
            SchematicCategory::DelayAndReverb,
            "Cross-Fed Ping-Pong Delay",
            "Cross-Fed Stereo Ping-Pong Delay with Analog Damping",
            "DL2",
            "dsp::circuits::PingPongStereoDelayStage",
            "Dual circular delay lines with stereo crossfeed ping-pong matrix and warm high-cut damping.",
            gui::Color::fromHex(0x8338EC),
            { "Delay Time", "ms", 10.0f, 2000.0f, 375.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Feedback", "%", 0.0f, 95.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Ping-Pong Width", "%", 0.0f, 100.0f, 85.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Damping", "Hz", 800.0f, 16000.0f, 4800.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BbdCompanderDelay,
            SchematicCategory::DelayAndReverb,
            "Companded 4096-Stage BBD Echo",
            "RMS Companded 4096-Stage Bucket-Brigade Analog Delay",
            "DL3",
            "dsp::circuits::CompanderBbdDelay",
            "2:1 RMS input compressor -> 4096-stage BBD with clock-linked 4th-order LPF (f_c ~ f_clk) -> 1:2 RMS output expander.",
            gui::Color::fromHex(0xB5179E),
            { "Delay", "ms", 20.0f, 550.0f, 240.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Feedback", "%", 0.0f, 105.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Chorus/Vibrato Mod Depth", "%", 0.0f, 100.0f, 30.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Compander Drive", "dB", -12.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Blend", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::MicroPitchHarmonizer,
            SchematicCategory::DelayAndReverb,
            "Dual-Voice MicroPitch Shift",
            "Deglitched 16-Bit PCM Dual-Voice Pitch Harmonizer",
            "DL4",
            "dsp::circuits::DualMicroPitchStage",
            "16-bit converter quantization + raised-cosine deglitched pitch shifter (+cents/12ms Left, -cents/24ms Right).",
            gui::Color::fromHex(0x4CC9F0),
            { "Pitch Left", "ct", -50.0f, 50.0f, -9.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pitch Right", "ct", -50.0f, 50.0f, 9.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Delay Slap", "ms", 2.0f, 250.0f, 18.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Feedback", "%", 0.0f, 85.0f, 15.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::MagneticDrumEcho,
            SchematicCategory::DelayAndReverb,
            "Spinning Magnetic Drum Echo",
            "Spinning Steel Magnetic Drum Multi-Tap Delay with Valve Preamp",
            "DL5",
            "dsp::circuits::MagneticDrumEchoStage",
            "Spinning steel magnetic drum multi-head delay with zero tape-stretch wobble, rhythmic swell taps, and 12AX7 valve push.",
            gui::Color::fromHex(0xF72585),
            { "Selector", "", 0.0f, 3.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "Head 1 (75ms)", "Head 2 (150ms)", "Head 3 (225ms)", "Head 4 + Multi (310ms)" }, { 0.0f, 1.0f, 2.0f, 3.0f } },
            { "Function Switch", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::SteppedRotarySwitch, { "ECHO Single Repeat", "RIP Multi-Repeat", "SWELL All-Head Reverb-Echo" }, { 0.0f, 1.0f, 2.0f } },
            {
                { "Input Level", "dB", -12.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Length of Swell", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tone", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Dry / Echo Balance", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Drum Capstan Speed", "%", -20.0f, 20.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::Companded12BitRackDelay,
            SchematicCategory::DelayAndReverb,
            "12-Bit Companded Rack Delay",
            "12-Bit Floating-Point Gain-Ranging Delay with Clock Pitch-Bending",
            "DL6",
            "dsp::circuits::Companded12BitRackDelayStage",
            "Early digital 12-bit floating-point gain-ranging rack delay with JFET converter grit and sample-clock pitch-bending.",
            gui::Color::fromHex(0x4EA8DE),
            { "Delay Time", "ms", 15.0f, 1600.0f, 320.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Clock Bandwidth", "x", 0.5f, 2.0f, 1.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Feedback", "%", 0.0f, 98.0f, 42.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "VCO Sweep Depth/Rate", "Hz", 0.1f, 12.0f, 1.4f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DynamicDuckingDelay,
            SchematicCategory::DelayAndReverb,
            "Envelope-Ducked Digital Delay",
            "Envelope-Suppressed Digital Delay with Phase-Reversed Stereo Width",
            "DL7",
            "dsp::circuits::DynamicDuckingDelayStage",
            "Envelope-suppressed digital delay that ducks repeats while the dry note plays and swells into phrase gaps.",
            gui::Color::fromHex(0x00BBF9),
            { "Delay Time", "ms", 10.0f, 1500.0f, 380.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Feedback", "%", 0.0f, 95.0f, 42.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Ducking Threshold", "dB", -48.0f, 0.0f, -22.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Ducking Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release", "ms", 20.0f, 2000.0f, 240.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 38.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Rhythmic8TapDiffuser,
            SchematicCategory::DelayAndReverb,
            "8-Tap All-Pass Diffuser",
            "8-Voice All-Pass Smearing Multi-Tap Array with Taper & Chop Envelope",
            "DL8",
            "dsp::circuits::Rhythmic8TapDiffuserStage",
            "8-voice rhythmic multi-tap delay array with all-pass diffusion smearing and taper envelope turning hits into gated blooms.",
            gui::Color::fromHex(0x9B5DE5),
            { "Cluster Span", "ms", 12.0f, 600.0f, 165.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Tap Taper", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Diffusion Smear", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Feedback", "%", 0.0f, 92.0f, 35.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::AsynchronousGrainCloud,
            SchematicCategory::DelayAndReverb,
            "Granular Cloud Delay",
            "Windowed Circular-Buffer Granular Slicer with Reverse & Octave Scatter",
            "DL9",
            "dsp::circuits::AsynchronousGrainCloudStage",
            "Windowed circular-buffer granular delay cloud with reverse grain splicing, pitch scatter, and ambient texture freeze.",
            gui::Color::fromHex(0xC77DFF),
            { "Delay Position", "ms", 15.0f, 1200.0f, 220.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Grain Size", "ms", 8.0f, 350.0f, 85.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pitch Shift", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Cloud Density", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::StudioAdtTapeDoubler,
            SchematicCategory::DelayAndReverb,
            "Varispeed Capstan Tape ADT",
            "Short-Gap Artificial Double Tracker (15ms-65ms) with Organic Capstan Wander",
            "DL10",
            "dsp::circuits::VarispeedAdtDoublerStage",
            "Dual-deck varispeed tape phase/timing modulator (15ms-65ms) tuned for mono-compatible vocal and bus thickening.",
            gui::Color::fromHex(0xF15BB5),
            { "ADT Delay Offset", "ms", -20.0f, 65.0f, 24.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Varispeed Wobble Depth", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Wobble Rate", "Hz", 0.1f, 12.0f, 1.2f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Tape Head Bump", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::OctaveShimmerReverb,
            SchematicCategory::DelayAndReverb,
            "Halo Octave Shimmer",
            "+12 Semitone Pitch-Regenerated Hadamard Shimmer Tank",
            "SH1",
            "dsp::circuits::OctaveShimmerFdnTank",
            "Dual-voice +12 semitone grain pitch shifter embedded inside a 4x4 Hadamard FDN feedback loop for angelic octave bloom.",
            gui::Color::fromHex(0xC77DFF),
            { "Decay Time", "s", 0.5f, 25.0f, 4.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Shimmer Octave Regen", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Diffusion Size", "%", 0.0f, 100.0f, 70.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Damping", "Hz", 800.0f, 16000.0f, 5200.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PlateReverb,
            SchematicCategory::DelayAndReverb,
            "Modulated Gold-Foil Plate",
            "Modulated Figure-8 Gold-Foil Plate Reverb Tank",
            "RV1",
            "dsp::circuits::StudioReverbFDN",
            "Quadrature LFO 'Spin & Wander' modulated delay taps inside a 4x4 Hadamard figure-8 plate tank with HF damping.",
            gui::Color::fromHex(0x7209B7),
            { "Damper Decay Time", "s", 0.5f, 5.5f, 2.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pre-Delay", "ms", 0.0f, 200.0f, 24.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Input HPF", "Hz", 40.0f, 400.0f, 90.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "HF Damping", "Hz", 1200.0f, 15000.0f, 6200.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 32.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::HallReverbFDN,
            SchematicCategory::DelayAndReverb,
            "Spin & Wander Concert Hall",
            "Spin & Wander Modulated Figure-8 Concert Hall Matrix",
            "RV2",
            "dsp::circuits::ConcertHallFdnStage",
            "Lush modulated figure-8 hall acoustic network with deep low-mid sustain and adjustable air absorption.",
            gui::Color::fromHex(0x560BAD),
            { "Mid RT60 Decay", "s", 0.4f, 18.0f, 2.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "Bass Multiply", "x", 0.4f, 2.5f, 1.25f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Hall Size", "%", 0.0f, 100.0f, 68.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
                { "Pre-Delay", "ms", 0.0f, 250.0f, 28.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::VerticalFader, {} },
                { "Diffusion", "%", 0.0f, 100.0f, 75.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 30.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DispersionSpringReverb,
            SchematicCategory::DelayAndReverb,
            "Triple-Helix Studio Spring",
            "Chirp-Dispersive Triple-Helix Mechanical Spring Tank",
            "RV3",
            "dsp::circuits::DispersionSpringReverb",
            "10 high-dispersion all-pass stages inside a 4-point Hermite resonant metallic delay tank recreating authentic helical spring splash.",
            gui::Color::fromHex(0x9D4EDD),
            { "Dwell Drive", "dB", -18.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Decay Time", "s", 1.2f, 4.5f, 2.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Spring Chirp Tension", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Tone", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::NonlinearGatedReverb,
            SchematicCategory::DelayAndReverb,
            "Non-Linear Gated Drum Room",
            "Dense Early-Reflection Studio Room + RMS Hold Gate",
            "RV4",
            "dsp::circuits::NonlinearGatedReverb",
            "Dense multi-tap early-reflection room cluster with an RMS hold-and-chop gate for explosive snare and drum room punch.",
            gui::Color::fromHex(0x7B2CBF),
            { "Gate Hold Time", "ms", 20.0f, 450.0f, 140.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Gate Release Slope", "ms", 10.0f, 350.0f, 65.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pre-Delay", "ms", 0.0f, 150.0f, 15.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "EQ Tilt", "dB", -12.0f, 12.0f, 2.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DispersiveSteelPlate,
            SchematicCategory::DelayAndReverb,
            "Cold-Rolled Steel Plate",
            "High-Tension Cold-Rolled Steel Sheet Tank with Fast High-Frequency Whip",
            "RV5",
            "dsp::circuits::DispersiveSteelPlateStage",
            "600-lb cold-rolled steel sheet plate tank with explosive high-frequency dispersion whip for snares and lead vocals.",
            gui::Color::fromHex(0x4CC9F0),
            { "Damper Decay", "s", 0.5f, 5.5f, 2.1f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Steel Whip Dispersion", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pre-Delay", "ms", 0.0f, 200.0f, 18.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Damping", "Hz", 800.0f, 16000.0f, 6800.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 32.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::TiledLiveDrumChamber,
            SchematicCategory::DelayAndReverb,
            "Plaster & Tile Echo Chamber",
            "High-Diffusion Plaster & Ceramic Acoustic Echo Chamber with Mic Distance",
            "RV6",
            "dsp::circuits::TiledLiveDrumChamberStage",
            "High-diffusion plaster and ceramic tile studio echo chamber delivering midrange bark and 3D drum depth.",
            gui::Color::fromHex(0x9B5DE5),
            { "Mic Distance", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Chamber Decay", "s", 0.4f, 4.8f, 1.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pre-Delay", "ms", 0.0f, 180.0f, 22.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Tile HF Absorption", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 30.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::EarlyReflectionAmbience,
            SchematicCategory::DelayAndReverb,
            "Early-Reflection Room Matrix",
            "Tail-Free Early-Reflection Geometry Engine for Dry Studio Depth",
            "RV7",
            "dsp::circuits::EarlyReflectionAmbienceStage",
            "Tail-free early-reflection acoustic geometry cluster placing dry close-miked drums and DI tracks into 3D space.",
            gui::Color::fromHex(0x00F5D4),
            { "Room Size", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pre-Delay", "ms", 0.0f, 120.0f, 14.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Wall High-Cut", "Hz", 1000.0f, 16000.0f, 5800.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Reflection Density", "%", 0.0f, 100.0f, 70.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 28.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::InfiniteVelvetBlackhole,
            SchematicCategory::DelayAndReverb,
            "Event Horizon Tank",
            "Super-Massive Modulated All-Pass Cascade with Gravity Feedback Freeze",
            "RV8",
            "dsp::circuits::InfiniteAbyssalGravityStage",
            "Super-massive modulated all-pass cosmos tank with gravity feedback freeze for cinematic ambient swells.",
            gui::Color::fromHex(0x7209B7),
            { "Gravity", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Size", "%", 0.0f, 100.0f, 75.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pre-Delay", "ms", 0.0f, 400.0f, 35.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Mod Depth/Rate", "Hz", 0.1f, 12.0f, 0.85f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 42.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ZeroLatencyIrConvolver,
            SchematicCategory::DelayAndReverb,
            "Studio IR Convolver",
            "Partitioned Short/Long FIR Impulse Response Engine with Length Stretch",
            "RV9",
            "dsp::circuits::ZeroLatencyIrConvolverStage",
            "Zero-latency partitioned impulse response convolver modeling real acoustic room captures and hardware reverb tanks.",
            gui::Color::fromHex(0x4361EE),
            { "IR Length Stretch", "%", 25.0f, 200.0f, 100.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Room Size Transpose", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pre-Delay", "ms", 0.0f, 250.0f, 18.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Filter", "Hz", 800.0f, 16000.0f, 6500.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 32.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::GoldenRatioDualRackDelay,
            SchematicCategory::DelayAndReverb,
            "Gold Rack Delay",
            "1 : 1.618034 Irrational Left/Right Tap Spacing Eliminating Comb Flutter",
            "DL11",
            "dsp::circuits::GoldenRatioDualDelayStage",
            "Dual 20-bit rack delay lines locked at the 1.618034 golden ratio so stereo repeats never collide into metallic flutter.",
            gui::Color::fromHex(0x9B5DE5),
            { "Delay 1 Time", "ms", 15.0f, 1200.0f, 280.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Subdivision Ratio", ":1", 1.0f, 2.618f, 1.618f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Feedback", "%", 0.0f, 95.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Voicing", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Modulated16BitRackDelay,
            SchematicCategory::DelayAndReverb,
            "16-Bit VCO Modulated Delay",
            "VCO Sample-Clock Modulated 16-Bit PCM Delay with JFET Input Push",
            "DL12",
            "dsp::circuits::Modulated16BitRackDelayStage",
            "1983 studio rack 16-bit linear PCM delay with VCO clock modulation and punchy JFET converter saturation.",
            gui::Color::fromHex(0x4EA8DE),
            { "JFET Preamp Drive", "dB", -12.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Delay Time", "ms", 10.0f, 1024.0f, 240.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Feedback", "%", 0.0f, 95.0f, 38.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mod Depth/Rate", "Hz", 0.1f, 12.0f, 1.2f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ReverseEnvelopeEcho,
            SchematicCategory::DelayAndReverb,
            "Reverse Swell Echo",
            "Dual LIFO Reverse-Read Circular Buffers with Raised-Cosine Swell Window",
            "DL13",
            "dsp::circuits::ReverseEnvelopeEchoStage",
            "Dual ping-pong LIFO reverse playback delay line with exponential swell envelopes and oxide high-cut damping.",
            gui::Color::fromHex(0xC77DFF),
            { "Reverse Window", "ms", 40.0f, 1200.0f, 320.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Feedback", "%", 0.0f, 92.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Envelope Swell Crossfade", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Filter", "Hz", 800.0f, 16000.0f, 4500.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 42.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::CoiledTubingAcousticDelay,
            SchematicCategory::DelayAndReverb,
            "Coiled Pipe Delay",
            "50-Foot Coiled Copper Acoustic Pipe Delay with Kirchhoff Visco-Thermal Loss",
            "DL14",
            "dsp::circuits::CoiledTubingAcousticDelayStage",
            "Models a 1960s studio acoustic hose delay line driven by a horn transducer with pipe standing-wave resonances.",
            gui::Color::fromHex(0xFCA311),
            { "Pipe Delay", "ms", 5.0f, 180.0f, 44.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Horn Bandpass Resonance", "", 0.5f, 10.0f, 3.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Transducer Drive", "dB", -12.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Stereo Spread", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 38.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SubMsHaasPhaseAligner,
            SchematicCategory::DelayAndReverb,
            "Sub-Millisecond Micro-Widener",
            "Precision 0.01ms to 15ms Micro-Delay with Head-Shadow Stereo Widening",
            "DL15",
            "dsp::circuits::SubMsHaasPhaseAlignerStage",
            "Psychoacoustic Haas precedence micro-delay and all-pass phase rotator widening mono tracks without pan imbalance.",
            gui::Color::fromHex(0x00F5D4),
            { "L/R Haas Delay", "ms", 0.1f, 30.0f, 8.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Head-Shadow LPF", "Hz", 1500.0f, 20000.0f, 5800.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Side Level Trim", "dB", -15.0f, 15.0f, 1.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Bass Mono Cutoff", "Hz", 20.0f, 500.0f, 110.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ElectrostaticOilCanDelay,
            SchematicCategory::DelayAndReverb,
            "Oil-Can Flywheel Echo",
            "Rotating Neoprene Charge Belt in Dielectric Oil with Swampy Vibrato Decay",
            "DL16",
            "dsp::circuits::ElectrostaticOilCanDelayStage",
            "Electrostatic charge-storage belt rotating in dielectric oil with swampy flywheel wow and dark 12AX7 tube repeats.",
            gui::Color::fromHex(0xF72585),
            { "Belt Speed Delay", "ms", 60.0f, 420.0f, 195.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Charge Regeneration", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Flywheel Wobble", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "12AX7 Dark Tone", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DispersivePrismDelay,
            SchematicCategory::DelayAndReverb,
            "Prism Disperser Delay",
            "24-Stage High-Q All-Pass Cascade Where Highs Echo First & Lows Sweep Downward",
            "DL17",
            "dsp::circuits::DispersivePrismDelayStage",
            "High-order dispersive all-pass delay prism smearing transient echoes into frequency-swept harmonic cascades.",
            gui::Color::fromHex(0x00BBF9),
            { "Delay Time", "ms", 10.0f, 1000.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "All-Pass Stages", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pinch Frequency", "Hz", 120.0f, 6000.0f, 1150.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Feedback", "%", 0.0f, 95.0f, 42.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Vintage4LeverDigitalVerb,
            SchematicCategory::DelayAndReverb,
            "4-Lever Vintage Digital Verb",
            "16-Bit TTL All-Pass/Comb Computer with Independent Bass/Treble Decay Levers",
            "RV10",
            "dsp::circuits::Vintage4LeverDigitalVerbStage",
            "Pioneering 1976 4-lever digital reverberator with 11kHz transformer bandwidth and randomized comb address pointers.",
            gui::Color::fromHex(0xFF9F1C),
            { "White Lever — Decay Time", "s", 0.4f, 4.5f, 2.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::LeverSlider, {} },
            { "Blue Lever — Bass Multiplier", "x", 0.5f, 2.0f, 1.25f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::LeverSlider, {} },
            {
                { "Red Lever — Treble Multiplier", "x", 0.25f, 1.0f, 0.65f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::LeverSlider, {} },
                { "Black Lever — Pre-Delay", "ms", 0.0f, 60.0f, 20.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::LeverSlider, {} },
                { "Wet/Dry Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Program Mode", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "Reverb", "Delay", "Chorus" }, { 0.0f, 1.0f, 2.0f } },
                { "Radiation", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::PushButtonLatching, { "OFF", "DECORR ON" }, { 0.0f, 1.0f } }
            }
        },
        {
            SchematicComponentType::AcousticRoomComputer1982,
            SchematicCategory::DelayAndReverb,
            "6-Surface Acoustic Room Matrix",
            "6-Surface Early-Reflection Room Matrix Feeding an 8-Branch Late Reverberator",
            "RV11",
            "dsp::circuits::AcousticRoomComputer1982Stage",
            "1982 studio room simulation processor coupling explicit wall reflections with a warm 8-branch late FDN.",
            gui::Color::fromHex(0x3A86FF),
            { "Room Volume", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "RT60 Decay", "s", 0.3f, 15.0f, 2.1f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Low/High Contour", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "First Wall Delay", "ms", 2.0f, 150.0f, 24.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Room Mix", "%", 0.0f, 100.0f, 32.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::HighDensityDigitalPlate,
            SchematicCategory::DelayAndReverb,
            "High-Density Digital Plate",
            "15-Bit Companded Diffusion Cascade for Explosive Snare & Vocal Plate Sizzle",
            "RV12",
            "dsp::circuits::HighDensityDigitalPlateStage",
            "High-density 15-bit studio digital plate tank voiced for explosive 1980s snare splash and bright vocal sheen.",
            gui::Color::fromHex(0xF15BB5),
            { "Reverb Time", "s", 0.3f, 10.0f, 2.3f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Initial Delay", "ms", 0.1f, 200.0f, 26.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High Ratio", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Diffusion", "%", 0.0f, 100.0f, 75.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SympatheticSoundboardVerb,
            SchematicCategory::DelayAndReverb,
            "Grand Piano Soundboard",
            "Sitka Spruce Soundboard Bridge Dispersion + Tuned Sympathetic String Bank",
            "RV13",
            "dsp::circuits::SympatheticSoundboardVerbStage",
            "Acoustic grand piano soundboard and undamped sympathetic string resonator adding organic harmonic sustain.",
            gui::Color::fromHex(0xFFD166),
            { "Pedal Sustain Time", "s", 0.4f, 12.0f, 3.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "String Tuning", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Wood Bridge Coupling", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "HF Damping", "Hz", 1000.0f, 14000.0f, 5400.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SubOctaveAbyssalVerb,
            SchematicCategory::DelayAndReverb,
            "Dark Sub-Octave Tank",
            "Sub-Harmonic -12 Semitone Divider Embedded in a Dark 4x4 Hadamard FDN",
            "RV14",
            "dsp::circuits::SubOctaveAbyssalVerbStage",
            "Generates a dark -12 semitone sub-octave cathedral foundation underneath synths, brass, and low strings.",
            gui::Color::fromHex(0x560BAD),
            { "Decay Time", "s", 0.8f, 24.0f, 4.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sub-Octave Level", "dB", -18.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Low-Cut Guard", "Hz", 20.0f, 400.0f, 65.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Darkness", "Hz", 800.0f, 12000.0f, 2800.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 38.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SplitDecayConcertHall,
            SchematicCategory::DelayAndReverb,
            "Split-Decay Concert Hall",
            "Modulated Figure-8 All-Pass Hall with Independent Bass/Mid Crossover Loops",
            "RV15",
            "dsp::circuits::SplitDecayConcertHallStage",
            "1979 digital concert hall lattice with independent Bass and Midrange decay multipliers and wide lateral taps.",
            gui::Color::fromHex(0x7209B7),
            { "Bass Decay", "s", 0.6f, 25.0f, 3.6f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "Mid Decay", "s", 0.4f, 18.0f, 2.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Crossover Freq", "Hz", 150.0f, 1500.0f, 450.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::VerticalFader, {} },
                { "Treble Roll-Off", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::VerticalFader, {} },
                { "Mix", "%", 0.0f, 100.0f, 32.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Coaxial6SpringStudioVerb,
            SchematicCategory::DelayAndReverb,
            "Coaxial 6-Spring Studio Tank",
            "Counter-Wound Helical Springs with Electromagnetic Velocity Feedback",
            "RV16",
            "dsp::circuits::Coaxial6SpringStudioVerbStage",
            "Balanced 6-spring studio master reverb using counter-wound coaxial helices to eliminate single-spring boing.",
            gui::Color::fromHex(0x9D4EDD),
            { "Motional Decay Time", "s", 1.5f, 4.5f, 2.6f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Bass/Treble Tilt", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pre-Delay", "ms", 0.0f, 120.0f, 16.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Stereo Tank Width", "%", 0.0f, 100.0f, 80.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ExponentialReverseVerb,
            SchematicCategory::DelayAndReverb,
            "Exponential Reverse Bloom Verb",
            "64-Tap Reflection Cluster with Rising Exponential Ramp & Abrupt Gate Cliff",
            "RV17",
            "dsp::circuits::ExponentialReverseVerbStage",
            "Weights 64 room reflections along an exponentially rising ramp for real-time reverse-room vocal and snare swells.",
            gui::Color::fromHex(0xB5179E),
            { "Reverse Swell Time", "ms", 50.0f, 950.0f, 280.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pre-Delay", "ms", 0.0f, 200.0f, 20.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Diffusion Density", "%", 0.0f, 100.0f, 70.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "High-Cut Filter", "Hz", 800.0f, 16000.0f, 4800.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 42.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::InfiniteSpectralFreezeVerb,
            SchematicCategory::DelayAndReverb,
            "Infinite Freeze Drone",
            "Phase-Randomized Resonator Bank with Transient-Ducked Harmonic Crossfade",
            "RV18",
            "dsp::circuits::InfiniteSpectralFreezeStage",
            "Locks sustained harmonic energy into a phase-randomized infinite drone pad that updates smoothly on new notes.",
            gui::Color::fromHex(0x4CC9F0),
            { "Freeze Glissando Time", "ms", 20.0f, 2500.0f, 340.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Spectral Blur Window", "ms", 15.0f, 800.0f, 120.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Octave Shimmer Balance", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Filter Cutoff", "Hz", 60.0f, 16000.0f, 3200.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },

        // =====================================================================
        // TAB 6: SPLIT / MOD (16 Signal Splitters, Chorus/Flange/Phase & CV Modulators: SP_A..E, MD1..7, CV1..4)
        // =====================================================================
        {
            SchematicComponentType::TransientTonalSplitter,
            SchematicCategory::ModulationAndRoute,
            "Transient / Tonal Split",
            "Splits Audio into Phase-Coherent Transient Stream (Lane A) and Tonal Stream (Lane B)",
            "SP_A",
            "dsp::circuits::StructuralStreamSplitter",
            "Complementary mask M=((Ef-Es)/(Ef+eps))^gamma separating Transient & Tonal streams with zero phase distortion (M+(1-M)=1).",
            gui::Color::fromHex(0x00F5D4),
            { "Transient/Tonal Balance", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Split Sensitivity", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Mask Recovery", "ms", 10.0f, 500.0f, 85.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Transient Gain", "dB", -15.0f, 15.0f, 1.5f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Tonal Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::LinkwitzRiley4BandCrossover,
            SchematicCategory::ModulationAndRoute,
            "4-Band LR4 Crossover",
            "Splits Audio into Sub, Low-Mid, High-Mid, and Air Lanes for Multi-Band Processing",
            "SP_B",
            "dsp::circuits::LinkwitzRiley4BandCrossoverStage",
            "24dB/oct phase-compensated 4-band Linkwitz-Riley crossover splitting Sub, Low-Mid, High-Mid, and Air lanes.",
            gui::Color::fromHex(0x00BBF9),
            { "Low/LMF Freq", "Hz", 20.0f, 500.0f, 110.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "LMF/HMF Freq", "Hz", 250.0f, 4000.0f, 850.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "HMF/High Freq", "Hz", 1500.0f, 20000.0f, 4800.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Spectral Tilt", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::MidSideImager,
            SchematicCategory::ModulationAndRoute,
            "Elliptical M/S Matrix",
            "Splits Stereo Signal into Center (Mid) and Edges (Side) for Independent Processing",
            "SP_C",
            "dsp::circuits::EllipticalMidSideMatrixStage",
            "Phase-coherent Lateral/Vertical (Mid/Side) spatial matrix with 1st-order Butterworth sub-bass mono crossover.",
            gui::Color::fromHex(0x3A86FF),
            { "Mid Level", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Side Level", "dB", -15.0f, 15.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Stereo Width", "%", 0.0f, 200.0f, 115.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Bass Mono Crossover", "Hz", 30.0f, 400.0f, 120.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::ParallelSplitBus,
            SchematicCategory::ModulationAndRoute,
            "Parallel Phase Blender",
            "Parallel Signal Splitter with Auto-Latency Compensation & Continuous Phase Rotator",
            "SP_D",
            "dsp::circuits::ParallelWetDryPhaseBlenderStage",
            "Splits dry signal around all following [PARALLEL] components and sums wet/dry buses in phase.",
            gui::Color::fromHex(0xD4AF37),
            { "Wet/Dry Mix", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
            { "Wet Phase Rotation", "deg", -180.0f, 180.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Wet HPF", "Hz", 20.0f, 500.0f, 85.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Wet LPF", "Hz", 1500.0f, 20000.0f, 7500.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::FrequencyDuckingRouter,
            SchematicCategory::ModulationAndRoute,
            "Freq Ducking Router",
            "Routes Any Lane's Envelope to Duck or Modulate Another Lane's Parameters",
            "SP_E",
            "dsp::circuits::FrequencyConsciousDuckingRouterStage",
            "Dynamic sidechain ducking matrix routing frequency-selective envelope followers to duck competing bus lanes.",
            gui::Color::fromHex(0x90BE6D),
            { "Ducking Threshold", "dB", -48.0f, 0.0f, -22.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Ducking Depth", "dB", -24.0f, 0.0f, -9.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Focus Center Freq", "Hz", 80.0f, 12000.0f, 1200.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Release Time", "ms", 20.0f, 2000.0f, 180.0f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::BbdChorus,
            SchematicCategory::ModulationAndRoute,
            "4-Mode Pushbutton BBD Chorus",
            "Motionless-Center BBD Phase-Cross-Mixed Stereo Widener (Zero Pitch Wobble)",
            "MD1",
            "dsp::circuits::BucketBrigadeChorus",
            "Bucket-brigade modulated delay lines driven by 180-degree anti-phase triangle LFOs for motionless-center stereo width.",
            gui::Color::fromHex(0x00BBF9),
            { "Dimension Mode Buttons", "", 0.0f, 5.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::PushButtonBank, { "OFF", "Mode 1", "Mode 2", "Mode 3", "Mode 4", "Mode 1+4" }, { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f } },
            { "BBD Modulation Depth", "%", 50.0f, 150.0f, 100.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "LFO Rate", "Hz", 0.2f, 1.8f, 0.6f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Wet/Dry Mix", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::TriStereoBbdChorus,
            SchematicCategory::ModulationAndRoute,
            "Tri-Stereo BBD Chorus",
            "Three Parallel 120-Deg-Phase-Offset LFO BBD Lines for Lush 1980s Studio Swirl",
            "MD2",
            "dsp::circuits::TriStereoBbdChorusStage",
            "3-phase LFO matrix driving 3 MN3007 bucket-brigade delay lines delivering lush 1980s studio rack swirl.",
            gui::Color::fromHex(0x9B5DE5),
            { "Left Mod Depth", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Center Mod Depth", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Right Mod Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "LFO Rate", "Hz", 0.1f, 6.0f, 0.85f, true, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::ThroughZeroTapeFlanger,
            SchematicCategory::ModulationAndRoute,
            "Through-Zero Tape Flange",
            "Dual-Tape-Deck Time-Crossed Flanger with Deep Sub-Zero Phase Cancellation",
            "MD3",
            "dsp::circuits::ThroughZeroTapeFlanger",
            "Offsets dry reference reel by 2.5ms so modulated tape reel sweeps through zero relative delay for deep jet-comb cancellation.",
            gui::Color::fromHex(0x0096C7),
            { "Zero-Point Offset", "ms", -5.0f, 5.0f, 0.35f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sweep Rate", "Hz", 0.05f, 8.0f, 0.45f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Sweep Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Feedback", "%", -95.0f, 95.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::AnalogPhaser,
            SchematicCategory::ModulationAndRoute,
            "Optical Vibe Phaser",
            "4-to-12 Stage Incandescent Lamp & Photoresistor All-Pass Sweep Network",
            "MD4",
            "dsp::circuits::OpticalLampPhaserStage",
            "Allpass phase-shift cascade with quadrature incandescent lamp LFO sweep and warm resonant feedback.",
            gui::Color::fromHex(0x00F5D4),
            { "Speed", "Hz", 0.1f, 12.0f, 1.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Intensity", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Lamp Bias / Preamp Drive", "dB", -12.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Resonance Feedback", "%", 0.0f, 95.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Chorus/Vibrato Mode", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "CHORUS (Dry+Wet)", "VIBRATO (Pitch Wet)" }, { 0.0f, 1.0f } }
            }
        },
        {
            SchematicComponentType::MonoCompatibleWidener,
            SchematicCategory::ModulationAndRoute,
            "Mono-Safe Comb Widener",
            "Complementary All-Pass Comb Filter Bank Summing to 100% Phase-Cancelled Mono",
            "MD5",
            "dsp::circuits::PhasePureStereoWidener",
            "L_out = L + w*A(Mid), R_out = R - w*A(Mid). Collapsing L+R cancels +w*A(Mid) and -w*A(Mid) to 0.0 (100% mono-pure!).",
            gui::Color::fromHex(0x4CC9F0),
            { "Stereo Width", "%", 0.0f, 150.0f, 75.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Low-End Mono Cutoff", "Hz", 20.0f, 400.0f, 120.0f, true, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Comb Density", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Air Sheen", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::OpticalHarmonicTremolo,
            SchematicCategory::ModulationAndRoute,
            "Harmonic Crossover Tube Tremolo",
            "Alternates High-Pass and Low-Pass Filters via Optical/Bias Tube Modulation",
            "MD6",
            "dsp::circuits::OpticalHarmonicTremolo",
            "Splits signal at 450Hz and modulates Low and High bands in counter-phase via a nonlinear optical photoresistor LFO.",
            gui::Color::fromHex(0x48CAE4),
            { "Speed", "Hz", 0.2f, 12.0f, 2.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Intensity", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Crossover Freq", "Hz", 150.0f, 1500.0f, 450.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Tube Bias Asymmetry", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Harmonic/Bias Mode", "%", 0.0f, 100.0f, 75.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DiodeRingSubOctaveMod,
            SchematicCategory::ModulationAndRoute,
            "Formant Throat Shifter",
            "Linear Predictive Coding Vocal Tract Shifter with Hard-Tune Robot Lock",
            "MD7",
            "dsp::circuits::FormantThroatPitchShifterStage",
            "Vocal tract formant and sub-octave harmonic modulator shifting throat resonance and robotic carrier harmonics.",
            gui::Color::fromHex(0x4361EE),
            { "Formant Shift", "st", -12.0f, 12.0f, 2.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Pitch Shift", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Tube Drive", "dB", -12.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Voiced/Unvoiced Balance", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::CvAnalogDriftEngine,
            SchematicCategory::ModulationAndRoute,
            "Channel Tolerance Drift",
            "Injects 1-3% Left/Right Resistor/Cap Drift and Shared Power-Supply Sag Across Modules",
            "CV1",
            "dsp::circuits::ChannelToleranceDriftEngineStage",
            "Global analog tolerance & power-supply sag brain injecting organic L/R component drift and thermal bias across stages.",
            gui::Color::fromHex(0xFFB703),
            { "Tolerance Spread", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Thermal Drift Speed", "Hz", 0.1f, 12.0f, 0.85f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "B+ Sag Coupling", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Hum/Noise Floor", "dB", -96.0f, -48.0f, -78.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Channel Pair Skew", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::CvMacroKnobHub,
            SchematicCategory::ModulationAndRoute,
            "8-Target Macro Hub",
            "Binds Up to 8 Faceplate Parameters to a Single Master Knob with Custom Response Curves",
            "CV2",
            "dsp::circuits::MacroAttenuverterHubStage",
            "Multi-target macro CV controller binding multiple stage parameters to a single master knob with custom response curves.",
            gui::Color::fromHex(0xFF9F1C),
            { "Master Macro", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Target A Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Target B Depth", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Response Curve", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Slew Smoothing", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::CvEnvelopeFollowerSlew,
            SchematicCategory::ModulationAndRoute,
            "Slope Envelope Follower",
            "Converts Input Transients into a Bipolar CV Wire to Modulate Any EQ or Saturator Knob",
            "CV3",
            "dsp::circuits::SlopeEnvelopeFollowerDusgStage",
            "Audio-to-CV transient envelope follower with attack/release slew shaping for dynamic filter and drive modulation.",
            gui::Color::fromHex(0x06D6A0),
            { "Input Sensitivity", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Attack Rise", "ms", 0.1f, 100.0f, 3.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Release Fall", "ms", 20.0f, 2000.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Detector Focus", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "CV Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Polarity", "%", 0.0f, 100.0f, 75.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::CvOrganicLfoPerlin,
            SchematicCategory::ModulationAndRoute,
            "Sync LFO & Perlin Drift",
            "Multi-Wave LFO + Non-Repeating Perlin Noise Drift for Tape Wow & Filter Movement",
            "CV4",
            "dsp::circuits::OrganicLfoPerlinStage",
            "Multi-wave tempo-syncable LFO blended with non-repeating Perlin noise drift for organic tape wow and filter movement.",
            gui::Color::fromHex(0x72EFDD),
            { "Rate", "Hz", 0.1f, 12.0f, 1.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Depth", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Waveform Morph", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Perlin 1/f Chaos Blend", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Stereo Phase Offset", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SubAnchorCrossoverRouter,
            SchematicCategory::ModulationAndRoute,
            "Sub-Anchor Split Bus",
            "24dB/Oct Linkwitz-Riley Crossover Locking Sub-Bass Mono While Saturating Highs",
            "SP_F",
            "dsp::circuits::SubAnchorCrossoverRouterStage",
            "Splits sub-bass fundamentals (Lane A) from upper harmonics (Lane B) with 360-degree phase alignment.",
            gui::Color::fromHex(0x00F5D4),
            { "Crossover Freq", "Hz", 40.0f, 320.0f, 110.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sub Clean Compression", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "High Harmonic Drive", "dB", 0.0f, 18.0f, 5.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Sub/High Balance", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} },
                { "Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::DualMonoLrSplitMatrix,
            SchematicCategory::ModulationAndRoute,
            "Dual-Mono Drift Split",
            "Bifilar Transformer-Isolated L/R Channel Router with Tolerance & Phase Offset",
            "SP_G",
            "dsp::circuits::DualMonoLrSplitStage",
            "Decouples Left and Right channels for independent console component drift and inter-channel micro-delay.",
            gui::Color::fromHex(0x3A86FF),
            { "Left Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Right Gain", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "L/R Component Drift", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Crosstalk", "%", 0.0f, 100.0f, 25.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Stereo Balance", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::DynamicDuckingFxContainer,
            SchematicCategory::ModulationAndRoute,
            "Ducking FX Loop",
            "Fast Feed-Forward RMS Envelope Ducker Suppressing Parallel Wet FX on Transients",
            "SP_H",
            "dsp::circuits::DynamicDuckingFxContainerStage",
            "Automatically ducks parallel wet effects while dry lead or snare transients play, blooming into gaps.",
            gui::Color::fromHex(0x90BE6D),
            { "Dry Transient Sensitivity", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "FX Ducking Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Hold/Release Swell", "ms", 20.0f, 2000.0f, 220.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "FX Tone Tilt", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Wet Mix", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::HarmonicTonalNoiseSplitter,
            SchematicCategory::ModulationAndRoute,
            "Voice / Breath Splitter",
            "Routes Periodic Harmonic Partials to Lane A and Transients/Noise to Lane B",
            "SP_I",
            "dsp::circuits::HarmonicTonalNoiseSplitterStage",
            "Autocorrelation comb predictor separating pitched harmonic body from unvoiced breath, pick, and cymbal noise.",
            gui::Color::fromHex(0x06D6A0),
            { "Pitched Voice Level", "dB", -15.0f, 15.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Unvoiced Breath Level", "dB", -15.0f, 15.0f, -1.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Pitch Threshold", "dB", -48.0f, 0.0f, -24.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Breath Air Shelf", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::AzimuthCenterWideSplitter,
            SchematicCategory::ModulationAndRoute,
            "Phantom Center Splitter",
            "Coherence-Based Pan Extractor Isolating Center Vocals from Wide Guitars",
            "SP_J",
            "dsp::circuits::AzimuthCenterWideSplitterStage",
            "Inter-channel phase coherence mask separating phantom-center vocals/snare from hard-panned stereo instruments.",
            gui::Color::fromHex(0x4CC9F0),
            { "Phantom Center Level", "dB", -15.0f, 15.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Wide Sides Level", "dB", -15.0f, 15.0f, 0.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Coherence Focus", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Low/High Punch Tilt", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Output Trim", "dB", -15.0f, 15.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {}, {}, true }
            }
        },
        {
            SchematicComponentType::DualRotorCabinetHornDrum,
            SchematicCategory::ModulationAndRoute,
            "Tube Driven Rotary Cab",
            "800Hz Crossover Feeding Phenolic Treble Horn & Heavy Wooden Bass Rotor",
            "MD8",
            "dsp::circuits::DualRotorCabinetHornDrumStage",
            "Models a 6550 tube-driven rotary speaker cabinet with asynchronous horn/drum belt inertia and Doppler swirl.",
            gui::Color::fromHex(0xFCA311),
            { "Rotor Speed Switch", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "CHORALE Slow", "BRAKE Stop", "TREMOLO Fast" }, { 0.0f, 1.0f, 2.0f } },
            { "6550 Tube Amplifier Drive", "dB", 0.0f, 24.0f, 8.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Horn / Bass Drum Balance", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Rotor Acceleration Inertia", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mic Distance & Spread", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Cabinet Wet/Dry Mix", "%", 0.0f, 100.0f, 70.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::BalancedRackAnalogChorus,
            SchematicCategory::ModulationAndRoute,
            "Quadrature BBD Rack Chorus",
            "Dual MN3007 BBD Lines with Quadrature Stereo LFO & Compander Dynamics",
            "MD9",
            "dsp::circuits::BalancedRackAnalogChorusStage",
            "1979 balanced studio rack analog chorus with 90-degree quadrature LFO motion and warm companded BBD lines.",
            gui::Color::fromHex(0x00BBF9),
            { "Mode", "", 0.0f, 2.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "Chorus I", "Chorus II", "Vibrato" }, { 0.0f, 1.0f, 2.0f } },
            { "Rate", "Hz", 0.1f, 12.0f, 0.95f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Depth", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "BBD Preamp Drive", "dB", -12.0f, 18.0f, 3.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::RackThroughZeroFlanger,
            SchematicCategory::ModulationAndRoute,
            "Through-Zero BBD Rack Flanger",
            "Dual Matched SAD1024 BBD Decks Sweeping Through Zero Relative Delay",
            "MD10",
            "dsp::circuits::RackThroughZeroFlangerStage",
            "1975 studio rack through-zero flanger crossing a fixed 2.8ms reference deck for deep jet-comb cancellation.",
            gui::Color::fromHex(0x0096C7),
            { "LFO Rate", "Hz", 0.1f, 12.0f, 0.55f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Manual Sweep", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Envelope Follower Depth", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Feedback", "%", -95.0f, 95.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Reel Bounce / Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Jfet8StageRackPhaser,
            SchematicCategory::ModulationAndRoute,
            "8-Stage JFET Rack Phaser",
            "8 Matched 2N5457 JFET All-Pass Stages with Twin-Notch Resonant Feedback",
            "MD11",
            "dsp::circuits::Jfet8StageRackPhaserStage",
            "8-stage studio rack JFET phaser carving 4 deep all-pass notches with warm 2nd-harmonic FET bias.",
            gui::Color::fromHex(0x72EFDD),
            { "Rotor Speed", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Sweep Width", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Resonance Feedback", "", 0.5f, 10.0f, 4.5f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Stage Count", "%", 0.0f, 100.0f, 75.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::StudioRmsEnvelopeFilter,
            SchematicCategory::ModulationAndRoute,
            "Dual-Vactrol Envelope Filter",
            "Dual LED/CdS Vactrol Optocouplers Driving a 12dB/Oct State-Variable Filter",
            "MD12",
            "dsp::circuits::StudioRmsEnvelopeFilterStage",
            "Vactrol-coupled RMS dynamic envelope filter sculpting liquid funk/synth vowel sweeps from pick and drum attacks.",
            gui::Color::fromHex(0x80ED99),
            { "Sensitivity Gain", "dB", -18.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Peak Q", "", 0.4f, 6.0f, 2.4f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Filter Mode", "", 0.0f, 2.0f, 1.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ToggleSwitch3Way, { "LP (Low-Pass)", "BP (Band-Pass)", "HP (High-Pass)" }, { 0.0f, 1.0f, 2.0f } },
                { "Sweep Direction", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "UP", "DOWN" }, { 0.0f, 1.0f } },
                { "Range", "", 0.0f, 1.0f, 0.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ToggleSwitch2Way, { "Lo", "Hi" }, { 0.0f, 1.0f } }
            }
        },
        {
            SchematicComponentType::Analog16BandVocoderMatrix,
            SchematicCategory::ModulationAndRoute,
            "16-Band Formant Vocoder",
            "16-Band 18dB/Oct Formant Filterbank with Unvoiced Sibilance Injection",
            "MD13",
            "dsp::circuits::Analog16BandVocoderStage",
            "16-band analog formant vocoder and spectral resonator with formant transposition and consonant clarity.",
            gui::Color::fromHex(0x4361EE),
            { "Formant Shift", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Carrier Pitch", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Envelope Release", "ms", 20.0f, 2000.0f, 140.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Unvoiced Sibilance Injection", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::Binaural3DHrtfPanner,
            SchematicCategory::ModulationAndRoute,
            "3D Binaural Panner",
            "Spherical-Head Inter-Aural Time Delay & Pinna Elevation Spectral Cue Engine",
            "MD14",
            "dsp::circuits::Binaural3DHrtfPannerStage",
            "Places stereo or mono sources into a 360-degree binaural soundstage using Woodworth ITD and pinna notches.",
            gui::Color::fromHex(0x48CAE4),
            { "Azimuth", "deg", -180.0f, 180.0f, 25.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Elevation", "deg", -45.0f, 90.0f, 15.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Distance Air Loss", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Head ITD Scale", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Width/Mix", "%", 0.0f, 100.0f, 70.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::VacuumTubeTapeAdt1966,
            SchematicCategory::ModulationAndRoute,
            "Varispeed Valve Tape ADT",
            "Valve Oscillator Varispeed Controller Sweeping a 1-Inch Tube Tape Machine",
            "MD15",
            "dsp::circuits::VacuumTubeTapeAdt1966Stage",
            "1966 varispeed vacuum-tube artificial double tracker with manual reel flange and 82Hz head-bump.",
            gui::Color::fromHex(0xF15BB5),
            { "Varispeed Delay", "ms", -30.0f, 50.0f, 18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Manual Reel Flange", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Valve Drive", "dB", -12.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "82 Hz Bump", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Tape HF Loss", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::GermaniumDiodeRingMod,
            SchematicCategory::ModulationAndRoute,
            "Germanium Ring Mod",
            "Quad Matched OA10 Germanium Diode Lattice with Center-Tapped RF/Audio Iron",
            "MD16",
            "dsp::circuits::GermaniumRingModulatorStage",
            "Balanced quad germanium diode ring modulator with soft 0.28V forward conduction and pitch-tracked carrier.",
            gui::Color::fromHex(0xF72585),
            { "Carrier Freq", "Hz", 20.0f, 4000.0f, 280.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "LFO Rate", "Hz", 0.1f, 12.0f, 1.4f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "LFO FM Depth", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "OA10 Diode Drive", "dB", -12.0f, 18.0f, 4.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PhysicalDopplerAnimator,
            SchematicCategory::ModulationAndRoute,
            "Doppler Pass-By Engine",
            "Kinematic Doppler Pitch Trajectory Coupled to 1/r Distance & Humidity LPF",
            "MD17",
            "dsp::circuits::PhysicalDopplerAnimatorStage",
            "Simulates physical source pass-by velocity, Doppler pitch shift, inverse-square distance, and air absorption.",
            gui::Color::fromHex(0x00F5D4),
            { "Velocity", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Listener Distance", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Cycle Rate", "Hz", 0.1f, 12.0f, 0.65f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Air Humidity Absorption", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Mix", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::TempoSynced16StepGateCv,
            SchematicCategory::ModulationAndRoute,
            "16-Step Rhythm Gate",
            "16-Step Analog Multiplexer Driving Slew-Smoothed VCA & Resonant SVF",
            "CV5",
            "dsp::circuits::TempoSynced16StepGateCvStage",
            "16-step rhythmic gate and filter sequence modulator with variable RC portamento glide and stereo step offset.",
            gui::Color::fromHex(0xFFB703),
            { "Step Rate", "Hz", 0.1f, 12.0f, 2.2f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Gate Duty Cycle", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Step Slew Smoothing", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Swing", "%", 0.0f, 100.0f, 50.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Depth/Mix", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::TransientAdEnvelopeCv,
            SchematicCategory::ModulationAndRoute,
            "Percussive AD Trigger",
            "Schmitt-Trigger Transient Comparator Firing Exponential Attack-Decay CV",
            "CV6",
            "dsp::circuits::TransientAdEnvelopeCvStage",
            "Extracts a crisp exponential Attack-Decay control envelope from drum/pick hits to modulate drive and filter snap.",
            gui::Color::fromHex(0x06D6A0),
            { "Trigger Threshold", "dB", -48.0f, 0.0f, -22.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
            { "Attack Time", "ms", 0.1f, 100.0f, 2.5f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Decay Time", "ms", 20.0f, 2000.0f, 180.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "Exponential Curvature", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "CV Depth", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::SampleAndHoldSlewCv,
            SchematicCategory::ModulationAndRoute,
            "Sample-and-Hold Slew Randomizer",
            "Avalanche Zener Noise Source + JFET Sample-Hold Switch + RC Glide Lag",
            "CV7",
            "dsp::circuits::SampleAndHoldSlewCvStage",
            "West-Coast modular sample-and-hold random voltage generator with continuous RC slew portamento.",
            gui::Color::fromHex(0x9B5DE5),
            { "Clock Rate", "Hz", 0.1f, 12.0f, 1.8f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "Slew Lag", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Probability Distribution", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Correlation Memory", "%", 0.0f, 100.0f, 40.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "CV Depth", "%", 0.0f, 100.0f, 60.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PitchToCvFollower,
            SchematicCategory::ModulationAndRoute,
            "Pitch-to-CV Tracker",
            "Zero-Crossing 1V/Octave Pitch Extractor Locking Filter & Harmonic Bias",
            "CV8",
            "dsp::circuits::PitchToCvFollowerStage",
            "Tracks input fundamental pitch at 1V/octave to lock resonant filters and harmonic saturation to the played key.",
            gui::Color::fromHex(0x4CC9F0),
            { "Base Reference Pitch", "st", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            { "1V/Oct Scaling", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
            {
                { "Portamento Glide", "%", 0.0f, 100.0f, 35.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Confidence Gate", "%", 0.0f, 100.0f, 45.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "CV Offset", "%", 0.0f, 100.0f, 55.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::ContinuousKnob, {} }
            }
        },
        {
            SchematicComponentType::PsychoacousticCentroidCv,
            SchematicCategory::AutoAndSmart,
            "Auto LUFS Gain-Stager",
            "Slow K-Weighted Loudness Fader Calibrating Downstream Analog Sweet-Spot Input",
            "AU5",
            "dsp::circuits::AutoLufsGainStagerStage",
            "Rides a slow K-weighted loudness fader at the start of the rack so hot or quiet tracks always hit downstream analog stages at their sweet spot.",
            gui::Color::fromHex(0xFF9F1C),
            { "Target Loudness", "dB", -30.0f, -6.0f, -18.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            { "Max Gain Range", "dB", 0.0f, 18.0f, 12.0f, false, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::VerticalFader, {} },
            {
                { "Integration Window", "ms", 100.0f, 2000.0f, 400.0f, true, ParameterControlTier::Tier2_CorePhysicsEngine, HardwareControlKind::ContinuousKnob, {} },
                { "K-Weighting Tilt", "%", 0.0f, 100.0f, 65.0f, false, ParameterControlTier::Tier1_InputConditioning, HardwareControlKind::ContinuousKnob, {} },
                { "Gate Freeze Floor", "dB", -60.0f, -24.0f, -45.0f, false, ParameterControlTier::Tier3_CircuitVoiceModifier, HardwareControlKind::ContinuousKnob, {} },
                { "Sweet-Spot Trim", "dB", -12.0f, 12.0f, 0.0f, false, ParameterControlTier::Tier4_OutputBlendTelemetry, HardwareControlKind::VerticalFader, {} }
            }
        }
    };
    return s_rawCatalog;
}

/**
 * @brief Classifies any SchematicComponentType into the 9-Archetype Matrix (A..I).
 */
[[nodiscard]] inline PrimaryModuleArchetype classifyPrimaryArchetype(SchematicComponentType type) noexcept {
    switch (type) {
        // A. Preamps, Buffers & Transformers
        case SchematicComponentType::ClassAPreamp:
        case SchematicComponentType::ClassABOpAmp2520:
        case SchematicComponentType::Tube12AX7:
        case SchematicComponentType::PentodeEF86:
        case SchematicComponentType::DirectHeatedTriode:
        case SchematicComponentType::HighVoltageCascodeTube:
        case SchematicComponentType::JfetBuffer:
        case SchematicComponentType::InputTransformer:
        case SchematicComponentType::OutputTransformer:
        case SchematicComponentType::OutputTransformerMatrix:
        case SchematicComponentType::SilverToroidalIron:
        case SchematicComponentType::PaperInOilInterstageCap:
        case SchematicComponentType::BlueStripeSiliconPre:
        case SchematicComponentType::Console4000IcStage:
        case SchematicComponentType::JapaneseTapeDeckPre:
        case SchematicComponentType::SubMiniatureTubeMicPre:
        case SchematicComponentType::AmericanTubeConsolePre:
        case SchematicComponentType::GermanBroadcastValvePre:
        case SchematicComponentType::ConsoleSummingBusAmp:
        case SchematicComponentType::HeadroomCalibratorPre:
        case SchematicComponentType::Olympic69LustraphonePre:
        case SchematicComponentType::Abbey68TransistorPre:
        case SchematicComponentType::LondonARangeZenerPre:
        case SchematicComponentType::WestCoastAm10Pre:
        case SchematicComponentType::TransformerlessJfetBjtPre:
        case SchematicComponentType::HighRail80BroadcastPre:
        case SchematicComponentType::Criteria2002HybridPre:
        case SchematicComponentType::AirStudioZobelPre:
        case SchematicComponentType::Soul60s24VClassAPre:
        case SchematicComponentType::LaStudioQuadTransistorPre:
        case SchematicComponentType::Octal50sBroadcastTubePre:
        case SchematicComponentType::DavenSteppedTubePre:
        case SchematicComponentType::SwissBroadcastLinePre:
        case SchematicComponentType::ZeroFeedbackMasteringPre:
        case SchematicComponentType::LayeredPermalloyInterstage:
        case SchematicComponentType::Balanced600BridgingIron:
        case SchematicComponentType::SiliconValveOutputIron:
        case SchematicComponentType::BesselWidebandServoIron:
        case SchematicComponentType::CobaltAmorphousCoreIron:
        case SchematicComponentType::GermanDualPentodeCinemaPre:
        case SchematicComponentType::ZeroFeedbackPureTriodePre:
        case SchematicComponentType::CurrentModeTransimpedancePre:
        case SchematicComponentType::EasternBroadcastPentodePre:
        case SchematicComponentType::JapaneseDiscreteClassAPre:
        case SchematicComponentType::VariImpedanceCableLoadPre:
            return PrimaryModuleArchetype::A_PreampsBuffersTransformers;

        // B. Compressors, Limiters & Gain Cells
        case SchematicComponentType::FetCompressor1176:
        case SchematicComponentType::OptoCompressorT4B:
        case SchematicComponentType::VariMuTubeCompressor:
        case SchematicComponentType::DiodeBridgeCompressor:
        case SchematicComponentType::DualOptoVcaCompressor:
        case SchematicComponentType::ZenerShuntLimiter:
        case SchematicComponentType::SidechainThrustMatrix:
        case SchematicComponentType::VcaBusCompressor:
        case SchematicComponentType::BrickwallLimiter:
        case SchematicComponentType::SplitBandDeEsser:
        case SchematicComponentType::UpDownMultibandCompressor:
        case SchematicComponentType::OverEasyRmsSmackVca:
        case SchematicComponentType::BroadcastTriodeLimiter:
        case SchematicComponentType::PsychoacousticMasteringLimiter:
        case SchematicComponentType::TransconductanceRoomSlammer:
        case SchematicComponentType::BroadcastFieldEffectCrusher:
        case SchematicComponentType::RmsVcaPeakStopLimiter:
        case SchematicComponentType::DiscreteMasteringMsComp:
        case SchematicComponentType::BritishPhotoOpticalComp:
        case SchematicComponentType::DualSlopeBusVcaComp:
        case SchematicComponentType::SolidStateT4BLeveler:
        case SchematicComponentType::ParametricDynamicEq4Band:
        case SchematicComponentType::PhaseSafeDrumBleedGate:
        case SchematicComponentType::ZeroDistortionVocalRider:
        case SchematicComponentType::Broadcast5BandFmLimiter:
        case SchematicComponentType::SuperTube6H30PVariMu:
        case SchematicComponentType::LinearPhaseUpwardLifter:
        case SchematicComponentType::CrestFactorDensityController:
        case SchematicComponentType::SilkBerylliumAirExciter:
            return PrimaryModuleArchetype::B_CompressorsLimitersGainCells;

        // C. Multi-Band & Parametric EQs
        case SchematicComponentType::InductorEq3Band:
        case SchematicComponentType::ProportionalQGyratorEq:
        case SchematicComponentType::ParallelPassiveLcEq:
        case SchematicComponentType::Olympic69InductorEq:
        case SchematicComponentType::Detroit7BandToroidalEq:
        case SchematicComponentType::LondonARangeInductorEq:
        case SchematicComponentType::ParallelStateVariableEq:
        case SchematicComponentType::SteppedMasteringTransferEq:
        case SchematicComponentType::EllipticalMidSideEq:
            return PrimaryModuleArchetype::C_MultiBandParametricEqs;

        // D. Passive & Specialty Tone EQs
        case SchematicComponentType::PassivePultecEq:
        case SchematicComponentType::BaxandallTone:
        case SchematicComponentType::ResonantSubSculptor:
        case SchematicComponentType::FulcrumTiltAirEq:
        case SchematicComponentType::ResonantFilter:
        case SchematicComponentType::TransistorLadderFilter:
        case SchematicComponentType::SidechainSpectralCarver32:
        case SchematicComponentType::ProportionalGraphicEq10Band:
        case SchematicComponentType::PassiveMidDipPeakEq:
        case SchematicComponentType::PassiveHighLowBracketEq:
        case SchematicComponentType::MasteringBaxandallEq:
        case SchematicComponentType::GermanBroadcastSilkyEq:
        case SchematicComponentType::DualInductorPCurveEq:
        case SchematicComponentType::Iso226DynamicLoudnessEq:
        case SchematicComponentType::BridgedTwinTNotchEq:
            return PrimaryModuleArchetype::D_PassiveSpecialtyToneEqs;

        // E. Spectral & Smart Processors
        case SchematicComponentType::SpectralResonanceSuppressor:
        case SchematicComponentType::BarkAuditoryUnmasker:
        case SchematicComponentType::PsychoacousticSubSynth:
        case SchematicComponentType::HarmonicPitchTrackingEq:
        case SchematicComponentType::PhasePolarityAligner:
        case SchematicComponentType::FrequencyDuckingRouter:
        case SchematicComponentType::FixedResonant12BandFilter:
        case SchematicComponentType::SpectralBrickwallBinEq:
        case SchematicComponentType::PhaseCompensatedBassMono:
        case SchematicComponentType::DynamicSpectralDeHarsher:
        case SchematicComponentType::RealTimeSpectralDeReverb:
        case SchematicComponentType::SinusoidalTonalNoiseSeparator:
        case SchematicComponentType::SubSampleCrossCorrPhaseAligner:
            return PrimaryModuleArchetype::E_SpectralSmartProcessors;

        // F. Transient Shapers & Gates
        case SchematicComponentType::TransientShaper:
        case SchematicComponentType::TransientTonalSplitter:
        case SchematicComponentType::HysteresisNoiseGate:
        case SchematicComponentType::LookaheadUpwardExpander:
        case SchematicComponentType::HarmonicTonalNoiseSplitter:
        case SchematicComponentType::AzimuthCenterWideSplitter:
            return PrimaryModuleArchetype::F_TransientShapersAndGates;

        // G. Saturators, Wave-Shapers & Clippers
        case SchematicComponentType::GermaniumBroadcastPre:
        case SchematicComponentType::ParallelSaturationBus:
        case SchematicComponentType::TapeHysteresis:
        case SchematicComponentType::StatelessHarmonicInflator:
        case SchematicComponentType::DacBitcrusherStage:
        case SchematicComponentType::TrigonometricWavefolder:
        case SchematicComponentType::OversampledVariableClipper:
        case SchematicComponentType::HighFreqTapeLimiter:
        case SchematicComponentType::LoFiCassetteVhsWarper:
        case SchematicComponentType::VariableSilkExciter:
        case SchematicComponentType::VintageAuralExciter:
        case SchematicComponentType::AsymmetricalDiodeClipper:
        case SchematicComponentType::HalfInch30IpsMasteringTape:
        case SchematicComponentType::OneInch15IpsTubeTape:
        case SchematicComponentType::DifferentialAirCompander:
        case SchematicComponentType::StainlessWireRecorder:
        case SchematicComponentType::MasteringVinylCuttingLathe:
        case SchematicComponentType::TriodePentode3BandMatrix:
        case SchematicComponentType::EnvelopeGatedTransientExciter:
        case SchematicComponentType::ConsoleBusRailOverdrive:
        case SchematicComponentType::StudioPcm14BitConverter:
        case SchematicComponentType::SlidingActionSpectralCompander:
        case SchematicComponentType::OpticalSoundtrackFilmExciter:
        case SchematicComponentType::CollectorSaturationMatrix3Path:
        case SchematicComponentType::DynamicFluxDeSaturator:
        case SchematicComponentType::GainRangingFloatingPointDac:
        case SchematicComponentType::DielectricAbsorptionCapAging:
        case SchematicComponentType::MasteringAdcOverdrive:
        case SchematicComponentType::SplitBandMasteringClipper:
        case SchematicComponentType::HilbertWaveformSymmetrizer:
        case SchematicComponentType::Adaa2ndOrderClipper:
            return PrimaryModuleArchetype::G_SaturatorsWaveShapersClippers;

        // H. Delays, Tape, BBD & Pitch Shifters (+ Modulated Delay Lines & Routing/CV)
        case SchematicComponentType::TapeEcho:
        case SchematicComponentType::BbdCompanderDelay:
        case SchematicComponentType::MicroPitchHarmonizer:
        case SchematicComponentType::PingPongDelay:
        case SchematicComponentType::BbdChorus:
        case SchematicComponentType::AnalogPhaser:
        case SchematicComponentType::ThroughZeroTapeFlanger:
        case SchematicComponentType::OpticalHarmonicTremolo:
        case SchematicComponentType::DiodeRingSubOctaveMod:
        case SchematicComponentType::MonoCompatibleWidener:
        case SchematicComponentType::MidSideImager:
        case SchematicComponentType::ParallelSplitBus:
        case SchematicComponentType::MagneticDrumEcho:
        case SchematicComponentType::Companded12BitRackDelay:
        case SchematicComponentType::DynamicDuckingDelay:
        case SchematicComponentType::Rhythmic8TapDiffuser:
        case SchematicComponentType::AsynchronousGrainCloud:
        case SchematicComponentType::StudioAdtTapeDoubler:
        case SchematicComponentType::LinkwitzRiley4BandCrossover:
        case SchematicComponentType::TriStereoBbdChorus:
        case SchematicComponentType::CvAnalogDriftEngine:
        case SchematicComponentType::CvMacroKnobHub:
        case SchematicComponentType::CvEnvelopeFollowerSlew:
        case SchematicComponentType::CvOrganicLfoPerlin:
        case SchematicComponentType::GoldenRatioDualRackDelay:
        case SchematicComponentType::Modulated16BitRackDelay:
        case SchematicComponentType::ReverseEnvelopeEcho:
        case SchematicComponentType::CoiledTubingAcousticDelay:
        case SchematicComponentType::SubMsHaasPhaseAligner:
        case SchematicComponentType::ElectrostaticOilCanDelay:
        case SchematicComponentType::DispersivePrismDelay:
        case SchematicComponentType::SubAnchorCrossoverRouter:
        case SchematicComponentType::DualMonoLrSplitMatrix:
        case SchematicComponentType::DynamicDuckingFxContainer:
        case SchematicComponentType::DualRotorCabinetHornDrum:
        case SchematicComponentType::BalancedRackAnalogChorus:
        case SchematicComponentType::RackThroughZeroFlanger:
        case SchematicComponentType::Jfet8StageRackPhaser:
        case SchematicComponentType::StudioRmsEnvelopeFilter:
        case SchematicComponentType::Analog16BandVocoderMatrix:
        case SchematicComponentType::Binaural3DHrtfPanner:
        case SchematicComponentType::VacuumTubeTapeAdt1966:
        case SchematicComponentType::GermaniumDiodeRingMod:
        case SchematicComponentType::PhysicalDopplerAnimator:
        case SchematicComponentType::TempoSynced16StepGateCv:
        case SchematicComponentType::TransientAdEnvelopeCv:
        case SchematicComponentType::SampleAndHoldSlewCv:
        case SchematicComponentType::PitchToCvFollower:
        case SchematicComponentType::PsychoacousticCentroidCv:
            return PrimaryModuleArchetype::H_DelaysTapeBbdPitchShifters;

        // I. Reverbs & Physical Spaces
        case SchematicComponentType::PlateReverb:
        case SchematicComponentType::HallReverbFDN:
        case SchematicComponentType::OctaveShimmerReverb:
        case SchematicComponentType::DispersionSpringReverb:
        case SchematicComponentType::NonlinearGatedReverb:
        case SchematicComponentType::DispersiveSteelPlate:
        case SchematicComponentType::TiledLiveDrumChamber:
        case SchematicComponentType::EarlyReflectionAmbience:
        case SchematicComponentType::InfiniteVelvetBlackhole:
        case SchematicComponentType::ZeroLatencyIrConvolver:
        case SchematicComponentType::Vintage4LeverDigitalVerb:
        case SchematicComponentType::AcousticRoomComputer1982:
        case SchematicComponentType::HighDensityDigitalPlate:
        case SchematicComponentType::SympatheticSoundboardVerb:
        case SchematicComponentType::SubOctaveAbyssalVerb:
        case SchematicComponentType::SplitDecayConcertHall:
        case SchematicComponentType::Coaxial6SpringStudioVerb:
        case SchematicComponentType::ExponentialReverseVerb:
        case SchematicComponentType::InfiniteSpectralFreezeVerb:
            return PrimaryModuleArchetype::I_ReverbsAndPhysicalSpaces;
    }
    return PrimaryModuleArchetype::A_PreampsBuffersTransformers;
}

/**
 * @brief Returns the Trait Tag bitmask (`TAG_TUBE`, `TAG_TRANSFORMER`, `TAG_INDUCTOR`, `TAG_TAPE`, etc.)
 * for the "Tag-Based Modifier Decorator" step of the 3-Step Decomposition Engine.
 */
[[nodiscard]] inline uint32_t getComponentTraitTags(SchematicComponentType type) noexcept {
    switch (type) {
        case SchematicComponentType::Tube12AX7:
        case SchematicComponentType::PentodeEF86:
        case SchematicComponentType::DirectHeatedTriode:
        case SchematicComponentType::HighVoltageCascodeTube:
        case SchematicComponentType::SubMiniatureTubeMicPre:
        case SchematicComponentType::AmericanTubeConsolePre:
        case SchematicComponentType::TrigonometricWavefolder:
            return TAG_TUBE;

        case SchematicComponentType::ClassAPreamp:
        case SchematicComponentType::InputTransformer:
        case SchematicComponentType::OutputTransformer:
        case SchematicComponentType::OutputTransformerMatrix:
        case SchematicComponentType::SilverToroidalIron:
        case SchematicComponentType::MidSideImager:
        case SchematicComponentType::ConsoleSummingBusAmp:
        case SchematicComponentType::VariableSilkExciter:
            return TAG_TRANSFORMER;

        case SchematicComponentType::InductorEq3Band:
        case SchematicComponentType::ResonantSubSculptor:
            return TAG_INDUCTOR | TAG_TRANSFORMER;

        case SchematicComponentType::PassivePultecEq:
        case SchematicComponentType::ParallelPassiveLcEq:
            return TAG_INDUCTOR | TAG_TRANSFORMER;

        case SchematicComponentType::TapeHysteresis:
        case SchematicComponentType::TapeEcho:
        case SchematicComponentType::ThroughZeroTapeFlanger:
        case SchematicComponentType::HighFreqTapeLimiter:
        case SchematicComponentType::LoFiCassetteVhsWarper:
        case SchematicComponentType::MagneticDrumEcho:
        case SchematicComponentType::StudioAdtTapeDoubler:
            return TAG_TAPE | TAG_TRANSFORMER;

        case SchematicComponentType::BbdCompanderDelay:
        case SchematicComponentType::MicroPitchHarmonizer:
        case SchematicComponentType::DacBitcrusherStage:
        case SchematicComponentType::BbdChorus:
        case SchematicComponentType::Companded12BitRackDelay:
        case SchematicComponentType::TriStereoBbdChorus:
            return TAG_BBD_OR_VINTAGE_DIGITAL;

        case SchematicComponentType::FetCompressor1176:
        case SchematicComponentType::DiodeBridgeCompressor:
        case SchematicComponentType::VcaBusCompressor:
        case SchematicComponentType::SidechainThrustMatrix:
        case SchematicComponentType::SplitBandDeEsser:
        case SchematicComponentType::UpDownMultibandCompressor:
        case SchematicComponentType::HysteresisNoiseGate:
        case SchematicComponentType::OverEasyRmsSmackVca:
        case SchematicComponentType::LookaheadUpwardExpander:
        case SchematicComponentType::BroadcastTriodeLimiter:
        case SchematicComponentType::DynamicDuckingDelay:
        case SchematicComponentType::FrequencyDuckingRouter:
            return TAG_SIDECHAIN_CAPABLE | TAG_TRANSFORMER;

        case SchematicComponentType::OptoCompressorT4B:
        case SchematicComponentType::VariMuTubeCompressor:
            return TAG_TUBE | TAG_TRANSFORMER | TAG_SIDECHAIN_CAPABLE;

        case SchematicComponentType::DualOptoVcaCompressor:
        case SchematicComponentType::GermanBroadcastValvePre:
            return TAG_TRANSFORMER | TAG_SIDECHAIN_CAPABLE;

        default:
            return TAG_NONE;
    }
}

/**
 * @brief Classifies any parameter into one of the 4 Mandatory Control Tiers
 * so the Faceplate Builder can assign visual hierarchy (Tier 2 = LARGE_KNOB, Tier 1/3 = SMALL_KNOB).
 */
[[nodiscard]] inline ParameterControlTier classifyParamTier(const ComponentParamSpec& p, int slotIdx) noexcept {
    std::string lower = p.name;
    for (char& c : lower) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    const auto has = [&](std::string_view w) { return lower.find(w) != std::string::npos; };

    // Tier 4: Output, Makeup, Blend & Dry/Wet Mix
    if (has("mix") || has("output") || has("makeup") || has("dry") || has("wet")) {
        return ParameterControlTier::Tier4_OutputBlendTelemetry;
    }
    // Tier 1: Input Conditioning, Pre-Delay, HPF/LPF Scope Bounds
    if (has("hpf") || has("lpf") || has("pre-delay") || has("predelay") || has("scope") ||
        has("bound") || has("pad") || has("input trim")) {
        return ParameterControlTier::Tier1_InputConditioning;
    }
    // Tier 3: Circuit Nonlinearity, Bias, Sag, Core Alloy, Damping, Spin, Wow/Flutter
    if (has("bias") || has("sag") || has("starve") || has("core") || has("ni/") || has("damp") ||
        has("spin") || has("wow") || has("flutter") || has("symmetry") || has("contour") ||
        has("slew") || has("sheen") || has("warmth") || has("tilt") || has("emphasis")) {
        return ParameterControlTier::Tier3_CircuitVoiceModifier;
    }
    // Default by slot: slot 0 & 1 are primary core controls unless matched above
    (void)slotIdx;
    return ParameterControlTier::Tier2_CorePhysicsEngine;
}

/**
 * @brief Curated Flagship Studio Engineering Filter (retains the 150 highest-tier, distinct
 * studio workhorses across all 7 tabs while pruning 66 niche/novelty/duplicate variants).
 */
[[nodiscard]] inline bool isCuratedStudioComponent(const SchematicComponentMeta& m) noexcept {
    const std::string& d = m.rawDesignator.empty() ? m.designator : m.rawDesignator;
    switch (m.category) {
        case SchematicCategory::PreampsAndTubes:
            // 24 highest-tier studio preamps, console strips, broadcast valves & mastering line drivers
            return d == "PR1"  || d == "PR2"  || d == "PR3"  || d == "PR4"  ||
                   d == "PR5"  || d == "PR6"  || d == "PR7"  || d == "PR8"  ||
                   d == "PR9"  || d == "PR11" || d == "PR12" || d == "PR13" ||
                   d == "PR14" || d == "PR15" || d == "PR16" || d == "PR17" ||
                   d == "PR18" || d == "PR20" || d == "PR21" || d == "PR27" ||
                   d == "PR28" || d == "PR30" || d == "PR32" || d == "PR33";

        case SchematicCategory::IronAndTape:
            // 25 highest-tier studio transformers, mastering/valve tape decks, companders, lathe & harmonic exciters
            return d == "IR1"  || d == "IR2"  || d == "IR3"  || d == "IR4"  ||
                   d == "IR6"  || d == "IR7"  || d == "IR9"  || d == "IR10" ||
                   d == "TP1"  || d == "TP2"  || d == "TP3"  || d == "TP4"  ||
                   d == "TP5"  || d == "TP6"  || d == "TP8"  ||
                   d == "ST1"  || d == "ST2"  || d == "ST3"  || d == "ST4"  ||
                   d == "ST5"  || d == "ST6"  || d == "ST7"  || d == "ST9"  ||
                   d == "ST10" || d == "ST11";

        case SchematicCategory::EqAndFilters:
            // 25 highest-tier passive, inductor, console, mastering & spectral equalizers
            return d == "EQ1"  || d == "EQ2"  || d == "EQ3"  || d == "EQ4"  ||
                   d == "EQ5"  || d == "EQ6"  || d == "EQ7"  || d == "EQ8"  ||
                   d == "EQ10" || d == "EQ11" || d == "EQ12" || d == "EQ13" ||
                   d == "EQ14" || d == "EQ15" || d == "EQ16" || d == "EQ17" ||
                   d == "EQ18" || d == "EQ19" || d == "EQ20" ||
                   d == "SP1"  || d == "SP4"  || d == "SP6"  || d == "SP9"  ||
                   d == "SP10" || d == "SP13";

        case SchematicCategory::AnalogDynamics:
            // 26 highest-tier FET, Opto, Vari-Mu, Diode, VCA, De-Esser, Gates & Mastering Clippers/Limiters
            return d == "CP1"  || d == "CP2"  || d == "CP3"  || d == "CP4"  ||
                   d == "CP5"  || d == "CP6"  || d == "CP7"  || d == "CP8"  ||
                   d == "CP9"  || d == "CP10" || d == "CP11" || d == "CP12" ||
                   d == "CP13" || d == "CP14" || d == "CP15" || d == "CP16" ||
                   d == "CP18" || d == "CP19" || d == "CP21" || d == "CP24" ||
                   d == "CP27" ||
                   d == "CL1"  || d == "CL2"  || d == "CL3"  || d == "CL4"  ||
                   d == "CL7";

        case SchematicCategory::DelayAndReverb:
            // 24 highest-tier tape/BBD/drum echoes, rack digital delays, shimmer, plates, springs, chambers & halls
            return d == "DL1"  || d == "DL2"  || d == "DL3"  || d == "DL4"  ||
                   d == "DL5"  || d == "DL6"  || d == "DL7"  || d == "DL8"  ||
                   d == "DL9"  || d == "DL10" || d == "DL11" || d == "DL12" ||
                   d == "SH1"  ||
                   d == "RV1"  || d == "RV2"  || d == "RV3"  || d == "RV4"  ||
                   d == "RV5"  || d == "RV6"  || d == "RV7"  || d == "RV8"  ||
                   d == "RV9"  || d == "RV10" || d == "RV15";

        case SchematicCategory::ModulationAndRoute:
            // 21 highest-tier studio splitters, BBD choruses, through-zero flangers, phasers, rotary, 3D panner & CV/macro hubs
            return d == "SP_A" || d == "SP_B" || d == "SP_C" || d == "SP_D" ||
                   d == "SP_E" || d == "SP_F" || d == "SP_H" ||
                   d == "MD1"  || d == "MD2"  || d == "MD3"  || d == "MD4"  ||
                   d == "MD5"  || d == "MD6"  || d == "MD8"  || d == "MD9"  ||
                   d == "MD10" || d == "MD11" || d == "MD14" || d == "MD15" ||
                   d == "CV2"  || d == "CV3";

        case SchematicCategory::AutoAndSmart:
            // 5 smart mixing & gain-staging tools
            return d == "AU1" || d == "AU2" || d == "AU3" || d == "AU4" || d == "AU5";
    }
    return false;
}

/**
 * @brief 3-Step Generative Decomposition & Pre-Flight Validation Engine.
 * Decomposes every component in the catalog across all 4 Mandatory Control Tiers using:
 *   - Step 1: Real Acoustic Units Enforcement & Multi-Band EQ Frequency De-Hardcoding
 *   - Step 2: Trait-Tag Modifier Decorator Injection (`TAG_TUBE`, `TAG_TRANSFORMER`, etc.)
 *   - Step 3: Archetype Matrix (A..I) 4-Tier Enrichment (guaranteeing 5 primary parameters per module)
 */
inline void enrichAndValidateComponentGrammar(SchematicComponentMeta& m) {
    const std::string& d = m.rawDesignator.empty() ? m.designator : m.rawDesignator;
    auto normalizeSteppedParam = [](ComponentParamSpec& p) {
        if (!p.stepLabels.empty()) {
            p.isLogarithmic = false;
            if ((p.minVal == 40.0f && p.maxVal == 12000.0f) ||
                (p.minVal == 1.0f && p.maxVal == 500.0f)) {
                p.minVal     = 0.0f;
                p.maxVal     = static_cast<float>(std::max<size_t>(1, p.stepLabels.size() - 1));
                p.defaultVal = std::round(p.maxVal * 0.5f);
            }
            if (p.stepLabels.size() == 2 && p.minVal == 0.0f && p.maxVal == 1.0f) {
                const std::string& s0 = p.stepLabels[0];
                const std::string& s1 = p.stepLabels[1];
                if ((s0 == "IN" || s0.find("IN") == 0 || s0.find("Hardware Relay") == 0) &&
                    (s1 == "OUT" || s1 == "BYP" || s1.find("OUT") == 0 || s1.find("BYP") == 0)) {
                    p.stepLabels = { "BYP", "IN" };
                    p.defaultVal = 1.0f;
                }
            }
        }
    };
    normalizeSteppedParam(m.param1);
    normalizeSteppedParam(m.param2);
    for (auto& ep : m.extraParams) {
        normalizeSteppedParam(ep);
    }

    // Calibrated flat/hardware defaults for benchmark EQ & Compressor units
    if (d == "EQ1") {
        m.param1.defaultVal = 2.0f; // 60 Hz
        m.param2.defaultVal = 0.0f; // Low Boost = 0
        if (m.extraParams.size() >= 1) m.extraParams[0].defaultVal = 0.0f; // Low Atten = 0
    } else if (d == "EQ2") {
        m.param1.unit       = "kHz";
        m.param1.defaultVal = 4.0f; // 10 kHz
        m.param2.unit       = "kHz";
        m.param2.defaultVal = 1.0f; // 10 kHz
        if (m.extraParams.size() >= 1) m.extraParams[0].defaultVal = 0.0f; // High Boost = 0
        if (m.extraParams.size() >= 3) m.extraParams[2].defaultVal = 0.0f; // High Atten = 0
    } else if (d == "EQ3") {
        m.param1.defaultVal = 0.0f;
        if (m.extraParams.size() >= 3)  m.extraParams[2].defaultVal  = 0.0f;
        if (m.extraParams.size() >= 7)  m.extraParams[6].defaultVal  = 0.0f;
        if (m.extraParams.size() >= 11) m.extraParams[10].defaultVal = 0.0f;
    } else if (d == "EQ5") {
        if (m.extraParams.size() >= 5) m.extraParams[4].defaultVal = 1.0f; // SHELF
        if (m.extraParams.size() >= 6) m.extraParams[5].defaultVal = 1.0f; // SHELF
        if (m.extraParams.size() >= 7) { m.extraParams[6].stepLabels = { "OUT", "IN" }; m.extraParams[6].defaultVal = 0.0f; }
        if (m.extraParams.size() >= 8) { m.extraParams[7].stepLabels = { "BYP", "IN" }; m.extraParams[7].defaultVal = 1.0f; }
    } else if (d == "EQ6") {
        if (m.extraParams.size() >= 9)  m.extraParams[8].defaultVal  = 20.0f;
        if (m.extraParams.size() >= 10) m.extraParams[9].defaultVal  = 20000.0f;
        if (m.extraParams.size() >= 14) m.extraParams[13].defaultVal = 1.0f;
    } else if (d == "EQ8") {
        m.param1.unit       = "kHz";
        m.param1.defaultVal = 2.0f; // 10 kHz
        m.param2.defaultVal = 0.0f; // Air Gain = 0 dB
    } else if (d == "EQ11") {
        if (m.extraParams.size() >= 1) m.extraParams[0].unit       = "kHz";
        if (m.extraParams.size() >= 2) m.extraParams[1].defaultVal = 0.0f;
        if (m.extraParams.size() >= 3) m.extraParams[2].defaultVal = 0.0f;
        if (m.extraParams.size() >= 4) m.extraParams[3].defaultVal = 0.0f;
    } else if (d == "EQ12") {
        m.param1.defaultVal = 0.0f;  // OFF
        m.param2.defaultVal = 10.0f; // OFF
    }

    m.param1.tier = classifyParamTier(m.param1, 0);
    m.param2.tier = classifyParamTier(m.param2, 1);
    for (size_t i = 0; i < m.extraParams.size(); ++i) {
        m.extraParams[i].tier = classifyParamTier(m.extraParams[i], static_cast<int>(i + 2));
    }

    m.params.clear();
    if (m.param1.isValid()) m.params.push_back(m.param1);
    if (m.param2.isValid()) m.params.push_back(m.param2);
    for (const auto& ep : m.extraParams) {
        if (ep.isValid()) m.params.push_back(ep);
    }
}

inline const std::vector<SchematicComponentMeta>& getSchematicCatalog() {
    static const std::vector<SchematicComponentMeta> s_enrichedCatalog = []() {
        std::vector<SchematicComponentMeta> cat = getRawSchematicCatalog();
        for (auto& m : cat) {
            m.rawDesignator = m.designator;
        }

        // Sequential gap-free designator numbering for the 150 curated studio modules
        // (Pass 0 = curated 150 modules, Pass 1 = non-curated archive modules)
        int prCnt = 0, irCnt = 0, tpCnt = 0, stCnt = 0;
        int eqCnt = 0, spCnt = 0, cpCnt = 0, clCnt = 0;
        int dlCnt = 0, rvCnt = 0, spSplitCnt = 0, mdCnt = 0, cvCnt = 0;
        for (int pass = 0; pass < 2; ++pass) {
            const bool wantCurated = (pass == 0);
            for (auto& m : cat) {
                if (isCuratedStudioComponent(m) != wantCurated) continue;
                const std::string& rd = m.rawDesignator;
                if (rd.rfind("SP_", 0) == 0) {
                    m.designator = std::string("SP_") + static_cast<char>('A' + (spSplitCnt++));
                } else if (rd.rfind("PR", 0) == 0) {
                    m.designator = std::format("PR{}", ++prCnt);
                } else if (rd.rfind("IR", 0) == 0) {
                    m.designator = std::format("IR{}", ++irCnt);
                } else if (rd.rfind("TP", 0) == 0) {
                    m.designator = std::format("TP{}", ++tpCnt);
                } else if (rd.rfind("ST", 0) == 0) {
                    m.designator = std::format("ST{}", ++stCnt);
                } else if (rd.rfind("EQ", 0) == 0) {
                    m.designator = std::format("EQ{}", ++eqCnt);
                } else if (rd.rfind("SP", 0) == 0) {
                    m.designator = std::format("SP{}", ++spCnt);
                } else if (rd.rfind("CP", 0) == 0) {
                    m.designator = std::format("CP{}", ++cpCnt);
                } else if (rd.rfind("CL", 0) == 0) {
                    m.designator = std::format("CL{}", ++clCnt);
                } else if (rd.rfind("DL", 0) == 0) {
                    m.designator = std::format("DL{}", ++dlCnt);
                } else if (rd.rfind("RV", 0) == 0) {
                    m.designator = std::format("RV{}", ++rvCnt);
                } else if (rd.rfind("MD", 0) == 0) {
                    m.designator = std::format("MD{}", ++mdCnt);
                } else if (rd.rfind("CV", 0) == 0) {
                    m.designator = std::format("CV{}", ++cvCnt);
                }
            }
        }

        std::set<std::string> seenManifestSignatures;
        for (auto& m : cat) {
            enrichAndValidateComponentGrammar(m);
            std::string sig;
            for (size_t p = 0; p < m.getParamCount(); ++p) {
                const auto& ps = m.getParamSpec(static_cast<int>(p));
                sig += std::format("{}:{}:{:.1f}:{:.1f}|", ps.name, ps.unit, ps.minVal, ps.maxVal);
            }
            if (!seenManifestSignatures.insert(sig).second && !m.extraParams.empty()) {
                // Disambiguate with circuit-specific designator & schematic tuning offset
                auto& lastP = m.extraParams.back();
                lastP.name  = std::format("{} ({})", lastP.name, m.designator);
                if (!m.params.empty()) {
                    m.params.back().name = lastP.name;
                }
            }
        }
        return cat;
    }();
    return s_enrichedCatalog;
}

inline const SchematicComponentMeta& getComponentMeta(SchematicComponentType type) {
    const auto& cat = getSchematicCatalog();
    for (const auto& m : cat) {
        if (m.type == type) return m;
    }
    return cat.front();
}

inline const std::vector<SchematicComponentMeta>& getCuratedSchematicCatalog() {
    static const std::vector<SchematicComponentMeta> s_curatedCatalog = []() {
        std::vector<SchematicComponentMeta> out;
        for (const auto& m : getSchematicCatalog()) {
            if (isCuratedStudioComponent(m)) {
                out.push_back(m);
            }
        }
        auto sortKey = [](const SchematicComponentMeta& m) {
            const std::string& d = m.designator;
            int prefixRank = 99;
            int num = 0;
            if (d.rfind("SP_", 0) == 0) {
                prefixRank = 12;
                num = (d.size() >= 4) ? static_cast<int>(d[3] - 'A') : 0;
            } else {
                if (d.rfind("PR", 0) == 0)      prefixRank = 1;
                else if (d.rfind("IR", 0) == 0) prefixRank = 2;
                else if (d.rfind("TP", 0) == 0) prefixRank = 3;
                else if (d.rfind("ST", 0) == 0) prefixRank = 4;
                else if (d.rfind("EQ", 0) == 0) prefixRank = 5;
                else if (d.rfind("SP", 0) == 0) prefixRank = 6;
                else if (d.rfind("CP", 0) == 0) prefixRank = 7;
                else if (d.rfind("CL", 0) == 0) prefixRank = 8;
                else if (d.rfind("DL", 0) == 0) prefixRank = 9;
                else if (d.rfind("SH", 0) == 0) prefixRank = 10;
                else if (d.rfind("RV", 0) == 0) prefixRank = 11;
                else if (d.rfind("MD", 0) == 0) prefixRank = 13;
                else if (d.rfind("CV", 0) == 0) prefixRank = 14;
                else if (d.rfind("AU", 0) == 0) prefixRank = 15;
                if (d.size() > 2) {
                    try { num = std::stoi(d.substr(2)); } catch (...) {}
                }
            }
            return std::make_tuple(static_cast<int>(m.category), prefixRank, num);
        };
        std::sort(out.begin(), out.end(), [&](const SchematicComponentMeta& a, const SchematicComponentMeta& b) {
            return sortKey(a) < sortKey(b);
        });
        return out;
    }();
    return s_curatedCatalog;
}

/**
 * @brief Authentic per-topology hardware faceplate control layouts (2, 3, 4, or 5 rotary controls).
 * Eliminates the 5-knob cookie-cutter template while keeping unused internal parameter slots
 * calibrated at unity `defaultVal` under the hood.
 */
enum class AuthenticStageControlLayout : uint8_t {
    LayoutA_2KnobClassic                = 2, // 2 rotary knobs + 1 Mode Toggle switch + STAGE IN
    LayoutB_3KnobVintagePreampTapeReverb = 3, // 3 rotary knobs + 1 Voicing switch + STAGE IN (+ optional Tube Window / Link IO)
    LayoutC_4KnobQuadDynamicsDelayMod   = 4, // 4 rotary knobs + 1 Stepped Ratio/Mode Selector + STAGE IN
    LayoutD_5ControlEqFilterNetwork     = 5  // 5 controls (Stepped Frequency selectors + continuous Gain/Q knobs) + STAGE IN
};

[[nodiscard]] inline AuthenticStageControlLayout getAuthenticStageControlLayout(SchematicComponentType type) noexcept {
    // Layout D — 5-Control Multi-Band EQ / Filter Network (numKnobs = 5)
    switch (type) {
        case SchematicComponentType::InductorEq3Band:
        case SchematicComponentType::PassivePultecEq:
        case SchematicComponentType::BaxandallTone:
        case SchematicComponentType::ResonantFilter:
        case SchematicComponentType::ProportionalQGyratorEq:
        case SchematicComponentType::ParallelPassiveLcEq:
        case SchematicComponentType::FulcrumTiltAirEq:
        case SchematicComponentType::TransistorLadderFilter:
        case SchematicComponentType::SidechainSpectralCarver32:
        case SchematicComponentType::SpectralResonanceSuppressor:
        case SchematicComponentType::BarkAuditoryUnmasker:
        case SchematicComponentType::ResonantSubSculptor:
            return AuthenticStageControlLayout::LayoutD_5ControlEqFilterNetwork;
        default:
            break;
    }

    // Layout A — 2-Knob Classic Studio Layout (numKnobs = 2, + 1 Mode Toggle)
    switch (type) {
        case SchematicComponentType::OptoCompressorT4B:
        case SchematicComponentType::BritishPhotoOpticalComp:
        case SchematicComponentType::SolidStateT4BLeveler:
        case SchematicComponentType::JfetBuffer:
        case SchematicComponentType::OversampledVariableClipper:
        case SchematicComponentType::AsymmetricalDiodeClipper:
        case SchematicComponentType::ZenerShuntLimiter:
        case SchematicComponentType::TransientTonalSplitter:
        case SchematicComponentType::ParallelSplitBus:
        case SchematicComponentType::MidSideImager:
        case SchematicComponentType::MonoCompatibleWidener:
        case SchematicComponentType::HeadroomCalibratorPre:
        case SchematicComponentType::ConsoleSummingBusAmp:
        case SchematicComponentType::VariImpedanceCableLoadPre:
        case SchematicComponentType::SidechainThrustMatrix:
            return AuthenticStageControlLayout::LayoutA_2KnobClassic;
        default:
            break;
    }

    // Layout B — 3-Knob Vintage Preamp / Tape / Reverb Layout (numKnobs = 3, + 1 Voicing Switch)
    switch (type) {
        case SchematicComponentType::ClassAPreamp:
        case SchematicComponentType::ClassABOpAmp2520:
        case SchematicComponentType::Tube12AX7:
        case SchematicComponentType::PentodeEF86:
        case SchematicComponentType::DirectHeatedTriode:
        case SchematicComponentType::InputTransformer:
        case SchematicComponentType::OutputTransformer:
        case SchematicComponentType::TapeHysteresis:
        case SchematicComponentType::PlateReverb:
        case SchematicComponentType::DispersionSpringReverb:
        case SchematicComponentType::HallReverbFDN:
        case SchematicComponentType::OctaveShimmerReverb:
        case SchematicComponentType::NonlinearGatedReverb:
            return AuthenticStageControlLayout::LayoutB_3KnobVintagePreampTapeReverb;
        default:
            break;
    }

    // Layout C — 4-Knob Quad + Stepped Ratio/Mode Selector (numKnobs = 4, + 1 Stepped Selector/Button)
    switch (type) {
        case SchematicComponentType::FetCompressor1176:
        case SchematicComponentType::VcaBusCompressor:
        case SchematicComponentType::VariMuTubeCompressor:
        case SchematicComponentType::DiodeBridgeCompressor:
        case SchematicComponentType::DualOptoVcaCompressor:
        case SchematicComponentType::TransientShaper:
        case SchematicComponentType::BrickwallLimiter:
        case SchematicComponentType::HysteresisNoiseGate:
        case SchematicComponentType::SplitBandDeEsser:
        case SchematicComponentType::UpDownMultibandCompressor:
        case SchematicComponentType::TapeEcho:
        case SchematicComponentType::PingPongDelay:
        case SchematicComponentType::BbdCompanderDelay:
        case SchematicComponentType::MicroPitchHarmonizer:
        case SchematicComponentType::BbdChorus:
        case SchematicComponentType::AnalogPhaser:
        case SchematicComponentType::ThroughZeroTapeFlanger:
        case SchematicComponentType::OpticalHarmonicTremolo:
        case SchematicComponentType::DiodeRingSubOctaveMod:
            return AuthenticStageControlLayout::LayoutC_4KnobQuadDynamicsDelayMod;
        default:
            break;
    }

    // Classify remaining catalog modules by category and designator prefix
    const auto& meta = getComponentMeta(type);
    const std::string& d = meta.rawDesignator.empty() ? meta.designator : meta.rawDesignator;

    if (meta.category == SchematicCategory::EqAndFilters ||
        d.rfind("EQ", 0) == 0 || (d.rfind("SP", 0) == 0 && d.rfind("SP_", 0) != 0)) {
        return AuthenticStageControlLayout::LayoutD_5ControlEqFilterNetwork;
    }
    if (d.rfind("CL", 0) == 0 || d.rfind("SP_", 0) == 0) {
        return AuthenticStageControlLayout::LayoutA_2KnobClassic;
    }
    if (meta.category == SchematicCategory::PreampsAndTubes ||
        meta.category == SchematicCategory::IronAndTape ||
        d.rfind("RV", 0) == 0 || d == "SH1") {
        return AuthenticStageControlLayout::LayoutB_3KnobVintagePreampTapeReverb;
    }
    return AuthenticStageControlLayout::LayoutC_4KnobQuadDynamicsDelayMod;
}

/**
 * @brief Secondary Filter Pill Bar predicate for each of the 6 Component Library tabs.
 * subFilterIdx: 0 = All, 1..4 = Category-specific sub-archetype filter pills.
 */
[[nodiscard]] inline bool matchesSchematicSubFilter(const SchematicComponentMeta& m, int subFilterIdx) noexcept {
    if (subFilterIdx <= 0) return true;
    const std::string& d = m.rawDesignator.empty() ? m.designator : m.rawDesignator;

    switch (m.category) {
        case SchematicCategory::PreampsAndTubes: // 36 modules (PR1..PR36)
            if (subFilterIdx == 1) { // Class-A & Console
                return d == "PR1" || d == "PR2" || d == "PR3" || d == "PR8" || d == "PR9" ||
                       d == "PR17" || d == "PR18" || d == "PR19" || d == "PR20" || d == "PR21" ||
                       d == "PR23" || d == "PR24" || d == "PR25" || d == "PR26" || d == "PR35";
            }
            if (subFilterIdx == 2) { // Broadcast Valve
                return d == "PR4" || d == "PR5" || d == "PR6" || d == "PR7" || d == "PR12" ||
                       d == "PR13" || d == "PR14" || d == "PR27" || d == "PR28" ||
                       d == "PR31" || d == "PR32" || d == "PR34";
            }
            if (subFilterIdx == 3) { // Mastering Transfer
                return d == "PR10" || d == "PR11" || d == "PR22" || d == "PR29" || d == "PR30" || d == "PR33";
            }
            if (subFilterIdx == 4) { // Summing & Utility
                return d == "PR15" || d == "PR16" || d == "PR36";
            }
            break;

        case SchematicCategory::IronAndTape: // 12 curated modules (IR1..3, TP1/3/4, ST1..6)
            if (subFilterIdx == 1) return d.rfind("IR", 0) == 0; // Studio Transformers
            if (subFilterIdx == 2) { // Mastering Tape
                return d == "TP1" || d == "TP2" || d == "TP3" || d == "TP4" || d == "TP5" || d == "TP7" || d == "TP9";
            }
            if (subFilterIdx == 3) { // Tube & Harmonic
                return d == "ST1" || d == "ST2" || d == "ST3" || d == "ST4" || d == "ST7" ||
                       d == "ST10" || d == "ST11" || d == "ST12" || d == "ST15" || d == "ST16" || d == "ST18";
            }
            if (subFilterIdx == 4) { // Exciters & Sub Synth
                return d == "ST5" || d == "ST6" || d == "TP6" || d == "TP8" || d == "ST8" || d == "ST9" || d == "ST13" || d == "ST17";
            }
            break;

        case SchematicCategory::EqAndFilters: // 36 modules (EQ1..EQ23, SP1..SP13)
            if (subFilterIdx == 1) { // Passive & Inductor
                return d == "EQ1" || d == "EQ2" || d == "EQ3" || d == "EQ4" ||
                       d == "EQ11" || d == "EQ12" || d == "EQ13" || d == "EQ14" || d == "EQ15" ||
                       d == "EQ21" || d == "EQ23";
            }
            if (subFilterIdx == 2) { // Console & Mastering
                return d == "EQ5" || d == "EQ6" || d == "EQ7" || d == "EQ8" ||
                       d == "EQ10" || d == "EQ16" || d == "EQ17" || d == "EQ18" || d == "EQ19" || d == "EQ22";
            }
            if (subFilterIdx == 3) { // Spectral FFT
                return d == "SP1" || d == "SP3" || d == "SP4" ||
                       d == "SP5" || d == "SP7" || d == "SP8" || d == "SP10" ||
                       d == "SP11" || d == "SP12";
            }
            if (subFilterIdx == 4) { // Elliptical & Phase
                return d == "EQ20" || d == "SP6" || d == "SP9" || d == "SP13";
            }
            break;

        case SchematicCategory::AnalogDynamics: // 36 modules (CP1..CP29, CL1..CL7)
            if (subFilterIdx == 1) { // FET & Optical
                return d == "CP1" || d == "CP2" || d == "CP7" || d == "CP17" || d == "CP20" || d == "CP22";
            }
            if (subFilterIdx == 2) { // Vari-Mu, Tube & Zener
                return d == "CP3" || d == "CP4" || d == "CP14" || d == "CP15" || d == "CP27";
            }
            if (subFilterIdx == 3) { // VCA & Bus Glue
                return d == "CP5" || d == "CP6" || d == "CP8" || d == "CP9" ||
                       d == "CP16" || d == "CP18" || d == "CP19" || d == "CP21" ||
                       d == "CP26" || d == "CP28" || d == "CP29";
            }
            if (subFilterIdx == 4) { // De-Ess, Gate & Clip
                return d == "CP10" || d == "CP11" || d == "CP12" || d == "CP13" || d == "CP24" ||
                       d.rfind("CL", 0) == 0;
            }
            break;

        case SchematicCategory::DelayAndReverb: // 36 modules (DL1..DL17, SH1, RV1..RV18)
            if (subFilterIdx == 1) { // Tape & Mechanical
                return d == "DL1" || d == "DL3" || d == "DL5" || d == "DL10" || d == "DL14" || d == "DL15" || d == "DL16";
            }
            if (subFilterIdx == 2) { // Rack Digital Delays
                return d == "DL2" || d == "DL4" || d == "DL6" || d == "DL7" || d == "DL8" ||
                       d == "DL9" || d == "DL11" || d == "DL12" || d == "DL13" || d == "DL17" || d == "SH1";
            }
            if (subFilterIdx == 3) { // Plates & Springs
                return d == "RV1" || d == "RV3" || d == "RV5" || d == "RV12" || d == "RV13" || d == "RV16";
            }
            if (subFilterIdx == 4) { // Chambers, Rooms & Halls
                return d == "RV2" || d == "RV4" || d == "RV6" || d == "RV7" || d == "RV8" ||
                       d == "RV9" || d == "RV10" || d == "RV11" || d == "RV14" ||
                       d == "RV15" || d == "RV17" || d == "RV18";
            }
            break;

        case SchematicCategory::ModulationAndRoute: // 11 curated modules (SP_A..D, MD1..6, MD8)
            if (subFilterIdx == 1) return d.rfind("SP_", 0) == 0; // Mix Splitters & Routers
            if (subFilterIdx == 2) { // Rack Chorus & Flange
                return d == "MD1" || d == "MD2" || d == "MD3" || d == "MD9" || d == "MD10" || d == "MD15";
            }
            if (subFilterIdx == 3) { // Rotary, Phase & Tremolo
                return d == "MD4" || d == "MD6" || d == "MD7" || d == "MD8" ||
                       d == "MD11" || d == "MD12" || d == "MD13" || d == "MD14" || d == "MD16" || d == "MD17";
            }
            if (subFilterIdx == 4) return d == "MD5" || d.rfind("CV", 0) == 0;  // Stereo Widener & Utility
            break;

        case SchematicCategory::AutoAndSmart: // 5 modules (AU1..AU5)
            if (subFilterIdx == 1) return d == "AU1" || d == "AU2"; // Level Riders (2)
            if (subFilterIdx == 2) return d == "AU3";               // Dynamic EQ (1)
            if (subFilterIdx == 3) return d == "AU4";               // Resonance (1)
            if (subFilterIdx == 4) return d == "AU5";               // Gain Staging (1)
            break;
    }
    return true;
}

/**
 * @brief Source-Type Filter for the Component Library so users can quickly filter
 * components suitable for Bass, Guitar, Drums, Vocals, or Keys/Synths.
 * Universal components (kSourceUniversal) match every source filter.
 */
enum class SchematicSourceFilter : int {
    All    = 0,
    Bass   = 1,
    Guitar = 2,
    Drums  = 3,
    Vocals = 4,
    Keys   = 5
};

enum SchematicSourceMask : uint32_t {
    kSourceNone      = 0u,
    kSourceBass      = 1u << 0,
    kSourceGuitar    = 1u << 1,
    kSourceDrums     = 1u << 2,
    kSourceVocals    = 1u << 3,
    kSourceKeys      = 1u << 4,
    kSourceUniversal = kSourceBass | kSourceGuitar | kSourceDrums | kSourceVocals | kSourceKeys
};

[[nodiscard]] inline const char* getSchematicSourceFilterLabel(SchematicSourceFilter srcFilter) noexcept {
    switch (srcFilter) {
        case SchematicSourceFilter::All:    return "ALL";
        case SchematicSourceFilter::Bass:   return "BASS";
        case SchematicSourceFilter::Guitar: return "GUITAR";
        case SchematicSourceFilter::Drums:  return "DRUMS";
        case SchematicSourceFilter::Vocals: return "VOCALS";
        case SchematicSourceFilter::Keys:   return "KEYS";
    }
    return "ALL";
}

/**
 * @brief Returns the bitmask of studio instrument/source types a component is voiced for.
 * Components returning kSourceUniversal apply to all sources universally and appear
 * in any source filter (Bass, Guitar, Drums, Vocals, Keys).
 */
[[nodiscard]] inline uint32_t getSchematicComponentSourceMask(const SchematicComponentMeta& m) noexcept {
    const std::string& d = m.rawDesignator.empty() ? m.designator : m.rawDesignator;

    switch (m.category) {
        case SchematicCategory::PreampsAndTubes:
            // Universal bus, mastering & utility line stages
            if (d == "PR11" || d == "PR15" || d == "PR16" || d == "PR21" || d == "PR30") return kSourceUniversal;
            if (d == "PR1")  return kSourceBass | kSourceGuitar | kSourceKeys;
            if (d == "PR2")  return kSourceBass | kSourceGuitar | kSourceDrums | kSourceVocals;
            if (d == "PR3")  return kSourceGuitar | kSourceDrums | kSourceBass;
            if (d == "PR4")  return kSourceBass | kSourceVocals | kSourceKeys;
            if (d == "PR5")  return kSourceGuitar | kSourceBass | kSourceDrums;
            if (d == "PR6")  return kSourceBass | kSourceVocals | kSourceKeys;
            if (d == "PR7")  return kSourceBass | kSourceGuitar | kSourceDrums;
            if (d == "PR8")  return kSourceGuitar | kSourceDrums;
            if (d == "PR9")  return kSourceDrums | kSourceKeys;
            if (d == "PR10") return kSourceGuitar | kSourceDrums | kSourceVocals;
            if (d == "PR12") return kSourceVocals;
            if (d == "PR13") return kSourceBass | kSourceGuitar | kSourceVocals | kSourceKeys;
            if (d == "PR14") return kSourceVocals | kSourceKeys | kSourceGuitar;
            if (d == "PR17") return kSourceGuitar | kSourceVocals | kSourceDrums;
            if (d == "PR18") return kSourceGuitar | kSourceVocals | kSourceDrums;
            if (d == "PR19") return kSourceGuitar | kSourceBass | kSourceKeys;
            if (d == "PR20") return kSourceBass | kSourceDrums;
            if (d == "PR22") return kSourceDrums | kSourceKeys | kSourceVocals;
            if (d == "PR23") return kSourceVocals | kSourceGuitar;
            if (d == "PR24") return kSourceDrums | kSourceGuitar;
            if (d == "PR25") return kSourceBass | kSourceDrums | kSourceGuitar;
            if (d == "PR26") return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "PR27") return kSourceVocals | kSourceKeys;
            if (d == "PR28") return kSourceVocals | kSourceKeys;
            if (d == "PR29") return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "PR31") return kSourceGuitar | kSourceKeys | kSourceVocals;
            if (d == "PR32") return kSourceGuitar | kSourceBass | kSourceVocals;
            if (d == "PR33") return kSourceDrums | kSourceKeys;
            if (d == "PR34") return kSourceVocals | kSourceKeys;
            if (d == "PR35") return kSourceGuitar | kSourceDrums;
            if (d == "PR36") return kSourceBass | kSourceGuitar;
            return kSourceUniversal;

        case SchematicCategory::IronAndTape:
            // Universal master iron, multitrack/mastering tape & full-spectrum harmonic processors
            if (d == "IR4" || d == "IR10" || d == "TP1" || d == "TP4" ||
                d == "ST1" || d == "ST3"  || d == "ST10" || d == "ST12" || d == "ST15") {
                return kSourceUniversal;
            }
            if (d == "IR1")  return kSourceBass | kSourceDrums;
            if (d == "IR2")  return kSourceVocals | kSourceKeys;
            if (d == "IR3")  return kSourceGuitar | kSourceDrums;
            if (d == "IR5")  return kSourceGuitar | kSourceVocals;
            if (d == "IR6")  return kSourceGuitar | kSourceVocals | kSourceKeys;
            if (d == "IR7")  return kSourceBass | kSourceVocals | kSourceKeys;
            if (d == "IR8")  return kSourceBass | kSourceGuitar | kSourceDrums;
            if (d == "IR9")  return kSourceBass | kSourceKeys | kSourceDrums;
            if (d == "TP2")  return kSourceGuitar | kSourceDrums | kSourceVocals;
            if (d == "TP3")  return kSourceGuitar | kSourceKeys;
            if (d == "TP5")  return kSourceBass | kSourceDrums | kSourceGuitar;
            if (d == "TP6")  return kSourceVocals | kSourceKeys;
            if (d == "TP7")  return kSourceGuitar | kSourceVocals;
            if (d == "TP8")  return kSourceVocals | kSourceKeys;
            if (d == "TP9")  return kSourceVocals | kSourceKeys;
            if (d == "ST2")  return kSourceBass | kSourceGuitar | kSourceDrums | kSourceKeys;
            if (d == "ST4")  return kSourceBass;
            if (d == "ST5")  return kSourceVocals | kSourceKeys | kSourceDrums;
            if (d == "ST6")  return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "ST7")  return kSourceBass | kSourceGuitar | kSourceDrums;
            if (d == "ST8")  return kSourceDrums | kSourceKeys;
            if (d == "ST9")  return kSourceKeys | kSourceGuitar | kSourceDrums;
            if (d == "ST11") return kSourceDrums | kSourceVocals | kSourceGuitar;
            if (d == "ST13") return kSourceDrums | kSourceKeys;
            if (d == "ST16") return kSourceBass | kSourceDrums;
            if (d == "ST17") return kSourceDrums | kSourceKeys;
            if (d == "ST18") return kSourceGuitar | kSourceKeys | kSourceVocals;
            return kSourceUniversal;

        case SchematicCategory::EqAndFilters:
            // Universal full-range console, mastering & spectral unmasking EQs
            if (d == "EQ3"  || d == "EQ4"  || d == "EQ5"  || d == "EQ8"  ||
                d == "EQ16" || d == "EQ17" || d == "EQ19" || d == "EQ22" || d == "EQ23" ||
                d == "SP1"  || d == "SP4"  || d == "SP8") {
                return kSourceUniversal;
            }
            if (d == "EQ1")  return kSourceBass | kSourceDrums;
            if (d == "EQ2")  return kSourceGuitar | kSourceVocals | kSourceKeys;
            if (d == "EQ6")  return kSourceDrums | kSourceGuitar | kSourceBass;
            if (d == "EQ7")  return kSourceBass | kSourceGuitar | kSourceDrums;
            if (d == "EQ10") return kSourceBass | kSourceKeys | kSourceGuitar;
            if (d == "EQ11") return kSourceGuitar | kSourceVocals | kSourceKeys;
            if (d == "EQ12") return kSourceGuitar | kSourceVocals;
            if (d == "EQ13") return kSourceBass | kSourceGuitar | kSourceDrums | kSourceVocals;
            if (d == "EQ14") return kSourceBass | kSourceGuitar | kSourceDrums | kSourceKeys;
            if (d == "EQ15") return kSourceGuitar | kSourceDrums | kSourceVocals;
            if (d == "EQ18") return kSourceVocals | kSourceKeys;
            if (d == "EQ20") return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "EQ21") return kSourceBass | kSourceDrums;
            if (d == "SP3")  return kSourceBass | kSourceGuitar | kSourceDrums;
            if (d == "SP5")  return kSourceBass | kSourceVocals | kSourceKeys | kSourceGuitar;
            if (d == "SP6")  return kSourceBass | kSourceGuitar | kSourceDrums;
            if (d == "SP7")  return kSourceGuitar | kSourceKeys;
            if (d == "SP9")  return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "SP10") return kSourceGuitar | kSourceVocals | kSourceKeys;
            if (d == "SP11") return kSourceDrums | kSourceVocals | kSourceGuitar;
            if (d == "SP12") return kSourceVocals | kSourceGuitar | kSourceKeys;
            if (d == "SP13") return kSourceBass | kSourceGuitar | kSourceDrums;
            return kSourceUniversal;

        case SchematicCategory::AnalogDynamics:
            // Universal bus compressors, vari-mu limiters & mastering clippers
            if (d == "CP3"  || d == "CP5"  || d == "CP6"  || d == "CP7"  || d == "CP9"  ||
                d == "CP19" || d == "CP21" || d == "CP27" || d == "CP28" ||
                d == "CL1"  || d == "CL2"  || d == "CL3"  || d == "CL4"  || d == "CL7") {
                return kSourceUniversal;
            }
            if (d == "CP1")  return kSourceBass | kSourceDrums | kSourceVocals | kSourceGuitar;
            if (d == "CP2")  return kSourceVocals | kSourceBass | kSourceGuitar | kSourceKeys;
            if (d == "CP4")  return kSourceBass | kSourceDrums | kSourceGuitar;
            if (d == "CP8")  return kSourceBass | kSourceDrums | kSourceGuitar;
            if (d == "CP10") return kSourceDrums | kSourceGuitar | kSourceBass;
            if (d == "CP11") return kSourceDrums | kSourceKeys | kSourceBass;
            if (d == "CP12") return kSourceVocals;
            if (d == "CP13") return kSourceGuitar | kSourceDrums | kSourceBass;
            if (d == "CP14") return kSourceVocals | kSourceGuitar | kSourceKeys;
            if (d == "CP15") return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "CP16") return kSourceDrums;
            if (d == "CP17") return kSourceBass | kSourceDrums | kSourceGuitar;
            if (d == "CP18") return kSourceBass | kSourceDrums | kSourceVocals;
            if (d == "CP20") return kSourceBass | kSourceVocals | kSourceGuitar;
            if (d == "CP22") return kSourceBass | kSourceVocals | kSourceGuitar;
            if (d == "CP24") return kSourceDrums;
            if (d == "CP26") return kSourceVocals | kSourceKeys;
            if (d == "CP29") return kSourceDrums | kSourceGuitar | kSourceBass;
            if (d == "CL5")  return kSourceBass | kSourceDrums;
            if (d == "CL6")  return kSourceBass | kSourceVocals | kSourceKeys;
            return kSourceUniversal;

        case SchematicCategory::DelayAndReverb:
            // Universal studio plates, halls, chambers & stereo delays
            if (d == "DL2"  || d == "DL11" || d == "RV1"  || d == "RV2"  || d == "RV6" ||
                d == "RV7"  || d == "RV9"  || d == "RV10" || d == "RV11") {
                return kSourceUniversal;
            }
            if (d == "DL1")  return kSourceGuitar | kSourceVocals | kSourceKeys;
            if (d == "DL3")  return kSourceGuitar | kSourceKeys | kSourceBass;
            if (d == "DL4")  return kSourceVocals | kSourceGuitar | kSourceKeys;
            if (d == "DL5")  return kSourceGuitar | kSourceKeys | kSourceVocals;
            if (d == "DL6")  return kSourceGuitar | kSourceVocals | kSourceKeys | kSourceDrums;
            if (d == "DL7")  return kSourceVocals | kSourceGuitar | kSourceKeys;
            if (d == "DL8")  return kSourceDrums | kSourceKeys | kSourceVocals;
            if (d == "DL9")  return kSourceGuitar | kSourceKeys;
            if (d == "DL10") return kSourceVocals | kSourceGuitar | kSourceKeys;
            if (d == "DL12") return kSourceGuitar | kSourceKeys | kSourceVocals;
            if (d == "DL13") return kSourceGuitar | kSourceKeys | kSourceVocals;
            if (d == "DL14") return kSourceGuitar | kSourceVocals | kSourceDrums;
            if (d == "DL15") return kSourceGuitar | kSourceKeys | kSourceVocals | kSourceBass;
            if (d == "DL16") return kSourceGuitar | kSourceKeys;
            if (d == "DL17") return kSourceDrums | kSourceKeys;
            if (d == "SH1")  return kSourceGuitar | kSourceKeys | kSourceVocals;
            if (d == "RV3")  return kSourceGuitar | kSourceKeys | kSourceDrums;
            if (d == "RV4")  return kSourceDrums;
            if (d == "RV5")  return kSourceDrums | kSourceVocals | kSourceGuitar;
            if (d == "RV8")  return kSourceKeys | kSourceGuitar | kSourceVocals;
            if (d == "RV12") return kSourceDrums | kSourceVocals;
            if (d == "RV13") return kSourceKeys | kSourceGuitar;
            if (d == "RV14") return kSourceBass | kSourceKeys | kSourceGuitar;
            if (d == "RV15") return kSourceVocals | kSourceKeys;
            if (d == "RV16") return kSourceGuitar | kSourceKeys;
            if (d == "RV17") return kSourceDrums | kSourceGuitar | kSourceKeys | kSourceVocals;
            if (d == "RV18") return kSourceKeys | kSourceGuitar;
            return kSourceUniversal;

        case SchematicCategory::ModulationAndRoute:
            // Universal signal splitters, wideners, 3D panner & macro/CV modulation brains
            if (d == "SP_A" || d == "SP_B" || d == "SP_C" || d == "SP_D" || d == "SP_E" ||
                d == "SP_G" || d == "SP_H" || d == "MD5"  || d == "MD14" ||
                d == "CV1"  || d == "CV2"  || d == "CV3"  || d == "CV4") {
                return kSourceUniversal;
            }
            if (d == "SP_F") return kSourceBass | kSourceDrums | kSourceKeys;
            if (d == "SP_I") return kSourceVocals;
            if (d == "SP_J") return kSourceVocals | kSourceGuitar;
            if (d == "MD1")  return kSourceKeys | kSourceGuitar | kSourceBass;
            if (d == "MD2")  return kSourceGuitar | kSourceKeys;
            if (d == "MD3")  return kSourceGuitar | kSourceKeys | kSourceDrums | kSourceBass;
            if (d == "MD4")  return kSourceGuitar | kSourceKeys;
            if (d == "MD6")  return kSourceGuitar | kSourceKeys;
            if (d == "MD7")  return kSourceVocals;
            if (d == "MD8")  return kSourceKeys | kSourceGuitar | kSourceVocals;
            if (d == "MD9")  return kSourceGuitar | kSourceKeys | kSourceBass;
            if (d == "MD10") return kSourceGuitar | kSourceKeys | kSourceBass;
            if (d == "MD11") return kSourceKeys | kSourceGuitar | kSourceBass;
            if (d == "MD12") return kSourceBass | kSourceGuitar | kSourceKeys;
            if (d == "MD13") return kSourceVocals | kSourceKeys;
            if (d == "MD15") return kSourceVocals | kSourceGuitar | kSourceKeys;
            if (d == "MD16") return kSourceKeys | kSourceGuitar | kSourceDrums;
            if (d == "MD17") return kSourceKeys | kSourceGuitar;
            if (d == "CV5")  return kSourceKeys | kSourceGuitar;
            if (d == "CV6")  return kSourceDrums | kSourceBass;
            if (d == "CV7")  return kSourceKeys | kSourceGuitar;
            if (d == "CV8")  return kSourceBass | kSourceGuitar | kSourceKeys | kSourceVocals;
            return kSourceUniversal;

        case SchematicCategory::AutoAndSmart:
            if (d == "AU1") return kSourceVocals;
            if (d == "AU2") return kSourceBass;
            return kSourceUniversal; // AU3, AU4, AU5 apply universally
    }
    return kSourceUniversal;
}

[[nodiscard]] inline bool matchesSchematicSourceFilter(const SchematicComponentMeta& m,
                                                       SchematicSourceFilter srcFilter) noexcept {
    if (srcFilter == SchematicSourceFilter::All) return true;
    const uint32_t mask = getSchematicComponentSourceMask(m);
    switch (srcFilter) {
        case SchematicSourceFilter::Bass:   return (mask & kSourceBass) != 0u;
        case SchematicSourceFilter::Guitar: return (mask & kSourceGuitar) != 0u;
        case SchematicSourceFilter::Drums:  return (mask & kSourceDrums) != 0u;
        case SchematicSourceFilter::Vocals: return (mask & kSourceVocals) != 0u;
        case SchematicSourceFilter::Keys:   return (mask & kSourceKeys) != 0u;
        default:                            return true;
    }
}

[[nodiscard]] inline std::string formatComponentSourceBadgeText(const SchematicComponentMeta& m, bool shortForm = false) {
    const uint32_t mask = getSchematicComponentSourceMask(m);
    if ((mask & kSourceUniversal) == kSourceUniversal) {
        return shortForm ? "ALL SOURCES" : "UNIVERSAL (ALL SOURCES)";
    }
    std::string out;
    auto appendTag = [&](uint32_t bit, const char* tag) {
        if ((mask & bit) != 0u) {
            if (!out.empty()) out += " \xC2\xB7 ";
            out += tag;
        }
    };
    appendTag(kSourceBass,   "BASS");
    appendTag(kSourceGuitar, shortForm ? "GTR" : "GUITAR");
    appendTag(kSourceDrums,  "DRUMS");
    appendTag(kSourceVocals, shortForm ? "VOC" : "VOCALS");
    appendTag(kSourceKeys,   "KEYS");
    return out.empty() ? "ALL SOURCES" : out;
}

/**
 * @brief Returns the formatted label + module count for a Secondary Filter Pill (index 0..4).
 */
[[nodiscard]] inline std::string getSchematicSubFilterLabel(
    SchematicCategory cat, int subFilterIdx, SchematicSourceFilter srcFilter = SchematicSourceFilter::All) {
    const char* baseLabel = "All";
    switch (cat) {
        case SchematicCategory::PreampsAndTubes:
            if (subFilterIdx == 1) baseLabel = "Class-A & Console";
            else if (subFilterIdx == 2) baseLabel = "Broadcast Valve";
            else if (subFilterIdx == 3) baseLabel = "Mastering Transfer";
            else if (subFilterIdx == 4) baseLabel = "Summing & Utility";
            break;
        case SchematicCategory::IronAndTape:
            if (subFilterIdx == 1) baseLabel = "Studio Transformers";
            else if (subFilterIdx == 2) baseLabel = "Mastering Tape";
            else if (subFilterIdx == 3) baseLabel = "Tube & Harmonic";
            else if (subFilterIdx == 4) baseLabel = "Lathe & Compander";
            break;
        case SchematicCategory::EqAndFilters:
            if (subFilterIdx == 1) baseLabel = "Passive & Inductor";
            else if (subFilterIdx == 2) baseLabel = "Console & Mastering";
            else if (subFilterIdx == 3) baseLabel = "Spectral FFT";
            else if (subFilterIdx == 4) baseLabel = "Elliptical & Phase";
            break;
        case SchematicCategory::AnalogDynamics:
            if (subFilterIdx == 1) baseLabel = "FET & Optical";
            else if (subFilterIdx == 2) baseLabel = "Vari-Mu, Tube & Zener";
            else if (subFilterIdx == 3) baseLabel = "VCA & Bus Glue";
            else if (subFilterIdx == 4) baseLabel = "De-Ess, Gate & Clip";
            break;
        case SchematicCategory::DelayAndReverb:
            if (subFilterIdx == 1) baseLabel = "Tape & Mechanical";
            else if (subFilterIdx == 2) baseLabel = "Rack Digital Delays";
            else if (subFilterIdx == 3) baseLabel = "Plates & Springs";
            else if (subFilterIdx == 4) baseLabel = "Chambers, Rooms & Halls";
            break;
        case SchematicCategory::ModulationAndRoute:
            if (subFilterIdx == 1) baseLabel = "Mix Splitters & Routers";
            else if (subFilterIdx == 2) baseLabel = "Rack Chorus & Flange";
            else if (subFilterIdx == 3) baseLabel = "Rotary, Phase & 3D";
            else if (subFilterIdx == 4) baseLabel = "Widener & CV";
            break;
        case SchematicCategory::AutoAndSmart:
            if (subFilterIdx == 1) baseLabel = "Auto Levellers";
            else if (subFilterIdx == 2) baseLabel = "Dynamic EQ";
            else if (subFilterIdx == 3) baseLabel = "Resonance";
            else if (subFilterIdx == 4) baseLabel = "Spectral Tilt";
            break;
    }

    int count = 0;
    for (const auto& m : getCuratedSchematicCatalog()) {
        if (m.category == cat &&
            matchesSchematicSubFilter(m, subFilterIdx) &&
            matchesSchematicSourceFilter(m, srcFilter)) {
            ++count;
        }
    }
    return std::format("{} ({})", baseLabel, count);
}

// =========================================================================
// Ecosystem Orchestration Engine: Smart Plumbing Rules (RULE_01..RULE_08)
// & Internal Circuit Calibration Trims
// =========================================================================
struct AutoPlumbingSpec {
    bool injectDcGuard{false};               // RULE_01: 5 Hz 1st-order HPF UTIL_DC_BLOCKER after asymmetrical wave-shapers
    bool enableComponentDrift{true};         // RULE_02: Seeded L/R component tolerance variance + -90 dBFS thermal floor
    float defaultDriftVariancePct{1.5f};     // Default L/R component tolerance variance (1.0% .. 2.5%)
    float thermalNoiseFloorDb{-90.0f};       // Thermal bias floor (-90 dBFS)
    bool eligibleForOsIsland{false};         // RULE_03: Consolidate adjacent non-linear blocks into shared OS island
    int recommendedOsFactor{2};              // 1x, 2x, or 4x oversampling
    const char* pairedDetectorName{""};      // RULE_04: Native Detector Brain-Pairing (e.g. "DET_PEAK_1176 [Feedback]")
    float passiveInsertionMakeupDb{0.0f};    // RULE_05: Passive RLC insertion loss unity makeup (+14..+16 dB)
    bool supportsAutoGainLink{true};         // RULE_06: Inverse Drive/Output coupling + Faceplate "LINK IO" toggle
    int linkedOutputParamSlot{-1};           // Slot of output/makeup parameter if present (-1 = use internal stage trim)
    float inverseLinkRatio{-0.65f};          // Ratio applied when Drive is turned up
    int baseLatencySamples{0};               // RULE_07: Parallel Phase & Latency Sync
    bool wrapsDelayCompander{false};         // RULE_08: NE570 2:1 RMS Comp -> Delay -> 1:2 Expander wrapper
    bool injectSafetySubsonicClamp{false};   // 15 Hz infrasonic guard + +18 dBFS safety soft-limiter & NaN/Inf clamp
    uint32_t smartPlumbingFlags{0u};         // Bitmask of active SmartPlumbingFlags
    ComponentParamSpec internalTrim1{"Q-Bias Cal", "%", -50.0f, 50.0f, 0.0f, false};  // Slot 5 (Hidden by default)
    ComponentParamSpec internalTrim2{"Analog Drift", "%", 0.0f, 5.0f, 1.5f, false};   // Slot 6 (Hidden by default)
};

[[nodiscard]] inline SchematicElectronicSymbol getElectronicSymbolType(SchematicComponentType type) noexcept {
    if ((getComponentTraitTags(type) & TAG_TUBE) != 0u) {
        return SchematicElectronicSymbol::DualTriodeValve;
    }
    const auto& meta = getComponentMeta(type);
    const std::string& d = meta.rawDesignator.empty() ? meta.designator : meta.rawDesignator;
    if (d == "PR4" || d == "PR5" || d == "PR6" || d == "PR12" || d == "PR13" ||
        d == "PR14" || d == "PR27" || d == "PR28" || d == "PR31" || d == "PR32" ||
        d == "PR34" || d == "ST2" || d == "ST10" || d == "ST13" || d == "CP2" ||
        d == "CP3" || d == "CP14" || d == "CP27" || d == "DL5" || d == "MD6" || d == "MD15") {
        return SchematicElectronicSymbol::DualTriodeValve;
    }
    switch (meta.category) {
        case SchematicCategory::PreampsAndTubes:
            return SchematicElectronicSymbol::Transistor;
        case SchematicCategory::IronAndTape:
            if (d.rfind("TP", 0) == 0) return SchematicElectronicSymbol::TapeDelayHead;
            if (d.rfind("IR", 0) == 0) return SchematicElectronicSymbol::TransformerCoils;
            return SchematicElectronicSymbol::DualTriodeValve;
        case SchematicCategory::EqAndFilters:
            return SchematicElectronicSymbol::InductorCapacitorTank;
        case SchematicCategory::AnalogDynamics:
            return SchematicElectronicSymbol::DiodeBridgeVca;
        case SchematicCategory::DelayAndReverb:
            return SchematicElectronicSymbol::TapeDelayHead;
        case SchematicCategory::ModulationAndRoute:
            return SchematicElectronicSymbol::CrossoverMatrix;
        case SchematicCategory::AutoAndSmart:
            if (d == "AU3" || d == "AU4") return SchematicElectronicSymbol::InductorCapacitorTank;
            return SchematicElectronicSymbol::DiodeBridgeVca;
    }
    return SchematicElectronicSymbol::Transistor;
}

inline AutoPlumbingSpec getAutoPlumbingSpec(SchematicComponentType type) noexcept {
    AutoPlumbingSpec spec{};
    const auto& meta = getComponentMeta(type);
    const std::string& d = meta.rawDesignator.empty() ? meta.designator : meta.rawDesignator;

    // RULE_01, RULE_03, RULE_06 for Preamps, Vacuum Tubes, Iron, Tape & Saturators
    if (meta.category == SchematicCategory::PreampsAndTubes ||
        meta.category == SchematicCategory::IronAndTape) {
        spec.injectDcGuard            = true;
        spec.enableComponentDrift     = true;
        spec.defaultDriftVariancePct  = 1.8f;
        spec.thermalNoiseFloorDb      = -95.0f;
        spec.eligibleForOsIsland      = (type != SchematicComponentType::JfetBuffer &&
                                         type != SchematicComponentType::PaperInOilInterstageCap);
        spec.recommendedOsFactor      = (type == SchematicComponentType::GermaniumBroadcastPre ||
                                         type == SchematicComponentType::TrigonometricWavefolder ||
                                         type == SchematicComponentType::DirectHeatedTriode) ? 4 : 2;
        spec.supportsAutoGainLink     = true;
        spec.linkedOutputParamSlot    = -1; // Uses automatic inverse output compensation
        spec.inverseLinkRatio         = -0.60f;
        const bool isTubeStage        = ((getComponentTraitTags(type) & TAG_TUBE) != 0u);
        spec.internalTrim1            = { isTubeStage ? "Cathode Bias" : "Circuit Bias Trim", "%", -50.0f, 50.0f, 0.0f, false };
        spec.internalTrim2            = { isTubeStage ? "Tube/Iron Drift" : "Component Tolerance", "%", 0.0f, 5.0f, 1.8f, false };
    }
    // RULE_02, RULE_05 for EQs & Filters
    else if (meta.category == SchematicCategory::EqAndFilters) {
        spec.injectDcGuard            = false;
        spec.enableComponentDrift     = true;
        spec.defaultDriftVariancePct  = 2.0f;
        spec.thermalNoiseFloorDb      = -95.0f;
        spec.supportsAutoGainLink     = true;
        spec.eligibleForOsIsland      = (type == SchematicComponentType::TransistorLadderFilter ||
                                         type == SchematicComponentType::SilkBerylliumAirExciter ||
                                         type == SchematicComponentType::ResonantFilter);
        spec.recommendedOsFactor      = spec.eligibleForOsIsland ? 2 : 1;
        spec.injectSafetySubsonicClamp = (type == SchematicComponentType::ResonantFilter ||
                                          type == SchematicComponentType::TransistorLadderFilter ||
                                          d == "EQ10");
        if (d.rfind("SP", 0) == 0) {
            spec.baseLatencySamples   = 64;
        }
        if (type == SchematicComponentType::PassivePultecEq || d == "EQ1" || d == "EQ21") {
            spec.passiveInsertionMakeupDb = 16.0f; // RULE_05: Passive -16 dB RLC insertion loss + Tube makeup
            spec.injectDcGuard            = true;
            spec.internalTrim1            = { "Tube Makeup Trim", "dB", -6.0f, 6.0f, 0.0f, false };
            spec.internalTrim2            = { "PIO Cap Drift", "%", 0.0f, 5.0f, 2.5f, false };
        } else if (type == SchematicComponentType::ParallelPassiveLcEq || d == "EQ11" || d == "EQ15" || d == "EQ23") {
            spec.passiveInsertionMakeupDb = 14.0f; // RULE_05: Passive LC network makeup
            spec.injectDcGuard            = true;
            spec.internalTrim1            = { "LC Tank Trim", "dB", -6.0f, 6.0f, 0.0f, false };
            spec.internalTrim2            = { "Inductor Drift", "%", 0.0f, 5.0f, 2.0f, false };
        } else {
            spec.internalTrim1            = { "Gyrator Q Trim", "%", -50.0f, 50.0f, 0.0f, false };
            spec.internalTrim2            = { "L/R Cap Drift", "%", 0.0f, 5.0f, 1.5f, false };
        }
    }
    // RULE_01, RULE_03, RULE_04, RULE_06 for Dynamics, Compressors & Clippers
    else if (meta.category == SchematicCategory::AnalogDynamics) {
        spec.injectDcGuard            = true;
        spec.enableComponentDrift     = true;
        spec.defaultDriftVariancePct  = 1.5f;
        spec.thermalNoiseFloorDb      = -95.0f;
        spec.eligibleForOsIsland      = true;
        spec.recommendedOsFactor      = (type == SchematicComponentType::FetCompressor1176 ||
                                         type == SchematicComponentType::OversampledVariableClipper ||
                                         type == SchematicComponentType::DiodeBridgeCompressor ||
                                         type == SchematicComponentType::ZenerShuntLimiter ||
                                         d.rfind("CL", 0) == 0) ? 4 : 2;
        spec.supportsAutoGainLink     = true;
        spec.internalTrim1            = { "Q-Bias Cal", "%", -50.0f, 50.0f, 0.0f, false };
        spec.internalTrim2            = { "Cell Drift", "%", 0.0f, 5.0f, 1.5f, false };

        if (type == SchematicComponentType::FetCompressor1176) {
            spec.pairedDetectorName   = "DET_PEAK_FET [Feedback]";
            spec.linkedOutputParamSlot = 1; // Output Level
            spec.inverseLinkRatio     = -0.55f;
        } else if (type == SchematicComponentType::OptoCompressorT4B) {
            spec.pairedDetectorName   = "DET_OPTO_CELL [Feedback]";
            spec.linkedOutputParamSlot = 4; // Output Trim
            spec.inverseLinkRatio     = -0.25f;
            spec.internalTrim1        = { "Opto Memory Trim", "%", -50.0f, 50.0f, 0.0f, false };
        } else if (type == SchematicComponentType::VariMuTubeCompressor) {
            spec.pairedDetectorName   = "DET_VARIMU_6386 [Feedback]";
            spec.linkedOutputParamSlot = 7; // Output Makeup Trim
            spec.inverseLinkRatio     = -0.50f;
            spec.internalTrim1        = { "Cathode Balance", "%", -50.0f, 50.0f, 0.0f, false };
        } else if (type == SchematicComponentType::VcaBusCompressor) {
            spec.pairedDetectorName   = "DET_RMS_QUAD_VCA [Feed-Forward]";
            spec.linkedOutputParamSlot = 1; // Make-Up Gain
            spec.inverseLinkRatio     = -0.45f;
        } else if (type == SchematicComponentType::DiodeBridgeCompressor) {
            spec.pairedDetectorName   = "DET_DIODE_BRIDGE [Feedback]";
        } else if (type == SchematicComponentType::DualOptoVcaCompressor) {
            spec.pairedDetectorName   = "DET_DUAL_OPTO_VCA [Serial]";
        } else if (type == SchematicComponentType::ZenerShuntLimiter) {
            spec.pairedDetectorName   = "DET_ZENER_SHUNT [Feedback]";
        } else if (type == SchematicComponentType::SpectralResonanceSuppressor ||
                   type == SchematicComponentType::BarkAuditoryUnmasker) {
            spec.pairedDetectorName   = "DET_SPECTRAL_MEDIAN [Linear]";
            spec.baseLatencySamples   = 64; // RULE_07: Parallel latency compensation
        }
    }
    // RULE_08 & Feedback Safety for Delay & Reverb
    else if (meta.category == SchematicCategory::DelayAndReverb) {
        spec.enableComponentDrift      = true;
        spec.defaultDriftVariancePct   = 1.5f;
        spec.thermalNoiseFloorDb       = -95.0f;
        spec.supportsAutoGainLink      = true;
        spec.injectSafetySubsonicClamp = true; // 15 Hz infrasonic guard + +18 dBFS safety soft-limiter in feedback loop
        spec.wrapsDelayCompander       = (type == SchematicComponentType::BbdCompanderDelay ||
                                          type == SchematicComponentType::PingPongDelay ||
                                          type == SchematicComponentType::Companded12BitRackDelay);
        spec.injectDcGuard             = (type == SchematicComponentType::TapeEcho || spec.wrapsDelayCompander || d.rfind("DL", 0) == 0);
        spec.internalTrim1             = { "Clock / Bias Trim", "%", -50.0f, 50.0f, 0.0f, false };
        spec.internalTrim2             = { "Tank Tolerance", "%", 0.0f, 5.0f, 1.5f, false };
    }
    // Modulation, Splitters & Routing
    else {
        spec.enableComponentDrift      = true;
        spec.defaultDriftVariancePct   = 1.5f;
        spec.thermalNoiseFloorDb       = -95.0f;
        spec.supportsAutoGainLink      = true;
        if (d.rfind("SP_", 0) == 0 || d == "AU4") {
            spec.baseLatencySamples    = 32; // Parallel / Splitter phase-coherent branch delay sync
        }
        if (meta.category == SchematicCategory::AutoAndSmart) {
            spec.injectDcGuard         = true;
        }
        spec.internalTrim1             = { "Phase / Bias Trim", "%", -50.0f, 50.0f, 0.0f, false };
        spec.internalTrim2             = { "L/R Tolerance", "%", 0.0f, 5.0f, 1.5f, false };
    }

    uint32_t flags = PLUMB_NONE;
    if (spec.injectDcGuard)                 flags |= PLUMB_DC_GUARD_5HZ;
    if (spec.eligibleForOsIsland)           flags |= PLUMB_OVERSAMPLE_ADAA;
    if (spec.passiveInsertionMakeupDb > 0)  flags |= PLUMB_PASSIVE_MAKEUP;
    if (spec.enableComponentDrift)          flags |= PLUMB_DRIFT_AND_THERMAL;
    if (spec.baseLatencySamples > 0 || d.rfind("SP_", 0) == 0 || d.rfind("SP", 0) == 0) {
        flags |= PLUMB_PHASE_LATENCY_SYNC;
    }
    if (spec.injectSafetySubsonicClamp)     flags |= PLUMB_SAFETY_SUBSONIC_NAN_CLAMP;
    spec.smartPlumbingFlags = flags;

    return spec;
}

inline uint32_t SchematicNode::getSmartPlumbingFlags() const noexcept {
    uint32_t flags = getAutoPlumbingSpec(type).smartPlumbingFlags;
    if (isParallel) flags |= PLUMB_PHASE_LATENCY_SYNC;
    return flags;
}

struct StagePlumbingPlan {
    bool dcGuardActive{false};           // RULE_01
    float driftMultL{1.0f};              // RULE_02 (Left channel component tolerance multiplier)
    float driftMultR{1.0f};              // RULE_02 (Right channel component tolerance multiplier)
    float thermalFloorLinear{0.0f};      // RULE_02 (-90 dBFS thermal bias)
    int osIslandId{0};                   // RULE_03 (0 = ungrouped, >=1 = shared oversampling island ID)
    bool isIslandEntry{false};           // RULE_03 (True if this stage begins a shared oversampling island)
    bool isIslandExit{false};            // RULE_03 (True if this stage ends a shared oversampling island)
    int islandOsFactor{1};               // RULE_03 (2x or 4x)
    std::string detectorBadge{};         // RULE_04
    float passiveMakeupDb{0.0f};         // RULE_05
    bool autoGainLinkAvailable{false};   // RULE_06
    int parallelLatencyCompSamples{0};   // RULE_07
    bool companderWrapperActive{false};  // RULE_08
    std::string summaryBadges{};         // Human-readable active rule badges for UI Inspector
};

/**
 * @brief Deterministic 8-Rule "Smart Plumbing" Engine (`GraphSanitizerAndSanityEngine`).
 * Evaluates a SchematicNode chain and computes shared oversampling islands, DC blockers,
 * seeded L/R component drift, detector pairings, passive makeup, and parallel latency sync.
 */
class GraphSanitizerAndSanityEngine {
public:
    [[nodiscard]] static std::vector<StagePlumbingPlan> analyzeChain(const std::vector<SchematicNode>& chain) {
        std::vector<StagePlumbingPlan> plans(chain.size());
        int currentIslandId = 0;

        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& node = chain[i];
            const auto spec  = getAutoPlumbingSpec(node.type);
            auto& plan       = plans[i];

            // RULE_01: Asymmetrical DC Guard
            plan.dcGuardActive = spec.injectDcGuard && !node.isBypassed;

            // RULE_02: Seeded L/R Component Tolerance Drift & Thermal Floor
            const float driftPct = std::clamp(node.trim2Val, 0.0f, 5.0f) * 0.01f;
            const float seedSign = ((i % 2) == 0) ? 1.0f : -1.0f;
            plan.driftMultL         = 1.0f - 0.45f * driftPct * seedSign;
            plan.driftMultR         = 1.0f + 0.55f * driftPct * seedSign;
            plan.thermalFloorLinear = spec.enableComponentDrift ? 3.16e-5f : 0.0f; // -90 dBFS

            // RULE_03: Auto-Oversampling Islands (consolidate contiguous non-linear series stages)
            const bool canJoinIsland = spec.eligibleForOsIsland && !node.isBypassed && !node.isParallel;
            if (canJoinIsland) {
                const bool prevInIsland = (i > 0 && plans[i - 1].osIslandId > 0);
                if (!prevInIsland) {
                    ++currentIslandId;
                    plan.osIslandId     = currentIslandId;
                    plan.isIslandEntry  = true;
                    plan.islandOsFactor = spec.recommendedOsFactor;
                } else {
                    plan.osIslandId     = plans[i - 1].osIslandId;
                    plan.isIslandEntry  = false;
                    plan.islandOsFactor = std::max(plans[i - 1].islandOsFactor, spec.recommendedOsFactor);
                    for (size_t k = 0; k < i; ++k) {
                        if (plans[k].osIslandId == plan.osIslandId) {
                            plans[k].islandOsFactor = plan.islandOsFactor;
                        }
                    }
                }
                plan.isIslandExit = true;
                if (prevInIsland) {
                    plans[i - 1].isIslandExit = false;
                }
            }

            // RULE_04: Dangling Gain Cell Brain-Pairing
            plan.detectorBadge = spec.pairedDetectorName;

            // RULE_05: Passive Filter Loss & Impedance Match
            plan.passiveMakeupDb = spec.passiveInsertionMakeupDb;

            // RULE_06: Drive / Makeup Inverse Link
            plan.autoGainLinkAvailable = spec.supportsAutoGainLink;

            // RULE_07: Parallel Phase & Latency Sync
            plan.parallelLatencyCompSamples = node.isParallel ? spec.baseLatencySamples : 0;

            // RULE_08: Vintage Delay Compander Wrapper
            plan.companderWrapperActive = spec.wrapsDelayCompander;

            // Build human-readable summary badges for the Schematic Inspector
            std::string badges;
            if (plan.osIslandId > 0) {
                badges += std::format("[OS-ISLAND #{} ({}x)] ", plan.osIslandId, plan.islandOsFactor);
            }
            if (plan.dcGuardActive) {
                badges += "[DC-GUARD 5Hz] ";
            }
            if (spec.enableComponentDrift && node.trim2Val > 0.05f) {
                badges += std::format("[DRIFT {:.1f}% + THERMAL -95dB] ", node.trim2Val);
            }
            if (!plan.detectorBadge.empty()) {
                badges += std::format("[{}] ", plan.detectorBadge);
            }
            if (plan.passiveMakeupDb > 0.1f) {
                badges += std::format("[+{:.0f}dB MAKEUP] ", plan.passiveMakeupDb);
            }
            if (plan.companderWrapperActive) {
                badges += "[2:1 RMS COMPANDER] ";
            }
            if (spec.injectSafetySubsonicClamp) {
                badges += "[SAFETY SUB-SONIC & NAN CLAMP] ";
            }
            if ((node.getSmartPlumbingFlags() & PLUMB_PHASE_LATENCY_SYNC) != 0u) {
                badges += "[PHASE / LATENCY SYNC] ";
            }
            if (node.autoGainLinked) {
                badges += "[IO-LINKED] ";
            }
            plan.summaryBadges = badges;
        }
        return plans;
    }
};

inline SchematicNode makeDefaultSchematicNode(SchematicComponentType type, bool parallel = false) {
    const auto& meta = getComponentMeta(type);
    const auto  plum = getAutoPlumbingSpec(type);
    const size_t pCount = meta.getParamCount();
    std::vector<float> pVals(pCount, 0.0f);
    for (size_t i = 0; i < pCount; ++i) {
        pVals[i] = meta.getParamSpec(static_cast<int>(i)).defaultVal;
    }
    return SchematicNode{
        .type = type,
        .isParallel = parallel || (type == SchematicComponentType::ParallelSplitBus),
        .isBypassed = false,
        .parallelMixPercent = (parallel || type == SchematicComponentType::ParallelSplitBus) ? 50.0f : 100.0f,
        .param1Val = (pCount > 0) ? pVals[0] : 0.0f,
        .param2Val = (pCount > 1) ? pVals[1] : 0.0f,
        .param3Val = (pCount > 2) ? pVals[2] : 0.0f,
        .param4Val = (pCount > 3) ? pVals[3] : 0.0f,
        .param5Val = (pCount > 4) ? pVals[4] : 0.0f,
        .paramValues = std::move(pVals),
        .exposeInternalTrims = false,
        .autoGainLinked = false,
        .trim1Val = plum.internalTrim1.defaultVal,
        .trim2Val = plum.internalTrim2.defaultVal
    };
}

/**
 * @brief Saves a schematic node chain to disk so clicking the project in the workspace
 * sidebar reloads its exact component topology.
 */
inline bool saveSchematicChainToFile(const std::string& filePath, const std::vector<SchematicNode>& chain) {
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << "# Avra Studio Hardware Schematic Topology v6\n";
    for (const auto& node : chain) {
        const auto& meta = getComponentMeta(node.type);
        std::string extraValsCsv;
        for (size_t k = 5; k < node.paramValues.size(); ++k) {
            if (!extraValsCsv.empty()) extraValsCsv += ",";
            extraValsCsv += std::format("{:.4f}", node.paramValues[k]);
        }
        out << static_cast<int>(node.type) << "|"
            << meta.designator << "|"
            << (node.isParallel ? 1 : 0) << "|"
            << node.parallelMixPercent << "|"
            << node.param1Val << "|"
            << node.param2Val << "|"
            << (node.isBypassed ? 1 : 0) << "|"
            << node.param3Val << "|"
            << node.param4Val << "|"
            << node.param5Val << "|"
            << (node.exposeInternalTrims ? 1 : 0) << "|"
            << (node.autoGainLinked ? 1 : 0) << "|"
            << node.trim1Val << "|"
            << node.trim2Val << "|"
            // Columns 14..21 belonged to the retired Component Bay; neutral
            // placeholders keep the positional format readable by older builds.
            << "0|0|0|15|0.5|-110|1|0|"
            << extraValsCsv << "\n";
    }
    return true;
}

inline bool loadSchematicChainFromFile(const std::string& filePath, std::vector<SchematicNode>& outChain) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(filePath, ec)) return false;

    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    std::vector<SchematicNode> chain;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string tok;
        std::vector<std::string> parts;
        while (std::getline(ss, tok, '|')) {
            parts.push_back(tok);
        }
        if (parts.size() >= 6) {
            try {
                const int typeInt = std::stoi(parts[0]);
                const bool isPar  = (std::stoi(parts[2]) != 0);
                const float mix   = std::stof(parts[3]);
                const float p1    = std::stof(parts[4]);
                const float p2    = std::stof(parts[5]);
                const auto type   = static_cast<SchematicComponentType>(typeInt);
                SchematicNode n   = makeDefaultSchematicNode(type, isPar);
                n.parallelMixPercent = mix;
                n.param1Val = p1;
                n.param2Val = p2;
                if (parts.size() >= 7)  n.isBypassed          = (std::stoi(parts[6]) != 0);
                if (parts.size() >= 8)  n.param3Val           = std::stof(parts[7]);
                if (parts.size() >= 9)  n.param4Val           = std::stof(parts[8]);
                if (parts.size() >= 10) n.param5Val           = std::stof(parts[9]);
                if (parts.size() >= 11) n.exposeInternalTrims = (std::stoi(parts[10]) != 0);
                if (parts.size() >= 12) n.autoGainLinked      = (std::stoi(parts[11]) != 0);
                if (parts.size() >= 13) n.trim1Val            = std::stof(parts[12]);
                if (parts.size() >= 14) n.trim2Val            = std::stof(parts[13]);
                // parts[14..21]: retired Component Bay settings, ignored.
                n.syncLegacyToVector(n.paramValues.size());
                if (parts.size() >= 23 && !parts[22].empty()) {
                    std::stringstream css(parts[22]);
                    std::string ctok;
                    size_t slotIdx = 5;
                    while (std::getline(css, ctok, ',')) {
                        if (!ctok.empty()) {
                            n.setParamVal(static_cast<int>(slotIdx), std::stof(ctok));
                        }
                        ++slotIdx;
                    }
                }
                chain.push_back(n);
            } catch (...) {}
        }
    }
    if (!chain.empty()) {
        outChain = std::move(chain);
        return true;
    }
    return false;
}

inline std::vector<SchematicNode> loadProjectSchematicChain(const std::string& projectName) {
    std::vector<SchematicNode> chain;

    namespace fs = std::filesystem;
    const fs::path wsRoot    = resolveAvraWorkspaceRoot();
    const fs::path projDir   = wsRoot / "plugins" / projectName;
    const fs::path schemFile = projDir / (projectName + ".schematic");

    // 1. Try loading explicit .schematic file from plugins/<projectName>/ first
    if (loadSchematicChainFromFile(schemFile.string(), chain)) {
        return chain;
    }
    if (projectName == "Avra Gain") {
        const fs::path altSchem = wsRoot / "plugins" / "AvraGain" / "AvraGain.schematic";
        if (loadSchematicChainFromFile(altSchem.string(), chain)) {
            return chain;
        }
    }

    // 2. Fallback for built-in demo project if no saved .schematic exists
    if (projectName == "Avra Gain" || projectName == "GainPlugin" || projectName == "AvraGain") {
        SchematicNode n = makeDefaultSchematicNode(SchematicComponentType::JfetBuffer, false);
        chain.push_back(n);
        return chain;
    }

    // 3. Fallback: parse generated C++ header comments (e.g., "// --- STAGE 1 [Q1]: ... (PARALLEL) ---")
    const fs::path headerFile = projDir / (projectName + ".h");
    if (fs::exists(headerFile)) {
        std::ifstream in(headerFile);
        std::string line;
        while (std::getline(in, line)) {
            const auto stagePos = line.find("// --- STAGE ");
            if (stagePos != std::string::npos) {
                const auto lb = line.find('[', stagePos);
                const auto rb = line.find(']', stagePos);
                if (lb != std::string::npos && rb != std::string::npos && rb > lb + 1) {
                    const std::string desig = line.substr(lb + 1, rb - lb - 1);
                    const bool isPar = (line.find("(PARALLEL)") != std::string::npos);
                    for (const auto& meta : getSchematicCatalog()) {
                        if (meta.designator == desig || meta.rawDesignator == desig) {
                            chain.push_back(makeDefaultSchematicNode(meta.type, isPar));
                            break;
                        }
                    }
                }
            }
        }
        if (!chain.empty()) return chain;
    }

    // 4. Default fallback chain for any custom plugin
    chain.push_back(makeDefaultSchematicNode(SchematicComponentType::ClassAPreamp, false));
    chain.push_back(makeDefaultSchematicNode(SchematicComponentType::InductorEq3Band, false));
    chain.push_back(makeDefaultSchematicNode(SchematicComponentType::VcaBusCompressor, false));
    return chain;
}

// =============================================================================
// NON-DESTRUCTIVE 5-INSTRUMENT STARTER PRESET ENGINE
// (`0: Init / Flat`, `1: Drums`, `2: Bass`, `3: Guitar`, `4: Keys / Synth`, `5: Vocals`)
// =============================================================================

using SchematicSpec = std::vector<SchematicNode>;

inline constexpr int kInstrumentStarterPresetCount = 6;

[[nodiscard]] inline const char* getInstrumentStarterPresetName(int presetIdx) noexcept {
    static constexpr std::array<const char*, kInstrumentStarterPresetCount> kNames = {
        "Init / Flat",
        "Drums",
        "Bass",
        "Guitar",
        "Keys / Synth",
        "Vocals"
    };
    return kNames[static_cast<size_t>(std::clamp(presetIdx, 0, kInstrumentStarterPresetCount - 1))];
}

namespace detail {

[[nodiscard]] inline std::string toLowerAscii(std::string_view sv) {
    std::string out(sv);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

[[nodiscard]] inline bool containsWordCI(std::string_view haystack, std::string_view needle) {
    return toLowerAscii(haystack).find(toLowerAscii(needle)) != std::string::npos;
}

/**
 * @brief Parses a numeric value from a hardware step label (e.g. "1.6k" -> 1600.0f,
 * "0.36" with unit "kHz" -> 360.0f, "4:1" -> 4.0f, "OFF"/"OUT" -> -1.0f if isOffOut=true).
 */
[[nodiscard]] inline bool parseStepLabelNumericValue(
    const ComponentParamSpec& pSpec,
    std::string_view label,
    float& outVal,
    bool& outIsOff,
    bool& outIsAuto) noexcept {
    outVal    = 0.0f;
    outIsOff  = false;
    outIsAuto = false;

    const std::string low = toLowerAscii(label);

    // Explicit per-detent physical values (ComponentParamSpec::stepValues) take precedence
    // over parsing numbers out of the label text (needed for labels such as "MIC -50").
    if (!pSpec.stepValues.empty() && pSpec.stepValues.size() == pSpec.stepLabels.size()) {
        for (size_t i = 0; i < pSpec.stepLabels.size(); ++i) {
            if (pSpec.stepLabels[i] == label) {
                if (low == "off" || low == "out" || low == "byp" || low == "flat") {
                    outIsOff = true;
                    return false;
                }
                outVal = pSpec.stepValues[i];
                return true;
            }
        }
    }
    if (low == "off" || low == "out" || low == "byp" || low == "flat" || low.find("out ") == 0) {
        outIsOff = true;
        return false;
    }
    if (low.find("auto") != std::string::npos || low == "a1" || low == "a2" || low.find("dual-rc") != std::string::npos) {
        outIsAuto = true;
    }

    size_t pos = 0;
    while (pos < low.size() && !((low[pos] >= '0' && low[pos] <= '9') ||
           ((low[pos] == '+' || low[pos] == '-') && pos + 1 < low.size() && low[pos + 1] >= '0' && low[pos + 1] <= '9'))) {
        ++pos;
    }
    if (pos >= low.size()) {
        return false;
    }
    size_t endPos = pos;
    if (low[endPos] == '+' || low[endPos] == '-') ++endPos;
    bool hasDot = false;
    while (endPos < low.size()) {
        const char ch = low[endPos];
        if (ch >= '0' && ch <= '9') {
            ++endPos;
        } else if (ch == '.' && !hasDot) {
            hasDot = true;
            ++endPos;
        } else {
            break;
        }
    }
    try {
        outVal = std::stof(low.substr(pos, endPos - pos));
    } catch (...) {
        return false;
    }

    const bool hasKSuffix = (endPos < low.size() && low[endPos] == 'k');
    const bool unitIsKhz  = (toLowerAscii(pSpec.unit) == "khz");
    if (hasKSuffix || unitIsKhz) {
        outVal *= 1000.0f;
    } else if (toLowerAscii(pSpec.unit) == "s" && containsWordCI(pSpec.name, "release")) {
        outVal *= 1000.0f;
    }
    return true;
}

/**
 * @brief Snaps a target physical value (Hz, ms, dB, ratio) to the nearest available hardware detent
 * on a stepped control (`SteppedRotarySwitch`, `ConcentricDualKnob`, `PushButtonBank`), returning
 * the exact parameter value `val` in `[pSpec.minVal, pSpec.maxVal]` for that step index.
 */
[[nodiscard]] inline float snapParamToTarget(
    const ComponentParamSpec& pSpec,
    float targetPhysicalVal,
    bool preferOffDetent = false,
    bool preferAutoDetent = false) noexcept {
    if (pSpec.stepLabels.empty() ||
        (pSpec.controlKind != HardwareControlKind::SteppedRotarySwitch &&
         pSpec.controlKind != HardwareControlKind::ConcentricDualKnob &&
         pSpec.controlKind != HardwareControlKind::PushButtonBank)) {
        return math::clamp(targetPhysicalVal, pSpec.minVal, pSpec.maxVal);
    }

    const int numSteps = static_cast<int>(pSpec.stepLabels.size());
    const int maxIdx   = std::max(1, numSteps - 1);
    auto indexToParamVal = [&](int idx) noexcept -> float {
        const int clampedIdx = std::clamp(idx, 0, maxIdx);
        const float norm     = static_cast<float>(clampedIdx) / static_cast<float>(maxIdx);
        return pSpec.minVal + norm * (pSpec.maxVal - pSpec.minVal);
    };

    if (preferOffDetent) {
        for (int i = 0; i < numSteps; ++i) {
            float v = 0.0f; bool isOff = false, isAuto = false;
            (void)parseStepLabelNumericValue(pSpec, pSpec.stepLabels[static_cast<size_t>(i)], v, isOff, isAuto);
            if (isOff) return indexToParamVal(i);
        }
    }
    if (preferAutoDetent) {
        for (int i = 0; i < numSteps; ++i) {
            float v = 0.0f; bool isOff = false, isAuto = false;
            (void)parseStepLabelNumericValue(pSpec, pSpec.stepLabels[static_cast<size_t>(i)], v, isOff, isAuto);
            if (isAuto) return indexToParamVal(i);
        }
    }

    const bool isFreqOrTime = (toLowerAscii(pSpec.unit) == "hz" ||
                               toLowerAscii(pSpec.unit) == "khz" ||
                               toLowerAscii(pSpec.unit) == "ms" ||
                               toLowerAscii(pSpec.unit) == "s" ||
                               containsWordCI(pSpec.name, "freq") ||
                               containsWordCI(pSpec.name, "cps") ||
                               containsWordCI(pSpec.name, "kcs") ||
                               containsWordCI(pSpec.name, "cut") ||
                               containsWordCI(pSpec.name, "filter"));

    int   bestIdx  = -1;
    float bestDist = 1e18f;
    for (int i = 0; i < numSteps; ++i) {
        float stepVal = 0.0f;
        bool  isOff   = false;
        bool  isAuto  = false;
        if (!parseStepLabelNumericValue(pSpec, pSpec.stepLabels[static_cast<size_t>(i)], stepVal, isOff, isAuto)) {
            if (isOff && isFreqOrTime && targetPhysicalVal <= 22.0f) {
                return indexToParamVal(i);
            }
            continue;
        }
        float dist = 0.0f;
        if (isFreqOrTime && stepVal > 0.0f && targetPhysicalVal > 0.0f) {
            dist = std::abs(std::log2(stepVal / targetPhysicalVal));
        } else {
            dist = std::abs(stepVal - targetPhysicalVal);
        }
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx  = i;
        }
    }

    if (bestIdx >= 0) {
        return indexToParamVal(bestIdx);
    }
    return math::clamp(targetPhysicalVal, pSpec.minVal, pSpec.maxVal);
}

} // namespace detail

/**
 * @brief Applies one of the 6 non-destructive Instrument Starter Presets to `spec`:
 *   - `0`: `"Init / Flat"` (Resets all stages to calibrated hardware defaults)
 *   - `1`: `"Drums"`
 *   - `2`: `"Bass"`
 *   - `3`: `"Guitar"`
 *   - `4`: `"Keys / Synth"`
 *   - `5`: `"Vocals"`
 *
 * Strictly hands-off on Tabs 5 & 6 (`DelayAndReverb`, `RoutingAndSplitters`, `ModulationAndRoute`)
 * for presets `1..5`, and scales multi-stage Drive and EQ boosts by $1 / \sqrt{N}$.
 */
inline void applyInstrumentStarterPreset(SchematicSpec& spec, int presetIdx) {
    presetIdx = std::clamp(presetIdx, 0, kInstrumentStarterPresetCount - 1);

    // Preset 0: Init / Flat — reset every stage (including Tabs 5 & 6) to calibrated default posture
    if (presetIdx == 0) {
        for (auto& node : spec) {
            const auto& meta = getComponentMeta(node.type);
            const auto  plum = getAutoPlumbingSpec(node.type);
            const int   cnt  = static_cast<int>(meta.getParamCount());
            for (int p = 0; p < cnt; ++p) {
                node.setParamVal(p, meta.getParamSpec(p).defaultVal);
            }
            node.trim1Val           = plum.internalTrim1.defaultVal;
            node.trim2Val           = plum.internalTrim2.defaultVal;
            if (!node.isParallel) {
                node.parallelMixPercent = 100.0f;
            }
            node.isBypassed         = false;
        }
        return;
    }

    // 1.2 Chain-Aware Gain & EQ Scaling (1 / sqrt(N))
    int nSat = 0;
    int nEq  = 0;
    for (const auto& node : spec) {
        if (node.isBypassed) continue;
        const auto& meta = getComponentMeta(node.type);
        if (meta.category == SchematicCategory::PreampsAndTubes ||
            meta.category == SchematicCategory::IronAndTape) {
            ++nSat;
        } else if (meta.category == SchematicCategory::EqAndFilters) {
            ++nEq;
        }
    }
    const float satScale = 1.0f / std::sqrt(static_cast<float>(std::max(1, nSat)));
    const float eqScale  = 1.0f / std::sqrt(static_cast<float>(std::max(1, nEq)));

    // 2. Category 1 & 2 Target Table
    struct SatPresetTarget {
        float driveRangeFraction;
        float driveBoostDb;
        float hpfHz;
        float trimCompRatio;
    };
    static constexpr std::array<SatPresetTarget, 6> kSatTargets = {{
        { 0.00f, 0.0f, 20.0f,  0.00f }, // 0: Init / Flat
        { 0.40f, 3.5f, 30.0f, -0.45f }, // 1: Drums
        { 0.58f, 6.0f, 20.0f, -0.50f }, // 2: Bass
        { 0.62f, 7.0f, 80.0f, -0.55f }, // 3: Guitar
        { 0.28f, 2.0f, 50.0f, -0.35f }, // 4: Keys / Synth
        { 0.42f, 4.0f, 80.0f, -0.45f }  // 5: Vocals
    }};
    const SatPresetTarget& satT = kSatTargets[static_cast<size_t>(presetIdx)];

    // 3. Category 3 (EQ & Spectral) 4-Zone Target Table
    struct EqPresetTarget {
        float z1GainDb, z1FreqHz;
        float z2GainDb, z2FreqHz;
        float z3GainDb, z3FreqHz;
        float z4GainDb, z4FreqHz;
        float hpfHz;
        float mid3BandGainDb, mid3BandFreqHz;
    };
    static constexpr std::array<EqPresetTarget, 6> kEqTargets = {{
        {  0.0f,  80.0f,  0.0f, 350.0f,  0.0f, 2500.0f,  0.0f, 10000.0f, 20.0f,  0.0f, 1600.0f }, // 0: Init / Flat
        { +3.0f,  60.0f, -2.5f, 360.0f, +2.5f, 3800.0f, +2.0f, 12000.0f, 30.0f, -2.5f,  360.0f }, // 1: Drums
        { +3.5f,  60.0f, -2.0f, 250.0f, +2.5f,  800.0f, -2.0f, 10000.0f, 20.0f, +2.5f,  700.0f }, // 2: Bass
        { -1.0f, 100.0f, -1.5f, 350.0f, +3.0f, 2000.0f, +1.5f,  8000.0f, 80.0f, +3.0f, 1600.0f }, // 3: Guitar
        { +1.5f, 110.0f, -2.0f, 350.0f, -2.0f, 1400.0f, +3.0f, 12000.0f, 50.0f, -2.0f, 1600.0f }, // 4: Keys / Synth
        { -1.5f, 110.0f, -2.0f, 250.0f, +2.5f, 3200.0f, +3.5f, 12000.0f, 80.0f, +2.5f, 3200.0f }  // 5: Vocals
    }};
    const EqPresetTarget& eqT = kEqTargets[static_cast<size_t>(presetIdx)];
    const float z1Db   = eqT.z1GainDb * eqScale;
    const float z2Db   = eqT.z2GainDb * eqScale;
    const float z3Db   = eqT.z3GainDb * eqScale;
    const float z4Db   = eqT.z4GainDb * eqScale;
    const float mid3Db = eqT.mid3BandGainDb * eqScale;

    for (auto& node : spec) {
        const auto& meta = getComponentMeta(node.type);
        const std::string& rawDesig = meta.rawDesignator.empty() ? meta.designator : meta.rawDesignator;
        const int   pCnt = static_cast<int>(meta.getParamCount());

        // 1.1 Strictly Hands-Off on Tabs 5 & 6 (Delays, Reverbs, Splitters, Modulation & CV)
        if (meta.category == SchematicCategory::DelayAndReverb ||
            meta.category == SchematicCategory::ModulationAndRoute) {
            continue;
        }

        // Start from clean calibrated hardware defaults on Tabs 1-4 before applying instrument targets
        for (int p = 0; p < pCnt; ++p) {
            node.setParamVal(p, meta.getParamSpec(p).defaultVal);
        }

        // =====================================================================
        // CATEGORY 1 & 2: PREAMPS, TUBES, TRANSFORMERS, TAPE & SATURATORS
        // =====================================================================
        if (meta.category == SchematicCategory::PreampsAndTubes ||
            meta.category == SchematicCategory::IronAndTape) {
            const float effFrac    = satT.driveRangeFraction * satScale;
            const float effBoostDb = satT.driveBoostDb * satScale;
            const float trimDb     = satT.trimCompRatio * effBoostDb;

            for (int p = 0; p < pCnt; ++p) {
                const auto& ps = meta.getParamSpec(p);
                const std::string pLow = detail::toLowerAscii(ps.name);

                const bool isDriveParam = (p == 0 ||
                    detail::containsWordCI(pLow, "drive") ||
                    detail::containsWordCI(pLow, "input gain") ||
                    detail::containsWordCI(pLow, "mic/line gain") ||
                    detail::containsWordCI(pLow, "input level") ||
                    detail::containsWordCI(pLow, "input trim") ||
                    detail::containsWordCI(pLow, "saturation") ||
                    detail::containsWordCI(pLow, "flux") ||
                    detail::containsWordCI(pLow, "density") ||
                    detail::containsWordCI(pLow, "harmonic") ||
                    detail::containsWordCI(pLow, "pentode") ||
                    detail::containsWordCI(pLow, "triode") ||
                    detail::containsWordCI(pLow, "grit")) &&
                    !detail::containsWordCI(pLow, "output") &&
                    !detail::containsWordCI(pLow, "makeup") &&
                    !detail::containsWordCI(pLow, "blend") &&
                    !detail::containsWordCI(pLow, "mix") &&
                    !detail::containsWordCI(pLow, "freq") &&
                    !detail::containsWordCI(pLow, "mode");

                const bool isHpfParam = (detail::containsWordCI(pLow, "high-pass") ||
                                         detail::containsWordCI(pLow, "hpf") ||
                                         detail::containsWordCI(pLow, "low cut") ||
                                         detail::containsWordCI(pLow, "bass cut") ||
                                         detail::containsWordCI(pLow, "rumble"));

                const bool isOutTrimParam = (p > 0 && (detail::containsWordCI(pLow, "output") ||
                                                       detail::containsWordCI(pLow, "atten") ||
                                                       detail::containsWordCI(pLow, "fader") ||
                                                       (detail::containsWordCI(pLow, "trim") && !detail::containsWordCI(pLow, "input")) ||
                                                       detail::containsWordCI(pLow, "makeup")));

                if (isHpfParam) {
                    const bool preferOff = (satT.hpfHz <= 22.0f) || (rawDesig == "PR2" && presetIdx == 1);
                    node.setParamVal(p, detail::snapParamToTarget(ps, satT.hpfHz, preferOff, false));
                } else if (isOutTrimParam && ps.controlKind == HardwareControlKind::ContinuousKnob) {
                    if (ps.minVal < 0.0f && ps.maxVal > 0.0f) {
                        node.setParamVal(p, math::clamp(trimDb, ps.minVal, ps.maxVal));
                    } else {
                        node.setParamVal(p, math::clamp(ps.defaultVal + trimDb, ps.minVal, ps.maxVal));
                    }
                } else if (isDriveParam && ps.controlKind != HardwareControlKind::ToggleSwitch2Way &&
                           ps.controlKind != HardwareControlKind::ToggleSwitch3Way &&
                           ps.controlKind != HardwareControlKind::PushButtonLatching) {
                    if (!ps.stepLabels.empty()) {
                        const float targetDb = (ps.defaultVal >= -5.0f && ps.defaultVal <= 5.0f)
                            ? effBoostDb
                            : (ps.defaultVal + effFrac * (ps.maxVal - ps.defaultVal));
                        node.setParamVal(p, detail::snapParamToTarget(ps, targetDb));
                    } else if (detail::toLowerAscii(ps.unit) == "db" && (ps.maxVal - ps.defaultVal) >= 12.0f) {
                        node.setParamVal(p, math::clamp(ps.defaultVal + effBoostDb, ps.minVal, ps.maxVal));
                    } else {
                        const float base = std::max(ps.minVal, ps.defaultVal);
                        node.setParamVal(p, math::clamp(base + effFrac * (ps.maxVal - base), ps.minVal, ps.maxVal));
                    }
                }
            }

            if (rawDesig == "PR5" && pCnt >= 3) {
                node.setParamVal(0, 2.0f);  // 28 dB warm sweet-spot coarse voltage gain
                node.setParamVal(1, 0.0f);  // 0.0 dB fine gain trim (prevents pentode fizz)
                node.setParamVal(2, -2.0f); // -2.0 dB output pad
            } else if (rawDesig == "ST2" && pCnt >= 5) {
                node.setParamVal(0, 1.0f);  // P1 Pentode
                node.setParamVal(1, 1.0f);  // 6 kHz LPF to keep highs smooth
                node.setParamVal(2, 7.5f);  // +7.5 dB musical valve drive
                node.setParamVal(3, 52.0f); // 52% plate bias
                node.setParamVal(4, -3.0f); // -3.0 dB output level
            } else if (rawDesig == "ST1" && pCnt >= 2) {
                node.setParamVal(1, (presetIdx == 2 || presetIdx == 1) ? 2.0f : 1.0f);
            } else if (rawDesig == "ST4" && pCnt >= 3) {
                node.setParamVal(0, 4.5f);
                node.setParamVal(1, 45.0f);
                node.setParamVal(2, 160.0f);
            } else if (rawDesig == "ST6" && pCnt >= 5) {
                node.setParamVal(0, 35.0f);
                node.setParamVal(1, 4.5f);
                node.setParamVal(2, 5.5f);
                node.setParamVal(3, 1.0f); // 80 Hz crossover
                node.setParamVal(4, 0.0f);
            }
            continue;
        }

        // =====================================================================
        // CATEGORY 6: AUTO LEVELLERS & SMART GAIN-STAGERS (AU1..AU5)
        // =====================================================================
        if (meta.category == SchematicCategory::AutoAndSmart) {
            if (rawDesig == "AU1" && pCnt >= 5) {
                node.setParamVal(0, -16.0f); // Target Vocal Level
                node.setParamVal(1, 6.0f);   // Leveller Range (dB)
                node.setParamVal(2, 45.0f);  // Breath Gate Sensitivity (%)
                node.setParamVal(3, 160.0f); // Fader Speed (ms)
                node.setParamVal(4, 0.0f);   // Output Trim (dB)
                continue;
            }
            if (rawDesig == "AU2" && pCnt >= 6) {
                node.setParamVal(0, -15.0f); // Target Bass Level
                node.setParamVal(1, 6.0f);   // Leveller Range (dB)
                node.setParamVal(2, 160.0f); // Fundamental Ceiling (Hz)
                node.setParamVal(3, 60.0f);  // Fret & Slap Guard (%)
                node.setParamVal(4, 55.0f);  // Note Onset Speed (ms)
                node.setParamVal(5, 0.0f);   // Output Trim (dB)
                continue;
            }
        }

        // =====================================================================
        // CATEGORY 3: EQUALIZERS, FILTERS & SPECTRAL PROCESSORS
        // =====================================================================
        if (meta.category == SchematicCategory::EqAndFilters) {
            if (rawDesig.rfind("SP", 0) == 0) {
                if (rawDesig == "SP1") {
                    if (pCnt > 0) node.setParamVal(0, 35.0f * eqScale);
                    if (pCnt > 1) node.setParamVal(1, 35.0f * eqScale);
                    if (pCnt > 3) {
                        const float brighten = (presetIdx == 5 || presetIdx == 4) ? 15.0f : ((presetIdx == 2) ? 0.0f : 8.0f);
                        node.setParamVal(3, brighten * eqScale);
                    }
                } else if (rawDesig == "SP2" || rawDesig == "SP10") {
                    for (int p = 0; p < pCnt; ++p) {
                        const auto& ps = meta.getParamSpec(p);
                        if (detail::containsWordCI(ps.name, "depth") || detail::containsWordCI(ps.name, "suppress") ||
                            detail::containsWordCI(ps.name, "tame") || p == 0) {
                            const float span = ps.maxVal - ps.minVal;
                            const float val  = (ps.minVal < 0.0f && ps.maxVal <= 0.0f)
                                ? (ps.minVal * 0.30f * eqScale)
                                : (ps.minVal + 0.30f * span * eqScale);
                            node.setParamVal(p, math::clamp(val, ps.minVal, ps.maxVal));
                        } else if (detail::containsWordCI(ps.name, "freq") || detail::containsWordCI(ps.name, "focus")) {
                            node.setParamVal(p, math::clamp(eqT.z3FreqHz, ps.minVal, ps.maxVal));
                        }
                    }
                } else if (rawDesig == "SP3") {
                    const float subDb = (presetIdx == 1 || presetIdx == 2 || presetIdx == 3)
                        ? (3.5f * eqScale)
                        : ((presetIdx == 4) ? 0.0f : (-2.5f * eqScale));
                    const float subPct = (presetIdx == 1 || presetIdx == 2) ? (45.0f * eqScale)
                        : ((presetIdx == 3) ? (30.0f * eqScale) : ((presetIdx == 4) ? 15.0f : 0.0f));
                    if (pCnt > 0) node.setParamVal(0, subPct);
                    if (pCnt > 1) node.setParamVal(1, subPct * 0.85f);
                    for (int p = 2; p < pCnt; ++p) {
                        const auto& ps = meta.getParamSpec(p);
                        if (detail::toLowerAscii(ps.unit) == "db") {
                            node.setParamVal(p, math::clamp(subDb, ps.minVal, ps.maxVal));
                        }
                    }
                } else {
                    for (int p = 0; p < pCnt; ++p) {
                        const auto& ps = meta.getParamSpec(p);
                        if (ps.controlKind == HardwareControlKind::ContinuousKnob && detail::toLowerAscii(ps.unit) == "db" &&
                            !detail::containsWordCI(ps.name, "trim") && !detail::containsWordCI(ps.name, "thresh")) {
                            node.setParamVal(p, math::clamp(z3Db, ps.minVal, ps.maxVal));
                        }
                    }
                }
                continue;
            }

            if (rawDesig == "EQ7" || rawDesig == "EQ14") {
                const std::array<std::pair<float, float>, 4> zones = {{
                    { eqT.z1FreqHz, z1Db },
                    { eqT.z2FreqHz, z2Db },
                    { eqT.z3FreqHz, z3Db },
                    { eqT.z4FreqHz, z4Db }
                }};
                for (int p = 0; p < pCnt; ++p) {
                    const auto& ps = meta.getParamSpec(p);
                    if (ps.controlKind != HardwareControlKind::VerticalFader &&
                        !detail::containsWordCI(ps.name, "band gain")) {
                        continue;
                    }
                    float faderHz = 0.0f;
                    bool isOff = false, isAuto = false;
                    if (!detail::parseStepLabelNumericValue(ps, ps.name, faderHz, isOff, isAuto) || faderHz <= 0.0f) {
                        continue;
                    }
                    if (detail::containsWordCI(ps.name, "khz") && faderHz < 100.0f) {
                        faderHz *= 1000.0f;
                    }
                    float sumDb = 0.0f;
                    for (const auto& [zHz, zGain] : zones) {
                        const float octDist = std::log2(faderHz / std::max(20.0f, zHz));
                        sumDb += zGain * std::exp(-0.5f * (octDist / 0.68f) * (octDist / 0.68f));
                    }
                    node.setParamVal(p, math::clamp(sumDb, ps.minVal, ps.maxVal));
                }
                continue;
            }

            if (rawDesig == "EQ1") {
                const float targetCps = (presetIdx == 1 || presetIdx == 2) ? 60.0f : 100.0f;
                const float boostVal  = (presetIdx == 1 || presetIdx == 2) ? (5.0f * eqScale)
                                      : ((presetIdx == 4) ? (2.5f * eqScale) : (1.0f * eqScale));
                const float attenVal  = (presetIdx == 1 || presetIdx == 2) ? (3.0f * eqScale)
                                      : ((presetIdx == 4) ? (1.0f * eqScale) : (2.5f * eqScale));
                if (pCnt > 0) node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), targetCps));
                if (pCnt > 1) node.setParamVal(1, math::clamp(boostVal, meta.getParamSpec(1).minVal, meta.getParamSpec(1).maxVal));
                if (pCnt > 2) node.setParamVal(2, math::clamp(attenVal, meta.getParamSpec(2).minVal, meta.getParamSpec(2).maxVal));
                continue;
            }
            if (rawDesig == "EQ2") {
                const float targetKcsHz = (presetIdx == 3) ? 8000.0f : ((presetIdx == 1 || presetIdx == 2) ? 10000.0f : 12000.0f);
                const float hiBoost     = std::max(0.0f, std::max(z3Db, z4Db)) * 1.4f;
                const float hiAtten     = std::max(0.0f, -z4Db) * 1.6f;
                if (pCnt > 0) node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), targetKcsHz));
                if (pCnt > 1) node.setParamVal(1, detail::snapParamToTarget(meta.getParamSpec(1), 10000.0f));
                if (pCnt > 2) node.setParamVal(2, math::clamp(hiBoost, meta.getParamSpec(2).minVal, meta.getParamSpec(2).maxVal));
                if (pCnt > 4) node.setParamVal(4, math::clamp(hiAtten, meta.getParamSpec(4).minVal, meta.getParamSpec(4).maxVal));
                continue;
            }
            if (rawDesig == "EQ3" && pCnt >= 17) {
                const std::array<std::pair<float, float>, 4> bands = {{
                    { z1Db, eqT.z1FreqHz },
                    { z2Db, eqT.z2FreqHz },
                    { z3Db, eqT.z3FreqHz },
                    { z4Db, eqT.z4FreqHz }
                }};
                for (int b = 0; b < 4; ++b) {
                    const int baseIdx = b * 4;
                    const float gDb   = bands[static_cast<size_t>(b)].first;
                    const float fHz   = bands[static_cast<size_t>(b)].second;
                    node.setParamVal(baseIdx + 0, math::clamp(std::abs(gDb), 0.0f, 20.0f));
                    node.setParamVal(baseIdx + 1, detail::snapParamToTarget(meta.getParamSpec(baseIdx + 1), fHz));
                    node.setParamVal(baseIdx + 3, (gDb > 0.1f) ? 0.0f : ((gDb < -0.1f) ? 2.0f : 1.0f));
                }
                node.setParamVal(16, detail::snapParamToTarget(meta.getParamSpec(16), eqT.hpfHz, eqT.hpfHz <= 22.0f));
                continue;
            }
            if (rawDesig == "EQ4" && pCnt >= 5) {
                node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), eqT.z1FreqHz));
                node.setParamVal(1, detail::snapParamToTarget(meta.getParamSpec(1), eqT.mid3BandFreqHz));
                node.setParamVal(2, math::clamp(z1Db, meta.getParamSpec(2).minVal, meta.getParamSpec(2).maxVal));
                node.setParamVal(3, math::clamp(mid3Db, meta.getParamSpec(3).minVal, meta.getParamSpec(3).maxVal));
                node.setParamVal(4, math::clamp(z4Db, meta.getParamSpec(4).minVal, meta.getParamSpec(4).maxVal));
                continue;
            }
            if (rawDesig == "EQ5" && pCnt >= 6) {
                node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), eqT.z1FreqHz));
                node.setParamVal(1, detail::snapParamToTarget(meta.getParamSpec(1), z1Db));
                node.setParamVal(2, detail::snapParamToTarget(meta.getParamSpec(2), eqT.mid3BandFreqHz));
                node.setParamVal(3, detail::snapParamToTarget(meta.getParamSpec(3), mid3Db));
                node.setParamVal(4, detail::snapParamToTarget(meta.getParamSpec(4), eqT.z4FreqHz));
                node.setParamVal(5, detail::snapParamToTarget(meta.getParamSpec(5), z4Db));
                continue;
            }
            if (rawDesig == "EQ8" && pCnt >= 5) {
                node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), eqT.z4FreqHz));
                node.setParamVal(1, math::clamp(std::max(0.0f, z4Db * 1.8f), meta.getParamSpec(1).minVal, meta.getParamSpec(1).maxVal));
                node.setParamVal(2, math::clamp((z4Db - z1Db) * 0.35f, meta.getParamSpec(2).minVal, meta.getParamSpec(2).maxVal));
                node.setParamVal(4, math::clamp(z1Db, meta.getParamSpec(4).minVal, meta.getParamSpec(4).maxVal));
                continue;
            }
            if (rawDesig == "EQ11" && pCnt >= 6) {
                const float loPeakHz = (presetIdx == 3 || presetIdx == 4) ? 300.0f : 200.0f;
                const float dipHz    = (presetIdx == 1 || presetIdx == 3) ? 500.0f : 300.0f;
                const float hiPeakHz = (presetIdx == 1) ? 4000.0f : 3000.0f;
                node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), loPeakHz));
                node.setParamVal(1, detail::snapParamToTarget(meta.getParamSpec(1), dipHz));
                node.setParamVal(2, detail::snapParamToTarget(meta.getParamSpec(2), hiPeakHz));
                node.setParamVal(3, math::clamp(std::max(0.0f, z1Db) * 1.2f, 0.0f, 10.0f));
                node.setParamVal(4, math::clamp(std::max(0.0f, -z2Db) * 1.4f, 0.0f, 10.0f));
                node.setParamVal(5, math::clamp(std::max(0.0f, z3Db) * 1.3f, 0.0f, 10.0f));
                continue;
            }
            if (rawDesig == "EQ12" && pCnt >= 2) {
                node.setParamVal(0, detail::snapParamToTarget(meta.getParamSpec(0), eqT.hpfHz, eqT.hpfHz <= 22.0f));
                node.setParamVal(1, detail::snapParamToTarget(meta.getParamSpec(1), (presetIdx == 2) ? 10000.0f : 20000.0f, presetIdx != 2));
                continue;
            }

            std::vector<int> gainSlots;
            std::vector<int> freqSlots;
            for (int p = 0; p < pCnt; ++p) {
                const auto& ps = meta.getParamSpec(p);
                const std::string pLow = detail::toLowerAscii(ps.name);
                if (detail::containsWordCI(pLow, "high-pass") || detail::containsWordCI(pLow, "hpf") || detail::containsWordCI(pLow, "low cut") || detail::containsWordCI(pLow, "elliptical")) {
                    node.setParamVal(p, detail::snapParamToTarget(ps, eqT.hpfHz, eqT.hpfHz <= 22.0f));
                } else if (detail::containsWordCI(pLow, "gain") || detail::containsWordCI(pLow, "boost") || detail::containsWordCI(pLow, "shelf") || detail::containsWordCI(pLow, "bell")) {
                    if (detail::toLowerAscii(ps.unit) == "db" && !detail::containsWordCI(pLow, "trim") && !detail::containsWordCI(pLow, "makeup")) {
                        gainSlots.push_back(p);
                    }
                } else if (detail::containsWordCI(pLow, "freq") || detail::containsWordCI(pLow, "turnover") || detail::containsWordCI(pLow, "cutoff")) {
                    freqSlots.push_back(p);
                }
            }

            if (gainSlots.size() == 2) {
                node.setParamVal(gainSlots[0], detail::snapParamToTarget(meta.getParamSpec(gainSlots[0]), z1Db));
                node.setParamVal(gainSlots[1], detail::snapParamToTarget(meta.getParamSpec(gainSlots[1]), z4Db));
                if (freqSlots.size() >= 2) {
                    node.setParamVal(freqSlots[0], detail::snapParamToTarget(meta.getParamSpec(freqSlots[0]), eqT.z1FreqHz));
                    node.setParamVal(freqSlots[1], detail::snapParamToTarget(meta.getParamSpec(freqSlots[1]), eqT.z4FreqHz));
                }
            } else if (gainSlots.size() == 3) {
                node.setParamVal(gainSlots[0], detail::snapParamToTarget(meta.getParamSpec(gainSlots[0]), z1Db));
                node.setParamVal(gainSlots[1], detail::snapParamToTarget(meta.getParamSpec(gainSlots[1]), mid3Db));
                node.setParamVal(gainSlots[2], detail::snapParamToTarget(meta.getParamSpec(gainSlots[2]), z4Db));
                if (freqSlots.size() >= 3) {
                    node.setParamVal(freqSlots[0], detail::snapParamToTarget(meta.getParamSpec(freqSlots[0]), eqT.z1FreqHz));
                    node.setParamVal(freqSlots[1], detail::snapParamToTarget(meta.getParamSpec(freqSlots[1]), eqT.mid3BandFreqHz));
                    node.setParamVal(freqSlots[2], detail::snapParamToTarget(meta.getParamSpec(freqSlots[2]), eqT.z4FreqHz));
                } else if (freqSlots.size() == 2) {
                    node.setParamVal(freqSlots[0], detail::snapParamToTarget(meta.getParamSpec(freqSlots[0]), eqT.z1FreqHz));
                    node.setParamVal(freqSlots[1], detail::snapParamToTarget(meta.getParamSpec(freqSlots[1]), eqT.mid3BandFreqHz));
                }
            } else if (gainSlots.size() >= 4) {
                const std::array<float, 4> gTargets = { z1Db, z2Db, z3Db, z4Db };
                const std::array<float, 4> fTargets = { eqT.z1FreqHz, eqT.z2FreqHz, eqT.z3FreqHz, eqT.z4FreqHz };
                for (size_t b = 0; b < 4; ++b) {
                    node.setParamVal(gainSlots[b], detail::snapParamToTarget(meta.getParamSpec(gainSlots[b]), gTargets[b]));
                    if (b < freqSlots.size()) {
                        node.setParamVal(freqSlots[b], detail::snapParamToTarget(meta.getParamSpec(freqSlots[b]), fTargets[b]));
                    }
                }
            } else if (gainSlots.size() == 1) {
                node.setParamVal(gainSlots[0], detail::snapParamToTarget(meta.getParamSpec(gainSlots[0]), mid3Db));
                if (!freqSlots.empty()) {
                    node.setParamVal(freqSlots[0], detail::snapParamToTarget(meta.getParamSpec(freqSlots[0]), eqT.mid3BandFreqHz));
                }
            }
            continue;
        }

        // =====================================================================
        // CATEGORY 4: COMPRESSORS, LIMITERS, GATES, TRANSIENTS & CLIPPERS
        // =====================================================================
        if (meta.category == SchematicCategory::AnalogDynamics) {
            if (rawDesig == "CP10" && pCnt >= 2) {
                static constexpr std::array<std::pair<float, float>, 6> kTransTargets = {{
                    {  0.0f,  0.0f }, // 0: Init
                    { +4.5f, -2.0f }, // 1: Drums
                    { +2.0f, +1.5f }, // 2: Bass
                    { +2.5f,  0.0f }, // 3: Guitar
                    {  0.0f, +2.5f }, // 4: Keys / Synth
                    { +1.0f, +1.5f }  // 5: Vocals
                }};
                const auto [atkDb, susDb] = kTransTargets[static_cast<size_t>(presetIdx)];
                node.setParamVal(0, math::clamp(atkDb, meta.getParamSpec(0).minVal, meta.getParamSpec(0).maxVal));
                node.setParamVal(1, math::clamp(susDb, meta.getParamSpec(1).minVal, meta.getParamSpec(1).maxVal));
                continue;
            }
            if (rawDesig == "CP12" || rawDesig == "CP25") {
                if (rawDesig == "CP12") {
                    if (pCnt > 0) node.setParamVal(0, 6500.0f);
                    if (pCnt > 1) node.setParamVal(1, (presetIdx == 5) ? 8.0f : 0.0f);
                } else {
                    if (pCnt > 1) node.setParamVal(1, (presetIdx == 5) ? 6.0f : 0.0f);
                }
                continue;
            }
            if (rawDesig == "CP13" || rawDesig == "CP24") {
                for (int p = 0; p < pCnt; ++p) {
                    const auto& ps = meta.getParamSpec(p);
                    if (detail::containsWordCI(ps.name, "thresh") || p == 0) {
                        const float targetTh = (presetIdx == 1) ? -32.0f : -60.0f;
                        node.setParamVal(p, math::clamp(targetTh, ps.minVal, ps.maxVal));
                    } else if (detail::containsWordCI(ps.name, "range") || detail::containsWordCI(ps.name, "bleed") || detail::containsWordCI(ps.name, "atten")) {
                        if (presetIdx == 1) {
                            const float cutVal = (ps.minVal < 0.0f) ? -14.0f : 14.0f;
                            node.setParamVal(p, math::clamp(cutVal, ps.minVal, ps.maxVal));
                        } else {
                            node.setParamVal(p, math::clamp(0.0f, ps.minVal, ps.maxVal));
                        }
                    }
                }
                continue;
            }
            if (rawDesig.rfind("CL", 0) == 0) {
                static constexpr std::array<float, 6> kClipDriveDb  = { 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f };
                static constexpr std::array<float, 6> kClipDrivePct = { 0.0f, 15.0f, 15.0f, 15.0f, 10.0f, 10.0f };
                const float drvDb  = kClipDriveDb[static_cast<size_t>(presetIdx)];
                const float drvPct = kClipDrivePct[static_cast<size_t>(presetIdx)];
                for (int p = 0; p < pCnt; ++p) {
                    const auto& ps = meta.getParamSpec(p);
                    if (detail::containsWordCI(ps.name, "ceil") || detail::containsWordCI(ps.name, "out")) {
                        if (ps.minVal < 0.0f && ps.maxVal <= 0.5f) {
                            node.setParamVal(p, math::clamp(-0.5f, ps.minVal, ps.maxVal));
                        }
                    } else if (detail::containsWordCI(ps.name, "knee")) {
                        node.setParamVal(p, math::clamp(60.0f, ps.minVal, ps.maxVal));
                    } else if (detail::containsWordCI(ps.name, "mix")) {
                        node.setParamVal(p, math::clamp(85.0f, ps.minVal, ps.maxVal));
                    } else if (p == 0 || detail::containsWordCI(ps.name, "drive") || detail::containsWordCI(ps.name, "input") ||
                               detail::containsWordCI(ps.name, "effect") || detail::containsWordCI(ps.name, "gain")) {
                        if (detail::toLowerAscii(ps.unit) == "%") {
                            node.setParamVal(p, math::clamp(drvPct, ps.minVal, ps.maxVal));
                        } else {
                            node.setParamVal(p, math::clamp(drvDb, ps.minVal, ps.maxVal));
                        }
                    }
                }
                continue;
            }

            if (rawDesig == "CP1" || rawDesig == "CP6" ||
                rawDesig == "CP14" || rawDesig == "CP15") {
                struct FetPresetTarget {
                    float cp1InputDb;
                    float cp6Drive;
                    float outDb;
                    float cp1Ratio;
                    float cp6Ratio;
                    float atk1to7;
                    float rel1to7;
                };
                static constexpr std::array<FetPresetTarget, 6> kFetTargets = {{
                    { -22.0f, 4.0f, 0.0f, 4.0f, 4.0f, 4.0f, 4.0f }, // 0: Init
                    { -20.0f, 4.5f, 1.0f, 4.0f, 4.0f, 2.5f, 6.0f }, // 1: Drums
                    { -18.0f, 5.0f, 1.5f, 8.0f, 6.0f, 3.5f, 4.5f }, // 2: Bass
                    { -22.0f, 4.0f, 1.0f, 4.0f, 4.0f, 3.0f, 5.0f }, // 3: Guitar
                    { -24.0f, 3.5f, 0.5f, 4.0f, 2.0f, 3.0f, 4.0f }, // 4: Keys / Synth
                    { -20.0f, 4.5f, 1.5f, 4.0f, 4.0f, 4.5f, 5.5f }  // 5: Vocals
                }};
                const FetPresetTarget& ft = kFetTargets[static_cast<size_t>(presetIdx)];
                if (rawDesig == "CP1" && pCnt >= 5) {
                    node.setParamVal(0, ft.cp1InputDb);
                    node.setParamVal(1, ft.outDb);
                    node.setParamVal(2, ft.atk1to7);
                    node.setParamVal(3, ft.rel1to7);
                    node.setParamVal(4, detail::snapParamToTarget(meta.getParamSpec(4), ft.cp1Ratio));
                } else if (rawDesig == "CP6" && pCnt >= 5) {
                    node.setParamVal(0, ft.cp6Drive);
                    node.setParamVal(1, ft.atk1to7);
                    node.setParamVal(2, ft.rel1to7);
                    node.setParamVal(3, ft.outDb);
                    node.setParamVal(4, detail::snapParamToTarget(meta.getParamSpec(4), ft.cp6Ratio));
                } else {
                    for (int p = 0; p < pCnt; ++p) {
                        const auto& ps = meta.getParamSpec(p);
                        if (p == 0 || detail::containsWordCI(ps.name, "input")) {
                            const float frac = (ft.cp6Drive - 3.0f) / 7.0f;
                            node.setParamVal(p, math::clamp(ps.minVal + (0.35f + 0.45f * frac) * (ps.maxVal - ps.minVal), ps.minVal, ps.maxVal));
                        } else if (detail::containsWordCI(ps.name, "output") || detail::containsWordCI(ps.name, "makeup")) {
                            node.setParamVal(p, math::clamp(ft.outDb, ps.minVal, ps.maxVal));
                        } else if (detail::containsWordCI(ps.name, "recover") || detail::containsWordCI(ps.name, "release")) {
                            node.setParamVal(p, detail::snapParamToTarget(ps, (presetIdx == 1) ? 100.0f : 350.0f));
                        }
                    }
                }
                continue;
            }

            if (rawDesig == "CP2" || rawDesig == "CP20" || rawDesig == "CP22") {
                struct OptoPresetTarget {
                    float pr100;
                    float gain100;
                    float r37Emph;
                };
                static constexpr std::array<OptoPresetTarget, 6> kOptoTargets = {{
                    { 35.0f, 50.0f, 100.0f }, // 0: Init
                    { 42.0f, 52.0f, 100.0f }, // 1: Drums
                    { 58.0f, 56.0f, 100.0f }, // 2: Bass
                    { 46.0f, 53.0f, 100.0f }, // 3: Guitar
                    { 35.0f, 50.0f, 100.0f }, // 4: Keys / Synth
                    { 55.0f, 56.0f,  65.0f }  // 5: Vocals
                }};
                const OptoPresetTarget& ot = kOptoTargets[static_cast<size_t>(presetIdx)];
                for (int p = 0; p < pCnt; ++p) {
                    const auto& ps = meta.getParamSpec(p);
                    if (detail::containsWordCI(ps.name, "peak reduction")) {
                        const float scale = (ps.maxVal <= 12.0f) ? 0.1f : 1.0f;
                        node.setParamVal(p, math::clamp(ot.pr100 * scale, ps.minVal, ps.maxVal));
                    } else if (detail::containsWordCI(ps.name, "gain") || detail::containsWordCI(ps.name, "output")) {
                        const float scale = (ps.maxVal <= 12.0f) ? 0.1f : 1.0f;
                        node.setParamVal(p, math::clamp(ot.gain100 * scale, ps.minVal, ps.maxVal));
                    } else if (detail::containsWordCI(ps.name, "emph") || detail::containsWordCI(ps.name, "r37") || detail::containsWordCI(ps.name, "contour")) {
                        if (ps.maxVal <= 12.0f) {
                            node.setParamVal(p, (presetIdx == 5) ? 3.5f : 0.0f);
                        } else {
                            node.setParamVal(p, ot.r37Emph);
                        }
                    } else if (detail::containsWordCI(ps.name, "mode") && ps.stepLabels.size() == 2) {
                        const bool firstIsComp = detail::containsWordCI(ps.stepLabels[0], "comp");
                        node.setParamVal(p, firstIsComp ? 0.0f : 1.0f);
                    }
                }
                continue;
            }

            struct CompPresetTarget {
                float threshDb;
                float thresh0to10;
                float ratio;
                float attackMs;
                float releaseMs;
                float scHpfHz;
                float makeupDb;
                bool  preferAutoRel;
            };
            static constexpr std::array<CompPresetTarget, 6> kCompTargets = {{
                { -10.0f, 3.0f, 2.0f, 15.0f, 300.0f, 20.0f, 0.0f, false }, // 0: Init
                { -14.0f, 4.5f, 4.0f, 30.0f, 100.0f, 90.0f, 3.0f, false }, // 1: Drums
                { -16.0f, 5.2f, 4.0f, 12.0f, 300.0f, 70.0f, 4.0f, false }, // 2: Bass
                { -14.0f, 4.5f, 3.0f, 10.0f, 300.0f, 80.0f, 2.5f, false }, // 3: Guitar
                { -12.0f, 3.8f, 2.0f, 18.0f, 700.0f, 50.0f, 2.0f, true  }, // 4: Keys / Synth
                { -16.0f, 5.2f, 3.5f,  4.0f, 400.0f, 80.0f, 4.0f, true  }  // 5: Vocals
            }};
            const CompPresetTarget& ct = kCompTargets[static_cast<size_t>(presetIdx)];

            for (int p = 0; p < pCnt; ++p) {
                const auto& ps = meta.getParamSpec(p);
                const std::string pLow = detail::toLowerAscii(ps.name);

                if (detail::containsWordCI(pLow, "thresh")) {
                    if (ps.minVal >= 0.0f && ps.maxVal <= 12.0f) {
                        node.setParamVal(p, math::clamp(ct.thresh0to10, ps.minVal, ps.maxVal));
                    } else {
                        node.setParamVal(p, detail::snapParamToTarget(ps, ct.threshDb));
                    }
                } else if (detail::containsWordCI(pLow, "ratio") || detail::containsWordCI(pLow, "slope")) {
                    node.setParamVal(p, detail::snapParamToTarget(ps, ct.ratio));
                } else if (detail::containsWordCI(pLow, "attack")) {
                    if (ps.controlKind == HardwareControlKind::ToggleSwitch2Way) {
                        node.setParamVal(p, (presetIdx == 5) ? 0.0f : 1.0f);
                    } else {
                        node.setParamVal(p, detail::snapParamToTarget(ps, ct.attackMs));
                    }
                } else if (detail::containsWordCI(pLow, "release") || detail::containsWordCI(pLow, "recovery") || detail::containsWordCI(pLow, "time constant")) {
                    node.setParamVal(p, detail::snapParamToTarget(ps, ct.releaseMs, false, ct.preferAutoRel));
                } else if (detail::containsWordCI(pLow, "hpf") || detail::containsWordCI(pLow, "sidechain") || detail::containsWordCI(pLow, "thrust")) {
                    if (ps.controlKind == HardwareControlKind::ToggleSwitch3Way || ps.maxVal <= 3.0f) {
                        node.setParamVal(p, (presetIdx == 1 || presetIdx == 2) ? 1.0f : 0.0f);
                    } else {
                        node.setParamVal(p, detail::snapParamToTarget(ps, ct.scHpfHz));
                    }
                } else if (detail::containsWordCI(pLow, "make-up") || detail::containsWordCI(pLow, "makeup") || detail::containsWordCI(pLow, "output")) {
                    node.setParamVal(p, detail::snapParamToTarget(ps, ct.makeupDb));
                }
            }
            continue;
        }
    }
}

} // namespace avra::studio
