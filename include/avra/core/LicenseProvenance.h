#pragma once

/**
 * @file LicenseProvenance.h
 * @brief Multi-layer Non-Commercial Provenance, Binary Watermark & Tamper Guard for Avra Studio
 *        and all exported VST3, CLAP, and Standalone plugins.
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include "avra/gui/Graphics.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace avra::provenance {

// =============================================================================
// 1. Forensic Binary Provenance Stamp (.rdata Watermark for DMCA Verification)
// =============================================================================
inline constexpr const char kAvraBinaryProvenanceStamp[] =
    "AVRA_PROVENANCE_STAMP_V1::CREATED_WITH_AVRA_STUDIO::"
    "FREE_OPEN_SOURCE_NON_COMMERCIAL_ONLY::"
    "COMMERCIAL_SALE_OR_PAYWALL_STRICTLY_PROHIBITED::"
    "REPO=https://github.com/angryk10-ctrl/Avra";

// Canonical UI attribution strings protected by FNV-1a integrity verification
inline constexpr const char kTopBarBadgeText[] =
    "BUILT WITH AVRA STUDIO \xC2\xB7 FREE NON-COMMERCIAL";

inline constexpr const char kLicensePopupTitle[] =
    "AVRA STUDIO \xE2\x80\x94 FREE NON-COMMERCIAL COMMUNITY PLUGIN";

inline constexpr const char kLicensePopupLine1[] =
    "Built with Avra Studio (https://github.com/angryk10-ctrl/Avra).";

inline constexpr const char kLicensePopupLine2[] =
    "This plugin is 100% FREE for personal, educational, and studio music production.";

inline constexpr const char kLicensePopupLine3[] =
    "Commercial sale, paywalling, or paid bundling of this plugin is strictly prohibited.";

inline constexpr const char kLicensePopupLine4[] =
    "If you paid money for this plugin, request an immediate refund and report the seller.";

// =============================================================================
// 2. Compile-Time & Runtime FNV-1a 64-Bit Integrity Guard
// =============================================================================
[[nodiscard]] constexpr uint64_t fnv1a64(std::string_view sv, uint64_t seed = 0xCBF29CE484222325ULL) noexcept {
    uint64_t hash = seed;
    for (unsigned char c : sv) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

inline constexpr uint64_t kExpectedProvenanceHash =
    fnv1a64(kLicensePopupLine3,
        fnv1a64(kTopBarBadgeText,
            fnv1a64(kAvraBinaryProvenanceStamp)));

/**
 * @brief Ensures the linker retains `kAvraBinaryProvenanceStamp` in `.rdata` across
 *        all exported `.vst3`, `.clap`, and `.exe` binaries.
 */
inline bool keepProvenanceStampAlive() noexcept {
    static volatile const char* s_ref = kAvraBinaryProvenanceStamp;
    return s_ref != nullptr && s_ref[0] == 'A';
}

/**
 * @brief Verifies that the non-commercial attribution strings have not been blanked out
 *        or tampered with in the generated source or binary.
 */
[[nodiscard]] inline bool verifyIntegrity(
    const char* badgeText = kTopBarBadgeText,
    const char* popupLine3 = kLicensePopupLine3) noexcept {
    if (!badgeText || !popupLine3) return false;
    const uint64_t actual =
        fnv1a64(popupLine3,
            fnv1a64(badgeText,
                fnv1a64(kAvraBinaryProvenanceStamp)));
    return actual == kExpectedProvenanceHash;
}

/**
 * @brief Returns `1.0f` when the non-commercial attribution is intact, or `0.0f` (muting audio)
 *        if the attribution strings were modified or stripped.
 */
[[nodiscard]] inline float getIntegrityGainMultiplier(
    const char* badgeText = kTopBarBadgeText,
    const char* popupLine3 = kLicensePopupLine3) noexcept {
    return verifyIntegrity(badgeText, popupLine3) ? 1.0f : 0.0f;
}

// =============================================================================
// 3. One-Time First-Launch Machine Stamp Helper
// =============================================================================
[[nodiscard]] inline std::filesystem::path getFirstLaunchStampPath(std::string_view pluginName) {
    namespace fs = std::filesystem;
    std::string clean;
    clean.reserve(pluginName.size());
    for (char c : pluginName) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            clean.push_back(c);
        }
    }
    if (clean.empty()) clean = "AvraPlugin";

#if defined(_WIN32)
    char appData[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        std::error_code ec;
        fs::path dir = fs::path(appData) / "AvraAudio" / "LicenseNotices";
        fs::create_directories(dir, ec);
        return dir / (clean + "_v2.seen");
    }
#endif
    return fs::temp_directory_path() / ("avra_notice_" + clean + "_v2.seen");
}

[[nodiscard]] inline bool shouldShowFirstLaunchNotice(std::string_view pluginName) {
    std::error_code ec;
    const auto p = getFirstLaunchStampPath(pluginName);
    return !std::filesystem::exists(p, ec);
}

inline void markFirstLaunchNoticeSeen(std::string_view pluginName) {
    std::error_code ec;
    const auto p = getFirstLaunchStampPath(pluginName);
    std::ofstream out(p, std::ios::trunc);
    out << kAvraBinaryProvenanceStamp << "\n";
}

// =============================================================================
// 4. Shared Direct2D Renderer for Top-Bar Pill & Non-Commercial Provenance Modal
// =============================================================================
inline void drawTopBarProvenancePill(
    gui::Graphics& g,
    const gui::Rect<float>& pillRect,
    bool isModalOpen) {
    g.fillRoundedRect(pillRect, 4.0f,
                      isModalOpen ? gui::Color::fromHex(0x142A28) : gui::Color::fromHex(0x141A20));
    g.drawRoundedRect(pillRect, 4.0f,
                      isModalOpen ? gui::Color::fromHex(0x00F5D4) : gui::Color::fromHex(0x00F5D4, 0.65f), 1.3f);
    g.drawText(kTopBarBadgeText, pillRect,
               isModalOpen ? gui::Color::fromHex(0x00F5D4) : gui::Color::fromHex(0xE6F2F8),
               9.5f, true, true);
}

inline void drawProvenanceModalOverlay(
    gui::Graphics& g,
    const gui::Rect<float>& localBounds,
    std::string_view pluginTitle) {
    // Full-window dark scrim rendered in paintOverlay() AFTER all child knobs/switches
    g.fillRect(localBounds, gui::Color(0.02f, 0.03f, 0.05f, 0.90f));

    const float boxW = std::min(580.0f, localBounds.width - 40.0f);
    const float boxH = 244.0f;
    const gui::Rect<float> box = {
        (localBounds.width - boxW) * 0.5f,
        (localBounds.height - boxH) * 0.5f,
        boxW,
        boxH
    };

    // Deep drop shadow + 100% opaque studio slate card
    g.fillRoundedRect({ box.x + 5.0f, box.y + 8.0f, box.width, box.height }, 8.0f, gui::Color(0.0f, 0.0f, 0.0f, 0.80f));
    g.fillRoundedRect(box, 8.0f, gui::Color::fromHex(0x0B1117));
    g.fillRoundedRect({ box.x + 2.0f, box.y + 2.0f, box.width - 4.0f, 58.0f }, 6.0f, gui::Color::fromHex(0x111C26));
    g.drawRoundedRect(box, 8.0f, gui::Color::fromHex(0x00F5D4, 0.92f), 1.8f);

    g.drawText(kLicensePopupTitle,
               { box.x + 18.0f, box.y + 12.0f, box.width - 36.0f, 22.0f },
               gui::Color::fromHex(0x00F5D4), 12.0f, true, true);

    g.drawText(std::string("Plugin: ") + std::string(pluginTitle) + "  \xC2\xB7  Open-Source Freeware",
               { box.x + 18.0f, box.y + 36.0f, box.width - 36.0f, 18.0f },
               gui::Color::fromHex(0xFFB448), 10.2f, true, true);

    g.drawLine({ box.x + 20.0f, box.y + 62.0f }, { box.x + box.width - 20.0f, box.y + 62.0f },
               gui::Color::fromHex(0x263C4C), 1.2f);

    g.drawText(kLicensePopupLine1,
               { box.x + 22.0f, box.y + 74.0f, box.width - 44.0f, 20.0f },
               gui::Color::fromHex(0xF0F6FA), 10.2f, true, false);
    g.drawText(kLicensePopupLine2,
               { box.x + 22.0f, box.y + 98.0f, box.width - 44.0f, 20.0f },
               gui::Color::fromHex(0xF0F6FA), 10.2f, true, false);
    g.drawText(kLicensePopupLine3,
               { box.x + 22.0f, box.y + 122.0f, box.width - 44.0f, 20.0f },
               gui::Color::fromHex(0xFF857A), 10.2f, true, true);
    g.drawText(kLicensePopupLine4,
               { box.x + 22.0f, box.y + 146.0f, box.width - 44.0f, 20.0f },
               gui::Color::fromHex(0xC8D8E4), 9.8f, true, false);

    const gui::Rect<float> okBtn = { box.x + (box.width - 250.0f) * 0.5f, box.y + box.height - 48.0f, 250.0f, 32.0f };
    g.fillRoundedRect(okBtn, 5.0f, gui::Color::fromHex(0x123834));
    g.drawRoundedRect(okBtn, 5.0f, gui::Color::fromHex(0x00F5D4), 1.5f);
    g.drawText("GOT IT \xE2\x80\x94 CONTINUE (FREE USE)", okBtn, gui::Color::fromHex(0x00F5D4), 10.0f, true, true);
}

inline void drawTamperWarningBanner(gui::Graphics& g, const gui::Rect<float>& localBounds) {
    const gui::Rect<float> banner = { 20.0f, localBounds.height * 0.5f - 28.0f, localBounds.width - 40.0f, 56.0f };
    g.fillRoundedRect(banner, 6.0f, gui::Color::fromHex(0x3A0A0E, 0.96f));
    g.drawRoundedRect(banner, 6.0f, gui::Color::fromHex(0xFF4D4D), 2.0f);
    g.drawText("LICENSE INTEGRITY ERROR \xE2\x80\x94 NON-COMMERCIAL ATTRIBUTION TAMPERED (AUDIO MUTED)",
               banner, gui::Color::fromHex(0xFFD6D6), 11.5f, true, true);
}

} // namespace avra::provenance
