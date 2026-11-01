#pragma once

#include "avra/core/AudioDeviceOutput.h"
#include "avra/gui/Geometry.h"
#include "avra/gui/Graphics.h"
#include "avra/studio/SchematicSpec.h"
#include "avra/studio/UiBuilderSpec.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace avra::studio {

// =============================================================================
// 1. UNIFIED 50-STEP COMMAND-PATTERN UNDO / REDO ENGINE (Ctrl+Z / Ctrl+Y)
// =============================================================================

struct ProjectSnapshot {
    std::string                actionLabel{"Initial State"};
    std::vector<SchematicNode> schematicChain{};
    FaceplateLayout            faceplateLayout{};
    float                      globalInputDriveDb{0.0f};
    float                      globalOutputTrimDb{0.0f};
    float                      globalParallelMixPct{100.0f};
    bool                       globalMasterBypass{false};
    int                        selectedSchematicIdx{0};
    int                        selectedWidgetIdx{-1};

    [[nodiscard]] static bool nearlyEqual(float a, float b, float eps = 1e-4f) noexcept {
        return std::abs(a - b) <= eps;
    }

    [[nodiscard]] static bool nodesEqual(const SchematicNode& a, const SchematicNode& b) noexcept {
        if (a.paramValues.size() != b.paramValues.size()) return false;
        for (size_t i = 0; i < a.paramValues.size(); ++i) {
            if (!nearlyEqual(a.paramValues[i], b.paramValues[i])) return false;
        }
        return a.type == b.type &&
               a.isParallel == b.isParallel &&
               a.isBypassed == b.isBypassed &&
               nearlyEqual(a.parallelMixPercent, b.parallelMixPercent) &&
               nearlyEqual(a.param1Val, b.param1Val) &&
               nearlyEqual(a.param2Val, b.param2Val) &&
               nearlyEqual(a.param3Val, b.param3Val) &&
               nearlyEqual(a.param4Val, b.param4Val) &&
               nearlyEqual(a.param5Val, b.param5Val) &&
               a.exposeInternalTrims == b.exposeInternalTrims &&
               a.autoGainLinked == b.autoGainLinked &&
               nearlyEqual(a.trim1Val, b.trim1Val) &&
               nearlyEqual(a.trim2Val, b.trim2Val);
    }

    [[nodiscard]] static bool widgetsEqual(const UiCanvasWidget& a, const UiCanvasWidget& b) noexcept {
        return a.type == b.type &&
               nearlyEqual(a.x, b.x) &&
               nearlyEqual(a.y, b.y) &&
               nearlyEqual(a.w, b.w) &&
               nearlyEqual(a.h, b.h) &&
               a.title == b.title &&
               a.subtitle == b.subtitle &&
               a.scalePreset == b.scalePreset &&
               a.boundStageIdx == b.boundStageIdx &&
               a.boundParamSlot == b.boundParamSlot &&
               nearlyEqual(a.previewNorm, b.previewNorm) &&
               a.accentColor == b.accentColor &&
               a.fontSizePreset == b.fontSizePreset &&
               a.knobRole == b.knobRole &&
               a.macroStageIdx2 == b.macroStageIdx2 &&
               a.macroParamSlot2 == b.macroParamSlot2 &&
               a.customImagePath == b.customImagePath &&
               nearlyEqual(a.rotationDeg, b.rotationDeg) &&
               a.hasWaxMark == b.hasWaxMark &&
               nearlyEqual(a.waxMarkNorm, b.waxMarkNorm);
    }

    [[nodiscard]] static bool themesEqual(const FaceplateTheme& a, const FaceplateTheme& b) noexcept {
        return a.finish == b.finish &&
               a.grain == b.grain &&
               a.cornerScrews == b.cornerScrews &&
               a.showTopToolbar == b.showTopToolbar &&
               a.hardwareStyle == b.hardwareStyle &&
               a.screwStyle == b.screwStyle &&
               a.useCustomFinishColor == b.useCustomFinishColor &&
               a.customFinishHex == b.customFinishHex &&
               a.gridSnapPx == b.gridSnapPx &&
               a.globalKnobType == b.globalKnobType &&
               a.globalSwitchType == b.globalSwitchType &&
               a.globalMeterType == b.globalMeterType &&
               a.globalAccentColor == b.globalAccentColor &&
               a.globalFontSizePreset == b.globalFontSizePreset &&
               a.globalFontFamily == b.globalFontFamily &&
               a.globalFontColor == b.globalFontColor &&
               a.useCustomFontColor == b.useCustomFontColor &&
               a.customFontColorHex == b.customFontColorHex &&
               a.unifyFaceplateTheme == b.unifyFaceplateTheme &&
               nearlyEqual(a.bevelDepthPx, b.bevelDepthPx) &&
               a.panelBorderStyle == b.panelBorderStyle &&
               a.useCustomKnobColors == b.useCustomKnobColors &&
               a.knobBodyHex == b.knobBodyHex &&
               a.knobCapHex == b.knobCapHex &&
               a.knobIndicatorHex == b.knobIndicatorHex &&
               a.knobTickStyle == b.knobTickStyle &&
               a.meterFaceplateHex == b.meterFaceplateHex &&
               a.meterGlowHex == b.meterGlowHex &&
               a.labelPlaqueStyle == b.labelPlaqueStyle &&
               a.brandName == b.brandName &&
               a.pluginTitle == b.pluginTitle &&
               a.subtitleText == b.subtitleText &&
               a.badgeFooterText == b.badgeFooterText &&
               a.styleTemplateIdx == b.styleTemplateIdx;
    }

    [[nodiscard]] bool equalsContent(const ProjectSnapshot& other) const noexcept {
        if (schematicChain.size() != other.schematicChain.size()) return false;
        if (faceplateLayout.widgets.size() != other.faceplateLayout.widgets.size()) return false;
        if (!nearlyEqual(globalInputDriveDb, other.globalInputDriveDb) ||
            !nearlyEqual(globalOutputTrimDb, other.globalOutputTrimDb) ||
            !nearlyEqual(globalParallelMixPct, other.globalParallelMixPct) ||
            globalMasterBypass != other.globalMasterBypass) {
            return false;
        }
        if (!themesEqual(faceplateLayout.theme, other.faceplateLayout.theme)) return false;
        for (size_t i = 0; i < schematicChain.size(); ++i) {
            if (!nodesEqual(schematicChain[i], other.schematicChain[i])) return false;
        }
        for (size_t i = 0; i < faceplateLayout.widgets.size(); ++i) {
            if (!widgetsEqual(faceplateLayout.widgets[i], other.faceplateLayout.widgets[i])) return false;
        }
        return true;
    }
};

class HistoryManager {
public:
    static constexpr size_t kMaxSteps = 50;

    void clear() noexcept {
        undoStack_.clear();
        redoStack_.clear();
    }

    /**
     * @brief Records `stateBeforeAction` onto the 50-step Undo ring buffer and clears Redo history.
     */
    void pushState(ProjectSnapshot stateBeforeAction, std::string actionLabel) {
        stateBeforeAction.actionLabel = std::move(actionLabel);
        undoStack_.push_back(std::move(stateBeforeAction));
        while (undoStack_.size() > kMaxSteps) {
            undoStack_.pop_front();
        }
        redoStack_.clear();
    }

    [[nodiscard]] bool canUndo() const noexcept { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack_.empty(); }

    [[nodiscard]] size_t undoDepth() const noexcept { return undoStack_.size(); }
    [[nodiscard]] size_t redoDepth() const noexcept { return redoStack_.size(); }

    [[nodiscard]] std::string getUndoLabel() const {
        return undoStack_.empty() ? "" : undoStack_.back().actionLabel;
    }

    [[nodiscard]] std::string getRedoLabel() const {
        return redoStack_.empty() ? "" : redoStack_.back().actionLabel;
    }

    bool undo(const ProjectSnapshot& currentLiveState,
              ProjectSnapshot& outRestoredState,
              std::string& outUndoneActionLabel) {
        if (undoStack_.empty()) return false;
        ProjectSnapshot prev = std::move(undoStack_.back());
        undoStack_.pop_back();

        outUndoneActionLabel = prev.actionLabel;
        ProjectSnapshot redoSnap = currentLiveState;
        redoSnap.actionLabel     = outUndoneActionLabel;
        redoStack_.push_back(std::move(redoSnap));
        while (redoStack_.size() > kMaxSteps) {
            redoStack_.pop_front();
        }

        outRestoredState = std::move(prev);
        return true;
    }

    bool redo(const ProjectSnapshot& currentLiveState,
              ProjectSnapshot& outRestoredState,
              std::string& outRedoneActionLabel) {
        if (redoStack_.empty()) return false;
        ProjectSnapshot next = std::move(redoStack_.back());
        redoStack_.pop_back();

        outRedoneActionLabel = next.actionLabel;
        ProjectSnapshot undoSnap = currentLiveState;
        undoSnap.actionLabel     = outRedoneActionLabel;
        undoStack_.push_back(std::move(undoSnap));
        while (undoStack_.size() > kMaxSteps) {
            undoStack_.pop_front();
        }

        outRestoredState = std::move(next);
        return true;
    }

private:
    std::deque<ProjectSnapshot> undoStack_{};
    std::deque<ProjectSnapshot> redoStack_{};
};

// =============================================================================
// 2. STUDIO PREFERENCES / SETTINGS STATE & PERSISTENCE (Ctrl+,)
// =============================================================================

struct StudioPreferences {
    // Tab 0: Audio Engine & I/O
    int audioDriverMode{0};          // 0 = Studio Balanced (6-Buffer), 1 = Ultra-Low-Latency (3-Buffer), 2 = High-Stability (8-Buffer)
    int outputDeviceIdx{0};          // Index into avra::audio_io::enumerateOutputDevices() (0 = System Default WAVE_MAPPER)
    int sampleRateHz{48000};         // 44100, 48000, 88200, 96000
    int blockBufferSize{256};        // 64, 128, 256, 512, 1024
    int globalOversamplingCapIdx{2}; // 0 = Off (1x), 1 = 2x, 2 = 4x, 3 = 8x

    // Tab 1: Compiler & Export Paths + Selectable Export Format Checkboxes
    bool exportFormatVst3{true};       // Default: [✓] VST3 (.vst3) enabled
    bool exportFormatClap{false};      // Default: [ ] CLAP (.clap) disabled
    bool exportFormatStandalone{false};// Default: [ ] Standalone (.exe) disabled
    std::string vst3TargetPath{"C:\\Program Files\\Common Files\\VST3"};
    std::string clapTargetPath{"C:\\Program Files\\Common Files\\CLAP"};
    std::string activeExportFolderDisplay{"Project Default (plugins/<Plugin>/dist/)"};
    std::string vendorName{"AVRA AUDIO"};
    std::string manufacturerCode{"AVRA"};
    std::string clExePath{"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Tools\\MSVC\\bin\\Hostx64\\x64\\cl.exe"};
    std::string cmakeExePath{"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe"};
    std::string vsCodeExePath{"code.exe"};
    int activeUndoCount{0};
    int activeRedoCount{0};

    void ensureAtLeastOneExportFormat() noexcept {
        if (!exportFormatVst3 && !exportFormatClap && !exportFormatStandalone) {
            exportFormatVst3 = true;
        }
    }

    [[nodiscard]] std::string getSelectedFormatsShortList() const {
        std::string out;
        if (exportFormatVst3) {
            out += "VST3";
        }
        if (exportFormatClap) {
            if (!out.empty()) out += " + ";
            out += "CLAP";
        }
        if (exportFormatStandalone) {
            if (!out.empty()) out += " + ";
            out += "APP";
        }
        if (out.empty()) out = "VST3";
        return out;
    }

    [[nodiscard]] std::string getBuildButtonLabel() const {
        return std::format("\xE2\x9A\xA1 BUILD {}", getSelectedFormatsShortList());
    }

    [[nodiscard]] std::string getExportFormatCheckboxesSummary() const {
        return std::format("[{}] VST3 (.vst3)  |  [{}] CLAP (.clap)  |  [{}] Standalone (.exe)",
                           exportFormatVst3 ? "\xE2\x9C\x93" : " ",
                           exportFormatClap ? "\xE2\x9C\x93" : " ",
                           exportFormatStandalone ? "\xE2\x9C\x93" : " ");
    }

    // Tab 2: UI & Workflow Behavior
    int  knobMouseMode{0};           // 0 = Vertical Drag, 1 = Circular Drag, 2 = Fine-Tune Shift Multiplier
    int  autoSaveIntervalIdx{2};     // 0 = Off, 1 = 1 min, 2 = 2 min (Default), 3 = 5 min
    int  defaultGridSizePx{8};       // 0 = Off, 4, 8, 16
    bool showFpsCpuOverlay{true};    // Live FPS & DSP CPU telemetry badge

    [[nodiscard]] const char* getDriverModeName() const noexcept {
        switch (audioDriverMode) {
            case 1:  return "Ultra-Low-Latency (3-Buffer Time-Critical)";
            case 2:  return "High-Stability Mastering (8-Buffer Safe)";
            case 0:
            default: return "Studio Balanced (6-Buffer Event Queue)";
        }
    }

    [[nodiscard]] std::string getOutputDeviceName() const {
        const auto devs = avra::audio_io::enumerateOutputDevices();
        if (devs.empty()) return "System Default Output (WAVE_MAPPER)";
        const size_t idx = (outputDeviceIdx >= 0 && static_cast<size_t>(outputDeviceIdx) < devs.size())
            ? static_cast<size_t>(outputDeviceIdx)
            : 0;
        return devs[idx].name;
    }

    [[nodiscard]] int getOutputDeviceHardwareId() const {
        const auto devs = avra::audio_io::enumerateOutputDevices();
        if (devs.empty()) return -1;
        const size_t idx = (outputDeviceIdx >= 0 && static_cast<size_t>(outputDeviceIdx) < devs.size())
            ? static_cast<size_t>(outputDeviceIdx)
            : 0;
        return devs[idx].deviceId;
    }

    [[nodiscard]] const char* getOversamplingCapName() const noexcept {
        switch (globalOversamplingCapIdx) {
            case 0:  return "Off (1x Native Eco)";
            case 1:  return "2x Polyphase Anti-Aliasing";
            case 3:  return "8x Mastering Grade (Ultra Linear)";
            case 2:
            default: return "4x Studio Standard (ADAA + Polyphase)";
        }
    }

    [[nodiscard]] const char* getKnobMouseModeName() const noexcept {
        switch (knobMouseMode) {
            case 1:  return "Circular / Diagonal Rotary Drag";
            case 2:  return "Precision Fine-Tune (0.25x Velocity)";
            case 0:
            default: return "Linear Vertical Drag (Studio Standard)";
        }
    }

    [[nodiscard]] std::string getGridSnapName() const {
        if (defaultGridSizePx <= 1) return "Off (Freeform Widget Drag)";
        if (defaultGridSizePx == 4) return "4 px Fine Grid";
        if (defaultGridSizePx == 16) return "16 px Coarse Grid";
        return std::format("{} px Studio Grid", defaultGridSizePx);
    }

    [[nodiscard]] const char* getAutoSaveIntervalName() const noexcept {
        switch (autoSaveIntervalIdx) {
            case 0:  return "Off (Manual Ctrl+S Only)";
            case 1:  return "Every 1 Minute (.avra.bak)";
            case 3:  return "Every 5 Minutes (.avra.bak)";
            case 2:
            default: return "Every 2 Minutes (.avra.bak)";
        }
    }

    [[nodiscard]] int getAutoSaveIntervalSeconds() const noexcept {
        switch (autoSaveIntervalIdx) {
            case 0:  return 0;
            case 1:  return 60;
            case 3:  return 300;
            case 2:
            default: return 120;
        }
    }
};

enum class StudioModalSpecialAction {
    None = 0,
    CycleExportOutputFolder,
    VerifyCompilerToolchain,
    ClearUndoRedoHistory
};

inline std::string escapeJsonString(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

inline std::string unescapeJsonString(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            const char next = s[++i];
            switch (next) {
                case '\"': out += '\"'; break;
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                default:   out += next; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

inline bool extractJsonStringField(std::string_view json, std::string_view key, std::string& outVal) {
    const std::string pattern = std::format("\"{}\"", key);
    const size_t kPos = json.find(pattern);
    if (kPos == std::string_view::npos) return false;
    const size_t colonPos = json.find(':', kPos + pattern.size());
    if (colonPos == std::string_view::npos) return false;
    const size_t qStart = json.find('\"', colonPos + 1);
    if (qStart == std::string_view::npos) return false;
    size_t qEnd = qStart + 1;
    while (qEnd < json.size()) {
        if (json[qEnd] == '\"' && json[qEnd - 1] != '\\') break;
        ++qEnd;
    }
    if (qEnd >= json.size()) return false;
    outVal = unescapeJsonString(json.substr(qStart + 1, qEnd - qStart - 1));
    return true;
}

inline bool extractJsonNumberField(std::string_view json, std::string_view key, float& outVal) {
    const std::string pattern = std::format("\"{}\"", key);
    const size_t kPos = json.find(pattern);
    if (kPos == std::string_view::npos) return false;
    const size_t colonPos = json.find(':', kPos + pattern.size());
    if (colonPos == std::string_view::npos) return false;
    size_t vStart = colonPos + 1;
    while (vStart < json.size() && (json[vStart] == ' ' || json[vStart] == '\t' || json[vStart] == '\r' || json[vStart] == '\n')) {
        ++vStart;
    }
    size_t vEnd = vStart;
    while (vEnd < json.size() && ((json[vEnd] >= '0' && json[vEnd] <= '9') || json[vEnd] == '-' || json[vEnd] == '+' || json[vEnd] == '.' || json[vEnd] == 'e' || json[vEnd] == 'E')) {
        ++vEnd;
    }
    if (vEnd == vStart) return false;
    try {
        outVal = std::stof(std::string(json.substr(vStart, vEnd - vStart)));
        return true;
    } catch (...) {
        return false;
    }
}

inline bool extractJsonIntField(std::string_view json, std::string_view key, int& outVal) {
    float f = 0.0f;
    if (!extractJsonNumberField(json, key, f)) return false;
    outVal = static_cast<int>(std::lround(f));
    return true;
}

inline bool extractJsonBoolField(std::string_view json, std::string_view key, bool& outVal) {
    const std::string pattern = std::format("\"{}\"", key);
    const size_t kPos = json.find(pattern);
    if (kPos == std::string_view::npos) return false;
    const size_t colonPos = json.find(':', kPos + pattern.size());
    if (colonPos == std::string_view::npos) return false;
    size_t vStart = colonPos + 1;
    while (vStart < json.size() && (json[vStart] == ' ' || json[vStart] == '\t' || json[vStart] == '\r' || json[vStart] == '\n')) {
        ++vStart;
    }
    if (json.substr(vStart, 4) == "true") {
        outVal = true;
        return true;
    }
    if (json.substr(vStart, 5) == "false") {
        outVal = false;
        return true;
    }
    return false;
}

inline bool saveStudioPreferencesToFile(const std::filesystem::path& filePath, const StudioPreferences& prefs) {
    std::error_code ec;
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path(), ec);
    }
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;

    out << "{\n"
        << "  \"audioDriverMode\": " << prefs.audioDriverMode << ",\n"
        << "  \"outputDeviceIdx\": " << prefs.outputDeviceIdx << ",\n"
        << "  \"sampleRateHz\": " << prefs.sampleRateHz << ",\n"
        << "  \"blockBufferSize\": " << prefs.blockBufferSize << ",\n"
        << "  \"globalOversamplingCapIdx\": " << prefs.globalOversamplingCapIdx << ",\n"
        << "  \"vst3TargetPath\": \"" << escapeJsonString(prefs.vst3TargetPath) << "\",\n"
        << "  \"clapTargetPath\": \"" << escapeJsonString(prefs.clapTargetPath) << "\",\n"
        << "  \"vendorName\": \"" << escapeJsonString(prefs.vendorName) << "\",\n"
        << "  \"manufacturerCode\": \"" << escapeJsonString(prefs.manufacturerCode) << "\",\n"
        << "  \"clExePath\": \"" << escapeJsonString(prefs.clExePath) << "\",\n"
        << "  \"cmakeExePath\": \"" << escapeJsonString(prefs.cmakeExePath) << "\",\n"
        << "  \"vsCodeExePath\": \"" << escapeJsonString(prefs.vsCodeExePath) << "\",\n"
        << "  \"knobMouseMode\": " << prefs.knobMouseMode << ",\n"
        << "  \"autoSaveIntervalIdx\": " << prefs.autoSaveIntervalIdx << ",\n"
        << "  \"defaultGridSizePx\": " << prefs.defaultGridSizePx << ",\n"
        << "  \"showFpsCpuOverlay\": " << (prefs.showFpsCpuOverlay ? "true" : "false") << ",\n"
        << "  \"exportFormatVst3\": " << (prefs.exportFormatVst3 ? "true" : "false") << ",\n"
        << "  \"exportFormatClap\": " << (prefs.exportFormatClap ? "true" : "false") << ",\n"
        << "  \"exportFormatStandalone\": " << (prefs.exportFormatStandalone ? "true" : "false") << "\n"
        << "}\n";
    return true;
}

inline bool loadStudioPreferencesFromFile(const std::filesystem::path& filePath, StudioPreferences& outPrefs) {
    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec)) return false;
    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string json = ss.str();
    if (json.empty()) return false;

    StudioPreferences p = outPrefs;
    extractJsonIntField(json, "audioDriverMode", p.audioDriverMode);
    extractJsonIntField(json, "outputDeviceIdx", p.outputDeviceIdx);
    extractJsonIntField(json, "sampleRateHz", p.sampleRateHz);
    extractJsonIntField(json, "blockBufferSize", p.blockBufferSize);
    extractJsonIntField(json, "globalOversamplingCapIdx", p.globalOversamplingCapIdx);
    extractJsonStringField(json, "vst3TargetPath", p.vst3TargetPath);
    extractJsonStringField(json, "clapTargetPath", p.clapTargetPath);
    extractJsonStringField(json, "vendorName", p.vendorName);
    extractJsonStringField(json, "manufacturerCode", p.manufacturerCode);
    extractJsonStringField(json, "clExePath", p.clExePath);
    extractJsonStringField(json, "cmakeExePath", p.cmakeExePath);
    extractJsonStringField(json, "vsCodeExePath", p.vsCodeExePath);
    extractJsonIntField(json, "knobMouseMode", p.knobMouseMode);
    extractJsonIntField(json, "autoSaveIntervalIdx", p.autoSaveIntervalIdx);
    extractJsonIntField(json, "defaultGridSizePx", p.defaultGridSizePx);
    extractJsonBoolField(json, "showFpsCpuOverlay", p.showFpsCpuOverlay);
    extractJsonBoolField(json, "exportFormatVst3", p.exportFormatVst3);
    extractJsonBoolField(json, "exportFormatClap", p.exportFormatClap);
    extractJsonBoolField(json, "exportFormatStandalone", p.exportFormatStandalone);
    p.vendorName       = "AVRA AUDIO";
    p.manufacturerCode = "AVRA";
    p.ensureAtLeastOneExportFormat();

    outPrefs = std::move(p);
    return true;
}

// =============================================================================
// 3. PERSISTENT `.avra` JSON PROJECT SERIALIZATION & AUTO-SAVE (.avra.bak)
// =============================================================================

inline std::string serializeSchematicChainToPipeLines(const std::vector<SchematicNode>& chain) {
    std::ostringstream out;
    for (const auto& node : chain) {
        const auto& meta = getComponentMeta(node.type);
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
            // retired Component Bay columns 14..21 (neutral placeholders)
            << "0|0|0|15|0.5|-110|1|0\n";
    }
    return out.str();
}

inline std::string serializeFaceplateLayoutToPipeLines(const FaceplateLayout& layout) {
    std::ostringstream out;
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
        << layout.theme.styleTemplateIdx << "\n";

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

/**
 * @brief Serializes the complete project state into a clean JSON `.avra` project file
 * (including all schematic stages, parameter values, internal trim-pot calibrations,
 * faceplate widget coordinates, sticker text/angles, wax marks, and plugin metadata).
 */
inline bool saveProjectToAvraJsonFile(
    const std::filesystem::path& filePath,
    const std::string& projectName,
    const std::vector<SchematicNode>& chain,
    const FaceplateLayout& layout,
    float globalInputDriveDb = 0.0f,
    float globalOutputTrimDb = 0.0f,
    float globalParallelMixPct = 100.0f,
    bool globalMasterBypass = false) {
    std::error_code ec;
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path(), ec);
    }
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) return false;

    const std::string schemLines = serializeSchematicChainToPipeLines(chain);
    const std::string faceLines  = serializeFaceplateLayoutToPipeLines(layout);

    out << "{\n"
        << "  \"format\": \"AvraStudioProject\",\n"
        << "  \"version\": 1,\n"
        << "  \"projectName\": \"" << escapeJsonString(projectName) << "\",\n"
        << "  \"brandName\": \"" << escapeJsonString(layout.theme.brandName) << "\",\n"
        << "  \"pluginTitle\": \"" << escapeJsonString(layout.theme.pluginTitle) << "\",\n"
        << "  \"subtitleText\": \"" << escapeJsonString(layout.theme.subtitleText) << "\",\n"
        << "  \"globalInputDriveDb\": " << globalInputDriveDb << ",\n"
        << "  \"globalOutputTrimDb\": " << globalOutputTrimDb << ",\n"
        << "  \"globalParallelMixPct\": " << globalParallelMixPct << ",\n"
        << "  \"globalMasterBypass\": " << (globalMasterBypass ? "true" : "false") << ",\n"
        << "  \"stageCount\": " << chain.size() << ",\n"
        << "  \"widgetCount\": " << layout.widgets.size() << ",\n"
        << "  \"schematicStages\": [\n";

    for (size_t i = 0; i < chain.size(); ++i) {
        const auto& st   = chain[i];
        const auto& meta = getComponentMeta(st.type);
        out << "    {\n"
            << "      \"index\": " << i << ",\n"
            << "      \"designator\": \"" << escapeJsonString(meta.designator) << "\",\n"
            << "      \"name\": \"" << escapeJsonString(meta.shortName) << "\",\n"
            << "      \"typeId\": " << static_cast<int>(st.type) << ",\n"
            << "      \"isParallel\": " << (st.isParallel ? "true" : "false") << ",\n"
            << "      \"isBypassed\": " << (st.isBypassed ? "true" : "false") << ",\n"
            << "      \"parallelMixPercent\": " << st.parallelMixPercent << ",\n"
            << "      \"params\": [" << st.param1Val << ", " << st.param2Val << ", " << st.param3Val << ", " << st.param4Val << ", " << st.param5Val << "],\n"
            << "      \"trims\": [" << st.trim1Val << ", " << st.trim2Val << "]\n"
            << "    }" << (i + 1 < chain.size() ? "," : "") << "\n";
    }

    out << "  ],\n"
        << "  \"faceplateWidgets\": [\n";

    for (size_t i = 0; i < layout.widgets.size(); ++i) {
        const auto& w = layout.widgets[i];
        out << "    {\n"
            << "      \"index\": " << i << ",\n"
            << "      \"typeId\": " << static_cast<int>(w.type) << ",\n"
            << "      \"title\": \"" << escapeJsonString(w.title) << "\",\n"
            << "      \"subtitle\": \"" << escapeJsonString(w.subtitle) << "\",\n"
            << "      \"bounds\": [" << w.x << ", " << w.y << ", " << w.w << ", " << w.h << "],\n"
            << "      \"boundStageIdx\": " << w.boundStageIdx << ",\n"
            << "      \"boundParamSlot\": " << w.boundParamSlot << ",\n"
            << "      \"rotationDeg\": " << w.rotationDeg << ",\n"
            << "      \"hasWaxMark\": " << (w.hasWaxMark ? "true" : "false") << ",\n"
            << "      \"waxMarkNorm\": " << w.waxMarkNorm << "\n"
            << "    }" << (i + 1 < layout.widgets.size() ? "," : "") << "\n";
    }

    out << "  ],\n"
        << "  \"schematicPayload\": \"" << escapeJsonString(schemLines) << "\",\n"
        << "  \"faceplatePayload\": \"" << escapeJsonString(faceLines) << "\"\n"
        << "}\n";
    return true;
}

inline bool loadProjectFromAvraJsonFile(
    const std::filesystem::path& filePath,
    std::string& outProjectName,
    std::vector<SchematicNode>& outChain,
    FaceplateLayout& outLayout,
    float* outGlobalInputDriveDb = nullptr,
    float* outGlobalOutputTrimDb = nullptr,
    float* outGlobalParallelMixPct = nullptr,
    bool* outGlobalMasterBypass = nullptr) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(filePath, ec)) return false;

    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string json = ss.str();
    if (json.empty()) return false;

    std::string projName;
    if (extractJsonStringField(json, "projectName", projName) && !projName.empty()) {
        outProjectName = projName;
    }

    if (outGlobalInputDriveDb)   extractJsonNumberField(json, "globalInputDriveDb", *outGlobalInputDriveDb);
    if (outGlobalOutputTrimDb)   extractJsonNumberField(json, "globalOutputTrimDb", *outGlobalOutputTrimDb);
    if (outGlobalParallelMixPct) extractJsonNumberField(json, "globalParallelMixPct", *outGlobalParallelMixPct);
    if (outGlobalMasterBypass)   extractJsonBoolField(json, "globalMasterBypass", *outGlobalMasterBypass);

    std::string schemPayload;
    std::string facePayload;
    const bool hasSchem = extractJsonStringField(json, "schematicPayload", schemPayload);
    const bool hasFace  = extractJsonStringField(json, "faceplatePayload", facePayload);
    if (!hasSchem && !hasFace) return false;

    // Write temporary in-memory stream via temp file or parse directly using the existing loaders
    const fs::path tempDir = fs::temp_directory_path(ec);
    bool loadedOk = false;

    if (hasSchem && !schemPayload.empty()) {
        const fs::path tmpSchem = tempDir / "avra_tmp_load.schematic";
        {
            std::ofstream tmpOut(tmpSchem, std::ios::trunc);
            tmpOut << "# Avra Studio Hardware Schematic Topology v5\n" << schemPayload;
        }
        std::vector<SchematicNode> parsedChain;
        if (loadSchematicChainFromFile(tmpSchem.string(), parsedChain)) {
            outChain = std::move(parsedChain);
            loadedOk = true;
        }
        fs::remove(tmpSchem, ec);
    }

    if (hasFace && !facePayload.empty()) {
        const fs::path tmpFace = tempDir / "avra_tmp_load.faceplate";
        {
            std::ofstream tmpOut(tmpFace, std::ios::trunc);
            tmpOut << "# Avra Studio Hardware Faceplate UI Layout v7\n" << facePayload;
        }
        FaceplateLayout parsedLayout;
        if (loadFaceplateLayoutFromFile(tmpFace, parsedLayout)) {
            outLayout = std::move(parsedLayout);
            loadedOk = true;
        }
        fs::remove(tmpFace, ec);
    }

    if (loadedOk) {
        if (ensureMultiStageActiveBayLayout(outLayout, outChain)) {
            const float inDb  = outGlobalInputDriveDb   ? *outGlobalInputDriveDb   : 0.0f;
            const float outDb = outGlobalOutputTrimDb   ? *outGlobalOutputTrimDb   : 0.0f;
            const float mix   = outGlobalParallelMixPct ? *outGlobalParallelMixPct : 100.0f;
            const bool  byp   = outGlobalMasterBypass   ? *outGlobalMasterBypass   : false;
            saveProjectToAvraJsonFile(filePath, outProjectName, outChain, outLayout, inDb, outDb, mix, byp);
        }
    }

    return loadedOk;
}

inline std::vector<std::string> loadRecentProjectsList(const std::filesystem::path& wsRoot) {
    std::vector<std::string> list;
    const auto path = wsRoot / ".avra_recent_projects.txt";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) return list;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) list.push_back(line);
        if (list.size() >= 8) break;
    }
    return list;
}

inline void recordRecentProjectPath(const std::filesystem::path& wsRoot, const std::string& projectEntry) {
    if (projectEntry.empty()) return;
    std::vector<std::string> list = loadRecentProjectsList(wsRoot);
    list.erase(std::remove(list.begin(), list.end(), projectEntry), list.end());
    list.insert(list.begin(), projectEntry);
    if (list.size() > 8) list.resize(8);

    const auto path = wsRoot / ".avra_recent_projects.txt";
    std::ofstream out(path, std::ios::trunc);
    for (const auto& item : list) {
        out << item << "\n";
    }
}

// =============================================================================
// 4. INTEGRATED TOP-LEFT STUDIO MENU BAR, DROPDOWNS & MODAL OVERLAYS
// =============================================================================

enum class TopMenuBarSection : int {
    None  = -1,
    File  = 0,
    Edit  = 1,
    View  = 2,
    Build = 3,
    Help  = 4
};

enum class ActiveStudioModal : int {
    None                    = 0,
    Preferences             = 1,
    ShortcutsCheatSheet     = 2,
    SchematicReferenceGuide = 3,
    AboutAndDiagnostics     = 4
};

enum class StudioMenuCommandId : int {
    None = 0,
    // FILE
    File_NewProject,
    File_OpenProject,
    File_OpenRecentSubmenu,   // parent row "Open Recent  >" (opens the flyout, never closes the menu)
    File_OpenRecent0,         // flyout entries 0..7 (matches loadRecentProjectsList() cap of 8)
    File_OpenRecent1,
    File_OpenRecent2,
    File_OpenRecent3,
    File_OpenRecent4,
    File_OpenRecent5,
    File_OpenRecent6,
    File_OpenRecent7,
    File_SaveProject,
    File_SaveProjectAs,
    File_OpenInIde,
    File_ImportExportBlueprint,
    File_ExportFaceplatePng,
    File_Preferences,
    File_ExitStudio,

    // EDIT
    Edit_Undo,
    Edit_Redo,
    Edit_DuplicateSelected,
    Edit_CopyStyle,
    Edit_PasteStyle,
    Edit_DeleteSelected,
    Edit_SelectAllWidgets,
    Edit_ResetAllParamsToUnity,

    // VIEW
    View_ToggleProjectDrawer,
    View_SwitchToSchematic,
    View_SwitchToFaceplate,
    View_SwitchToLiveRack,
    View_SwitchToBuild,
    View_CycleGridSnap,
    View_ToggleFullscreen,

    // BUILD & AUDIO
    Build_CompileAllNow,
    Build_ChooseOutputFolder,
    Build_ToggleFormatVst3,
    Build_ToggleFormatClap,
    Build_ToggleFormatStandalone,
    Build_RunForensicDspAudit,
    Build_OpenInVsCode,
    Build_OpenInIde,
    Build_OpenFolderInExplorer,
    Build_AudioDeviceSettings,
    Build_PanicHardMute,

    // HELP
    Help_ShortcutsCheatSheet,
    Help_SchematicReferenceGuide,
    Help_AboutAndDiagnostics
};

struct StudioDropdownMenuItem {
    StudioMenuCommandId id{StudioMenuCommandId::None};
    std::string         label{};
    std::string         shortcut{};
    bool                enabled{true};
    bool                isSeparatorAfter{false};
    bool                hasSubmenu{false};
};

[[nodiscard]] constexpr StudioMenuCommandId recentProjectCommandForIndex(size_t idx) noexcept {
    return (idx < 8)
        ? static_cast<StudioMenuCommandId>(static_cast<int>(StudioMenuCommandId::File_OpenRecent0) + static_cast<int>(idx))
        : StudioMenuCommandId::None;
}

[[nodiscard]] constexpr int recentProjectIndexForCommand(StudioMenuCommandId cmd) noexcept {
    const int v = static_cast<int>(cmd) - static_cast<int>(StudioMenuCommandId::File_OpenRecent0);
    return (v >= 0 && v < 8) ? v : -1;
}

struct TopHeaderMenuBarGeometry {
    static constexpr float kMenuBarStartX   = 144.0f;
    static constexpr float kMenuItemW       = 42.0f;
    static constexpr float kMenuItemH       = 30.0f;
    static constexpr float kMenuItemY       = 10.0f;
    static constexpr int   kMenuButtonCount = 5;   // FILE, EDIT, VIEW, BUILD, HELP
    static constexpr float kQuickIconsX     = kMenuBarStartX + kMenuButtonCount * kMenuItemW + 6.0f; // 360
    static constexpr float kQuickIconW      = 28.0f;
    static constexpr float kQuickIconGap    = 3.0f;

    // "Open Recent" flyout submenu geometry
    static constexpr float kSubmenuW        = 280.0f;
    static constexpr float kMenuRowPitch    = 28.0f;
    static constexpr float kMenuRowH        = 26.0f;

    [[nodiscard]] static constexpr TopMenuBarSection getSectionForButtonIndex(int menuIdx) noexcept {
        switch (menuIdx) {
            case 0: return TopMenuBarSection::File;
            case 1: return TopMenuBarSection::Edit;
            case 2: return TopMenuBarSection::View;
            case 3: return TopMenuBarSection::Build;
            case 4: return TopMenuBarSection::Help;
            default: return TopMenuBarSection::None;
        }
    }

    [[nodiscard]] static constexpr int getButtonIndexForSection(TopMenuBarSection section) noexcept {
        switch (section) {
            case TopMenuBarSection::File:  return 0;
            case TopMenuBarSection::Edit:  return 1;
            case TopMenuBarSection::View:  return 2;
            case TopMenuBarSection::Build: return 3;
            case TopMenuBarSection::Help:  return 4;
            default: return -1;
        }
    }

    [[nodiscard]] static gui::Rect<float> getMenuButtonRect(int menuIdx) noexcept {
        return {
            kMenuBarStartX + static_cast<float>(menuIdx) * kMenuItemW,
            kMenuItemY,
            kMenuItemW - 2.0f,
            kMenuItemH
        };
    }

    [[nodiscard]] static gui::Rect<float> getQuickIconRect(int iconIdx) noexcept {
        // iconIdx: 0 = Undo [↶], 1 = Redo [↷]
        if (iconIdx < 0 || iconIdx >= 2) return {};
        return {
            kQuickIconsX + static_cast<float>(iconIdx) * (kQuickIconW + kQuickIconGap),
            kMenuItemY,
            kQuickIconW,
            kMenuItemH
        };
    }

    [[nodiscard]] static gui::Rect<float> getDropdownPanelRect(TopMenuBarSection section, size_t itemCount) noexcept {
        if (section == TopMenuBarSection::None || itemCount == 0) return {};
        const int btnIdx = std::max(0, getButtonIndexForSection(section));
        const float x = kMenuBarStartX + static_cast<float>(btnIdx) * kMenuItemW;
        const float y = 44.0f;
        const float w = 318.0f;
        const float h = static_cast<float>(itemCount) * 28.0f + 12.0f;
        return { x, y, w, h };
    }

    // Row rect for item `rowIdx` inside a dropdown panel (identical maths to paint/hit-test).
    [[nodiscard]] static gui::Rect<float> getDropdownRowRect(const gui::Rect<float>& panel, size_t rowIdx) noexcept {
        return { panel.x + 5.0f, panel.y + 6.0f + static_cast<float>(rowIdx) * kMenuRowPitch, panel.width - 10.0f, kMenuRowH };
    }

    // Flyout panel: flush to the right edge of the FILE dropdown, top aligned with the "Open Recent" row.
    [[nodiscard]] static gui::Rect<float> getRecentSubmenuRect(const gui::Rect<float>& panel,
                                                              const gui::Rect<float>& parentRow,
                                                              size_t entryCount) noexcept {
        const size_t rows = std::max<size_t>(1, entryCount);
        return { panel.right() + 2.0f, parentRow.y, kSubmenuW, static_cast<float>(rows) * kMenuRowPitch + 12.0f };
    }

    [[nodiscard]] static gui::Rect<float> getRecentSubmenuRowRect(const gui::Rect<float>& sub, size_t rowIdx) noexcept {
        return { sub.x + 5.0f, sub.y + 6.0f + static_cast<float>(rowIdx) * kMenuRowPitch, sub.width - 10.0f, kMenuRowH };
    }
};

inline std::vector<StudioDropdownMenuItem> buildDropdownMenuItems(
    TopMenuBarSection section,
    const HistoryManager& history,
    const std::vector<std::string>& recentProjects,
    int currentGridSnapPx,
    bool hasClipboardStyle,
    const StudioPreferences* prefs = nullptr) {
    std::vector<StudioDropdownMenuItem> items;
    switch (section) {
        case TopMenuBarSection::File: {
            items.push_back({ StudioMenuCommandId::File_NewProject,  "New Plugin Project...", "Ctrl+N", true, false });
            items.push_back({ StudioMenuCommandId::File_OpenProject, "Open Project (.avra)...", "Ctrl+O", true, false });
            // Single parent row; the individual recent projects live in the hover flyout.
            (void)recentProjects;
            items.push_back({ StudioMenuCommandId::File_OpenRecentSubmenu, "Open Recent", ">", true, true, true });
            items.push_back({ StudioMenuCommandId::File_SaveProject,           "Save Project (.avra)",                     "Ctrl+S",       true, false });
            items.push_back({ StudioMenuCommandId::File_SaveProjectAs,         "Save Project As (.avra)...",               "Ctrl+Shift+S", true, false });
            items.push_back({ StudioMenuCommandId::File_OpenInIde,             "Open Project in IDE...",                   "Ctrl+Shift+O", true, true  });
            items.push_back({ StudioMenuCommandId::File_ImportExportBlueprint, "Export Stage Blueprint (.avrachain)...",   "",             true, false });
            items.push_back({ StudioMenuCommandId::File_ExportFaceplatePng,    "Export Faceplate Snapshot (.png)...",      "",             true, true  });
            items.push_back({ StudioMenuCommandId::File_Preferences,           "Preferences / Studio Settings...",         "Ctrl+,",       true, true  });
            items.push_back({ StudioMenuCommandId::File_ExitStudio,            "Exit Avra Studio",                         "Alt+F4",       true, false });
            break;
        }

        case TopMenuBarSection::Edit: {
            const std::string undoText = history.canUndo()
                ? std::format("Undo: {}", history.getUndoLabel())
                : "Undo (Nothing to Undo)";
            const std::string redoText = history.canRedo()
                ? std::format("Redo: {}", history.getRedoLabel())
                : "Redo (Nothing to Redo)";
            items.push_back({ StudioMenuCommandId::Edit_Undo,                  undoText,                                  "Ctrl+Z",       history.canUndo(), false });
            items.push_back({ StudioMenuCommandId::Edit_Redo,                  redoText,                                  "Ctrl+Y",       history.canRedo(), true  });
            items.push_back({ StudioMenuCommandId::Edit_DuplicateSelected,     "Duplicate Selected Stage / Widget",       "Ctrl+D",       true,              false });
            items.push_back({ StudioMenuCommandId::Edit_CopyStyle,             "Copy Widget / Theme Style",               "Ctrl+Shift+C", true,              false });
            items.push_back({ StudioMenuCommandId::Edit_PasteStyle,            "Paste Widget / Theme Style",              "Ctrl+Shift+V", hasClipboardStyle, true  });
            items.push_back({ StudioMenuCommandId::Edit_DeleteSelected,        "Delete Selected Stage / Widget",          "Del",          true,              false });
            items.push_back({ StudioMenuCommandId::Edit_SelectAllWidgets,      "Select All Faceplate Widgets",            "Ctrl+A",       true,              true  });
            items.push_back({ StudioMenuCommandId::Edit_ResetAllParamsToUnity, "Reset All Parameters to Unity / Default", "Ctrl+R",       true,              false });
            break;
        }

        case TopMenuBarSection::View: {
            const std::string gridLabel = (currentGridSnapPx <= 1)
                ? "Toggle Grid Snap (Current: Off)"
                : std::format("Toggle Grid Snap (Current: {}px)", currentGridSnapPx);
            items.push_back({ StudioMenuCommandId::View_ToggleProjectDrawer, "Toggle Left Project Drawer",           "Ctrl+B", true, true  });
            items.push_back({ StudioMenuCommandId::View_SwitchToSchematic,   "Switch to Schematic Builder",          "F1",     true, false });
            items.push_back({ StudioMenuCommandId::View_SwitchToFaceplate,   "Switch to Faceplate UI Builder",       "F2",     true, false });
            items.push_back({ StudioMenuCommandId::View_SwitchToLiveRack,    "Switch to Live Test Rack",             "F3",     true, false });
            items.push_back({ StudioMenuCommandId::View_SwitchToBuild,       "Switch to Build & Export Center",      "F4",     true, true  });
            items.push_back({ StudioMenuCommandId::View_CycleGridSnap,       gridLabel,                              "Ctrl+G", true, false });
            items.push_back({ StudioMenuCommandId::View_ToggleFullscreen,    "Fullscreen Studio Mode",               "F11",    true, true  });
            items.push_back({ StudioMenuCommandId::Build_AudioDeviceSettings, "Audio Device & Buffer Settings...",   "Ctrl+,", true, false });
            items.push_back({ StudioMenuCommandId::Build_PanicHardMute,      "Panic / Hard Mute Audio Engine",       "Esc",    true, false });
            break;
        }

        case TopMenuBarSection::Build: {
            const bool fVst3 = prefs ? prefs->exportFormatVst3 : true;
            const bool fClap = prefs ? prefs->exportFormatClap : false;
            const bool fApp  = prefs ? prefs->exportFormatStandalone : false;
            items.push_back({ StudioMenuCommandId::Build_CompileAllNow,          "Build & Export Center (Configure Formats & Path)...",                      "F7",     true, false });
            items.push_back({ StudioMenuCommandId::Build_ChooseOutputFolder,     "Choose Build Output Path (Browse Folder)...",                              "",       true, true  });
            items.push_back({ StudioMenuCommandId::Build_ToggleFormatVst3,       std::format("[{}] Export Target: VST3 (.vst3)", fVst3 ? "\xE2\x9C\x93" : " "),      "",       true, false });
            items.push_back({ StudioMenuCommandId::Build_ToggleFormatClap,       std::format("[{}] Export Target: CLAP (.clap)", fClap ? "\xE2\x9C\x93" : " "),      "",       true, false });
            items.push_back({ StudioMenuCommandId::Build_ToggleFormatStandalone, std::format("[{}] Export Target: Standalone (.exe)", fApp ? "\xE2\x9C\x93" : " "),  "",       true, true  });
            items.push_back({ StudioMenuCommandId::Build_OpenInIde,              "Open Project in IDE",                                                      "Ctrl+Shift+O", true, false });
            items.push_back({ StudioMenuCommandId::Build_OpenFolderInExplorer,   "Open Output / Project Folder in Explorer",                                "",       true, true  });
            items.push_back({ StudioMenuCommandId::Build_RunForensicDspAudit,    "Run Forensic DSP & Dead-Knob Audit",                                      "F8",     true, true  });
            items.push_back({ StudioMenuCommandId::Build_AudioDeviceSettings,    "Audio Device & Buffer Settings...",                                       "Ctrl+,", true, true  });
            items.push_back({ StudioMenuCommandId::Build_PanicHardMute,          "Panic / Hard Mute Audio Engine",                                          "Esc",    true, false });
            break;
        }

        case TopMenuBarSection::Help: {
            items.push_back({ StudioMenuCommandId::Help_ShortcutsCheatSheet,     "Keyboard Shortcuts Cheat-Sheet Modal",   "F12 / ?", true, false });
            items.push_back({ StudioMenuCommandId::Help_SchematicReferenceGuide, "Schematic Component Reference Guide",    "",        true, true  });
            items.push_back({ StudioMenuCommandId::Help_AboutAndDiagnostics,     "About Avra Studio & Toolchain Status",   "",        true, false });
            break;
        }

        case TopMenuBarSection::None:
        default:
            break;
    }
    return items;
}

inline void paintIntegratedMenuBarAndQuickIcons(
    gui::Graphics& g,
    TopMenuBarSection activeSection,
    int hoveredMenuIdx,
    int hoveredQuickIconIdx,
    bool isProjectDirty,
    const HistoryManager& history) {
    using namespace avra::gui;
    static constexpr const char* kMenuNames[TopHeaderMenuBarGeometry::kMenuButtonCount] = { "FILE", "EDIT", "VIEW", "BUILD", "HELP" };

    for (int i = 0; i < TopHeaderMenuBarGeometry::kMenuButtonCount; ++i) {
        const Rect<float> r = TopHeaderMenuBarGeometry::getMenuButtonRect(i);
        const bool isOpen   = (activeSection == TopHeaderMenuBarGeometry::getSectionForButtonIndex(i));
        const bool isHover  = (hoveredMenuIdx == i);

        if (isOpen) {
            g.fillRoundedRect(r, 4.0f, Color::fromHex(0x182A34));
            g.drawRoundedRect(r, 4.0f, Color::fromHex(0x00F5D4), 1.3f);
        } else if (isHover) {
            g.fillRoundedRect(r, 4.0f, Color::fromHex(0x16222B));
            g.drawRoundedRect(r, 4.0f, Color::fromHex(0x365062), 1.0f);
        } else {
            g.fillRoundedRect(r, 4.0f, Color::fromHex(0x10171D));
            g.drawRoundedRect(r, 4.0f, Color::fromHex(0x23313C), 1.0f);
        }

        const Color textCol = isOpen
            ? Color::fromHex(0x00F5D4)
            : (isHover ? Color::fromHex(0xF0F6FA) : Color::fromHex(0xB8C7D4));
        g.drawText(kMenuNames[i], r, textCol, 9.5f, true, true);
    }

    // 2 Quick-Action Icons: [↶] Undo, [↷] Redo
    (void)isProjectDirty;
    for (int idx = 0; idx < 2; ++idx) {
        const Rect<float> r = TopHeaderMenuBarGeometry::getQuickIconRect(idx);
        const bool isHover  = (hoveredQuickIconIdx == idx);
        const bool enabled  = (idx == 0) ? history.canUndo() : history.canRedo();

        const Color bgCol = isHover && enabled
            ? Color::fromHex(0x192B36)
            : Color::fromHex(0x0E151B);
        const Color borderCol = (isHover && enabled)
            ? Color::fromHex(0x00F5D4)
            : Color::fromHex(0x243340);

        g.fillRoundedRect(r, 4.0f, bgCol);
        g.drawRoundedRect(r, 4.0f, borderCol, 1.0f);

        const Color ic = enabled ? Color::fromHex(0xE8F1F2) : Color::fromHex(0x4A5A66);
        if (idx == 0) {
            g.drawText("\xE2\x86\xB6", r, ic, 13.0f, true, true);
        } else {
            g.drawText("\xE2\x86\xB7", r, ic, 13.0f, true, true);
        }
    }
}

// Clean flyout label for a recent .avra project path, e.g.
//   "C:\Avra\plugins\MasteringProcessor\MasteringProcessor.avra"
//     -> "MasteringProcessor  —  plugins/MasteringProcessor"
[[nodiscard]] inline std::string formatRecentProjectFlyoutLabel(const std::string& rawPath) {
    const std::filesystem::path p(rawPath);
    const std::string stem = p.stem().string().empty() ? rawPath : p.stem().string();
    const auto folder = p.parent_path();
    std::string where = folder.filename().string();
    const std::string parentName = folder.parent_path().filename().string();
    if (!parentName.empty()) where = parentName + "/" + where;
    std::string label = where.empty() ? std::format("{} (.avra)", stem)
                                      : std::format("{}  \xE2\x80\x94  {}", stem, where);
    constexpr size_t kMaxChars = 40; // fits the 280 px flyout at the 10 pt menu font
    if (label.size() > kMaxChars) {
        label = std::format("{} (.avra)", stem);
        if (label.size() > kMaxChars) label = label.substr(0, kMaxChars - 3) + "...";
    }
    return label;
}

// Index of the row that owns the flyout (the "Open Recent" row), or -1.
[[nodiscard]] inline int findSubmenuParentRow(const std::vector<StudioDropdownMenuItem>& items) noexcept {
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].hasSubmenu) return static_cast<int>(i);
    }
    return -1;
}

// Zone that keeps the flyout open: the parent row widened rightward across the
// 7 px gap to the flyout, so moving the mouse sideways never drops the submenu.
[[nodiscard]] inline gui::Rect<float> getRecentSubmenuKeepOpenZone(TopMenuBarSection activeSection,
                                                                  const std::vector<StudioDropdownMenuItem>& items) noexcept {
    const int parentIdx = findSubmenuParentRow(items);
    if (activeSection != TopMenuBarSection::File || parentIdx < 0) return {};
    const auto panel = TopHeaderMenuBarGeometry::getDropdownPanelRect(activeSection, items.size());
    const auto row   = TopHeaderMenuBarGeometry::getDropdownRowRect(panel, static_cast<size_t>(parentIdx));
    return { row.x, row.y, (panel.right() + 3.0f) - row.x, row.height };
}

[[nodiscard]] inline gui::Rect<float> getActiveRecentSubmenuRect(TopMenuBarSection activeSection,
                                                                const std::vector<StudioDropdownMenuItem>& items,
                                                                size_t recentCount) noexcept {
    const int parentIdx = findSubmenuParentRow(items);
    if (activeSection != TopMenuBarSection::File || parentIdx < 0) return {};
    const auto panel = TopHeaderMenuBarGeometry::getDropdownPanelRect(activeSection, items.size());
    const auto row   = TopHeaderMenuBarGeometry::getDropdownRowRect(panel, static_cast<size_t>(parentIdx));
    return TopHeaderMenuBarGeometry::getRecentSubmenuRect(panel, row, std::min<size_t>(recentCount, 8));
}

inline void paintRecentProjectsSubmenu(
    gui::Graphics& g,
    const gui::Rect<float>& sub,
    const std::vector<std::string>& recentProjects,
    gui::Point<float> mousePos) {
    using namespace avra::gui;
    g.fillRoundedRect({ sub.x + 4.0f, sub.y + 6.0f, sub.width, sub.height }, 6.0f, Color::fromHex(0x000000, 0.65f));
    g.fillRoundedRect(sub, 6.0f, Color::fromHex(0x0D141B, 0.98f));
    g.drawRoundedRect(sub, 6.0f, Color::fromHex(0x00F5D4, 0.75f), 1.4f);

    if (recentProjects.empty()) {
        const auto row = TopHeaderMenuBarGeometry::getRecentSubmenuRowRect(sub, 0);
        g.drawText("(No Recent Projects)", { row.x + 10.0f, row.y, row.width - 20.0f, row.height },
                   Color::fromHex(0x4E5E6A), 10.0f, false, true);
        return;
    }
    const size_t n = std::min<size_t>(recentProjects.size(), 8);
    for (size_t i = 0; i < n; ++i) {
        const auto row = TopHeaderMenuBarGeometry::getRecentSubmenuRowRect(sub, i);
        const bool isHover = row.contains(mousePos);
        if (isHover) {
            g.fillRoundedRect(row, 4.0f, Color::fromHex(0x18313E));
            g.drawRoundedRect(row, 4.0f, Color::fromHex(0x00F5D4, 0.55f), 1.0f);
        }
        g.drawText(formatRecentProjectFlyoutLabel(recentProjects[i]),
                   { row.x + 10.0f, row.y, row.width - 20.0f, row.height },
                   isHover ? Color::fromHex(0x00F5D4) : Color::fromHex(0xE6EEF4), 10.0f, false, true);
    }
}

// Returns the recent-project index under `pt`, or -1 (empty list / disabled placeholder / miss).
[[nodiscard]] inline int hitTestRecentProjectsSubmenu(const gui::Rect<float>& sub,
                                                      size_t recentCount,
                                                      gui::Point<float> pt) noexcept {
    if (!sub.contains(pt)) return -1;
    const size_t n = std::min<size_t>(recentCount, 8);
    for (size_t i = 0; i < n; ++i) {
        if (TopHeaderMenuBarGeometry::getRecentSubmenuRowRect(sub, i).contains(pt)) return static_cast<int>(i);
    }
    return -1;
}

inline void paintDropdownMenuOverlay(
    gui::Graphics& g,
    TopMenuBarSection activeSection,
    const std::vector<StudioDropdownMenuItem>& items,
    gui::Point<float> mousePos,
    const std::vector<std::string>* recentProjects = nullptr,
    bool recentSubmenuOpen = false) {
    using namespace avra::gui;
    if (activeSection == TopMenuBarSection::None || items.empty()) return;

    const Rect<float> panel = TopHeaderMenuBarGeometry::getDropdownPanelRect(activeSection, items.size());

    // Drop shadow + dark glass slate menu panel
    g.fillRoundedRect({ panel.x + 4.0f, panel.y + 6.0f, panel.width, panel.height }, 6.0f, Color::fromHex(0x000000, 0.65f));
    g.fillRoundedRect(panel, 6.0f, Color::fromHex(0x0D141B, 0.98f));
    g.drawRoundedRect(panel, 6.0f, Color::fromHex(0x00F5D4, 0.75f), 1.4f);

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        const Rect<float> rowRect = TopHeaderMenuBarGeometry::getDropdownRowRect(panel, i);
        const bool isHover = item.enabled &&
            (rowRect.contains(mousePos) || (item.hasSubmenu && recentSubmenuOpen));

        if (isHover) {
            g.fillRoundedRect(rowRect, 4.0f, Color::fromHex(0x18313E));
            g.drawRoundedRect(rowRect, 4.0f, Color::fromHex(0x00F5D4, 0.55f), 1.0f);
        }

        const Color labelCol = !item.enabled
            ? Color::fromHex(0x4E5E6A)
            : (isHover ? Color::fromHex(0x00F5D4) : Color::fromHex(0xE6EEF4));
        const Color shortCol = !item.enabled
            ? Color::fromHex(0x3E4C56)
            : (isHover ? Color::fromHex(0xFF9F1C) : Color::fromHex(0x889BA8));

        g.drawText(item.label, { rowRect.x + 10.0f, rowRect.y, rowRect.width - 96.0f, rowRect.height }, labelCol, 10.0f, false, true);
        if (item.hasSubmenu) {
            // Submenu indicator: same 10 pt menu font as the row label, right-aligned.
            g.drawText(item.shortcut, { rowRect.right() - 24.0f, rowRect.y, 16.0f, rowRect.height }, labelCol, 10.0f, true, true);
        } else if (!item.shortcut.empty()) {
            g.drawText(item.shortcut, { rowRect.right() - 94.0f, rowRect.y, 84.0f, rowRect.height }, shortCol, 9.0f, true, true);
        }

        if (item.isSeparatorAfter && i + 1 < items.size()) {
            g.drawLine({ panel.x + 10.0f, rowRect.bottom() + 1.0f },
                       { panel.right() - 10.0f, rowRect.bottom() + 1.0f },
                       Color::fromHex(0x22323E), 1.0f);
        }
    }

    if (recentSubmenuOpen && recentProjects != nullptr) {
        const auto sub = getActiveRecentSubmenuRect(activeSection, items, recentProjects->size());
        if (sub.width > 0.0f) {
            paintRecentProjectsSubmenu(g, sub, *recentProjects, mousePos);
        }
    }
}

[[nodiscard]] inline StudioMenuCommandId hitTestDropdownMenuOverlay(
    TopMenuBarSection activeSection,
    const std::vector<StudioDropdownMenuItem>& items,
    gui::Point<float> pt) noexcept {
    using namespace avra::gui;
    if (activeSection == TopMenuBarSection::None || items.empty()) return StudioMenuCommandId::None;
    const Rect<float> panel = TopHeaderMenuBarGeometry::getDropdownPanelRect(activeSection, items.size());
    if (!panel.contains(pt)) return StudioMenuCommandId::None;

    for (size_t i = 0; i < items.size(); ++i) {
        if (TopHeaderMenuBarGeometry::getDropdownRowRect(panel, i).contains(pt)) {
            return items[i].enabled ? items[i].id : StudioMenuCommandId::None;
        }
    }
    return StudioMenuCommandId::None;
}

// =============================================================================
// 5. PREFERENCES / STUDIO SETTINGS MODAL (Ctrl+,) & HELP MODALS
// =============================================================================

struct StudioModalGeometry {
    [[nodiscard]] static gui::Rect<float> getDialogRect(gui::Rect<float> winBounds) noexcept {
        const float w = 780.0f;
        const float h = 510.0f;
        return {
            (winBounds.width - w) * 0.5f,
            (winBounds.height - h) * 0.5f,
            w,
            h
        };
    }

    [[nodiscard]] static gui::Rect<float> getCloseBtnRect(gui::Rect<float> dlg) noexcept {
        return { dlg.right() - 104.0f, dlg.y + 12.0f, 88.0f, 28.0f };
    }

    [[nodiscard]] static gui::Rect<float> getPreferencesTabRect(gui::Rect<float> dlg, int tabIdx) noexcept {
        const float tabW = 242.0f;
        return { dlg.x + 20.0f + static_cast<float>(tabIdx) * (tabW + 7.0f), dlg.y + 52.0f, tabW, 32.0f };
    }

    [[nodiscard]] static gui::Rect<float> getPrefOptionCycleRect(gui::Rect<float> dlg, int rowIdx) noexcept {
        return { dlg.x + 320.0f, dlg.y + 108.0f + static_cast<float>(rowIdx) * 56.0f, 430.0f, 36.0f };
    }

    [[nodiscard]] static gui::Rect<float> getPrefFormatCheckboxRect(gui::Rect<float> dlg, int fmtIdx) noexcept {
        const gui::Rect<float> box = getPrefOptionCycleRect(dlg, 0);
        const float pillW = (box.width - 16.0f) / 3.0f;
        return {
            box.x + static_cast<float>(fmtIdx) * (pillW + 8.0f),
            box.y,
            pillW,
            box.height
        };
    }
};

inline void drawAvraHeroEmblem(gui::Graphics& g, gui::Rect<float> iconBox, const std::string& iconPath) {
    using namespace avra::gui;
    g.fillRoundedRect(iconBox, 6.0f, Color::fromHex(0x0A0F14));
    g.drawImageFromFile(iconPath, iconBox, 1.0f);
    g.drawRoundedRect(iconBox, 6.0f, Color::fromHex(0x00F5D4, 0.65f), 1.2f);

    const Rect<float> plaque = {
        iconBox.x + iconBox.width * 0.14f,
        iconBox.bottom() - 22.0f,
        iconBox.width * 0.72f,
        20.0f
    };
    g.fillLinearGradientRect(plaque, Color::fromHex(0xDCE4EC), Color::fromHex(0x8A96A2), 3.0f);
    g.drawRoundedRect(plaque, 3.0f, Color::fromHex(0x0B1014), 1.4f);
    g.drawLine({ plaque.x + 2.5f, plaque.y + 1.2f }, { plaque.right() - 2.5f, plaque.y + 1.2f }, Color::fromHex(0xFFFFFF, 0.85f), 1.0f);

    const float rivetCy = plaque.getCentre().y;
    g.fillCircle({ plaque.x + 4.2f, rivetCy }, 1.6f, Color::fromHex(0x3A444E));
    g.fillCircle({ plaque.right() - 4.2f, rivetCy }, 1.6f, Color::fromHex(0x3A444E));

    const float textStartX = plaque.x + (plaque.width - 45.0f) * 0.5f;
    g.drawText("AVRA", { textStartX, plaque.y + 1.0f, 34.0f, plaque.height - 2.0f }, Color::fromHex(0x0E1318), 11.5f, true, true);
    g.drawText("+",    { textStartX + 32.0f, plaque.y, 13.0f, plaque.height - 2.0f }, Color::fromHex(0xFF6B1A), 12.5f, true, true);
}

inline void paintActiveStudioModal(
    gui::Graphics& g,
    gui::Rect<float> winBounds,
    ActiveStudioModal modal,
    int activePreferencesTab,
    const StudioPreferences& prefs,
    gui::Point<float> mousePos) {
    using namespace avra::gui;
    if (modal == ActiveStudioModal::None) return;

    // Translucent backdrop
    g.fillRect(winBounds, Color::fromHex(0x04070A, 0.78f));

    const Rect<float> dlg = StudioModalGeometry::getDialogRect(winBounds);
    g.fillRoundedRect({ dlg.x + 6.0f, dlg.y + 8.0f, dlg.width, dlg.height }, 10.0f, Color::fromHex(0x000000, 0.70f));
    g.fillRoundedRect(dlg, 10.0f, Color::fromHex(0x0E151C));
    g.drawRoundedRect(dlg, 10.0f, Color::fromHex(0x00F5D4), 1.8f);

    // Header bar inside modal
    g.fillRoundedRect({ dlg.x + 2.0f, dlg.y + 2.0f, dlg.width - 4.0f, 44.0f }, 8.0f, Color::fromHex(0x14202A));
    const char* modalTitle = "STUDIO PREFERENCES & AUDIO ENGINE SETTINGS (Ctrl+,)";
    if (modal == ActiveStudioModal::ShortcutsCheatSheet)     modalTitle = "KEYBOARD SHORTCUTS & ACCELERATORS CHEAT-SHEET (F12 / ?)";
    if (modal == ActiveStudioModal::SchematicReferenceGuide) modalTitle = "SCHEMATIC COMPONENT & HARDWARE BENCHMARK REFERENCE GUIDE";
    if (modal == ActiveStudioModal::AboutAndDiagnostics)     modalTitle = "ABOUT AVRA STUDIO & C++20 TOOLCHAIN DIAGNOSTICS";

    g.drawText(modalTitle, { dlg.x + 20.0f, dlg.y + 8.0f, dlg.width - 140.0f, 30.0f }, Color::fromHex(0x00F5D4), 13.0f, false, true);

    const Rect<float> closeBtn = StudioModalGeometry::getCloseBtnRect(dlg);
    const bool closeHover = closeBtn.contains(mousePos);
    g.fillRoundedRect(closeBtn, 5.0f, closeHover ? Color::fromHex(0x3A1620) : Color::fromHex(0x221218));
    g.drawRoundedRect(closeBtn, 5.0f, Color::fromHex(0xFF4D5A), 1.2f);
    g.drawText("CLOSE [ESC]", closeBtn, Color::fromHex(0xFF858D), 9.5f, true, true);

    if (modal == ActiveStudioModal::Preferences) {
        static constexpr const char* kTabTitles[3] = {
            "1. AUDIO ENGINE & I/O",
            "2. COMPILER & EXPORT PATHS",
            "3. UI & WORKFLOW BEHAVIOR"
        };
        for (int t = 0; t < 3; ++t) {
            const Rect<float> tr = StudioModalGeometry::getPreferencesTabRect(dlg, t);
            const bool isSel = (activePreferencesTab == t);
            const bool isHov = tr.contains(mousePos);
            g.fillRoundedRect(tr, 5.0f, isSel ? Color::fromHex(0x183442) : (isHov ? Color::fromHex(0x152530) : Color::fromHex(0x0B1117)));
            g.drawRoundedRect(tr, 5.0f, isSel ? Color::fromHex(0x00F5D4) : Color::fromHex(0x283A48), isSel ? 1.5f : 1.0f);
            g.drawText(kTabTitles[t], tr, isSel ? Color::fromHex(0x00F5D4) : Color::fromHex(0xB8C7D4), 10.0f, true, true);
        }

        auto drawSettingRow = [&](int rowIdx, const char* title, const char* desc, const std::string& valStr, bool interactive = true, const char* actionBadge = "CLICK \xE2\x96\xB8") {
            const float ry = dlg.y + 108.0f + static_cast<float>(rowIdx) * 56.0f;
            g.drawText(title, { dlg.x + 24.0f, ry, 285.0f, 20.0f }, Color::fromHex(0xF0F6FA), 11.0f, false, true);
            g.drawText(desc,  { dlg.x + 24.0f, ry + 19.0f, 285.0f, 16.0f }, Color::fromHex(0x7E93A4), 9.0f, false, true);

            const Rect<float> box = StudioModalGeometry::getPrefOptionCycleRect(dlg, rowIdx);
            const bool hov = interactive && box.contains(mousePos);
            g.fillRoundedRect(box, 5.0f, hov ? Color::fromHex(0x182C38) : Color::fromHex(0x090F14));
            g.drawRoundedRect(box, 5.0f, hov ? Color::fromHex(0x00F5D4) : Color::fromHex(0x273946), 1.2f);
            const float valWidth = (interactive && actionBadge != nullptr) ? (box.width - 78.0f) : (box.width - 24.0f);
            g.drawText(valStr, { box.x + 12.0f, box.y, valWidth, box.height },
                       interactive ? Color::fromHex(0xFF9F1C) : Color::fromHex(0x9CEAEF), 9.8f, false, true);
            if (interactive && actionBadge != nullptr) {
                g.drawText(actionBadge, { box.right() - 70.0f, box.y, 62.0f, box.height }, Color::fromHex(0x00F5D4), 8.5f, true, true);
            }
        };

        if (activePreferencesTab == 0) {
            drawSettingRow(0, "Audio Buffer Queue Mode", "Low-latency Windows real-time audio engine tuning", prefs.getDriverModeName());
            drawSettingRow(1, "Output Speaker Device", "Hardware D/A monitoring bus for Live Test Rack", prefs.getOutputDeviceName());
            drawSettingRow(2, "Engine Sample Rate", "Internal DSP & Windows WaveOut playback sample rate", std::format("{} Hz", prefs.sampleRateHz));
            drawSettingRow(3, "Block Buffer Size", "DSP callback block size (lower = tighter latency)", std::format("{} samples ({:.1f} ms)", prefs.blockBufferSize, 1000.0f * static_cast<float>(prefs.blockBufferSize) / static_cast<float>(std::max(1, prefs.sampleRateHz))));
            drawSettingRow(4, "DSP Oversampling Quality", "Anti-aliasing ceiling for nonlinear stages & exports", prefs.getOversamplingCapName());
        } else if (activePreferencesTab == 1) {
            // Row 0: Selectable Export Format Checkboxes ([✓] VST3 (.vst3) | [ ] CLAP (.clap) | [ ] Standalone (.exe))
            {
                const float ry = dlg.y + 108.0f;
                g.drawText("Export Target Formats", { dlg.x + 24.0f, ry, 285.0f, 20.0f }, Color::fromHex(0xF0F6FA), 11.0f, false, true);
                g.drawText("Select which binaries BUILD compiles into dist/ (Default: VST3)", { dlg.x + 24.0f, ry + 19.0f, 285.0f, 16.0f }, Color::fromHex(0x7E93A4), 9.0f, false, true);

                static constexpr const char* kFmtNames[3] = { "VST3 (.vst3)", "CLAP (.clap)", "Standalone (.exe)" };
                const bool fmtChecked[3] = { prefs.exportFormatVst3, prefs.exportFormatClap, prefs.exportFormatStandalone };
                for (int f = 0; f < 3; ++f) {
                    const Rect<float> fBox = StudioModalGeometry::getPrefFormatCheckboxRect(dlg, f);
                    const bool fHov = fBox.contains(mousePos);
                    const bool fOn  = fmtChecked[f];
                    g.fillRoundedRect(fBox, 5.0f, fOn ? Color::fromHex(0x14332C) : (fHov ? Color::fromHex(0x182C38) : Color::fromHex(0x090F14)));
                    g.drawRoundedRect(fBox, 5.0f, fOn ? Color::fromHex(0x06D6A0) : (fHov ? Color::fromHex(0x00F5D4) : Color::fromHex(0x273946)), fOn ? 1.5f : 1.0f);
                    const std::string pillText = std::format("[{}] {}", fOn ? "\xE2\x9C\x93" : " ", kFmtNames[f]);
                    g.drawText(pillText, fBox, fOn ? Color::fromHex(0x06D6A0) : Color::fromHex(0x9FB2C0), 9.5f, true, true);
                }
            }
            drawSettingRow(1, "Active Build Output Folder", "Destination folder for compiled .vst3, .clap & .exe binaries", prefs.activeExportFolderDisplay, true, "CHANGE \xE2\x96\xB8");
            drawSettingRow(2, "Plugin Vendor & 4-Char Code", "Locked studio brand identity embedded into VST3/CLAP binaries", "AVRA AUDIO  [Code: AVRA  |  com.avra.*]", false, nullptr);
            drawSettingRow(3, "MSVC C++20 Compiler (cl.exe)", "Auto-detected Visual Studio C++20 x64 compiler", prefs.clExePath, false, nullptr);
            drawSettingRow(4, "CMake & IDE Toolchain", "Auto-detected CMake build generator & IDE", prefs.cmakeExePath, false, nullptr);
        } else {
            drawSettingRow(0, "Rotary Knob Mouse Behaviour", "Drag interaction mode for all studio & faceplate rotary knobs", prefs.getKnobMouseModeName());
            drawSettingRow(1, "Auto-Save Recovery Interval", "Automatic background backup to <Project>.avra.bak", prefs.getAutoSaveIntervalName());
            drawSettingRow(2, "Default Faceplate Grid Snap", "Snap quantum when dragging widgets in UI Builder (Ctrl+G)", prefs.getGridSnapName());
            drawSettingRow(3, "Show Live FPS / CPU Overlay", "Display real-time UI FPS & DSP CPU % telemetry badge", prefs.showFpsCpuOverlay ? "ENABLED (Live FPS + DSP CPU %)" : "HIDDEN");
            drawSettingRow(4, "Undo / Redo Ring Capacity", "50-step command history in RAM (Ctrl+Z / Ctrl+Y)",
                           std::format("50 Steps Max  [Active: {} Undo / {} Redo]", prefs.activeUndoCount, prefs.activeRedoCount), true, "CLEAR \xE2\x96\xB8");
        }

        g.drawText("All changes are saved automatically to .avra_preferences.json and applied immediately.",
                   { dlg.x + 24.0f, dlg.bottom() - 34.0f, dlg.width - 48.0f, 22.0f },
                   Color::fromHex(0x889BA8), 9.5f, true, false);
    } else if (modal == ActiveStudioModal::ShortcutsCheatSheet) {
        struct ShortcutPair { const char* keys; const char* desc; };
        static constexpr ShortcutPair kLeftCol[] = {
            { "Ctrl + N",       "New Plugin Project Wizard" },
            { "Ctrl + O",       "Open Project (.avra)" },
            { "Ctrl + S",       "Save Project (.avra + .schematic + .faceplate)" },
            { "Ctrl + Shift+S", "Save Project As..." },
            { "Ctrl + ,",       "Open Preferences & Audio Settings Modal" },
            { "Ctrl + Z",       "Undo Last Action (50-Step History)" },
            { "Ctrl + Y",       "Redo Action (or Ctrl+Shift+Z)" },
            { "Ctrl + D",       "Duplicate Selected Stage or Widget" },
            { "Del / Backspace","Delete Selected Stage, Widget, or Sticker" },
            { "Ctrl + R",       "Reset All Parameters to Unity / Default" }
        };
        static constexpr ShortcutPair kRightCol[] = {
            { "F1 / F2 / F3 / F4","Switch: Schematic / Faceplate / Live Rack / Build" },
            { "Right-Click Stem","Assign Custom .WAV File to Test Signal Stem Slot" },
            { "F7",             "Compile Selected Formats (VST3 / CLAP / App)" },
            { "F8",             "Run Forensic DSP & Dead-Knob Audit" },
            { "F9",             "Open Project in IDE (also Ctrl+Shift+O)" },
            { "F11",            "Toggle Fullscreen Studio Mode" },
            { "Ctrl + B",       "Toggle Left Project Drawer" },
            { "Ctrl + G",       "Cycle Faceplate Grid Snap (4px / 8px / 16px / Off)" },
            { "Shift+RightClick","Draw / Toggle Console Wax-Pencil Mark on Knob" },
            { "Esc",            "Close Active Modal / Panic Hard-Mute Audio" }
        };
        for (int i = 0; i < 10; ++i) {
            const float ry = dlg.y + 66.0f + static_cast<float>(i) * 40.0f;
            g.fillRoundedRect({ dlg.x + 22.0f, ry, 118.0f, 28.0f }, 4.0f, Color::fromHex(0x162632));
            g.drawRoundedRect({ dlg.x + 22.0f, ry, 118.0f, 28.0f }, 4.0f, Color::fromHex(0x00F5D4, 0.55f), 1.0f);
            g.drawText(kLeftCol[i].keys, { dlg.x + 22.0f, ry, 118.0f, 28.0f }, Color::fromHex(0xFF9F1C), 9.5f, true, true);
            g.drawText(kLeftCol[i].desc, { dlg.x + 150.0f, ry, 230.0f, 28.0f }, Color::fromHex(0xE6EEF4), 9.5f, false, true);

            g.fillRoundedRect({ dlg.x + 398.0f, ry, 126.0f, 28.0f }, 4.0f, Color::fromHex(0x162632));
            g.drawRoundedRect({ dlg.x + 398.0f, ry, 126.0f, 28.0f }, 4.0f, Color::fromHex(0x00F5D4, 0.55f), 1.0f);
            g.drawText(kRightCol[i].keys, { dlg.x + 398.0f, ry, 126.0f, 28.0f }, Color::fromHex(0xFF9F1C), 9.5f, true, true);
            g.drawText(kRightCol[i].desc, { dlg.x + 534.0f, ry, 230.0f, 28.0f }, Color::fromHex(0xE6EEF4), 9.5f, false, true);
        }
    } else if (modal == ActiveStudioModal::SchematicReferenceGuide) {
        struct GuideCard {
            const char* title;
            const char* line1;
            const char* line2;
        };
        static constexpr GuideCard kGuideCards[6] = {
            {
                "1. 150 STUDIO MODULES ACROSS 7 CATEGORIES + 6 SOURCE FILTERS",
                "PRE / TUBE (24) \xC2\xB7 IRON / SAT (25) \xC2\xB7 EQ / SPEC (25) \xC2\xB7 COMP / CLIP (26)",
                "TIME / VERB (24) \xC2\xB7 SPLIT / MOD (21) \xC2\xB7 AUTO / SMART (5) \xE2\x80\x94 filterable by All, Bass, Guitar, Drums, Vocals & Keys."
            },
            {
                "2. SERIES TRUNK & PARALLEL Y-SPLIT (Y / \xCE\xA3) ROUTING",
                "Drag modules left/right to reorder along the main series trunk, or drag downward into Path B",
                "(or click SERIES / PARALLEL) to fork a parallel Y-Split [Y] and Mixer [\xCE\xA3] with Wet/Dry & 180\xC2\xB0 polarity."
            },
            {
                "3. FACEPLATE SIGNAL-CHAIN STRIP (S1 \xE2\x86\x92 S2 ...) & ACTIVE MODULE BAY",
                "Multi-stage chains show compact S1 \xE2\x86\x92 S2 \xE2\x86\x92 S3 selector pills at the top above a full-width active module bay.",
                "Click any S1..Sn pill to inspect its full-size controls below, click its Jewel LED to A/B bypass, or [x] in UI Builder to remove."
            },
            {
                "4. ACOUSTIC LAB BENCH, SIGNAL PROBE & LATENCY (PDC) REPORT",
                "In LIVE TEST RACK, click LAB BENCH for Bode/THD/Hysteresis plots, or PROBE to overlay/solo any inter-stage wire tap.",
                "The PDC pill shows the exact look-ahead / oversampling latency reported to your DAW via VST3 and CLAP."
            },
            {
                "5. CONSTANT-LOUDNESS AUDITIONING (LINK IO, AUTO-LEVEL, \xCE\x94 DELTA)",
                "Enable LINK IO on gain stages for inverse output compensation. Use AUTO-LEVEL to match input/output loudness,",
                "and \xCE\x94 DELTA to solo the wet/dry difference signal during critical auditioning."
            },
            {
                "6. CONSOLE WAX-PENCIL MARKS & GAFF TAPE LOCKS",
                "Shift + Right-Click any rotary knob skirt to draw a yellow China-Marker sweet-spot detent. Place Stage Gaff Tape",
                "or Console Masking Tape over any control in the Faceplate Builder to physically lock it against accidental changes."
            }
        };
        for (int i = 0; i < 6; ++i) {
            const float ry = dlg.y + 62.0f + static_cast<float>(i) * 63.0f;
            const Rect<float> card = { dlg.x + 24.0f, ry, dlg.width - 48.0f, 56.0f };
            g.fillRoundedRect(card, 5.0f, Color::fromHex(0x091016));
            g.drawRoundedRect(card, 5.0f, Color::fromHex(0x223340), 1.0f);
            g.drawText(kGuideCards[i].title,
                       { card.x + 12.0f, card.y + 4.0f, card.width - 24.0f, 18.0f },
                       Color::fromHex(0xFF9F1C), 9.8f, false, true);
            g.drawText(kGuideCards[i].line1,
                       { card.x + 12.0f, card.y + 21.0f, card.width - 24.0f, 16.0f },
                       Color::fromHex(0xD8E4EC), 9.0f, false, false);
            g.drawText(kGuideCards[i].line2,
                       { card.x + 12.0f, card.y + 36.0f, card.width - 24.0f, 16.0f },
                       Color::fromHex(0xB8C8D4), 9.0f, false, false);
        }
    } else if (modal == ActiveStudioModal::AboutAndDiagnostics) {
        std::error_code iconEc;
        const std::string iconPath = std::filesystem::exists("assets/avra_studio_icon.png", iconEc)
            ? "assets/avra_studio_icon.png"
            : "../assets/avra_studio_icon.png";
        const Rect<float> heroBox = { dlg.x + 26.0f, dlg.y + 62.0f, dlg.width - 52.0f, 92.0f };
        g.fillRoundedRect(heroBox, 6.0f, Color::fromHex(0x0B151D));
        g.drawRoundedRect(heroBox, 6.0f, Color::fromHex(0x00F5D4, 0.80f), 1.4f);
        drawAvraHeroEmblem(g, { heroBox.x + 8.0f, heroBox.y + 6.0f, 80.0f, 80.0f }, iconPath);
        g.drawText("AVRA STUDIO  \xE2\x80\x94  CUSTOM AUDIO PLUGIN FOUNDRY  (v1.0.0)",
                   { heroBox.x + 100.0f, heroBox.y + 12.0f, heroBox.width - 114.0f, 24.0f },
                   Color::fromHex(0x00F5D4), 14.5f, false, true);
        g.drawText("Modular Circuit Schematic Designer, Skeuomorphic Faceplate Forge & C++20 Plugin Compiler",
                   { heroBox.x + 100.0f, heroBox.y + 39.0f, heroBox.width - 114.0f, 20.0f },
                   Color::fromHex(0xD4AF37), 11.0f, false, true);
        g.drawText("Native C++20 Direct2D GPU Interface  \xC2\xB7  64-Bit Real-Time Floating-Point Audio Engine",
                   { heroBox.x + 100.0f, heroBox.y + 61.0f, heroBox.width - 114.0f, 18.0f },
                   Color::fromHex(0x9FB2C0), 9.4f, false, false);

        const std::array<std::string, 6> kDiagLines = {
            std::format("Audio Engine: 64-Bit SIMD C++20 Core  |  Sample Rate: {} Hz  |  Block Size: {} samples\nOversampling Phase Modes: Zero-Latency Min-Phase IIR & Mastering Linear-Phase FIR",
                        prefs.sampleRateHz, prefs.blockBufferSize),
            "Circuit Library: 150 C++20 Studio Modules across 7 Categories\n24 PRE/TUBE \xC2\xB7 25 IRON/SAT \xC2\xB7 25 EQ/SPEC \xC2\xB7 26 COMP/CLIP \xC2\xB7 24 TIME/VERB \xC2\xB7 21 SPLIT/MOD \xC2\xB7 5 AUTO/SMART",
            "Faceplate Designer: 51 Unified UI Style Templates, 34 Rotary Knob & Fader Styles\n10 Switch Styles, 18 Meters/Scopes & 32 Studio Decals + Custom PNG Upload",
            "Studio Blueprints & Test Rack: 6 Multi-Stage Studio Blueprints + 10 Test Signal Generators\nAcoustic Lab Bench (Bode/THD/Hysteresis), Inter-Stage Signal Probe & Custom .WAV Stem Slots",
            "Plugin Export & Workflow: Native 64-Bit VST3 (.vst3), CLAP (.clap) & Standalone (.exe)\n50-Step Undo/Redo History + Automatic Crash-Recovery Backup (.avra.bak)",
            "Compiler Toolchain: MSVC C++20 (cl.exe) + CMake 3.20+ Self-Contained Build Pipeline\nZero External SDK Dependencies  \xC2\xB7  Direct2D Hardware-Accelerated Rendering"
        };
        for (int i = 0; i < 6; ++i) {
            const float ry = dlg.y + 164.0f + static_cast<float>(i) * 46.0f;
            g.fillRoundedRect({ dlg.x + 26.0f, ry, dlg.width - 52.0f, 38.0f }, 5.0f, Color::fromHex(0x091016));
            g.drawRoundedRect({ dlg.x + 26.0f, ry, dlg.width - 52.0f, 38.0f }, 5.0f, Color::fromHex(0x223340), 1.0f);
            g.drawText(kDiagLines[static_cast<size_t>(i)], { dlg.x + 40.0f, ry, dlg.width - 80.0f, 38.0f },
                       Color::fromHex(0xE8F1F2), 9.8f, false, false);
        }
    }
}

/**
 * @brief Handles mouse clicks when a Studio Modal is active.
 * @return true if the click was consumed by the modal overlay.
 */
inline bool handleActiveStudioModalClick(
    gui::Rect<float> winBounds,
    gui::Point<float> pt,
    ActiveStudioModal& ioModal,
    int& ioPreferencesTab,
    StudioPreferences& ioPrefs,
    int& ioCurrentGridSnapPx,
    const std::filesystem::path& workspaceRoot,
    StudioModalSpecialAction* outSpecialAction = nullptr) {
    if (outSpecialAction) *outSpecialAction = StudioModalSpecialAction::None;
    if (ioModal == ActiveStudioModal::None) return false;

    const gui::Rect<float> dlg = StudioModalGeometry::getDialogRect(winBounds);
    const gui::Rect<float> closeBtn = StudioModalGeometry::getCloseBtnRect(dlg);

    if (closeBtn.contains(pt) || !dlg.contains(pt)) {
        ioModal = ActiveStudioModal::None;
        return true;
    }

    if (ioModal == ActiveStudioModal::Preferences) {
        for (int t = 0; t < 3; ++t) {
            if (StudioModalGeometry::getPreferencesTabRect(dlg, t).contains(pt)) {
                ioPreferencesTab = t;
                return true;
            }
        }

        bool changed = false;
        if (ioPreferencesTab == 0) {
            if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 0).contains(pt)) {
                ioPrefs.audioDriverMode = (ioPrefs.audioDriverMode + 1) % 3;
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 1).contains(pt)) {
                const auto devs = avra::audio_io::enumerateOutputDevices();
                const int devCount = std::max(1, static_cast<int>(devs.size()));
                ioPrefs.outputDeviceIdx = (ioPrefs.outputDeviceIdx + 1) % devCount;
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 2).contains(pt)) {
                static constexpr int kRates[4] = { 44100, 48000, 88200, 96000 };
                int idx = 1;
                for (int i = 0; i < 4; ++i) if (kRates[i] == ioPrefs.sampleRateHz) idx = i;
                ioPrefs.sampleRateHz = kRates[(idx + 1) % 4];
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 3).contains(pt)) {
                static constexpr int kBufs[5] = { 64, 128, 256, 512, 1024 };
                int idx = 2;
                for (int i = 0; i < 5; ++i) if (kBufs[i] == ioPrefs.blockBufferSize) idx = i;
                ioPrefs.blockBufferSize = kBufs[(idx + 1) % 5];
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 4).contains(pt)) {
                ioPrefs.globalOversamplingCapIdx = (ioPrefs.globalOversamplingCapIdx + 1) % 4;
                changed = true;
            }
        } else if (ioPreferencesTab == 1) {
            if (StudioModalGeometry::getPrefFormatCheckboxRect(dlg, 0).contains(pt)) {
                ioPrefs.exportFormatVst3 = !ioPrefs.exportFormatVst3;
                ioPrefs.ensureAtLeastOneExportFormat();
                changed = true;
            } else if (StudioModalGeometry::getPrefFormatCheckboxRect(dlg, 1).contains(pt)) {
                ioPrefs.exportFormatClap = !ioPrefs.exportFormatClap;
                ioPrefs.ensureAtLeastOneExportFormat();
                changed = true;
            } else if (StudioModalGeometry::getPrefFormatCheckboxRect(dlg, 2).contains(pt)) {
                ioPrefs.exportFormatStandalone = !ioPrefs.exportFormatStandalone;
                ioPrefs.ensureAtLeastOneExportFormat();
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 1).contains(pt)) {
                if (outSpecialAction) *outSpecialAction = StudioModalSpecialAction::CycleExportOutputFolder;
                changed = true;
            }
        } else if (ioPreferencesTab == 2) {
            if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 0).contains(pt)) {
                ioPrefs.knobMouseMode = (ioPrefs.knobMouseMode + 1) % 3;
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 1).contains(pt)) {
                ioPrefs.autoSaveIntervalIdx = (ioPrefs.autoSaveIntervalIdx + 1) % 4;
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 2).contains(pt)) {
                if (ioPrefs.defaultGridSizePx == 4)       ioPrefs.defaultGridSizePx = 8;
                else if (ioPrefs.defaultGridSizePx == 8)  ioPrefs.defaultGridSizePx = 16;
                else if (ioPrefs.defaultGridSizePx == 16) ioPrefs.defaultGridSizePx = 0;
                else                                      ioPrefs.defaultGridSizePx = 4;
                ioCurrentGridSnapPx = ioPrefs.defaultGridSizePx;
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 3).contains(pt)) {
                ioPrefs.showFpsCpuOverlay = !ioPrefs.showFpsCpuOverlay;
                changed = true;
            } else if (StudioModalGeometry::getPrefOptionCycleRect(dlg, 4).contains(pt)) {
                if (outSpecialAction) *outSpecialAction = StudioModalSpecialAction::ClearUndoRedoHistory;
                changed = true;
            }
        }

        if (changed) {
            saveStudioPreferencesToFile(workspaceRoot / ".avra_preferences.json", ioPrefs);
        }
    }
    return true;
}

} // namespace avra::studio
