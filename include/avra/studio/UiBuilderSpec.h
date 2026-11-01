#pragma once

#include "avra/gui/controls/HardwareControls.h"
#include "avra/core/LicenseProvenance.h"
#include "avra/studio/SchematicSpec.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <format>
#include <cmath>
#include <algorithm>

namespace avra::studio {

enum class UiWidgetCategory : int {
    Knobs             = 0,
    SwitchesAndLeds   = 1,
    MetersAndDisplays = 2,
    FramesAndBadges   = 3
};

enum class UiWidgetType : int {
    PointerKnob           = 0,  // Fluted silver skirt + tapered dark pointer bar
    SkirtedCapKnob        = 1,  // Fluted silver skirt + round dark center cap
    ConcentricEqKnob      = 2,  // Fluted silver skirt + orange outer ring marker + inner cap
    MarconiRedKnob        = 3,  // Fluted silver skirt + winged crimson-red pointer cap
    ModernArcKnob         = 4,  // Dark anodized cap + glowing LED arc ring
    BakeliteMasterKnob    = 5,  // Vintage fluted bakelite rotary with brass center insert
    ChromeToggleSwitch    = 6,  // 3D chrome bat-handle toggle + hex nut + Jewel LED
    IlluminatedPushButton = 7,  // Backlit rectangular studio push switch
    VerticalStudioFader   = 8,  // Conductive-plastic vertical console fader + dB scale
    JewelLed              = 9,  // Standalone faceted Jewel pilot lamp
    AnalogVuMeter         = 10, // Backlit vintage Gain Reduction needle meter
    LedBargraphMeter      = 11, // 16-segment stereo peak LED ladder meter
    DigitalTrimBargraph   = 12, // 18-segment IN / OUT / OUT-IN digital trim bargraph
    SectionFrame          = 13, // Recessed engraved section frame with header title pill
    RivetedNameplate      = 14, // Brushed aluminum 4-rivet hardware model badge
    EngravedNameplate     = RivetedNameplate, // Alias for RivetedNameplate / EngravedNameplate
    SilkscreenLabel       = 15, // Custom silkscreen panel typography label
    VentilationGrille     = 16, // Machined cooling louver vents for tube / Class-A styling
    ColorCapConsoleKnob   = 17, // Classic console fluted skirt + color-coded anodized center cap
    ChickenHeadKnob       = 18, // Vintage pointed amplifier/stompbox beak rotary
    FlatVectorKnob        = 19, // Clean minimalist flat vector circle with animated pie/wedge fill
    GoldDomeKnob          = 20, // Mirror-polished 24K brushed gold/brass dome rotary
    MacroHaloKnob         = 21, // Big center studio dial flanked by dual segmented LED halo wings
    RoundVintageVuMeter   = 22, // Circular vintage porthole VU meter (Warm Cream/Amber or Ice-Blue)
    GlowingTubeWindow     = 23, // Recessed glass porthole with glowing 12AX7/EL34 vacuum tube filament
    CrtOscilloscopeScreen = 24, // Phosphor CRT/OLED live waveform & stereo graticule scope
    LcdParameterReadout   = 25, // Backlit digital LCD/VFD rack parameter readout display
    KnurledBarrelKnob     = 26, // Heavy diamond-knurled black steel barrel rotary
    ColletBroadcastKnob   = 27, // Precision cylindrical broadcast collet knob with colored cap
    StarburstSynthKnob    = 28, // Synth-style fluted black skirt with brushed silver top disc
    GrimyRelicKnob        = 29, // Distressed road-worn rotary with chipped paint & oxidized brass
    SteppedAttenuatorKnob = 30, // Mastering Detented 24-Step Attenuator with mechanical click-ticks
    RockerPowerSwitch     = 31, // Illuminated 3D red/amber/cyan rocker power switch
    HorizontalCrossfader  = 32, // Horizontal studio mix / crossfader slider rail
    NixieTubeDisplay      = 33, // Vintage amber/orange Nixie tube numeric parameter readout
    StereoGoniometerScope = 34, // Circular Lissajous X/Y stereo phase goniometer scope
    RackHandlePair        = 35, // Heavy cast-chrome 19" rack grab handle bar
    PatchbayJackSocket    = 36, // 1/4" TRS chrome hex nut front-panel patchbay jack socket
    LockingCageToggle     = 37, // Industrial safety-cage guarded metal toggle switch
    FlatPillVectorSwitch  = 38, // Modern flat vector capsule toggle pill
    PastelShaderCurveMeter= 39, // Modern pastel shader EQ/spectral curve telemetry display
    VintageSticker        = 40, // 20 preset road-worn stickers, studio tape strips & custom image decals
    CyberLuxeHaloKnob     = 41, // Ribbed dark carbon/gunmetal dial with glowing cyan halo ring
    NeumorphicGlassKnob   = 42, // Soft extruded white/cream neumorphic dial with warm/cool perimeter glow arc
    SciFiHudRingKnob      = 43, // Futuristic starship HUD concentric segmented energy ring dial
    ValveWheelRelicKnob   = 44, // Industrial red cast-iron 5-spoke valve handwheel
    SubmarineHelmKnob     = 45, // Nautical brass 8-spoke ship-wheel helm & bioluminescent porthole dial
    TattooFlashKnob       = 46, // Traditional tattoo iron coil / sacred ruby heart / chrome skull dial
    MetalSkullKnob        = 47, // Forged 3D pewter skull rotary in spiked iron collar
    FlowerDaisyKnob       = 48, // 60s psychedelic white daisy petal rotary with rainbow spiral center
    ReaperCryptKnob       = 49, // Necropolis carved skull / hourglass / bone pointer dial
    HellfireMagmaKnob     = 50, // Cracked volcanic obsidian & glowing molten lava fissure rotary
    ApocalypseCogKnob     = 51, // Post-apocalyptic rusted gear cogwheel / bottle-cap rotary
    PunkSpikedKnob        = 52, // DIY punk studded collar / 2-tone ska checkerboard / stencil rotary
    MeadowWoodSliceKnob   = 53, // Carved tree-trunk wood ring rotary with sprouting leaf/flower
    SkySunburstKnob       = 54, // Golden sunburst ray / cumulus cloud orb rotary
    StormRaindropKnob     = 55, // Wet storm-slate anodized dial with electric lightning cyan arc
    GlacialIceCrystalKnob = 56, // Faceted translucent frozen ice-crystal / snowflake rotary
    NeumorphicPillButton  = 57, // Soft extruded neumorphic frosted glass pill button & glowing dot switch
    SciFiHologramSwitch   = 58, // Glowing cyan/emerald chamfered starship tactical switch
    NavalBrassToggle      = 59, // Heavy riveted submarine/hazard brass toggle switch
    PunkSafetyPinToggle   = 60, // DIY punk safety-pin & studded leather toggle switch
    HellfireEmberSwitch   = 61, // Volcanic obsidian & glowing magma ember switch
    CyberLuxeMultiBarMeter= 62, // High-density multi-column cyan/amber digital bargraph meter
    NeumorphicDualArcMeter= 63, // Translucent frosted glass neumorphic arc VU needle meter
    SubmarinePortholeGauge= 64, // Heavy riveted brass diving-pressure gauge VU meter with aquatic face
    ReaperScytheArchMeter = 65, // Pointed gothic crypt lancet arch meter with Reaper Scythe needle
    HellfireHornedVuMeter = 66, // Horned demonic circular hellfire gauge meter with blazing magma face
    ApocalypseGeigerVuMeter=67, // Yellow radiation trefoil hazard Geiger-counter VU meter
    PunkAnarchyVuMeter    = 68, // Spiked punk metal VU meter with hand-scrawled Anarchy graffiti face
    NatureElementalVuMeter= 69, // Botanical wood/leaf, sunlit cloud, storm lightning, or icicle-crowned VU meter

    // Backward-compatible widget aliases
    NeumorphicFrostedKnob   = NeumorphicGlassKnob,
    SciFiHologramKnob       = SciFiHudRingKnob,
    SubmarineHelmWheelKnob  = SubmarineHelmKnob,
    SubmarineBioFishKnob    = SubmarineHelmKnob,
    NewWaveSunsetKnob       = CyberLuxeHaloKnob,
    TattooSacredHeartKnob   = TattooFlashKnob,
    TattooSkullRoseKnob     = TattooFlashKnob,
    MetalSkullRivetKnob     = MetalSkullKnob,
    FlowerPowerDaisyKnob    = FlowerDaisyKnob,
    GrimReaperHourglassKnob = ReaperCryptKnob,
    HellfireMagmaOrbKnob    = HellfireMagmaKnob,
    PunkSpikedCollarKnob    = PunkSpikedKnob,
    PunkCheckerboardKnob    = PunkSpikedKnob,
    NatureWoodSliceKnob     = MeadowWoodSliceKnob
};

enum class UiScalePreset : int {
    // Core presets (0..8 preserved for backward-compatible .faceplate files)
    LinearDb          = 0,  // -16 .. 0 .. +16 dB (Console / EQ Boost-Cut)
    GainLineMic       = 1,  // -20 .. +80 dB (Preamp Mic/Line Gain)
    LowFreqHz         = 2,  // OFF, 35, 60, 110, 220 Hz (Orange OFF)
    MidFreqHz         = 3,  // OFF, .36, .70, 1.6, 3.2, 4.8, 7.2 kHz (Orange OFF)
    HighPassHz        = 4,  // OFF, 50, 80, 160, 300 Hz (Orange OFF)
    ThresholdDb       = 5,  // +10 .. -20 dB (Compressor Threshold)
    RatioSteps        = 6,  // 1.5:1 .. 6:1 (Bus Compressor Ratio)
    RecoveryAuto      = 7,  // 100 .. 1500, A1, A2 (Orange A2)
    PercentDryWet     = 8,  // 0% .. 100% (Percent Scale)
    Percent0To100     = 8,  // Alias for 0 .. 100%

    // Extended Domain-Specific Scales (9..33)
    MasterVolumeDb    = 9,  // -OO, -30, -20, -10, -6, -3, 0, +3, +6 dB (Master Volume)
    DriveZeroTo36Db   = 10, // 0, 6, 12, 18, 24, 30, 36 dB (Tube / FET / Vari-Mu Drive)
    FineTrimDb        = 11, // -6, -4, -2, 0, +2, +4, +6 dB (Mastering Fine Trim)
    AttackFastMs      = 12, // 0.1, 0.3, 1, 3, 10, 30 ms (Fast VCA / Bus Comp Attack)
    AttackFet1176     = 13, // 1 (800us) .. 7 (20us) (Fast FET Attack Dial)
    AttackSlowMs      = 14, // 1, 5, 15, 30, 60, 100, 150 ms (Opto / Vari-Mu Attack)
    ReleaseFastMs     = 15, // 30, 60, 120, 250, 400, 600 ms (Fast FET / Limiter Release)
    ReleaseVariMuSec  = 16, // 0.1, 0.2, 0.4, 0.8, 1.5, 2.5 s (Vari-Mu / Opto Recovery)
    ThresholdDeepDb   = 17, // 0, -6, -12, -18, -24, -30, -36 dB (Deep Bus Threshold)
    LimiterCeilingDb  = 18, // -12, -9, -6, -3, -1.5, -0.5, 0.0 dBFS (Brickwall / Clipper Ceiling)
    PeakReduction0100 = 19, // 0, 20, 40, 60, 80, 100 (Optical Peak Reduction)
    RatioFetLim       = 20, // 4:1, 8:1, 12:1, 20:1, ALL (Fast FET Limiter Ratios)
    RatioGentle       = 21, // 1.1:1, 1.3:1, 1.5:1, 2:1, 3:1 (Mastering Soft Ratio)
    HighAirFreqKhz    = 22, // 2.5, 5, 8, 10, 12, 16, 20 kHz (High Shelf / Air Band)
    FullSweepFreqHz   = 23, // 20, 60, 200, 600, 2k, 6k, 18k Hz (Full-Range Filter Cutoff)
    ModRateHz         = 24, // 0.1, 0.3, 0.8, 1.5, 3.0, 6.0 Hz (LFO Modulation Rate)
    DelayTimeMs       = 25, // 25, 100, 250, 500, 900, 1500 ms (Tape / BBD Delay Time)
    ReverbDecaySec    = 26, // 0.4, 1.2, 2.5, 4.5, 7.0, 10.0 s (Plate / Hall Decay)
    StereoWidthPct    = 27, // MONO, 50, 100, 125, 150, 200 % (Stereo / M-S Width)
    CoreMatrixNiFeSt  = 28, // NICKEL, Ni/Fe, IRON, Fe/St, STEEL (3-Core Transformer Matrix)
    DialOneToTen      = 29, // 1..10 (Classic Vintage Studio Dial)
    DialZeroToTen     = 30, // 0..10 (Classic Analog 0-10 Dial)
    StereoPanLCR      = 31, // L100, L50, C, R50, R100 (Studio Stereo Balance / Pan)
    FreqLowShelf1073  = 32, // 35, 60, 110, 220 Hz (1073 Low Shelf Frequencies)
    MixDryWet         = 33, // DRY, 25, 50, 75, WET (Parallel & Master Dry/Wet Mix)
    MakeupGain0To20Db = 34  // 0, 4, 8, 12, 16, 20 dB (Compressor Make-Up Gain)
};

/**
 * @brief Functional role baked into each knob in the palette so binding & scale are auto-applied.
 */
enum class UiKnobRole : int {
    StageOrCustom    = 0,  // Auto-synced from Schematic stage
    MasterVolume     = 1,  // Master Volume Knob (auto-binds to Master Output)
    InputDrive       = 2,  // Input Drive Knob (auto-binds to Input Stage / Global In)
    CompAttack       = 3,  // Comp Attack Knob (auto-binds to Compressor Attack / Drive)
    CompRelease      = 4,  // Comp Release Knob (auto-binds to Compressor Release / Recovery)
    CompThreshold    = 5,  // Comp Threshold Knob (auto-binds to Compressor Threshold / Peak Red)
    CompRatio        = 6,  // Comp Ratio Knob (auto-binds to Compressor Ratio)
    ParallelMix      = 7,  // Parallel Wet/Dry Knob (auto-binds to Parallel Wet/Dry Mix)
    EqLowGain        = 8,  // EQ Low / Bass Knob (auto-binds to EQ Low Band)
    EqMidGain        = 9,  // EQ Mid Presence Knob (auto-binds to EQ Mid Bell)
    EqHighGain       = 10, // EQ High / Air Knob (auto-binds to EQ High Shelf / Air)
    FilterCutoffHz   = 11, // HPF / Cutoff Freq Knob (auto-binds to Filter / Sidechain HPF)
    PreampDrive      = 12, // Preamp / Tube Drive Knob (auto-binds to Preamp / Tube / OpAmp Drive)
    SaturationPct    = 13, // Iron / Tape Saturation Knob (auto-binds to Iron / Tape / Tube Bias)
    LimiterCeiling   = 14, // Limiter / Clipper Ceiling Knob (auto-binds to Limiter / Clipper Ceiling)
    DelaySpaceTime   = 15, // Delay / Reverb Time Knob (auto-binds to Delay Time / Reverb Decay)
    StereoWidth      = 16, // Stereo Width Knob (auto-binds to Stereo Widener / M-S Width)
    GateThreshold    = 17, // Gate Threshold Knob (auto-binds to Hysteresis Noise Gate / Dynamics)
    DeEsserSibilance = 18, // De-Esser Sibilance Knob (auto-binds to Split-Band De-Esser / Spectral)
    FilterResonanceQ = 19, // Filter Resonance Q Knob (auto-binds to Ladder VCF / Wah / SVF Q)
    ModulationRate   = 20, // LFO / Mod Rate Knob (auto-binds to Chorus / Phaser / Flanger / Tremolo Rate)
    BitcrushFold     = 21, // Bitcrush / Wavefold Knob (auto-binds to Bitcrusher / Wavefolder / Inflator)
    CabinetToneTilt  = 22, // Cabinet / Tone Tilt Knob (auto-binds to Cabinet Sim / Tilt EQ)
    EqLowAtten       = 23, // Passive Low Atten / Dip Knob (auto-binds to EQ Low Atten / Dip)
    EqLowFreq        = 24, // EQ Low Frequency Selector (auto-binds to EQ Low Freq Hz)
    EqMidFreq        = 25, // EQ Mid Frequency Selector (auto-binds to EQ Mid Freq Hz)
    EqHighFreq       = 26, // EQ High / Air Frequency Selector (auto-binds to EQ High Freq kHz)
    CompMakeupGain   = 27, // Comp Makeup / Output Gain Knob (auto-binds to Comp Makeup / Output Gain)
    CompSidechainHpf = 28, // Comp Sidechain HPF / Thrust Tilt (auto-binds to Comp SC HPF / Tilt)
    TubeBiasAsym     = 29, // Tube Bias / Plate Sag / Asymmetry (auto-binds to Preamp Bias / Sag)
    TransformerCore  = 30, // Transformer Core Alloy Matrix (auto-binds to Ni/Fe/Steel Core Matrix)
    TransientAttack  = 31, // Transient Shaper Attack Punch (auto-binds to Transient Attack)
    TransientSustain = 32, // Transient Shaper Sustain Body (auto-binds to Transient Sustain)
    DelayFeedback    = 33, // Delay / Echo Feedback % (auto-binds to Delay Regeneration / Feedback)
    DelayWowFlutter  = 34, // Tape Wow & Flutter / BBD Mod (auto-binds to Delay Wow / Mod / Detune)
    DelayWetMix      = 35, // Delay Wet / Dry Mix % (auto-binds to Delay Wet Mix)
    ReverbDecay      = 36, // Reverb Decay Time RT60 (auto-binds to Plate / Hall / Shimmer Decay)
    ReverbPreDelay   = 37, // Reverb Pre-Delay / Room Size (auto-binds to Reverb Pre-Delay / Size)
    ReverbDamping    = 38, // Reverb HF Damping / Tone (auto-binds to Reverb HF Damping)
    ReverbShimmer    = 39, // Octave Shimmer / Pitch Mix (auto-binds to Shimmer / Harmonizer Mix)
    ReverbWetMix     = 40, // Reverb Wet / Dry Mix % (auto-binds to Reverb Wet Mix)
    ModulationDepth  = 41  // Chorus / Phaser / Flanger Depth (auto-binds to Modulation Depth)
};

enum class UiAccentColor : int {
    WarmAmber      = 0,
    NeonCyan       = 1,
    CrimsonRed     = 2,
    EmeraldGreen   = 3,
    RoyalGold      = 4,
    CobaltBlue     = 5,
    PureWhite      = 6,
    VioletGlow     = 7,
    SunsetOrange   = 8,
    NeonMagenta    = 9,
    LimeChartreuse = 10,
    IceSkyBlue     = 11,
    StudioBlue     = CobaltBlue,
    VioletPurple   = VioletGlow
};

inline gui::Color getUiAccentColorValue(UiAccentColor c) noexcept {
    switch (c) {
        case UiAccentColor::NeonCyan:       return gui::Color::fromHex(0x00F5D4);
        case UiAccentColor::CrimsonRed:     return gui::Color::fromHex(0xFF4D5A);
        case UiAccentColor::EmeraldGreen:   return gui::Color::fromHex(0x52B788);
        case UiAccentColor::RoyalGold:      return gui::Color::fromHex(0xE8C547);
        case UiAccentColor::CobaltBlue:     return gui::Color::fromHex(0x4EA8DE);
        case UiAccentColor::PureWhite:      return gui::Color::fromHex(0xF0F6FA);
        case UiAccentColor::VioletGlow:     return gui::Color::fromHex(0xB565F2);
        case UiAccentColor::SunsetOrange:   return gui::Color::fromHex(0xFF6B2B);
        case UiAccentColor::NeonMagenta:    return gui::Color::fromHex(0xFF2E93);
        case UiAccentColor::LimeChartreuse: return gui::Color::fromHex(0xA8F028);
        case UiAccentColor::IceSkyBlue:     return gui::Color::fromHex(0x72E5FC);
        case UiAccentColor::WarmAmber:
        default:                            return gui::Color::fromHex(0xFF9F1C);
    }
}

inline const char* getUiAccentColorName(UiAccentColor c) noexcept {
    switch (c) {
        case UiAccentColor::NeonCyan:       return "NEON CYAN";
        case UiAccentColor::CrimsonRed:     return "CRIMSON RED";
        case UiAccentColor::EmeraldGreen:   return "EMERALD GREEN";
        case UiAccentColor::RoyalGold:      return "ROYAL GOLD";
        case UiAccentColor::CobaltBlue:     return "COBALT BLUE";
        case UiAccentColor::PureWhite:      return "PURE WHITE";
        case UiAccentColor::VioletGlow:     return "VIOLET GLOW";
        case UiAccentColor::SunsetOrange:   return "SUNSET ORANGE";
        case UiAccentColor::NeonMagenta:    return "NEON MAGENTA";
        case UiAccentColor::LimeChartreuse: return "LIME CHARTREUSE";
        case UiAccentColor::IceSkyBlue:     return "ICE SKY BLUE";
        case UiAccentColor::WarmAmber:
        default:                            return "WARM AMBER";
    }
}

inline const char* getUiFontSizeName(int preset) noexcept {
    switch (preset) {
        case 0:  return "SMALL (85%)";
        case 2:  return "LARGE (116%)";
        case 3:  return "XL BOLD (132%)";
        case 1:
        default: return "NORMAL (100%)";
    }
}

inline const char* getUiFontSizeShortName(int preset) noexcept {
    switch (preset) {
        case 0:  return "SMALL";
        case 2:  return "LARGE";
        case 3:  return "XL BOLD";
        case 1:
        default: return "NORMAL";
    }
}

inline float getUiFontSizePt(int preset) noexcept {
    switch (preset) {
        case 0:  return 8.5f;
        case 2:  return 13.0f;
        case 3:  return 16.0f;
        case 1:
        default: return 10.5f;
    }
}

inline float getUiFontScaleFactor(int preset) noexcept {
    switch (preset) {
        case 0:  return 0.86f;
        case 2:  return 1.16f;
        case 3:  return 1.32f;
        case 1:
        default: return 1.00f;
    }
}

inline const char* getFontFamilyName(gui::FontFamily f) noexcept {
    switch (f) {
        case gui::FontFamily::TechnicalMono:   return "TECH MONO (Consolas)";
        case gui::FontFamily::BroadcastDin:    return "DIN CONSOLE (Bahnschrift)";
        case gui::FontFamily::VintageConsole:  return "VINTAGE SILK (Trebuchet)";
        case gui::FontFamily::BoutiqueSerif:   return "BOUTIQUE SERIF (Georgia)";
        case gui::FontFamily::StencilBlock:    return "STENCIL BLOCK (Franklin)";
        case gui::FontFamily::GeometricClean:  return "GEOMETRIC (Calibri)";
        case gui::FontFamily::RetroTypewriter: return "TYPEWRITER (Courier)";
        case gui::FontFamily::StudioSans:
        default:                               return "STUDIO SANS (Segoe UI)";
    }
}

inline const char* getFontFamilyShortName(gui::FontFamily f) noexcept {
    switch (f) {
        case gui::FontFamily::TechnicalMono:   return "TECH MONO";
        case gui::FontFamily::BroadcastDin:    return "DIN CONSOLE";
        case gui::FontFamily::VintageConsole:  return "VINTAGE SILK";
        case gui::FontFamily::BoutiqueSerif:   return "SERIF";
        case gui::FontFamily::StencilBlock:    return "STENCIL";
        case gui::FontFamily::GeometricClean:  return "GEOMETRIC";
        case gui::FontFamily::RetroTypewriter: return "TYPEWRITER";
        case gui::FontFamily::StudioSans:
        default:                               return "STUDIO SANS";
    }
}

enum class UiSkinFontColor : int {
    SilkscreenWhite  = 0,  // Crisp Studio Silkscreen Off-White (0xE8F1F2)
    ObsidianBlackInk = 1,  // Deep Black Ink — ideal for light/pastel/silver/cream skins (0x14181C)
    WarmCreamIvory   = 2,  // Vintage Warm Cream / Ivory Ink (0xFFF3D6)
    VintageGoldLeaf  = 3,  // 24K Gold Leaf Silkscreen (0xE8C547)
    WarmAmberInk     = 4,  // Warm Studio Amber (0xFF9F1C)
    NeonCyanInk      = 5,  // Electric Neon Cyan (0x00F5D4)
    EmeraldGreenInk  = 6,  // Console Emerald Green (0x52B788)
    CrimsonRedInk    = 7,  // Mastering Crimson Red (0xFF4D5A)
    CobaltBlueInk    = 8,  // Studio Cobalt Blue (0x4EA8DE)
    VioletGlowInk    = 9,  // Synth Violet Glow (0xB565F2)
    SunsetOrangeInk  = 10, // Warm Sunset Orange (0xFF6B2B)
    BrushedSilverInk = 11  // Subtle Brushed Silver Grey (0xB4C0C8)
};

inline gui::Color getSkinFontPresetColorValue(UiSkinFontColor c) noexcept {
    switch (c) {
        case UiSkinFontColor::ObsidianBlackInk: return gui::Color::fromHex(0x14181C);
        case UiSkinFontColor::WarmCreamIvory:   return gui::Color::fromHex(0xFFF3D6);
        case UiSkinFontColor::VintageGoldLeaf:  return gui::Color::fromHex(0xE8C547);
        case UiSkinFontColor::WarmAmberInk:     return gui::Color::fromHex(0xFF9F1C);
        case UiSkinFontColor::NeonCyanInk:      return gui::Color::fromHex(0x00F5D4);
        case UiSkinFontColor::EmeraldGreenInk:  return gui::Color::fromHex(0x52B788);
        case UiSkinFontColor::CrimsonRedInk:    return gui::Color::fromHex(0xFF4D5A);
        case UiSkinFontColor::CobaltBlueInk:    return gui::Color::fromHex(0x4EA8DE);
        case UiSkinFontColor::VioletGlowInk:    return gui::Color::fromHex(0xB565F2);
        case UiSkinFontColor::SunsetOrangeInk:  return gui::Color::fromHex(0xFF6B2B);
        case UiSkinFontColor::BrushedSilverInk: return gui::Color::fromHex(0xB4C0C8);
        case UiSkinFontColor::SilkscreenWhite:
        default:                                return gui::Color::fromHex(0xE8F1F2);
    }
}

inline const char* getSkinFontPresetColorName(UiSkinFontColor c) noexcept {
    switch (c) {
        case UiSkinFontColor::ObsidianBlackInk: return "BLACK INK";
        case UiSkinFontColor::WarmCreamIvory:   return "WARM CREAM";
        case UiSkinFontColor::VintageGoldLeaf:  return "GOLD LEAF";
        case UiSkinFontColor::WarmAmberInk:     return "WARM AMBER";
        case UiSkinFontColor::NeonCyanInk:      return "NEON CYAN";
        case UiSkinFontColor::EmeraldGreenInk:  return "EMERALD";
        case UiSkinFontColor::CrimsonRedInk:    return "CRIMSON RED";
        case UiSkinFontColor::CobaltBlueInk:    return "COBALT BLUE";
        case UiSkinFontColor::VioletGlowInk:    return "VIOLET GLOW";
        case UiSkinFontColor::SunsetOrangeInk:  return "ORANGE";
        case UiSkinFontColor::BrushedSilverInk: return "SILVER GREY";
        case UiSkinFontColor::SilkscreenWhite:
        default:                                return "SILK WHITE";
    }
}

struct UiWidgetCatalogEntry {
    UiWidgetType  type;
    UiWidgetCategory category;
    const char*   name;
    const char*   subtitle;
    float         defaultW;
    float         defaultH;
    UiKnobRole    knobRole{UiKnobRole::StageOrCustom};
    const char*   bakedTitle{"GAIN"};
    const char*   bakedValText{"0.0 dB"};
    UiScalePreset bakedScale{UiScalePreset::LinearDb};
    float         bakedNorm{0.5f};
};

inline const std::vector<UiWidgetCatalogEntry>& getUiWidgetCatalog() {
    static const std::vector<UiWidgetCatalogEntry> kCatalog = {
        // Functional Knobs with Baked-In Parameter Binding & Relevant Scale (41 studio roles + 11 distinct visual styles)
        { UiWidgetType::BakeliteMasterKnob,  UiWidgetCategory::Knobs, "Master Volume Knob",    "Auto-Binds: Master Output (-OO..+6dB)", 128.0f, 136.0f, UiKnobRole::MasterVolume,     "MASTER VOL",   "0.0 dB",   UiScalePreset::MasterVolumeDb,  0.72f },
        { UiWidgetType::PointerKnob,         UiWidgetCategory::Knobs, "Input Drive Knob",      "Auto-Binds: Input Stage (-20..+80dB)",  122.0f, 132.0f, UiKnobRole::InputDrive,       "INPUT DRIVE",  "0.0 dB",   UiScalePreset::GainLineMic,     0.50f },
        { UiWidgetType::MarconiRedKnob,      UiWidgetCategory::Knobs, "Preamp / Tube Drive",   "Auto-Binds: Tube/Preamp (0..+36 dB)",   122.0f, 132.0f, UiKnobRole::PreampDrive,      "TUBE DRIVE",   "+9.0 dB",  UiScalePreset::DriveZeroTo36Db, 0.35f },
        { UiWidgetType::ChickenHeadKnob,     UiWidgetCategory::Knobs, "Tube Bias / Plate Sag", "Auto-Binds: Preamp Bias / Sag (0..100%)", 122.0f, 132.0f, UiKnobRole::TubeBiasAsym,   "TUBE BIAS",    "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::GoldDomeKnob,        UiWidgetCategory::Knobs, "Iron / Tape Saturation","Auto-Binds: Iron/Tape Sat (0..100%)",   122.0f, 132.0f, UiKnobRole::SaturationPct,    "SATURATION",   "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::ChickenHeadKnob,     UiWidgetCategory::Knobs, "Transformer Core Alloy","Auto-Binds: Ni/Fe/Steel Core Matrix",   122.0f, 132.0f, UiKnobRole::TransformerCore,  "CORE MATRIX",  "50 %",     UiScalePreset::CoreMatrixNiFeSt,0.50f },
        { UiWidgetType::ColorCapConsoleKnob, UiWidgetCategory::Knobs, "EQ Low / Bass Gain",    "Auto-Binds: EQ Low Shelf (-16..+16dB)", 122.0f, 132.0f, UiKnobRole::EqLowGain,        "LOW / BASS",   "+2.0 dB",  UiScalePreset::LinearDb,        0.56f },
        { UiWidgetType::SkirtedCapKnob,      UiWidgetCategory::Knobs, "EQ Low Atten / Dip",    "Auto-Binds: Passive Low Dip (0..12 dB)", 122.0f, 132.0f, UiKnobRole::EqLowAtten,       "LOW ATTEN",    "1.5 dB",   UiScalePreset::DialZeroToTen,   0.15f },
        { UiWidgetType::ColorCapConsoleKnob, UiWidgetCategory::Knobs, "EQ Low Frequency Hz",   "Auto-Binds: EQ Low Freq (35..220 Hz)",  122.0f, 132.0f, UiKnobRole::EqLowFreq,        "LOW FREQ",     "60 Hz",    UiScalePreset::LowFreqHz,       0.35f },
        { UiWidgetType::ConcentricEqKnob,    UiWidgetCategory::Knobs, "EQ Mid Presence Gain",  "Auto-Binds: EQ Mid Bell (-16..+16dB)",  122.0f, 132.0f, UiKnobRole::EqMidGain,        "MID PRESENCE", "+2.5 dB",  UiScalePreset::LinearDb,        0.58f },
        { UiWidgetType::ColorCapConsoleKnob, UiWidgetCategory::Knobs, "EQ Mid Frequency kHz",  "Auto-Binds: EQ Mid Freq (.36..7.2kHz)", 122.0f, 132.0f, UiKnobRole::EqMidFreq,        "MID FREQ",     "1.6 kHz",  UiScalePreset::MidFreqHz,       0.40f },
        { UiWidgetType::ColorCapConsoleKnob, UiWidgetCategory::Knobs, "EQ High / Air Gain",    "Auto-Binds: EQ High Band (-16..+16dB)", 122.0f, 132.0f, UiKnobRole::EqHighGain,       "HIGH / AIR",   "+3.0 dB",  UiScalePreset::LinearDb,        0.60f },
        { UiWidgetType::GoldDomeKnob,        UiWidgetCategory::Knobs, "EQ High / Air Freq",    "Auto-Binds: EQ High Freq (2.5..20kHz)", 122.0f, 132.0f, UiKnobRole::EqHighFreq,       "AIR FREQ",     "10 kHz",   UiScalePreset::HighAirFreqKhz,  0.50f },
        { UiWidgetType::SkirtedCapKnob,      UiWidgetCategory::Knobs, "HPF Cutoff Freq Hz",    "Auto-Binds: Filter / HPF (OFF..300Hz)", 122.0f, 132.0f, UiKnobRole::FilterCutoffHz,   "HPF / FREQ",   "80 Hz",    UiScalePreset::HighPassHz,      0.35f },
        { UiWidgetType::FlatVectorKnob,      UiWidgetCategory::Knobs, "Filter Resonance Q",    "Auto-Binds: Ladder VCF / Wah Q (0..10)",122.0f, 132.0f, UiKnobRole::FilterResonanceQ, "RESONANCE Q",  "35 %",     UiScalePreset::DialZeroToTen,   0.35f },
        { UiWidgetType::ChickenHeadKnob,     UiWidgetCategory::Knobs, "Fulcrum / Tone Tilt",   "Auto-Binds: Mastering Tilt / Air EQ",   122.0f, 132.0f, UiKnobRole::CabinetToneTilt,  "TONE TILT",    "0.0 dB",   UiScalePreset::LinearDb,        0.50f },
        { UiWidgetType::SkirtedCapKnob,      UiWidgetCategory::Knobs, "Comp Threshold Knob",   "Auto-Binds: Comp Thresh (+10..-20dB)",  122.0f, 132.0f, UiKnobRole::CompThreshold,    "THRESHOLD",    "-12.0 dB", UiScalePreset::ThresholdDb,     0.55f },
        { UiWidgetType::PointerKnob,         UiWidgetCategory::Knobs, "Comp Ratio Knob",       "Auto-Binds: Comp Ratio (1.5:1..20:1)",  122.0f, 132.0f, UiKnobRole::CompRatio,        "COMP RATIO",   "4.0 :1",   UiScalePreset::RatioSteps,      0.60f },
        { UiWidgetType::PointerKnob,         UiWidgetCategory::Knobs, "Comp Attack Knob",      "Auto-Binds: Comp Attack (0.1..30 ms)",  122.0f, 132.0f, UiKnobRole::CompAttack,       "COMP ATTACK",  "5.0 ms",   UiScalePreset::AttackFastMs,    0.38f },
        { UiWidgetType::PointerKnob,         UiWidgetCategory::Knobs, "Comp Release Knob",     "Auto-Binds: Comp Release (100..A2)",    122.0f, 132.0f, UiKnobRole::CompRelease,      "COMP RELEASE", "100 ms",   UiScalePreset::RecoveryAuto,    0.35f },
        { UiWidgetType::GoldDomeKnob,        UiWidgetCategory::Knobs, "Comp Makeup Gain",      "Auto-Binds: Comp Makeup (-6..+24 dB)",  122.0f, 132.0f, UiKnobRole::CompMakeupGain,   "MAKEUP GAIN",  "+4.0 dB",  UiScalePreset::DriveZeroTo36Db, 0.35f },
        { UiWidgetType::ColorCapConsoleKnob, UiWidgetCategory::Knobs, "Comp Sidechain HPF",    "Auto-Binds: Sidechain HPF / Tilt",      122.0f, 132.0f, UiKnobRole::CompSidechainHpf, "SC HPF",       "90 Hz",    UiScalePreset::HighPassHz,      0.40f },
        { UiWidgetType::ModernArcKnob,       UiWidgetCategory::Knobs, "Limiter Ceiling Knob",  "Auto-Binds: Peak Ceiling (-12..0dBFS)", 122.0f, 132.0f, UiKnobRole::LimiterCeiling,   "CEILING",      "-0.5 dB",  UiScalePreset::LimiterCeilingDb,0.88f },
        { UiWidgetType::FlatVectorKnob,      UiWidgetCategory::Knobs, "Transient Attack Punch","Auto-Binds: Transient Attack (-15..+15)",122.0f, 132.0f, UiKnobRole::TransientAttack,  "TRANS ATTACK", "+4.5 dB",  UiScalePreset::LinearDb,        0.65f },
        { UiWidgetType::FlatVectorKnob,      UiWidgetCategory::Knobs, "Transient Sustain Body","Auto-Binds: Transient Sustain (-15..+15)",122.0f, 132.0f, UiKnobRole::TransientSustain,"SUSTAIN",      "0.0 dB",   UiScalePreset::LinearDb,        0.50f },
        { UiWidgetType::SkirtedCapKnob,      UiWidgetCategory::Knobs, "Gate Threshold Knob",   "Auto-Binds: Noise Gate (0..-36dB)",     122.0f, 132.0f, UiKnobRole::GateThreshold,    "GATE THRESH",  "-42.0 dB", UiScalePreset::ThresholdDeepDb, 0.48f },
        { UiWidgetType::ModernArcKnob,       UiWidgetCategory::Knobs, "De-Esser Sibilance",    "Auto-Binds: De-Esser / Spectral (0..100%)", 122.0f, 132.0f, UiKnobRole::DeEsserSibilance, "SIBILANCE",    "55 %",     UiScalePreset::PercentDryWet,   0.55f },
        { UiWidgetType::ChickenHeadKnob,     UiWidgetCategory::Knobs, "Delay / Echo Time ms",  "Auto-Binds: Delay / Echo Time (ms)",    122.0f, 132.0f, UiKnobRole::DelaySpaceTime,   "DELAY TIME",   "340 ms",   UiScalePreset::DelayTimeMs,     0.35f },
        { UiWidgetType::SkirtedCapKnob,      UiWidgetCategory::Knobs, "Delay Feedback %",      "Auto-Binds: Echo / BBD Feedback %",     122.0f, 132.0f, UiKnobRole::DelayFeedback,    "FEEDBACK",     "42 %",     UiScalePreset::PercentDryWet,   0.42f },
        { UiWidgetType::PointerKnob,         UiWidgetCategory::Knobs, "Tape Wow / Mod Depth",  "Auto-Binds: Tape Wow & Flutter / Mod",  122.0f, 132.0f, UiKnobRole::DelayWowFlutter,  "WOW / MOD",    "25 %",     UiScalePreset::PercentDryWet,   0.25f },
        { UiWidgetType::MacroHaloKnob,       UiWidgetCategory::Knobs, "Delay Wet / Dry Mix",   "Auto-Binds: Delay Wet Mix (0..100%)",   132.0f, 142.0f, UiKnobRole::DelayWetMix,      "DELAY MIX",    "35 %",     UiScalePreset::PercentDryWet,   0.35f },
        { UiWidgetType::GoldDomeKnob,        UiWidgetCategory::Knobs, "Reverb Decay RT60 (s)", "Auto-Binds: Plate / Hall / Shimmer Decay", 122.0f, 132.0f, UiKnobRole::ReverbDecay,   "REV DECAY",    "2.4 s",    UiScalePreset::ReverbDecaySec,  0.35f },
        { UiWidgetType::SkirtedCapKnob,      UiWidgetCategory::Knobs, "Reverb Pre-Delay / Size","Auto-Binds: Reverb Pre-Delay / Room",  122.0f, 132.0f, UiKnobRole::ReverbPreDelay,   "PRE-DELAY",    "24 ms",    UiScalePreset::DelayTimeMs,     0.25f },
        { UiWidgetType::ConcentricEqKnob,    UiWidgetCategory::Knobs, "Reverb HF Damping",     "Auto-Binds: Plate / Hall HF Damping",   122.0f, 132.0f, UiKnobRole::ReverbDamping,    "HF DAMPING",   "38 %",     UiScalePreset::PercentDryWet,   0.38f },
        { UiWidgetType::MacroHaloKnob,       UiWidgetCategory::Knobs, "Shimmer Octave Pitch",  "Auto-Binds: Shimmer / MicroPitch Mix",  132.0f, 142.0f, UiKnobRole::ReverbShimmer,    "SHIMMER",      "45 %",     UiScalePreset::PercentDryWet,   0.45f },
        { UiWidgetType::MacroHaloKnob,       UiWidgetCategory::Knobs, "Reverb Wet / Dry Mix",  "Auto-Binds: Reverb Wet Mix (0..100%)",  132.0f, 142.0f, UiKnobRole::ReverbWetMix,     "REVERB MIX",   "32 %",     UiScalePreset::PercentDryWet,   0.32f },
        { UiWidgetType::ChickenHeadKnob,     UiWidgetCategory::Knobs, "LFO / Mod Rate Hz",     "Auto-Binds: Chorus / Phaser / Trem Rate", 122.0f, 132.0f, UiKnobRole::ModulationRate, "MOD RATE",     "1.5 Hz",   UiScalePreset::ModRateHz,       0.40f },
        { UiWidgetType::ColorCapConsoleKnob, UiWidgetCategory::Knobs, "Chorus / Mod Depth %",  "Auto-Binds: Chorus / Flanger / Trem Depth", 122.0f, 132.0f, UiKnobRole::ModulationDepth,"MOD DEPTH",    "55 %",     UiScalePreset::PercentDryWet,   0.55f },
        { UiWidgetType::FlatVectorKnob,      UiWidgetCategory::Knobs, "Stereo Width Knob",     "Auto-Binds: M/S & Widener (0..200%)",   122.0f, 132.0f, UiKnobRole::StereoWidth,      "STEREO WIDTH", "125 %",    UiScalePreset::StereoWidthPct,  0.62f },
        { UiWidgetType::MarconiRedKnob,      UiWidgetCategory::Knobs, "Bitcrush / Wavefold",   "Auto-Binds: Bitcrusher / Wavefolder",   122.0f, 132.0f, UiKnobRole::BitcrushFold,     "CRUSH / FOLD", "40 %",     UiScalePreset::PercentDryWet,   0.40f },
        { UiWidgetType::MacroHaloKnob,       UiWidgetCategory::Knobs, "Parallel Wet/Dry Knob", "Auto-Binds: Parallel Mix (0..100%)",    132.0f, 142.0f, UiKnobRole::ParallelMix,      "PARALLEL MIX", "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::KnurledBarrelKnob,   UiWidgetCategory::Knobs, "Knurled Black Barrel",  "Heavy Diamond-Knurled Steel Rotary",    122.0f, 132.0f, UiKnobRole::InputDrive,       "INPUT GAIN",   "0.0 dB",   UiScalePreset::GainLineMic,     0.50f },
        { UiWidgetType::ColletBroadcastKnob, UiWidgetCategory::Knobs, "Collet Broadcast Knob", "German Broadcast Collet + Coloured Cap", 122.0f, 132.0f, UiKnobRole::EqMidGain,        "BROADCAST EQ", "+1.5 dB",  UiScalePreset::LinearDb,        0.55f },
        { UiWidgetType::StarburstSynthKnob,  UiWidgetCategory::Knobs, "Starburst Synth Knob",  "Fluted Synth Skirt + Silver Top Disc",  122.0f, 132.0f, UiKnobRole::FilterCutoffHz,   "VCF CUTOFF",   "600 Hz",   UiScalePreset::FullSweepFreqHz, 0.50f },
        { UiWidgetType::GrimyRelicKnob,      UiWidgetCategory::Knobs, "Grimy Road-Worn Knob",  "Chipped Enamel + Oxidized Brass Cap",   122.0f, 132.0f, UiKnobRole::SaturationPct,    "RELIC DRIVE",  "65 %",     UiScalePreset::PercentDryWet,   0.65f },
        { UiWidgetType::SteppedAttenuatorKnob,UiWidgetCategory::Knobs,"24-Step Attenuator",    "Mastering Detented Stepped Rotary",     126.0f, 136.0f, UiKnobRole::MasterVolume,     "STEPPED TRIM", "0.0 dB",   UiScalePreset::FineTrimDb,      0.50f },
        { UiWidgetType::CyberLuxeHaloKnob,   UiWidgetCategory::Knobs, "Cyber Halo Knob",       "Ribbed Carbon Dial + Cyan Halo Ring",   122.0f, 132.0f, UiKnobRole::ModulationDepth,  "CYBER MOD",    "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::NeumorphicGlassKnob, UiWidgetCategory::Knobs, "Frosted Glass Knob",    "Extruded Neumorphic Dial + Glow Arc",   122.0f, 132.0f, UiKnobRole::ModulationDepth,  "GLASS DIAL",   "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::SciFiHudRingKnob,    UiWidgetCategory::Knobs, "Sci-Fi HUD Dial",       "Starship HUD Segmented Energy Ring",    122.0f, 132.0f, UiKnobRole::ModulationRate,   "HUD FREQ",     "1.0 kHz",  UiScalePreset::ModRateHz,       0.50f },
        { UiWidgetType::ValveWheelRelicKnob, UiWidgetCategory::Knobs, "Valve Wheel Knob",      "Industrial Red Cast-Iron Handwheel",    122.0f, 132.0f, UiKnobRole::SaturationPct,    "VALVE DRIVE",  "60 %",     UiScalePreset::PercentDryWet,   0.60f },
        { UiWidgetType::SubmarineHelmKnob,   UiWidgetCategory::Knobs, "Submarine Helm Dial",   "Naval Brass 8-Spoke Ship Wheel",        122.0f, 132.0f, UiKnobRole::ModulationDepth,  "DEPTH HELM",   "60 %",     UiScalePreset::PercentDryWet,   0.60f },
        { UiWidgetType::TattooFlashKnob,     UiWidgetCategory::Knobs, "Tattoo Flash Knob",     "Traditional Sacred Ruby Heart Rotary",  122.0f, 132.0f, UiKnobRole::SaturationPct,    "HEART DRIVE",  "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::MetalSkullKnob,      UiWidgetCategory::Knobs, "Metal Skull Knob",      "3D Pewter Skull in Spiked Collar",      122.0f, 132.0f, UiKnobRole::PreampDrive,      "SKULL GAIN",   "66 %",     UiScalePreset::PercentDryWet,   0.66f },
        { UiWidgetType::FlowerDaisyKnob,     UiWidgetCategory::Knobs, "Flower Daisy Knob",     "60s Psychedelic Daisy Petal Rotary",    122.0f, 132.0f, UiKnobRole::ModulationDepth,  "FLOWER MOD",   "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::ReaperCryptKnob,     UiWidgetCategory::Knobs, "Reaper Crypt Knob",     "Necropolis Hourglass & Skull Dial",     122.0f, 132.0f, UiKnobRole::ReverbDecay,      "CRYPT DECAY",  "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::HellfireMagmaKnob,   UiWidgetCategory::Knobs, "Hellfire Magma Knob",   "Volcanic Obsidian & Lava Fissure",      122.0f, 132.0f, UiKnobRole::SaturationPct,    "MAGMA DRIVE",  "75 %",     UiScalePreset::PercentDryWet,   0.75f },
        { UiWidgetType::ApocalypseCogKnob,   UiWidgetCategory::Knobs, "Apocalypse Cog Knob",   "Rusted Wasteland Gear Cogwheel",        122.0f, 132.0f, UiKnobRole::SaturationPct,    "RUST DRIVE",   "55 %",     UiScalePreset::PercentDryWet,   0.55f },
        { UiWidgetType::PunkSpikedKnob,      UiWidgetCategory::Knobs, "Punk Spiked Knob",      "DIY Punk Studded Spiked Collar",        122.0f, 132.0f, UiKnobRole::PreampDrive,      "ANARCHY GAIN", "80 %",     UiScalePreset::PercentDryWet,   0.80f },
        { UiWidgetType::MeadowWoodSliceKnob, UiWidgetCategory::Knobs, "Wood Slice Knob",       "Carved Tree-Trunk Ring + Leaf",         122.0f, 132.0f, UiKnobRole::ModulationDepth,  "ORGANIC MIX",  "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::SkySunburstKnob,     UiWidgetCategory::Knobs, "Sunburst Cloud Knob",   "Golden Sun-Ray & Cloud Orb Dial",       122.0f, 132.0f, UiKnobRole::EqHighGain,       "SOLAR AIR",    "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::StormRaindropKnob,   UiWidgetCategory::Knobs, "Storm Lightning Knob",  "Wet Storm-Slate + Electric Arc",        122.0f, 132.0f, UiKnobRole::ModulationDepth,  "STORM MOD",    "50 %",     UiScalePreset::PercentDryWet,   0.50f },
        { UiWidgetType::GlacialIceCrystalKnob,UiWidgetCategory::Knobs,"Ice Crystal Knob",      "Faceted Frozen Glacial Snowflake",      122.0f, 132.0f, UiKnobRole::ReverbShimmer,    "GLACIAL MIX",  "55 %",     UiScalePreset::PercentDryWet,   0.55f },

        // Switches, Push Buttons, Faders & LEDs (13 styles)
        { UiWidgetType::ChromeToggleSwitch,    UiWidgetCategory::SwitchesAndLeds,   "Chrome Toggle Switch",  "3D Bat Lever + Hex Nut + Jewel LED",    76.0f,  84.0f },
        { UiWidgetType::IlluminatedPushButton, UiWidgetCategory::SwitchesAndLeds,   "Lit Push Button",       "Backlit Console Hardware Switch",       86.0f,  54.0f },
        { UiWidgetType::RockerPowerSwitch,     UiWidgetCategory::SwitchesAndLeds,   "Lit Rocker Switch",     "Illuminated 3D Power Rocker Switch",    68.0f,  82.0f },
        { UiWidgetType::LockingCageToggle,     UiWidgetCategory::SwitchesAndLeds,   "Locking Cage Toggle",   "Industrial Guarded Cage Toggle Switch", 76.0f,  86.0f },
        { UiWidgetType::FlatPillVectorSwitch,  UiWidgetCategory::SwitchesAndLeds,   "Flat Pill Vector Sw",   "Modern Minimalist Vector Capsule Pill", 86.0f,  50.0f },
        { UiWidgetType::NeumorphicPillButton,  UiWidgetCategory::SwitchesAndLeds,   "Frosted Pill Switch",   "Soft Extruded Neumorphic Glass Pill",   86.0f,  52.0f },
        { UiWidgetType::SciFiHologramSwitch,   UiWidgetCategory::SwitchesAndLeds,   "Sci-Fi HUD Switch",     "Glowing Starship Tactical HUD Pad",     86.0f,  52.0f },
        { UiWidgetType::NavalBrassToggle,      UiWidgetCategory::SwitchesAndLeds,   "Naval Brass Toggle",    "Riveted Submarine Brass Lever Switch",  76.0f,  84.0f },
        { UiWidgetType::PunkSafetyPinToggle,   UiWidgetCategory::SwitchesAndLeds,   "Punk Safety-Pin Sw",    "DIY Studded Pyramid & Safety-Pin",      76.0f,  84.0f },
        { UiWidgetType::HellfireEmberSwitch,   UiWidgetCategory::SwitchesAndLeds,   "Hellfire Ember Sw",     "Volcanic Obsidian & Glowing Magma Sw",  76.0f,  84.0f },
        { UiWidgetType::VerticalStudioFader,   UiWidgetCategory::SwitchesAndLeds,   "Console Studio Fader",  "Conductive-Plastic Fader + dB Scale",   76.0f, 196.0f },
        { UiWidgetType::HorizontalCrossfader,  UiWidgetCategory::SwitchesAndLeds,   "Horizontal Crossfader", "Horizontal Mix / Crossfader Rail",     188.0f,  66.0f },
        { UiWidgetType::JewelLed,              UiWidgetCategory::SwitchesAndLeds,   "Faceted Jewel Lamp",    "Glowing Chrome-Bezel Pilot Indicator",  44.0f,  44.0f },

        // Meters, Tube Windows & Digital / CRT Displays (18 styles)
        { UiWidgetType::AnalogVuMeter,         UiWidgetCategory::MetersAndDisplays, "Backlit VU GR Meter",   "Multi-Theme Analog Needle VU Meter",   228.0f, 118.0f },
        { UiWidgetType::RoundVintageVuMeter,   UiWidgetCategory::MetersAndDisplays, "Round Porthole VU",     "Circular Brass/Bakelite Bezel VU",     142.0f, 142.0f },
        { UiWidgetType::LedBargraphMeter,      UiWidgetCategory::MetersAndDisplays, "16-Seg LED Peak Meter", "Stereo Green/Amber/Red Peak Ladder",    68.0f, 176.0f },
        { UiWidgetType::DigitalTrimBargraph,   UiWidgetCategory::MetersAndDisplays, "IN/OUT Trim Bargraph",  "18-Seg Dual LED Bar + OUT-IN Delta",   126.0f,  40.0f },
        { UiWidgetType::GlowingTubeWindow,     UiWidgetCategory::MetersAndDisplays, "Glowing Vacuum Tube",   "12AX7/EL34 Cathode Porthole Window",   112.0f, 144.0f },
        { UiWidgetType::CrtOscilloscopeScreen, UiWidgetCategory::MetersAndDisplays, "CRT Waveform Scope",    "Phosphor CRT Graticule & Live Wave",   224.0f, 116.0f },
        { UiWidgetType::StereoGoniometerScope, UiWidgetCategory::MetersAndDisplays, "Lissajous Goniometer",  "Circular Stereo X/Y Phase Scope",      144.0f, 144.0f },
        { UiWidgetType::LcdParameterReadout,   UiWidgetCategory::MetersAndDisplays, "LCD Digital Readout",   "Backlit Rack Digital Parameter Screen", 216.0f,  68.0f },
        { UiWidgetType::NixieTubeDisplay,      UiWidgetCategory::MetersAndDisplays, "Nixie Tube Readout",    "Glowing Vintage Amber Nixie Display",  196.0f,  74.0f },
        { UiWidgetType::PastelShaderCurveMeter,UiWidgetCategory::MetersAndDisplays, "Pastel Shader Curve",   "Modern Spectral & EQ Transfer Shader", 224.0f, 116.0f },
        { UiWidgetType::CyberLuxeMultiBarMeter,UiWidgetCategory::MetersAndDisplays, "Cyber Multi-Bar Meter", "High-Density 8-Col Cyan/Amber Matrix", 196.0f, 180.0f },
        { UiWidgetType::NeumorphicDualArcMeter,UiWidgetCategory::MetersAndDisplays, "Frosted Glass VU",      "Neumorphic Extruded Frosted Arc VU",   224.0f, 116.0f },
        { UiWidgetType::SubmarinePortholeGauge,UiWidgetCategory::MetersAndDisplays, "Submarine Porthole VU", "Riveted Brass Diving Pressure Gauge",  152.0f, 152.0f },
        { UiWidgetType::ReaperScytheArchMeter, UiWidgetCategory::MetersAndDisplays, "Reaper Scythe Arch VU", "Gothic Crypt Arch + Scythe Needle",    196.0f, 180.0f },
        { UiWidgetType::HellfireHornedVuMeter, UiWidgetCategory::MetersAndDisplays, "Hellfire Horned VU",    "Horned Demon Magma Circular Gauge",    160.0f, 160.0f },
        { UiWidgetType::ApocalypseGeigerVuMeter,UiWidgetCategory::MetersAndDisplays,"Geiger Hazard VU",      "Fallout Radiation Trefoil Yellow VU",  224.0f, 116.0f },
        { UiWidgetType::PunkAnarchyVuMeter,    UiWidgetCategory::MetersAndDisplays, "Punk Anarchy VU",       "Spiked Collar + Graffiti Anarchy VU",  224.0f, 116.0f },
        { UiWidgetType::NatureElementalVuMeter,UiWidgetCategory::MetersAndDisplays, "Elemental Nature VU",   "Botanical Wood / Cloud / Ice VU",      224.0f, 116.0f },

        // Vintage Stickers, Studio Tape Strips, Frames, Badges & Hardware (38 items: 32 Stickers/Tape + 6 Frames/Badges)
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Smiley Face",   "Aged Grunge Smiley Decal + Editable Text",   96.0f, 110.0f, UiKnobRole::StageOrCustom, "HAVE A NICE CLIP", "ANALOG VIBES",    static_cast<UiScalePreset>(0),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Skull & Bones", "Worn Rock Skull & Crossbones Decal",         96.0f, 110.0f, UiKnobRole::StageOrCustom, "DANGER // LOUD",   "HIGH GAIN",       static_cast<UiScalePreset>(1),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Gold Star",     "Die-Cut 5-Point Studio Gold Star Decal",     96.0f, 110.0f, UiKnobRole::StageOrCustom, "GOLD STAR",        "",                static_cast<UiScalePreset>(2),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Tape: Console Scribble", "Torn Cream Masking Tape + Marker Text",     168.0f,  58.0f, UiKnobRole::StageOrCustom, "VOCAL COMP +3dB",  "DO NOT RESET",    static_cast<UiScalePreset>(3),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Tape: Stage Gaff Tape",  "Matte Black Cloth Gaffer Tape + Paint Pen", 156.0f,  38.0f, UiKnobRole::StageOrCustom, "DO NOT TOUCH!",    "",                static_cast<UiScalePreset>(4),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Tape: Embossed Label",   "3D Glossy Plastic Punched Label Tape",      156.0f,  38.0f, UiKnobRole::StageOrCustom, "MASTERING UNIT",   "",                static_cast<UiScalePreset>(5),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: High Voltage",  "Yellow/Black OSHA Lightning Bolt Plaque",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "HIGH VOLTAGE",     "450V B+ PLATE",   static_cast<UiScalePreset>(6),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Radioactive",   "Die-Cut Fallout Trefoil Warning Decal",      96.0f, 110.0f, UiKnobRole::StageOrCustom, "RADIOACTIVE",      "HOT SIGNAL",      static_cast<UiScalePreset>(7),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Balance Zen",   "Die-Cut Yin-Yang Studio Balance Decal",      96.0f, 110.0f, UiKnobRole::StageOrCustom, "BALANCE",          "ZEN",             static_cast<UiScalePreset>(8),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Peace Sign",    "Faded 70s Retro Peace Symbol Decal",         96.0f, 110.0f, UiKnobRole::StageOrCustom, "ANALOG PEACE",     "1972 SOUND",      static_cast<UiScalePreset>(9),  0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Magic 8-Ball",  "Scuffed Black Billiard 8-Ball Decal",        96.0f, 110.0f, UiKnobRole::StageOrCustom, "8-BALL SAYS",      "NO!",             static_cast<UiScalePreset>(10), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Studio Advisory","Black & White 3-Tier Explicit Plaque",     128.0f,  78.0f, UiKnobRole::StageOrCustom, "ADVISORY",         "EXPLICIT THD",    static_cast<UiScalePreset>(11), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: VIP Backstage", "Satin Concert Tour Laminate Pass",           96.0f, 110.0f, UiKnobRole::StageOrCustom, "WORLD TOUR '94",   "FOH ENGINEER",    static_cast<UiScalePreset>(12), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Fragile Stamp", "Red Stencil Flight-Case Vacuum Tube Plaque",148.0f,  68.0f, UiKnobRole::StageOrCustom, "FRAGILE",          "VACUUM TUBES",    static_cast<UiScalePreset>(13), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Area 51 Alien", "90s Neon-Green Alien Head Vinyl Decal",      96.0f, 110.0f, UiKnobRole::StageOrCustom, "AREA 51",          "SPACEY!",         static_cast<UiScalePreset>(14), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Biohazard",     "Industrial Orange/Black Biohazard Decal",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "BIOHAZARD",        "TOXIC BASS",      static_cast<UiScalePreset>(15), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Tape: Fix It In The Mix","Yellow/Black Diagonal Hazard-Stripe Tape",  168.0f,  52.0f, UiKnobRole::StageOrCustom, "FIX IT IN THE MIX","",                static_cast<UiScalePreset>(16), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Client Knob",   "Circular Unplugged 1/4\" TRS Jack Decal",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "CLIENT KNOB",      "UNPLUGGED",       static_cast<UiScalePreset>(17), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Phase Checked", "Green Factory QC Stamp w/ Checkmark & Ø",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "PHASE CHECKED",    "MONO SAFE",       static_cast<UiScalePreset>(18), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Ear Fatigue",   "Square Red/White Studio First-Aid Badge",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "EAR FATIGUE",      "TAKE A BREAK",    static_cast<UiScalePreset>(19), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: 2-Inch Reel",   "Circular 3-Slot NAB Magnetic Tape Reel",     96.0f, 110.0f, UiKnobRole::StageOrCustom, "2-INCH REEL",      "30 IPS MASTER",   static_cast<UiScalePreset>(20), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Sub Seismic",   "Yellow Plaque w/ Richter Seismograph Wave", 148.0f,  68.0f, UiKnobRole::StageOrCustom, "SUB BASS",         "SEISMIC ZONE",    static_cast<UiScalePreset>(21), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Cal Certified", "Brushed Silver Lab Calibration Tag (0 VU)", 148.0f,  68.0f, UiKnobRole::StageOrCustom, "CAL CERTIFIED",    "0 VU = +4 dBu",   static_cast<UiScalePreset>(22), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: 100% Raw Vocal","Circular Vintage Microphone Icon Badge",     96.0f, 110.0f, UiKnobRole::StageOrCustom, "100% RAW",         "NO PITCH CORR",   static_cast<UiScalePreset>(23), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: 3 AM Espresso", "Circular Dark-Roast Coffee Cup & Steam",     96.0f, 110.0f, UiKnobRole::StageOrCustom, "3 AM SESSION",     "ESPRESSO FUEL",   static_cast<UiScalePreset>(24), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Panic Button",  "Circular Red Arcade Push-Button Badge",      96.0f, 110.0f, UiKnobRole::StageOrCustom, "PANIC BUTTON",     "MUTE IF SUCK",    static_cast<UiScalePreset>(25), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Tape Op Stamp", "Red Rubber Ink-Stamp on Cream Tape Label",  148.0f,  68.0f, UiKnobRole::StageOrCustom, "TAPE OP",          "PRINT IT!",       static_cast<UiScalePreset>(26), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: 48V Phantom",   "High-Voltage Spark + Ribbon Mic Warning",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "48V PHANTOM",      "RIBBON KILLER",   static_cast<UiScalePreset>(27), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Tape: Final_Mix_v12.wav","Retro Studio DAT / Master Reel Spine Tape", 176.0f,  54.0f, UiKnobRole::StageOrCustom, "FINAL_MIX_V12_REAL.WAV", "PRINT MASTER", static_cast<UiScalePreset>(28), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: Space Heater",  "Circular Orange Class-A Thermal Flame",      96.0f, 110.0f, UiKnobRole::StageOrCustom, "CLASS-A",          "SPACE HEATER",    static_cast<UiScalePreset>(29), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Sticker: What Headroom?","VU Needle Bent Against +3 dB Stop Pin",      96.0f, 110.0f, UiKnobRole::StageOrCustom, "HEADROOM",         "WHAT HEADROOM?",  static_cast<UiScalePreset>(30), 0.5f },
        { UiWidgetType::VintageSticker,        UiWidgetCategory::FramesAndBadges,   "Custom Image Sticker",   "Upload Any PNG/JPG/BMP as Die-Cut Decal",    96.0f, 110.0f, UiKnobRole::StageOrCustom, "CUSTOM DECAL",     "CLICK UPLOAD",    static_cast<UiScalePreset>(31), 0.5f },
        { UiWidgetType::SectionFrame,          UiWidgetCategory::FramesAndBadges,   "Engraved Section Box",  "Recessed Frame + Header Title Pill",       400.0f, 440.0f },
        { UiWidgetType::RivetedNameplate,      UiWidgetCategory::FramesAndBadges,   "Riveted Metal Badge",   "6 Badge Styles (Aluminum/Brass/Glass)",    126.0f,  68.0f },
        { UiWidgetType::SilkscreenLabel,       UiWidgetCategory::FramesAndBadges,   "Silkscreen Text Label", "Custom Font Size & Colour Typography",     140.0f,  26.0f },
        { UiWidgetType::VentilationGrille,     UiWidgetCategory::FramesAndBadges,   "Tube Cooling Louvers",  "Recessed Amplifier Ventilation Slots",     136.0f,  72.0f },
        { UiWidgetType::RackHandlePair,        UiWidgetCategory::FramesAndBadges,   "19\" Rack Grab Handle", "Heavy Cast-Chrome Rack Pull Handle",        36.0f, 220.0f },
        { UiWidgetType::PatchbayJackSocket,    UiWidgetCategory::FramesAndBadges,   "1/4\" TRS Patch Jack",  "Front-Panel Chrome Hex Patch Socket",       62.0f,  68.0f }
    };
    return kCatalog;
}

inline const char* getChassisFinishName(gui::ChassisFinish f) noexcept {
    switch (f) {
        case gui::ChassisFinish::AnodizedTeal:      return "ANODIZED TEAL";
        case gui::ChassisFinish::BritishSlateBlue:  return "SLATE BLUE";
        case gui::ChassisFinish::ObsidianBlack:     return "OBSIDIAN BLACK";
        case gui::ChassisFinish::BrushedSilver:     return "BRUSHED SILVER";
        case gui::ChassisFinish::VintageGold:       return "VINTAGE GOLD";
        case gui::ChassisFinish::HammertoneGreen:   return "HAMMERTONE GREEN";
        case gui::ChassisFinish::VintageCream:      return "VINTAGE CREAM";
        case gui::ChassisFinish::CrimsonMastering:  return "CRIMSON MASTER";
        case gui::ChassisFinish::CobaltStudioBlue:  return "COBALT BLUE";
        case gui::ChassisFinish::WarmCopperBronze:  return "COPPER BRONZE";
        case gui::ChassisFinish::SunburstAmberWood: return "SUNBURST WOOD";
        case gui::ChassisFinish::RoyalNavyBlue:     return "ROYAL NAVY BLUE";
        case gui::ChassisFinish::CyberpunkViolet:   return "CYBERPUNK VIOLET";
        case gui::ChassisFinish::ArcticWhiteEnamel: return "ARCTIC WHITE";
        default:                                    return "OBSIDIAN BLACK";
    }
}

inline const char* getChassisFinishEnumCode(gui::ChassisFinish f) noexcept {
    switch (f) {
        case gui::ChassisFinish::AnodizedTeal:      return "avra::gui::ChassisFinish::AnodizedTeal";
        case gui::ChassisFinish::BritishSlateBlue:  return "avra::gui::ChassisFinish::BritishSlateBlue";
        case gui::ChassisFinish::ObsidianBlack:     return "avra::gui::ChassisFinish::ObsidianBlack";
        case gui::ChassisFinish::BrushedSilver:     return "avra::gui::ChassisFinish::BrushedSilver";
        case gui::ChassisFinish::VintageGold:       return "avra::gui::ChassisFinish::VintageGold";
        case gui::ChassisFinish::HammertoneGreen:   return "avra::gui::ChassisFinish::HammertoneGreen";
        case gui::ChassisFinish::VintageCream:      return "avra::gui::ChassisFinish::VintageCream";
        case gui::ChassisFinish::CrimsonMastering:  return "avra::gui::ChassisFinish::CrimsonMastering";
        case gui::ChassisFinish::CobaltStudioBlue:  return "avra::gui::ChassisFinish::CobaltStudioBlue";
        case gui::ChassisFinish::WarmCopperBronze:  return "avra::gui::ChassisFinish::WarmCopperBronze";
        case gui::ChassisFinish::SunburstAmberWood: return "avra::gui::ChassisFinish::SunburstAmberWood";
        case gui::ChassisFinish::RoyalNavyBlue:     return "avra::gui::ChassisFinish::RoyalNavyBlue";
        case gui::ChassisFinish::CyberpunkViolet:   return "avra::gui::ChassisFinish::CyberpunkViolet";
        case gui::ChassisFinish::ArcticWhiteEnamel: return "avra::gui::ChassisFinish::ArcticWhiteEnamel";
        default:                                    return "avra::gui::ChassisFinish::ObsidianBlack";
    }
}

inline const char* getSurfaceGrainName(gui::SurfaceGrain g) noexcept {
    switch (g) {
        case gui::SurfaceGrain::VerticalBrushed:     return "VERTICAL BRUSHED";
        case gui::SurfaceGrain::HorizontalBrushed:   return "HORIZ BRUSHED";
        case gui::SurfaceGrain::MatteSmooth:         return "MATTE ANODIZED";
        case gui::SurfaceGrain::CarbonWeave:         return "CARBON WEAVE";
        case gui::SurfaceGrain::WrinklePowderCoat:   return "WRINKLE COAT";
        case gui::SurfaceGrain::DiamondCrosshatch:   return "DIAMOND KNURL";
        case gui::SurfaceGrain::BeadBlastedMatte:    return "BEAD BLASTED";
        case gui::SurfaceGrain::HammertoneDimpled:   return "HAMMERTONE DIMPLE";
        case gui::SurfaceGrain::WalnutWoodCheeks:    return "WALNUT WOOD CHEEK";
        case gui::SurfaceGrain::PerforatedHexMesh:   return "PERFORATED MESH";
        case gui::SurfaceGrain::FullCarvedWoodGrain: return "CARVED WOOD PANEL";
        case gui::SurfaceGrain::TolexAmpLeather:     return "NEXTEL SUEDE COAT";
        case gui::SurfaceGrain::GrimyRoadWornRelic:  return "GRIMY RELIC WEAR";
        case gui::SurfaceGrain::OxidizedPatina:      return "OXIDIZED PATINA";
        case gui::SurfaceGrain::SleekGlassAcrylic:   return "SLEEK GLASS";
        case gui::SurfaceGrain::TitaniumMicroGrid:   return "TITANIUM GRID";
        case gui::SurfaceGrain::DiamondTreadplate:   return "MACHINED KNURL PLATE";
        case gui::SurfaceGrain::WovenTweed:          return "LINEN ACOUSTIC WEAVE";
        default:                                     return "VERTICAL BRUSHED";
    }
}

inline const char* getSurfaceGrainEnumCode(gui::SurfaceGrain g) noexcept {
    switch (g) {
        case gui::SurfaceGrain::VerticalBrushed:     return "avra::gui::SurfaceGrain::VerticalBrushed";
        case gui::SurfaceGrain::HorizontalBrushed:   return "avra::gui::SurfaceGrain::HorizontalBrushed";
        case gui::SurfaceGrain::MatteSmooth:         return "avra::gui::SurfaceGrain::MatteSmooth";
        case gui::SurfaceGrain::CarbonWeave:         return "avra::gui::SurfaceGrain::CarbonWeave";
        case gui::SurfaceGrain::WrinklePowderCoat:   return "avra::gui::SurfaceGrain::WrinklePowderCoat";
        case gui::SurfaceGrain::DiamondCrosshatch:   return "avra::gui::SurfaceGrain::DiamondCrosshatch";
        case gui::SurfaceGrain::BeadBlastedMatte:    return "avra::gui::SurfaceGrain::BeadBlastedMatte";
        case gui::SurfaceGrain::HammertoneDimpled:   return "avra::gui::SurfaceGrain::HammertoneDimpled";
        case gui::SurfaceGrain::WalnutWoodCheeks:    return "avra::gui::SurfaceGrain::WalnutWoodCheeks";
        case gui::SurfaceGrain::PerforatedHexMesh:   return "avra::gui::SurfaceGrain::PerforatedHexMesh";
        case gui::SurfaceGrain::FullCarvedWoodGrain: return "avra::gui::SurfaceGrain::FullCarvedWoodGrain";
        case gui::SurfaceGrain::TolexAmpLeather:     return "avra::gui::SurfaceGrain::TolexAmpLeather";
        case gui::SurfaceGrain::GrimyRoadWornRelic:  return "avra::gui::SurfaceGrain::GrimyRoadWornRelic";
        case gui::SurfaceGrain::OxidizedPatina:      return "avra::gui::SurfaceGrain::OxidizedPatina";
        case gui::SurfaceGrain::SleekGlassAcrylic:   return "avra::gui::SurfaceGrain::SleekGlassAcrylic";
        case gui::SurfaceGrain::TitaniumMicroGrid:   return "avra::gui::SurfaceGrain::TitaniumMicroGrid";
        case gui::SurfaceGrain::DiamondTreadplate:   return "avra::gui::SurfaceGrain::DiamondTreadplate";
        case gui::SurfaceGrain::WovenTweed:          return "avra::gui::SurfaceGrain::WovenTweed";
        default:                                     return "avra::gui::SurfaceGrain::VerticalBrushed";
    }
}

inline const char* getHardwareStyleName(gui::ChassisHardwareStyle h) noexcept {
    switch (h) {
        case gui::ChassisHardwareStyle::CornerScrews4:      return "4X CORNER BOLTS";
        case gui::ChassisHardwareStyle::RackEarsAndBolts:   return "19\" RACK EARS";
        case gui::ChassisHardwareStyle::IndustrialRivets8:  return "8X RAIL MOUNT";
        case gui::ChassisHardwareStyle::HeavyPerimeter12:   return "12X PERIMETER";
        case gui::ChassisHardwareStyle::CornerGussetPlates: return "4X CORNER PLATES";
        case gui::ChassisHardwareStyle::CleanScrewless:     return "CLEAN SCREWLESS";
        case gui::ChassisHardwareStyle::LeatherAmpCorners:  return "COUNTERSUNK BRASS";
        default:                                            return "4X CORNER BOLTS";
    }
}

inline const char* getHardwareStyleEnumCode(gui::ChassisHardwareStyle h) noexcept {
    switch (h) {
        case gui::ChassisHardwareStyle::CornerScrews4:      return "avra::gui::ChassisHardwareStyle::CornerScrews4";
        case gui::ChassisHardwareStyle::RackEarsAndBolts:   return "avra::gui::ChassisHardwareStyle::RackEarsAndBolts";
        case gui::ChassisHardwareStyle::IndustrialRivets8:  return "avra::gui::ChassisHardwareStyle::IndustrialRivets8";
        case gui::ChassisHardwareStyle::HeavyPerimeter12:   return "avra::gui::ChassisHardwareStyle::HeavyPerimeter12";
        case gui::ChassisHardwareStyle::CornerGussetPlates: return "avra::gui::ChassisHardwareStyle::CornerGussetPlates";
        case gui::ChassisHardwareStyle::CleanScrewless:     return "avra::gui::ChassisHardwareStyle::CleanScrewless";
        case gui::ChassisHardwareStyle::LeatherAmpCorners:  return "avra::gui::ChassisHardwareStyle::LeatherAmpCorners";
        default:                                            return "avra::gui::ChassisHardwareStyle::CornerScrews4";
    }
}

inline const char* getScrewHeadStyleName(gui::ScrewHeadStyle s) noexcept {
    switch (s) {
        case gui::ScrewHeadStyle::PhillipsCross:     return "PHILLIPS CROSS";
        case gui::ScrewHeadStyle::SocketHexAllen:    return "HEX ALLEN SOCKET";
        case gui::ScrewHeadStyle::SlottedFlathead:   return "SLOTTED FLATHEAD";
        case gui::ScrewHeadStyle::TorxStar6:         return "6-PT TORX STAR";
        case gui::ScrewHeadStyle::GoldPlatedBrass:   return "24K GOLD BRASS";
        case gui::ScrewHeadStyle::BlackOxideStealth: return "BLACK OXIDE";
        case gui::ScrewHeadStyle::DomedChromeRivet:  return "CHROME DOME RIVET";
        case gui::ScrewHeadStyle::CountersunkFlush:  return "FLUSH COUNTERSUNK";
        case gui::ScrewHeadStyle::KnurledThumbscrew: return "KNURLED THUMB";
        case gui::ScrewHeadStyle::RustedRelicBolt:   return "RUSTED RELIC BOLT";
        default:                                     return "PHILLIPS CROSS";
    }
}

inline const char* getScrewHeadStyleEnumCode(gui::ScrewHeadStyle s) noexcept {
    switch (s) {
        case gui::ScrewHeadStyle::PhillipsCross:     return "avra::gui::ScrewHeadStyle::PhillipsCross";
        case gui::ScrewHeadStyle::SocketHexAllen:    return "avra::gui::ScrewHeadStyle::SocketHexAllen";
        case gui::ScrewHeadStyle::SlottedFlathead:   return "avra::gui::ScrewHeadStyle::SlottedFlathead";
        case gui::ScrewHeadStyle::TorxStar6:         return "avra::gui::ScrewHeadStyle::TorxStar6";
        case gui::ScrewHeadStyle::GoldPlatedBrass:   return "avra::gui::ScrewHeadStyle::GoldPlatedBrass";
        case gui::ScrewHeadStyle::BlackOxideStealth: return "avra::gui::ScrewHeadStyle::BlackOxideStealth";
        case gui::ScrewHeadStyle::DomedChromeRivet:  return "avra::gui::ScrewHeadStyle::DomedChromeRivet";
        case gui::ScrewHeadStyle::CountersunkFlush:  return "avra::gui::ScrewHeadStyle::CountersunkFlush";
        case gui::ScrewHeadStyle::KnurledThumbscrew: return "avra::gui::ScrewHeadStyle::KnurledThumbscrew";
        case gui::ScrewHeadStyle::RustedRelicBolt:   return "avra::gui::ScrewHeadStyle::RustedRelicBolt";
        default:                                     return "avra::gui::ScrewHeadStyle::PhillipsCross";
    }
}

inline const char* getBadgeVisualStyleName(gui::BadgeVisualStyle b) noexcept {
    switch (b) {
        case gui::BadgeVisualStyle::CastBrassPlaque:      return "CAST BRASS PLAQUE";
        case gui::BadgeVisualStyle::SleekLaserGlass:      return "SLEEK LASER GLASS";
        case gui::BadgeVisualStyle::GrimyMilitaryTag:     return "GRIMY MILITARY TAG";
        case gui::BadgeVisualStyle::VintageCreamEmblem:   return "CREAM ENAMEL";
        case gui::BadgeVisualStyle::MinimalSilkscreen:    return "SILKSCREEN STAMP";
        case gui::BadgeVisualStyle::BrushedAluminumPlate:
        default:                                          return "BRUSHED ALUMINUM";
    }
}

inline const char* getBadgeVisualStyleEnumCode(gui::BadgeVisualStyle b) noexcept {
    switch (b) {
        case gui::BadgeVisualStyle::CastBrassPlaque:      return "avra::gui::BadgeVisualStyle::CastBrassPlaque";
        case gui::BadgeVisualStyle::SleekLaserGlass:      return "avra::gui::BadgeVisualStyle::SleekLaserGlass";
        case gui::BadgeVisualStyle::GrimyMilitaryTag:     return "avra::gui::BadgeVisualStyle::GrimyMilitaryTag";
        case gui::BadgeVisualStyle::VintageCreamEmblem:   return "avra::gui::BadgeVisualStyle::VintageCreamEmblem";
        case gui::BadgeVisualStyle::MinimalSilkscreen:    return "avra::gui::BadgeVisualStyle::MinimalSilkscreen";
        case gui::BadgeVisualStyle::BrushedAluminumPlate:
        default:                                          return "avra::gui::BadgeVisualStyle::BrushedAluminumPlate";
    }
}

inline const char* getStickerPresetName(gui::StickerPreset p) noexcept {
    switch (p) {
        case gui::StickerPreset::GrungeSmiley:         return "SMILEY FACE";
        case gui::StickerPreset::SkullAndBones:        return "SKULL & BONES";
        case gui::StickerPreset::RockStarEmblem:       return "GOLD STAR";
        case gui::StickerPreset::ScribbleMaskingTape:  return "SCRIBBLE TAPE";
        case gui::StickerPreset::BlackGaffTape:        return "STAGE GAFF TAPE";
        case gui::StickerPreset::EmbossedDymoStrip:    return "EMBOSSED LABEL TAPE";
        case gui::StickerPreset::HighVoltageBolt:      return "HIGH VOLTAGE";
        case gui::StickerPreset::RadioactiveTrefoil:   return "RADIOACTIVE";
        case gui::StickerPreset::YinYangCircle:        return "BALANCE - ZEN";
        case gui::StickerPreset::RetroPeaceSign:       return "PEACE SIGN";
        case gui::StickerPreset::MagicEightBall:       return "8-BALL SAYS - NO!";
        case gui::StickerPreset::AdvisoryExplicit:     return "STUDIO ADVISORY";
        case gui::StickerPreset::BackstageVipPass:     return "VIP BACKSTAGE";
        case gui::StickerPreset::FragileFlightStamp:   return "FRAGILE STAMP";
        case gui::StickerPreset::AlienHeadDecal:       return "AREA 51 - SPACEY!";
        case gui::StickerPreset::BiohazardWarning:     return "BIOHAZARD";
        case gui::StickerPreset::FixItInTheMix:        return "FIX IT IN THE MIX";
        case gui::StickerPreset::ClientKnobUnplugged:  return "CLIENT KNOB UNPLUGGED";
        case gui::StickerPreset::PhaseCheckedMonoSafe: return "PHASE CHECKED MONO";
        case gui::StickerPreset::EarFatigueFirstAid:   return "EAR FATIGUE BREAK";
        case gui::StickerPreset::TwoInchReel30Ips:     return "2-INCH REEL 30 IPS";
        case gui::StickerPreset::SubBassSeismicZone:   return "SUB BASS SEISMIC";
        case gui::StickerPreset::CalCertified0Vu:      return "CAL CERTIFIED 0 VU";
        case gui::StickerPreset::RawNoPitchCorr:       return "100% RAW VOCAL";
        case gui::StickerPreset::ThreeAmEspressoFuel:  return "3 AM ESPRESSO FUEL";
        case gui::StickerPreset::PanicButtonMute:      return "PANIC BUTTON MUTE";
        case gui::StickerPreset::TapeOpPrintIt:        return "TAPE OP PRINT IT!";
        case gui::StickerPreset::PhantomRibbonKiller:  return "48V RIBBON KILLER";
        case gui::StickerPreset::FinalMixV12Wav:       return "FINAL_MIX_V12.WAV";
        case gui::StickerPreset::ClassASpaceHeater:    return "CLASS-A SPACE HEATER";
        case gui::StickerPreset::HeadroomWhatHeadroom: return "WHAT HEADROOM?";
        case gui::StickerPreset::CustomImageDecal:     return "CUSTOM IMAGE";
        default:                                       return "SMILEY FACE";
    }
}

inline const char* getStickerPresetEnumCode(gui::StickerPreset p) noexcept {
    switch (p) {
        case gui::StickerPreset::GrungeSmiley:         return "avra::gui::StickerPreset::GrungeSmiley";
        case gui::StickerPreset::SkullAndBones:        return "avra::gui::StickerPreset::SkullAndBones";
        case gui::StickerPreset::RockStarEmblem:       return "avra::gui::StickerPreset::RockStarEmblem";
        case gui::StickerPreset::ScribbleMaskingTape:  return "avra::gui::StickerPreset::ScribbleMaskingTape";
        case gui::StickerPreset::BlackGaffTape:        return "avra::gui::StickerPreset::BlackGaffTape";
        case gui::StickerPreset::EmbossedDymoStrip:    return "avra::gui::StickerPreset::EmbossedDymoStrip";
        case gui::StickerPreset::HighVoltageBolt:      return "avra::gui::StickerPreset::HighVoltageBolt";
        case gui::StickerPreset::RadioactiveTrefoil:   return "avra::gui::StickerPreset::RadioactiveTrefoil";
        case gui::StickerPreset::YinYangCircle:        return "avra::gui::StickerPreset::YinYangCircle";
        case gui::StickerPreset::RetroPeaceSign:       return "avra::gui::StickerPreset::RetroPeaceSign";
        case gui::StickerPreset::MagicEightBall:       return "avra::gui::StickerPreset::MagicEightBall";
        case gui::StickerPreset::AdvisoryExplicit:     return "avra::gui::StickerPreset::AdvisoryExplicit";
        case gui::StickerPreset::BackstageVipPass:     return "avra::gui::StickerPreset::BackstageVipPass";
        case gui::StickerPreset::FragileFlightStamp:   return "avra::gui::StickerPreset::FragileFlightStamp";
        case gui::StickerPreset::AlienHeadDecal:       return "avra::gui::StickerPreset::AlienHeadDecal";
        case gui::StickerPreset::BiohazardWarning:     return "avra::gui::StickerPreset::BiohazardWarning";
        case gui::StickerPreset::FixItInTheMix:        return "avra::gui::StickerPreset::FixItInTheMix";
        case gui::StickerPreset::ClientKnobUnplugged:  return "avra::gui::StickerPreset::ClientKnobUnplugged";
        case gui::StickerPreset::PhaseCheckedMonoSafe: return "avra::gui::StickerPreset::PhaseCheckedMonoSafe";
        case gui::StickerPreset::EarFatigueFirstAid:   return "avra::gui::StickerPreset::EarFatigueFirstAid";
        case gui::StickerPreset::TwoInchReel30Ips:     return "avra::gui::StickerPreset::TwoInchReel30Ips";
        case gui::StickerPreset::SubBassSeismicZone:   return "avra::gui::StickerPreset::SubBassSeismicZone";
        case gui::StickerPreset::CalCertified0Vu:      return "avra::gui::StickerPreset::CalCertified0Vu";
        case gui::StickerPreset::RawNoPitchCorr:       return "avra::gui::StickerPreset::RawNoPitchCorr";
        case gui::StickerPreset::ThreeAmEspressoFuel:  return "avra::gui::StickerPreset::ThreeAmEspressoFuel";
        case gui::StickerPreset::PanicButtonMute:      return "avra::gui::StickerPreset::PanicButtonMute";
        case gui::StickerPreset::TapeOpPrintIt:        return "avra::gui::StickerPreset::TapeOpPrintIt";
        case gui::StickerPreset::PhantomRibbonKiller:  return "avra::gui::StickerPreset::PhantomRibbonKiller";
        case gui::StickerPreset::FinalMixV12Wav:       return "avra::gui::StickerPreset::FinalMixV12Wav";
        case gui::StickerPreset::ClassASpaceHeater:    return "avra::gui::StickerPreset::ClassASpaceHeater";
        case gui::StickerPreset::HeadroomWhatHeadroom: return "avra::gui::StickerPreset::HeadroomWhatHeadroom";
        case gui::StickerPreset::CustomImageDecal:     return "avra::gui::StickerPreset::CustomImageDecal";
        default:                                       return "avra::gui::StickerPreset::GrungeSmiley";
    }
}

inline const char* getScalePresetName(UiScalePreset p) noexcept {
    switch (p) {
        case UiScalePreset::LinearDb:          return "-16..+16 dB";
        case UiScalePreset::GainLineMic:       return "-20..+80 dB";
        case UiScalePreset::LowFreqHz:         return "LOW Hz (OFF..220)";
        case UiScalePreset::MidFreqHz:         return "MID kHz (OFF..7.2)";
        case UiScalePreset::HighPassHz:        return "HPF Hz (OFF..300)";
        case UiScalePreset::ThresholdDb:       return "THRESH (+10..-20)";
        case UiScalePreset::RatioSteps:        return "RATIO (1.5..6:1)";
        case UiScalePreset::RecoveryAuto:      return "RECOV (100..A2)";
        case UiScalePreset::PercentDryWet:     return "MIX (0..100%)";
        case UiScalePreset::MasterVolumeDb:    return "MASTER (-\xE2\x88\x9E..0..+24dB)";
        case UiScalePreset::DriveZeroTo36Db:   return "DRIVE (0..+36dB)";
        case UiScalePreset::FineTrimDb:        return "FINE (-6..+6dB)";
        case UiScalePreset::AttackFastMs:      return "ATK (0.1..30ms)";
        case UiScalePreset::AttackFet1176:     return "ATK FET (1..7)";
        case UiScalePreset::AttackSlowMs:      return "ATK (1..150ms)";
        case UiScalePreset::ReleaseFastMs:     return "REL (30..600ms)";
        case UiScalePreset::ReleaseVariMuSec:  return "REL (0.1..2.5s)";
        case UiScalePreset::ThresholdDeepDb:   return "THRESH (0..-36dB)";
        case UiScalePreset::LimiterCeilingDb:  return "CEIL (-12..0dBFS)";
        case UiScalePreset::PeakReduction0100: return "PEAK RED (0..100)";
        case UiScalePreset::RatioFetLim:       return "RATIO (4..20..ALL)";
        case UiScalePreset::RatioGentle:       return "RATIO (1.1..3:1)";
        case UiScalePreset::HighAirFreqKhz:    return "AIR kHz (2.5..20)";
        case UiScalePreset::FullSweepFreqHz:   return "SWEEP (20..18kHz)";
        case UiScalePreset::ModRateHz:         return "RATE (0.1..6Hz)";
        case UiScalePreset::DelayTimeMs:       return "TIME (25..1500ms)";
        case UiScalePreset::ReverbDecaySec:    return "DECAY (0.4..10s)";
        case UiScalePreset::StereoWidthPct:    return "WIDTH (MONO..200%)";
        case UiScalePreset::CoreMatrixNiFeSt:  return "CORE (Ni/Fe/St)";
        case UiScalePreset::DialOneToTen:      return "DIAL (1..10)";
        case UiScalePreset::DialZeroToTen:     return "DIAL (0..10)";
        case UiScalePreset::StereoPanLCR:      return "PAN (L100..C..R100)";
        case UiScalePreset::FreqLowShelf1073:  return "LOW SHELF (35..220Hz)";
        case UiScalePreset::MixDryWet:         return "MIX (DRY..WET)";
        case UiScalePreset::MakeupGain0To20Db: return "MAKE-UP (0..20dB)";
        default:                               return "-16..+16 dB";
    }
}

inline const char* getScalePresetDropdownDescription(UiScalePreset p) noexcept {
    switch (p) {
        case UiScalePreset::LinearDb:          return "-16 .. 0 .. +16 dB  (Console / EQ Boost-Cut)";
        case UiScalePreset::GainLineMic:       return "-20 .. +80 dB  (Mic / Line Preamp Gain)";
        case UiScalePreset::LowFreqHz:         return "OFF, 35, 60, 110, 220 Hz  (Low Band Hz)";
        case UiScalePreset::MidFreqHz:         return "OFF, .36, .70, 1.6, 3.2, 4.8, 7.2 kHz  (Mid Bell)";
        case UiScalePreset::HighPassHz:        return "OFF, 50, 80, 160, 300 Hz  (HPF Cutoff)";
        case UiScalePreset::ThresholdDb:       return "+10 .. 0 .. -10 .. -20 dB  (Studio Threshold)";
        case UiScalePreset::RatioSteps:        return "1.5:1, 2:1, 3:1, 4:1, 6:1  (Bus Comp Ratio)";
        case UiScalePreset::RecoveryAuto:      return "100, 400, 800, 1500 ms, A1, A2  (Auto Recovery)";
        case UiScalePreset::PercentDryWet:     return "0, 25, 50, 75, 100 %  (Parallel Mix / Sat)";
        case UiScalePreset::MasterVolumeDb:    return "-\xE2\x88\x9E, -18, -12, -6, 0, +6, +12, +18, +24 dB  (Master Vol)";
        case UiScalePreset::DriveZeroTo36Db:   return "0, 6, 12, 18, 24, 30, 36 dB  (Tube / FET Drive)";
        case UiScalePreset::FineTrimDb:        return "-6, -4, -2, 0, +2, +4, +6 dB  (Mastering Fine Trim)";
        case UiScalePreset::AttackFastMs:      return "0.1, 0.3, 1, 3, 10, 30 ms  (Fast Comp Attack)";
        case UiScalePreset::AttackFet1176:     return "1 (800us) .. 4 .. 7 (20us)  (Fast FET Attack Dial)";
        case UiScalePreset::AttackSlowMs:      return "1, 5, 15, 30, 60, 100, 150 ms  (Slow Opto/Vari-Mu)";
        case UiScalePreset::ReleaseFastMs:     return "30, 60, 120, 250, 400, 600 ms  (Fast Comp Release)";
        case UiScalePreset::ReleaseVariMuSec:  return "0.1, 0.2, 0.4, 0.8, 1.5, 2.5 s  (Vari-Mu Recovery)";
        case UiScalePreset::ThresholdDeepDb:   return "0, -6, -12, -18, -24, -30, -36 dB  (Deep Threshold)";
        case UiScalePreset::LimiterCeilingDb:  return "-12, -6, -3, -1.5, -0.5, 0.0 dBFS  (Peak Ceiling)";
        case UiScalePreset::PeakReduction0100: return "0, 20, 40, 60, 80, 100  (Optical Peak Reduction)";
        case UiScalePreset::RatioFetLim:       return "4:1, 8:1, 12:1, 20:1, ALL  (FET Limiter Ratios)";
        case UiScalePreset::RatioGentle:       return "1.1:1, 1.3:1, 1.5:1, 2:1, 3:1  (Mastering Ratio)";
        case UiScalePreset::HighAirFreqKhz:    return "2.5, 5, 8, 10, 12, 16, 20 kHz  (High Shelf / Air)";
        case UiScalePreset::FullSweepFreqHz:   return "20, 60, 200, 600, 2k, 6k, 18k Hz  (Full Filter Sweep)";
        case UiScalePreset::ModRateHz:         return "0.1, 0.3, 0.8, 1.5, 3.0, 6.0 Hz  (Modulation Rate)";
        case UiScalePreset::DelayTimeMs:       return "25, 100, 250, 500, 900, 1500 ms  (Echo / BBD Time)";
        case UiScalePreset::ReverbDecaySec:    return "0.4, 1.2, 2.5, 4.5, 7.0, 10.0 s  (Reverb Decay)";
        case UiScalePreset::StereoWidthPct:    return "MONO, 50, 100, 125, 150, 200 %  (Stereo Width)";
        case UiScalePreset::CoreMatrixNiFeSt:  return "NICKEL, Ni/Fe, IRON, Fe/St, STEEL  (3-Core Matrix)";
        case UiScalePreset::DialOneToTen:      return "1, 2, 3, 4, 5, 6, 7, 8, 9, 10  (Classic 1-10 Dial)";
        case UiScalePreset::DialZeroToTen:     return "0, 2, 4, 6, 8, 10  (Classic 0-10 Analog Dial)";
        case UiScalePreset::StereoPanLCR:      return "L100, L50, C, R50, R100  (Studio Pan / Balance)";
        case UiScalePreset::FreqLowShelf1073:  return "35, 60, 110, 220 Hz  (British Console Low Shelf Hz)";
        case UiScalePreset::MixDryWet:         return "DRY, 25, 50, 75, WET  (Parallel / Master Mix)";
        case UiScalePreset::MakeupGain0To20Db: return "0, 4, 8, 12, 16, 20 dB  (Compressor Make-Up Gain)";
        default:                               return "-16 .. 0 .. +16 dB";
    }
}

inline std::vector<std::string> getScalePresetLabels(
    UiScalePreset p,
    bool& outFirstOrange,
    bool& outLastOrange,
    std::string_view widgetTitle = "") {
    outFirstOrange = false;
    outLastOrange  = false;
    if (widgetTitle == "MIX" || widgetTitle == "PAR MIX" || widgetTitle == "MASTER MIX") {
        return {"DRY", "25", "50", "75", "WET"};
    }
    switch (p) {
        case UiScalePreset::GainLineMic:
            return {"-20", "-10", "0", "10", "20", "30", "40", "50", "60", "70", "80"};
        case UiScalePreset::LowFreqHz:
            outFirstOrange = true;
            return {"OFF", "35", "60", "110", "220"};
        case UiScalePreset::FreqLowShelf1073:
            return {"35", "60", "110", "220"};
        case UiScalePreset::MidFreqHz:
            outFirstOrange = true;
            return {"OFF", ".36", ".70", "1.6", "3.2", "4.8", "7.2"};
        case UiScalePreset::HighPassHz:
            outFirstOrange = true;
            return {"OFF", "50", "80", "160", "300"};
        case UiScalePreset::ThresholdDb:
            return {"-20", "-15", "-10", "-5", "0", "+5", "+10"};
        case UiScalePreset::RatioSteps:
            return {"1.5:1", "2:1", "3:1", "4:1", "6:1"};
        case UiScalePreset::RecoveryAuto:
            outLastOrange = true;
            return {"100", "400", "800", "1500", "A1", "A2"};
        case UiScalePreset::PercentDryWet:
            return {"0", "25", "50", "75", "100"};
        case UiScalePreset::MixDryWet:
            return {"DRY", "25", "50", "75", "WET"};
        case UiScalePreset::MakeupGain0To20Db:
            return {"0", "4", "8", "12", "16", "20"};
        case UiScalePreset::MasterVolumeDb:
            outFirstOrange = true;
            return {"-\xE2\x88\x9E", "-18", "-12", "-6", "0", "+6", "+12", "+18", "+24"};
        case UiScalePreset::DriveZeroTo36Db:
            return {"0", "6", "12", "18", "24", "30", "36"};
        case UiScalePreset::FineTrimDb:
            return {"-6", "-4", "-2", "0", "+2", "+4", "+6"};
        case UiScalePreset::AttackFastMs:
            return {"0.1", "0.3", "1", "3", "10", "30"};
        case UiScalePreset::AttackFet1176:
            outLastOrange = true;
            return {"1", "2", "3", "4", "5", "6", "7"};
        case UiScalePreset::AttackSlowMs:
            return {"1", "5", "15", "30", "60", "100", "150"};
        case UiScalePreset::ReleaseFastMs:
            return {"30", "60", "120", "250", "400", "600"};
        case UiScalePreset::ReleaseVariMuSec:
            return {"0.1", "0.2", "0.4", "0.8", "1.5", "2.5"};
        case UiScalePreset::ThresholdDeepDb:
            return {"-36", "-30", "-24", "-18", "-12", "-6", "0"};
        case UiScalePreset::LimiterCeilingDb:
            outLastOrange = true;
            return {"-12", "-9", "-6", "-3", "-1.5", "-0.5", "0.0"};
        case UiScalePreset::PeakReduction0100:
            return {"0", "20", "40", "60", "80", "100"};
        case UiScalePreset::RatioFetLim:
            outLastOrange = true;
            return {"4:1", "8:1", "12:1", "20:1", "ALL"};
        case UiScalePreset::RatioGentle:
            return {"1.1:1", "1.3:1", "1.5:1", "2:1", "3:1"};
        case UiScalePreset::HighAirFreqKhz:
            return {"2.5", "5", "8", "10", "12", "16", "20"};
        case UiScalePreset::FullSweepFreqHz:
            return {"20", "60", "200", "600", "2k", "6k", "18k"};
        case UiScalePreset::ModRateHz:
            return {"0.1", "0.3", "0.8", "1.5", "3.0", "6.0"};
        case UiScalePreset::DelayTimeMs:
            return {"25", "100", "250", "500", "900", "1500"};
        case UiScalePreset::ReverbDecaySec:
            return {"0.4", "1.2", "2.5", "4.5", "7.0", "10"};
        case UiScalePreset::StereoWidthPct:
            outFirstOrange = true;
            return {"MONO", "50", "100", "125", "150", "200"};
        case UiScalePreset::CoreMatrixNiFeSt:
            return {"NICKEL", "Ni/Fe", "IRON", "Fe/St", "STEEL"};
        case UiScalePreset::DialOneToTen:
            return {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
        case UiScalePreset::DialZeroToTen:
            return {"0", "2", "4", "6", "8", "10"};
        case UiScalePreset::StereoPanLCR:
            return {"L100", "L50", "C", "R50", "R100"};
        case UiScalePreset::LinearDb:
        default:
            return {"-16", "-12", "-8", "-4", "0", "+4", "+8", "+12", "+16"};
    }
}

inline const char* getWidgetTypeName(UiWidgetType t) noexcept {
    switch (t) {
        case UiWidgetType::PointerKnob:           return "POINTER KNOB";
        case UiWidgetType::SkirtedCapKnob:        return "SKIRTED CAP KNOB";
        case UiWidgetType::ConcentricEqKnob:      return "CONCENTRIC KNOB";
        case UiWidgetType::MarconiRedKnob:        return "WINGED RED KNOB";
        case UiWidgetType::ModernArcKnob:         return "LED ARC KNOB";
        case UiWidgetType::BakeliteMasterKnob:    return "BAKELITE ROTARY";
        case UiWidgetType::ColorCapConsoleKnob:   return "COLOUR CAP CONSOLE";
        case UiWidgetType::ChickenHeadKnob:       return "CHICKEN HEAD KNOB";
        case UiWidgetType::FlatVectorKnob:        return "FLAT VECTOR KNOB";
        case UiWidgetType::GoldDomeKnob:          return "24K GOLD DOME";
        case UiWidgetType::MacroHaloKnob:         return "MACRO HALO KNOB";
        case UiWidgetType::KnurledBarrelKnob:     return "KNURLED BARREL";
        case UiWidgetType::ColletBroadcastKnob:   return "COLLET BROADCAST";
        case UiWidgetType::StarburstSynthKnob:    return "STARBURST SYNTH";
        case UiWidgetType::GrimyRelicKnob:        return "GRIMY RELIC KNOB";
        case UiWidgetType::SteppedAttenuatorKnob: return "24-STEP ATTEN";
        case UiWidgetType::CyberLuxeHaloKnob:     return "CYBER HALO KNOB";
        case UiWidgetType::NeumorphicGlassKnob:   return "FROSTED GLASS KNOB";
        case UiWidgetType::SciFiHudRingKnob:      return "SCI-FI HUD DIAL";
        case UiWidgetType::ValveWheelRelicKnob:   return "VALVE WHEEL KNOB";
        case UiWidgetType::SubmarineHelmKnob:     return "SUBMARINE HELM";
        case UiWidgetType::TattooFlashKnob:       return "TATTOO FLASH KNOB";
        case UiWidgetType::MetalSkullKnob:        return "METAL SKULL KNOB";
        case UiWidgetType::FlowerDaisyKnob:       return "FLOWER DAISY KNOB";
        case UiWidgetType::ReaperCryptKnob:       return "REAPER CRYPT KNOB";
        case UiWidgetType::HellfireMagmaKnob:     return "HELLFIRE MAGMA KNOB";
        case UiWidgetType::ApocalypseCogKnob:     return "APOCALYPSE COG";
        case UiWidgetType::PunkSpikedKnob:        return "PUNK SPIKED KNOB";
        case UiWidgetType::MeadowWoodSliceKnob:   return "WOOD SLICE KNOB";
        case UiWidgetType::SkySunburstKnob:       return "SUNBURST CLOUD KNOB";
        case UiWidgetType::StormRaindropKnob:     return "STORM LIGHTNING KNOB";
        case UiWidgetType::GlacialIceCrystalKnob: return "ICE CRYSTAL KNOB";
        case UiWidgetType::ChromeToggleSwitch:    return "CHROME TOGGLE";
        case UiWidgetType::IlluminatedPushButton: return "LIT PUSH BUTTON";
        case UiWidgetType::RockerPowerSwitch:     return "LIT ROCKER SW";
        case UiWidgetType::LockingCageToggle:     return "LOCKING CAGE SW";
        case UiWidgetType::FlatPillVectorSwitch:  return "FLAT PILL VECTOR";
        case UiWidgetType::NeumorphicPillButton:  return "FROSTED PILL SW";
        case UiWidgetType::SciFiHologramSwitch:   return "SCI-FI HUD SW";
        case UiWidgetType::NavalBrassToggle:      return "NAVAL BRASS SW";
        case UiWidgetType::PunkSafetyPinToggle:   return "PUNK SAFETY PIN SW";
        case UiWidgetType::HellfireEmberSwitch:   return "HELLFIRE EMBER SW";
        case UiWidgetType::VerticalStudioFader:   return "STUDIO FADER";
        case UiWidgetType::HorizontalCrossfader:  return "CROSSFADER RAIL";
        case UiWidgetType::JewelLed:              return "JEWEL LAMP";
        case UiWidgetType::AnalogVuMeter:         return "ANALOG VU METER";
        case UiWidgetType::RoundVintageVuMeter:   return "ROUND PORTHOLE VU";
        case UiWidgetType::LedBargraphMeter:      return "LED PEAK METER";
        case UiWidgetType::DigitalTrimBargraph:   return "TRIM BARGRAPH";
        case UiWidgetType::GlowingTubeWindow:     return "TUBE WINDOW";
        case UiWidgetType::CrtOscilloscopeScreen: return "CRT SCOPE SCREEN";
        case UiWidgetType::StereoGoniometerScope: return "LISSAJOUS SCOPE";
        case UiWidgetType::LcdParameterReadout:   return "LCD READOUT";
        case UiWidgetType::NixieTubeDisplay:      return "NIXIE READOUT";
        case UiWidgetType::PastelShaderCurveMeter:return "PASTEL SHADER";
        case UiWidgetType::CyberLuxeMultiBarMeter:return "CYBER MULTI-BAR";
        case UiWidgetType::NeumorphicDualArcMeter:return "FROSTED GLASS VU";
        case UiWidgetType::SubmarinePortholeGauge:return "SUB PORTHOLE VU";
        case UiWidgetType::ReaperScytheArchMeter: return "REAPER SCYTHE VU";
        case UiWidgetType::HellfireHornedVuMeter: return "HELLFIRE HORNED VU";
        case UiWidgetType::ApocalypseGeigerVuMeter:return "GEIGER HAZARD VU";
        case UiWidgetType::PunkAnarchyVuMeter:    return "PUNK ANARCHY VU";
        case UiWidgetType::NatureElementalVuMeter:return "ELEMENTAL NATURE VU";
        case UiWidgetType::SectionFrame:          return "SECTION FRAME";
        case UiWidgetType::RivetedNameplate:      return "RIVETED BADGE";
        case UiWidgetType::SilkscreenLabel:       return "SILKSCREEN TEXT";
        case UiWidgetType::VentilationGrille:     return "COOLING LOUVERS";
        case UiWidgetType::RackHandlePair:        return "RACK GRAB HANDLE";
        case UiWidgetType::PatchbayJackSocket:    return "1/4\" PATCH JACK";
        case UiWidgetType::VintageSticker:        return "VINTAGE STICKER";
        default:                                  return "WIDGET";
    }
}

inline bool isKnobWidget(UiWidgetType t) noexcept {
    return t == UiWidgetType::PointerKnob ||
           t == UiWidgetType::SkirtedCapKnob ||
           t == UiWidgetType::ConcentricEqKnob ||
           t == UiWidgetType::MarconiRedKnob ||
           t == UiWidgetType::ModernArcKnob ||
           t == UiWidgetType::BakeliteMasterKnob ||
           t == UiWidgetType::ColorCapConsoleKnob ||
           t == UiWidgetType::ChickenHeadKnob ||
           t == UiWidgetType::FlatVectorKnob ||
           t == UiWidgetType::GoldDomeKnob ||
           t == UiWidgetType::MacroHaloKnob ||
           t == UiWidgetType::KnurledBarrelKnob ||
           t == UiWidgetType::ColletBroadcastKnob ||
           t == UiWidgetType::StarburstSynthKnob ||
           t == UiWidgetType::GrimyRelicKnob ||
           t == UiWidgetType::SteppedAttenuatorKnob ||
           t == UiWidgetType::CyberLuxeHaloKnob ||
           t == UiWidgetType::NeumorphicGlassKnob ||
           t == UiWidgetType::SciFiHudRingKnob ||
           t == UiWidgetType::ValveWheelRelicKnob ||
           t == UiWidgetType::SubmarineHelmKnob ||
           t == UiWidgetType::TattooFlashKnob ||
           t == UiWidgetType::MetalSkullKnob ||
           t == UiWidgetType::FlowerDaisyKnob ||
           t == UiWidgetType::ReaperCryptKnob ||
           t == UiWidgetType::HellfireMagmaKnob ||
           t == UiWidgetType::ApocalypseCogKnob ||
           t == UiWidgetType::PunkSpikedKnob ||
           t == UiWidgetType::MeadowWoodSliceKnob ||
           t == UiWidgetType::SkySunburstKnob ||
           t == UiWidgetType::StormRaindropKnob ||
           t == UiWidgetType::GlacialIceCrystalKnob;
}

inline bool isFaderWidget(UiWidgetType t) noexcept {
    return t == UiWidgetType::VerticalStudioFader ||
           t == UiWidgetType::HorizontalCrossfader;
}

inline bool isKnobOrFaderWidget(UiWidgetType t) noexcept {
    return isKnobWidget(t) || isFaderWidget(t);
}

inline bool isSwitchWidget(UiWidgetType t) noexcept {
    return t == UiWidgetType::ChromeToggleSwitch ||
           t == UiWidgetType::IlluminatedPushButton ||
           t == UiWidgetType::RockerPowerSwitch ||
           t == UiWidgetType::LockingCageToggle ||
           t == UiWidgetType::FlatPillVectorSwitch ||
           t == UiWidgetType::NeumorphicPillButton ||
           t == UiWidgetType::SciFiHologramSwitch ||
           t == UiWidgetType::NavalBrassToggle ||
           t == UiWidgetType::PunkSafetyPinToggle ||
           t == UiWidgetType::HellfireEmberSwitch;
}

inline bool isMeterWidget(UiWidgetType t) noexcept {
    return t == UiWidgetType::AnalogVuMeter ||
           t == UiWidgetType::RoundVintageVuMeter ||
           t == UiWidgetType::LedBargraphMeter ||
           t == UiWidgetType::DigitalTrimBargraph ||
           t == UiWidgetType::GlowingTubeWindow ||
           t == UiWidgetType::CrtOscilloscopeScreen ||
           t == UiWidgetType::StereoGoniometerScope ||
           t == UiWidgetType::LcdParameterReadout ||
           t == UiWidgetType::NixieTubeDisplay ||
           t == UiWidgetType::PastelShaderCurveMeter ||
           t == UiWidgetType::CyberLuxeMultiBarMeter ||
           t == UiWidgetType::NeumorphicDualArcMeter ||
           t == UiWidgetType::SubmarinePortholeGauge ||
           t == UiWidgetType::ReaperScytheArchMeter ||
           t == UiWidgetType::HellfireHornedVuMeter ||
           t == UiWidgetType::ApocalypseGeigerVuMeter ||
           t == UiWidgetType::PunkAnarchyVuMeter ||
           t == UiWidgetType::NatureElementalVuMeter;
}

inline UiWidgetType nextKnobWidgetType(UiWidgetType cur) noexcept {
    constexpr std::array<UiWidgetType, 32> kKnobs = {{
        UiWidgetType::PointerKnob,
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
        UiWidgetType::GlacialIceCrystalKnob
    }};
    for (size_t i = 0; i < kKnobs.size(); ++i) {
        if (kKnobs[i] == cur) {
            return kKnobs[(i + 1) % kKnobs.size()];
        }
    }
    return UiWidgetType::PointerKnob;
}

inline UiWidgetType nextSwitchWidgetType(UiWidgetType cur) noexcept {
    constexpr std::array<UiWidgetType, 10> kSwitches = {{
        UiWidgetType::ChromeToggleSwitch,
        UiWidgetType::IlluminatedPushButton,
        UiWidgetType::RockerPowerSwitch,
        UiWidgetType::LockingCageToggle,
        UiWidgetType::FlatPillVectorSwitch,
        UiWidgetType::NeumorphicPillButton,
        UiWidgetType::SciFiHologramSwitch,
        UiWidgetType::NavalBrassToggle,
        UiWidgetType::PunkSafetyPinToggle,
        UiWidgetType::HellfireEmberSwitch
    }};
    for (size_t i = 0; i < kSwitches.size(); ++i) {
        if (kSwitches[i] == cur) {
            return kSwitches[(i + 1) % kSwitches.size()];
        }
    }
    return UiWidgetType::ChromeToggleSwitch;
}

inline UiWidgetType nextMeterWidgetType(UiWidgetType cur) noexcept {
    constexpr std::array<UiWidgetType, 18> kMeters = {{
        UiWidgetType::AnalogVuMeter,
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
        UiWidgetType::NatureElementalVuMeter
    }};
    for (size_t i = 0; i < kMeters.size(); ++i) {
        if (kMeters[i] == cur) {
            return kMeters[(i + 1) % kMeters.size()];
        }
    }
    return UiWidgetType::AnalogVuMeter;
}

inline const char* getSectionFrameStyleName(gui::SectionFrameStyle s) noexcept {
    switch (s) {
        case gui::SectionFrameStyle::RivetedSubPlate: return "RIVETED PLATE";
        case gui::SectionFrameStyle::DoubleHairline:  return "DOUBLE HAIRLINE";
        case gui::SectionFrameStyle::HeavyConsoleBay: return "CONSOLE BAY";
        case gui::SectionFrameStyle::TechnicalCorner: return "CORNER BRACKETS";
        case gui::SectionFrameStyle::MinimalHeader:   return "MINIMAL HEADER";
        case gui::SectionFrameStyle::PipingTolex:     return "PIPING TOLEX";
        case gui::SectionFrameStyle::GothicArch:      return "GOTHIC ARCH";
        case gui::SectionFrameStyle::EngravedBevel:
        default:                                      return "ENGRAVED BEVEL";
    }
}

inline const char* getSectionFrameStyleEnumCode(gui::SectionFrameStyle s) noexcept {
    switch (s) {
        case gui::SectionFrameStyle::RivetedSubPlate: return "avra::gui::SectionFrameStyle::RivetedSubPlate";
        case gui::SectionFrameStyle::DoubleHairline:  return "avra::gui::SectionFrameStyle::DoubleHairline";
        case gui::SectionFrameStyle::HeavyConsoleBay: return "avra::gui::SectionFrameStyle::HeavyConsoleBay";
        case gui::SectionFrameStyle::TechnicalCorner: return "avra::gui::SectionFrameStyle::TechnicalCorner";
        case gui::SectionFrameStyle::MinimalHeader:   return "avra::gui::SectionFrameStyle::MinimalHeader";
        case gui::SectionFrameStyle::PipingTolex:     return "avra::gui::SectionFrameStyle::PipingTolex";
        case gui::SectionFrameStyle::GothicArch:      return "avra::gui::SectionFrameStyle::GothicArch";
        case gui::SectionFrameStyle::EngravedBevel:
        default:                                      return "avra::gui::SectionFrameStyle::EngravedBevel";
    }
}

inline UiWidgetType nextAnyWidgetType(UiWidgetType cur) noexcept {
    if (isKnobWidget(cur))   return nextKnobWidgetType(cur);
    if (isMeterWidget(cur))  return nextMeterWidgetType(cur);
    if (isSwitchWidget(cur)) return nextSwitchWidgetType(cur);
    if (cur == UiWidgetType::VerticalStudioFader)  return UiWidgetType::HorizontalCrossfader;
    if (cur == UiWidgetType::HorizontalCrossfader) return UiWidgetType::VerticalStudioFader;
    return cur;
}

inline UiScalePreset inferScalePresetForParam(
    const ComponentParamSpec& p,
    SchematicComponentType modType = SchematicComponentType::JfetBuffer) noexcept {
    const bool is1073Module = (modType == SchematicComponentType::InductorEq3Band ||
                               modType == SchematicComponentType::ClassAPreamp);
    if (is1073Module && (p.name.find("Low Freq") != std::string::npos || (p.unit == "Hz" && p.minVal >= 30.0f && p.maxVal <= 240.0f))) {
        return UiScalePreset::FreqLowShelf1073;
    }
    if (p.name.find("HPF") != std::string::npos) return UiScalePreset::HighPassHz;
    if (p.name.find("Rate") != std::string::npos || p.name.find("Speed") != std::string::npos ||
        (p.unit == "Hz" && p.maxVal <= 50.0f)) {
        return UiScalePreset::ModRateHz;
    }
    if (p.name.find("Cutoff") != std::string::npos) return UiScalePreset::FullSweepFreqHz;
    if (p.unit == "Hz") {
        if (is1073Module) {
            if (p.maxVal > 8000.0f) return UiScalePreset::FullSweepFreqHz;
            return (p.maxVal <= 600.0f) ? UiScalePreset::LowFreqHz : UiScalePreset::MidFreqHz;
        }
        if (p.maxVal >= 12000.0f && p.minVal >= 800.0f) return UiScalePreset::HighAirFreqKhz;
        return UiScalePreset::FullSweepFreqHz;
    }
    if (p.unit == "kHz") return is1073Module ? UiScalePreset::MidFreqHz : UiScalePreset::HighAirFreqKhz;
    if (p.name.find("Peak Reduction") != std::string::npos) return UiScalePreset::PeakReduction0100;
    if (p.name.find("Width") != std::string::npos) return UiScalePreset::StereoWidthPct;
    if (p.name.find("Ni") != std::string::npos && p.name.find("Fe") != std::string::npos) return UiScalePreset::CoreMatrixNiFeSt;
    if (p.unit == "%") {
        std::string lowName = p.name;
        for (char& c : lowName) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        if (lowName.find("mix") != std::string::npos || lowName.find("dry") != std::string::npos ||
            lowName.find("wet") != std::string::npos || lowName.find("blend") != std::string::npos) {
            return UiScalePreset::MixDryWet;
        }
        return UiScalePreset::Percent0To100;
    }
    if (p.unit == ":1")  return UiScalePreset::RatioSteps;
    if (p.name.find("Echo") != std::string::npos || p.name.find("Delay") != std::string::npos || p.name.find("Clock") != std::string::npos) {
        return UiScalePreset::DelayTimeMs;
    }
    if (p.name.find("Decay") != std::string::npos || p.unit == "s") return UiScalePreset::ReverbDecaySec;
    if (p.name.find("Attack") != std::string::npos) return UiScalePreset::AttackFastMs;
    if (p.name.find("Release") != std::string::npos) return (p.maxVal <= 650.0f) ? UiScalePreset::ReleaseFastMs : UiScalePreset::RecoveryAuto;
    if (p.unit == "ms") return UiScalePreset::RecoveryAuto;
    if (p.unit == "dBFS" || p.name.find("Ceiling") != std::string::npos) return UiScalePreset::LimiterCeilingDb;
    if (p.name.find("Thresh") != std::string::npos) {
        return (p.minVal <= -30.0f) ? UiScalePreset::ThresholdDeepDb : UiScalePreset::ThresholdDb;
    }
    if (p.minVal < 0.0f && p.maxVal <= 0.0f) return (p.minVal <= -30.0f) ? UiScalePreset::ThresholdDeepDb : UiScalePreset::ThresholdDb;
    if (p.name.find("Make-Up") != std::string::npos || p.name.find("Makeup") != std::string::npos) {
        return UiScalePreset::MakeupGain0To20Db;
    }
    if (p.name.find("Preamp Gain") != std::string::npos) return UiScalePreset::GainLineMic;
    if (p.name.find("Drive") != std::string::npos && p.minVal >= 0.0f && p.unit == "dB") return UiScalePreset::DriveZeroTo36Db;
    return UiScalePreset::LinearDb;
}

[[nodiscard]] inline float computeParamNorm(const ComponentParamSpec& p, float val) noexcept {
    const float clamped = std::clamp(val, p.minVal, p.maxVal);
    const bool isStepped = !p.stepLabels.empty() ||
        p.controlKind == HardwareControlKind::SteppedRotarySwitch ||
        p.controlKind == HardwareControlKind::ToggleSwitch2Way ||
        p.controlKind == HardwareControlKind::ToggleSwitch3Way ||
        p.controlKind == HardwareControlKind::PushButtonLatching ||
        p.controlKind == HardwareControlKind::PushButtonBank;
    if (!isStepped) {
        if (p.isLogarithmic && p.minVal > 1e-4f && p.maxVal > p.minVal) {
            return std::clamp(math::logFreqToNormalized(clamped, p.minVal, p.maxVal), 0.0f, 1.0f);
        }
        if (p.minVal < -1e-4f && p.maxVal > 1e-4f &&
            (std::abs(p.defaultVal) < 1e-4f || std::abs(p.minVal + p.maxVal) < 0.01f)) {
            if (clamped < 0.0f) {
                return std::clamp(0.5f * (1.0f - (clamped / p.minVal)), 0.0f, 0.5f);
            }
            return std::clamp(0.5f + 0.5f * (clamped / p.maxVal), 0.5f, 1.0f);
        }
    }
    const float span = std::max(0.0001f, p.maxVal - p.minVal);
    return std::clamp((clamped - p.minVal) / span, 0.0f, 1.0f);
}

[[nodiscard]] inline float computeParamValFromNorm(const ComponentParamSpec& p, float norm) noexcept {
    const float t = std::clamp(norm, 0.0f, 1.0f);
    const bool isStepped = !p.stepLabels.empty() ||
        p.controlKind == HardwareControlKind::SteppedRotarySwitch ||
        p.controlKind == HardwareControlKind::ToggleSwitch2Way ||
        p.controlKind == HardwareControlKind::ToggleSwitch3Way ||
        p.controlKind == HardwareControlKind::PushButtonLatching ||
        p.controlKind == HardwareControlKind::PushButtonBank;
    if (!isStepped) {
        if (p.isLogarithmic && p.minVal > 1e-4f && p.maxVal > p.minVal) {
            return std::clamp(math::normalizedToLogFreq(t, p.minVal, p.maxVal), p.minVal, p.maxVal);
        }
        if (p.minVal < -1e-4f && p.maxVal > 1e-4f &&
            (std::abs(p.defaultVal) < 1e-4f || std::abs(p.minVal + p.maxVal) < 0.01f)) {
            if (std::abs(t - 0.5f) < 0.004f) {
                return 0.0f;
            }
            if (t < 0.5f) {
                return std::clamp(p.minVal * (1.0f - 2.0f * t), p.minVal, 0.0f);
            }
            return std::clamp(p.maxVal * (2.0f * t - 1.0f), 0.0f, p.maxVal);
        }
    }
    return std::clamp(p.minVal + t * (p.maxVal - p.minVal), p.minVal, p.maxVal);
}

[[nodiscard]] inline std::vector<std::string> buildContinuousFreqTickLabels(
    const ComponentParamSpec& pSpec,
    SchematicComponentType modType) {
    (void)modType;
    if (!pSpec.stepLabels.empty()) {
        if (pSpec.stepLabels.size() <= 7) {
            return pSpec.stepLabels;
        }
        std::vector<std::string> decimated;
        decimated.reserve(6);
        const size_t n = pSpec.stepLabels.size();
        for (size_t i = 0; i < 6; ++i) {
            const size_t idx = (i * (n - 1) + 2) / 5;
            decimated.push_back(pSpec.stepLabels[idx]);
        }
        return decimated;
    }
    if (pSpec.unit == "%" && pSpec.minVal == 0.0f && pSpec.maxVal == 100.0f) {
        std::string lowName = pSpec.name;
        for (char& c : lowName) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        if (lowName.find("mix") != std::string::npos || lowName.find("dry") != std::string::npos ||
            lowName.find("wet") != std::string::npos || lowName.find("blend") != std::string::npos) {
            return {"DRY", "25", "50", "75", "WET"};
        }
        return {"0", "25", "50", "75", "100"};
    }

    auto fmtParamTick = [&](float v) -> std::string {
        if (pSpec.unit == "Hz" || pSpec.unit == "kHz") {
            if (pSpec.unit == "kHz") {
                return (std::abs(v - std::round(v)) < 0.08f)
                    ? std::format("{:.0f}k", std::round(v))
                    : std::format("{:.1f}k", v);
            }
            if (pSpec.maxVal <= 50.0f) {
                return (std::abs(v - std::round(v)) < 0.08f && pSpec.maxVal >= 10.0f)
                    ? std::format("{:.0f}", std::round(v))
                    : std::format("{:.1f}", v);
            }
            if (v >= 10000.0f) {
                return std::format("{:.0f}k", v * 0.001f);
            }
            if (v >= 1000.0f) {
                const float kv = v * 0.001f;
                if (std::abs(kv - std::round(kv)) < 0.08f) {
                    return std::format("{:.0f}k", std::round(kv));
                }
                return std::format("{:.1f}k", kv);
            }
            return std::format("{:.0f}", std::round(v));
        }
        if (pSpec.unit == ":1") {
            return (std::abs(v - std::round(v)) < 0.08f)
                ? std::format("{:.0f}:1", std::round(v))
                : std::format("{:.1f}:1", v);
        }
        if (pSpec.minVal < -1e-4f && pSpec.maxVal > 1e-4f) {
            if (std::abs(v) < 0.05f) {
                return "0";
            }
            if (std::abs(v - std::round(v)) < 0.08f) {
                return std::format("{:+.0f}", std::round(v));
            }
            return std::format("{:+.1f}", v);
        }
        if (std::abs(v) < 1e-4f) {
            return "0";
        }
        const float span = pSpec.maxVal - pSpec.minVal;
        if (std::abs(v - std::round(v)) < 0.08f && span >= 3.5f) {
            return std::format("{:.0f}", std::round(v));
        }
        if (span < 1.5f) {
            return (std::abs(v * 10.0f - std::round(v * 10.0f)) < 0.08f)
                ? std::format("{:.1f}", v)
                : std::format("{:.2f}", v);
        }
        return std::format("{:.1f}", v);
    };

    std::vector<std::string> ticks;
    ticks.reserve(5);
    for (int i = 0; i < 5; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        const float v = computeParamValFromNorm(pSpec, t);
        ticks.push_back(fmtParamTick(v));
    }
    return ticks;
}

struct DefaultUIBlueprint {
    gui::ChassisFinish heritageFinish{gui::ChassisFinish::AnodizedTeal};
    UiWidgetType       primaryKnobStyle{UiWidgetType::PointerKnob};
    UiWidgetType       secondaryKnobStyle{UiWidgetType::SkirtedCapKnob};
    UiAccentColor      heritageAccent{UiAccentColor::WarmAmber};
    UiWidgetType       telemetryProbe{UiWidgetType::AnalogVuMeter};
};

inline DefaultUIBlueprint getDefaultUIBlueprint(SchematicComponentType type) noexcept {
    switch (type) {
        // 1176 FET Core: Blackface aluminum, Skirted knobs, Warm Amber VU
        case SchematicComponentType::FetCompressor1176:
            return { gui::ChassisFinish::ObsidianBlack, UiWidgetType::SkirtedCapKnob, UiWidgetType::PointerKnob, UiAccentColor::WarmAmber, UiWidgetType::AnalogVuMeter };
        // LA-2A Optical Core: Vintage Cream faceplate, Chicken-Head dials, Royal Gold VU
        case SchematicComponentType::OptoCompressorT4B:
            return { gui::ChassisFinish::VintageCream, UiWidgetType::ChickenHeadKnob, UiWidgetType::BakeliteMasterKnob, UiAccentColor::RoyalGold, UiWidgetType::AnalogVuMeter };
        // Passive Offset-RC & Parallel LC: Blue/Green Hammertone, Bakelite & Chicken-Head knobs, Tube Window
        case SchematicComponentType::PassivePultecEq:
            return { gui::ChassisFinish::BritishSlateBlue, UiWidgetType::BakeliteMasterKnob, UiWidgetType::ChickenHeadKnob, UiAccentColor::IceSkyBlue, UiWidgetType::GlowingTubeWindow };
        case SchematicComponentType::ParallelPassiveLcEq:
            return { gui::ChassisFinish::HammertoneGreen, UiWidgetType::BakeliteMasterKnob, UiWidgetType::GoldDomeKnob, UiAccentColor::EmeraldGreen, UiWidgetType::GlowingTubeWindow };
        // Class-A Preamp & 3-Band Inductor EQ: Slate Blue, Winged Red Marconi & Concentric Dual-Ring EQ knobs
        case SchematicComponentType::ClassAPreamp:
            return { gui::ChassisFinish::BritishSlateBlue, UiWidgetType::MarconiRedKnob, UiWidgetType::PointerKnob, UiAccentColor::CrimsonRed, UiWidgetType::RoundVintageVuMeter };
        case SchematicComponentType::InductorEq3Band:
            return { gui::ChassisFinish::BritishSlateBlue, UiWidgetType::ConcentricEqKnob, UiWidgetType::MarconiRedKnob, UiAccentColor::CobaltBlue, UiWidgetType::RoundVintageVuMeter };
        // Discrete Op-Amp & Proportional-Q EQ: Cobalt Studio Blue, Color-Cap Console Knobs
        case SchematicComponentType::ClassABOpAmp2520:
        case SchematicComponentType::ProportionalQGyratorEq:
            return { gui::ChassisFinish::CobaltStudioBlue, UiWidgetType::ColorCapConsoleKnob, UiWidgetType::ColorCapConsoleKnob, UiAccentColor::CobaltBlue, UiWidgetType::LedBargraphMeter };
        // Quad-VCA Master Bus Compressor: Obsidian Console, Color-Cap Console Knobs
        case SchematicComponentType::VcaBusCompressor:
            return { gui::ChassisFinish::ObsidianBlack, UiWidgetType::ColorCapConsoleKnob, UiWidgetType::PointerKnob, UiAccentColor::EmeraldGreen, UiWidgetType::AnalogVuMeter };
        // Vari-Mu / Dual-Triode / Direct-Heated Triode / Cascode Tube Stages: Warm Copper/Amber, Gold Dome & Chicken-Head, Glowing Tube Window
        case SchematicComponentType::VariMuTubeCompressor:
        case SchematicComponentType::Tube12AX7:
        case SchematicComponentType::DirectHeatedTriode:
        case SchematicComponentType::HighVoltageCascodeTube:
            return { gui::ChassisFinish::WarmCopperBronze, UiWidgetType::GoldDomeKnob, UiWidgetType::ChickenHeadKnob, UiAccentColor::WarmAmber, UiWidgetType::GlowingTubeWindow };
        // Spectral / Transient-Tonal Splitter / Polyphase Clipper: Cyberpunk Violet / Royal Navy, Flat Vector & Macro Halo, CRT Scope
        case SchematicComponentType::SpectralResonanceSuppressor:
        case SchematicComponentType::BarkAuditoryUnmasker:
            return { gui::ChassisFinish::CyberpunkViolet, UiWidgetType::MacroHaloKnob, UiWidgetType::FlatVectorKnob, UiAccentColor::NeonCyan, UiWidgetType::CrtOscilloscopeScreen };
        case SchematicComponentType::TransientTonalSplitter:
        case SchematicComponentType::OversampledVariableClipper:
        case SchematicComponentType::MonoCompatibleWidener:
            return { gui::ChassisFinish::RoyalNavyBlue, UiWidgetType::FlatVectorKnob, UiWidgetType::ModernArcKnob, UiAccentColor::SunsetOrange, UiWidgetType::CrtOscilloscopeScreen };
        // Tape Echo / BBD / Reverbs
        case SchematicComponentType::TapeEcho:
        case SchematicComponentType::PingPongDelay:
        case SchematicComponentType::BbdCompanderDelay:
            return { gui::ChassisFinish::HammertoneGreen, UiWidgetType::ChickenHeadKnob, UiWidgetType::SkirtedCapKnob, UiAccentColor::EmeraldGreen, UiWidgetType::RoundVintageVuMeter };
        case SchematicComponentType::PlateReverb:
        case SchematicComponentType::HallReverbFDN:
        case SchematicComponentType::OctaveShimmerReverb:
            return { gui::ChassisFinish::VintageGold, UiWidgetType::GoldDomeKnob, UiWidgetType::MacroHaloKnob, UiAccentColor::RoyalGold, UiWidgetType::LcdParameterReadout };
        default: {
            const auto& meta = getComponentMeta(type);
            if (meta.category == SchematicCategory::PreampsAndTubes) {
                return { gui::ChassisFinish::WarmCopperBronze, UiWidgetType::MarconiRedKnob, UiWidgetType::ChickenHeadKnob, UiAccentColor::WarmAmber, UiWidgetType::GlowingTubeWindow };
            }
            if (meta.category == SchematicCategory::IronAndTape) {
                return { gui::ChassisFinish::SunburstAmberWood, UiWidgetType::GoldDomeKnob, UiWidgetType::PointerKnob, UiAccentColor::RoyalGold, UiWidgetType::RoundVintageVuMeter };
            }
            if (meta.category == SchematicCategory::EqAndFilters) {
                return { gui::ChassisFinish::CobaltStudioBlue, UiWidgetType::ColorCapConsoleKnob, UiWidgetType::ConcentricEqKnob, UiAccentColor::CobaltBlue, UiWidgetType::CrtOscilloscopeScreen };
            }
            if (meta.category == SchematicCategory::AnalogDynamics) {
                return { gui::ChassisFinish::CrimsonMastering, UiWidgetType::SkirtedCapKnob, UiWidgetType::PointerKnob, UiAccentColor::CrimsonRed, UiWidgetType::AnalogVuMeter };
            }
            return { gui::ChassisFinish::AnodizedTeal, UiWidgetType::PointerKnob, UiWidgetType::SkirtedCapKnob, UiAccentColor::WarmAmber, UiWidgetType::AnalogVuMeter };
        }
    }
}

enum class KnobCapStyle : int {
    BakeliteFluted  = 0,
    MarconiWinged   = 1,
    BrushedAluminum = 2,
    ColletCap       = 3
};

struct FaceplateTheme {
    gui::ChassisFinish        finish{gui::ChassisFinish::AnodizedTeal};
    gui::SurfaceGrain         grain{gui::SurfaceGrain::VerticalBrushed};
    int                       surfaceFinish{0}; // 0 = VERTICAL BRUSHED
    gui::Color                chassisBg{gui::Color::fromHex(0x245A5C)};
    gui::Color                panelBg{gui::Color::fromHex(0x173D3F)};
    gui::Color                accentColor{gui::Color::fromHex(0xFF7A1A)};
    gui::Color                textColor{gui::Color::fromHex(0xEDEAE0)};
    int                       knobCapStyle{static_cast<int>(KnobCapStyle::BakeliteFluted)};
    bool                      cornerScrews{true};
    bool                      showTopToolbar{true};
    gui::ChassisHardwareStyle hardwareStyle{gui::ChassisHardwareStyle::CornerScrews4};
    gui::ScrewHeadStyle       screwStyle{gui::ScrewHeadStyle::PhillipsCross};
    bool                      useCustomFinishColor{false};
    uint32_t                  customFinishHex{0x1A4347};
    int                       gridSnapPx{8}; // 8px, 16px, 4px, or 1 (Free)
    bool                      showGrid{false};
    UiWidgetType              globalKnobType{UiWidgetType::PointerKnob};
    UiWidgetType              globalSwitchType{UiWidgetType::ChromeToggleSwitch};
    UiWidgetType              globalMeterType{UiWidgetType::AnalogVuMeter};
    UiAccentColor             globalAccentColor{UiAccentColor::WarmAmber};
    int                       globalFontSizePreset{2};
    gui::FontFamily           globalFontFamily{gui::FontFamily::StudioSans};
    UiSkinFontColor           globalFontColor{UiSkinFontColor::SilkscreenWhite};
    bool                      useCustomFontColor{false};
    uint32_t                  customFontColorHex{0xE6ECEF};
    bool                      unifyFaceplateTheme{true}; // true = Cohesive KENNEDARTH TEAL CONSOLE brand aesthetic by default
    // 6-Layer Unified Style Template Extended Properties
    float                     bevelDepthPx{2.5f};
    gui::SectionFrameStyle    panelBorderStyle{gui::SectionFrameStyle::EngravedBevel};
    bool                      useCustomKnobColors{false};
    uint32_t                  knobBodyHex{0xC8CED4};
    uint32_t                  knobCapHex{0x1E2226};
    uint32_t                  knobIndicatorHex{0xF0F4F8};
    gui::KnobTickMarkStyle    knobTickStyle{gui::KnobTickMarkStyle::RadialLines};
    uint32_t                  meterFaceplateHex{0x181614};
    uint32_t                  meterGlowHex{0xD87A18};
    gui::UiLabelPlaqueStyle   labelPlaqueStyle{gui::UiLabelPlaqueStyle::DirectSilkscreen};
    std::string               brandName{"AVRA AUDIO"};
    std::string               pluginTitle{"AVRA GAIN"};
    std::string               subtitleText{"CUSTOM CHASSIS"};
    std::string               badgeFooterText{"CLASS-A EFFECTS"};
    bool                      showSnapGuides{false};
    int                       styleTemplateIdx{0};
    bool                      earsOnlyBlackoutMode{false};
    bool                      isCustomThemeModified{false};
    bool                      hasSurfaceOverlayOverride{false};
    std::string               customThemeName{};
    gui::BadgeVisualStyle     globalBadgeStyle{gui::BadgeVisualStyle::BrushedAluminumPlate};
    int                       textPlateColorPreset{0}; // 0 = Style Theme Match, 1..11 = Studio Plate Presets, 12 = Clear/None
    bool                      useCustomTextPlateColor{false};
    uint32_t                  customTextPlateColorHex{0x0E2A2C};
};

enum class UiStyleTemplate : int {
    // Classic 12 Templates (0..11)
    SleekModernDark          = 0,
    GrimyRoadWornRelic       = 1,
    BritishConsoleSlate      = 2,
    Blackface1176Studio      = 3,
    GermanBroadcastCream     = 4,
    VintageTubeBakelite      = 5,
    CrimsonMasteringLab      = 6,
    CyberpunkSynthMatrix     = 7,
    ArcticMinimalistVector   = 8,
    BoutiqueSunburstWood     = 9,
    OxidizedCopperIndustrial = 10,
    HeritageMultiPanel       = 11,

    // 24 New Studio, Amp, Synth, Modern & Dark-Art Themes (12..35)
    // Studio Hardware (6)
    PultecHammertoneBlue     = 12,
    Api500CobaltConsole      = 13,
    TeletronixLa2aOptoGrey   = 14,
    Fairchild670VariMuBlack  = 15,
    Ssl4000GBusConsole       = 16,
    ManleyMassiveNavy        = 17,
    // Studio Mastering & Transfer (5)
    SwissMasteringTransferFinish      = 18,
    Plexi1959GoldPanel       = 19,
    DualRectifierTreadplate  = 20,
    BritishAc30CopperTop     = 21,
    CitrusCrunchAmpTolex     = 22,
    // Vintage & Modular Synths (5)
    Juno106SynthwaveChorus   = 23,
    WestCoastBuchlaGoldLab   = 24,
    PutneyVcs3PinMatrix      = 25,
    Ms20SemiModularPatch     = 26,
    Op1FieldCncAluminum      = 27,
    // Modern & Pastel Shader DSP (4)
    SoothePastelGlassShader  = 28,
    ProQSpectrumObsidian     = 29,
    ValhallaNordicVector     = 30,
    TokyoDawnMasteringLab    = 31,
    // Relic & Bunker Hardware (2)
    SovietColdWarBunkerRadar = 32,
    ChernobylGeigerDosimeter = 33,

    // 17 Reference Style Templates (34..50)
    FrostedGlassNeumorphism  = 34,
    MinimalistPureVector     = 35,
    GreyscaleMonochrome      = 36,
    FuturisticSciFiHud       = 37,
    RetroDistressedHazard    = 38,
    UnderwaterSubmarine      = 39,
    TattooArtTraditional     = 40,
    HeavyMetalForged         = 41,
    ClassicRockAmpHead       = 42,
    FlowerPowerPsychedelic   = 43,
    GrimReaperNecropolis     = 44,
    HellFireMagma            = 45,
    ApocalypseWasteland      = 46,
    GrassMeadowBotanical     = 47,
    SkyCloudsGoldenHour      = 48,
    RainStormLightning       = 49,
    SnowIcyGlacial           = 50
};

static constexpr int kTotalUiStyleTemplates = 51;

inline uint32_t getDefaultStylePlateColorHex(UiStyleTemplate t) noexcept {
    switch (t) {
        case UiStyleTemplate::FrostedGlassNeumorphism:   return 0xF8FAFC; // Frosted White Plate (with black text)
        case UiStyleTemplate::MinimalistPureVector:      return 0xFFFFFF; // Pure White Plate (with black text)
        case UiStyleTemplate::GermanBroadcastCream:      return 0xF5EFE0; // Warm Broadcast Cream Plate
        case UiStyleTemplate::ArcticMinimalistVector:    return 0xF4F8FB; // Arctic Ice White Plate
        case UiStyleTemplate::BoutiqueSunburstWood:      return 0xF4F6F8; // 80s Remote White Plate
        case UiStyleTemplate::TeletronixLa2aOptoGrey:    return 0xDCE2E8; // Brushed Silver-Grey Plate
        case UiStyleTemplate::BritishAc30CopperTop:      return 0xEFE4C8; // Mastering Champagne Gold Plate
        case UiStyleTemplate::Op1FieldCncAluminum:       return 0xEEF2F6; // Field CNC Aluminum White Plate
        case UiStyleTemplate::ValhallaNordicVector:      return 0xFAF4EE; // Nordic Warm Ivory Plate
        case UiStyleTemplate::TattooArtTraditional:      return 0xF7EBD2; // Aged Flash Parchment Plate
        case UiStyleTemplate::FlowerPowerPsychedelic:    return 0xFFFBEB; // 60s Daisy Cream-White Plate
        case UiStyleTemplate::SkyCloudsGoldenHour:       return 0xF8FAFC; // Cumulus Cloud White Plate
        case UiStyleTemplate::SnowIcyGlacial:            return 0xE8F6FE; // Glacial Ice White Plate
        case UiStyleTemplate::SleekModernDark:           return 0x0E2A2C; // Kennedarth Petrol Teal Plate
        case UiStyleTemplate::GrimyRoadWornRelic:        return 0x1A261F; // Relic Olive Plate
        case UiStyleTemplate::BritishConsoleSlate:       return 0x1C2630; // British Console Slate Plate
        case UiStyleTemplate::Blackface1176Studio:       return 0x101316; // Blackface Charcoal Plate
        case UiStyleTemplate::VintageTubeBakelite:       return 0x281810; // Bakelite Brown Plate
        case UiStyleTemplate::CrimsonMasteringLab:       return 0x2A0B10; // Mastering Crimson Plate
        case UiStyleTemplate::CyberpunkSynthMatrix:      return 0x140B24; // Cyber Violet Plate
        case UiStyleTemplate::OxidizedCopperIndustrial:  return 0x142823; // Verdigris Patina Plate
        case UiStyleTemplate::HeritageMultiPanel:        return 0x12161A; // Heritage Charcoal Plate
        case UiStyleTemplate::PultecHammertoneBlue:      return 0x182634; // Hammertone Blue Plate
        case UiStyleTemplate::Api500CobaltConsole:       return 0x101824; // American Cobalt Plate
        case UiStyleTemplate::Fairchild670VariMuBlack:   return 0x121417; // Broadcast Vari-Mu Plate
        case UiStyleTemplate::Ssl4000GBusConsole:        return 0x1E2328; // British 4K Slate Plate
        case UiStyleTemplate::ManleyMassiveNavy:         return 0x0E182B; // Boutique Royal Navy Plate
        case UiStyleTemplate::SwissMasteringTransferFinish: return 0x16191D; // Nextel Suede Plate
        case UiStyleTemplate::Plexi1959GoldPanel:        return 0x1D241B; // Vienna Broadcast Olive Plate
        case UiStyleTemplate::DualRectifierTreadplate:   return 0x12161B; // Tokyo PCM Charcoal Plate
        case UiStyleTemplate::CitrusCrunchAmpTolex:      return 0x101720; // Nordic Spectral Slate Plate
        case UiStyleTemplate::Juno106SynthwaveChorus:    return 0x141720; // Analog Synthwave Plate
        case UiStyleTemplate::WestCoastBuchlaGoldLab:    return 0x0F2038; // West-Coast Cobalt Plate
        case UiStyleTemplate::PutneyVcs3PinMatrix:       return 0x12241C; // Pin-Matrix Green Plate
        case UiStyleTemplate::Ms20SemiModularPatch:      return 0x101317; // Patchbay Ebony Plate
        case UiStyleTemplate::SoothePastelGlassShader:   return 0x101324; // Spectral Indigo Plate
        case UiStyleTemplate::ProQSpectrumObsidian:      return 0x0C1017; // Spectrum Obsidian Plate
        case UiStyleTemplate::TokyoDawnMasteringLab:     return 0x12171E; // Precision Gunmetal Plate
        case UiStyleTemplate::SovietColdWarBunkerRadar:  return 0x1A2218; // Bunker Olive Plate
        case UiStyleTemplate::ChernobylGeigerDosimeter:  return 0x241F10; // Dosimeter Bronze Plate
        case UiStyleTemplate::GreyscaleMonochrome:       return 0x181B1F; // Monochrome Charcoal Plate
        case UiStyleTemplate::FuturisticSciFiHud:        return 0x05161E; // Sci-Fi Starship Cyan-Slate Plate
        case UiStyleTemplate::RetroDistressedHazard:     return 0x2A1E0D; // Distressed Hazard Umber Plate
        case UiStyleTemplate::UnderwaterSubmarine:       return 0x08262B; // Submarine Abyssal Teal Plate
        case UiStyleTemplate::HeavyMetalForged:          return 0x121418; // Forged Blackened Iron Plate
        case UiStyleTemplate::ClassicRockAmpHead:        return 0x221810; // 70s Walnut & Bronze Plate
        case UiStyleTemplate::GrimReaperNecropolis:      return 0x0D1412; // Crypt Obsidian Plate
        case UiStyleTemplate::HellFireMagma:             return 0x220905; // Volcanic Magma Ash Plate
        case UiStyleTemplate::ApocalypseWasteland:       return 0x241A12; // Wasteland Rust Umber Plate
        case UiStyleTemplate::GrassMeadowBotanical:      return 0x1C2816; // Woodland Bark & Moss Plate
        case UiStyleTemplate::RainStormLightning:        return 0x0F1C2A; // Rain Storm Slate Plate
        default:                                         return 0x0E2A2C;
    }
}

inline const char* getTextPlatePresetColorName(int presetIdx) noexcept {
    switch (presetIdx) {
        case 0:  return "THEME MATCH";
        case 1:  return "PURE WHITE";
        case 2:  return "WARM CREAM";
        case 3:  return "SILVER MIST";
        case 4:  return "SMOKED BLACK";
        case 5:  return "PETROL TEAL";
        case 6:  return "CONSOLE SLATE";
        case 7:  return "ROYAL NAVY";
        case 8:  return "CRIMSON DARK";
        case 9:  return "OLIVE GREEN";
        case 10: return "WALNUT BROWN";
        case 11: return "CYBER VIOLET";
        case 12: return "NONE (CLEAR)";
        default: return "THEME MATCH";
    }
}

inline gui::Color getTextPlatePresetColorValue(int presetIdx, const FaceplateTheme& theme) noexcept {
    if (presetIdx == 12) {
        return gui::Color(0.0f, 0.0f, 0.0f, 0.0f);
    }
    uint32_t hex = 0x0E2A2C;
    switch (presetIdx) {
        case 0: {
            const auto tmpl = static_cast<UiStyleTemplate>(std::clamp(theme.styleTemplateIdx, 0, kTotalUiStyleTemplates - 1));
            hex = getDefaultStylePlateColorHex(tmpl);
            break;
        }
        case 1:  hex = 0xF8FAFC; break;
        case 2:  hex = 0xF5EBD6; break;
        case 3:  hex = 0xDCE4EC; break;
        case 4:  hex = 0x0B0F14; break;
        case 5:  hex = 0x0E2A2C; break;
        case 6:  hex = 0x1C2630; break;
        case 7:  hex = 0x0E182B; break;
        case 8:  hex = 0x2A0B10; break;
        case 9:  hex = 0x1A261C; break;
        case 10: hex = 0x261710; break;
        case 11: hex = 0x160B28; break;
        default: hex = getDefaultStylePlateColorHex(static_cast<UiStyleTemplate>(std::clamp(theme.styleTemplateIdx, 0, kTotalUiStyleTemplates - 1))); break;
    }
    const gui::Color c = gui::Color::fromHex(hex);
    const float luma = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
    return c.withAlpha(luma > 0.48f ? 0.68f : 0.58f);
}

inline gui::Color getSkinTextPlateColorValue(const FaceplateTheme& theme) noexcept {
    if (theme.useCustomTextPlateColor) {
        const gui::Color c = gui::Color::fromHex(theme.customTextPlateColorHex);
        const float luma = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
        return c.withAlpha(luma > 0.48f ? 0.68f : 0.58f);
    }
    return getTextPlatePresetColorValue(theme.textPlateColorPreset, theme);
}

inline std::string getSkinTextPlateColorName(const FaceplateTheme& theme) {
    if (theme.useCustomTextPlateColor) {
        return std::format("RGB #{:06X}", theme.customTextPlateColorHex);
    }
    return getTextPlatePresetColorName(theme.textPlateColorPreset);
}

inline gui::Color getSkinFontColorValue(const FaceplateTheme& theme) noexcept {
    if (theme.useCustomFontColor) {
        return gui::Color::fromHex(theme.customFontColorHex);
    }
    return getSkinFontPresetColorValue(theme.globalFontColor);
}

inline std::string getSkinFontColorName(const FaceplateTheme& theme) {
    if (theme.useCustomFontColor) {
        return std::format("RGB #{:06X}", theme.customFontColorHex);
    }
    return getSkinFontPresetColorName(theme.globalFontColor);
}

/**
 * @brief Single positioned widget on the virtual 1020x560 Faceplate Canvas.
 */
struct UiCanvasWidget {
    UiWidgetType  type{UiWidgetType::PointerKnob};
    float         x{40.0f};
    float         y{80.0f};
    float         w{122.0f};
    float         h{132.0f};
    std::string   title{"GAIN"};
    std::string   subtitle{"0.0 dB"};
    UiScalePreset scalePreset{UiScalePreset::LinearDb};
    int           boundStageIdx{-1};  // -1 = Global param, 0..N-1 = Schematic Stage index
    int           boundParamSlot{0};  // 0..4 primary, 5=Trim1, 6=Trim2, 8=AutoGainLink, 9=ParMix, 3=Bypass
    float         previewNorm{0.5f};  // Live preview normalized value in UI Builder
    UiAccentColor accentColor{UiAccentColor::WarmAmber};
    int           fontSizePreset{2};  // 0=Small, 1=Normal, 2=Large, 3=XL
    UiKnobRole    knobRole{UiKnobRole::StageOrCustom};
    int           macroStageIdx2{-99}; // Pillar 5.2: >= -1 when merged via Alt+Drag ("One-Knobify" Macro)
    int           macroParamSlot2{0};  // Secondary parameter slot driven by this Macro Knob
    std::string   customImagePath{};   // Optional user-uploaded PNG/JPG/BMP path for VintageSticker / CustomImageDecal
    float         rotationDeg{0.0f};   // Rotation angle in degrees (-180..+180) for VintageSticker / Tape
    bool          hasWaxMark{false};   // Console China-Marker / Wax-Pencil sweet-spot tick mark enabled (Shift + Right-Click)
    float         waxMarkNorm{0.5f};   // Normalized angle [0..1] of the wax-pencil sweet-spot mark
    std::string   parameterBindingId{}; // Explicit lock-free parameter binding ID (e.g. "S1_P1", "S1_LINK_IO", "S1_TELEMETRY")
    std::vector<std::string> customTickLabels{}; // Optional hardware step labels for SteppedRotarySwitch / ConcentricDualKnob
};

[[nodiscard]] inline std::string computeWidgetParameterBindingId(const UiCanvasWidget& w) {
    if (!w.parameterBindingId.empty()) return w.parameterBindingId;
    if (w.boundStageIdx >= 0) {
        if (isMeterWidget(w.type)) {
            return std::format("S{}_TELEMETRY", w.boundStageIdx + 1);
        }
        if (w.type == UiWidgetType::SectionFrame) {
            return std::format("S{}_SECTION_FRAME", w.boundStageIdx + 1);
        }
        if (isSwitchWidget(w.type) || w.type == UiWidgetType::JewelLed) {
            if (w.title == "LINK IO" || w.boundParamSlot == 108 || (w.boundParamSlot == 8 && w.title.find("LINK") != std::string::npos)) {
                return std::format("S{}_LINK_IO", w.boundStageIdx + 1);
            }
            if (w.title == "TOLERANCE" || w.subtitle == "DRIFT|CLEAN" || w.boundParamSlot == 106) {
                return std::format("S{}_TRIM2", w.boundStageIdx + 1);
            }
            if (w.title == "STAGE IN" || w.subtitle == "IN|BYP" || w.boundParamSlot == 110) {
                return std::format("S{}_STAGE_IN", w.boundStageIdx + 1);
            }
            if (w.boundParamSlot >= 0) {
                return std::format("S{}_P{}", w.boundStageIdx + 1, w.boundParamSlot + 1);
            }
            return std::format("S{}_STAGE_IN", w.boundStageIdx + 1);
        }
        if (w.knobRole == UiKnobRole::ParallelMix || w.title == "MIX" || w.title == "PAR MIX" || w.boundParamSlot == 109) {
            return std::format("S{}_PAR_MIX", w.boundStageIdx + 1);
        }
        if (w.boundParamSlot == 105) return std::format("S{}_TRIM1", w.boundStageIdx + 1);
        if (w.boundParamSlot == 106) return std::format("S{}_TRIM2", w.boundStageIdx + 1);
        return std::format("S{}_P{}", w.boundStageIdx + 1, w.boundParamSlot + 1);
    }
    if (isMeterWidget(w.type)) return "MASTER_TELEMETRY";
    if (isSwitchWidget(w.type)) return "MASTER_BYPASS";
    if (w.boundParamSlot == 0) return "GLOBAL_INPUT_DRIVE";
    if (w.boundParamSlot == 1) return "MASTER_OUTPUT_TRIM";
    if (w.boundParamSlot == 2 || w.boundParamSlot == 9) return "MASTER_DRY_WET_MIX";
    return "MASTER_CONTROL";
}

/**
 * @brief Automatically resolves & applies the best parameter binding in `chain` for a newly added knob role.
 */
inline void applyBakedKnobAutoBinding(UiCanvasWidget& w, const std::vector<SchematicNode>& chain) {
    auto containsKey = [](const std::string& hay, const char* needle) -> bool {
        if (!needle || !*needle) return false;
        std::string u = hay;
        std::string n = needle;
        for (char& c : u) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        for (char& c : n) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return u.find(n) != std::string::npos;
    };

    auto bindToStageSlot = [&](int stageIdx, int slot) {
        w.boundStageIdx  = stageIdx;
        w.boundParamSlot = slot;
        if (stageIdx >= 0 && stageIdx < static_cast<int>(chain.size())) {
            const auto& node = chain[static_cast<size_t>(stageIdx)];
            const auto& meta = getComponentMeta(node.type);
            const int pCount = static_cast<int>(meta.getParamCount());
            if (slot >= 0 && slot < pCount && w.knobRole != UiKnobRole::ParallelMix && slot != 9) {
                const auto& pSpec = meta.getParamSpec(slot);
                const float pVal  = node.getParamVal(slot);
                w.previewNorm     = computeParamNorm(pSpec, pVal);
                w.subtitle        = std::format("{:.1f} {}", pVal, pSpec.unit);
            } else {
                w.boundParamSlot = 9;
                w.previewNorm    = node.parallelMixPercent * 0.01f;
                w.subtitle       = std::format("{:.0f} %", node.parallelMixPercent);
            }
        }
    };

    auto bindByCategoryAndKeywords = [&](int catFilter, const char* kw1, const char* kw2, int fallbackSlot) -> bool {
        // First pass: exact parameter keyword match within matching category (or any category if catFilter < 0)
        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& meta = getComponentMeta(chain[i].type);
            if (catFilter >= 0 && static_cast<int>(meta.category) != catFilter) continue;
            const int pCount = static_cast<int>(meta.getParamCount());
            for (int p = 0; p < pCount; ++p) {
                const auto& ps = meta.getParamSpec(p);
                if (containsKey(ps.name, kw1) || containsKey(ps.name, kw2)) {
                    bindToStageSlot(static_cast<int>(i), p);
                    return true;
                }
            }
        }
        // Second pass: keyword match across any stage in chain
        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& meta = getComponentMeta(chain[i].type);
            const int pCount = static_cast<int>(meta.getParamCount());
            for (int p = 0; p < pCount; ++p) {
                const auto& ps = meta.getParamSpec(p);
                if (containsKey(ps.name, kw1) || containsKey(ps.name, kw2)) {
                    bindToStageSlot(static_cast<int>(i), p);
                    return true;
                }
            }
        }
        // Third pass: fallback slot on first stage of matching category
        if (catFilter >= 0) {
            for (size_t i = 0; i < chain.size(); ++i) {
                const auto& meta = getComponentMeta(chain[i].type);
                if (static_cast<int>(meta.category) == catFilter) {
                    const int pCount = static_cast<int>(meta.getParamCount());
                    bindToStageSlot(static_cast<int>(i), std::clamp(fallbackSlot, 0, pCount - 1));
                    return true;
                }
            }
        }
        if (!chain.empty()) {
            const int pCount = static_cast<int>(getComponentMeta(chain[0].type).getParamCount());
            bindToStageSlot(0, std::clamp(fallbackSlot, 0, pCount - 1));
            return true;
        }
        w.boundStageIdx  = -1;
        w.boundParamSlot = 0;
        return false;
    };

    switch (w.knobRole) {
        case UiKnobRole::MasterVolume:
            w.boundStageIdx  = -1;
            w.boundParamSlot = 1; // Global Master Output Volume
            break;

        case UiKnobRole::InputDrive:
            w.boundStageIdx  = -1;
            w.boundParamSlot = 0; // Global Input Drive (or first Preamp stage)
            for (size_t i = 0; i < chain.size(); ++i) {
                const auto& meta = getComponentMeta(chain[i].type);
                if (meta.category == SchematicCategory::PreampsAndTubes) {
                    bindToStageSlot(static_cast<int>(i), 0);
                    break;
                }
            }
            break;

        case UiKnobRole::ParallelMix:
            w.boundStageIdx  = -1;
            w.boundParamSlot = 2; // Global Parallel Wet/Dry Mix
            for (size_t i = 0; i < chain.size(); ++i) {
                if (chain[i].isParallel || chain[i].type == SchematicComponentType::ParallelSplitBus) {
                    bindToStageSlot(static_cast<int>(i), 9);
                    break;
                }
            }
            break;

        case UiKnobRole::PreampDrive:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::PreampsAndTubes), "GAIN", "DRIVE", 0);
            break;

        case UiKnobRole::TubeBiasAsym:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::PreampsAndTubes), "BIAS", "HEADROOM", 1);
            break;

        case UiKnobRole::SaturationPct:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::IronAndTape), "DRIVE", "SAT", 0);
            break;

        case UiKnobRole::TransformerCore:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::IronAndTape), "NI", "CORE", 1);
            break;

        case UiKnobRole::EqLowGain:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "LOW", "BASS", 0);
            break;

        case UiKnobRole::EqLowAtten:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "ATTEN", "DIP", 1);
            break;

        case UiKnobRole::EqLowFreq:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "LOW FREQ", "X-OVER", 2);
            break;

        case UiKnobRole::EqMidGain:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "MID", "BELL", 1);
            break;

        case UiKnobRole::EqMidFreq:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "MID FREQ", "FREQ", 2);
            break;

        case UiKnobRole::EqHighGain:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "HIGH", "AIR", 3);
            break;

        case UiKnobRole::EqHighFreq:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "HIGH FREQ", "AIR", 3);
            break;

        case UiKnobRole::FilterCutoffHz:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "HPF", "CUTOFF", 4);
            break;

        case UiKnobRole::FilterResonanceQ:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "RESONANCE", "PEAK", 1);
            break;

        case UiKnobRole::CabinetToneTilt:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::EqAndFilters), "TILT", "MIC", 0);
            break;

        case UiKnobRole::CompThreshold:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "THRESH", "PEAK RED", 0);
            break;

        case UiKnobRole::CompRatio:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "RATIO", "KNEE", 1);
            break;

        case UiKnobRole::CompAttack:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "ATTACK", "DRIVE", 2);
            break;

        case UiKnobRole::CompRelease:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "RELEASE", "RECOV", 3);
            break;

        case UiKnobRole::CompMakeupGain:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "MAKEUP", "OUTPUT", 4);
            break;

        case UiKnobRole::CompSidechainHpf:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "HPF", "TILT", 3);
            break;

        case UiKnobRole::LimiterCeiling:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "CEILING", "CLIP", 0);
            break;

        case UiKnobRole::TransientAttack:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "ATTACK", "TRANSIENT", 0);
            break;

        case UiKnobRole::TransientSustain:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "SUSTAIN", "TONAL", 1);
            break;

        case UiKnobRole::GateThreshold:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "OPEN", "GATE", 0);
            break;

        case UiKnobRole::DeEsserSibilance:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::AnalogDynamics), "SIBILANCE", "SUPPRESS", 0);
            break;

        case UiKnobRole::DelaySpaceTime:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "ECHO", "DELAY", 0);
            break;

        case UiKnobRole::DelayFeedback:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "FEEDBACK", "REGEN", 1);
            break;

        case UiKnobRole::DelayWowFlutter:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "WOW", "DETUNE", 2);
            break;

        case UiKnobRole::DelayWetMix:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "WET MIX", "MIX", 3);
            break;

        case UiKnobRole::ReverbDecay:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "DECAY", "TENSION", 0);
            break;

        case UiKnobRole::ReverbPreDelay:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "SIZE", "HOLD", 1);
            break;

        case UiKnobRole::ReverbDamping:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "DAMPING", "TONE", 1);
            break;

        case UiKnobRole::ReverbShimmer:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "SHIMMER", "OCTAVE", 1);
            break;

        case UiKnobRole::ReverbWetMix:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::DelayAndReverb), "WET MIX", "MIX", 2);
            break;

        case UiKnobRole::ModulationRate:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::ModulationAndRoute), "RATE", "LFO", 0);
            break;

        case UiKnobRole::ModulationDepth:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::ModulationAndRoute), "DEPTH", "FLANGE", 1);
            break;

        case UiKnobRole::StereoWidth:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::ModulationAndRoute), "WIDTH", "SIDE", 0);
            break;

        case UiKnobRole::BitcrushFold:
            bindByCategoryAndKeywords(static_cast<int>(SchematicCategory::IronAndTape), "FOLD", "CRUSH", 0);
            break;

        case UiKnobRole::StageOrCustom:
        default:
            break;
    }
}

/**
 * @brief Returns a concise description of a widget's auto-applied parameter binding for the inspector header.
 */
inline std::string describeWidgetAutoBinding(const UiCanvasWidget& w, const std::vector<SchematicNode>& chain) {
    auto describeSingleSlot = [&](int stageIdx, int slot) -> std::string {
        if (stageIdx >= 0 && stageIdx < static_cast<int>(chain.size())) {
            const auto& node = chain[static_cast<size_t>(stageIdx)];
            const auto& meta = getComponentMeta(node.type);
            const auto  plum = getAutoPlumbingSpec(node.type);
            const int pCount = static_cast<int>(meta.getParamCount());
            if (slot == 105 || (slot == 5 && pCount <= 5)) return std::format("S{} [TRIM] {} ({})", stageIdx + 1, plum.internalTrim1.name, meta.shortName);
            if (slot == 106 || (slot == 6 && pCount <= 6) || (slot == 7 && pCount <= 7)) return std::format("S{} [TRIM] {} ({})", stageIdx + 1, plum.internalTrim2.name, meta.shortName);
            if (slot == 108 || (slot == 8 && pCount <= 8)) return std::format("S{} Auto-Gain Link IO ({})", stageIdx + 1, meta.shortName);
            if (slot == 109 || (slot == 9 && pCount <= 9)) return std::format("S{} Parallel Mix ({})", stageIdx + 1, meta.shortName);
            if (slot >= 0 && slot < pCount) {
                const auto& ps = meta.getParamSpec(slot);
                return std::format("S{} {} ({})", stageIdx + 1, ps.name, meta.shortName);
            }
            return std::format("S{} Parallel Mix ({})", stageIdx + 1, meta.shortName);
        }
        if (slot == 0) return "Global Input Drive";
        if (slot == 1) return "Master Output Volume";
        return "Global Parallel Mix";
    };

    if (isKnobWidget(w.type) && w.macroStageIdx2 >= -1) {
        return std::format("MACRO [{}  +  {}]",
                           describeSingleSlot(w.boundStageIdx, w.boundParamSlot),
                           describeSingleSlot(w.macroStageIdx2, w.macroParamSlot2));
    }

    if (isSwitchWidget(w.type)) {
        if (w.title.find("MUTE") != std::string::npos || w.subtitle.find("MUTE") != std::string::npos) {
            return "Global Output Mute (ACTIVE / MUTE)";
        }
        if (w.title.find("PHASE") != std::string::npos || w.subtitle.find("180") != std::string::npos) {
            return "180° Phase Polarity Invert";
        }
    }
    if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size())) {
        const auto& node = chain[static_cast<size_t>(w.boundStageIdx)];
        const auto& meta = getComponentMeta(node.type);
        const auto  plum = getAutoPlumbingSpec(node.type);
        const int pCount = static_cast<int>(meta.getParamCount());
        if (isSwitchWidget(w.type) || w.type == UiWidgetType::JewelLed) {
            if (w.boundParamSlot == 108 || (w.boundParamSlot == 8 && pCount <= 8) || w.title.find("LINK") != std::string::npos) {
                return std::format("S{} Drive/Out Inverse Link ({})", w.boundStageIdx + 1, meta.shortName);
            }
            if (w.title == "TOLERANCE" || w.subtitle == "DRIFT|CLEAN" || w.boundParamSlot == 106 ||
                (w.boundParamSlot == 6 && pCount <= 6) || (w.boundParamSlot == 7 && pCount <= 7)) {
                return std::format("S{} [TRIM] {} ({})", w.boundStageIdx + 1, plum.internalTrim2.name, meta.shortName);
            }
            if (w.boundParamSlot == 110 || (w.boundParamSlot == 10 && pCount <= 10) || w.title == "STAGE IN" || w.subtitle == "IN|BYP") {
                return std::format("S{} In/Out Bypass ({})", w.boundStageIdx + 1, meta.shortName);
            }
            if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount) {
                const auto& ps = meta.getParamSpec(w.boundParamSlot);
                return std::format("S{} {} ({})", w.boundStageIdx + 1, ps.name, meta.shortName);
            }
            return std::format("S{} In/Out Bypass ({})", w.boundStageIdx + 1, meta.shortName);
        }
        return describeSingleSlot(w.boundStageIdx, w.boundParamSlot);
    }
    if (isSwitchWidget(w.type)) return "Master Unit In/Out Bypass";
    if (w.boundParamSlot == 0) return "Global Input Drive";
    if (w.boundParamSlot == 1) return "Master Output Volume";
    if (w.boundParamSlot == 2 || w.boundParamSlot == 9) return "Global Parallel Mix";
    return "Master Unit In/Out Bypass";
}

/**
 * @brief Returns ONLY the scale presets that are relevant to the given knob's bound parameter & role.
 */
inline std::vector<UiScalePreset> getRelevantScalesForKnob(
    const UiCanvasWidget& w,
    const std::vector<SchematicNode>& chain) {
    std::vector<UiScalePreset> options;

    auto containsUpper = [](const std::string& hay, const char* needle) -> bool {
        std::string u = hay;
        for (char& c : u) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return u.find(needle) != std::string::npos;
    };

    // 1. Check explicit baked-in knob role first
    switch (w.knobRole) {
        case UiKnobRole::MasterVolume:
        case UiKnobRole::InputDrive:
        case UiKnobRole::PreampDrive:
        case UiKnobRole::EqLowGain:
        case UiKnobRole::EqLowAtten:
        case UiKnobRole::EqMidGain:
        case UiKnobRole::EqHighGain:
        case UiKnobRole::CabinetToneTilt:
        case UiKnobRole::CompMakeupGain:
        case UiKnobRole::TransientAttack:
        case UiKnobRole::TransientSustain:
            options = {
                UiScalePreset::MasterVolumeDb,
                UiScalePreset::LinearDb,
                UiScalePreset::GainLineMic,
                UiScalePreset::DriveZeroTo36Db,
                UiScalePreset::FineTrimDb,
                UiScalePreset::PercentDryWet,
                UiScalePreset::DialZeroToTen,
                UiScalePreset::DialOneToTen
            };
            break;

        case UiKnobRole::CompAttack:
            options = {
                UiScalePreset::AttackFastMs,
                UiScalePreset::AttackFet1176,
                UiScalePreset::AttackSlowMs,
                UiScalePreset::LinearDb,
                UiScalePreset::DialOneToTen
            };
            break;

        case UiKnobRole::CompRelease:
            options = {
                UiScalePreset::RecoveryAuto,
                UiScalePreset::ReleaseFastMs,
                UiScalePreset::ReleaseVariMuSec,
                UiScalePreset::DialOneToTen
            };
            break;

        case UiKnobRole::CompThreshold:
        case UiKnobRole::LimiterCeiling:
        case UiKnobRole::GateThreshold:
            options = {
                UiScalePreset::ThresholdDb,
                UiScalePreset::ThresholdDeepDb,
                UiScalePreset::LimiterCeilingDb,
                UiScalePreset::PeakReduction0100,
                UiScalePreset::DialOneToTen
            };
            break;

        case UiKnobRole::CompRatio:
            options = {
                UiScalePreset::RatioSteps,
                UiScalePreset::RatioFetLim,
                UiScalePreset::RatioGentle
            };
            break;

        case UiKnobRole::FilterCutoffHz:
        case UiKnobRole::CompSidechainHpf:
        case UiKnobRole::EqLowFreq:
        case UiKnobRole::EqMidFreq:
        case UiKnobRole::EqHighFreq:
        case UiKnobRole::ModulationRate:
            options = {
                UiScalePreset::HighPassHz,
                UiScalePreset::LowFreqHz,
                UiScalePreset::MidFreqHz,
                UiScalePreset::HighAirFreqKhz,
                UiScalePreset::FullSweepFreqHz,
                UiScalePreset::ModRateHz
            };
            break;

        case UiKnobRole::DelaySpaceTime:
        case UiKnobRole::ReverbDecay:
        case UiKnobRole::ReverbPreDelay:
            options = {
                UiScalePreset::DelayTimeMs,
                UiScalePreset::ReverbDecaySec,
                UiScalePreset::RecoveryAuto,
                UiScalePreset::PercentDryWet
            };
            break;

        case UiKnobRole::ParallelMix:
        case UiKnobRole::SaturationPct:
        case UiKnobRole::TubeBiasAsym:
        case UiKnobRole::TransformerCore:
        case UiKnobRole::StereoWidth:
        case UiKnobRole::DeEsserSibilance:
        case UiKnobRole::FilterResonanceQ:
        case UiKnobRole::BitcrushFold:
        case UiKnobRole::DelayFeedback:
        case UiKnobRole::DelayWowFlutter:
        case UiKnobRole::DelayWetMix:
        case UiKnobRole::ReverbDamping:
        case UiKnobRole::ReverbShimmer:
        case UiKnobRole::ReverbWetMix:
        case UiKnobRole::ModulationDepth:
            options = {
                UiScalePreset::PercentDryWet,
                UiScalePreset::StereoWidthPct,
                UiScalePreset::CoreMatrixNiFeSt,
                UiScalePreset::DialZeroToTen,
                UiScalePreset::DialOneToTen
            };
            break;

        case UiKnobRole::StageOrCustom:
        default:
            break;
    }

    // 2. If StageOrCustom (e.g. synced directly from a Schematic stage), inspect the bound stage parameter & title
    if (options.empty()) {
        std::string paramName = w.title;
        std::string paramUnit;
        float minVal = -16.0f, maxVal = 16.0f;

        if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size())) {
            const auto& meta = getComponentMeta(chain[static_cast<size_t>(w.boundStageIdx)].type);
            const int pCount = static_cast<int>(meta.getParamCount());
            if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount && w.boundParamSlot != 9) {
                const auto& ps = meta.getParamSpec(w.boundParamSlot);
                paramName = ps.name;
                paramUnit = ps.unit;
                minVal    = ps.minVal;
                maxVal    = ps.maxVal;
            } else {
                paramUnit = "%";
            }
        } else {
            if (w.boundParamSlot == 0 || w.boundParamSlot == 1) paramUnit = "dB";
            else paramUnit = "%";
        }

        if (containsUpper(paramName, "ATTACK") || containsUpper(w.title, "ATTACK")) {
            options = {
                UiScalePreset::AttackFastMs,
                UiScalePreset::AttackFet1176,
                UiScalePreset::AttackSlowMs,
                UiScalePreset::LinearDb,
                UiScalePreset::DialOneToTen
            };
        } else if (containsUpper(paramName, "RELEASE") || containsUpper(paramName, "RECOV") ||
                   containsUpper(paramName, "TIME CONSTANT") || containsUpper(w.title, "RELEASE")) {
            options = {
                UiScalePreset::RecoveryAuto,
                UiScalePreset::ReleaseFastMs,
                UiScalePreset::ReleaseVariMuSec,
                UiScalePreset::DialOneToTen
            };
        } else if (containsUpper(paramName, "ECHO") || containsUpper(paramName, "DELAY") ||
                   containsUpper(paramName, "CLOCK") || containsUpper(paramName, "DECAY") ||
                   paramUnit == "ms" || paramUnit == "s") {
            options = {
                UiScalePreset::DelayTimeMs,
                UiScalePreset::ReverbDecaySec,
                UiScalePreset::RecoveryAuto,
                UiScalePreset::ReleaseFastMs,
                UiScalePreset::DialOneToTen
            };
        } else if (paramUnit == "Hz" || paramUnit == "kHz" ||
                   containsUpper(paramName, "FREQ") || containsUpper(paramName, "HPF") ||
                   containsUpper(paramName, "CUTOFF") || containsUpper(paramName, "RATE")) {
            options = {
                UiScalePreset::HighPassHz,
                UiScalePreset::LowFreqHz,
                UiScalePreset::MidFreqHz,
                UiScalePreset::HighAirFreqKhz,
                UiScalePreset::FullSweepFreqHz,
                UiScalePreset::ModRateHz
            };
        } else if (paramUnit == ":1" || containsUpper(paramName, "RATIO") || containsUpper(w.title, "RATIO")) {
            options = {
                UiScalePreset::RatioSteps,
                UiScalePreset::RatioFetLim,
                UiScalePreset::RatioGentle
            };
        } else if (paramUnit == "dBFS" || (minVal < 0.0f && maxVal <= 0.0f) ||
                   containsUpper(paramName, "THRESH") || containsUpper(paramName, "CEIL") ||
                   containsUpper(paramName, "PEAK RED") || containsUpper(w.title, "THRESH")) {
            options = {
                UiScalePreset::ThresholdDb,
                UiScalePreset::ThresholdDeepDb,
                UiScalePreset::LimiterCeilingDb,
                UiScalePreset::PeakReduction0100,
                UiScalePreset::DialOneToTen
            };
        } else if (paramUnit == "%" || paramUnit == "ct" ||
                   containsUpper(paramName, "MIX") || containsUpper(paramName, "WIDTH") ||
                   containsUpper(paramName, "BIAS") || containsUpper(paramName, "HYSTERESIS") ||
                   containsUpper(paramName, "SAT") || containsUpper(paramName, "DEPTH")) {
            options = {
                UiScalePreset::PercentDryWet,
                UiScalePreset::StereoWidthPct,
                UiScalePreset::CoreMatrixNiFeSt,
                UiScalePreset::DialZeroToTen,
                UiScalePreset::DialOneToTen
            };
        } else {
            // Gain / Volume / Drive / Trim / EQ Boost-Cut (dB)
            options = {
                UiScalePreset::LinearDb,
                UiScalePreset::MasterVolumeDb,
                UiScalePreset::GainLineMic,
                UiScalePreset::DriveZeroTo36Db,
                UiScalePreset::FineTrimDb,
                UiScalePreset::DialOneToTen
            };
        }
    }

    // Ensure the widget's current scalePreset is always in the list
    if (std::find(options.begin(), options.end(), w.scalePreset) == options.end()) {
        options.insert(options.begin(), w.scalePreset);
    }
    return options;
}

/**
 * @brief Width (reference px) of one tile in the top S1 -> S2 -> S3 signal-chain selector strip
 * (x = 34..730, 8 px gaps). With 1-3 stages each tile is capped at 220 px so the strip stays
 * compact at the top-left; with more stages the tiles share the full 696 px.
 * Shared by the painter, the Studio hit-test and the exported plugin editor.
 */
[[nodiscard]] inline float getSignalChainTileWidthRef(int stageCount) noexcept {
    constexpr float kAvailW = 696.0f;
    constexpr float kGap    = 8.0f;
    const int n = std::max(1, stageCount);
    const float shared = (kAvailW - static_cast<float>(n - 1) * kGap) / static_cast<float>(n);
    return (n <= 3) ? std::min(220.0f, shared) : shared;
}

/**
 * @brief Complete Faceplate Layout (Theme + Positioned Widgets) in reference 1020x560 space.
 */
struct FaceplateLayout {
    static constexpr float kRefWidth  = 1020.0f;
    static constexpr float kRefHeight = 560.0f;
    static constexpr float kStageViewportLeft  = 34.0f;
    static constexpr float kStageViewportRight = 730.0f;
    static constexpr float kStageViewportWidth = kStageViewportRight - kStageViewportLeft;

    FaceplateTheme              theme{};
    std::vector<UiCanvasWidget> widgets{};
    mutable float               rackScrollX{0.0f}; // Retired horizontal scroll offset (always 0.0f)
    mutable int                 activeStageIdx{0}; // Focused active stage index (0 .. totalStages - 1) when > 3 stages are loaded

    [[nodiscard]] bool isMasterMeterWidget(const UiCanvasWidget& w) const noexcept {
        return isMeterWidget(w.type) && w.x >= 730.0f && w.w >= 115.0f;
    }

    /**
     * @brief Returns true for any permanent widget in the right-hand METERING · OUTPUT bay
     * (or global Input/Output section), including the SectionFrame, Master Meter, Output Trim,
     * Master Mix, Mono/Stereo switch, In/Byp switch, and RivetedNameplate badge.
     * These widgets cannot be deleted, though their style, colour, scale, font, and text are editable.
     */
    [[nodiscard]] bool isPermanentMasterSectionWidget(const UiCanvasWidget& w) const noexcept {
        if (w.type == UiWidgetType::VintageSticker) return false;
        if (w.type == UiWidgetType::RivetedNameplate) return true;
        if (isMasterMeterWidget(w)) return true;
        if (w.boundStageIdx < 0) return true;
        if (w.x >= 730.0f) return true;
        return false;
    }

    [[nodiscard]] bool isScrollableStageWidget(const UiCanvasWidget& w) const noexcept {
        return w.boundStageIdx >= 0 && !isMasterMeterWidget(w);
    }

    [[nodiscard]] int getStageCount() const noexcept {
        int maxStage = 0;
        for (const auto& w : widgets) {
            if (w.boundStageIdx >= 0 && !isMasterMeterWidget(w)) {
                maxStage = std::max(maxStage, w.boundStageIdx + 1);
            }
        }
        return maxStage;
    }

    // The top S1 -> S2 -> S3 selector strip is shown only when 2 or more components are loaded;
    // when only 1 component is loaded, the strip is hidden and the stage bay expands to the top.
    [[nodiscard]] bool usesSignalChainSelectorStrip() const noexcept {
        return getStageCount() > 1;
    }

    [[nodiscard]] bool isWidgetVisibleForActiveStage(const UiCanvasWidget& w) const noexcept {
        const int stageCount = getStageCount();
        if (stageCount <= 0) return true;
        if (w.boundStageIdx < 0 || isMasterMeterWidget(w)) return true;
        const int clampedActive = std::clamp(activeStageIdx, 0, stageCount - 1);
        return w.boundStageIdx == clampedActive;
    }

    [[nodiscard]] float getMaxStageRightX() const noexcept {
        float maxRight = 0.0f;
        for (const auto& w : widgets) {
            if (w.type == UiWidgetType::SectionFrame && w.boundStageIdx >= 0) {
                maxRight = std::max(maxRight, w.x + w.w);
            }
        }
        return maxRight;
    }

    [[nodiscard]] float getMaxRackScrollX() const noexcept {
        return 0.0f;
    }

    [[nodiscard]] float getEffectiveWidgetX(const UiCanvasWidget& w) const noexcept {
        return w.x;
    }

    void scrollToStage(int stageIdx) const noexcept {
        const int stageCount = std::max(1, getStageCount());
        activeStageIdx = std::clamp(stageIdx, 0, stageCount - 1);
        rackScrollX    = 0.0f;
    }

    [[nodiscard]] std::vector<UiCanvasWidget> getWidgetsForStage(size_t stageIndex) const {
        std::vector<UiCanvasWidget> result;
        bool hasStageMeter = false;
        for (const auto& w : widgets) {
            if (w.type == UiWidgetType::SectionFrame || w.type == UiWidgetType::RivetedNameplate) {
                continue;
            }
            if (w.boundStageIdx == static_cast<int>(stageIndex)) {
                UiCanvasWidget copy = w;
                if (copy.parameterBindingId.empty()) {
                    copy.parameterBindingId = computeWidgetParameterBindingId(copy);
                }
                if (isMeterWidget(copy.type)) hasStageMeter = true;
                result.push_back(std::move(copy));
            }
        }
        if (!hasStageMeter) {
            for (const auto& w : widgets) {
                if (isMeterWidget(w.type)) {
                    UiCanvasWidget copy = w;
                    copy.boundStageIdx = static_cast<int>(stageIndex);
                    copy.parameterBindingId = std::format("S{}_TELEMETRY", stageIndex + 1);
                    result.push_back(std::move(copy));
                    break;
                }
            }
        }
        // Include calibrated internal parameter slots (0..6) in the stage binding manifest when fewer than 6 faceplate controls are shown
        for (int slot = 0; slot <= 6 && result.size() < 7; ++slot) {
            bool alreadyPresent = false;
            for (const auto& rw : result) {
                if (!isMeterWidget(rw.type) && rw.boundParamSlot == slot) {
                    alreadyPresent = true;
                    break;
                }
            }
            if (!alreadyPresent) {
                UiCanvasWidget virt{};
                virt.type               = UiWidgetType::PointerKnob;
                virt.boundStageIdx      = static_cast<int>(stageIndex);
                virt.boundParamSlot     = slot;
                virt.parameterBindingId = computeWidgetParameterBindingId(virt);
                result.push_back(std::move(virt));
            }
        }
        return result;
    }
};

/**
 * @brief 6-Layer UnifiedStyleTemplate Specification matching the TypeScript schema:
 *   1. surface (baseColorHex, textureType, bevelDepthPx, panelBorderStyle)
 *   2. knobStyle (silhouette, bodyColorHex, capColorHex, indicatorColorHex, showOuterTickMarks)
 *   3. switchStyle (CHROME_BAT_TOGGLE | ILLUMINATED_SQUARE_PUSH | NEON_ROCKER_SWITCH | LOCKING_CAGE_TOGGLE | FLAT_PILL_VECTOR)
 *   4. meterStyle (archetype, faceplateColorHex, activeGlowHex)
 *   5. typography (fontFamily, labelStyle, primaryTextColorHex)
 *   6. hardwareTrim (RACK_EARS_19IN | TORX_BOLTS | BRASS_FLATHEADS | LEATHER_AMP_CORNERS | NONE_FRAMELESS)
 */
enum class UnifiedTextureType : int {
    BRUSHED_METAL_HORIZ = 0,
    HAMMERTONE_PEBBLE   = 1,
    DIAMOND_TREADPLATE  = 2,
    WOVEN_TWEED         = 3,
    WRINKLE_ENAMEL      = 4,
    FROSTED_GLASS       = 5,
    WEATHERED_PATINA    = 6,
    CARBON_WEAVE        = 7
};

enum class UnifiedPanelBorderStyle : int {
    RIVETED_PLATES  = 0,
    PIPING_TOLEX    = 1,
    CHAMFERED_CNC   = 2,
    HAIRLINE_VECTOR = 3,
    GOTHIC_ARCH     = 4
};

enum class UnifiedKnobSilhouette : int {
    MARCONI_WINGED     = 0, // Classic console rectangular finger wing
    CHICKEN_HEAD       = 1, // Pointed beak dial (LA-2A / Tweed)
    FLUTED_BAKELITE    = 2, // Scalloped vintage edges (Passive EQ / Vari-Mu)
    SKIRTED_AMP        = 3, // Numbered 0-10 silver/gold skirt (Silver/Gold Numbered Skirt)
    COLLET_COLOR_CAP   = 4, // Round pot with colored center cap (Console Collet Color Cap)
    KNURLED_METAL_DOME = 5, // Heavy cross-hatched cylinder (Telecaster / Rectifier)
    VECTOR_ARC_RING    = 6  // Flat modern minimalist ring (Modern Minimalist Arc)
};

enum class UnifiedSwitchStyle : int {
    CHROME_BAT_TOGGLE       = 0,
    ILLUMINATED_SQUARE_PUSH = 1,
    NEON_ROCKER_SWITCH      = 2,
    LOCKING_CAGE_TOGGLE     = 3,
    FLAT_PILL_VECTOR        = 4
};

enum class UnifiedMeterArchetype : int {
    VINTAGE_VU_NEEDLE   = 0,
    LED_SEGMENT_LADDER  = 1,
    CRT_GREEN_PHOSPHOR  = 2,
    NIXIE_TUBE          = 3,
    PASTEL_SHADER_CURVE = 4
};

enum class UnifiedFontFamily : int {
    ENGRAVED_DIN          = 0,
    STENCILED_MILITARY    = 1,
    SILKSCREEN_SERIF      = 2,
    MODERN_GEOMETRIC_SANS = 3,
    LCD_PIXEL_MONO        = 4
};

enum class UnifiedHardwareTrim : int {
    RACK_EARS_19IN      = 0,
    TORX_BOLTS          = 1,
    BRASS_FLATHEADS     = 2,
    LEATHER_AMP_CORNERS = 3,
    NONE_FRAMELESS      = 4
};

struct UnifiedStyleTemplate {
    UiStyleTemplate templateEnum{UiStyleTemplate::SleekModernDark};
    const char*     id{"sleek_modern_dark"};
    const char*     name{"SLEEK MODERN DARK"};
    const char*     summaryPills[4]{"Frosted Glass", "Vector Arc Ring", "LED Ladder", "Laser Etch"};

    // 1. CHASSIS & SURFACE MATERIAL
    struct SurfaceLayer {
        uint32_t                baseColorHex{0x14171A};
        UnifiedTextureType      textureType{UnifiedTextureType::FROSTED_GLASS};
        float                   bevelDepthPx{2.0f};
        UnifiedPanelBorderStyle panelBorderStyle{UnifiedPanelBorderStyle::CHAMFERED_CNC};
    } surface;

    // 2. KNOB SILHOUETTE & CAP GEOMETRY
    struct KnobStyleLayer {
        UnifiedKnobSilhouette  silhouette{UnifiedKnobSilhouette::VECTOR_ARC_RING};
        uint32_t               bodyColorHex{0x1C2228};
        uint32_t               capColorHex{0x283038};
        uint32_t               indicatorColorHex{0x00F5D4};
        gui::KnobTickMarkStyle showOuterTickMarks{gui::KnobTickMarkStyle::GlowArc};
    } knobStyle;

    // 3. SWITCH & BUTTON ARCHETYPE
    UnifiedSwitchStyle switchStyle{UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH};

    // 4. TELEMETRY & METER SKIN
    struct MeterStyleLayer {
        UnifiedMeterArchetype archetype{UnifiedMeterArchetype::PASTEL_SHADER_CURVE};
        uint32_t              faceplateColorHex{0x0E141B};
        uint32_t              activeGlowHex{0x00F5D4};
    } meterStyle;

    // 5. TYPOGRAPHY & LABELING
    struct TypographyLayer {
        UnifiedFontFamily       fontFamily{UnifiedFontFamily::MODERN_GEOMETRIC_SANS};
        gui::UiLabelPlaqueStyle labelStyle{gui::UiLabelPlaqueStyle::GlowingLaserEtch};
        uint32_t                primaryTextColorHex{0xE8F1F2};
    } typography;

    // 6. CORNER HARDWARE & FASTENERS
    UnifiedHardwareTrim hardwareTrim{UnifiedHardwareTrim::TORX_BOLTS};

    // Legacy mappings for fine-grained compatibility
    gui::ChassisFinish   fallbackFinish{gui::ChassisFinish::ObsidianBlack};
    gui::BadgeVisualStyle badgeStyle{gui::BadgeVisualStyle::SleekLaserGlass};
    UiAccentColor        accentPreset{UiAccentColor::NeonCyan};
};

inline const std::array<UnifiedStyleTemplate, kTotalUiStyleTemplates>& getAllUnifiedStyleTemplates() noexcept {
    using namespace avra::gui;
    static const std::array<UnifiedStyleTemplate, kTotalUiStyleTemplates> kTemplates = {{
        // 0: KennedarthTealConsole (Teal Console Default)
        {
            UiStyleTemplate::SleekModernDark, "kennedarth_teal_console", "KENNEDARTH TEAL CONSOLE",
            {"Anodized Teal", "3D Marconi Skirt", "Backlit Warm VU", "Riveted Plaque"},
            { 0x245A5C, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::MARCONI_WINGED, 0xC8CED4, 0x1E2022, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x181614, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::AnodizedTeal, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 1: GrimyRoadWornRelic
        {
            UiStyleTemplate::GrimyRoadWornRelic, "grimy_road_worn_relic", "RELIC",
            {"Weathered Patina", "Chicken Head", "Vintage VU", "Embossed Tape"},
            { 0x354A3E, UnifiedTextureType::WEATHERED_PATINA, 3.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::CHICKEN_HEAD, 0xC8CED4, 0x1E2022, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xE5D4AA, 0xFF9F1C },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::EmbossedDymoTape, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::HammertoneGreen, BadgeVisualStyle::GrimyMilitaryTag, UiAccentColor::WarmAmber
        },
        // 2: BritishConsoleSlate
        {
            UiStyleTemplate::BritishConsoleSlate, "british_console_slate", "BRITISH CONSOLE SLATE",
            {"Brushed Slate", "Marconi Winged", "Vintage VU", "Traffolyte"},
            { 0x3D4C5A, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::MARCONI_WINGED, 0xC8CED4, 0x9E2428, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF5EBD0, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 3: Blackface1176Studio -> FET BLACKFACE STUDIO
        {
            UiStyleTemplate::Blackface1176Studio, "fet_blackface_studio", "FET BLACKFACE STUDIO",
            {"Brushed Black", "Skirted Dial 0-10", "Vintage VU", "19\" Rack Ears"},
            { 0x171A1D, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::SKIRTED_AMP, 0xC8CED4, 0x1E2022, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF6E8C3, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 4: GermanBroadcastCream
        {
            UiStyleTemplate::GermanBroadcastCream, "german_broadcast_cream", "GERMAN BROADCAST LAB",
            {"Wrinkle Cream", "Collet Colour Cap", "Vintage VU", "Traffolyte"},
            { 0xDDD3BE, UnifiedTextureType::WRINKLE_ENAMEL, 2.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::NEON_ROCKER_SWITCH,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFAF2DF, 0xC84B00 },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0x181A1C },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::VintageCream, BadgeVisualStyle::VintageCreamEmblem, UiAccentColor::SunsetOrange
        },
        // 5: VintageTubeBakelite
        {
            UiStyleTemplate::VintageTubeBakelite, "vintage_tube_bakelite", "VINTAGE TUBE BAKELITE",
            {"Hammertone Bronze", "Fluted Bakelite", "Vintage VU", "Traffolyte"},
            { 0x5C3A28, UnifiedTextureType::HAMMERTONE_PEBBLE, 3.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0xC8CED4, 0x1E1612, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF4E2B8, 0xFF9F1C },
            { UnifiedFontFamily::SILKSCREEN_SERIF, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::WarmCopperBronze, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },
        // 6: CrimsonMasteringLab
        {
            UiStyleTemplate::CrimsonMasteringLab, "crimson_mastering_lab", "CRIMSON MASTERING LAB",
            {"Brushed Crimson", "Knurled Dome", "LED Ladder", "Direct Silk"},
            { 0x5E1922, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0xC8CED4, 0x1E2022, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x14080B, 0xFF9F1C },
            { UnifiedFontFamily::LCD_PIXEL_MONO, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::CrimsonMastering, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },
        // 7: CyberpunkSynthMatrix
        {
            UiStyleTemplate::CyberpunkSynthMatrix, "cyberpunk_synth_matrix", "CYBERPUNK SYNTH MATRIX",
            {"Carbon Weave", "Vector Arc Ring", "CRT Phosphor", "Laser Etch"},
            { 0x241538, UnifiedTextureType::CARBON_WEAVE, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x1E1828, 0xEDEAE0, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::NEON_ROCKER_SWITCH,
            { UnifiedMeterArchetype::CRT_GREEN_PHOSPHOR, 0x0A0512, 0x00F5D4 },
            { UnifiedFontFamily::LCD_PIXEL_MONO, UiLabelPlaqueStyle::GlowingLaserEtch, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::CyberpunkViolet, BadgeVisualStyle::SleekLaserGlass, UiAccentColor::WarmAmber
        },
        // 8: ArcticMinimalistVector
        {
            UiStyleTemplate::ArcticMinimalistVector, "arctic_minimalist_vector", "ARCTIC MINIMAL VECTOR",
            {"Frosted Glass", "Vector Arc Ring", "Pastel Shader", "Frameless"},
            { 0xE6ECF0, UnifiedTextureType::FROSTED_GLASS, 1.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::PASTEL_SHADER_CURVE, 0x1A2430, 0xC84B00 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0x181A1C },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::ArcticWhiteEnamel, BadgeVisualStyle::MinimalSilkscreen, UiAccentColor::SunsetOrange
        },
        // 9: BoutiqueSunburstWood -> 80s DIGITAL REMOTE WHITE
        {
            UiStyleTemplate::BoutiqueSunburstWood, "80s_digital_remote_white", "80s DIGITAL REMOTE WHITE",
            {"Arctic White Enamel", "Collet Colour Cap", "LED Bargraph", "19\" Rack Ears"},
            { 0xEAECEF, UnifiedTextureType::WRINKLE_ENAMEL, 2.0f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x121820, 0xC84B00 },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0x181A1C },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::ArcticWhiteEnamel, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::SunsetOrange
        },
        // 10: OxidizedCopperIndustrial
        {
            UiStyleTemplate::OxidizedCopperIndustrial, "oxidized_copper_industrial", "OXIDIZED PATINA RELIC",
            {"Weathered Patina", "Chicken Head", "Nixie Tube", "Embossed Tape"},
            { 0x2F5249, UnifiedTextureType::WEATHERED_PATINA, 3.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::CHICKEN_HEAD, 0xC8CED4, 0x1E2022, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::NIXIE_TUBE, 0x121A18, 0xFF9F1C },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::EmbossedDymoTape, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::WarmCopperBronze, BadgeVisualStyle::GrimyMilitaryTag, UiAccentColor::WarmAmber
        },
        // 11: HeritageMultiPanel
        {
            UiStyleTemplate::HeritageMultiPanel, "heritage_multi_panel", "HERITAGE MULTI-PANEL",
            {"Era Sub-Panels", "Heritage Knobs", "Vintage VU", "Torx Bolts"},
            { 0x1A1E22, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::SKIRTED_AMP, 0xC8CED4, 0x1E2022, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF5E9C8, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },

        // ====================================================================
        // 24 STUDIO, MASTERING, SYNTH, MODERN & DARK-ART THEMES (12..35)
        // ====================================================================

        // 12: PultecHammertoneBlue -> PASSIVE HAMMERTONE BLUE
        {
            UiStyleTemplate::PultecHammertoneBlue, "passive_hammertone_blue", "PASSIVE HAMMERTONE BLUE",
            {"Hammertone Blue", "Fluted Bakelite", "Vintage VU", "19\" Rack Ears"},
            { 0x3B556E, UnifiedTextureType::HAMMERTONE_PEBBLE, 3.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0xC8CED4, 0x181C20, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF7E8C4, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 13: Api500CobaltConsole (Studio Hardware #2)
        {
            UiStyleTemplate::Api500CobaltConsole, "american_500_cobalt", "AMERICAN 500 COBALT",
            {"Wrinkle Charcoal", "Collet Colour Cap", "LED Ladder", "19\" Rack Ears"},
            { 0x1C2530, UnifiedTextureType::WRINKLE_ENAMEL, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x1E2634, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x0E131A, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::CobaltStudioBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 14: TeletronixLa2aOptoGrey (Studio Hardware #3)
        {
            UiStyleTemplate::TeletronixLa2aOptoGrey, "vintage_opto_grey", "VINTAGE OPTO GREY",
            {"Brushed Silver", "Chicken Head", "Vintage VU", "19\" Rack Ears"},
            { 0x8E969E, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::CHICKEN_HEAD, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFAF0CE, 0xC84B00 },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0x181A1C },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::BrushedSilver, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::SunsetOrange
        },
        // 15: Fairchild670VariMuBlack (Studio Hardware #4)
        {
            UiStyleTemplate::Fairchild670VariMuBlack, "broadcast_varimu_black", "BROADCAST VARI-MU BLACK",
            {"Wrinkle Enamel", "Fluted Bakelite", "Vintage VU", "Traffolyte"},
            { 0x191B1E, UnifiedTextureType::WRINKLE_ENAMEL, 3.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF5E6BE, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },
        // 16: Ssl4000GBusConsole -> BRITISH 4K CONSOLE GREY
        {
            UiStyleTemplate::Ssl4000GBusConsole, "british_4k_console_grey", "BRITISH 4K CONSOLE GREY",
            {"Brushed Console", "Collet Colour Cap", "Vintage VU", "19\" Rack Ears"},
            { 0x3A4047, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.0f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x1E2226, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF4ECD6, 0xFF7A1A },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 17: ManleyMassiveNavy (Studio Hardware #6)
        {
            UiStyleTemplate::ManleyMassiveNavy, "boutique_passive_navy", "BOUTIQUE PASSIVE NAVY",
            {"Brushed Navy", "Knurled Dome", "Vintage VU", "Brass Bolts"},
            { 0x1A2842, UnifiedTextureType::BRUSHED_METAL_HORIZ, 3.0f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0xC8CED4, 0x1C222B, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF8ECC8, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::RoyalNavyBlue, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },

        // 18: SwissMasteringTransfer (Studio Mastering #7)
        {
            UiStyleTemplate::SwissMasteringTransferFinish, "swiss_mastering_transfer", "SWISS MASTERING TRANSFER",
            {"Nextel Dark Suede", "Machined Silver Capstans", "Twin Backlit VU", "Illuminated Transport Keys"},
            { 0x26292E, UnifiedTextureType::WRINKLE_ENAMEL, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x1E2024, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF8EDD0, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 19: ViennaBroadcastLab (Studio Mastering #8)
        {
            UiStyleTemplate::Plexi1959GoldPanel, "vienna_broadcast_lab", "VIENNA BROADCAST LAB",
            {"Olive Anodized Frame", "Detented Stepped Bakelite", "Edgewise Needle Meter", "Engraved Traffolyte"},
            { 0x3B4638, UnifiedTextureType::BRUSHED_METAL_HORIZ, 3.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0xC8CED4, 0x181A1D, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFAF0D2, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::HammertoneGreen, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },
        // 20: Tokyo1982PcmRack (Studio Mastering #9)
        {
            UiStyleTemplate::DualRectifierTreadplate, "tokyo_1982_pcm_rack", "TOKYO 1982 PCM RACK",
            {"Anodized Charcoal Rack", "Silver-Skirted Encoders", "Red/Amber LED Bargraph", "Brushed Rack Ears"},
            { 0x1E2228, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::SKIRTED_AMP, 0xC8CED4, 0x1E2024, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x101317, 0xFF7A1A },
            { UnifiedFontFamily::LCD_PIXEL_MONO, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::WarmAmber
        },
        // 21: MasteringEllipticalGold (Studio Mastering #10)
        {
            UiStyleTemplate::BritishAc30CopperTop, "mastering_elliptical_gold", "MASTERING ELLIPTICAL GOLD",
            {"Bead-Blasted Champagne", "24-Step Detent Switches", "Phase Lissajous Scope", "Countersunk Brass"},
            { 0xC4A86B, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF7EBD0, 0xC84B00 },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0x181A1C },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::VintageGold, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::SunsetOrange
        },
        // 22: NordicSpectralGlass (Studio Mastering #11)
        {
            UiStyleTemplate::CitrusCrunchAmpTolex, "nordic_spectral_glass", "NORDIC SPECTRAL GLASS",
            {"Frosted Slate Acrylic", "Hairline Vector Rings", "Real-Time Spectral Shader", "Frameless Floating Pill"},
            { 0x1C2430, UnifiedTextureType::FROSTED_GLASS, 1.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x1E242E, 0xEDEAE0, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::PASTEL_SHADER_CURVE, 0x0E141C, 0xFF9F1C },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::GlowingLaserEtch, 0xEDEAE0 },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::SleekLaserGlass, UiAccentColor::WarmAmber
        },

        // 23: Juno106SynthwaveChorus (Synth Theme #1)
        {
            UiStyleTemplate::Juno106SynthwaveChorus, "analog_84_synthwave", "1984 ANALOG SYNTHWAVE",
            {"Brushed Slate", "Collet Colour Cap", "LED Ladder", "Torx Bolts"},
            { 0x232730, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.0f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x181B22, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x0E1015, 0xFF7A1A },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::SleekLaserGlass, UiAccentColor::WarmAmber
        },
        // 24: WestCoastBuchlaGoldLab (Synth Theme #2)
        {
            UiStyleTemplate::WestCoastBuchlaGoldLab, "west_coast_gold_lab", "WEST-COAST SYNTH LAB",
            {"Brushed Cobalt", "Marconi Winged", "CRT Phosphor", "Embossed Tape"},
            { 0x1D3B66, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::MARCONI_WINGED, 0xC8CED4, 0x1E2634, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::CRT_GREEN_PHOSPHOR, 0x081612, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::EmbossedDymoTape, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::CobaltStudioBlue, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },
        // 25: PutneyVcs3PinMatrix (Synth Theme #3)
        {
            UiStyleTemplate::PutneyVcs3PinMatrix, "british_pin_matrix_synth", "PIN-MATRIX SYNTH LAB",
            {"Hammertone Green", "Collet Colour Cap", "CRT Phosphor", "Traffolyte"},
            { 0x244235, UnifiedTextureType::HAMMERTONE_PEBBLE, 3.0f, UnifiedPanelBorderStyle::PIPING_TOLEX },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x181C1A, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::CRT_GREEN_PHOSPHOR, 0x0A1610, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xEDEAE0 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::HammertoneGreen, BadgeVisualStyle::VintageCreamEmblem, UiAccentColor::WarmAmber
        },
        // 26: Ms20SemiModularPatch (Synth Theme #4)
        {
            UiStyleTemplate::Ms20SemiModularPatch, "semi_modular_patch_bay", "SEMI-MODULAR PATCH BAY",
            {"Wrinkle Enamel", "Fluted Bakelite", "Vintage VU", "Embossed Tape"},
            { 0x15181C, UnifiedTextureType::WRINKLE_ENAMEL, 2.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0xC8CED4, 0x181B20, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF0E6CC, 0xFF9F1C },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::EmbossedDymoTape, 0xEDEAE0 },
            UnifiedHardwareTrim::RACK_EARS_19IN,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::GrimyMilitaryTag, UiAccentColor::WarmAmber
        },
        // 27: Op1FieldCncAluminum (Synth Theme #5)
        {
            UiStyleTemplate::Op1FieldCncAluminum, "nordic_field_cnc_aluminum", "FIELD CNC ALUMINUM",
            {"Brushed Silver", "Vector Arc Ring", "Pastel Shader", "Frameless"},
            { 0xD4DCE2, UnifiedTextureType::BRUSHED_METAL_HORIZ, 1.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::PASTEL_SHADER_CURVE, 0x161C24, 0xC84B00 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0x181A1C },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::BrushedSilver, BadgeVisualStyle::MinimalSilkscreen, UiAccentColor::SunsetOrange
        },

        // 28: SoothePastelGlassShader (Modern Theme #1)
        {
            UiStyleTemplate::SoothePastelGlassShader, "spectral_pastel_shader", "SPECTRAL PASTEL SHADER",
            {"Frosted Glass", "Vector Arc Ring", "Pastel Shader", "Laser Etch"},
            { 0x171A2E, UnifiedTextureType::FROSTED_GLASS, 1.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x1C2032, 0xEDEAE0, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::PASTEL_SHADER_CURVE, 0x0F1222, 0xFF9F1C },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::GlowingLaserEtch, 0xEDEAE0 },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::CyberpunkViolet, BadgeVisualStyle::SleekLaserGlass, UiAccentColor::WarmAmber
        },
        // 29: ProQSpectrumObsidian (Modern Theme #2)
        {
            UiStyleTemplate::ProQSpectrumObsidian, "spectrum_mastering_obsidian", "SPECTRUM OBSIDIAN LAB",
            {"Frosted Obsidian", "Vector Arc Ring", "Pastel Shader", "Laser Etch"},
            { 0x11161E, UnifiedTextureType::FROSTED_GLASS, 1.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x181E26, 0xEDEAE0, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::PASTEL_SHADER_CURVE, 0x0A0E14, 0xFF7A1A },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::GlowingLaserEtch, 0xEDEAE0 },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::SleekLaserGlass, UiAccentColor::WarmAmber
        },
        // 30: ValhallaNordicVector (Modern Theme #3)
        {
            UiStyleTemplate::ValhallaNordicVector, "nordic_minimal_vector", "NORDIC MINIMAL VECTOR",
            {"Frosted Cream", "Vector Arc Ring", "Pastel Shader", "Frameless"},
            { 0xEFE6DC, UnifiedTextureType::FROSTED_GLASS, 1.0f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xC8CED4, 0x141618, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::PASTEL_SHADER_CURVE, 0x1E1822, 0xC84B00 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0x181A1C },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::VintageCream, BadgeVisualStyle::MinimalSilkscreen, UiAccentColor::SunsetOrange
        },
        // 31: TokyoDawnMasteringLab (Modern Theme #4)
        {
            UiStyleTemplate::TokyoDawnMasteringLab, "precision_gunmetal_mastering", "PRECISION MASTERING LAB",
            {"Brushed Gunmetal", "Collet Colour Cap", "LED Ladder", "Torx Bolts"},
            { 0x20262E, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.0f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xC8CED4, 0x1C222A, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x0D1116, 0xFF7A1A },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::DirectSilkscreen, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::SleekLaserGlass, UiAccentColor::WarmAmber
        },

        // 32: SovietColdWarBunkerRadar
        {
            UiStyleTemplate::SovietColdWarBunkerRadar, "bunker_radar", "BUNKER RADAR",
            {"Olive Hammertone", "Marconi Winged", "CRT Phosphor", "Embossed Tape"},
            { 0x384634, UnifiedTextureType::HAMMERTONE_PEBBLE, 3.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::MARCONI_WINGED, 0xC8CED4, 0x1C221A, 0xEDEAE0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::CRT_GREEN_PHOSPHOR, 0x08140A, 0xFF9F1C },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::EmbossedDymoTape, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::HammertoneGreen, BadgeVisualStyle::GrimyMilitaryTag, UiAccentColor::WarmAmber
        },
        // 33: ChernobylGeigerDosimeter
        {
            UiStyleTemplate::ChernobylGeigerDosimeter, "chernobyl_geiger_dosimeter", "GEIGER DOSIMETER LAB",
            {"Hazard Patina", "Knurled Dome", "Nixie Tube", "Embossed Tape"},
            { 0x4A4020, UnifiedTextureType::WEATHERED_PATINA, 3.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0xC8CED4, 0x1E1C16, 0xEDEAE0, KnobTickMarkStyle::Numbered0To10 },
            UnifiedSwitchStyle::NEON_ROCKER_SWITCH,
            { UnifiedMeterArchetype::NIXIE_TUBE, 0x14120A, 0xFF9F1C },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::EmbossedDymoTape, 0xEDEAE0 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::VintageGold, BadgeVisualStyle::GrimyMilitaryTag, UiAccentColor::WarmAmber
        },
        // 34: FrostedGlassNeumorphism
        {
            UiStyleTemplate::FrostedGlassNeumorphism, "frosted_glass", "FROSTED GLASS",
            {"Frosted Acrylic", "Frosted Glass Dial", "Dual Frosted Arc VU", "Soft Pill Switch"},
            { 0xD5DEE8, UnifiedTextureType::FROSTED_GLASS, 1.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xEFF4F8, 0xE2EAF2, 0xFF8C38, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xEBF1F6, 0xFF8C38 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::TraffolytePlaque, 0x111827 },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        },
        // 36: MinimalistPureVector
        {
            UiStyleTemplate::MinimalistPureVector, "minimalist_pure_vector", "MINIMALIST",
            {"Pure Matte White", "Flat Vector Dial", "Minimal Needle VU", "Hairline Frame"},
            { 0xF2F4F7, UnifiedTextureType::FROSTED_GLASS, 1.0f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xFFFFFF, 0xF7F9FB, 0x181C20, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFFFFFF, 0x181C20 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0x181C20 },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::ArcticWhiteEnamel, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::CobaltBlue
        },
        // 36: GreyscaleMonochrome
        {
            UiStyleTemplate::GreyscaleMonochrome, "greyscale_monochrome", "GREYSCALE / MONOCHROME",
            {"Matte Charcoal", "Spun Silver Cap", "Dark Monochrome VU", "Torx Bolts"},
            { 0x26292E, UnifiedTextureType::BRUSHED_METAL_HORIZ, 2.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::SKIRTED_AMP, 0xD0D6DC, 0x8E969E, 0xFFFFFF, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x1A1D20, 0xE2E8F0 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::DirectSilkscreen, 0xE8EEF2 },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        },
        // 38: FuturisticSciFiHud
        {
            UiStyleTemplate::FuturisticSciFiHud, "futuristic_scifi_hud", "FUTURISTIC / SCI-FI",
            {"Starship HUD Grid", "Holographic Energy Dial", "Plasma Multi-Bar", "Tactical HUD Pad"},
            { 0x071C24, UnifiedTextureType::CARBON_WEAVE, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0x0A2834, 0x05161E, 0x00F5D4, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::LED_SEGMENT_LADDER, 0x041016, 0x00F5D4 },
            { UnifiedFontFamily::LCD_PIXEL_MONO, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::AnodizedTeal, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::NeonCyan
        },
        // 39: RetroDistressedHazard
        {
            UiStyleTemplate::RetroDistressedHazard, "retro_distressed_hazard", "RETRO DISTRESSED",
            {"Hazard Yellow & Rust", "Red Valve Handwheel", "Cracked Vintage VU", "Heavy Rivets"},
            { 0xB58922, UnifiedTextureType::WEATHERED_PATINA, 4.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0x5C4838, 0x9E2A2B, 0xF5B800, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xE6CF9E, 0xFF9F1C },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::VintageGold, BadgeVisualStyle::GrimyMilitaryTag, UiAccentColor::PureWhite
        },
        // 40: UnderwaterSubmarine
        {
            UiStyleTemplate::UnderwaterSubmarine, "underwater_submarine", "UNDERWATER / SUBMARINE",
            {"Abyssal Brass Hull", "Brass Ship-Wheel Helm", "Diving Porthole Gauge", "Naval Brass Toggle"},
            { 0x103C42, UnifiedTextureType::WEATHERED_PATINA, 3.5f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0xB8860B, 0x124E58, 0x38F8FF, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x8CE0E6, 0x00E5FF },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xD8F8FF },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::AnodizedTeal, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::NeonCyan
        },
        // 40: TattooArtTraditional
        {
            UiStyleTemplate::TattooArtTraditional, "tattoo_art_traditional", "TATTOO ART",
            {"Aged Flash Parchment", "Sacred Heart & Skull", "Rose-Banner Vintage VU", "Brass Flatheads"},
            { 0xD4B483, UnifiedTextureType::WEATHERED_PATINA, 3.0f, UnifiedPanelBorderStyle::PIPING_TOLEX },
            { UnifiedKnobSilhouette::MARCONI_WINGED, 0x5C4033, 0xB81D24, 0xFFF3B0, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF5DFB8, 0xD90429 },
            { UnifiedFontFamily::SILKSCREEN_SERIF, UiLabelPlaqueStyle::TraffolytePlaque, 0x1E120C },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::VintageCream, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::WarmAmber
        },
        // 43: HeavyMetalForged
        {
            UiStyleTemplate::HeavyMetalForged, "heavy_metal_forged", "METAL",
            {"Spiked Blackened Iron", "3D Pewter Skull Knob", "Forged Dark-Iron VU", "Clean Forged Plate"},
            { 0x181A1E, UnifiedTextureType::DIAMOND_TREADPLATE, 4.0f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0x2B3038, 0x94A3B8, 0xFF4500, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x1A1816, 0xFF4500 },
            { UnifiedFontFamily::SILKSCREEN_SERIF, UiLabelPlaqueStyle::TraffolytePlaque, 0xF8FAFC },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::SunsetOrange
        },
        // 42: ClassicRockAmpHead (70s Classic Rock Studio Recording Console)
        {
            UiStyleTemplate::ClassicRockAmpHead, "classic_rock_console", "CLASSIC ROCK",
            {"70s Walnut & Bronze Desk", "Fluted Brass Skirt Knob", "Warm Backlit Desk VU", "Cast Brass Plaque"},
            { 0x26221E, UnifiedTextureType::BRUSHED_METAL_HORIZ, 3.0f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::SKIRTED_AMP, 0xD4AF37, 0x1E1812, 0xFFFFFF, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x1A1816, 0xFF9F1C },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::WarmCopperBronze, BadgeVisualStyle::CastBrassPlaque, UiAccentColor::PureWhite
        },
        // 45: FlowerPowerPsychedelic
        {
            UiStyleTemplate::FlowerPowerPsychedelic, "flower_power_psychedelic", "FLOWER POWER",
            {"60s Psychedelic Swirl", "Daisy Petal Rotary", "Warm Cream Backlit VU", "Groovy Peace Tin"},
            { 0xF59E0B, UnifiedTextureType::WRINKLE_ENAMEL, 2.0f, UnifiedPanelBorderStyle::PIPING_TOLEX },
            { UnifiedKnobSilhouette::COLLET_COLOR_CAP, 0xFFFBEB, 0xEC4899, 0x581C87, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFEF3C7, 0xEC4899 },
            { UnifiedFontFamily::SILKSCREEN_SERIF, UiLabelPlaqueStyle::TraffolytePlaque, 0x281020 },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::VintageGold, BadgeVisualStyle::VintageCreamEmblem, UiAccentColor::WarmAmber
        },
        // 44: GrimReaperNecropolis
        {
            UiStyleTemplate::GrimReaperNecropolis, "grim_reaper_necropolis", "GRIM REAPER",
            {"Gothic Crypt Stone", "Skull & Bone Dial", "Reaper Scythe Arch VU", "Brushed Crypt Plaque"},
            { 0x141C1A, UnifiedTextureType::WEATHERED_PATINA, 4.0f, UnifiedPanelBorderStyle::GOTHIC_ARCH },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0x26322E, 0x0E1512, 0xFFFFFF, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x162820, 0x38F888 },
            { UnifiedFontFamily::ENGRAVED_DIN, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::ObsidianBlack, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        },
        // 47: HellFireMagma
        {
            UiStyleTemplate::HellFireMagma, "hell_fire_magma", "HELL FIRE",
            {"Cracked Volcanic Magma", "Molten Fissure Knob", "Horned Demon Hellfire VU", "Ember Switch"},
            { 0x240C08, UnifiedTextureType::WEATHERED_PATINA, 4.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0x1C0A06, 0xFF4500, 0xFFFFFF, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::NEON_ROCKER_SWITCH,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFF9F1C, 0xFF4500 },
            { UnifiedFontFamily::SILKSCREEN_SERIF, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::CrimsonMastering, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        },
        // 49: ApocalypseWasteland
        {
            UiStyleTemplate::ApocalypseWasteland, "apocalypse_wasteland", "APOCALYPSE",
            {"Fallout Scrap & Barbed Wire", "Rusted Cogwheel Dial", "Radiation Geiger VU", "Hazard Stripes"},
            { 0x4A3B32, UnifiedTextureType::WEATHERED_PATINA, 4.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::KNURLED_METAL_DOME, 0x6E472B, 0x3D2516, 0xFFFFFF, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xF5CB22, 0xF5B800 },
            { UnifiedFontFamily::STENCILED_MILITARY, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::WarmCopperBronze, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        },
        // 47: GrassMeadowBotanical
        {
            UiStyleTemplate::GrassMeadowBotanical, "grass_meadow_botanical", "GRASS / MEADOW",
            {"Woodland Bark & Moss", "Carved Wood-Slice Knob", "Botanical Wood VU", "Wildflower Trim"},
            { 0x4E3624, UnifiedTextureType::WEATHERED_PATINA, 3.0f, UnifiedPanelBorderStyle::RIVETED_PLATES },
            { UnifiedKnobSilhouette::FLUTED_BAKELITE, 0x8D6346, 0x5C3A21, 0xFFFFFF, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::CHROME_BAT_TOGGLE,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xEED9B3, 0x4ADE80 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::BRASS_FLATHEADS,
            ChassisFinish::WarmCopperBronze, BadgeVisualStyle::VintageCreamEmblem, UiAccentColor::PureWhite
        },
        // 52: SkyCloudsGoldenHour
        {
            UiStyleTemplate::SkyCloudsGoldenHour, "sky_clouds_golden_hour", "SKY / CLOUDS",
            {"Golden-Hour Cumulus Sky", "Sunburst & Cloud Dial", "Sunlit Cloud VU", "Soft Sky Pill"},
            { 0x8AB4D8, UnifiedTextureType::FROSTED_GLASS, 1.5f, UnifiedPanelBorderStyle::HAIRLINE_VECTOR },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xFDE68A, 0xF59E0B, 0xFFFFFF, KnobTickMarkStyle::Dots },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xFEF3C7, 0xF59E0B },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::TraffolytePlaque, 0x0F172A },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        },
        // 53: RainStormLightning
        {
            UiStyleTemplate::RainStormLightning, "rain_storm_lightning", "RAIN / STORM",
            {"Rain-Streaked Storm Slate", "Electric Lightning Dial", "Storm Arc Blue VU", "Torx Bolts"},
            { 0x1E2D3D, UnifiedTextureType::FROSTED_GLASS, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0x283C50, 0x0F1C2A, 0x38BDF8, KnobTickMarkStyle::GlowArc },
            UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0x102236, 0x38BDF8 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::TraffolytePlaque, 0xFFFFFF },
            UnifiedHardwareTrim::TORX_BOLTS,
            ChassisFinish::BritishSlateBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::IceSkyBlue
        },
        // 54: SnowIcyGlacial
        {
            UiStyleTemplate::SnowIcyGlacial, "snow_icy_glacial", "SNOW / ICY",
            {"Frozen Glacial Ice & Icicles", "Faceted Ice-Crystal Knob", "Glacial Ice VU", "Frost Frame"},
            { 0x1A4B6E, UnifiedTextureType::FROSTED_GLASS, 2.5f, UnifiedPanelBorderStyle::CHAMFERED_CNC },
            { UnifiedKnobSilhouette::VECTOR_ARC_RING, 0xBAE6FD, 0x38BDF8, 0xFFFFFF, KnobTickMarkStyle::RadialLines },
            UnifiedSwitchStyle::FLAT_PILL_VECTOR,
            { UnifiedMeterArchetype::VINTAGE_VU_NEEDLE, 0xE0F2FE, 0x0284C7 },
            { UnifiedFontFamily::MODERN_GEOMETRIC_SANS, UiLabelPlaqueStyle::TraffolytePlaque, 0xF0F8FF },
            UnifiedHardwareTrim::NONE_FRAMELESS,
            ChassisFinish::CobaltStudioBlue, BadgeVisualStyle::BrushedAluminumPlate, UiAccentColor::PureWhite
        }
    }};
    return kTemplates;
}

inline const UnifiedStyleTemplate& getUnifiedStyleTemplateSpec(UiStyleTemplate t) noexcept {
    const auto& all = getAllUnifiedStyleTemplates();
    const int idx = std::clamp(static_cast<int>(t), 0, kTotalUiStyleTemplates - 1);
    return all[static_cast<size_t>(idx)];
}

inline const char* getUiStyleTemplateName(UiStyleTemplate t) noexcept {
    return getUnifiedStyleTemplateSpec(t).name;
}

inline std::string getUiStyleTemplateDescription(UiStyleTemplate t) {
    const auto& spec = getUnifiedStyleTemplateSpec(t);
    return std::format("{}  |  {}  |  {}  |  {}",
                       spec.summaryPills[0], spec.summaryPills[1], spec.summaryPills[2], spec.summaryPills[3]);
}

inline bool hasCustomSurfaceOrColorOverride(const FaceplateTheme& theme) noexcept {
    using namespace avra::gui;
    if (theme.hasSurfaceOverlayOverride)
        return true;

    const int activeTplIdx = std::clamp(theme.styleTemplateIdx, 0, kTotalUiStyleTemplates - 1);
    const auto activeTpl = static_cast<UiStyleTemplate>(activeTplIdx);
    const auto& spec = getUnifiedStyleTemplateSpec(activeTpl);
    const bool isKennedarth = (activeTpl == UiStyleTemplate::SleekModernDark);

    SurfaceGrain defaultGrain = SurfaceGrain::HorizontalBrushed;
    switch (spec.surface.textureType) {
        case UnifiedTextureType::BRUSHED_METAL_HORIZ:
            defaultGrain = isKennedarth ? SurfaceGrain::VerticalBrushed : SurfaceGrain::HorizontalBrushed;
            break;
        case UnifiedTextureType::HAMMERTONE_PEBBLE:   defaultGrain = SurfaceGrain::HammertoneDimpled; break;
        case UnifiedTextureType::DIAMOND_TREADPLATE:  defaultGrain = SurfaceGrain::DiamondTreadplate; break;
        case UnifiedTextureType::WOVEN_TWEED:         defaultGrain = SurfaceGrain::WovenTweed; break;
        case UnifiedTextureType::WRINKLE_ENAMEL:      defaultGrain = SurfaceGrain::WrinklePowderCoat; break;
        case UnifiedTextureType::FROSTED_GLASS:       defaultGrain = SurfaceGrain::SleekGlassAcrylic; break;
        case UnifiedTextureType::WEATHERED_PATINA:
            defaultGrain = (activeTpl == UiStyleTemplate::GrimyRoadWornRelic)
                ? SurfaceGrain::GrimyRoadWornRelic
                : SurfaceGrain::OxidizedPatina;
            break;
        case UnifiedTextureType::CARBON_WEAVE:        defaultGrain = SurfaceGrain::CarbonWeave; break;
    }

    if (theme.grain != defaultGrain)
        return true;
    if (theme.finish != spec.fallbackFinish)
        return true;
    if (theme.useCustomFinishColor && theme.customFinishHex != spec.surface.baseColorHex)
        return true;
    return false;
}

inline void applyUiStyleTemplate(
    FaceplateLayout& layout,
    UiStyleTemplate tpl,
    const std::vector<SchematicNode>& chain = {}) {
    using namespace avra::gui;

    const auto& spec = getUnifiedStyleTemplateSpec(tpl);
    const bool isHeritage   = (tpl == UiStyleTemplate::HeritageMultiPanel);
    const bool isKennedarth = (tpl == UiStyleTemplate::SleekModernDark);

    layout.theme.styleTemplateIdx          = static_cast<int>(tpl);
    layout.theme.isCustomThemeModified     = false;
    layout.theme.hasSurfaceOverlayOverride = false;
    layout.theme.customThemeName.clear();
    layout.theme.unifyFaceplateTheme   = !isHeritage;
    layout.theme.finish               = spec.fallbackFinish;
    layout.theme.useCustomFinishColor = !isHeritage && !isKennedarth;
    layout.theme.customFinishHex      = spec.surface.baseColorHex;
    layout.theme.bevelDepthPx         = spec.surface.bevelDepthPx;

    // 1. Surface procedural texture & border style
    switch (spec.surface.textureType) {
        case UnifiedTextureType::BRUSHED_METAL_HORIZ:
            layout.theme.grain = isKennedarth ? SurfaceGrain::VerticalBrushed : SurfaceGrain::HorizontalBrushed;
            break;
        case UnifiedTextureType::HAMMERTONE_PEBBLE:   layout.theme.grain = SurfaceGrain::HammertoneDimpled; break;
        case UnifiedTextureType::DIAMOND_TREADPLATE:  layout.theme.grain = SurfaceGrain::DiamondTreadplate; break;
        case UnifiedTextureType::WOVEN_TWEED:         layout.theme.grain = SurfaceGrain::WovenTweed; break;
        case UnifiedTextureType::WRINKLE_ENAMEL:      layout.theme.grain = SurfaceGrain::WrinklePowderCoat; break;
        case UnifiedTextureType::FROSTED_GLASS:       layout.theme.grain = SurfaceGrain::SleekGlassAcrylic; break;
        case UnifiedTextureType::WEATHERED_PATINA:
            layout.theme.grain = (tpl == UiStyleTemplate::GrimyRoadWornRelic)
                ? SurfaceGrain::GrimyRoadWornRelic
                : SurfaceGrain::OxidizedPatina;
            break;
        case UnifiedTextureType::CARBON_WEAVE:        layout.theme.grain = SurfaceGrain::CarbonWeave; break;
    }
    layout.theme.surfaceFinish = static_cast<int>(layout.theme.grain);

    SectionFrameStyle targetFrameStyle = SectionFrameStyle::EngravedBevel;
    switch (spec.surface.panelBorderStyle) {
        case UnifiedPanelBorderStyle::RIVETED_PLATES:  targetFrameStyle = SectionFrameStyle::RivetedSubPlate; break;
        case UnifiedPanelBorderStyle::PIPING_TOLEX:    targetFrameStyle = SectionFrameStyle::PipingTolex; break;
        case UnifiedPanelBorderStyle::CHAMFERED_CNC:   targetFrameStyle = SectionFrameStyle::EngravedBevel; break;
        case UnifiedPanelBorderStyle::HAIRLINE_VECTOR: targetFrameStyle = SectionFrameStyle::DoubleHairline; break;
        case UnifiedPanelBorderStyle::GOTHIC_ARCH:     targetFrameStyle = SectionFrameStyle::GothicArch; break;
    }
    layout.theme.panelBorderStyle = targetFrameStyle;

    // 2. Knob silhouette, custom colors & tick marks
    switch (spec.knobStyle.silhouette) {
        case UnifiedKnobSilhouette::MARCONI_WINGED:
            layout.theme.globalKnobType = isKennedarth ? UiWidgetType::PointerKnob : UiWidgetType::MarconiRedKnob;
            break;
        case UnifiedKnobSilhouette::CHICKEN_HEAD:       layout.theme.globalKnobType = UiWidgetType::ChickenHeadKnob; break;
        case UnifiedKnobSilhouette::FLUTED_BAKELITE:    layout.theme.globalKnobType = UiWidgetType::BakeliteMasterKnob; break;
        case UnifiedKnobSilhouette::SKIRTED_AMP:        layout.theme.globalKnobType = UiWidgetType::SkirtedCapKnob; break;
        case UnifiedKnobSilhouette::COLLET_COLOR_CAP:   layout.theme.globalKnobType = UiWidgetType::ColletBroadcastKnob; break;
        case UnifiedKnobSilhouette::KNURLED_METAL_DOME: layout.theme.globalKnobType = UiWidgetType::KnurledBarrelKnob; break;
        case UnifiedKnobSilhouette::VECTOR_ARC_RING:    layout.theme.globalKnobType = UiWidgetType::ModernArcKnob; break;
    }
    layout.theme.useCustomKnobColors = !isHeritage && !isKennedarth;
    layout.theme.knobBodyHex         = spec.knobStyle.bodyColorHex;
    layout.theme.knobCapHex          = spec.knobStyle.capColorHex;
    layout.theme.knobIndicatorHex    = spec.knobStyle.indicatorColorHex;
    layout.theme.knobTickStyle       = spec.knobStyle.showOuterTickMarks;

    // 3. Switch & Button archetype
    switch (spec.switchStyle) {
        case UnifiedSwitchStyle::CHROME_BAT_TOGGLE:       layout.theme.globalSwitchType = UiWidgetType::ChromeToggleSwitch; break;
        case UnifiedSwitchStyle::ILLUMINATED_SQUARE_PUSH: layout.theme.globalSwitchType = UiWidgetType::IlluminatedPushButton; break;
        case UnifiedSwitchStyle::NEON_ROCKER_SWITCH:      layout.theme.globalSwitchType = UiWidgetType::RockerPowerSwitch; break;
        case UnifiedSwitchStyle::LOCKING_CAGE_TOGGLE:     layout.theme.globalSwitchType = UiWidgetType::LockingCageToggle; break;
        case UnifiedSwitchStyle::FLAT_PILL_VECTOR:        layout.theme.globalSwitchType = UiWidgetType::FlatPillVectorSwitch; break;
    }

    // 4. Telemetry & Meter skin
    switch (spec.meterStyle.archetype) {
        case UnifiedMeterArchetype::VINTAGE_VU_NEEDLE:   layout.theme.globalMeterType = UiWidgetType::AnalogVuMeter; break;
        case UnifiedMeterArchetype::LED_SEGMENT_LADDER:  layout.theme.globalMeterType = UiWidgetType::LedBargraphMeter; break;
        case UnifiedMeterArchetype::CRT_GREEN_PHOSPHOR:  layout.theme.globalMeterType = UiWidgetType::CrtOscilloscopeScreen; break;
        case UnifiedMeterArchetype::NIXIE_TUBE:          layout.theme.globalMeterType = UiWidgetType::NixieTubeDisplay; break;
        case UnifiedMeterArchetype::PASTEL_SHADER_CURVE: layout.theme.globalMeterType = UiWidgetType::PastelShaderCurveMeter; break;
    }
    layout.theme.meterFaceplateHex = spec.meterStyle.faceplateColorHex;
    layout.theme.meterGlowHex      = spec.meterStyle.activeGlowHex;
    layout.theme.globalAccentColor = spec.accentPreset;

    // Specialized per-theme custom Knob, Switch & Meter overrides for the reference styles
    switch (tpl) {
        case UiStyleTemplate::FrostedGlassNeumorphism:
            layout.theme.globalKnobType   = UiWidgetType::NeumorphicGlassKnob;
            layout.theme.globalSwitchType = UiWidgetType::NeumorphicPillButton;
            layout.theme.globalMeterType  = UiWidgetType::NeumorphicDualArcMeter;
            break;
        case UiStyleTemplate::MinimalistPureVector:
            layout.theme.globalKnobType   = UiWidgetType::FlatVectorKnob;
            layout.theme.globalSwitchType = UiWidgetType::FlatPillVectorSwitch;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::GreyscaleMonochrome:
            layout.theme.globalKnobType   = UiWidgetType::SkirtedCapKnob;
            layout.theme.globalSwitchType = UiWidgetType::ChromeToggleSwitch;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::FuturisticSciFiHud:
            layout.theme.globalKnobType   = UiWidgetType::SciFiHudRingKnob;
            layout.theme.globalSwitchType = UiWidgetType::SciFiHologramSwitch;
            layout.theme.globalMeterType  = UiWidgetType::CyberLuxeMultiBarMeter;
            break;
        case UiStyleTemplate::RetroDistressedHazard:
            layout.theme.globalKnobType   = UiWidgetType::ValveWheelRelicKnob;
            layout.theme.globalSwitchType = UiWidgetType::NavalBrassToggle;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::UnderwaterSubmarine:
            layout.theme.globalKnobType   = UiWidgetType::SubmarineHelmKnob;
            layout.theme.globalSwitchType = UiWidgetType::NavalBrassToggle;
            layout.theme.globalMeterType  = UiWidgetType::SubmarinePortholeGauge;
            break;
        case UiStyleTemplate::TattooArtTraditional:
            layout.theme.globalKnobType   = UiWidgetType::TattooFlashKnob;
            layout.theme.globalSwitchType = UiWidgetType::NavalBrassToggle;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::HeavyMetalForged:
            layout.theme.globalKnobType   = UiWidgetType::MetalSkullKnob;
            layout.theme.globalSwitchType = UiWidgetType::ChromeToggleSwitch;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::ClassicRockAmpHead:
            layout.theme.globalKnobType   = UiWidgetType::SkirtedCapKnob;
            layout.theme.globalSwitchType = UiWidgetType::ChromeToggleSwitch;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::FlowerPowerPsychedelic:
            layout.theme.globalKnobType   = UiWidgetType::FlowerDaisyKnob;
            layout.theme.globalSwitchType = UiWidgetType::NavalBrassToggle;
            layout.theme.globalMeterType  = UiWidgetType::AnalogVuMeter;
            break;
        case UiStyleTemplate::GrimReaperNecropolis:
            layout.theme.globalKnobType   = UiWidgetType::ReaperCryptKnob;
            layout.theme.globalSwitchType = UiWidgetType::ChromeToggleSwitch;
            layout.theme.globalMeterType  = UiWidgetType::ReaperScytheArchMeter;
            break;
        case UiStyleTemplate::HellFireMagma:
            layout.theme.globalKnobType   = UiWidgetType::HellfireMagmaKnob;
            layout.theme.globalSwitchType = UiWidgetType::HellfireEmberSwitch;
            layout.theme.globalMeterType  = UiWidgetType::HellfireHornedVuMeter;
            break;
        case UiStyleTemplate::ApocalypseWasteland:
            layout.theme.globalKnobType   = UiWidgetType::ApocalypseCogKnob;
            layout.theme.globalSwitchType = UiWidgetType::NavalBrassToggle;
            layout.theme.globalMeterType  = UiWidgetType::ApocalypseGeigerVuMeter;
            break;
        case UiStyleTemplate::GrassMeadowBotanical:
            layout.theme.globalKnobType   = UiWidgetType::MeadowWoodSliceKnob;
            layout.theme.globalSwitchType = UiWidgetType::NavalBrassToggle;
            layout.theme.globalMeterType  = UiWidgetType::NatureElementalVuMeter;
            break;
        case UiStyleTemplate::SkyCloudsGoldenHour:
            layout.theme.globalKnobType   = UiWidgetType::SkySunburstKnob;
            layout.theme.globalSwitchType = UiWidgetType::NeumorphicPillButton;
            layout.theme.globalMeterType  = UiWidgetType::NatureElementalVuMeter;
            break;
        case UiStyleTemplate::RainStormLightning:
            layout.theme.globalKnobType   = UiWidgetType::StormRaindropKnob;
            layout.theme.globalSwitchType = UiWidgetType::SciFiHologramSwitch;
            layout.theme.globalMeterType  = UiWidgetType::NatureElementalVuMeter;
            break;
        case UiStyleTemplate::SnowIcyGlacial:
            layout.theme.globalKnobType   = UiWidgetType::GlacialIceCrystalKnob;
            layout.theme.globalSwitchType = UiWidgetType::NeumorphicPillButton;
            layout.theme.globalMeterType  = UiWidgetType::NatureElementalVuMeter;
            break;
        default:
            break;
    }

    // 5. Typography & Label plaque style (Default for all plugins & styles: Studio Sans, Size Large)
    (void)spec.typography.fontFamily;
    layout.theme.globalFontFamily   = FontFamily::StudioSans;
    layout.theme.globalFontSizePreset = 2;
    layout.theme.labelPlaqueStyle   = spec.typography.labelStyle;
    layout.theme.useCustomFontColor = !isHeritage;
    layout.theme.customFontColorHex = spec.typography.primaryTextColorHex;
    layout.theme.globalFontColor    = (spec.typography.primaryTextColorHex < 0x404040)
        ? UiSkinFontColor::ObsidianBlackInk
        : UiSkinFontColor::SilkscreenWhite;
    layout.theme.textPlateColorPreset    = 0;
    layout.theme.useCustomTextPlateColor = false;
    layout.theme.customTextPlateColorHex = getDefaultStylePlateColorHex(tpl);

    // 6. Corner Hardware & Fasteners
    switch (spec.hardwareTrim) {
        case UnifiedHardwareTrim::RACK_EARS_19IN:
            layout.theme.hardwareStyle = ChassisHardwareStyle::RackEarsAndBolts;
            layout.theme.screwStyle    = ScrewHeadStyle::PhillipsCross;
            break;
        case UnifiedHardwareTrim::TORX_BOLTS:
            layout.theme.hardwareStyle = ChassisHardwareStyle::CornerScrews4;
            layout.theme.screwStyle    = ScrewHeadStyle::TorxStar6;
            break;
        case UnifiedHardwareTrim::BRASS_FLATHEADS:
            layout.theme.hardwareStyle = ChassisHardwareStyle::IndustrialRivets8;
            layout.theme.screwStyle    = ScrewHeadStyle::GoldPlatedBrass;
            break;
        case UnifiedHardwareTrim::LEATHER_AMP_CORNERS:
            layout.theme.hardwareStyle = ChassisHardwareStyle::LeatherAmpCorners;
            layout.theme.screwStyle    = ScrewHeadStyle::DomedChromeRivet;
            break;
        case UnifiedHardwareTrim::NONE_FRAMELESS:
            layout.theme.hardwareStyle = ChassisHardwareStyle::CleanScrewless;
            layout.theme.screwStyle    = ScrewHeadStyle::CountersunkFlush;
            break;
    }
    layout.theme.cornerScrews = (layout.theme.hardwareStyle != ChassisHardwareStyle::CleanScrewless);

    const BadgeVisualStyle targetBadgeStyle = spec.badgeStyle;
    layout.theme.globalBadgeStyle = targetBadgeStyle;

    for (auto& w : layout.widgets) {
        w.fontSizePreset = layout.theme.globalFontSizePreset;
        if (w.type == UiWidgetType::SectionFrame) {
            w.scalePreset = static_cast<UiScalePreset>(static_cast<int>(targetFrameStyle));
            if (layout.theme.unifyFaceplateTheme) {
                w.accentColor = layout.theme.globalAccentColor;
            } else if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size())) {
                const auto bp = getDefaultUIBlueprint(chain[static_cast<size_t>(w.boundStageIdx)].type);
                w.boundParamSlot = static_cast<int>(bp.heritageFinish);
                w.accentColor    = bp.heritageAccent;
            }
        } else if (w.type == UiWidgetType::RivetedNameplate) {
            w.scalePreset = static_cast<UiScalePreset>(static_cast<int>(targetBadgeStyle));
            w.accentColor = layout.theme.globalAccentColor;
            if (w.boundStageIdx == -1 && std::abs(w.y - 428.0f) <= 24.0f) {
                w.x = 776.0f;
                w.y = 438.0f;
                w.w = 180.0f;
                w.h = 76.0f;
            }
        } else if (isKnobWidget(w.type)) {
            if (!layout.theme.unifyFaceplateTheme && w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size())) {
                const auto bp = getDefaultUIBlueprint(chain[static_cast<size_t>(w.boundStageIdx)].type);
                w.type        = (w.boundParamSlot == 0) ? bp.primaryKnobStyle : bp.secondaryKnobStyle;
                w.accentColor = bp.heritageAccent;
            } else {
                w.type        = layout.theme.globalKnobType;
                w.accentColor = layout.theme.globalAccentColor;
            }
            if (w.boundStageIdx == -1 && (w.title == "OUTPUT TRIM" || w.title == "MASTER MIX") && std::abs(w.y - 210.0f) <= 26.0f) {
                w.x = (w.title == "OUTPUT TRIM") ? 783.0f : 891.0f;
                w.y = 222.0f;
                w.w = 58.0f;
                w.h = 58.0f;
            }
        } else if (isFaderWidget(w.type)) {
            w.accentColor = layout.theme.globalAccentColor;
        } else if (isSwitchWidget(w.type)) {
            if (w.boundStageIdx == -1 && (w.boundParamSlot == 5 || w.boundParamSlot == 6)) {
                w.type        = UiWidgetType::IlluminatedPushButton;
                w.accentColor = (w.boundParamSlot == 6) ? UiAccentColor::NeonCyan : UiAccentColor::WarmAmber;
            } else {
                w.type        = layout.theme.globalSwitchType;
                w.accentColor = layout.theme.globalAccentColor;
            }
            if (w.boundStageIdx == -1 && (w.title == "STEREO MODE" || w.title == "MASTER BYP") && std::abs(w.y - 342.0f) <= 26.0f) {
                w.x = (w.title == "STEREO MODE") ? 768.0f : 876.0f;
                w.y = 352.0f;
                w.w = 88.0f;
                w.h = 54.0f;
            }
        } else if (isMeterWidget(w.type) && w.type != UiWidgetType::DigitalTrimBargraph) {
            // Preserve per-stage topology feature widgets (GlowingTubeWindow, TransferCurve, SpectralTilt, Oscilloscope) when switching unified themes
            if (w.boundStageIdx >= 0 && !layout.isMasterMeterWidget(w)) {
                w.accentColor = (w.type == UiWidgetType::GlowingTubeWindow) ? UiAccentColor::WarmAmber : layout.theme.globalAccentColor;
            } else {
                w.type        = layout.theme.globalMeterType;
                w.accentColor = layout.theme.globalAccentColor;
                if (w.x >= 730.0f) {
                    w.x = 776.0f;
                    w.y = 88.0f;
                    w.w = 180.0f;
                    w.h = 88.0f;
                }
            }
        }
    }
}

struct SavedUserTheme {
    std::string    name{"CUSTOM THEME"};
    FaceplateTheme theme{};
};

inline void applySavedUserTheme(
    FaceplateLayout& layout,
    const SavedUserTheme& userTheme,
    const std::vector<SchematicNode>& chain = {}) {
    using namespace avra::gui;

    const std::string savedBrand       = layout.theme.brandName;
    const std::string savedPluginTitle = layout.theme.pluginTitle;
    const std::string savedSubText     = layout.theme.subtitleText;
    const std::string savedBadgeFooter = layout.theme.badgeFooterText;
    const int         savedGridSnap    = layout.theme.gridSnapPx;
    const bool        savedShowGrid    = layout.theme.showGrid;
    const bool        savedShowGuides  = layout.theme.showSnapGuides;
    const bool        savedEarsOnly    = layout.theme.earsOnlyBlackoutMode;

    layout.theme                       = userTheme.theme;
    layout.theme.brandName             = savedBrand.empty() ? "AVRA AUDIO" : savedBrand;
    layout.theme.pluginTitle           = savedPluginTitle;
    layout.theme.subtitleText          = savedSubText;
    layout.theme.badgeFooterText       = savedBadgeFooter;
    layout.theme.gridSnapPx            = savedGridSnap;
    layout.theme.showGrid              = savedShowGrid;
    layout.theme.showSnapGuides        = savedShowGuides;
    layout.theme.earsOnlyBlackoutMode  = savedEarsOnly;
    layout.theme.customThemeName       = userTheme.name;
    layout.theme.isCustomThemeModified = false;

    const SectionFrameStyle targetFrameStyle = layout.theme.panelBorderStyle;
    const BadgeVisualStyle  targetBadgeStyle = layout.theme.globalBadgeStyle;

    for (auto& w : layout.widgets) {
        w.fontSizePreset = layout.theme.globalFontSizePreset;
        if (w.type == UiWidgetType::SectionFrame) {
            w.scalePreset = static_cast<UiScalePreset>(static_cast<int>(targetFrameStyle));
            if (layout.theme.unifyFaceplateTheme) {
                w.accentColor = layout.theme.globalAccentColor;
            } else if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size())) {
                const auto bp = getDefaultUIBlueprint(chain[static_cast<size_t>(w.boundStageIdx)].type);
                w.boundParamSlot = static_cast<int>(bp.heritageFinish);
                w.accentColor    = bp.heritageAccent;
            }
        } else if (w.type == UiWidgetType::RivetedNameplate) {
            w.scalePreset = static_cast<UiScalePreset>(static_cast<int>(targetBadgeStyle));
            w.accentColor = layout.theme.globalAccentColor;
        } else if (isKnobWidget(w.type)) {
            w.type        = layout.theme.globalKnobType;
            w.accentColor = layout.theme.globalAccentColor;
        } else if (isFaderWidget(w.type)) {
            w.accentColor = layout.theme.globalAccentColor;
        } else if (isSwitchWidget(w.type)) {
            w.type        = layout.theme.globalSwitchType;
            w.accentColor = layout.theme.globalAccentColor;
        } else if (isMeterWidget(w.type) && w.type != UiWidgetType::DigitalTrimBargraph) {
            if (w.boundStageIdx >= 0 && !layout.isMasterMeterWidget(w)) {
                w.accentColor = (w.type == UiWidgetType::GlowingTubeWindow) ? UiAccentColor::WarmAmber : layout.theme.globalAccentColor;
            } else {
                w.type        = layout.theme.globalMeterType;
                w.accentColor = layout.theme.globalAccentColor;
            }
        }
    }
}

/**
 * @brief Dynamically auto-fits `widget.w` and `widget.h` for `UiWidgetType::VintageSticker` on spawn and text edit
 * by measuring both text strings via DirectWrite `IDWriteTextLayout::GetMetrics()` (`gui::Graphics::measureTextMetrics`),
 * and clamps minimum resize drag bounds (`minWidth = 90px`, `minHeight = 46px` for 2-line tape).
 */
inline void autoFitStickerWidgetBounds(
    UiCanvasWidget& widget,
    gui::FontFamily fontFamily = gui::FontFamily::StudioSans,
    int fontSizePreset = 2,
    bool clampOnlyForResize = false) noexcept {
    if (widget.type != UiWidgetType::VintageSticker) return;

    const auto preset = static_cast<gui::StickerPreset>(std::abs(static_cast<int>(widget.scalePreset)) % gui::kStickerPresetCount);
    const auto shapeClass = gui::getStickerShapeClass(preset);
    const bool hasSub = !widget.subtitle.empty();
    const bool isPaperTape = (shapeClass == gui::StickerShapeClass::SHAPE_PAPER_TAPE_RECT ||
                              preset == gui::StickerPreset::EmbossedDymoStrip);
    const bool isWidePlaque = (preset == gui::StickerPreset::FragileFlightStamp ||
                               preset == gui::StickerPreset::AdvisoryExplicit ||
                               preset == gui::StickerPreset::SubBassSeismicZone ||
                               preset == gui::StickerPreset::CalCertified0Vu ||
                               preset == gui::StickerPreset::TapeOpPrintIt);

    const float basePt = (fontSizePreset == 0) ? 10.5f : ((fontSizePreset == 2) ? 13.5f : 12.0f);
    const auto mTitle = gui::Graphics::measureTextMetrics(widget.title, basePt * 1.08f, true, fontFamily);
    const auto mSub   = gui::Graphics::measureTextMetrics(widget.subtitle, basePt * 0.85f, true, fontFamily);

    const float maxLineWidth    = std::max(mTitle.x, mSub.x);
    const float totalTextHeight = mTitle.y + (hasSub ? (mSub.y + 4.0f) : 0.0f);

    float minWidth  = 90.0f;
    float minHeight = hasSub ? 46.0f : 36.0f;
    if (!isPaperTape && !isWidePlaque) {
        minWidth  = 90.0f;
        minHeight = (!widget.title.empty() || !widget.subtitle.empty()) ? 96.0f : 76.0f;
    }

    if (clampOnlyForResize) {
        widget.w = std::max({ widget.w, minWidth, maxLineWidth + 24.0f });
        widget.h = std::max({ widget.h, minHeight, totalTextHeight + 26.0f });
        return;
    }

    float typeDefaultW = 96.0f;
    float typeDefaultH = 110.0f;
    if (preset == gui::StickerPreset::ScribbleMaskingTape || preset == gui::StickerPreset::FinalMixV12Wav) {
        typeDefaultW = 168.0f;
        typeDefaultH = 58.0f;
    } else if (preset == gui::StickerPreset::BlackGaffTape || preset == gui::StickerPreset::EmbossedDymoStrip) {
        typeDefaultW = hasSub ? 168.0f : 156.0f;
        typeDefaultH = hasSub ? 58.0f  : 38.0f;
    } else if (preset == gui::StickerPreset::FixItInTheMix) {
        typeDefaultW = 168.0f;
        typeDefaultH = hasSub ? 58.0f : 48.0f;
    } else if (isWidePlaque) {
        typeDefaultW = 148.0f;
        typeDefaultH = 68.0f;
    }

    widget.w = std::max({ widget.w, typeDefaultW, minWidth, maxLineWidth + 32.0f });
    widget.h = std::max({ widget.h, typeDefaultH, minHeight, totalTextHeight + 26.0f });
    if (!isPaperTape && !isWidePlaque && (!widget.title.empty() || !widget.subtitle.empty())) {
        widget.h = std::max(widget.h, 76.0f + std::max(26.0f, totalTextHeight + 10.0f) + 8.0f);
    }
}

/**
 * @brief Returns true if a control widget (`targetIdx`) is physically taped down by an overlapping
 * Gaff Tape (`BlackGaffTape`), Masking Tape (`ScribbleMaskingTape`), Hazard Tape (`FixItInTheMix`),
 * Master Spine Tape (`FinalMixV12Wav`), or Dymo Label strip on the faceplate.
 */
[[nodiscard]] inline bool isWidgetLockedByTape(
    const FaceplateLayout& layout,
    size_t targetIdx,
    std::string* outTapeLabel = nullptr) noexcept {
    if (targetIdx >= layout.widgets.size()) return false;
    const auto& target = layout.widgets[targetIdx];
    if (!isKnobWidget(target.type) && !isSwitchWidget(target.type) &&
        target.type != UiWidgetType::VerticalStudioFader &&
        target.type != UiWidgetType::HorizontalCrossfader) {
        return false;
    }

    const float kCx = target.x + target.w * 0.5f;
    const float kCy = target.y + target.h * 0.5f;
    const float rx  = target.w * 0.28f;
    const float ry  = target.h * 0.28f;
    const std::array<gui::Point<float>, 9> samplePts = {{
        { kCx, kCy },
        { kCx - rx, kCy }, { kCx + rx, kCy },
        { kCx, kCy - ry }, { kCx, kCy + ry },
        { kCx - rx * 0.7f, kCy - ry * 0.7f },
        { kCx + rx * 0.7f, kCy - ry * 0.7f },
        { kCx - rx * 0.7f, kCy + ry * 0.7f },
        { kCx + rx * 0.7f, kCy + ry * 0.7f }
    }};

    for (size_t i = 0; i < layout.widgets.size(); ++i) {
        if (i == targetIdx) continue;
        const auto& sticker = layout.widgets[i];
        if (sticker.type != UiWidgetType::VintageSticker) continue;

        const auto preset = static_cast<gui::StickerPreset>(std::abs(static_cast<int>(sticker.scalePreset)) % gui::kStickerPresetCount);
        const bool isTapePreset = (gui::getStickerShapeClass(preset) == gui::StickerShapeClass::SHAPE_PAPER_TAPE_RECT ||
                                   preset == gui::StickerPreset::EmbossedDymoStrip);
        if (!isTapePreset) continue;

        const float tCx  = sticker.x + sticker.w * 0.5f;
        const float tCy  = sticker.y + sticker.h * 0.5f;
        const float rad  = -sticker.rotationDeg * 0.0174532925f;
        const float cosA = std::cos(rad);
        const float sinA = std::sin(rad);

        bool overlaps = false;
        for (const auto& pt : samplePts) {
            const float dx = pt.x - tCx;
            const float dy = pt.y - tCy;
            const float lx = tCx + dx * cosA - dy * sinA;
            const float ly = tCy + dx * sinA + dy * cosA;
            if (lx >= sticker.x + 2.0f && lx <= sticker.x + sticker.w - 2.0f &&
                ly >= sticker.y + 2.0f && ly <= sticker.y + sticker.h - 2.0f) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) {
            if (outTapeLabel) {
                *outTapeLabel = sticker.title.empty() ? "GAFF TAPE LOCK" : sticker.title;
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief Builds a completely clean, blank faceplate canvas while preserving the user's chosen global theme/style scheme.
 */
inline FaceplateLayout buildBlankFaceplate(
    const std::string& title = "CUSTOM PLUGIN",
    gui::ChassisFinish finish = gui::ChassisFinish::AnodizedTeal,
    gui::SurfaceGrain grain = gui::SurfaceGrain::VerticalBrushed,
    gui::ChassisHardwareStyle hwStyle = gui::ChassisHardwareStyle::CornerScrews4,
    const FaceplateTheme* preserveScheme = nullptr) {
    FaceplateLayout layout;
    if (preserveScheme != nullptr) {
        layout.theme = *preserveScheme;
    }
    layout.theme.finish         = finish;
    layout.theme.grain          = grain;
    layout.theme.hardwareStyle  = hwStyle;
    layout.theme.cornerScrews   = (hwStyle != gui::ChassisHardwareStyle::CleanScrewless);
    layout.theme.showTopToolbar = true;
    layout.theme.brandName      = "AVRA AUDIO";
    layout.theme.pluginTitle    = title;
    layout.theme.subtitleText   = "CUSTOM FACEPLATE";
    return layout;
}

/**
 * @brief Builds the clean generic AVRA GAIN demo utility faceplate layout
 * using a symmetrical 3-bay studio rack architecture (Input Gain | Metering & Badge | Master Output).
 */
inline FaceplateLayout buildAvraGainDemoFaceplate(const std::string& title = "AVRA GAIN") {
    FaceplateLayout layout;
    layout.theme.finish            = gui::ChassisFinish::AnodizedTeal;
    layout.theme.grain             = gui::SurfaceGrain::VerticalBrushed;
    layout.theme.cornerScrews      = true;
    layout.theme.showTopToolbar    = true;
    layout.theme.globalKnobType    = UiWidgetType::PointerKnob;
    layout.theme.globalSwitchType  = UiWidgetType::ChromeToggleSwitch;
    layout.theme.globalMeterType   = UiWidgetType::AnalogVuMeter;
    layout.theme.globalAccentColor = UiAccentColor::WarmAmber;
    layout.theme.brandName         = "AVRA AUDIO";
    layout.theme.pluginTitle       = title;
    layout.theme.subtitleText      = "CUSTOM CHASSIS";
    layout.theme.badgeFooterText   = "CLASS-A EFFECTS";

    // 1. Symmetrical 3-Bay Background Section Frames inside x = 34..966 (34..334 | 350..670 | 686..966)
    layout.widgets.push_back({
        UiWidgetType::SectionFrame, 34.0f, 66.0f, 300.0f, 468.0f,
        "INPUT  ·  GAIN STAGE", "", UiScalePreset::LinearDb, -1, 0, 0.5f
    });
    layout.widgets.push_back({
        UiWidgetType::SectionFrame, 350.0f, 66.0f, 320.0f, 468.0f,
        "PRECISION METERING", "", UiScalePreset::LinearDb, -1, 0, 0.5f
    });
    layout.widgets.push_back({
        UiWidgetType::SectionFrame, 686.0f, 66.0f, 280.0f, 468.0f,
        "MASTER OUTPUT", "", UiScalePreset::LinearDb, -1, 0, 0.5f
    });

    // 2. Left Bay (x: 34..334, center = 184): Large Input Gain Knob + Phase & Mute Switches
    layout.widgets.push_back({
        UiWidgetType::PointerKnob, 104.0f, 118.0f, 160.0f, 172.0f,
        "INPUT GAIN", "0.0 dB", UiScalePreset::MasterVolumeDb, 0, 0, 0.50f,
        UiAccentColor::WarmAmber, 2, UiKnobRole::InputDrive
    });
    layout.widgets.push_back({
        UiWidgetType::ChromeToggleSwitch, 72.0f, 336.0f, 100.0f, 112.0f,
        "PHASE", "NORM|180", UiScalePreset::LinearDb, -1, 3, 1.0f
    });
    layout.widgets.push_back({
        UiWidgetType::ChromeToggleSwitch, 194.0f, 336.0f, 100.0f, 112.0f,
        "MUTE", "ACTIVE|MUTED", UiScalePreset::LinearDb, -1, 3, 1.0f
    });

    // 3. Center Bay (x: 350..670, center = 510): Backlit VU Meter + In/Out Bargraph + Riveted Nameplate
    layout.widgets.push_back({
        UiWidgetType::AnalogVuMeter, 376.0f, 102.0f, 268.0f, 142.0f,
        "GAIN REDUCTION dB", "AVRA AUDIO", UiScalePreset::LinearDb, 0, 0, 0.25f
    });
    layout.widgets.push_back({
        UiWidgetType::DigitalTrimBargraph, 376.0f, 268.0f, 268.0f, 72.0f,
        "IN / OUT", "DELTA dB", UiScalePreset::LinearDb, -1, 0, 0.50f
    });
    layout.widgets.push_back({
        UiWidgetType::RivetedNameplate, 396.0f, 368.0f, 228.0f, 112.0f,
        title, "CLASS A GAIN STAGE", UiScalePreset::LinearDb, -1, 0, 0.5f
    });

    // 4. Right Bay (x: 686..966, center = 826): Large Master Output Knob + Balance, Topology, Bypass & Studio Audition
    layout.widgets.push_back({
        UiWidgetType::PointerKnob, 750.0f, 106.0f, 152.0f, 158.0f,
        "MASTER OUTPUT", "0.0 dB", UiScalePreset::MasterVolumeDb, -1, 1, 0.50f,
        UiAccentColor::WarmAmber, 2, UiKnobRole::MasterVolume
    });
    layout.widgets.push_back({
        UiWidgetType::PointerKnob, 692.0f, 300.0f, 112.0f, 120.0f,
        "STEREO BAL", "CENTER", UiScalePreset::StereoPanLCR, -1, 2, 0.50f,
        UiAccentColor::WarmAmber, 2, UiKnobRole::ParallelMix
    });
    layout.widgets.push_back({
        UiWidgetType::ChromeToggleSwitch, 812.0f, 318.0f, 72.0f, 48.0f,
        "STEREO MODE", "STEREO|MONO (PURE)", UiScalePreset::LinearDb, -1, 4, 1.0f
    });
    layout.widgets.push_back({
        UiWidgetType::ChromeToggleSwitch, 888.0f, 318.0f, 72.0f, 48.0f,
        "", "IN|BYP", UiScalePreset::LinearDb, -1, 3, 1.0f
    });
    layout.widgets.push_back({
        UiWidgetType::IlluminatedPushButton, 812.0f, 392.0f, 72.0f, 24.0f,
        "AUTO-LEVEL", "", UiScalePreset::LinearDb, -1, 5, 0.0f,
        UiAccentColor::WarmAmber, 2
    });
    layout.widgets.push_back({
        UiWidgetType::IlluminatedPushButton, 888.0f, 392.0f, 72.0f, 24.0f,
        "Δ DELTA", "", UiScalePreset::LinearDb, -1, 6, 0.0f,
        UiAccentColor::NeonCyan, 2
    });

    return layout;
}

inline void validateAndEnrichModuleUI(FaceplateLayout& layout, const std::vector<SchematicNode>& nodes = {});

/**
 * @brief Returns true ONLY when the module is a genuine Vacuum Tube circuit (never on Passive/Inductor/Gyrator EQs,
 * Solid-State FET/VCA/Diode compressors, or Transformers).
 */
[[nodiscard]] inline bool isVacuumTubeStage(const SchematicComponentMeta& meta) noexcept {
    if ((getComponentTraitTags(meta.type) & TAG_TUBE) != 0u) {
        return true;
    }
    if (meta.category == SchematicCategory::EqAndFilters ||
        meta.designator.rfind("EQ", 0) == 0 ||
        meta.designator.rfind("SP", 0) == 0 ||
        meta.designator.rfind("IR", 0) == 0) {
        return false;
    }
    switch (meta.type) {
        case SchematicComponentType::Tube12AX7:
        case SchematicComponentType::PentodeEF86:
        case SchematicComponentType::DirectHeatedTriode:
        case SchematicComponentType::VariMuTubeCompressor:
        case SchematicComponentType::OptoCompressorT4B:
        case SchematicComponentType::OneInch15IpsTubeTape:
        case SchematicComponentType::HighVoltageCascodeTube:
        case SchematicComponentType::SubMiniatureTubeMicPre:
        case SchematicComponentType::SuperTube6H30PVariMu:
            return true;
        default:
            break;
    }
    const auto hasToken = [&](std::string_view needle) noexcept -> bool {
        return meta.fullName.find(needle) != std::string::npos ||
               meta.shortName.find(needle) != std::string::npos;
    };
    return hasToken("Tube") || hasToken("Valve") || hasToken("Triode") ||
           hasToken("Pentode") || hasToken("12AX7") || hasToken("6386") ||
           hasToken("6AS6") || hasToken("EF86") || hasToken("EL84") ||
           hasToken("Vari-Mu");
}

/**
 * @brief Returns the hardware-accurate valve designation label below the oval GlowingTubeWindow for any tube stage.
 */
[[nodiscard]] inline std::string getVacuumTubeWindowLabel(const SchematicComponentMeta& meta) {
    if (meta.type == SchematicComponentType::VariMuTubeCompressor ||
        meta.type == SchematicComponentType::DirectHeatedTriode ||
        meta.designator == "CP3" || meta.designator == "PR6") {
        return "6386 VALVE";
    }
    if (meta.type == SchematicComponentType::OptoCompressorT4B || meta.designator == "CP2") {
        return "12AX7 / 12BH7";
    }
    if (meta.type == SchematicComponentType::PentodeEF86 || meta.designator == "PR5") {
        return "6AS6 PENTODE";
    }
    if (meta.type == SchematicComponentType::Tube12AX7 || meta.designator == "PR4") {
        return "12AX7 TRIODE";
    }
    const auto hasValve = [&](std::string_view v) noexcept -> bool {
        return meta.fullName.find(v) != std::string::npos ||
               meta.shortName.find(v) != std::string::npos ||
               meta.schematicNotes.find(v) != std::string::npos;
    };
    if (hasValve("EF86"))  return "EF86 VALVE";
    if (hasValve("6SN7"))  return "6SN7 VALVE";
    if (hasValve("EL84"))  return "EL84 VALVE";
    if (hasValve("12AU7")) return "12AU7 VALVE";
    if (hasValve("ECC83")) return "ECC83 VALVE";
    if (hasValve("6H30P")) return "6H30P VALVE";
    if (hasValve("6386"))  return "6386 VALVE";
    if (hasValve("6AS6"))  return "6AS6 PENTODE";
    return "12AX7 VALVE";
}

/**
 * @brief Resolves the Col 2 Topology Feature Widget for a wide stage bay (`bayW >= 600.0f`).
 */
inline void getStageTopologyFeatureScopeSpec(
    const SchematicComponentMeta& meta,
    UiWidgetType& outType,
    std::string& outTitle,
    std::string& outSubtitle,
    UiAccentColor fallbackAccent,
    UiAccentColor& outAccent) {
    if (isVacuumTubeStage(meta)) {
        outType     = UiWidgetType::GlowingTubeWindow;
        outTitle    = getVacuumTubeWindowLabel(meta);
        outSubtitle = "CATHODE GLOW";
        outAccent   = UiAccentColor::WarmAmber;
    } else if (meta.category == SchematicCategory::AnalogDynamics ||
               meta.category == SchematicCategory::AutoAndSmart) {
        outType     = UiWidgetType::PastelShaderCurveMeter;
        outTitle    = "TRANSFER CURVE";
        outSubtitle = "KNEE / RATIO";
        outAccent   = fallbackAccent;
    } else if (meta.category == SchematicCategory::EqAndFilters) {
        outType     = UiWidgetType::PastelShaderCurveMeter;
        outTitle    = "EQ CURVE / TILT";
        outSubtitle = "SPECTRAL TILT";
        outAccent   = fallbackAccent;
    } else {
        outType     = UiWidgetType::CrtOscilloscopeScreen;
        outTitle    = "STAGE WAVE / PHASE";
        outSubtitle = "OSCILLOSCOPE";
        outAccent   = fallbackAccent;
    }
}

using SchematicNodeSpec = SchematicNode;

/**
 * @brief Returns true if a badge line-1 string is empty or matches legacy stage-count placeholders.
 */
[[nodiscard]] inline bool isLegacyGenericBadgeSub1(std::string_view s) noexcept {
    if (s.empty()) return true;
    std::string up(s);
    for (char& ch : up) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return up.find("STAGE CHASSIS") != std::string::npos ||
           up.find("STAGE HARDWARE TOPOLOGY") != std::string::npos ||
           up == "CUSTOM HARDWARE TOPOLOGY";
}

/**
 * @brief Returns true if a badge footer string is empty or matches legacy generic placeholders.
 */
[[nodiscard]] inline bool isLegacyGenericBadgeFooter(std::string_view s) noexcept {
    if (s.empty()) return true;
    std::string up(s);
    for (char& ch : up) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return up == "CUSTOM HARDWARE FOUNDRY" ||
           up == "CUSTOM HARDWARE CHASSIS" ||
           up == "STEREO UTILITY DEMO" ||
           up == "CLASS-A PREAMP & LINE STAGE" ||
           up == "VACUUM TUBE SATURATION UNIT" ||
           up == "MAGNETIC TAPE HYSTERESIS UNIT" ||
           up == "STUDIO TRANSFORMER & IRON STAGE" ||
           up == "PRECISION ANALOG EQUALIZER" ||
           up == "DISCRETE ANALOG COMPRESSOR" ||
           up == "ACOUSTIC REVERB TANK ENGINE" ||
           up == "ANALOG TAPE ECHO & DELAY" ||
           up == "ANALOG MODULATION & ROUTING" ||
           up.find("PRECISION STEREO") != std::string::npos;
}

/**
 * @brief Derives an authentic hardware topology descriptor for the Engraved/Riveted Nameplate bottom line
 * directly from the active schematic chain (`schematicChain_`).
 */
[[nodiscard]] inline std::string buildChassisTopologySubtitle(
    const std::vector<SchematicNode>& chain,
    bool isPureMono = true) {
    (void)isPureMono;
    if (chain.empty()) {
        return "CUSTOM HARDWARE CHASSIS";
    }

    bool hasPreamp        = false;
    bool hasTube          = false;
    bool hasIronOrTape    = false;
    bool hasEq            = false;
    bool hasPassiveEq     = false;
    bool hasDynamics      = false;
    bool hasTubeOrOptoDyn = false;
    bool hasDelay         = false;
    bool hasReverb        = false;
    bool hasModulation    = false;

    for (const auto& n : chain) {
        const auto& meta = getComponentMeta(n.type);
        if (isVacuumTubeStage(meta)) {
            hasTube = true;
        }
        switch (meta.category) {
            case SchematicCategory::PreampsAndTubes:
                hasPreamp = true;
                break;
            case SchematicCategory::IronAndTape:
                hasIronOrTape = true;
                break;
            case SchematicCategory::EqAndFilters:
                hasEq = true;
                if (meta.fullName.find("Passive") != std::string::npos ||
                    meta.shortName.find("Passive") != std::string::npos ||
                    n.type == SchematicComponentType::PassivePultecEq ||
                    n.type == SchematicComponentType::ParallelPassiveLcEq) {
                    hasPassiveEq = true;
                }
                break;
            case SchematicCategory::AnalogDynamics:
            case SchematicCategory::AutoAndSmart:
                hasDynamics = true;
                if (isVacuumTubeStage(meta) ||
                    n.type == SchematicComponentType::OptoCompressorT4B ||
                    n.type == SchematicComponentType::VariMuTubeCompressor ||
                    n.type == SchematicComponentType::DualOptoVcaCompressor ||
                    meta.fullName.find("Opto") != std::string::npos ||
                    meta.fullName.find("T4B") != std::string::npos ||
                    meta.fullName.find("Vari-Mu") != std::string::npos) {
                    hasTubeOrOptoDyn = true;
                }
                break;
            case SchematicCategory::DelayAndReverb:
                if (n.type == SchematicComponentType::PlateReverb ||
                    n.type == SchematicComponentType::HallReverbFDN ||
                    n.type == SchematicComponentType::OctaveShimmerReverb ||
                    meta.designator.starts_with("RV") ||
                    meta.designator == "SH1" ||
                    meta.fullName.find("Reverb") != std::string::npos ||
                    meta.fullName.find("Plate") != std::string::npos ||
                    meta.fullName.find("Hall") != std::string::npos ||
                    meta.fullName.find("Chamber") != std::string::npos ||
                    meta.fullName.find("Spring") != std::string::npos) {
                    hasReverb = true;
                } else {
                    hasDelay = true;
                }
                break;
            case SchematicCategory::ModulationAndRoute:
                hasModulation = true;
                break;
        }
    }

    // 0. 3-Stage Class-A Channel (PR2 -> EQ4 -> CP4)
    if (chain.size() == 3 &&
        chain[0].type == SchematicComponentType::ClassAPreamp &&
        chain[1].type == SchematicComponentType::InductorEq3Band &&
        chain[2].type == SchematicComponentType::DiodeBridgeCompressor) {
        return "PREAMP · EQ · DIODE BRIDGE DYNAMICS";
    }
    // 1. Preamp/Tube + EQ + Dynamics (e.g., TubeFetChannel, FetGyratorStrip)
    if ((hasPreamp || hasTube) && hasEq && hasDynamics) {
        return hasTube ? "TUBE · EQ · DYNAMICS CHANNEL" : "CLASS-A · EQ · COMP CHANNEL";
    }
    // 2. Delay + Reverb + Modulation (e.g., EchoPlateChorus)
    if (hasDelay && hasReverb && hasModulation) {
        return "TAPE · PLATE · CHORUS ENGINE";
    }
    // 3. EQ + Vari-Mu / Opto Dynamics
    if (hasEq && hasTubeOrOptoDyn && !hasDelay && !hasReverb) {
        return hasPassiveEq ? "PASSIVE EQ · TUBE DYNAMICS" : "TUBE EQ · OPTICAL DYNAMICS";
    }

    auto stageShortTag = [](const SchematicNode& n) -> std::string {
        const auto& m = getComponentMeta(n.type);
        switch (m.category) {
            case SchematicCategory::PreampsAndTubes:
                return isVacuumTubeStage(m) ? "VACUUM TUBE" : "CLASS-A PREAMP";
            case SchematicCategory::IronAndTape:
                return (m.designator.starts_with("TP") || m.fullName.find("Tape") != std::string::npos)
                    ? "MASTERING TAPE" : "IRON TRANSFORMER";
            case SchematicCategory::EqAndFilters:
                return (m.fullName.find("Passive") != std::string::npos) ? "PASSIVE EQ" : "ANALOG EQ";
            case SchematicCategory::AnalogDynamics:
                return isVacuumTubeStage(m) ? "TUBE COMPRESSOR" : "DISCRETE COMPRESSOR";
            case SchematicCategory::DelayAndReverb:
                return m.designator.starts_with("RV") ? "STUDIO REVERB" : "TAPE ECHO";
            case SchematicCategory::ModulationAndRoute:
            default:
                return "MODULATION MATRIX";
        }
    };

    if (chain.size() == 1) {
        const auto& m = getComponentMeta(chain[0].type);
        switch (m.category) {
            case SchematicCategory::PreampsAndTubes:
                return isVacuumTubeStage(m) ? "VACUUM TUBE SATURATION UNIT" : "CLASS-A PREAMP & LINE STAGE";
            case SchematicCategory::IronAndTape:
                return (m.designator.starts_with("TP") || m.fullName.find("Tape") != std::string::npos)
                    ? "MAGNETIC TAPE HYSTERESIS UNIT" : "STUDIO TRANSFORMER & IRON STAGE";
            case SchematicCategory::EqAndFilters:
                return "PRECISION ANALOG EQUALIZER";
            case SchematicCategory::AnalogDynamics:
                return "DISCRETE ANALOG COMPRESSOR";
            case SchematicCategory::DelayAndReverb:
                return m.designator.starts_with("RV") ? "ACOUSTIC REVERB TANK ENGINE" : "ANALOG TAPE ECHO & DELAY";
            case SchematicCategory::ModulationAndRoute:
            default:
                return "ANALOG MODULATION & ROUTING";
        }
    }

    if (chain.size() == 2) {
        if (hasPreamp && hasIronOrTape) {
            return hasTube ? "VACUUM TUBE & IRON STAGE" : "CLASS-A PREAMP & IRON STAGE";
        }
        const std::string t0 = stageShortTag(chain[0]);
        const std::string t1 = stageShortTag(chain[1]);
        if (t0 == t1) {
            return std::format("DUAL {} STAGE", t0);
        }
        return std::format("{} · {}", t0, t1);
    }

    // 3+ mixed stages: summarize active categories or return "CUSTOM ANALOG SIGNAL CHAIN"
    std::vector<std::string> catTags;
    if (hasPreamp || hasTube) catTags.push_back(hasTube ? "TUBE" : "CLASS-A");
    if (hasIronOrTape)        catTags.push_back("IRON");
    if (hasEq)                catTags.push_back("EQ");
    if (hasDynamics)          catTags.push_back("DYNAMICS");
    if (hasDelay)             catTags.push_back("DELAY");
    if (hasReverb)            catTags.push_back("REVERB");
    if (hasModulation)        catTags.push_back("MOD");

    if (catTags.size() >= 2 && catTags.size() <= 3) {
        std::string joined;
        for (size_t i = 0; i < catTags.size(); ++i) {
            if (i > 0) joined += " · ";
            joined += catTags[i];
        }
        return joined + " CHAIN";
    }
    return "CUSTOM ANALOG SIGNAL CHAIN";
}

[[nodiscard]] inline UiWidgetType resolveWidgetTypeForParam(
    const ComponentParamSpec& pSpec,
    const SchematicComponentMeta& meta,
    UiWidgetType primaryKnob,
    UiWidgetType switchType,
    bool useHeritage) noexcept {
    switch (pSpec.controlKind) {
        case HardwareControlKind::SteppedRotarySwitch:
            if (useHeritage && meta.type == SchematicComponentType::ClassAPreamp &&
                (pSpec.name.find("Gain") != std::string::npos || pSpec.name.find("GAIN") != std::string::npos)) {
                return UiWidgetType::MarconiRedKnob;
            }
            return UiWidgetType::SteppedAttenuatorKnob;
        case HardwareControlKind::ConcentricDualKnob:
            return UiWidgetType::ConcentricEqKnob;
        case HardwareControlKind::VerticalFader:
        case HardwareControlKind::LeverSlider:
            return UiWidgetType::VerticalStudioFader;
        case HardwareControlKind::ToggleSwitch2Way:
        case HardwareControlKind::ToggleSwitch3Way:
            return switchType;
        case HardwareControlKind::PushButtonLatching:
        case HardwareControlKind::PushButtonBank:
            return (switchType != UiWidgetType::ChromeToggleSwitch)
                ? switchType
                : UiWidgetType::IlluminatedPushButton;
        case HardwareControlKind::ContinuousKnob:
        default:
            return primaryKnob;
    }
}

[[nodiscard]] inline std::string formatWidgetSubtitleForParam(const ComponentParamSpec& pSpec, float val) {
    if (pSpec.controlKind == HardwareControlKind::ToggleSwitch2Way ||
        pSpec.controlKind == HardwareControlKind::ToggleSwitch3Way ||
        pSpec.controlKind == HardwareControlKind::PushButtonLatching ||
        pSpec.controlKind == HardwareControlKind::PushButtonBank) {
        if (pSpec.stepLabels.size() == 2) {
            // In HardwareControls.h, topLabel is active when isOn == true (previewNorm > 0.5f, index 1)
            // and botLabel is active when isOn == false (previewNorm <= 0.5f, index 0).
            return pSpec.stepLabels[1] + "|" + pSpec.stepLabels[0];
        }
        if (!pSpec.stepLabels.empty()) {
            std::string joined;
            for (size_t i = 0; i < pSpec.stepLabels.size(); ++i) {
                if (i > 0) joined += "|";
                joined += pSpec.stepLabels[i];
            }
            return joined;
        }
        return "ON|OFF";
    }
    if (!pSpec.stepLabels.empty()) {
        const float norm = computeParamNorm(pSpec, val);
        const int maxIdx = static_cast<int>(pSpec.stepLabels.size()) - 1;
        const int idx    = std::clamp(static_cast<int>(std::lround(norm * static_cast<float>(maxIdx))), 0, maxIdx);
        const std::string& lbl = pSpec.stepLabels[static_cast<size_t>(idx)];
        if (pSpec.unit.empty() || lbl == "OFF" || lbl == "OUT" || lbl == "AUTO" ||
            lbl == "A1" || lbl == "A2" || lbl.find(pSpec.unit) != std::string::npos) {
            return lbl;
        }
        return std::format("{} {}", lbl, pSpec.unit);
    }
    if (pSpec.unit.empty()) {
        return std::format("{:.1f}", val);
    }
    if (pSpec.unit == "Hz") {
        if (std::abs(val) >= 10000.0f) {
            return std::format("{:.1f} kHz", val * 0.001f);
        }
        if (std::abs(val) >= 1000.0f) {
            return std::format("{:.2f} kHz", val * 0.001f);
        }
        if (std::abs(val) >= 100.0f && pSpec.maxVal > 50.0f) {
            return std::format("{:.0f} Hz", val);
        }
        if (std::abs(val) >= 50.0f && pSpec.maxVal > 50.0f) {
            return std::format("{:.1f} Hz", val);
        }
        return std::format("{:.2f} Hz", val);
    }
    if (pSpec.unit == "ms") {
        if (std::abs(val) >= 1000.0f) {
            return std::format("{:.2f} s", val * 0.001f);
        }
        if (std::abs(val) >= 100.0f) {
            return std::format("{:.0f} ms", val);
        }
        return std::format("{:.1f} ms", val);
    }
    if (pSpec.unit == "%") {
        return std::format("{:.0f} %", val);
    }
    return std::format("{:.1f} {}", val, pSpec.unit);
}

/**
 * @brief Dynamically generates a complete, symbiotic hardware faceplate layout directly from the active Schematic Builder nodes,
 * automatically populating ALL available parameter controls (`0 .. meta.getParamCount() - 1`) for every stage with adaptive
 * knob sizing, balanced centered rows, bottom switch strip, and deduplicated Parallel Mix controls.
 */
inline FaceplateLayout buildFaceplateFromSchematicWithTheme(
    const std::string& pluginName,
    const std::vector<SchematicNode>& nodes,
    const FaceplateTheme& baseTheme) {
    if (nodes.empty()) {
        return buildBlankFaceplate(pluginName, baseTheme.finish, baseTheme.grain, baseTheme.hardwareStyle, &baseTheme);
    }

    FaceplateLayout layout;
    layout.theme                = baseTheme;
    layout.theme.cornerScrews   = (baseTheme.hardwareStyle != gui::ChassisHardwareStyle::CleanScrewless);
    layout.theme.showTopToolbar = true;
    if (layout.theme.brandName.empty()) {
        layout.theme.brandName = "AVRA AUDIO";
    }
    layout.theme.pluginTitle    = pluginName;
    if (isLegacyGenericBadgeSub1(layout.theme.subtitleText)) {
        layout.theme.subtitleText = "CUSTOM CHASSIS";
    }
    if (isLegacyGenericBadgeFooter(layout.theme.badgeFooterText)) {
        layout.theme.badgeFooterText = "CLASS-A EFFECTS";
    }

    const UiWidgetType  knobType   = layout.theme.globalKnobType;
    const UiWidgetType  switchType = layout.theme.globalSwitchType;
    const UiWidgetType  meterType  = layout.theme.globalMeterType;
    const UiAccentColor accentCol  = layout.theme.globalAccentColor;
    const int           fontPreset = layout.theme.globalFontSizePreset;

    const size_t totalStages = nodes.size();

    // 1. Right Master & Metering Section (x: 746..986, y: 58..530, width: 240, height: 472, centerX = 866, leftColCenterX = 812, rightColCenterX = 920)
    layout.widgets.push_back({
        UiWidgetType::SectionFrame, 746.0f, 58.0f, 240.0f, 472.0f,
        "METERING  ·  OUTPUT", "", UiScalePreset::LinearDb, -1, 0, 0.5f, accentCol, fontPreset
    });

    if (meterType == UiWidgetType::AnalogVuMeter) {
        layout.widgets.push_back({
            UiWidgetType::AnalogVuMeter, 776.0f, 88.0f, 180.0f, 88.0f,
            "GAIN REDUCTION dB", "AVRA AUDIO", UiScalePreset::LinearDb, -1, 111, 0.35f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::RoundVintageVuMeter || meterType == UiWidgetType::SubmarinePortholeGauge || meterType == UiWidgetType::HellfireHornedVuMeter) {
        layout.widgets.push_back({
            meterType, 776.0f, 88.0f, 180.0f, 88.0f,
            "OUTPUT / GR", "VU METER", UiScalePreset::LinearDb, -1, 111, 0.35f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::LedBargraphMeter) {
        layout.widgets.push_back({
            UiWidgetType::LedBargraphMeter, 776.0f, 88.0f, 180.0f, 88.0f,
            "PEAK / GR  dB", "20-LED BAR", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::GlowingTubeWindow) {
        layout.widgets.push_back({
            UiWidgetType::GlowingTubeWindow, 776.0f, 88.0f, 180.0f, 88.0f,
            "12AX7 PLATE", "CATHODE GLOW", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::CrtOscilloscopeScreen) {
        layout.widgets.push_back({
            UiWidgetType::CrtOscilloscopeScreen, 776.0f, 88.0f, 180.0f, 88.0f,
            "CRT WAVEFORM", "OSCILLOSCOPE", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::StereoGoniometerScope) {
        layout.widgets.push_back({
            UiWidgetType::StereoGoniometerScope, 776.0f, 88.0f, 180.0f, 88.0f,
            "STEREO PHASE", "LISSAJOUS VECTOR", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::NixieTubeDisplay) {
        layout.widgets.push_back({
            UiWidgetType::NixieTubeDisplay, 776.0f, 88.0f, 180.0f, 88.0f,
            "NIXIE TELEMETRY", "IN-14 CATHODE", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::LcdParameterReadout) {
        layout.widgets.push_back({
            UiWidgetType::LcdParameterReadout, 776.0f, 88.0f, 180.0f, 88.0f,
            "DIGITAL READOUT", "0.0 dBFS  |  GR 0.0", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::PastelShaderCurveMeter) {
        layout.widgets.push_back({
            UiWidgetType::PastelShaderCurveMeter, 776.0f, 88.0f, 180.0f, 88.0f,
            "SPECTRAL SHADER", "DYNAMIC TRANSFER", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    } else if (meterType == UiWidgetType::CyberLuxeMultiBarMeter ||
               meterType == UiWidgetType::NeumorphicDualArcMeter ||
               meterType == UiWidgetType::ReaperScytheArchMeter ||
               meterType == UiWidgetType::ApocalypseGeigerVuMeter ||
               meterType == UiWidgetType::PunkAnarchyVuMeter ||
               meterType == UiWidgetType::NatureElementalVuMeter) {
        layout.widgets.push_back({
            meterType, 776.0f, 88.0f, 180.0f, 88.0f,
            "GAIN REDUCTION dB", "AVRA AUDIO", UiScalePreset::LinearDb, -1, 111, 0.35f, accentCol, fontPreset
        });
    } else {
        layout.widgets.push_back({
            UiWidgetType::DigitalTrimBargraph, 776.0f, 88.0f, 180.0f, 88.0f,
            "IN / OUT", "DELTA dB", UiScalePreset::LinearDb, -1, 111, 0.65f, accentCol, fontPreset
        });
    }

    // Row 1 Knobs (y = 222.0f, knobSize = 58.0f): OUTPUT TRIM (centerX = 812.0f) & MASTER MIX (centerX = 920.0f)
    layout.widgets.push_back({
        knobType, 783.0f, 222.0f, 58.0f, 58.0f,
        "OUTPUT TRIM", "0.0 dB", UiScalePreset::MasterVolumeDb, -1, 1, 0.50f, accentCol, fontPreset, UiKnobRole::MasterVolume
    });
    layout.widgets.push_back({
        knobType, 891.0f, 222.0f, 58.0f, 58.0f,
        "MASTER MIX", "100 %", UiScalePreset::MixDryWet, -1, 2, 1.0f, accentCol, fontPreset, UiKnobRole::ParallelMix
    });

    // Row 2 Toggle Switches (y = 352.0f, w = 88.0f, h = 54.0f): STEREO MODE (centerX = 812.0f) & MASTER BYP (centerX = 920.0f)
    layout.widgets.push_back({
        switchType, 768.0f, 352.0f, 88.0f, 54.0f,
        "STEREO MODE", "STEREO|MONO (PURE)", UiScalePreset::LinearDb, -1, 4, 1.0f, accentCol, fontPreset
    });
    layout.widgets.push_back({
        switchType, 876.0f, 352.0f, 88.0f, 54.0f,
        "MASTER BYP", "IN|BYP", UiScalePreset::LinearDb, -1, 3, 1.0f, accentCol, fontPreset
    });

    // Bottom RivetedNameplate (x = 776.0f, y = 438.0f, w = 180.0f, h = 76.0f, centerX = 866.0f)
    layout.widgets.push_back({
        UiWidgetType::RivetedNameplate, 776.0f, 438.0f, 180.0f, 76.0f,
        pluginName,
        std::format("{}|{}", layout.theme.subtitleText, layout.theme.badgeFooterText),
        static_cast<UiScalePreset>(static_cast<int>(layout.theme.globalBadgeStyle)), -1, 0, 0.5f, accentCol, fontPreset
    });
    (void)totalStages;

    auto populateStageBayControls = [&](size_t stageIdx, float bx, float by, float bayW, float bayH) {
        (void)by;
        (void)bayH;
        const auto& st   = nodes[stageIdx];
        const auto& meta = getComponentMeta(st.type);
        const auto  plum = getAutoPlumbingSpec(st.type);
        const auto  bp   = getDefaultUIBlueprint(st.type);
        const size_t paramCount = meta.getParamCount();

        const bool useHeritage          = !layout.theme.unifyFaceplateTheme;
        const UiWidgetType primaryKnob  = useHeritage ? bp.primaryKnobStyle : knobType;
        const UiAccentColor stageAccent = useHeritage ? bp.heritageAccent  : accentCol;

        const auto hasKeywordLower = [](std::string s, std::string_view kw) -> bool {
            for (char& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            return s.find(kw) != std::string::npos;
        };

        std::vector<int> faderSlots;
        std::vector<int> rotarySlots;
        std::vector<int> switchSlots;
        std::vector<int> meterSlots;
        bool moduleAlreadyHasMixParam = false;

        for (int p = 0; p < static_cast<int>(paramCount); ++p) {
            const auto& ps = meta.getParamSpec(p);
            if (hasKeywordLower(ps.name, "mix") || hasKeywordLower(ps.name, "blend") ||
                hasKeywordLower(ps.name, "dry") || hasKeywordLower(ps.name, "wet") ||
                hasKeywordLower(ps.name, "echo volume")) {
                moduleAlreadyHasMixParam = true;
            }
            if (ps.controlKind == HardwareControlKind::IndicatorMeter) {
                meterSlots.push_back(p);
                continue;
            }
            if (ps.controlKind == HardwareControlKind::ToggleSwitch2Way ||
                ps.controlKind == HardwareControlKind::ToggleSwitch3Way ||
                ps.controlKind == HardwareControlKind::PushButtonLatching ||
                ps.controlKind == HardwareControlKind::PushButtonBank) {
                switchSlots.push_back(p);
            } else if (ps.controlKind == HardwareControlKind::VerticalFader ||
                       ps.controlKind == HardwareControlKind::LeverSlider) {
                faderSlots.push_back(p);
            } else {
                rotarySlots.push_back(p);
            }
        }
        if (rotarySlots.empty() && faderSlots.empty() && !switchSlots.empty()) {
            rotarySlots.push_back(switchSlots.front());
            switchSlots.erase(switchSlots.begin());
        }

        // Hard Rule on LINK IO: NEVER render LINK IO unless BOTH an Input/Drive/Gain knob AND an Output/Makeup/Trim knob exist!
        bool hasInputKnob  = false;
        bool hasOutputKnob = false;
        for (int p : rotarySlots) {
            const std::string& pName = meta.getParamSpec(p).name;
            if (p == 0 || hasKeywordLower(pName, "in") || hasKeywordLower(pName, "drive") ||
                hasKeywordLower(pName, "gain") || hasKeywordLower(pName, "trim") ||
                hasKeywordLower(pName, "thresh")) {
                hasInputKnob = true;
            }
            if (p > 0 && (hasKeywordLower(pName, "out") || hasKeywordLower(pName, "makeup") ||
                          hasKeywordLower(pName, "make-up") || hasKeywordLower(pName, "trim") ||
                          hasKeywordLower(pName, "level"))) {
                hasOutputKnob = true;
            }
        }
        bool hasNativeLinkSwitch = false;
        for (int p : switchSlots) {
            if (hasKeywordLower(meta.getParamSpec(p).name, "link")) {
                hasNativeLinkSwitch = true;
            }
        }
        // Graphic-EQ / lever-bank stages (EQ7, EQ14, ST3, RV10 ...) and stages with native link switches never get an auto LINK IO.
        const bool allowLinkIo = (plum.supportsAutoGainLink && hasInputKnob && hasOutputKnob && faderSlots.empty() && !hasNativeLinkSwitch);
        const int trim1Slot = 105;
        const int trim2Slot = 106;
        const int linkSlot  = 108;
        const int mixSlot   = 109;
        const int inSlot    = 110;

        const auto applyKnobTickLabels = [&](UiCanvasWidget& wItem, const ComponentParamSpec& pSpec) {
            auto freqTicks = buildContinuousFreqTickLabels(pSpec, st.type);
            if (!freqTicks.empty()) {
                wItem.customTickLabels = std::move(freqTicks);
            }
        };

        struct PendingRotaryItem {
            UiWidgetType  widgetType;
            std::string   title;
            std::string   subtitle;
            UiScalePreset scalePreset;
            int           boundSlot;
            float         normVal;
            UiAccentColor accent;
            UiKnobRole    role;
            std::vector<std::string> customTicks;
        };

        std::vector<PendingRotaryItem> rotaryItems;
        rotaryItems.reserve(rotarySlots.size() + 3);
        for (int p : rotarySlots) {
            const auto& pSpec = meta.getParamSpec(p);
            const float pVal  = st.getParamVal(p);
            const UiWidgetType kStyle = resolveWidgetTypeForParam(pSpec, meta, primaryKnob, switchType, useHeritage);
            UiScalePreset pScale = inferScalePresetForParam(pSpec, st.type);
            if (pSpec.name == "Low Freq" && st.type == SchematicComponentType::InductorEq3Band) {
                pScale = UiScalePreset::LowFreqHz;
            }
            std::vector<std::string> ticks = buildContinuousFreqTickLabels(pSpec, st.type);
            rotaryItems.push_back({
                kStyle,
                pSpec.name,
                formatWidgetSubtitleForParam(pSpec, pVal),
                pScale,
                p,
                computeParamNorm(pSpec, pVal),
                stageAccent,
                UiKnobRole::StageOrCustom,
                std::move(ticks)
            });
        }

        if (st.exposeInternalTrims) {
            for (int tIdx = 0; tIdx < 2; ++tIdx) {
                const auto& tSpec = (tIdx == 0) ? plum.internalTrim1 : plum.internalTrim2;
                const float tVal  = (tIdx == 0) ? st.trim1Val : st.trim2Val;
                const int   tSlot = (tIdx == 0) ? trim1Slot : trim2Slot;
                rotaryItems.push_back({
                    useHeritage ? UiWidgetType::GoldDomeKnob : primaryKnob,
                    tSpec.name,
                    std::format("{:.1f} {}", tVal, tSpec.unit),
                    (tIdx == 0) ? UiScalePreset::LinearDb : UiScalePreset::PercentDryWet,
                    tSlot,
                    computeParamNorm(tSpec, tVal),
                    useHeritage ? UiAccentColor::RoyalGold : stageAccent,
                    UiKnobRole::StageOrCustom,
                    {}
                });
            }
        }

        // Every parallel component in the chain exposes a dedicated PARALLEL LEVEL knob
        // on the plugin faceplate UI so the user can balance each parallel branch directly from the plugin.
        if (st.isParallel) {
            rotaryItems.push_back({
                primaryKnob,
                "PARALLEL LEVEL",
                std::format("{:.0f} %", st.parallelMixPercent),
                UiScalePreset::MixDryWet,
                mixSlot,
                st.parallelMixPercent * 0.01f,
                stageAccent,
                UiKnobRole::ParallelMix,
                {}
            });
        }

        const float stageCenterX = bx + bayW * 0.5f;
        {
            // Vertical Fader Bank (e.g. EQ7 10-Band, EQ14 7-Band, ST3 4-Fader, RV10 4-Lever, AU2 3-Fader + 3-Knob):
            // Proportionally partition horizontal width between the Fader Bank (left) and Rotary Controls (right),
            // and never stack 3 rotary knobs vertically (maximum 2 rows at y = 160.0f and y = 306.0f).
            if (!faderSlots.empty()) {
                const size_t numFaders = faderSlots.size();
                const size_t numRot    = rotaryItems.size();
                constexpr float faderY = 146.0f;
                constexpr float faderH = 256.0f;
                const float faderWNominal = (numFaders <= 6) ? 74.0f : 48.0f;

                float bankLeft  = 0.0f;
                float bankWidth = 0.0f;
                float rotLeft   = 0.0f;
                float rotRight  = bx + bayW - 20.0f;
                if (numRot == 0) {
                    bankWidth = std::min(652.0f, static_cast<float>(numFaders) * ((numFaders <= 6) ? 92.0f : 62.0f));
                    bankLeft  = stageCenterX - bankWidth * 0.5f;
                } else if (numFaders >= 6) {
                    bankLeft  = bx + 18.0f;
                    bankWidth = (numRot == 1) ? 530.0f : 468.0f;
                    rotLeft   = bankLeft + bankWidth + 14.0f;
                } else {
                    const float faderPitch = (numFaders <= 2) ? 96.0f : ((numFaders == 3) ? 90.0f : ((numRot >= 5) ? 74.0f : 82.0f));
                    bankLeft  = bx + ((numFaders >= 4 && numRot >= 5) ? 22.0f : 26.0f);
                    bankWidth = static_cast<float>(numFaders) * faderPitch;
                    rotLeft   = bankLeft + bankWidth + 20.0f;
                }
                const float faderCellW = bankWidth / static_cast<float>(numFaders);
                const float faderW     = std::min(faderWNominal, faderCellW - 4.0f);

                for (size_t fi = 0; fi < numFaders; ++fi) {
                    const int p       = faderSlots[fi];
                    const auto& pSpec = meta.getParamSpec(p);
                    const float pVal  = st.getParamVal(p);
                    const float fcx   = bankLeft + (static_cast<float>(fi) + 0.5f) * faderCellW;
                    UiCanvasWidget fw = {
                        UiWidgetType::VerticalStudioFader,
                        fcx - faderW * 0.5f, faderY, faderW, faderH,
                        pSpec.name,
                        formatWidgetSubtitleForParam(pSpec, pVal),
                        inferScalePresetForParam(pSpec, st.type),
                        static_cast<int>(stageIdx), p,
                        computeParamNorm(pSpec, pVal),
                        stageAccent, fontPreset
                    };
                    fw.customTickLabels = buildContinuousFreqTickLabels(pSpec, st.type);
                    layout.widgets.push_back(std::move(fw));
                }

                if (numRot > 0) {
                    const float rotSpan    = std::max(110.0f, rotRight - rotLeft);
                    const float rotCenterX = 0.5f * (rotLeft + rotRight);
                    auto emitFaderRotaryRow = [&](size_t startIdx, size_t count, float rowY, float knobSz, float cellCapW) {
                        if (count == 0) return;
                        const float usableW = std::min(rotSpan, static_cast<float>(count) * cellCapW);
                        const float cellW   = usableW / static_cast<float>(count);
                        const float rowLeft = rotCenterX - usableW * 0.5f;
                        for (size_t c = 0; c < count; ++c) {
                            const auto& item = rotaryItems[startIdx + c];
                            const float rcx  = rowLeft + (static_cast<float>(c) + 0.5f) * cellW;
                            UiCanvasWidget wItem{
                                item.widgetType,
                                rcx - knobSz * 0.5f, rowY, knobSz, knobSz,
                                item.title, item.subtitle, item.scalePreset,
                                static_cast<int>(stageIdx), item.boundSlot,
                                item.normVal, item.accent, fontPreset, item.role
                            };
                            wItem.customTickLabels = item.customTicks;
                            layout.widgets.push_back(std::move(wItem));
                        }
                    };

                    if (numRot == 1) {
                        emitFaderRotaryRow(0, 1, 228.0f, 74.0f, 160.0f);
                    } else if (numRot == 2 && rotSpan >= 230.0f) {
                        emitFaderRotaryRow(0, 2, 228.0f, 74.0f, 160.0f);
                    } else {
                        const size_t topCols  = (numRot + 1) / 2;
                        const size_t botCols  = numRot / 2;
                        const float  knobSz   = (topCols <= 2 && rotSpan >= 240.0f) ? 68.0f : ((topCols <= 3) ? 60.0f : 54.0f);
                        const float  cellCapW = (topCols <= 2) ? 160.0f : 124.0f;
                        emitFaderRotaryRow(0,       topCols, 160.0f, knobSz, cellCapW);
                        emitFaderRotaryRow(topCols, botCols, 306.0f, knobSz, cellCapW);
                    }
                }
            } else {
                // Adaptive Knob Diameter, Row Count & Horizontal/Vertical Centering (K Total Knobs)
                const size_t K = rotaryItems.size();
                auto emitCenteredKnobRow = [&](size_t startIdx, size_t count, float rowY, float knobSz, float maxUsableW, float cellCapW, int rowFontPreset) {
                    if (count == 0) return;
                    const float usableW = std::min(maxUsableW, static_cast<float>(count) * cellCapW);
                    const float cellW   = usableW / static_cast<float>(count);
                    const float rowLeft = stageCenterX - usableW * 0.5f;
                    for (size_t c = 0; c < count; ++c) {
                        const auto& item = rotaryItems[startIdx + c];
                        const float cx   = rowLeft + (static_cast<float>(c) + 0.5f) * cellW;
                        UiCanvasWidget wItem{
                            item.widgetType,
                            cx - knobSz * 0.5f, rowY, knobSz, knobSz,
                            item.title, item.subtitle, item.scalePreset,
                            static_cast<int>(stageIdx), item.boundSlot,
                            item.normVal, item.accent, rowFontPreset, item.role
                        };
                        wItem.customTickLabels = item.customTicks;
                        layout.widgets.push_back(std::move(wItem));
                    }
                };

                if (K <= 4) {
                    // Case A: 1 <= K <= 4 Knobs (Single Centered Row)
                    const float knobSz = (K <= 3) ? 88.0f : 78.0f;
                    emitCenteredKnobRow(0, K, 228.0f, knobSz, 640.0f, 165.0f, fontPreset);
                } else if (K <= 8) {
                    // Case B: 5 <= K <= 8 Knobs (2 Balanced Rows: topCols = (K + 1) / 2, botCols = K / 2)
                    const size_t topCols = (K + 1) / 2;
                    const size_t botCols = K / 2;
                    emitCenteredKnobRow(0,       topCols, 160.0f, 68.0f, 656.0f, 156.0f, fontPreset);
                    emitCenteredKnobRow(topCols, botCols, 306.0f, 68.0f, 656.0f, 156.0f, fontPreset);
                } else if (K <= 14) {
                    // Case C: 9 <= K <= 14 Knobs (2 Balanced Rows: topCols = (K + 1) / 2, botCols = K / 2 — NEVER wrap onto a 3rd row!)
                    const size_t topCols = (K + 1) / 2;
                    const size_t botCols = K / 2;
                    const float  knobSz  = (K <= 12) ? 56.0f : 52.0f;
                    emitCenteredKnobRow(0,       topCols, 162.0f, knobSz, 664.0f, 108.0f, fontPreset);
                    emitCenteredKnobRow(topCols, botCols, 308.0f, knobSz, 664.0f, 108.0f, fontPreset);
                } else {
                    // Case D: K >= 15 Knobs (3 Balanced Rows)
                    const size_t r1Cols = (K + 2) / 3;
                    const size_t r2Cols = (K - r1Cols + 1) / 2;
                    const size_t r3Cols = K - r1Cols - r2Cols;
                    const int compactFont = 0;
                    emitCenteredKnobRow(0,               r1Cols, 152.0f, 46.0f, 664.0f, 96.0f, compactFont);
                    emitCenteredKnobRow(r1Cols,          r2Cols, 254.0f, 46.0f, 664.0f, 96.0f, compactFont);
                    emitCenteredKnobRow(r1Cols + r2Cols, r3Cols, 356.0f, 46.0f, 664.0f, 96.0f, compactFont);
                }
            }

            // Bottom Switch & Button Strip (y = 448.0f .. 514.0f, centered at swCenterY = 478.0f)
            // Distribute all stage switches/buttons, explicit IndicatorMeter (if any), optional LINK IO, and rightmost STAGE IN (IN|BYP)
            struct BottomStripEntry {
                UiWidgetType  type;
                float         w;
                float         h;
                std::string   title;
                std::string   subtitle;
                UiScalePreset scale;
                int           slot;
                float         norm;
                UiAccentColor accent;
            };
            std::vector<BottomStripEntry> bottomItems;
            for (int mp : meterSlots) {
                const auto& mSpec = meta.getParamSpec(mp);
                bottomItems.push_back({
                    UiWidgetType::DigitalTrimBargraph, 108.0f, 44.0f,
                    mSpec.name, mSpec.unit.empty() ? "METER" : mSpec.unit,
                    UiScalePreset::LinearDb, mp, 0.5f, stageAccent
                });
            }
            for (int sp : switchSlots) {
                const auto& pSpec = meta.getParamSpec(sp);
                const float pVal  = st.getParamVal(sp);
                const UiWidgetType swStyle = resolveWidgetTypeForParam(pSpec, meta, primaryKnob, switchType, useHeritage);
                bottomItems.push_back({
                    swStyle,
                    92.0f,
                    56.0f,
                    pSpec.name,
                    formatWidgetSubtitleForParam(pSpec, pVal),
                    inferScalePresetForParam(pSpec, st.type),
                    sp,
                    computeParamNorm(pSpec, pVal),
                    stageAccent
                });
            }
            if (allowLinkIo) {
                bottomItems.push_back({
                    switchType, 92.0f, 56.0f,
                    "LINK IO", "LINK|FREE", UiScalePreset::LinearDb,
                    linkSlot, st.autoGainLinked ? 1.0f : 0.0f, stageAccent
                });
            }
            bottomItems.push_back({
                switchType, 92.0f, 56.0f,
                "STAGE IN", "IN|BYP", UiScalePreset::LinearDb,
                inSlot, st.isBypassed ? 0.0f : 1.0f, stageAccent
            });

            constexpr float swCenterY = 478.0f;
            const size_t totalSw = bottomItems.size();
            const float stripLeft  = bx + 18.0f;
            const float stripRight = bx + bayW - 18.0f;
            const float stripSpan  = std::max(120.0f, stripRight - stripLeft);

            if (totalSw == 1) {
                // Single STAGE IN switch: anchor cleanly at the right end of the bottom strip
                const auto& item = bottomItems.front();
                const float cx   = stripRight - item.w * 0.5f - 12.0f;
                layout.widgets.push_back({
                    item.type, cx - item.w * 0.5f, swCenterY - item.h * 0.5f, item.w, item.h,
                    item.title, item.subtitle, item.scale,
                    static_cast<int>(stageIdx), item.slot, item.norm, item.accent, fontPreset
                });
            } else {
                const float cellW = stripSpan / static_cast<float>(totalSw);
                for (size_t si = 0; si < totalSw; ++si) {
                    const auto& item = bottomItems[si];
                    const float wClamped = std::min(item.w, std::max(58.0f, cellW - 6.0f));
                    const float cx = stripLeft + (static_cast<float>(si) + 0.5f) * cellW;
                    layout.widgets.push_back({
                        item.type, cx - wClamped * 0.5f, swCenterY - item.h * 0.5f, wClamped, item.h,
                        item.title, item.subtitle, item.scale,
                        static_cast<int>(stageIdx), item.slot, item.norm, item.accent, fontPreset
                    });
                }
            }
            return;
        }

    };

    // 2. Stage Bays: every stage (1..N) always gets the full-width main control window
    //    (x = 34..730, y = 108..530 when >1 stages, or y = 58..530 when 1 stage).
    const float kStageFrameX = 34.0f;
    const float kStageFrameY = (totalStages > 1) ? 108.0f : 58.0f;
    const float kStageFrameW = 696.0f;
    const float kStageFrameH = (totalStages > 1) ? 422.0f : 472.0f;
    for (size_t i = 0; i < totalStages; ++i) {
        const auto& st   = nodes[i];
        const auto& meta = getComponentMeta(st.type);
        const auto  bp   = getDefaultUIBlueprint(st.type);
        const int heritageFinishSlot = layout.theme.unifyFaceplateTheme ? -1 : static_cast<int>(bp.heritageFinish);

        std::string frameTitle = meta.shortName;
        layout.widgets.push_back({
            UiWidgetType::SectionFrame, kStageFrameX, kStageFrameY, kStageFrameW, kStageFrameH,
            frameTitle,
            "", UiScalePreset::LinearDb, static_cast<int>(i), heritageFinishSlot, 0.5f,
            layout.theme.unifyFaceplateTheme ? accentCol : bp.heritageAccent, fontPreset
        });

        populateStageBayControls(i, kStageFrameX, kStageFrameY, kStageFrameW, kStageFrameH);
    }
    validateAndEnrichModuleUI(layout, nodes);
    return layout;
}

using SchematicNodeSpec = SchematicNode;

inline FaceplateLayout buildFaceplateFromSchematicWithTheme(
    const std::vector<SchematicNode>& nodes,
    const FaceplateTheme& theme) {
    const std::string title = theme.pluginTitle.empty() ? "CUSTOM PLUGIN" : theme.pluginTitle;
    return buildFaceplateFromSchematicWithTheme(title, nodes, theme);
}

/**
 * @brief Returns the default visual telemetry widget archetype for any PrimaryModuleArchetype (A..I).
 */
[[nodiscard]] inline UiWidgetType getDefaultTelemetryForArchetype(PrimaryModuleArchetype arch) noexcept {
    switch (arch) {
        case PrimaryModuleArchetype::A_PreampsBuffersTransformers:
            return UiWidgetType::DigitalTrimBargraph;
        case PrimaryModuleArchetype::B_CompressorsLimitersGainCells:
            return UiWidgetType::AnalogVuMeter;
        case PrimaryModuleArchetype::C_MultiBandParametricEqs:
        case PrimaryModuleArchetype::D_PassiveSpecialtyToneEqs:
            return UiWidgetType::LedBargraphMeter;
        case PrimaryModuleArchetype::E_SpectralSmartProcessors:
        case PrimaryModuleArchetype::F_TransientShapersAndGates:
        case PrimaryModuleArchetype::G_SaturatorsWaveShapersClippers:
            return UiWidgetType::CrtOscilloscopeScreen;
        case PrimaryModuleArchetype::H_DelaysTapeBbdPitchShifters:
            return UiWidgetType::StereoGoniometerScope;
        case PrimaryModuleArchetype::I_ReverbsAndPhysicalSpaces:
            return UiWidgetType::LedBargraphMeter;
    }
    return UiWidgetType::AnalogVuMeter;
}

/**
 * @brief Pre-Flight Validation & Enrichment Gate (`validateAndEnrichModuleUI`).
 */
inline void validateAndEnrichModuleUI(
    FaceplateLayout& layout,
    const std::vector<SchematicNode>& nodes) {
    bool hasTelemetryMeter = false;

    for (auto& w : layout.widgets) {
        if (isMeterWidget(w.type)) {
            hasTelemetryMeter = true;
        }
        if (isKnobOrFaderWidget(w.type)) {
            std::string lowerTitle = w.title;
            for (char& c : lowerTitle) {
                if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            }
            const auto hasWord = [&](std::string_view kw) {
                return lowerTitle.find(kw) != std::string::npos;
            };
            if (hasWord("stereo bal") || lowerTitle == "balance" || lowerTitle == "pan") {
                w.scalePreset = UiScalePreset::StereoPanLCR;
            }
            if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(nodes.size())) {
                const auto& st   = nodes[static_cast<size_t>(w.boundStageIdx)];
                const auto& meta = getComponentMeta(st.type);
                const int pCount = static_cast<int>(meta.getParamCount());
                if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount) {
                    const auto& pSpec = meta.getParamSpec(w.boundParamSlot);
                    w.scalePreset = inferScalePresetForParam(pSpec, st.type);
                    if (w.scalePreset != UiScalePreset::DialOneToTen &&
                        w.scalePreset != UiScalePreset::DialZeroToTen) {
                        auto customTicks = buildContinuousFreqTickLabels(pSpec, st.type);
                        if (!customTicks.empty()) {
                            w.customTickLabels = std::move(customTicks);
                        }
                    }
                }
            }
        }
    }

    // Gate #5: Mandatory Visual Telemetry Check
    if (!hasTelemetryMeter) {
        const PrimaryModuleArchetype arch = nodes.empty()
            ? PrimaryModuleArchetype::B_CompressorsLimitersGainCells
            : classifyPrimaryArchetype(nodes.front().type);
        const UiWidgetType defMeter = getDefaultTelemetryForArchetype(arch);
        layout.widgets.push_back({
            defMeter, 776.0f, 88.0f, 180.0f, 88.0f,
            "OUTPUT / GR  VU", "AVRA AUDIO", UiScalePreset::LinearDb, -1, 111, 0.35f,
            layout.theme.globalAccentColor, layout.theme.globalFontSizePreset
        });
    }

    for (auto& w : layout.widgets) {
        if (w.parameterBindingId.empty()) {
            w.parameterBindingId = computeWidgetParameterBindingId(w);
        }
    }
}

inline FaceplateLayout buildFaceplateFromSchematic(
    const std::string& pluginName,
    const std::vector<SchematicNode>& nodes,
    gui::ChassisFinish finish = gui::ChassisFinish::AnodizedTeal,
    gui::SurfaceGrain grain = gui::SurfaceGrain::VerticalBrushed) {
    FaceplateTheme theme;
    theme.finish = finish;
    theme.grain  = grain;
    return buildFaceplateFromSchematicWithTheme(pluginName, nodes, theme);
}

/**
 * @brief Updates the normalized values and subtitles of widgets bound to Schematic stages
 * without resetting user-customized widget positions or sizes.
 */
inline void syncFaceplateParamValuesFromSchematic(
    FaceplateLayout& layout,
    const std::vector<SchematicNode>& nodes) {
    if (isLegacyGenericBadgeSub1(layout.theme.subtitleText)) {
        layout.theme.subtitleText = "CUSTOM CHASSIS";
    }
    if (isLegacyGenericBadgeFooter(layout.theme.badgeFooterText)) {
        layout.theme.badgeFooterText = "CLASS-A EFFECTS";
    }
    for (auto& w : layout.widgets) {
        if (w.type == UiWidgetType::RivetedNameplate) {
            if (w.title.empty() && !layout.theme.pluginTitle.empty()) {
                w.title = layout.theme.pluginTitle;
            }
            const auto pipePos = w.subtitle.find('|');
            std::string sub1 = (pipePos != std::string::npos) ? w.subtitle.substr(0, pipePos) : w.subtitle;
            std::string sub2 = (pipePos != std::string::npos) ? w.subtitle.substr(pipePos + 1) : layout.theme.badgeFooterText;
            if (isLegacyGenericBadgeSub1(sub1) && sub2 != "PREAMP · EQ · DIODE BRIDGE DYNAMICS") {
                sub1 = "CUSTOM CHASSIS";
            }
            if (isLegacyGenericBadgeFooter(sub2)) {
                sub2 = !isLegacyGenericBadgeFooter(layout.theme.badgeFooterText)
                    ? layout.theme.badgeFooterText
                    : "CLASS-A EFFECTS";
            }
            w.subtitle = std::format("{}|{}", sub1, sub2);
        }
        if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(nodes.size())) {
            const auto& st   = nodes[static_cast<size_t>(w.boundStageIdx)];
            const auto& meta = getComponentMeta(st.type);
            const auto  plum = getAutoPlumbingSpec(st.type);
            const int pCount = static_cast<int>(meta.getParamCount());

            if (isSwitchWidget(w.type) || w.type == UiWidgetType::JewelLed) {
                if (w.title == "LINK IO" || w.boundParamSlot == 108 || (w.boundParamSlot == 8 && w.title.find("LINK") != std::string::npos)) {
                    w.previewNorm = st.autoGainLinked ? 1.0f : 0.0f;
                } else if (w.title == "TOLERANCE" || w.subtitle == "DRIFT|CLEAN" || w.boundParamSlot == 106 ||
                           (w.boundParamSlot == 6 && pCount <= 6) || (w.boundParamSlot == 7 && pCount <= 7)) {
                    w.previewNorm = computeParamNorm(plum.internalTrim2, st.trim2Val) >= 0.25f ? 1.0f : 0.0f;
                } else if (w.title == "STAGE IN" || w.subtitle == "IN|BYP" || w.boundParamSlot == 110 ||
                           (w.boundParamSlot == 10 && pCount <= 10)) {
                    w.previewNorm = st.isBypassed ? 0.0f : 1.0f;
                } else if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount) {
                    const auto& pSpec = meta.getParamSpec(w.boundParamSlot);
                    const float pVal  = st.getParamVal(w.boundParamSlot);
                    w.previewNorm     = computeParamNorm(pSpec, pVal);
                    w.subtitle        = formatWidgetSubtitleForParam(pSpec, pVal);
                } else if (w.title.find("PHASE") == std::string::npos && w.subtitle.find("180") == std::string::npos) {
                    w.previewNorm = st.isBypassed ? 0.0f : 1.0f;
                }
            } else if (isKnobOrFaderWidget(w.type)) {
                if (w.knobRole == UiKnobRole::ParallelMix || w.title == "PARALLEL LEVEL" || w.title == "MIX" || w.title == "PAR MIX" || w.boundParamSlot == 109) {
                    w.previewNorm = st.parallelMixPercent * 0.01f;
                    w.subtitle    = std::format("{:.0f} %", st.parallelMixPercent);
                } else if (st.exposeInternalTrims && (w.title == plum.internalTrim1.name || w.title == plum.internalTrim2.name)) {
                    const bool isT1   = (w.title == plum.internalTrim1.name);
                    const auto& tSpec = isT1 ? plum.internalTrim1 : plum.internalTrim2;
                    const float tVal  = isT1 ? st.trim1Val : st.trim2Val;
                    w.previewNorm     = computeParamNorm(tSpec, tVal);
                    w.subtitle        = std::format("{:.1f} {}", tVal, tSpec.unit);
                } else if (w.boundParamSlot >= 0 && w.boundParamSlot < pCount) {
                    const auto& pSpec = meta.getParamSpec(w.boundParamSlot);
                    const float pVal  = st.getParamVal(w.boundParamSlot);
                    w.previewNorm     = computeParamNorm(pSpec, pVal);
                    w.subtitle        = formatWidgetSubtitleForParam(pSpec, pVal);
                    w.scalePreset     = inferScalePresetForParam(pSpec, st.type);
                    if (w.scalePreset != UiScalePreset::DialOneToTen &&
                        w.scalePreset != UiScalePreset::DialZeroToTen) {
                        auto customTicks = buildContinuousFreqTickLabels(pSpec, st.type);
                        if (!customTicks.empty()) {
                            w.customTickLabels = std::move(customTicks);
                        }
                    }
                } else {
                    w.previewNorm = st.parallelMixPercent * 0.01f;
                    w.subtitle    = std::format("{:.0f} %", st.parallelMixPercent);
                }
            } else if (!layout.isMasterMeterWidget(w) && isMeterWidget(w.type)) {
                if (pCount > 0) {
                    const auto& pSpec = meta.getParamSpec(0);
                    const float pVal  = st.getParamVal(0);
                    w.previewNorm     = computeParamNorm(pSpec, pVal);
                }
            }
        }
    }
}

/**
 * @brief Saves a FaceplateLayout to disk (`plugins/<Name>/<Name>.faceplate`).
 */
inline std::string encodePipeText(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '|') {
            out += "&#124;";
        } else if (c != '\r' && c != '\n') {
            out += c;
        }
    }
    return out;
}

inline std::string decodePipeText(std::string s) {
    std::size_t pos = 0;
    while ((pos = s.find("&#124;", pos)) != std::string::npos) {
        s.replace(pos, 6, "|");
        pos += 1;
    }
    return s;
}

inline std::string serializeFaceplateLayoutToString(const FaceplateLayout& layout) {
    std::ostringstream out;
    out << "# Avra Studio Hardware Faceplate UI Layout v7\n";
    out << "THEME|"
        << static_cast<int>(layout.theme.finish) << "|"
        << static_cast<int>(layout.theme.grain) << "|"
        << (layout.theme.cornerScrews ? 1 : 0) << "|"
        << (layout.theme.showTopToolbar ? 1 : 0) << "|"
        << encodePipeText(layout.theme.brandName) << "|"
        << encodePipeText(layout.theme.pluginTitle) << "|"
        << encodePipeText(layout.theme.subtitleText) << "|"
        << static_cast<int>(layout.theme.hardwareStyle) << "|"
        << layout.theme.gridSnapPx << "|"
        << static_cast<int>(layout.theme.globalKnobType) << "|"
        << static_cast<int>(layout.theme.globalSwitchType) << "|"
        << static_cast<int>(layout.theme.globalMeterType) << "|"
        << static_cast<int>(layout.theme.globalAccentColor) << "|"
        << layout.theme.globalFontSizePreset << "|"
        << static_cast<int>(layout.theme.screwStyle) << "|"
        << (layout.theme.useCustomFinishColor ? 1 : 0) << "|"
        << layout.theme.customFinishHex << "|"
        << encodePipeText(layout.theme.badgeFooterText) << "|"
        << (layout.theme.unifyFaceplateTheme ? 1 : 0) << "|"
        << static_cast<int>(layout.theme.globalFontFamily) << "|"
        << static_cast<int>(layout.theme.globalFontColor) << "|"
        << (layout.theme.useCustomFontColor ? 1 : 0) << "|"
        << layout.theme.customFontColorHex << "|"
        << layout.theme.bevelDepthPx << "|"
        << static_cast<int>(layout.theme.panelBorderStyle) << "|"
        << (layout.theme.useCustomKnobColors ? 1 : 0) << "|"
        << layout.theme.knobBodyHex << "|"
        << layout.theme.knobCapHex << "|"
        << layout.theme.knobIndicatorHex << "|"
        << static_cast<int>(layout.theme.knobTickStyle) << "|"
        << layout.theme.meterFaceplateHex << "|"
        << layout.theme.meterGlowHex << "|"
        << static_cast<int>(layout.theme.labelPlaqueStyle) << "|"
        << layout.theme.styleTemplateIdx << "|"
        << 0 << "|"
        << (layout.theme.isCustomThemeModified ? 1 : 0) << "|"
        << encodePipeText(layout.theme.customThemeName) << "|"
        << static_cast<int>(layout.theme.globalBadgeStyle) << "|"
        << layout.theme.textPlateColorPreset << "|"
        << (layout.theme.useCustomTextPlateColor ? 1 : 0) << "|"
        << layout.theme.customTextPlateColorHex << "\n";

    for (const auto& w : layout.widgets) {
        out << "WIDGET|"
            << static_cast<int>(w.type) << "|"
            << w.x << "|" << w.y << "|" << w.w << "|" << w.h << "|"
            << static_cast<int>(w.scalePreset) << "|"
            << w.boundStageIdx << "|" << w.boundParamSlot << "|"
            << w.previewNorm << "|"
            << encodePipeText(w.title) << "|" << encodePipeText(w.subtitle) << "|"
            << static_cast<int>(w.accentColor) << "|"
            << w.fontSizePreset << "|"
            << static_cast<int>(w.knobRole) << "|"
            << w.macroStageIdx2 << "|"
            << w.macroParamSlot2 << "|"
            << encodePipeText(w.customImagePath) << "|"
            << w.rotationDeg << "|"
            << (w.hasWaxMark ? 1 : 0) << "|"
            << w.waxMarkNorm << "\n";
    }
    return out.str();
}

inline bool saveFaceplateLayoutToFile(const std::filesystem::path& filePath, const FaceplateLayout& layout) {
    std::error_code ec;
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path(), ec);
    }
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << serializeFaceplateLayoutToString(layout);
    return true;
}

inline void parseThemeFromPipeParts(const std::vector<std::string>& parts, FaceplateTheme& theme) {
    if (parts.size() < 8) return;
    theme.finish         = static_cast<gui::ChassisFinish>(std::stoi(parts[1]));
    theme.grain          = static_cast<gui::SurfaceGrain>(std::stoi(parts[2]));
    theme.surfaceFinish  = static_cast<int>(theme.grain);
    theme.cornerScrews   = (std::stoi(parts[3]) != 0);
    theme.showTopToolbar = (std::stoi(parts[4]) != 0);
    const std::string loadedBrand = decodePipeText(parts[5]);
    theme.brandName      = loadedBrand.empty() ? "AVRA AUDIO" : loadedBrand;
    theme.pluginTitle    = decodePipeText(parts[6]);
    theme.subtitleText   = decodePipeText(parts[7]);
    if (parts.size() >= 9) {
        theme.hardwareStyle = static_cast<gui::ChassisHardwareStyle>(std::stoi(parts[8]));
    }
    if (parts.size() >= 10) {
        theme.gridSnapPx = std::stoi(parts[9]);
    }
    if (parts.size() >= 15) {
        theme.globalKnobType       = static_cast<UiWidgetType>(std::stoi(parts[10]));
        theme.globalSwitchType     = static_cast<UiWidgetType>(std::stoi(parts[11]));
        theme.globalMeterType      = static_cast<UiWidgetType>(std::stoi(parts[12]));
        theme.globalAccentColor    = static_cast<UiAccentColor>(std::stoi(parts[13]));
        theme.globalFontSizePreset = std::stoi(parts[14]);
    }
    if (parts.size() >= 19) {
        theme.screwStyle           = static_cast<gui::ScrewHeadStyle>(std::stoi(parts[15]));
        theme.useCustomFinishColor = (std::stoi(parts[16]) != 0);
        theme.customFinishHex      = static_cast<uint32_t>(std::stoul(parts[17]));
        theme.badgeFooterText      = decodePipeText(parts[18]);
    }
    if (parts.size() >= 20) {
        theme.unifyFaceplateTheme  = (std::stoi(parts[19]) != 0);
    }
    if (parts.size() >= 24) {
        theme.globalFontFamily     = static_cast<gui::FontFamily>(std::stoi(parts[20]));
        theme.globalFontColor      = static_cast<UiSkinFontColor>(std::stoi(parts[21]));
        theme.useCustomFontColor   = (std::stoi(parts[22]) != 0);
        theme.customFontColorHex   = static_cast<uint32_t>(std::stoul(parts[23]));
    }
    if (parts.size() >= 34) {
        theme.bevelDepthPx        = std::stof(parts[24]);
        theme.panelBorderStyle    = static_cast<gui::SectionFrameStyle>(std::stoi(parts[25]));
        theme.useCustomKnobColors = (std::stoi(parts[26]) != 0);
        theme.knobBodyHex         = static_cast<uint32_t>(std::stoul(parts[27]));
        theme.knobCapHex          = static_cast<uint32_t>(std::stoul(parts[28]));
        theme.knobIndicatorHex    = static_cast<uint32_t>(std::stoul(parts[29]));
        theme.knobTickStyle       = static_cast<gui::KnobTickMarkStyle>(std::stoi(parts[30]));
        theme.meterFaceplateHex   = static_cast<uint32_t>(std::stoul(parts[31]));
        theme.meterGlowHex        = static_cast<uint32_t>(std::stoul(parts[32]));
        theme.labelPlaqueStyle    = static_cast<gui::UiLabelPlaqueStyle>(std::stoi(parts[33]));
    }
    if (parts.size() >= 35) {
        theme.styleTemplateIdx    = std::clamp(std::stoi(parts[34]), 0, kTotalUiStyleTemplates - 1);
    }
    if (parts.size() >= 36) {
        theme.earsOnlyBlackoutMode = false;
    }
    if (parts.size() >= 39) {
        theme.isCustomThemeModified = (std::stoi(parts[36]) != 0);
        theme.customThemeName       = decodePipeText(parts[37]);
        theme.globalBadgeStyle      = static_cast<gui::BadgeVisualStyle>(std::clamp(std::stoi(parts[38]), 0, 5));
    }
    if (parts.size() >= 42) {
        theme.textPlateColorPreset    = std::clamp(std::stoi(parts[39]), 0, 12);
        theme.useCustomTextPlateColor = (std::stoi(parts[40]) != 0);
        theme.customTextPlateColorHex = static_cast<uint32_t>(std::stoul(parts[41]));
    }
}

/**
 * @brief Loads a `FaceplateLayout` directly from any `.faceplate` or `.avra_ui` file on disk.
 * Preserves 100% of user personalizations (style template, finish, custom colors, widget types, positions, labels, stickers).
 */
inline bool loadFaceplateLayoutFromFile(const std::filesystem::path& filePath, FaceplateLayout& outLayout) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(filePath, ec)) return false;

    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    FaceplateLayout layout;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '|')) {
            parts.push_back(item);
        }
        if (!line.empty() && line.back() == '|') {
            parts.push_back("");
        }
        if (parts.empty()) continue;

        try {
            if (parts[0] == "THEME" && parts.size() >= 8) {
                parseThemeFromPipeParts(parts, layout.theme);
            } else if (parts[0] == "WIDGET" && parts.size() >= 12) {
                // Backward-compatibility: if a legacy v6 file had an unescaped '|' inside `subtitle` (e.g. "NORM|180"),
                // merge `parts[11]` and `parts[12]` back into `subtitle`.
                if (parts.size() == 20) {
                    parts[11] = parts[11] + "|" + parts[12];
                    parts.erase(parts.begin() + 12);
                }
                UiCanvasWidget w;
                w.type           = static_cast<UiWidgetType>(std::stoi(parts[1]));
                w.x              = std::stof(parts[2]);
                w.y              = std::stof(parts[3]);
                w.w              = std::stof(parts[4]);
                w.h              = std::stof(parts[5]);
                w.scalePreset    = static_cast<UiScalePreset>(std::stoi(parts[6]));
                w.boundStageIdx  = std::stoi(parts[7]);
                w.boundParamSlot = std::stoi(parts[8]);
                w.previewNorm    = std::stof(parts[9]);
                w.title          = decodePipeText(parts[10]);
                w.subtitle       = decodePipeText(parts[11]);
                if (parts.size() >= 13) {
                    w.accentColor = static_cast<UiAccentColor>(std::clamp(std::stoi(parts[12]), 0, 11));
                }
                if (parts.size() >= 14) {
                    w.fontSizePreset = std::stoi(parts[13]);
                }
                if (parts.size() >= 15) {
                    w.knobRole = static_cast<UiKnobRole>(std::stoi(parts[14]));
                }
                if (parts.size() >= 17) {
                    w.macroStageIdx2  = std::stoi(parts[15]);
                    w.macroParamSlot2 = std::stoi(parts[16]);
                }
                if (parts.size() >= 18) {
                    w.customImagePath = decodePipeText(parts[17]);
                }
                if (parts.size() >= 19) {
                    w.rotationDeg = std::stof(parts[18]);
                }
                if (parts.size() >= 21) {
                    w.hasWaxMark  = (std::stoi(parts[19]) != 0);
                    w.waxMarkNorm = math::clamp(std::stof(parts[20]), 0.0f, 1.0f);
                }
                if (w.title == "STEREO BAL" || w.title == "BALANCE" || w.title == "PAN") {
                    w.scalePreset = UiScalePreset::StereoPanLCR;
                }
                if (isMeterWidget(w.type) && w.x >= 730.0f) {
                    w.boundStageIdx = -1;
                }
                if (w.boundStageIdx == -1 && (w.boundParamSlot == 4 || w.subtitle == "MONO (PURE)|STEREO" || w.subtitle == "STEREO|MONO (PURE)" || w.title == "MONO (PURE)" || w.title == "STEREO MODE")) {
                    w.title = "STEREO MODE";
                    w.subtitle = "STEREO|MONO (PURE)";
                    w.previewNorm = 1.0f;
                } else if (w.boundStageIdx == -1 && (w.boundParamSlot == 3 || w.subtitle == "IN|BYP" || w.title == "BYPASS" || w.title == "MASTER")) {
                    w.title = "MASTER BYP";
                } else if (w.boundStageIdx >= 0 && !layout.isMasterMeterWidget(w) && w.type == UiWidgetType::GlowingTubeWindow) {
                    w.subtitle = "CATHODE GLOW";
                }
                // Ensure top-row large knobs have >= 14px clearance below y=68 section headers
                if (isKnobWidget(w.type) && w.title == "MASTER OUTPUT" && w.h >= 150.0f) {
                    w.x = 750.0f;
                    w.y = 108.0f;
                    w.w = 152.0f;
                    w.h = 160.0f;
                } else if (isKnobWidget(w.type) && w.h >= 150.0f && w.y >= 98.0f && w.y <= 110.0f) {
                    w.y = 118.0f;
                } else if (isKnobWidget(w.type) && w.title == "STEREO BAL" && w.y >= 290.0f && w.y <= 316.0f) {
                    w.y = 302.0f;
                    w.w = 114.0f;
                    w.h = 122.0f;
                } else if (isSwitchWidget(w.type) && w.h >= 105.0f && w.y >= 324.0f && w.y <= 330.0f) {
                    w.y = 336.0f;
                } else if (isSwitchWidget(w.type) && w.boundStageIdx == -1 && w.x >= 800.0f && w.y >= 285.0f && w.y <= 310.0f) {
                    w.y = 314.0f;
                    w.h = 48.0f;
                } else if (w.type == UiWidgetType::IlluminatedPushButton && w.boundStageIdx == -1 && w.x >= 800.0f && w.y >= 360.0f && w.y <= 372.0f) {
                    w.y = 382.0f;
                }
                layout.widgets.push_back(std::move(w));
            }
        } catch (...) {}
    }

    if (!layout.widgets.empty()) {
        layout.theme.showGrid = false;
        outLayout = std::move(layout);
        return true;
    }
    return false;
}

inline bool saveSavedUserThemesToFile(
    const std::filesystem::path& workspaceRoot,
    const std::vector<SavedUserTheme>& themes) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path filePath = workspaceRoot / "presets" / "user_ui_themes.cfg";
    if (filePath.has_parent_path()) {
        fs::create_directories(filePath.parent_path(), ec);
    }
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << "# Avra Studio Saved User UI Themes\n";
    for (const auto& ut : themes) {
        FaceplateLayout tmp;
        tmp.theme                       = ut.theme;
        tmp.theme.customThemeName       = ut.name;
        tmp.theme.isCustomThemeModified = false;
        out << serializeFaceplateLayoutToString(tmp);
    }
    return true;
}

inline std::vector<SavedUserTheme> loadSavedUserThemesFromFile(
    const std::filesystem::path& workspaceRoot) {
    namespace fs = std::filesystem;
    std::vector<SavedUserTheme> themes;
    std::error_code ec;
    const fs::path filePath = workspaceRoot / "presets" / "user_ui_themes.cfg";
    if (!fs::exists(filePath, ec)) return themes;
    std::ifstream in(filePath);
    if (!in.is_open()) return themes;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '|')) {
            parts.push_back(item);
        }
        if (!line.empty() && line.back() == '|') {
            parts.push_back("");
        }
        if (parts.empty()) continue;

        try {
            if (parts[0] == "THEME" && parts.size() >= 8) {
                SavedUserTheme ut;
                parseThemeFromPipeParts(parts, ut.theme);
                ut.name = ut.theme.customThemeName.empty() ? "USER THEME" : ut.theme.customThemeName;
                ut.theme.customThemeName       = ut.name;
                ut.theme.isCustomThemeModified = false;
                themes.push_back(std::move(ut));
            }
        } catch (...) {}
    }
    return themes;
}

/**
 * @brief Auto-migrates existing / saved `.avra` or `.faceplate` projects with old widget coordinates
 * (`SectionFrame` at `y < 118.0f` or `w < 500.0f`, or stage controls at `y < 130.0f`) when `chain.size() > 3`.
 * Preserves `layout.theme`, `layout.activeStageIdx`, all user-placed decorative widgets (`VintageSticker`,
 * `SilkscreenLabel`, `VentilationGrille`, `RackHandlePair`, `PatchbayJackSocket`), and custom widget styles/wax-marks,
 * while snapping all stage `SectionFrame`s and controls into the full-width `x = 18.0f, y = 124.0f, w = 726.0f, h = 412.0f`
 * Main Control Window below the Top Signal-Chain Window (`y = 50.0f .. 106.0f`).
 */
inline bool ensureMultiStageActiveBayLayout(
    FaceplateLayout& layout,
    const std::vector<SchematicNode>& chain) {
    if (isLegacyGenericBadgeSub1(layout.theme.subtitleText)) {
        layout.theme.subtitleText = "CUSTOM CHASSIS";
    }
    if (isLegacyGenericBadgeFooter(layout.theme.badgeFooterText)) {
        layout.theme.badgeFooterText = "CLASS-A EFFECTS";
    }
    // Always keep the bottom-right RivetedNameplate title, stage count, topology descriptor, and METERING · OUTPUT alignment synchronized
    float masterFrameRight = 986.0f;
    bool  masterAlignmentNeedsSnap = false;
    bool  hasMeteringOutputBay = false;
    bool  hasMasterMeter       = false;
    bool  hasOutputTrimKnob    = false;
    bool  hasMasterMixKnob     = false;
    bool  hasMonoStereoSwitch  = false;
    bool  hasMasterBypassSw    = false;
    bool  hasRivetedBadge      = false;
    for (auto& w : layout.widgets) {
        if (w.type == UiWidgetType::SectionFrame && w.boundStageIdx == -1 && w.x >= 730.0f) {
            hasMeteringOutputBay = true;
            masterFrameRight = w.x + w.w;
            if (w.y < 54.0f || w.w < 236.0f ||
                std::abs(w.x - 746.0f) > 0.5f || std::abs(w.y - 58.0f) > 0.5f ||
                std::abs(w.w - 240.0f) > 0.5f || std::abs(w.h - 472.0f) > 0.5f) {
                masterAlignmentNeedsSnap = true;
            }
            w.x = 746.0f;
            w.y = 58.0f;
            w.w = 240.0f;
            w.h = 472.0f;
        }
    }
    for (auto& w : layout.widgets) {
        if (w.boundStageIdx >= 0 && w.boundParamSlot == 9 && w.title == "PAR MIX") {
            w.title = "MIX";
            masterAlignmentNeedsSnap = true;
        }
        if (w.type == UiWidgetType::SectionFrame && w.boundStageIdx >= 0) {
            if (static_cast<size_t>(w.boundStageIdx) < chain.size()) {
                const auto& stMeta = getComponentMeta(chain[static_cast<size_t>(w.boundStageIdx)].type);
                if (w.title.size() >= 3 && w.title[0] == 'S' && std::isdigit(static_cast<unsigned char>(w.title[1]))) {
                    w.title = stMeta.shortName;
                    masterAlignmentNeedsSnap = true;
                }
            }
            const float expectedY = (chain.size() > 1) ? 108.0f : 58.0f;
            const float expectedH = (chain.size() > 1) ? 422.0f : 472.0f;
            if (std::abs(w.x - 34.0f) > 0.5f || std::abs(w.y - expectedY) > 0.5f ||
                std::abs(w.w - 696.0f) > 0.5f || std::abs(w.h - expectedH) > 0.5f) {
                w.x = 34.0f;
                w.y = expectedY;
                w.w = 696.0f;
                w.h = expectedH;
                masterAlignmentNeedsSnap = true;
            }
        } else if (hasMeteringOutputBay && layout.isMasterMeterWidget(w) && w.x >= 730.0f) {
            hasMasterMeter = true;
            w.boundStageIdx = -1;
            float tx = 776.0f, ty = 88.0f, tw = 180.0f, th = 88.0f;
            if (std::abs(w.x - tx) > 0.5f || std::abs(w.y - ty) > 0.5f ||
                std::abs(w.w - tw) > 0.5f || std::abs(w.h - th) > 0.5f) {
                masterAlignmentNeedsSnap = true;
            }
            w.x = tx; w.y = ty; w.w = tw; w.h = th;
        } else if (w.type == UiWidgetType::RivetedNameplate) {
            hasRivetedBadge = true;
            if (w.title.empty() && !layout.theme.pluginTitle.empty()) {
                w.title = layout.theme.pluginTitle;
            }
            const auto pipePos = w.subtitle.find('|');
            std::string sub1 = (pipePos != std::string::npos) ? w.subtitle.substr(0, pipePos) : w.subtitle;
            std::string sub2 = (pipePos != std::string::npos) ? w.subtitle.substr(pipePos + 1) : layout.theme.badgeFooterText;
            if (isLegacyGenericBadgeSub1(sub1) && sub2 != "PREAMP · EQ · DIODE BRIDGE DYNAMICS") {
                sub1 = "CUSTOM CHASSIS";
            }
            if (isLegacyGenericBadgeFooter(sub2)) {
                sub2 = !isLegacyGenericBadgeFooter(layout.theme.badgeFooterText)
                    ? layout.theme.badgeFooterText
                    : "CLASS-A EFFECTS";
            }
            w.subtitle = std::format("{}|{}", sub1, sub2);
            if (hasMeteringOutputBay && w.x >= 730.0f) {
                if (std::abs(w.x - 776.0f) > 0.5f || std::abs(w.y - 438.0f) > 0.5f ||
                    std::abs(w.w - 180.0f) > 0.5f || std::abs(w.h - 76.0f) > 0.5f) {
                    masterAlignmentNeedsSnap = true;
                }
                w.x = 776.0f;
                w.y = 438.0f;
                w.w = 180.0f;
                w.h = 76.0f;
            }
        } else if (hasMeteringOutputBay && w.boundStageIdx == -1 && w.x >= 730.0f &&
                   (w.boundParamSlot == 4 || w.subtitle == "MONO (PURE)|STEREO" || w.subtitle == "STEREO|MONO (PURE)" || w.title == "MONO (PURE)" || w.title == "STEREO MODE")) {
            hasMonoStereoSwitch = true;
            w.title = "STEREO MODE";
            w.subtitle = "STEREO|MONO (PURE)";
            w.type  = layout.theme.globalSwitchType;
            if (std::abs(w.x - 768.0f) > 0.5f || std::abs(w.y - 352.0f) > 0.5f ||
                std::abs(w.w - 88.0f) > 0.5f || std::abs(w.h - 54.0f) > 0.5f) {
                masterAlignmentNeedsSnap = true;
            }
            w.x = 768.0f;
            w.y = 352.0f;
            w.w = 88.0f;
            w.h = 54.0f;
        } else if (hasMeteringOutputBay && w.boundStageIdx == -1 && w.x >= 730.0f &&
                   (w.boundParamSlot == 3 || w.boundParamSlot == 10)) {
            hasMasterBypassSw = true;
            w.title = "MASTER BYP";
            w.type  = layout.theme.globalSwitchType;
            if (w.x + w.w > masterFrameRight - 8.0f ||
                std::abs(w.x - 876.0f) > 0.5f || std::abs(w.y - 352.0f) > 0.5f ||
                std::abs(w.w - 88.0f) > 0.5f || std::abs(w.h - 54.0f) > 0.5f) {
                masterAlignmentNeedsSnap = true;
            }
            w.x = 876.0f;
            w.y = 352.0f;
            w.w = 88.0f;
            w.h = 54.0f;
        } else if (hasMeteringOutputBay && w.boundStageIdx == -1 && w.x >= 730.0f &&
                   w.boundParamSlot == 1 && isKnobWidget(w.type)) {
            hasOutputTrimKnob = true;
            if (std::abs(w.x - 783.0f) > 0.5f || std::abs(w.y - 222.0f) > 0.5f ||
                std::abs(w.w - 58.0f) > 0.5f || std::abs(w.h - 58.0f) > 0.5f) {
                masterAlignmentNeedsSnap = true;
            }
            w.x = 783.0f;
            w.y = 222.0f;
            w.w = 58.0f;
            w.h = 58.0f;
        } else if (hasMeteringOutputBay && w.boundStageIdx == -1 && w.x >= 730.0f &&
                   (w.boundParamSlot == 2 || w.boundParamSlot == 9 ||
                    w.title == "MASTER MIX" || w.title == "DRY / WET")) {
            hasMasterMixKnob = true;
            w.boundParamSlot = 2;
            w.knobRole       = UiKnobRole::ParallelMix;
            if (std::abs(w.x - 891.0f) > 0.5f || std::abs(w.y - 222.0f) > 0.5f ||
                std::abs(w.w - 58.0f) > 0.5f || std::abs(w.h - 58.0f) > 0.5f) {
                masterAlignmentNeedsSnap = true;
            }
            w.x = 891.0f;
            w.y = 222.0f;
            w.w = 58.0f;
            w.h = 58.0f;
        }
    }

    if (hasMeteringOutputBay) {
        const size_t beforeCount = layout.widgets.size();
        std::erase_if(layout.widgets, [](const UiCanvasWidget& w) {
            return w.boundStageIdx == -1 && w.x >= 730.0f &&
                   (w.boundParamSlot == 5 || w.boundParamSlot == 6 ||
                    w.title == "AUTO-LEVEL" || w.title == "Δ DELTA" ||
                    w.title.find("DELTA") != std::string::npos);
        });
        if (layout.widgets.size() != beforeCount) {
            masterAlignmentNeedsSnap = true;
        }
    }

    // Self-heal any missing permanent widget in the METERING · OUTPUT bay (including RivetedNameplate badge)
    if (hasMeteringOutputBay) {
        if (!hasMasterMeter) {
            UiCanvasWidget m{
                layout.theme.globalMeterType, 776.0f, 88.0f, 180.0f, 88.0f,
                "GAIN REDUCTION dB", "AVRA AUDIO", UiScalePreset::LinearDb, -1, 111, 0.35f,
                layout.theme.globalAccentColor, layout.theme.globalFontSizePreset
            };
            m.parameterBindingId = computeWidgetParameterBindingId(m);
            layout.widgets.push_back(std::move(m));
            masterAlignmentNeedsSnap = true;
        }
        if (!hasOutputTrimKnob) {
            UiCanvasWidget outKnob{
                layout.theme.globalKnobType, 783.0f, 222.0f, 58.0f, 58.0f,
                "OUTPUT TRIM", "0.0 dB", UiScalePreset::MasterVolumeDb, -1, 1, 0.50f,
                layout.theme.globalAccentColor, layout.theme.globalFontSizePreset, UiKnobRole::MasterVolume
            };
            outKnob.parameterBindingId = computeWidgetParameterBindingId(outKnob);
            layout.widgets.push_back(std::move(outKnob));
            masterAlignmentNeedsSnap = true;
        }
        if (!hasMasterMixKnob) {
            UiCanvasWidget mixKnob{
                layout.theme.globalKnobType, 891.0f, 222.0f, 58.0f, 58.0f,
                "MASTER MIX", "100 %", UiScalePreset::MixDryWet, -1, 2, 1.0f,
                layout.theme.globalAccentColor, layout.theme.globalFontSizePreset, UiKnobRole::ParallelMix
            };
            mixKnob.parameterBindingId = computeWidgetParameterBindingId(mixKnob);
            auto it = layout.widgets.begin();
            for (; it != layout.widgets.end(); ++it) {
                if (it->boundStageIdx == -1 && it->boundParamSlot == 1 && isKnobWidget(it->type)) {
                    mixKnob.type           = it->type;
                    mixKnob.accentColor    = it->accentColor;
                    mixKnob.fontSizePreset = it->fontSizePreset;
                    ++it;
                    break;
                }
            }
            layout.widgets.insert(it, std::move(mixKnob));
            masterAlignmentNeedsSnap = true;
        }
        if (!hasMonoStereoSwitch) {
            UiCanvasWidget monoSw{
                layout.theme.globalSwitchType, 768.0f, 352.0f, 88.0f, 54.0f,
                "STEREO MODE", "STEREO|MONO (PURE)", UiScalePreset::LinearDb, -1, 4, 1.0f,
                layout.theme.globalAccentColor, layout.theme.globalFontSizePreset
            };
            monoSw.parameterBindingId = computeWidgetParameterBindingId(monoSw);
            layout.widgets.push_back(std::move(monoSw));
            masterAlignmentNeedsSnap = true;
        }
        if (!hasMasterBypassSw) {
            UiCanvasWidget bypSw{
                layout.theme.globalSwitchType, 876.0f, 352.0f, 88.0f, 54.0f,
                "MASTER BYP", "IN|BYP", UiScalePreset::LinearDb, -1, 3, 1.0f,
                layout.theme.globalAccentColor, layout.theme.globalFontSizePreset
            };
            bypSw.parameterBindingId = computeWidgetParameterBindingId(bypSw);
            layout.widgets.push_back(std::move(bypSw));
            masterAlignmentNeedsSnap = true;
        }
        if (!hasRivetedBadge) {
            const std::string bTitle = !layout.theme.pluginTitle.empty() ? layout.theme.pluginTitle : "CUSTOM PLUGIN";
            const std::string bSub1  = !isLegacyGenericBadgeSub1(layout.theme.subtitleText)
                ? layout.theme.subtitleText
                : "CUSTOM CHASSIS";
            const std::string bSub2  = !isLegacyGenericBadgeFooter(layout.theme.badgeFooterText)
                ? layout.theme.badgeFooterText
                : "CLASS-A EFFECTS";
            UiCanvasWidget badge{
                UiWidgetType::RivetedNameplate, 776.0f, 438.0f, 180.0f, 76.0f,
                bTitle,
                std::format("{}|{}", bSub1, bSub2),
                UiScalePreset::LinearDb, -1, 0, 0.5f,
                layout.theme.globalAccentColor, layout.theme.globalFontSizePreset
            };
            badge.parameterBindingId = computeWidgetParameterBindingId(badge);
            layout.widgets.push_back(std::move(badge));
            masterAlignmentNeedsSnap = true;
        }
    }

    if (chain.empty()) {
        return masterAlignmentNeedsSnap;
    }

    bool needsMigration = masterAlignmentNeedsSnap;
    std::vector<int>   stageKnobCounts(chain.size(), 0);
    std::vector<int>   stageWidgetCounts(chain.size(), 0);
    std::vector<float> stageFirstKnobW(chain.size(), -1.0f);
    std::vector<float> stageFirstKnobH(chain.size(), -1.0f);
    std::vector<float> stageFrameWidths(chain.size(), 0.0f);
    std::vector<bool>  stageHasParallelMixKnob(chain.size(), false);

    for (const auto& w : layout.widgets) {
        if (w.type == UiWidgetType::SectionFrame) {
            if (w.x < 31.0f || (w.x + w.w) > 990.0f) {
                needsMigration = true;
            }
            if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size())) {
                stageFrameWidths[static_cast<size_t>(w.boundStageIdx)] = w.w;
                if (w.title.find('(') != std::string::npos) {
                    needsMigration = true;
                }
            }
        }
        if (w.boundStageIdx >= 0 && w.boundStageIdx < static_cast<int>(chain.size()) && !layout.isMasterMeterWidget(w)) {
            const size_t sIdx = static_cast<size_t>(w.boundStageIdx);
            if (w.type != UiWidgetType::SectionFrame) {
                ++stageWidgetCounts[sIdx];
            }
            if (w.title == "BUS MODE" || w.subtitle == "SER|PAR" || w.subtitle == "NORM|VOICE" ||
                w.title == "TOLERANCE" || w.subtitle == "DRIFT|CLEAN" || w.boundParamSlot == 106 ||
                w.title == "STAGE WAVE / PHASE" || w.title == "EQ CURVE / TILT" ||
                w.title == "TRANSFER CURVE" || w.subtitle == "CATHODE GLOW") {
                needsMigration = true;
            }
            if (isMeterWidget(w.type)) {
                const auto& meta = getComponentMeta(chain[sIdx].type);
                bool hasIndicatorSpec = false;
                for (size_t pi = 0; pi < meta.getParamCount(); ++pi) {
                    if (meta.getParamSpec(static_cast<int>(pi)).controlKind == HardwareControlKind::IndicatorMeter) {
                        hasIndicatorSpec = true;
                        break;
                    }
                }
                if (!hasIndicatorSpec) {
                    needsMigration = true;
                }
            }
            // Upgrade cached NORM/VOICE toggle switches bound to continuous parameter labels, or older small (< 52px tall) bottom switches
            if (isSwitchWidget(w.type) &&
                (w.h < 52.0f ||
                 w.title.find("TAPE FEEDBACK") != std::string::npos ||
                 w.title.find("CHORUS DEPTH") != std::string::npos ||
                 w.title.find("REVERB WET MIX") != std::string::npos ||
                 w.title.find("PLATE DECAY") != std::string::npos ||
                 w.subtitle.find("NORM|VOICE") != std::string::npos)) {
                needsMigration = true;
            }
            if (isKnobWidget(w.type)) {
                ++stageKnobCounts[sIdx];
                if (w.boundParamSlot == 109) {
                    stageHasParallelMixKnob[sIdx] = true;
                }
                if (stageFirstKnobW[sIdx] < 0.0f) {
                    stageFirstKnobW[sIdx] = w.w;
                    stageFirstKnobH[sIdx] = w.h;
                }
            }
        }
    }

    for (size_t s = 0; s < chain.size(); ++s) {
        const auto& meta = getComponentMeta(chain[s].type);
        const int minExpectedWidgets = static_cast<int>(meta.getParamCount()) + 1 + (chain[s].isParallel ? 1 : 0);
        if (stageWidgetCounts[s] < minExpectedWidgets || stageHasParallelMixKnob[s] != chain[s].isParallel) {
            needsMigration = true;
            break;
        }
    }

    {
        int stageFrameCount = 0;
        bool hasStage0Knobs = false;
        for (const auto& w : layout.widgets) {
            if (w.boundStageIdx >= 0 && !layout.isMasterMeterWidget(w)) {
                if (w.type == UiWidgetType::SectionFrame) {
                    ++stageFrameCount;
                    const float expectedY = (chain.size() > 1) ? 108.0f : 58.0f;
                    const float expectedH = (chain.size() > 1) ? 422.0f : 472.0f;
                    if (std::abs(w.y - expectedY) > 2.0f || std::abs(w.w - 696.0f) > 2.0f || std::abs(w.h - expectedH) > 2.0f || std::abs(w.x - 34.0f) > 2.0f) {
                        needsMigration = true;
                    }
                } else {
                    if (w.y < 130.0f) {
                        needsMigration = true;
                    }
                    if (isKnobWidget(w.type) && w.boundStageIdx < static_cast<int>(chain.size()) &&
                        stageKnobCounts[static_cast<size_t>(w.boundStageIdx)] <= 14 && w.y > 312.0f) {
                        needsMigration = true;
                    }
                    if (w.boundStageIdx == 0 && (isKnobWidget(w.type) || w.type == UiWidgetType::VerticalStudioFader)) {
                        hasStage0Knobs = true;
                    }
                    // Old or unaligned vertical studio faders (< 245 px tall) -> rebuild as the aligned fader bank.
                    if (w.type == UiWidgetType::VerticalStudioFader && w.h < 245.0f) {
                        needsMigration = true;
                    }
                    // Auto per-stage MIX knob on a stage that is not in parallel routing -> rebuild.
                    if (w.boundParamSlot == 109 && w.boundStageIdx < static_cast<int>(chain.size()) &&
                        !chain[static_cast<size_t>(w.boundStageIdx)].isParallel) {
                        needsMigration = true;
                    }
                }
            }
        }
        if (!hasStage0Knobs || stageFrameCount != static_cast<int>(chain.size())) {
            needsMigration = true;
        }
    }
    if (!needsMigration) {
        return false;
    }

    const std::string title = layout.theme.pluginTitle.empty() ? "CUSTOM PLUGIN" : layout.theme.pluginTitle;
    const int savedActiveStage = std::clamp(layout.activeStageIdx, 0, static_cast<int>(chain.size()) - 1);
    const auto oldWidgets = layout.widgets;

    layout = buildFaceplateFromSchematicWithTheme(title, chain, layout.theme);
    layout.activeStageIdx = savedActiveStage;

    for (auto& nw : layout.widgets) {
        for (const auto& ow : oldWidgets) {
            if (nw.type == UiWidgetType::RivetedNameplate && ow.type == UiWidgetType::RivetedNameplate) {
                nw.scalePreset    = ow.scalePreset;
                nw.accentColor    = ow.accentColor;
                nw.fontSizePreset = ow.fontSizePreset;
                if (!ow.title.empty())    nw.title    = ow.title;
                if (!ow.subtitle.empty()) nw.subtitle = ow.subtitle;
                break;
            }
            if (nw.type == UiWidgetType::SectionFrame && ow.type == UiWidgetType::SectionFrame &&
                nw.boundStageIdx == -1 && ow.boundStageIdx == -1) {
                nw.scalePreset    = ow.scalePreset;
                nw.accentColor    = ow.accentColor;
                nw.fontSizePreset = ow.fontSizePreset;
                if (!ow.title.empty()) nw.title = ow.title;
                break;
            }
            if (nw.boundStageIdx == ow.boundStageIdx && nw.boundParamSlot == ow.boundParamSlot &&
                layout.isMasterMeterWidget(nw) == layout.isMasterMeterWidget(ow) &&
                ((isKnobWidget(nw.type) && isKnobWidget(ow.type)) ||
                 (isFaderWidget(nw.type) && isFaderWidget(ow.type)) ||
                 (isSwitchWidget(nw.type) && isSwitchWidget(ow.type)) ||
                 (isMeterWidget(nw.type) && isMeterWidget(ow.type)))) {
                nw.type           = (!layout.isMasterMeterWidget(nw) &&
                                     (nw.type == UiWidgetType::GlowingTubeWindow || nw.type == UiWidgetType::SteppedAttenuatorKnob))
                                        ? nw.type
                                        : ow.type;
                nw.accentColor    = ow.accentColor;
                nw.fontSizePreset = ow.fontSizePreset;
                if (nw.boundStageIdx < 0 || ow.title == nw.title) {
                    nw.scalePreset = ow.scalePreset;
                }
                nw.hasWaxMark     = ow.hasWaxMark;
                nw.waxMarkNorm    = ow.waxMarkNorm;
                if (nw.boundStageIdx < 0 || layout.isMasterMeterWidget(nw)) {
                    if (!ow.title.empty())    nw.title    = ow.title;
                    if (!ow.subtitle.empty()) nw.subtitle = ow.subtitle;
                }
                break;
            }
        }
    }
    for (const auto& ow : oldWidgets) {
        if (ow.type == UiWidgetType::VintageSticker ||
            ow.type == UiWidgetType::SilkscreenLabel ||
            ow.type == UiWidgetType::VentilationGrille ||
            ow.type == UiWidgetType::RackHandlePair ||
            ow.type == UiWidgetType::PatchbayJackSocket) {
            layout.widgets.push_back(ow);
        }
    }
    return true;
}

/**
 * @brief Loads a project's `.faceplate` layout from `plugins/<Name>/<Name>.faceplate` (or falls back to schematic/default).
 */
inline FaceplateLayout loadProjectFaceplateLayout(
    const std::string& projectName,
    const std::vector<SchematicNode>& fallbackSchematic = {},
    const std::filesystem::path& workspaceRoot = resolveAvraWorkspaceRoot()) {
    namespace fs = std::filesystem;
    const fs::path fpPath = workspaceRoot / "plugins" / projectName / (projectName + ".faceplate");

    FaceplateLayout layout;
    if (loadFaceplateLayoutFromFile(fpPath, layout)) {
        if (ensureMultiStageActiveBayLayout(layout, fallbackSchematic)) {
            saveFaceplateLayoutToFile(fpPath, layout);
        }
        return layout;
    }
    if (projectName == "GainPlugin" || projectName == "Avra Gain") {
        const fs::path altPath = workspaceRoot / "plugins" / "AvraGain" / "AvraGain.faceplate";
        if (loadFaceplateLayoutFromFile(altPath, layout)) {
            if (ensureMultiStageActiveBayLayout(layout, fallbackSchematic)) {
                saveFaceplateLayoutToFile(altPath, layout);
            }
            return layout;
        }
    }

    if (projectName == "GainPlugin" || projectName == "Avra Gain" || projectName == "AvraGain") {
        return buildAvraGainDemoFaceplate("AVRA GAIN");
    }
    return buildFaceplateFromSchematic(projectName, fallbackSchematic);
}

/**
 * @brief Returns the hit-test and render rectangle for the `[ 👁 EARS ONLY ]` (`DARK METERS`)
 * chassis header button inside `canvasRect`.
 */
[[nodiscard]] inline gui::Rect<float> getFaceplateEarsOnlyButtonRect(gui::Rect<float> canvasRect) noexcept {
    const float sx = canvasRect.width  / FaceplateLayout::kRefWidth;
    const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
    return {
        canvasRect.x + (FaceplateLayout::kRefWidth - 294.0f) * sx,
        canvasRect.y + 9.0f * sy,
        132.0f * sx,
        28.0f * sy
    };
}

/**
 * @brief Control-aware foreground relic weathering pass for `12: GRIMY RELIC WEAR`
 * (`SurfaceGrain::GrimyRoadWornRelic`). Weathers stage bays, rotary knob dial rings,
 * toggle switches/buttons, meter bezels, nameplate corners, and chassis mounting screws.
 */
inline void drawRelicControlWeathering(
    gui::Graphics& g,
    gui::Rect<float> canvasRect,
    const FaceplateLayout& layout,
    int weatherLayer = 0) {
    using namespace avra::gui;

    if (layout.theme.grain != SurfaceGrain::GrimyRoadWornRelic && layout.theme.surfaceFinish != 12) {
        return;
    }

    const float sx = canvasRect.width  / FaceplateLayout::kRefWidth;
    const float sy = canvasRect.height / FaceplateLayout::kRefHeight;
    const float sc = std::min(sx, sy);

    auto mapRect = [&](float rx, float ry, float rw, float rh) -> Rect<float> {
        return {
            canvasRect.x + rx * sx,
            canvasRect.y + ry * sy,
            rw * sx,
            rh * sy
        };
    };

    auto hash01 = [](uint32_t s) noexcept -> float {
        s ^= s >> 16;
        s *= 0x7feb352du;
        s ^= s >> 15;
        s *= 0x846ca68bu;
        s ^= s >> 16;
        return static_cast<float>(s & 0x00FFFFFFu) * (1.0f / 16777215.0f);
    };

    // Layer 0: Under-control finger-oil halos, high-traffic 7-to-2 o'clock dial-ring rub arcs, and switch thumb grease
    if (weatherLayer == 0) {
        for (size_t i = 0; i < layout.widgets.size(); ++i) {
            const auto& w = layout.widgets[i];
            if (!layout.isWidgetVisibleForActiveStage(w)) continue;
            const float effX = layout.getEffectiveWidgetX(w);
            const Rect<float> wr = mapRect(effX, w.y, w.w, w.h);
            const uint32_t wSeed = static_cast<uint32_t>(i * 747796405u + 2891336453u);

            if (isKnobWidget(w.type)) {
                const float minDim = std::min(wr.width, wr.height);
                const Point<float> c = { wr.getCentre().x, wr.y + minDim * 0.45f };
                const float outerR = minDim * 0.42f;

                // 1. Soft dark finger-oil smudge halo slightly offset below-right of the knob skirt
                const Point<float> smudgeC = { c.x + outerR * 0.14f, c.y + outerR * 0.18f };
                g.fillCircle(smudgeC, outerR * 1.18f, Color(0.04f, 0.03f, 0.02f, 0.14f));
                g.fillCircle(smudgeC, outerR * 0.98f, Color(0.04f, 0.03f, 0.02f, 0.22f));
                g.fillCircle({ c.x - outerR * 0.10f, c.y + outerR * 0.08f }, outerR * 0.90f, Color(0.16f, 0.11f, 0.06f, 0.18f));

                // 2. 4 irregular chipped/rubbed bare-aluminum wear arcs and micro-scratches along 7 o'clock to 2 o'clock
                // Standard screen angles: 7 o'clock = 120 deg (2.094 rad) clockwise through 9/12 o'clock to 2 o'clock = 330 deg (5.760 rad)
                for (int arcIdx = 0; arcIdx < 4; ++arcIdx) {
                    const uint32_t aSeed = wSeed ^ static_cast<uint32_t>(arcIdx * 1597334677u);
                    const float baseAngle = 2.15f + static_cast<float>(arcIdx) * 0.88f + (hash01(aSeed) - 0.5f) * 0.32f;
                    const float spanAngle = 0.28f + hash01(aSeed + 1u) * 0.34f;
                    const float ringR     = outerR * (0.80f + hash01(aSeed + 2u) * 0.22f);

                    const int segs = 7;
                    Point<float> prevP{};
                    for (int s = 0; s <= segs; ++s) {
                        const float t = static_cast<float>(s) / static_cast<float>(segs);
                        const float ang = baseAngle + t * spanAngle;
                        const float jitterR = ringR + (hash01(aSeed + static_cast<uint32_t>(s * 17)) - 0.5f) * 2.2f * sc;
                        const Point<float> curP = {
                            c.x + std::cos(ang) * jitterR,
                            c.y + std::sin(ang) * jitterR
                        };
                        if (s > 0) {
                            // Dark primer edge + bright bare-aluminum wear arc
                            g.drawLine({ prevP.x - 0.8f * sc, prevP.y - 0.8f * sc },
                                       { curP.x - 0.8f * sc, curP.y - 0.8f * sc },
                                       Color(0.05f, 0.05f, 0.06f, 0.46f), 2.2f * sc);
                            g.drawLine(prevP, curP, Color(0.78f, 0.82f, 0.86f, 0.55f), 1.45f * sc);
                        }
                        prevP = curP;
                    }

                    // Tangential fingernail micro-scratch near the dial tick mark
                    const float scratchAng = baseAngle + spanAngle * 0.5f;
                    const Point<float> sp0 = {
                        c.x + std::cos(scratchAng) * (ringR - 3.0f * sc),
                        c.y + std::sin(scratchAng) * (ringR - 3.0f * sc)
                    };
                    const Point<float> sp1 = {
                        c.x + std::cos(scratchAng + 0.14f) * (ringR + 5.5f * sc),
                        c.y + std::sin(scratchAng + 0.14f) * (ringR + 5.5f * sc)
                    };
                    g.drawLine(sp0, sp1, Color(0.82f, 0.86f, 0.90f, 0.52f), 0.95f * sc);
                }
            } else if (isSwitchWidget(w.type)) {
                const Point<float> c = wr.getCentre();
                const float swR = std::min(wr.width, wr.height) * 0.36f;

                // Vertical thumb-wear rub mark & warm brownish-amber finger-grease patina around switch nut / below button bezel
                g.fillRoundedRect({ c.x - swR * 0.85f, c.y - swR * 0.55f, swR * 1.70f, swR * 2.15f },
                                  swR * 0.65f, Color(0.18f, 0.13f, 0.07f, 0.35f));
                g.fillCircle({ c.x + swR * 0.10f, c.y + swR * 0.32f }, swR * 0.88f, Color(0.05f, 0.04f, 0.03f, 0.28f));
            }
        }
        return;
    }

    // Layer 1: Foreground edge chips on SectionFrame grooves, switch/button corner nicks, meter/nameplate corner wear, and screw slip scratches
    const Color chipMetal  = Color::fromHex(0xC8D0D8, 0.68f);
    const Color chipBright = Color::fromHex(0xE2E8EE, 0.74f);
    const Color chipPrimer = Color(0.05f, 0.05f, 0.06f, 0.62f);

    for (size_t i = 0; i < layout.widgets.size(); ++i) {
        const auto& w = layout.widgets[i];
        if (!layout.isWidgetVisibleForActiveStage(w)) continue;
        const float effX = layout.getEffectiveWidgetX(w);
        const Rect<float> wr = mapRect(effX, w.y, w.w, w.h);
        const uint32_t wSeed = static_cast<uint32_t>(i * 1103515245u + 12345u);

        if (w.type == UiWidgetType::SectionFrame) {
            const Rect<float> groove = wr.reduced(2.0f * sc);
            // Deterministic bare-metal paint chips & edge-rubbing silver highlights along top, bottom, and side rails
            for (int cIdx = 0; cIdx < 9; ++cIdx) {
                const uint32_t cs = wSeed ^ static_cast<uint32_t>(cIdx * 374761393u);
                const float frac = 0.08f + 0.84f * hash01(cs);
                const float chipLen = (5.0f + hash01(cs + 1u) * 11.0f) * sc;
                const bool onTop = (cIdx % 2 == 0);
                const float ry = onTop ? (groove.y + 6.0f * sc) : (groove.bottom() - 1.0f * sc);
                const float rx = groove.x + frac * groove.width;
                // Avoid the top-center title gap on the top rail
                if (onTop && std::abs(rx - groove.getCentre().x) < groove.width * 0.24f) continue;

                g.drawLine({ rx, ry - 0.8f * sc }, { rx + chipLen, ry - 0.8f * sc }, chipPrimer, 2.0f * sc);
                g.drawLine({ rx, ry }, { rx + chipLen, ry }, chipMetal, 1.35f * sc);
            }

            // Corner wear nicks on the 4 corners of the SectionFrame
            const Point<float> corners[4] = {
                { groove.x + 2.0f * sc,        groove.y + 8.0f * sc },
                { groove.right() - 8.0f * sc,  groove.y + 8.0f * sc },
                { groove.x + 2.0f * sc,        groove.bottom() - 2.0f * sc },
                { groove.right() - 8.0f * sc,  groove.bottom() - 2.0f * sc }
            };
            for (int k = 0; k < 4; ++k) {
                g.drawLine(corners[k], { corners[k].x + 6.5f * sc, corners[k].y + (k < 2 ? 2.5f : -2.5f) * sc }, chipBright, 1.4f * sc);
            }
        } else if (isSwitchWidget(w.type)) {
            const Point<float> c = wr.getCentre();
            const float bw = std::min(wr.width * 0.42f, 22.0f * sc);
            const float bh = std::min(wr.height * 0.34f, 16.0f * sc);
            // Tiny bare-metal nicks on switch nut / button corners
            g.drawLine({ c.x - bw, c.y - bh }, { c.x - bw + 3.8f * sc, c.y - bh + 1.4f * sc }, chipBright, 1.25f * sc);
            g.drawLine({ c.x + bw - 3.6f * sc, c.y + bh }, { c.x + bw, c.y + bh - 1.8f * sc }, chipMetal, 1.25f * sc);
            g.drawLine({ c.x - 3.2f * sc, c.y + bh + 4.5f * sc }, { c.x + 4.2f * sc, c.y + bh + 5.2f * sc },
                       Color(0.78f, 0.82f, 0.86f, 0.48f), 1.0f * sc);
        } else if (isMeterWidget(w.type) || w.type == UiWidgetType::RivetedNameplate) {
            const Rect<float> mr = wr.reduced(3.0f * sc);
            // Bare-metal corner rub highlights & diagonal bezel scratches around the 4 corners of the VU meter / nameplate
            g.drawLine({ mr.x, mr.y + 5.0f * sc }, { mr.x + 6.5f * sc, mr.y }, chipBright, 1.5f * sc);
            g.drawLine({ mr.right() - 6.5f * sc, mr.y }, { mr.right(), mr.y + 5.0f * sc }, chipMetal, 1.4f * sc);
            g.drawLine({ mr.x, mr.bottom() - 5.0f * sc }, { mr.x + 7.0f * sc, mr.bottom() }, chipMetal, 1.4f * sc);
            g.drawLine({ mr.right() - 7.5f * sc, mr.bottom() }, { mr.right(), mr.bottom() - 5.5f * sc }, chipBright, 1.5f * sc);

            // Long horizontal rack-wear rub streak along the bottom bezel edge
            g.drawLine({ mr.x + mr.width * 0.18f, mr.bottom() + 0.8f * sc },
                       { mr.x + mr.width * 0.56f, mr.bottom() + 0.8f * sc },
                       chipPrimer, 2.0f * sc);
            g.drawLine({ mr.x + mr.width * 0.18f, mr.bottom() },
                       { mr.x + mr.width * 0.54f, mr.bottom() },
                       chipMetal, 1.25f * sc);
        }
    }

    // Chassis mounting screws weathering + screwdriver slip scratches near 2 of the mounting screws
    const float topBarRefH = layout.theme.showTopToolbar ? 46.0f : 0.0f;
    const Rect<float> chassis = mapRect(0.0f, topBarRefH, FaceplateLayout::kRefWidth, FaceplateLayout::kRefHeight - topBarRefH);
    const Point<float> screwPts[4] = {
        { chassis.x + 15.0f * sx,      chassis.y + 18.0f * sy },
        { chassis.right() - 15.0f * sx, chassis.y + 18.0f * sy },
        { chassis.x + 15.0f * sx,      chassis.bottom() - 18.0f * sy },
        { chassis.right() - 15.0f * sx, chassis.bottom() - 18.0f * sy }
    };
    for (int sIdx = 0; sIdx < 4; ++sIdx) {
        const Point<float> sp = screwPts[sIdx];
        // Oxidized washer grime halo + scraped bare-aluminum ring around each mounting screw
        g.fillCircle({ sp.x + 1.0f * sc, sp.y + 1.5f * sc }, 10.5f * sc, Color(0.08f, 0.06f, 0.04f, 0.38f));
        g.drawCircle(sp, 8.2f * sc, chipMetal, 1.15f * sc);

        // Screwdriver slip gouge on top-left and bottom-right rack bolts
        if (sIdx == 0 || sIdx == 3) {
            const float dirX = (sIdx == 0) ? 1.0f : -1.0f;
            const float dirY = (sIdx == 0) ? 0.68f : -0.58f;
            const Point<float> g0 = { sp.x + dirX * 5.0f * sc,  sp.y + dirY * 4.0f * sc };
            const Point<float> g1 = { sp.x + dirX * 26.0f * sc, sp.y + dirY * 19.0f * sc };
            g.drawLine({ g0.x, g0.y - 0.9f * sc }, { g1.x, g1.y - 0.9f * sc }, chipPrimer, 2.1f * sc);
            g.drawLine(g0, g1, chipBright, 1.35f * sc);
        }
    }
}

/**
 * @brief Renders a complete WYSIWYG Hardware Faceplate inside any target canvas rectangle.
 * Automatically scales from the 1020x560 reference space to `canvasRect`.
 */
/**
 * @brief Maps a `UiStyleTemplate` to its exact `assets/themes/backdrop_XX.jpg` asset number,
 *        or returns `-1` for procedural anodized chassis templates.
 */
[[nodiscard]] constexpr int getThemeBackdropAssetIndex(UiStyleTemplate tpl) noexcept {
    switch (tpl) {
        case UiStyleTemplate::FrostedGlassNeumorphism: return 35;
        case UiStyleTemplate::MinimalistPureVector:    return 36;
        case UiStyleTemplate::GreyscaleMonochrome:     return 37;
        case UiStyleTemplate::FuturisticSciFiHud:      return 38;
        case UiStyleTemplate::RetroDistressedHazard:   return 39;
        case UiStyleTemplate::UnderwaterSubmarine:     return 40;
        case UiStyleTemplate::TattooArtTraditional:    return 42;
        case UiStyleTemplate::HeavyMetalForged:        return 43;
        case UiStyleTemplate::ClassicRockAmpHead:      return 44;
        case UiStyleTemplate::FlowerPowerPsychedelic:  return 46;
        case UiStyleTemplate::GrimReaperNecropolis:    return 47;
        case UiStyleTemplate::HellFireMagma:           return 48;
        case UiStyleTemplate::ApocalypseWasteland:     return 49;
        case UiStyleTemplate::GrassMeadowBotanical:    return 51;
        case UiStyleTemplate::SkyCloudsGoldenHour:     return 52;
        case UiStyleTemplate::RainStormLightning:      return 53;
        case UiStyleTemplate::SnowIcyGlacial:          return 54;
        default:                                       return -1;
    }
}

inline void paintFaceplateCanvas(
    gui::Graphics& g,
    gui::Rect<float> canvasRect,
    const FaceplateLayout& layout,
    int selectedWidgetIdx = -1,
    bool showEditorOverlays = true,
    float liveGrDb = 3.5f,
    float liveInDb = -12.0f,
    float liveOutDb = -9.5f,
    float liveOutDbR = -999.0f,
    bool isDraggingWidget = false,
    gui::Point<float> hoverMousePos = { -1000.0f, -1000.0f }) {
    using namespace avra::gui;

    const float sx = canvasRect.width  / FaceplateLayout::kRefWidth;
    const float sy = canvasRect.height / FaceplateLayout::kRefHeight;

    auto mapRect = [&](float rx, float ry, float rw, float rh) -> Rect<float> {
        return {
            canvasRect.x + rx * sx,
            canvasRect.y + ry * sy,
            rw * sx,
            rh * sy
        };
    };

    // 1. Top Dark Studio Header Strip (if enabled)
    const float topBarRefH = layout.theme.showTopToolbar ? 46.0f : 0.0f;
    if (layout.theme.showTopToolbar) {
        Rect<float> topBar = mapRect(0.0f, 0.0f, FaceplateLayout::kRefWidth, topBarRefH);
        g.fillLinearGradientRect(topBar, Color::fromHex(0x23272C), Color::fromHex(0x16191D), 4.0f);
        g.drawLine({ topBar.x, topBar.bottom() }, { topBar.right(), topBar.bottom() }, Color::fromHex(0x090C0E), 1.5f);

        g.drawText(layout.theme.pluginTitle, mapRect(18.0f, 8.0f, 340.0f, 30.0f), Color::fromHex(0xF0F6F8), 16.0f, false, true);

        avra::provenance::drawTopBarProvenancePill(g, mapRect(FaceplateLayout::kRefWidth - 332.0f, 9.0f, 318.0f, 28.0f), false);
    }

    const bool darkMeters = false;
    const float effLiveGrDb   = liveGrDb;
    const float effLiveInDb   = liveInDb;
    const float effLiveOutDb  = liveOutDb;
    const float effLiveOutDbR = liveOutDbR;

    // 2. Main Brushed Anodized Hardware Chassis or Bespoke Reference Backdrop
    Rect<float> chassisRect = mapRect(4.0f, topBarRefH + 4.0f, FaceplateLayout::kRefWidth - 8.0f, FaceplateLayout::kRefHeight - topBarRefH - 8.0f);
    const int activeTplIdx = std::clamp(layout.theme.styleTemplateIdx, 0, kTotalUiStyleTemplates - 1);
    const int backdropAssetIdx = getThemeBackdropAssetIndex(static_cast<UiStyleTemplate>(activeTplIdx));
    const bool hasBespokeBackdrop = (backdropAssetIdx > 0);
    bool drewBespokeBackdrop = false;
    if (hasBespokeBackdrop) {
        drewBespokeBackdrop = g.drawThemeBackdrop(std::format("backdrop_{}.jpg", backdropAssetIdx), chassisRect, 1.0f);
    }
    if (drewBespokeBackdrop) {
        if (hasCustomSurfaceOrColorOverride(layout.theme)) {
            HardwareMaterials::drawSurfaceOverlay(
                g,
                chassisRect,
                layout.theme.finish,
                layout.theme.grain,
                layout.theme.useCustomFinishColor,
                layout.theme.customFinishHex,
                0.56f);
        }
        HardwareMaterials::drawChassisMountingAndScrews(
            g,
            chassisRect,
            layout.theme.cornerScrews,
            layout.theme.hardwareStyle,
            layout.theme.screwStyle,
            layout.theme.bevelDepthPx);
    } else {
        HardwareMaterials::drawBrushedAnodizedChassis(
            g,
            chassisRect,
            layout.theme.finish,
            layout.theme.grain,
            layout.theme.cornerScrews,
            layout.theme.hardwareStyle,
            layout.theme.screwStyle,
            layout.theme.useCustomFinishColor,
            layout.theme.customFinishHex,
            layout.theme.bevelDepthPx);

        // 2.1. Fallback Procedural Backdrop Artwork if external asset is unavailable
        const auto activeTpl = static_cast<UiStyleTemplate>(activeTplIdx);
        switch (activeTpl) {
            case UiStyleTemplate::FrostedGlassNeumorphism: {
                // Soft warm-amber & cool-sky ambient light blooms behind translucent frosted glass
                g.fillRadialGradientCircle({ chassisRect.x + chassisRect.width * 0.22f, chassisRect.y + chassisRect.height * 0.62f },
                                           220.0f * sx, Color::fromHex(0xF59E42, 0.18f), Color::fromHex(0xF59E42, 0.0f));
                g.fillRadialGradientCircle({ chassisRect.x + chassisRect.width * 0.62f, chassisRect.y + chassisRect.height * 0.35f },
                                           240.0f * sx, Color::fromHex(0x38BDF8, 0.18f), Color::fromHex(0x38BDF8, 0.0f));
                g.drawRoundedRect(chassisRect.reduced(4.0f), 8.0f, Color::fromHex(0xFFFFFF, 0.85f), 1.6f);
                break;
            }
            case UiStyleTemplate::FuturisticSciFiHud: {
                // Holographic cyan & emerald HUD grid lines + glowing outer tech brackets
                for (int r = 1; r < 8; ++r) {
                    const float gy = chassisRect.y + chassisRect.height * (static_cast<float>(r) / 8.0f);
                    g.drawLine({ chassisRect.x + 14.0f, gy }, { chassisRect.right() - 14.0f, gy }, Color::fromHex(0x00F5D4, 0.09f), 0.9f);
                }
                for (int c = 1; c < 12; ++c) {
                    const float gx = chassisRect.x + chassisRect.width * (static_cast<float>(c) / 12.0f);
                    g.drawLine({ gx, chassisRect.y + 14.0f }, { gx, chassisRect.bottom() - 14.0f }, Color::fromHex(0x00F5D4, 0.07f), 0.9f);
                }
                g.drawRoundedRect(chassisRect.reduced(6.0f), 6.0f, Color::fromHex(0x00F5D4, 0.65f), 1.8f);
                // Glowing neon green side vertical HUD rails
                g.drawLine({ chassisRect.x + 4.0f, chassisRect.y + 60.0f * sy },
                           { chassisRect.x + 4.0f, chassisRect.bottom() - 60.0f * sy }, Color::fromHex(0x22C55E, 0.85f), 3.0f);
                g.drawLine({ chassisRect.right() - 4.0f, chassisRect.y + 60.0f * sy },
                           { chassisRect.right() - 4.0f, chassisRect.bottom() - 60.0f * sy }, Color::fromHex(0x22C55E, 0.85f), 3.0f);
                break;
            }
            case UiStyleTemplate::RetroDistressedHazard: {
                // Weathered caterpillar-yellow industrial plate on left bay + diagonal black hazard stripes + rust edges
                const Rect<float> yellowPlate = {
                    chassisRect.x + 24.0f * sx,
                    chassisRect.y + 68.0f * sy,
                    chassisRect.width * 0.70f,
                    chassisRect.height - 86.0f * sy
                };
                g.fillRoundedRect(yellowPlate, 5.0f, Color::fromHex(0xD49B1A, 0.72f));
                g.drawRoundedRect(yellowPlate, 5.0f, Color::fromHex(0x2B1E0D, 0.85f), 2.2f);
                const float stripeTop = yellowPlate.bottom() - 145.0f * sy;
                for (float x = yellowPlate.x + 20.0f * sx; x < yellowPlate.right() - 36.0f * sx; x += 56.0f * sx) {
                    std::array<Point<float>, 4> st = {{
                        { x, stripeTop },
                        { x + 28.0f * sx, stripeTop },
                        { x - 18.0f * sx, yellowPlate.bottom() - 4.0f },
                        { x - 46.0f * sx, yellowPlate.bottom() - 4.0f }
                    }};
                    g.fillPolygon(st, Color::fromHex(0x181512, 0.82f));
                }
                break;
            }
            case UiStyleTemplate::UnderwaterSubmarine: {
                // Abyssal ocean light rays + rising bioluminescent bubbles + swaying kelp silhouettes
                g.fillRadialGradientCircle({ chassisRect.x + chassisRect.width * 0.36f, chassisRect.y + 20.0f },
                                           chassisRect.width * 0.55f,
                                           Color::fromHex(0x00E5FF, 0.24f), Color::fromHex(0x021218, 0.0f));
                for (int r = 0; r < 5; ++r) {
                    const float rx = chassisRect.x + (110.0f + static_cast<float>(r) * 135.0f) * sx;
                    std::array<Point<float>, 4> ray = {{
                        { rx, chassisRect.y + 4.0f },
                        { rx + 38.0f * sx, chassisRect.y + 4.0f },
                        { rx + 95.0f * sx, chassisRect.bottom() - 8.0f },
                        { rx + 25.0f * sx, chassisRect.bottom() - 8.0f }
                    }};
                    g.fillPolygon(ray, Color::fromHex(0x48CAE4, 0.06f));
                }
                for (int b = 0; b < 18; ++b) {
                    const float bx = chassisRect.x + 36.0f + static_cast<float>((b * 73) % 920) * sx;
                    const float by = chassisRect.y + 28.0f + static_cast<float>((b * 53) % 430) * sy;
                    const float br = (2.5f + static_cast<float>(b % 4) * 1.8f) * sx;
                    g.drawCircle({ bx, by }, br, Color::fromHex(0x90F1EF, 0.42f), 1.1f);
                    g.fillCircle({ bx - br * 0.35f, by - br * 0.35f }, br * 0.28f, Color::fromHex(0xFFFFFF, 0.55f));
                }
                break;
            }
            case UiStyleTemplate::TattooArtTraditional: {
                // Vintage traditional tattoo flash parchment with burnt-umber vignette, nautical stars & corner roses
                g.drawRoundedRect(chassisRect.reduced(6.0f), 6.0f, Color::fromHex(0x3D2314, 0.85f), 2.6f);
                // Corner crimson roses (bottom-left and bottom-right)
                for (int side = 0; side < 2; ++side) {
                    const float rx = (side == 0) ? (chassisRect.x + 28.0f * sx) : (chassisRect.right() - 28.0f * sx);
                    const float ry = chassisRect.bottom() - 28.0f * sy;
                    g.fillCircle({ rx - 14.0f * sx, ry - 6.0f * sy }, 12.0f * sx, Color::fromHex(0x2D6A4F));
                    g.fillCircle({ rx + 14.0f * sx, ry + 6.0f * sy }, 12.0f * sx, Color::fromHex(0x2D6A4F));
                    g.fillRadialGradientCircle({ rx, ry }, 22.0f * sx, Color::fromHex(0xE11D48), Color::fromHex(0x640D14));
                    g.drawCircle({ rx, ry }, 22.0f * sx, Color::fromHex(0x1C0D08), 2.0f);
                    g.drawCircle({ rx, ry }, 13.0f * sx, Color::fromHex(0x1C0D08), 1.5f);
                }
                break;
            }
            case UiStyleTemplate::HeavyMetalForged: {
                // Forged dark armor plate with pyramid-studded side straps & glowing ember sparks at bottom
                for (float py = chassisRect.y + 24.0f * sy; py < chassisRect.bottom() - 20.0f * sy; py += 26.0f * sy) {
                    for (int side = 0; side < 2; ++side) {
                        const float px = (side == 0) ? (chassisRect.x + 12.0f * sx) : (chassisRect.right() - 12.0f * sx);
                        g.fillRect({ px - 4.5f * sx, py - 4.5f * sy, 9.0f * sx, 9.0f * sy }, Color::fromHex(0x64748B));
                        g.fillCircle({ px - 1.0f * sx, py - 1.0f * sy }, 2.2f * sx, Color::fromHex(0xE2E8F0));
                    }
                }
                g.fillLinearGradientRect({ chassisRect.x + 6.0f, chassisRect.bottom() - 48.0f * sy, chassisRect.width - 12.0f, 42.0f * sy },
                                         Color::fromHex(0xFF4500, 0.0f), Color::fromHex(0xFF5500, 0.25f));
                break;
            }
            case UiStyleTemplate::ClassicRockAmpHead: {
                // 1970s Classic Rock Studio Console: Walnut side cheeks + brass trim rails + padded leather bolster
                const float cheekW = 28.0f * sx;
                g.fillLinearGradientRect({ chassisRect.x, chassisRect.y, cheekW, chassisRect.height },
                                         Color::fromHex(0x6E3B1E, 0.95f), Color::fromHex(0x3D1E0D, 0.95f));
                g.fillLinearGradientRect({ chassisRect.right() - cheekW, chassisRect.y, cheekW, chassisRect.height },
                                         Color::fromHex(0x3D1E0D, 0.95f), Color::fromHex(0x6E3B1E, 0.95f));
                g.drawLine({ chassisRect.x + cheekW, chassisRect.y }, { chassisRect.x + cheekW, chassisRect.bottom() },
                           Color::fromHex(0xD4AF37, 0.85f), 2.2f);
                g.drawLine({ chassisRect.right() - cheekW, chassisRect.y }, { chassisRect.right() - cheekW, chassisRect.bottom() },
                           Color::fromHex(0xD4AF37, 0.85f), 2.2f);
                break;
            }
            case UiStyleTemplate::FlowerPowerPsychedelic: {
                // Vivid 60s rainbow concentric psychedelic ribbons + daisy border blossoms
                const Point<float> c = { chassisRect.x + chassisRect.width * 0.38f, chassisRect.bottom() + 20.0f * sy };
                g.drawArc(c, 430.0f * sx, 3.14f, 6.28f, Color::fromHex(0xF472B6, 0.38f), 38.0f * sx);
                g.drawArc(c, 370.0f * sx, 3.14f, 6.28f, Color::fromHex(0xFB923C, 0.38f), 36.0f * sx);
                g.drawArc(c, 310.0f * sx, 3.14f, 6.28f, Color::fromHex(0xFACC15, 0.42f), 36.0f * sx);
                g.drawArc(c, 250.0f * sx, 3.14f, 6.28f, Color::fromHex(0x38BDF8, 0.38f), 34.0f * sx);
                g.drawArc(c, 190.0f * sx, 3.14f, 6.28f, Color::fromHex(0xA855F7, 0.35f), 32.0f * sx);
                // Daisy chain along bottom edge
                for (float fx = chassisRect.x + 28.0f * sx; fx < chassisRect.right() - 28.0f * sx; fx += 44.0f * sx) {
                    const float fy = chassisRect.bottom() - 12.0f * sy;
                    g.fillCircle({ fx, fy }, 7.5f * sx, Color::fromHex(0xFFFFFF, 0.88f));
                    g.fillCircle({ fx, fy }, 3.8f * sx, Color::fromHex(0xFACC15));
                }
                break;
            }
            case UiStyleTemplate::GrimReaperNecropolis: {
                // 4 Gothic Cathedral Stone Arches framing the stage control columns + glowing ectoplasm mist
                for (int col = 0; col < 4; ++col) {
                    const float ax = chassisRect.x + (34.0f + static_cast<float>(col) * 172.0f) * sx;
                    const float ay = chassisRect.y + 62.0f * sy;
                    const float aw = 160.0f * sx;
                    const float ah = chassisRect.height - 76.0f * sy;
                    g.drawRoundedRect({ ax, ay, aw, ah }, 18.0f, Color::fromHex(0x3E4E46, 0.65f), 2.4f);
                    g.drawLine({ ax, ay + ah }, { ax, ay + 24.0f * sy }, Color::fromHex(0x1C2621, 0.85f), 5.0f);
                }
                g.fillLinearGradientRect({ chassisRect.x + 6.0f, chassisRect.bottom() - 85.0f * sy, chassisRect.width - 12.0f, 79.0f * sy },
                                         Color::fromHex(0x38F888, 0.0f), Color::fromHex(0x38F888, 0.28f));
                break;
            }
            case UiStyleTemplate::HellFireMagma: {
                // Glowing molten lava rivers cracking across dark volcanic basalt + roaring bottom flames
                g.fillLinearGradientRect({ chassisRect.x + 6.0f, chassisRect.bottom() - 96.0f * sy, chassisRect.width - 12.0f, 90.0f * sy },
                                         Color::fromHex(0xFF4500, 0.0f), Color::fromHex(0xFF9900, 0.48f));
                // Glowing magma fissures between knob bays
                for (int c = 1; c <= 4; ++c) {
                    const float fx = chassisRect.x + static_cast<float>(c) * 175.0f * sx;
                    g.drawLine({ fx - 12.0f * sx, chassisRect.y + 55.0f * sy },
                               { fx + 8.0f * sx,  chassisRect.y + 235.0f * sy }, Color::fromHex(0xFF5500, 0.55f), 3.6f);
                    g.drawLine({ fx + 8.0f * sx,  chassisRect.y + 235.0f * sy },
                               { fx - 6.0f * sx,  chassisRect.bottom() - 14.0f }, Color::fromHex(0xFFB703, 0.65f), 2.8f);
                }
                for (float fx = chassisRect.x + 18.0f * sx; fx < chassisRect.right() - 18.0f * sx; fx += 28.0f * sx) {
                    const float fh = (22.0f + static_cast<float>(static_cast<int>(fx) % 26)) * sy;
                    std::array<Point<float>, 3> fl = {{
                        { fx - 14.0f * sx, chassisRect.bottom() - 2.0f },
                        { fx,              chassisRect.bottom() - 2.0f - fh },
                        { fx + 14.0f * sx, chassisRect.bottom() - 2.0f }
                    }};
                    g.fillPolygon(fl, Color::fromHex(0xFF6B00, 0.45f));
                }
                break;
            }
            case UiStyleTemplate::ApocalypseWasteland: {
                // Corroded scrap-metal plates + horizontal black/yellow hazard stripe divider + radiation trefoil
                const float bandY = chassisRect.y + chassisRect.height * 0.54f;
                g.fillRect({ chassisRect.x + 16.0f * sx, bandY, chassisRect.width - 32.0f * sx, 16.0f * sy }, Color::fromHex(0xEAB308, 0.82f));
                for (float x = chassisRect.x + 20.0f * sx; x < chassisRect.right() - 32.0f * sx; x += 28.0f * sx) {
                    std::array<Point<float>, 4> hz = {{
                        { x, bandY },
                        { x + 14.0f * sx, bandY },
                        { x + 4.0f * sx, bandY + 16.0f * sy },
                        { x - 10.0f * sx, bandY + 16.0f * sy }
                    }};
                    g.fillPolygon(hz, Color::fromHex(0x181512, 0.88f));
                }
                break;
            }
            case UiStyleTemplate::GrassMeadowBotanical: {
                // Lush meadow green grass blades, shamrock clovers & wildflowers along bottom and side borders
                g.fillLinearGradientRect({ chassisRect.x + 4.0f, chassisRect.bottom() - 72.0f * sy, chassisRect.width - 8.0f, 68.0f * sy },
                                         Color::fromHex(0x166534, 0.0f), Color::fromHex(0x15803D, 0.65f));
                for (float gx = chassisRect.x + 14.0f * sx; gx < chassisRect.right() - 14.0f * sx; gx += 14.0f * sx) {
                    const float gh = (22.0f + static_cast<float>(static_cast<int>(gx) % 24)) * sy;
                    const float tilt = (static_cast<float>((static_cast<int>(gx) / 14) % 5) - 2.0f) * 3.5f * sx;
                    g.drawLine({ gx, chassisRect.bottom() - 2.0f },
                               { gx + tilt, chassisRect.bottom() - 2.0f - gh },
                               Color::fromHex(0x4ADE80, 0.72f), 2.2f);
                }
                for (float fx = chassisRect.x + 42.0f * sx; fx < chassisRect.right() - 42.0f * sx; fx += 85.0f * sx) {
                    const float fy = chassisRect.bottom() - (14.0f + static_cast<float>(static_cast<int>(fx) % 16)) * sy;
                    g.fillCircle({ fx, fy }, 4.2f * sx, (static_cast<int>(fx) & 1) ? Color::fromHex(0xFACC15) : Color::fromHex(0xFFFFFF));
                }
                break;
            }
            case UiStyleTemplate::SkyCloudsGoldenHour: {
                // Golden-hour peach sunset horizon glow + soft billowing white cumulus cloud banks + wind streamlines
                g.fillLinearGradientRect({ chassisRect.x + 4.0f, chassisRect.bottom() - 120.0f * sy, chassisRect.width - 8.0f, 116.0f * sy },
                                         Color::fromHex(0xFDBA74, 0.0f), Color::fromHex(0xFB923C, 0.38f));
                for (int c = 0; c < 7; ++c) {
                    const float cxCloud = chassisRect.x + (80.0f + static_cast<float>(c) * 145.0f) * sx;
                    const float cyCloud = chassisRect.bottom() - (24.0f + static_cast<float>((c * 19) % 35)) * sy;
                    g.fillRadialGradientCircle({ cxCloud, cyCloud }, 68.0f * sx,
                                               Color::fromHex(0xFFFFFF, 0.42f), Color::fromHex(0xFFFFFF, 0.0f));
                }
                // Graceful wind streamlines across mid-sky
                g.drawArc({ chassisRect.x + 280.0f * sx, chassisRect.y + 240.0f * sy }, 120.0f * sx,
                          1.8f, 4.2f, Color::fromHex(0xFFFFFF, 0.32f), 1.6f);
                break;
            }
            case UiStyleTemplate::RainStormLightning: {
                // Forked electric-blue lightning bolts + rain streaks + beaded condensation droplets
                g.drawLine({ chassisRect.x + 140.0f * sx, chassisRect.y + 6.0f },
                           { chassisRect.x + 118.0f * sx, chassisRect.y + 52.0f * sy }, Color::fromHex(0xBAE6FD, 0.65f), 2.0f);
                g.drawLine({ chassisRect.x + 118.0f * sx, chassisRect.y + 52.0f * sy },
                           { chassisRect.x + 134.0f * sx, chassisRect.y + 88.0f * sy }, Color::fromHex(0x38BDF8, 0.50f), 1.5f);
                for (int d = 0; d < 28; ++d) {
                    const float dx = chassisRect.x + 28.0f + static_cast<float>((d * 61) % 950) * sx;
                    const float dy = chassisRect.y + 20.0f + static_cast<float>((d * 43) % 430) * sy;
                    const float len = (14.0f + static_cast<float>(d % 5) * 6.0f) * sy;
                    g.drawLine({ dx, dy }, { dx, dy + len }, Color::fromHex(0x7DD3FC, 0.30f), 1.4f);
                    g.fillCircle({ dx, dy + len + 1.5f * sy }, (2.0f + static_cast<float>(d % 3) * 0.8f) * sx, Color::fromHex(0xE0F2FE, 0.55f));
                }
                break;
            }
            case UiStyleTemplate::SnowIcyGlacial: {
                // Frosted crystalline perimeter glow + hanging top & bottom glacial icicles + snowflakes
                g.drawRoundedRect(chassisRect.reduced(5.0f), 6.0f, Color::fromHex(0xE0F2FE, 0.78f), 2.4f);
                for (float ix = chassisRect.x + 14.0f; ix < chassisRect.right() - 20.0f; ix += 19.0f * sx) {
                    const float ih = (14.0f + static_cast<float>((static_cast<int>(ix) * 7) % 24)) * sy;
                    std::array<Point<float>, 3> icTop = {{
                        { ix, chassisRect.y + 2.0f },
                        { ix + 4.5f * sx, chassisRect.y + 2.0f + ih },
                        { ix + 9.0f * sx, chassisRect.y + 2.0f }
                    }};
                    g.fillPolygon(icTop, Color::fromHex(0xE0F2FE, 0.72f));
                    const float bh = (10.0f + static_cast<float>((static_cast<int>(ix) * 11) % 18)) * sy;
                    std::array<Point<float>, 3> icBot = {{
                        { ix, chassisRect.bottom() - 2.0f },
                        { ix + 4.5f * sx, chassisRect.bottom() - 2.0f - bh },
                        { ix + 9.0f * sx, chassisRect.bottom() - 2.0f }
                    }};
                    g.fillPolygon(icBot, Color::fromHex(0xBAE6FD, 0.58f));
                }
                break;
            }
            default:
                break;
        }
    }

    // RULE 1: Ban visible graph-paper grid lines on the metal faceplate!
    // Only render subtle snap dots while actively dragging/resizing a widget in the Faceplate Builder.
    if (showEditorOverlays && isDraggingWidget && layout.theme.gridSnapPx > 1) {
        const float gridStep = (layout.theme.gridSnapPx >= 16) ? 32.0f : 32.0f;
        for (float gx = gridStep; gx < FaceplateLayout::kRefWidth; gx += gridStep) {
            const float px = canvasRect.x + gx * sx;
            for (float gy = topBarRefH + gridStep; gy < FaceplateLayout::kRefHeight; gy += gridStep) {
                const float py = canvasRect.y + gy * sy;
                g.fillCircle({ px, py }, 1.2f, Color::fromHex(0x00F5D4, 0.24f));
            }
        }
    }

    // Precision Snap Alignment Guides (Center Crosshair + Selected Widget Alignment Rails)
    if (showEditorOverlays && layout.theme.showSnapGuides) {
        const float cx = canvasRect.x + (FaceplateLayout::kRefWidth * 0.5f) * sx;
        const float cy = canvasRect.y + ((topBarRefH + FaceplateLayout::kRefHeight) * 0.5f) * sy;
        g.drawLine({ cx, chassisRect.y + 4.0f }, { cx, chassisRect.bottom() - 4.0f }, Color::fromHex(0x00F5D4, 0.35f), 1.0f);
        g.drawLine({ chassisRect.x + 4.0f, cy }, { chassisRect.right() - 4.0f, cy }, Color::fromHex(0x00F5D4, 0.35f), 1.0f);
        if (selectedWidgetIdx >= 0 && selectedWidgetIdx < static_cast<int>(layout.widgets.size())) {
            const auto& sw = layout.widgets[static_cast<size_t>(selectedWidgetIdx)];
            const Rect<float> swr = mapRect(layout.getEffectiveWidgetX(sw), sw.y, sw.w, sw.h);
            const Point<float> swc = swr.getCentre();
            g.drawLine({ swc.x, chassisRect.y + 4.0f }, { swc.x, chassisRect.bottom() - 4.0f }, Color::fromHex(0xFF9F1C, 0.45f), 1.0f);
            g.drawLine({ chassisRect.x + 4.0f, swc.y }, { chassisRect.right() - 4.0f, swc.y }, Color::fromHex(0xFF9F1C, 0.45f), 1.0f);
        }
    }

    const Color sectionPlateBg = HardwareMaterials::getSectionPlateColor(
        layout.theme.finish,
        layout.theme.useCustomFinishColor,
        layout.theme.customFinishHex);

    if (showEditorOverlays && layout.widgets.empty()) {
        Rect<float> msgBox = mapRect(120.0f, 220.0f, 780.0f, 110.0f);
        g.fillRoundedRect(msgBox, 6.0f, Color::fromHex(0x0A0E12, 0.72f));
        g.drawRoundedRect(msgBox, 6.0f, Color::fromHex(0x00F5D4, 0.45f), 1.2f);
        g.drawText("FACEPLATE CHASSIS CLEARED  —  SYMBIOTIC SCHEMATIC SYNC ACTIVE",
                   { msgBox.x + 16.0f, msgBox.y + 16.0f, msgBox.width - 32.0f, 26.0f },
                   Color::fromHex(0x00F5D4), 12.5f, true, true);
        g.drawText(std::format("Active Default Scheme:  Knobs = {}   |   Switches = {}   |   Meters = {}",
                               getWidgetTypeName(layout.theme.globalKnobType),
                               getWidgetTypeName(layout.theme.globalSwitchType),
                               getWidgetTypeName(layout.theme.globalMeterType)),
                   { msgBox.x + 16.0f, msgBox.y + 44.0f, msgBox.width - 32.0f, 22.0f },
                   Color::fromHex(0xD4AF37), 10.5f, true, true);
        g.drawText("Add components in the Schematic Builder to auto-populate controls in this scheme, or drag widgets from the left palette!",
                   { msgBox.x + 16.0f, msgBox.y + 70.0f, msgBox.width - 32.0f, 22.0f },
                   Color::fromHex(0xD8E2E6), 9.8f, true, false);
    }

    // Apply whole-skin typography (Font Family, Font Size Scale, Silkscreen Text Color, and Text Plate Color)
    g.setFontFamily(layout.theme.globalFontFamily);
    g.setFontScale(getUiFontScaleFactor(layout.theme.globalFontSizePreset));
    g.setSkinTextColor(true, getSkinFontColorValue(layout.theme));
    g.setSkinTextPlateColor(true, getSkinTextPlateColorValue(layout.theme));

    // 2.5. Top Signal-Chain Strip (x = 34..744, y = 58..100, height = 42px) when > 3 stages are loaded
    // No dark backing box is drawn so the brushed metal chassis and left rack-ear seam (x = 26) remain fully visible.
    if (layout.usesSignalChainSelectorStrip()) {
        const int stageCount = layout.getStageCount();
        const int activeSt   = std::clamp(layout.activeStageIdx, 0, stageCount - 1);

        const float startXRef = 34.0f;
        const float availWRef = 696.0f;
        const float gapRef    = 8.0f;
        const float tileYRef  = 58.0f;
        const float tileHRef  = 42.0f;
        const float tileWRef  = getSignalChainTileWidthRef(stageCount);
        const float stripRightRef = startXRef + static_cast<float>(stageCount) * tileWRef +
                                    static_cast<float>(stageCount - 1) * gapRef;

        // Horizontal signal-flow bus line across center of tile strip (y = 76.0f in ref coords)
        const Color themeAccentCol = getUiAccentColorValue(layout.theme.globalAccentColor);
        const Color stripBusCol    = HardwareMaterials::resolveChassisAccentInk(g, themeAccentCol);
        const float busY  = canvasRect.y + (tileYRef + tileHRef * 0.5f) * sy;
        const float busX0 = canvasRect.x + startXRef * sx;
        const float busX1 = canvasRect.x + std::min(startXRef + availWRef, stripRightRef) * sx;
        g.drawLine({ busX0, busY }, { busX1, busY }, stripBusCol.withAlpha(0.36f), 1.3f);

        // Small '▸' chevron arrows in the 8px gaps between tiles
        for (int s = 0; s + 1 < stageCount; ++s) {
            const float gapXRef = startXRef + static_cast<float>(s) * (tileWRef + gapRef) + tileWRef;
            const Rect<float> chevR = mapRect(gapXRef, tileYRef + 14.0f, gapRef, 16.0f);
            g.drawText("\xE2\x96\xB8", chevR, stripBusCol.withAlpha(0.75f), 8.5f, true, true);
        }

        for (int s = 0; s < stageCount; ++s) {
            const float tileXRef = startXRef + static_cast<float>(s) * (tileWRef + gapRef);
            const Rect<float> tileR = mapRect(tileXRef, tileYRef, tileWRef, tileHRef);
            const bool isSelected = (s == activeSt);

            std::string frameTitle = std::format("S{} · STAGE {}", s + 1, s + 1);
            int heritageFinishSlot = -1;
            bool stageEngaged      = true;
            bool stageIsParallel   = false;
            float stageParMixPct   = 100.0f;

            for (const auto& w : layout.widgets) {
                if (w.boundStageIdx == s) {
                    if (w.type == UiWidgetType::SectionFrame && !w.title.empty()) {
                        frameTitle         = w.title;
                        heritageFinishSlot = w.boundParamSlot;
                    } else if ((isSwitchWidget(w.type) || w.type == UiWidgetType::JewelLed) &&
                               (w.boundParamSlot == 110 || w.title == "STAGE IN" || w.subtitle == "IN|BYP")) {
                        stageEngaged = (w.previewNorm > 0.5f);
                    } else if (w.boundParamSlot == 109 || w.knobRole == UiKnobRole::ParallelMix || w.title == "PARALLEL LEVEL") {
                        stageIsParallel = true;
                        stageParMixPct  = std::clamp(w.previewNorm * 100.0f, 0.0f, 100.0f);
                    }
                }
            }

            const std::string desigHeader = std::format("S{}", s + 1);
            (void)frameTitle;
            (void)stageIsParallel;
            (void)stageParMixPct;

            const Color tileSkinBase = (!layout.theme.unifyFaceplateTheme && heritageFinishSlot >= 0)
                ? HardwareMaterials::getSectionPlateColor(static_cast<ChassisFinish>(heritageFinishSlot), false, 0)
                : sectionPlateBg;
            HardwareMaterials::drawRivetedChainTile(
                g, tileR, desigHeader, "",
                isSelected, stageEngaged, darkMeters, std::min(sx, sy),
                tileSkinBase, themeAccentCol);

            // Subtle dark-slate '[×]' delete button ONLY when showEditorOverlays == true (+ FACEPLATE UI BUILDER)
            if (showEditorOverlays && stageCount > 1) {
                const Rect<float> xBtn = mapRect(tileXRef + tileWRef - 16.5f, tileYRef + 3.5f, 12.5f, 12.5f);
                const bool xHov = xBtn.contains(hoverMousePos);
                g.fillRoundedRect(xBtn, 2.5f, xHov ? Color::fromHex(0x321218, 0.96f) : Color::fromHex(0x182026, 0.92f));
                g.drawRoundedRect(xBtn, 2.5f, xHov ? Color::fromHex(0xFF4D5A, 0.90f) : Color::fromHex(0x2E3C46, 0.70f), 1.0f);
                g.drawText("\xC3\x97", xBtn, xHov ? Color::fromHex(0xFF4D5A) : Color::fromHex(0xB8C4D0), 8.2f, true, true);
            }
        }
    }

    // 3. Two-Pass Widget Rendering: Frames & Grilles first, then Interactive Controls/Meters/Badges on top
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < layout.widgets.size(); ++i) {
            const auto& w = layout.widgets[i];
            if (!layout.isWidgetVisibleForActiveStage(w)) continue;
            const bool isFrame = (w.type == UiWidgetType::SectionFrame ||
                                  w.type == UiWidgetType::VentilationGrille ||
                                  w.type == UiWidgetType::RackHandlePair);
            if ((pass == 0 && !isFrame) || (pass == 1 && isFrame)) continue;

            const float effX = layout.getEffectiveWidgetX(w);

            Rect<float> wr = mapRect(effX, w.y, w.w, w.h);
            const Color accentCol = getUiAccentColorValue(w.accentColor);
            const float fontSizePt = getUiFontSizePt(w.fontSizePreset);
            const bool transparentMeter = false;

            switch (w.type) {
                case UiWidgetType::SectionFrame: {
                    const Color framePlateBg = (!layout.theme.unifyFaceplateTheme && w.boundStageIdx >= 0 && w.boundParamSlot >= 0)
                        ? HardwareMaterials::getSectionPlateColor(static_cast<ChassisFinish>(w.boundParamSlot), false, 0)
                        : sectionPlateBg;
                    const Color frameOutline = (w.accentColor == UiAccentColor::WarmAmber)
                        ? Color::fromHex(0x487A82, 0.42f)
                        : accentCol.withAlpha(0.55f);
                    const auto rawStyle = static_cast<SectionFrameStyle>(static_cast<int>(w.scalePreset) % 8);
                    const bool screwsEnabled = layout.theme.cornerScrews &&
                        (layout.theme.hardwareStyle != ChassisHardwareStyle::CleanScrewless);
                    const auto fStyle = (!screwsEnabled && rawStyle == SectionFrameStyle::RivetedSubPlate)
                        ? SectionFrameStyle::EngravedBevel
                        : rawStyle;
                    HardwareMaterials::drawSectionFrame(g, wr, w.title, framePlateBg, frameOutline, 11.5f, fStyle, layout.theme.screwStyle);
                    break;
                }

                case UiWidgetType::VentilationGrille:
                    HardwareMaterials::drawVentilationGrille(g, wr, w.title);
                    break;

                case UiWidgetType::RackHandlePair:
                    HardwareMaterials::drawRackHandlePair(g, wr, w.title);
                    break;

                case UiWidgetType::PatchbayJackSocket:
                    HardwareMaterials::drawPatchbayJackSocket(g, wr, w.title, accentCol);
                    break;

                case UiWidgetType::PointerKnob:
                case UiWidgetType::SkirtedCapKnob:
                case UiWidgetType::ConcentricEqKnob:
                case UiWidgetType::MarconiRedKnob:
                case UiWidgetType::ModernArcKnob:
                case UiWidgetType::BakeliteMasterKnob:
                case UiWidgetType::ColorCapConsoleKnob:
                case UiWidgetType::ChickenHeadKnob:
                case UiWidgetType::FlatVectorKnob:
                case UiWidgetType::GoldDomeKnob:
                case UiWidgetType::MacroHaloKnob:
                case UiWidgetType::KnurledBarrelKnob:
                case UiWidgetType::ColletBroadcastKnob:
                case UiWidgetType::StarburstSynthKnob:
                case UiWidgetType::GrimyRelicKnob:
                case UiWidgetType::SteppedAttenuatorKnob:
                case UiWidgetType::CyberLuxeHaloKnob:
                case UiWidgetType::NeumorphicGlassKnob:
                case UiWidgetType::SciFiHudRingKnob:
                case UiWidgetType::ValveWheelRelicKnob:
                case UiWidgetType::SubmarineHelmKnob:
                case UiWidgetType::TattooFlashKnob:
                case UiWidgetType::MetalSkullKnob:
                case UiWidgetType::FlowerDaisyKnob:
                case UiWidgetType::ReaperCryptKnob:
                case UiWidgetType::HellfireMagmaKnob:
                case UiWidgetType::ApocalypseCogKnob:
                case UiWidgetType::PunkSpikedKnob:
                case UiWidgetType::MeadowWoodSliceKnob:
                case UiWidgetType::SkySunburstKnob:
                case UiWidgetType::StormRaindropKnob:
                case UiWidgetType::GlacialIceCrystalKnob: {
                    bool firstOrange = false;
                    bool lastOrange  = false;
                    auto labels = !w.customTickLabels.empty()
                        ? w.customTickLabels
                        : getScalePresetLabels(w.scalePreset, firstOrange, lastOrange, w.title);
                    if (!w.customTickLabels.empty()) {
                        firstOrange = (labels.front() == "OFF" || labels.front() == "-\xE2\x88\x9E" || labels.front() == "-INF");
                        lastOrange  = (labels.back() == "ALL" || labels.back() == "MAX");
                    }
                    HardwareKnobStyle kStyle = HardwareKnobStyle::PointerSkirted;
                    if (w.type == UiWidgetType::SkirtedCapKnob) {
                        kStyle = HardwareKnobStyle::SkirtedCap;
                    } else if (w.type == UiWidgetType::ConcentricEqKnob) {
                        kStyle = HardwareKnobStyle::ConcentricDual;
                    } else if (w.type == UiWidgetType::MarconiRedKnob) {
                        kStyle = HardwareKnobStyle::MarconiRed;
                    } else if (w.type == UiWidgetType::ModernArcKnob) {
                        kStyle = HardwareKnobStyle::ModernArc;
                    } else if (w.type == UiWidgetType::BakeliteMasterKnob) {
                        kStyle = HardwareKnobStyle::BakeliteMaster;
                    } else if (w.type == UiWidgetType::ColorCapConsoleKnob) {
                        kStyle = HardwareKnobStyle::ColorCapConsole;
                    } else if (w.type == UiWidgetType::ChickenHeadKnob) {
                        kStyle = HardwareKnobStyle::ChickenHead;
                    } else if (w.type == UiWidgetType::FlatVectorKnob) {
                        kStyle = HardwareKnobStyle::FlatVector;
                    } else if (w.type == UiWidgetType::GoldDomeKnob) {
                        kStyle = HardwareKnobStyle::GoldDome;
                    } else if (w.type == UiWidgetType::MacroHaloKnob) {
                        kStyle = HardwareKnobStyle::MacroHalo;
                    } else if (w.type == UiWidgetType::KnurledBarrelKnob) {
                        kStyle = HardwareKnobStyle::KnurledBlackBarrel;
                    } else if (w.type == UiWidgetType::ColletBroadcastKnob) {
                        kStyle = HardwareKnobStyle::ColletBroadcast;
                    } else if (w.type == UiWidgetType::StarburstSynthKnob) {
                        kStyle = HardwareKnobStyle::StarburstSynth;
                    } else if (w.type == UiWidgetType::GrimyRelicKnob) {
                        kStyle = HardwareKnobStyle::GrimyRelic;
                    } else if (w.type == UiWidgetType::SteppedAttenuatorKnob) {
                        kStyle = HardwareKnobStyle::SteppedAttenuator;
                    } else if (w.type == UiWidgetType::CyberLuxeHaloKnob) {
                        kStyle = HardwareKnobStyle::CyberLuxeHalo;
                    } else if (w.type == UiWidgetType::NeumorphicGlassKnob) {
                        kStyle = HardwareKnobStyle::NeumorphicGlass;
                    } else if (w.type == UiWidgetType::SciFiHudRingKnob) {
                        kStyle = HardwareKnobStyle::SciFiHudRing;
                    } else if (w.type == UiWidgetType::ValveWheelRelicKnob) {
                        kStyle = HardwareKnobStyle::ValveWheelRelic;
                    } else if (w.type == UiWidgetType::SubmarineHelmKnob) {
                        kStyle = HardwareKnobStyle::SubmarineHelm;
                    } else if (w.type == UiWidgetType::TattooFlashKnob) {
                        kStyle = HardwareKnobStyle::TattooFlash;
                    } else if (w.type == UiWidgetType::MetalSkullKnob) {
                        kStyle = HardwareKnobStyle::MetalSkull;
                    } else if (w.type == UiWidgetType::FlowerDaisyKnob) {
                        kStyle = HardwareKnobStyle::FlowerDaisy;
                    } else if (w.type == UiWidgetType::ReaperCryptKnob) {
                        kStyle = HardwareKnobStyle::ReaperCrypt;
                    } else if (w.type == UiWidgetType::HellfireMagmaKnob) {
                        kStyle = HardwareKnobStyle::HellfireMagma;
                    } else if (w.type == UiWidgetType::ApocalypseCogKnob) {
                        kStyle = HardwareKnobStyle::ApocalypseCog;
                    } else if (w.type == UiWidgetType::PunkSpikedKnob) {
                        kStyle = HardwareKnobStyle::PunkSpiked;
                    } else if (w.type == UiWidgetType::MeadowWoodSliceKnob) {
                        kStyle = HardwareKnobStyle::MeadowWoodSlice;
                    } else if (w.type == UiWidgetType::SkySunburstKnob) {
                        kStyle = HardwareKnobStyle::SkySunburst;
                    } else if (w.type == UiWidgetType::StormRaindropKnob) {
                        kStyle = HardwareKnobStyle::StormRaindrop;
                    } else if (w.type == UiWidgetType::GlacialIceCrystalKnob) {
                        kStyle = HardwareKnobStyle::GlacialIceCrystal;
                    }
                    HardwareKnob::drawRotaryVisual(
                        g, wr, kStyle, w.previewNorm,
                        w.title, darkMeters ? "" : w.subtitle, "", labels,
                        firstOrange, lastOrange,
                        w.type == UiWidgetType::ConcentricEqKnob,
                        0.65f,
                        accentCol,
                        layout.theme.useCustomKnobColors,
                        layout.theme.knobBodyHex,
                        layout.theme.knobCapHex,
                        layout.theme.knobIndicatorHex,
                        layout.theme.knobTickStyle,
                        layout.theme.labelPlaqueStyle,
                        w.hasWaxMark,
                        w.waxMarkNorm);
                    break;
                }

                case UiWidgetType::ChromeToggleSwitch: {
                    std::string topLbl = "IN";
                    std::string botLbl = "OUT";
                    const auto pipePos = w.subtitle.find('|');
                    if (pipePos != std::string::npos) {
                        topLbl = w.subtitle.substr(0, pipePos);
                        botLbl = w.subtitle.substr(pipePos + 1);
                    }
                    const auto secondPipe = botLbl.find('|');
                    if (secondPipe != std::string::npos) {
                        const std::string midLbl = botLbl.substr(0, secondPipe);
                        botLbl = botLbl.substr(secondPipe + 1);
                        const int pos012 = static_cast<int>(std::round(w.previewNorm * 2.0f));
                        HardwareToggleSwitch::drawToggle3WayVisual(
                            g, wr, pos012, w.title, topLbl, midLbl, botLbl, true, accentCol, layout.theme.labelPlaqueStyle);
                    } else {
                        if (topLbl == "MONO (PURE)" && botLbl == "STEREO") {
                            topLbl = "STEREO";
                            botLbl = "MONO (PURE)";
                        }
                        const bool swIsUp = (w.previewNorm >= 0.5f);
                        HardwareToggleSwitch::drawToggleVisual(
                            g, wr, swIsUp, w.title, topLbl, botLbl, true, accentCol, layout.theme.labelPlaqueStyle);
                    }
                    break;
                }

                case UiWidgetType::IlluminatedPushButton: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : w.subtitle;
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawIlluminatedPushButton(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::RockerPowerSwitch: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawRockerPowerSwitch(
                        g, wr, swIsOn, w.title, accentCol, subPair, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::LockingCageToggle: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ARM|SAFE" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawLockingCageToggle(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::FlatPillVectorSwitch: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawFlatPillVectorSwitch(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::NeumorphicPillButton: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawNeumorphicPillSwitch(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::SciFiHologramSwitch: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawSciFiHologramSwitch(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::NavalBrassToggle: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawNavalBrassToggle(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::PunkSafetyPinToggle: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawPunkSafetyPinToggle(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::HellfireEmberSwitch: {
                    const std::string subPair = (w.subtitle == "MONO (PURE)|STEREO") ? "STEREO|MONO (PURE)" : (w.subtitle.empty() ? "ON|OFF" : w.subtitle);
                    const bool swIsOn = (w.previewNorm >= 0.5f);
                    HardwareMaterials::drawHellfireEmberSwitch(
                        g, wr, swIsOn, w.title, subPair, accentCol, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::VerticalStudioFader: {
                    bool firstOr = false, lastOr = false;
                    const std::vector<std::string> faderTicks = !w.customTickLabels.empty()
                        ? w.customTickLabels
                        : getScalePresetLabels(w.scalePreset, firstOr, lastOr, w.title);
                    HardwareMaterials::drawStudioFader(
                        g, wr, w.previewNorm, w.title, darkMeters ? "" : w.subtitle, accentCol, faderTicks, layout.theme.labelPlaqueStyle);
                    break;
                }

                case UiWidgetType::HorizontalCrossfader: {
                    HardwareMaterials::drawHorizontalCrossfader(
                        g, wr, w.previewNorm, w.title, darkMeters ? "" : w.subtitle, accentCol);
                    break;
                }

                case UiWidgetType::JewelLed: {
                    HardwareMaterials::drawJewelLed(
                        g, wr.getCentre(), !darkMeters && (w.previewNorm > 0.5f), accentCol,
                        std::min(wr.width, wr.height) * 0.28f);
                    if (!w.title.empty()) {
                        g.drawText(w.title, { wr.x - 16.0f, wr.bottom() - 12.0f, wr.width + 32.0f, 12.0f },
                                   g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2)), std::min(10.0f, fontSizePt * 0.78f), true, true);
                    }
                    break;
                }

                case UiWidgetType::AnalogVuMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color vuAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    AnalogVuMeter::drawVuVisual(
                        g, wr, effLiveGrDb, w.title,
                        w.subtitle.empty() ? "AVRA AUDIO" : w.subtitle,
                        effLiveOutDb, vuAccent, layout.theme.meterFaceplateHex, transparentMeter);
                    break;
                }

                case UiWidgetType::RoundVintageVuMeter:
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    HardwareMaterials::drawRoundVintageVu(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, accentCol, transparentMeter);
                    break;

                case UiWidgetType::LedBargraphMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const float effOutDbR = (effLiveOutDbR <= -900.0f) ? effLiveOutDb : effLiveOutDbR;
                    const float normL = (effLiveOutDb <= -58.0f) ? 0.0f : std::clamp((effLiveOutDb + 36.0f) / 39.0f, 0.0f, 1.0f);
                    const float normR = (effOutDbR <= -58.0f) ? 0.0f : std::clamp((effOutDbR + 36.0f) / 39.0f, 0.0f, 1.0f);
                    HardwareMaterials::drawLedPeakBargraph(g, wr, normL, w.title, normR);
                    break;
                }

                case UiWidgetType::DigitalTrimBargraph:
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    HardwareMaterials::drawDigitalTrimBargraph(g, wr, effLiveInDb, effLiveOutDb);
                    break;

                case UiWidgetType::GlowingTubeWindow: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const float stageDriveDb = (!layout.isMasterMeterWidget(w) && w.boundStageIdx >= 0)
                        ? (w.previewNorm * 18.0f)
                        : effLiveGrDb;
                    HardwareMaterials::drawGlowingTubeWindow(
                        g, wr, effLiveOutDb, std::max(effLiveGrDb, stageDriveDb), w.title, accentCol);
                    break;
                }

                case UiWidgetType::CrtOscilloscopeScreen: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color crtAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawCrtOscilloscopeScreen(
                        g, wr, effLiveOutDb, effLiveGrDb, w.title, crtAccent);
                    break;
                }

                case UiWidgetType::StereoGoniometerScope:
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    HardwareMaterials::drawStereoGoniometerScope(
                        g, wr, effLiveOutDb, effLiveGrDb, w.title, accentCol);
                    break;

                case UiWidgetType::LcdParameterReadout:
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    HardwareMaterials::drawLcdParameterReadout(
                        g, wr, effLiveInDb, effLiveOutDb, effLiveGrDb, w.title, w.subtitle, accentCol);
                    break;

                case UiWidgetType::NixieTubeDisplay: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color nixieAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawNixieTubeDisplay(
                        g, wr, effLiveOutDb, effLiveGrDb, w.title, w.subtitle, nixieAccent);
                    break;
                }

                case UiWidgetType::PastelShaderCurveMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color shaderAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawPastelShaderCurveMeter(
                        g, wr, effLiveOutDb, effLiveGrDb, w.title, shaderAccent,
                        layout.theme.meterFaceplateHex != 0 ? layout.theme.meterFaceplateHex : 0x10141Eu);
                    break;
                }

                case UiWidgetType::CyberLuxeMultiBarMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    const float effOutDbR = (effLiveOutDbR <= -900.0f) ? effLiveOutDb : effLiveOutDbR;
                    const float normL = (effLiveOutDb <= -58.0f) ? 0.65f : math::clamp((effLiveOutDb + 36.0f) / 39.0f, 0.0f, 1.0f);
                    const float normR = (effOutDbR <= -58.0f) ? 0.61f : math::clamp((effOutDbR + 36.0f) / 39.0f, 0.0f, 1.0f);
                    HardwareMaterials::drawCyberLuxeMultiBarMeter(
                        g, wr, normL, w.title, normR, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::NeumorphicDualArcMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawNeumorphicDualArcMeter(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::SubmarinePortholeGauge: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawSubmarinePortholeGauge(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::ReaperScytheArchMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawReaperScytheArchMeter(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::HellfireHornedVuMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawHellfireHornedVuMeter(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::ApocalypseGeigerVuMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawApocalypseGeigerVuMeter(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::PunkAnarchyVuMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawPunkAnarchyVuMeter(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::NatureElementalVuMeter: {
                    if (darkMeters) {
                        HardwareMaterials::drawPoweredOffMeterHousing(g, wr);
                        break;
                    }
                    const Color meterAccent = (layout.theme.meterGlowHex != 0 && layout.theme.unifyFaceplateTheme)
                        ? Color::fromHex(layout.theme.meterGlowHex)
                        : accentCol;
                    HardwareMaterials::drawNatureElementalVuMeter(
                        g, wr, effLiveGrDb, effLiveOutDb, w.title, meterAccent, transparentMeter);
                    break;
                }

                case UiWidgetType::RivetedNameplate: {
                    const auto pipePos = w.subtitle.find('|');
                    std::string subLine1 = (pipePos != std::string::npos) ? w.subtitle.substr(0, pipePos) : w.subtitle;
                    std::string subLine2 = !layout.theme.badgeFooterText.empty()
                        ? layout.theme.badgeFooterText
                        : ((pipePos != std::string::npos) ? w.subtitle.substr(pipePos + 1) : "CLASS-A EFFECTS");
                    if (isLegacyGenericBadgeSub1(subLine1) && subLine2 != "PREAMP · EQ · DIODE BRIDGE DYNAMICS") {
                        subLine1 = "CUSTOM CHASSIS";
                    }
                    if (isLegacyGenericBadgeFooter(subLine2)) {
                        subLine2 = "CLASS-A EFFECTS";
                    }
                    const std::string liveModelTitle = !w.title.empty()
                        ? w.title
                        : (!layout.theme.pluginTitle.empty() ? layout.theme.pluginTitle : "CUSTOM PLUGIN");
                    const auto bStyle = static_cast<BadgeVisualStyle>(static_cast<int>(w.scalePreset) % 6);
                    HardwareMaterials::drawRivetedNameplate(
                        g, wr,
                        "AVRA AUDIO",
                        liveModelTitle,
                        subLine1,
                        subLine2,
                        bStyle,
                        accentCol);
                    break;
                }

                case UiWidgetType::SilkscreenLabel: {
                    const Color lblCol = (w.accentColor == UiAccentColor::WarmAmber)
                        ? g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2))
                        : HardwareMaterials::resolveChassisAccentInk(g, accentCol);
                    HardwareMaterials::drawLabelPlaque(g, wr, w.title, layout.theme.labelPlaqueStyle, lblCol, fontSizePt, accentCol, true);
                    break;
                }

                case UiWidgetType::VintageSticker: {
                    const auto sPreset = static_cast<StickerPreset>(std::abs(static_cast<int>(w.scalePreset)) % gui::kStickerPresetCount);
                    const uint32_t cornerSeed = static_cast<uint32_t>(i * 2654435761u)
                                              ^ static_cast<uint32_t>(static_cast<uint32_t>(w.scalePreset) * 2246822519u)
                                              ^ static_cast<uint32_t>(w.title.size() * 374761393u);
                    HardwareMaterials::drawVintageSticker(
                        g, wr, sPreset, w.title, w.subtitle, accentCol, w.customImagePath, fontSizePt, w.rotationDeg, cornerSeed);
                    break;
                }
            }
        }

        // Apply control-aware relic weathering after pass 0 (under controls) and after pass 1 (over frames/bezels/screws)
        if (!drewBespokeBackdrop) {
            drawRelicControlWeathering(g, canvasRect, layout, pass);
        }
    }

    // Restore Studio UI typography before drawing editor selection overlay pill & returning
    g.resetSkinTypography();

    if (showEditorOverlays && selectedWidgetIdx >= 0 && selectedWidgetIdx < static_cast<int>(layout.widgets.size()) &&
        layout.isWidgetVisibleForActiveStage(layout.widgets[static_cast<size_t>(selectedWidgetIdx)])) {
        const auto& w = layout.widgets[static_cast<size_t>(selectedWidgetIdx)];
        Rect<float> wr = mapRect(layout.getEffectiveWidgetX(w), w.y, w.w, w.h);
        const bool isRotatableSticker = (w.type == UiWidgetType::VintageSticker);

        // 1. Coordinate/dimension readout pill positioned below the widget (or above the (R) handle near bottom edge)
        const std::string infoText = isRotatableSticker
            ? std::format("{} ({:.0f},{:.0f}) {:.0f}x{:.0f} [{:+.0f}°]", w.title, w.x, w.y, w.w, w.h, w.rotationDeg)
            : std::format("{} ({:.0f},{:.0f}) {:.0f}x{:.0f}", w.title, w.x, w.y, w.w, w.h);
        const float pillW = std::clamp(g.measureTextWidth(infoText, 8.2f, true) + 16.0f, 110.0f, 190.0f);
        float pillY = wr.bottom() + 10.0f;
        if (pillY + 20.0f > canvasRect.bottom() - 4.0f) {
            pillY = std::max(canvasRect.y + 2.0f, wr.y - 44.0f);
        }
        const float pillX = std::clamp(wr.getCentre().x - pillW * 0.5f,
                                       canvasRect.x + 4.0f,
                                       std::max(canvasRect.x + 4.0f, canvasRect.right() - pillW - 4.0f));
        const Rect<float> pill = { pillX, pillY, pillW, 18.0f };
        g.fillRoundedRect(pill, 3.5f, Color::fromHex(0x071216, 0.94f));
        g.drawRoundedRect(pill, 3.5f, Color::fromHex(0x00F5D4), 1.0f);
        g.drawText(infoText, pill, Color::fromHex(0x00F5D4), 8.2f, true, true);

        // 2. Topmost layer: Selection frame, (R) Rotate handle, [X] Delete button, and Bottom-Right Resize handle
        if (isRotatableSticker) {
            g.pushRotation(w.rotationDeg, wr.getCentre());
        }

        g.drawRoundedRect(wr, 4.0f, Color::fromHex(0x00F5D4), 1.8f);

        if (isRotatableSticker) {
            const float handleX = wr.getCentre().x;
            const float handleY = wr.y - 22.0f;
            g.drawLine({ handleX, wr.y }, { handleX, handleY }, Color::fromHex(0xFF9F1C), 1.8f);
            g.fillCircle({ handleX, handleY }, 7.5f, Color::fromHex(0x071216));
            g.drawCircle({ handleX, handleY }, 7.5f, Color::fromHex(0xFF9F1C), 1.8f);
            g.drawText("R", { handleX - 7.0f, handleY - 7.0f, 14.0f, 14.0f }, Color::fromHex(0xFF9F1C), 7.5f, true, true);
        }

        // Top-right quick-delete '[X]' button centered at { wr.right() - 2.0f, wr.y - 10.0f }
        // Omit '[X]' for permanent Metering / Master section widgets and the Badge
        if (!layout.isPermanentMasterSectionWidget(w)) {
            Rect<float> delBtn = { wr.right() - 10.0f, wr.y - 18.0f, 16.0f, 16.0f };
            g.fillRoundedRect(delBtn, 4.0f, Color::fromHex(0x280E14, 0.96f));
            g.drawRoundedRect(delBtn, 4.0f, Color::fromHex(0xFF4D5A), 1.4f);
            g.drawText("X", delBtn, Color::fromHex(0xFF4D5A), 8.5f, true, true);
        }

        // Bottom-right resize handle centered at { wr.right() - 5.0f, wr.bottom() - 5.0f }
        Rect<float> grip = { wr.right() - 10.0f, wr.bottom() - 10.0f, 10.0f, 10.0f };
        g.fillRect(grip, Color::fromHex(0x00F5D4));
        g.drawRect(grip, Color::fromHex(0x05080A), 1.0f);

        if (isRotatableSticker) {
            g.popRotation();
        }
    }
}

/**
 * @brief Renders a crisp 36x36px live visual thumbnail of any UI Widget Type inside the UI Widgets library list.
 */
inline void drawWidgetMiniThumbnail(
    gui::Graphics& g,
    gui::Rect<float> thumbRect,
    UiWidgetType type,
    gui::Color accent,
    UiScalePreset scalePreset = UiScalePreset::LinearDb) {
    using namespace avra::gui;
    g.fillRoundedRect(thumbRect, 4.0f, Color::fromHex(0x070B0F));
    g.drawRoundedRect(thumbRect, 4.0f, accent.withAlpha(0.35f), 1.0f);

    if (type == UiWidgetType::VintageSticker) {
        const auto sPreset = static_cast<StickerPreset>(std::abs(static_cast<int>(scalePreset)) % gui::kStickerPresetCount);
        const bool isStrip = (gui::getStickerShapeClass(sPreset) == gui::StickerShapeClass::SHAPE_PAPER_TAPE_RECT ||
                              sPreset == StickerPreset::EmbossedDymoStrip);
        const Rect<float> sr = isStrip
            ? Rect<float>{ thumbRect.x + 2.0f, thumbRect.y + 10.0f, thumbRect.width - 4.0f, thumbRect.height - 20.0f }
            : Rect<float>{ thumbRect.x + 3.0f, thumbRect.y + 3.0f, thumbRect.width - 6.0f, thumbRect.height - 6.0f };
        HardwareMaterials::drawVintageSticker(g, sr, sPreset, "", "", accent, "", 7.0f);
        return;
    }

    const Point<float> c = thumbRect.getCentre();
    const float r = std::min(thumbRect.width, thumbRect.height) * 0.34f;

    if (isKnobWidget(type)) {
        // Outer scale arc
        for (int t = 0; t < 7; ++t) {
            const float a = (-135.0f + static_cast<float>(t) * 45.0f - 90.0f) * 0.01745329f;
            const Point<float> p1 = { c.x + std::cos(a) * (r + 2.5f), c.y + std::sin(a) * (r + 2.5f) };
            const Point<float> p2 = { c.x + std::cos(a) * (r + 4.5f), c.y + std::sin(a) * (r + 4.5f) };
            g.drawLine(p1, p2, (t <= 4) ? accent : Color::fromHex(0x526470), 1.1f);
        }
        if (type == UiWidgetType::ChickenHeadKnob) {
            g.fillCircle(c, r * 0.72f, Color::fromHex(0x281B14));
            g.drawLine(c, { c.x + r * 0.75f, c.y - r * 0.75f }, accent, 3.2f);
        } else if (type == UiWidgetType::MarconiRedKnob) {
            g.fillCircle(c, r * 0.90f, Color::fromHex(0x8E2024));
            g.fillRoundedRect({ c.x - 2.5f, c.y - r * 0.85f, 5.0f, r * 1.4f }, 1.5f, Color::fromHex(0xB82B30));
            g.drawLine(c, { c.x, c.y - r * 0.85f }, Color::fromHex(0xFFFFFF), 1.4f);
        } else if (type == UiWidgetType::GoldDomeKnob) {
            g.fillCircle(c, r * 0.92f, Color::fromHex(0xB89128));
            g.fillCircle(c, r * 0.65f, Color::fromHex(0xE8C547));
            g.drawLine(c, { c.x + r * 0.55f, c.y - r * 0.55f }, Color::fromHex(0x14181E), 1.6f);
        } else if (type == UiWidgetType::ModernArcKnob || type == UiWidgetType::FlatVectorKnob || type == UiWidgetType::MacroHaloKnob) {
            g.drawCircle(c, r * 0.92f, accent, 2.0f);
            g.fillCircle(c, r * 0.68f, Color::fromHex(0x182028));
            g.drawLine(c, { c.x + r * 0.55f, c.y - r * 0.55f }, accent, 1.8f);
        } else {
            g.fillCircle(c, r * 0.92f, Color::fromHex(0x222930));
            g.drawCircle(c, r * 0.92f, Color::fromHex(0x4A5866), 1.0f);
            g.fillCircle(c, r * 0.60f, (type == UiWidgetType::ColorCapConsoleKnob) ? accent : Color::fromHex(0xC8D2DC));
            g.drawLine(c, { c.x + r * 0.62f, c.y - r * 0.62f }, accent, 1.8f);
        }
    } else if (isSwitchWidget(type) || isFaderWidget(type)) {
        if (type == UiWidgetType::VerticalStudioFader) {
            g.drawLine({ c.x, thumbRect.y + 5.0f }, { c.x, thumbRect.bottom() - 5.0f }, Color::fromHex(0x3A4652), 2.0f);
            g.fillRoundedRect({ c.x - 6.0f, c.y - 4.0f, 12.0f, 8.0f }, 2.0f, accent);
        } else if (type == UiWidgetType::HorizontalCrossfader) {
            g.drawLine({ thumbRect.x + 5.0f, c.y }, { thumbRect.right() - 5.0f, c.y }, Color::fromHex(0x3A4652), 2.0f);
            g.fillRoundedRect({ c.x - 4.0f, c.y - 6.0f, 8.0f, 12.0f }, 2.0f, accent);
        } else if (type == UiWidgetType::IlluminatedPushButton || type == UiWidgetType::FlatPillVectorSwitch) {
            g.fillRoundedRect({ c.x - 9.0f, c.y - 6.0f, 18.0f, 12.0f }, 3.0f, accent.withAlpha(0.35f));
            g.drawRoundedRect({ c.x - 9.0f, c.y - 6.0f, 18.0f, 12.0f }, 3.0f, accent, 1.2f);
        } else {
            g.fillCircle(c, 5.0f, Color::fromHex(0x8896A4));
            g.drawLine(c, { c.x, c.y - 9.0f }, Color::fromHex(0xE2ECF4), 2.6f);
            g.fillCircle({ c.x, c.y - 9.0f }, 2.5f, accent);
        }
    } else if (isMeterWidget(type) || type == UiWidgetType::JewelLed) {
        if (type == UiWidgetType::AnalogVuMeter || type == UiWidgetType::RoundVintageVuMeter) {
            g.fillRoundedRect({ thumbRect.x + 4.0f, thumbRect.y + 6.0f, thumbRect.width - 8.0f, thumbRect.height - 12.0f }, 2.5f, Color::fromHex(0xF2E4C2));
            g.drawLine({ c.x, thumbRect.bottom() - 7.0f }, { c.x + 5.0f, thumbRect.y + 9.0f }, Color::fromHex(0xC82828), 1.5f);
        } else if (type == UiWidgetType::LedBargraphMeter || type == UiWidgetType::DigitalTrimBargraph) {
            for (int seg = 0; seg < 4; ++seg) {
                const float sy = thumbRect.bottom() - 7.0f - static_cast<float>(seg) * 5.5f;
                const Color sc = (seg == 3) ? Color::fromHex(0xFF4D5A) : ((seg == 2) ? Color::fromHex(0xFF9F1C) : Color::fromHex(0x52B788));
                g.fillRect({ c.x - 7.0f, sy, 6.0f, 3.8f }, sc);
                g.fillRect({ c.x + 1.0f, sy, 6.0f, 3.8f }, sc);
            }
        } else if (type == UiWidgetType::GlowingTubeWindow) {
            g.fillCircle(c, r * 0.85f, Color::fromHex(0xFF6B2B, 0.35f));
            g.fillCircle(c, r * 0.45f, Color::fromHex(0xFF9F1C));
        } else if (type == UiWidgetType::JewelLed) {
            g.fillCircle(c, 6.0f, accent);
        } else {
            // CRT / Goniometer / Nixie / Shader
            g.drawLine({ thumbRect.x + 5.0f, c.y }, { c.x - 2.0f, c.y - 6.0f }, accent, 1.4f);
            g.drawLine({ c.x - 2.0f, c.y - 6.0f }, { c.x + 3.0f, c.y + 6.0f }, accent, 1.4f);
            g.drawLine({ c.x + 3.0f, c.y + 6.0f }, { thumbRect.right() - 5.0f, c.y }, accent, 1.4f);
        }
    } else {
        // Frame / Badge / Hardware
        g.drawRoundedRect({ thumbRect.x + 5.0f, thumbRect.y + 7.0f, thumbRect.width - 10.0f, thumbRect.height - 14.0f }, 2.0f, accent, 1.3f);
        g.drawLine({ thumbRect.x + 8.0f, c.y }, { thumbRect.right() - 8.0f, c.y }, accent.withAlpha(0.65f), 1.0f);
    }
}

} // namespace avra::studio
