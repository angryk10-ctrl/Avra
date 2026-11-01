#pragma once

#include "avra/core/Parameter.h"
#include "avra/gui/Component.h"
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace avra::gui {

/**
 * @brief Surface finish presets for Avra Hardware Faceplates (14 studio,
 * vintage & modern finishes).
 */
enum class ChassisFinish : int {
    AnodizedTeal = 0,       // Classic Petrol/Teal Brushed Anodized Aluminum
    BritishSlateBlue = 1,   // Classic Console Slate Blue-Grey
    ObsidianBlack = 2,      // Mastering Grade Black Anodized
    BrushedSilver = 3,      // Vintage Natural Brushed Aluminum
    VintageGold = 4,        // Classic Warm Champagne / Gold Anodized
    HammertoneGreen = 5,    // Vintage Military / Tube Hammertone Olive-Green
    VintageCream = 6,       // Classic Warm Retro Enamel Cream / Ivory
    CrimsonMastering = 7,   // Deep Anodized Burgundy / Mastering Crimson
    CobaltStudioBlue = 8,   // Deep Electric Cobalt Studio Blue
    WarmCopperBronze = 9,   // Boutique Brushed Copper-Bronze
    SunburstAmberWood = 10, // Warm Carved Teak / Sunburst Amber Wood Faceplate
    RoyalNavyBlue = 11,     // Deep Satin Navy Rack Enclosure
    CyberpunkViolet = 12,   // Deep Synth Indigo / Cyberpunk Violet
    ArcticWhiteEnamel = 13  // Clean Lab White / Broadcast Silver-White Enamel
};

enum class SurfaceGrain : int {
    VerticalBrushed = 0,
    HorizontalBrushed = 1,
    MatteSmooth = 2,
    CarbonWeave = 3,
    WrinklePowderCoat = 4,
    DiamondCrosshatch = 5,
    BeadBlastedMatte = 6,
    HammertoneDimpled = 7,
    WalnutWoodCheeks = 8,
    PerforatedHexMesh = 9,
    FullCarvedWoodGrain = 10,
    TolexAmpLeather = 11,
    GrimyRoadWornRelic = 12, // Distressed road-worn relic with edge paint wear,
                             // scratches & studio grime
    OxidizedPatina = 13,     // Aged oxidized copper/verdigris & mottled industrial patina
    SleekGlassAcrylic = 14,  // Hyper-modern dark smoked glass with diagonal
                             // specular reflection sheen
    TitaniumMicroGrid = 15,  // Precision CNC laser-etched micro-grid titanium plate
    DiamondTreadplate = 16,  // Raised 3D industrial diamond treadplate metal lozenges
    WovenTweed = 17          // Vintage lacquered diagonal woven tweed amplifier cloth
};

enum class ChassisHardwareStyle : int {
    CornerScrews4 = 0,      // 4 Corner Bolts
    RackEarsAndBolts = 1,   // 19" Studio Rack Ears + Slotted Rack Holes + 6 Bolts
    IndustrialRivets8 = 2,  // 8 Perimeter Bolts along Top & Bottom Rails
    HeavyPerimeter12 = 3,   // 12 Heavy Enclosure Bolts around all 4 edges
    CornerGussetPlates = 4, // 4 Bevelled Corner Gusset Plates + Fasteners
    CleanScrewless = 5,     // Clean Modern Screwless Bezel
    LeatherAmpCorners = 6   // 4 Stitched Leather / Black-Oxide Amplifier Corner
                            // Protectors with Dual Rivets
};

enum class ScrewHeadStyle : int {
    PhillipsCross = 0,     // Classic #2 Cross-Recessed Studio Screw
    SocketHexAllen = 1,    // CNC Machined 6-Sided Allen Hex Socket Bolt
    SlottedFlathead = 2,   // Vintage Single-Slot Flathead Screw
    TorxStar6 = 3,         // Modern Boutique 6-Lobe Star / Torx Pin
    GoldPlatedBrass = 4,   // Warm Polished 24K Gold / Brass Fastener
    BlackOxideStealth = 5, // Murdered-Out Matte Black Oxide Socket Bolt
    DomedChromeRivet = 6,  // Mirror-Polished Domed Chrome Rivet
    CountersunkFlush = 7,  // Dual-Ring Anodized Countersunk Collar Bolt
    KnurledThumbscrew = 8, // Tall Fluted Studio Rack Thumbscrew
    RustedRelicBolt = 9    // Oxidized Road-Worn Relic Bolt with Rust Halo
};

enum class SectionFrameStyle : int {
    EngravedBevel = 0,   // Classic recessed dark engraved border + top title pill
    RivetedSubPlate = 1, // Recessed sub-panel plate with 4 corner rivets & metallic bevel
    DoubleHairline = 2,  // Precision dual-line silkscreen border + boxed title pill
    HeavyConsoleBay = 3, // Thick console channel module border with shaded top banner bar
    TechnicalCorner = 4, // Technical L-bracket corner markers + top silkscreen rule
    MinimalHeader = 5,   // Clean top horizontal rule + centered header without side/bottom box
    PipingTolex = 6,     // Vintage amplifier gold/cream piping bead border with
                         // recessed tolex bay
    GothicArch = 7       // Dark-art pointed cathedral arch header border with ornate
                         // corner finials
};

enum class KnobTickMarkStyle : int {
    RadialLines = 0,   // Classic studio radial tick lines
    Dots = 1,          // Circular metallic/glowing dot markers around dial perimeter
    Numbered0To10 = 2, // Full 0..10 numbered amplifier/console perimeter scale
    GlowArc = 3,       // Continuous modern LED glow ring arc around dial
    None = 4           // Clean frameless / tickless minimalist dial
};

enum class UiLabelPlaqueStyle : int {
    DirectSilkscreen = 0, // Crisp direct-on-chassis silkscreen ink
    EmbossedDymoTape = 1, // 3D raised glossy plastic Dymo label tape strip with
                          // punched white text
    TraffolytePlaque = 2, // 2-ply engraved phenolic Traffolyte plaque with micro-rivets
    GlowingLaserEtch = 3  // Dark smoked-glass capsule with neon laser-etched underline glow
};

enum class BadgeVisualStyle : int {
    BrushedAluminumPlate = 0, // Classic 4-rivet brushed silver plate
    CastBrassPlaque = 1,      // Recessed dark bronze/black plaque with 24K gold/brass
                              // border & rivets
    SleekLaserGlass = 2,      // Modern dark glass pill with glowing neon accent border
    GrimyMilitaryTag = 3,     // Distressed stenciled black-oxide dog-tag with chipped edges
    VintageCreamEmblem = 4,   // Warm vintage ivory/cream enamel plate with dark brass trim
    MinimalSilkscreen = 5     // Frameless direct-on-chassis precision silkscreen branding stamp
};

/**
 * @brief Shape-aware rendering classification for Faceplate Stickers, Tape
 * Strips, Vinyl Decals & Rigid Plaques.
 */
enum class StickerShapeClass : int {
    SHAPE_PAPER_TAPE_RECT = 0,     // Rectangular tape/paper strips with top-right
                                   // dog-ear corner peel & torn left/right edges
    SHAPE_ROUND_VINYL_DECAL = 1,   // Circular die-cut white/cream vinyl border (3px) + top-hemisphere
                                   // vinyl gloss arc + circular drop shadow (NO corner peel)
    SHAPE_RIGID_PASS_OR_PLAQUE = 2 // Rigid plastic passes, metal plaques, or embossed Dymo tape with clean
                                   // rounded corners, crisp bevel edge & drop shadow (NO corner peel)
};

/**
 * @brief 32 Studio Engineering Stickers, Tape Strips, Vinyl Badges, Plaques &
 * Custom Image Decals for Faceplate UI Builder.
 */
enum class StickerPreset : int {
    GrungeSmiley = 0,          // 1. Smiley Face ("HAVE A NICE CLIP" / "ANALOG VIBES") —
                               // SHAPE_ROUND_VINYL_DECAL
    SkullAndBones = 1,         // 2. Skull & Bones ("DANGER // LOUD" / "HIGH GAIN") —
                               // SHAPE_ROUND_VINYL_DECAL
    RockStarEmblem = 2,        // 3. Gold Star ("GOLD STAR") — SHAPE_ROUND_VINYL_DECAL
    ScribbleMaskingTape = 3,   // 4. Console Scribble Tape ("VOCAL COMP +3dB" / "DO
                               // NOT RESET") — SHAPE_PAPER_TAPE_RECT
    BlackGaffTape = 4,         // 5. Stage Gaff Tape ("DO NOT TOUCH!") — SHAPE_PAPER_TAPE_RECT
    EmbossedDymoStrip = 5,     // 6. Embossed Label Tape ("MASTERING UNIT") —
                               // SHAPE_RIGID_PASS_OR_PLAQUE
    HighVoltageBolt = 6,       // 7. High Voltage ("HIGH VOLTAGE" / "450V B+ PLATE") —
                               // SHAPE_RIGID_PASS_OR_PLAQUE
    RadioactiveTrefoil = 7,    // 8. Radioactive ("RADIOACTIVE" / "HOT SIGNAL") —
                               // SHAPE_ROUND_VINYL_DECAL
    YinYangCircle = 8,         // 9. Balance - Zen ("BALANCE" / "ZEN") — SHAPE_ROUND_VINYL_DECAL
    RetroPeaceSign = 9,        // 10. Analog Peace ("ANALOG PEACE" / "1972 SOUND") —
                               // SHAPE_ROUND_VINYL_DECAL
    MagicEightBall = 10,       // 11. 8-Ball Says - No! ("8-BALL SAYS" / "NO!") —
                               // SHAPE_ROUND_VINYL_DECAL
    AdvisoryExplicit = 11,     // 12. Studio Advisory ("ADVISORY" / "EXPLICIT THD") —
                               // SHAPE_RIGID_PASS_OR_PLAQUE
    BackstageVipPass = 12,     // 13. VIP Backstage ("WORLD TOUR '94" / "FOH
                               // ENGINEER") — SHAPE_RIGID_PASS_OR_PLAQUE
    FragileFlightStamp = 13,   // 14. Fragile Stamp ("FRAGILE" / "VACUUM TUBES") —
                               // SHAPE_RIGID_PASS_OR_PLAQUE
    AlienHeadDecal = 14,       // 15. Area 51 - Spacey! ("AREA 51" / "SPACEY!") —
                               // SHAPE_ROUND_VINYL_DECAL
    BiohazardWarning = 15,     // 16. Biohazard ("BIOHAZARD" / "TOXIC BASS") —
                               // SHAPE_ROUND_VINYL_DECAL
    FixItInTheMix = 16,        // 17. Fix It In The Mix ("FIX IT IN THE MIX") — SHAPE_PAPER_TAPE_RECT
    ClientKnobUnplugged = 17,  // 18. Client Knob - Unplugged ("CLIENT KNOB" /
                               // "UNPLUGGED") — SHAPE_ROUND_VINYL_DECAL
    PhaseCheckedMonoSafe = 18, // 19. Phase Checked - Mono Safe ("PHASE CHECKED" /
                               // "MONO SAFE") — SHAPE_ROUND_VINYL_DECAL
    EarFatigueFirstAid = 19,   // 20. Ear Fatigue - Take A Break ("EAR FATIGUE" /
                               // "TAKE A BREAK") — SHAPE_RIGID_PASS_OR_PLAQUE
    TwoInchReel30Ips = 20,     // 21. 2-Inch Reel - 30 IPS Master ("2-INCH REEL" / "30
                               // IPS MASTER") — SHAPE_ROUND_VINYL_DECAL
    SubBassSeismicZone = 21,   // 22. Sub Bass - Seismic Zone ("SUB BASS" / "SEISMIC
                               // ZONE") — SHAPE_RIGID_PASS_OR_PLAQUE
    CalCertified0Vu = 22,      // 23. Cal Certified - 0 VU = +4 dBu ("CAL CERTIFIED" /
                               // "0 VU = +4 dBu") — SHAPE_RIGID_PASS_OR_PLAQUE
    RawNoPitchCorr = 23,       // 24. 100% Raw - No Pitch Corr ("100% RAW" / "NO PITCH
                               // CORR") — SHAPE_ROUND_VINYL_DECAL
    ThreeAmEspressoFuel = 24,  // 25. 3 AM Session - Espresso Fuel ("3 AM SESSION"
                               // / "ESPRESSO FUEL") — SHAPE_ROUND_VINYL_DECAL
    PanicButtonMute = 25,      // 26. Panic Button - Mute If Suck ("PANIC BUTTON" /
                               // "MUTE IF SUCK") — SHAPE_ROUND_VINYL_DECAL
    TapeOpPrintIt = 26,        // 27. Tape Op - Print It! ("TAPE OP" / "PRINT IT!") —
                               // SHAPE_RIGID_PASS_OR_PLAQUE
    PhantomRibbonKiller = 27,  // 28. 48V Phantom - Ribbon Killer ("48V PHANTOM" /
                               // "RIBBON KILLER") — SHAPE_ROUND_VINYL_DECAL
    FinalMixV12Wav = 28,       // 29. FINAL_MIX_V12_REAL.WAV ("FINAL_MIX_V12_REAL.WAV")
                               // — SHAPE_PAPER_TAPE_RECT
    ClassASpaceHeater = 29,    // 30. Class-A - Space Heater ("CLASS-A" / "SPACE
                               // HEATER") — SHAPE_ROUND_VINYL_DECAL
    HeadroomWhatHeadroom = 30, // 31. Headroom - What Headroom? ("HEADROOM" /
                               // "WHAT HEADROOM?") — SHAPE_ROUND_VINYL_DECAL
    CustomImageDecal = 31      // 32. Custom Uploaded Image Decal — SHAPE_RIGID_PASS_OR_PLAQUE
};

inline constexpr int kStickerPresetCount = 32;

[[nodiscard]] constexpr StickerShapeClass getStickerShapeClass(StickerPreset p) noexcept {
    switch (p) {
    case StickerPreset::ScribbleMaskingTape:
    case StickerPreset::BlackGaffTape:
    case StickerPreset::FixItInTheMix:
    case StickerPreset::FinalMixV12Wav:
        return StickerShapeClass::SHAPE_PAPER_TAPE_RECT;

    case StickerPreset::EmbossedDymoStrip:
    case StickerPreset::HighVoltageBolt:
    case StickerPreset::AdvisoryExplicit:
    case StickerPreset::BackstageVipPass:
    case StickerPreset::FragileFlightStamp:
    case StickerPreset::EarFatigueFirstAid:
    case StickerPreset::SubBassSeismicZone:
    case StickerPreset::CalCertified0Vu:
    case StickerPreset::TapeOpPrintIt:
    case StickerPreset::CustomImageDecal:
        return StickerShapeClass::SHAPE_RIGID_PASS_OR_PLAQUE;

    case StickerPreset::GrungeSmiley:
    case StickerPreset::SkullAndBones:
    case StickerPreset::RockStarEmblem:
    case StickerPreset::RadioactiveTrefoil:
    case StickerPreset::YinYangCircle:
    case StickerPreset::RetroPeaceSign:
    case StickerPreset::MagicEightBall:
    case StickerPreset::AlienHeadDecal:
    case StickerPreset::BiohazardWarning:
    case StickerPreset::ClientKnobUnplugged:
    case StickerPreset::PhaseCheckedMonoSafe:
    case StickerPreset::TwoInchReel30Ips:
    case StickerPreset::RawNoPitchCorr:
    case StickerPreset::ThreeAmEspressoFuel:
    case StickerPreset::PanicButtonMute:
    case StickerPreset::PhantomRibbonKiller:
    case StickerPreset::ClassASpaceHeater:
    case StickerPreset::HeadroomWhatHeadroom:
    default:
        return StickerShapeClass::SHAPE_ROUND_VINYL_DECAL;
    }
}

/**
 * @brief Procedural Direct2D Hardware Material, Bezel, Screw, Jewel LED &
 * Nameplate Painters. Zero external PNG dependencies required — 100%
 * resolution-independent GPU vector/shader rendering.
 */
struct HardwareMaterials {
    static Color scaleColorHex(uint32_t hexRgb, float factor, float alpha = 1.0f) noexcept {
        const float r = std::clamp(static_cast<float>((hexRgb >> 16) & 0xFF) * factor, 0.0f, 255.0f) / 255.0f;
        const float g = std::clamp(static_cast<float>((hexRgb >> 8) & 0xFF) * factor, 0.0f, 255.0f) / 255.0f;
        const float b = std::clamp(static_cast<float>(hexRgb & 0xFF) * factor, 0.0f, 255.0f) / 255.0f;
        return {r, g, b, alpha};
    }

    /**
     * @brief Detects light/silver/cream/gold chassis templates where silkscreen
     * text resolves to dark ink.
     */
    static bool isLightChassis(const Graphics &g) noexcept {
        const Color silk = g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF));
        const float luma = 0.299f * silk.r + 0.587f * silk.g + 0.114f * silk.b;
        return luma < 0.38f;
    }

    static Color resolveWidgetPlateFill(const Graphics &g, float darkAlpha = 0.56f, float lightAlpha = 0.68f) noexcept {
        const bool lightCh = isLightChassis(g);
        const Color fallback = lightCh ? Color::fromHex(0xF8FAFC, lightAlpha) : Color::fromHex(0x0E161E, darkAlpha);
        const Color resolved = g.resolveTextPlateColor(fallback);
        if (resolved.a <= 0.01f) return resolved;
        const float luma = 0.299f * resolved.r + 0.587f * resolved.g + 0.114f * resolved.b;
        const float targetAlpha = (luma > 0.48f) ? lightAlpha : darkAlpha;
        return resolved.withAlpha(targetAlpha);
    }

    static bool isLightPlateFill(const Graphics &g, Color plateCol) noexcept {
        if (plateCol.a <= 0.05f) return isLightChassis(g);
        const float luma = 0.299f * plateCol.r + 0.587f * plateCol.g + 0.114f * plateCol.b;
        return luma > 0.48f;
    }

    static Color resolvePlateInkColor(const Graphics &g, Color plateCol) noexcept {
        const bool lightPlate = isLightPlateFill(g, plateCol);
        const Color silk = g.resolveSilkscreenColor(lightPlate ? Color::fromHex(0x0F172A) : Color::fromHex(0xF8FAFC));
        const float inkLuma = 0.299f * silk.r + 0.587f * silk.g + 0.114f * silk.b;
        if (plateCol.a > 0.08f) {
            if (lightPlate && inkLuma > 0.68f) return Color::fromHex(0x0F172A);
            if (!lightPlate && inkLuma < 0.28f) return Color::fromHex(0xF8FAFC);
        }
        return silk;
    }

    static Color resolvePlateReadoutInk(const Graphics &g, Color plateCol, Color accentCol) noexcept {
        const bool lightPlate = isLightPlateFill(g, plateCol);
        if (lightPlate) {
            return resolvePlateInkColor(g, plateCol);
        }
        const float tickLuma = 0.299f * accentCol.r + 0.587f * accentCol.g + 0.114f * accentCol.b;
        if (tickLuma < 0.45f) return resolvePlateInkColor(g, plateCol);
        return Color(std::clamp(accentCol.r * 1.15f + 0.14f, 0.0f, 1.0f),
                     std::clamp(accentCol.g * 1.15f + 0.12f, 0.0f, 1.0f),
                     std::clamp(accentCol.b * 1.15f + 0.10f, 0.0f, 1.0f), 1.0f);
    }

    /**
     * @brief Resolves warm orange/amber/accent colors to high-contrast Burnt
     * Orange (#C84B00) on light chassis, or Signal Orange / Warm Amber (#FF7A1A)
     * on dark/mid-tone chassis.
     */
    static Color resolveChassisAccentInk(const Graphics &g, Color accentCol) noexcept {
        const bool isWarm = (accentCol.r > accentCol.b + 0.20f && accentCol.r >= accentCol.g && accentCol.g >= accentCol.b * 0.72f);
        if (!isLightChassis(g)) {
            if (isWarm) { return Color::fromHex(0xFF7A1A, accentCol.a); }
            return {std::clamp(accentCol.r * 1.18f + 0.08f, 0.0f, 1.0f), std::clamp(accentCol.g * 1.18f + 0.08f, 0.0f, 1.0f),
                    std::clamp(accentCol.b * 1.18f + 0.08f, 0.0f, 1.0f), accentCol.a};
        }
        if (isWarm) { return Color::fromHex(0xC84B00, accentCol.a); }
        return {std::clamp(accentCol.r * 0.30f, 0.0f, 0.52f), std::clamp(accentCol.g * 0.30f, 0.0f, 0.44f),
                std::clamp(accentCol.b * 0.34f, 0.0f, 0.58f), accentCol.a};
    }

    /**
     * @brief Renders a high-contrast recessed dark micro-readout pill (#090D12)
     * with luminous gold/amber or accent typography so live readouts, switch
     * states, and callouts remain 100% readable across all 36 styles.
     */
    static void drawHighContrastReadoutPill(Graphics &g, Rect<float> zone, std::string_view text, float fontSize = 9.4f,
                                            Color pillTextCol = Color::fromHex(0xFFBE3B), float pillAlpha = 0.84f) noexcept {
        if (text.empty()) return;
        const float effFs = fontSize * g.getFontScale();
        const float pillW = std::clamp(static_cast<float>(text.size()) * (effFs * 0.64f) + 12.0f, 26.0f, zone.width - 2.0f);
        const float pillH = std::clamp(zone.height, 11.5f, 15.5f);
        const Rect<float> pill = {zone.x + (zone.width - pillW) * 0.5f, zone.y + (zone.height - pillH) * 0.5f, pillW, pillH};
        g.fillRoundedRect(pill, 3.0f, Color::fromHex(0x080C10, pillAlpha));
        g.drawRoundedRect(pill, 3.0f, Color::fromHex(0x2E3C46, pillAlpha * 0.75f), 0.8f);
        // Ensure text inside the dark pill is always bright & luminous (>10:1
        // contrast)
        const Color brightInk =
            (pillTextCol.r >= pillTextCol.g && pillTextCol.g >= pillTextCol.b * 0.70f)
                ? Color::fromHex(0xFFC442)
                : Color{std::clamp(pillTextCol.r * 1.25f + 0.18f, 0.35f, 1.0f), std::clamp(pillTextCol.g * 1.25f + 0.18f, 0.35f, 1.0f),
                        std::clamp(pillTextCol.b * 1.25f + 0.18f, 0.35f, 1.0f), 1.0f};
        g.drawText(text, pill, brightInk, fontSize, true, true);
    }

    /**
     * @brief Returns a 1px dark engraved text drop shadow across all hardware
     * themes.
     */
    static Color resolveChassisTextShadow(const Graphics &, float darkAlpha = 0.45f) noexcept {
        return Color::fromHex(0x000000, darkAlpha);
    }

    static uint32_t getPresetFinishBaseHex(ChassisFinish finish) noexcept {
        switch (finish) {
        case ChassisFinish::AnodizedTeal:
            return 0x245A5C;
        case ChassisFinish::BritishSlateBlue:
            return 0x2B3746;
        case ChassisFinish::BrushedSilver:
            return 0x8E969E;
        case ChassisFinish::VintageGold:
            return 0x4E4126;
        case ChassisFinish::HammertoneGreen:
            return 0x2B3E2E;
        case ChassisFinish::VintageCream:
            return 0x565042;
        case ChassisFinish::CrimsonMastering:
            return 0x4A1D24;
        case ChassisFinish::CobaltStudioBlue:
            return 0x1D3558;
        case ChassisFinish::WarmCopperBronze:
            return 0x4E3224;
        case ChassisFinish::SunburstAmberWood:
            return 0x8C4A18;
        case ChassisFinish::RoyalNavyBlue:
            return 0x162844;
        case ChassisFinish::CyberpunkViolet:
            return 0x2D1646;
        case ChassisFinish::ArcticWhiteEnamel:
            return 0xABB4BC;
        case ChassisFinish::ObsidianBlack:
        default:
            return 0x24282E;
        }
    }

    static void drawRackScrew(Graphics &g, Point<float> c, float r = 6.0f, ScrewHeadStyle style = ScrewHeadStyle::PhillipsCross) noexcept {
        // Recessed hole shadow
        g.fillCircle({c.x + 0.8f, c.y + 1.2f}, r + 1.2f, Color::fromHex(0x06080A, 0.85f));

        // Outer metallic head / washer material based on ScrewHeadStyle
        if (style == ScrewHeadStyle::GoldPlatedBrass) {
            g.fillRadialGradientCircle(c, r, Color::fromHex(0xF8E18C), Color::fromHex(0x7A5A18), {-r * 0.35f, -r * 0.35f});
            g.drawCircle(c, r, Color::fromHex(0x281C06), 1.0f);
        } else if (style == ScrewHeadStyle::BlackOxideStealth) {
            g.fillRadialGradientCircle(c, r, Color::fromHex(0x383E46), Color::fromHex(0x121519), {-r * 0.35f, -r * 0.35f});
            g.drawCircle(c, r, Color::fromHex(0x07090B), 1.0f);
        } else if (style == ScrewHeadStyle::DomedChromeRivet) {
            g.fillRadialGradientCircle(c, r, Color::fromHex(0xFAFDFF), Color::fromHex(0x46505A), {-r * 0.42f, -r * 0.42f});
            g.drawCircle(c, r, Color::fromHex(0x181C20), 0.9f);
            g.fillCircle({c.x - r * 0.28f, c.y - r * 0.28f}, r * 0.22f, Color::fromHex(0xFFFFFF, 0.55f));
            return;
        } else if (style == ScrewHeadStyle::KnurledThumbscrew) {
            g.fillCircle({c.x + 1.4f, c.y + 2.0f}, r * 1.18f, Color::fromHex(0x040608, 0.85f));
            g.fillRadialGradientCircle(c, r * 1.15f, Color::fromHex(0xD8E0E8), Color::fromHex(0x3E4650), {-r * 0.4f, -r * 0.4f});
            for (int k = 0; k < 12; ++k) {
                const float ka = static_cast<float>(k) * (6.2831853f / 12.0f);
                g.drawLine({c.x + std::cos(ka) * r * 0.82f, c.y + std::sin(ka) * r * 0.82f},
                           {c.x + std::cos(ka) * r * 1.14f, c.y + std::sin(ka) * r * 1.14f}, Color::fromHex(0x14181C, 0.75f), 1.1f);
            }
            g.drawCircle(c, r * 0.78f, Color::fromHex(0x222830), 1.0f);
            g.drawLine({c.x - r * 0.55f, c.y}, {c.x + r * 0.55f, c.y}, Color::fromHex(0x12161A), 1.6f);
            return;
        } else if (style == ScrewHeadStyle::RustedRelicBolt) {
            // Corrosion / rust halo around washer
            g.fillRadialGradientCircle(c, r * 1.55f, Color::fromHex(0x6E3214, 0.45f), Color::fromHex(0x3A1808, 0.0f));
            g.fillRadialGradientCircle(c, r, Color::fromHex(0x7A5438), Color::fromHex(0x2C1E14), {-r * 0.3f, -r * 0.3f});
            g.drawCircle(c, r, Color::fromHex(0x1A100A), 1.1f);
            g.drawLine({c.x - r * 0.65f, c.y - r * 0.25f}, {c.x + r * 0.65f, c.y + r * 0.25f}, Color::fromHex(0x140C08), 1.8f);
            g.drawLine({c.x - r * 0.45f, c.y + r * 0.35f}, {c.x + r * 0.35f, c.y - r * 0.45f}, Color::fromHex(0xB89878, 0.45f), 0.9f);
            return;
        } else {
            g.fillRadialGradientCircle(c, r, Color::fromHex(0xC8D0D8), Color::fromHex(0x485058), {-r * 0.35f, -r * 0.35f});
            g.drawCircle(c, r, Color::fromHex(0x181C20), 1.0f);
        }

        // Inner drive socket geometry
        switch (style) {
        case ScrewHeadStyle::SocketHexAllen:
        case ScrewHeadStyle::BlackOxideStealth: {
            g.fillCircle(c, r * 0.54f, Color::fromHex(0x16191D));
            std::array<Point<float>, 7> hexPts{};
            for (size_t i = 0; i < 7; ++i) {
                const float a = static_cast<float>(i) * (3.14159265f / 3.0f) + 0.25f;
                hexPts[i] = {c.x + std::cos(a) * r * 0.44f, c.y + std::sin(a) * r * 0.44f};
            }
            g.drawPolyline(hexPts, (style == ScrewHeadStyle::BlackOxideStealth) ? Color::fromHex(0x4A525C) : Color::fromHex(0x78828C),
                           1.1f);
            break;
        }
        case ScrewHeadStyle::SlottedFlathead: {
            g.drawLine({c.x - r * 0.72f, c.y - r * 0.24f}, {c.x + r * 0.72f, c.y + r * 0.24f}, Color::fromHex(0x101316), 1.9f);
            g.drawLine({c.x - r * 0.72f, c.y - r * 0.24f + 1.0f}, {c.x + r * 0.72f, c.y + r * 0.24f + 1.0f},
                       Color::fromHex(0xD8E2EC, 0.35f), 0.8f);
            break;
        }
        case ScrewHeadStyle::TorxStar6: {
            g.fillCircle(c, r * 0.52f, Color::fromHex(0x161A1E));
            for (int i = 0; i < 3; ++i) {
                const float a = static_cast<float>(i) * (3.14159265f / 3.0f);
                const float dx = std::cos(a) * r * 0.46f;
                const float dy = std::sin(a) * r * 0.46f;
                g.drawLine({c.x - dx, c.y - dy}, {c.x + dx, c.y + dy}, Color::fromHex(0x0B0E11), 1.6f);
            }
            g.fillCircle(c, r * 0.16f,
                         Color::fromHex(0x88929C)); // Center security pin
            break;
        }
        case ScrewHeadStyle::CountersunkFlush: {
            g.drawCircle(c, r * 0.68f, Color::fromHex(0x1E242A), 1.2f);
            g.fillCircle(c, r * 0.38f, Color::fromHex(0x12161A));
            break;
        }
        case ScrewHeadStyle::GoldPlatedBrass:
        case ScrewHeadStyle::PhillipsCross:
        default: {
            const Color slotCol = (style == ScrewHeadStyle::GoldPlatedBrass) ? Color::fromHex(0x362406) : Color::fromHex(0x101316);
            g.fillCircle(c, r * 0.46f, (style == ScrewHeadStyle::GoldPlatedBrass) ? Color::fromHex(0x52380B) : Color::fromHex(0x252A30));
            g.drawLine({c.x - r * 0.38f, c.y}, {c.x + r * 0.38f, c.y}, slotCol, 1.5f);
            g.drawLine({c.x, c.y - r * 0.38f}, {c.x, c.y + r * 0.38f}, slotCol, 1.5f);
            break;
        }
        }
    }

    static void drawJewelLed(Graphics &g, Point<float> c, bool isOn, Color onColor = Color::fromHex(0xFF9F1C), float r = 5.5f) noexcept {
        // Outer glow halo when lit
        if (isOn) { g.fillRadialGradientCircle(c, r * 2.4f, onColor.withAlpha(0.42f), onColor.withAlpha(0.0f)); }
        // Chrome bezel ring & recessed dark socket well
        g.fillCircle({c.x + 0.8f, c.y + 1.4f}, r + 1.8f, Color::fromHex(0x040709, 0.85f));
        g.fillRadialGradientCircle(c, r + 1.5f, Color::fromHex(0xDCE4EC), Color::fromHex(0x363E46), {-1.6f, -1.6f});
        g.drawCircle(c, r + 1.5f, Color::fromHex(0x12161A), 0.9f);
        g.fillCircle(c, r * 0.94f, Color::fromHex(0x0A0D10));
        // 3D Glass Jewel Dome (warm amber #FFA033 with white specular hot-spot when
        // ON, dark unlit amber-brown #2A1C10 when OFF)
        if (isOn) {
            g.fillRadialGradientCircle(c, r * 2.1f, onColor.withAlpha(0.38f), onColor.withAlpha(0.0f));
            g.fillRadialGradientCircle(c, r * 0.84f, Color::fromHex(0xFFF6C8), onColor, {-r * 0.28f, -r * 0.28f});
            g.fillCircle({c.x - r * 0.28f, c.y - r * 0.28f}, std::max(0.9f, r * 0.24f), Color::fromHex(0xFFFFFF, 0.90f));
        } else {
            g.fillRadialGradientCircle(c, r * 0.84f, Color::fromHex(0x462E18), Color::fromHex(0x180F08), {-r * 0.22f, -r * 0.22f});
            g.fillCircle({c.x - r * 0.25f, c.y - r * 0.25f}, std::max(0.7f, r * 0.18f), Color::fromHex(0xFFFFFF, 0.28f));
        }
        g.drawCircle(c, r * 0.84f, Color::fromHex(0x080B0E), 0.8f);
    }

    static uint32_t resolveRawChassisHex(ChassisFinish finish, bool useCustomColor, uint32_t customColorHex) noexcept {
        if (useCustomColor) return customColorHex & 0xFFFFFFu;
        switch (finish) {
        case ChassisFinish::AnodizedTeal:
            return 0x245A5Cu;
        case ChassisFinish::BritishSlateBlue:
            return 0x2B3746u;
        case ChassisFinish::BrushedSilver:
            return 0x8E969Eu;
        case ChassisFinish::VintageGold:
            return 0x4E4126u;
        case ChassisFinish::HammertoneGreen:
            return 0x2B3E2Eu;
        case ChassisFinish::VintageCream:
            return 0x565042u;
        case ChassisFinish::CrimsonMastering:
            return 0x4A1D24u;
        case ChassisFinish::CobaltStudioBlue:
            return 0x1D3558u;
        case ChassisFinish::WarmCopperBronze:
            return 0x4E3224u;
        case ChassisFinish::SunburstAmberWood:
            return 0x8C4A18u;
        case ChassisFinish::RoyalNavyBlue:
            return 0x162844u;
        case ChassisFinish::CyberpunkViolet:
            return 0x2D1646u;
        case ChassisFinish::ArcticWhiteEnamel:
            return 0xABB4BCu;
        case ChassisFinish::ObsidianBlack:
        default:
            return 0x20242Au;
        }
    }

    static float hashPixelSigned(int x, int y, uint32_t seed = 0u) noexcept {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u + seed * 2246822519u;
        h = (h ^ (h >> 13u)) * 1274126177u;
        h = h ^ (h >> 16u);
        return static_cast<float>(h & 0xFFFFu) * (2.0f / 65535.0f) - 1.0f;
    }

    static float hashPixel01(int x, int y, uint32_t seed = 0u) noexcept {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u + seed * 2246822519u;
        h = (h ^ (h >> 13u)) * 1274126177u;
        h = h ^ (h >> 16u);
        return static_cast<float>(h & 0xFFFFu) * (1.0f / 65535.0f);
    }

    static float valueNoise2D(float x, float y, uint32_t seed = 0u) noexcept {
        const int ix = static_cast<int>(std::floor(x));
        const int iy = static_cast<int>(std::floor(y));
        const float fx = x - static_cast<float>(ix);
        const float fy = y - static_cast<float>(iy);
        const float u = fx * fx * (3.0f - 2.0f * fx);
        const float v = fy * fy * (3.0f - 2.0f * fy);
        const float n00 = hashPixelSigned(ix, iy, seed);
        const float n10 = hashPixelSigned(ix + 1, iy, seed);
        const float n01 = hashPixelSigned(ix, iy + 1, seed);
        const float n11 = hashPixelSigned(ix + 1, iy + 1, seed);
        const float nx0 = n00 + u * (n10 - n00);
        const float nx1 = n01 + u * (n11 - n01);
        return nx0 + v * (nx1 - nx0);
    }

    static float fbm2D(float x, float y, uint32_t seed = 0u) noexcept {
        float sum = 0.0f;
        float amp = 0.5f;
        float freq = 1.0f;
        for (int i = 0; i < 3; ++i) {
            sum += amp * valueNoise2D(x * freq, y * freq, seed + static_cast<uint32_t>(i) * 101u);
            freq *= 2.03f;
            amp *= 0.5f;
        }
        return sum;
    }

    static void synthesizeStudioSurfaceBitmap(uint32_t *pixels, uint32_t w, uint32_t h, SurfaceGrain grain, uint32_t rawBaseHex) noexcept {
        if (!pixels || w == 0 || h == 0) return;

        // 1. Physical Albedo Conditioning (HSL saturation & luminance clamp so neon
        // colors become studio hardware)
        float rIn = static_cast<float>((rawBaseHex >> 16) & 0xFFu) / 255.0f;
        float gIn = static_cast<float>((rawBaseHex >> 8) & 0xFFu) / 255.0f;
        float bIn = static_cast<float>(rawBaseHex & 0xFFu) / 255.0f;

        const float cMax = (std::max)({rIn, gIn, bIn});
        const float cMin = (std::min)({rIn, gIn, bIn});
        const float delta = cMax - cMin;

        float hue = 0.0f;
        float sat = 0.0f;
        float lum = 0.5f * (cMax + cMin);

        if (delta > 1.0e-5f) {
            sat = delta / (1.0f - std::abs(2.0f * lum - 1.0f));
            if (cMax == rIn) {
                hue = std::fmod((gIn - bIn) / delta, 6.0f);
                if (hue < 0.0f) hue += 6.0f;
            } else if (cMax == gIn) {
                hue = ((bIn - rIn) / delta) + 2.0f;
            } else {
                hue = ((rIn - gIn) / delta) + 4.0f;
            }
            hue /= 6.0f;
        }

        // Preserve the user's selected colour across all surface finishes (only
        // clamp extreme 0/1 bounds so surface relief remains visible)
        float maxSat = 0.96f;
        float minLum = 0.05f;
        float maxLum = 0.94f;
        sat = std::clamp(sat, 0.0f, maxSat);
        lum = std::clamp(lum, minLum, maxLum);

        auto hslToRgb = [](float hVal, float sVal, float lVal, float &outR, float &outG, float &outB) noexcept {
            const float c = (1.0f - std::abs(2.0f * lVal - 1.0f)) * sVal;
            const float hPrime = hVal * 6.0f;
            const float x = c * (1.0f - std::abs(std::fmod(hPrime, 2.0f) - 1.0f));
            float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
            if (hPrime < 1.0f) {
                r1 = c;
                g1 = x;
                b1 = 0.0f;
            } else if (hPrime < 2.0f) {
                r1 = x;
                g1 = c;
                b1 = 0.0f;
            } else if (hPrime < 3.0f) {
                r1 = 0.0f;
                g1 = c;
                b1 = x;
            } else if (hPrime < 4.0f) {
                r1 = 0.0f;
                g1 = x;
                b1 = c;
            } else if (hPrime < 5.0f) {
                r1 = x;
                g1 = 0.0f;
                b1 = c;
            } else {
                r1 = c;
                g1 = 0.0f;
                b1 = x;
            }
            const float m = lVal - 0.5f * c;
            outR = std::clamp(r1 + m, 0.0f, 1.0f);
            outG = std::clamp(g1 + m, 0.0f, 1.0f);
            outB = std::clamp(b1 + m, 0.0f, 1.0f);
        };

        float baseR = 0.2f, baseG = 0.25f, baseB = 0.28f;
        hslToRgb(hue, sat, lum, baseR, baseG, baseB);

        // Precompute 1D convolved anisotropic brush buffers for O(1) per-pixel
        // lookup
        std::vector<float> vertBrushField;
        std::vector<float> horizBrushField;
        std::vector<float> relicScratchField;
        if (grain == SurfaceGrain::VerticalBrushed || grain == SurfaceGrain::PerforatedHexMesh ||
            grain == SurfaceGrain::GrimyRoadWornRelic || grain == SurfaceGrain::TitaniumMicroGrid) {
            vertBrushField.resize(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0f);
            constexpr int kKernelH = 48;
            const float invK = 1.0f / static_cast<float>(kKernelH);
            for (uint32_t x = 0; x < w; ++x) {
                const float colMacro = hashPixelSigned(static_cast<int>(x), 0, 17u) * 0.35f;
                float running = 0.0f;
                for (int y = -kKernelH; y < static_cast<int>(h); ++y) {
                    running += hashPixelSigned(static_cast<int>(x), y + kKernelH, 31u);
                    if (y >= 0) {
                        running -= hashPixelSigned(static_cast<int>(x), y, 31u);
                        // Scale by sqrt(48) * invK so standard deviation stays ~1.0 before
                        // ±7% modulation
                        const float conv = (running * invK) * 3.8f + colMacro;
                        vertBrushField[static_cast<size_t>(y) * w + x] = std::clamp(conv, -1.2f, 1.2f);
                    }
                }
            }
        }
        if (grain == SurfaceGrain::GrimyRoadWornRelic) {
            // Precompute 48+ procedural hairline scratches across the ENTIRE 1000x560
            // faceplate interior (x = 0..1000) Positive values = bright bare-aluminum
            // catchlight lip, negative values = dark scratch trough
            relicScratchField.resize(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0f);
            constexpr int kNumScratches = 52;
            for (int s = 0; s < kNumScratches; ++s) {
                const uint32_t sSeed = 8801u + static_cast<uint32_t>(s) * 97u;
                const float x0 = (0.03f + 0.94f * hashPixel01(s, 1, sSeed)) * static_cast<float>(w);
                const float y0 = (0.05f + 0.90f * hashPixel01(s, 2, sSeed)) * static_cast<float>(h);
                const float angle = (hashPixel01(s, 3, sSeed) - 0.5f) * 2.95f;
                const float len = 22.0f + 115.0f * hashPixel01(s, 4, sSeed);
                const float ca = std::cos(angle);
                const float sa = std::sin(angle);
                const float nx = -sa;
                const float ny = ca;
                const float bend = (hashPixel01(s, 5, sSeed) - 0.5f) * 5.5f;
                const int steps = static_cast<int>(len * 1.5f);
                for (int st = 0; st <= steps; ++st) {
                    const float t = static_cast<float>(st) / static_cast<float>(std::max(1, steps));
                    const float taper = std::sin(t * 3.14159265f);
                    const float arc = std::sin(t * 3.14159265f) * bend;
                    const float px = x0 + ca * (t * len) + nx * arc;
                    const float py = y0 + sa * (t * len) + ny * arc;
                    const int ix = static_cast<int>(px + 0.5f);
                    const int iy = static_cast<int>(py + 0.5f);
                    if (ix >= 1 && ix + 1 < static_cast<int>(w) && iy >= 1 && iy + 1 < static_cast<int>(h)) {
                        // Dark scratch trough
                        relicScratchField[static_cast<size_t>(iy) * w + static_cast<size_t>(ix)] = -0.32f * taper;
                        // 1px bright bare-aluminum catchlight lip offset bottom-right
                        const int lx = std::clamp(ix + (nx >= 0.0f ? 1 : -1), 0, static_cast<int>(w) - 1);
                        const int ly = std::clamp(iy + 1, 0, static_cast<int>(h) - 1);
                        relicScratchField[static_cast<size_t>(ly) * w + static_cast<size_t>(lx)] =
                            (std::max)(relicScratchField[static_cast<size_t>(ly) * w + static_cast<size_t>(lx)], 0.72f * taper);
                    }
                }
            }
        }
        if (grain == SurfaceGrain::HorizontalBrushed) {
            horizBrushField.resize(static_cast<size_t>(w) * static_cast<size_t>(h), 0.0f);
            constexpr int kKernelW = 64;
            const float invK = 1.0f / static_cast<float>(kKernelW);
            for (uint32_t y = 0; y < h; ++y) {
                const float rowMacro = hashPixelSigned(0, static_cast<int>(y), 23u) * 0.35f;
                float running = 0.0f;
                for (int x = -kKernelW; x < static_cast<int>(w); ++x) {
                    running += hashPixelSigned(x + kKernelW, static_cast<int>(y), 47u);
                    if (x >= 0) {
                        running -= hashPixelSigned(x, static_cast<int>(y), 47u);
                        const float conv = (running * invK) * 4.2f + rowMacro;
                        horizBrushField[static_cast<size_t>(y) * w + static_cast<size_t>(x)] = std::clamp(conv, -1.2f, 1.2f);
                    }
                }
            }
        }

        const float invW = 1.0f / static_cast<float>(w);
        const float invH = 1.0f / static_cast<float>(h);

        for (uint32_t y = 0; y < h; ++y) {
            const float fy = static_cast<float>(y);
            const float v = fy * invH;

            // Universal vertical luminance gradient: lighten(chassisBg, +12%) at top
            // to darken(chassisBg, -24%) at bottom
            const float vertGrad = 1.12f * (1.0f - v) + 0.76f * v;
            const float ryNormV = (fy - static_cast<float>(h - 1u) * 0.5f) / std::max(1.0f, static_cast<float>(h - 1u) * 0.5f);

            // Vertical edge ambient occlusion (within 18px of top/bottom)
            float edgeDistY = (std::min)(fy, static_cast<float>(h - 1u - y));
            float aoY = (edgeDistY < 18.0f) ? (1.0f - 0.18f * (1.0f - edgeDistY / 18.0f)) : 1.0f;

            for (uint32_t x = 0; x < w; ++x) {
                const float fx = static_cast<float>(x);
                const float u = fx * invW;
                const float rxNormU = (fx - static_cast<float>(w - 1u) * 0.5f) / std::max(1.0f, static_cast<float>(w - 1u) * 0.5f);

                // 28% dark radial corner vignette (symmetric left/right at x=0 and
                // x=w-1)
                const float rCornerSq = std::clamp((rxNormU * rxNormU + ryNormV * ryNormV) * 0.5f, 0.0f, 1.0f);
                const float cornerVignette = 1.0f - 0.28f * (rCornerSq * rCornerSq);

                // Seam & perimeter horizontal AO (within 18px of outer edges & x=26 /
                // x=974 rack-ear seams)
                const float edgeDistX = (std::min)(fx, static_cast<float>(w - 1u - x));
                const float seamDistL = std::abs(fx - 26.0f);
                const float seamDistR = std::abs(fx - static_cast<float>(w - 26u));
                float aoX = 1.0f;
                if (edgeDistX < 18.0f) { aoX *= (1.0f - 0.14f * (1.0f - edgeDistX / 18.0f)); }
                if (grain != SurfaceGrain::WalnutWoodCheeks && w > 128u) {
                    const float seamDist = (std::min)(seamDistL, seamDistR);
                    if (seamDist < 14.0f) { aoX *= (1.0f - 0.10f * (1.0f - seamDist / 14.0f)); }
                }

                // Overhead Studio Lamp Reflection + studio sheen band (+6% white across
                // 0.25..0.55) Nextel Suede Coat (TolexAmpLeather) disables the central
                // overhead specular lamp highlight completely (studioLamp = 0.0f)
                const float du = rxNormU * 0.68f;
                const float dv = (v - 0.14f) * 2.10f;
                const float studioSheen = (v >= 0.25f && v <= 0.55f)
                                              ? (0.06f * std::sin(((v - 0.25f) / 0.30f) * 3.14159265f) * (1.0f - 0.25f * rxNormU * rxNormU))
                                              : 0.0f;
                const float studioLamp =
                    (grain == SurfaceGrain::TolexAmpLeather) ? 0.0f : (0.09f * std::exp(-(du * du + dv * dv) * 2.2f) + studioSheen);

                float r = baseR * vertGrad;
                float g = baseG * vertGrad;
                float b = baseB * vertGrad;

                switch (grain) {
                case SurfaceGrain::VerticalBrushed: {
                    // 0: Sub-pixel vertical anisotropic hairlines (±7% luminance) +
                    // horizontal specular sheen
                    const float streak = vertBrushField[static_cast<size_t>(y) * w + x];
                    const float sheen = 0.045f * std::exp(-((u - 0.50f) * (u - 0.50f)) * 8.0f) * (1.0f - 0.5f * v);
                    const float mod = 1.0f + streak * 0.07f + sheen;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::HorizontalBrushed: {
                    // 1: Sub-pixel horizontal anisotropic hairlines (±7% luminance) +
                    // vertical cylindrical roll-off
                    const float streak = horizBrushField[static_cast<size_t>(y) * w + x];
                    const float cyl = 0.045f * std::cos((v - 0.32f) * 3.14159265f);
                    const float mod = 1.0f + streak * 0.07f + cyl;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::MatteSmooth: {
                    // 2: MATTE ANODIZED — Rich dyed saturation (S * 1.10f), wide silky
                    // cylindrical studio lamp sheen (+11%) & ±1.2% isotropic satin pore
                    const float diag = (1.0f - 0.35f * rxNormU * rxNormU) * 0.35f + v * 0.65f;
                    const float diagPhase = std::clamp((diag - 0.42f) * 2.2f, -1.5707963f, 1.5707963f);
                    const float satinSheen = 0.11f * std::cos(diagPhase);
                    const float microPore = valueNoise2D(fx * 0.45f, fy * 0.45f, 71u) * 0.012f;
                    const float deepEdgeX = (edgeDistX < 32.0f) ? (0.88f + 0.12f * (edgeDistX / 32.0f)) : 1.0f;
                    const float deepEdgeY = (edgeDistY < 32.0f) ? (0.88f + 0.12f * (edgeDistY / 32.0f)) : 1.0f;
                    const float mod = (1.0f + satinSheen + microPore) * deepEdgeX * deepEdgeY;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::CarbonWeave: {
                    // 3: 4x4 2x2 twill carbon-fiber micro-weave preserving the selected
                    // chassis colour
                    const int cellX = static_cast<int>(x) / 4;
                    const int cellY = static_cast<int>(y) / 4;
                    const int subX = static_cast<int>(x) & 3;
                    const int subY = static_cast<int>(y) & 3;
                    const bool isHorizBundle = (((cellX + cellY) & 1) == 0);
                    const float tFiber = isHorizBundle ? (static_cast<float>(subX) / 3.0f) : (static_cast<float>(subY) / 3.0f);
                    const float strandWave = std::sin(tFiber * 3.14159265f);
                    const float mod = 0.72f + 0.38f * strandWave + (isHorizBundle ? 0.03f : -0.05f);
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::WrinklePowderCoat: {
                    // 4: Vintage wrinkle enamel — high-frequency domain-warped cellular
                    // ridge noise (scale = 3.5px)
                    const float wx = fx / 3.5f + 0.65f * valueNoise2D(fx * 0.14f, fy * 0.14f, 201u);
                    const float wy = fy / 3.5f + 0.65f * valueNoise2D(fx * 0.14f, fy * 0.14f, 209u);
                    const float n = valueNoise2D(wx, wy, 223u);
                    const float ridge = 1.0f - std::abs(n); // Sharp crest in [0..1]
                    const float crest = ridge * ridge;
                    const float micro = hashPixelSigned(static_cast<int>(x), static_cast<int>(y), 229u) * 0.02f;
                    const float mod = 0.86f + 0.24f * crest + micro;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::DiamondCrosshatch: {
                    // 5: Precision CNC micro-knurling (4px x 4px diamond pitch) pyramidal
                    // normal shading
                    const float nx = std::sin(fx * 1.5707963f);
                    const float ny = std::sin(fy * 1.5707963f);
                    const float ndotl = (nx * 0.7071f + ny * 0.7071f);
                    const float mod = 1.0f + ndotl * 0.11f;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::BeadBlastedMatte: {
                    // 6: BEAD BLASTED — 2.0px/4.0px shot-peened micro-crater
                    // finite-difference normals (±14%) + crystalline glass-bead glints
                    // (+26%)
                    const auto samplePeenHeight = [](float px, float py) noexcept -> float {
                        return 0.70f * valueNoise2D(px / 2.0f, py / 2.0f, 307u) + 0.30f * valueNoise2D(px / 4.0f, py / 4.0f, 311u);
                    };
                    const float dHx = samplePeenHeight(fx + 1.0f, fy) - samplePeenHeight(fx - 1.0f, fy);
                    const float dHy = samplePeenHeight(fx, fy + 1.0f) - samplePeenHeight(fx, fy - 1.0f);
                    const float peenShade = (dHx * -0.707f + dHy * -0.707f) * 0.14f;

                    const float hFacet = hashPixel01(static_cast<int>(x), static_cast<int>(y), 6006u);
                    float glintAdd = 0.0f;
                    float pitSub = 0.0f;
                    if (hFacet > 0.96f) {
                        const float lampEnv = std::clamp(0.78f + studioLamp * 2.2f, 0.70f, 1.24f);
                        glintAdd = (0.16f + (hFacet - 0.96f) * 2.5f) * lampEnv; // up to +26% bright metallic sparkle
                    } else if (hFacet < 0.05f) {
                        pitSub = 0.12f; // -12% dark micro-pits
                    }
                    const float mod = 1.0f + peenShade;
                    r = std::max(0.0f, r * mod - pitSub * (0.45f + 0.55f * r)) + glintAdd * 0.95f;
                    g = std::max(0.0f, g * mod - pitSub * (0.45f + 0.55f * g)) + glintAdd * 0.98f;
                    b = std::max(0.0f, b * mod - pitSub * (0.45f + 0.55f * b)) + glintAdd * 1.03f;
                    break;
                }
                case SurfaceGrain::HammertoneDimpled: {
                    // 7: Smooth 2D Voronoi cellular distance field (cellSize = 9.0px)
                    // concave dimple normal shading
                    constexpr float kCell = 9.0f;
                    const float gx = fx / kCell;
                    const float gy = fy / kCell;
                    const int ix = static_cast<int>(std::floor(gx));
                    const int iy = static_cast<int>(std::floor(gy));
                    float minDist2 = 100.0f;
                    float bestDx = 0.0f, bestDy = 0.0f;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            const int cx = ix + dx;
                            const int cy = iy + dy;
                            const float jx = static_cast<float>(cx) + 0.18f + 0.64f * hashPixel01(cx, cy, 401u);
                            const float jy = static_cast<float>(cy) + 0.18f + 0.64f * hashPixel01(cx, cy, 409u);
                            const float vx = gx - jx;
                            const float vy = gy - jy;
                            const float d2 = vx * vx + vy * vy;
                            if (d2 < minDist2) {
                                minDist2 = d2;
                                bestDx = vx;
                                bestDy = vy;
                            }
                        }
                    }
                    const float dist = std::sqrt(minDist2);
                    // Concave basin normal lit from top-left (-0.707, -0.707) + rim sheen
                    const float tRim = std::clamp((dist - 0.25f) / 0.55f, 0.0f, 1.0f);
                    const float rimCurve = tRim * tRim * (3.0f - 2.0f * tRim);
                    const float basinDot = std::clamp((bestDx * 0.707f + bestDy * 0.707f) * 0.85f, -0.55f, 0.55f);
                    const float mod = 0.94f + 0.11f * basinDot + 0.06f * rimCurve;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::WalnutWoodCheeks: {
                    // 8: Center (x = 44..956) Matte Anodized Metal; Outer strips (0..44,
                    // 956..1000) 3D American Black Walnut
                    const float cheekW = 44.0f * (static_cast<float>(w) / 1000.0f);
                    const bool isLeftCheek = (fx < cheekW);
                    const bool isRightCheek = (fx >= static_cast<float>(w) - cheekW);
                    if (isLeftCheek || isRightCheek) {
                        const float localX = isLeftCheek ? fx : (fx - (static_cast<float>(w) - cheekW));
                        const float warp = fbm2D(localX * 0.09f, fy * 0.018f, 503u) * 3.8f;
                        const float ring = 0.5f + 0.5f * std::sin((localX * 0.52f + warp) * 3.14159265f);
                        const float pores = valueNoise2D(localX * 0.85f, fy * 0.14f, 509u);
                        const float darkPore = (pores < -0.42f) ? -0.18f : 0.0f;
                        // Walnut palette: #3B2314 (0.231, 0.137, 0.078) to #5C3820 (0.361,
                        // 0.220, 0.125)
                        r = 0.231f + (0.361f - 0.231f) * ring + darkPore * 0.4f;
                        g = 0.137f + (0.220f - 0.137f) * ring + darkPore * 0.3f;
                        b = 0.078f + (0.125f - 0.078f) * ring + darkPore * 0.2f;
                        // 3D chamfer bevel at the inner edge of the walnut cheek
                        const float innerDist = isLeftCheek ? (cheekW - localX) : localX;
                        if (innerDist < 3.5f) {
                            const float bev = isLeftCheek ? 0.62f : 1.18f;
                            r *= bev;
                            g *= bev;
                            b *= bev;
                        } else if (localX < 3.0f || localX > cheekW - 3.0f) {
                            r *= 0.78f;
                            g *= 0.78f;
                            b *= 0.78f;
                        }
                    } else {
                        const float diag = u * 0.35f + v * 0.65f;
                        const float diagPhase = std::clamp((diag - 0.42f) * 2.2f, -1.5707963f, 1.5707963f);
                        const float satinSheen = 0.08f * std::cos(diagPhase);
                        const float microPore = valueNoise2D(fx * 0.45f, fy * 0.45f, 71u) * 0.012f;
                        float mod = 1.0f + satinSheen + microPore;
                        // Cast shadow from the raised 3D walnut cheeks onto the metal panel
                        const float distFromLeftCheek = fx - cheekW;
                        const float distFromRightCheek = (static_cast<float>(w) - cheekW) - fx;
                        if (distFromLeftCheek >= 0.0f && distFromLeftCheek < 9.0f) {
                            mod *= (0.72f + 0.28f * (distFromLeftCheek / 9.0f));
                        } else if (distFromRightCheek >= 0.0f && distFromRightCheek < 7.0f) {
                            mod *= (0.80f + 0.20f * (distFromRightCheek / 7.0f));
                        }
                        r *= mod;
                        g *= mod;
                        b *= mod;
                    }
                    break;
                }
                case SurfaceGrain::PerforatedHexMesh: {
                    // 9: Staggered hexagonal micro-perforation grid (4px pitch, 1.4px
                    // hole radius)
                    constexpr float kPitchX = 4.0f;
                    constexpr float kPitchY = 3.5f;
                    const int row = static_cast<int>(std::floor(fy / kPitchY));
                    const float xShift = (row & 1) ? (kPitchX * 0.5f) : 0.0f;
                    const float cellY = (static_cast<float>(row) + 0.5f) * kPitchY;
                    const int col = static_cast<int>(std::floor((fx - xShift) / kPitchX));
                    const float cellX = (static_cast<float>(col) + 0.5f) * kPitchX + xShift;
                    const float dx = fx - cellX;
                    const float dy = fy - cellY;
                    const float d = std::sqrt(dx * dx + dy * dy);
                    const float streak = vertBrushField[static_cast<size_t>(y) * w + x] * 0.05f;
                    if (d < 1.35f) {
                        // Dark recessed micro-hole interior (-55% brightness)
                        const float holeDepth = 0.42f + 0.08f * (d / 1.35f);
                        r *= holeDepth;
                        g *= holeDepth;
                        b *= holeDepth;
                    } else if (d < 2.05f && dx > 0.0f && dy > 0.0f) {
                        // 1px bottom-right metallic catchlight on the hole rim
                        const float catchLight = 1.16f + streak;
                        r *= catchLight;
                        g *= catchLight;
                        b *= catchLight;
                    } else {
                        const float mod = 1.0f + streak;
                        r *= mod;
                        g *= mod;
                        b *= mod;
                    }
                    break;
                }
                case SurfaceGrain::FullCarvedWoodGrain: {
                    // 10: Full-panel carved hardwood grain preserving the selected
                    // chassis colour
                    const float warp = fbm2D(fx * 0.008f, fy * 0.035f, 601u) * 6.5f;
                    const float fiber = 0.5f + 0.5f * std::sin((fy * 0.24f + warp) * 3.14159265f);
                    const float fineGrain = valueNoise2D(fx * 0.06f, fy * 0.65f, 613u) * 0.08f;
                    const float pore = (valueNoise2D(fx * 0.18f, fy * 0.85f, 619u) < -0.48f) ? -0.14f : 0.0f;
                    const float fiberMod = (0.76f + 0.36f * fiber) * (1.0f + fineGrain + pore);
                    r *= fiberMod;
                    g *= fiberMod;
                    b *= fiberMod;
                    break;
                }
                case SurfaceGrain::TolexAmpLeather: {
                    // 11: NEXTEL SUEDE COAT — Oren-Nayar velvet lighting preserving the
                    // selected chassis colour
                    const float flock35 = valueNoise2D(fx / 3.5f, fy / 3.5f, 701u);
                    const float flock70 = valueNoise2D(fx / 7.0f, fy / 7.0f, 709u);
                    const float roundedGlobule = 0.62f * (1.0f - 0.65f * flock35 * flock35 - 0.35f * flock70 * flock70) +
                                                 0.38f * (0.65f * flock35 + 0.35f * flock70);

                    const float napAlong = (fx * 0.8f + fy * 0.6f) / 6.0f;
                    const float napCross = (-fx * 0.6f + fy * 0.8f) / 2.6f;
                    const float suedeNap =
                        (0.60f * valueNoise2D(napAlong, napCross, 719u) + 0.40f * valueNoise2D(napAlong * 0.5f, napCross * 0.65f, 727u)) *
                        0.035f;

                    const float rxNorm = (u - 0.5f) * 2.0f;
                    const float ryNorm = (v - 0.5f) * 2.0f;
                    const float rimDistSq = std::clamp(rxNorm * rxNorm * 0.65f + ryNorm * ryNorm * 0.85f, 0.0f, 1.0f);
                    const float grazingLift = 0.06f * (rimDistSq * rimDistSq);

                    const float mod = 0.96f + 0.065f * roundedGlobule + suedeNap;
                    r = std::clamp(r * mod + grazingLift, 0.02f, 0.98f);
                    g = std::clamp(g * mod + grazingLift, 0.02f, 0.98f);
                    b = std::clamp(b * mod + grazingLift, 0.02f, 0.98f);
                    break;
                }
                case SurfaceGrain::GrimyRoadWornRelic: {
                    // 12: GRIMY RELIC WEAR — Full-panel FBM grime/oil blotches, rubbed
                    // wear islands, and 52 procedural hairline scratches preserving the
                    // selected chassis colour
                    const float streak = vertBrushField[static_cast<size_t>(y) * w + x] * 0.065f;
                    const float grimeMacro = 0.5f + 0.5f * fbm2D(fx * 0.015f, fy * 0.018f, 809u);
                    const float grimeDetail = 0.5f + 0.5f * fbm2D(fx * 0.048f, fy * 0.052f, 811u);
                    const float grimeDarken = 1.0f - (0.24f * (0.65f * grimeMacro + 0.35f * grimeDetail));
                    r = r * (1.0f + streak) * grimeDarken;
                    g = g * (1.0f + streak) * grimeDarken;
                    b = b * (1.0f + streak) * grimeDarken;

                    const float tarnishField = fbm2D(fx * 0.024f + 1.3f, fy * 0.028f + 0.7f, 823u);
                    if (tarnishField > 0.12f) {
                        const float tPat = std::clamp((tarnishField - 0.12f) * 0.45f, 0.0f, 0.18f);
                        r = r * (1.0f - tPat) + (r * 0.85f + 0.06f) * tPat;
                        g = g * (1.0f - tPat) + (g * 0.82f + 0.04f) * tPat;
                        b = b * (1.0f - tPat) + (b * 0.75f + 0.02f) * tPat;
                    }

                    const float minBoundary = (std::min)({edgeDistY, edgeDistX, seamDistL, seamDistR});
                    const float edgeFactor = (minBoundary < 22.0f) ? (1.0f - minBoundary / 22.0f) * 0.60f : 0.0f;
                    const float interiorWear = 0.5f + 0.5f * fbm2D(fx * 0.022f, fy * 0.026f, 829u);
                    const float chipDetail = 0.5f + 0.5f * valueNoise2D(fx * 0.16f, fy * 0.16f, 839u);
                    const float wearMask = interiorWear * 0.78f + chipDetail * 0.24f + edgeFactor;
                    if (wearMask > 0.68f) {
                        const float bareBlend = std::clamp((wearMask - 0.68f) * 1.4f, 0.0f, 0.38f);
                        r = r * (1.0f - bareBlend) + std::min(0.95f, r * 1.28f + 0.14f) * bareBlend;
                        g = g * (1.0f - bareBlend) + std::min(0.95f, g * 1.28f + 0.15f) * bareBlend;
                        b = b * (1.0f - bareBlend) + std::min(0.95f, b * 1.28f + 0.16f) * bareBlend;
                    }

                    // Apply 52 full-panel hairline scratches (dark trough + 1px bright
                    // catchlight lip)
                    const float scratch = relicScratchField[static_cast<size_t>(y) * w + x];
                    if (scratch > 0.01f) {
                        const float sBlend = std::clamp(scratch, 0.0f, 0.65f);
                        r = r * (1.0f - sBlend) + std::min(0.96f, r + 0.28f) * sBlend;
                        g = g * (1.0f - sBlend) + std::min(0.96f, g + 0.29f) * sBlend;
                        b = b * (1.0f - sBlend) + std::min(0.96f, b + 0.31f) * sBlend;
                    } else if (scratch < -0.01f) {
                        const float troughMult = std::clamp(1.0f + scratch, 0.52f, 1.0f);
                        r *= troughMult;
                        g *= troughMult;
                        b *= troughMult;
                    }
                    break;
                }
                case SurfaceGrain::OxidizedPatina: {
                    // 13: Multi-octave domain-warped FBM patina preserving the selected
                    // chassis colour
                    const float wx = fx * 0.014f + 1.8f * fbm2D(fx * 0.01f, fy * 0.01f, 907u);
                    const float wy = fy * 0.014f + 1.8f * fbm2D(fx * 0.01f, fy * 0.01f, 911u);
                    const float pat = fbm2D(wx, wy, 919u);
                    const float pit = hashPixelSigned(static_cast<int>(x), static_cast<int>(y), 929u) * 0.032f;
                    const float patMod = 1.0f + pat * 0.22f + pit;
                    r = std::clamp(r * patMod + (pat < -0.05f ? 0.04f : 0.0f), 0.0f, 1.0f);
                    g = std::clamp(g * patMod + (pat > 0.08f ? 0.05f : 0.0f), 0.0f, 1.0f);
                    b = std::clamp(b * patMod + (pat > 0.08f ? 0.04f : 0.0f), 0.0f, 1.0f);
                    break;
                }
                case SurfaceGrain::SleekGlassAcrylic: {
                    // 14: Deep smoked obsidian-tinted glass (L <= 0.22) with smooth
                    // diagonal Fresnel reflection bands
                    const float diag = u * 0.78f + v * 0.34f;
                    const float band1 = std::exp(-((diag - 0.32f) * (diag - 0.32f)) * 180.0f) * 0.055f;
                    const float band2 = std::exp(-((diag - 0.58f) * (diag - 0.58f)) * 95.0f) * 0.080f;
                    const float band3 = std::exp(-((diag - 0.82f) * (diag - 0.82f)) * 210.0f) * 0.045f;
                    const float fresnel = band1 + band2 + band3;
                    r += fresnel;
                    g += fresnel * 1.04f;
                    b += fresnel * 1.10f;
                    // Subtle edge refraction highlight near chassis rim
                    const float minEdge = (std::min)(edgeDistX, edgeDistY);
                    if (minEdge >= 3.0f && minEdge <= 5.0f) {
                        const float rimGlow = (1.0f - std::abs(minEdge - 4.0f)) * 0.14f;
                        r += rimGlow * 0.55f;
                        g += rimGlow * 0.90f;
                        b += rimGlow * 1.00f;
                    }
                    break;
                }
                case SurfaceGrain::TitaniumMicroGrid: {
                    // 15: Precision CNC laser-etched 6px x 6px micro-grid on brushed
                    // titanium
                    const float streak = vertBrushField[static_cast<size_t>(y) * w + x] * 0.045f;
                    const int gx = static_cast<int>(x) % 6;
                    const int gy = static_cast<int>(y) % 6;
                    float gridMod = 1.0f + streak;
                    if (gx == 0 || gy == 0) {
                        gridMod *= 0.84f; // Milled micro-channel groove
                    } else if (gx == 1 || gy == 1) {
                        gridMod *= 1.09f; // Crisp top-left specular edge catchlight
                    }
                    r *= gridMod;
                    g *= gridMod;
                    b *= gridMod;
                    break;
                }
                case SurfaceGrain::DiamondTreadplate: {
                    // 16: Industrial 3D embossed diamond treadplate lozenges on a 16x16
                    // staggered grid
                    const float streak = hashPixelSigned(static_cast<int>(x), static_cast<int>(y), 1009u) * 0.022f;
                    const int cellX = static_cast<int>(x) / 16;
                    const int cellY = static_cast<int>(y) / 16;
                    const float lx = (fx - (static_cast<float>(cellX) + 0.5f) * 16.0f);
                    const float ly = (fy - (static_cast<float>(cellY) + 0.5f) * 16.0f);
                    const bool alt = (((cellX + cellY) & 1) == 0);
                    // Rotate local coordinates by +45 or -45 degrees
                    const float rx = (lx + (alt ? ly : -ly)) * 0.70710678f;
                    const float ry = (ly - (alt ? lx : -lx)) * 0.70710678f;
                    // Diamond lozenge test: |rx| / 5.2 + |ry| / 2.2 <= 1.0
                    const float dLug = std::abs(rx) / 5.2f + std::abs(ry) / 2.2f;
                    float mod = 1.0f + streak;
                    if (dLug <= 1.0f) {
                        // 4-facet raised diamond normal shading
                        const float nx = (rx >= 0.0f) ? 0.55f : -0.55f;
                        const float ny = (ry >= 0.0f) ? 0.80f : -0.80f;
                        const float ndotl = -(nx * 0.707f + ny * 0.707f);
                        const float ridgeBoost = (dLug < 0.22f) ? 0.08f : 0.0f;
                        mod = 1.04f + ndotl * 0.14f + ridgeBoost;
                    } else if (dLug < 1.18f && (lx > 0.0f || ly > 0.0f)) {
                        mod = 0.85f; // Subtle drop shadow around raised lug
                    }
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                case SurfaceGrain::WovenTweed:
                default: {
                    // 17: Fine diagonal 2x2 woven studio acoustic baffle fabric /
                    // lacquered twill
                    const int tx = static_cast<int>(x);
                    const int ty = static_cast<int>(y);
                    const int phase = (tx + ty * 2) & 7;
                    const bool warpOver = (phase < 4);
                    const float subPhase = static_cast<float>(phase & 3) / 3.0f;
                    const float cyl = std::sin(subPhase * 3.14159265f);
                    const float microFiber = hashPixelSigned(tx, ty, 1103u) * 0.035f;
                    const float threadTone = warpOver ? (0.92f + 0.18f * cyl) : (0.78f + 0.15f * cyl);
                    const float mod = threadTone + microFiber;
                    r *= mod;
                    g *= mod;
                    b *= mod;
                    break;
                }
                }

                // Apply Studio Lighting Envelope, 28% Radial Corner Vignette &
                // Edge/Seam Ambient Occlusion
                const float env = aoX * aoY * cornerVignette;
                const uint32_t outR = static_cast<uint32_t>(std::clamp((r + studioLamp) * env, 0.0f, 1.0f) * 255.0f + 0.5f);
                const uint32_t outG = static_cast<uint32_t>(std::clamp((g + studioLamp) * env, 0.0f, 1.0f) * 255.0f + 0.5f);
                const uint32_t outB = static_cast<uint32_t>(std::clamp((b + studioLamp) * env, 0.0f, 1.0f) * 255.0f + 0.5f);

                // Pack as BGRA (DXGI_FORMAT_B8G8R8A8_UNORM)
                pixels[static_cast<size_t>(y) * w + x] = 0xFF000000u | (outR << 16u) | (outG << 8u) | outB;
            }
        }
    }

    static void drawSurfaceOverlay(Graphics &g, Rect<float> bounds, ChassisFinish finish = ChassisFinish::AnodizedTeal,
                                   SurfaceGrain grain = SurfaceGrain::VerticalBrushed, bool useCustomColor = false,
                                   uint32_t customColorHex = 0x245A5C, float alpha = 1.0f) noexcept {
        const uint32_t rawBaseHex = resolveRawChassisHex(finish, useCustomColor, customColorHex);
        constexpr uint32_t kSurfW = 1000u;
        constexpr uint32_t kSurfH = 560u;
        constexpr uint64_t kSurfaceShaderRev = 0x05ULL;
        const uint64_t cacheKey = (static_cast<uint64_t>(static_cast<uint32_t>(grain) & 0xFFu) << 56u) | (kSurfaceShaderRev << 52u) |
                                  (static_cast<uint64_t>(rawBaseHex & 0xFFFFFFu) << 28u) |
                                  (static_cast<uint64_t>(kSurfW & 0x3FFFu) << 14u) | static_cast<uint64_t>(kSurfH & 0x3FFFu);

        // Single-call GPU cached per-pixel procedural studio surface bitmap
        g.drawCachedProceduralBitmap(
            cacheKey, kSurfW, kSurfH, bounds,
            [grain, rawBaseHex](uint32_t *pixels, uint32_t w, uint32_t h) {
                synthesizeStudioSurfaceBitmap(pixels, w, h, grain, rawBaseHex);
            },
            alpha);
    }

    static void drawChassisMountingAndScrews(Graphics &g, Rect<float> bounds, bool drawScrews = true,
                                             ChassisHardwareStyle hwStyle = ChassisHardwareStyle::CornerScrews4,
                                             ScrewHeadStyle screwStyle = ScrewHeadStyle::PhillipsCross,
                                             float bevelDepthPx = 2.0f) noexcept {
        // 2-Pass Outer Chassis Bevel (1px white 16% inner lip + 1.5px black 55%
        // outer rim + directional 45-deg bevel)
        const float bevW = std::clamp(bevelDepthPx, 1.0f, 8.0f);
        g.drawLine({bounds.x + 5.0f, bounds.y + 1.5f}, {bounds.right() - 5.0f, bounds.y + 1.5f}, Color::fromHex(0xFFFFFF, 0.22f), 2.0f);
        g.drawLine({bounds.x + 1.5f, bounds.y + 5.0f}, {bounds.x + 1.5f, bounds.bottom() - 5.0f}, Color::fromHex(0xFFFFFF, 0.16f), 2.0f);
        g.drawLine({bounds.x + 5.0f, bounds.bottom() - 1.5f}, {bounds.right() - 5.0f, bounds.bottom() - 1.5f},
                   Color::fromHex(0x020507, 0.78f), 2.2f);
        g.drawLine({bounds.right() - 1.5f, bounds.y + 5.0f}, {bounds.right() - 1.5f, bounds.bottom() - 5.0f},
                   Color::fromHex(0x020507, 0.78f), 2.2f);
        g.drawRoundedRect(bounds.reduced(1.2f), 5.5f, Color(1.0f, 1.0f, 1.0f, 0.16f), 1.0f);
        g.drawRoundedRect(bounds.reduced(0.5f), 6.0f, Color(0.0f, 0.0f, 0.0f, 0.55f), std::max(1.5f, bevW * 0.75f));

        // Hardware Mounting Details (7 mounting layouts + 10 screw head styles)
        const float scaleX = bounds.width / 1000.0f;
        const float earW = 26.0f * scaleX;
        const ChassisHardwareStyle effectiveHw = drawScrews ? hwStyle : ChassisHardwareStyle::CleanScrewless;
        if (effectiveHw == ChassisHardwareStyle::CornerScrews4) {
            drawRackScrew(g, {bounds.x + 13.0f * scaleX, bounds.y + 14.0f}, 5.5f, screwStyle);
            drawRackScrew(g, {bounds.right() - 13.0f * scaleX, bounds.y + 14.0f}, 5.5f, screwStyle);
            drawRackScrew(g, {bounds.x + 13.0f * scaleX, bounds.bottom() - 14.0f}, 5.5f, screwStyle);
            drawRackScrew(g, {bounds.right() - 13.0f * scaleX, bounds.bottom() - 14.0f}, 5.5f, screwStyle);
        } else if (effectiveHw == ChassisHardwareStyle::RackEarsAndBolts) {
            Rect<float> leftEar = {bounds.x, bounds.y, earW, bounds.height};
            Rect<float> rightEar = {bounds.right() - earW, bounds.y, earW, bounds.height};
            g.fillRect(leftEar, Color::fromHex(0x0C1014, 0.45f));
            g.fillRect(rightEar, Color::fromHex(0x0C1014, 0.45f));
            g.drawLine({leftEar.right(), bounds.y}, {leftEar.right(), bounds.bottom()}, Color::fromHex(0x080A0C), 1.5f);
            g.drawLine({rightEar.x, bounds.y}, {rightEar.x, bounds.bottom()}, Color::fromHex(0x080A0C), 1.5f);

            for (float ry : {bounds.y + 24.0f, bounds.y + bounds.height * 0.5f, bounds.bottom() - 24.0f}) {
                g.fillRoundedRect({bounds.x + 4.0f * scaleX, ry - 5.0f, 18.0f * scaleX, 10.0f}, 4.5f, Color::fromHex(0x06080A));
                g.fillRoundedRect({bounds.right() - 22.0f * scaleX, ry - 5.0f, 18.0f * scaleX, 10.0f}, 4.5f, Color::fromHex(0x06080A));
                drawRackScrew(g, {bounds.x + 13.0f * scaleX, ry}, 5.0f, screwStyle);
                drawRackScrew(g, {bounds.right() - 13.0f * scaleX, ry}, 5.0f, screwStyle);
            }
        } else if (effectiveHw == ChassisHardwareStyle::IndustrialRivets8) {
            for (int i = 0; i < 4; ++i) {
                const float ry = bounds.y + 22.0f + static_cast<float>(i) * ((bounds.height - 44.0f) / 3.0f);
                drawRackScrew(g, {bounds.x + 11.0f, ry}, 4.5f, screwStyle);
                drawRackScrew(g, {bounds.right() - 11.0f, ry}, 4.5f, screwStyle);
            }
        } else if (effectiveHw == ChassisHardwareStyle::HeavyPerimeter12) {
            for (int i = 0; i < 4; ++i) {
                const float ry = bounds.y + 20.0f + static_cast<float>(i) * ((bounds.height - 40.0f) / 3.0f);
                drawRackScrew(g, {bounds.x + 11.0f, ry}, 4.5f, screwStyle);
                drawRackScrew(g, {bounds.right() - 11.0f, ry}, 4.5f, screwStyle);
            }
            for (int i = 1; i <= 2; ++i) {
                const float rx = bounds.x + static_cast<float>(i) * (bounds.width / 3.0f);
                drawRackScrew(g, {rx, bounds.bottom() - 8.5f}, 4.2f, screwStyle);
            }
        } else if (effectiveHw == ChassisHardwareStyle::CornerGussetPlates) {
            const float gp = 28.0f;
            for (Rect<float> plate : std::array<Rect<float>, 4>{{{bounds.x + 2.0f, bounds.y + 2.0f, gp, gp},
                                                                 {bounds.right() - gp - 2.0f, bounds.y + 2.0f, gp, gp},
                                                                 {bounds.x + 2.0f, bounds.bottom() - gp - 2.0f, gp, gp},
                                                                 {bounds.right() - gp - 2.0f, bounds.bottom() - gp - 2.0f, gp, gp}}}) {
                g.fillRoundedRect(plate, 4.0f, Color::fromHex(0x282E36, 0.85f));
                g.drawRoundedRect(plate, 4.0f, Color::fromHex(0x64707C, 0.6f), 1.2f);
                drawRackScrew(g, plate.getCentre(), 5.2f, screwStyle);
            }
        } else if (effectiveHw == ChassisHardwareStyle::LeatherAmpCorners) {
            // 4 Machined Countersunk Brass Corner Bezels with user-selected Screw Head
            const float arm = 30.0f;
            const float thick = 11.0f;
            const Color capFill = Color::fromHex(0x8C6B28, 0.88f);
            const Color bevelCol = Color::fromHex(0xE8C872, 0.72f);
            // Top-Left
            g.fillRoundedRect({bounds.x, bounds.y, arm, thick}, 3.5f, capFill);
            g.fillRoundedRect({bounds.x, bounds.y, thick, arm}, 3.5f, capFill);
            g.drawLine({bounds.x + 2.5f, bounds.y + 2.5f}, {bounds.x + arm - 2.5f, bounds.y + 2.5f}, bevelCol, 1.0f);
            drawRackScrew(g, {bounds.x + 12.0f, bounds.y + 12.0f}, 4.8f, screwStyle);
            // Top-Right
            g.fillRoundedRect({bounds.right() - arm, bounds.y, arm, thick}, 3.5f, capFill);
            g.fillRoundedRect({bounds.right() - thick, bounds.y, thick, arm}, 3.5f, capFill);
            g.drawLine({bounds.right() - arm + 2.5f, bounds.y + 2.5f}, {bounds.right() - 2.5f, bounds.y + 2.5f}, bevelCol, 1.0f);
            drawRackScrew(g, {bounds.right() - 12.0f, bounds.y + 12.0f}, 4.8f, screwStyle);
            // Bottom-Left
            g.fillRoundedRect({bounds.x, bounds.bottom() - thick, arm, thick}, 3.5f, capFill);
            g.fillRoundedRect({bounds.x, bounds.bottom() - arm, thick, arm}, 3.5f, capFill);
            g.drawLine({bounds.x + 2.5f, bounds.bottom() - 2.5f}, {bounds.x + arm - 2.5f, bounds.bottom() - 2.5f}, bevelCol, 1.0f);
            drawRackScrew(g, {bounds.x + 12.0f, bounds.bottom() - 12.0f}, 4.8f, screwStyle);
            // Bottom-Right
            g.fillRoundedRect({bounds.right() - arm, bounds.bottom() - thick, arm, thick}, 3.5f, capFill);
            g.fillRoundedRect({bounds.right() - thick, bounds.bottom() - arm, thick, arm}, 3.5f, capFill);
            g.drawLine({bounds.right() - arm + 2.5f, bounds.bottom() - 2.5f}, {bounds.right() - 2.5f, bounds.bottom() - 2.5f}, bevelCol,
                       1.0f);
            drawRackScrew(g, {bounds.right() - 12.0f, bounds.bottom() - 12.0f}, 4.8f, screwStyle);
        }
    }

    static void drawBrushedAnodizedChassis(Graphics &g, Rect<float> bounds, ChassisFinish finish = ChassisFinish::AnodizedTeal,
                                           SurfaceGrain grain = SurfaceGrain::VerticalBrushed, bool drawScrews = true,
                                           ChassisHardwareStyle hwStyle = ChassisHardwareStyle::CornerScrews4,
                                           ScrewHeadStyle screwStyle = ScrewHeadStyle::PhillipsCross, bool useCustomColor = false,
                                           uint32_t customColorHex = 0x245A5C, float bevelDepthPx = 2.0f) noexcept {
        drawSurfaceOverlay(g, bounds, finish, grain, useCustomColor, customColorHex, 1.0f);
        drawChassisMountingAndScrews(g, bounds, drawScrews, hwStyle, screwStyle, bevelDepthPx);
    }

    static Color getSectionPlateColor(ChassisFinish finish, bool useCustomColor = false, uint32_t customColorHex = 0x245A5C) noexcept {
        if (useCustomColor) {
            static uint32_t s_lastHex = 0xFFFFFFFFu;
            static Color s_lastCol = Color::fromHex(0x173D3F);
            const uint32_t key = customColorHex & 0xFFFFFFu;
            if (key != s_lastHex) {
                uint32_t px = 0xFF000000u;
                synthesizeStudioSurfaceBitmap(&px, 1u, 1u, SurfaceGrain::MatteSmooth, customColorHex);
                s_lastCol = scaleColorHex(px & 0xFFFFFFu, 0.86f, 1.0f);
                s_lastHex = key;
            }
            return s_lastCol;
        }
        switch (finish) {
        case ChassisFinish::AnodizedTeal:
            return Color::fromHex(0x173D3F);
        case ChassisFinish::BritishSlateBlue:
            return Color::fromHex(0x232D39);
        case ChassisFinish::BrushedSilver:
            return Color::fromHex(0x7B838B);
        case ChassisFinish::VintageGold:
            return Color::fromHex(0x3E341E);
        case ChassisFinish::HammertoneGreen:
            return Color::fromHex(0x223224);
        case ChassisFinish::VintageCream:
            return Color::fromHex(0x464135);
        case ChassisFinish::CrimsonMastering:
            return Color::fromHex(0x38151B);
        case ChassisFinish::CobaltStudioBlue:
            return Color::fromHex(0x162946);
        case ChassisFinish::WarmCopperBronze:
            return Color::fromHex(0x3C261B);
        case ChassisFinish::SunburstAmberWood:
            return Color::fromHex(0x683510);
        case ChassisFinish::RoyalNavyBlue:
            return Color::fromHex(0x101F36);
        case ChassisFinish::CyberpunkViolet:
            return Color::fromHex(0x221036);
        case ChassisFinish::ArcticWhiteEnamel:
            return Color::fromHex(0x8E98A2);
        case ChassisFinish::ObsidianBlack:
        default:
            return Color::fromHex(0x1C2025);
        }
    }

    static void drawSectionFrame(Graphics &g, Rect<float> frame, std::string_view title, Color bgPlateColor = Color::fromHex(0x173D3F),
                                 Color accentOutline = Color::fromHex(0x385A62, 0.65f), float titleFontSize = 13.5f,
                                 SectionFrameStyle style = SectionFrameStyle::EngravedBevel,
                                 ScrewHeadStyle rivetScrewStyle = ScrewHeadStyle::DomedChromeRivet) noexcept {
        (void)titleFontSize;
        const Color textCol = g.resolveSilkscreenColor(Color::fromHex(0xEDEAE0), 0.96f);
        std::string upperTitle(title);
        for (char &ch : upperTitle) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }

        // Check for optional sub-section divider separated by '|'
        std::string mainHeader = upperTitle;
        std::string subHeader;
        const auto pipePos = upperTitle.find('|');
        if (pipePos != std::string::npos) {
            mainHeader = upperTitle.substr(0, pipePos);
            subHeader = upperTitle.substr(pipePos + 1);
        }

        // Strip leading stage prefixes like "S1 · ", "S2 - ", "S10 · " so the
        // component frame header displays strictly the component name
        if (mainHeader.size() >= 3 && mainHeader[0] == 'S' && std::isdigit(static_cast<unsigned char>(mainHeader[1]))) {
            std::size_t idx = 2;
            while (idx < mainHeader.size() && std::isdigit(static_cast<unsigned char>(mainHeader[idx]))) {
                ++idx;
            }
            while (idx < mainHeader.size() && mainHeader[idx] == ' ') {
                ++idx;
            }
            if (idx + 1 < mainHeader.size() &&
                static_cast<unsigned char>(mainHeader[idx]) == 0xC2 &&
                static_cast<unsigned char>(mainHeader[idx + 1]) == 0xB7) {
                idx += 2;
            } else if (idx < mainHeader.size() && (mainHeader[idx] == '-' || mainHeader[idx] == ':' || mainHeader[idx] == '.')) {
                ++idx;
            }
            while (idx < mainHeader.size() && mainHeader[idx] == ' ') {
                ++idx;
            }
            if (idx > 2 && idx < mainHeader.size()) {
                mainHeader.erase(0, idx);
            }
        }

        // Strip parenthetical codes like `(PR2)`, `(EQ4)`, `(CP4)` and normalize
        // spacing
        while (true) {
            const auto lp = mainHeader.find('(');
            if (lp == std::string::npos) break;
            const auto rp = mainHeader.find(')', lp);
            if (rp == std::string::npos) break;
            mainHeader.erase(lp, rp - lp + 1);
        }
        // Collapse multiple consecutive spaces into a single space
        {
            std::string collapsed;
            collapsed.reserve(mainHeader.size());
            bool prevSpace = false;
            for (char c : mainHeader) {
                if (c == ' ') {
                    if (!prevSpace && !collapsed.empty()) { collapsed.push_back(' '); }
                    prevSpace = true;
                } else {
                    collapsed.push_back(c);
                    prevSpace = false;
                }
            }
            while (!collapsed.empty() && collapsed.back() == ' ') {
                collapsed.pop_back();
            }
            mainHeader = std::move(collapsed);
        }
        auto replaceSubstring = [](std::string &s, std::string_view from, std::string_view to) {
            std::size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos) {
                s.replace(pos, from.size(), to);
                pos += to.size();
            }
        };
        replaceSubstring(mainHeader, "BRITISH 3-BAND INDUCTOR EQ", "3-BAND INDUCTOR EQ");
        replaceSubstring(mainHeader, "PASSIVE LOW-END TRICK NETWORK", "PASSIVE LOW-END EQ");
        replaceSubstring(mainHeader, "VARISPEED CAPSTAN ADT TAPE DOUBLER", "VARISPEED ADT DOUBLER");
        replaceSubstring(mainHeader, "VARIABLE-MU", "VARI-MU");
        replaceSubstring(mainHeader, "DIODE-BRIDGE COMPRESSOR", "DIODE-BRIDGE COMP");
        replaceSubstring(mainHeader, "COMPRESSOR", "COMP");
        replaceSubstring(mainHeader, "EQUALIZER", "EQ");
        replaceSubstring(mainHeader, "TRANSFORMER", "IRON");
        replaceSubstring(mainHeader, "GENERATOR", "GEN");
        replaceSubstring(mainHeader, "OSCILLATOR", "OSC");
        replaceSubstring(mainHeader, "MODULATOR", "MOD");
        replaceSubstring(mainHeader, "PROCESSOR", "PROC");
        replaceSubstring(mainHeader, "SIMULATOR", "SIM");
        replaceSubstring(mainHeader, "CONTROLLER", "CTRL");
        replaceSubstring(mainHeader, "AMPLIFIER", "AMP");
        replaceSubstring(mainHeader, "PREAMPLIFIER", "PREAMP");
        replaceSubstring(mainHeader, "REVERBERATOR", "REVERB");
        replaceSubstring(mainHeader, "ATTENUATOR", "ATTEN");
        replaceSubstring(mainHeader, "SYNTHESIZER", "SYNTH");
        replaceSubstring(mainHeader, "CONDITIONER", "COND");

        const float scaleY = g.getFontScale();
        const float hFrac = frame.height / 468.0f;

        // Count visible glyphs (treating UTF-8 multi-byte sequences like "·" as 1
        // character)
        auto countVisibleChars = [](std::string_view sv) noexcept -> std::size_t {
            std::size_t count = 0;
            for (unsigned char c : sv) {
                if ((c & 0xC0u) != 0x80u) { ++count; }
            }
            return count;
        };

        // In narrow side-by-side bays, cap visible title length at a clean word
        // boundary so Graphics::drawText never auto-shrinks the font
        const std::size_t maxChars = (frame.width < 300.0f * scaleY) ? 23u : ((frame.width < 420.0f * scaleY) ? 28u : 38u);
        while (countVisibleChars(mainHeader) > maxChars) {
            const auto lastSpace = mainHeader.rfind(' ');
            if (lastSpace == std::string::npos || lastSpace < 6) { break; }
            mainHeader.erase(lastSpace);
        }

        // Uniform, high-legibility silkscreen header font size
        const float kUniformHeaderFs = (frame.width < 300.0f * scaleY) ? 11.8f : 12.6f;
        const float topRuleY = frame.y + 9.0f * scaleY;
        const Rect<float> grooveFrame = {frame.x, topRuleY, frame.width, std::max(16.0f, frame.bottom() - topRuleY)};
        const float cx = frame.getCentre().x;
        const float measuredHeaderW = mainHeader.empty() ? 0.0f : g.measureTextWidth(mainHeader, kUniformHeaderFs, true);
        const float estHeaderW = static_cast<float>(countVisibleChars(mainHeader)) * (kUniformHeaderFs * scaleY * 0.62f);
        const float gapW = mainHeader.empty() ? 0.0f : std::min(frame.width - 14.0f, std::max(measuredHeaderW, estHeaderW) + 18.0f);
        const float gapLeft = cx - gapW * 0.5f;
        const float gapRight = cx + gapW * 0.5f;

        const bool lightMetal = isLightChassis(g);
        const Color effAccentOutline = resolveChassisAccentInk(g, accentOutline);
        (void)bgPlateColor;

        // 3-Pass Machined Routed Groove Helper (+1.0, +1.2 catchlight +
        // -0.5, -0.5 upper-left shadow + 2.2px black cut)
        const Color catchlightCol = lightMetal ? Color(1.0f, 1.0f, 1.0f, 0.68f) : Color(1.0f, 1.0f, 1.0f, 0.36f);
        const Color ulShadowCol = Color(0.0f, 0.0f, 0.0f, 0.52f);
        const Color primaryCutCol = Color(0.0f, 0.0f, 0.0f, 0.84f);

        auto draw3PassHorizontalGroove = [&](float xLeft, float xRight, float yLine) {
            if (xRight <= xLeft + 4.0f) return;
            g.drawLine({xLeft + 1.0f, yLine + 1.2f}, {xRight + 1.0f, yLine + 1.2f}, catchlightCol, 1.2f);
            g.drawLine({xLeft - 0.5f, yLine - 0.5f}, {xRight - 0.5f, yLine - 0.5f}, ulShadowCol, 1.2f);
            g.drawLine({xLeft, yLine}, {xRight, yLine}, primaryCutCol, 2.0f);
        };

        auto drawBrokenEtchedGroove = [&](Color darkGroove, Color specHighlight) {
            (void)darkGroove;
            (void)specHighlight;
            const float r = 7.0f;
            const float x0 = grooveFrame.x;
            const float y0 = topRuleY;
            const float x1 = grooveFrame.right();
            const float y1 = grooveFrame.bottom();

            // Pass 1: Lower-right catchlight (+1.0, +1.2)
            g.drawLine({x0 + 1.0f, y0 + r + 1.2f}, {x0 + 1.0f, y1 - r + 1.2f}, catchlightCol, 1.2f);
            g.drawLine({x1 + 1.0f, y0 + r + 1.2f}, {x1 + 1.0f, y1 - r + 1.2f}, catchlightCol, 1.2f);
            g.drawLine({x0 + r + 1.0f, y1 + 1.2f}, {x1 - r + 1.0f, y1 + 1.2f}, catchlightCol, 1.2f);

            // Pass 2: Upper-left shadow (-0.5, -0.5)
            g.drawLine({x0 - 0.5f, y0 + r - 0.5f}, {x0 - 0.5f, y1 - r - 0.5f}, ulShadowCol, 1.2f);
            g.drawLine({x1 - 0.5f, y0 + r - 0.5f}, {x1 - 0.5f, y1 - r - 0.5f}, ulShadowCol, 1.2f);
            g.drawLine({x0 + r - 0.5f, y1 - 0.5f}, {x1 - r - 0.5f, y1 - 0.5f}, ulShadowCol, 1.2f);

            // Pass 3: Primary 2.0px black machined cut
            g.drawLine({x0, y0 + r}, {x0, y1 - r}, primaryCutCol, 2.0f);
            g.drawLine({x1, y0 + r}, {x1, y1 - r}, primaryCutCol, 2.0f);
            g.drawLine({x0 + r, y1}, {x1 - r, y1}, primaryCutCol, 2.0f);

            // Top-left & Top-right broken horizontal segments leaving clean gap for
            // silkscreened header
            if (gapW > 0.0f && gapLeft > x0 + r + 4.0f) {
                draw3PassHorizontalGroove(x0 + r, gapLeft, y0);
                draw3PassHorizontalGroove(gapRight, x1 - r, y0);
            } else {
                draw3PassHorizontalGroove(x0 + r, x1 - r, y0);
            }

            // 4 rounded corner arcs (3-pass machined groove)
            g.drawArc({x0 + r + 1.0f, y0 + r + 1.2f}, r, -1.5707963f, 0.0f, catchlightCol, 1.1f);
            g.drawArc({x1 - r + 1.0f, y0 + r + 1.2f}, r, 0.0f, 1.5707963f, catchlightCol, 1.1f);
            g.drawArc({x1 - r + 1.0f, y1 - r + 1.2f}, r, 1.5707963f, 3.1415926f, catchlightCol, 1.2f);
            g.drawArc({x0 + r + 1.0f, y1 - r + 1.2f}, r, -3.1415926f, -1.5707963f, catchlightCol, 1.1f);

            g.drawArc({x0 + r, y0 + r}, r, -1.5707963f, 0.0f, primaryCutCol, 2.0f);
            g.drawArc({x1 - r, y0 + r}, r, 0.0f, 1.5707963f, primaryCutCol, 2.0f);
            g.drawArc({x1 - r, y1 - r}, r, 1.5707963f, 3.1415926f, primaryCutCol, 2.0f);
            g.drawArc({x0 + r, y1 - r}, r, -3.1415926f, -1.5707963f, primaryCutCol, 2.0f);
        };

        // Zero opaque fill inside SectionFrame: let the procedural chassis surface
        // shine through across 100% of the panel!
        switch (style) {
        case SectionFrameStyle::RivetedSubPlate: {
            drawBrokenEtchedGroove(primaryCutCol, catchlightCol);
            const float inset = 8.0f;
            for (Point<float> pt : std::array<Point<float>, 4>{{{grooveFrame.x + inset, grooveFrame.y + inset},
                                                                {grooveFrame.right() - inset, grooveFrame.y + inset},
                                                                {grooveFrame.x + inset, grooveFrame.bottom() - inset},
                                                                {grooveFrame.right() - inset, grooveFrame.bottom() - inset}}}) {
                drawRackScrew(g, pt, 3.4f, rivetScrewStyle);
            }
            break;
        }
        case SectionFrameStyle::DoubleHairline: {
            drawBrokenEtchedGroove(primaryCutCol, catchlightCol);
            if (grooveFrame.width > 16.0f && grooveFrame.height > 16.0f) {
                g.drawRoundedRect(grooveFrame.reduced(4.0f), 4.0f, effAccentOutline.withAlpha(lightMetal ? 0.55f : 0.32f), 1.0f);
            }
            break;
        }
        case SectionFrameStyle::HeavyConsoleBay:
        case SectionFrameStyle::TechnicalCorner:
        case SectionFrameStyle::MinimalHeader:
        case SectionFrameStyle::PipingTolex:
        case SectionFrameStyle::GothicArch:
        case SectionFrameStyle::EngravedBevel:
        default: {
            drawBrokenEtchedGroove(primaryCutCol, catchlightCol);
            break;
        }
        }

        (void)hFrac;
        (void)subHeader;

        // Silkscreen section title directly onto the brushed metal with
        // high-contrast dark engraved halo & crisp silkscreen ink
        if (!mainHeader.empty()) {
            const Color hdrPlateCol = resolveWidgetPlateFill(g, 0.58f, 0.70f);
            const bool lightPlate = isLightPlateFill(g, hdrPlateCol);
            const Color headerInk = resolvePlateInkColor(g, hdrPlateCol);
            const Color haloDark = lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.62f) : Color(0.0f, 0.0f, 0.0f, 0.85f);
            Rect<float> hdrRect = {gapLeft, topRuleY - 9.5f * scaleY, gapW, 19.0f * scaleY};
            const Rect<float> hdrPill = {hdrRect.x + 2.0f, hdrRect.y + 1.5f * scaleY, std::max(24.0f, hdrRect.width - 4.0f), 16.0f * scaleY};
            if (hdrPlateCol.a > 0.02f) {
                g.fillRoundedRect(hdrPill, 3.0f, hdrPlateCol);
                g.drawRoundedRect(hdrPill, 3.0f, lightPlate ? Color::fromHex(0x64748B, 0.42f) : Color::fromHex(0xFFFFFF, 0.18f), 0.8f);
            }
            g.drawText(mainHeader, {hdrRect.x - 0.5f, hdrRect.y + 0.7f, hdrRect.width, hdrRect.height}, haloDark, kUniformHeaderFs, true,
                       true);
            g.drawText(mainHeader, hdrRect, headerInk, kUniformHeaderFs, true, true);
        }
    }

    /**
     * @brief Renders control parameter labels using the active template's Label
     * Plaque Style:
     * - DirectSilkscreen: Crisp direct-on-chassis silkscreen ink with 1px dark
     * drop shadow (NO boxes!)
     * - EmbossedDymoTape: 3D raised glossy plastic Dymo tape strip with punched
     * white lettering
     * - TraffolytePlaque: 2-ply engraved phenolic plate with bevelled border &
     * micro-rivets
     */
    static void drawLabelPlaque(Graphics &g, Rect<float> textRect, std::string_view text, UiLabelPlaqueStyle plaqueStyle, Color textCol,
                                float fontSize, Color accentCol = Color::fromHex(0xFF9F1C), bool isBold = true) noexcept {
        if (text.empty()) return;
        std::string upperText(text);
        for (char &ch : upperText) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }

        // Render with a properly sized, semi-translucent theme-matched backing plate
        // so text is crisp while letting the chassis surface breathe through.
        (void)textCol;
        const Color plateCol = resolveWidgetPlateFill(g, 0.58f, 0.70f);
        const bool lightPlate = isLightPlateFill(g, plateCol);
        const Color effTextCol = resolvePlateInkColor(g, plateCol);
        const Color textShadow = lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.58f) : Color(0.0f, 0.0f, 0.0f, 0.82f);
        const float effFontSz = fontSize * g.getFontScale();
        const float spanCap = std::max(textRect.width + 24.0f, 132.0f);
        if (plaqueStyle == UiLabelPlaqueStyle::DirectSilkscreen || plaqueStyle == UiLabelPlaqueStyle::GlowingLaserEtch) {
            const float estW = std::clamp(static_cast<float>(upperText.size()) * (effFontSz * 0.74f) + 18.0f, 44.0f, spanCap);
            const float boxH = std::clamp(textRect.height, 13.5f, 16.5f);
            const Rect<float> pill = {textRect.x + (textRect.width - estW) * 0.5f, textRect.y + (textRect.height - boxH) * 0.5f, estW,
                                      boxH};
            if (plateCol.a > 0.02f) {
                g.fillRoundedRect(pill, 2.5f, plateCol);
                if (plaqueStyle == UiLabelPlaqueStyle::GlowingLaserEtch) {
                    g.drawRoundedRect(pill, 2.5f, accentCol.withAlpha(0.48f), 0.9f);
                } else {
                    g.drawRoundedRect(pill, 2.5f, lightPlate ? Color::fromHex(0x64748B, 0.42f) : Color::fromHex(0xFFFFFF, 0.18f), 0.8f);
                }
            }
            g.drawText(upperText, {pill.x + 0.6f, pill.y + 0.7f, pill.width, pill.height}, textShadow, fontSize, true, isBold);
            g.drawText(upperText, pill, effTextCol, fontSize, true, isBold);
            return;
        }

        const float estW = std::clamp(static_cast<float>(upperText.size()) * (effFontSz * 0.74f) + 20.0f, 46.0f, spanCap);
        const float boxH = std::clamp(textRect.height, 13.5f, 16.8f);
        Rect<float> strip = {textRect.x + (textRect.width - estW) * 0.5f, textRect.y + (textRect.height - boxH) * 0.5f, estW, boxH};

        if (plaqueStyle == UiLabelPlaqueStyle::EmbossedDymoTape) {
            if (plateCol.a > 0.02f) {
                const Color topGrad = Color(std::clamp(plateCol.r * 1.12f + 0.04f, 0.0f, 1.0f),
                                            std::clamp(plateCol.g * 1.12f + 0.04f, 0.0f, 1.0f),
                                            std::clamp(plateCol.b * 1.12f + 0.04f, 0.0f, 1.0f), plateCol.a);
                const Color botGrad = Color(plateCol.r * 0.82f, plateCol.g * 0.82f, plateCol.b * 0.82f, plateCol.a);
                g.fillLinearGradientRect(strip, topGrad, botGrad, 2.0f);
                g.drawLine({strip.x + 1.5f, strip.y + 1.0f}, {strip.right() - 1.5f, strip.y + 1.0f},
                           lightPlate ? Color::fromHex(0xFFFFFF, 0.55f) : Color::fromHex(0xFFFFFF, 0.24f), 0.8f);
                g.drawRoundedRect(strip, 2.0f, lightPlate ? Color::fromHex(0x475569, 0.42f) : Color::fromHex(0xFFFFFF, 0.16f), 0.8f);
            }
            g.drawText(upperText, {strip.x + 0.6f, strip.y + 0.7f, strip.width, strip.height}, textShadow, fontSize, true, true);
            g.drawText(upperText, strip, effTextCol, fontSize, true, true);
        } else if (plaqueStyle == UiLabelPlaqueStyle::TraffolytePlaque) {
            if (plateCol.a > 0.02f) {
                g.fillRoundedRect(strip, 2.2f, plateCol);
                g.drawRoundedRect(strip, 2.2f, lightPlate ? Color::fromHex(0x475569, 0.48f) : Color::fromHex(0xE6ECEF, 0.36f), 0.9f);
                if (strip.width >= 44.0f) {
                    const Color rivetCol = lightPlate ? Color::fromHex(0x475569, 0.65f) : Color::fromHex(0xD0D8E0, 0.68f);
                    g.fillCircle({strip.x + 3.2f, strip.getCentre().y}, 1.1f, rivetCol);
                    g.fillCircle({strip.right() - 3.2f, strip.getCentre().y}, 1.1f, rivetCol);
                }
            }
            g.drawText(upperText, {strip.x + 0.6f, strip.y + 0.7f, strip.width, strip.height}, textShadow, fontSize, true, true);
            g.drawText(upperText, strip, effTextCol, fontSize, true, true);
        }
        (void)accentCol;
    }

    static void drawRivetedNameplate(Graphics &g, Rect<float> box, std::string_view brand, std::string_view modelTitle,
                                     std::string_view sub1, std::string_view sub2,
                                     BadgeVisualStyle style = BadgeVisualStyle::BrushedAluminumPlate,
                                     Color accentCol = Color::fromHex(0xE06D1A)) noexcept {
        const float h = box.height;
        const float bScale = std::clamp(h / 88.0f, 0.85f, 1.08f);
        const float inset = 6.5f;
        const std::array<Point<float>, 4> corners = {{{box.x + inset, box.y + inset},
                                                      {box.right() - inset, box.y + inset},
                                                      {box.x + inset, box.bottom() - inset},
                                                      {box.right() - inset, box.bottom() - inset}}};

        if (style == BadgeVisualStyle::MinimalSilkscreen) {
            const bool lightBg = isLightChassis(g);
            const Color effAccent = (lightBg && (0.299f * accentCol.r + 0.587f * accentCol.g + 0.114f * accentCol.b) > 0.65f)
                ? Color::fromHex(0x1E293B)
                : accentCol;
            g.fillRoundedRect({box.x + 2.0f, box.y + 3.0f, box.width, box.height}, 4.5f, Color::fromHex(0x030709, 0.55f));
            g.fillLinearGradientRect(box, lightBg ? Color::fromHex(0xF8FAFC, 0.95f) : Color::fromHex(0x141A22, 0.94f),
                                     lightBg ? Color::fromHex(0xE2E8F0, 0.95f) : Color::fromHex(0x0A0E14, 0.96f), 4.5f);
            g.drawRoundedRect(box, 4.5f, effAccent.withAlpha(0.75f), 1.4f);
            g.drawRoundedRect(box.reduced(2.5f), 3.0f, lightBg ? Color::fromHex(0x94A3B8, 0.45f) : Color::fromHex(0xFFFFFF, 0.14f), 0.8f);
            for (Point<float> pt : corners) {
                drawRackScrew(g, pt, 2.6f, ScrewHeadStyle::DomedChromeRivet);
            }
            const Color ink = lightBg ? Color::fromHex(0x0F172A) : Color::fromHex(0xF8FAFC);
            const Color subInk = lightBg ? Color::fromHex(0x334155) : Color::fromHex(0xCBD5E1);
            g.drawText(brand.empty() ? "AVRA AUDIO" : brand, {box.x + 8.0f, box.y + h * 0.08f, box.width - 16.0f, h * 0.20f}, effAccent,
                       8.2f * bScale, true, true);
            g.drawText(modelTitle, {box.x + 6.0f, box.y + h * 0.29f, box.width - 12.0f, h * 0.28f}, ink, 12.8f * bScale, true, true);
            g.drawLine({box.x + 14.0f, box.y + h * 0.58f}, {box.right() - 14.0f, box.y + h * 0.58f}, effAccent.withAlpha(0.45f), 1.0f);
            g.drawText(sub1, {box.x + 6.0f, box.y + h * 0.62f, box.width - 12.0f, h * 0.16f}, subInk, 7.2f * bScale, true, true);
            g.drawText(sub2, {box.x + 6.0f, box.y + h * 0.78f, box.width - 12.0f, h * 0.16f}, subInk.withAlpha(0.85f), 6.6f * bScale, true, true);
            return;
        }

        // Cast drop shadow grounding the physical metal plate onto the chassis
        g.fillRoundedRect({box.x + 2.5f, box.y + 3.8f, box.width, box.height}, 4.5f, Color::fromHex(0x030709, 0.82f));

        Color brandCol = Color::fromHex(0x22282E);
        Color titleCol = Color::fromHex(0x14181C);
        Color ruleCol = Color::fromHex(0x4E5862);
        Color sub1Col = Color::fromHex(0x242A30);
        Color sub2Col = Color::fromHex(0x323A42);
        bool isLightAluminum = false;

        if (style == BadgeVisualStyle::CastBrassPlaque) {
            g.fillLinearGradientRect(box, Color::fromHex(0x241B10), Color::fromHex(0x120D07), 4.0f);
            g.drawRoundedRect(box, 4.0f, Color::fromHex(0xE8C547), 2.0f);
            g.drawRoundedRect(box.reduced(2.8f), 2.5f, Color::fromHex(0x9A7828, 0.7f), 0.9f);
            for (Point<float> pt : corners) {
                drawRackScrew(g, pt, 3.0f, ScrewHeadStyle::GoldPlatedBrass);
            }
            brandCol = Color::fromHex(0xE8C547);
            titleCol = Color::fromHex(0xFFF3C4);
            ruleCol = Color::fromHex(0xB89438);
            sub1Col = Color::fromHex(0xE0C268);
            sub2Col = Color::fromHex(0xB89C52);
        } else if (style == BadgeVisualStyle::SleekLaserGlass) {
            g.fillLinearGradientRect(box, Color::fromHex(0x161E26, 0.96f), Color::fromHex(0x090D12, 0.98f), 5.0f);
            // 2px 3D bevelled outer edge (#FFFFFF top/left highlight, #040709
            // bottom/right shadow)
            g.drawLine({box.x + 3.0f, box.y + 1.2f}, {box.right() - 3.0f, box.y + 1.2f}, Color::fromHex(0xFFFFFF, 0.42f), 2.0f);
            g.drawLine({box.x + 1.2f, box.y + 3.0f}, {box.x + 1.2f, box.bottom() - 3.0f}, Color::fromHex(0xFFFFFF, 0.30f), 2.0f);
            g.drawLine({box.x + 3.0f, box.bottom() - 1.2f}, {box.right() - 3.0f, box.bottom() - 1.2f}, Color::fromHex(0x020406, 0.92f),
                       2.0f);
            g.drawLine({box.right() - 1.2f, box.y + 3.0f}, {box.right() - 1.2f, box.bottom() - 3.0f}, Color::fromHex(0x020406, 0.92f),
                       2.0f);
            g.drawRoundedRect(box, 5.0f, accentCol.withAlpha(0.75f), 1.4f);
            for (Point<float> pt : corners) {
                drawRackScrew(g, pt, 3.0f, ScrewHeadStyle::DomedChromeRivet);
            }
            brandCol = accentCol;
            titleCol = Color::fromHex(0xF4FAFF);
            ruleCol = accentCol.withAlpha(0.45f);
            sub1Col = Color::fromHex(0xB8C8D4);
            sub2Col = Color::fromHex(0x889AA8);
        } else if (style == BadgeVisualStyle::GrimyMilitaryTag) {
            g.fillLinearGradientRect(box, Color::fromHex(0x2A2824), Color::fromHex(0x171512), 3.0f);
            g.drawRoundedRect(box, 3.0f, Color::fromHex(0x786E60), 1.4f);
            g.drawLine({box.x + 6.0f, box.y + 3.0f}, {box.x + 24.0f, box.y + 11.0f}, Color::fromHex(0xB8C0C8, 0.45f), 1.1f);
            g.drawLine({box.right() - 28.0f, box.bottom() - 9.0f}, {box.right() - 8.0f, box.bottom() - 3.0f},
                       Color::fromHex(0xB8C0C8, 0.40f), 1.1f);
            for (Point<float> pt : corners) {
                drawRackScrew(g, pt, 3.0f, ScrewHeadStyle::RustedRelicBolt);
            }
            brandCol = Color::fromHex(0xD8A858);
            titleCol = Color::fromHex(0xE8DFCE);
            ruleCol = Color::fromHex(0x6E6454);
            sub1Col = Color::fromHex(0xC4B8A2);
            sub2Col = Color::fromHex(0x9E927E);
        } else if (style == BadgeVisualStyle::VintageCreamEmblem) {
            isLightAluminum = true;
            g.fillLinearGradientRect(box, Color::fromHex(0xFAF0D8), Color::fromHex(0xDFC99E), 4.5f);
            g.drawRoundedRect(box, 4.5f, Color::fromHex(0x4A3618), 1.8f);
            g.drawRoundedRect(box.reduced(2.5f), 3.0f, Color::fromHex(0x8A6832, 0.65f), 1.0f);
            for (Point<float> pt : corners) {
                drawRackScrew(g, pt, 2.8f, ScrewHeadStyle::SlottedFlathead);
            }
            brandCol = Color::fromHex(0x6E4818);
            titleCol = Color::fromHex(0x221608);
            ruleCol = Color::fromHex(0x8A6832);
            sub1Col = Color::fromHex(0x3A2810);
            sub2Col = Color::fromHex(0x5A4220);
        } else {
            // KENNEDARTH TEAL Machined Brushed-Aluminum Nameplate Plaque (#D8DEE4 to
            // #9BA4AE)
            isLightAluminum = true;
            g.fillLinearGradientRect(box, Color::fromHex(0xDDE3E8), Color::fromHex(0x9BA4AE), 4.5f);
            for (float y = box.y + 2.5f; y < box.bottom() - 2.5f; y += 1.8f) {
                const bool isHi = (static_cast<int>(y) & 1) == 0;
                g.drawLine({box.x + 2.0f, y}, {box.right() - 2.0f, y},
                           isHi ? Color::fromHex(0xFFFFFF, 0.22f) : Color::fromHex(0x303840, 0.10f), 0.9f);
            }
            // 2px 3D bevelled outer edge (#FFFFFF top/left highlight, #20282C
            // bottom/right shadow)
            g.drawLine({box.x + 3.0f, box.y + 1.2f}, {box.right() - 3.0f, box.y + 1.2f}, Color::fromHex(0xFFFFFF, 0.82f), 1.8f);
            g.drawLine({box.x + 1.2f, box.y + 3.0f}, {box.x + 1.2f, box.bottom() - 3.0f}, Color::fromHex(0xFFFFFF, 0.65f), 1.6f);
            g.drawLine({box.x + 3.0f, box.bottom() - 1.2f}, {box.right() - 3.0f, box.bottom() - 1.2f}, Color::fromHex(0x20282C, 0.88f),
                       1.8f);
            g.drawLine({box.right() - 1.2f, box.y + 3.0f}, {box.right() - 1.2f, box.bottom() - 3.0f}, Color::fromHex(0x20282C, 0.88f),
                       1.8f);
            g.drawRoundedRect(box, 4.5f, Color::fromHex(0x182024), 1.3f);

            // 4 Realistic Corner Rivets / Screws with recessed slot shadows
            for (Point<float> pt : corners) {
                g.fillCircle({pt.x + 0.6f, pt.y + 0.9f}, 3.3f, Color::fromHex(0x1A2026, 0.65f));
                g.fillRadialGradientCircle(pt, 3.1f, Color::fromHex(0xF6FAFC), Color::fromHex(0x58626C), {-1.0f, -1.0f});
                g.drawCircle(pt, 3.1f, Color::fromHex(0x22282E), 0.8f);
                g.drawLine({pt.x - 1.8f, pt.y - 1.2f}, {pt.x + 1.8f, pt.y + 1.2f}, Color::fromHex(0x1E242A, 0.85f), 0.9f);
            }
        }

        // Convert all badge lines to uppercase
        auto toUpperStr = [](std::string_view sv) {
            std::string s(sv);
            for (char &ch : s)
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            return s;
        };
        const std::string brandUp = "AVRA AUDIO";
        const std::string titleUp = toUpperStr(modelTitle.empty() ? "CUSTOM HARDWARE" : modelTitle);
        const std::string sub1Up = toUpperStr(sub1);
        const std::string sub2Up = toUpperStr(sub2);

        if (isLightAluminum) {
            // Subtle white bottom-right engraving bevel highlight on aluminum plaque
            g.drawText(brandUp, {box.x + 8.8f, box.y + h * 0.06f + 0.8f, box.width - 16.0f, h * 0.19f}, Color::fromHex(0xFFFFFF, 0.55f),
                       8.2f * bScale, true, true);
            g.drawText(titleUp, {box.x + 8.8f, box.y + h * 0.23f + 0.9f, box.width - 16.0f, h * 0.32f}, Color::fromHex(0xFFFFFF, 0.60f),
                       13.6f * bScale, true, true);
        }

        g.drawText(brandUp, {box.x + 8.0f, box.y + h * 0.06f, box.width - 16.0f, h * 0.19f}, brandCol, 8.2f * bScale, true, true);
        g.drawText(titleUp, {box.x + 8.0f, box.y + h * 0.23f, box.width - 16.0f, h * 0.32f}, titleCol, 13.6f * bScale, true, true);

        // Etched horizontal divider rule
        g.drawLine({box.x + 14.0f, box.y + h * 0.57f}, {box.right() - 14.0f, box.y + h * 0.57f}, ruleCol, 1.1f);
        if (isLightAluminum) {
            g.drawLine({box.x + 14.0f, box.y + h * 0.57f + 1.0f}, {box.right() - 14.0f, box.y + h * 0.57f + 1.0f},
                       Color::fromHex(0xFFFFFF, 0.55f), 0.9f);
        }
        g.drawText(sub1Up, {box.x + 6.0f, box.y + h * 0.60f, box.width - 12.0f, h * 0.18f}, sub1Col, 7.3f * bScale, true, true);
        g.drawText(sub2Up, {box.x + 6.0f, box.y + h * 0.77f, box.width - 12.0f, h * 0.18f}, sub2Col, 6.8f * bScale, true, true);
    }

    static void drawEngravedNameplate(Graphics &g, Rect<float> box, std::string_view brand, std::string_view modelTitle,
                                      std::string_view sub1, std::string_view sub2 = "CUSTOM HARDWARE FOUNDRY",
                                      BadgeVisualStyle style = BadgeVisualStyle::BrushedAluminumPlate,
                                      Color accentCol = Color::fromHex(0xE06D1A)) noexcept {
        drawRivetedNameplate(g, box, brand, modelTitle, sub1, sub2, style, accentCol);
    }

    /**
     * @brief Skin-Matched Smoked-Glass Signal-Chain Navigation Tile
     * Renders a recessed smoked-glass lens tinted to the active chassis finish
     * (`skinBaseCol`) and styled with the active UI skin accent
     * (`skinAccentCol`), top specular reflection glaze, recessed bypass Jewel
     * LED, subtle glass divider, and centered luminous typography for the
     * multi-stage S1 -> S2 -> S3 navigation chain.
     */
    static void drawRivetedChainTile(Graphics &g, Rect<float> box, std::string_view stageHeader, std::string_view moduleTitle,
                                     bool isSelected, bool stageEngaged, bool darkMeters = false, float scaleFactor = 1.0f,
                                     Color skinBaseCol = Color::fromHex(0x173D3F),
                                     Color skinAccentCol = Color::fromHex(0xE07A28)) noexcept {
        const float rad = 5.0f;

        auto mixCol = [](Color a, Color b, float t, float alphaOut) noexcept -> Color {
            const float u = 1.0f - t;
            return {std::clamp(a.r * u + b.r * t, 0.0f, 1.0f), std::clamp(a.g * u + b.g * t, 0.0f, 1.0f),
                    std::clamp(a.b * u + b.b * t, 0.0f, 1.0f), alphaOut};
        };

        const Color darkObsidianTop = Color::fromHex(0x090D11);
        const Color darkObsidianBot = Color::fromHex(0x040608);

        // 1. Recessed chassis socket shadow + bottom-right metal lip highlight
        // tinted to chassis
        g.fillRoundedRect({box.x - 1.0f, box.y - 1.0f, box.width + 2.0f, box.height + 2.5f}, rad + 1.0f, Color::fromHex(0x020406, 0.84f));
        g.drawLine({box.x + 4.0f, box.bottom() + 1.0f}, {box.right() - 4.0f, box.bottom() + 1.0f},
                   mixCol(skinBaseCol, Color::fromHex(0xFFFFFF), 0.55f, 0.22f), 1.0f);

        // 2. Skin-Tinted Smoked Dark-Glass Body Gradient
        const Color tintedTopBase = mixCol(darkObsidianTop, skinBaseCol, isSelected ? 0.42f : 0.28f, 1.0f);
        const Color glassTop = isSelected ? mixCol(tintedTopBase, skinAccentCol, 0.14f, 0.94f) : tintedTopBase.withAlpha(0.90f);
        const Color glassBot = mixCol(darkObsidianBot, skinBaseCol, isSelected ? 0.22f : 0.14f, isSelected ? 0.97f : 0.95f);
        g.fillLinearGradientRect(box, glassTop, glassBot, rad);

        // 3. Upper-Half Translucent Glass Specular Reflection Glaze + Top Rim Glint
        const Rect<float> glossBand = {box.x + 1.5f, box.y + 1.2f, std::max(4.0f, box.width - 3.0f), box.height * 0.45f};
        const Color glossTint = mixCol(Color::fromHex(0xFFFFFF), skinBaseCol, 0.22f, isSelected ? 0.16f : 0.10f);
        g.fillLinearGradientRect(glossBand, glossTint, glossTint.withAlpha(0.01f), rad - 1.0f);
        g.drawLine({box.x + 5.0f, box.y + 1.3f}, {box.right() - 5.0f, box.y + 1.3f}, Color::fromHex(0xFFFFFF, isSelected ? 0.36f : 0.20f),
                   1.1f);

        // 4. Glass Bevel Border & Active Selection Rim matched to UI Skin
        const Color activeRim = {std::clamp(skinAccentCol.r * 1.12f + 0.06f, 0.25f, 1.0f),
                                 std::clamp(skinAccentCol.g * 1.12f + 0.06f, 0.25f, 1.0f),
                                 std::clamp(skinAccentCol.b * 1.12f + 0.06f, 0.20f, 1.0f), 1.0f};
        const Color idleRim = mixCol(skinBaseCol, Color::fromHex(0x90A4B0), 0.38f, 0.85f);
        if (isSelected) {
            g.drawRoundedRect({box.x + 1.0f, box.y + 1.0f, box.width - 2.0f, box.height - 2.0f}, rad - 0.5f, activeRim.withAlpha(0.28f),
                              1.8f);
            g.drawRoundedRect(box, rad, activeRim.withAlpha(0.92f), 1.5f);
            g.fillRoundedRect({box.x + 8.0f, box.bottom() - 2.4f, std::max(8.0f, box.width - 16.0f), 2.0f}, 1.0f,
                              activeRim.withAlpha(0.95f));
        } else {
            g.drawRoundedRect({box.x + 0.8f, box.y + 0.8f, box.width - 1.6f, box.height - 1.6f}, rad - 0.5f,
                              Color::fromHex(0x000000, 0.65f), 1.0f);
            g.drawRoundedRect(box, rad, idleRim, 1.1f);
        }

        // 5. Recessed Jewel LED indicator centered vertically on the left
        (void)moduleTitle;
        drawJewelLed(g, {box.x + 12.5f * scaleFactor, box.y + box.height * 0.50f}, !darkMeters && stageEngaged,
                     stageEngaged ? (isSelected ? activeRim : Color::fromHex(0x00E676)) : Color::fromHex(0x4A545E),
                     std::max(2.8f, 3.3f * scaleFactor));

        // 6. Centered Luminous Stage Label ("S1", "S2", etc.) across full pill height
        auto toUpperStr = [](std::string_view sv) {
            std::string s(sv);
            for (char &ch : s)
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            return s;
        };
        std::string hdrUp = toUpperStr(stageHeader);
        const auto dotPos = hdrUp.find('\xC2');
        if (dotPos != std::string::npos) {
            hdrUp = hdrUp.substr(0, dotPos);
        }
        while (!hdrUp.empty() && hdrUp.back() == ' ') {
            hdrUp.pop_back();
        }

        const Rect<float> hdrR = {box.x + 18.0f * scaleFactor, box.y, std::max(20.0f, box.width - 36.0f * scaleFactor),
                                  box.height};

        const Color idleHdrCol = mixCol(skinBaseCol, Color::fromHex(0xF0F6FA), 0.78f, 1.0f);
        const Color activeHdrCol = mixCol(activeRim, Color::fromHex(0xFFFFFF), 0.25f, 1.0f);
        const Color hdrCol = !stageEngaged ? Color::fromHex(0x64727C) : (isSelected ? activeHdrCol : idleHdrCol);
        const float hdrPt = !stageEngaged ? 10.2f : (isSelected ? 11.2f : 10.6f);

        // Dark 1px glass depth drop-shadow + luminous centered stage designator (S1, S2, ...)
        g.drawText(hdrUp, {hdrR.x + 0.8f, hdrR.y + 0.9f, hdrR.width, hdrR.height}, Color::fromHex(0x000000, 0.85f), hdrPt, true, true);
        g.drawText(hdrUp, hdrR, hdrCol, hdrPt, true, true);
    }

    static void drawDigitalTrimBargraph(Graphics &g, Rect<float> box, float inDb, float outDb) noexcept {
        // Deep recessed dark bay with top/left shadow bevel and bottom/right
        // specular highlight
        g.fillRoundedRect(box, 3.5f, Color::fromHex(0x070B0D));
        g.drawLine({box.x + 2.0f, box.y + 1.0f}, {box.right() - 2.0f, box.y + 1.0f}, Color::fromHex(0x020405, 0.95f), 1.8f);
        g.drawLine({box.x + 1.0f, box.y + 2.0f}, {box.x + 1.0f, box.bottom() - 2.0f}, Color::fromHex(0x020405, 0.95f), 1.8f);
        g.drawLine({box.x + 2.0f, box.bottom()}, {box.right() - 2.0f, box.bottom()}, Color::fromHex(0x385A62, 0.65f), 1.1f);
        g.drawRoundedRect(box, 3.5f, Color::fromHex(0x162428), 1.1f);

        const float h = box.height;
        const float rowH = h * 0.30f;
        const float yIn = box.y + h * 0.06f;
        const float yOut = box.y + h * 0.37f;
        const float yDel = box.y + h * 0.67f;

        g.drawText("IN", {box.x + 6.0f, yIn, 30.0f, rowH}, Color::fromHex(0xB8C6CC), 7.8f, false, true);
        g.drawText("OUT", {box.x + 6.0f, yOut, 30.0f, rowH}, Color::fromHex(0xB8C6CC), 7.8f, false, true);
        g.drawText("OUT-IN", {box.x + 6.0f, yDel, 42.0f, rowH}, Color::fromHex(0xB8C6CC), 7.4f, false, true);

        const int numSegs = 20;
        const float segStartX = box.x + 42.0f;
        const float segStepX = std::max(2.2f, (box.width - 48.0f) / static_cast<float>(numSegs));
        const float segW = std::max(1.8f, segStepX - 1.4f);
        const float barH = std::clamp(rowH * 0.62f, 4.5f, 8.5f);

        const float inNorm = math::clamp((inDb + 48.0f) / 54.0f, 0.0f, 1.0f);
        const float outNorm = math::clamp((outDb + 48.0f) / 54.0f, 0.0f, 1.0f);

        for (int i = 0; i < numSegs; ++i) {
            const float thresh = static_cast<float>(i + 1) / static_cast<float>(numSegs);
            const float sx = segStartX + static_cast<float>(i) * segStepX;
            const Color activeCol = (i >= 17) ? Color::fromHex(0xF04848) : (i >= 13 ? Color::fromHex(0xF4A226) : Color::fromHex(0x4CC870));
            const Color dimCol = Color::fromHex(0x151E22);

            const bool litIn = (inNorm >= thresh);
            const bool litOut = (outNorm >= thresh);
            g.fillRect({sx, yIn + (rowH - barH) * 0.5f, segW, barH}, litIn ? activeCol : dimCol);
            g.fillRect({sx, yOut + (rowH - barH) * 0.5f, segW, barH}, litOut ? activeCol : dimCol);
        }

        float diffDb = outDb - inDb;
        if (std::abs(diffDb) < 0.05f) { diffDb = 0.0f; }
        const std::string diffStr =
            (inDb <= -80.0f && outDb <= -80.0f) ? "0.0 dB" : ((diffDb == 0.0f) ? "0.0 dB" : std::format("{:+.1f} dB", diffDb));
        g.drawText(diffStr, {box.right() - 56.0f, yDel, 50.0f, rowH}, Color::fromHex(0xE08A38), 8.2f, false, true);
    }

    /**
     * @brief Draws a high-contrast smoked backing pill behind switch state labels
     * ("MONO (PURE)", "STEREO", "IN", "BYP", "-20 dB", etc.) so state labels
     * remain 100% legible over any backdrop texture.
     */
    static void drawSwitchStatePill(Graphics &g, float cx, float labelY, std::string_view text, bool isActive, Color accentCol,
                                    float fontSize = 8.2f, float maxW = 104.0f) noexcept {
        if (text.empty()) return;
        const Color plateCol = resolveWidgetPlateFill(g, 0.56f, 0.68f);
        const bool lightPlate = isLightPlateFill(g, plateCol);
        const float fs = std::max(fontSize, 8.2f);
        const float effFs = fs * g.getFontScale();
        const float spanCap = std::max(maxW, 128.0f);
        const float pillW = std::clamp(static_cast<float>(text.size()) * (effFs * 0.72f) + 16.0f, 32.0f, spanCap);
        const Rect<float> pill = {cx - pillW * 0.5f, labelY + 0.3f, pillW, 13.2f};
        if (plateCol.a > 0.02f) {
            g.fillRoundedRect(pill, 2.5f, plateCol);
            if (isActive) {
                g.drawRoundedRect(pill, 2.5f, accentCol.withAlpha(0.58f), 1.0f);
            } else {
                g.drawRoundedRect(pill, 2.5f, lightPlate ? Color::fromHex(0x64748B, 0.42f) : Color::fromHex(0xFFFFFF, 0.18f), 0.8f);
            }
        }
        const Color baseInk = resolvePlateInkColor(g, plateCol);
        const Color activeInk = resolvePlateReadoutInk(g, plateCol, accentCol);
        const Color ink = isActive ? activeInk : baseInk;
        g.drawText(text, {pill.x + 0.6f, labelY + 0.7f, pill.width, 13.4f},
                   lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.58f) : Color(0.0f, 0.0f, 0.0f, 0.82f), fs, true, isActive);
        g.drawText(text, {pill.x, labelY, pill.width, 13.4f}, ink, fs, true, isActive);
    }

    /**
     * @brief Recessed Studio Console Push Button with 3D Mechanical Cap &
     * Embedded Jewel LED Bar (Replaces giant bright colored rectangles with
     * authentic physical studio switch hardware).
     */
    static void drawIlluminatedPushButton(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view stateSub,
                                          Color litColor = Color::fromHex(0xFFA033),
                                          UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = stateSub.empty() ? std::string_view("IN") : stateSub;
        std::string_view offText = "OUT";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }

        // Compact utility push buttons (e.g., height <= 36px without a '|' pole
        // state pair):
        const bool isCompactUtilityBtn = (bounds.height <= 36.0f && pipePos == std::string_view::npos && !title.empty());
        if (isCompactUtilityBtn) {
            Rect<float> bezel = {bounds.x + 2.0f, bounds.y, std::max(34.0f, bounds.width - 4.0f), std::clamp(bounds.height, 20.0f, 28.0f)};

            g.fillRoundedRect({bezel.x + 1.5f, bezel.y + 2.0f, bezel.width, bezel.height}, 3.5f, Color::fromHex(0x030608, 0.85f));
            g.fillRoundedRect(bezel, 3.5f, Color::fromHex(0x0D1115));
            g.drawRoundedRect(bezel, 3.5f, Color::fromHex(0x343E46), 1.1f);

            Rect<float> cap = bezel.reduced(2.2f);
            g.fillLinearGradientRect(cap, isOn ? Color::fromHex(0x283038) : Color::fromHex(0x323A42), Color::fromHex(0x161B20), 2.5f);
            g.drawLine({cap.x + 2.0f, cap.y + 1.0f}, {cap.right() - 2.0f, cap.y + 1.0f}, Color::fromHex(0xFFFFFF, 0.22f), 1.0f);
            g.drawRoundedRect(cap, 2.5f, Color::fromHex(0x0A0D10), 1.0f);

            Rect<float> ledBar = {cap.x + 8.0f, cap.y + 2.2f, std::max(12.0f, cap.width - 16.0f), 3.2f};
            if (isOn) {
                Rect<float> tightBloom = {std::max(cap.x + 1.5f, ledBar.x - 2.5f), std::max(cap.y + 0.8f, ledBar.y - 1.5f),
                                          std::min(cap.width - 3.0f, ledBar.width + 5.0f),
                                          std::min(cap.height - 2.0f, ledBar.height + 3.0f)};
                g.fillRoundedRect(tightBloom, 2.0f, litColor.withAlpha(0.28f));
                g.fillRoundedRect(ledBar, 1.5f, litColor);
                g.drawLine({ledBar.x + 1.5f, ledBar.y + 0.9f}, {ledBar.right() - 1.5f, ledBar.y + 0.9f}, Color::fromHex(0xFFFFFF, 0.78f),
                           1.0f);
            } else {
                g.fillRoundedRect(ledBar, 1.5f, Color::fromHex(0x2A1E14));
                g.drawRoundedRect(ledBar, 1.5f, Color::fromHex(0x140E0A), 0.8f);
            }

            g.drawText(title, {cap.x + 2.0f, cap.y + 6.0f, cap.width - 4.0f, std::max(11.0f, cap.height - 6.0f)},
                       isOn ? litColor : Color::fromHex(0xDCE6EC), 8.2f, true, true);
            return;
        }

        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const bool isCompact = (bounds.height < 68.0f);
        const float titleH = hasBottomTitle ? 15.5f : 0.0f;
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy =
            hasBottomTitle ? ((bounds.height <= 64.0f)
                                  ? (bounds.y + 12.0f)
                                  : (bounds.y + std::clamp((bounds.height - 18.0f) * 0.5f, 26.0f, std::max(26.0f, bounds.height - 46.0f))))
                           : (bounds.y + bounds.height * 0.50f);

        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float topLabelY = cy - (isCompact ? 24.5f : 27.5f);
        const float botLabelY = cy + (isCompact ? 12.0f : 15.5f);
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);

        drawSwitchStatePill(g, cx, topLabelY, onText, isOn, litColor, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, botLabelY, offText, !isOn, litColor, stateFs, maxSpanW);

        const float btnW = std::clamp(bounds.width - 18.0f, 44.0f, 58.0f);
        const float btnH = isCompact ? 21.5f : 24.0f;
        Rect<float> bezel = {cx - btnW * 0.5f, cy - btnH * 0.5f, btnW, btnH};

        // Recessed shadow well + dark anodized switch housing collar
        g.fillRoundedRect({bezel.x + 1.5f, bezel.y + 2.2f, bezel.width, bezel.height}, 3.5f, Color::fromHex(0x030608, 0.85f));
        g.fillRoundedRect(bezel, 3.5f, Color::fromHex(0x0B0E12));
        g.drawRoundedRect(bezel, 3.5f, Color::fromHex(0x3A444E), 1.2f);

        // 3D Sculpted Mechanical Push-Button Cap (depresses slightly inward when
        // ON)
        Rect<float> cap = bezel.reduced(2.2f);
        if (isOn) {
            cap.y += 0.6f;
            g.fillLinearGradientRect(cap, litColor.withAlpha(0.36f), Color::fromHex(0x181E24), 2.5f);
            g.fillRadialGradientCircle(cap.getCentre(), cap.width * 0.58f, litColor.withAlpha(0.32f), litColor.withAlpha(0.0f));
            g.drawRoundedRect(cap, 2.5f, litColor.withAlpha(0.75f), 1.0f);
        } else {
            g.fillRoundedRect({cap.x + 0.8f, cap.y + 1.4f, cap.width, cap.height}, 2.5f, Color::fromHex(0x040608, 0.80f));
            g.fillLinearGradientRect(cap, Color::fromHex(0x363E46), Color::fromHex(0x181D22), 2.5f);
            g.drawLine({cap.x + 2.0f, cap.y + 1.0f}, {cap.right() - 2.0f, cap.y + 1.0f}, Color::fromHex(0xFFFFFF, 0.28f), 1.0f);
            g.drawRoundedRect(cap, 2.5f, Color::fromHex(0x0A0D10), 1.0f);
        }

        // Embedded Jewel LED Lens Bar along top of cap
        Rect<float> ledBar = {cap.x + 5.0f, cap.y + 2.2f, cap.width - 10.0f, 3.4f};
        if (isOn) {
            g.fillRoundedRect(ledBar, 1.5f, litColor);
            g.drawLine({ledBar.x + 1.5f, ledBar.y + 0.9f}, {ledBar.right() - 1.5f, ledBar.y + 0.9f}, Color::fromHex(0xFFFFFF, 0.85f), 1.0f);
        } else {
            g.fillRoundedRect(ledBar, 1.5f, Color::fromHex(0x261C14));
            g.drawRoundedRect(ledBar, 1.5f, Color::fromHex(0x120C08), 0.8f);
        }

        // Tactile micro-grip ridges on lower cap face (never duplicate long state
        // labels inside the cap)
        const float gripY = cap.y + cap.height * 0.64f;
        g.drawLine({cap.x + 8.0f, gripY - 1.6f}, {cap.right() - 8.0f, gripY - 1.6f},
                   isOn ? litColor.withAlpha(0.65f) : Color::fromHex(0x64748B, 0.55f), 1.1f);
        g.drawLine({cap.x + 8.0f, gripY + 1.6f}, {cap.right() - 8.0f, gripY + 1.6f},
                   isOn ? litColor.withAlpha(0.65f) : Color::fromHex(0x64748B, 0.55f), 1.1f);

        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, titleH}, title, plaqueStyle,
                            g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.6f, litColor, true);
        }
    }

    /**
     * @brief Formats verbose parameter titles into concise, high-legibility
     * studio fader labels so multi-band graphic EQs and narrow fader banks
     * render at full font size without horizontal cramping or pill collisions.
     */
    static std::string compactFaderTitle(std::string_view rawTitle, float faderWidth) noexcept {
        while (!rawTitle.empty() && rawTitle.front() == ' ') rawTitle.remove_prefix(1);
        while (!rawTitle.empty() && rawTitle.back() == ' ') rawTitle.remove_suffix(1);
        if (rawTitle.empty()) return {};

        // 1. Frequency-prefixed graphic EQ faders ("50 Hz Sub-Bass Tank" -> "50 HZ",
        //    "2.0 kHz Vocal" -> "2.0 KHZ", "31 Hz Band Gain" -> "31 HZ")
        if (std::isdigit(static_cast<unsigned char>(rawTitle.front()))) {
            std::string s(rawTitle);
            for (std::string_view unitTok : {"kHz", "khz", "KHz", "KHZ", "Hz", "hz", "HZ"}) {
                const auto pos = s.find(unitTok);
                if (pos != std::string::npos && pos <= 8) {
                    std::string freqPart = s.substr(0, pos + unitTok.size());
                    for (char &ch : freqPart) {
                        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                    }
                    return freqPart;
                }
            }
        }

        std::string up(rawTitle);
        for (char &ch : up) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }

        // 2. Curated studio fader shorthand for common multi-fader modules
        if (up == "TARGET BASS LEVEL" || up == "TARGET VOCAL LEVEL") return (faderWidth < 68.0f) ? "TARGET LVL" : "TARGET LEVEL";
        if (up == "TARGET LOUDNESS") return "TARGET LOUD";
        if (up == "LEVELLER RANGE") return (faderWidth < 68.0f) ? "RANGE" : "LEVEL RANGE";
        if (up == "MAX GAIN RANGE") return "GAIN RANGE";
        if (up == "SWEET-SPOT TRIM") return "SWEET TRIM";
        if (up == "MASTER FADER TRIM") return "MASTER TRIM";
        if (up == "OUTPUT FADER LEVEL" || up == "OUTPUT FADER") return "OUT FADER";
        if (up == "LOW SHELF GAIN") return "LOW SHELF";
        if (up == "LOW-MID BELL GAIN") return "LOW-MID";
        if (up == "HIGH-MID BELL GAIN") return "HIGH-MID";
        if (up == "HIGH SHELF GAIN") return "HIGH SHELF";
        if (up == "GAIN / THRESHOLD") return "GAIN / THR";
        if (up == "MID RT60 DECAY") return "MID DECAY";
        if (up == "BASS MULTIPLY") return "BASS MULT";
        if (up == "CROSSOVER FREQ") return "XOVER FREQ";
        if (up == "TREBLE ROLL-OFF") return "HF ROLL-OFF";

        // 3. Strip redundant trailing words if the label is still long
        const size_t maxChars = (faderWidth < 58.0f) ? 9u : 12u;
        for (std::string_view suffix : {" TANK", " BAND GAIN", " BELL GAIN", " SHELF GAIN", " FADER", " GAIN", " LEVEL"}) {
            if (up.size() > maxChars && up.ends_with(suffix)) {
                up.erase(up.size() - suffix.size());
                break;
            }
        }
        return up;
    }

    /**
     * @brief Tall console / graphic-EQ vertical fader: recessed slot, 5
     * graduation ticks with a highlighted 0 dB centre detent, sculpted metallic
     * cap, band title + value readout underneath.
     */
    static void drawVerticalStudioFader(Graphics &g, Rect<float> bounds, float norm, std::string_view title, std::string_view valText,
                                        Color accentCol = Color::fromHex(0x00F5D4),
                                        std::span<const std::string> customTicks = {},
                                        UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        const float cx = bounds.x + bounds.width * 0.5f;
        const float slotW = 5.0f;
        const float slotY0 = bounds.y + 14.0f;
        const float slotY1 = std::max(slotY0 + 24.0f, bounds.bottom() - 42.0f);
        const float slotH = slotY1 - slotY0;
        const float n = math::clamp(norm, 0.0f, 1.0f);
        const Color faderAccent = resolveChassisAccentInk(g, accentCol);
        const Color tickCol = g.resolveSilkscreenColor(Color::fromHex(0xDCE6EC), 0.94f);

        // 5 graduation ticks top-to-bottom (norm 1.0 down to 0.0); uses customTicks
        // (ordered min..max) when supplied.
        constexpr std::array<const char *, 5> kDefaultGrad = {{"+12", "+6", "0", "-6", "-12"}};
        std::array<std::string_view, 5> gradLabels{};
        bool hasExplicitZero = false;
        for (size_t i = 0; i < 5; ++i) {
            if (customTicks.size() >= 2) {
                const float normTick = 1.0f - static_cast<float>(i) / 4.0f;
                const size_t idx = std::min(customTicks.size() - 1,
                                            static_cast<size_t>(std::round(normTick * static_cast<float>(customTicks.size() - 1))));
                gradLabels[i] = customTicks[idx];
            } else {
                gradLabels[i] = kDefaultGrad[i];
            }
            if (gradLabels[i] == "0" || gradLabels[i] == "0.0" || gradLabels[i] == "+0" || gradLabels[i] == "-0" || gradLabels[i] == "C") {
                hasExplicitZero = true;
            }
        }
        const bool roomForNumbers = bounds.width >= 44.0f;
        const float tickInner = slotW * 0.5f + 3.0f;
        const float tickOuter = std::min(bounds.width * 0.5f - 1.0f, tickInner + 8.0f);
        for (size_t i = 0; i < gradLabels.size(); ++i) {
            const float ty = slotY0 + slotH * (static_cast<float>(i) / 4.0f);
            const std::string_view lbl = gradLabels[i];
            const bool isZero = hasExplicitZero ? (lbl == "0" || lbl == "0.0" || lbl == "+0" || lbl == "-0" || lbl == "C") : (i == 2);
            const Color tc = isZero ? faderAccent : tickCol;
            const float lw = isZero ? 1.8f : 1.0f;
            g.drawLine({cx - tickOuter, ty}, {cx - tickInner, ty}, tc, lw);
            g.drawLine({cx + tickInner, ty}, {cx + tickOuter, ty}, tc, lw);
            if (roomForNumbers && !lbl.empty()) {
                const float pillW = std::clamp(static_cast<float>(lbl.size()) * 5.2f + 8.0f, 18.0f, 30.0f);
                const float pillX = (cx - tickOuter) - pillW - 2.5f;
                const Rect<float> tickPill = {pillX, ty - 6.0f, pillW, 12.0f};
                const Color fPlateCol = resolveWidgetPlateFill(g, 0.58f, 0.70f);
                const bool lightPlate = isLightPlateFill(g, fPlateCol);
                if (fPlateCol.a > 0.02f) {
                    g.fillRoundedRect(tickPill, 2.0f, fPlateCol);
                    g.drawRoundedRect(tickPill, 2.0f, lightPlate ? Color::fromHex(0x64748B, 0.38f) : Color::fromHex(0xFFFFFF, 0.16f), 0.65f);
                }
                const Color lblInk = isZero ? resolvePlateReadoutInk(g, fPlateCol, faderAccent) : resolvePlateInkColor(g, fPlateCol);
                g.drawText(lbl, tickPill, lblInk, 8.0f, true, true);
            }
        }

        // Recessed track slot
        const Rect<float> slot = {cx - slotW * 0.5f, slotY0 - 2.0f, slotW, slotH + 4.0f};
        g.fillRoundedRect(slot, 2.5f, Color::fromHex(0x05080A));
        g.drawLine({slot.x + 0.5f, slot.y + 1.0f}, {slot.x + 0.5f, slot.bottom() - 1.0f}, Color::fromHex(0x000000, 0.9f), 1.0f);
        g.drawRoundedRect(slot, 2.5f, Color::fromHex(0x2B333C), 1.0f);

        // Sculpted studio fader cap
        const float capW = std::min(30.0f, bounds.width * 0.58f);
        const float capH = 15.0f;
        const float capY = slotY1 - n * slotH; // cap centre line
        const Rect<float> cap = {cx - capW * 0.5f, capY - capH * 0.5f, capW, capH};
        g.fillRoundedRect({cap.x + 1.5f, cap.y + 3.0f, cap.width, cap.height}, 2.5f, Color::fromHex(0x020304, 0.85f));
        g.fillLinearGradientRect(cap, Color::fromHex(0x6A737C), Color::fromHex(0x1C2126), 2.5f);
        g.fillLinearGradientRect({cap.x + 1.5f, cap.y + 1.5f, cap.width - 3.0f, capH * 0.42f}, Color::fromHex(0xC9D1D8, 0.55f),
                                 Color::fromHex(0x3A4148, 0.0f), 1.5f);
        g.drawLine({cap.x + 2.5f, capY}, {cap.right() - 2.5f, capY}, Color::fromHex(0xF4F8FA), 1.6f);
        g.drawRoundedRect(cap, 2.5f, Color::fromHex(0x0C0F12), 1.1f);

        // Band title plaque + high-contrast value readout pill (sized to fit fader column pitch cleanly)
        const std::string shortTitle = compactFaderTitle(title, bounds.width);
        const float maxPillW = (bounds.width < 58.0f) ? (bounds.width + 12.0f) : std::clamp(bounds.width + 16.0f, 68.0f, 92.0f);
        const Color plateCol = resolveWidgetPlateFill(g, 0.62f, 0.74f);
        const bool lightPlate = isLightPlateFill(g, plateCol);
        const Color titleInk = resolvePlateInkColor(g, plateCol);
        const Color textShadow = lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.58f) : Color(0.0f, 0.0f, 0.0f, 0.82f);

        if (!shortTitle.empty()) {
            const float titleFs = (bounds.width < 56.0f) ? 8.8f : 9.4f;
            const float effTitleFs = titleFs * g.getFontScale();
            const float tPillW = std::clamp(static_cast<float>(shortTitle.size()) * (effTitleFs * 0.70f) + 12.0f, 42.0f, maxPillW);
            const Rect<float> tPill = {cx - tPillW * 0.5f, bounds.bottom() - 32.5f, tPillW, 15.2f};
            if (plateCol.a > 0.02f) {
                g.fillRoundedRect(tPill, 2.5f, plateCol);
                if (plaqueStyle == UiLabelPlaqueStyle::GlowingLaserEtch) {
                    g.drawRoundedRect(tPill, 2.5f, faderAccent.withAlpha(0.48f), 0.9f);
                } else {
                    g.drawRoundedRect(tPill, 2.5f, lightPlate ? Color::fromHex(0x64748B, 0.44f) : Color::fromHex(0xFFFFFF, 0.20f), 0.8f);
                }
            }
            g.drawText(shortTitle, {tPill.x + 0.6f, tPill.y + 0.7f, tPill.width, tPill.height}, textShadow, titleFs, true, true);
            g.drawText(shortTitle, tPill, titleInk, titleFs, true, true);
        }

        if (!valText.empty()) {
            const float valFs = (bounds.width < 56.0f) ? 8.5f : 9.0f;
            const float effValFs = valFs * g.getFontScale();
            const float pillW = std::clamp(static_cast<float>(valText.size()) * (effValFs * 0.68f) + 12.0f, 38.0f, maxPillW);
            const Rect<float> rPill = {cx - pillW * 0.5f, bounds.bottom() - 15.2f, pillW, 13.8f};
            if (plateCol.a > 0.02f) {
                g.fillRoundedRect(rPill, 2.2f, plateCol);
                g.drawRoundedRect(rPill, 2.2f, lightPlate ? Color::fromHex(0x64748B, 0.44f) : Color::fromHex(0xFFFFFF, 0.18f), 0.75f);
            }
            const Color readoutInk = resolvePlateReadoutInk(g, plateCol, faderAccent);
            g.drawText(valText, {rPill.x + 0.6f, rPill.y + 0.6f, rPill.width, rPill.height}, textShadow, valFs, true, true);
            g.drawText(valText, rPill, readoutInk, valFs, true, true);
        }
    }

    /**
     * @brief Conductive-Plastic Console Vertical Fader (kept for existing
     * callers; forwards to drawVerticalStudioFader so the faceplate preview and
     * exported plugins render identically).
     */
    static void drawStudioFader(Graphics &g, Rect<float> bounds, float norm, std::string_view title, std::string_view valText,
                                Color accentCol = Color::fromHex(0x00F5D4), std::span<const std::string> customTicks = {},
                                UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        drawVerticalStudioFader(g, bounds, norm, title, valText, accentCol, customTicks, plaqueStyle);
    }

    /**
     * @brief 20-Segment Recessed Stereo Peak LED Matrix Meter (Green / Amber /
     * Red).
     */
    static void drawLedPeakBargraph(Graphics &g, Rect<float> bounds, float normLevelL, std::string_view title,
                                    float normLevelR = -1.0f) noexcept {
        if (normLevelR < 0.0f) normLevelR = normLevelL * 0.96f;
        // Deep recessed dark bay
        g.fillRoundedRect(bounds, 4.0f, Color::fromHex(0x070B0E));
        g.drawLine({bounds.x + 2.0f, bounds.y + 1.0f}, {bounds.right() - 2.0f, bounds.y + 1.0f}, Color::fromHex(0x020406, 0.95f), 1.8f);
        g.drawLine({bounds.x + 1.0f, bounds.y + 2.0f}, {bounds.x + 1.0f, bounds.bottom() - 2.0f}, Color::fromHex(0x020406, 0.95f), 1.8f);
        g.drawRoundedRect(bounds, 4.0f, Color::fromHex(0x283E46), 1.1f);

        constexpr int kSegs = 20;
        const float topY = bounds.y + 7.0f;
        const float botY = bounds.bottom() - 19.0f;
        const float stepY = (botY - topY) / static_cast<float>(kSegs);
        const float barW = std::max(6.0f, (bounds.width - 26.0f) * 0.44f);
        const float leftX = bounds.x + 8.0f;
        const float rightX = bounds.right() - 8.0f - barW;

        for (int i = 0; i < kSegs; ++i) {
            const int segFromBot = kSegs - 1 - i;
            const float thresh = static_cast<float>(segFromBot + 1) / static_cast<float>(kSegs);
            const float sy = topY + static_cast<float>(i) * stepY;

            Color activeCol = Color::fromHex(0x4CC870);
            if (segFromBot >= 17)
                activeCol = Color::fromHex(0xF04848);
            else if (segFromBot >= 13)
                activeCol = Color::fromHex(0xF4A226);

            const bool litL = (normLevelL >= thresh);
            const bool litR = (normLevelR >= thresh);
            g.fillRoundedRect({leftX, sy, barW, std::max(2.0f, stepY - 1.6f)}, 1.2f, litL ? activeCol : Color::fromHex(0x151E24));
            g.fillRoundedRect({rightX, sy, barW, std::max(2.0f, stepY - 1.6f)}, 1.2f, litR ? activeCol : Color::fromHex(0x151E24));
        }
        drawLabelPlaque(g, {bounds.x, bounds.bottom() - 15.0f, bounds.width, 13.0f}, title, UiLabelPlaqueStyle::DirectSilkscreen,
                        g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.0f);
    }

    /**
     * @brief Machined Cooling Louver Ventilation Grille for Tube / Class-A
     * Amplifier Styling.
     */
    static void drawVentilationGrille(Graphics &g, Rect<float> bounds, std::string_view title) noexcept {
        g.fillRoundedRect(bounds, 4.0f, Color::fromHex(0x0D1115, 0.65f));
        g.drawRoundedRect(bounds, 4.0f, Color::fromHex(0x2D3640), 1.0f);

        const int cols = 2;
        const float colW = (bounds.width - 20.0f) / static_cast<float>(cols);
        for (int c = 0; c < cols; ++c) {
            const float sx = bounds.x + 7.0f + static_cast<float>(c) * (colW + 6.0f);
            for (float sy = bounds.y + 8.0f; sy < bounds.bottom() - 18.0f; sy += 7.0f) {
                g.fillRoundedRect({sx, sy, colW, 3.8f}, 1.8f, Color::fromHex(0x050709));
                g.drawLine({sx + 1.0f, sy + 4.2f}, {sx + colW - 1.0f, sy + 4.2f}, Color::fromHex(0x46525E, 0.45f), 0.8f);
            }
        }
        if (!title.empty()) {
            g.drawText(title, {bounds.x, bounds.bottom() - 14.0f, bounds.width, 12.0f},
                       g.resolveSilkscreenColor(Color::fromHex(0x9BA8B0), 0.90f), 7.8f, true, true);
        }
    }

    /**
     * @brief Circular Vintage Porthole VU Meter (Warm Cream/Amber, Ice-Blue or
     * White Broadcast Dial with Brass/Bakelite Bezel).
     */
    static void drawRoundVintageVu(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                   Color accentCol = Color::fromHex(0xFF9F1C), bool transparentBezel = false) noexcept {
        const bool isWideMasterBay = (bounds.width > bounds.height * 1.35f);
        if (isWideMasterBay && !transparentBezel) {
            // Frame the circular porthole inside a full 200x104 recessed studio
            // instrument bay so it aligns flush with the 200px output column
            g.fillRoundedRect({bounds.x + 2.0f, bounds.y + 3.0f, bounds.width, bounds.height}, 6.0f, Color::fromHex(0x030507, 0.78f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x281D16), Color::fromHex(0x140E0A), 6.0f);
            g.drawRoundedRect(bounds, 6.0f, accentCol.withAlpha(0.70f), 1.6f);
            g.drawRoundedRect(bounds.reduced(3.0f), 4.0f, Color::fromHex(0xFFE09E, 0.25f), 0.9f);
            for (Point<float> pt : std::array<Point<float>, 4>{{{bounds.x + 7.0f, bounds.y + 7.0f},
                                                                {bounds.right() - 7.0f, bounds.y + 7.0f},
                                                                {bounds.x + 7.0f, bounds.bottom() - 7.0f},
                                                                {bounds.right() - 7.0f, bounds.bottom() - 7.0f}}}) {
                drawRackScrew(g, pt, 2.8f, ScrewHeadStyle::GoldPlatedBrass);
            }
        }
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = isWideMasterBay ? (bounds.y + bounds.height * 0.48f) : (bounds.y + (bounds.height - 16.0f) * 0.5f);
        const float outerR = std::max(18.0f, std::min(bounds.width, bounds.height - (isWideMasterBay ? 8.0f : 18.0f)) * 0.45f);
        const Point<float> c{cx, cy};

        const float dialR = outerR * 0.85f;
        const bool isCoolBlue = (accentCol.b > accentCol.r + 0.15f);
        const bool isWhite = (accentCol.r > 0.85f && accentCol.g > 0.85f && accentCol.b > 0.85f);

        if (!transparentBezel) {
            // Drop shadow & heavy bakelite/brass circular bezel
            g.fillCircle({cx + 2.5f, cy + 3.5f}, outerR + 1.5f, Color::fromHex(0x040608, 0.82f));
            g.fillRadialGradientCircle(c, outerR, Color::fromHex(0x3A3F46), Color::fromHex(0x111418), {-outerR * 0.35f, -outerR * 0.35f});
            g.drawCircle(c, outerR, Color::fromHex(0x080A0C), 2.0f);
            g.drawCircle(c, outerR * 0.90f, Color::fromHex(0x9AA6B0, 0.55f), 1.2f);

            // Dial face color theme driven by accentCol (Warm Amber = Vintage
            // Yellow/Cream Paper; Cyan/Blue = Ice Blue; else Warm Cream)
            if (isCoolBlue) {
                g.fillRadialGradientCircle(c, dialR, Color::fromHex(0x3CD2FF), Color::fromHex(0x0C3A6E), {0.0f, dialR * 0.25f});
            } else if (isWhite) {
                g.fillRadialGradientCircle(c, dialR, Color::fromHex(0xFAF8F2), Color::fromHex(0xC8C2B4), {0.0f, -dialR * 0.2f});
            } else {
                g.fillRadialGradientCircle(c, dialR, Color::fromHex(0xFFE38E), Color::fromHex(0xB8741A), {0.0f, dialR * 0.25f});
            }
            g.drawCircle(c, dialR, Color::fromHex(0x18120A), 1.4f);
        }

        const Point<float> pivot = {cx, cy + dialR * 0.58f};
        const float arcR = dialR * 1.02f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;
        const Color inkCol = isCoolBlue ? Color::fromHex(0xE8F8FF) : Color::fromHex(0x22180C);
        const Color redCol = isCoolBlue ? Color::fromHex(0xFF4D6D) : Color::fromHex(0xC81E1E);

        g.drawArc(pivot, arcR, leftAng, 0.24f, inkCol, 1.4f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, redCol, 2.2f);

        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            const Color tc = (i >= 4) ? redCol : inkCol;
            g.drawLine({pivot.x + (arcR - 2.0f) * s, pivot.y - (arcR - 2.0f) * co},
                       {pivot.x + (arcR + 5.0f) * s, pivot.y - (arcR + 5.0f) * co}, tc, (i >= 4) ? 1.8f : 1.2f);
            const float lx = pivot.x + (arcR + 11.5f) * s;
            const float ly = pivot.y - (arcR + 11.5f) * co;
            g.drawText(kLbl[i], {lx - 12.0f, ly - 6.0f, 24.0f, 12.0f}, tc, 7.2f, true, i >= 4);
        }

        g.drawText("VU", {cx - 20.0f, cy - dialR * 0.08f, 40.0f, 16.0f}, inkCol, 10.0f, true, true);

        float needleNorm = 0.0f;
        if (outDb > -52.0f) {
            needleNorm = (outDb <= -14.0f) ? ((outDb + 52.0f) / 38.0f) * 0.80f : (0.80f + ((outDb + 14.0f) / 18.0f) * 0.20f);
            needleNorm = math::clamp(needleNorm, 0.0f, 1.0f);
        }
        const float na = leftAng + needleNorm * (rightAng - leftAng);
        const float ns = std::sin(na), nc = std::cos(na);
        const Point<float> nBase = {pivot.x + (arcR * 0.26f) * ns, std::min(cy + dialR - 3.0f, pivot.y - (arcR * 0.26f) * nc)};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * ns, pivot.y - (arcR + 4.0f) * nc};
        g.drawLine({nBase.x + 1.2f, nBase.y + 1.5f}, {nTip.x + 1.2f, nTip.y + 1.5f}, Color::fromHex(0x000000, 0.35f), 2.2f);
        g.drawLine(nBase, nTip, redCol, 1.8f);
        g.fillCircle({cx, cy + dialR * 0.52f}, dialR * 0.16f, Color::fromHex(0x1E2228));

        (void)grDb;
        g.drawText(title, {bounds.x, bounds.bottom() - 14.0f, bounds.width, 13.0f}, g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2)),
                   8.4f, true, true);
        if (isWideMasterBay) {
            g.drawText("PEAK", {bounds.x + 10.0f, cy - 14.0f, 38.0f, 11.0f}, Color::fromHex(0xFFE09E, 0.85f), 6.8f, true, true);
            drawJewelLed(g, {bounds.x + 29.0f, cy + 4.0f}, outDb > -3.0f, Color::fromHex(0xFF3B30), 3.4f);
            const std::string dbStr = (outDb <= -58.0f) ? "-OO dB" : std::format("{:+.1f} dB", outDb);
            g.drawText("LEVEL", {bounds.right() - 48.0f, cy - 14.0f, 38.0f, 11.0f}, Color::fromHex(0xFFE09E, 0.85f), 6.8f, true, true);
            g.drawText(dbStr, {bounds.right() - 52.0f, cy + 0.0f, 46.0f, 12.0f}, accentCol, 7.2f, true, true);
        }
    }

    /**
     * @brief Recessed Glass Vacuum Tube Porthole Window with Anode Plates, Grid
     * Wires & Reactive Cathode Glow. Dynamically brightens and pulses in
     * intensity based on the stage's live Drive setting and real-time audio
     * envelope / THD.
     */
    static void drawGlowingTubeWindow(Graphics &g, Rect<float> bounds, float outDb, float grDb, std::string_view title,
                                      Color accentCol = Color::fromHex(0xFF9F1C)) noexcept {
        const bool hasTitle = !title.empty();
        const float captionH = hasTitle ? 13.0f : 2.0f;
        const float padX = std::clamp(bounds.width * 0.08f, 3.0f, 6.0f);
        const float winH = std::max(28.0f, bounds.height - captionH - 4.0f);
        const float winW = std::clamp(std::min(bounds.width - padX * 2.0f, winH * 0.78f), 26.0f, 86.0f);
        Rect<float> win = {bounds.x + (bounds.width - winW) * 0.5f, bounds.y + 2.0f, winW, winH};
        const float radius = win.width * 0.48f;

        // 1. Recessed dark circular/pill-shaped chassis cutout with chamfered
        // metallic bezel
        g.fillRoundedRect({win.x + 1.6f, win.y + 2.4f, win.width, win.height}, radius, Color::fromHex(0x030507, 0.88f));
        g.fillLinearGradientRect(win, Color::fromHex(0x788490), Color::fromHex(0x283038), radius);
        g.drawRoundedRect(win, radius, Color::fromHex(0x101418), 1.4f);

        Rect<float> well = win.reduced(2.8f);
        const float wellR = std::max(4.0f, radius - 2.4f);
        g.fillLinearGradientRect(well, Color::fromHex(0x090706), Color::fromHex(0x160E09), wellR);
        g.drawRoundedRect(well, wellR, Color::fromHex(0x040506, 0.92f), 1.6f);

        // Compute reactive filament intensity from stage Drive (encoded in
        // grDb/outDb) + live audio envelope
        const float envNorm = math::clamp((outDb + 48.0f) / 48.0f, 0.0f, 1.0f);
        const float driveNorm = math::clamp(grDb / 18.0f, 0.0f, 1.0f);
        const float sigDrive = math::clamp(0.28f + 0.44f * envNorm + 0.38f * driveNorm, 0.25f, 1.0f);
        const Point<float> c = well.getCentre();
        const float minDim = std::min(well.width, well.height);

        // 2. Dynamic Reactive Filament Glow (#FF6A00 -> #FFAA33) behind and inside
        // the glass envelope
        g.fillRadialGradientCircle(c, minDim * (0.48f + 0.14f * sigDrive), Color::fromHex(0xFF6A00, 0.34f + 0.48f * sigDrive),
                                   Color::fromHex(0xFF3300, 0.0f));

        // 3. Internal glass vacuum tube envelope with dark-grey anode plates and
        // fine horizontal grid wires
        const float tubeW = std::min(42.0f, well.width * 0.64f);
        const float tubeH = std::min(78.0f, well.height * 0.84f);
        Rect<float> glass = {c.x - tubeW * 0.5f, c.y - tubeH * 0.48f, tubeW, tubeH};
        // Chrome getter flash at top of glass dome
        g.fillRadialGradientCircle({c.x, glass.y + tubeW * 0.28f}, tubeW * 0.38f, Color::fromHex(0x88929C, 0.38f),
                                   Color::fromHex(0x1E2226, 0.0f));
        g.drawRoundedRect(glass, tubeW * 0.44f, Color::fromHex(0xFFD8A8, 0.48f), 1.2f);

        // Dark-grey dual-triode anode plate cage + mica spacers
        Rect<float> plate = glass.reduced(tubeW * 0.18f, tubeH * 0.16f);
        g.fillRoundedRect(plate, 2.2f, Color::fromHex(0x24262A, 0.90f));
        g.drawRoundedRect(plate, 2.2f, Color::fromHex(0x5E646C, 0.85f), 0.9f);
        g.drawLine({glass.x + 2.0f, plate.y}, {glass.right() - 2.0f, plate.y}, Color::fromHex(0x9E9488, 0.65f), 1.0f);
        g.drawLine({glass.x + 2.0f, plate.bottom()}, {glass.right() - 2.0f, plate.bottom()}, Color::fromHex(0x9E9488, 0.65f), 1.0f);

        // Fine horizontal control-grid wires inside the anode window
        for (float wy = plate.y + 3.0f; wy < plate.bottom() - 2.5f; wy += 3.2f) {
            g.drawLine({plate.x + 2.0f, wy}, {plate.right() - 2.0f, wy}, Color::fromHex(0xB88652, 0.42f + 0.30f * sigDrive), 0.8f);
        }

        // Central glowing orange/amber cathode filament column + top/bottom heater
        // tips (#FF6A00 -> #FFAA33 -> #FFF2B2)
        const float coreR = std::max(3.5f, tubeW * (0.24f + 0.10f * sigDrive));
        g.fillRadialGradientCircle(c, coreR * 1.45f, Color::fromHex(0xFFAA33, 0.55f + 0.42f * sigDrive), Color::fromHex(0xFF6A00, 0.0f));
        g.drawLine({c.x, plate.y + 1.5f}, {c.x, plate.bottom() - 1.5f}, Color::fromHex(0xFFF2B2, 0.72f + 0.28f * sigDrive), 2.0f);
        const Color heaterCol = Color::fromHex(0xFFF4B8, 0.70f + 0.30f * sigDrive);
        g.fillRadialGradientCircle({c.x, plate.y + 2.0f}, tubeW * 0.28f, heaterCol, Color::fromHex(0xFF6A00, 0.0f));
        g.fillRadialGradientCircle({c.x, plate.bottom() - 2.0f}, tubeW * 0.32f, heaterCol, Color::fromHex(0xFF6A00, 0.0f));

        // 4. Protective wire/mesh grille lines across the recessed cutout
        for (float gy = well.y + 6.0f; gy < well.bottom() - 5.0f; gy += 5.5f) {
            g.drawLine({well.x + 2.5f, gy}, {well.right() - 2.5f, gy}, Color::fromHex(0x080706, 0.62f), 1.1f);
            g.drawLine({well.x + 2.5f, gy - 0.6f}, {well.right() - 2.5f, gy - 0.6f}, Color::fromHex(0x78828C, 0.24f), 0.7f);
        }

        (void)accentCol;
        if (hasTitle) {
            g.drawText(title, {bounds.x - 2.0f, bounds.bottom() - captionH, bounds.width + 4.0f, captionH},
                       g.resolveSilkscreenColor(Color::fromHex(0xDCE2E6), 0.90f), 7.6f, true, true);
        }
    }

    /**
     * @brief Phosphor CRT / OLED Waveform & Stereo Lissajous Scope Screen.
     */
    static void drawCrtOscilloscopeScreen(Graphics &g, Rect<float> bounds, float outDb, float grDb, std::string_view title,
                                          Color accentCol = Color::fromHex(0x00F5D4)) noexcept {
        Rect<float> scr = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 18.0f};
        g.fillRoundedRect(scr, 4.5f, Color::fromHex(0x071014));
        g.drawRoundedRect(scr, 4.5f, Color::fromHex(0x2C3E48), 1.4f);

        // Reticle crosshair & graticule grid
        const Point<float> c = scr.getCentre();
        for (int i = 1; i <= 3; ++i) {
            const float gx = scr.x + scr.width * (static_cast<float>(i) * 0.25f);
            const float gy = scr.y + scr.height * (static_cast<float>(i) * 0.25f);
            g.drawLine({gx, scr.y + 3.0f}, {gx, scr.bottom() - 3.0f}, accentCol.withAlpha(0.14f), 0.8f);
            g.drawLine({scr.x + 3.0f, gy}, {scr.right() - 3.0f, gy}, accentCol.withAlpha(0.14f), 0.8f);
        }

        // Dynamic harmonic waveform scaled by live signal level
        const float amp = (outDb <= -58.0f) ? 0.04f : math::clamp((outDb + 42.0f) / 44.0f, 0.08f, 0.92f);
        std::array<Point<float>, 32> wave{};
        for (size_t i = 0; i < wave.size(); ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(wave.size() - 1);
            const float phase = t * 6.2831853f * 2.0f + outDb * 0.35f;
            const float yVal = (std::sin(phase) + 0.32f * std::sin(phase * 3.0f)) * 0.75f * amp;
            wave[i] = {scr.x + 6.0f + t * (scr.width - 12.0f), c.y - yVal * (scr.height * 0.38f)};
        }
        g.drawPolyline(wave, accentCol.withAlpha(0.35f), 3.4f);
        g.drawPolyline(wave, accentCol, 1.6f);

        (void)grDb;
        g.drawText(title, {bounds.x, bounds.bottom() - 14.0f, bounds.width, 13.0f}, g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2)),
                   8.2f, true, true);
    }

    /**
     * @brief Backlit Digital LCD / VFD Studio Readout Screen (80s/90s Digital
     * Rack & Synth Display).
     */
    static void drawLcdParameterReadout(Graphics &g, Rect<float> bounds, float inDb, float outDb, float grDb, std::string_view title,
                                        std::string_view subText, Color accentCol = Color::fromHex(0x00F5D4)) noexcept {
        Rect<float> lcd = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
        g.fillRoundedRect(lcd, 4.0f, Color::fromHex(0x08161C));
        g.drawRoundedRect(lcd, 4.0f, accentCol.withAlpha(0.65f), 1.3f);

        g.drawText(title, {lcd.x + 8.0f, lcd.y + 4.0f, lcd.width - 16.0f, 14.0f}, accentCol.withAlpha(0.80f), 8.0f, false, true);
        const std::string mainStr = (outDb <= -58.0f) ? "IDLE  //  48.0 kHz" : std::format("OUT {:+.1f} dB   GR -{:.1f} dB", outDb, grDb);
        g.drawText(mainStr, {lcd.x + 8.0f, lcd.y + lcd.height * 0.34f, lcd.width - 16.0f, 20.0f}, accentCol, 11.0f, false, true);
        const std::string botStr = subText.empty() ? std::format("IN {:+.1f} dB  |  2x OVERSAMPLED", inDb) : std::string(subText);
        g.drawText(botStr, {lcd.x + 8.0f, lcd.bottom() - 16.0f, lcd.width - 16.0f, 13.0f}, Color::fromHex(0x9CB6C0), 7.8f, false, false);
    }

    /**
     * @brief Illuminated Studio Mains / Channel Rocker Switch
     * (`ROCKER_POWER_SWITCH`). Renders a 3D Beveled Upper/Lower Rocker Paddle
     * with neon/amber backlit upper lens window, paired left jewel LED,
     * top/bottom state labels, and bottom title plaque.
     */
    static void drawRockerPowerSwitch(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title,
                                      Color accentCol = Color::fromHex(0xFF4D5A), std::string_view stateSub = "ON|OFF",
                                      UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view topLbl = "ON";
        std::string_view botLbl = "OFF";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            topLbl = stateSub.substr(0, pipePos);
            botLbl = stateSub.substr(pipePos + 1);
            const auto secondPipe = botLbl.find('|');
            if (secondPipe != std::string_view::npos) { botLbl = botLbl.substr(secondPipe + 1); }
        } else if (!stateSub.empty()) {
            topLbl = stateSub;
        }

        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const bool isCompact = (bounds.height < 68.0f);
        const float titleH = hasBottomTitle ? 15.5f : 0.0f;
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy =
            hasBottomTitle ? ((bounds.height <= 64.0f)
                                  ? (bounds.y + 12.0f)
                                  : (bounds.y + std::clamp((bounds.height - 18.0f) * 0.5f, 26.0f, std::max(26.0f, bounds.height - 46.0f))))
                           : (bounds.y + bounds.height * 0.50f);

        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float topLabelY = cy - (isCompact ? 24.5f : 27.5f);
        const float botLabelY = cy + (isCompact ? 12.0f : 15.5f);
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);

        drawSwitchStatePill(g, cx, topLabelY, topLbl, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, botLabelY, botLbl, !isOn, accentCol, stateFs, maxSpanW);

        drawJewelLed(g, {cx - 24.5f, cy}, isOn, accentCol, isCompact ? 3.5f : 3.8f);

        const float rockW = isCompact ? 26.5f : 30.0f;
        const float rockH = isCompact ? 22.0f : 25.0f;
        Rect<float> housing = {cx - rockW * 0.5f, cy - rockH * 0.5f, rockW, rockH};

        // Recessed shadow well + beveled metallic rocker bezel
        g.fillRoundedRect({housing.x + 1.5f, housing.y + 2.2f, housing.width, housing.height}, 3.2f, Color::fromHex(0x040608, 0.85f));
        g.fillRoundedRect(housing, 3.2f, Color::fromHex(0x0D1014));
        g.drawRoundedRect(housing, 3.2f, Color::fromHex(0x46525C), 1.3f);

        Rect<float> rocker = housing.reduced(2.4f);
        const float midY = rocker.getCentre().y;
        const Rect<float> topHalf = {rocker.x, rocker.y, rocker.width, rocker.height * 0.50f};
        const Rect<float> botHalf = {rocker.x, midY, rocker.width, rocker.height * 0.50f};

        if (isOn) {
            // Top half pressed inward & glowing; bottom half tilted outward catching
            // specular light
            g.fillLinearGradientRect(topHalf, accentCol.withAlpha(0.88f), accentCol.withAlpha(0.52f), 1.8f);
            g.fillRadialGradientCircle(topHalf.getCentre(), rocker.width * 0.55f, Color::fromHex(0xFFF6C8, 0.52f),
                                       accentCol.withAlpha(0.0f));
            g.fillLinearGradientRect(botHalf, Color::fromHex(0x444E58), Color::fromHex(0x22282E), 1.8f);
            g.drawLine({botHalf.x + 1.5f, botHalf.bottom() - 1.0f}, {botHalf.right() - 1.5f, botHalf.bottom() - 1.0f},
                       Color::fromHex(0xFFFFFF, 0.35f), 1.1f);
        } else {
            // Top half tilted outward catching specular highlight; bottom half
            // pressed inward
            g.fillLinearGradientRect(topHalf, Color::fromHex(0x444E58), Color::fromHex(0x242A30), 1.8f);
            g.drawLine({topHalf.x + 1.5f, topHalf.y + 1.0f}, {topHalf.right() - 1.5f, topHalf.y + 1.0f}, Color::fromHex(0xFFFFFF, 0.38f),
                       1.1f);
            g.fillLinearGradientRect(botHalf, Color::fromHex(0x161A1E), Color::fromHex(0x242A30), 1.8f);
        }

        // Crisp center pivot crease + I / O markings
        g.drawLine({rocker.x + 1.0f, midY}, {rocker.right() - 1.0f, midY}, Color::fromHex(0x06080A, 0.88f), 1.3f);
        g.drawText("I", topHalf, isOn ? Color::fromHex(0x160C08) : Color::fromHex(0xDCE4EA), 7.4f, true, true);
        g.drawText("O", botHalf, isOn ? Color::fromHex(0xDCE4EA) : accentCol, 7.4f, true, true);

        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, titleH}, title, plaqueStyle,
                            g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.6f, accentCol, true);
        }
    }

    /**
     * @brief Horizontal Studio Ribbon / Crossfader Slider with Center Detent &
     * Machined Cap.
     */
    static void drawHorizontalCrossfader(Graphics &g, Rect<float> bounds, float norm, std::string_view title, std::string_view valText,
                                         Color accentCol = Color::fromHex(0x00F5D4)) noexcept {
        const float trackY = bounds.y + (bounds.height - 16.0f) * 0.48f;
        const float leftX = bounds.x + 14.0f;
        const float rightX = bounds.right() - 14.0f;
        const float trackW = std::max(24.0f, rightX - leftX);

        // Tick marks along horizontal track
        for (int i = 0; i <= 10; ++i) {
            const float tx = leftX + (static_cast<float>(i) / 10.0f) * trackW;
            const bool isCenter = (i == 5);
            const Color tc = isCenter ? accentCol : g.resolveSilkscreenColor(Color::fromHex(0x9BA8B0), 0.75f);
            g.drawLine({tx, trackY - (isCenter ? 10.0f : 7.0f)}, {tx, trackY + (isCenter ? 10.0f : 7.0f)}, tc, isCenter ? 1.5f : 0.9f);
        }

        Rect<float> slot = {leftX - 2.0f, trackY - 3.0f, trackW + 4.0f, 6.0f};
        g.fillRoundedRect(slot, 2.5f, Color::fromHex(0x080B0E));
        g.drawRoundedRect(slot, 2.5f, Color::fromHex(0x2B333C), 1.0f);

        const float capX = leftX + math::clamp(norm, 0.0f, 1.0f) * trackW;
        g.fillRect({leftX, trackY - 1.5f, std::max(0.0f, capX - leftX), 3.0f}, accentCol.withAlpha(0.75f));

        Rect<float> cap = {capX - 9.0f, trackY - 11.0f, 18.0f, 22.0f};
        g.fillRoundedRect({cap.x + 1.5f, cap.y + 2.0f, cap.width, cap.height}, 2.5f, Color::fromHex(0x040608, 0.85f));
        g.fillLinearGradientRect(cap, Color::fromHex(0xDCE2E8), Color::fromHex(0x5A626A), 2.5f);
        g.drawLine({capX, cap.y + 3.0f}, {capX, cap.bottom() - 3.0f}, accentCol, 2.0f);
        g.drawRoundedRect(cap, 2.5f, Color::fromHex(0x161A1E), 1.1f);

        std::string caption = valText.empty() ? std::string(title) : std::format("{} · {}", title, valText);
        for (char &ch : caption) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
        const float cx = bounds.x + bounds.width * 0.5f;
        const float fs = 8.8f;
        const float effFs = fs * g.getFontScale();
        const float pillW = std::clamp(static_cast<float>(caption.size()) * (effFs * 0.68f) + 14.0f, 52.0f, std::max(64.0f, bounds.width));
        const Rect<float> cPill = {cx - pillW * 0.5f, bounds.bottom() - 15.8f, pillW, 14.6f};
        const Color plateCol = resolveWidgetPlateFill(g, 0.60f, 0.72f);
        const bool lightPlate = isLightPlateFill(g, plateCol);
        if (plateCol.a > 0.02f) {
            g.fillRoundedRect(cPill, 2.5f, plateCol);
            g.drawRoundedRect(cPill, 2.5f, lightPlate ? Color::fromHex(0x64748B, 0.42f) : Color::fromHex(0xFFFFFF, 0.18f), 0.75f);
        }
        const Color ink = resolvePlateInkColor(g, plateCol);
        const Color textShadow = lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.58f) : Color(0.0f, 0.0f, 0.0f, 0.82f);
        g.drawText(caption, {cPill.x + 0.6f, cPill.y + 0.7f, cPill.width, cPill.height}, textShadow, fs, true, true);
        g.drawText(caption, cPill, ink, fs, true, true);
    }

    /**
     * @brief Warm Glowing Vintage Nixie Tube / Cathode Numeric Telemetry Display.
     */
    static void drawNixieTubeDisplay(Graphics &g, Rect<float> bounds, float outDb, float grDb, std::string_view title,
                                     std::string_view subtitle = "", Color accentCol = Color::fromHex(0xFF7B00)) noexcept {
        (void)subtitle;
        (void)accentCol;
        Rect<float> housing = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 16.0f};
        g.fillRoundedRect(housing, 5.0f, Color::fromHex(0x0D0906));
        g.drawRoundedRect(housing, 5.0f, Color::fromHex(0x4E3822), 1.5f);

        const std::string valStr = (outDb <= -58.0f) ? "-OO dB" : std::format("{:+05.1f}", outDb);
        const size_t numTubes = 5;
        const float padX = 8.0f;
        const float tubeW = (housing.width - padX * 2.0f - (numTubes - 1) * 4.0f) / static_cast<float>(numTubes);
        const float tubeH = housing.height - 10.0f;

        for (size_t i = 0; i < numTubes; ++i) {
            const float tx = housing.x + padX + static_cast<float>(i) * (tubeW + 4.0f);
            Rect<float> bulb = {tx, housing.y + 5.0f, tubeW, tubeH};
            g.fillLinearGradientRect(bulb, Color::fromHex(0x1C120B), Color::fromHex(0x0E0805), tubeW * 0.35f);
            g.fillRadialGradientCircle(bulb.getCentre(), tubeW * 0.65f, Color::fromHex(0xFF6B00, 0.36f), Color::fromHex(0xFF3300, 0.0f));
            g.drawRoundedRect(bulb, tubeW * 0.35f, Color::fromHex(0x6E5238, 0.65f), 1.0f);
            const std::string chStr(1, (i < valStr.size()) ? valStr[i] : '0');
            g.drawText(chStr, bulb, Color::fromHex(0xFFB703), std::min(16.0f, tubeH * 0.56f), true, true);
        }
        const std::string botTitle = (grDb > 0.1f) ? std::format("{}  (GR -{:.1f} dB)", title, grDb) : std::string(title);
        g.drawText(botTitle, {bounds.x, bounds.bottom() - 14.0f, bounds.width, 13.0f}, g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2)),
                   8.0f, true, true);
    }

    /**
     * @brief Stereo Lissajous Goniometer / Phase Vectorscope Radar Display.
     */
    static void drawStereoGoniometerScope(Graphics &g, Rect<float> bounds, float outDb, float grDb, std::string_view title,
                                          Color accentCol = Color::fromHex(0x00F5D4)) noexcept {
        (void)grDb;
        Rect<float> scr = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 16.0f};
        g.fillRoundedRect(scr, 5.0f, Color::fromHex(0x060E12));
        g.drawRoundedRect(scr, 5.0f, Color::fromHex(0x2A3C46), 1.4f);

        const Point<float> c = scr.getCentre();
        const float r = std::max(14.0f, std::min(scr.width, scr.height) * 0.42f);
        g.drawCircle(c, r, accentCol.withAlpha(0.22f), 1.0f);
        g.drawCircle(c, r * 0.55f, accentCol.withAlpha(0.14f), 0.8f);
        // M/S and L/R diagonal axes
        g.drawLine({c.x, c.y - r}, {c.x, c.y + r}, accentCol.withAlpha(0.20f), 0.9f);
        g.drawLine({c.x - r, c.y}, {c.x + r, c.y}, accentCol.withAlpha(0.20f), 0.9f);
        const float d = r * 0.707f;
        g.drawLine({c.x - d, c.y - d}, {c.x + d, c.y + d}, accentCol.withAlpha(0.18f), 0.8f);
        g.drawLine({c.x - d, c.y + d}, {c.x + d, c.y - d}, accentCol.withAlpha(0.18f), 0.8f);

        const float amp = (outDb <= -58.0f) ? 0.08f : math::clamp((outDb + 42.0f) / 44.0f, 0.15f, 0.95f);
        std::array<Point<float>, 36> liss{};
        for (size_t i = 0; i < liss.size(); ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(liss.size() - 1) * 6.2831853f * 2.0f;
            const float lx = std::sin(t * 1.5f + outDb * 0.2f) * r * 0.48f * amp;
            const float ly = std::cos(t + outDb * 0.1f) * r * 0.82f * amp;
            liss[i] = {c.x + lx, c.y + ly};
        }
        g.drawPolyline(liss, accentCol.withAlpha(0.32f), 3.0f);
        g.drawPolyline(liss, accentCol, 1.4f);

        g.drawText(title, {bounds.x, bounds.bottom() - 14.0f, bounds.width, 13.0f}, g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2)),
                   8.0f, true, true);
    }

    /**
     * @brief Cast-Aluminum 19" Studio Rack Grab-Handle Bar.
     */
    static void drawRackHandlePair(Graphics &g, Rect<float> bounds, std::string_view title) noexcept {
        (void)title;
        const float cx = bounds.getCentre().x;
        Rect<float> colTop = {cx - bounds.width * 0.38f, bounds.y + 2.0f, bounds.width * 0.76f, 12.0f};
        Rect<float> colBot = {cx - bounds.width * 0.38f, bounds.bottom() - 14.0f, bounds.width * 0.76f, 12.0f};
        g.fillRoundedRect(colTop, 3.0f, Color::fromHex(0x1C2026));
        g.fillRoundedRect(colBot, 3.0f, Color::fromHex(0x1C2026));
        g.drawRoundedRect(colTop, 3.0f, Color::fromHex(0x4A525C), 1.0f);
        g.drawRoundedRect(colBot, 3.0f, Color::fromHex(0x4A525C), 1.0f);

        const float barW = std::clamp(bounds.width * 0.44f, 10.0f, 22.0f);
        Rect<float> bar = {cx - barW * 0.5f, bounds.y + 8.0f, barW, std::max(16.0f, bounds.height - 16.0f)};
        g.fillRoundedRect({bar.x + 2.5f, bar.y + 2.0f, bar.width, bar.height}, barW * 0.4f, Color::fromHex(0x040608, 0.80f));
        g.fillLinearGradientRect(bar, Color::fromHex(0xDCE4EC), Color::fromHex(0x646E78), barW * 0.4f);
        g.drawLine({bar.x + bar.width * 0.32f, bar.y + 4.0f}, {bar.x + bar.width * 0.32f, bar.bottom() - 4.0f},
                   Color::fromHex(0xFFFFFF, 0.55f), 1.6f);
        g.drawRoundedRect(bar, barW * 0.4f, Color::fromHex(0x1E242A), 1.2f);
    }

    /**
     * @brief 1/4" Chrome TRS Hardware Patchbay Jack Socket.
     */
    static void drawPatchbayJackSocket(Graphics &g, Rect<float> bounds, std::string_view title,
                                       Color accentCol = Color::fromHex(0xFF9F1C)) noexcept {
        const float captionH = 12.5f;
        const Point<float> c = {bounds.getCentre().x, bounds.y + (bounds.height - captionH) * 0.50f};
        const float r = std::max(9.0f, std::min(bounds.width, bounds.height - captionH) * 0.38f);

        // Shadow & outer washer
        g.fillCircle({c.x + 1.5f, c.y + 2.2f}, r + 1.0f, Color::fromHex(0x040608, 0.82f));
        g.fillRadialGradientCircle(c, r, Color::fromHex(0xD8E0E8), Color::fromHex(0x525A64), {-r * 0.3f, -r * 0.3f});
        g.drawCircle(c, r, Color::fromHex(0x1A1E24), 1.1f);

        // Hex nut polygon
        std::array<Point<float>, 7> hex{};
        for (size_t i = 0; i < 7; ++i) {
            const float a = static_cast<float>(i) * (3.14159265f / 3.0f);
            hex[i] = {c.x + std::cos(a) * r * 0.82f, c.y + std::sin(a) * r * 0.82f};
        }
        g.drawPolyline(hex, Color::fromHex(0x283038), 1.3f);

        // Chrome sleeve ferrule & dark 1/4" jack bore
        g.fillRadialGradientCircle(c, r * 0.60f, Color::fromHex(0xFAFDFF), Color::fromHex(0x68727C), {-r * 0.2f, -r * 0.2f});
        g.fillCircle(c, r * 0.38f, Color::fromHex(0x06080A));
        g.drawCircle(c, r * 0.42f, accentCol.withAlpha(0.65f), 0.9f);

        g.drawText(title, {bounds.x - 4.0f, bounds.bottom() - captionH, bounds.width + 8.0f, captionH},
                   g.resolveSilkscreenColor(Color::fromHex(0xE8F1F2)), 7.6f, true, true);
    }

    /**
     * @brief Industrial / Military Toggle Switch flanked by U-Shaped Tubular
     * Steel Safety Guard Cage Bars (`LOCKING_CAGE_TOGGLE`).
     */
    static void drawLockingCageToggle(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title,
                                      std::string_view stateSub = "IN|BYP", Color accentCol = Color::fromHex(0xFF9F1C),
                                      UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view topLbl = "ARM";
        std::string_view botLbl = "SAFE";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            topLbl = stateSub.substr(0, pipePos);
            botLbl = stateSub.substr(pipePos + 1);
            const auto secondPipe = botLbl.find('|');
            if (secondPipe != std::string_view::npos) { botLbl = botLbl.substr(secondPipe + 1); }
        }

        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const bool isCompact = (bounds.height < 68.0f);
        const float titleH = hasBottomTitle ? 15.5f : 0.0f;
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy =
            hasBottomTitle ? ((bounds.height <= 64.0f)
                                  ? (bounds.y + 12.0f)
                                  : (bounds.y + std::clamp((bounds.height - 18.0f) * 0.5f, 26.0f, std::max(26.0f, bounds.height - 46.0f))))
                           : (bounds.y + bounds.height * 0.50f);
        const float nutR = isCompact ? 8.2f : 9.0f;

        // Paired Jewel LED on far left
        drawJewelLed(g, {cx - 26.5f, cy}, isOn, accentCol, 3.3f);

        // 1. Machined Dark Steel Safety Guard Baseplate
        Rect<float> guardPlate = {cx - 19.5f, cy - 11.5f, 39.0f, 23.0f};
        g.fillRoundedRect({guardPlate.x + 1.4f, guardPlate.y + 2.0f, guardPlate.width, guardPlate.height}, 2.8f,
                          Color::fromHex(0x030608, 0.84f));
        g.fillLinearGradientRect(guardPlate, Color::fromHex(0x2A323A), Color::fromHex(0x14181D), 2.8f);
        g.drawRoundedRect(guardPlate, 2.8f, Color::fromHex(0x46525C), 1.0f);

        // 2. Left & Right U-Shaped Tubular Chrome Safety Guard Cage Bars
        for (float side : {-1.0f, 1.0f}) {
            const float barX = cx + side * 14.5f;
            const float barTop = cy - 9.2f;
            const float barBot = cy + 9.2f;
            // Drop shadow on baseplate
            g.drawLine({barX + 1.4f, barTop + 1.4f}, {barX + 1.4f, barBot + 1.4f}, Color::fromHex(0x020406, 0.82f), 3.4f);
            // Mounting collars at top & bottom of each cage bar
            for (float my : {barTop, barBot}) {
                g.fillCircle({barX, my}, 2.5f, Color::fromHex(0x525C66));
                g.drawCircle({barX, my}, 2.5f, Color::fromHex(0x161B20), 0.7f);
            }
            // Cylindrical steel bar body + bright specular highlight
            g.drawLine({barX, barTop}, {barX, barBot}, Color::fromHex(0x6E7A86), 3.2f);
            g.drawLine({barX - 0.5f, barTop + 0.5f}, {barX - 0.5f, barBot - 0.5f}, Color::fromHex(0xF0F6FA), 1.3f);
        }

        // 3. Center 6-Sided Hex Nut & Bushing Well
        g.fillCircle({cx + 1.4f, cy + 2.2f}, nutR + 0.8f, Color::fromHex(0x030608, 0.82f));
        g.fillRadialGradientCircle({cx, cy}, nutR + 0.6f, Color::fromHex(0xA4AEB8), Color::fromHex(0x3A424A), {-2.0f, -2.0f});
        std::array<Point<float>, 7> hex{};
        for (size_t i = 0; i < 7; ++i) {
            const float a = static_cast<float>(i) * (3.14159265f / 3.0f) + 0.5235988f;
            hex[i] = {cx + std::cos(a) * nutR * 0.88f, cy + std::sin(a) * nutR * 0.88f};
        }
        g.fillPolygon(std::span<const Point<float>>(hex.data(), 6), Color::fromHex(0xC4CCD4));
        g.drawPolyline(hex, Color::fromHex(0x20262C), 1.1f);
        g.fillCircle({cx, cy}, 3.6f, Color::fromHex(0x080B0E));

        // 4. Pull-to-Unlock Lever with Anodized Red/Accent Locking Sleeve Tip
        const float batLen = isCompact ? 8.8f : 10.0f;
        const float tipY = isOn ? (cy - batLen) : (cy + batLen);
        g.drawLine({cx + 1.6f, cy + 1.6f}, {cx + 1.6f, tipY + 1.6f}, Color::fromHex(0x030608, 0.78f), isCompact ? 5.6f : 6.2f);
        const float baseHalfW = isCompact ? 2.0f : 2.3f;
        const float tipHalfW = isCompact ? 3.8f : 4.2f;
        std::array<Point<float>, 4> batPoly = {{{cx - baseHalfW, cy}, {cx - tipHalfW, tipY}, {cx + tipHalfW, tipY}, {cx + baseHalfW, cy}}};
        g.fillPolygon(batPoly, accentCol);
        g.drawLine({cx - baseHalfW * 0.5f, cy}, {cx - tipHalfW * 0.5f, tipY}, Color::fromHex(0xFFFFFF, 0.65f), 1.5f);
        g.fillRadialGradientCircle({cx, tipY}, tipHalfW, Color::fromHex(0xFFF4EC), accentCol, {-1.2f, -1.2f});
        g.drawCircle({cx, tipY}, tipHalfW, Color::fromHex(0x1E1410), 0.9f);

        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float topLabelY = cy - (isCompact ? 24.5f : 27.5f);
        const float botLabelY = cy + (isCompact ? 12.0f : 15.5f);
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, topLabelY, topLbl, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, botLabelY, botLbl, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, titleH}, title, plaqueStyle,
                            g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.6f, accentCol, true);
        }
    }

    /**
     * @brief Modern Minimalist Vector Capsule Pill Switch
     * (`FLAT_PILL_VECTOR_SWITCH`). Renders a sleek horizontal stadium/pill vector
     * capsule with a sliding circular vector thumb and glowing accent state.
     */
    static void drawFlatPillVectorSwitch(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title,
                                         std::string_view stateSub = "IN|OUT", Color accentCol = Color::fromHex(0xFFA033),
                                         UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = "IN";
        std::string_view offText = "OUT";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }

        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const bool isCompact = (bounds.height < 68.0f);
        const float titleH = hasBottomTitle ? 15.5f : 0.0f;
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy =
            hasBottomTitle ? ((bounds.height <= 64.0f)
                                  ? (bounds.y + 12.0f)
                                  : (bounds.y + std::clamp((bounds.height - 18.0f) * 0.5f, 26.0f, std::max(26.0f, bounds.height - 46.0f))))
                           : (bounds.y + bounds.height * 0.50f);

        const bool lightBg = isLightChassis(g);
        const Color activeInk = resolveChassisAccentInk(g, accentCol);
        const Color shadowCol = resolveChassisTextShadow(g, 0.55f);
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0xDCE2E6), 0.94f);

        // 1. Horizontal Flat Vector Capsule Pill Track
        const float pillW = std::clamp(bounds.width - 14.0f, 46.0f, 60.0f);
        const float pillH = isCompact ? 20.5f : 23.0f;
        const float pillR = pillH * 0.5f;
        const Rect<float> pill = {cx - pillW * 0.5f, cy - pillR, pillW, pillH};

        if (isOn) { g.drawRoundedRect(pill.reduced(-1.2f), pillR + 1.2f, accentCol.withAlpha(0.28f), 2.2f); }
        g.fillRoundedRect({pill.x + 1.0f, pill.y + 1.5f, pill.width, pill.height}, pillR, Color(0.0f, 0.0f, 0.0f, 0.45f));
        g.fillRoundedRect(pill, pillR, lightBg ? Color::fromHex(0xDCE4EC) : Color::fromHex(0x0E141A));
        if (isOn) { g.fillRoundedRect(pill, pillR, accentCol.withAlpha(lightBg ? 0.24f : 0.30f)); }
        g.drawRoundedRect(pill, pillR, isOn ? accentCol : (lightBg ? Color::fromHex(0x788898) : Color::fromHex(0x384858)),
                          isOn ? 1.6f : 1.2f);

        // 2. Vector I/O Micro-Glyph inside opposite half of the pill track
        if (isOn) {
            g.drawLine({pill.x + 9.5f, cy - 4.0f}, {pill.x + 9.5f, cy + 4.0f}, accentCol, 1.7f);
        } else {
            g.drawCircle({pill.right() - 10.0f, cy}, 3.5f, lightBg ? Color::fromHex(0x64748B) : Color::fromHex(0x526476), 1.4f);
        }

        // 3. Sliding Circular Vector Thumb Disc (Right when ON, Left when OFF)
        const float thumbR = pillR - 2.4f;
        const Point<float> thumbPt = {isOn ? (pill.right() - pillR) : (pill.x + pillR), cy};
        const float accentLum = 0.2126f * accentCol.r + 0.7152f * accentCol.g + 0.0722f * accentCol.b;
        const bool paleAccentOnLight = (lightBg && accentLum > 0.65f);
        g.fillCircle({thumbPt.x + 0.8f, thumbPt.y + 1.2f}, thumbR, Color(0.0f, 0.0f, 0.0f, 0.52f));
        const Color thumbFill = isOn ? (paleAccentOnLight ? Color::fromHex(0x1E293B) : accentCol)
                                     : (lightBg ? Color::fromHex(0x64748B) : Color::fromHex(0x8898A8));
        g.fillCircle(thumbPt, thumbR, thumbFill);
        g.drawCircle(thumbPt, thumbR,
                     (lightBg || accentLum > 0.65f) ? Color::fromHex(0x0F172A, 0.88f) : Color::fromHex(0xFFFFFF, 0.78f), 1.3f);
        const Color notchFill = isOn ? (paleAccentOnLight ? accentCol : Color::fromHex(0x0E141A, 0.90f))
                                     : Color::fromHex(0x141C24, 0.78f);
        g.fillCircle(thumbPt, thumbR * 0.36f, notchFill);
        g.drawCircle(thumbPt, thumbR * 0.36f, Color::fromHex(0x0A0F14, 0.75f), 0.8f);

        // 4. State Labels & Parameter Title
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float topLabelY = cy - (isCompact ? 24.5f : 27.5f);
        const float botLabelY = cy + (isCompact ? 12.0f : 15.5f);
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, topLabelY, onText, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, botLabelY, offText, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, titleH}, title, plaqueStyle,
                            silkInk, 8.6f, accentCol, true);
        }
        (void)activeInk;
        (void)shadowCol;
    }

    /**
     * @brief High-Resolution Anti-Aliased Backlit Transfer & Spectral Telemetry
     * Scope (`PASTEL_SHADER_CURVE`).
     */
    static void drawPastelShaderCurveMeter(Graphics &g, Rect<float> bounds, float outDb, float grDb, std::string_view title,
                                           Color accentCol = Color::fromHex(0x00F5D4), uint32_t faceplateHex = 0x0D1418) noexcept {
        Rect<float> scr = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 16.0f};
        // Deep recessed bezel with dark inner glass shadow
        g.fillRoundedRect({scr.x + 1.5f, scr.y + 2.5f, scr.width, scr.height}, 5.5f, Color::fromHex(0x030608, 0.85f));
        const Color bgTop = scaleColorHex(faceplateHex, 0.95f, 0.98f);
        const Color bgBot = scaleColorHex(faceplateHex, 0.62f, 0.99f);
        g.fillLinearGradientRect(scr, bgTop, bgBot, 5.5f);
        g.drawRoundedRect(scr.reduced(1.2f), 4.5f, Color::fromHex(0x020406, 0.88f), 2.4f);
        g.drawRoundedRect(scr, 5.5f, Color::fromHex(0x385A62, 0.72f), 1.2f);

        // Subtle studio graticule lines
        for (int i = 1; i <= 4; ++i) {
            const float gx = scr.x + scr.width * (static_cast<float>(i) * 0.20f);
            g.drawLine({gx, scr.y + 4.0f}, {gx, scr.bottom() - 4.0f}, accentCol.withAlpha(0.10f), 0.8f);
        }
        const float midY = scr.getCentre().y;
        g.drawLine({scr.x + 4.0f, midY}, {scr.right() - 4.0f, midY}, accentCol.withAlpha(0.18f), 0.9f);

        // High-density 64-point smooth anti-aliased harmonic transfer curve
        const float driveNorm = (outDb <= -58.0f) ? 0.12f : math::clamp((outDb + 44.0f) / 46.0f, 0.15f, 0.96f);
        const float grPull = math::clamp(grDb / 18.0f, 0.0f, 0.65f);
        std::array<Point<float>, 64> curve1{};
        std::array<Point<float>, 64> curve2{};
        for (size_t i = 0; i < curve1.size(); ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(curve1.size() - 1);
            const float x = scr.x + 6.0f + t * (scr.width - 12.0f);
            const float env = std::sin(t * 3.14159265f);
            const float y1 =
                (std::sin(t * 6.2831853f + outDb * 0.12f) * 0.44f * driveNorm + std::cos(t * 9.42477f) * 0.20f * (1.0f - grPull)) * env;
            const float y2 = (std::cos(t * 6.2831853f - grDb * 0.20f) * 0.30f * driveNorm) * env;
            curve1[i] = {x, midY - y1 * (scr.height * 0.38f)};
            curve2[i] = {x, midY - y2 * (scr.height * 0.32f)};
            if ((i & 1u) == 0u) { g.drawLine({x, midY}, curve1[i], accentCol.withAlpha(0.12f), 2.0f); }
        }
        g.drawPolyline(curve2, Color::fromHex(0x88A0A8, 0.38f), 1.6f);
        g.drawPolyline(curve1, accentCol.withAlpha(0.28f), 3.6f);
        g.drawPolyline(curve1, accentCol, 1.6f);

        const std::string telem =
            (outDb <= -58.0f) ? "SPECTRAL DELTA  //  0.0 dB" : std::format("OUT {:+.1f} dB   \xCE\x94 -{:.1f} dB", outDb, grDb);
        g.drawText(telem, {scr.x + 8.0f, scr.y + 3.0f, scr.width - 16.0f, 13.0f}, accentCol, 7.6f, false, true);
        drawLabelPlaque(g, {bounds.x, bounds.bottom() - 14.0f, bounds.width, 13.0f}, title, UiLabelPlaqueStyle::DirectSilkscreen,
                        g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.0f, accentCol);
    }

    static void drawNeumorphicPillSwitch(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view stateSub,
                                         Color accentCol = Color::fromHex(0xF59E42),
                                         UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = "IN", offText = "BYP";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }
        const bool isCompact = (bounds.height < 68.0f);
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = hasBottomTitle ? ((bounds.height <= 64.0f) ? (bounds.y + 12.0f) : (bounds.y + (bounds.height - 18.0f) * 0.5f))
                                        : (bounds.y + bounds.height * 0.50f);
        const float pillW = std::clamp(bounds.width - 14.0f, 46.0f, 62.0f);
        const float pillH = isCompact ? 20.5f : 23.0f;
        const float pillR = pillH * 0.5f;
        Rect<float> pill = {cx - pillW * 0.5f, cy - pillR, pillW, pillH};
        const float accentLum = 0.2126f * accentCol.r + 0.7152f * accentCol.g + 0.0722f * accentCol.b;
        const bool isPaleAccent = (accentLum > 0.68f);

        // 1. Outer neumorphic cast shadow + top-left bevel highlight + ceramic pill body
        g.fillRoundedRect({pill.x + 1.8f, pill.y + 2.4f, pill.width, pill.height}, pillR, Color::fromHex(0x05090E, 0.55f));
        g.fillRoundedRect({pill.x - 1.0f, pill.y - 1.0f, pill.width, pill.height}, pillR, Color::fromHex(0xFFFFFF, 0.60f));
        g.fillLinearGradientRect(pill, Color::fromHex(0xEEF3F8), Color::fromHex(0xC5D0DC), pillR);
        g.drawRoundedRect(pill, pillR, Color::fromHex(0x283644, 0.88f), 1.2f);

        // 2. Recessed inner groove track so the sliding thumb sits inside a visible channel
        const Rect<float> track = pill.reduced(2.4f);
        const float trackR = std::max(3.0f, track.height * 0.5f);
        g.fillLinearGradientRect(track, Color::fromHex(0xAEBCCB), Color::fromHex(0xDAE3EC), trackR);
        if (isOn && !isPaleAccent) {
            g.fillRoundedRect(track, trackR, accentCol.withAlpha(0.22f));
        }
        g.drawRoundedRect(track, trackR, Color::fromHex(0x526272, 0.72f), 0.9f);

        // Subtle I / O track glyph on the opposite side of the thumb
        if (isOn) {
            g.drawLine({track.x + 8.5f, cy - 3.6f}, {track.x + 8.5f, cy + 3.6f},
                       isPaleAccent ? Color::fromHex(0x283848) : accentCol, 1.8f);
        } else {
            g.drawCircle({track.right() - 9.0f, cy}, 3.2f, Color::fromHex(0x586878), 1.3f);
        }

        // 3. High-contrast 3D Sliding Thumb Disc & Center Notch (never white-on-white)
        const float dotR = pillH * 0.38f;
        const Point<float> dotPt = {isOn ? (pill.right() - pillR) : (pill.x + pillR), cy};
        g.fillCircle({dotPt.x + 1.0f, dotPt.y + 1.4f}, dotR + 0.4f, Color::fromHex(0x081018, 0.62f));

        if (isOn) {
            if (isPaleAccent) {
                // When accentCol is white/ice-pale, use a deep gunmetal-slate thumb with a glowing bright center notch
                g.fillRadialGradientCircle(dotPt, dotR, Color::fromHex(0x334155), Color::fromHex(0x0F172A), {-1.2f, -1.2f});
                g.drawCircle(dotPt, dotR, Color::fromHex(0x090E14), 1.4f);
                g.fillCircle(dotPt, dotR * 0.42f, accentCol);
                g.drawCircle(dotPt, dotR * 0.42f, Color::fromHex(0x090E14, 0.85f), 0.9f);
            } else {
                const Color darkAccent = {accentCol.r * 0.52f, accentCol.g * 0.52f, accentCol.b * 0.52f, 1.0f};
                g.fillRadialGradientCircle(dotPt, dotR, accentCol, darkAccent, {-1.2f, -1.2f});
                g.drawCircle(dotPt, dotR, Color::fromHex(0x141C24, 0.92f), 1.4f);
                g.fillCircle(dotPt, dotR * 0.38f, Color::fromHex(0x101820, 0.90f));
                g.fillCircle(dotPt, dotR * 0.20f, Color::fromHex(0xFFFFFF, 0.92f));
            }
        } else {
            g.fillRadialGradientCircle(dotPt, dotR, Color::fromHex(0x788898), Color::fromHex(0x425060), {-1.2f, -1.2f});
            g.drawCircle(dotPt, dotR, Color::fromHex(0x1E293B, 0.90f), 1.3f);
            g.fillCircle(dotPt, dotR * 0.36f, Color::fromHex(0x1E293B, 0.88f));
            g.fillCircle({dotPt.x - 0.8f, dotPt.y - 0.8f}, dotR * 0.18f, Color::fromHex(0xE2E8F0, 0.85f));
        }
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0x3A4652), 0.94f);
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, cy - (isCompact ? 24.5f : 27.5f), onText, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, cy + (isCompact ? 12.0f : 15.5f), offText, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, 15.5f}, title, plaqueStyle, silkInk,
                            8.6f, accentCol, true);
        }
    }

    static void drawSciFiHologramSwitch(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view stateSub,
                                        Color accentCol = Color::fromHex(0x00F5D4),
                                        UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = "IN", offText = "BYP";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }
        const bool isCompact = (bounds.height < 68.0f);
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = hasBottomTitle ? ((bounds.height <= 64.0f) ? (bounds.y + 12.0f) : (bounds.y + (bounds.height - 18.0f) * 0.5f))
                                        : (bounds.y + bounds.height * 0.50f);
        const float w = std::clamp(bounds.width - 14.0f, 44.0f, 60.0f);
        const float h = isCompact ? 20.5f : 23.0f;
        Rect<float> pad = {cx - w * 0.5f, cy - h * 0.5f, w, h};
        g.fillRoundedRect(pad, 2.5f, Color::fromHex(0x04121A, 0.94f));
        if (isOn) { g.fillRoundedRect(pad.reduced(1.5f), 2.0f, accentCol.withAlpha(0.26f)); }
        g.drawRoundedRect(pad, 2.5f, isOn ? accentCol : accentCol.withAlpha(0.42f), isOn ? 1.5f : 1.0f);
        g.drawLine({pad.x + 3.0f, pad.y}, {pad.x + 10.0f, pad.y}, Color::fromHex(0xFFFFFF, 0.7f), 1.5f);
        g.drawLine({pad.right() - 10.0f, pad.bottom()}, {pad.right() - 3.0f, pad.bottom()}, Color::fromHex(0xFFFFFF, 0.7f), 1.5f);
        // Holographic status core inside pad (never duplicate long state labels
        // inside the pad)
        const Rect<float> core = {isOn ? (pad.right() - pad.width * 0.48f - 2.5f) : (pad.x + 2.5f), pad.y + 3.0f, pad.width * 0.48f,
                                  pad.height - 6.0f};
        g.fillRoundedRect(core, 1.5f, isOn ? accentCol.withAlpha(0.85f) : accentCol.withAlpha(0.22f));
        g.drawRoundedRect(core, 1.5f, isOn ? Color::fromHex(0xE8FFFF, 0.9f) : accentCol.withAlpha(0.45f), 0.9f);
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0xB8F2E6), 0.94f);
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, cy - (isCompact ? 24.5f : 27.5f), onText, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, cy + (isCompact ? 12.0f : 15.5f), offText, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, 15.5f}, title, plaqueStyle, silkInk,
                            8.6f, accentCol, true);
        }
    }

    static void drawNavalBrassToggle(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view stateSub,
                                     Color accentCol = Color::fromHex(0x00E5FF),
                                     UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = "IN", offText = "BYP";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }
        const bool isCompact = (bounds.height < 68.0f);
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = hasBottomTitle ? ((bounds.height <= 64.0f) ? (bounds.y + 12.0f) : (bounds.y + (bounds.height - 18.0f) * 0.5f))
                                        : (bounds.y + bounds.height * 0.50f);
        const float nutR = isCompact ? 9.5f : 10.6f;
        g.fillCircle({cx + 1.5f, cy + 2.2f}, nutR + 1.5f, Color::fromHex(0x040709, 0.85f));
        g.fillRadialGradientCircle({cx, cy}, nutR + 1.0f, Color::fromHex(0xE5C158), Color::fromHex(0x5A4314), {-2.0f, -2.0f});
        g.drawCircle({cx, cy}, nutR + 1.0f, Color::fromHex(0x281C06), 1.2f);
        for (int i = 0; i < 4; ++i) {
            const float a = static_cast<float>(i) * 1.5707963f + 0.785398f;
            g.fillCircle({cx + std::cos(a) * (nutR * 0.78f), cy + std::sin(a) * (nutR * 0.78f)}, 1.4f, Color::fromHex(0x3A280A));
        }
        g.fillCircle({cx, cy}, 4.2f, Color::fromHex(0x0D0A04));
        const float batLen = isCompact ? 9.2f : 10.6f;
        const float tipY = isOn ? (cy - batLen) : (cy + batLen);
        g.drawLine({cx + 1.5f, cy + 1.5f}, {cx + 1.5f, tipY + 1.5f}, Color::fromHex(0x000000, 0.75f), 5.6f);
        g.drawLine({cx, cy}, {cx, tipY}, Color::fromHex(0xD4AF37), 4.0f);
        g.fillRadialGradientCircle({cx, tipY}, 4.2f, Color::fromHex(0xFFF3B0), Color::fromHex(0x7A591B), {-1.0f, -1.0f});
        drawJewelLed(g, {cx - 22.5f, cy}, isOn, accentCol, 3.3f);
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0xEDEAE0), 0.94f);
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, cy - (isCompact ? 24.5f : 27.5f), onText, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, cy + (isCompact ? 12.0f : 15.5f), offText, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, 15.5f}, title, plaqueStyle, silkInk,
                            8.6f, accentCol, true);
        }
    }

    static void drawPunkSafetyPinToggle(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view stateSub,
                                        Color accentCol = Color::fromHex(0xFF1E8E),
                                        UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = "IN", offText = "BYP";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }
        const bool isCompact = (bounds.height < 68.0f);
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = hasBottomTitle ? ((bounds.height <= 64.0f) ? (bounds.y + 12.0f) : (bounds.y + (bounds.height - 18.0f) * 0.5f))
                                        : (bounds.y + bounds.height * 0.50f);
        // Pyramid punk stud base + safety pin lever
        const float studR = isCompact ? 9.2f : 10.4f;
        Rect<float> stud = {cx - studR, cy - studR, studR * 2.0f, studR * 2.0f};
        g.fillRect(stud, Color::fromHex(0x282C32));
        g.drawRect(stud, Color::fromHex(0xD8E0E8), 1.2f);
        g.drawLine({stud.x, stud.y}, {cx, cy}, Color::fromHex(0xFFFFFF, 0.65f), 1.0f);
        g.drawLine({stud.right(), stud.y}, {cx, cy}, Color::fromHex(0x9098A0, 0.65f), 1.0f);
        g.drawLine({stud.x, stud.bottom()}, {cx, cy}, Color::fromHex(0x50565E, 0.65f), 1.0f);
        g.drawLine({stud.right(), stud.bottom()}, {cx, cy}, Color::fromHex(0x141619, 0.85f), 1.0f);
        const float tipY = isOn ? (cy - 10.2f) : (cy + 10.2f);
        g.drawLine({cx - 2.2f, cy}, {cx - 1.3f, tipY}, Color::fromHex(0xE2E8F0), 1.8f);
        g.drawLine({cx + 2.2f, cy}, {cx + 1.3f, tipY}, Color::fromHex(0x94A3B8), 1.8f);
        g.fillCircle({cx, tipY}, 3.8f, isOn ? accentCol : Color::fromHex(0xCBD5E1));
        g.drawCircle({cx, tipY}, 3.8f, Color::fromHex(0x0F172A, 0.90f), 1.1f);
        drawJewelLed(g, {cx - 22.0f, cy}, isOn, accentCol, 3.3f);
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0xF8FAFC), 0.96f);
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, cy - (isCompact ? 24.5f : 27.5f), onText, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, cy + (isCompact ? 12.0f : 15.5f), offText, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, 15.5f}, title, plaqueStyle, silkInk,
                            8.6f, accentCol, true);
        }
    }

    static void drawHellfireEmberSwitch(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view stateSub,
                                        Color accentCol = Color::fromHex(0xFF4500),
                                        UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        std::string_view onText = "IN", offText = "BYP";
        const auto pipePos = stateSub.find('|');
        if (pipePos != std::string_view::npos) {
            onText = stateSub.substr(0, pipePos);
            offText = stateSub.substr(pipePos + 1);
            const auto secondPipe = offText.find('|');
            if (secondPipe != std::string_view::npos) { offText = offText.substr(secondPipe + 1); }
        }
        const bool isCompact = (bounds.height < 68.0f);
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = hasBottomTitle ? ((bounds.height <= 64.0f) ? (bounds.y + 12.0f) : (bounds.y + (bounds.height - 18.0f) * 0.5f))
                                        : (bounds.y + bounds.height * 0.50f);
        const float r = isCompact ? 9.8f : 11.0f;
        if (isOn) { g.fillRadialGradientCircle({cx, cy}, r * 1.65f, Color::fromHex(0xFF5500, 0.45f), Color::fromHex(0xFF0000, 0.0f)); }
        g.fillCircle({cx, cy}, r, Color::fromHex(0x160C08));
        g.drawCircle({cx, cy}, r, isOn ? Color::fromHex(0xFF6A00) : Color::fromHex(0x4A2518), 1.6f);
        const float tipY = isOn ? (cy - 8.6f) : (cy + 8.6f);
        g.drawLine({cx, cy}, {cx, tipY}, isOn ? Color::fromHex(0xFFB700) : Color::fromHex(0x6E473B), 3.8f);
        g.fillCircle({cx, tipY}, 4.0f, isOn ? Color::fromHex(0xFFF099) : Color::fromHex(0x4A2E24));
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0xFFCC99), 0.94f);
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        drawSwitchStatePill(g, cx, cy - (isCompact ? 24.5f : 27.5f), onText, isOn, accentCol, stateFs, maxSpanW);
        drawSwitchStatePill(g, cx, cy + (isCompact ? 12.0f : 15.5f), offText, !isOn, accentCol, stateFs, maxSpanW);
        if (hasBottomTitle) {
            drawLabelPlaque(g, {cx - maxSpanW * 0.5f, cy + (isCompact ? 28.0f : 31.5f), maxSpanW, 15.5f}, title, plaqueStyle, silkInk,
                            8.6f, accentCol, true);
        }
    }

    static void drawCyberLuxeMultiBarMeter(Graphics &g, Rect<float> bounds, float normL, std::string_view title, float normR = -1.0f,
                                           Color accentCol = Color::fromHex(0x00E5FF), bool transparentBezel = false) noexcept {
        if (normR < 0.0f) normR = normL * 0.94f;
        if (!transparentBezel) {
            // Deep recessed carbon-glass bezel with subtle cyan edge accent
            g.fillRoundedRect({bounds.x + 2.0f, bounds.y + 3.0f, bounds.width, bounds.height}, 4.5f, Color::fromHex(0x000000, 0.72f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x0A1017), Color::fromHex(0x04070B), 4.0f);
            g.drawRoundedRect(bounds, 4.0f, Color::fromHex(0x263442), 1.4f);
            g.drawRoundedRect(bounds.reduced(1.2f), 3.2f, accentCol.withAlpha(0.42f), 0.9f);
        }

        Rect<float> inner = bounds.reduced(6.0f);
        // Subtle horizontal dB grid lines + scale numbers down the center/right
        for (int r = 1; r < 10; ++r) {
            const float y = inner.y + 10.0f + (inner.height - 24.0f) * (static_cast<float>(r) / 10.0f);
            g.drawLine({inner.x + 2.0f, y}, {inner.right() - 2.0f, y}, accentCol.withAlpha(0.09f), 0.7f);
        }
        const int cols = 8;
        const int segs = 26;
        const float barAreaH = std::max(24.0f, inner.height - 24.0f);
        const float colW = (inner.width - 10.0f) / static_cast<float>(cols);
        const float segH = std::max(1.8f, barAreaH / static_cast<float>(segs));
        const float barW = std::max(2.5f, colW - 2.6f);

        static constexpr std::array<float, 8> kBandProfile = {0.82f, 0.94f, 1.00f, 0.89f, 0.76f, 0.92f, 0.98f, 0.84f};
        for (int c = 0; c < cols; ++c) {
            const float baseN = (c < 4) ? normL : normR;
            const float cn = math::clamp(baseN * kBandProfile[static_cast<size_t>(c)], 0.0f, 1.0f);
            const float x = inner.x + 5.0f + static_cast<float>(c) * colW;
            int peakSeg = -1;
            for (int s = 0; s < segs; ++s) {
                const float thresh = static_cast<float>(s + 1) / static_cast<float>(segs);
                const float y = inner.bottom() - 13.0f - static_cast<float>(s + 1) * segH;
                const bool lit = (cn >= thresh);
                if (lit) peakSeg = s;
                const Color segCol = (s >= 22) ? Color::fromHex(0xFF9F1C) : ((s >= 17) ? Color::fromHex(0x64F8FF) : accentCol);
                g.fillRect({x, y, barW, std::max(1.2f, segH - 1.0f)},
                           lit ? segCol : Color::fromHex(0x0D1620, transparentBezel ? 0.55f : 1.0f));
            }
            // Glowing peak-hold cap segment
            const int capIdx = std::clamp(peakSeg + 2, 1, segs - 1);
            const float capY = inner.bottom() - 13.0f - static_cast<float>(capIdx + 1) * segH;
            g.fillRect({x, capY, barW, std::max(1.4f, segH - 0.8f)},
                       (capIdx >= 21) ? Color::fromHex(0xFFB703) : Color::fromHex(0xE0FCFF, 0.90f));
        }
        g.drawText("L   M   H   S", {inner.x + 4.0f, inner.y - 1.0f, inner.width - 8.0f, 10.0f}, Color::fromHex(0x7C98AD), 6.8f, true,
                   true);
        g.drawText(title.empty() ? "CYBER TELEMETRY" : title, {inner.x, inner.bottom() - 11.5f, inner.width, 11.5f}, accentCol, 7.2f, true,
                   true);
    }

    static void drawNeumorphicDualArcMeter(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                           Color accentCol = Color::fromHex(0xF59E42), bool transparentBezel = false) noexcept {
        (void)grDb;
        const float accentLuma = 0.299f * accentCol.r + 0.587f * accentCol.g + 0.114f * accentCol.b;
        const Color effAccent = (accentLuma > 0.78f) ? Color::fromHex(0xEA580C) : accentCol;
        Rect<float> well = bounds.reduced(5.0f);
        if (!transparentBezel) {
            // Soft neumorphic drop shadow + crisp top-left ceramic highlight
            g.fillRoundedRect({bounds.x + 3.0f, bounds.y + 3.5f, bounds.width, bounds.height}, 8.0f, Color::fromHex(0x0F172A, 0.48f));
            g.fillRoundedRect({bounds.x - 1.5f, bounds.y - 1.5f, bounds.width, bounds.height}, 8.0f, Color::fromHex(0xFFFFFF, 0.88f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0xEEF3F8), Color::fromHex(0xCFD8E3), 8.0f);
            g.drawRoundedRect(bounds, 8.0f, Color::fromHex(0x64748B, 0.75f), 1.4f);

            g.fillLinearGradientRect(well, Color::fromHex(0xC9D4E0), Color::fromHex(0xE8EEF5), 6.0f);
            g.drawRoundedRect(well, 6.0f, Color::fromHex(0x64748B, 0.65f), 1.1f);
        }

        const float cx = well.getCentre().x;
        const Point<float> pivot = {cx, well.bottom() + well.height * 0.16f};
        const float arcR = well.height * 0.64f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;

        g.drawArc(pivot, arcR, leftAng, 0.24f, Color::fromHex(0x475569, 0.85f), 2.0f);
        g.drawArc(pivot, arcR - 2.0f, 0.24f, rightAng, effAccent.withAlpha(0.38f), 5.0f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, effAccent, 2.4f);

        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            const Color tc = (i >= 4) ? effAccent : Color::fromHex(0x1E293B);
            g.drawLine({pivot.x + (arcR - 2.5f) * s, pivot.y - (arcR - 2.5f) * co},
                       {pivot.x + (arcR + 5.0f) * s, pivot.y - (arcR + 5.0f) * co}, tc, (i >= 4) ? 1.8f : 1.3f);
            g.drawText(kLbl[i], {pivot.x + (arcR + 12.0f) * s - 12.0f, pivot.y - (arcR + 12.0f) * co - 6.0f, 24.0f, 12.0f}, tc, 7.4f, true,
                       true);
        }

        const float norm = (outDb <= -58.0f) ? 0.62f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float na = leftAng + norm * (rightAng - leftAng);
        const Point<float> nBase = {pivot.x + (arcR * 0.24f) * std::sin(na),
                                    std::min(well.bottom() - 2.0f, pivot.y - (arcR * 0.24f) * std::cos(na))};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * std::sin(na), pivot.y - (arcR + 4.0f) * std::cos(na)};
        g.drawLine(nBase, nTip, effAccent.withAlpha(0.38f), 4.6f);
        g.drawLine(nBase, nTip, Color::fromHex(0x0F172A), 2.0f);
        g.fillCircle({cx, well.bottom() - 3.0f}, 4.5f, Color::fromHex(0x334155));

        const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
        g.drawText(title.empty() ? "GAIN REDUCTION dB" : title, {well.x + 8.0f, well.bottom() - 14.0f, well.width * 0.58f, 12.0f},
                   Color::fromHex(0x1E293B), 7.2f, false, true);
        g.drawText(dbStr, {well.right() - 62.0f, well.bottom() - 14.0f, 54.0f, 12.0f}, effAccent, 7.4f, false, true);
    }

    static void drawSubmarinePortholeGauge(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                           Color accentCol = Color::fromHex(0x00E5FF), bool transparentBezel = false) noexcept {
        (void)grDb;
        const bool isWideBay = (bounds.width > bounds.height * 1.35f);
        if (isWideBay && !transparentBezel) {
            // Full 200x104 riveted naval bulkhead instrument housing so the
            // porthole aligns flush with the 200px output column
            g.fillRoundedRect({bounds.x + 2.5f, bounds.y + 3.5f, bounds.width, bounds.height}, 6.0f, Color::fromHex(0x010406, 0.85f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x1C3338), Color::fromHex(0x0B181C), 6.0f);
            g.drawRoundedRect(bounds, 6.0f, Color::fromHex(0x9C7222), 1.8f);
            g.drawRoundedRect(bounds.reduced(3.0f), 4.0f, accentCol.withAlpha(0.32f), 0.9f);
            for (Point<float> pt : std::array<Point<float>, 4>{{{bounds.x + 7.0f, bounds.y + 7.0f},
                                                                {bounds.right() - 7.0f, bounds.y + 7.0f},
                                                                {bounds.x + 7.0f, bounds.bottom() - 7.0f},
                                                                {bounds.right() - 7.0f, bounds.bottom() - 7.0f}}}) {
                drawRackScrew(g, pt, 2.8f, ScrewHeadStyle::GoldPlatedBrass);
            }
        }
        const Point<float> c = bounds.getCentre();
        const float R = std::min(bounds.width, bounds.height) * 0.45f;
        const float faceR = R * 0.77f;
        if (!transparentBezel) {
            g.fillCircle({c.x + 3.0f, c.y + 4.0f}, R + 2.5f, Color::fromHex(0x010406, 0.88f));
            // Heavy riveted naval brass bezel with multi-stop metallic sheen
            g.fillRadialGradientCircle(c, R + 1.5f, Color::fromHex(0xF3D678), Color::fromHex(0x4E360C), {-5.0f, -5.0f});
            g.drawCircle(c, R + 1.5f, Color::fromHex(0x1E1404), 1.8f);
            g.drawCircle(c, R * 0.96f, Color::fromHex(0xFCE89C, 0.55f), 1.0f);
            for (int i = 0; i < 8; ++i) {
                const float a = static_cast<float>(i) * (6.2831853f / 8.0f);
                const Point<float> rv = {c.x + std::cos(a) * (R * 0.88f), c.y + std::sin(a) * (R * 0.88f)};
                g.fillCircle({rv.x + 0.8f, rv.y + 1.0f}, 2.4f, Color::fromHex(0x1E1404, 0.85f));
                g.fillCircle(rv, 2.2f, Color::fromHex(0x8C661E));
                g.fillCircle({rv.x - 0.6f, rv.y - 0.6f}, 1.1f, Color::fromHex(0xFFF0AD));
            }
            // Bioluminescent aqua porthole face with 360-degree pressure gauge
            // ticks
            g.fillRadialGradientCircle(c, faceR, Color::fromHex(0xC2F5F8), Color::fromHex(0x165363), {-3.0f, -4.0f});
            g.drawCircle(c, faceR, Color::fromHex(0x072026), 2.2f);
        }
        g.drawCircle(c, faceR * 0.82f, Color::fromHex(0x0A2E38, 0.65f), 1.1f);
        for (int t = 0; t < 24; ++t) {
            const float a = -2.35f + (static_cast<float>(t) / 23.0f) * 4.70f;
            const float rIn = (t % 3 == 0) ? (faceR * 0.66f) : (faceR * 0.73f);
            g.drawLine({c.x + std::sin(a) * rIn, c.y - std::cos(a) * rIn},
                       {c.x + std::sin(a) * (faceR * 0.82f), c.y - std::cos(a) * (faceR * 0.82f)}, Color::fromHex(0x07252E, 0.85f),
                       (t % 3 == 0) ? 1.5f : 0.9f);
        }
        // Curved glass reflection highlight
        g.drawArc(c, faceR * 0.88f, -0.95f, -0.15f, Color::fromHex(0xFFFFFF, 0.45f), 2.6f);

        const float norm = (outDb <= -58.0f) ? 0.62f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float ang = -2.1f + norm * 4.2f;
        g.drawLine({c.x - std::sin(ang) * (faceR * 0.22f), c.y + std::cos(ang) * (faceR * 0.22f)},
                   {c.x + std::sin(ang) * (faceR * 0.75f), c.y - std::cos(ang) * (faceR * 0.75f)}, Color::fromHex(0x071C22), 2.4f);
        g.fillCircle(c, 4.8f, Color::fromHex(0x9C711E));
        g.fillCircle(c, 2.2f, Color::fromHex(0xF8E28C));
        g.drawText("VU", {c.x - faceR, c.y - faceR * 0.34f, faceR * 2.0f, 11.0f}, Color::fromHex(0x062026), 7.2f, true, true);
        g.drawText(title.empty() ? "OCEAN DEPTH" : title, {c.x - faceR, c.y + faceR * 0.24f, faceR * 2.0f, 11.0f}, Color::fromHex(0x062026),
                   6.8f, true, true);
        (void)accentCol;
        if (isWideBay) {
            g.drawText("DEPTH", {bounds.x + 8.0f, c.y - 14.0f, 40.0f, 11.0f}, Color::fromHex(0xBAE6FD), 6.8f, true, true);
            drawJewelLed(g, {bounds.x + 28.0f, c.y + 4.0f}, true, accentCol, 3.4f);
            const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
            g.drawText("PRESS", {bounds.right() - 48.0f, c.y - 14.0f, 40.0f, 11.0f}, Color::fromHex(0xBAE6FD), 6.8f, true, true);
            g.drawText(dbStr, {bounds.right() - 52.0f, c.y + 0.0f, 46.0f, 12.0f}, accentCol, 7.2f, true, true);
        }
        (void)title;
    }

    static void drawReaperScytheArchMeter(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                          Color accentCol = Color::fromHex(0x38F888), bool transparentBezel = false) noexcept {
        (void)grDb;
        Rect<float> win = bounds.reduced(5.5f);
        if (!transparentBezel) {
            g.fillRoundedRect({bounds.x + 2.5f, bounds.y + 3.0f, bounds.width, bounds.height}, 8.0f, Color::fromHex(0x000000, 0.80f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x28332E), Color::fromHex(0x0E1411), 8.0f);
            g.drawRoundedRect(bounds, 8.0f, Color::fromHex(0x4E6158), 2.0f);

            g.fillLinearGradientRect(win, Color::fromHex(0x1B3B2B), Color::fromHex(0x07100C), 6.0f);
            g.drawRoundedRect(win, 6.0f, accentCol.withAlpha(0.62f), 1.3f);

            const Point<float> roseC = {win.getCentre().x, win.y + 13.0f};
            g.drawCircle(roseC, 8.5f, Color::fromHex(0x4E665A, 0.85f), 1.4f);
            for (int p = 0; p < 6; ++p) {
                const float pa = static_cast<float>(p) * 1.0471975f;
                g.drawCircle({roseC.x + std::cos(pa) * 4.2f, roseC.y + std::sin(pa) * 4.2f}, 3.2f, accentCol.withAlpha(0.45f), 1.0f);
            }
            g.drawArc({win.getCentre().x, win.y + 16.0f}, 12.0f, -1.05f, 1.05f,
                      Color::fromHex(0x4E665A, 0.55f), 1.4f);
        }

        const float cx = win.getCentre().x;
        const Point<float> pivot = {cx, win.bottom() + win.height * 0.16f};
        const float arcR = win.height * 0.68f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;

        if (!transparentBezel) {
            g.fillRadialGradientCircle({cx, win.y + win.height * 0.58f}, win.height * 0.65f, Color::fromHex(0x98F5B8, 0.32f),
                                       Color::fromHex(0x0B1E14, 0.0f));
        }
        g.drawArc(pivot, arcR, leftAng, 0.24f, Color::fromHex(0x6EE7B7, 0.78f), 1.8f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, accentCol, 2.4f);

        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            const Color tc = (i >= 4) ? Color::fromHex(0x38F888) : Color::fromHex(0xFFFFFF);
            g.drawLine({pivot.x + (arcR - 2.5f) * s, pivot.y - (arcR - 2.5f) * co},
                       {pivot.x + (arcR + 5.5f) * s, pivot.y - (arcR + 5.5f) * co}, tc, (i >= 4) ? 1.8f : 1.3f);
            g.drawText(kLbl[i], {pivot.x + (arcR + 13.0f) * s - 14.0f, pivot.y - (arcR + 13.0f) * co - 6.0f, 28.0f, 12.0f}, tc, 8.0f, true,
                       true);
        }

        const float norm = (outDb <= -58.0f) ? 0.58f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float na = leftAng + norm * (rightAng - leftAng);
        const Point<float> nBase = {pivot.x + (arcR * 0.24f) * std::sin(na),
                                    std::min(win.bottom() - 2.0f, pivot.y - (arcR * 0.24f) * std::cos(na))};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * std::sin(na), pivot.y - (arcR + 4.0f) * std::cos(na)};
        g.drawLine(nBase, nTip, accentCol.withAlpha(0.42f), 4.6f);
        g.drawLine(nBase, nTip, Color::fromHex(0xFFFFFF), 2.1f);
        g.drawArc(nTip, 6.5f, na - 1.3f, na + 0.4f, Color::fromHex(0x38F888), 2.0f);
        g.fillCircle({cx, win.bottom() - 3.0f}, 4.2f, Color::fromHex(0x38F888));

        const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
        g.drawText(title.empty() ? "GAIN REDUCTION dB" : title, {win.x + 8.0f, win.bottom() - 15.0f, win.width * 0.62f, 13.0f},
                   Color::fromHex(0xFFFFFF), 8.0f, false, true);
        g.drawText(dbStr, {win.right() - 64.0f, win.bottom() - 15.0f, 56.0f, 13.0f}, Color::fromHex(0x38F888), 8.2f, false, true);
    }

    static void drawHellfireHornedVuMeter(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                          Color accentCol = Color::fromHex(0xFF4500), bool transparentBezel = false) noexcept {
        (void)grDb;
        Rect<float> face = bounds.reduced(6.0f);
        if (!transparentBezel) {
            for (int s = 0; s < 5; ++s) {
                const float t = static_cast<float>(s) / 4.0f;
                const float hr = (7.0f - t * 4.2f);
                g.fillCircle({bounds.x + 12.0f - t * 8.0f, bounds.y + 8.0f - t * 14.0f}, hr, Color::fromHex(0x2B140C));
                g.fillCircle({bounds.right() - 12.0f + t * 8.0f, bounds.y + 8.0f - t * 14.0f}, hr, Color::fromHex(0x2B140C));
                g.drawCircle({bounds.x + 12.0f - t * 8.0f, bounds.y + 8.0f - t * 14.0f}, hr, Color::fromHex(0xFF6B00, 0.65f), 1.0f);
                g.drawCircle({bounds.right() - 12.0f + t * 8.0f, bounds.y + 8.0f - t * 14.0f}, hr, Color::fromHex(0xFF6B00, 0.65f), 1.0f);
            }
            g.fillRoundedRect({bounds.x + 2.5f, bounds.y + 3.5f, bounds.width, bounds.height}, 6.0f, Color::fromHex(0x000000, 0.85f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x3D180A), Color::fromHex(0x160703), 6.0f);
            g.drawRoundedRect(bounds, 6.0f, Color::fromHex(0xFF6B00, 0.85f), 1.8f);
            g.fillLinearGradientRect(face, Color::fromHex(0xFFF099), Color::fromHex(0xC82800), 4.0f);
            for (int f = 0; f < 11; ++f) {
                const float fx = face.x + 12.0f + static_cast<float>(f) * ((face.width - 24.0f) / 10.0f);
                std::array<Point<float>, 3> fl = {{{fx - 9.0f, face.bottom()},
                                                   {fx, face.bottom() - (16.0f + 7.0f * static_cast<float>(f % 2))},
                                                   {fx + 9.0f, face.bottom()}}};
                g.fillPolygon(fl, Color::fromHex(0x3D0800, 0.55f));
            }
            g.drawRoundedRect(face, 4.0f, Color::fromHex(0x240600), 1.8f);
        }

        const float cx = face.getCentre().x;
        const Point<float> pivot = {cx, face.bottom() + face.height * 0.16f};
        const float arcR = face.height * 0.68f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;

        g.drawArc(pivot, arcR, leftAng, 0.24f, Color::fromHex(0x2B0500), 1.8f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, Color::fromHex(0x800000), 2.4f);

        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            g.drawLine({pivot.x + (arcR - 2.5f) * s, pivot.y - (arcR - 2.5f) * co},
                       {pivot.x + (arcR + 5.5f) * s, pivot.y - (arcR + 5.5f) * co}, Color::fromHex(0x2B0500), (i >= 4) ? 1.8f : 1.2f);
            g.drawText(kLbl[i], {pivot.x + (arcR + 13.0f) * s - 12.0f, pivot.y - (arcR + 13.0f) * co - 6.0f, 24.0f, 12.0f},
                       Color::fromHex(0x2B0500), 7.2f, true, true);
        }

        const float norm = (outDb <= -58.0f) ? 0.64f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float na = leftAng + norm * (rightAng - leftAng);
        const Point<float> nBase = {pivot.x + (arcR * 0.24f) * std::sin(na),
                                    std::min(face.bottom() - 2.0f, pivot.y - (arcR * 0.24f) * std::cos(na))};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * std::sin(na), pivot.y - (arcR + 4.0f) * std::cos(na)};
        g.drawLine(nBase, nTip, Color::fromHex(0x1A0000), 2.4f);
        g.fillCircle({cx, face.bottom() - 3.0f}, 4.2f, Color::fromHex(0x2B0500));

        const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
        g.drawText(title.empty() ? "INFERNO VU" : title, {face.x + 8.0f, face.bottom() - 14.0f, face.width * 0.58f, 12.0f},
                   Color::fromHex(0xFFF3B0), 7.0f, false, true);
        g.drawText(dbStr, {face.right() - 62.0f, face.bottom() - 14.0f, 54.0f, 12.0f}, Color::fromHex(0xFFF3B0), 7.4f, false, true);
        (void)accentCol;
    }

    static void drawApocalypseGeigerVuMeter(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                            Color accentCol = Color::fromHex(0xF5B800), bool transparentBezel = false) noexcept {
        (void)grDb;
        Rect<float> face = bounds.reduced(6.5f);
        if (!transparentBezel) {
            g.fillRoundedRect({bounds.x + 2.5f, bounds.y + 3.0f, bounds.width, bounds.height}, 4.5f, Color::fromHex(0x000000, 0.80f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x4A3D31), Color::fromHex(0x231C16), 4.0f);
            g.drawRoundedRect(bounds, 4.0f, Color::fromHex(0x6E5640), 1.8f);
            for (int cxIdx = 0; cxIdx < 2; ++cxIdx) {
                for (int cyIdx = 0; cyIdx < 2; ++cyIdx) {
                    const float bx = (cxIdx == 0) ? (bounds.x + 5.0f) : (bounds.right() - 5.0f);
                    const float by = (cyIdx == 0) ? (bounds.y + 5.0f) : (bounds.bottom() - 5.0f);
                    g.fillCircle({bx, by}, 2.0f, Color::fromHex(0x18130F));
                }
            }
            g.fillLinearGradientRect(face, Color::fromHex(0xF8D838), Color::fromHex(0xB88200), 3.0f);
            g.drawRoundedRect(face, 3.0f, Color::fromHex(0x1E1812), 1.6f);
        }

        const float cx = face.getCentre().x;
        const Point<float> pivot = {cx, face.bottom() + face.height * 0.16f};
        const float arcR = face.height * 0.68f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;

        const Point<float> rc = {cx, face.y + face.height * 0.56f};
        for (int i = 0; i < 3; ++i) {
            const float a0 = static_cast<float>(i) * 2.094395f - 0.38f;
            g.drawArc(rc, 8.5f, a0, a0 + 0.76f, Color::fromHex(0x1A1612, 0.75f), 5.0f);
        }
        g.fillCircle(rc, 2.5f, Color::fromHex(0x1A1612));

        g.drawArc(pivot, arcR, leftAng, 0.24f, Color::fromHex(0x1A1612), 1.8f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, Color::fromHex(0xB91C1C), 2.4f);
        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            const Color tc = (i >= 4) ? Color::fromHex(0x991B1B) : Color::fromHex(0x1A1612);
            g.drawLine({pivot.x + (arcR - 2.5f) * s, pivot.y - (arcR - 2.5f) * co},
                       {pivot.x + (arcR + 5.5f) * s, pivot.y - (arcR + 5.5f) * co}, tc, (i >= 4) ? 1.8f : 1.2f);
            g.drawText(kLbl[i], {pivot.x + (arcR + 13.0f) * s - 12.0f, pivot.y - (arcR + 13.0f) * co - 6.0f, 24.0f, 12.0f}, tc, 7.2f, true,
                       true);
        }
        g.drawLine({face.right() - 22.0f, face.y + 2.0f}, {face.right() - 10.0f, face.y + 16.0f}, Color::fromHex(0xFFFFFF, 0.55f), 1.0f);
        g.drawLine({face.right() - 10.0f, face.y + 16.0f}, {face.right() - 2.0f, face.y + 12.0f}, Color::fromHex(0xFFFFFF, 0.45f), 0.9f);

        const float norm = (outDb <= -58.0f) ? 0.60f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float na = leftAng + norm * (rightAng - leftAng);
        const Point<float> nBase = {pivot.x + (arcR * 0.24f) * std::sin(na),
                                    std::min(face.bottom() - 2.0f, pivot.y - (arcR * 0.24f) * std::cos(na))};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * std::sin(na), pivot.y - (arcR + 4.0f) * std::cos(na)};
        g.drawLine(nBase, nTip, Color::fromHex(0xC81D13), 2.4f);
        g.fillCircle({cx, face.bottom() - 3.0f}, 4.2f, Color::fromHex(0x1A1612));

        const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
        g.drawText(title.empty() ? "RAD / GEIGER VU" : title, {face.x + 6.0f, face.bottom() - 13.5f, face.width * 0.58f, 12.0f},
                   Color::fromHex(0x1A1612), 7.0f, false, true);
        g.drawText(dbStr, {face.right() - 58.0f, face.bottom() - 13.5f, 52.0f, 12.0f}, Color::fromHex(0x991B1B), 7.2f, false, true);
        (void)accentCol;
    }

    static void drawPunkAnarchyVuMeter(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                       Color accentCol = Color::fromHex(0xFF1E8E), bool transparentBezel = false) noexcept {
        (void)grDb;
        Rect<float> face = bounds.reduced(5.5f);
        if (!transparentBezel) {
            for (int i = 0; i < 7; ++i) {
                const float sx = bounds.x + 10.0f + static_cast<float>(i) * ((bounds.width - 20.0f) / 6.0f);
                std::array<Point<float>, 3> spL = {{{sx - 4.5f, bounds.y + 2.0f}, {sx, bounds.y - 9.5f}, {sx, bounds.y + 2.0f}}};
                std::array<Point<float>, 3> spR = {{{sx, bounds.y + 2.0f}, {sx, bounds.y - 9.5f}, {sx + 4.5f, bounds.y + 2.0f}}};
                g.fillPolygon(spL, Color::fromHex(0xF1F5F9));
                g.fillPolygon(spR, Color::fromHex(0x64748B));
            }
            g.fillRoundedRect(bounds, 3.5f, Color::fromHex(0x181A1E));
            g.drawRoundedRect(bounds, 3.5f, Color::fromHex(0x94A3B8), 1.8f);
            g.fillLinearGradientRect(face, Color::fromHex(0xF8FAFC), Color::fromHex(0xCBD5E1), 2.5f);
            g.drawRoundedRect(face, 2.5f, Color::fromHex(0x0F172A), 1.4f);
        }

        const float cx = face.getCentre().x;
        const Point<float> pivot = {cx, face.bottom() + face.height * 0.16f};
        const float arcR = face.height * 0.68f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;

        const Point<float> ac = {cx, face.y + face.height * 0.56f};
        g.drawCircle(ac, 10.0f, Color::fromHex(0xE11D48, 0.72f), 2.0f);
        g.drawLine({ac.x, ac.y - 12.0f}, {ac.x - 8.5f, ac.y + 10.0f}, Color::fromHex(0xE11D48, 0.85f), 2.2f);
        g.drawLine({ac.x, ac.y - 12.0f}, {ac.x + 8.5f, ac.y + 10.0f}, Color::fromHex(0xE11D48, 0.85f), 2.2f);
        g.drawLine({ac.x - 11.5f, ac.y + 2.0f}, {ac.x + 11.5f, ac.y - 1.5f}, Color::fromHex(0xE11D48, 0.85f), 2.2f);

        g.drawArc(pivot, arcR, leftAng, 0.24f, Color::fromHex(0x0F172A), 1.8f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, Color::fromHex(0xE11D48), 2.4f);
        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            const Color tc = (i >= 4) ? Color::fromHex(0xE11D48) : Color::fromHex(0x0F172A);
            g.drawLine({pivot.x + (arcR - 2.5f) * s, pivot.y - (arcR - 2.5f) * co},
                       {pivot.x + (arcR + 5.5f) * s, pivot.y - (arcR + 5.5f) * co}, tc, (i >= 4) ? 1.8f : 1.2f);
            g.drawText(kLbl[i], {pivot.x + (arcR + 13.0f) * s - 12.0f, pivot.y - (arcR + 13.0f) * co - 6.0f, 24.0f, 12.0f}, tc, 7.2f, true,
                       true);
        }

        const float norm = (outDb <= -58.0f) ? 0.62f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float na = leftAng + norm * (rightAng - leftAng);
        const Point<float> nBase = {pivot.x + (arcR * 0.24f) * std::sin(na),
                                    std::min(face.bottom() - 2.0f, pivot.y - (arcR * 0.24f) * std::cos(na))};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * std::sin(na), pivot.y - (arcR + 4.0f) * std::cos(na)};
        g.drawLine(nBase, nTip, accentCol, 2.4f);
        g.fillCircle({cx, face.bottom() - 3.0f}, 4.0f, Color::fromHex(0x0F172A));

        const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
        g.drawText(title.empty() ? "ANARCHY VU" : title, {face.x + 6.0f, face.bottom() - 13.5f, face.width * 0.58f, 12.0f},
                   Color::fromHex(0x0F172A), 7.0f, false, true);
        g.drawText(dbStr, {face.right() - 58.0f, face.bottom() - 13.5f, 52.0f, 12.0f}, Color::fromHex(0xE11D48), 7.2f, false, true);
    }

    static void drawNatureElementalVuMeter(Graphics &g, Rect<float> bounds, float grDb, float outDb, std::string_view title,
                                           Color accentCol = Color::fromHex(0x4ADE80), bool transparentBezel = false) noexcept {
        (void)grDb;
        const float accentLuma = 0.299f * accentCol.r + 0.587f * accentCol.g + 0.114f * accentCol.b;
        const bool isIceOrStorm = (accentCol.b > accentCol.r && accentCol.b > 0.65f);
        const bool isSkyGolden = (accentCol.r > 0.85f && accentCol.g > 0.55f && accentCol.b < 0.45f) || (accentLuma > 0.82f);
        const Color effAccent = (accentLuma > 0.82f) ? Color::fromHex(0xD97706) : (isSkyGolden ? Color::fromHex(0xD97706) : accentCol);

        Color frameBg = isIceOrStorm ? Color::fromHex(0x162C3D) : (isSkyGolden ? Color::fromHex(0xCBD5E1) : Color::fromHex(0x3E2723));
        Color frameBorder = isIceOrStorm ? Color::fromHex(0x7DD3FC) : (isSkyGolden ? Color::fromHex(0x64748B) : Color::fromHex(0x8D6E63));
        Color faceTop = isIceOrStorm ? Color::fromHex(0x0F2537) : (isSkyGolden ? Color::fromHex(0xFFFBEB) : Color::fromHex(0xF5E6C8));
        Color faceBot = isIceOrStorm ? Color::fromHex(0x1E4E6E) : (isSkyGolden ? Color::fromHex(0xFDE68A) : Color::fromHex(0xD7B889));
        Color scaleCol = isIceOrStorm ? Color::fromHex(0xBAE6FD) : (isSkyGolden ? Color::fromHex(0x78350F) : Color::fromHex(0x3E2723));

        Rect<float> face = bounds.reduced(5.0f);
        if (!transparentBezel) {
            g.fillRoundedRect({bounds.x + 2.0f, bounds.y + 3.0f, bounds.width, bounds.height}, 6.0f, Color::fromHex(0x000000, 0.55f));
            g.fillRoundedRect(bounds, 6.0f, frameBg);
            g.drawRoundedRect(bounds, 6.0f, frameBorder, 1.8f);

            g.fillLinearGradientRect(face, faceTop, faceBot, 4.0f);
            g.drawRoundedRect(face, 4.0f, scaleCol.withAlpha(0.45f), 1.1f);
        }

        const float cx = face.getCentre().x;
        const Point<float> pivot = {cx, face.bottom() + face.height * 0.16f};
        const float arcR = face.height * 0.64f;
        constexpr float leftAng = -0.58f;
        constexpr float rightAng = +0.58f;

        if (isIceOrStorm && !transparentBezel) {
            g.fillRadialGradientCircle({cx, face.y + face.height * 0.56f}, face.height * 0.65f, Color::fromHex(0x38BDF8, 0.32f),
                                       Color::fromHex(0x38BDF8, 0.0f));
        }
        g.drawArc(pivot, arcR, leftAng, 0.24f, scaleCol, 1.8f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, effAccent, 2.4f);
        constexpr std::array<float, 6> kPos = {{0.0f, 0.24f, 0.44f, 0.62f, 0.80f, 1.0f}};
        constexpr std::array<const char *, 6> kLbl = {{"-20", "-10", "-6", "-3", "0", "+3"}};
        for (size_t i = 0; i < kPos.size(); ++i) {
            const float a = leftAng + kPos[i] * (rightAng - leftAng);
            const float s = std::sin(a), co = std::cos(a);
            const Color tc = (i >= 4) ? effAccent : scaleCol;
            g.drawLine({pivot.x + (arcR - 2.5f) * s, pivot.y - (arcR - 2.5f) * co},
                       {pivot.x + (arcR + 5.0f) * s, pivot.y - (arcR + 5.0f) * co}, tc, (i >= 4) ? 1.8f : 1.3f);
            g.drawText(kLbl[i], {pivot.x + (arcR + 12.0f) * s - 12.0f, pivot.y - (arcR + 12.0f) * co - 6.0f, 24.0f, 12.0f}, tc, 7.4f, true,
                       true);
        }

        const float norm = (outDb <= -58.0f) ? 0.60f : math::clamp((outDb + 20.0f) / 23.0f, 0.0f, 1.0f);
        const float na = leftAng + norm * (rightAng - leftAng);
        const Point<float> nBase = {pivot.x + (arcR * 0.24f) * std::sin(na),
                                    std::min(face.bottom() - 2.0f, pivot.y - (arcR * 0.24f) * std::cos(na))};
        const Point<float> nTip = {pivot.x + (arcR + 4.0f) * std::sin(na), pivot.y - (arcR + 4.0f) * std::cos(na)};
        g.drawLine(nBase, nTip, effAccent, 2.4f);
        g.fillCircle({cx, face.bottom() - 3.0f}, 4.2f, scaleCol);

        const std::string dbStr = (outDb <= -58.0f) ? "-0.0 dB" : std::format("{:+.1f} dB", outDb);
        g.drawText(title.empty() ? "ELEMENTAL VU" : title, {face.x + 6.0f, face.bottom() - 13.5f, face.width * 0.58f, 12.0f}, scaleCol,
                   7.2f, false, true);
        g.drawText(dbStr, {face.right() - 58.0f, face.bottom() - 13.5f, 52.0f, 12.0f}, effAccent, 7.2f, false, true);
    }

    /**
     * @brief Renders 20 preset road-worn vintage stickers, studio
     * masking/gaff/Dymo tapes, and user-uploaded custom image
     * decals (`customImagePath`) with editable text & peeling
     * edges.
     */
    static void drawVintageSticker(Graphics &g, Rect<float> bounds, StickerPreset preset, std::string_view title, std::string_view subtitle,
                                   Color accentCol = Color::fromHex(0xFF9F1C), std::string_view customImagePath = "",
                                   float fontSizePt = 10.5f, float rotationDeg = 0.0f, uint32_t cornerSeed = 0u) {
        struct ScopedRotation {
            Graphics &gr;
            bool active;
            ScopedRotation(Graphics &gRef, float deg, Point<float> c) noexcept : gr(gRef), active(std::abs(deg) > 0.01f) {
                if (active) gr.pushRotation(deg, c);
            }
            ~ScopedRotation() noexcept {
                if (active) gr.popRotation();
            }
        } rotGuard(g, rotationDeg, bounds.getCentre());

        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = bounds.y + bounds.height * 0.5f;
        const float minDim = (std::min)(bounds.width, bounds.height);
        const Color agedPaper = Color::fromHex(0xD8CBB0);
        const Color peelShadow = Color::fromHex(0x040709, 0.72f);

        // Deterministic per-sticker seed (never hashes x/y so corners
        // & wear stay rock-solid while dragging)
        const uint32_t effectiveSeed =
            (cornerSeed != 0u)
                ? cornerSeed
                : (static_cast<uint32_t>(preset) * 2246822519u ^ static_cast<uint32_t>(title.size() * 374761393u) ^ 0x9E3779B9u);
        const uint32_t wearSeed = effectiveSeed ^ (static_cast<uint32_t>(preset) * 2654435761u);
        auto hashFloat = [&](uint32_t idx) noexcept -> float {
            uint32_t h = wearSeed ^ (idx * 0x9E3779B9u);
            h ^= h >> 16;
            h *= 0x85EBCA6Bu;
            h ^= h >> 13;
            return static_cast<float>(h & 0xFFFFu) / 65535.0f;
        };

        // 4-Corner 3D Dog-Ear Peeling Corner helper for all
        // rectangular, square, and banner/tape stickers
        auto drawDogEarCornerPeel = [&](Rect<float> r) {
            if (r.width < 20.0f || r.height < 16.0f) return;
            const int dogEarCorner = static_cast<int>(effectiveSeed % 4u);
            const float earSize = std::clamp(std::min(r.width, r.height) * 0.22f, 11.0f, 18.0f);

            float cCornerX = r.x;
            float cCornerY = r.y;
            float sx = 1.0f;
            float sy = 1.0f;
            if (dogEarCorner == 1) {
                cCornerX = r.right();
                cCornerY = r.y;
                sx = -1.0f;
                sy = 1.0f;
            } else if (dogEarCorner == 2) {
                cCornerX = r.right();
                cCornerY = r.bottom();
                sx = -1.0f;
                sy = -1.0f;
            } else if (dogEarCorner == 3) {
                cCornerX = r.x;
                cCornerY = r.bottom();
                sx = 1.0f;
                sy = -1.0f;
            }

            const Point<float> pCorner = {cCornerX, cCornerY};
            const Point<float> pEdgeX = {cCornerX + sx * earSize, cCornerY};
            const Point<float> pEdgeY = {cCornerX, cCornerY + sy * earSize};
            const Point<float> pFlapTip = {cCornerX + sx * (earSize * 0.92f), cCornerY + sy * (earSize * 0.92f)};

            // 1. Outer Corner Cutout (covers sticker corner & outer
            // border stroke)
            const Point<float> pCornerPad = {cCornerX - sx * 2.0f, cCornerY - sy * 2.0f};
            const Point<float> pEdgeXPad = {pEdgeX.x, cCornerY - sy * 2.0f};
            const Point<float> pEdgeYPad = {cCornerX - sx * 2.0f, pEdgeY.y};
            const Color cutoutCol(0.04f, 0.06f, 0.08f, 0.92f);
            g.fillPolygon(std::array<Point<float>, 3>{{pCornerPad, pEdgeXPad, pEdgeYPad}}, cutoutCol);
            g.fillPolygon(std::array<Point<float>, 3>{{pCorner, pEdgeX, pEdgeY}}, cutoutCol);

            // 2. Drop Shadow Under the Flap
            const std::array<Point<float>, 3> shadowTri = {{{pFlapTip.x + sx * 2.5f, pFlapTip.y + sy * 2.5f}, pEdgeX, pEdgeY}};
            g.fillPolygon(shadowTri, Color(0.0f, 0.0f, 0.0f, 0.48f));

            // 3. Aged Paper-Backing Flap Triangle
            const std::array<Point<float>, 3> flapTri = {{pFlapTip, pEdgeX, pEdgeY}};
            g.fillPolygon(flapTri, Color::fromHex(0xD8CEBE));
            g.drawLine(pEdgeX, pFlapTip, Color(0.15f, 0.13f, 0.10f, 0.65f), 1.0f);
            g.drawLine(pEdgeY, pFlapTip, Color(0.15f, 0.13f, 0.10f, 0.65f), 1.0f);
            g.drawLine(pEdgeX, pEdgeY, Color(1.0f, 0.98f, 0.92f, 0.85f), 1.3f);
        };

        // 1. SHAPE_PAPER_TAPE_RECT helper: edge nicks + paper
        // stain/creases underneath text
        auto drawPaperTapePeelAndWear = [&](Rect<float> r) {
            if (r.width < 16.0f || r.height < 14.0f) return;
            const float rx = r.x;
            const float ry = r.y;
            const float rw = r.width;
            const float rh = r.height;

            for (uint32_t n = 0; n < 4; ++n) {
                const float t = 0.20f + 0.60f * hashFloat(110u + n);
                const float nickSz = std::clamp(minDim * 0.038f, 1.4f, 2.8f);
                if (n % 2 == 0) {
                    const float ny = (n == 0) ? ry : (r.bottom() - nickSz * 0.6f);
                    g.fillCircle({rx + rw * t, ny + nickSz * 0.3f}, nickSz, Color::fromHex(0xDFD2B4, 0.32f));
                } else {
                    const float nx = (n == 1) ? rx : (r.right() - nickSz * 0.6f);
                    g.fillCircle({nx + nickSz * 0.3f, ry + rh * t}, nickSz, Color::fromHex(0xDFD2B4, 0.32f));
                }
            }

            // Coffee-ring stain & subtle paper fibers/creases (~20%
            // opacity underneath text)
            const Point<float> stainC{rx + rw * (0.22f + 0.55f * hashFloat(5)), ry + rh * (0.25f + 0.50f * hashFloat(6))};
            const float stainR = std::clamp(minDim * 0.24f, 6.0f, 16.0f);
            g.drawArc(stainC, stainR, 0.4f + hashFloat(7) * 1.5f, 2.9f + hashFloat(7) * 1.5f, Color::fromHex(0x523414, 0.18f), 1.2f);
            g.drawArc(stainC, stainR * 0.92f, 0.6f + hashFloat(7) * 1.5f, 2.4f + hashFloat(7) * 1.5f, Color::fromHex(0x3A220A, 0.12f),
                      0.8f);

            const Point<float> c1A{rx + rw * (0.08f + 0.12f * hashFloat(10)), ry + rh * (0.32f + 0.18f * hashFloat(11))};
            const Point<float> c1B{rx + rw * (0.68f + 0.20f * hashFloat(12)), ry + rh * (0.08f + 0.16f * hashFloat(13))};
            g.drawLine(c1A, c1B, Color::fromHex(0x16110B, 0.18f), 0.9f);
            g.drawLine({c1A.x, c1A.y + 1.0f}, {c1B.x, c1B.y + 1.0f}, Color::fromHex(0xF6ECD6, 0.20f), 0.9f);
        };

        // 2. SHAPE_ROUND_VINYL_DECAL helpers: Clean circular die-cut
        // warm aged parchment border (3px thick) + top-hemisphere
        // vinyl gloss arc + circular drop shadow (NO corner peel!)
        auto drawRoundVinylDecalBase = [&](Point<float> c, float r, Color innerFill) {
            const float rimW = 3.0f;
            g.fillCircle({c.x + 2.2f, c.y + 3.2f}, r + rimW, peelShadow);
            g.fillCircle(c, r + rimW, Color::fromHex(0xD8CBB0));
            g.drawCircle(c, r + rimW, Color::fromHex(0x383228, 0.72f), 1.0f);
            g.fillCircle(c, r, innerFill);
        };
        auto drawRoundVinylGloss = [&](Point<float> c, float r) {
            g.drawArc(c, r * 0.84f, -2.55f, -0.59f, Color::fromHex(0xFFFFFF, 0.24f), std::max(1.5f, r * 0.06f));
            g.drawArc(c, r + 1.5f, -2.65f, -0.49f, Color::fromHex(0xFFFFFF, 0.35f), 1.0f);
        };

        // 3. SHAPE_RIGID_PASS_OR_PLAQUE helper: Clean rounded corners
        // with crisp 3D bevel edge and drop shadow (NO paper corner
        // peel!)
        auto drawRigidPlaqueBase = [&](Rect<float> card, float cornerR, Color fillCol, Color borderCol) {
            g.fillRoundedRect({card.x + 2.2f, card.y + 3.0f, card.width, card.height}, cornerR, peelShadow);
            g.fillRoundedRect(card, cornerR, fillCol);
            g.drawLine({card.x + cornerR, card.y + 1.0f}, {card.right() - cornerR, card.y + 1.0f}, Color::fromHex(0xFFFFFF, 0.38f), 1.2f);
            g.drawLine({card.x + 1.0f, card.y + cornerR}, {card.x + 1.0f, card.bottom() - cornerR}, Color::fromHex(0xFFFFFF, 0.24f), 1.1f);
            g.drawLine({card.x + cornerR, card.bottom() - 1.0f}, {card.right() - cornerR, card.bottom() - 1.0f},
                       Color::fromHex(0x050709, 0.55f), 1.2f);
            g.drawRoundedRect(card, cornerR, borderCol, 1.3f);
        };

        // Helper: draw bottom vintage sticker caption banner with
        // dynamic DirectWrite text-width auto-sizing
        auto drawStickerCaptionBanner = [&](float bannerY, float bannerH, Color bgCol, Color fgCol, Color borderCol) {
            if (title.empty() && subtitle.empty()) return;
            const bool twoLine = (!title.empty() && !subtitle.empty());
            const float fsMain = std::clamp(fontSizePt * 0.85f, 7.2f, twoLine ? 11.2f : 12.2f);
            const float fsSub = std::clamp(fontSizePt * 0.68f, 6.2f, 9.2f);
            const std::string combined =
                subtitle.empty() ? std::string(title) : (title.empty() ? std::string(subtitle) : std::format("{} · {}", title, subtitle));

            float reqTextW = 0.0f;
            float estFitW = 0.0f;
            const float fScale = g.getFontScale();
            if (twoLine) {
                reqTextW = std::max(g.measureTextWidth(title, fsMain, true), g.measureTextWidth(subtitle, fsSub, true));
                estFitW = std::max(static_cast<float>(title.size()) * (fsMain * fScale * 0.65f),
                                   static_cast<float>(subtitle.size()) * (fsSub * fScale * 0.65f));
            } else {
                reqTextW = g.measureTextWidth(combined, fsMain, true);
                estFitW = static_cast<float>(combined.size()) * (fsMain * fScale * 0.65f);
            }
            const float pillW = std::max({bounds.width - 6.0f, reqTextW + 16.0f, estFitW + 12.0f});
            const Rect<float> bRect = {cx - pillW * 0.5f, bannerY, pillW, bannerH};

            g.fillRoundedRect({bRect.x + 1.4f, bRect.y + 2.0f, bRect.width, bRect.height}, 3.5f, peelShadow);
            g.fillRoundedRect(bRect, 3.5f, bgCol);
            g.drawRoundedRect(bRect, 3.5f, borderCol, 1.3f);

            if (twoLine) {
                g.drawText(title, {bRect.x + 4.0f, bRect.y + 0.5f, bRect.width - 8.0f, bannerH * 0.55f}, fgCol, fsMain, true, true);
                g.drawText(subtitle, {bRect.x + 4.0f, bRect.y + bannerH * 0.50f, bRect.width - 8.0f, bannerH * 0.48f},
                           fgCol.withAlpha(0.92f), fsSub, true, true);
            } else {
                g.drawText(combined, {bRect.x + 4.0f, bRect.y, bRect.width - 8.0f, bRect.height}, fgCol, fsMain, true, true);
            }
        };

        // Helper for computing circular vinyl decal center & radius
        // above optional bottom caption pill
        auto computeRoundDecalGeom = [&](float &outBannerH, Point<float> &outCenter, float &outRadius) {
            const bool hasText = (!title.empty() || !subtitle.empty());
            outBannerH = hasText ? std::clamp(bounds.height * ((!title.empty() && !subtitle.empty()) ? 0.27f : 0.23f), 19.0f, 30.0f) : 0.0f;
            const float emblemH = std::max(24.0f, bounds.height - outBannerH - 2.0f);
            outCenter = {cx, bounds.y + emblemH * 0.50f};
            outRadius = std::max(12.0f, std::min(bounds.width * 0.42f, emblemH * 0.44f));
            return hasText;
        };

        // Unified weathered studio patina helper invoked on every
        // sticker preset
        auto drawStickerSurfaceWeathering = [&](Rect<float> r, bool isRound, Point<float> c, float rad) {
            if (r.width < 14.0f || r.height < 12.0f) return;

            if (isRound) {
                // Edge Rubbing / Grime Vignette on round vinyl decals
                g.drawCircle(c, std::max(4.0f, rad - 1.0f), Color::fromHex(0x2A2116, 0.38f), 2.2f);
                for (uint32_t k = 0; k < 4; ++k) {
                    const float ang = hashFloat(200u + k * 7u) * 6.2831853f;
                    const float span = 0.14f + hashFloat(201u + k * 7u) * 0.16f;
                    g.drawArc(c, rad + 2.2f, ang, ang + span, Color::fromHex(0x8C7D62, 0.55f), 2.2f);
                }
            } else {
                // Edge Rubbing / Grime Vignette & darkened corner smudges
                // on rectangular plaques/badges/tapes
                const Rect<float> insetR = {r.x + 2.5f, r.y + 2.5f, std::max(4.0f, r.width - 5.0f), std::max(4.0f, r.height - 5.0f)};
                g.drawRoundedRect(insetR, 4.0f, Color::fromHex(0x2A2116, 0.36f), 1.8f);
                const float smudgeR = std::clamp(std::min(r.width, r.height) * 0.20f, 4.0f, 14.0f);
                const Color smudgeIn = Color::fromHex(0x1E160E, 0.28f);
                const Color smudgeOut = Color::fromHex(0x1E160E, 0.0f);
                g.fillRadialGradientCircle({insetR.x + 3.0f, insetR.y + 3.0f}, smudgeR, smudgeIn, smudgeOut);
                g.fillRadialGradientCircle({insetR.right() - 3.0f, insetR.y + 3.0f}, smudgeR, smudgeIn, smudgeOut);
                g.fillRadialGradientCircle({insetR.x + 3.0f, insetR.bottom() - 3.0f}, smudgeR, smudgeIn, smudgeOut);
                g.fillRadialGradientCircle({insetR.right() - 3.0f, insetR.bottom() - 3.0f}, smudgeR, smudgeIn, smudgeOut);
            }

            // Surface Scuffs, Micro-Scratches & Tea/Oil Spotting across
            // face and caption banner
            const float faceSpanX = isRound ? (rad * 1.35f) : (r.width * 0.72f);
            const float faceSpanY = isRound ? (rad * 1.35f) : (r.height * 0.72f);
            const Point<float> faceCenter = isRound ? c : r.getCentre();

            for (uint32_t s = 0; s < 5; ++s) {
                const float sx0 = faceCenter.x + (hashFloat(220u + s * 11u) - 0.5f) * faceSpanX;
                const float sy0 = faceCenter.y + (hashFloat(221u + s * 11u) - 0.5f) * faceSpanY;
                const float ang = (hashFloat(222u + s * 11u) - 0.5f) * 2.4f;
                const float len = 5.0f + hashFloat(223u + s * 11u) * (std::min(faceSpanX, faceSpanY) * 0.38f);
                const Point<float> p1{sx0, sy0};
                const Point<float> p2{sx0 + std::cos(ang) * len, sy0 + std::sin(ang) * len};
                g.drawLine(p1, p2, (s & 1u) ? Color::fromHex(0xD8CCB4, 0.22f) : Color::fromHex(0x18120C, 0.24f), 0.9f);
            }

            // Subtle partial coffee/oil ring arc
            const Point<float> ringC{faceCenter.x + (hashFloat(280u) - 0.5f) * (faceSpanX * 0.36f),
                                     faceCenter.y + (hashFloat(281u) - 0.5f) * (faceSpanY * 0.36f)};
            const float ringR = std::clamp(std::min(faceSpanX, faceSpanY) * (0.22f + 0.14f * hashFloat(282u)), 4.5f, 18.0f);
            const float ringA0 = hashFloat(283u) * 6.2831853f;
            const float ringLen = 1.4f + hashFloat(284u) * 1.8f;
            g.drawArc(ringC, ringR, ringA0, ringA0 + ringLen, Color::fromHex(0x4A3219, 0.16f), 1.4f);

            // 6 deterministic micro-specks across the sticker face and
            // caption banner
            for (uint32_t p = 0; p < 6; ++p) {
                const float px = r.x + r.width * (0.14f + 0.72f * hashFloat(300u + p * 5u));
                const float py = r.y + r.height * (0.14f + 0.76f * hashFloat(301u + p * 5u));
                if (isRound && py < c.y + rad && std::hypot(px - c.x, py - c.y) > rad + 1.5f) continue;
                const float pr = 0.7f + 0.8f * hashFloat(302u + p * 5u);
                g.fillCircle({px, py}, pr, (p & 1u) ? Color::fromHex(0x241A10, 0.30f) : Color::fromHex(0x8C7853, 0.28f));
            }
        };

        // If user uploaded a custom image file, render inside a clean
        // die-cut plaque (SHAPE_RIGID_PASS_OR_PLAQUE)
        if (!customImagePath.empty()) {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            drawRigidPlaqueBase(card, 6.0f, agedPaper, Color::fromHex(0x584E3C));

            const bool hasText = (!title.empty() || !subtitle.empty());
            const float textZoneH = hasText ? std::clamp(card.height * 0.26f, 16.0f, 28.0f) : 0.0f;
            const Rect<float> imgBox = {card.x + 4.0f, card.y + 4.0f, card.width - 8.0f, std::max(12.0f, card.height - 8.0f - textZoneH)};
            g.fillRoundedRect(imgBox, 4.0f, Color::fromHex(0x16181C));
            if (!g.drawImageFromFile(customImagePath, imgBox, 0.96f)) {
                g.drawText("IMAGE NOT FOUND", imgBox, Color::fromHex(0xFF6B2B), 8.0f, true, true);
            }
            if (hasText) {
                drawStickerCaptionBanner(card.bottom() - textZoneH - 2.0f, textZoneH, Color::fromHex(0x181A1E, 0.94f), agedPaper,
                                         accentCol);
            }
            drawStickerSurfaceWeathering(bounds, false, bounds.getCentre(), minDim * 0.42f);
            drawDogEarCornerPeel(card);
            return;
        }

        switch (preset) {
        // ================================================================
        // 0. GRUNGE SMILEY FACE (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::GrungeSmiley: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0xE8B824));
            g.fillRadialGradientCircle(c, r, Color::fromHex(0xF6CF38), Color::fromHex(0xC89614), {-r * 0.25f, -r * 0.25f});
            g.drawCircle(c, r * 0.92f, Color::fromHex(0x181612), std::max(1.6f, r * 0.06f));

            const float eyeOffX = r * 0.32f;
            const float eyeY = c.y - r * 0.24f;
            const float eyeRx = std::max(2.2f, r * 0.11f);
            const float eyeRy = std::max(4.0f, r * 0.22f);
            g.fillEllipse({c.x - eyeOffX, eyeY}, eyeRx, eyeRy, Color::fromHex(0x161412));
            g.fillEllipse({c.x + eyeOffX, eyeY}, eyeRx, eyeRy, Color::fromHex(0x161412));
            g.fillCircle({c.x - eyeOffX - eyeRx * 0.25f, eyeY - eyeRy * 0.35f}, std::max(1.0f, eyeRx * 0.32f), agedPaper);
            g.fillCircle({c.x + eyeOffX - eyeRx * 0.25f, eyeY - eyeRy * 0.35f}, std::max(1.0f, eyeRx * 0.32f), agedPaper);

            g.drawArc(c, r * 0.60f, 0.35f, 2.79f, Color::fromHex(0x161412), std::max(2.0f, r * 0.085f));
            g.drawLine({c.x - r * 0.62f, c.y + r * 0.12f}, {c.x - r * 0.50f, c.y + r * 0.26f}, Color::fromHex(0x161412),
                       std::max(1.6f, r * 0.06f));
            g.drawLine({c.x + r * 0.62f, c.y + r * 0.12f}, {c.x + r * 0.50f, c.y + r * 0.26f}, Color::fromHex(0x161412),
                       std::max(1.6f, r * 0.06f));
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x161412), Color::fromHex(0xF6CF38),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 1. SKULL & CROSSBONES (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::SkullAndBones: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x141618));
            g.drawCircle(c, r * 0.90f, accentCol.withAlpha(0.65f), 1.2f);

            const float bLen = r * 0.76f;
            g.drawLine({c.x - bLen, c.y - bLen * 0.65f}, {c.x + bLen, c.y + bLen * 0.65f}, agedPaper, std::max(2.6f, r * 0.12f));
            g.drawLine({c.x - bLen, c.y + bLen * 0.65f}, {c.x + bLen, c.y - bLen * 0.65f}, agedPaper, std::max(2.6f, r * 0.12f));
            for (float sx : {-1.0f, 1.0f}) {
                for (float sy : {-1.0f, 1.0f}) {
                    g.fillCircle({c.x + sx * bLen, c.y + sy * bLen * 0.65f}, std::max(2.2f, r * 0.10f), agedPaper);
                }
            }

            g.fillCircle({c.x, c.y - r * 0.12f}, r * 0.46f, agedPaper);
            g.fillRoundedRect({c.x - r * 0.26f, c.y + r * 0.12f, r * 0.52f, r * 0.34f}, 3.0f, agedPaper);
            g.drawCircle({c.x, c.y - r * 0.12f}, r * 0.46f, Color::fromHex(0x141618), 1.2f);

            g.fillCircle({c.x - r * 0.17f, c.y - r * 0.10f}, r * 0.12f, Color::fromHex(0x141618));
            g.fillCircle({c.x + r * 0.17f, c.y - r * 0.10f}, r * 0.12f, Color::fromHex(0x141618));
            std::array<Point<float>, 3> nose = {
                {{c.x, c.y + r * 0.02f}, {c.x - r * 0.07f, c.y + r * 0.16f}, {c.x + r * 0.07f, c.y + r * 0.16f}}};
            g.fillPolygon(nose, Color::fromHex(0x141618));
            for (int t = -2; t <= 2; ++t) {
                const float tx = c.x + static_cast<float>(t) * (r * 0.09f);
                g.drawLine({tx, c.y + r * 0.22f}, {tx, c.y + r * 0.42f}, Color::fromHex(0x141618), 1.2f);
            }
            g.drawLine({c.x - r * 0.24f, c.y + r * 0.32f}, {c.x + r * 0.24f, c.y + r * 0.32f}, Color::fromHex(0x141618), 1.2f);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x141618), agedPaper, accentCol);
            }
            break;
        }

        // ================================================================
        // 2. GOLD STAR EMBLEM ('GOLD STAR', SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::RockStarEmblem: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x181A1E));
            g.drawCircle(c, r * 0.90f, Color::fromHex(0xE8C547), 1.4f);

            std::array<Point<float>, 10> starPts{};
            const float rOuter = r * 0.80f;
            const float rInner = r * 0.34f;
            for (int i = 0; i < 10; ++i) {
                const float a = -1.5707963f + static_cast<float>(i) * 0.6283185f;
                const float rad = (i % 2 == 0) ? rOuter : rInner;
                starPts[static_cast<size_t>(i)] = {c.x + std::cos(a) * rad, c.y + std::sin(a) * rad};
            }
            g.fillPolygon(starPts, Color::fromHex(0xF0C228));
            for (int i = 0; i < 10; ++i) {
                g.drawLine(c, starPts[static_cast<size_t>(i)], (i % 2 == 0) ? agedPaper.withAlpha(0.65f) : Color::fromHex(0x121418, 0.7f),
                           1.1f);
            }
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x181A1E), Color::fromHex(0xF0C228),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 3. CONSOLE SCRIBBLE MASKING TAPE (SHAPE_PAPER_TAPE_RECT)
        // ================================================================
        case StickerPreset::ScribbleMaskingTape: {
            const Rect<float> tape = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            g.fillRect({tape.x + 2.2f, tape.y + 3.0f, tape.width, tape.height}, Color::fromHex(0x030608, 0.58f));

            std::array<Point<float>, 10> poly = {{{tape.x + 3.5f, tape.y},
                                                  {tape.right() - 2.0f, tape.y + 1.0f},
                                                  {tape.right() - 5.5f, tape.y + tape.height * 0.33f},
                                                  {tape.right() + 0.5f, tape.y + tape.height * 0.67f},
                                                  {tape.right() - 4.0f, tape.bottom()},
                                                  {tape.x + 2.0f, tape.bottom() - 1.0f},
                                                  {tape.x + 5.5f, tape.y + tape.height * 0.72f},
                                                  {tape.x - 0.5f, tape.y + tape.height * 0.45f},
                                                  {tape.x + 4.5f, tape.y + tape.height * 0.20f},
                                                  {tape.x + 3.5f, tape.y}}};
            g.fillPolygon(poly, Color::fromHex(0xDFD2B2));
            g.drawPolyline(poly, Color::fromHex(0x8C7A56), 1.1f);

            g.drawLine({tape.x + 4.0f, tape.y + 1.2f}, {tape.right() - 4.0f, tape.y + 1.8f}, Color::fromHex(0xB89858, 0.20f), 1.4f);
            g.drawLine({tape.x + 3.0f, tape.bottom() - 1.6f}, {tape.right() - 5.0f, tape.bottom() - 1.2f}, Color::fromHex(0xA88848, 0.20f),
                       1.4f);
            drawPaperTapePeelAndWear(tape);

            const Color markerInk = Color::fromHex(0x12161C, 0.96f);
            const Color subInk = Color::fromHex(0xB82018, 0.95f);
            if (!subtitle.empty()) {
                g.drawText(title.empty() ? "VOCAL COMP +3dB" : title,
                           {tape.x + 6.0f, tape.y + 1.5f, tape.width - 14.0f, tape.height * 0.54f}, markerInk, fontSizePt * 1.04f, true,
                           true);
                g.drawText(subtitle, {tape.x + 6.0f, tape.y + tape.height * 0.50f, tape.width - 12.0f, tape.height * 0.46f}, subInk,
                           fontSizePt * 0.82f, true, true);
            } else {
                g.drawText(title.empty() ? "VOCAL COMP +3dB" : title,
                           {tape.x + 6.0f, tape.y + 2.0f, tape.width - 14.0f, tape.height - 4.0f}, markerInk, fontSizePt * 1.08f, true,
                           true);
            }
            break;
        }

        // ================================================================
        // 4. STAGE GAFF TAPE (SHAPE_PAPER_TAPE_RECT)
        // ================================================================
        case StickerPreset::BlackGaffTape: {
            const Rect<float> tape = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            g.fillRect({tape.x + 2.2f, tape.y + 3.0f, tape.width, tape.height}, peelShadow);
            g.fillRoundedRect(tape, 2.0f, Color::fromHex(0x171A1E));

            for (float gx = tape.x + 5.0f; gx < tape.right() - 4.0f; gx += 5.5f) {
                g.drawLine({gx, tape.y + 1.0f}, {gx, tape.bottom() - 1.0f}, Color::fromHex(0x262C34, 0.36f), 0.8f);
            }
            for (float gy = tape.y + 3.5f; gy < tape.bottom() - 2.0f; gy += 4.5f) {
                g.drawLine({tape.x + 1.0f, gy}, {tape.right() - 1.0f, gy}, Color::fromHex(0x262C34, 0.32f), 0.8f);
                g.drawLine({tape.x - 3.2f, gy - 0.6f}, {tape.x + 3.8f, gy + 0.5f}, Color::fromHex(0xAEB8C2, 0.72f), 1.0f);
                g.drawLine({tape.right() - 3.8f, gy + 0.5f}, {tape.right() + 3.2f, gy - 0.6f}, Color::fromHex(0xAEB8C2, 0.72f), 1.0f);
            }
            g.drawRoundedRect(tape, 2.0f, Color::fromHex(0x38404A), 1.1f);
            drawPaperTapePeelAndWear(tape);

            const Color paintPenWhite = Color::fromHex(0xF0ECE2, 0.96f);
            if (!subtitle.empty()) {
                g.drawText(title.empty() ? "DO NOT TOUCH!" : title, {tape.x + 6.0f, tape.y + 1.5f, tape.width - 14.0f, tape.height * 0.54f},
                           paintPenWhite, fontSizePt * 1.04f, true, true);
                g.drawText(subtitle, {tape.x + 6.0f, tape.y + tape.height * 0.50f, tape.width - 12.0f, tape.height * 0.46f}, accentCol,
                           fontSizePt * 0.82f, true, true);
            } else {
                g.drawText(title.empty() ? "DO NOT TOUCH!" : title, {tape.x + 6.0f, tape.y + 2.0f, tape.width - 14.0f, tape.height - 4.0f},
                           paintPenWhite, fontSizePt * 1.08f, true, true);
            }
            break;
        }

        // ================================================================
        // 5. EMBOSSED DYMO PUNCH TAPE (SHAPE_RIGID_PASS_OR_PLAQUE —
        // NO paper peel!)
        // ================================================================
        case StickerPreset::EmbossedDymoStrip: {
            const Rect<float> dymo = {bounds.x + 3.0f, bounds.y + 4.0f, bounds.width - 6.0f, bounds.height - 8.0f};
            g.fillRoundedRect({dymo.x + 2.0f, dymo.y + 2.8f, dymo.width, dymo.height}, 2.5f, peelShadow);
            const Color baseTape = (accentCol.r > 0.7f && accentCol.g > 0.5f && accentCol.b < 0.3f)
                                       ? Color::fromHex(0x9E1B22)
                                       : Color{accentCol.r * 0.58f, accentCol.g * 0.58f, accentCol.b * 0.58f, 1.0f};
            g.fillLinearGradientRect(
                dymo,
                Color{std::min(1.0f, baseTape.r * 1.35f), std::min(1.0f, baseTape.g * 1.35f), std::min(1.0f, baseTape.b * 1.35f), 1.0f},
                Color{baseTape.r * 0.65f, baseTape.g * 0.65f, baseTape.b * 0.65f, 1.0f}, 2.5f);
            g.drawLine({dymo.x + 2.0f, dymo.y + 1.5f}, {dymo.right() - 2.0f, dymo.y + 1.5f}, Color::fromHex(0xFFFFFF, 0.42f), 1.2f);
            g.drawRoundedRect(dymo, 2.5f, Color::fromHex(0x0B0E12), 1.3f);

            g.drawCircle({dymo.x + 6.0f, cy}, 2.8f, Color::fromHex(0xE8DEC4, 0.42f), 1.0f);
            g.drawCircle({dymo.right() - 6.0f, cy}, 2.8f, Color::fromHex(0xE8DEC4, 0.42f), 1.0f);
            g.fillCircle({dymo.x + 6.0f, cy}, 2.0f, Color::fromHex(0x101418));
            g.fillCircle({dymo.right() - 6.0f, cy}, 2.0f, Color::fromHex(0x101418));

            const std::string punchTxt =
                subtitle.empty() ? std::string(title.empty() ? "MASTERING UNIT" : title) : std::format("{} · {}", title, subtitle);
            g.drawText(punchTxt, {dymo.x + 13.0f, dymo.y + 1.2f, dymo.width - 24.0f, dymo.height}, Color::fromHex(0x080A0C, 0.85f),
                       fontSizePt * 1.02f, true, true);
            g.drawText(punchTxt, {dymo.x + 12.0f, dymo.y, dymo.width - 24.0f, dymo.height}, Color::fromHex(0xEFECE2), fontSizePt * 1.02f,
                       true, true);
            break;
        }

        // ================================================================
        // 6. HIGH VOLTAGE LIGHTNING BOLT (SHAPE_RIGID_PASS_OR_PLAQUE
        // — NO paper peel!)
        // ================================================================
        case StickerPreset::HighVoltageBolt: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            drawRigidPlaqueBase(card, 5.0f, Color::fromHex(0xF0C224), Color::fromHex(0x141412));
            g.drawRoundedRect({card.x + 3.0f, card.y + 3.0f, card.width - 6.0f, card.height - 6.0f}, 3.5f, Color::fromHex(0x141412), 2.0f);

            const bool hasText = (!title.empty() || !subtitle.empty());
            const float bannerH =
                hasText ? std::clamp(bounds.height * ((!title.empty() && !subtitle.empty()) ? 0.27f : 0.23f), 19.0f, 30.0f) : 0.0f;
            const float bCy = card.y + (card.height - bannerH - 2.0f) * 0.48f;
            const float h = std::max(14.0f, (card.height - bannerH - 2.0f) * 0.38f);

            std::array<Point<float>, 6> bolt = {{{cx + h * 0.22f, bCy - h * 0.90f},
                                                 {cx - h * 0.45f, bCy + h * 0.05f},
                                                 {cx - h * 0.02f, bCy + h * 0.05f},
                                                 {cx - h * 0.28f, bCy + h * 0.92f},
                                                 {cx + h * 0.45f, bCy - h * 0.08f},
                                                 {cx + h * 0.04f, bCy - h * 0.08f}}};
            g.fillPolygon(bolt, Color::fromHex(0x141412));
            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x141412), Color::fromHex(0xF0C224),
                                         Color::fromHex(0xF0C224));
            }
            break;
        }

        // ================================================================
        // 7. RADIOACTIVE TREFOIL (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::RadioactiveTrefoil: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0xE5AD1C));
            g.drawCircle(c, r * 0.90f, Color::fromHex(0x161412), 2.0f);

            for (int b = 0; b < 3; ++b) {
                const float baseAng = -0.5235988f + static_cast<float>(b) * 2.0943951f;
                std::array<Point<float>, 4> blade = {
                    {{c.x + std::cos(baseAng) * r * 0.22f, c.y + std::sin(baseAng) * r * 0.22f},
                     {c.x + std::cos(baseAng - 0.44f) * r * 0.78f, c.y + std::sin(baseAng - 0.44f) * r * 0.78f},
                     {c.x + std::cos(baseAng) * r * 0.82f, c.y + std::sin(baseAng) * r * 0.82f},
                     {c.x + std::cos(baseAng + 0.44f) * r * 0.78f, c.y + std::sin(baseAng + 0.44f) * r * 0.78f}}};
                g.fillPolygon(blade, Color::fromHex(0x161412));
            }
            g.fillCircle(c, r * 0.16f, Color::fromHex(0x161412));
            g.drawCircle(c, r * 0.21f, Color::fromHex(0xE5AD1C), 2.0f);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x161412), Color::fromHex(0xE5AD1C),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 8. YIN-YANG CIRCLE ('BALANCE - ZEN',
        // SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::YinYangCircle: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x16181C));

            std::array<Point<float>, 8> leftHalf = {{{c.x, c.y - r},
                                                     {c.x - r * 0.707f, c.y - r * 0.707f},
                                                     {c.x - r, c.y},
                                                     {c.x - r * 0.707f, c.y + r * 0.707f},
                                                     {c.x, c.y + r},
                                                     {c.x, c.y + r * 0.5f},
                                                     {c.x, c.y - r * 0.5f},
                                                     {c.x, c.y - r}}};
            g.fillPolygon(leftHalf, agedPaper);
            g.fillCircle({c.x, c.y - r * 0.5f}, r * 0.5f, agedPaper);
            g.fillCircle({c.x, c.y + r * 0.5f}, r * 0.5f, Color::fromHex(0x16181C));
            g.fillCircle({c.x, c.y - r * 0.5f}, r * 0.14f, Color::fromHex(0x16181C));
            g.fillCircle({c.x, c.y + r * 0.5f}, r * 0.14f, agedPaper);
            g.drawCircle(c, r, Color::fromHex(0x16181C), 2.0f);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x16181C), agedPaper, accentCol);
            }
            break;
        }

        // ================================================================
        // 9. RETRO PEACE SIGN (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::RetroPeaceSign: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x28382C));
            const float symR = r * 0.78f;
            const float sw = std::max(2.4f, r * 0.11f);
            g.drawCircle(c, symR, accentCol, sw);
            g.drawLine({c.x, c.y - symR}, {c.x, c.y + symR}, accentCol, sw);
            g.drawLine(c, {c.x - symR * 0.707f, c.y + symR * 0.707f}, accentCol, sw);
            g.drawLine(c, {c.x + symR * 0.707f, c.y + symR * 0.707f}, accentCol, sw);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x18221A), agedPaper, accentCol);
            }
            break;
        }

        // ================================================================
        // 10. MAGIC 8-BALL ('8-BALL SAYS - NO!',
        // SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::MagicEightBall: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x10141A));
            g.fillRadialGradientCircle(c, r, Color::fromHex(0x30363E), Color::fromHex(0x0C0E12), {-r * 0.30f, -r * 0.30f});
            g.fillCircle({c.x, c.y - r * 0.06f}, r * 0.44f, agedPaper);
            g.drawText("8", {c.x - r * 0.44f, c.y - r * 0.50f, r * 0.88f, r * 0.88f}, Color::fromHex(0x101216), std::max(11.0f, r * 0.58f),
                       true, true);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x121418), agedPaper, accentCol);
            }
            break;
        }

        // ================================================================
        // 11. STUDIO ADVISORY EXPLICIT (SHAPE_RIGID_PASS_OR_PLAQUE —
        // NO paper peel!)
        // ================================================================
        case StickerPreset::AdvisoryExplicit: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            drawRigidPlaqueBase(card, 3.0f, agedPaper, Color::fromHex(0x121416));

            const float h3 = (card.height - 6.0f) / 3.0f;
            const Rect<float> topBar = {card.x + 3.0f, card.y + 3.0f, card.width - 6.0f, h3};
            const Rect<float> midBar = {card.x + 3.0f, card.y + 3.0f + h3, card.width - 6.0f, h3};
            const Rect<float> botBar = {card.x + 3.0f, card.y + 3.0f + h3 * 2.0f, card.width - 6.0f, h3};

            g.fillRect(topBar, Color::fromHex(0x141618));
            g.fillRect(midBar, agedPaper);
            g.fillRect(botBar, Color::fromHex(0x141618));

            g.drawText("STUDIO", topBar, agedPaper, std::clamp(h3 * 0.62f, 7.0f, 13.0f), true, true);
            g.drawText(title.empty() ? "ADVISORY" : title, midBar, Color::fromHex(0x141618), std::clamp(h3 * 0.68f, 7.5f, 14.0f), true,
                       true);
            g.drawText(subtitle.empty() ? "EXPLICIT THD" : subtitle, botBar, agedPaper, std::clamp(h3 * 0.56f, 6.8f, 11.5f), true, true);
            break;
        }

        // ================================================================
        // 12. VIP BACKSTAGE PASS (SHAPE_RIGID_PASS_OR_PLAQUE — NO
        // paper peel!)
        // ================================================================
        case StickerPreset::BackstageVipPass: {
            const Rect<float> pass = {bounds.x + 3.0f, bounds.y + 2.0f, bounds.width - 6.0f, bounds.height - 4.0f};
            drawRigidPlaqueBase(pass, 5.0f, agedPaper, Color::fromHex(0x524838));
            const Rect<float> inner = {pass.x + 2.5f, pass.y + 2.5f, pass.width - 5.0f, pass.height - 5.0f};
            g.fillLinearGradientRect(inner, Color::fromHex(0xC82430), Color::fromHex(0x6E1018), 4.0f);
            g.drawRoundedRect(inner, 4.0f, accentCol, 1.2f);

            g.fillRoundedRect({cx - 10.0f, inner.y + 3.0f, 20.0f, 4.5f}, 2.0f, Color::fromHex(0x101418));
            const Rect<float> vipBar = {inner.x + 3.0f, inner.y + 10.0f, inner.width - 6.0f, std::max(14.0f, inner.height * 0.26f)};
            g.fillRoundedRect(vipBar, 2.5f, Color::fromHex(0xE8C547));

            g.drawText("VIP // ALL ACCESS", vipBar, Color::fromHex(0x141618), std::clamp(vipBar.height * 0.58f, 7.0f, 11.0f), true, true);
            g.drawText(title.empty() ? "BACKSTAGE" : title,
                       {inner.x + 4.0f, vipBar.bottom() + 2.0f, inner.width - 8.0f, (inner.bottom() - vipBar.bottom()) * 0.55f}, agedPaper,
                       fontSizePt * 0.98f, true, true);
            if (!subtitle.empty()) {
                g.drawText(subtitle,
                           {inner.x + 4.0f, vipBar.bottom() + (inner.bottom() - vipBar.bottom()) * 0.50f, inner.width - 8.0f,
                            (inner.bottom() - vipBar.bottom()) * 0.45f},
                           Color::fromHex(0xE8C547), fontSizePt * 0.78f, true, true);
            }
            break;
        }

        // ================================================================
        // 13. FRAGILE FLIGHT STAMP (SHAPE_RIGID_PASS_OR_PLAQUE — NO
        // paper peel!)
        // ================================================================
        case StickerPreset::FragileFlightStamp: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            const Color redInk = Color::fromHex(0xB8201C);
            drawRigidPlaqueBase(card, 4.0f, Color::fromHex(0xE6D8B8), redInk);
            g.drawRoundedRect({card.x + 3.5f, card.y + 3.5f, card.width - 7.0f, card.height - 7.0f}, 2.5f, redInk, 2.0f);

            if (!subtitle.empty()) {
                g.drawText(title.empty() ? "FRAGILE" : title, {card.x + 6.0f, card.y + 3.0f, card.width - 14.0f, card.height * 0.54f},
                           redInk, fontSizePt * 1.08f, true, true);
                g.drawLine({card.x + 8.0f, card.y + card.height * 0.55f}, {card.right() - 8.0f, card.y + card.height * 0.55f}, redInk,
                           1.2f);
                g.drawText(subtitle, {card.x + 6.0f, card.y + card.height * 0.55f, card.width - 12.0f, card.height * 0.40f},
                           Color::fromHex(0x1E1C18), fontSizePt * 0.80f, true, true);
            } else {
                g.drawText(title.empty() ? "FRAGILE // TUBES" : title,
                           {card.x + 6.0f, card.y + 4.0f, card.width - 14.0f, card.height - 8.0f}, redInk, fontSizePt * 1.08f, true, true);
            }
            break;
        }

        // ================================================================
        // 14. AREA 51 ALIEN HEAD ('AREA 51 - SPACEY!',
        // SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::AlienHeadDecal: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x121614));

            g.fillEllipse({c.x, c.y - r * 0.08f}, r * 0.62f, r * 0.56f, Color::fromHex(0x7CE038));
            std::array<Point<float>, 3> chin = {
                {{c.x - r * 0.56f, c.y + r * 0.04f}, {c.x + r * 0.56f, c.y + r * 0.04f}, {c.x, c.y + r * 0.74f}}};
            g.fillPolygon(chin, Color::fromHex(0x7CE038));

            g.fillEllipse({c.x - r * 0.25f, c.y - r * 0.02f}, r * 0.22f, r * 0.14f, Color::fromHex(0x101214));
            g.fillEllipse({c.x + r * 0.25f, c.y - r * 0.02f}, r * 0.22f, r * 0.14f, Color::fromHex(0x101214));
            g.fillCircle({c.x - r * 0.30f, c.y - r * 0.06f}, r * 0.05f, agedPaper);
            g.fillCircle({c.x + r * 0.20f, c.y - r * 0.06f}, r * 0.05f, agedPaper);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x121614), Color::fromHex(0x7CE038),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 15. BIOHAZARD WARNING (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::BiohazardWarning: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0xE86418));
            g.drawCircle(c, r * 0.90f, Color::fromHex(0x141412), 2.0f);

            for (int i = 0; i < 3; ++i) {
                const float a = -1.5707963f + static_cast<float>(i) * 2.0943951f;
                const Point<float> rc = {c.x + std::cos(a) * r * 0.34f, c.y + std::sin(a) * r * 0.34f};
                const Point<float> cut = {c.x + std::cos(a) * r * 0.46f, c.y + std::sin(a) * r * 0.46f};
                g.fillCircle(rc, r * 0.34f, Color::fromHex(0x141412));
                g.fillCircle(cut, r * 0.22f, Color::fromHex(0xE86418));
            }
            g.drawCircle(c, r * 0.22f, Color::fromHex(0x141412), 2.2f);
            g.fillCircle(c, r * 0.12f, Color::fromHex(0xE86418));
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x141412), Color::fromHex(0xE86418),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 16. FIX IT IN THE MIX — Rectangular Yellow/Black
        // hazard-stripe tape (SHAPE_PAPER_TAPE_RECT)
        // ================================================================
        case StickerPreset::FixItInTheMix: {
            const Rect<float> tape = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            g.fillRect({tape.x + 2.2f, tape.y + 3.0f, tape.width, tape.height}, peelShadow);
            g.fillRoundedRect(tape, 2.5f, Color::fromHex(0xF0C224));

            const float stripeStep = 18.0f;
            for (float sx = tape.x - tape.height; sx < tape.right() + 6.0f; sx += stripeStep) {
                std::array<Point<float>, 4> stripe = {{{std::clamp(sx, tape.x, tape.right()), tape.bottom()},
                                                       {std::clamp(sx + 8.5f, tape.x, tape.right()), tape.bottom()},
                                                       {std::clamp(sx + tape.height * 0.7f + 8.5f, tape.x, tape.right()), tape.y},
                                                       {std::clamp(sx + tape.height * 0.7f, tape.x, tape.right()), tape.y}}};
                g.fillPolygon(stripe, Color::fromHex(0x161412));
            }
            g.drawRoundedRect(tape, 2.5f, Color::fromHex(0x161412), 1.4f);
            drawPaperTapePeelAndWear(tape);

            const Rect<float> centerPlate = {tape.x + 10.0f, tape.y + 5.0f, tape.width - 22.0f, tape.height - 10.0f};
            g.fillRoundedRect(centerPlate, 2.5f, Color::fromHex(0x14161A, 0.94f));
            g.drawRoundedRect(centerPlate, 2.5f, Color::fromHex(0xF0C224), 1.3f);

            const std::string t1 = title.empty() ? "FIX IT IN THE MIX" : std::string(title);
            if (!subtitle.empty()) {
                g.drawText(t1, {centerPlate.x + 4.0f, centerPlate.y + 1.0f, centerPlate.width - 8.0f, centerPlate.height * 0.56f},
                           Color::fromHex(0xF0C224), fontSizePt * 0.96f, true, true);
                g.drawText(subtitle,
                           {centerPlate.x + 4.0f, centerPlate.y + centerPlate.height * 0.52f, centerPlate.width - 8.0f,
                            centerPlate.height * 0.45f},
                           agedPaper, fontSizePt * 0.76f, true, true);
            } else {
                g.drawText(t1, centerPlate, Color::fromHex(0xF0C224), fontSizePt * 1.02f, true, true);
            }
            break;
        }

        // ================================================================
        // 17. CLIENT KNOB - UNPLUGGED — Circular unplugged 1/4" TRS
        // jack icon (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::ClientKnobUnplugged: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x18222C));
            g.drawCircle(c, r * 0.88f, Color::fromHex(0x00F5D4, 0.75f), 1.5f);

            // Angled 1/4" TRS phone plug + dangling unplugged cable arc
            g.drawArc({c.x - r * 0.25f, c.y + r * 0.30f}, r * 0.40f, 0.4f, 2.8f, Color::fromHex(0x6C7A88), 2.4f);
            g.drawLine({c.x - r * 0.38f, c.y + r * 0.22f}, {c.x - r * 0.05f, c.y - r * 0.11f}, Color::fromHex(0x28323C), 6.0f);
            g.drawLine({c.x - r * 0.05f, c.y - r * 0.11f}, {c.x + r * 0.35f, c.y - r * 0.51f}, Color::fromHex(0xDCE4EC), 3.2f);
            g.drawLine({c.x + r * 0.12f, c.y - r * 0.28f}, {c.x + r * 0.20f, c.y - r * 0.36f}, Color::fromHex(0x12161C), 3.4f);
            g.fillCircle({c.x + r * 0.38f, c.y - r * 0.54f}, 2.4f, Color::fromHex(0xF0C224));
            // Red 'NO / UNPLUGGED' slash
            g.drawLine({c.x - r * 0.56f, c.y - r * 0.56f}, {c.x + r * 0.56f, c.y + r * 0.56f}, Color::fromHex(0xFF4D5A, 0.88f), 2.4f);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x141A22), Color::fromHex(0x00F5D4),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 18. PHASE CHECKED - MONO SAFE — Circular Green QC Inspector
        // Stamp with Checkmark & Ø (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::PhaseCheckedMonoSafe: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x102A1E));
            const Color qcGreen = Color::fromHex(0x52B788);
            g.drawCircle(c, r * 0.88f, qcGreen, 1.8f);
            g.drawCircle(c, r * 0.74f, qcGreen.withAlpha(0.55f), 1.0f);

            // Phase Ø symbol on left, bold QC checkmark on right
            const Point<float> pC{c.x - r * 0.24f, c.y - r * 0.04f};
            const float pr = r * 0.26f;
            g.drawCircle(pC, pr, agedPaper, 1.8f);
            g.drawLine({pC.x - pr * 1.15f, pC.y + pr * 1.15f}, {pC.x + pr * 1.15f, pC.y - pr * 1.15f}, agedPaper, 1.8f);

            g.drawLine({c.x + r * 0.04f, c.y + r * 0.06f}, {c.x + r * 0.20f, c.y + r * 0.26f}, qcGreen, 2.8f);
            g.drawLine({c.x + r * 0.20f, c.y + r * 0.26f}, {c.x + r * 0.52f, c.y - r * 0.24f}, qcGreen, 2.8f);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x0E2218), qcGreen, agedPaper);
            }
            break;
        }

        // ================================================================
        // 19. EAR FATIGUE - TAKE A BREAK — Square Red/White studio
        // first-aid cross badge (SHAPE_RIGID_PASS_OR_PLAQUE)
        // ================================================================
        case StickerPreset::EarFatigueFirstAid: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            const Color medRed = Color::fromHex(0xA82224);
            drawRigidPlaqueBase(card, 6.0f, Color::fromHex(0xDDD0B4), medRed);

            const bool hasText = (!title.empty() || !subtitle.empty());
            const float bannerH =
                hasText ? std::clamp(bounds.height * ((!title.empty() && !subtitle.empty()) ? 0.27f : 0.23f), 19.0f, 30.0f) : 0.0f;
            const float crossCy = card.y + (card.height - bannerH - 2.0f) * 0.48f;
            const float arm = std::max(10.0f, std::min(card.width, card.height - bannerH) * 0.28f);
            const float thick = arm * 0.62f;

            g.fillCircle({cx, crossCy}, arm * 1.18f, medRed);
            g.fillRoundedRect({cx - thick * 0.5f, crossCy - arm * 0.78f, thick, arm * 1.56f}, 1.5f, Color::fromHex(0xDDD0B4));
            g.fillRoundedRect({cx - arm * 0.78f, crossCy - thick * 0.5f, arm * 1.56f, thick}, 1.5f, Color::fromHex(0xDDD0B4));

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, medRed, Color::fromHex(0xDDD0B4),
                                         Color::fromHex(0x181414));
            }
            break;
        }

        // ================================================================
        // 20. 2-INCH REEL - 30 IPS MASTER — Circular 3-slot metallic
        // NAB magnetic tape reel emblem (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::TwoInchReel30Ips: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x1C1612));
            // Metallic flange + dark magnetic tape pack + 3 NAB cutouts
            g.fillRadialGradientCircle(c, r * 0.90f, Color::fromHex(0xD4DCE4), Color::fromHex(0x6C7680), {-r * 0.2f, -r * 0.2f});
            g.drawCircle(c, r * 0.72f, Color::fromHex(0x281B12), std::max(2.0f, r * 0.14f));
            for (int slot = 0; slot < 3; ++slot) {
                const float a = -1.5707963f + static_cast<float>(slot) * 2.0943951f;
                std::array<Point<float>, 4> hole = {{{c.x + std::cos(a - 0.22f) * r * 0.34f, c.y + std::sin(a - 0.22f) * r * 0.34f},
                                                     {c.x + std::cos(a - 0.30f) * r * 0.76f, c.y + std::sin(a - 0.30f) * r * 0.76f},
                                                     {c.x + std::cos(a + 0.30f) * r * 0.76f, c.y + std::sin(a + 0.30f) * r * 0.76f},
                                                     {c.x + std::cos(a + 0.22f) * r * 0.34f, c.y + std::sin(a + 0.22f) * r * 0.34f}}};
                g.fillPolygon(hole, Color::fromHex(0x181410));
            }
            g.fillCircle(c, r * 0.26f, Color::fromHex(0x22262C));
            g.drawCircle(c, r * 0.26f, Color::fromHex(0xE8C547), 1.4f);
            g.fillCircle(c, r * 0.10f, Color::fromHex(0xC8CED4));
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x181A1E), Color::fromHex(0xE8C547),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 21. SUB BASS - SEISMIC ZONE — Rectangular yellow warning
        // plaque with Richter seismograph wave spike
        // (SHAPE_RIGID_PASS_OR_PLAQUE)
        // ================================================================
        case StickerPreset::SubBassSeismicZone: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            const Color warnYel = Color::fromHex(0xF0C224);
            drawRigidPlaqueBase(card, 4.5f, warnYel, Color::fromHex(0x141412));

            const Rect<float> screen = {card.x + 5.0f, card.y + 5.0f, card.width - 10.0f, card.height * 0.50f};
            g.fillRoundedRect(screen, 3.0f, Color::fromHex(0x121618));
            g.drawRoundedRect(screen, 3.0f, Color::fromHex(0x2C363C), 1.0f);

            const float wCy = screen.getCentre().y;
            std::array<Point<float>, 11> seis = {{{screen.x + 4.0f, wCy},
                                                  {screen.x + screen.width * 0.18f, wCy},
                                                  {screen.x + screen.width * 0.26f, wCy - screen.height * 0.22f},
                                                  {screen.x + screen.width * 0.34f, wCy + screen.height * 0.28f},
                                                  {screen.x + screen.width * 0.44f, wCy - screen.height * 0.44f},
                                                  {screen.x + screen.width * 0.52f, wCy + screen.height * 0.45f},
                                                  {screen.x + screen.width * 0.60f, wCy - screen.height * 0.38f},
                                                  {screen.x + screen.width * 0.68f, wCy + screen.height * 0.25f},
                                                  {screen.x + screen.width * 0.76f, wCy - screen.height * 0.14f},
                                                  {screen.x + screen.width * 0.84f, wCy},
                                                  {screen.right() - 4.0f, wCy}}};
            g.drawPolyline(seis, Color::fromHex(0xFF5A28), 1.9f);

            const float txtTop = screen.bottom() + 2.0f;
            const float txtH = card.bottom() - txtTop - 2.0f;
            if (!subtitle.empty()) {
                g.drawText(title.empty() ? "SUB BASS" : title, {card.x + 4.0f, txtTop, card.width - 8.0f, txtH * 0.54f},
                           Color::fromHex(0x141412), fontSizePt * 0.96f, true, true);
                g.drawText(subtitle, {card.x + 4.0f, txtTop + txtH * 0.50f, card.width - 8.0f, txtH * 0.48f}, Color::fromHex(0x9E1B18),
                           fontSizePt * 0.78f, true, true);
            } else {
                g.drawText(title.empty() ? "SUB BASS // SEISMIC ZONE" : title, {card.x + 4.0f, txtTop, card.width - 8.0f, txtH},
                           Color::fromHex(0x141412), fontSizePt * 0.96f, true, true);
            }
            break;
        }

        // ================================================================
        // 22. CAL CERTIFIED - 0 VU = +4 dBu — Brushed silver
        // laboratory calibration inspection tag
        // (SHAPE_RIGID_PASS_OR_PLAQUE)
        // ================================================================
        case StickerPreset::CalCertified0Vu: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            drawRigidPlaqueBase(card, 4.0f, Color::fromHex(0xD4DCE4), Color::fromHex(0x4E5864));
            g.fillLinearGradientRect({card.x + 1.5f, card.y + 1.5f, card.width - 3.0f, card.height - 3.0f}, Color::fromHex(0xE2E8EE),
                                     Color::fromHex(0xAEB8C2), 3.5f);

            // Top crimson laboratory header bar + corner rivets
            const Rect<float> hdr = {card.x + 3.0f, card.y + 3.0f, card.width - 6.0f, std::max(14.0f, card.height * 0.30f)};
            g.fillRoundedRect(hdr, 2.0f, Color::fromHex(0x163858));
            g.fillCircle({card.x + 6.5f, card.bottom() - 6.5f}, 1.8f, Color::fromHex(0x48525C));
            g.fillCircle({card.right() - 6.5f, card.bottom() - 6.5f}, 1.8f, Color::fromHex(0x48525C));
            g.drawText(title.empty() ? "CAL CERTIFIED" : title, hdr, Color::fromHex(0xF4F8FB), std::clamp(hdr.height * 0.62f, 7.2f, 10.5f),
                       true, true);

            // Inspection checkbox + subtitle
            const float bodyY = hdr.bottom() + 2.0f;
            const float bodyH = card.bottom() - bodyY - 2.0f;
            const Rect<float> chkBox = {card.x + 8.0f, bodyY + (bodyH - 11.0f) * 0.5f, 11.0f, 11.0f};
            g.drawRect(chkBox, Color::fromHex(0x16202A), 1.2f);
            g.drawLine({chkBox.x + 2.0f, chkBox.y + 5.5f}, {chkBox.x + 4.8f, chkBox.bottom() - 2.0f}, Color::fromHex(0x186E3C), 1.8f);
            g.drawLine({chkBox.x + 4.8f, chkBox.bottom() - 2.0f}, {chkBox.right() - 1.5f, chkBox.y + 2.0f}, Color::fromHex(0x186E3C), 1.8f);

            g.drawText(subtitle.empty() ? "0 VU = +4 dBu" : subtitle,
                       {chkBox.right() + 4.0f, bodyY, card.right() - chkBox.right() - 8.0f, bodyH}, Color::fromHex(0x141A22),
                       fontSizePt * 0.86f, true, true);
            break;
        }

        // ================================================================
        // 23. 100% RAW - NO PITCH CORR — Circular vintage microphone
        // icon badge (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::RawNoPitchCorr: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x201816));
            g.drawCircle(c, r * 0.88f, Color::fromHex(0xE88828), 1.6f);

            // Vintage pill microphone capsule + horizontal grille ribs
            // + yoke stand
            const Rect<float> micHead = {c.x - r * 0.25f, c.y - r * 0.56f, r * 0.50f, r * 0.72f};
            g.fillRoundedRect(micHead, r * 0.24f, Color::fromHex(0xD8E0E8));
            g.drawRoundedRect(micHead, r * 0.24f, Color::fromHex(0x141618), 1.3f);
            for (int gLine = 1; gLine <= 4; ++gLine) {
                const float gy = micHead.y + static_cast<float>(gLine) * (micHead.height / 5.0f);
                g.drawLine({micHead.x + 2.5f, gy}, {micHead.right() - 2.5f, gy}, Color::fromHex(0x38424C), 1.2f);
            }
            g.drawArc({c.x, c.y - r * 0.12f}, r * 0.38f, 0.25f, 2.89f, Color::fromHex(0xE88828), 2.0f);
            g.drawLine({c.x, c.y + r * 0.26f}, {c.x, c.y + r * 0.62f}, Color::fromHex(0xE88828), 2.4f);
            g.drawLine({c.x - r * 0.28f, c.y + r * 0.62f}, {c.x + r * 0.28f, c.y + r * 0.62f}, Color::fromHex(0xE88828), 2.4f);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x1A1412), Color::fromHex(0xF0A038),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 24. 3 AM SESSION - ESPRESSO FUEL — Circular dark-roast
        // coffee cup with rising steam waves
        // (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::ThreeAmEspressoFuel: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x241812));
            g.drawCircle(c, r * 0.88f, Color::fromHex(0xD89A52), 1.5f);

            // Espresso cup + handle + saucer + 3 rising steam waves
            const Rect<float> cup = {c.x - r * 0.38f, c.y - r * 0.08f, r * 0.70f, r * 0.52f};
            g.fillRoundedRect(cup, 4.0f, agedPaper);
            g.drawArc({cup.right(), cup.getCentre().y}, r * 0.16f, -1.3f, 1.3f, agedPaper, 2.2f);
            g.fillEllipse({c.x - r * 0.03f, cup.y + 2.0f}, cup.width * 0.44f, 3.2f, Color::fromHex(0x3B2212));
            g.drawLine({c.x - r * 0.52f, cup.bottom() + 3.0f}, {c.x + r * 0.50f, cup.bottom() + 3.0f}, Color::fromHex(0xD89A52), 2.2f);

            for (int s = -1; s <= 1; ++s) {
                const float sx = c.x + static_cast<float>(s) * (r * 0.20f) - r * 0.04f;
                g.drawLine({sx, c.y - r * 0.20f}, {sx + 2.5f, c.y - r * 0.38f}, Color::fromHex(0xE8D8BE, 0.82f), 1.6f);
                g.drawLine({sx + 2.5f, c.y - r * 0.38f}, {sx - 1.5f, c.y - r * 0.56f}, Color::fromHex(0xE8D8BE, 0.82f), 1.6f);
            }
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x1E140E), Color::fromHex(0xE8B878),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 25. PANIC BUTTON - MUTE IF SUCK — Circular red arcade
        // push-button warning badge (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::PanicButtonMute: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x282C34));
            // Yellow/black collar + 3D domed red arcade plunger button
            g.drawCircle(c, r * 0.86f, Color::fromHex(0xF0C224), 2.6f);
            g.fillCircle({c.x, c.y + 2.0f}, r * 0.64f, Color::fromHex(0x580C12));
            g.fillRadialGradientCircle({c.x, c.y - 1.5f}, r * 0.62f, Color::fromHex(0xFF4852), Color::fromHex(0xB81420),
                                       {-r * 0.18f, -r * 0.18f});
            g.drawArc({c.x, c.y - 1.5f}, r * 0.46f, -2.5f, -0.7f, Color::fromHex(0xFFFFFF, 0.52f), 2.0f);
            g.drawText("PANIC", {c.x - r * 0.52f, c.y - r * 0.24f, r * 1.04f, r * 0.44f}, Color::fromHex(0xFFFFFF),
                       std::clamp(r * 0.30f, 7.0f, 10.5f), true, true);
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x181416), Color::fromHex(0xFF5864),
                                         Color::fromHex(0xF0C224));
            }
            break;
        }

        // ================================================================
        // 26. TAPE OP - PRINT IT! — Rectangular vintage red rubber
        // ink-stamp on cream master tape label
        // (SHAPE_RIGID_PASS_OR_PLAQUE)
        // ================================================================
        case StickerPreset::TapeOpPrintIt: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            const Color stampRed = Color::fromHex(0xC42420);
            drawRigidPlaqueBase(card, 4.0f, Color::fromHex(0xEFE4C8), Color::fromHex(0x786A50));

            // Double-ruled red rubber stamp box
            const Rect<float> sOut = {card.x + 4.5f, card.y + 4.5f, card.width - 9.0f, card.height - 9.0f};
            const Rect<float> sIn = {card.x + 7.0f, card.y + 7.0f, card.width - 14.0f, card.height - 14.0f};
            g.drawRoundedRect(sOut, 2.5f, stampRed, 2.2f);
            g.drawRoundedRect(sIn, 1.5f, stampRed.withAlpha(0.75f), 1.0f);

            if (!subtitle.empty()) {
                g.drawText(title.empty() ? "TAPE OP" : title, {sIn.x + 4.0f, sIn.y + 1.0f, sIn.width - 8.0f, sIn.height * 0.54f}, stampRed,
                           fontSizePt * 1.06f, true, true);
                g.drawLine({sIn.x + 8.0f, sIn.y + sIn.height * 0.54f}, {sIn.right() - 8.0f, sIn.y + sIn.height * 0.54f}, stampRed, 1.2f);
                g.drawText(subtitle, {sIn.x + 4.0f, sIn.y + sIn.height * 0.54f, sIn.width - 8.0f, sIn.height * 0.44f},
                           Color::fromHex(0x1C1814), fontSizePt * 0.84f, true, true);
            } else {
                g.drawText(title.empty() ? "TAPE OP · PRINT IT!" : title, sIn, stampRed, fontSizePt * 1.04f, true, true);
            }
            break;
        }

        // ================================================================
        // 27. 48V PHANTOM - RIBBON KILLER — Circular high-voltage
        // spark + vintage ribbon mic warning decal
        // (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::PhantomRibbonKiller: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x1C1414));

            // Yellow warning triangle inside round vinyl badge +
            // lightning bolt striking ribbon element
            std::array<Point<float>, 3> warnTri = {
                {{c.x, c.y - r * 0.78f}, {c.x - r * 0.74f, c.y + r * 0.56f}, {c.x + r * 0.74f, c.y + r * 0.56f}}};
            g.fillPolygon(warnTri, Color::fromHex(0xF0C224));
            g.drawPolyline(warnTri, Color::fromHex(0xD82820), 2.0f);

            std::array<Point<float>, 6> spark = {{{c.x + r * 0.10f, c.y - r * 0.42f},
                                                  {c.x - r * 0.22f, c.y + r * 0.04f},
                                                  {c.x - r * 0.02f, c.y + r * 0.04f},
                                                  {c.x - r * 0.14f, c.y + r * 0.46f},
                                                  {c.x + r * 0.22f, c.y - r * 0.02f},
                                                  {c.x + r * 0.02f, c.y - r * 0.02f}}};
            g.fillPolygon(spark, Color::fromHex(0x161412));
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x181212), Color::fromHex(0xF0C224),
                                         Color::fromHex(0xD82820));
            }
            break;
        }

        // ================================================================
        // 28. FINAL_MIX_V12_REAL.WAV — Retro studio DAT cassette /
        // master reel spine label (SHAPE_PAPER_TAPE_RECT)
        // ================================================================
        case StickerPreset::FinalMixV12Wav: {
            const Rect<float> tape = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            g.fillRect({tape.x + 2.2f, tape.y + 3.0f, tape.width, tape.height}, peelShadow);
            g.fillRoundedRect(tape, 2.5f, Color::fromHex(0xF2EAD6));

            // Retro red/orange/blue DAT spine index stripes on left +
            // ruled notebook lines
            const Rect<float> tagStripe = {tape.x + 2.0f, tape.y + 2.0f, 14.0f, tape.height - 4.0f};
            g.fillRect({tagStripe.x, tagStripe.y, tagStripe.width, tagStripe.height * 0.34f}, Color::fromHex(0xD83828));
            g.fillRect({tagStripe.x, tagStripe.y + tagStripe.height * 0.34f, tagStripe.width, tagStripe.height * 0.33f},
                       Color::fromHex(0xE88C18));
            g.fillRect({tagStripe.x, tagStripe.y + tagStripe.height * 0.67f, tagStripe.width, tagStripe.height * 0.33f},
                       Color::fromHex(0x1E64A8));
            g.drawLine({tape.x + 18.0f, tape.y + tape.height * 0.52f}, {tape.right() - 4.0f, tape.y + tape.height * 0.52f},
                       Color::fromHex(0x5882B8, 0.40f), 1.0f);
            g.drawRoundedRect(tape, 2.5f, Color::fromHex(0x7C705A), 1.1f);
            drawPaperTapePeelAndWear(tape);

            const Color penBlue = Color::fromHex(0x142238, 0.96f);
            const Color penRed = Color::fromHex(0xB82018, 0.95f);
            if (!subtitle.empty()) {
                g.drawText(title.empty() ? "FINAL_MIX_V12_REAL.WAV" : title,
                           {tape.x + 20.0f, tape.y + 1.5f, tape.width - 28.0f, tape.height * 0.52f}, penBlue, fontSizePt * 0.92f, true,
                           true);
                g.drawText(subtitle, {tape.x + 20.0f, tape.y + tape.height * 0.50f, tape.width - 26.0f, tape.height * 0.46f}, penRed,
                           fontSizePt * 0.78f, true, true);
            } else {
                g.drawText(title.empty() ? "FINAL_MIX_V12_REAL.WAV" : title,
                           {tape.x + 20.0f, tape.y + 2.0f, tape.width - 28.0f, tape.height - 4.0f}, penBlue, fontSizePt * 0.96f, true,
                           true);
            }
            break;
        }

        // ================================================================
        // 29. CLASS-A - SPACE HEATER — Circular orange thermal dial /
        // radiator flame emblem (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::ClassASpaceHeater: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x20120E));
            g.drawCircle(c, r * 0.88f, Color::fromHex(0xFF6B2B), 1.6f);

            // Glowing Class-A radiator fins + orange/gold thermal flame
            // tongue
            for (int fin = -2; fin <= 2; ++fin) {
                const float fx = c.x + static_cast<float>(fin) * (r * 0.22f);
                g.drawLine({fx, c.y + r * 0.18f}, {fx, c.y + r * 0.58f}, Color::fromHex(0xFF9F1C, 0.75f), 2.2f);
            }
            std::array<Point<float>, 5> outerFlame = {{{c.x, c.y - r * 0.68f},
                                                       {c.x + r * 0.42f, c.y - r * 0.10f},
                                                       {c.x + r * 0.26f, c.y + r * 0.28f},
                                                       {c.x - r * 0.26f, c.y + r * 0.28f},
                                                       {c.x - r * 0.42f, c.y - r * 0.10f}}};
            g.fillPolygon(outerFlame, Color::fromHex(0xFF5A1C));
            std::array<Point<float>, 4> innerFlame = {
                {{c.x, c.y - r * 0.38f}, {c.x + r * 0.22f, c.y + r * 0.04f}, {c.x, c.y + r * 0.26f}, {c.x - r * 0.22f, c.y + r * 0.04f}}};
            g.fillPolygon(innerFlame, Color::fromHex(0xF8D030));
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x1E120E), Color::fromHex(0xFF8830),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 30. HEADROOM - WHAT HEADROOM? — Circular badge with VU
        // needle bent against +3 dB stop pin
        // (SHAPE_ROUND_VINYL_DECAL)
        // ================================================================
        case StickerPreset::HeadroomWhatHeadroom: {
            float bannerH = 0.0f;
            Point<float> c{};
            float r = 16.0f;
            const bool hasText = computeRoundDecalGeom(bannerH, c, r);
            drawRoundVinylDecalBase(c, r, Color::fromHex(0x1E1A16));

            // Warm amber VU dial face + red overload zone + violently
            // bent needle against right stop pin
            g.fillRadialGradientCircle(c, r * 0.90f, Color::fromHex(0x3C2C18), Color::fromHex(0x181410), {0.0f, r * 0.2f});
            g.drawArc({c.x, c.y + r * 0.32f}, r * 0.72f, -2.35f, -1.30f, agedPaper, 1.8f);
            g.drawArc({c.x, c.y + r * 0.32f}, r * 0.72f, -1.30f, -0.75f, Color::fromHex(0xFF3B30), 2.8f);
            g.drawText("+3dB", {c.x + r * 0.05f, c.y - r * 0.56f, r * 0.68f, 13.0f}, Color::fromHex(0xFF4D5A), 7.5f, true, true);

            // Red stop pin + bent needle with impact sparks
            const Point<float> pivot{c.x, c.y + r * 0.48f};
            const Point<float> bendPt{c.x + r * 0.44f, c.y - r * 0.06f};
            const Point<float> tipPt{c.x + r * 0.22f, c.y - r * 0.42f};
            g.fillCircle({c.x + r * 0.50f, c.y - r * 0.04f}, 2.6f, Color::fromHex(0xFF3B30));
            g.drawLine(pivot, bendPt, Color::fromHex(0xFF6B2B), 2.2f);
            g.drawLine(bendPt, tipPt, Color::fromHex(0xFF6B2B), 2.2f);
            g.fillCircle(pivot, 3.2f, Color::fromHex(0xC8CED4));
            drawRoundVinylGloss(c, r);

            if (hasText) {
                drawStickerCaptionBanner(bounds.bottom() - bannerH - 1.5f, bannerH, Color::fromHex(0x181412), Color::fromHex(0xFF5A36),
                                         agedPaper);
            }
            break;
        }

        // ================================================================
        // 31. CUSTOM IMAGE DECAL (Placeholder frame when no image
        // uploaded yet — SHAPE_RIGID_PASS_OR_PLAQUE)
        // ================================================================
        case StickerPreset::CustomImageDecal:
        default: {
            const Rect<float> card = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            drawRigidPlaqueBase(card, 6.0f, agedPaper, Color::fromHex(0x584E3C));
            const Rect<float> inner = {card.x + 3.5f, card.y + 3.5f, card.width - 7.0f, card.height - 7.0f};
            g.fillRoundedRect(inner, 4.0f, Color::fromHex(0x181C22));
            g.drawRoundedRect(inner, 4.0f, accentCol, 1.3f);

            const float iconCy = inner.y + inner.height * 0.38f;
            g.drawRoundedRect({cx - 14.0f, iconCy - 9.0f, 28.0f, 18.0f}, 3.0f, accentCol, 1.4f);
            g.drawCircle({cx, iconCy}, 5.5f, agedPaper, 1.4f);

            g.drawText(title.empty() ? "CUSTOM STICKER" : title, {inner.x + 4.0f, inner.bottom() - 28.0f, inner.width - 8.0f, 14.0f},
                       agedPaper, std::clamp(fontSizePt * 0.85f, 7.5f, 11.0f), true, true);
            g.drawText(subtitle.empty() ? "CLICK 'UPLOAD IMG'" : subtitle,
                       {inner.x + 4.0f, inner.bottom() - 14.0f, inner.width - 8.0f, 12.0f}, accentCol,
                       std::clamp(fontSizePt * 0.72f, 6.8f, 9.5f), true, true);
            break;
        }
        }

        // Apply authentic studio weathering (edge grime, corner
        // smudges, scratches, coffee/oil ring arc & micro-specks) to
        // every sticker preset
        float weatherBannerH = 0.0f;
        Point<float> weatherC = bounds.getCentre();
        float weatherRad = minDim * 0.42f;
        const bool isRoundDecal = (getStickerShapeClass(preset) == StickerShapeClass::SHAPE_ROUND_VINYL_DECAL);
        if (isRoundDecal) { computeRoundDecalGeom(weatherBannerH, weatherC, weatherRad); }
        drawStickerSurfaceWeathering(bounds, isRoundDecal, weatherC, weatherRad);
        if (!isRoundDecal) {
            Rect<float> peelRect = {bounds.x + 2.0f, bounds.y + 2.0f, bounds.width - 4.0f, bounds.height - 4.0f};
            if (preset == StickerPreset::EmbossedDymoStrip) {
                peelRect = {bounds.x + 3.0f, bounds.y + 4.0f, bounds.width - 6.0f, bounds.height - 8.0f};
            } else if (preset == StickerPreset::BackstageVipPass) {
                peelRect = {bounds.x + 3.0f, bounds.y + 2.0f, bounds.width - 6.0f, bounds.height - 4.0f};
            }
            drawDogEarCornerPeel(peelRect);
        }
    }

    /**
     * @brief Smart Acoustic Unit Formatter (Law 6):
     * Automatically scales frequencies >= 1000 Hz to `kHz` (e.g.
     * `24000.0 Hz` -> `"24.0"`, `"kHz"`; `3800.0 Hz` -> `"3.8"`,
     * `"kHz"`) and times >= 1000 ms to `s` (e.g. `1500.0 ms` ->
     * `"1.5"`, `"s"`), splitting numeric readout and unit cleanly.
     */
    static void formatSmartReadoutAndUnit(std::string_view rawValText, std::string_view rawUnitText, std::string &outValueStr,
                                          std::string &outUnitStr) {
        std::string s(rawValText);
        // Trim leading/trailing spaces
        while (!s.empty() && s.front() == ' ')
            s.erase(s.begin());
        while (!s.empty() && s.back() == ' ')
            s.pop_back();

        outUnitStr = std::string(rawUnitText);

        // Replace ASCII -OO with proper Unicode -∞
        if (const auto ooPos = s.find("-OO"); ooPos != std::string::npos) { s.replace(ooPos, 3, "-\xE2\x88\x9E"); }

        // Check if s starts with a number (+/-/. or digit)
        if (!s.empty() && (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '+' || s[0] == '-' || s[0] == '.')) {
            // Find where the numeric part ends
            size_t numEnd = 0;
            bool seenDot = false;
            for (size_t i = 0; i < s.size(); ++i) {
                const char c = s[i];
                if ((i == 0 && (c == '+' || c == '-')) || std::isdigit(static_cast<unsigned char>(c))) {
                    numEnd = i + 1;
                } else if (c == '.' && !seenDot) {
                    seenDot = true;
                    numEnd = i + 1;
                } else {
                    break;
                }
            }
            if (numEnd > 0) {
                std::string numPart = s.substr(0, numEnd);
                std::string restUnit = s.substr(numEnd);
                while (!restUnit.empty() && restUnit.front() == ' ')
                    restUnit.erase(restUnit.begin());
                if (!restUnit.empty() && outUnitStr.empty()) { outUnitStr = restUnit; }
                try {
                    const float val = std::stof(numPart);
                    if (outUnitStr == "Hz" || outUnitStr == "hz") {
                        if (std::abs(val) >= 1000.0f) {
                            const float khz = val * 0.001f;
                            outValueStr = (khz >= 10.0f) ? std::format("{:.1f}", khz) : std::format("{:.2f}", khz);
                            outUnitStr = "kHz";
                            return;
                        } else if (std::abs(val) >= 100.0f) {
                            outValueStr = std::format("{:.0f}", val);
                            outUnitStr = "Hz";
                            return;
                        } else if (std::abs(val) >= 50.0f) {
                            outValueStr = std::format("{:.1f}", val);
                            outUnitStr = "Hz";
                            return;
                        } else {
                            outValueStr = std::format("{:.2f}", val);
                            outUnitStr = "Hz";
                            return;
                        }
                    } else if (outUnitStr == "ms") {
                        if (std::abs(val) >= 1000.0f) {
                            outValueStr = std::format("{:.2f}", val * 0.001f);
                            outUnitStr = "s";
                            return;
                        } else if (std::abs(val) >= 100.0f) {
                            outValueStr = std::format("{:.0f}", val);
                            outUnitStr = "ms";
                            return;
                        } else {
                            outValueStr = std::format("{:.1f}", val);
                            outUnitStr = "ms";
                            return;
                        }
                    } else if (outUnitStr == "%") {
                        outValueStr = std::format("{:.0f}", val);
                        return;
                    } else {
                        outValueStr = numPart;
                        return;
                    }
                } catch (...) {}
            }
        }
        outValueStr = s;
    }

    /**
     * @brief Renders a true "Lamp-Off" powered-down dark glass
     * meter housing when "EARS ONLY"
     * (`earsOnlyBlackoutMode` / `darkMetersMode`) is active,
     * without calling any active meter internals.
     */
    static void drawPoweredOffMeterHousing(Graphics &g, Rect<float> r) noexcept {
        // 1. Cast drop shadow + opaque powered-off dark glass
        // gradient (0x05070A to 0x090D12)
        g.fillRoundedRect({r.x + 1.5f, r.y + 2.5f, r.width, r.height}, 5.5f, Color(0.0f, 0.0f, 0.0f, 0.65f));
        g.fillLinearGradientRect(r, Color::fromHex(0x05070A), Color::fromHex(0x090D12), 5.5f);

        // 2. Outer hardware bezel & deep recessed inner glass shadow
        g.drawRoundedRect(r, 5.5f, Color::fromHex(0x28323B), 1.4f);
        if (r.width > 8.0f && r.height > 8.0f) {
            g.drawRoundedRect(r.reduced(2.0f), 4.0f, Color::fromHex(0x020305), 1.2f);
            // Subtle diagonal glass reflection across powered-off lens
            const float reflSpan = std::min(r.width - 10.0f, r.height * 1.15f);
            if (reflSpan > 6.0f) {
                g.drawLine({r.x + 6.0f, r.bottom() - 5.0f}, {r.x + 6.0f + reflSpan, r.y + 5.0f}, Color(1.0f, 1.0f, 1.0f, 0.055f), 2.2f);
            }
            g.drawLine({r.x + 5.0f, r.y + 2.0f}, {r.right() - 5.0f, r.y + 2.0f}, Color(1.0f, 1.0f, 1.0f, 0.07f), 1.0f);
        }

        // 3. Clean muted amber badge centered inside: "👁 EARS ONLY —
        // METER OFF"
        const float badgeW = std::clamp(r.width - 10.0f, 44.0f, 196.0f);
        const float badgeH = std::min(22.0f, std::max(14.0f, r.height - 6.0f));
        const Rect<float> badgeRect = {r.getCentre().x - badgeW * 0.5f, r.getCentre().y - badgeH * 0.5f, badgeW, badgeH};
        if (r.height >= 28.0f && r.width >= 90.0f) {
            g.fillRoundedRect(badgeRect, 3.5f, Color::fromHex(0x0D1117, 0.92f));
            g.drawRoundedRect(badgeRect, 3.5f, Color::fromHex(0xD4AF37, 0.35f), 1.0f);
        }
        const float fs = (r.width < 95.0f || r.height < 36.0f) ? 6.8f : 8.2f;
        const char *badgeTxt = (r.width < 88.0f) ? "METER OFF" : "EARS ONLY - METER OFF";
        g.drawText(badgeTxt, r.reduced(3.0f), Color::fromHex(0xD89B38, 0.86f), fs, true, true);
    }
};

/**
 * @brief Physical knob style matching vintage, console, stompbox,
 * synth, relic & modern hardware (16 styles).
 */
enum class HardwareKnobStyle : int {
    PointerSkirted = 0,      // Fluted silver metallic skirt + tapered 3D
                             // Marconi tear-drop pointer cap
    SkirtedCap = 1,          // Fluted silver metallic skirt + round dark 3D
                             // center cap with recessed white inlay
    MarconiRed = 2,          // Fluted silver metallic skirt + winged
                             // crimson-red 3D pointer cap
    ModernArc = 3,           // Fluted silver skirt + dark anodized 3D cap
                             // with subtle outer LED ring
    BakeliteMaster = 4,      // Large vintage fluted dark bakelite rotary
                             // with brass center insert
    ColorCapConsole = 5,     // Classic console fluted skirt with
                             // color-coded 3D anodized center cap
    ChickenHead = 6,         // Vintage pointed amplifier/stompbox beak knob
                             // with brass center rivet
    FlatVector = 7,          // Sculpted studio rotary with illuminated arc collar
    GoldDome = 8,            // Mirror-polished 24K brushed gold/brass dome rotary
    MacroHalo = 9,           // Big center studio dial flanked by dual
                             // segmented LED halo wings
    KnurledBlackBarrel = 10, // Heavy knurled black anodized steel
                             // cylinder (FET/VCA Limiter style)
    ColletBroadcast = 11,    // German/Swiss broadcast matte collet
                             // knob with colored cap & wedge pointer
    StarburstSynth = 12,     // Retro-futuristic polygonal synth grip
                             // with spun-silver starburst cap
    GrimyRelic = 13,         // Road-worn distressed relic knob with
                             // oxidized rim, scratches & aged pointer
    SteppedAttenuator = 14,  // 24-detent mastering rotary switch with visible gold
                             // ball-bearing detent ring
    VintageSkirtedNum = 15,  // Classic 60s numbered silver-skirted
                             // amp/console rotary
    ConcentricDual = 16,     // Dual-concentric console EQ knob with
                             // outer frequency ring & inner gain cap
    CyberLuxeHalo = 17,      // Ribbed dark carbon/gunmetal dial with
                             // glowing cyan halo ring
    NeumorphicGlass = 18,    // Soft extruded white/cream neumorphic
                             // dial with warm/cool perimeter glow arc
    SciFiHudRing = 19,       // Futuristic starship HUD concentric
                             // segmented energy ring dial
    ValveWheelRelic = 20,    // Industrial red cast-iron 5-spoke valve handwheel
    SubmarineHelm = 21,      // Nautical brass 8-spoke ship-wheel helm &
                             // bioluminescent porthole dial
    TattooFlash = 22,        // Traditional tattoo iron coil / sacred ruby
                             // heart / chrome skull dial
    MetalSkull = 23,         // Forged 3D pewter skull rotary in spiked iron collar
    FlowerDaisy = 24,        // 60s psychedelic white daisy petal rotary
                             // with rainbow spiral center
    ReaperCrypt = 25,        // Necropolis carved skull / hourglass / bone
                             // pointer dial
    HellfireMagma = 26,      // Cracked volcanic obsidian & glowing
                             // molten lava fissure rotary
    ApocalypseCog = 27,      // Post-apocalyptic rusted gear cogwheel /
                             // bottle-cap rotary
    PunkSpiked = 28,         // DIY punk studded collar / 2-tone ska
                             // checkerboard / stencil rotary
    MeadowWoodSlice = 29,    // Carved tree-trunk wood ring rotary with
                             // sprouting leaf/flower
    SkySunburst = 30,        // Golden sunburst ray / cumulus cloud orb rotary
    StormRaindrop = 31,      // Wet storm-slate anodized dial with
                             // electric lightning cyan arc
    GlacialIceCrystal = 32   // Faceted translucent frozen ice-crystal
                             // / snowflake rotary
};

/**
 * @brief Photorealistic Direct2D Hardware Rotary Knob with 6
 * physical depth passes: Pass 1: Ambient + Cast Drop Shadow
 * (+3px, +5px at 45% opacity) Pass 2: Machined Outer Brushed
 * Silver Skirt (#C8CED4..#687078) with 72 fine radial knurl ticks
 *   Pass 3: Recessed Inner Dish (#181C20..#2C3238) with inverted
 * top-left shadow bevel Pass 4: 3D Sculpted Pointer Cap (Marconi
 * Tear-Drop / Winged / Cylindrical / Bakelite) with top-left
 * specular highlight Pass 5: High-Contrast Recessed Off-White
 * Inlay Bar (#F0F4F8) integrated into the 3D cap nose (NEVER a
 * thin blue stick!) Pass 6: Radial Scale Ticks (225° to 495° with
 * major & minor sub-ticks) & Smart Silkscreen Typography
 * (Hz->kHz, ms->s)
 */
class HardwareKnob : public Component {
  public:
    explicit HardwareKnob(Parameter *mainParam, HardwareKnobStyle style = HardwareKnobStyle::PointerSkirted, std::string labelTitle = "",
                          Parameter *outerRingParam = nullptr)
        : param_(mainParam), outerParam_(outerRingParam), style_(style), customTitle_(std::move(labelTitle)) {}

    void setScaleLabels(std::vector<std::string> labels, bool firstIsOrangeOff = false, bool lastIsOrangeAuto = false) {
        scaleLabels_ = std::move(labels);
        firstIsOrange_ = firstIsOrangeOff;
        lastIsOrange_ = lastIsOrangeAuto;
    }

    void setSubReadoutOverride(std::string unitText) { unitOverride_ = std::move(unitText); }

    void setAccentColor(Color c) noexcept { accentCol_ = c; }

    static void drawRotaryVisual(Graphics &g, Rect<float> bounds, HardwareKnobStyle style, float norm, std::string_view title,
                                 std::string_view valText, std::string_view unitText, const std::vector<std::string> &scaleLabels,
                                 bool firstIsOrange = false, bool lastIsOrange = false, bool hasOuterRing = false, float outerNorm = 0.5f,
                                 Color accentCol = Color::fromHex(0xFF7A1A), bool useCustomKnobColors = false,
                                 uint32_t bodyColorHex = 0xC8CED4, uint32_t capColorHex = 0x1E2226, uint32_t indicatorColorHex = 0xEDEAE0,
                                 KnobTickMarkStyle tickStyle = KnobTickMarkStyle::RadialLines,
                                 UiLabelPlaqueStyle labelPlaque = UiLabelPlaqueStyle::DirectSilkscreen, bool hasWaxMark = false,
                                 float waxMarkNorm = 0.5f) noexcept {
        // Exact 225 deg (7 o'clock, -135 deg) to 495 deg (5 o'clock,
        // +135 deg) studio sweep
        constexpr float startAngle = -kPi * 0.75f; // -135 deg
        constexpr float endAngle = +kPi * 0.75f;   // +135 deg
        const float clampedNorm = math::clamp(norm, 0.0f, 1.0f);
        const float angle = startAngle + clampedNorm * (endAngle - startAngle);

        // Treat `bounds` (`r`) directly as the Physical Outer Chrome
        // Skirt Diameter — NEVER subtract labelH, 28.0f, or tickPad
        // from `bounds` when computing center & radius!
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy = bounds.y + bounds.height * 0.5f;
        const float minDim = std::min(bounds.width, bounds.height);
        const float R = (minDim > 92.0f) ? (minDim * 0.28f) : (minDim * 0.42f);
        const float uiScale = std::clamp(R / 27.0f, 0.68f, 1.15f);
        const Point<float> center{cx, cy};

        // ====================================================================
        // LAYER 7 (Perimeter Scale): ALWAYS-VISIBLE RADIAL TICKS &
        // LEGENDS OUTSIDE `R`
        // ====================================================================
        const bool lightChassis = HardwareMaterials::isLightChassis(g);
        const Color warmAmberTick = HardwareMaterials::resolveChassisAccentInk(g, accentCol);
        const bool hasCustomAccent = !(accentCol.r > 0.90f && accentCol.g > 0.35f && accentCol.g < 0.70f && accentCol.b < 0.22f);
        const Color silkTickCream = g.resolveSilkscreenColor(Color::fromHex(0xEDEAE0), 0.96f);
        const Color minorTickCol =
            lightChassis ? Color::fromHex(0x46525C, 0.72f) : g.resolveSilkscreenColor(Color::fromHex(0xA8B8C0), 0.58f);
        const Color scaleShadow = Color(0.0f, 0.0f, 0.0f, 0.55f);

        static const std::vector<std::string> kDefaultScaleLabels = {"-12", "-6", "0", "+6", "+12"};
        const std::vector<std::string> &activeScaleLabels = !scaleLabels.empty() ? scaleLabels : kDefaultScaleLabels;

        if (tickStyle != KnobTickMarkStyle::None) {
            const size_t numMajor = (tickStyle == KnobTickMarkStyle::Numbered0To10) ? 11 : activeScaleLabels.size();

            // Minor sub-ticks between major ticks (24 total minor
            // divisions) outside R
            if (tickStyle != KnobTickMarkStyle::Dots) {
                constexpr int kMinorDivs = 24;
                const float rMinIn = R + 2.2f * uiScale;
                const float rMinOut = R + 5.0f * uiScale;
                for (int m = 0; m <= kMinorDivs; ++m) {
                    const float mt = static_cast<float>(m) / static_cast<float>(kMinorDivs);
                    const float ma = startAngle + mt * (endAngle - startAngle);
                    const float ms = std::sin(ma), mc = std::cos(ma);
                    g.drawLine({cx + rMinIn * ms, cy - rMinIn * mc}, {cx + rMinOut * ms, cy - rMinOut * mc}, minorTickCol, 0.9f);
                }
            }

            if (tickStyle == KnobTickMarkStyle::GlowArc) {
                const float arcR = R + 4.0f * uiScale;
                g.drawArc(center, arcR, startAngle, endAngle, Color::fromHex(0x0A1014, 0.78f), 2.5f * uiScale);
                if (clampedNorm > 0.01f) { g.drawArc(center, arcR, startAngle, angle, warmAmberTick.withAlpha(0.85f), 2.8f * uiScale); }
            }

            const float stepTol = (numMajor > 1) ? (0.48f / static_cast<float>(numMajor - 1)) : 0.25f;

            // Perimeter label decimation: with more than 7 step labels
            // (e.g. a 20-position gain switch) every step still gets
            // its tick, but only 5 evenly spaced labels (always both
            // ends) are printed so neighbouring legends never stack on
            // top of each other.
            const bool decimateLabels = (tickStyle != KnobTickMarkStyle::Numbered0To10) && (activeScaleLabels.size() > 7);
            constexpr size_t kMaxPerimeterLabels = 5;
            const auto isLabelledStep = [&](size_t idx) noexcept -> bool {
                if (!decimateLabels) return true;
                const size_t last = activeScaleLabels.size() - 1;
                for (size_t k = 0; k < kMaxPerimeterLabels; ++k) {
                    const size_t pick = static_cast<size_t>(
                        std::lround(static_cast<double>(k) * static_cast<double>(last) / static_cast<double>(kMaxPerimeterLabels - 1)));
                    if (pick == idx) return true;
                }
                return false;
            };

            for (size_t i = 0; i < numMajor; ++i) {
                const float t = (numMajor > 1) ? static_cast<float>(i) / static_cast<float>(numMajor - 1) : 0.5f;
                const float a = startAngle + t * (endAngle - startAngle);
                const float sinA = std::sin(a);
                const float cosA = std::cos(a);

                const bool isCardinal = (i == 0) || (i + 1 == numMajor) || (i * 2 + 1 == numMajor);
                const bool isActiveStep = (style == HardwareKnobStyle::SteppedAttenuator) && (std::abs(t - clampedNorm) <= stepTol);
                bool isAmberCallout = (i == 0 && firstIsOrange) || (i + 1 == numMajor && lastIsOrange) || isActiveStep;
                std::string lblStr;
                if (tickStyle == KnobTickMarkStyle::Numbered0To10) {
                    lblStr = (i % 2 == 0) ? std::to_string(i) : "";
                } else if (i < activeScaleLabels.size() && isLabelledStep(i)) {
                    lblStr = activeScaleLabels[i];
                }
                if (lblStr == "OFF" || lblStr == "AUTO" || lblStr == "A1" || lblStr == "A2" || lblStr == "ALL" || lblStr == "LINE" ||
                    lblStr == "MIC" || lblStr.find("\xE2\x88\x9E") != std::string::npos) {
                    isAmberCallout = true;
                }
                const Color tickCol = isAmberCallout ? warmAmberTick : silkTickCream;

                if (tickStyle == KnobTickMarkStyle::Dots) {
                    const float rDot = R + 4.5f * uiScale;
                    const Color inactiveDotCol = lightChassis ? Color::fromHex(0x46525C, 0.85f) : tickCol;
                    g.fillCircle({cx + rDot * sinA, cy - rDot * cosA}, (isCardinal ? 2.0f : 1.4f) * uiScale,
                                 (t <= clampedNorm + 0.01f) ? warmAmberTick : inactiveDotCol);
                } else {
                    // Radial tick marks from R + 2.5f to R + 7.5f
                    const float rInner = R + 2.5f * uiScale;
                    const float rOuter = R + 7.5f * uiScale;
                    g.drawLine({cx + rInner * sinA, cy - rInner * cosA}, {cx + rOuter * sinA, cy - rOuter * cosA}, tickCol,
                               (isAmberCallout || isCardinal) ? 1.6f : 1.2f);
                }
            }
        }

        // ====================================================================
        // LAYER 1: CAST DROP SHADOW (GROUNDING KNOB ON CHASSIS)
        // ====================================================================
        g.fillRadialGradientCircle3Stops({cx + 1.0f, cy + 2.0f}, R * 1.05f, Color{0.0f, 0.0f, 0.0f, 0.48f}, 0.72f,
                                         Color{0.0f, 0.0f, 0.0f, 0.42f}, Color{0.0f, 0.0f, 0.0f, 0.0f});
        g.fillCircle({cx + R * 0.05f, cy + R * 0.11f}, R * 1.02f, Color{0.0f, 0.0f, 0.0f, 0.52f});

        const float angleDeg = angle * 57.2957795f;
        const float ptrSin = std::sin(angle);
        const float ptrCos = std::cos(angle);

        const bool isClassicRockGoldAmp = (style == HardwareKnobStyle::ChickenHead && useCustomKnobColors && bodyColorHex == 0xD4AF37u);
        const bool isNeonSynthOrEdmArc =
            (style == HardwareKnobStyle::ModernArc && useCustomKnobColors && (bodyColorHex == 0x3D1266u || bodyColorHex == 0x241C18u));

        if (static_cast<int>(style) < 17 && !isClassicRockGoldAmp && !isNeonSynthOrEdmArc) {
            // ====================================================================
            // LAYER 2: 6-STOP POLISHED METALLIC / BAKELITE / ANODIZED
            // SKIRT (RADIUS = R)
            // ====================================================================
            if (style == HardwareKnobStyle::GoldDome ||
                (style == HardwareKnobStyle::SkirtedCap && useCustomKnobColors && bodyColorHex == 0xD4AF37u)) {
                g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                 Color::fromHex(0xF8E8B8), Color::fromHex(0xC6A452), Color::fromHex(0xF4DF9E),
                                                 Color::fromHex(0x8C6B28), Color::fromHex(0xD4B464), Color::fromHex(0x6E5628));
            } else if (style == HardwareKnobStyle::GrimyRelic) {
                // Aged tarnished brass & oxidized bronze skirt
                g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                 Color::fromHex(0xB89B62), Color::fromHex(0x6E5A36), Color::fromHex(0x9A804C),
                                                 Color::fromHex(0x4A3C24), Color::fromHex(0x78643C), Color::fromHex(0x2B2214));
            } else if (style == HardwareKnobStyle::BakeliteMaster) {
                // Vintage brown-black scalloped Bakelite skirt + 8 grip
                // lobes
                g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                 Color::fromHex(0x3D2B22), Color::fromHex(0x241812), Color::fromHex(0x32221A),
                                                 Color::fromHex(0x160E0A), Color::fromHex(0x281B14), Color::fromHex(0x0B0705));
                for (int lobe = 0; lobe < 8; ++lobe) {
                    const float la = angle + static_cast<float>(lobe) * (kPi * 0.25f);
                    const Point<float> lp{cx + (R * 0.84f) * std::sin(la), cy - (R * 0.84f) * std::cos(la)};
                    g.fillCircle(lp, R * 0.19f, Color::fromHex(0x2A1C15));
                    g.drawCircle(lp, R * 0.19f, Color(1.0f, 0.88f, 0.75f, 0.16f), 0.9f);
                }
            } else if (style == HardwareKnobStyle::KnurledBlackBarrel || style == HardwareKnobStyle::ModernArc ||
                       style == HardwareKnobStyle::MacroHalo || style == HardwareKnobStyle::FlatVector ||
                       style == HardwareKnobStyle::ColorCapConsole) {
                // Anodized dark gunmetal / slate outer body
                g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                 Color::fromHex(0x3C444D), Color::fromHex(0x22272E), Color::fromHex(0x303740),
                                                 Color::fromHex(0x161A1F), Color::fromHex(0x262C34), Color::fromHex(0x0D1014));
            } else if (style == HardwareKnobStyle::ColletBroadcast) {
                // European broadcast matte pewter/slate collet skirt
                g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                 Color::fromHex(0x9CA6B0), Color::fromHex(0x68727C), Color::fromHex(0x88929C),
                                                 Color::fromHex(0x4E565E), Color::fromHex(0x747E88), Color::fromHex(0x363C42));
            } else {
                // Universal 6-stop polished metallic chrome skirt
                // (#F4F4F2 -> #AEB1B4 -> #F0F0EE -> #85888C -> #C9CBCD ->
                // #4F5154)
                g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                 Color::fromHex(0xF4F4F2), Color::fromHex(0xAEB1B4), Color::fromHex(0xF0F0EE),
                                                 Color::fromHex(0x85888C), Color::fromHex(0xC9CBCD), Color::fromHex(0x4F5154));
            }

            // Style-specific skirt detailing (Knurls, Starburst flutes,
            // Detent balls, or LED Halo Arcs)
            const float degToRad = 0.0174532925f;
            if (style == HardwareKnobStyle::ModernArc) {
                // LED ARC KNOB: Recessed dark LED track + vivid glowing
                // value arc + 15 LED segments
                const float rArc = R * 0.87f;
                g.drawArc(center, rArc, startAngle, endAngle, Color::fromHex(0x080C10, 0.92f), 4.2f * uiScale);
                if (clampedNorm > 0.008f) {
                    g.drawArc(center, rArc, startAngle, angle, warmAmberTick.withAlpha(0.32f), 6.8f * uiScale);
                    g.drawArc(center, rArc, startAngle, angle, warmAmberTick, 3.4f * uiScale);
                }
                constexpr int kArcLeds = 16;
                for (int s = 0; s <= kArcLeds; ++s) {
                    const float st = static_cast<float>(s) / static_cast<float>(kArcLeds);
                    const float sa = startAngle + st * (endAngle - startAngle);
                    const Point<float> lp{cx + rArc * std::sin(sa), cy - rArc * std::cos(sa)};
                    const bool lit = (st <= clampedNorm + 0.01f);
                    g.fillCircle(lp, (lit ? 1.55f : 1.05f) * uiScale, lit ? Color::fromHex(0xFFF6E0) : Color::fromHex(0x1C2630));
                }
            } else if (style == HardwareKnobStyle::MacroHalo) {
                // MACRO HALO KNOB: Dual-ring cyan outer modulation halo +
                // inner accent value arc
                const Color cyanHalo = Color::fromHex(0x00F5D4);
                const float rOuterHalo = R * 0.93f;
                const float rInnerHalo = R * 0.81f;
                g.drawArc(center, rOuterHalo, startAngle, endAngle, Color::fromHex(0x061014, 0.90f), 2.6f * uiScale);
                g.drawArc(center, rOuterHalo, startAngle, angle, cyanHalo.withAlpha(0.92f), 2.4f * uiScale);
                g.drawArc(center, rInnerHalo, startAngle, endAngle, Color::fromHex(0x0A0E12, 0.90f), 2.6f * uiScale);
                g.drawArc(center, rInnerHalo, startAngle, angle, warmAmberTick.withAlpha(0.95f), 2.4f * uiScale);
                for (int d = 0; d < 12; ++d) {
                    const float dt = static_cast<float>(d) / 11.0f;
                    const float da = startAngle + dt * (endAngle - startAngle);
                    const Point<float> dp{cx + rOuterHalo * std::sin(da), cy - rOuterHalo * std::cos(da)};
                    if (dt <= clampedNorm + 0.01f) { g.fillCircle(dp, 1.3f * uiScale, cyanHalo); }
                }
            } else if (style == HardwareKnobStyle::FlatVector) {
                // FLAT VECTOR KNOB: Clean dark vector disc + crisp outer
                // value arc
                g.fillCircle(center, R * 0.96f, Color::fromHex(0x141920));
                g.drawCircle(center, R * 0.95f, Color::fromHex(0x2A3440), 1.2f);
                g.drawArc(center, R * 0.88f, startAngle, endAngle, Color::fromHex(0x0A0E13), 3.0f * uiScale);
                if (clampedNorm > 0.008f) { g.drawArc(center, R * 0.88f, startAngle, angle, warmAmberTick, 3.0f * uiScale); }
            } else if (style == HardwareKnobStyle::StarburstSynth) {
                // STARBURST SYNTH: 12 deep dark scalloped notches around
                // the perimeter
                for (int f = 0; f < 12; ++f) {
                    const float fa = angle + static_cast<float>(f * 30) * degToRad;
                    const float fs = std::sin(fa), fc = std::cos(fa);
                    g.drawLine({cx + (R * 0.64f) * fs, cy - (R * 0.64f) * fc}, {cx + (R * 0.98f) * fs, cy - (R * 0.98f) * fc},
                               Color::fromHex(0x101418, 0.88f), 2.8f * uiScale);
                    const float fa2 = fa + 15.0f * degToRad;
                    g.drawLine({cx + (R * 0.68f) * std::sin(fa2), cy - (R * 0.68f) * std::cos(fa2)},
                               {cx + (R * 0.97f) * std::sin(fa2), cy - (R * 0.97f) * std::cos(fa2)}, Color(1.0f, 1.0f, 1.0f, 0.55f),
                               1.4f * uiScale);
                }
            } else if (style == HardwareKnobStyle::SteppedAttenuator) {
                // 24-STEP ATTENUATOR: 12 chrome detent ball bearings
                // around the collar
                for (int b = 0; b < 12; ++b) {
                    const float ba = static_cast<float>(b * 30) * degToRad;
                    const Point<float> bp{cx + (R * 0.89f) * std::sin(ba), cy - (R * 0.89f) * std::cos(ba)};
                    g.fillCircle(bp, 2.1f * uiScale, Color::fromHex(0x161A1E));
                    g.fillCircle({bp.x - 0.3f, bp.y - 0.3f}, 1.5f * uiScale, Color::fromHex(0xE8EEF2));
                }
            } else if (style != HardwareKnobStyle::BakeliteMaster) {
                // 72 Radial Knurl Lines around the skirt rim (deeper &
                // wider for KnurledBlackBarrel)
                const int kKnurls = (style == HardwareKnobStyle::KnurledBlackBarrel) ? 36 : 72;
                const float stepDeg = 360.0f / static_cast<float>(kKnurls);
                const float rKnurlIn = (style == HardwareKnobStyle::KnurledBlackBarrel) ? (R * 0.76f) : (R * 0.86f);
                const float rKnurlOut = R * 0.99f;
                const Color knurlDarkCol(0.0f, 0.0f, 0.0f, (style == HardwareKnobStyle::KnurledBlackBarrel) ? 0.65f : 0.24f);
                const Color knurlSpecCol(1.0f, 1.0f, 1.0f, (style == HardwareKnobStyle::KnurledBlackBarrel) ? 0.28f : 0.22f);
                for (int i = 0; i < kKnurls; ++i) {
                    const float aDark = angle + static_cast<float>(i) * stepDeg * degToRad;
                    const float aSpec = aDark + (stepDeg * 0.5f) * degToRad;
                    const float sd = std::sin(aDark), cd = std::cos(aDark);
                    const float ss = std::sin(aSpec), cs = std::cos(aSpec);
                    g.drawLine({cx + rKnurlIn * sd, cy - rKnurlIn * cd}, {cx + rKnurlOut * sd, cy - rKnurlOut * cd}, knurlDarkCol,
                               (style == HardwareKnobStyle::KnurledBlackBarrel) ? 1.6f : 1.1f);
                    g.drawLine({cx + rKnurlIn * ss, cy - rKnurlIn * cs}, {cx + rKnurlOut * ss, cy - rKnurlOut * cs}, knurlSpecCol, 0.9f);
                }
            }

            // 1px dark outer border and inner step highlight
            g.drawCircle(center, R, Color(0.0f, 0.0f, 0.0f, 0.65f), 1.0f);
            if (style != HardwareKnobStyle::FlatVector && style != HardwareKnobStyle::ModernArc && style != HardwareKnobStyle::MacroHalo) {
                g.drawCircle(center, R * 0.85f, Color(1.0f, 1.0f, 1.0f, 0.34f), 0.8f);
            }

            const bool effOuterRing = hasOuterRing || (style == HardwareKnobStyle::ConcentricDual);
            if (effOuterRing) {
                // CONCENTRIC KNOB: Dedicated bronze/gunmetal outer ring
                // with its own amber index pointer
                g.drawCircle(center, R * 0.74f, Color::fromHex(0x0E1216, 0.88f), 2.0f);
                const float outerAngle = startAngle + outerNorm * (endAngle - startAngle);
                g.drawLine({cx + (R * 0.62f) * std::sin(outerAngle), cy - (R * 0.62f) * std::cos(outerAngle)},
                           {cx + (R * 0.97f) * std::sin(outerAngle), cy - (R * 0.97f) * std::cos(outerAngle)}, warmAmberTick, 2.6f);
            }

            // ====================================================================
            // LAYER 3 & 4: 3D INNER CAP & STYLE-SPECIFIC SCULPTED
            // SILHOUETTE
            // ====================================================================
            const float R_cap = effOuterRing ? (R * 0.56f) : ((style == HardwareKnobStyle::ChickenHead) ? (R * 0.58f) : (R * 0.70f));

            // Cap cast shadow onto the skirt (+0.03R, +0.07R)
            if (style != HardwareKnobStyle::FlatVector) {
                g.fillCircle({cx + R * 0.03f, cy + R * 0.07f}, R_cap * 1.04f, Color{0.0f, 0.0f, 0.0f, 0.55f});
            }

            const bool isCrimsonCap = (style == HardwareKnobStyle::MarconiRed) ||
                                      (useCustomKnobColors && ((capColorHex >> 16) & 0xFF) > 0x90 && ((capColorHex >> 8) & 0xFF) < 0x58);

            Color capTop = Color::fromHex(0x434445);
            Color capMid = Color::fromHex(0x1E2022);
            Color capBot = Color::fromHex(0x060607);
            if (isCrimsonCap) {
                capTop = Color::fromHex(0xD44B4B);
                capMid = Color::fromHex(0x8E2222);
                capBot = Color::fromHex(0x3E0E0E);
            } else if (style == HardwareKnobStyle::GoldDome) {
                capTop = Color::fromHex(0xFCE694);
                capMid = Color::fromHex(0xC2922C);
                capBot = Color::fromHex(0x52380A);
            } else if (style == HardwareKnobStyle::StarburstSynth) {
                // Bright spun-aluminum silver center disc
                capTop = Color::fromHex(0xF4F8FC);
                capMid = Color::fromHex(0xB8C2CC);
                capBot = Color::fromHex(0x68727C);
            } else if (style == HardwareKnobStyle::BakeliteMaster) {
                capTop = Color::fromHex(0x4A3328);
                capMid = Color::fromHex(0x261812);
                capBot = Color::fromHex(0x0E0806);
            } else if (style == HardwareKnobStyle::GrimyRelic) {
                capTop = Color::fromHex(0x4E4234);
                capMid = Color::fromHex(0x2A221A);
                capBot = Color::fromHex(0x120E0A);
            } else if (style == HardwareKnobStyle::ColletBroadcast) {
                capTop = Color::fromHex(0x5E6872);
                capMid = Color::fromHex(0x384048);
                capBot = Color::fromHex(0x1C2026);
            } else if (style == HardwareKnobStyle::FlatVector) {
                capTop = Color::fromHex(0x202832);
                capMid = Color::fromHex(0x181E26);
                capBot = Color::fromHex(0x10151B);
            } else if (useCustomKnobColors && capColorHex != 0x1E2226 && capColorHex != 0x141618 && capColorHex != 0x161A1E) {
                const uint32_t darkCapHex = lightChassis ? 0x141618u : capColorHex;
                capTop = HardwareMaterials::scaleColorHex(darkCapHex, 1.32f, 1.0f);
                capMid = HardwareMaterials::scaleColorHex(darkCapHex, 0.85f, 1.0f);
                capBot = HardwareMaterials::scaleColorHex(darkCapHex, 0.32f, 1.0f);
            }

            g.fillLinearGradientCircle3Stops(center, R_cap, {cx - R_cap * 0.7f, cy - R_cap * 0.85f},
                                             {cx + R_cap * 0.7f, cy + R_cap * 0.85f}, capTop, 0.50f, capMid, capBot);

            // Outer cap ring + top-edge specular stroke + dome
            // highlight
            if (hasCustomAccent && style != HardwareKnobStyle::ColorCapConsole && style != HardwareKnobStyle::ColletBroadcast &&
                style != HardwareKnobStyle::GoldDome && style != HardwareKnobStyle::MarconiRed) {
                g.fillRadialGradientCircle(center, R_cap * 0.92f, warmAmberTick.withAlpha(0.32f), warmAmberTick.withAlpha(0.06f));
            }
            g.drawCircle(center, R_cap,
                         (style == HardwareKnobStyle::FlatVector || (hasCustomAccent && style != HardwareKnobStyle::SkirtedCap))
                             ? warmAmberTick.withAlpha(hasCustomAccent ? 0.85f : 0.65f)
                             : Color(0.0f, 0.0f, 0.0f, 0.78f),
                         (hasCustomAccent && style != HardwareKnobStyle::SkirtedCap) ? 1.5f : 1.2f);
            if (style != HardwareKnobStyle::FlatVector) {
                g.drawLinearGradientCircle(center, R_cap - 0.6f, {cx - R_cap, cy - R_cap}, {cx + R_cap, cy + R_cap},
                                           Color(1.0f, 1.0f, 1.0f, 0.20f), Color(0.0f, 0.0f, 0.0f, 0.65f), 1.0f);
                g.fillRadialGradientCircle({cx - R_cap * 0.22f, cy - R_cap * 0.24f}, R_cap * 0.68f, Color(1.0f, 1.0f, 1.0f, 0.16f),
                                           Color(1.0f, 1.0f, 1.0f, 0.0f));
            }

            // Style-specific inner cap inserts (ColorCapConsole,
            // ColletBroadcast, SkirtedCap, MacroHalo, GrimyRelic)
            if (style == HardwareKnobStyle::ColorCapConsole) {
                // Colored anodized console cap insert (0.48 * R)
                const float rInsert = R * 0.48f;
                g.fillCircle(center, rInsert, warmAmberTick.withAlpha(0.88f));
                g.fillRadialGradientCircle({cx - rInsert * 0.25f, cy - rInsert * 0.25f}, rInsert * 0.85f, Color(1.0f, 1.0f, 1.0f, 0.30f),
                                           Color(0.0f, 0.0f, 0.0f, 0.25f));
                g.drawCircle(center, rInsert, Color(0.0f, 0.0f, 0.0f, 0.65f), 1.1f);
            } else if (style == HardwareKnobStyle::ColletBroadcast) {
                // Inset colored broadcast collet cap nut (0.38 * R)
                const float rCollet = R * 0.38f;
                g.fillCircle(center, rCollet + 1.5f, Color::fromHex(0x101418));
                g.fillCircle(center, rCollet, warmAmberTick);
                g.drawCircle(center, rCollet * 0.55f, Color(0.0f, 0.0f, 0.0f, 0.35f), 0.9f);
            } else if (style == HardwareKnobStyle::SkirtedCap) {
                // Spun silver (or warm brass for 70s console) dish insert inside the skirted cap (0.38 * R)
                const float rDish = R * 0.38f;
                if (useCustomKnobColors && bodyColorHex == 0xD4AF37u) {
                    g.fillLinearGradientCircle3Stops(center, rDish, {cx - rDish, cy - rDish}, {cx + rDish, cy + rDish},
                                                     Color::fromHex(0xF8E8B8), 0.5f, Color::fromHex(0xC6A452), Color::fromHex(0x785A20));
                } else {
                    g.fillLinearGradientCircle3Stops(center, rDish, {cx - rDish, cy - rDish}, {cx + rDish, cy + rDish},
                                                     Color::fromHex(0xE8EEF2), 0.5f, Color::fromHex(0x9BA4AE), Color::fromHex(0x586068));
                }
                g.drawCircle(center, rDish, Color(0.0f, 0.0f, 0.0f, 0.55f), 1.0f);
            } else if (style == HardwareKnobStyle::MacroHalo) {
                // Glowing cyber hub ring in the center
                g.drawCircle(center, R * 0.26f, Color::fromHex(0x00F5D4, 0.80f), 1.4f);
                g.fillCircle(center, R * 0.12f, warmAmberTick);
            } else if (style == HardwareKnobStyle::GrimyRelic) {
                // Weathered brass center ring & patina scratch arcs
                g.drawCircle(center, R * 0.34f, Color::fromHex(0x9E824C, 0.55f), 1.2f);
                g.drawArc(center, R * 0.52f, -0.8f, 0.6f, Color::fromHex(0xC8A868, 0.32f), 1.1f);
            }

            // ====================================================================
            // LAYER 5 & 6: TAPERED BEAK / WING OR CYLINDRICAL POINTER
            // INLAY
            // ====================================================================
            const bool hasBeakRidge = (style == HardwareKnobStyle::SteppedAttenuator) || (style == HardwareKnobStyle::PointerSkirted) ||
                                      (style == HardwareKnobStyle::ChickenHead) || (style == HardwareKnobStyle::MarconiRed);

            const Color creamPtr =
                (style == HardwareKnobStyle::GoldDome || style == HardwareKnobStyle::StarburstSynth)
                    ? Color::fromHex(0x14181C)
                    : ((style == HardwareKnobStyle::ModernArc || style == HardwareKnobStyle::FlatVector)
                           ? warmAmberTick
                           : ((style == HardwareKnobStyle::MacroHalo)
                                  ? Color::fromHex(0x00F5D4)
                                  : ((style == HardwareKnobStyle::GrimyRelic) ? Color::fromHex(0xE8B85C) : Color::fromHex(0xEDEAE0))));

            if (hasBeakRidge) {
                // ChickenHead has an extra-long pointed beak extending
                // past the skirt; MarconiRed has a wide sculpted wing
                const float yNose = (style == HardwareKnobStyle::ChickenHead) ? (-R * 1.06f) : (-R * 0.92f);
                const float yTail = (style == HardwareKnobStyle::ChickenHead) ? (+R * 0.78f) : (+R * 0.68f);
                const float noseRadius = (style == HardwareKnobStyle::ChickenHead) ? (R * 0.062f) : (R * 0.088f);
                const float tailRadius = (style == HardwareKnobStyle::ChickenHead) ? (R * 0.195f) : (R * 0.155f);

                // Cast shadow on the skirt (+0.03R, +0.07R)
                {
                    const Point<float> shadowCenter{cx + R * 0.03f, cy + R * 0.07f};
                    g.pushRotation(angleDeg, shadowCenter);
                    g.fillTaperedFingerRidgeSolid(shadowCenter, yNose, yTail, noseRadius, tailRadius, Color{0.0f, 0.0f, 0.0f, 0.52f});
                    g.popRotation();
                }

                // Sculpted 3D beak body
                {
                    g.pushRotation(angleDeg, center);
                    const Color ridgeLitCol =
                        isCrimsonCap ? Color::fromHex(0xE05252)
                                     : ((style == HardwareKnobStyle::ChickenHead) ? Color::fromHex(0x382820) : Color::fromHex(0x434445));
                    const Color ridgeShadowCol =
                        isCrimsonCap ? Color::fromHex(0x4A1010)
                                     : ((style == HardwareKnobStyle::ChickenHead) ? Color::fromHex(0x0E0806) : Color::fromHex(0x090A0B));
                    g.fillAndStrokeTaperedFingerRidge(center, yNose, yTail, noseRadius, tailRadius, ridgeLitCol, ridgeShadowCol,
                                                      Color{1.0f, 1.0f, 1.0f, 0.18f}, 1.0f);
                    g.fillRadialGradientCircle(center, R * 0.24f, Color{1.0f, 1.0f, 1.0f, 0.16f}, Color{1.0f, 1.0f, 1.0f, 0.0f});
                    g.popRotation();
                }

                const float rPtrIn = R * 0.14f;
                const float rPtrOut = (style == HardwareKnobStyle::ChickenHead) ? (R * 0.98f) : (R * 0.85f);
                g.drawLine({cx + rPtrIn * ptrSin, cy - rPtrIn * ptrCos}, {cx + rPtrOut * ptrSin, cy - rPtrOut * ptrCos},
                           Color{0.0f, 0.0f, 0.0f, 0.72f}, 3.4f);
                g.drawLine({cx + rPtrIn * ptrSin, cy - rPtrIn * ptrCos}, {cx + rPtrOut * ptrSin, cy - rPtrOut * ptrCos}, creamPtr, 2.2f);
            } else {
                // Cylindrical / Dome / Halo / Collet / Starburst pointer
                // inlay
                const float rPtrIn = (style == HardwareKnobStyle::ColorCapConsole || style == HardwareKnobStyle::ColletBroadcast ||
                                      style == HardwareKnobStyle::SkirtedCap)
                                         ? (R * 0.36f)
                                         : (R * 0.16f);
                const float rPtrOut = (style == HardwareKnobStyle::KnurledBlackBarrel || style == HardwareKnobStyle::ColletBroadcast ||
                                       style == HardwareKnobStyle::BakeliteMaster)
                                          ? (R * 0.92f)
                                          : (R_cap * 0.92f);
                g.drawLine({cx + rPtrIn * ptrSin, cy - rPtrIn * ptrCos}, {cx + rPtrOut * ptrSin, cy - rPtrOut * ptrCos},
                           (style == HardwareKnobStyle::GoldDome || style == HardwareKnobStyle::StarburstSynth)
                               ? Color(1.0f, 1.0f, 1.0f, 0.55f)
                               : Color(0.0f, 0.0f, 0.0f, 0.72f),
                           3.4f);
                g.drawLine({cx + rPtrIn * ptrSin, cy - rPtrIn * ptrCos}, {cx + rPtrOut * ptrSin, cy - rPtrOut * ptrCos}, creamPtr, 2.3f);
                if (style == HardwareKnobStyle::ModernArc || style == HardwareKnobStyle::MacroHalo) {
                    g.fillCircle({cx + (rPtrOut - 1.5f) * ptrSin, cy - (rPtrOut - 1.5f) * ptrCos}, 2.0f * uiScale,
                                 Color::fromHex(0xFFF8E7));
                }
            }
        } else {
            // ====================================================================
            // BESPOKE DIE-CUT 32-BIT RGBA 3D DIALS FOR STYLES 17..32 (+
            // CLASSIC ROCK / NEW WAVE / EDM)
            // ====================================================================
            const float startRad = -2.3561945f;
            const float endRad = startRad + math::clamp(norm, 0.0f, 1.0f) * 4.712389f;
            const float angleRad = angle;
            // Cleanly separate Stage Component Knobs (Style A) from Metering Output Knobs (OUTPUT TRIM & MASTER MIX -> Style B)
            const bool isMeteringBayKnob =
                (title == "MASTER MIX" || title == "DRY / WET") ||
                (title == "OUTPUT TRIM" && firstIsOrange);
            const int colVar = isMeteringBayKnob ? 1 : 0;
            const bool isBotRow = isMeteringBayKnob;

            auto drawKnobSprite = [&](std::string_view spriteFile, float radiusScale, bool rotateWithKnob, Color ptrCol,
                                      float ptrInFrac = 0.35f, float ptrOutFrac = 0.92f, bool drawGlowArc = false,
                                      Color arcCol = Color::fromHex(0x00E5FF)) -> bool {
                const float sprR = R * radiusScale;
                const Rect<float> sprRect = {cx - sprR, cy - sprR, sprR * 2.0f, sprR * 2.0f};
                bool ok = false;
                if (rotateWithKnob) {
                    g.pushRotation(angleDeg, center);
                    ok = g.drawThemeBackdrop(spriteFile, sprRect, 1.0f);
                    g.popRotation();
                } else {
                    ok = g.drawThemeBackdrop(spriteFile, sprRect, 1.0f);
                }
                if (!ok) return false;
                if (drawGlowArc) {
                    g.drawArc(center, R * 0.94f, startRad, endRad, arcCol.withAlpha(0.38f), 6.0f);
                    g.drawArc(center, R * 0.94f, startRad, endRad, arcCol, 2.5f);
                }
                g.drawLine({cx + R * ptrInFrac * ptrSin + 0.8f, cy - R * ptrInFrac * ptrCos + 0.8f},
                           {cx + R * ptrOutFrac * ptrSin + 0.8f, cy - R * ptrOutFrac * ptrCos + 0.8f}, Color(0.0f, 0.0f, 0.0f, 0.72f),
                           3.6f);
                g.drawLine({cx + R * ptrInFrac * ptrSin, cy - R * ptrInFrac * ptrCos},
                           {cx + R * ptrOutFrac * ptrSin, cy - R * ptrOutFrac * ptrCos}, ptrCol, 2.4f);
                return true;
            };

            if (isClassicRockGoldAmp) {
                if (drawKnobSprite("knob_09.png", 1.18f, true, Color::fromHex(0xFFF8D6), 0.22f, 0.92f)) {
                    // Rendered 3D Gold-Anodized Brushed Pointer Amp Dial
                }
            } else if (isNeonSynthOrEdmArc) {
                const Color arcC = (colVar & 1) ? Color::fromHex(0x00F5D4) : Color::fromHex(0xFF1E8E);
                if (drawKnobSprite("knob_24.png", 1.18f, false, Color::fromHex(0xFFFFFF), 0.22f, 0.86f, true, arcC)) {
                    // Rendered 3D Neon Magenta/Cyan Synthwave Brushed Dial
                }
            } else
                switch (style) {
                case HardwareKnobStyle::CyberLuxeHalo: {
                    if (drawKnobSprite("knob_15.png", 1.18f, false, Color::fromHex(0xE8FCFF), 0.22f, 0.84f, true, accentCol)) break;
                    // Dark brushed carbon-anodized outer collar + glowing
                    // cyan LED ring + recessed obsidian cap
                    g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                     Color::fromHex(0x36404C), Color::fromHex(0x1C222A), Color::fromHex(0x2B333E),
                                                     Color::fromHex(0x11151B), Color::fromHex(0x232A34), Color::fromHex(0x080B0E));
                    for (int i = 0; i < 48; ++i) {
                        const float a = angleRad + static_cast<float>(i) * (6.2831853f / 48.0f);
                        g.drawLine({cx + std::cos(a) * R * 0.84f, cy + std::sin(a) * R * 0.84f},
                                   {cx + std::cos(a) * R * 0.98f, cy + std::sin(a) * R * 0.98f}, Color::fromHex(0x05070A, 0.72f), 1.1f);
                    }
                    g.drawCircle(center, R, Color::fromHex(0x040608), 1.4f);
                    // Recessed glowing cyan LED halo track inside the rim
                    const float rHalo = R * 0.81f;
                    g.drawArc(center, rHalo, startRad, startRad + 4.712389f, Color::fromHex(0x08131A), 4.0f);
                    g.drawArc(center, rHalo, startRad, endRad, accentCol.withAlpha(0.36f), 6.8f);
                    g.drawArc(center, rHalo, startRad, endRad, accentCol, 2.8f);
                    // Sculpted 3D brushed gunmetal cap
                    g.fillCircle({cx + 1.5f, cy + 2.5f}, R * 0.68f, Color::fromHex(0x000000, 0.65f));
                    g.fillRadialGradientCircle(center, R * 0.66f, Color::fromHex(0x2C3540), Color::fromHex(0x0E1318), {-3.0f, -3.5f});
                    g.drawCircle(center, R * 0.66f, Color::fromHex(0x4A5868, 0.65f), 1.1f);
                    g.drawLine({cx + R * 0.18f * ptrSin, cy - R * 0.18f * ptrCos}, {cx + R * 0.78f * ptrSin, cy - R * 0.78f * ptrCos},
                               accentCol.withAlpha(0.40f), 4.8f);
                    g.drawLine({cx + R * 0.18f * ptrSin, cy - R * 0.18f * ptrCos}, {cx + R * 0.78f * ptrSin, cy - R * 0.78f * ptrCos},
                               Color::fromHex(0xE8FCFF), 2.3f);
                    break;
                }
                case HardwareKnobStyle::NeumorphicGlass: {
                    // Soft-extruded ceramic/frosted-glass neumorphic disc:
                    // Warm Amber ring for Stage Component knobs, Sky-Blue ring for Metering Output knobs
                    const Color ringCol = hasCustomAccent
                                              ? warmAmberTick
                                              : (isMeteringBayKnob ? Color::fromHex(0x38BDF8) : Color::fromHex(0xF59E42));
                    if (drawKnobSprite("knob_23.png", 1.18f, false, ringCol, 0.20f, 0.78f, true, ringCol)) break;
                    g.fillCircle({cx + 3.5f, cy + 4.0f}, R * 0.98f, Color::fromHex(0x64748B, 0.42f));
                    g.fillCircle({cx - 2.5f, cy - 2.5f}, R * 0.98f, Color::fromHex(0xFFFFFF, 0.90f));
                    g.fillRadialGradientCircle(center, R * 0.95f, Color::fromHex(0xF1F5F9), Color::fromHex(0xCBD5E1), {-3.5f, -3.5f});
                    g.drawCircle(center, R * 0.95f, Color::fromHex(0xFFFFFF, 0.95f), 1.4f);

                    // Luminous neon outer ring + soft bloom
                    g.drawArc(center, R * 0.96f, startRad, endRad, ringCol.withAlpha(0.32f), 6.5f);
                    g.drawArc(center, R * 0.96f, startRad, endRad, ringCol, 2.5f);

                    // Inner raised frosted glass cap
                    g.fillCircle({cx + 2.0f, cy + 2.5f}, R * 0.68f, Color::fromHex(0x64748B, 0.35f));
                    g.fillRadialGradientCircle(center, R * 0.66f, Color::fromHex(0xFFFFFF), Color::fromHex(0xD8E2EC), {-2.5f, -2.5f});
                    g.drawCircle(center, R * 0.66f, Color::fromHex(0xFFFFFF, 0.92f), 1.2f);
                    g.drawLine({cx + R * 0.16f * ptrSin, cy - R * 0.16f * ptrCos}, {cx + R * 0.62f * ptrSin, cy - R * 0.62f * ptrCos},
                               ringCol, 2.5f);
                    break;
                }
                case HardwareKnobStyle::SciFiHudRing: {
                    // Cyan HUD ring for Stage Component knobs, Neon-Emerald HUD ring for Metering Output knobs
                    const Color hudCol = hasCustomAccent
                                             ? warmAmberTick
                                             : (isMeteringBayKnob ? Color::fromHex(0x22C55E) : Color::fromHex(0x00F5D4));
                    if (drawKnobSprite("knob_15.png", 1.18f, false, Color::fromHex(0xFFFFFF), 0.22f, 0.88f, true, hudCol)) break;
                    g.fillRadialGradientCircle(center, R, Color::fromHex(0x07222C), Color::fromHex(0x020A0E));
                    g.drawCircle(center, R * 0.98f, hudCol.withAlpha(0.55f), 1.4f);
                    // Segmented outer telemetry ring
                    g.drawArc(center, R * 0.88f, startRad, startRad + 4.712389f, Color::fromHex(0x092834), 3.4f);
                    g.drawArc(center, R * 0.88f, startRad, endRad, hudCol.withAlpha(0.38f), 6.5f);
                    g.drawArc(center, R * 0.88f, startRad, endRad, hudCol, 3.0f);
                    // Inner counter-rotating reticle ticks
                    for (int i = 0; i < 16; ++i) {
                        const float a = -angleRad * 0.5f + static_cast<float>(i) * (6.2831853f / 16.0f);
                        g.drawLine({cx + std::cos(a) * R * 0.56f, cy + std::sin(a) * R * 0.56f},
                                   {cx + std::cos(a) * R * 0.68f, cy + std::sin(a) * R * 0.68f}, hudCol.withAlpha(0.65f),
                                   (i % 4 == 0) ? 1.8f : 1.0f);
                    }
                    // Glowing plasma reactor core
                    g.fillRadialGradientCircle(center, R * 0.42f, Color::fromHex(0xE0FFFF), hudCol.withAlpha(0.15f));
                    g.drawCircle(center, R * 0.42f, hudCol, 1.5f);
                    g.drawLine({cx + R * 0.18f * ptrSin, cy - R * 0.18f * ptrCos}, {cx + R * 0.94f * ptrSin, cy - R * 0.94f * ptrCos},
                               Color::fromHex(0xFFFFFF), 2.3f);
                    break;
                }
                case HardwareKnobStyle::ValveWheelRelic: {
                    // RETRO DISTRESSED: Rusted industrial iron dials for Stage Components, Red 4-spoke steam valve wheel for Metering Output
                    if (drawKnobSprite(isMeteringBayKnob ? "knob_10.png" : "knob_04.png", 1.20f, true, Color::fromHex(0xF5B800),
                                       0.28f, 0.92f))
                        break;
                    if (isMeteringBayKnob) {
                        // Iconic 5-spoke red cast-iron steam valve wheel
                        g.fillCircle(center, R * 0.96f, Color::fromHex(0x181412, 0.85f));
                        for (int i = 0; i < 5; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 5.0f);
                            g.drawLine(center, {cx + std::sin(a) * R * 0.88f, cy - std::cos(a) * R * 0.88f}, Color::fromHex(0x8C221C),
                                       4.2f);
                        }
                        g.drawCircle(center, R * 0.88f, Color::fromHex(0xA82B24), 5.5f);
                        g.drawCircle(center, R * 0.88f, Color::fromHex(0xD94E34, 0.55f), 1.6f);
                        g.fillRadialGradientCircle(center, R * 0.28f, Color::fromHex(0x9E8C7A), Color::fromHex(0x3D3228));
                        g.fillCircle({cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos}, 2.8f, Color::fromHex(0xF5B800));
                    } else {
                        // Corroded heavy cast-iron industrial turret dial
                        // with oxidized brass face
                        g.fillRadialGradientCircle(center, R, Color::fromHex(0x5C4838), Color::fromHex(0x211A14));
                        for (int i = 0; i < 16; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 16.0f);
                            g.drawLine({cx + std::cos(a) * R * 0.80f, cy + std::sin(a) * R * 0.80f},
                                       {cx + std::cos(a) * R * 0.98f, cy + std::sin(a) * R * 0.98f}, Color::fromHex(0x16110D), 2.0f);
                        }
                        g.drawCircle(center, R * 0.98f, Color::fromHex(0x140F0C), 1.5f);
                        g.fillRadialGradientCircle(center, R * 0.66f, (colVar == 1) ? Color::fromHex(0xC8B294) : Color::fromHex(0x4A3B2E),
                                                   (colVar == 1) ? Color::fromHex(0x6E5840) : Color::fromHex(0x1F1812));
                        g.drawCircle(center, R * 0.66f, Color::fromHex(0x140F0C), 1.6f);
                        g.drawLine({cx + R * 0.15f * ptrSin, cy - R * 0.15f * ptrCos}, {cx + R * 0.86f * ptrSin, cy - R * 0.86f * ptrCos},
                                   Color::fromHex(0xF5B800), 2.6f);
                    }
                    break;
                }
                case HardwareKnobStyle::SubmarineHelm: {
                    // UNDERWATER / SUBMARINE: Ship Helm Wheel for Stage Component knobs, Brass Porthole Dial for Metering Output knobs
                    if (!isMeteringBayKnob) {
                        if (drawKnobSprite("knob_11.png", 1.24f, true, Color::fromHex(0xFFF8D6), 0.25f, 0.96f)) break;
                    } else {
                        if (drawKnobSprite("knob_12.png", 1.18f, false, Color::fromHex(0x38F8FF), 0.54f, 0.95f)) break;
                    }
                    if (!isMeteringBayKnob) {
                        // 8-spoke naval brass ship's helm wheel with turned
                        // handles extending to R*1.08
                        for (int i = 0; i < 8; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 8.0f);
                            g.drawLine(center, {cx + std::sin(a) * R * 1.06f, cy - std::cos(a) * R * 1.06f}, Color::fromHex(0x9C7222),
                                       3.4f);
                            g.fillCircle({cx + std::sin(a) * R * 1.06f, cy - std::cos(a) * R * 1.06f}, 2.6f, Color::fromHex(0xE8C568));
                        }
                        g.drawCircle(center, R * 0.78f, Color::fromHex(0x8C6218), 5.2f);
                        g.drawCircle(center, R * 0.78f, Color::fromHex(0xF4D57B, 0.75f), 1.4f);
                        g.fillRadialGradientCircle(center, R * 0.30f, Color::fromHex(0xF4D57B), Color::fromHex(0x5C3E0A));
                        g.drawLine(center, {cx + R * 0.96f * ptrSin, cy - R * 0.96f * ptrCos}, Color::fromHex(0xFFF8D6), 2.4f);
                    } else {
                        // Riveted bronze porthole dial with glowing
                        // bioluminescent marine life inside
                        g.fillRadialGradientCircle(center, R, Color::fromHex(0xD4A848), Color::fromHex(0x4A330B), {-3.0f, -3.0f});
                        g.drawCircle(center, R, Color::fromHex(0x1C1304), 1.6f);
                        for (int i = 0; i < 8; ++i) {
                            const float a = static_cast<float>(i) * (6.2831853f / 8.0f);
                            g.fillCircle({cx + std::cos(a) * R * 0.87f, cy + std::sin(a) * R * 0.87f}, 1.6f, Color::fromHex(0xFFF0AD));
                        }
                        g.fillRadialGradientCircle(center, R * 0.74f, Color::fromHex(0x106878), Color::fromHex(0x041F28));
                        g.drawCircle(center, R * 0.74f, Color::fromHex(0x021218), 1.8f);
                        if (colVar == 0) {
                            // Glowing cyan fish silhouette
                            g.fillCircle({cx - R * 0.06f, cy}, R * 0.24f, Color::fromHex(0x38F8FF, 0.85f));
                            std::array<Point<float>, 3> tail = {
                                {{cx + R * 0.12f, cy}, {cx + R * 0.42f, cy - R * 0.24f}, {cx + R * 0.42f, cy + R * 0.24f}}};
                            g.fillPolygon(tail, Color::fromHex(0x38F8FF, 0.85f));
                            g.fillCircle({cx - R * 0.16f, cy - R * 0.05f}, 1.8f, Color::fromHex(0x041F28));
                        } else if (colVar == 1) {
                            // Glowing cyan jellyfish silhouette
                            g.drawArc({cx, cy - R * 0.06f}, R * 0.28f, -1.57f, 1.57f, Color::fromHex(0x64F8FF, 0.90f), 5.0f);
                            for (int t = -2; t <= 2; ++t) {
                                const float tx = cx + static_cast<float>(t) * R * 0.10f;
                                g.drawLine({tx, cy - R * 0.02f}, {tx + static_cast<float>(t) * 2.0f, cy + R * 0.38f},
                                           Color::fromHex(0x64F8FF, 0.75f), 1.4f);
                            }
                        } else {
                            // Sonar sweep rings
                            g.drawCircle(center, R * 0.46f, Color::fromHex(0x38F888, 0.55f), 1.1f);
                            g.drawCircle(center, R * 0.24f, Color::fromHex(0x38F888, 0.55f), 1.1f);
                        }
                        g.drawLine({cx + R * 0.45f * ptrSin, cy - R * 0.45f * ptrCos}, {cx + R * 0.94f * ptrSin, cy - R * 0.94f * ptrCos},
                                   Color::fromHex(0xFFF3B0), 2.5f);
                    }
                    break;
                }
                case HardwareKnobStyle::TattooFlash: {
                    // TATTOO ART: Sacred Ruby Heart for Stage Component knobs, Iron Tattoo Machine for Metering Output knobs
                    if (!isMeteringBayKnob) {
                        if (drawKnobSprite("knob_13.png", 1.18f, false, Color::fromHex(0xFFF3B0), 0.48f, 0.94f)) break;
                    } else {
                        if (drawKnobSprite("knob_14.png", 1.18f, false, Color::fromHex(0xD4AF37), 0.46f, 0.94f)) break;
                    }
                    if (isBotRow) {
                        // Sculpted 3D Traditional Flash Skull with glowing
                        // ruby eyes
                        g.fillRadialGradientCircle({cx, cy - R * 0.08f}, R * 0.78f, Color::fromHex(0xF8F4E8), Color::fromHex(0x786E60),
                                                   {-3.0f, -3.0f});
                        g.fillRoundedRect({cx - R * 0.36f, cy + R * 0.20f, R * 0.72f, R * 0.44f}, 4.0f, Color::fromHex(0xD8CFC0));
                        g.drawCircle({cx, cy - R * 0.08f}, R * 0.78f, Color::fromHex(0x1C1410), 2.2f);
                        // Glowing crimson eye sockets
                        g.fillCircle({cx - R * 0.26f, cy - R * 0.06f}, R * 0.18f, Color::fromHex(0x1C0808));
                        g.fillCircle({cx + R * 0.26f, cy - R * 0.06f}, R * 0.18f, Color::fromHex(0x1C0808));
                        g.fillCircle({cx - R * 0.26f, cy - R * 0.06f}, R * 0.10f, Color::fromHex(0xE11D48));
                        g.fillCircle({cx + R * 0.26f, cy - R * 0.06f}, R * 0.10f, Color::fromHex(0xE11D48));
                        // Teeth verticals
                        for (int t = -2; t <= 2; ++t) {
                            const float tx = cx + static_cast<float>(t) * R * 0.12f;
                            g.drawLine({tx, cy + R * 0.24f}, {tx, cy + R * 0.56f}, Color::fromHex(0x1C1410), 1.4f);
                        }
                        g.drawLine({cx + R * 0.48f * ptrSin, cy - R * 0.48f * ptrCos}, {cx + R * 0.92f * ptrSin, cy - R * 0.92f * ptrCos},
                                   Color::fromHex(0xE11D48), 2.8f);
                    } else if (colVar == 2 || colVar == 3) {
                        // 3D Sculpted Sacred Heart (Ruby Red for col 2,
                        // Polished Silver for col 3)
                        const Color hTop = (colVar == 2) ? Color::fromHex(0xFF4D6D) : Color::fromHex(0xF8FAFC);
                        const Color hBot = (colVar == 2) ? Color::fromHex(0x800F2F) : Color::fromHex(0x475569);
                        const Color rimC = (colVar == 2) ? Color::fromHex(0xD4AF37) : Color::fromHex(0x1E293B);
                        g.fillRadialGradientCircle({cx - R * 0.34f, cy - R * 0.20f}, R * 0.46f, hTop, hBot);
                        g.fillRadialGradientCircle({cx + R * 0.34f, cy - R * 0.20f}, R * 0.46f, hTop, hBot);
                        std::array<Point<float>, 3> hTri = {
                            {{cx - R * 0.76f, cy - R * 0.04f}, {cx + R * 0.76f, cy - R * 0.04f}, {cx, cy + R * 0.82f}}};
                        g.fillPolygon(hTri, hBot);
                        g.drawCircle({cx - R * 0.34f, cy - R * 0.20f}, R * 0.46f, rimC, 2.0f);
                        g.drawCircle({cx + R * 0.34f, cy - R * 0.20f}, R * 0.46f, rimC, 2.0f);
                        g.fillCircle({cx - R * 0.42f, cy - R * 0.32f}, R * 0.14f, Color::fromHex(0xFFFFFF, 0.65f));
                        g.drawLine(center, {cx + R * 0.85f * ptrSin, cy - R * 0.85f * ptrCos}, Color::fromHex(0xFFF3B0), 2.8f);
                    } else {
                        // Heavy forged cast-iron tattoo machine / winged iron
                        // wheel dial
                        if (colVar == 0) {
                            g.fillRoundedRect({cx - R * 1.22f, cy - R * 0.22f, R * 2.44f, R * 0.44f}, 5.0f, Color::fromHex(0x2B2623));
                            g.drawRoundedRect({cx - R * 1.22f, cy - R * 0.22f, R * 2.44f, R * 0.44f}, 5.0f, Color::fromHex(0x5C5346), 1.5f);
                        }
                        g.fillRadialGradientCircle(center, R * 0.94f, Color::fromHex(0x524A42), Color::fromHex(0x181512));
                        g.drawCircle(center, R * 0.94f, Color::fromHex(0x0D0B09), 2.2f);
                        g.drawCircle(center, R * 0.62f, Color::fromHex(0x8C7D70), 2.0f);
                        g.fillCircle(center, R * 0.32f, Color::fromHex(0x2B2623));
                        g.drawLine({cx + R * 0.28f * ptrSin, cy - R * 0.28f * ptrCos}, {cx + R * 0.90f * ptrSin, cy - R * 0.90f * ptrCos},
                                   Color::fromHex(0xD4AF37), 2.8f);
                    }
                    break;
                }
                case HardwareKnobStyle::MetalSkull: {
                    // METAL: Dark forged iron dial with 12 steel rivets and
                    // 3D sculpted pewter skull center
                    if (drawKnobSprite("knob_08.png", 1.18f, false, accentCol, 0.52f, 0.96f)) break;
                    g.fillLinearGradientCircle6Stops(center, R, {cx - R * 0.75f, cy - R * 0.75f}, {cx + R * 0.75f, cy + R * 0.75f},
                                                     Color::fromHex(0x383E46), Color::fromHex(0x1A1D22), Color::fromHex(0x2C3138),
                                                     Color::fromHex(0x0E1013), Color::fromHex(0x22262C), Color::fromHex(0x07080A));
                    g.drawCircle(center, R, Color::fromHex(0x050608), 1.6f);
                    for (int i = 0; i < 12; ++i) {
                        const float a = angleRad + static_cast<float>(i) * (6.2831853f / 12.0f);
                        g.fillCircle({cx + std::cos(a) * R * 0.86f, cy + std::sin(a) * R * 0.86f}, 1.9f, Color::fromHex(0xCBD5E1));
                    }
                    g.fillCircle(center, R * 0.72f, Color::fromHex(0x0D0F12));
                    g.drawCircle(center, R * 0.72f, Color::fromHex(0x475569), 1.2f);
                    // 3D Pewter Skull Relief (cranium + jawbone + eye
                    // sockets + nasal cavity)
                    g.fillRadialGradientCircle({cx, cy - R * 0.10f}, R * 0.42f, Color::fromHex(0xF1F5F9), Color::fromHex(0x475569),
                                               {-2.0f, -2.5f});
                    g.fillRoundedRect({cx - R * 0.22f, cy + R * 0.14f, R * 0.44f, R * 0.28f}, 3.0f, Color::fromHex(0x94A3B8));
                    g.fillCircle({cx - R * 0.16f, cy - R * 0.08f}, R * 0.12f, Color::fromHex(0x090B0E));
                    g.fillCircle({cx + R * 0.16f, cy - R * 0.08f}, R * 0.12f, Color::fromHex(0x090B0E));
                    std::array<Point<float>, 3> nose = {
                        {{cx, cy + R * 0.02f}, {cx - R * 0.07f, cy + R * 0.15f}, {cx + R * 0.07f, cy + R * 0.15f}}};
                    g.fillPolygon(nose, Color::fromHex(0x090B0E));
                    g.drawLine({cx + R * 0.50f * ptrSin, cy - R * 0.50f * ptrCos}, {cx + R * 0.95f * ptrSin, cy - R * 0.95f * ptrCos},
                               accentCol, 2.8f);
                    break;
                }
                case HardwareKnobStyle::FlowerDaisy: {
                    // FLOWER POWER: White Daisy Petal rotary for Stage Component knobs, Pink Daisy Petal rotary for Metering Output knobs
                    if (drawKnobSprite(isMeteringBayKnob ? "knob_17.png" : "knob_16.png", 1.22f, true, Color::fromHex(0x4A154B),
                                       0.18f, 0.86f))
                        break;
                    for (int i = 0; i < 12; ++i) {
                        const float a = angleRad + static_cast<float>(i) * (6.2831853f / 12.0f);
                        const Point<float> pc = {cx + std::cos(a) * R * 0.68f, cy + std::sin(a) * R * 0.68f};
                        g.fillCircle(pc, R * 0.28f, (isBotRow && colVar == 1) ? Color::fromHex(0xF9A8D4) : Color::fromHex(0xFFFBEB));
                        g.drawCircle(pc, R * 0.28f, Color::fromHex(0x4A154B), 1.5f);
                    }
                    g.fillRadialGradientCircle(center, R * 0.52f, Color::fromHex(0xFDE047), Color::fromHex(0xEC4899), {-2.0f, -2.0f});
                    g.drawArc(center, R * 0.34f, angleRad, angleRad + 3.8f, Color::fromHex(0x38BDF8, 0.85f), 3.6f);
                    g.drawArc(center, R * 0.20f, angleRad + 2.0f, angleRad + 5.5f, Color::fromHex(0xF97316, 0.90f), 3.2f);
                    g.drawCircle(center, R * 0.52f, Color::fromHex(0x4A154B), 1.8f);
                    g.drawLine(center, {cx + R * 0.86f * ptrSin, cy - R * 0.86f * ptrCos}, Color::fromHex(0x4A154B), 2.8f);
                    break;
                }
                case HardwareKnobStyle::ReaperCrypt: {
                    // GRIM REAPER: Crypt Skull Dial for Stage Component knobs, Sculpted Bone Dial for Metering Output knobs
                    if (!isMeteringBayKnob) {
                        if (drawKnobSprite("knob_00.png", 1.18f, false, Color::fromHex(0xFFFFFF), 0.52f, 0.95f)) break;
                    } else {
                        if (drawKnobSprite("knob_02.png", 1.18f, true, Color::fromHex(0xFFFFFF), 0.42f, 0.95f)) break;
                    }
                    g.fillRadialGradientCircle(center, R, Color::fromHex(0x33423B), Color::fromHex(0x0D1411));
                    g.drawCircle(center, R, Color::fromHex(0x060A08), 1.8f);
                    g.drawCircle(center, R * 0.82f, Color::fromHex(0x38F888, 0.52f), 1.3f);
                    if (colVar == 0) {
                        // Crypt Skull relief
                        g.fillRadialGradientCircle({cx, cy - R * 0.08f}, R * 0.44f, Color::fromHex(0xE2E8CE), Color::fromHex(0x526358));
                        g.fillRoundedRect({cx - R * 0.22f, cy + R * 0.16f, R * 0.44f, R * 0.26f}, 3.0f, Color::fromHex(0x94A89B));
                        g.fillCircle({cx - R * 0.16f, cy - R * 0.08f}, R * 0.12f, Color::fromHex(0x08100C));
                        g.fillCircle({cx + R * 0.16f, cy - R * 0.08f}, R * 0.12f, Color::fromHex(0x08100C));
                    } else if (colVar == 1) {
                        // Brass & Ectoplasm Green Hourglass
                        g.fillRect({cx - R * 0.34f, cy - R * 0.46f, R * 0.68f, 3.5f}, Color::fromHex(0xB89B5E));
                        g.fillRect({cx - R * 0.34f, cy + R * 0.42f, R * 0.68f, 3.5f}, Color::fromHex(0xB89B5E));
                        std::array<Point<float>, 3> topG = {{{cx - R * 0.28f, cy - R * 0.42f}, {cx + R * 0.28f, cy - R * 0.42f}, {cx, cy}}};
                        std::array<Point<float>, 3> botG = {{{cx - R * 0.28f, cy + R * 0.42f}, {cx + R * 0.28f, cy + R * 0.42f}, {cx, cy}}};
                        g.fillPolygon(topG, Color::fromHex(0x38F888, 0.48f));
                        g.fillPolygon(botG, Color::fromHex(0x38F888, 0.85f));
                    } else if (colVar == 2) {
                        // Sculpted Anatomical Bone Joint
                        g.fillCircle({cx - R * 0.12f, cy - R * 0.36f}, R * 0.16f, Color::fromHex(0xE2E8D5));
                        g.fillCircle({cx + R * 0.12f, cy - R * 0.36f}, R * 0.16f, Color::fromHex(0xE2E8D5));
                        g.fillCircle({cx - R * 0.12f, cy + R * 0.36f}, R * 0.16f, Color::fromHex(0xCBD5C0));
                        g.fillCircle({cx + R * 0.12f, cy + R * 0.36f}, R * 0.16f, Color::fromHex(0xCBD5C0));
                        g.fillRoundedRect({cx - R * 0.14f, cy - R * 0.36f, R * 0.28f, R * 0.72f}, 4.0f, Color::fromHex(0xD8E0C8));
                    } else {
                        // Curved Scythe Blade across the dial
                        g.drawArc({cx - R * 0.10f, cy + R * 0.10f}, R * 0.56f, -2.2f, 0.4f, Color::fromHex(0xE2E8F0), 4.0f);
                    }
                    g.drawLine({cx + R * 0.38f * ptrSin, cy - R * 0.38f * ptrCos}, {cx + R * 0.94f * ptrSin, cy - R * 0.94f * ptrCos},
                               Color::fromHex(0x38F888), 2.6f);
                    break;
                }
                case HardwareKnobStyle::HellfireMagma: {
                    // HELL FIRE: Incandescent lava ring + cracked volcanic
                    // basalt crust + glowing magma core
                    if (drawKnobSprite("knob_03.png", 1.18f, true, Color::fromHex(0xFFFFFF), 0.25f, 0.94f, true, Color::fromHex(0xFFFFFF)))
                        break;
                    g.fillRadialGradientCircle(center, R, Color::fromHex(0x2B0C04), Color::fromHex(0x120402));
                    g.drawCircle(center, R * 0.92f, Color::fromHex(0xFF4500, 0.55f), 4.5f);
                    g.drawCircle(center, R * 0.92f, Color::fromHex(0xFFB703), 1.8f);
                    g.fillRadialGradientCircle(center, R * 0.76f, Color::fromHex(0xFF8800), Color::fromHex(0x260702));
                    // Dark volcanic rock plates with glowing orange lava
                    // fissures between them
                    for (int i = 0; i < 7; ++i) {
                        const float a = angleRad + static_cast<float>(i) * (6.2831853f / 7.0f);
                        const Point<float> p = {cx + std::cos(a) * R * 0.40f, cy + std::sin(a) * R * 0.40f};
                        g.fillCircle(p, R * 0.24f, Color::fromHex(0x1C0804));
                    }
                    for (int i = 0; i < 6; ++i) {
                        const float a = angleRad + static_cast<float>(i) * 1.0471975f;
                        g.drawLine(center, {cx + std::cos(a) * R * 0.74f, cy + std::sin(a) * R * 0.74f}, Color::fromHex(0xFFAA00), 1.9f);
                    }
                    g.fillCircle(center, R * 0.16f, Color::fromHex(0xFFF3B0));
                    g.drawLine(center, {cx + R * 0.92f * ptrSin, cy - R * 0.92f * ptrCos}, Color::fromHex(0xFFF8D6), 2.8f);
                    break;
                }
                case HardwareKnobStyle::ApocalypseCog: {
                    // APOCALYPSE: Rusted Iron Gear Cog for Stage Component knobs, Salvaged Bottle Cap for Metering Output knobs
                    if (!isMeteringBayKnob) {
                        if (drawKnobSprite("knob_04.png", 1.20f, true, Color::fromHex(0xF5B800), 0.35f, 0.92f)) break;
                    } else {
                        if (drawKnobSprite("knob_05.png", 1.18f, true, Color::fromHex(0xFACC15), 0.30f, 0.90f)) break;
                    }
                    if (isBotRow && colVar == 1) {
                        // Salvaged Red & White Crimped Soda Bottle Cap
                        for (int i = 0; i < 18; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 18.0f);
                            g.fillCircle({cx + std::cos(a) * R * 0.90f, cy + std::sin(a) * R * 0.90f}, 2.4f, Color::fromHex(0xCBD5E1));
                        }
                        g.fillRadialGradientCircle(center, R * 0.86f, Color::fromHex(0xE11D48), Color::fromHex(0x7F1D1D));
                        g.drawCircle(center, R * 0.72f, Color::fromHex(0xFFFFFF, 0.80f), 1.6f);
                        g.drawLine({cx - R * 0.45f, cy}, {cx + R * 0.45f, cy}, Color::fromHex(0xFFFFFF, 0.85f), 3.2f);
                        g.drawLine(center, {cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos}, Color::fromHex(0xFACC15), 2.6f);
                    } else if (colVar == 0) {
                        // Heavy Rusted 12-Tooth Salvaged Gear Cog
                        for (int i = 0; i < 12; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 12.0f);
                            g.fillCircle({cx + std::cos(a) * R * 0.94f, cy + std::sin(a) * R * 0.94f}, 3.4f, Color::fromHex(0x6E3B1E));
                        }
                        g.fillRadialGradientCircle(center, R * 0.86f, Color::fromHex(0x9C5832), Color::fromHex(0x3B1F0E));
                        g.drawCircle(center, R * 0.86f, Color::fromHex(0x211107), 1.8f);
                        g.fillCircle(center, R * 0.34f, Color::fromHex(0x1A1410));
                        g.drawCircle(center, R * 0.34f, Color::fromHex(0x5C341B), 1.5f);
                        g.drawLine({cx + R * 0.32f * ptrSin, cy - R * 0.32f * ptrCos}, {cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos},
                                   Color::fromHex(0xF5B800), 2.6f);
                    } else {
                        // Distressed Wasteland Turret / Cracked Enamel Dial
                        g.fillRadialGradientCircle(center, R * 0.95f, Color::fromHex(0x52473D), Color::fromHex(0x1E1915));
                        g.drawCircle(center, R * 0.95f, Color::fromHex(0x120E0B), 1.8f);
                        g.fillRadialGradientCircle(center, R * 0.68f, (colVar == 1) ? Color::fromHex(0xE2D9C8) : Color::fromHex(0x3D352E),
                                                   (colVar == 1) ? Color::fromHex(0x8C6239) : Color::fromHex(0x1C1814));
                        g.drawCircle(center, R * 0.68f, Color::fromHex(0x1A1410), 1.5f);
                        g.drawLine({cx + R * 0.18f * ptrSin, cy - R * 0.18f * ptrCos}, {cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos},
                                   Color::fromHex(0xF5B800), 2.6f);
                    }
                    break;
                }
                case HardwareKnobStyle::PunkSpiked: {
                    // PUNK: 3D Chrome Stud Spikes (col 0/2), 2-Tone Ska
                    // Checkerboard (col 1/3)
                    if (colVar & 1) {
                        if (drawKnobSprite("knob_07.png", 1.18f, true, Color::fromHex(0xFF1E8E), 0.30f, 0.90f)) break;
                    } else {
                        if (drawKnobSprite("knob_06.png", 1.24f, true, Color::fromHex(0xA3E635), 0.28f, 0.86f)) break;
                    }
                    if (colVar == 0 && !isBotRow) {
                        // 10 Tall 3D Chrome Cone Spikes radiating around the
                        // collar
                        for (int i = 0; i < 10; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 10.0f);
                            std::array<Point<float>, 3> sp = {
                                {{cx + std::cos(a - 0.18f) * R * 0.72f, cy + std::sin(a - 0.18f) * R * 0.72f},
                                 {cx + std::cos(a) * R * 1.14f, cy + std::sin(a) * R * 1.14f},
                                 {cx + std::cos(a + 0.18f) * R * 0.72f, cy + std::sin(a + 0.18f) * R * 0.72f}}};
                            g.fillPolygon(sp, Color::fromHex(0xF1F5F9));
                            g.drawLine(sp[0], sp[1], Color::fromHex(0x475569), 1.0f);
                        }
                        g.fillRadialGradientCircle(center, R * 0.74f, Color::fromHex(0x334155), Color::fromHex(0x0F172A));
                        g.drawCircle(center, R * 0.74f, Color::fromHex(0x94A3B8), 1.6f);
                        g.drawLine(center, {cx + R * 0.72f * ptrSin, cy - R * 0.72f * ptrCos}, Color::fromHex(0xFF1E8E), 2.8f);
                    } else if (colVar == 1) {
                        // 2-Tone Ska Black & White Checkerboard Button Dial
                        // inside a silver bezel
                        g.fillCircle(center, R * 0.96f, Color::fromHex(0x1E293B));
                        g.drawCircle(center, R * 0.96f, Color::fromHex(0xCBD5E1), 2.0f);
                        g.fillCircle(center, R * 0.74f, Color::fromHex(0xF8FAFC));
                        const float step = R * 0.34f;
                        for (int r = -2; r <= 1; ++r) {
                            for (int c = -2; c <= 1; ++c) {
                                if (((r + c) & 1) == 0) {
                                    const Rect<float> sq = {cx + static_cast<float>(c) * step, cy + static_cast<float>(r) * step, step,
                                                            step};
                                    if (std::hypot(sq.getCentre().x - cx, sq.getCentre().y - cy) < R * 0.64f) {
                                        g.fillRect(sq, Color::fromHex(0x0F172A));
                                    }
                                }
                            }
                        }
                        g.drawCircle(center, R * 0.74f, Color::fromHex(0x0F172A), 2.2f);
                        g.drawLine(center, {cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos}, Color::fromHex(0xFF1E8E), 2.8f);
                    } else {
                        // Acid-Lime or Hot-Pink Punk Pin Badge Dial
                        g.fillCircle(center, R * 0.96f, Color::fromHex(0x18181B));
                        g.drawCircle(center, R * 0.96f, Color::fromHex(0x71717A), 1.8f);
                        const Color badgeCol = (colVar == 0 && isBotRow) ? Color::fromHex(0xA3E635) : Color::fromHex(0x27272A);
                        g.fillRadialGradientCircle(center, R * 0.74f, badgeCol, Color::fromHex(0x09090B));
                        g.drawCircle(center, R * 0.74f, (colVar == 0 && isBotRow) ? Color::fromHex(0x18181B) : Color::fromHex(0xFF1E8E),
                                     1.8f);
                        g.drawLine(center, {cx + R * 0.86f * ptrSin, cy - R * 0.86f * ptrCos},
                                   (colVar == 0 && isBotRow) ? Color::fromHex(0x18181B) : Color::fromHex(0xA3E635), 2.8f);
                    }
                    break;
                }
                case HardwareKnobStyle::MeadowWoodSlice: {
                    // GRASS / MEADOW: Cross-cut natural timber slice with
                    // rough bark rim, growth rings, and tiny leaf/flower
                    // sprout
                    if (drawKnobSprite("knob_18.png", 1.20f, true, Color::fromHex(0xFEF08A), 0.22f, 0.86f)) break;
                    g.fillCircle(center, R * 0.98f, Color::fromHex(0x3B2313));
                    g.fillRadialGradientCircle(center, R * 0.86f, Color::fromHex(0xD2A679), Color::fromHex(0x8C5830), {-2.0f, -2.0f});
                    g.drawCircle(center, R * 0.86f, Color::fromHex(0x4A2C18), 2.2f);
                    g.drawCircle(center, R * 0.64f, Color::fromHex(0x6E4224, 0.55f), 1.1f);
                    g.drawCircle(center, R * 0.42f, Color::fromHex(0x6E4224, 0.55f), 1.1f);
                    g.drawCircle(center, R * 0.20f, Color::fromHex(0x6E4224, 0.55f), 1.1f);
                    // Natural timber split notch + pointer
                    g.drawLine(center, {cx + R * 0.84f * ptrSin, cy - R * 0.84f * ptrCos}, Color::fromHex(0x2E1A0E), 3.2f);
                    g.drawLine(center, {cx + R * 0.84f * ptrSin, cy - R * 0.84f * ptrCos}, Color::fromHex(0xFEF08A), 1.8f);
                    // Tiny blooming flower / clover accent on top edge of
                    // wood slice
                    g.fillCircle({cx - R * 0.22f, cy - R * 0.86f}, 3.2f, Color::fromHex(0x4ADE80));
                    g.fillCircle({cx - R * 0.14f, cy - R * 0.92f}, 2.6f,
                                 (colVar & 1) ? Color::fromHex(0xFACC15) : Color::fromHex(0xF472B6));
                    break;
                }
                case HardwareKnobStyle::SkySunburst: {
                    // SKY / CLOUDS: Pearlescent Cloud Dials for Stage Component knobs, Radiant Golden Sunburst Dials for Metering Output knobs
                    if (!isMeteringBayKnob) {
                        if (drawKnobSprite("knob_19.png", 1.18f, false, Color::fromHex(0xF59E0B), 0.35f, 0.90f)) break;
                    } else {
                        if (drawKnobSprite("knob_20.png", 1.22f, true, Color::fromHex(0xFFFFFF), 0.22f, 0.84f)) break;
                    }
                    if (isMeteringBayKnob) {
                        // Radiant Golden Sunburst Dial with 20 glowing solar
                        // rays
                        g.fillRadialGradientCircle(center, R * 1.12f, Color::fromHex(0xFDE047, 0.45f), Color::fromHex(0xF59E0B, 0.0f));
                        for (int i = 0; i < 20; ++i) {
                            const float a = angleRad + static_cast<float>(i) * (6.2831853f / 20.0f);
                            const float rOut = (i % 2 == 0) ? (R * 1.08f) : (R * 0.94f);
                            g.drawLine({cx + std::cos(a) * R * 0.66f, cy + std::sin(a) * R * 0.66f},
                                       {cx + std::cos(a) * rOut, cy + std::sin(a) * rOut}, Color::fromHex(0xFBBF24), 2.2f);
                        }
                        g.fillRadialGradientCircle(center, R * 0.72f, Color::fromHex(0xFFFBEB), Color::fromHex(0xF59E0B));
                        g.drawCircle(center, R * 0.72f, Color::fromHex(0xFFFFFF, 0.90f), 1.5f);
                        g.drawLine(center, {cx + R * 0.68f * ptrSin, cy - R * 0.68f * ptrCos}, Color::fromHex(0xFFFFFF), 2.6f);
                    } else {
                        // Pearlescent Sky-Blue & Fluffy Cumulus Cloud Dial
                        // with golden-hour rim light
                        g.fillRadialGradientCircle(center, R * 0.94f, Color::fromHex(0xE0F2FE), Color::fromHex(0x7DD3FC), {-3.0f, -3.0f});
                        g.fillCircle({cx - R * 0.24f, cy + R * 0.18f}, R * 0.36f, Color::fromHex(0xFFFFFF, 0.75f));
                        g.fillCircle({cx + R * 0.18f, cy + R * 0.22f}, R * 0.32f, Color::fromHex(0xFFFFFF, 0.75f));
                        g.drawCircle(center, R * 0.94f, Color::fromHex(0xFFFFFF, 0.90f), 1.6f);
                        g.drawArc(center, R * 0.94f, 0.4f, 2.6f, Color::fromHex(0xFBBF24, 0.75f), 2.2f);
                        g.drawLine({cx + R * 0.20f * ptrSin, cy - R * 0.20f * ptrCos}, {cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos},
                                   Color::fromHex(0xFFFFFF), 2.6f);
                    }
                    break;
                }
                case HardwareKnobStyle::StormRaindrop: {
                    // RAIN / STORM: Dark wet storm-slate dial with glowing
                    // electric-blue arc & water droplets
                    if (drawKnobSprite("knob_21.png", 1.18f, true, Color::fromHex(0xE0F2FE), 0.22f, 0.88f, true, Color::fromHex(0x38BDF8)))
                        break;
                    g.fillLinearGradientCircle6Stops(center, R * 0.96f, {cx - R * 0.7f, cy - R * 0.7f}, {cx + R * 0.7f, cy + R * 0.7f},
                                                     Color::fromHex(0x475E75), Color::fromHex(0x1E2D3D), Color::fromHex(0x33475B),
                                                     Color::fromHex(0x0F1822), Color::fromHex(0x263747), Color::fromHex(0x080D14));
                    g.drawCircle(center, R * 0.96f, Color::fromHex(0x070C12), 1.5f);
                    g.drawArc(center, R * 0.98f, startRad, endRad, Color::fromHex(0x38BDF8, 0.38f), 5.5f);
                    g.drawArc(center, R * 0.98f, startRad, endRad, Color::fromHex(0x7DD3FC), 2.4f);
                    g.fillRadialGradientCircle(center, R * 0.68f, Color::fromHex(0x283C4F), Color::fromHex(0x0C1520), {-2.5f, -2.5f});
                    g.drawCircle(center, R * 0.68f, Color::fromHex(0x7DD3FC, 0.55f), 1.2f);
                    // Water condensation droplets on knob cap
                    g.fillCircle({cx - R * 0.28f, cy - R * 0.18f}, 2.2f, Color::fromHex(0xBAE6FD, 0.75f));
                    g.fillCircle({cx + R * 0.24f, cy + R * 0.26f}, 1.8f, Color::fromHex(0xBAE6FD, 0.65f));
                    g.drawLine({cx + R * 0.16f * ptrSin, cy - R * 0.16f * ptrCos}, {cx + R * 0.88f * ptrSin, cy - R * 0.88f * ptrCos},
                               Color::fromHex(0xE0F2FE), 2.5f);
                    break;
                }
                case HardwareKnobStyle::GlacialIceCrystal: {
                    // SNOW / ICY: Faceted crystalline glacial ice dial with
                    // internal frost fractures & specular rim
                    if (drawKnobSprite("knob_22.png", 1.18f, true, Color::fromHex(0xFFFFFF), 0.20f, 0.88f)) break;
                    g.fillRadialGradientCircle(center, R * 0.96f, Color::fromHex(0xF0F9FF), Color::fromHex(0x38BDF8), {-3.0f, -3.5f});
                    g.drawCircle(center, R * 0.96f, Color::fromHex(0xFFFFFF, 0.95f), 2.0f);
                    g.fillRadialGradientCircle(center, R * 0.70f, Color::fromHex(0xFFFFFF), Color::fromHex(0x7DD3FC));
                    for (int i = 0; i < 6; ++i) {
                        const float a = angleRad + static_cast<float>(i) * (3.14159265f / 3.0f);
                        g.drawLine(center, {cx + std::cos(a) * R * 0.86f, cy + std::sin(a) * R * 0.86f}, Color::fromHex(0xFFFFFF, 0.85f),
                                   1.5f);
                    }
                    g.drawCircle(center, R * 0.70f, Color::fromHex(0xE0F2FE), 1.4f);
                    g.drawLine(center, {cx + R * 0.90f * ptrSin, cy - R * 0.90f * ptrCos}, Color::fromHex(0x0369A1), 2.8f);
                    g.drawLine(center, {cx + R * 0.90f * ptrSin, cy - R * 0.90f * ptrCos}, Color::fromHex(0xFFFFFF), 1.3f);
                    break;
                }
                default:
                    break;
                }
        }

        // Console China-Marker / Wax-Pencil Sweet-Spot Mark on the
        // outer skirt (Shift + Right-Click)
        if (hasWaxMark) {
            const float waxAngle = startAngle + math::clamp(waxMarkNorm, 0.0f, 1.0f) * (endAngle - startAngle);
            const float ws = std::sin(waxAngle), wc = std::cos(waxAngle);
            const float ws2 = std::sin(waxAngle + 0.016f), wc2 = std::cos(waxAngle + 0.016f);
            const float rWaxIn = R * 0.77f;
            const float rWaxOut = R + 6.8f;
            g.drawLine({cx + rWaxIn * ws + 0.8f, cy - rWaxIn * wc + 0.9f}, {cx + rWaxOut * ws + 0.8f, cy - rWaxOut * wc + 0.9f},
                       Color::fromHex(0x0A0D10, 0.62f), 3.6f);
            g.drawLine({cx + rWaxIn * ws, cy - rWaxIn * wc}, {cx + rWaxOut * ws, cy - rWaxOut * wc}, Color::fromHex(0xFFE97D, 0.95f), 2.6f);
            g.drawLine({cx + (R * 0.81f) * ws2, cy - (R * 0.81f) * wc2}, {cx + (R + 5.8f) * ws2, cy - (R + 5.8f) * wc2},
                       Color::fromHex(0xFFFDF0, 0.88f), 1.3f);
            g.fillCircle({cx + (rWaxOut + 0.4f) * ws, cy - (rWaxOut + 0.4f) * wc}, 1.6f, Color::fromHex(0xFFE97D, 0.92f));
        }

        (void)bodyColorHex;
        (void)indicatorColorHex;

        // ====================================================================
        // PASS 5.5 (Above Knob Sprite): High-Contrast Radial Scale
        // Legends
        // ====================================================================
        if (tickStyle != KnobTickMarkStyle::None) {
            const size_t numMajor = (tickStyle == KnobTickMarkStyle::Numbered0To10) ? 11 : activeScaleLabels.size();
            const float stepTol = (numMajor > 1) ? (0.48f / static_cast<float>(numMajor - 1)) : 0.25f;
            const bool decimateLabels = (tickStyle != KnobTickMarkStyle::Numbered0To10) && (activeScaleLabels.size() > 7);
            constexpr size_t kMaxPerimeterLabels = 5;
            const auto isLabelledStep = [&](size_t idx) noexcept -> bool {
                if (!decimateLabels) return true;
                const size_t last = activeScaleLabels.size() - 1;
                for (size_t k = 0; k < kMaxPerimeterLabels; ++k) {
                    const size_t pick = static_cast<size_t>(
                        std::lround(static_cast<double>(k) * static_cast<double>(last) / static_cast<double>(kMaxPerimeterLabels - 1)));
                    if (pick == idx) return true;
                }
                return false;
            };

            for (size_t i = 0; i < numMajor; ++i) {
                std::string lblStr;
                if (tickStyle == KnobTickMarkStyle::Numbered0To10) {
                    lblStr = (i % 2 == 0) ? std::to_string(i) : "";
                } else if (i < activeScaleLabels.size() && isLabelledStep(i)) {
                    lblStr = activeScaleLabels[i];
                }
                if (lblStr.empty()) continue;

                const float t = (numMajor > 1) ? static_cast<float>(i) / static_cast<float>(numMajor - 1) : 0.5f;
                const float a = startAngle + t * (endAngle - startAngle);
                const float sinA = std::sin(a);
                const float cosA = std::cos(a);
                const bool isActiveStep = (style == HardwareKnobStyle::SteppedAttenuator) && (std::abs(t - clampedNorm) <= stepTol);
                bool isAmberCallout = (i == 0 && firstIsOrange) || (i + 1 == numMajor && lastIsOrange) || isActiveStep;
                if (lblStr == "OFF" || lblStr == "AUTO" || lblStr == "A1" || lblStr == "A2" || lblStr == "ALL" || lblStr == "LINE" ||
                    lblStr == "MIC" || lblStr.find("\xE2\x88\x9E") != std::string::npos) {
                    isAmberCallout = true;
                }
                const float scaleFs = std::clamp(8.0f * uiScale, 6.8f, 8.8f);
                const float effScaleFs = scaleFs * g.getFontScale();
                const float pillW = std::clamp(static_cast<float>(lblStr.size()) * (effScaleFs * 0.68f) + 7.5f, 15.0f, 48.0f);
                const float pillH = 10.4f;
                const bool isBottomSkirtLabel = (cosA < -0.45f);
                float lx = cx;
                float ly = cy;
                if (isBottomSkirtLabel) {
                    const float sideSign = (sinA < 0.0f) ? -1.0f : 1.0f;
                    lx = cx + sinA * (R + 8.5f * uiScale) + sideSign * (pillW * 0.50f);
                    ly = cy + R * 0.56f;
                } else {
                    lx = cx + sinA * (R + 8.5f * uiScale + pillW * 0.46f);
                    ly = cy - cosA * (R + 8.5f * uiScale + pillH * 0.42f);
                }
                const Rect<float> tickPill = {lx - pillW * 0.5f, ly - pillH * 0.5f, pillW, pillH};
                const Color kPlateCol = HardwareMaterials::resolveWidgetPlateFill(g, 0.54f, 0.66f);
                const bool lightPlate = HardwareMaterials::isLightPlateFill(g, kPlateCol);
                if (kPlateCol.a > 0.02f) {
                    g.fillRoundedRect(tickPill, 2.2f, kPlateCol);
                    g.drawRoundedRect(tickPill, 2.2f,
                                      lightPlate ? Color::fromHex(0x64748B, 0.36f) : Color::fromHex(0xFFFFFF, 0.14f), 0.6f);
                }
                const Color lblInk = isAmberCallout ? HardwareMaterials::resolvePlateReadoutInk(g, kPlateCol, warmAmberTick)
                                                    : HardwareMaterials::resolvePlateInkColor(g, kPlateCol);
                const Color tickShadow = lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.55f) : scaleShadow;
                g.drawText(lblStr, {tickPill.x + 0.6f, ly - 5.6f + 0.7f, tickPill.width, 11.5f}, tickShadow, scaleFs, true, true);
                g.drawText(lblStr, {tickPill.x, ly - 5.6f, tickPill.width, 11.5f}, lblInk, scaleFs, true, true);
            }
        }

        // ====================================================================
        // PASS 6 (Outside R): Parameter Caption & Value Readout with
        // guaranteed vertical & horizontal separation
        // ====================================================================
        std::string smartVal, smartUnit;
        HardwareMaterials::formatSmartReadoutAndUnit(valText, unitText, smartVal, smartUnit);

        const float textSpanW = std::clamp(bounds.width + 24.0f, 74.0f, 98.0f);
        const float yCaption = cy + R + std::max(11.5f, 14.0f * uiScale);
        const float yReadout = yCaption + std::max(13.8f, 14.8f * uiScale);
        const Rect<float> titleRect = {cx - textSpanW * 0.5f, yCaption - 6.2f, textSpanW, 12.8f};
        const Rect<float> valRect = {cx - textSpanW * 0.5f, yReadout - 6.0f, textSpanW, 12.0f};

        // Line 1: 100% UPPERCASE bold silkscreen title centered below
        // the knob skirt with 1px black drop shadow
        HardwareMaterials::drawLabelPlaque(g, titleRect, title, labelPlaque, g.resolveSilkscreenColor(Color::fromHex(0xEDEAE0), 0.96f),
                                           std::clamp(9.0f * uiScale, 7.8f, 9.6f), warmAmberTick, true);

        // Line 2: High-contrast live value readout pill centered
        // cleanly below the title
        if (!valText.empty()) {
            const std::string combinedReadout = smartUnit.empty() ? smartVal : std::format("{} {}", smartVal, smartUnit);
            const float valFs = std::clamp(8.6f * uiScale, 7.6f, 9.2f);
            const float effValFs = valFs * g.getFontScale();
            const float pillW = std::clamp(static_cast<float>(combinedReadout.size()) * (effValFs * 0.68f) + 12.0f, 34.0f,
                                           std::max(textSpanW, 108.0f));
            const Rect<float> rPill = {cx - pillW * 0.5f, valRect.y + 0.3f, pillW, 11.4f};
            const Color rPlateCol = HardwareMaterials::resolveWidgetPlateFill(g, 0.58f, 0.70f);
            const bool lightPlate = HardwareMaterials::isLightPlateFill(g, rPlateCol);
            if (rPlateCol.a > 0.02f) {
                g.fillRoundedRect(rPill, 2.2f, rPlateCol);
                g.drawRoundedRect(rPill, 2.2f, lightPlate ? Color::fromHex(0x64748B, 0.42f) : Color::fromHex(0xFFFFFF, 0.16f), 0.7f);
            }
            const Color readoutInk = HardwareMaterials::resolvePlateReadoutInk(g, rPlateCol, warmAmberTick);
            g.drawText(combinedReadout, {rPill.x + 0.6f, valRect.y + 0.7f, rPill.width, valRect.height},
                       lightPlate ? Color(1.0f, 1.0f, 1.0f, 0.58f) : Color(0.0f, 0.0f, 0.0f, 0.80f), valFs, true, true);
            g.drawText(combinedReadout, {rPill.x, valRect.y, rPill.width, valRect.height}, readoutInk, valFs, true, true);
        }
    }

    void setHideValueReadout(bool hide) noexcept { hideValueReadout_ = hide; }

    void paint(Graphics &g) override {
        const auto local = getLocalBounds();
        if (!param_) return;

        const std::string &title = customTitle_.empty() ? param_->getName() : customTitle_;
        const std::string valText = hideValueReadout_ ? "" : (outerParam_ ? outerParam_->getCurrentText() : param_->getCurrentText());
        drawRotaryVisual(g, local, style_, param_->getNormalized(), title, valText, hideValueReadout_ ? "" : unitOverride_, scaleLabels_,
                         firstIsOrange_, lastIsOrange_, outerParam_ != nullptr, outerParam_ ? outerParam_->getNormalized() : 0.5f,
                         accentCol_);
    }

    void onMouseDown(const MouseEvent &e) override {
        if (!param_) return;
        editingOuter_ = (outerParam_ != nullptr) && e.isShiftDown;
        dragStartY_ = e.pos.y;
        dragStartVal_ = editingOuter_ ? outerParam_->getNormalized() : param_->getNormalized();
        repaint();
    }

    void onMouseDrag(const MouseEvent &e) override {
        Parameter *target = (editingOuter_ && outerParam_) ? outerParam_ : param_;
        if (!target) return;

        const float deltaY = dragStartY_ - e.pos.y;
        const float sensitivity = 135.0f;
        target->setNormalized(math::clamp(dragStartVal_ + deltaY / sensitivity, 0.0f, 1.0f));
        repaint();
    }

    void onDoubleClick(const MouseEvent &) override {
        if (param_) {
            param_->setPlain(param_->getDefaultValue());
            repaint();
        }
    }

  private:
    Parameter *param_{nullptr};
    Parameter *outerParam_{nullptr};
    HardwareKnobStyle style_{HardwareKnobStyle::PointerSkirted};
    std::string customTitle_{};
    std::string unitOverride_{};
    std::vector<std::string> scaleLabels_{};
    bool firstIsOrange_{false};
    bool lastIsOrange_{false};
    bool editingOuter_{false};
    bool hideValueReadout_{false};
    float dragStartY_{0.0f};
    float dragStartVal_{0.0f};
    Color accentCol_{Color::fromHex(0xE07A28)};
};

/**
 * @brief KENNEDARTH TEAL 3D Machined Chrome Bat-Handle Toggle Switch
 * with 6-Sided Hex Mounting Nut, Brushed Steel Washer, Dark
 * Bushing Well, Conical Chrome Bat Lever & Paired Recessed 3D
 * Jewel LED.
 */
class HardwareToggleSwitch : public Component {
  public:
    HardwareToggleSwitch(Parameter *param, std::string bottomTitle, std::string topStateLabel = "IN", std::string botStateLabel = "OUT",
                         bool ledOnLeft = true)
        : param_(param), title_(std::move(bottomTitle)), topLabel_(std::move(topStateLabel)), botLabel_(std::move(botStateLabel)),
          ledOnLeft_(ledOnLeft) {
        const auto p = botLabel_.find('|');
        if (p != std::string::npos) {
            midLabel_ = botLabel_.substr(0, p);
            botLabel_ = botLabel_.substr(p + 1);
            is3Way_ = true;
        }
        if (topLabel_ == "MONO (PURE)" && botLabel_ == "STEREO") {
            topLabel_ = "STEREO";
            botLabel_ = "MONO (PURE)";
            if (title_.empty()) {
                title_ = "STEREO MODE";
            }
        }
    }

    void setSwitchVisualStyle(int style) noexcept { visualStyle_ = style; }
    void setAccentColor(Color c) noexcept { accentCol_ = c; }
    void setPlaqueStyle(UiLabelPlaqueStyle ps) noexcept { plaqueStyle_ = ps; }

    static void drawToggle3WayVisual(Graphics &g, Rect<float> bounds, int pos012, std::string_view title,
                                     std::string_view topLabel = "MONO", std::string_view midLabel = "STEREO",
                                     std::string_view botLabel = "M/S", bool ledOnLeft = true, Color accentCol = Color::fromHex(0xFFA033),
                                     UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        pos012 = std::clamp(pos012, 0, 2);
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const bool isCompact = (bounds.height < 68.0f);
        const float titleH = hasBottomTitle ? 15.5f : 0.0f;
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy =
            hasBottomTitle ? ((bounds.height <= 64.0f)
                                  ? (bounds.y + 12.0f)
                                  : (bounds.y + std::clamp((bounds.height - 18.0f) * 0.5f, 26.0f, std::max(26.0f, bounds.height - 46.0f))))
                           : (bounds.y + bounds.height * 0.50f);
        const Color silkInk = g.resolveSilkscreenColor(Color::fromHex(0xEDEAE0), 0.96f);
        const Color stateShadow = Color(0.0f, 0.0f, 0.0f, 0.48f);
        const Color activeInk = HardwareMaterials::resolveChassisAccentInk(g, accentCol);
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);

        const Color topCol = (pos012 == 0) ? activeInk : silkInk;
        const Color midCol = (pos012 == 1) ? activeInk : silkInk;
        const Color botCol = (pos012 == 2) ? activeInk : silkInk;

        const float topLabelY = cy - (isCompact ? 24.5f : 27.5f);
        const float botLabelY = cy + (isCompact ? 12.0f : 15.5f);
        HardwareMaterials::drawSwitchStatePill(g, cx, topLabelY, topLabel, pos012 == 0, accentCol, stateFs, maxSpanW);

        const float nutR = isCompact ? 8.8f : 9.8f;
        g.drawText(midLabel, {cx + nutR + 2.5f, cy - 5.0f, 42.0f, 12.0f}, stateShadow, stateFs * 0.94f, false, pos012 == 1);
        g.drawText(midLabel, {cx + nutR + 2.5f, cy - 6.0f, 42.0f, 12.0f}, midCol, stateFs * 0.94f, false, pos012 == 1);

        HardwareMaterials::drawSwitchStatePill(g, cx, botLabelY, botLabel, pos012 == 2, accentCol, stateFs, maxSpanW);
        (void)topCol;
        (void)botCol;

        const Point<float> ledPos = ledOnLeft ? Point<float>{cx - 23.5f, cy} : Point<float>{cx, cy - 19.0f};
        HardwareMaterials::drawJewelLed(g, ledPos, true, accentCol, 3.3f);

        g.fillCircle({cx + 1.8f, cy + 2.8f}, nutR + 1.2f, Color::fromHex(0x030608, 0.82f));
        g.fillRadialGradientCircle({cx, cy}, nutR + 1.0f, Color::fromHex(0xA4AEB8), Color::fromHex(0x3A424A), {-2.0f, -2.0f});
        g.drawCircle({cx, cy}, nutR + 1.0f, Color::fromHex(0x14181C), 0.9f);

        std::array<Point<float>, 7> hex{};
        for (size_t i = 0; i < 7; ++i) {
            const float a = static_cast<float>(i) * (3.14159265f / 3.0f) + 0.5235988f;
            hex[i] = {cx + std::cos(a) * nutR * 0.88f, cy + std::sin(a) * nutR * 0.88f};
        }
        g.fillPolygon(std::span<const Point<float>>(hex.data(), 6), Color::fromHex(0xC4CCD4));
        g.drawPolyline(hex, Color::fromHex(0x20262C), 1.2f);
        g.drawCircle({cx, cy}, nutR * 0.68f, Color::fromHex(0xE8EEF2, 0.65f), 0.9f);

        g.fillCircle({cx, cy}, 3.9f, Color::fromHex(0x080B0E));
        g.drawCircle({cx, cy}, 3.9f, Color::fromHex(0x283038), 0.8f);

        const float batLen = isCompact ? 8.8f : 10.2f;
        const float tipY = (pos012 == 0) ? (cy - batLen) : ((pos012 == 2) ? (cy + batLen) : cy);
        const float tipHalfW = isCompact ? 3.7f : 4.1f;
        if (pos012 != 1) {
            g.drawLine({cx + 1.6f, cy + 1.6f}, {cx + 1.6f, tipY + 1.6f}, Color::fromHex(0x030608, 0.78f), isCompact ? 5.6f : 6.4f);
            const float baseHalfW = isCompact ? 2.1f : 2.4f;
            std::array<Point<float>, 4> batPoly = {
                {{cx - baseHalfW, cy}, {cx - tipHalfW, tipY}, {cx + tipHalfW, tipY}, {cx + baseHalfW, cy}}};
            g.fillPolygon(batPoly, Color::fromHex(0x707A84));
            g.drawLine({cx - baseHalfW * 0.5f, cy}, {cx - tipHalfW * 0.5f, tipY}, Color::fromHex(0xEEF4F8), isCompact ? 1.8f : 2.1f);
            g.drawLine({cx + baseHalfW * 0.6f, cy}, {cx + tipHalfW * 0.6f, tipY}, Color::fromHex(0x384048), 1.2f);
        } else {
            g.fillCircle({cx + 1.6f, cy + 1.8f}, tipHalfW + 0.5f, Color::fromHex(0x030608, 0.78f));
        }
        g.fillRadialGradientCircle({cx, tipY}, tipHalfW, Color::fromHex(0xFFFFFF), Color::fromHex(0x505860), {-1.2f, -1.2f});
        g.drawCircle({cx, tipY}, tipHalfW, Color::fromHex(0x1E242A), 0.8f);

        if (hasBottomTitle) {
            const float titleY = cy + (isCompact ? 28.0f : 31.5f);
            HardwareMaterials::drawLabelPlaque(g, {cx - maxSpanW * 0.5f, titleY, maxSpanW, titleH}, title, plaqueStyle,
                                               g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.6f, accentCol,
                                               true);
        }
    }

    static void drawToggleVisual(Graphics &g, Rect<float> bounds, bool isOn, std::string_view title, std::string_view topLabel = "IN",
                                 std::string_view botLabel = "OUT", bool ledOnLeft = true, Color accentCol = Color::fromHex(0xFFA033),
                                 UiLabelPlaqueStyle plaqueStyle = UiLabelPlaqueStyle::DirectSilkscreen) noexcept {
        // Always reserve and draw the bottom title when `title` is
        // non-empty so adjacent switches (e.g., PHASE & MUTE) align
        // identically
        const bool hasBottomTitle = (!title.empty() && title != "MONO (PURE)" && title != "STEREO MODE" &&
                                     title != "BYPASS" && title != "MASTER" && title != "MASTER BYP");
        const bool isCompact = (bounds.height < 68.0f);
        const float titleH = hasBottomTitle ? 15.5f : 0.0f;
        const float cx = bounds.x + bounds.width * 0.5f;
        const float cy =
            hasBottomTitle ? ((bounds.height <= 64.0f)
                                  ? (bounds.y + 12.0f)
                                  : (bounds.y + std::clamp((bounds.height - 18.0f) * 0.5f, 26.0f, std::max(26.0f, bounds.height - 46.0f))))
                           : (bounds.y + bounds.height * 0.50f);
        // High-contrast state pills for top (MONO (PURE)/IN/LINK) and
        // bottom (STEREO/BYP/FREE) state labels centered on `cx`
        const float stateFs = isCompact ? 8.2f : 8.6f;
        const float topLabelY = cy - (isCompact ? 24.5f : 27.5f);
        const float botLabelY = cy + (isCompact ? 12.0f : 15.5f);
        const float maxSpanW = std::clamp(bounds.width + 18.0f, 82.0f, 112.0f);
        HardwareMaterials::drawSwitchStatePill(g, cx, topLabelY, topLabel, isOn, accentCol, stateFs, maxSpanW);
        HardwareMaterials::drawSwitchStatePill(g, cx, botLabelY, botLabel, !isOn, accentCol, stateFs, maxSpanW);

        const float nutR = isCompact ? 8.8f : 9.8f; // ~19.5px diameter hex nut
        const Point<float> ledPos = ledOnLeft ? Point<float>{cx - 23.5f, cy} : Point<float>{cx, cy - 19.0f};
        HardwareMaterials::drawJewelLed(g, ledPos, isOn, accentCol,
                                        3.3f); // 6.6px diameter recessed jewel LED

        // 1. Cast 3px drop shadow + Brushed steel circular
        // washer ring
        g.fillCircle({cx + 1.8f, cy + 2.8f}, nutR + 1.2f, Color::fromHex(0x030608, 0.82f));
        g.fillRadialGradientCircle({cx, cy}, nutR + 1.0f, Color::fromHex(0xA4AEB8), Color::fromHex(0x3A424A), {-2.0f, -2.0f});
        g.drawCircle({cx, cy}, nutR + 1.0f, Color::fromHex(0x14181C), 0.9f);

        // 2. 6-Sided Hexagonal Mounting Nut with metallic bevel
        // shading
        std::array<Point<float>, 7> hex{};
        for (size_t i = 0; i < 7; ++i) {
            const float a = static_cast<float>(i) * (3.14159265f / 3.0f) + 0.5235988f;
            hex[i] = {cx + std::cos(a) * nutR * 0.88f, cy + std::sin(a) * nutR * 0.88f};
        }
        g.fillPolygon(std::span<const Point<float>>(hex.data(), 6), Color::fromHex(0xC4CCD4));
        g.drawPolyline(hex, Color::fromHex(0x20262C), 1.2f);
        g.drawCircle({cx, cy}, nutR * 0.68f, Color::fromHex(0xE8EEF2, 0.65f), 0.9f);

        // 3. Dark Bushing Well (dark recessed center hole)
        g.fillCircle({cx, cy}, 3.9f, Color::fromHex(0x080B0E));
        g.drawCircle({cx, cy}, 3.9f, Color::fromHex(0x283038), 0.8f);

        // 4. 3D Tapered Chrome Bat Lever (#E8EEF2 highlight to
        // #505860 shadow) with cast drop shadow
        const float batLen = isCompact ? 8.8f : 10.2f;
        const float tipY = isOn ? (cy - batLen) : (cy + batLen);
        g.drawLine({cx + 1.6f, cy + 1.6f}, {cx + 1.6f, tipY + 1.6f}, Color::fromHex(0x030608, 0.78f), isCompact ? 5.6f : 6.4f);

        // Tapered conical lever body polygon
        const float baseHalfW = isCompact ? 2.1f : 2.4f;
        const float tipHalfW = isCompact ? 3.7f : 4.1f;
        std::array<Point<float>, 4> batPoly = {{{cx - baseHalfW, cy}, {cx - tipHalfW, tipY}, {cx + tipHalfW, tipY}, {cx + baseHalfW, cy}}};
        g.fillPolygon(batPoly, Color::fromHex(0x707A84));
        g.drawLine({cx - baseHalfW * 0.5f, cy}, {cx - tipHalfW * 0.5f, tipY}, Color::fromHex(0xEEF4F8), isCompact ? 1.8f : 2.1f);
        g.drawLine({cx + baseHalfW * 0.6f, cy}, {cx + tipHalfW * 0.6f, tipY}, Color::fromHex(0x384048), 1.2f);

        // Rounded spherical chrome bat tip with white specular
        // hot-spot
        g.fillRadialGradientCircle({cx, tipY}, tipHalfW, Color::fromHex(0xFFFFFF), Color::fromHex(0x505860), {-1.2f, -1.2f});
        g.drawCircle({cx, tipY}, tipHalfW, Color::fromHex(0x1E242A), 0.8f);

        if (hasBottomTitle) {
            const float titleY = cy + (isCompact ? 28.0f : 31.5f);
            HardwareMaterials::drawLabelPlaque(g, {cx - maxSpanW * 0.5f, titleY, maxSpanW, titleH}, title, plaqueStyle,
                                               g.resolveSilkscreenColor(Color::fromHex(0xE6ECEF)), 8.6f, accentCol,
                                               true);
        }
    }

    void paint(Graphics &g) override {
        const auto local = getLocalBounds();
        const bool isOn = param_ ? (param_->getNormalized() >= 0.5f) : true;
        const std::string subPair = topLabel_ + "|" + botLabel_;
        if (visualStyle_ == 1) {
            HardwareMaterials::drawIlluminatedPushButton(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 2) {
            HardwareMaterials::drawRockerPowerSwitch(g, local, isOn, title_, accentCol_, subPair, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 3) {
            HardwareMaterials::drawLockingCageToggle(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 4) {
            HardwareMaterials::drawFlatPillVectorSwitch(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 5) {
            HardwareMaterials::drawNeumorphicPillSwitch(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 6) {
            HardwareMaterials::drawSciFiHologramSwitch(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 7) {
            HardwareMaterials::drawNavalBrassToggle(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 8) {
            HardwareMaterials::drawPunkSafetyPinToggle(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (visualStyle_ == 9) {
            HardwareMaterials::drawHellfireEmberSwitch(g, local, isOn, title_, subPair, accentCol_, plaqueStyle_);
            return;
        }
        if (is3Way_) {
            const float norm = param_ ? param_->getNormalized() : 0.5f;
            const int pos012 = static_cast<int>(std::round(norm * 2.0f));
            drawToggle3WayVisual(g, local, pos012, title_, topLabel_, midLabel_, botLabel_, ledOnLeft_, accentCol_, plaqueStyle_);
            return;
        }
        drawToggleVisual(g, local, isOn, title_, topLabel_, botLabel_, ledOnLeft_, accentCol_, plaqueStyle_);
    }

    void onMouseDown(const MouseEvent &) override {
        if (param_) {
            if (is3Way_) {
                const int cur = static_cast<int>(std::round(param_->getNormalized() * 2.0f));
                const int next = (cur + 1) % 3;
                param_->setNormalized(static_cast<float>(next) * 0.5f);
            } else {
                const bool cur = param_->getNormalized() > 0.5f;
                param_->setNormalized(cur ? 0.0f : 1.0f);
            }
            repaint();
        }
    }

  private:
    Parameter *param_{nullptr};
    std::string title_;
    std::string topLabel_;
    std::string midLabel_;
    std::string botLabel_;
    bool ledOnLeft_{true};
    bool is3Way_{false};
    int visualStyle_{0};
    Color accentCol_{Color::fromHex(0xFFA033)};
    UiLabelPlaqueStyle plaqueStyle_{UiLabelPlaqueStyle::DirectSilkscreen};
};

/**
 * @brief Interactive Backlit Studio Push Switch Component (for
 * generated VST3/CLAP/Standalone editors).
 */
class HardwarePushButton : public Component {
  public:
    HardwarePushButton(Parameter *param, std::string bottomTitle, std::string stateSub = "IN|BYP",
                       Color litColor = Color::fromHex(0xFFA033))
        : param_(param), title_(std::move(bottomTitle)), stateSub_(std::move(stateSub)), litColor_(litColor) {
        size_t pos = 0;
        while (pos < stateSub_.size()) {
            const size_t next = stateSub_.find('|', pos);
            if (next == std::string::npos) {
                states_.push_back(stateSub_.substr(pos));
                break;
            }
            states_.push_back(stateSub_.substr(pos, next - pos));
            pos = next + 1;
        }
    }

    void paint(Graphics &g) override {
        if (states_.size() > 2 && param_) {
            const int idx = std::clamp(static_cast<int>(std::round(param_->getNormalized() * static_cast<float>(states_.size() - 1))), 0,
                                       static_cast<int>(states_.size() - 1));
            const std::string &activeStr = states_[static_cast<size_t>(idx)];
            const bool isLit = (activeStr != "OFF" && activeStr != "OUT" && activeStr != "BYP");
            const std::string subPair = activeStr + "|" + activeStr;
            HardwareMaterials::drawIlluminatedPushButton(g, getLocalBounds(), isLit, title_, subPair, litColor_);
            return;
        }
        const bool isOn = param_ ? (param_->getNormalized() > 0.5f) : false;
        HardwareMaterials::drawIlluminatedPushButton(g, getLocalBounds(), isOn, title_, stateSub_, litColor_);
    }

    void onMouseDown(const MouseEvent &) override {
        if (param_) {
            const size_t stepCount =
                (states_.size() > 2) ? states_.size() : static_cast<size_t>(std::max<uint32_t>(2u, param_->getNumSteps()));
            if (stepCount > 2) {
                const size_t cur = static_cast<size_t>(std::round(param_->getNormalized() * static_cast<float>(stepCount - 1)));
                const size_t next = (cur + 1) % stepCount;
                param_->setNormalized(static_cast<float>(next) / static_cast<float>(stepCount - 1));
            } else {
                const bool cur = param_->getNormalized() > 0.5f;
                param_->setNormalized(cur ? 0.0f : 1.0f);
            }
            repaint();
        }
    }

  private:
    Parameter *param_{nullptr};
    std::string title_;
    std::string stateSub_;
    std::vector<std::string> states_;
    Color litColor_;
};

/**
 * @brief Interactive Conductive-Plastic Console Vertical Fader
 * Component (for generated VST3/CLAP/Standalone editors).
 */
class HardwareVerticalFader : public Component {
  public:
    std::function<void(float)> onValueChanged;

    HardwareVerticalFader(Parameter *param, std::string title, Color accentCol = Color::fromHex(0x00F5D4))
        : param_(param), title_(std::move(title)), accentCol_(accentCol) {}

    void setHideValueReadout(bool hide) noexcept { hideValueReadout_ = hide; }
    void setCustomTickLabels(std::vector<std::string> labels) { customTickLabels_ = std::move(labels); }
    void setPlaqueStyle(UiLabelPlaqueStyle ps) noexcept { plaqueStyle_ = ps; }

    void paint(Graphics &g) override {
        const float norm = param_ ? param_->getNormalized() : 0.5f;
        const std::string valText = (hideValueReadout_ || !param_) ? "" : param_->getCurrentText();
        HardwareMaterials::drawStudioFader(g, getLocalBounds(), norm, title_, valText, accentCol_, customTickLabels_, plaqueStyle_);
    }

    void onMouseDown(const MouseEvent &e) override {
        if (!param_) return;
        dragStartY_ = e.pos.y;
        dragStartVal_ = param_->getNormalized();
        repaint();
    }

    void onMouseDrag(const MouseEvent &e) override {
        if (!param_) return;
        const float fineMul = e.isShiftDown ? 4.0f : 1.0f;
        const float trackH = std::max(36.0f, getLocalBounds().height - 44.0f) * fineMul;
        const float deltaY = dragStartY_ - e.pos.y;
        const float newNorm = math::clamp(dragStartVal_ + deltaY / trackH, 0.0f, 1.0f);
        param_->setNormalized(newNorm);
        if (onValueChanged) onValueChanged(param_->getPlain());
        repaint();
    }

    void onMouseWheel(const MouseEvent &e, float deltaY) {
        if (!param_) return;
        const float step = e.isShiftDown ? 0.005f : 0.025f;
        const float newNorm = math::clamp(param_->getNormalized() + (deltaY > 0.0f ? step : -step), 0.0f, 1.0f);
        param_->setNormalized(newNorm);
        if (onValueChanged) onValueChanged(param_->getPlain());
        repaint();
    }

    void onDoubleClick(const MouseEvent &) override {
        if (param_) {
            param_->setPlain(param_->getDefaultValue());
            if (onValueChanged) onValueChanged(param_->getPlain());
            repaint();
        }
    }

  private:
    Parameter *param_{nullptr};
    std::string title_;
    Color accentCol_{Color::fromHex(0x00F5D4)};
    bool hideValueReadout_{false};
    float dragStartY_{0.0f};
    float dragStartVal_{0.0f};
    std::vector<std::string> customTickLabels_;
    UiLabelPlaqueStyle plaqueStyle_{UiLabelPlaqueStyle::DirectSilkscreen};
};

/**
 * @brief KENNEDARTH TEAL Recessed Backlit Analog VU / Gain Reduction
 * Needle Meter. Features deep inset 4px shadow bezel, dark
 * charcoal-brown face (#1A1816) illuminated by a warm amber
 * radial glow (#D87A18 at 38% opacity) from bottom center, curved
 * white/amber scale arc with sub-ticks, and a razor-sharp
 * anti-aliased orange/red needle (#F06418) with shadow.
 */
class AnalogVuMeter : public Component {
  public:
    AnalogVuMeter() = default;

    void setGainReductionDb(float grDb) noexcept {
        const float target = math::clamp(grDb, 0.0f, 24.0f);
        displayedGrDb_ = displayedGrDb_ * 0.72f + target * 0.28f;
    }

    void setOutputLevelDb(float outDb) noexcept {
        const float target = math::clamp(outDb, -60.0f, 6.0f);
        displayedOutDb_ =
            (target > displayedOutDb_) ? (displayedOutDb_ * 0.55f + target * 0.45f) : (displayedOutDb_ * 0.82f + target * 0.18f);
    }

    static void drawVuVisual(Graphics &g, Rect<float> bounds, float grDb, std::string_view titleText = "GAIN REDUCTION dB",
                             std::string_view brandSub = "AVRA AUDIO", float outDb = -12.0f, Color accentCol = Color::fromHex(0xD87A18),
                             uint32_t customFaceplateHex = 0, bool transparentBezel = false) noexcept {
        if (bounds.width < 110.0f || brandSub.find("CATHODE") != std::string_view::npos ||
            titleText.find("VALVE") != std::string_view::npos || titleText.find("TRIODE") != std::string_view::npos ||
            titleText.find("PENTODE") != std::string_view::npos || titleText.find("12AX7") != std::string_view::npos ||
            titleText.find("6386") != std::string_view::npos || titleText.find("6AS6") != std::string_view::npos) {
            HardwareMaterials::drawGlowingTubeWindow(g, bounds, outDb, grDb, titleText, accentCol);
            return;
        }

        Rect<float> face = bounds.reduced(5.0f);
        bool isLightPaper = false;
        if (customFaceplateHex != 0) {
            const Color cTop = HardwareMaterials::scaleColorHex(customFaceplateHex, 1.05f, 1.0f);
            const float luma = 0.299f * cTop.r + 0.587f * cTop.g + 0.114f * cTop.b;
            isLightPaper = (luma > 0.52f);
        }

        if (!transparentBezel) {
            // 1. Deep Inset Rounded-Rectangle Bezel (4px dark inner
            // shadow simulating glass depth)
            g.fillRoundedRect({bounds.x + 2.0f, bounds.y + 3.0f, bounds.width, bounds.height}, 7.0f, Color::fromHex(0x030608, 0.85f));
            g.fillLinearGradientRect(bounds, Color::fromHex(0x242A30), Color::fromHex(0x0D1014), 7.0f);
            g.drawRoundedRect(bounds, 7.0f, Color::fromHex(0x05080A), 1.8f);
            g.drawLine({bounds.x + 6.0f, bounds.bottom() - 1.0f}, {bounds.right() - 6.0f, bounds.bottom() - 1.0f},
                       Color::fromHex(0x385A62, 0.55f), 1.1f);

            if (customFaceplateHex != 0) {
                const Color cTop = HardwareMaterials::scaleColorHex(customFaceplateHex, 1.05f, 1.0f);
                const Color cBot = HardwareMaterials::scaleColorHex(customFaceplateHex, 0.82f, 1.0f);
                g.fillLinearGradientRect(face, cTop, cBot, 4.5f);
            } else {
                // KENNEDARTH TEAL Warm Backlit Dark Charcoal-Brown Face
                // (#1A1816 to #12100E)
                g.fillLinearGradientRect(face, Color::fromHex(0x161514), Color::fromHex(0x1E1A16), 4.5f);
            }

            // 2. Warm Amber Radial Backlight Glow (#D87A18 at 38%
            // opacity) from bottom center & lower corners
            const Color warmLamp = Color::fromHex(0xD87A18);
            g.fillRadialGradientCircle({face.getCentre().x, face.bottom() + 6.0f}, face.width * 0.62f,
                                       warmLamp.withAlpha(isLightPaper ? 0.22f : 0.38f), warmLamp.withAlpha(0.0f));
            g.fillRadialGradientCircle({face.x + 24.0f, face.bottom()}, face.width * 0.40f, warmLamp.withAlpha(0.24f),
                                       warmLamp.withAlpha(0.0f));
            g.fillRadialGradientCircle({face.right() - 24.0f, face.bottom()}, face.width * 0.40f, warmLamp.withAlpha(0.24f),
                                       warmLamp.withAlpha(0.0f));

            // 4px deep inner glass shadow along top & left of face
            g.fillLinearGradientRect({face.x, face.y, face.width, 6.0f}, Color::fromHex(0x020304, 0.72f), Color::fromHex(0x020304, 0.0f));
            g.drawRoundedRect(face.reduced(1.0f), 4.0f, Color::fromHex(0x040608, 0.80f), 2.2f);
        }

        const Point<float> pivot = {bounds.x + bounds.width * 0.5f, face.bottom() + face.height * 0.18f};
        constexpr float leftAng = -0.60f;
        constexpr float rightAng = +0.60f;
        const float maxArcByWidth = std::max(18.0f, (bounds.width * 0.34f) / std::sin(rightAng));
        const float arcR = std::min(face.height * 0.74f - 6.0f, maxArcByWidth);

        const Color inkCol = isLightPaper ? Color::fromHex(0x1E2226) : Color::fromHex(0xE8ECEE);
        const Color redZoneCol = isLightPaper ? Color::fromHex(0xC82828) : Color::fromHex(0xE88224);

        // 3. Curved Scale Arc + Minor Sub-Ticks + Major Numbers
        g.drawArc(pivot, arcR, leftAng, 0.24f, inkCol.withAlpha(0.88f), 1.3f);
        g.drawArc(pivot, arcR, 0.24f, rightAng, redZoneCol, 1.8f);

        for (int st = 0; st <= 20; ++st) {
            const float pos = static_cast<float>(st) / 20.0f;
            const float a = leftAng + pos * (rightAng - leftAng);
            const float s = std::sin(a), c = std::cos(a);
            const Color subCol = (pos >= 0.70f) ? redZoneCol.withAlpha(0.75f) : inkCol.withAlpha(0.55f);
            g.drawLine({pivot.x + (arcR - 1.0f) * s, pivot.y - (arcR - 1.0f) * c},
                       {pivot.x + (arcR + 2.8f) * s, pivot.y - (arcR + 2.8f) * c}, subCol, 0.9f);
        }

        struct ScaleMark {
            float normPos;
            const char *label;
            bool isOrange;
        };
        constexpr std::array<ScaleMark, 6> kVuMarks = {{{0.00f, "-20", false},
                                                        {0.24f, "-10", false},
                                                        {0.44f, "-6", false},
                                                        {0.62f, "-3", false},
                                                        {0.80f, "0", true},
                                                        {1.00f, "+3", true}}};

        for (const auto &m : kVuMarks) {
            const float a = leftAng + m.normPos * (rightAng - leftAng);
            const float s = std::sin(a), c = std::cos(a);
            const Color col = m.isOrange ? redZoneCol : inkCol;
            g.drawLine({pivot.x + (arcR - 2.0f) * s, pivot.y - (arcR - 2.0f) * c},
                       {pivot.x + (arcR + 5.5f) * s, pivot.y - (arcR + 5.5f) * c}, col, m.isOrange ? 1.8f : 1.3f);
            const float lx = pivot.x + (arcR + 13.0f) * s;
            const float ly = pivot.y - (arcR + 13.0f) * c;
            g.drawText(m.label, {lx - 14.0f, ly - 7.0f, 28.0f, 14.0f}, col, 8.0f, true, true);
        }

        if (grDb > 0.08f) {
            const float grNorm = math::clamp(grDb / 18.0f, 0.0f, 1.0f);
            const float grStartAng = rightAng - grNorm * (rightAng - leftAng);
            g.drawArc(pivot, arcR - 5.0f, grStartAng, rightAng, redZoneCol.withAlpha(0.85f), 2.4f);
        }

        g.drawText(titleText, {face.x + 7.0f, face.bottom() - 15.5f, face.width * 0.56f, 13.0f},
                   isLightPaper ? Color::fromHex(0x2E343A) : Color::fromHex(0xD2D8DC), 7.2f, false, true);

        const std::string dbReadout =
            (outDb <= -58.0f) ? "-OO dB"
                              : (grDb > 0.1f ? std::format("{:+.1f}dB GR {:.1f}", outDb, grDb) : std::format("{:+.1f} dB", outDb));
        g.drawText(dbReadout, {face.right() - 72.0f, face.bottom() - 15.5f, 66.0f, 13.0f},
                   (outDb >= -0.5f) ? Color::fromHex(0xF04848) : (isLightPaper ? Color::fromHex(0x1C2228) : Color::fromHex(0xE08A38)), 7.2f,
                   true, true);

        float needleNorm = 0.0f;
        if (outDb > -52.0f) {
            needleNorm = (outDb <= -14.0f) ? (((outDb + 52.0f) / 38.0f) * 0.80f) : (0.80f + ((outDb + 14.0f) / 18.0f) * 0.20f);
            needleNorm = math::clamp(needleNorm, 0.0f, 1.0f);
        }
        const float needleAngle = leftAng + needleNorm * (rightAng - leftAng);
        const float ns = std::sin(needleAngle), nc = std::cos(needleAngle);
        const float rBase = (pivot.y - (face.bottom() - 1.0f)) / std::max(0.2f, nc);
        const Point<float> needleBase = {pivot.x + rBase * ns, face.bottom() - 1.0f};
        const Point<float> needleTip = {pivot.x + (arcR + 6.5f) * ns, pivot.y - (arcR + 6.5f) * nc};

        g.drawLine({needleBase.x + 1.8f, needleBase.y}, {needleTip.x + 1.8f, needleTip.y + 2.2f},
                   Color::fromHex(0x030507, isLightPaper ? 0.30f : 0.72f), 2.4f);
        g.drawLine(needleBase, needleTip, Color::fromHex(0xF06418), 1.9f);
        (void)accentCol;
        if (!transparentBezel) { g.drawRoundedRect(face, 4.5f, Color::fromHex(0x3E4850, 0.45f), 1.0f); }
    }

    void setPoweredOffMode(bool off) noexcept { poweredOffMode_ = off; }

    void paint(Graphics &g) override {
        if (poweredOffMode_) {
            HardwareMaterials::drawPoweredOffMeterHousing(g, getLocalBounds());
            return;
        }
        drawVuVisual(g, getLocalBounds(), displayedGrDb_, "GAIN REDUCTION dB", "AVRA AUDIO", displayedOutDb_);
    }

  private:
    float displayedGrDb_{0.0f};
    float displayedOutDb_{-12.0f};
    bool poweredOffMode_{false};
};

} // namespace avra::gui