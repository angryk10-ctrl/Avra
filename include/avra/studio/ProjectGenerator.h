#pragma once

#include "avra/studio/SchematicSpec.h"
#include "avra/studio/UiBuilderSpec.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <format>
#include <algorithm>
#include <cctype>
#include <functional>

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif

namespace avra::studio {

enum class PluginArchetype {
    Mastering,      ///< Brickwall Limiter, Lookahead, True-Peak ISP, LUFS
    Imager,         ///< Stereo Imager, Mid/Side Width, Bass Mono Crossover, Correlation
    Distortion,     ///< Waveshaping, Analog Tape & Tube Saturation, Tone Biquad
    Chorus,         ///< Stereo BBD Chorus, Anti-Phase LFO, Haas Width
    Reverb,         ///< Algorithmic Space, Feedback Delay Network (FDN), Damping
    StereoDelay,    ///< Dual Delay, Crossfeed Ping-Pong, Tape Echo
    Compressor,     ///< Dynamics, VCA Envelope Follower, Variable Knee
    Transient,      ///< Transient Shaper, Dual Attack & Sustain Ballistics
    Equalizer,      ///< 3-Band Parametric EQ, Transposed Direct-Form II Biquads
    Phaser,         ///< Analog Phaser, 4-Stage Allpass Cascade, Resonant Feedback
    NoiseGate,      ///< Studio Noise Gate, Threshold, Hold, Release, Floor
    Utility         ///< Stereo Utility, Mid/Side Matrix, Phase, Width, Pan
};

struct ProjectConfig {
    std::string name{};
    std::string vendor{"Avra Audio (Free Non-Commercial)"};
    std::string pluginId{"com.avra.plugin"};
    std::string category{"Custom Studio Processor"};
    std::string outputDir{}; // Resolved dynamically relative to Avra installation root if empty
    PluginArchetype archetype{PluginArchetype::Mastering};

    // Enabled Avra Modules
    bool enableCore{true};
    bool enableDirect2dGui{true};
    bool enableClapBridge{true};
    bool enableStateSerialization{true};

    bool enableDspDynamics{true};
    bool enableDspSaturation{false};
    bool enableDspDelays{false};
    bool enableDspFilters{false};
    bool enableDspModulation{false};
    bool enableDspStereo{true};
    bool enableDspOversampling{false};
};

struct ToolchainStatus {
    bool hasCMake{false};
    bool hasMsvcCompiler{false};
    bool hasVsCode{false};
    bool hasVisualStudioIde{false};
    std::string cmakePath{};
    std::string vsCodePath{};
    std::string visualStudioPath{};

    [[nodiscard]] bool canCompilePlugins() const noexcept {
        return hasCMake && hasMsvcCompiler;
    }

    [[nodiscard]] std::string getReadinessSummary() const {
        return std::format(
            "CMake: {}  |  MSVC C++20 Build Tools: {}  |  VS Code: {}  |  Visual Studio IDE: {}",
            hasCMake ? "DETECTED" : "MISSING (Install CMake 3.20+)",
            hasMsvcCompiler ? "DETECTED" : "MISSING (Install VS C++ Build Tools)",
            hasVsCode ? "INSTALLED" : "NOT FOUND (Install VS Code + C/C++ Ext)",
            hasVisualStudioIde ? "INSTALLED" : "OPTIONAL");
    }

    [[nodiscard]] std::string getPrerequisitesNotice() const {
        if (hasCMake && hasMsvcCompiler && hasVsCode) {
            return "TOOLCHAIN READY: CMake + MSVC C++20 + VS Code detected. Direct 1-click VST3, CLAP & Standalone .EXE builds enabled!";
        }
        std::string missing = "REQUIRED FOR DIRECT VST3 / STANDALONE BUILDS: Please ensure ";
        bool first = true;
        if (!hasVsCode) {
            missing += "VS Code (https://code.visualstudio.com)";
            first = false;
        }
        if (!hasMsvcCompiler) {
            if (!first) missing += ", ";
            missing += "Visual Studio C++20 Build Tools (Desktop development with C++)";
            first = false;
        }
        if (!hasCMake) {
            if (!first) missing += ", ";
            missing += "CMake 3.20+";
        }
        missing += " are installed.";
        return missing;
    }
};

class ProjectGenerator {
public:
    ProjectGenerator() = default;
    explicit ProjectGenerator(std::vector<SchematicNode> chain) : chain_(std::move(chain)) {}

    [[nodiscard]] std::string generateStageCode(size_t stageIndex) const {
        return generateStageCodeForChain(chain_, stageIndex);
    }

    [[nodiscard]] static std::string generateStageCodeForChain(const std::vector<SchematicNode>& chain, size_t stageIndex) {
        if (stageIndex >= chain.size()) return {};
        const auto& node = chain[stageIndex];
        const auto& meta = getComponentMeta(node.type);
        const auto  plum = getAutoPlumbingSpec(node.type);
        const auto  plans = GraphSanitizerAndSanityEngine::analyzeChain(chain);
        const auto& plan = plans[stageIndex];

        std::ostringstream ss;
        ss << "// --- STAGE " << (stageIndex + 1) << " [" << meta.designator << "]: " << meta.fullName << " ---\n";
        ss << "// Smart Plumbing: " << plan.summaryBadges << "\n";
        ss << meta.cppClassName << " stage" << stageIndex << "_;\n";
        ss << "if (s" << stageIndex << "_in_->getPlain() >= 0.5f) {\n";
        ss << "    stage" << stageIndex << "_.processStereo(l, r);\n";
        if (plum.enableComponentDrift) {
            ss << "    // [DRIFT 1.5%-2.5% + THERMAL -95dB]\n";
            ss << "    l = l * " << std::format("{:.5f}f", plan.driftMultL) << " + 1.778e-5f;\n";
            ss << "    r = r * " << std::format("{:.5f}f", plan.driftMultR) << " - 1.778e-5f;\n";
        }
        if (plum.injectDcGuard) {
            ss << "    // [DC-GUARD 5Hz]: y[n] = x[n] - x[n-1] + 0.9995f * y[n-1]\n";
            ss << "    const float dcL = l - dcInL_" << stageIndex << "_ + 0.9995f * dcOutL_" << stageIndex << "_;\n";
            ss << "    dcInL_" << stageIndex << "_ = l; dcOutL_" << stageIndex << "_ = dcL; l = dcL;\n";
            ss << "    const float dcR = r - dcInR_" << stageIndex << "_ + 0.9995f * dcOutR_" << stageIndex << "_;\n";
            ss << "    dcInR_" << stageIndex << "_ = r; dcOutR_" << stageIndex << "_ = dcR; r = dcR;\n";
        }
        if (plum.passiveInsertionMakeupDb > 0.1f) {
            ss << "    // [PASSIVE MAKEUP +" << std::format("{:.0f}", plum.passiveInsertionMakeupDb) << "dB]\n";
            ss << "    const float makeupLin = math::dBToLinear(" << std::format("{:.1f}f", plum.passiveInsertionMakeupDb) << ");\n";
            ss << "    l *= makeupLin; r *= makeupLin;\n";
        }
        if (plum.injectSafetySubsonicClamp) {
            ss << "    // [SAFETY SUB-SONIC & NAN CLAMP]: 15 Hz infrasonic guard + +18 dBFS soft-limiter\n";
            ss << "    if (!std::isfinite(l)) l = 0.0f;\n";
            ss << "    if (!std::isfinite(r)) r = 0.0f;\n";
            ss << "    if (std::abs(l) > 1.0f) l = 7.9432823f * std::tanh(l / 7.9432823f);\n";
            ss << "    if (std::abs(r) > 1.0f) r = 7.9432823f * std::tanh(r / 7.9432823f);\n";
        }
        ss << "}\n";
        return ss.str();
    }

    static std::filesystem::path getWorkspaceRoot() {
        return resolveAvraWorkspaceRoot();
    }

    static std::filesystem::path getPluginsDir(const ProjectConfig& config) {
        if (!config.outputDir.empty()) {
            return std::filesystem::path(config.outputDir);
        }
        return getWorkspaceRoot() / "plugins";
    }

    static ToolchainStatus detectToolchain(bool forceRefresh = false) {
        static bool s_cached = false;
        static ToolchainStatus s_cachedStatus{};
        if (s_cached && !forceRefresh) {
            return s_cachedStatus;
        }

        namespace fs = std::filesystem;
        ToolchainStatus st{};

        // 1. Check CMake
        const char* cmakeCandidates[] = {
            "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
            "C:\\Program Files\\CMake\\bin\\cmake.exe"
        };
        for (const char* p : cmakeCandidates) {
            if (fs::exists(p)) {
                st.hasCMake = true;
                st.cmakePath = p;
                break;
            }
        }
#if defined(_WIN32)
        if (!st.hasCMake) {
            char buf[MAX_PATH]{};
            if (SearchPathA(nullptr, "cmake.exe", nullptr, MAX_PATH, buf, nullptr) > 0) {
                st.hasCMake = true;
                st.cmakePath = buf;
            }
        }
#endif

        // 2. Check Visual Studio / MSVC C++20 Build Tools (MSBuild / cl.exe)
        const char* msvcCandidates[] = {
            "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\MSBuild.exe"
        };
        for (const char* p : msvcCandidates) {
            if (fs::exists(p)) {
                st.hasMsvcCompiler = true;
                break;
            }
        }
#if defined(_WIN32)
        if (!st.hasMsvcCompiler) {
            char buf[MAX_PATH]{};
            if (SearchPathA(nullptr, "cl.exe", nullptr, MAX_PATH, buf, nullptr) > 0 ||
                SearchPathA(nullptr, "MSBuild.exe", nullptr, MAX_PATH, buf, nullptr) > 0) {
                st.hasMsvcCompiler = true;
            }
        }
#endif

        // 3. Check VS Code (code.cmd / code.exe)
#if defined(_WIN32)
        char localAppData[MAX_PATH]{};
        DWORD envLen = GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH);
        if (envLen > 0 && envLen < MAX_PATH) {
            fs::path userCode = fs::path(localAppData) / "Programs" / "Microsoft VS Code" / "bin" / "code.cmd";
            if (fs::exists(userCode)) {
                st.hasVsCode = true;
                st.vsCodePath = userCode.string();
            }
        }
        if (!st.hasVsCode && fs::exists("C:\\Program Files\\Microsoft VS Code\\bin\\code.cmd")) {
            st.hasVsCode = true;
            st.vsCodePath = "C:\\Program Files\\Microsoft VS Code\\bin\\code.cmd";
        }
        if (!st.hasVsCode) {
            char buf[MAX_PATH]{};
            if (SearchPathA(nullptr, "code.cmd", nullptr, MAX_PATH, buf, nullptr) > 0) {
                st.hasVsCode = true;
                st.vsCodePath = buf;
            }
        }
#endif

        // 4. Check Visual Studio IDE (devenv.exe)
        const char* vsIdeCandidates[] = {
            "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\devenv.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\devenv.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\Common7\\IDE\\devenv.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\IDE\\devenv.exe"
        };
        for (const char* p : vsIdeCandidates) {
            if (fs::exists(p)) {
                st.hasVisualStudioIde = true;
                st.visualStudioPath = p;
                break;
            }
        }

        s_cachedStatus = st;
        s_cached = true;
        return st;
    }

    static bool generate(const ProjectConfig& config, std::string& outMessage) {
        namespace fs = std::filesystem;

        const std::string pluginName = sanitizeName(config.name);
        if (pluginName.empty()) {
            outMessage = "Invalid plugin name.";
            return false;
        }

        const fs::path targetDir = getPluginsDir(config) / pluginName;

        try {
            fs::create_directories(targetDir);
            fs::create_directories(targetDir / "resources");
            fs::create_directories(targetDir / "presets");
            fs::create_directories(targetDir / ".vscode");

            // 1. Generate PluginProcessor Header: <PluginName>.h
            writeFile((targetDir / (pluginName + ".h")).string(), generateProcessorHeader(pluginName, config));

            // 2. Generate Single-File Embedded Assets Header & PluginEditor Header: EmbeddedAssets.h + <PluginName>Editor.h
            writeFile((targetDir / "EmbeddedAssets.h").string(), generateEmbeddedAssetsHeader(pluginName, buildBlankFaceplate(pluginName)));
            writeFile((targetDir / (pluginName + "Editor.h")).string(), generateEditorHeader(pluginName, config));

            // 3. Generate CLAP Bridge (.clap), VST3 Bridge (.vst3), and Standalone App (.exe)
            writeFile((targetDir / (pluginName + "Clap.cpp")).string(), generateClapBridge(pluginName, config));
            writeFile((targetDir / (pluginName + "Vst3.cpp")).string(), generateVst3Bridge(pluginName, config));
            writeFile((targetDir / (pluginName + "Standalone.cpp")).string(), generateStandaloneMain(pluginName, config));

            // 4. Generate Self-Contained Relative CMakeLists.txt, Icon Resource (avra_plugin.rc), VS Code Build Pipeline, Resources, Default Preset & README
            exportPluginIconResources(targetDir);
            writeFile((targetDir / "CMakeLists.txt").string(), generateStandaloneCMakeLists(pluginName));
            writeFile((targetDir / ".vscode" / "tasks.json").string(), generateVsCodeTasksJson(pluginName));
            writeFile((targetDir / ".vscode" / "launch.json").string(), generateVsCodeLaunchJson(pluginName));
            writeFile((targetDir / ".vscode" / "c_cpp_properties.json").string(), generateVsCodeCppPropertiesJson());
            writeFile((targetDir / "resources" / "faceplate_theme.json").string(), generateThemeManifest(pluginName, config));
            writeFile((targetDir / "presets" / "Default.avrapreset").string(), "# Avra Preset v1\n1:0.5\n2:0.5\n");
            writeFile((targetDir / "README.md").string(), generateProjectReadme(pluginName, config, {}));

            // 5. Update root CMakeLists.txt to register VST3, CLAP & Standalone targets
            updateCMakeLists(pluginName);

            outMessage = "Standalone Project '" + pluginName + "' (VST3 + CLAP + Standalone .EXE + VS Code tasks) generated at: " + targetDir.string();
            return true;
        } catch (const std::exception& e) {
            outMessage = std::string("Error generating project: ") + e.what();
            return false;
        }
    }

    /**
     * @brief Generates a custom multi-stage C++20 plugin, custom WYSIWYG hardware faceplate GUI,
     * VST3 + CLAP + Standalone desktop app runner, relative CMake project, and VS Code build tasks
     * directly from the Schematic Builder + Faceplate UI Builder.
     */
    static bool generateFromSchematicAndUi(
        const ProjectConfig& config,
        const std::vector<SchematicNode>& chain,
        const FaceplateLayout& faceplate,
        std::string& outMessage) {
        namespace fs = std::filesystem;

        const std::string pluginName = sanitizeName(config.name.empty() ? "AvraGain" : config.name);
        if (pluginName.empty()) {
            outMessage = "Invalid plugin name.";
            return false;
        }

        const fs::path targetDir = getPluginsDir(config) / pluginName;

        try {
            fs::create_directories(targetDir);
            fs::create_directories(targetDir / "resources");
            fs::create_directories(targetDir / "presets");
            fs::create_directories(targetDir / ".vscode");

            writeFile((targetDir / (pluginName + ".h")).string(), generateSchematicProcessorHeader(pluginName, config, chain));
            writeFile((targetDir / "EmbeddedAssets.h").string(), generateEmbeddedAssetsHeader(pluginName, faceplate));
            writeFile((targetDir / (pluginName + "Editor.h")).string(), generateCustomFaceplateEditorHeader(pluginName, config, faceplate));
            writeFile((targetDir / (pluginName + "Clap.cpp")).string(), generateClapBridge(pluginName, config));
            writeFile((targetDir / (pluginName + "Vst3.cpp")).string(), generateVst3Bridge(pluginName, config));
            writeFile((targetDir / (pluginName + "Standalone.cpp")).string(), generateStandaloneMain(pluginName, config));
            exportPluginIconResources(targetDir);
            writeFile((targetDir / "CMakeLists.txt").string(), generateStandaloneCMakeLists(pluginName));
            writeFile((targetDir / ".vscode" / "tasks.json").string(), generateVsCodeTasksJson(pluginName));
            writeFile((targetDir / ".vscode" / "launch.json").string(), generateVsCodeLaunchJson(pluginName));
            writeFile((targetDir / ".vscode" / "c_cpp_properties.json").string(), generateVsCodeCppPropertiesJson());
            writeFile((targetDir / "resources" / "faceplate_theme.json").string(), generateThemeManifest(pluginName, config));
            writeFile((targetDir / "presets" / "Default.avrapreset").string(), "# Avra Preset v1\n1:0.5\n");
            writeFile((targetDir / "README.md").string(), generateProjectReadme(pluginName, config, chain));

            saveSchematicChainToFile((targetDir / (pluginName + ".schematic")).string(), chain);
            saveFaceplateLayoutToFile(targetDir / (pluginName + ".faceplate"), faceplate);

            updateCMakeLists(pluginName);

            outMessage = std::format(
                "Standalone Project '{}' ({} hardware stages, {} faceplate widgets, VST3 + CLAP + Standalone .EXE) saved at {}",
                pluginName, chain.size(), faceplate.widgets.size(), targetDir.string());
            return true;
        } catch (const std::exception& e) {
            outMessage = std::string("Error generating project: ") + e.what();
            return false;
        }
    }

    /**
     * @brief Fast, non-blocking project state save (`💾 SAVE PROJECT` / `Ctrl+S`):
     * Immediately writes `<PluginName>.schematic`, `<PluginName>.faceplate`, `<PluginName>.h`,
     * `<PluginName>Editor.h`, `resources/faceplate_theme.json`, and `README.md` to `plugins/<PluginName>/`
     * without waiting for a full CMake configure/compile pass.
     */
    static bool saveProjectStateOnly(
        const ProjectConfig& config,
        const std::vector<SchematicNode>& chain,
        const FaceplateLayout& faceplate,
        std::string& outMessage) {
        namespace fs = std::filesystem;

        const std::string pluginName = sanitizeName(config.name.empty() ? "AvraGain" : config.name);
        if (pluginName.empty()) {
            outMessage = "Invalid plugin name.";
            return false;
        }

        const fs::path targetDir = getPluginsDir(config) / pluginName;

        try {
            fs::create_directories(targetDir);
            fs::create_directories(targetDir / "resources");
            fs::create_directories(targetDir / "presets");
            fs::create_directories(targetDir / ".vscode");

            writeFile((targetDir / (pluginName + ".h")).string(), generateSchematicProcessorHeader(pluginName, config, chain));
            writeFile((targetDir / "EmbeddedAssets.h").string(), generateEmbeddedAssetsHeader(pluginName, faceplate));
            writeFile((targetDir / (pluginName + "Editor.h")).string(), generateCustomFaceplateEditorHeader(pluginName, config, faceplate));

            if (!fs::exists(targetDir / (pluginName + "Clap.cpp"))) {
                writeFile((targetDir / (pluginName + "Clap.cpp")).string(), generateClapBridge(pluginName, config));
            }
            if (!fs::exists(targetDir / (pluginName + "Vst3.cpp"))) {
                writeFile((targetDir / (pluginName + "Vst3.cpp")).string(), generateVst3Bridge(pluginName, config));
            }
            if (!fs::exists(targetDir / (pluginName + "Standalone.cpp"))) {
                writeFile((targetDir / (pluginName + "Standalone.cpp")).string(), generateStandaloneMain(pluginName, config));
            }
            if (!fs::exists(targetDir / "CMakeLists.txt")) {
                exportPluginIconResources(targetDir);
            writeFile((targetDir / "CMakeLists.txt").string(), generateStandaloneCMakeLists(pluginName));
                writeFile((targetDir / ".vscode" / "tasks.json").string(), generateVsCodeTasksJson(pluginName));
                writeFile((targetDir / ".vscode" / "launch.json").string(), generateVsCodeLaunchJson(pluginName));
                writeFile((targetDir / ".vscode" / "c_cpp_properties.json").string(), generateVsCodeCppPropertiesJson());
                updateCMakeLists(pluginName);
            }

            writeFile((targetDir / "resources" / "faceplate_theme.json").string(), generateThemeManifest(pluginName, config));
            if (!fs::exists(targetDir / "presets" / "Default.avrapreset")) {
                writeFile((targetDir / "presets" / "Default.avrapreset").string(), "# Avra Preset v1\n1:0.5\n");
            }
            writeFile((targetDir / "README.md").string(), generateProjectReadme(pluginName, config, chain));

            saveSchematicChainToFile((targetDir / (pluginName + ".schematic")).string(), chain);
            saveFaceplateLayoutToFile(targetDir / (pluginName + ".faceplate"), faceplate);

            outMessage = std::format(
                "SAVED PROJECT '{}' ({} stages & {} UI widgets) -> plugins/{}/{}.faceplate + {}.schematic + {}Editor.h",
                pluginName, chain.size(), faceplate.widgets.size(), pluginName, pluginName, pluginName, pluginName);
            return true;
        } catch (const std::exception& e) {
            outMessage = std::string("Error saving project: ") + e.what();
            return false;
        }
    }

    static bool generateFromSchematic(const ProjectConfig& config, const std::vector<SchematicNode>& chain, std::string& outMessage) {
        const std::string pluginName = sanitizeName(config.name);
        FaceplateLayout defaultUi = buildFaceplateFromSchematic(pluginName.empty() ? "CustomStrip" : pluginName, chain);
        return generateFromSchematicAndUi(config, chain, defaultUi, outMessage);
    }

    /**
     * @brief Unregisters a plugin target block from CMakeLists.txt without deleting source files on disk.
     */
    static bool unregisterFromCMakeLists(const std::string& pluginName) {
        const std::string cleanName = sanitizeName(pluginName);
        if (cleanName.empty()) return false;

        const std::string cmakePath = (getWorkspaceRoot() / "CMakeLists.txt").string();
        std::ifstream inFile(cmakePath);
        if (!inFile.is_open()) return false;

        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string content = buffer.str();
        inFile.close();

        const std::string commentHeader = "# " + cleanName + " (CLAP + VST3 + Standalone";
        const std::string legacyHeader  = "# " + cleanName + " CLAP Plugin";
        const std::string targetCall    = "avra_add_clap_plugin(Avra" + cleanName + "_Clap";

        size_t startPos = content.find(commentHeader);
        if (startPos == std::string::npos) startPos = content.find(legacyHeader);
        if (startPos == std::string::npos) startPos = content.find(targetCall);
        if (startPos == std::string::npos) {
            return true; // Already not in CMakeLists.txt
        }

        // Find end of target_include_directories(Avra<Name>_Standalone ... ) or _Clap
        size_t endPos = content.find("target_include_directories(Avra" + cleanName + "_Standalone", startPos);
        if (endPos == std::string::npos) {
            endPos = content.find("target_include_directories(Avra" + cleanName + "_Clap", startPos);
        }
        if (endPos != std::string::npos) {
            endPos = content.find(')', endPos);
            if (endPos != std::string::npos) ++endPos;
        } else {
            endPos = content.find(')', startPos);
            if (endPos != std::string::npos) ++endPos;
        }

        if (endPos != std::string::npos && endPos > startPos) {
            while (endPos < content.size() && (content[endPos] == '\r' || content[endPos] == '\n')) {
                ++endPos;
            }
            if (content.compare(endPos, 7, "endif()") == 0) {
                endPos += 7;
                while (endPos < content.size() && (content[endPos] == '\r' || content[endPos] == '\n')) {
                    ++endPos;
                }
            }
            content.erase(startPos, endPos - startPos);
            std::ofstream outFile(cmakePath, std::ios::trunc);
            outFile << content;
            outFile.close();
        }

        // Also remove any stale CMake target intermediate folders/projects in build/
        namespace fs = std::filesystem;
        std::error_code ec;
        const fs::path buildDir = getWorkspaceRoot() / "build";
        for (const char* suffix : { "_Clap", "_Vst3", "_Standalone" }) {
            const std::string targetName = "Avra" + cleanName + suffix;
            fs::remove_all(buildDir / (targetName + ".dir"), ec);
            fs::remove(buildDir / (targetName + ".vcxproj"), ec);
            fs::remove(buildDir / (targetName + ".vcxproj.filters"), ec);
            fs::remove(buildDir / (targetName + ".vcxproj.user"), ec);
        }
        return true;
    }

    struct BuiltArtifactInfo {
        std::string formatLabel;
        std::string fileName;
        std::string fullPath;
        uintmax_t   sizeBytes{0};
        bool        exists{false};
    };

    struct BuildExecutionReport {
        bool                           succeeded{false};
        std::string                    pluginName;
        std::string                    sourceFolder;
        std::string                    destinationFolder;
        double                         elapsedSeconds{0.0};
        std::vector<BuiltArtifactInfo> artifacts;
        std::vector<std::string>       logLines;
        std::string                    summaryMessage;
    };

    static BuildExecutionReport inspectProjectArtifacts(
        const std::string& diskFolder,
        bool reqVst3 = true,
        bool reqClap = false,
        bool reqStandalone = false,
        const std::filesystem::path& customExportDir = {}) {
        namespace fs = std::filesystem;
        if (!reqVst3 && !reqClap && !reqStandalone) reqVst3 = true;
        BuildExecutionReport rep;
        const std::string cleanFolder = sanitizeName(diskFolder);
        const bool isCoreGain         = (cleanFolder == "GainPlugin" || cleanFolder == "AvraGain");
        const std::string outputBase  = isCoreGain ? "AvraGain" : cleanFolder;
        const fs::path rootDir        = getWorkspaceRoot();
        std::error_code ec;
        const fs::path pluginDir      = isCoreGain
            ? (fs::exists(rootDir / "plugins" / "AvraGain", ec) ? (rootDir / "plugins" / "AvraGain") : (rootDir / "examples" / "GainPlugin"))
            : (rootDir / "plugins" / cleanFolder);
        const fs::path defaultDistDir = pluginDir / "dist";
        const fs::path effectiveDist  = customExportDir.empty() ? defaultDistDir : customExportDir;
        const fs::path relDir         = rootDir / "build" / "Release";

        std::string destStr = effectiveDist.string();
        if (!destStr.empty() && destStr.back() != '\\' && destStr.back() != '/') {
            destStr.push_back('\\');
        }

        rep.pluginName        = outputBase;
        rep.sourceFolder      = pluginDir.string() + "\\";
        rep.destinationFolder = destStr;

        const std::pair<const char*, std::string> specs[3] = {
            { "VST3 Plugin (.vst3)",       outputBase + ".vst3" },
            { "CLAP Plugin (.clap)",       outputBase + ".clap" },
            { "Standalone App (.exe)",     outputBase + "_Standalone.exe" }
        };
        const bool reqFlags[3] = { reqVst3, reqClap, reqStandalone };

        bool allRequestedExist = true;
        for (int i = 0; i < 3; ++i) {
            const auto& [label, fname] = specs[i];
            const fs::path distFile    = effectiveDist / fname;
            const fs::path defDistFile = defaultDistDir / fname;
            const fs::path relFile     = relDir / fname;
            BuiltArtifactInfo info;
            info.formatLabel = label;
            info.fileName    = fname;
            if (fs::exists(distFile, ec)) {
                info.exists    = true;
                info.fullPath  = distFile.string();
                info.sizeBytes = fs::file_size(distFile, ec);
            } else if (fs::exists(defDistFile, ec)) {
                info.exists    = true;
                info.fullPath  = defDistFile.string();
                info.sizeBytes = fs::file_size(defDistFile, ec);
            } else if (reqFlags[i] && fs::exists(relFile, ec)) {
                info.exists    = true;
                info.fullPath  = relFile.string();
                info.sizeBytes = fs::file_size(relFile, ec);
            } else {
                info.exists   = false;
                info.fullPath = distFile.string();
                if (reqFlags[i]) {
                    allRequestedExist = false;
                }
            }
            rep.artifacts.push_back(std::move(info));
        }
        rep.succeeded = allRequestedExist;
        return rep;
    }

    /**
     * @brief Verbose 4-Stage Build Pipeline with real-time progress callback & live stdout capture.
     * Compiles the checked export formats (VST3 .vst3, CLAP .clap, and/or Standalone .exe; defaults to VST3)
     * and packages the selected binaries into `<pluginDir>\dist\` (and `customExportDir` if specified).
     */
    static bool buildAllPluginFormatsVerbose(
        const std::string& diskFolder,
        BuildExecutionReport& outReport,
        std::function<void(int step, int totalSteps, const std::string& stepTitle, const std::string& liveDetail)> onProgress = nullptr,
        bool buildVst3 = true,
        bool buildClap = false,
        bool buildStandalone = false,
        const std::filesystem::path& customExportDir = {}) {
#if defined(_WIN32)
        namespace fs = std::filesystem;
        if (!buildVst3 && !buildClap && !buildStandalone) buildVst3 = true;
        const uint64_t tStartMs = GetTickCount64();

        const std::string cleanFolder  = sanitizeName(diskFolder);
        const bool isCoreGain          = (cleanFolder == "GainPlugin" || cleanFolder == "AvraGain");
        const std::string targetPrefix = isCoreGain ? "AvraGain" : ("Avra" + cleanFolder);
        const std::string outputBase   = isCoreGain ? "AvraGain" : cleanFolder;
        const fs::path rootDir         = getWorkspaceRoot();
        const fs::path pluginDir       = isCoreGain
            ? (rootDir / "plugins" / "AvraGain")
            : (rootDir / "plugins" / cleanFolder);
        const fs::path distDir         = pluginDir / "dist";
        const fs::path effectiveDist   = customExportDir.empty() ? distDir : customExportDir;

        std::string destStr = effectiveDist.string();
        if (!destStr.empty() && destStr.back() != '\\' && destStr.back() != '/') {
            destStr.push_back('\\');
        }

        outReport                   = inspectProjectArtifacts(cleanFolder, buildVst3, buildClap, buildStandalone, customExportDir);
        outReport.succeeded         = false;
        outReport.pluginName        = outputBase;
        outReport.sourceFolder      = pluginDir.string() + "\\";
        outReport.destinationFolder = destStr;
        outReport.logLines.clear();

        auto emitProgress = [&](int step, const std::string& title, const std::string& detail) {
            outReport.logLines.push_back(std::format("[{}/4] {} — {}", step, title, detail));
            if (onProgress) onProgress(step, 4, title, detail);
        };

        const auto tc = detectToolchain();
        if (!tc.canCompilePlugins()) {
            outReport.summaryMessage = tc.getPrerequisitesNotice();
            emitProgress(1, "TOOLCHAIN ERROR", outReport.summaryMessage);
            return false;
        }

        // STEP 1/4: Verify & Sync C++20 DSP + Direct2D Faceplate Sources (keep all 3 exporters in the engine)
        emitProgress(1, "Syncing C++20 DSP & Direct2D UI Sources",
                     std::format("Source Dir: {}", outReport.sourceFolder));

        if (!isCoreGain) {
            ProjectConfig cfg;
            cfg.name     = cleanFolder;
            cfg.pluginId = std::format("com.company.{}", cleanFolder);
            if (!fs::exists(pluginDir / (cleanFolder + "Clap.cpp"))) {
                writeFile((pluginDir / (cleanFolder + "Clap.cpp")).string(), generateClapBridge(cleanFolder, cfg));
            }
            if (!fs::exists(pluginDir / (cleanFolder + "Vst3.cpp"))) {
                writeFile((pluginDir / (cleanFolder + "Vst3.cpp")).string(), generateVst3Bridge(cleanFolder, cfg));
            }
            if (!fs::exists(pluginDir / (cleanFolder + "Standalone.cpp"))) {
                writeFile((pluginDir / (cleanFolder + "Standalone.cpp")).string(), generateStandaloneMain(cleanFolder, cfg));
            }
            updateCMakeLists(cleanFolder);
        } else {
            ProjectConfig cfg;
            cfg.name     = "AvraGain";
            cfg.pluginId = "com.avra.gain";
            std::error_code ec;
            fs::create_directories(pluginDir, ec);
            if (!fs::exists(pluginDir / "AvraGain.h")) {
                writeFile((pluginDir / "AvraGain.h").string(), generateSchematicProcessorHeader("AvraGain", cfg, {}));
            }
            if (!fs::exists(pluginDir / "EmbeddedAssets.h")) {
                FaceplateLayout defaultFp;
                writeFile((pluginDir / "EmbeddedAssets.h").string(), generateEmbeddedAssetsHeader("AvraGain", defaultFp));
            }
            if (!fs::exists(pluginDir / "AvraGainEditor.h")) {
                FaceplateLayout defaultFp;
                writeFile((pluginDir / "AvraGainEditor.h").string(), generateCustomFaceplateEditorHeader("AvraGain", cfg, defaultFp));
            }
            if (!fs::exists(pluginDir / "AvraGainClap.cpp")) {
                writeFile((pluginDir / "AvraGainClap.cpp").string(), generateClapBridge("AvraGain", cfg));
            }
            if (!fs::exists(pluginDir / "AvraGainVst3.cpp")) {
                writeFile((pluginDir / "AvraGainVst3.cpp").string(), generateVst3Bridge("AvraGain", cfg));
            }
            if (!fs::exists(pluginDir / "AvraGainStandalone.cpp")) {
                writeFile((pluginDir / "AvraGainStandalone.cpp").string(), generateStandaloneMain("AvraGain", cfg));
            }
            if (!fs::exists(pluginDir / "avra_plugin.rc")) {
                exportPluginIconResources(pluginDir);
            }
            updateCMakeLists("AvraGain");
        }

        const std::string vst3Target = targetPrefix + "_Vst3";
        const std::string clapTarget = targetPrefix + "_Clap";
        const std::string appTarget  = targetPrefix + "_Standalone";

        std::string selectedTargetsArgs;
        std::string selectedFormatsLabel;
        if (buildVst3) {
            selectedTargetsArgs += " " + vst3Target;
            selectedFormatsLabel += "VST3";
        }
        if (buildClap) {
            selectedTargetsArgs += " " + clapTarget;
            if (!selectedFormatsLabel.empty()) selectedFormatsLabel += " + ";
            selectedFormatsLabel += "CLAP";
        }
        if (buildStandalone) {
            selectedTargetsArgs += " " + appTarget;
            if (!selectedFormatsLabel.empty()) selectedFormatsLabel += " + ";
            selectedFormatsLabel += "Standalone .EXE";
        }

        // STEP 2/4: Configure CMake Build Tree (only if cache is stale or .vcxproj does not exist yet)
        emitProgress(2, "Configuring CMake Release x64 Build Tree",
                     std::format("Selected Targets:{}", selectedTargetsArgs));
        const fs::path vcxprojPath = rootDir / "build" / (targetPrefix + "_Vst3.vcxproj");
        if (isCMakeCacheStale() || !fs::exists(vcxprojPath)) {
            std::string configErr;
            if (!refreshCMakeBuild(onProgress, &configErr)) {
                outReport.elapsedSeconds = static_cast<double>(GetTickCount64() - tStartMs) / 1000.0;
                outReport.summaryMessage = std::format(
                    "CMAKE CONFIGURE FAILED: {}",
                    configErr.empty() ? "Could not generate Visual Studio project files." : configErr);
                emitProgress(4, "Configure Failed", outReport.summaryMessage);
                return false;
            }
        }

        // STEP 3/4: Compile Selected Format Targets via MSVC
        const std::string cmakeExe = findCMakeExecutable();
        const std::string buildDir = (rootDir / "build").string();
        const std::string rootDirStr = rootDir.string();

        const std::string compileStepTitle = std::format("Compiling Native {} (MSVC Release x64)", selectedFormatsLabel);
        emitProgress(3, compileStepTitle,
                     std::format("Running cmake --build for{}...", selectedTargetsArgs));

        std::string cmdLine = std::format(
            "\"{}\" --build \"{}\" --config Release --target{} -- /m",
            cmakeExe, buildDir, selectedTargetsArgs);

        SECURITY_ATTRIBUTES sa{};
        sa.nLength              = sizeof(sa);
        sa.bInheritHandle       = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE hReadPipe  = nullptr;
        HANDLE hWritePipe = nullptr;
        const bool hasPipe = (CreatePipe(&hReadPipe, &hWritePipe, &sa, 0) != FALSE);
        if (hasPipe) {
            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
        }

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb          = sizeof(si);
        si.dwFlags     = STARTF_USESHOWWINDOW | (hasPipe ? STARTF_USESTDHANDLES : 0);
        si.wShowWindow = SW_HIDE;
        if (hasPipe) {
            si.hStdOutput = hWritePipe;
            si.hStdError  = hWritePipe;
        }

        std::vector<char> mutableCmd(cmdLine.begin(), cmdLine.end());
        mutableCmd.push_back('\0');

        if (CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, hasPipe ? TRUE : FALSE,
                           CREATE_NO_WINDOW, nullptr, rootDirStr.c_str(), &si, &pi)) {
            if (hasPipe && hWritePipe) {
                CloseHandle(hWritePipe);
                hWritePipe = nullptr;
            }

            std::string rollingBuffer;
            std::string lastCompilerError;
            if (hasPipe && hReadPipe) {
                char chunk[512];
                DWORD bytesRead = 0;
                while (ReadFile(hReadPipe, chunk, sizeof(chunk) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                    chunk[bytesRead] = '\0';
                    rollingBuffer.append(chunk, bytesRead);
                    size_t nlPos = 0;
                    while ((nlPos = rollingBuffer.find('\n')) != std::string::npos) {
                        std::string line = rollingBuffer.substr(0, nlPos);
                        rollingBuffer.erase(0, nlPos + 1);
                        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                        size_t firstNonSpace = line.find_first_not_of(" \t");
                        if (firstNonSpace != std::string::npos) {
                            line = line.substr(firstNonSpace);
                        }
                        if (!line.empty()) {
                            std::string lowerLine = line;
                            for (char& ch : lowerLine) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                            if (lowerLine.find("error ") != std::string::npos ||
                                lowerLine.find("error:") != std::string::npos ||
                                lowerLine.find("failed") != std::string::npos ||
                                lowerLine.find("msb1009") != std::string::npos) {
                                lastCompilerError = line;
                            }
                            if (onProgress) {
                                onProgress(3, 4, compileStepTitle, line);
                            }
                        }
                    }
                }
                CloseHandle(hReadPipe);
                hReadPipe = nullptr;
            }

            WaitForSingleObject(pi.hProcess, 180000);
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (exitCode == 0) {
                // STEP 4/4: Copy only selected binaries into <pluginDir>/dist/ (and customExportDir if set) and prune unchecked formats
                const fs::path relDir = rootDir / "build" / "Release";
                std::error_code ec;
                fs::create_directories(distDir, ec);
                if (!customExportDir.empty()) {
                    fs::create_directories(customExportDir, ec);
                }

                const fs::path vst3Bin = relDir / (outputBase + ".vst3");
                const fs::path clapBin = relDir / (outputBase + ".clap");
                const fs::path appBin  = relDir / (outputBase + "_Standalone.exe");

                if (buildVst3 && fs::exists(vst3Bin)) {
                    fs::copy_file(vst3Bin, distDir / vst3Bin.filename(), fs::copy_options::overwrite_existing, ec);
                    if (!customExportDir.empty()) {
                        fs::copy_file(vst3Bin, customExportDir / vst3Bin.filename(), fs::copy_options::overwrite_existing, ec);
                    }
                    const fs::path commonVst3Dir("C:\\Program Files\\Common Files\\VST3");
                    if (fs::exists(commonVst3Dir, ec)) {
                        fs::copy_file(vst3Bin, commonVst3Dir / vst3Bin.filename(), fs::copy_options::overwrite_existing, ec);
                    }
                } else if (!buildVst3 && fs::exists(distDir / vst3Bin.filename(), ec)) {
                    fs::remove(distDir / vst3Bin.filename(), ec);
                }

                if (buildClap && fs::exists(clapBin)) {
                    fs::copy_file(clapBin, distDir / clapBin.filename(), fs::copy_options::overwrite_existing, ec);
                    if (!customExportDir.empty()) {
                        fs::copy_file(clapBin, customExportDir / clapBin.filename(), fs::copy_options::overwrite_existing, ec);
                    }
                } else if (!buildClap && fs::exists(distDir / clapBin.filename(), ec)) {
                    fs::remove(distDir / clapBin.filename(), ec);
                }

                if (buildStandalone && fs::exists(appBin)) {
                    fs::copy_file(appBin, distDir / appBin.filename(), fs::copy_options::overwrite_existing, ec);
                    if (!customExportDir.empty()) {
                        fs::copy_file(appBin, customExportDir / appBin.filename(), fs::copy_options::overwrite_existing, ec);
                    }
                } else if (!buildStandalone && fs::exists(distDir / appBin.filename(), ec)) {
                    fs::remove(distDir / appBin.filename(), ec);
                }

                if (isCoreGain && fs::exists(rootDir / "examples" / "GainPlugin", ec)) {
                    const fs::path legacyDist = rootDir / "examples" / "GainPlugin" / "dist";
                    fs::create_directories(legacyDist, ec);
                    if (buildVst3 && fs::exists(vst3Bin)) fs::copy_file(vst3Bin, legacyDist / vst3Bin.filename(), fs::copy_options::overwrite_existing, ec);
                    if (buildClap && fs::exists(clapBin)) fs::copy_file(clapBin, legacyDist / clapBin.filename(), fs::copy_options::overwrite_existing, ec);
                    if (buildStandalone && fs::exists(appBin)) fs::copy_file(appBin, legacyDist / appBin.filename(), fs::copy_options::overwrite_existing, ec);
                }

                outReport = inspectProjectArtifacts(cleanFolder, buildVst3, buildClap, buildStandalone, customExportDir);
                outReport.succeeded = true;
                outReport.elapsedSeconds = static_cast<double>(GetTickCount64() - tStartMs) / 1000.0;

                uintmax_t vst3Kb = (!outReport.artifacts.empty()) ? (outReport.artifacts[0].sizeBytes / 1024) : 0;
                uintmax_t clapKb = (outReport.artifacts.size() > 1) ? (outReport.artifacts[1].sizeBytes / 1024) : 0;
                uintmax_t appKb  = (outReport.artifacts.size() > 2) ? (outReport.artifacts[2].sizeBytes / 1024) : 0;

                std::string builtList;
                if (buildVst3) {
                    builtList += std::format("{}.vst3 ({} KB)", outputBase, vst3Kb);
                }
                if (buildClap) {
                    if (!builtList.empty()) builtList += " + ";
                    builtList += std::format("{}.clap ({} KB)", outputBase, clapKb);
                }
                if (buildStandalone) {
                    if (!builtList.empty()) builtList += " + ";
                    builtList += std::format("{}_Standalone.exe ({} KB)", outputBase, appKb);
                }

                outReport.summaryMessage = std::format(
                    "BUILD COMPLETE ({:.1f}s): {} -> {}",
                    outReport.elapsedSeconds, builtList, outReport.destinationFolder);
                emitProgress(4, "Packaged & Verified Binaries in Destination Folder", outReport.summaryMessage);
                return true;
            }

            outReport.elapsedSeconds = static_cast<double>(GetTickCount64() - tStartMs) / 1000.0;
            outReport.summaryMessage = std::format(
                "BUILD FAILED (Exit Code {}): {}",
                exitCode,
                lastCompilerError.empty() ? std::format("Targets{} failed to compile.", selectedTargetsArgs) : lastCompilerError);
            emitProgress(4, "Build Failed", outReport.summaryMessage);
            return false;
        }

        if (hasPipe) {
            if (hReadPipe)  CloseHandle(hReadPipe);
            if (hWritePipe) CloseHandle(hWritePipe);
        }
        outReport.summaryMessage = "ERROR: Could not launch CMake build process. Ensure CMake and MSVC Build Tools are installed.";
        return false;
#else
        (void)diskFolder;
        (void)onProgress;
        (void)buildVst3;
        (void)buildClap;
        (void)buildStandalone;
        (void)customExportDir;
        outReport.summaryMessage = "Build only supported on Windows.";
        return false;
#endif
    }

    /**
     * @brief Directly compiles VST3 (.vst3), CLAP (.clap), and Standalone Desktop App (.exe)
     * for the given plugin folder and copies the built binaries into `plugins/<Name>/dist/`.
     */
    static bool buildAllPluginFormats(
        const std::string& diskFolder,
        std::string& outLog,
        const std::filesystem::path& customExportDir = {}) {
        BuildExecutionReport rep;
        const bool ok = buildAllPluginFormatsVerbose(diskFolder, rep, nullptr, true, true, true, customExportDir);
        outLog = rep.summaryMessage;
        return ok;
    }

    static bool buildPluginTarget(const std::string& targetName, std::string& outLog) {
        std::string folder = targetName;
        if (folder.starts_with("Avra")) folder = folder.substr(4);
        if (folder.ends_with("_Clap")) folder = folder.substr(0, folder.size() - 5);
        if (folder == "Gain") folder = "GainPlugin";
        return buildAllPluginFormats(folder, outLog);
    }

    /**
     * @brief Launches the compiled Standalone Desktop App (`<PluginName>_Standalone.exe`),
     * preferring the packaged binary inside `customExportDir` or `<pluginDir>/dist/`.
     */
    static bool launchStandaloneExe(
        const std::string& diskFolder,
        std::string& outLog,
        std::string* outExePath = nullptr,
        const std::filesystem::path& customExportDir = {}) {
#if defined(_WIN32)
        namespace fs = std::filesystem;
        const std::string cleanFolder = sanitizeName(diskFolder);
        const bool isCoreGain         = (cleanFolder == "GainPlugin" || cleanFolder == "AvraGain");
        const std::string outputBase  = isCoreGain ? "AvraGain" : cleanFolder;
        const fs::path rootDir        = getWorkspaceRoot();
        std::error_code ec;
        const fs::path pluginDir      = isCoreGain
            ? (fs::exists(rootDir / "plugins" / "AvraGain", ec) ? (rootDir / "plugins" / "AvraGain") : (rootDir / "examples" / "GainPlugin"))
            : (rootDir / "plugins" / cleanFolder);
        const fs::path distDir        = pluginDir / "dist";
        const fs::path effectiveDist  = customExportDir.empty() ? distDir : customExportDir;
        const fs::path relDir         = rootDir / "build" / "Release";

        fs::path customExe = effectiveDist / (outputBase + "_Standalone.exe");
        fs::path distExe   = distDir / (outputBase + "_Standalone.exe");
        fs::path relExe    = relDir / (outputBase + "_Standalone.exe");

        if (!fs::exists(customExe, ec) && fs::exists(distExe, ec)) {
            fs::create_directories(effectiveDist, ec);
            fs::copy_file(distExe, customExe, fs::copy_options::overwrite_existing, ec);
        } else if (!fs::exists(customExe, ec) && fs::exists(relExe, ec)) {
            fs::create_directories(effectiveDist, ec);
            fs::copy_file(relExe, customExe, fs::copy_options::overwrite_existing, ec);
        }

        fs::path targetExe = fs::exists(customExe, ec) ? customExe : (fs::exists(distExe, ec) ? distExe : relExe);
        if (!fs::exists(targetExe, ec)) {
            if (!buildAllPluginFormats(cleanFolder, outLog, customExportDir)) {
                return false;
            }
            targetExe = fs::exists(customExe, ec) ? customExe : (fs::exists(distExe, ec) ? distExe : relExe);
        }

        if (fs::exists(targetExe, ec)) {
            if (outExePath) *outExePath = targetExe.string();
            HINSTANCE res = ShellExecuteA(nullptr, "open", targetExe.string().c_str(), nullptr,
                                          targetExe.parent_path().string().c_str(), SW_SHOWNORMAL);
            if (reinterpret_cast<uintptr_t>(res) > 32) {
                outLog = std::format("RUNNING STANDALONE .EXE: Launched '{}' (48 kHz Live Desktop Host Window)!",
                                     targetExe.string());
                return true;
            }
        }
        outLog = std::format("ERROR: Could not launch {}_Standalone.exe from {}", outputBase, distDir.string());
        return false;
#else
        (void)diskFolder;
        (void)outExePath;
        outLog = "Standalone execution only supported on Windows.";
        return false;
#endif
    }

    static std::string sanitizeName(std::string name) {
        std::string result;
        for (char c : name) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                result.push_back(c);
            }
        }
        return result;
    }

    static bool isCMakeCacheStale() {
        namespace fs = std::filesystem;
        std::error_code ec;
        const fs::path rootDir   = getWorkspaceRoot();
        const fs::path cachePath = rootDir / "build" / "CMakeCache.txt";
        if (!fs::exists(cachePath, ec)) return true;

        std::ifstream in(cachePath);
        std::string line;
        std::string expectedRoot = rootDir.generic_string();
        for (char& c : expectedRoot) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        while (std::getline(in, line)) {
            if (line.starts_with("CMAKE_HOME_DIRECTORY:INTERNAL=")) {
                std::string cachedRoot = line.substr(std::string("CMAKE_HOME_DIRECTORY:INTERNAL=").size());
                while (!cachedRoot.empty() && (cachedRoot.back() == '\r' || cachedRoot.back() == '\n')) cachedRoot.pop_back();
                const fs::path cachedPath(cachedRoot);
                if (fs::exists(cachedPath, ec) && fs::equivalent(cachedPath, rootDir, ec)) {
                    return false;
                }
                for (char& c : cachedRoot) {
                    if (c == '\\') c = '/';
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return (cachedRoot != expectedRoot);
            }
        }
        return false;
    }

private:
    static std::string findCMakeExecutable() {
        namespace fs = std::filesystem;
        const char* candidates[] = {
            "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
            "C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
            "C:\\Program Files\\CMake\\bin\\cmake.exe"
        };
        for (const char* path : candidates) {
            if (fs::exists(path)) return path;
        }
        return "cmake.exe";
    }

    static bool refreshCMakeBuild(
        const std::function<void(int, int, const std::string&, const std::string&)>& onProgress = nullptr,
        std::string* outLastError = nullptr) {
#if defined(_WIN32)
        namespace fs = std::filesystem;
        const std::string cmakeExe = findCMakeExecutable();
        const fs::path rootPath    = getWorkspaceRoot();
        const fs::path buildPath   = rootPath / "build";

        std::error_code ec;
        const bool stale = isCMakeCacheStale();
        if (stale) {
            fs::remove(buildPath / "CMakeCache.txt", ec);
            fs::remove_all(buildPath / "CMakeFiles", ec);
        }
        fs::create_directories(buildPath, ec);

        std::string cmdLine = std::format("\"{}\" -B \"{}\" -S \"{}\"", cmakeExe, buildPath.string(), rootPath.string());
        std::vector<char> mutableCmd(cmdLine.begin(), cmdLine.end());
        mutableCmd.push_back('\0');

        SECURITY_ATTRIBUTES sa{};
        sa.nLength              = sizeof(sa);
        sa.bInheritHandle       = TRUE;
        sa.lpSecurityDescriptor = nullptr;

        HANDLE hReadPipe  = nullptr;
        HANDLE hWritePipe = nullptr;
        const bool hasPipe = (CreatePipe(&hReadPipe, &hWritePipe, &sa, 0) != FALSE);
        if (hasPipe) {
            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
        }

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};
        si.cb          = sizeof(si);
        si.dwFlags     = STARTF_USESHOWWINDOW | (hasPipe ? STARTF_USESTDHANDLES : 0);
        si.wShowWindow = SW_HIDE;
        if (hasPipe) {
            si.hStdOutput = hWritePipe;
            si.hStdError  = hWritePipe;
        }

        const std::string rootStr = rootPath.string();
        if (CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, hasPipe ? TRUE : FALSE,
                           CREATE_NO_WINDOW, nullptr, rootStr.c_str(), &si, &pi)) {
            if (hasPipe && hWritePipe) {
                CloseHandle(hWritePipe);
                hWritePipe = nullptr;
            }

            std::string rollingBuffer;
            std::string lastErrLine;
            if (hasPipe && hReadPipe) {
                char chunk[512];
                DWORD bytesRead = 0;
                while (ReadFile(hReadPipe, chunk, sizeof(chunk) - 1, &bytesRead, nullptr) && bytesRead > 0) {
                    chunk[bytesRead] = '\0';
                    rollingBuffer.append(chunk, bytesRead);
                    size_t nlPos = 0;
                    while ((nlPos = rollingBuffer.find('\n')) != std::string::npos) {
                        std::string line = rollingBuffer.substr(0, nlPos);
                        rollingBuffer.erase(0, nlPos + 1);
                        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                        size_t firstNonSpace = line.find_first_not_of(" \t");
                        if (firstNonSpace != std::string::npos) {
                            line = line.substr(firstNonSpace);
                        }
                        if (!line.empty()) {
                            std::string lowerLine = line;
                            for (char& ch : lowerLine) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                            if (lowerLine.find("error") != std::string::npos ||
                                lowerLine.find("failed") != std::string::npos) {
                                lastErrLine = line;
                            }
                            if (onProgress) {
                                onProgress(2, 4, "Configuring CMake Release x64 Build Tree", line);
                            }
                        }
                    }
                }
                CloseHandle(hReadPipe);
                hReadPipe = nullptr;
            }

            WaitForSingleObject(pi.hProcess, 180000);
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            if (outLastError && !lastErrLine.empty()) {
                *outLastError = lastErrLine;
            }
            return (exitCode == 0) && fs::exists(buildPath / "CMakeCache.txt", ec);
        }

        if (hasPipe) {
            if (hReadPipe)  CloseHandle(hReadPipe);
            if (hWritePipe) CloseHandle(hWritePipe);
        }
        if (outLastError) {
            *outLastError = "Could not start CMake configuration process.";
        }
        return false;
#else
        (void)onProgress;
        (void)outLastError;
        return false;
#endif
    }

    static void writeFile(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::trunc);
        file << content;
    }

    static void updateCMakeLists(const std::string& pluginName) {
        const bool isGain = (pluginName == "AvraGain" || pluginName == "GainPlugin");
        if (isGain) {
            std::error_code ec;
            if (std::filesystem::exists(getWorkspaceRoot() / "examples" / "GainPlugin" / "GainPluginClap.cpp", ec)) {
                return; // Core AvraGain targets in dev workspace are declared via examples/GainPlugin
            }
        }
        const std::string cmakePath = (getWorkspaceRoot() / "CMakeLists.txt").string();
        std::ifstream inFile(cmakePath);
        if (!inFile.is_open()) return;

        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string content = buffer.str();
        inFile.close();

        const std::string targetPrefix = isGain ? "AvraGain" : ("Avra" + pluginName);
        const std::string folderName   = isGain ? "AvraGain" : pluginName;
        const std::string vst3Decl     = "avra_add_vst3_plugin(" + targetPrefix + "_Vst3";
        if (content.find(vst3Decl) != std::string::npos &&
            (!isGain || content.find("plugins/AvraGain/AvraGainClap.cpp") != std::string::npos)) {
            return; // Already registered with VST3 + CLAP + Standalone
        }

        // Remove any older CLAP-only registration first so we replace it cleanly
        unregisterFromCMakeLists(folderName);
        {
            std::ifstream reload(cmakePath);
            std::stringstream b2;
            b2 << reload.rdbuf();
            content = b2.str();
        }

        std::ostringstream newTarget;
        newTarget << "\n# " << folderName << " (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)\n"
                  << "if(EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/plugins/" << folderName << "/" << folderName << "Clap.cpp\")\n"
                  << "avra_add_clap_plugin(" << targetPrefix << "_Clap\n"
                  << "    OUTPUT_NAME \"" << folderName << "\"\n"
                  << "    SOURCES\n"
                  << "        plugins/" << folderName << "/" << folderName << "Clap.cpp\n"
                  << ")\n"
                  << "target_include_directories(" << targetPrefix << "_Clap PRIVATE\n"
                  << "    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/" << folderName << "\n"
                  << ")\n\n"
                  << "avra_add_vst3_plugin(" << targetPrefix << "_Vst3\n"
                  << "    OUTPUT_NAME \"" << folderName << "\"\n"
                  << "    SOURCES\n"
                  << "        plugins/" << folderName << "/" << folderName << "Vst3.cpp\n"
                  << ")\n"
                  << "target_include_directories(" << targetPrefix << "_Vst3 PRIVATE\n"
                  << "    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/" << folderName << "\n"
                  << ")\n\n"
                  << "avra_add_standalone_plugin(" << targetPrefix << "_Standalone\n"
                  << "    OUTPUT_NAME \"" << folderName << "\"\n"
                  << "    SOURCES\n"
                  << "        plugins/" << folderName << "/" << folderName << "Standalone.cpp\n"
                  << ")\n"
                  << "target_include_directories(" << targetPrefix << "_Standalone PRIVATE\n"
                  << "    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/" << folderName << "\n"
                  << ")\n"
                  << "endif()\n";

        const std::string marker = "# Avra Studio Desktop Application (Avra.exe)";
        auto pos = content.find(marker);
        if (pos != std::string::npos) {
            content.insert(pos, newTarget.str() + "\n");
        } else {
            content += newTarget.str();
        }

        std::ofstream outFile(cmakePath, std::ios::trunc);
        outFile << content;
    }

    static std::string generateVst3Bridge(const std::string& name, const ProjectConfig& cfg) {
        std::string catLower = cfg.category;
        for (char& c : catLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        const char* vst3SubCat = "Fx|Tools";
        if (catLower.find("eq") != std::string::npos || catLower.find("filter") != std::string::npos || catLower.find("spectral") != std::string::npos) {
            vst3SubCat = "Fx|EQ";
        } else if (catLower.find("comp") != std::string::npos || catLower.find("limit") != std::string::npos || catLower.find("dynam") != std::string::npos || catLower.find("clip") != std::string::npos || catLower.find("gate") != std::string::npos) {
            vst3SubCat = "Fx|Dynamics";
        } else if (catLower.find("reverb") != std::string::npos || catLower.find("plate") != std::string::npos || catLower.find("hall") != std::string::npos || catLower.find("chamber") != std::string::npos || catLower.find("room") != std::string::npos) {
            vst3SubCat = "Fx|Reverb";
        } else if (catLower.find("delay") != std::string::npos || catLower.find("echo") != std::string::npos || catLower.find("tape") != std::string::npos) {
            vst3SubCat = "Fx|Delay";
        } else if (catLower.find("mod") != std::string::npos || catLower.find("chorus") != std::string::npos || catLower.find("flang") != std::string::npos || catLower.find("phas") != std::string::npos) {
            vst3SubCat = "Fx|Modulation";
        } else if (catLower.find("master") != std::string::npos || catLower.find("pre") != std::string::npos || catLower.find("tube") != std::string::npos || catLower.find("strip") != std::string::npos || catLower.find("console") != std::string::npos || catLower.find("custom") != std::string::npos) {
            vst3SubCat = "Fx|Mastering";
        }

        std::ostringstream ss;
        ss << "#include \"" << name << ".h\"\n"
           << "#include \"" << name << "Editor.h\"\n"
           << "#include \"avra/format/vst3/Vst3Plugin.h\"\n\n"
           << "AVRA_EXPORT_VST3(\n"
           << "    avra::plugins::" << name << ",\n"
           << "    \"" << cfg.pluginId << "\",\n"
           << "    \"" << name << "\",\n"
           << "    \"" << cfg.vendor << "\",\n"
           << "    \"https://github.com/angryk10-ctrl/Avra\",\n"
           << "    \"1.0.0\",\n"
           << "    \"" << vst3SubCat << "\"\n"
           << ")\n";
        return ss.str();
    }

    static std::string generateStandaloneMain(const std::string& name, const ProjectConfig& cfg) {
        (void)cfg;
        std::ostringstream ss;
        ss << "#include \"" << name << ".h\"\n"
           << "#include \"" << name << "Editor.h\"\n"
           << "#include \"avra/format/standalone/StandaloneApp.h\"\n\n"
           << "AVRA_EXPORT_STANDALONE(\n"
           << "    avra::plugins::" << name << ",\n"
           << "    L\"" << name << " \xE2\x80\x94 Avra Standalone Desktop App (Free Non-Commercial)\"\n"
           << ")\n";
        return ss.str();
    }

    static void exportPluginIconResources(const std::filesystem::path& targetDir) {
        namespace fs = std::filesystem;
        std::error_code ec;
        const fs::path srcIco = getWorkspaceRoot() / "assets" / "avra_studio.ico";
        const fs::path dstIco = targetDir / "avra_studio.ico";
        if (fs::exists(srcIco, ec)) {
            fs::copy_file(srcIco, dstIco, fs::copy_options::overwrite_existing, ec);
        }
        writeFile(
            (targetDir / "avra_plugin.rc").string(),
            "// Generated by Avra Studio\n"
            "#include <windows.h>\n"
            "1 ICON \"avra_studio.ico\"\n"
            "101 ICON \"avra_studio.ico\"\n\n"
            "VS_VERSION_INFO VERSIONINFO\n"
            " FILEVERSION 1,0,0,1\n"
            " PRODUCTVERSION 1,0,0,1\n"
            " FILEFLAGSMASK 0x3fL\n"
            " FILEFLAGS 0x0L\n"
            " FILEOS 0x40004L\n"
            " FILETYPE 0x2L\n"
            " FILESUBTYPE 0x0L\n"
            "BEGIN\n"
            "    BLOCK \"StringFileInfo\"\n"
            "    BEGIN\n"
            "        BLOCK \"040904b0\"\n"
            "        BEGIN\n"
            "            VALUE \"CompanyName\", \"Avra Audio (https://github.com/angryk10-ctrl/Avra)\"\n"
            "            VALUE \"FileDescription\", \"Created with Avra Studio - Free Non-Commercial Audio Plugin\"\n"
            "            VALUE \"FileVersion\", \"1.0.0.1\"\n"
            "            VALUE \"LegalCopyright\", \"Created with Avra Studio - Free Non-Commercial Use Only (Not for Sale)\"\n"
            "            VALUE \"ProductName\", \"Avra Studio Community Plugin (Free Non-Commercial)\"\n"
            "            VALUE \"ProductVersion\", \"1.0.0.1\"\n"
            "        END\n"
            "    END\n"
            "    BLOCK \"VarFileInfo\"\n"
            "    BEGIN\n"
            "        VALUE \"Translation\", 0x409, 1200\n"
            "    END\n"
            "END\n");
    }

    static std::string generateStandaloneCMakeLists(const std::string& name) {
        std::ostringstream ss;
        ss << "cmake_minimum_required(VERSION 3.20)\n"
           << "project(" << name << " VERSION 1.0.0 LANGUAGES CXX)\n\n"
           << "set(CMAKE_CXX_STANDARD 20)\n"
           << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n"
           << "# Relative path to the Avra SDK include directory (zero external JUCE/Steinberg SDK dependencies)\n"
           << "get_filename_component(AVRA_SDK_ROOT \"${CMAKE_CURRENT_SOURCE_DIR}/../..\" ABSOLUTE)\n\n"
           << "if(MSVC)\n"
           << "    add_compile_options(/W4 /utf-8 /permissive- $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi> $<$<CONFIG:Release>:/Ot> $<$<CONFIG:Release>:/fp:fast> $<$<CONFIG:Release>:/arch:AVX2>)\n"
           << "    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)\n"
           << "endif()\n\n"
           << "set(AVRA_PLUGIN_RC \"\")\n"
           << "if(WIN32 AND EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/avra_plugin.rc\")\n"
           << "    set(AVRA_PLUGIN_RC \"avra_plugin.rc\")\n"
           << "endif()\n\n"
           << "# 1. Native VST3 Plugin Target (" << name << ".vst3)\n"
           << "add_library(" << name << "_Vst3 MODULE\n"
           << "    " << name << "Vst3.cpp\n"
           << "    ${AVRA_PLUGIN_RC}\n"
           << ")\n"
           << "set_target_properties(" << name << "_Vst3 PROPERTIES PREFIX \"\" SUFFIX \".vst3\" OUTPUT_NAME \"" << name << "\")\n"
           << "target_include_directories(" << name << "_Vst3 PRIVATE \"${CMAKE_CURRENT_SOURCE_DIR}\" \"${AVRA_SDK_ROOT}/include\")\n"
           << "if(WIN32)\n"
           << "    target_link_libraries(" << name << "_Vst3 PRIVATE d2d1 dwrite user32 gdi32)\n"
           << "endif()\n\n"
           << "# 2. Standalone Desktop Application Target (" << name << "_Standalone.exe)\n"
           << "add_executable(" << name << "_Standalone WIN32\n"
           << "    " << name << "Standalone.cpp\n"
           << "    ${AVRA_PLUGIN_RC}\n"
           << ")\n"
           << "target_include_directories(" << name << "_Standalone PRIVATE \"${CMAKE_CURRENT_SOURCE_DIR}\" \"${AVRA_SDK_ROOT}/include\")\n"
           << "if(WIN32)\n"
           << "    target_link_libraries(" << name << "_Standalone PRIVATE d2d1 dwrite user32 gdi32 shell32 comdlg32 winmm)\n"
           << "endif()\n";
        return ss.str();
    }

    static std::string generateVsCodeTasksJson(const std::string& name) {
        std::ostringstream ss;
        ss << "{\n"
           << "  \"version\": \"2.0.0\",\n"
           << "  \"tasks\": [\n"
           << "    {\n"
           << "      \"label\": \"Avra: Build VST3 + CLAP + Standalone (.exe)\",\n"
           << "      \"type\": \"shell\",\n"
           << "      \"command\": \"cmake\",\n"
           << "      \"args\": [\n"
           << "        \"--build\", \"${workspaceFolder}/../../build\",\n"
           << "        \"--config\", \"Release\",\n"
           << "        \"--target\", \"Avra" << name << "_Vst3\", \"Avra" << name << "_Clap\", \"Avra" << name << "_Standalone\"\n"
           << "      ],\n"
           << "      \"group\": {\n"
           << "        \"kind\": \"build\",\n"
           << "        \"isDefault\": true\n"
           << "      },\n"
           << "      \"problemMatcher\": [\"$msCompile\"]\n"
           << "    },\n"
           << "    {\n"
           << "      \"label\": \"Avra: Build VST3 Only (.vst3)\",\n"
           << "      \"type\": \"shell\",\n"
           << "      \"command\": \"cmake\",\n"
           << "      \"args\": [\"--build\", \"${workspaceFolder}/../../build\", \"--config\", \"Release\", \"--target\", \"Avra" << name << "_Vst3\"],\n"
           << "      \"problemMatcher\": [\"$msCompile\"]\n"
           << "    },\n"
           << "    {\n"
           << "      \"label\": \"Avra: Build Standalone Desktop App (.exe)\",\n"
           << "      \"type\": \"shell\",\n"
           << "      \"command\": \"cmake\",\n"
           << "      \"args\": [\"--build\", \"${workspaceFolder}/../../build\", \"--config\", \"Release\", \"--target\", \"Avra" << name << "_Standalone\"],\n"
           << "      \"problemMatcher\": [\"$msCompile\"]\n"
           << "    }\n"
           << "  ]\n"
           << "}\n";
        return ss.str();
    }

    static std::string generateVsCodeLaunchJson(const std::string& name) {
        std::ostringstream ss;
        ss << "{\n"
           << "  \"version\": \"0.2.0\",\n"
           << "  \"configurations\": [\n"
           << "    {\n"
           << "      \"name\": \"Run " << name << " Standalone (.exe)\",\n"
           << "      \"type\": \"cppvsdbg\",\n"
           << "      \"request\": \"launch\",\n"
           << "      \"program\": \"${workspaceFolder}/../../build/Release/" << name << "_Standalone.exe\",\n"
           << "      \"args\": [],\n"
           << "      \"stopAtEntry\": false,\n"
           << "      \"cwd\": \"${workspaceFolder}\",\n"
           << "      \"environment\": [],\n"
           << "      \"preLaunchTask\": \"Avra: Build VST3 + CLAP + Standalone (.exe)\"\n"
           << "    }\n"
           << "  ]\n"
           << "}\n";
        return ss.str();
    }

    static std::string generateVsCodeCppPropertiesJson() {
        return R"({
  "configurations": [
    {
      "name": "Win32-Avra-C++20",
      "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/../../include"
      ],
      "defines": [
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN"
      ],
      "cppStandard": "c++20",
      "intelliSenseMode": "windows-msvc-x64"
    }
  ],
  "version": 4
}
)";
    }

    static std::string generateThemeManifest(const std::string& name, const ProjectConfig& cfg) {
        std::ostringstream ss;
        ss << "{\n"
           << "  \"pluginName\": \"" << name << "\",\n"
           << "  \"vendor\": \"" << cfg.vendor << "\",\n"
           << "  \"pluginId\": \"" << cfg.pluginId << "\",\n"
           << "  \"category\": \"" << cfg.category << "\",\n"
           << "  \"formats\": [\"VST3\", \"CLAP\", \"Standalone .EXE\"],\n"
           << "  \"chassisFinish\": \"AnodizedTeal\",\n"
           << "  \"knobStyle\": \"PointerSkirted\",\n"
           << "  \"meterStyle\": \"AnalogVuNeedle\",\n"
           << "  \"framework\": \"Avra C++20 Native (Zero JUCE)\"\n"
           << "}\n";
        return ss.str();
    }

    static std::string generateProjectReadme(const std::string& name, const ProjectConfig& cfg, const std::vector<SchematicNode>& chain) {
        std::ostringstream ss;
        ss << "# " << name << " (" << cfg.category << ")\n\n"
           << "Generated by **Avra Studio** — 100% Standalone C++20 Audio Plugin & Hardware UI Framework (Zero JUCE dependencies).\n\n"
           << "## Required Prerequisites for Direct VST3, CLAP & Standalone Compilation\n"
           << "1. **Visual Studio Code** (`https://code.visualstudio.com`) with the **C/C++** and **CMake Tools** extensions, OR **Visual Studio 2022+**.\n"
           << "2. **MSVC C++20 Build Tools** (*Desktop development with C++* workload in Visual Studio Installer).\n"
           << "3. **CMake 3.20+** (included with Visual Studio C++ Build Tools or `https://cmake.org`).\n\n"
           << "## Building in VS Code (`Ctrl+Shift+B`) or Avra Studio\n"
           << "- **Inside Avra Studio**: Click **`BUILD VST3 + CLAP + APP`** to compile `" << name << ".vst3`, `" << name << ".clap`, and `" << name << "_Standalone.exe` directly into `./dist/`, and click **`RUN STANDALONE .EXE`** to launch the desktop app.\n"
           << "- **Inside VS Code**: Open this folder (`plugins/" << name << "`) in VS Code and press **`Ctrl+Shift+B`** (runs `.vscode/tasks.json`) or press **`F5`** to build and debug `" << name << "_Standalone.exe`.\n\n"
           << "## Project Folder Structure (Relative Paths)\n"
           << "- `" << name << ".h` — Real-time C++20 DSP & Hardware Circuit Processor\n"
           << "- `" << name << "Editor.h` — 60 FPS Direct2D Brushed-Anodized Hardware Faceplate UI\n"
           << "- `" << name << "Vst3.cpp` — Native VST3 C-ABI COM Plugin Entry Point (`.vst3`)\n"
           << "- `" << name << "Clap.cpp` — CLAP C-ABI DAW Plugin Entry Point (`.clap`)\n"
           << "- `" << name << "Standalone.cpp` — Standalone Win32 Desktop Application Entry Point (`_Standalone.exe`)\n"
           << "- `.vscode/tasks.json` & `.vscode/launch.json` — Pre-configured VS Code `Ctrl+Shift+B` & `F5` build/debug pipeline\n"
           << "- `CMakeLists.txt` — Self-contained CMake build configuration using relative SDK paths (`../../include`)\n"
           << "- `presets/Default.avrapreset` — Default factory preset state\n"
           << "- `resources/faceplate_theme.json` — Hardware faceplate material & theme configuration\n\n";
        if (!chain.empty()) {
            ss << "## Hardware Schematic Signal Chain (" << chain.size() << " Stages)\n";
            for (size_t i = 0; i < chain.size(); ++i) {
                const auto& meta = getComponentMeta(chain[i].type);
                ss << (i + 1) << ". **[" << meta.designator << "] " << meta.fullName << "** (`" << meta.cppClassName << "`) — "
                   << (chain[i].isParallel ? "PARALLEL WET/DRY BUS" : "SERIES PATH") << "\n";
            }
        }
        return ss.str();
    }

    static std::string generateProcessorHeader(const std::string& name, const ProjectConfig& cfg) {
        std::ostringstream ss;
        ss << "#pragma once\n\n"
           << "#include \"avra/Avra.h\"\n"
           << "#include <cmath>\n"
           << "#include <memory>\n"
           << "#include <atomic>\n"
           << "#include <algorithm>\n\n"
           << "namespace avra::plugins {\n\n"
           << "/**\n"
           << " * @brief " << name << " - " << cfg.category << " Audio Processor.\n"
           << " * Generated by Avra Studio Project Wizard using Avra C++20 Core.\n"
           << " */\n"
           << "class " << name << " : public Processor {\n"
           << "public:\n"
           << "    enum ParamIds : ParameterId {\n"
           << "        kParam1Id = 1,\n"
           << "        kParam2Id = 2,\n"
           << "        kParam3Id = 3,\n"
           << "        kParam4Id = 4\n"
           << "    };\n\n";

        switch (cfg.archetype) {
            case PluginArchetype::Mastering:
                ss << "    " << name << "() {\n"
                   << "        pThreshold_ = &parameters_.add({ .id = kParam1Id, .name = \"Threshold\", .unit = \"dBFS\", .minValue = -24.0f, .maxValue = 0.0f, .defaultValue = -6.0f });\n"
                   << "        pCeiling_   = &parameters_.add({ .id = kParam2Id, .name = \"Ceiling\",   .unit = \"dBFS\", .minValue = -12.0f, .maxValue = 0.0f, .defaultValue = -0.1f });\n"
                   << "        pRelease_   = &parameters_.add({ .id = kParam3Id, .name = \"Release\",   .unit = \"ms\",   .minValue = 10.0f,  .maxValue = 800.0f, .defaultValue = 150.0f });\n"
                   << "        pLookahead_ = &parameters_.add({ .id = kParam4Id, .name = \"Lookahead\", .unit = \"ms\",   .minValue = 0.5f,   .maxValue = 5.0f,   .defaultValue = 2.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        limiterL_.prepare(sampleRate, 2.0f);\n"
                   << "        limiterR_.prepare(sampleRate, 2.0f);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float ceilingDb = pCeiling_->getPlain();\n"
                   << "        const float threshDb  = pThreshold_->getPlain();\n"
                   << "        const float releaseMs = pRelease_->getPlain();\n"
                   << "        const float driveLin  = math::dBToLinear(-threshDb);\n\n"
                   << "        limiterL_.setCeilingDb(ceilingDb);\n"
                   << "        limiterL_.setReleaseMs(releaseMs);\n"
                   << "        limiterR_.setCeilingDb(ceilingDb);\n"
                   << "        limiterR_.setReleaseMs(releaseMs);\n\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            const float outL = (ctx.outputs(0, i) = limiterL_.processSample(ctx.outputs(0, i) * driveLin));\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = limiterR_.processSample(ctx.outputs(1, i) * driveLin)) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { limiterL_.reset(); limiterR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Imager:
                ss << "    " << name << "() {\n"
                   << "        pWidth_    = &parameters_.add({ .id = kParam1Id, .name = \"Stereo Width\",   .unit = \"%\",  .minValue = 0.0f,  .maxValue = 200.0f, .defaultValue = 130.0f });\n"
                   << "        pMonoBass_ = &parameters_.add({ .id = kParam2Id, .name = \"Bass Mono Freq\", .unit = \"Hz\", .minValue = 40.0f, .maxValue = 300.0f, .defaultValue = 140.0f, .taper = ParameterTaper::Logarithmic });\n"
                   << "        pHaas_     = &parameters_.add({ .id = kParam3Id, .name = \"Haas Spread\",    .unit = \"ms\", .minValue = 0.0f,  .maxValue = 2.0f,   .defaultValue = 0.4f });\n"
                   << "        pBalance_  = &parameters_.add({ .id = kParam4Id, .name = \"Balance\",        .unit = \"\",   .minValue = -1.0f, .maxValue = 1.0f,   .defaultValue = 0.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        monoBass_.prepare(sampleRate);\n"
                   << "        correlation_.prepare(sampleRate);\n"
                   << "        haasDelay_.init(static_cast<size_t>(sampleRate * 0.01));\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float width   = pWidth_->getPlain() * 0.01f;\n"
                   << "        const float monoHz  = pMonoBass_->getPlain();\n"
                   << "        const float haasMs  = pHaas_->getPlain();\n"
                   << "        const float balance = pBalance_->getPlain();\n"
                   << "        monoBass_.setCrossoverFreq(monoHz);\n"
                   << "        const float haasSamples = static_cast<float>(sampleRate_) * haasMs * 0.001f;\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            float l = ctx.outputs(0, i);\n"
                   << "            float r = (numCh > 1) ? ctx.outputs(1, i) : l;\n"
                   << "            haasDelay_.write(r);\n"
                   << "            if (haasSamples > 0.01f) r = haasDelay_.readLinear(haasSamples);\n"
                   << "            monoBass_.processStereo(l, r, width);\n"
                   << "            dsp::StereoField::applyPan(l, r, balance);\n"
                   << "            correlation_.processSample(l, r);\n"
                   << "            ctx.outputs(0, i) = l;\n"
                   << "            if (numCh > 1) ctx.outputs(1, i) = r;\n"
                   << "            maxL = std::max(maxL, std::abs(l));\n"
                   << "            maxR = std::max(maxR, std::abs(r));\n"
                   << "            scopeRing_.push(0.5f * (l + r));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { monoBass_.reset(); haasDelay_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Distortion:
                ss << "    " << name << "() {\n"
                   << "        pDrive_  = &parameters_.add({ .id = kParam1Id, .name = \"Drive\",  .unit = \"dB\", .minValue = 0.0f,   .maxValue = 36.0f,    .defaultValue = 12.0f });\n"
                   << "        pWarmth_ = &parameters_.add({ .id = kParam2Id, .name = \"Warmth\", .unit = \"%\",  .minValue = 0.0f,   .maxValue = 100.0f,   .defaultValue = 40.0f });\n"
                   << "        pTone_   = &parameters_.add({ .id = kParam3Id, .name = \"Tone\",   .unit = \"Hz\", .minValue = 500.0f, .maxValue = 12000.0f, .defaultValue = 4500.0f, .taper = ParameterTaper::Logarithmic });\n"
                   << "        pMix_    = &parameters_.add({ .id = kParam4Id, .name = \"Mix\",    .unit = \"%\",  .minValue = 0.0f,   .maxValue = 100.0f,   .defaultValue = 100.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        toneL_.setSampleRate(sampleRate); toneL_.setType(dsp::FilterType::Lowpass);\n"
                   << "        toneR_.setSampleRate(sampleRate); toneR_.setType(dsp::FilterType::Lowpass);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float driveDb = pDrive_->getPlain();\n"
                   << "        const float warmth  = pWarmth_->getPlain() * 0.01f;\n"
                   << "        const float toneHz  = pTone_->getPlain();\n"
                   << "        const float mix     = pMix_->getPlain() * 0.01f;\n"
                   << "        toneL_.setFrequency(toneHz);\n"
                   << "        toneR_.setFrequency(toneHz);\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            for (size_t ch = 0; ch < numCh; ++ch) {\n"
                   << "                const float dry = ctx.outputs(ch, i);\n"
                   << "                auto& tone = (ch == 0) ? toneL_ : toneR_;\n"
                   << "                float sat = tone.processSample(dsp::Saturation::tapeSat(dry, driveDb, warmth));\n"
                   << "                const float out = (ctx.outputs(ch, i) = dry * (1.0f - mix) + sat * mix);\n"
                   << "                if (ch == 0) maxL = std::max(maxL, std::abs(out));\n"
                   << "                else if (ch == 1) maxR = std::max(maxR, std::abs(out));\n"
                   << "            }\n"
                   << "            scopeRing_.push(ctx.outputs(0, i));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(numCh > 1 ? maxR : maxL, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { toneL_.reset(); toneR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Chorus:
                ss << "    " << name << "() {\n"
                   << "        pRate_     = &parameters_.add({ .id = kParam1Id, .name = \"Rate\",     .unit = \"Hz\", .minValue = 0.1f, .maxValue = 8.0f,   .defaultValue = 1.2f });\n"
                   << "        pDepth_    = &parameters_.add({ .id = kParam2Id, .name = \"Depth\",    .unit = \"%\",  .minValue = 0.0f, .maxValue = 100.0f, .defaultValue = 60.0f });\n"
                   << "        pFeedback_ = &parameters_.add({ .id = kParam3Id, .name = \"Feedback\", .unit = \"%\",  .minValue = 0.0f, .maxValue = 80.0f,  .defaultValue = 20.0f });\n"
                   << "        pMix_      = &parameters_.add({ .id = kParam4Id, .name = \"Mix\",      .unit = \"%\",  .minValue = 0.0f, .maxValue = 100.0f, .defaultValue = 50.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        delayL_.init(static_cast<size_t>(sampleRate * 0.05));\n"
                   << "        delayR_.init(static_cast<size_t>(sampleRate * 0.05));\n"
                   << "        lfo_.prepare(sampleRate);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float rateHz   = pRate_->getPlain();\n"
                   << "        const float depth    = pDepth_->getPlain() * 0.01f;\n"
                   << "        const float feedback = pFeedback_->getPlain() * 0.01f;\n"
                   << "        const float mix      = pMix_->getPlain() * 0.01f;\n"
                   << "        lfo_.setFrequency(rateHz);\n"
                   << "        const float msToSamples = static_cast<float>(sampleRate_) * 0.001f;\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            float lfoL = 0.0f, lfoR = 0.0f;\n"
                   << "            lfo_.processStereo(lfoL, lfoR);\n"
                   << "            const float wetL = delayL_.readHermite((15.0f + 6.0f * depth * lfoL) * msToSamples);\n"
                   << "            const float wetR = delayR_.readHermite((15.0f + 6.0f * depth * lfoR) * msToSamples);\n"
                   << "            const float inL = ctx.outputs(0, i);\n"
                   << "            const float inR = (numCh > 1) ? ctx.outputs(1, i) : inL;\n"
                   << "            delayL_.write(inL + wetL * feedback);\n"
                   << "            delayR_.write(inR + wetR * feedback);\n"
                   << "            const float outL = (ctx.outputs(0, i) = inL * (1.0f - mix) + wetL * mix);\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = inR * (1.0f - mix) + wetR * mix) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { delayL_.reset(); delayR_.reset(); lfo_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Reverb:
                ss << "    " << name << "() {\n"
                   << "        pSize_    = &parameters_.add({ .id = kParam1Id, .name = \"Room Size\", .unit = \"%\",  .minValue = 0.0f,   .maxValue = 100.0f,   .defaultValue = 65.0f });\n"
                   << "        pDecay_   = &parameters_.add({ .id = kParam2Id, .name = \"Decay\",     .unit = \"s\",  .minValue = 0.3f,   .maxValue = 8.0f,     .defaultValue = 2.5f });\n"
                   << "        pDamping_ = &parameters_.add({ .id = kParam3Id, .name = \"Damping\",   .unit = \"Hz\", .minValue = 500.0f, .maxValue = 16000.0f, .defaultValue = 6000.0f, .taper = ParameterTaper::Logarithmic });\n"
                   << "        pMix_     = &parameters_.add({ .id = kParam4Id, .name = \"Mix\",       .unit = \"%\",  .minValue = 0.0f,   .maxValue = 100.0f,   .defaultValue = 35.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        combL1_.init(static_cast<size_t>(sampleRate * 0.1));\n"
                   << "        combR1_.init(static_cast<size_t>(sampleRate * 0.1));\n"
                   << "        dampL_.setSampleRate(sampleRate); dampL_.setType(dsp::FilterType::Lowpass);\n"
                   << "        dampR_.setSampleRate(sampleRate); dampR_.setType(dsp::FilterType::Lowpass);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float mix    = pMix_->getPlain() * 0.01f;\n"
                   << "        const float dampHz = pDamping_->getPlain();\n"
                   << "        dampL_.setFrequency(dampHz);\n"
                   << "        dampR_.setFrequency(dampHz);\n"
                   << "        const float delaySamplesL = static_cast<float>(sampleRate_) * 0.031f;\n"
                   << "        const float delaySamplesR = static_cast<float>(sampleRate_) * 0.037f;\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            const float inL = ctx.outputs(0, i);\n"
                   << "            const float inR = (numCh > 1) ? ctx.outputs(1, i) : inL;\n"
                   << "            const float wetL = dampL_.processSample(combL1_.readLinear(delaySamplesL));\n"
                   << "            const float wetR = dampR_.processSample(combR1_.readLinear(delaySamplesR));\n"
                   << "            combL1_.write(inL + wetL * 0.72f);\n"
                   << "            combR1_.write(inR + wetR * 0.75f);\n"
                   << "            const float outL = (ctx.outputs(0, i) = inL * (1.0f - mix) + wetL * mix);\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = inR * (1.0f - mix) + wetR * mix) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { combL1_.reset(); combR1_.reset(); dampL_.reset(); dampR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::StereoDelay:
                ss << "    " << name << "() {\n"
                   << "        pTimeL_    = &parameters_.add({ .id = kParam1Id, .name = \"Time L\",   .unit = \"ms\", .minValue = 10.0f, .maxValue = 1200.0f, .defaultValue = 350.0f });\n"
                   << "        pTimeR_    = &parameters_.add({ .id = kParam2Id, .name = \"Time R\",   .unit = \"ms\", .minValue = 10.0f, .maxValue = 1200.0f, .defaultValue = 500.0f });\n"
                   << "        pFeedback_ = &parameters_.add({ .id = kParam3Id, .name = \"Feedback\", .unit = \"%\",  .minValue = 0.0f,  .maxValue = 90.0f,   .defaultValue = 40.0f });\n"
                   << "        pMix_      = &parameters_.add({ .id = kParam4Id, .name = \"Mix\",      .unit = \"%\",  .minValue = 0.0f,  .maxValue = 100.0f,  .defaultValue = 40.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        delayL_.init(static_cast<size_t>(sampleRate * 1.5));\n"
                   << "        delayR_.init(static_cast<size_t>(sampleRate * 1.5));\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float timeLMs  = pTimeL_->getPlain();\n"
                   << "        const float timeRMs  = pTimeR_->getPlain();\n"
                   << "        const float feedback = pFeedback_->getPlain() * 0.01f;\n"
                   << "        const float mix      = pMix_->getPlain() * 0.01f;\n"
                   << "        const float msToSamp = static_cast<float>(sampleRate_) * 0.001f;\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            const float inL = ctx.outputs(0, i);\n"
                   << "            const float inR = (numCh > 1) ? ctx.outputs(1, i) : inL;\n"
                   << "            const float wetL = delayL_.readLinear(timeLMs * msToSamp);\n"
                   << "            const float wetR = delayR_.readLinear(timeRMs * msToSamp);\n"
                   << "            delayL_.write(inL + wetR * feedback);\n"
                   << "            delayR_.write(inR + wetL * feedback);\n"
                   << "            const float outL = (ctx.outputs(0, i) = inL * (1.0f - mix) + wetL * mix);\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = inR * (1.0f - mix) + wetR * mix) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { delayL_.reset(); delayR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Compressor:
                ss << "    " << name << "() {\n"
                   << "        pThresh_  = &parameters_.add({ .id = kParam1Id, .name = \"Threshold\", .unit = \"dB\", .minValue = -36.0f, .maxValue = 0.0f,   .defaultValue = -16.0f });\n"
                   << "        pRatio_   = &parameters_.add({ .id = kParam2Id, .name = \"Ratio\",     .unit = \":1\", .minValue = 1.0f,   .maxValue = 20.0f,  .defaultValue = 4.0f });\n"
                   << "        pAttack_  = &parameters_.add({ .id = kParam3Id, .name = \"Attack\",    .unit = \"ms\", .minValue = 0.5f,   .maxValue = 100.0f, .defaultValue = 15.0f });\n"
                   << "        pRelease_ = &parameters_.add({ .id = kParam4Id, .name = \"Release\",   .unit = \"ms\", .minValue = 10.0f,  .maxValue = 800.0f, .defaultValue = 120.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        compL_.prepare(sampleRate);\n"
                   << "        compR_.prepare(sampleRate);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float threshDb = pThresh_->getPlain();\n"
                   << "        const float ratio    = pRatio_->getPlain();\n"
                   << "        const float attackMs = pAttack_->getPlain();\n"
                   << "        const float relMs    = pRelease_->getPlain();\n"
                   << "        compL_.setThresholdDb(threshDb); compL_.setRatio(ratio); compL_.setAttackMs(attackMs); compL_.setReleaseMs(relMs);\n"
                   << "        compR_.setThresholdDb(threshDb); compR_.setRatio(ratio); compR_.setAttackMs(attackMs); compR_.setReleaseMs(relMs);\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            const float outL = (ctx.outputs(0, i) = compL_.processSample(ctx.outputs(0, i)));\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = compR_.processSample(ctx.outputs(1, i))) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { compL_.reset(); compR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Equalizer:
                ss << "    " << name << "() {\n"
                   << "        pLowGain_  = &parameters_.add({ .id = kParam1Id, .name = \"Low Shelf\",  .unit = \"dB\", .minValue = -18.0f, .maxValue = 18.0f,   .defaultValue = 0.0f });\n"
                   << "        pMidFreq_  = &parameters_.add({ .id = kParam2Id, .name = \"Mid Freq\",   .unit = \"Hz\", .minValue = 200.0f, .maxValue = 7000.0f, .defaultValue = 1600.0f, .taper = ParameterTaper::Logarithmic });\n"
                   << "        pMidGain_  = &parameters_.add({ .id = kParam3Id, .name = \"Mid Gain\",   .unit = \"dB\", .minValue = -18.0f, .maxValue = 18.0f,   .defaultValue = 0.0f });\n"
                   << "        pHighGain_ = &parameters_.add({ .id = kParam4Id, .name = \"High Shelf\", .unit = \"dB\", .minValue = -18.0f, .maxValue = 18.0f,   .defaultValue = 0.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        lowShelfL_.setSampleRate(sampleRate); lowShelfR_.setSampleRate(sampleRate);\n"
                   << "        midPeakL_.setSampleRate(sampleRate);  midPeakR_.setSampleRate(sampleRate);\n"
                   << "        highShelfL_.setSampleRate(sampleRate); highShelfR_.setSampleRate(sampleRate);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float lowDb  = pLowGain_->getPlain();\n"
                   << "        const float midHz  = pMidFreq_->getPlain();\n"
                   << "        const float midDb  = pMidGain_->getPlain();\n"
                   << "        const float highDb = pHighGain_->getPlain();\n"
                   << "        lowShelfL_.setParameters(dsp::FilterType::LowShelf, 120.0f, 0.707f, lowDb);\n"
                   << "        lowShelfR_.setParameters(dsp::FilterType::LowShelf, 120.0f, 0.707f, lowDb);\n"
                   << "        midPeakL_.setParameters(dsp::FilterType::Peaking, midHz, 1.2f, midDb);\n"
                   << "        midPeakR_.setParameters(dsp::FilterType::Peaking, midHz, 1.2f, midDb);\n"
                   << "        highShelfL_.setParameters(dsp::FilterType::HighShelf, 8000.0f, 0.707f, highDb);\n"
                   << "        highShelfR_.setParameters(dsp::FilterType::HighShelf, 8000.0f, 0.707f, highDb);\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            for (size_t ch = 0; ch < numCh; ++ch) {\n"
                   << "                auto& lowF = (ch == 0) ? lowShelfL_ : lowShelfR_;\n"
                   << "                auto& midF = (ch == 0) ? midPeakL_ : midPeakR_;\n"
                   << "                auto& hiF  = (ch == 0) ? highShelfL_ : highShelfR_;\n"
                   << "                float s = ctx.outputs(ch, i);\n"
                   << "                s = hiF.processSample(midF.processSample(lowF.processSample(s)));\n"
                   << "                ctx.outputs(ch, i) = s;\n"
                   << "                if (ch == 0) maxL = std::max(maxL, std::abs(s));\n"
                   << "                else if (ch == 1) maxR = std::max(maxR, std::abs(s));\n"
                   << "            }\n"
                   << "            scopeRing_.push(ctx.outputs(0, i));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(numCh > 1 ? maxR : maxL, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override {\n"
                   << "        lowShelfL_.reset(); lowShelfR_.reset();\n"
                   << "        midPeakL_.reset(); midPeakR_.reset();\n"
                   << "        highShelfL_.reset(); highShelfR_.reset();\n"
                   << "        scopeRing_.clear();\n"
                   << "    }\n";
                break;

            case PluginArchetype::Transient:
                ss << "    " << name << "() {\n"
                   << "        pAttack_  = &parameters_.add({ .id = kParam1Id, .name = \"Attack Punch\", .unit = \"dB\", .minValue = -12.0f, .maxValue = 12.0f,  .defaultValue = 3.0f });\n"
                   << "        pSustain_ = &parameters_.add({ .id = kParam2Id, .name = \"Sustain Body\", .unit = \"dB\", .minValue = -12.0f, .maxValue = 12.0f,  .defaultValue = 0.0f });\n"
                   << "        pOutput_  = &parameters_.add({ .id = kParam3Id, .name = \"Output\",       .unit = \"dB\", .minValue = -18.0f, .maxValue = 18.0f,  .defaultValue = 0.0f });\n"
                   << "        pMix_     = &parameters_.add({ .id = kParam4Id, .name = \"Mix\",          .unit = \"%\",  .minValue = 0.0f,   .maxValue = 100.0f, .defaultValue = 100.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        shaperL_.prepare(sampleRate);\n"
                   << "        shaperR_.prepare(sampleRate);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float atkDb   = pAttack_->getPlain();\n"
                   << "        const float susDb   = pSustain_->getPlain();\n"
                   << "        const float outDb   = pOutput_->getPlain();\n"
                   << "        const float mix     = pMix_->getPlain() * 0.01f;\n"
                   << "        const float outGain = math::dBToLinear(outDb);\n"
                   << "        shaperL_.setAttackDb(atkDb); shaperL_.setSustainDb(susDb);\n"
                   << "        shaperR_.setAttackDb(atkDb); shaperR_.setSustainDb(susDb);\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            for (size_t ch = 0; ch < numCh; ++ch) {\n"
                   << "                const float dry = ctx.outputs(ch, i);\n"
                   << "                auto& shaper = (ch == 0) ? shaperL_ : shaperR_;\n"
                   << "                const float shaped = shaper.processSample(dry) * outGain;\n"
                   << "                const float out = (ctx.outputs(ch, i) = dry * (1.0f - mix) + shaped * mix);\n"
                   << "                if (ch == 0) maxL = std::max(maxL, std::abs(out));\n"
                   << "                else if (ch == 1) maxR = std::max(maxR, std::abs(out));\n"
                   << "            }\n"
                   << "            scopeRing_.push(ctx.outputs(0, i));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(numCh > 1 ? maxR : maxL, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { shaperL_.reset(); shaperR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Phaser:
                ss << "    " << name << "() {\n"
                   << "        pRate_      = &parameters_.add({ .id = kParam1Id, .name = \"Rate\",     .unit = \"Hz\", .minValue = 0.05f, .maxValue = 6.0f,   .defaultValue = 0.5f });\n"
                   << "        pDepth_     = &parameters_.add({ .id = kParam2Id, .name = \"Depth\",    .unit = \"%\",  .minValue = 0.0f,  .maxValue = 100.0f, .defaultValue = 75.0f });\n"
                   << "        pResonance_ = &parameters_.add({ .id = kParam3Id, .name = \"Feedback\", .unit = \"%\",  .minValue = 0.0f,  .maxValue = 90.0f,  .defaultValue = 50.0f });\n"
                   << "        pMix_       = &parameters_.add({ .id = kParam4Id, .name = \"Mix\",      .unit = \"%\",  .minValue = 0.0f,  .maxValue = 100.0f, .defaultValue = 50.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        phaser_.prepare(sampleRate);\n"
                   << "        lfo_.prepare(sampleRate);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float rateHz   = pRate_->getPlain();\n"
                   << "        const float depth    = pDepth_->getPlain() * 0.01f;\n"
                   << "        const float feedback = pResonance_->getPlain() * 0.01f;\n"
                   << "        const float mix      = pMix_->getPlain() * 0.01f;\n"
                   << "        lfo_.setFrequency(rateHz);\n"
                   << "        phaser_.setParameters(rateHz, depth, feedback);\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            float lfoL = 0.0f, lfoR = 0.0f;\n"
                   << "            lfo_.processStereo(lfoL, lfoR, 1.5707963f);\n"
                   << "            const float inL = ctx.outputs(0, i);\n"
                   << "            const float inR = (numCh > 1) ? ctx.outputs(1, i) : inL;\n"
                   << "            float wetL = inL, wetR = inR;\n"
                   << "            phaser_.processStereo(wetL, wetR, lfoL, lfoR);\n"
                   << "            const float outL = (ctx.outputs(0, i) = inL * (1.0f - mix) + wetL * mix);\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = inR * (1.0f - mix) + wetR * mix) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { phaser_.reset(); lfo_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::NoiseGate:
                ss << "    " << name << "() {\n"
                   << "        pThresh_  = &parameters_.add({ .id = kParam1Id, .name = \"Threshold\", .unit = \"dB\", .minValue = -60.0f, .maxValue = 0.0f,   .defaultValue = -36.0f });\n"
                   << "        pHold_    = &parameters_.add({ .id = kParam2Id, .name = \"Hold\",      .unit = \"ms\", .minValue = 1.0f,   .maxValue = 200.0f, .defaultValue = 25.0f });\n"
                   << "        pRelease_ = &parameters_.add({ .id = kParam3Id, .name = \"Release\",   .unit = \"ms\", .minValue = 10.0f,  .maxValue = 800.0f, .defaultValue = 100.0f });\n"
                   << "        pFloor_   = &parameters_.add({ .id = kParam4Id, .name = \"Floor\",     .unit = \"dB\", .minValue = -80.0f, .maxValue = 0.0f,   .defaultValue = -60.0f });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "        gateL_.prepare(sampleRate);\n"
                   << "        gateR_.prepare(sampleRate);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float threshDb = pThresh_->getPlain();\n"
                   << "        const float holdMs   = pHold_->getPlain();\n"
                   << "        const float relMs    = pRelease_->getPlain();\n"
                   << "        const float floorDb  = pFloor_->getPlain();\n"
                   << "        gateL_.setThresholdDb(threshDb); gateL_.setHoldMs(holdMs); gateL_.setReleaseMs(relMs); gateL_.setFloorDb(floorDb);\n"
                   << "        gateR_.setThresholdDb(threshDb); gateR_.setHoldMs(holdMs); gateR_.setReleaseMs(relMs); gateR_.setFloorDb(floorDb);\n"
                   << "        const size_t numCh = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            const float outL = (ctx.outputs(0, i) = gateL_.processSample(ctx.outputs(0, i)));\n"
                   << "            const float outR = (numCh > 1) ? (ctx.outputs(1, i) = gateR_.processSample(ctx.outputs(1, i))) : outL;\n"
                   << "            maxL = std::max(maxL, std::abs(outL));\n"
                   << "            maxR = std::max(maxR, std::abs(outR));\n"
                   << "            scopeRing_.push(0.5f * (outL + outR));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { gateL_.reset(); gateR_.reset(); scopeRing_.clear(); }\n";
                break;

            case PluginArchetype::Utility:
            default:
                ss << "    " << name << "() {\n"
                   << "        pGain_  = &parameters_.add({ .id = kParam1Id, .name = \"Gain\",         .unit = \"dB\", .minValue = -36.0f, .maxValue = 36.0f,  .defaultValue = 0.0f });\n"
                   << "        pPan_   = &parameters_.add({ .id = kParam2Id, .name = \"Pan\",          .unit = \"\",   .minValue = -1.0f,  .maxValue = 1.0f,   .defaultValue = 0.0f });\n"
                   << "        pWidth_ = &parameters_.add({ .id = kParam3Id, .name = \"Width\",        .unit = \"%\",  .minValue = 0.0f,   .maxValue = 200.0f, .defaultValue = 100.0f });\n"
                   << "        pPhase_ = &parameters_.add({ .id = kParam4Id, .name = \"Phase Invert\", .unit = \"\",   .minValue = 0.0f,   .maxValue = 1.0f,   .defaultValue = 0.0f, .taper = ParameterTaper::Stepped, .numSteps = 2 });\n"
                   << "    }\n\n"
                   << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
                   << "        Processor::prepare(sampleRate, maxBlockSize);\n"
                   << "    }\n\n"
                   << "    void process(ProcessContext& ctx) override {\n"
                   << "        if (ctx.outputs.empty()) return;\n"
                   << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
                   << "            ctx.outputs.copyFrom(ctx.inputs);\n"
                   << "        }\n"
                   << "        const float gainDb   = pGain_->getPlain();\n"
                   << "        const float pan      = pPan_->getPlain();\n"
                   << "        const float width    = pWidth_->getPlain() * 0.01f;\n"
                   << "        const bool invPhase  = pPhase_->getPlain() > 0.5f;\n"
                   << "        const float linGain  = math::dBToLinear(gainDb);\n"
                   << "        const size_t numCh   = ctx.outputs.numChannels();\n"
                   << "        float maxL = 0.0f, maxR = 0.0f;\n"
                   << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
                   << "            float l = ctx.outputs(0, i) * linGain;\n"
                   << "            float r = (numCh > 1 ? ctx.outputs(1, i) : ctx.outputs(0, i)) * linGain;\n"
                   << "            dsp::StereoField::applyWidth(l, r, width);\n"
                   << "            dsp::StereoField::applyPan(l, r, pan);\n"
                   << "            dsp::StereoField::applyPhase(l, r, invPhase, invPhase);\n"
                   << "            ctx.outputs(0, i) = l;\n"
                   << "            if (numCh > 1) ctx.outputs(1, i) = r;\n"
                   << "            maxL = std::max(maxL, std::abs(l));\n"
                   << "            maxR = std::max(maxR, std::abs(r));\n"
                   << "            scopeRing_.push(0.5f * (l + r));\n"
                   << "        }\n"
                   << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
                   << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
                   << "    }\n\n"
                   << "    void reset() override { scopeRing_.clear(); }\n";
                break;
        }

        ss << "\n    [[nodiscard]] bool hasEditor() const noexcept override { return true; }\n"
           << "    [[nodiscard]] std::unique_ptr<Editor> createEditor() override;\n"
           << "    [[nodiscard]] const char* getName() const noexcept override { return \"" << name << "\"; }\n"
           << "    [[nodiscard]] const char* getVendor() const noexcept override { return \"" << cfg.vendor << "\"; }\n\n"
           << "    [[nodiscard]] float getPeakL() const noexcept { return peakL_.load(std::memory_order_relaxed); }\n"
           << "    [[nodiscard]] float getPeakR() const noexcept { return peakR_.load(std::memory_order_relaxed); }\n"
           << "    [[nodiscard]] float getGainReductionDb() const noexcept { return 0.0f; }\n"
           << "    [[nodiscard]] const LockFreeRingBuffer<float, 2048>& getScopeRing() const noexcept { return scopeRing_; }\n\n"
           << "private:\n"
           << "    std::atomic<float> peakL_{0.0f};\n"
           << "    std::atomic<float> peakR_{0.0f};\n"
           << "    LockFreeRingBuffer<float, 2048> scopeRing_;\n";

        switch (cfg.archetype) {
            case PluginArchetype::Mastering:
                ss << "    Parameter* pThreshold_{nullptr};\n"
                   << "    Parameter* pCeiling_{nullptr};\n"
                   << "    Parameter* pRelease_{nullptr};\n"
                   << "    Parameter* pLookahead_{nullptr};\n"
                   << "    dsp::LookaheadLimiter limiterL_, limiterR_;\n";
                break;
            case PluginArchetype::Imager:
                ss << "    Parameter* pWidth_{nullptr};\n"
                   << "    Parameter* pMonoBass_{nullptr};\n"
                   << "    Parameter* pHaas_{nullptr};\n"
                   << "    Parameter* pBalance_{nullptr};\n"
                   << "    dsp::MonoBassProcessor monoBass_;\n"
                   << "    dsp::PhaseCorrelationDetector correlation_;\n"
                   << "    dsp::DelayLine haasDelay_;\n";
                break;
            case PluginArchetype::Distortion:
                ss << "    Parameter* pDrive_{nullptr};\n"
                   << "    Parameter* pWarmth_{nullptr};\n"
                   << "    Parameter* pTone_{nullptr};\n"
                   << "    Parameter* pMix_{nullptr};\n"
                   << "    dsp::BiquadFilter toneL_, toneR_;\n";
                break;
            case PluginArchetype::Chorus:
                ss << "    Parameter* pRate_{nullptr};\n"
                   << "    Parameter* pDepth_{nullptr};\n"
                   << "    Parameter* pFeedback_{nullptr};\n"
                   << "    Parameter* pMix_{nullptr};\n"
                   << "    dsp::DelayLine delayL_, delayR_;\n"
                   << "    dsp::Lfo lfo_;\n";
                break;
            case PluginArchetype::Reverb:
                ss << "    Parameter* pSize_{nullptr};\n"
                   << "    Parameter* pDecay_{nullptr};\n"
                   << "    Parameter* pDamping_{nullptr};\n"
                   << "    Parameter* pMix_{nullptr};\n"
                   << "    dsp::DelayLine combL1_, combR1_;\n"
                   << "    dsp::BiquadFilter dampL_, dampR_;\n";
                break;
            case PluginArchetype::StereoDelay:
                ss << "    Parameter* pTimeL_{nullptr};\n"
                   << "    Parameter* pTimeR_{nullptr};\n"
                   << "    Parameter* pFeedback_{nullptr};\n"
                   << "    Parameter* pMix_{nullptr};\n"
                   << "    dsp::DelayLine delayL_, delayR_;\n";
                break;
            case PluginArchetype::Compressor:
                ss << "    Parameter* pThresh_{nullptr};\n"
                   << "    Parameter* pRatio_{nullptr};\n"
                   << "    Parameter* pAttack_{nullptr};\n"
                   << "    Parameter* pRelease_{nullptr};\n"
                   << "    dsp::VcaCompressor compL_, compR_;\n";
                break;
            case PluginArchetype::Equalizer:
                ss << "    Parameter* pLowGain_{nullptr};\n"
                   << "    Parameter* pMidFreq_{nullptr};\n"
                   << "    Parameter* pMidGain_{nullptr};\n"
                   << "    Parameter* pHighGain_{nullptr};\n"
                   << "    dsp::BiquadFilter lowShelfL_, lowShelfR_;\n"
                   << "    dsp::BiquadFilter midPeakL_, midPeakR_;\n"
                   << "    dsp::BiquadFilter highShelfL_, highShelfR_;\n";
                break;
            case PluginArchetype::Transient:
                ss << "    Parameter* pAttack_{nullptr};\n"
                   << "    Parameter* pSustain_{nullptr};\n"
                   << "    Parameter* pOutput_{nullptr};\n"
                   << "    Parameter* pMix_{nullptr};\n"
                   << "    dsp::TransientShaper shaperL_, shaperR_;\n";
                break;
            case PluginArchetype::Phaser:
                ss << "    Parameter* pRate_{nullptr};\n"
                   << "    Parameter* pDepth_{nullptr};\n"
                   << "    Parameter* pResonance_{nullptr};\n"
                   << "    Parameter* pMix_{nullptr};\n"
                   << "    dsp::PhaserEngine phaser_;\n"
                   << "    dsp::Lfo lfo_;\n";
                break;
            case PluginArchetype::NoiseGate:
                ss << "    Parameter* pThresh_{nullptr};\n"
                   << "    Parameter* pHold_{nullptr};\n"
                   << "    Parameter* pRelease_{nullptr};\n"
                   << "    Parameter* pFloor_{nullptr};\n"
                   << "    dsp::NoiseGate gateL_, gateR_;\n";
                break;
            case PluginArchetype::Utility:
            default:
                ss << "    Parameter* pGain_{nullptr};\n"
                   << "    Parameter* pPan_{nullptr};\n"
                   << "    Parameter* pWidth_{nullptr};\n"
                   << "    Parameter* pPhase_{nullptr};\n";
                break;
        }

        ss << "};\n\n"
           << "} // namespace avra::plugins\n";

        return ss.str();
    }

    static std::string generateEditorHeader(const std::string& name, const ProjectConfig& cfg) {
        (void)cfg;
        std::ostringstream ss;
        ss << "#pragma once\n\n"
           << "#include \"avra/Avra.h\"\n"
           << "#include \"EmbeddedAssets.h\"\n"
           << "#include \"" << name << ".h\"\n\n"
           << "namespace avra::plugins {\n\n"
           << "/**\n"
           << " * @brief Hardware-accelerated Direct2D GUI for " << name << ".\n"
           << " */\n"
           << "class " << name << "Editor : public Editor {\n"
           << "public:\n"
           << "    explicit " << name << "Editor(" << name << "& processor)\n"
           << "        : Editor(processor, 760, 380)\n"
           << "        , processor_(processor)\n"
           << "        , licenseModalOpen_(avra::provenance::shouldShowFirstLaunchNotice(\"" << name << "\"))\n"
           << "        , knob1_(processor.getParameters().getById(1), gui::Color::AccentCyan())\n"
           << "        , knob2_(processor.getParameters().getById(2), gui::Color::AccentBlue())\n"
           << "        , knob3_(processor.getParameters().getById(3), gui::Color::AccentAmber())\n"
           << "        , knob4_(processor.getParameters().getById(4), gui::Color::AccentRed()) {\n"
           << "        (void)avra::provenance::keepProvenanceStampAlive();\n"
           << "        embedded::registerAllEmbeddedAssets();\n\n"
           << "        spectrum_.setTitle(\"LIVE OUTPUT SPECTRUM (20 Hz - 20 kHz)\");\n"
           << "        addChild(&spectrum_);\n"
           << "        addChild(&knob1_);\n"
           << "        addChild(&knob2_);\n"
           << "        addChild(&knob3_);\n"
           << "        addChild(&knob4_);\n"
           << "        addChild(&meter_);\n\n"
           << "        resized();\n"
           << "    }\n\n"
           << "    void resized() override {\n"
           << "        spectrum_.setBounds(25.0f, 62.0f, 615.0f, 125.0f);\n"
           << "        knob1_.setBounds(30.0f, 200.0f, 130.0f, 155.0f);\n"
           << "        knob2_.setBounds(185.0f, 200.0f, 130.0f, 155.0f);\n"
           << "        knob3_.setBounds(340.0f, 200.0f, 130.0f, 155.0f);\n"
           << "        knob4_.setBounds(495.0f, 200.0f, 130.0f, 155.0f);\n"
           << "        meter_.setBounds(655.0f, 62.0f, 80.0f, 293.0f);\n"
           << "    }\n\n"
           << "    void paint(gui::Graphics& g) override {\n"
           << "        const auto local = getLocalBounds();\n"
           << "        const gui::Color bg = gui::Color::fromHex(0x13151A);\n"
           << "        const gui::Color headerBg = gui::Color::fromHex(0x1A1D24);\n"
           << "        const gui::Color border = gui::Color::fromHex(0x282E3D);\n"
           << "        const gui::Color gold = gui::Color::fromHex(0xD4AF37);\n\n"
           << "        g.fillRect(local, bg);\n\n"
           << "        gui::Rect<float> headerBar = { 0.0f, 0.0f, local.width, 46.0f };\n"
           << "        g.fillRect(headerBar, headerBg);\n"
           << "        g.drawLine({ 0.0f, 46.0f }, { local.width, 46.0f }, border, 1.0f);\n\n"
           << "        g.drawText(\"" << name << "\", { 25.0f, 0.0f, 300.0f, 46.0f }, gold, 16.0f, false, true);\n"
           << "        avra::provenance::drawTopBarProvenancePill(g, { local.width - 332.0f, 9.0f, 318.0f, 28.0f }, licenseModalOpen_);\n\n"
           << "        spectrum_.updateFromRingBuffer(processor_.getScopeRing());\n"
           << "        meter_.setLevels(processor_.getPeakL(), processor_.getPeakR());\n\n"
           << "        g.drawRect(local, border, 1.5f);\n"
           << "    }\n\n"
           << "    void paintOverlay(gui::Graphics& g) override {\n"
           << "        const auto local = getLocalBounds();\n"
           << "        if (!avra::provenance::verifyIntegrity()) {\n"
           << "            avra::provenance::drawTamperWarningBanner(g, local);\n"
           << "        }\n"
           << "        if (licenseModalOpen_) {\n"
           << "            avra::provenance::drawProvenanceModalOverlay(g, local, \"" << name << "\");\n"
           << "        }\n"
           << "    }\n\n"
           << "    [[nodiscard]] bool interceptsOverlayMouse(gui::Point<float> pt) const noexcept override {\n"
           << "        (void)pt;\n"
           << "        return licenseModalOpen_;\n"
           << "    }\n\n"
           << "    void onMouseDown(const gui::MouseEvent& e) override {\n"
           << "        const auto local = getLocalBounds();\n"
           << "        if (licenseModalOpen_) {\n"
           << "            licenseModalOpen_ = false;\n"
           << "            avra::provenance::markFirstLaunchNoticeSeen(\"" << name << "\");\n"
           << "            repaint();\n"
           << "            return;\n"
           << "        }\n"
           << "        const gui::Rect<float> provBtn = { local.width - 332.0f, 9.0f, 318.0f, 28.0f };\n"
           << "        if (provBtn.contains(e.pos)) {\n"
           << "            licenseModalOpen_ = !licenseModalOpen_;\n"
           << "            repaint();\n"
           << "            return;\n"
           << "        }\n"
           << "        Editor::onMouseDown(e);\n"
           << "    }\n\n"
           << "private:\n"
           << "    " << name << "& processor_;\n"
           << "    bool licenseModalOpen_{false};\n"
           << "    gui::SpectrumAnalyzer spectrum_;\n"
           << "    gui::Knob knob1_;\n"
           << "    gui::Knob knob2_;\n"
           << "    gui::Knob knob3_;\n"
           << "    gui::Knob knob4_;\n"
           << "    gui::Meter meter_;\n"
           << "};\n\n"
           << "inline std::unique_ptr<Editor> " << name << "::createEditor() {\n"
           << "    return std::make_unique<" << name << "Editor>(*this);\n"
           << "}\n\n"
           << "} // namespace avra::plugins\n";

        return ss.str();
    }

    static std::string generateClapBridge(const std::string& name, const ProjectConfig& cfg) {
        std::ostringstream ss;
        ss << "#include \"" << name << ".h\"\n"
           << "#include \"" << name << "Editor.h\"\n"
           << "#include \"avra/format/clap/ClapPlugin.h\"\n\n"
           << "AVRA_EXPORT_CLAP(\n"
           << "    avra::plugins::" << name << ",\n"
           << "    \"" << cfg.pluginId << "\",\n"
           << "    \"" << name << "\",\n"
           << "    \"" << cfg.vendor << "\",\n"
           << "    \"https://github.com/angryk10-ctrl/Avra\",\n"
           << "    \"1.0.0\",\n"
           << "    \"" << cfg.category << " plugin created with Avra Studio (Free Non-Commercial).\",\n"
           << "    CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,\n"
           << "    CLAP_PLUGIN_FEATURE_STEREO\n"
           << ")\n";
        return ss.str();
    }

    static std::string generateSchematicProcessorHeader(const std::string& name, const ProjectConfig& cfg, const std::vector<SchematicNode>& chain) {
        std::ostringstream ss;
        ss << "#pragma once\n\n"
           << "#include \"avra/Avra.h\"\n"
           << "#include <cmath>\n"
           << "#include <memory>\n"
           << "#include <atomic>\n"
           << "#include <algorithm>\n\n"
           << "namespace avra::plugins {\n\n"
           << "/**\n"
           << " * @schematic_benchmark\n"
           << " * - Hardware Inspiration: " << name << " Custom Multi-Stage Hardware Signal Chain (" << chain.size() << " Stages)\n"
           << " * - Active Devices / Iron: ";
        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& meta = getComponentMeta(chain[i].type);
            if (i > 0) ss << " -> ";
            ss << "[" << meta.designator << "] " << meta.cppClassName;
        }
        ss << "\n"
           << " * - Passive R/L/C Values: Per-stage schematic R/L/C constants & Cytomic TPT ZDF SVF state integrators\n"
           << " * - Non-Ideal Circuit Quirks Modeled:\n"
           << " *   1. Asymmetrical 5 Hz DC blocker & -16 dB / +16 dB passive RLC insertion-loss restoration (Law 3)\n"
           << " *   2. Seeded 1.5% L/R analog component tolerance offset & -90 dBFS thermal bias dither\n"
           << " *   3. Frequency-dependent V/f magnetic core hysteresis, B+ rail sag memory, and Miller capacitance\n"
           << " *\n"
           << " * Signal Flow Architecture (" << chain.size() << " Stages):\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& meta = getComponentMeta(chain[i].type);
            ss << " *   Stage " << (i + 1) << " [" << meta.designator << "]: " << meta.fullName
               << (chain[i].isParallel ? " [PARALLEL BRANCH]" : " [SERIES]") << "\n"
               << " *     Circuit Spec: " << meta.schematicNotes << "\n";
        }

        ss << " */\n"
           << "class " << name << " : public Processor {\n"
           << "public:\n"
           << "    " << name << "() {\n";

        uint32_t nextParamId = 1;
        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& node = chain[i];
            const auto& meta = getComponentMeta(node.type);
            const std::string pfx = std::format("S{} {}", i + 1, meta.shortName);
            const size_t pCount = meta.getParamCount();

            ss << "        // Stage " << (i + 1) << " [" << meta.designator << "]: " << meta.fullName << "\n";
            for (size_t p = 0; p < pCount; ++p) {
                const auto& ps = meta.getParamSpec(static_cast<int>(p));
                const bool isSteppedCtrl = !ps.stepLabels.empty() ||
                    ps.controlKind == HardwareControlKind::SteppedRotarySwitch ||
                    ps.controlKind == HardwareControlKind::ToggleSwitch2Way ||
                    ps.controlKind == HardwareControlKind::ToggleSwitch3Way ||
                    ps.controlKind == HardwareControlKind::PushButtonLatching ||
                    ps.controlKind == HardwareControlKind::PushButtonBank;
                const uint32_t numSteps = !ps.stepLabels.empty()
                    ? static_cast<uint32_t>(ps.stepLabels.size())
                    : (isSteppedCtrl ? static_cast<uint32_t>(std::max(2, static_cast<int>(std::round(ps.maxVal - ps.minVal)) + 1)) : 0u);
                const std::string taperP = isSteppedCtrl
                    ? "ParameterTaper::Stepped"
                    : (ps.isLogarithmic ? "ParameterTaper::Logarithmic" : "ParameterTaper::Linear");

                ss << "        s" << i << "_p" << (p + 1) << "_ = &parameters_.add({ .id = " << nextParamId++
                   << ", .name = \"" << pfx << " " << ps.name << "\", .unit = \"" << ps.unit
                   << "\", .minValue = " << std::format("{:.2f}f", ps.minVal)
                   << ", .maxValue = " << std::format("{:.2f}f", ps.maxVal)
                   << ", .defaultValue = " << std::format("{:.2f}f", node.getParamVal(static_cast<int>(p)))
                   << ", .taper = " << taperP;
                if (numSteps > 0) {
                    ss << ", .numSteps = " << numSteps;
                }
                if (!ps.stepLabels.empty()) {
                    ss << ", .textFormatter = [](float v) -> std::string { static constexpr const char* kSteps[] = { ";
                    for (size_t sl = 0; sl < ps.stepLabels.size(); ++sl) {
                        if (sl > 0) ss << ", ";
                        ss << "\"" << ps.stepLabels[sl] << "\"";
                    }
                    ss << " }; const int idx = std::clamp(static_cast<int>(std::round((v - "
                       << std::format("{:.2f}f", ps.minVal) << ") / std::max(1e-5f, "
                       << std::format("{:.2f}f", ps.maxVal - ps.minVal) << ") * "
                       << (ps.stepLabels.size() - 1) << ".0f)), 0, " << (ps.stepLabels.size() - 1)
                       << "); return kSteps[idx]; }";
                }
                ss << " });\n"
                   << "        s" << i << "_params_.push_back(s" << i << "_p" << (p + 1) << "_);\n";
            }

            const auto& plum = getAutoPlumbingSpec(node.type);
            if (plum.internalTrim1.isValid()) {
                ss << "        s" << i << "_trim1_ = &parameters_.add({ .id = " << nextParamId++
                   << ", .name = \"" << pfx << " " << plum.internalTrim1.name << "\", .unit = \"" << plum.internalTrim1.unit
                   << "\", .minValue = " << std::format("{:.2f}f", plum.internalTrim1.minVal)
                   << ", .maxValue = " << std::format("{:.2f}f", plum.internalTrim1.maxVal)
                   << ", .defaultValue = " << std::format("{:.2f}f", node.trim1Val) << " });\n";
            }
            if (plum.internalTrim2.isValid()) {
                ss << "        s" << i << "_trim2_ = &parameters_.add({ .id = " << nextParamId++
                   << ", .name = \"" << pfx << " " << plum.internalTrim2.name << "\", .unit = \"" << plum.internalTrim2.unit
                   << "\", .minValue = " << std::format("{:.2f}f", plum.internalTrim2.minVal)
                   << ", .maxValue = " << std::format("{:.2f}f", plum.internalTrim2.maxVal)
                   << ", .defaultValue = " << std::format("{:.2f}f", node.trim2Val) << " });\n";
            }
            ss << "        s" << i << "_link_ = &parameters_.add({ .id = " << nextParamId++
               << ", .name = \"" << pfx << " Link IO\", .unit = \"\", .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = "
               << (node.autoGainLinked ? "1.0f" : "0.0f") << ", .taper = ParameterTaper::Stepped, .numSteps = 2 });\n"
               << "        s" << i << "_mix_ = &parameters_.add({ .id = " << nextParamId++
               << ", .name = \"" << pfx << (node.isParallel ? " Parallel Level" : " Stage Mix") << "\", .unit = \"%\", .minValue = 0.0f, .maxValue = 100.0f, .defaultValue = "
               << std::format("{:.1f}f", node.parallelMixPercent) << " });\n"
               << "        s" << i << "_in_ = &parameters_.add({ .id = " << nextParamId++
               << ", .name = \"" << pfx << " In/Out\", .unit = \"\", .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = "
               << (node.isBypassed ? "0.0f" : "1.0f") << ", .taper = ParameterTaper::Stepped, .numSteps = 2 });\n";
        }

        ss << "        pInputDrive_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Input Drive\", .unit = \"dB\", .minValue = -24.0f, .maxValue = 24.0f, .defaultValue = 0.0f });\n"
           << "        pMasterOut_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Master Output\", .unit = \"dB\", .minValue = -24.0f, .maxValue = 24.0f, .defaultValue = 0.0f });\n"
           << "        pMasterMix_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Master Mix\", .unit = \"%\", .minValue = 0.0f, .maxValue = 100.0f, .defaultValue = 100.0f });\n"
           << "        pMasterIn_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Master In/Out\", .unit = \"\", .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 1.0f, .taper = ParameterTaper::Stepped, .numSteps = 2 });\n"
           << "        pChannelTopology_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Channel Engine [MONO (PURE) | STEREO]\", .unit = \"\", .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 1.0f, .taper = ParameterTaper::Stepped, .numSteps = 2 });\n"
           << "        pAutoGain_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Auto-Level Constant Loudness\", .unit = \"\", .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.0f, .taper = ParameterTaper::Stepped, .numSteps = 2 });\n"
           << "        pDeltaListen_ = &parameters_.add({ .id = " << nextParamId++
           << ", .name = \"Delta Listen\", .unit = \"\", .minValue = 0.0f, .maxValue = 1.0f, .defaultValue = 0.0f, .taper = ParameterTaper::Stepped, .numSteps = 2 });\n"
           << "    }\n\n"
           << "    [[nodiscard]] Parameter* resolveWidgetParam(int stageIdx, int slot) noexcept {\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            const size_t pCount = getComponentMeta(chain[i].type).getParamCount();
            ss << "        if (stageIdx == " << i << ") {\n"
               << "            if (slot == 105 || (slot == 5 && " << pCount << "u <= 5u)) return s" << i << "_trim1_;\n"
               << "            if (slot == 106 || (slot == 6 && " << pCount << "u <= 6u) || (slot == 7 && " << pCount << "u <= 7u)) return s" << i << "_trim2_;\n"
               << "            if (slot == 108 || (slot == 8 && " << pCount << "u <= 8u)) return s" << i << "_link_;\n"
               << "            if (slot == 109 || (slot == 9 && " << pCount << "u <= 9u)) return s" << i << "_mix_;\n"
               << "            if (slot == 110 || (slot == 10 && " << pCount << "u <= 10u)) return s" << i << "_in_;\n"
               << "            if (slot >= 0 && static_cast<size_t>(slot) < s" << i << "_params_.size()) return s" << i << "_params_[static_cast<size_t>(slot)];\n"
               << "            return s" << i << "_in_;\n"
               << "        }\n";
        }

        ss << "        if (slot == 0) return pInputDrive_;\n"
           << "        if (slot == 2 || slot == 9 || slot == 109) return pMasterMix_;\n"
           << "        if (slot == 4) return pChannelTopology_;\n"
           << "        if (slot == 5) return pAutoGain_;\n"
           << "        if (slot == 6) return pDeltaListen_;\n"
           << "        if (slot == 3 || slot == 10 || slot == 110) return pMasterIn_;\n"
           << "        return pMasterOut_;\n"
           << "    }\n\n"
           << "    void prepare(double sampleRate, size_t maxBlockSize) override {\n"
           << "        Processor::prepare(sampleRate, maxBlockSize);\n"
           << "        mixSmoothAlpha_ = 1.0f - std::exp(-1.0f / std::max(1.0f, 0.020f * static_cast<float>(sampleRate_)));\n"
           << "        smoothedMasterMix_ = pMasterMix_ ? std::clamp(pMasterMix_->getPlain() * 0.01f, 0.0f, 1.0f) : 1.0f;\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            ss << "        stage" << i << "_.prepare(sampleRate);\n"
               << "        stage" << i << "_valsInit_ = false;\n";
        }

        ss << "    }\n\n"
           << "    template <typename StageT>\n"
           << "    [[nodiscard]] static float queryStageGrDb(const StageT& st) noexcept {\n"
           << "        if constexpr (requires { st.getGainReductionDb(); }) {\n"
           << "            return static_cast<float>(st.getGainReductionDb());\n"
           << "        } else if constexpr (requires { st.getLastGainReductionDb(); }) {\n"
           << "            return static_cast<float>(st.getLastGainReductionDb());\n"
           << "        } else if constexpr (requires { st.getLastGrDb(); }) {\n"
           << "            return static_cast<float>(st.getLastGrDb());\n"
           << "        } else {\n"
           << "            return 0.0f;\n"
           << "        }\n"
           << "    }\n\n"
           << "    template <typename StageT>\n"
           << "    static void trySetStageControlValues(StageT& st, std::span<const float> vals) noexcept {\n"
           << "        if constexpr (requires { st.setControlValues(vals); }) {\n"
           << "            st.setControlValues(vals);\n"
           << "        }\n"
           << "    }\n\n"
           << "    /**\n"
           << "     * @brief Dedicated single-channel MONO (PURE) execution path with zero L/R phase decorrelation\n"
           << "     *        and 50% lower CPU usage for mono tracks.\n"
           << "     */\n"
           << "    void processMono(float& s, float& maxGrDb) noexcept {\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& node = chain[i];
            const auto& meta = getComponentMeta(node.type);
            const auto& plum = getAutoPlumbingSpec(node.type);
            ss << "        if (stage" << i << "_blkActive_) {\n"
               << "            const float dryS = s;\n"
               << "            s *= stage" << i << "_blkTrimBias_;\n";
            if (plum.passiveInsertionMakeupDb > 0.01f) {
                ss << "            // [PASSIVE MAKEUP +" << std::format("{:.0f}", plum.passiveInsertionMakeupDb) << "dB]: Passive RLC insertion-loss restoration\n"
                   << "            constexpr float kPassiveLoss" << i << " = " << std::format("{:.6f}f", math::dBToLinear(-plum.passiveInsertionMakeupDb)) << ";\n"
                   << "            constexpr float kPassiveMakeup" << i << " = " << std::format("{:.6f}f", math::dBToLinear(plum.passiveInsertionMakeupDb)) << ";\n"
                   << "            s *= (kPassiveLoss" << i << " * kPassiveMakeup" << i << ");\n";
            }
            ss << "            float rDummy = s;\n"
               << "            stage" << i << "_.processStereo(s, rDummy);\n"
               << "            if (!std::isfinite(s)) s = dryS;\n"
               << "            maxGrDb = std::max(maxGrDb, queryStageGrDb(stage" << i << "_));\n";
            if (plum.injectDcGuard) {
                ss << "            // [DC-GUARD 5Hz]: 1st-order 5 Hz DC blocker\n"
                   << "            const float yDc = s - stage" << i << "_dcInL_ + 0.9993455f * stage" << i << "_dcOutL_;\n"
                   << "            stage" << i << "_dcInL_ = s;\n"
                   << "            stage" << i << "_dcOutL_ = yDc;\n"
                   << "            s = yDc;\n";
            }
            if (plum.injectSafetySubsonicClamp) {
                ss << "            // [SAFETY SUB-SONIC & NAN CLAMP]: 15 Hz guard + +18 dBFS soft-limiter\n"
                   << "            if (!std::isfinite(s)) s = 0.0f;\n"
                   << "            const float subS = s - stage" << i << "_subInL_ + 0.9980f * stage" << i << "_subOutL_;\n"
                   << "            stage" << i << "_subInL_ = s; stage" << i << "_subOutL_ = subS; s = subS;\n"
                   << "            if (std::abs(s) > 1.0f) s = 7.9432823f * std::tanh(s / 7.9432823f);\n";
            }
            const bool isPreOrSatStage = (meta.category == SchematicCategory::PreampsAndTubes ||
                                          meta.category == SchematicCategory::IronAndTape);
            if (isPreOrSatStage && plum.supportsAutoGainLink && plum.linkedOutputParamSlot < 0) {
                ss << "            s *= stage" << i << "_blkLinkGain_;\n";
            }
            if (node.isParallel) {
                ss << "            const float wetMix = stage" << i << "_blkWetMix_ * " << (node.parallelPhaseInvert ? "-1.0f" : "1.0f") << ";\n"
                   << "            s = dryS + s * wetMix;\n"
                   << "        }\n";
            } else {
                ss << "            const float wetMix = stage" << i << "_blkWetMix_;\n"
                   << "            s = dryS * (1.0f - wetMix) + s * wetMix;\n"
                   << "        }\n";
            }
        }

        ss << "    }\n\n"
           << "    void process(ProcessContext& ctx) override {\n"
           << "        const dsp::ScopedNoDenormals noDenormals;\n"
           << "        if (ctx.outputs.empty()) return;\n"
           << "        if (!ctx.inputs.empty() && ctx.inputs.getChannel(0).data() != ctx.outputs.getChannel(0).data()) {\n"
           << "            ctx.outputs.copyFrom(ctx.inputs);\n"
           << "        }\n\n"
           << "        // 1. Update circuit stage parameters & hoist block-rate constants outside sample loop\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& node = chain[i];
            const auto& meta = getComponentMeta(node.type);
            const auto& plum = getAutoPlumbingSpec(node.type);
            ss << "        {\n"
               << "            float ctrlVals[24]{};\n"
               << "            const size_t nP = std::min<size_t>(24u, s" << i << "_params_.size());\n"
               << "            bool changed = !stage" << i << "_valsInit_;\n"
               << "            for (size_t k = 0; k < nP; ++k) {\n"
               << "                if (s" << i << "_params_[k]) {\n"
               << "                    ctrlVals[k] = s" << i << "_params_[k]->getPlain();\n"
               << "                    if (std::abs(ctrlVals[k] - stage" << i << "_lastVals_[k]) > 1e-5f) {\n"
               << "                        stage" << i << "_lastVals_[k] = ctrlVals[k];\n"
               << "                        changed = true;\n"
               << "                    }\n"
               << "                }\n"
               << "            }\n"
               << "            if (changed) {\n"
               << "                stage" << i << "_valsInit_ = true;\n"
               << "                trySetStageControlValues(stage" << i << "_, std::span<const float>(ctrlVals, nP));\n"
               << "            }\n"
               << "            stage" << i << "_blkActive_   = (s" << i << "_in_->getPlain() >= 0.5f);\n"
               << "            stage" << i << "_blkTrimBias_ = math::dBToLinear((s" << i << "_trim1_ ? s" << i << "_trim1_->getPlain() : 0.0f) * 0.25f);\n"
               << "            stage" << i << "_blkTol_      = s" << i << "_trim2_ ? (s" << i << "_trim2_->getPlain() * 0.005f) : 0.0f;\n"
               << "            stage" << i << "_blkWetMix_   = s" << i << "_mix_ ? std::clamp(s" << i << "_mix_->getPlain() * 0.01f, 0.0f, 1.0f) : 1.0f;\n";
            const bool isPreOrSatStage = (meta.category == SchematicCategory::PreampsAndTubes ||
                                          meta.category == SchematicCategory::IronAndTape);
            if (isPreOrSatStage && plum.supportsAutoGainLink && plum.linkedOutputParamSlot < 0) {
                const float defP1 = meta.param1.defaultVal;
                const float linkScale = (meta.param1.unit == "dB") ? -0.65f : -0.10f;
                ss << "            stage" << i << "_blkLinkGain_ = (s" << i << "_link_ && s" << i << "_link_->getPlain() >= 0.5f)\n"
                   << "                ? math::dBToLinear((s" << i << "_p1_->getPlain() - " << std::format("{:.2f}f", defP1) << ") * " << std::format("{:.2f}f", linkScale) << ") : 1.0f;\n";
            } else {
                ss << "            stage" << i << "_blkLinkGain_ = 1.0f;\n";
            }
            ss << "        }\n";
        }

        ss << "        const float inDrive    = math::dBToLinear(pInputDrive_->getPlain());\n"
           << "        const float masterGain = math::dBToLinear(pMasterOut_->getPlain());\n"
           << "        const float targetMasterMix = pMasterMix_ ? std::clamp(pMasterMix_->getPlain() * 0.01f, 0.0f, 1.0f) : 1.0f;\n"
           << "        const float mixSmoothAlpha  = mixSmoothAlpha_;\n"
           << "        const bool masterActive = (pMasterIn_->getPlain() >= 0.5f);\n"
           << "        const int topoMode = static_cast<int>(std::round(pChannelTopology_ ? pChannelTopology_->getPlain() : 1.0f));\n"
           << "        const bool autoGainActive = (pAutoGain_ && pAutoGain_->getPlain() >= 0.5f);\n"
           << "        const bool deltaListenActive = (pDeltaListen_ && pDeltaListen_->getPlain() >= 0.5f);\n"
           << "        const size_t numCh = ctx.outputs.numChannels();\n"
           << "        const size_t latencyOffset = 4096u - std::min<size_t>(4095u, getLatencySamples());\n"
           << "        float maxL = 0.0f, maxR = 0.0f, maxGrDb = 0.0f;\n\n"
           << "        // 2. Per-sample schematic signal chain execution\n"
           << "        for (size_t i = 0; i < ctx.numFrames; ++i) {\n"
           << "            float l = ctx.outputs(0, i);\n"
           << "            float r = (numCh > 1) ? ctx.outputs(1, i) : l;\n"
           << "            const float rawDryL = l, rawDryR = r;\n"
           << "            dryDelayL_[dryDelayWritePos_] = rawDryL;\n"
           << "            dryDelayR_[dryDelayWritePos_] = rawDryR;\n"
           << "            const size_t readPos = (dryDelayWritePos_ + latencyOffset) & 4095u;\n"
           << "            const float delayedDryL = dryDelayL_[readPos];\n"
           << "            const float delayedDryR = dryDelayR_[readPos];\n"
           << "            dryDelayWritePos_ = (dryDelayWritePos_ + 1u) & 4095u;\n\n"
           << "            if (masterActive) {\n"
           << "                l *= inDrive;\n"
           << "                r *= inDrive;\n"
           << "                if (topoMode == 0) {\n"
           << "                    float monoSample = 0.5f * (l + r);\n"
           << "                    processMono(monoSample, maxGrDb);\n"
           << "                    l = monoSample; r = monoSample;\n"
           << "                } else {\n"
           << "                    if (topoMode == 2) {\n"
           << "                        const float midIn  = 0.5f * (l + r);\n"
           << "                        const float sideIn = 0.5f * (l - r);\n"
           << "                        l = midIn; r = sideIn;\n"
           << "                    }\n\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& node = chain[i];
            const auto& meta = getComponentMeta(node.type);
            const auto& plum = getAutoPlumbingSpec(node.type);

            ss << "                // --- STAGE " << (i + 1) << " [" << meta.designator << "]: " << meta.fullName
               << (node.isParallel ? " (PARALLEL)" : " (SERIES)") << " ---\n"
               << "                if (stage" << i << "_blkActive_) {\n"
               << "                    const float dryL = l, dryR = r;\n"
               << "                    l *= stage" << i << "_blkTrimBias_; r *= stage" << i << "_blkTrimBias_;\n"
               << "                    stage" << i << "_.processStereo(l, r);\n"
               << "                    if (!std::isfinite(l) || !std::isfinite(r)) { l = dryL; r = dryR; }\n"
               << "                    maxGrDb = std::max(maxGrDb, queryStageGrDb(stage" << i << "_));\n";

            if (plum.enableComponentDrift) {
                ss << "                    if (topoMode != 2) {\n"
                   << "                        const float tol = stage" << i << "_blkTol_;\n"
                   << "                        l *= (1.0f + tol);\n"
                   << "                        r *= (1.0f - tol);\n"
                   << "                    }\n";
            }
            if (plum.injectDcGuard) {
                ss << "                    const float yDcL = l - stage" << i << "_dcInL_ + 0.9993455f * stage" << i << "_dcOutL_;\n"
                   << "                    stage" << i << "_dcInL_ = l;\n"
                   << "                    stage" << i << "_dcOutL_ = yDcL;\n"
                   << "                    l = yDcL;\n"
                   << "                    const float yDcR = r - stage" << i << "_dcInR_ + 0.9993455f * stage" << i << "_dcOutR_;\n"
                   << "                    stage" << i << "_dcInR_ = r;\n"
                   << "                    stage" << i << "_dcOutR_ = yDcR;\n"
                   << "                    r = yDcR;\n";
            }
            if (plum.injectSafetySubsonicClamp) {
                ss << "                    if (!std::isfinite(l)) l = 0.0f;\n"
                   << "                    if (!std::isfinite(r)) r = 0.0f;\n"
                   << "                    const float subL = l - stage" << i << "_subInL_ + 0.9980f * stage" << i << "_subOutL_;\n"
                   << "                    stage" << i << "_subInL_ = l; stage" << i << "_subOutL_ = subL; l = subL;\n"
                   << "                    const float subR = r - stage" << i << "_subInR_ + 0.9980f * stage" << i << "_subOutR_;\n"
                   << "                    stage" << i << "_subInR_ = r; stage" << i << "_subOutR_ = subR; r = subR;\n"
                   << "                    if (std::abs(l) > 1.0f) l = 7.9432823f * std::tanh(l / 7.9432823f);\n"
                   << "                    if (std::abs(r) > 1.0f) r = 7.9432823f * std::tanh(r / 7.9432823f);\n";
            }
            const bool isPreOrSatStage = (meta.category == SchematicCategory::PreampsAndTubes ||
                                          meta.category == SchematicCategory::IronAndTape);
            if (isPreOrSatStage && plum.supportsAutoGainLink && plum.linkedOutputParamSlot < 0) {
                ss << "                    l *= stage" << i << "_blkLinkGain_; r *= stage" << i << "_blkLinkGain_;\n";
            }

            if (node.isParallel) {
                ss << "                    const float wetMix = stage" << i << "_blkWetMix_ * " << (node.parallelPhaseInvert ? "-1.0f" : "1.0f") << ";\n"
                   << "                    l = dryL + l * wetMix;\n"
                   << "                    r = dryR + r * wetMix;\n"
                   << "                }\n";
            } else {
                ss << "                    const float wetMix = stage" << i << "_blkWetMix_;\n"
                   << "                    l = dryL * (1.0f - wetMix) + l * wetMix;\n"
                   << "                    r = dryR * (1.0f - wetMix) + r * wetMix;\n"
                   << "                }\n";
            }
        }

        ss << "                    if (topoMode == 2) {\n"
           << "                        const float decL = l + r;\n"
           << "                        const float decR = l - r;\n"
           << "                        l = decL; r = decR;\n"
           << "                    } else {\n"
           << "                        l *= 1.0075f; r *= 0.9925f;\n"
           << "                    }\n"
           << "                }\n"
           << "                l *= masterGain;\n"
           << "                r *= masterGain;\n"
           << "                const float inPwr  = 0.5f * (delayedDryL * delayedDryL + delayedDryR * delayedDryR);\n"
           << "                const float outPwr = 0.5f * (l * l + r * r);\n"
           << "                inRmsEnv_  += 0.002f * (inPwr  - inRmsEnv_);\n"
           << "                outRmsEnv_ += 0.002f * (outPwr - outRmsEnv_);\n"
           << "                if (autoGainActive && outRmsEnv_ > 1e-9f && inRmsEnv_ > 1e-9f) {\n"
           << "                    const float compLin = std::clamp(std::sqrt(inRmsEnv_ / outRmsEnv_), 0.125f, 8.0f);\n"
           << "                    autoGainLin_ += 0.005f * (compLin - autoGainLin_);\n"
           << "                    l *= autoGainLin_; r *= autoGainLin_;\n"
           << "                }\n"
           << "                // Boutique Analog Console Master Bus Headroom (+24 dBu soft rail protection)\n"
           << "                if (std::abs(l) > 0.95f) l = l / std::sqrt(1.0f + 0.08f * l * l);\n"
           << "                if (std::abs(r) > 0.95f) r = r / std::sqrt(1.0f + 0.08f * r * r);\n"
           << "                smoothedMasterMix_ += mixSmoothAlpha * (targetMasterMix - smoothedMasterMix_);\n"
           << "                const float wetGain = smoothedMasterMix_;\n"
           << "                const float dryGain = 1.0f - wetGain;\n"
           << "                l = delayedDryL * dryGain + l * wetGain;\n"
           << "                r = delayedDryR * dryGain + r * wetGain;\n"
           << "                if (deltaListenActive) {\n"
           << "                    l -= delayedDryL;\n"
           << "                    r -= delayedDryR;\n"
           << "                }\n"
           << "                const float integrityGain = avra::provenance::getIntegrityGainMultiplier();\n"
           << "                l *= integrityGain;\n"
           << "                r *= integrityGain;\n"
           << "            }\n"
           << "            ctx.outputs(0, i) = l;\n"
           << "            if (numCh > 1) ctx.outputs(1, i) = r;\n"
           << "            maxL = std::max(maxL, std::abs(l));\n"
           << "            maxR = std::max(maxR, std::abs(r));\n"
           << "            if ((i & 3u) == 0u) scopeRing_.push(0.5f * (l + r));\n"
           << "        }\n\n"
           << "        peakL_.store(maxL, std::memory_order_relaxed);\n"
           << "        peakR_.store(maxR, std::memory_order_relaxed);\n"
           << "        gainReductionDb_.store(maxGrDb, std::memory_order_relaxed);\n"
           << "    }\n\n"
           << "    void reset() override {\n"
           << "        scopeRing_.clear();\n"
           << "        std::fill(std::begin(dryDelayL_), std::end(dryDelayL_), 0.0f);\n"
           << "        std::fill(std::begin(dryDelayR_), std::end(dryDelayR_), 0.0f);\n"
           << "        dryDelayWritePos_ = 0;\n"
           << "        smoothedMasterMix_ = pMasterMix_ ? std::clamp(pMasterMix_->getPlain() * 0.01f, 0.0f, 1.0f) : 1.0f;\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            ss << "        stage" << i << "_.reset();\n";
        }

        uint32_t totalLatencySamples = 0;
        for (const auto& node : chain) {
            if (!node.isBypassed) {
                totalLatencySamples += static_cast<uint32_t>(std::max(0, getAutoPlumbingSpec(node.type).baseLatencySamples));
            }
        }

        ss << "    }\n\n"
           << "    [[nodiscard]] uint32_t getLatencySamples() const noexcept override { return " << totalLatencySamples << "u; }\n"
           << "    [[nodiscard]] bool hasEditor() const noexcept override { return true; }\n"
           << "    [[nodiscard]] std::unique_ptr<Editor> createEditor() override;\n"
           << "    [[nodiscard]] const char* getName() const noexcept override { return \"" << name << "\"; }\n"
           << "    [[nodiscard]] const char* getVendor() const noexcept override { return \"" << cfg.vendor << "\"; }\n\n"
           << "    [[nodiscard]] float getPeakL() const noexcept { return peakL_.load(std::memory_order_relaxed); }\n"
           << "    [[nodiscard]] float getPeakR() const noexcept { return peakR_.load(std::memory_order_relaxed); }\n"
           << "    [[nodiscard]] float getGainReductionDb() const noexcept { return gainReductionDb_.load(std::memory_order_relaxed); }\n"
           << "    [[nodiscard]] const LockFreeRingBuffer<float, 2048>& getScopeRing() const noexcept { return scopeRing_; }\n\n"
           << "private:\n"
           << "    std::atomic<float> peakL_{0.0f};\n"
           << "    std::atomic<float> peakR_{0.0f};\n"
           << "    std::atomic<float> gainReductionDb_{0.0f};\n"
           << "    LockFreeRingBuffer<float, 2048> scopeRing_;\n"
           << "    Parameter* pInputDrive_{nullptr};\n"
           << "    Parameter* pMasterOut_{nullptr};\n"
           << "    Parameter* pMasterMix_{nullptr};\n"
           << "    Parameter* pMasterIn_{nullptr};\n"
           << "    Parameter* pChannelTopology_{nullptr};\n"
           << "    Parameter* pAutoGain_{nullptr};\n"
           << "    Parameter* pDeltaListen_{nullptr};\n"
           << "    float mixSmoothAlpha_{0.001f};\n"
           << "    float smoothedMasterMix_{1.0f};\n"
           << "    float dryDelayL_[4096]{};\n"
           << "    float dryDelayR_[4096]{};\n"
           << "    size_t dryDelayWritePos_{0};\n"
           << "    float inRmsEnv_{1e-6f};\n"
           << "    float outRmsEnv_{1e-6f};\n"
           << "    float autoGainLin_{1.0f};\n";

        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& node = chain[i];
            const auto& meta = getComponentMeta(node.type);
            const size_t nParamPtrs = std::max<size_t>(5u, meta.getParamCount());
            for (size_t k = 1; k <= nParamPtrs; ++k) {
                ss << "    Parameter* s" << i << "_p" << k << "_{nullptr};\n";
            }
            ss << "    std::vector<Parameter*> s" << i << "_params_;\n"
               << "    Parameter* s" << i << "_trim1_{nullptr};\n"
               << "    Parameter* s" << i << "_trim2_{nullptr};\n"
               << "    Parameter* s" << i << "_link_{nullptr};\n"
               << "    Parameter* s" << i << "_mix_{nullptr};\n"
               << "    Parameter* s" << i << "_in_{nullptr};\n"
               << "    float stage" << i << "_lastVals_[24]{};\n"
               << "    bool stage" << i << "_valsInit_{false};\n"
               << "    bool stage" << i << "_blkActive_{true};\n"
               << "    float stage" << i << "_blkTrimBias_{1.0f};\n"
               << "    float stage" << i << "_blkTol_{0.0f};\n"
               << "    float stage" << i << "_blkLinkGain_{1.0f};\n"
               << "    float stage" << i << "_blkWetMix_{1.0f};\n"
               << "    float stage" << i << "_dcInL_{0.0f}, stage" << i << "_dcOutL_{0.0f};\n"
               << "    float stage" << i << "_dcInR_{0.0f}, stage" << i << "_dcOutR_{0.0f};\n"
               << "    float stage" << i << "_subInL_{0.0f}, stage" << i << "_subOutL_{0.0f};\n"
               << "    float stage" << i << "_subInR_{0.0f}, stage" << i << "_subOutR_{0.0f};\n"
               << "    " << meta.cppClassName << " stage" << i << "_;\n";
        }

        ss << "};\n\n"
           << "} // namespace avra::plugins\n";
        return ss.str();
    }

    static std::string generateCustomFaceplateEditorHeader(
        const std::string& name,
        const ProjectConfig& cfg,
        const FaceplateLayout& rawFaceplate) {
        (void)cfg;
        FaceplateLayout faceplate = rawFaceplate;
        bool hasMasterMeterInBay = false;
        for (auto& w : faceplate.widgets) {
            if (isMeterWidget(w.type) && w.x >= 730.0f) {
                hasMasterMeterInBay = true;
                w.boundStageIdx = -1;
            }
            if (w.boundStageIdx == -1 && (w.boundParamSlot == 4 || w.subtitle == "MONO (PURE)|STEREO" || w.title == "STEREO MODE" || w.title == "MONO (PURE)")) {
                w.title = "STEREO MODE";
                w.subtitle = "STEREO|MONO (PURE)";
                w.previewNorm = 1.0f;
            }
        }
        if (!hasMasterMeterInBay) {
            faceplate.widgets.push_back({
                faceplate.theme.globalMeterType, 776.0f, 88.0f, 180.0f, 88.0f,
                "GAIN REDUCTION dB", "AVRA AUDIO", UiScalePreset::LinearDb, -1, 111, 0.35f,
                faceplate.theme.globalAccentColor, faceplate.theme.globalFontSizePreset
            });
        }
        std::ostringstream ss;
        ss << "#pragma once\n\n"
           << "#include \"avra/Avra.h\"\n"
           << "#include \"EmbeddedAssets.h\"\n"
           << "#include \"" << name << ".h\"\n"
           << "#include <vector>\n"
           << "#include <memory>\n"
           << "#include <algorithm>\n\n"
           << "namespace avra::plugins {\n\n"
           << "/**\n"
           << " * @brief Custom Direct2D Brushed-Anodized Hardware Faceplate GUI for " << name << ".\n"
           << " * Designed in Avra Studio Faceplate UI Builder (1020x560 reference space).\n"
           << " * Uses EmbeddedAssets.h for 100% zero-dependency single-file .vst3 / .exe execution.\n"
           << " */\n"
           << "class " << name << "Editor : public Editor {\n"
           << "public:\n"
           << "    explicit " << name << "Editor(" << name << "& processor)\n"
           << "        : Editor(processor, 1020, 560)\n"
           << "        , processor_(processor)\n"
           << "        , licenseModalOpen_(avra::provenance::shouldShowFirstLaunchNotice(\"" << name << "\")) {\n"
           << "        (void)avra::provenance::keepProvenanceStampAlive();\n"
           << "        embedded::registerAllEmbeddedAssets();\n\n";

        const int totalStages = faceplate.getStageCount();
        const bool useStageStrip = faceplate.usesSignalChainSelectorStrip();

        for (size_t i = 0; i < faceplate.widgets.size(); ++i) {
            const auto& w = faceplate.widgets[i];
            if (isKnobWidget(w.type)) {
                const char* styleEnum = "gui::HardwareKnobStyle::PointerSkirted";
                if (w.type == UiWidgetType::SkirtedCapKnob) {
                    styleEnum = "gui::HardwareKnobStyle::SkirtedCap";
                } else if (w.type == UiWidgetType::ConcentricEqKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ConcentricDual";
                } else if (w.type == UiWidgetType::MarconiRedKnob) {
                    styleEnum = "gui::HardwareKnobStyle::MarconiRed";
                } else if (w.type == UiWidgetType::ModernArcKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ModernArc";
                } else if (w.type == UiWidgetType::BakeliteMasterKnob) {
                    styleEnum = "gui::HardwareKnobStyle::BakeliteMaster";
                } else if (w.type == UiWidgetType::ColorCapConsoleKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ColorCapConsole";
                } else if (w.type == UiWidgetType::ChickenHeadKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ChickenHead";
                } else if (w.type == UiWidgetType::FlatVectorKnob) {
                    styleEnum = "gui::HardwareKnobStyle::FlatVector";
                } else if (w.type == UiWidgetType::GoldDomeKnob) {
                    styleEnum = "gui::HardwareKnobStyle::GoldDome";
                } else if (w.type == UiWidgetType::MacroHaloKnob) {
                    styleEnum = "gui::HardwareKnobStyle::MacroHalo";
                } else if (w.type == UiWidgetType::KnurledBarrelKnob) {
                    styleEnum = "gui::HardwareKnobStyle::KnurledBlackBarrel";
                } else if (w.type == UiWidgetType::ColletBroadcastKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ColletBroadcast";
                } else if (w.type == UiWidgetType::StarburstSynthKnob) {
                    styleEnum = "gui::HardwareKnobStyle::StarburstSynth";
                } else if (w.type == UiWidgetType::GrimyRelicKnob) {
                    styleEnum = "gui::HardwareKnobStyle::GrimyRelic";
                } else if (w.type == UiWidgetType::SteppedAttenuatorKnob) {
                    styleEnum = "gui::HardwareKnobStyle::SteppedAttenuator";
                } else if (w.type == UiWidgetType::CyberLuxeHaloKnob) {
                    styleEnum = "gui::HardwareKnobStyle::CyberLuxeHalo";
                } else if (w.type == UiWidgetType::NeumorphicGlassKnob) {
                    styleEnum = "gui::HardwareKnobStyle::NeumorphicGlass";
                } else if (w.type == UiWidgetType::SciFiHudRingKnob) {
                    styleEnum = "gui::HardwareKnobStyle::SciFiHudRing";
                } else if (w.type == UiWidgetType::ValveWheelRelicKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ValveWheelRelic";
                } else if (w.type == UiWidgetType::SubmarineHelmKnob) {
                    styleEnum = "gui::HardwareKnobStyle::SubmarineHelm";
                } else if (w.type == UiWidgetType::TattooFlashKnob) {
                    styleEnum = "gui::HardwareKnobStyle::TattooFlash";
                } else if (w.type == UiWidgetType::MetalSkullKnob) {
                    styleEnum = "gui::HardwareKnobStyle::MetalSkull";
                } else if (w.type == UiWidgetType::FlowerDaisyKnob) {
                    styleEnum = "gui::HardwareKnobStyle::FlowerDaisy";
                } else if (w.type == UiWidgetType::ReaperCryptKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ReaperCrypt";
                } else if (w.type == UiWidgetType::HellfireMagmaKnob) {
                    styleEnum = "gui::HardwareKnobStyle::HellfireMagma";
                } else if (w.type == UiWidgetType::ApocalypseCogKnob) {
                    styleEnum = "gui::HardwareKnobStyle::ApocalypseCog";
                } else if (w.type == UiWidgetType::PunkSpikedKnob) {
                    styleEnum = "gui::HardwareKnobStyle::PunkSpiked";
                } else if (w.type == UiWidgetType::MeadowWoodSliceKnob) {
                    styleEnum = "gui::HardwareKnobStyle::MeadowWoodSlice";
                } else if (w.type == UiWidgetType::SkySunburstKnob) {
                    styleEnum = "gui::HardwareKnobStyle::SkySunburst";
                } else if (w.type == UiWidgetType::StormRaindropKnob) {
                    styleEnum = "gui::HardwareKnobStyle::StormRaindrop";
                } else if (w.type == UiWidgetType::GlacialIceCrystalKnob) {
                    styleEnum = "gui::HardwareKnobStyle::GlacialIceCrystal";
                }

                const auto ac = getUiAccentColorValue(w.accentColor);
                bool firstOr = false, lastOr = false;
                const UiScalePreset effectiveScale = (w.title == "LOW FREQ") ? UiScalePreset::LowFreqHz : w.scalePreset;
                std::vector<std::string> scaleVec;
                if (!w.customTickLabels.empty()) {
                    scaleVec = w.customTickLabels;
                    firstOr = (!scaleVec.empty() && (scaleVec.front() == "OFF" || scaleVec.front() == "-\xE2\x88\x9E" || scaleVec.front() == "-INF"));
                    lastOr  = (!scaleVec.empty() && (scaleVec.back() == "ALL" || scaleVec.back() == "MAX"));
                } else {
                    scaleVec = getScalePresetLabels(effectiveScale, firstOr, lastOr, w.title);
                }

                ss << "        {\n"
                   << "            auto k = std::make_unique<gui::HardwareKnob>(processor.resolveWidgetParam("
                   << w.boundStageIdx << ", " << w.boundParamSlot << "), "
                   << styleEnum << ", \"" << w.title << "\");\n"
                   << "            k->setAccentColor(gui::Color{" << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "            k->setScaleLabels({";
                for (size_t s = 0; s < scaleVec.size(); ++s) {
                    if (s > 0) ss << ", ";
                    ss << "\"" << scaleVec[s] << "\"";
                }
                ss << "}, " << (firstOr ? "true" : "false") << ", " << (lastOr ? "true" : "false") << ");\n"
                   << "            k->setBounds(" << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << ");\n"
                   << "            addChild(k.get());\n"
                   << "            knobs_.push_back(std::move(k));\n"
                   << "            knobStageIdx_.push_back(" << w.boundStageIdx << ");\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::VerticalStudioFader) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                bool firstOr = false, lastOr = false;
                const std::vector<std::string> faderTicks = !w.customTickLabels.empty()
                    ? w.customTickLabels
                    : getScalePresetLabels(w.scalePreset, firstOr, lastOr, w.title);
                ss << "        {\n"
                   << "            auto fd = std::make_unique<gui::HardwareVerticalFader>(processor.resolveWidgetParam("
                   << w.boundStageIdx << ", " << w.boundParamSlot << "), \""
                   << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "            fd->setCustomTickLabels({";
                for (size_t s = 0; s < faderTicks.size(); ++s) {
                    if (s > 0) ss << ", ";
                    ss << "\"" << faderTicks[s] << "\"";
                }
                ss << "});\n"
                   << "            fd->setPlaqueStyle(static_cast<gui::UiLabelPlaqueStyle>(" << static_cast<int>(faceplate.theme.labelPlaqueStyle) << "));\n"
                   << "            fd->setBounds(" << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << ");\n"
                   << "            addChild(fd.get());\n"
                   << "            faders_.push_back(std::move(fd));\n"
                   << "            faderStageIdx_.push_back(" << w.boundStageIdx << ");\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::ChromeToggleSwitch ||
                       w.type == UiWidgetType::RockerPowerSwitch ||
                       w.type == UiWidgetType::LockingCageToggle ||
                       w.type == UiWidgetType::FlatPillVectorSwitch ||
                       w.type == UiWidgetType::NeumorphicPillButton ||
                       w.type == UiWidgetType::SciFiHologramSwitch ||
                       w.type == UiWidgetType::NavalBrassToggle ||
                       w.type == UiWidgetType::PunkSafetyPinToggle ||
                       w.type == UiWidgetType::HellfireEmberSwitch) {
                std::string topLbl = "IN", botLbl = "BYP";
                const auto pipePos = w.subtitle.find('|');
                if (pipePos != std::string::npos) {
                    topLbl = w.subtitle.substr(0, pipePos);
                    botLbl = w.subtitle.substr(pipePos + 1);
                }
                if ((w.boundStageIdx == -1 && w.boundParamSlot == 4) || (topLbl == "MONO (PURE)" && botLbl == "STEREO")) {
                    topLbl = "STEREO";
                    botLbl = "MONO (PURE)";
                }
                const int swSlot = w.boundParamSlot;
                const std::string swTitle = (w.title == topLbl || w.title == botLbl) ? "" : w.title;
                const auto ac = getUiAccentColorValue(w.accentColor);
                const int swVisualStyle = (w.type == UiWidgetType::RockerPowerSwitch) ? 2
                    : (w.type == UiWidgetType::LockingCageToggle) ? 3
                    : (w.type == UiWidgetType::FlatPillVectorSwitch) ? 4
                    : (w.type == UiWidgetType::NeumorphicPillButton) ? 5
                    : (w.type == UiWidgetType::SciFiHologramSwitch) ? 6
                    : (w.type == UiWidgetType::NavalBrassToggle) ? 7
                    : (w.type == UiWidgetType::PunkSafetyPinToggle) ? 8
                    : (w.type == UiWidgetType::HellfireEmberSwitch) ? 9 : 0;
                ss << "        {\n"
                   << "            auto sw = std::make_unique<gui::HardwareToggleSwitch>(processor.resolveWidgetParam("
                   << w.boundStageIdx << ", " << swSlot << "), \""
                   << swTitle << "\", \"" << topLbl << "\", \"" << botLbl << "\");\n"
                   << "            sw->setSwitchVisualStyle(" << swVisualStyle << ");\n"
                   << "            sw->setAccentColor(gui::Color{" << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "            sw->setPlaqueStyle(static_cast<gui::UiLabelPlaqueStyle>(" << static_cast<int>(faceplate.theme.labelPlaqueStyle) << "));\n"
                   << "            sw->setBounds(" << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << ");\n"
                   << "            addChild(sw.get());\n"
                   << "            switches_.push_back(std::move(sw));\n"
                   << "            switchStageIdx_.push_back(" << w.boundStageIdx << ");\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::IlluminatedPushButton) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                const int swSlot = w.boundParamSlot;
                ss << "        {\n"
                   << "            auto pb = std::make_unique<gui::HardwarePushButton>(processor.resolveWidgetParam("
                   << w.boundStageIdx << ", " << swSlot << "), \""
                   << w.title << "\", \"" << w.subtitle << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "            pb->setBounds(" << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << ");\n"
                   << "            addChild(pb.get());\n"
                   << "            pushButtons_.push_back(std::move(pb));\n"
                   << "            pushButtonStageIdx_.push_back(" << w.boundStageIdx << ");\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::AnalogVuMeter) {
                ss << "        vuMeter_.setBounds(" << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << ");\n"
                   << "        addChild(&vuMeter_);\n";
            }
        }

        ss << "        syncActiveStageVisibility();\n"
           << "        syncEarsOnlyState();\n"
           << "    }\n\n"
           << "    void syncEarsOnlyState() {\n"
           << "        vuMeter_.setPoweredOffMode(earsOnlyBlackoutMode_);\n"
           << "        for (auto& k : knobs_) {\n"
           << "            if (k) k->setHideValueReadout(earsOnlyBlackoutMode_);\n"
           << "        }\n"
           << "        for (auto& fd : faders_) {\n"
           << "            if (fd) fd->setHideValueReadout(earsOnlyBlackoutMode_);\n"
           << "        }\n"
           << "    }\n\n"
           << "    void syncActiveStageVisibility() {\n"
           << "        for (size_t i = 0; i < knobs_.size(); ++i) {\n"
           << "            const int sIdx = knobStageIdx_[i];\n"
           << "            (void)sIdx;\n"
           << "            knobs_[i]->setVisible(" << (useStageStrip ? "(sIdx < 0 || sIdx == activeStageIdx_)" : "true") << ");\n"
           << "        }\n"
           << "        for (size_t i = 0; i < faders_.size(); ++i) {\n"
           << "            const int sIdx = faderStageIdx_[i];\n"
           << "            (void)sIdx;\n"
           << "            faders_[i]->setVisible(" << (useStageStrip ? "(sIdx < 0 || sIdx == activeStageIdx_)" : "true") << ");\n"
           << "        }\n"
           << "        for (size_t i = 0; i < switches_.size(); ++i) {\n"
           << "            const int sIdx = switchStageIdx_[i];\n"
           << "            (void)sIdx;\n"
           << "            switches_[i]->setVisible(" << (useStageStrip ? "(sIdx < 0 || sIdx == activeStageIdx_)" : "true") << ");\n"
           << "        }\n"
           << "        for (size_t i = 0; i < pushButtons_.size(); ++i) {\n"
           << "            const int sIdx = pushButtonStageIdx_[i];\n"
           << "            (void)sIdx;\n"
           << "            pushButtons_[i]->setVisible(" << (useStageStrip ? "(sIdx < 0 || sIdx == activeStageIdx_)" : "true") << ");\n"
           << "        }\n"
           << "    }\n\n"
           << "    void paint(gui::Graphics& g) override {\n"
           << "        const auto local = getLocalBounds();\n";

        const float topH = faceplate.theme.showTopToolbar ? 46.0f : 0.0f;
        if (faceplate.theme.showTopToolbar) {
            ss << "        g.fillLinearGradientRect({ 0.0f, 0.0f, local.width, 46.0f }, gui::Color::fromHex(0x23272C), gui::Color::fromHex(0x16191D));\n"
               << "        g.drawLine({ 0.0f, 46.0f }, { local.width, 46.0f }, gui::Color::fromHex(0x090C0E), 1.5f);\n"
               << "        g.drawImageFromMemory(embedded::kEmbeddedStudioIconPng, sizeof(embedded::kEmbeddedStudioIconPng), { 10.0f, 9.0f, 28.0f, 28.0f });\n"
               << "        g.drawText(\"" << faceplate.theme.pluginTitle << "\", { 44.0f, 8.0f, 360.0f, 30.0f }, gui::Color::fromHex(0xF0F6F8), 16.0f, false, true);\n"
               << "        avra::provenance::drawTopBarProvenancePill(g, { local.width - 332.0f, 9.0f, 318.0f, 28.0f }, licenseModalOpen_);\n";
        }

        const auto skinFontCol = getSkinFontColorValue(faceplate.theme);
        const auto skinPlateCol = getSkinTextPlateColorValue(faceplate.theme);
        const int activeTplIdx = std::clamp(faceplate.theme.styleTemplateIdx, 0, kTotalUiStyleTemplates - 1);
        const int backdropAssetIdx = getThemeBackdropAssetIndex(static_cast<UiStyleTemplate>(activeTplIdx));
        const bool hasBespokeBackdrop = (backdropAssetIdx > 0);
        ss << "        g.setFontFamily(static_cast<gui::FontFamily>(" << static_cast<int>(faceplate.theme.globalFontFamily) << "));\n"
           << "        g.setFontScale(" << std::format("{:.2f}f", getUiFontScaleFactor(faceplate.theme.globalFontSizePreset)) << ");\n"
           << "        g.setSkinTextColor(true, gui::Color{" << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", skinFontCol.r, skinFontCol.g, skinFontCol.b) << "});\n"
           << "        g.setSkinTextPlateColor(true, gui::Color{" << std::format("{:.3f}f, {:.3f}f, {:.3f}f, {:.3f}f", skinPlateCol.r, skinPlateCol.g, skinPlateCol.b, skinPlateCol.a) << "});\n"
           << "        gui::Rect<float> chassis = { 4.0f, " << std::format("{:.1f}f", topH + 4.0f)
           << ", local.width - 8.0f, local.height - " << std::format("{:.1f}f", topH + 8.0f) << " };\n";
        if (hasBespokeBackdrop) {
            ss << "        const bool drewBespoke = g.drawThemeBackdrop(\"" << std::format("backdrop_{}.jpg", backdropAssetIdx) << "\", chassis, 1.0f);\n"
               << "        if (drewBespoke) {\n";
            if (hasCustomSurfaceOrColorOverride(faceplate.theme)) {
                ss << "            gui::HardwareMaterials::drawSurfaceOverlay(g, chassis, "
                   << getChassisFinishEnumCode(faceplate.theme.finish) << ", "
                   << getSurfaceGrainEnumCode(faceplate.theme.grain) << ", "
                   << (faceplate.theme.useCustomFinishColor ? "true" : "false") << ", "
                   << std::format("0x{:06X}u", faceplate.theme.customFinishHex) << ", 0.56f);\n";
            }
            ss << "            gui::HardwareMaterials::drawChassisMountingAndScrews(g, chassis, "
               << (faceplate.theme.cornerScrews ? "true" : "false") << ", "
               << getHardwareStyleEnumCode(faceplate.theme.hardwareStyle) << ", "
               << getScrewHeadStyleEnumCode(faceplate.theme.screwStyle) << ", "
               << std::format("{:.1f}f", faceplate.theme.bevelDepthPx) << ");\n"
               << "        } else {\n"
               << "            gui::HardwareMaterials::drawBrushedAnodizedChassis(g, chassis, "
               << getChassisFinishEnumCode(faceplate.theme.finish) << ", "
               << getSurfaceGrainEnumCode(faceplate.theme.grain) << ", "
               << (faceplate.theme.cornerScrews ? "true" : "false") << ", "
               << getHardwareStyleEnumCode(faceplate.theme.hardwareStyle) << ", "
               << getScrewHeadStyleEnumCode(faceplate.theme.screwStyle) << ", "
               << (faceplate.theme.useCustomFinishColor ? "true" : "false") << ", "
               << std::format("0x{:06X}u", faceplate.theme.customFinishHex) << ", " << std::format("{:.1f}f", faceplate.theme.bevelDepthPx) << ");\n"
               << "        }\n";
        } else {
            ss << "        const bool drewBespoke = false;\n"
               << "        (void)drewBespoke;\n"
               << "        gui::HardwareMaterials::drawBrushedAnodizedChassis(g, chassis, "
               << getChassisFinishEnumCode(faceplate.theme.finish) << ", "
               << getSurfaceGrainEnumCode(faceplate.theme.grain) << ", "
               << (faceplate.theme.cornerScrews ? "true" : "false") << ", "
               << getHardwareStyleEnumCode(faceplate.theme.hardwareStyle) << ", "
               << getScrewHeadStyleEnumCode(faceplate.theme.screwStyle) << ", "
               << (faceplate.theme.useCustomFinishColor ? "true" : "false") << ", "
               << std::format("0x{:06X}u", faceplate.theme.customFinishHex) << ", " << std::format("{:.1f}f", faceplate.theme.bevelDepthPx) << ");\n";
        }
        ss << "        const auto plateCol = gui::HardwareMaterials::getSectionPlateColor("
           << getChassisFinishEnumCode(faceplate.theme.finish) << ", "
           << (faceplate.theme.useCustomFinishColor ? "true" : "false") << ", "
           << std::format("0x{:06X}u", faceplate.theme.customFinishHex) << ");\n";

        if (useStageStrip && totalStages > 1) {
            const float startX = 34.0f;
            const float gap    = 8.0f;
            const float tileY  = 58.0f;
            const float tileH  = 42.0f;
            const float tileW  = getSignalChainTileWidthRef(totalStages);
            const float stripRight = startX + static_cast<float>(totalStages) * tileW + static_cast<float>(totalStages - 1) * gap;

            const auto themeAc = getUiAccentColorValue(faceplate.theme.globalAccentColor);
            const std::string themeAcStr = std::format("gui::Color{{ {:.2f}f, {:.2f}f, {:.2f}f, 1.0f }}", themeAc.r, themeAc.g, themeAc.b);
            ss << "        {\n"
               << "            const auto stripBusCol = gui::HardwareMaterials::resolveChassisAccentInk(g, " << themeAcStr << ");\n"
               << "            g.drawLine({ 34.0f, 79.0f }, { " << std::format("{:.1f}f", stripRight) << ", 79.0f }, stripBusCol.withAlpha(0.36f), 1.3f);\n";
            for (int s = 0; s + 1 < totalStages; ++s) {
                const float gapX = startX + static_cast<float>(s) * (tileW + gap) + tileW;
                ss << "            g.drawText(\"\\xE2\\x96\\xB8\", { " << std::format("{:.1f}f, {:.1f}f, {:.1f}f, 16.0f", gapX, tileY + 14.0f, gap) << " }, stripBusCol.withAlpha(0.75f), 8.5f, true, true);\n";
            }
            ss << "        }\n";

            for (int s = 0; s < totalStages; ++s) {
                const float tx = startX + static_cast<float>(s) * (tileW + gap);
                const std::string desigHeader = std::format("S{}", s + 1);
                ss << "        {\n"
                   << "            const bool isAct = (activeStageIdx_ == " << s << ");\n"
                   << "            auto* ledP = processor_.resolveWidgetParam(" << s << ", 110);\n"
                   << "            const bool isOn = ledP ? (ledP->getPlain() >= 0.5f) : true;\n"
                   << "            gui::Rect<float> tr = { " << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", tx, tileY, tileW, tileH) << " };\n"
                   << "            gui::HardwareMaterials::drawRivetedChainTile(g, tr, \"" << desigHeader << "\", \"\", isAct, isOn, earsOnlyBlackoutMode_, 1.0f, plateCol, " << themeAcStr << ");\n"
                   << "        }\n";
            }
        }

        for (size_t wi = 0; wi < faceplate.widgets.size(); ++wi) {
            const auto& w = faceplate.widgets[wi];
            if (isKnobWidget(w.type) || w.type == UiWidgetType::VerticalStudioFader || isSwitchWidget(w.type)) {
                continue;
            }
            const bool guardStage = (useStageStrip && w.boundStageIdx >= 0 && !faceplate.isMasterMeterWidget(w) && w.x < 730.0f);
            if (guardStage) {
                ss << "        if (activeStageIdx_ == " << w.boundStageIdx << ") {\n";
            }
            if (w.type == UiWidgetType::SectionFrame) {
                const auto rawStyle = static_cast<gui::SectionFrameStyle>(static_cast<int>(w.scalePreset) % 8);
                const bool screwsEnabled = faceplate.theme.cornerScrews &&
                    (faceplate.theme.hardwareStyle != gui::ChassisHardwareStyle::CleanScrewless);
                const auto fStyle = (!screwsEnabled && rawStyle == gui::SectionFrameStyle::RivetedSubPlate)
                    ? gui::SectionFrameStyle::EngravedBevel
                    : rawStyle;
                const auto ac = getUiAccentColorValue(w.accentColor);
                const std::string outColStr = (w.accentColor == UiAccentColor::WarmAmber || w.title.find("METERING") != std::string::npos)
                    ? "gui::Color::fromHex(0x487A82, 0.42f)"
                    : std::format("gui::Color{{{:.3f}f, {:.3f}f, {:.3f}f, 0.55f}}", ac.r, ac.g, ac.b);
                ss << "        gui::HardwareMaterials::drawSectionFrame(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, \"" << w.title << "\", plateCol, " << outColStr << ", 11.5f, "
                   << getSectionFrameStyleEnumCode(fStyle) << ", "
                   << getScrewHeadStyleEnumCode(faceplate.theme.screwStyle) << ");\n";
            } else if (w.type == UiWidgetType::RivetedNameplate) {
                const auto pipePos = w.subtitle.find('|');
                std::string subLine1 = (pipePos != std::string::npos) ? w.subtitle.substr(0, pipePos) : w.subtitle;
                std::string subLine2 = !faceplate.theme.badgeFooterText.empty()
                    ? faceplate.theme.badgeFooterText
                    : ((pipePos != std::string::npos) ? w.subtitle.substr(pipePos + 1) : "CLASS-A EFFECTS");
                if (isLegacyGenericBadgeSub1(subLine1) && subLine2 != "PREAMP · EQ · DIODE BRIDGE DYNAMICS") {
                    subLine1 = "CUSTOM CHASSIS";
                }
                if (isLegacyGenericBadgeFooter(subLine2)) {
                    subLine2 = "CLASS-A EFFECTS";
                }
                const std::string badgeBrandHeader = "AVRA AUDIO";
                const std::string badgeModelTitle = !w.title.empty()
                    ? w.title
                    : (!faceplate.theme.pluginTitle.empty() ? faceplate.theme.pluginTitle : name);
                const auto bStyle = static_cast<gui::BadgeVisualStyle>(static_cast<int>(w.scalePreset) % 6);
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        gui::HardwareMaterials::drawRivetedNameplate(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, \"" << badgeBrandHeader << "\", \"" << badgeModelTitle << "\", \"" << subLine1 << "\", \"" << subLine2 << "\", "
                   << getBadgeVisualStyleEnumCode(bStyle) << ", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n";
            } else if (w.type == UiWidgetType::RackHandlePair) {
                ss << "        gui::HardwareMaterials::drawRackHandlePair(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, \"" << w.title << "\");\n";
            } else if (w.type == UiWidgetType::PatchbayJackSocket) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        gui::HardwareMaterials::drawPatchbayJackSocket(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n";
            } else if (w.type == UiWidgetType::PastelShaderCurveMeter) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawPastelShaderCurveMeter(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb, 0.0f, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "}, "
                   << std::format("0x{:06X}u", faceplate.theme.meterFaceplateHex) << ");\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::HorizontalCrossfader) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        gui::HardwareMaterials::drawHorizontalCrossfader(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, " << std::format("{:.2f}f", w.previewNorm) << ", \"" << w.title << "\", earsOnlyBlackoutMode_ ? \"\" : \"" << w.subtitle << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n";
            } else if (w.type == UiWidgetType::StereoGoniometerScope) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawStereoGoniometerScope(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb, 0.0f, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::NixieTubeDisplay) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawNixieTubeDisplay(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb, 0.0f, \"" << w.title << "\", \"" << w.subtitle << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::DigitalTrimBargraph) {
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawDigitalTrimBargraph(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb - 1.5f, outDb);\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::LedBargraphMeter) {
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDbL = math::linearTodB(std::max(1e-5f, processor_.getPeakL()));\n"
                   << "            const float outDbR = math::linearTodB(std::max(1e-5f, processor_.getPeakR()));\n"
                   << "            const float normL  = (outDbL <= -58.0f) ? 0.0f : std::clamp((outDbL + 36.0f) / 39.0f, 0.0f, 1.0f);\n"
                   << "            const float normR  = (outDbR <= -58.0f) ? 0.0f : std::clamp((outDbR + 36.0f) / 39.0f, 0.0f, 1.0f);\n"
                   << "            gui::HardwareMaterials::drawLedPeakBargraph(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, normL, \"" << w.title << "\", normR);\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::JewelLed) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                const int ledSlot = (w.boundParamSlot >= 0) ? w.boundParamSlot : 110;
                ss << "        {\n"
                   << "            auto* ledParam = processor_.resolveWidgetParam(" << w.boundStageIdx << ", " << ledSlot << ");\n"
                   << "            const bool ledOn = !earsOnlyBlackoutMode_ && (ledParam ? (ledParam->getPlain() >= 0.5f) : true);\n"
                   << "            gui::HardwareMaterials::drawJewelLed(g, { "
                   << std::format("{:.1f}f, {:.1f}f", w.x + w.w * 0.5f, w.y + w.h * 0.5f)
                   << " }, ledOn, gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "}, 6.0f);\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::VentilationGrille) {
                ss << "        gui::HardwareMaterials::drawVentilationGrille(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, \"" << w.title << "\");\n";
            } else if (w.type == UiWidgetType::RoundVintageVuMeter) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawRoundVintageVu(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, processor_.getGainReductionDb(), outDb, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "}, false);\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::GlowingTubeWindow) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawGlowingTubeWindow(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb, 0.0f, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::CrtOscilloscopeScreen) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawCrtOscilloscopeScreen(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb, 0.0f, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::LcdParameterReadout) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::drawLcdParameterReadout(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, outDb - 1.5f, outDb, 0.0f, \"" << w.title << "\", \"" << w.subtitle << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "});\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::CyberLuxeMultiBarMeter) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDbL = math::linearTodB(std::max(1e-5f, processor_.getPeakL()));\n"
                   << "            const float outDbR = math::linearTodB(std::max(1e-5f, processor_.getPeakR()));\n"
                   << "            const float normL  = (outDbL <= -58.0f) ? 0.0f : std::clamp((outDbL + 36.0f) / 39.0f, 0.0f, 1.0f);\n"
                   << "            const float normR  = (outDbR <= -58.0f) ? 0.0f : std::clamp((outDbR + 36.0f) / 39.0f, 0.0f, 1.0f);\n"
                   << "            gui::HardwareMaterials::drawCyberLuxeMultiBarMeter(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, normL, \"" << w.title << "\", normR, gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "}, false);\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::NeumorphicDualArcMeter ||
                       w.type == UiWidgetType::SubmarinePortholeGauge ||
                       w.type == UiWidgetType::ReaperScytheArchMeter ||
                       w.type == UiWidgetType::HellfireHornedVuMeter ||
                       w.type == UiWidgetType::ApocalypseGeigerVuMeter ||
                       w.type == UiWidgetType::PunkAnarchyVuMeter ||
                       w.type == UiWidgetType::NatureElementalVuMeter) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                const char* fnName = (w.type == UiWidgetType::NeumorphicDualArcMeter) ? "drawNeumorphicDualArcMeter"
                    : (w.type == UiWidgetType::SubmarinePortholeGauge) ? "drawSubmarinePortholeGauge"
                    : (w.type == UiWidgetType::ReaperScytheArchMeter) ? "drawReaperScytheArchMeter"
                    : (w.type == UiWidgetType::HellfireHornedVuMeter) ? "drawHellfireHornedVuMeter"
                    : (w.type == UiWidgetType::ApocalypseGeigerVuMeter) ? "drawApocalypseGeigerVuMeter"
                    : (w.type == UiWidgetType::PunkAnarchyVuMeter) ? "drawPunkAnarchyVuMeter"
                    : "drawNatureElementalVuMeter";
                ss << "        if (earsOnlyBlackoutMode_) {\n"
                   << "            gui::HardwareMaterials::drawPoweredOffMeterHousing(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h) << " });\n"
                   << "        } else {\n"
                   << "            const float outDb = math::linearTodB(std::max(1e-5f, std::max(processor_.getPeakL(), processor_.getPeakR())));\n"
                   << "            gui::HardwareMaterials::" << fnName << "(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, processor_.getGainReductionDb(), outDb, \"" << w.title << "\", gui::Color{"
                   << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "}, false);\n"
                   << "        }\n";
            } else if (w.type == UiWidgetType::VintageSticker) {
                const auto sPreset = static_cast<gui::StickerPreset>(std::abs(static_cast<int>(w.scalePreset)) % gui::kStickerPresetCount);
                const auto ac = getUiAccentColorValue(w.accentColor);
                const uint32_t cornerSeed = static_cast<uint32_t>(wi * 2654435761u)
                                          ^ static_cast<uint32_t>(static_cast<uint32_t>(w.scalePreset) * 2246822519u)
                                          ^ static_cast<uint32_t>(w.title.size() * 374761393u);
                std::string escapedImgPath = w.customImagePath;
                for (char& ch : escapedImgPath) {
                    if (ch == '\\') ch = '/';
                }
                ss << "        gui::HardwareMaterials::drawVintageSticker(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, " << getStickerPresetEnumCode(sPreset) << ", \"" << w.title << "\", \"" << w.subtitle
                   << "\", gui::Color{" << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b)
                   << "}, \"" << escapedImgPath << "\", 10.5f, " << std::format("{:.1f}f, {}u", w.rotationDeg, cornerSeed) << ");\n";
            } else if (w.type == UiWidgetType::SilkscreenLabel) {
                const auto ac = getUiAccentColorValue(w.accentColor);
                const float fontSizePt = getUiFontSizePt(w.fontSizePreset);
                ss << "        {\n"
                   << "            const gui::Color accentCol{" << std::format("{:.3f}f, {:.3f}f, {:.3f}f, 1.0f", ac.r, ac.g, ac.b) << "};\n"
                   << "            const gui::Color lblCol = (" << (w.accentColor == UiAccentColor::WarmAmber ? "true" : "false") << ")\n"
                   << "                ? g.resolveSilkscreenColor(gui::Color::fromHex(0xE8F1F2))\n"
                   << "                : gui::HardwareMaterials::resolveChassisAccentInk(g, accentCol);\n"
                   << "            gui::HardwareMaterials::drawLabelPlaque(g, { "
                   << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f", w.x, w.y, w.w, w.h)
                   << " }, \"" << w.title << "\", static_cast<gui::UiLabelPlaqueStyle>("
                   << static_cast<int>(faceplate.theme.labelPlaqueStyle) << "), lblCol, "
                   << std::format("{:.1f}f", fontSizePt) << ", accentCol, true);\n"
                   << "        }\n";
            }
            if (guardStage) {
                ss << "        }\n";
            }
        }

        ss << "        const float outPeak = std::max(processor_.getPeakL(), processor_.getPeakR());\n"
           << "        const float outDb   = math::linearTodB(std::max(1e-5f, outPeak));\n"
           << "        vuMeter_.setPoweredOffMode(earsOnlyBlackoutMode_);\n"
           << "        vuMeter_.setOutputLevelDb(earsOnlyBlackoutMode_ ? -60.0f : outDb);\n"
           << "        vuMeter_.setGainReductionDb(earsOnlyBlackoutMode_ ? 0.0f : processor_.getGainReductionDb());\n"
           << "    }\n\n"
           << "    void paintOverlay(gui::Graphics& g) override {\n"
           << "        const auto local = getLocalBounds();\n"
           << "        if (!avra::provenance::verifyIntegrity()) {\n"
           << "            avra::provenance::drawTamperWarningBanner(g, local);\n"
           << "        }\n"
           << "        if (licenseModalOpen_) {\n"
           << "            avra::provenance::drawProvenanceModalOverlay(g, local, \"" << faceplate.theme.pluginTitle << "\");\n"
           << "        }\n"
           << "    }\n\n"
           << "    [[nodiscard]] bool interceptsOverlayMouse(gui::Point<float> pt) const noexcept override {\n"
           << "        (void)pt;\n"
           << "        return licenseModalOpen_;\n"
           << "    }\n\n"
           << "    void onMouseDown(const gui::MouseEvent& e) override {\n"
           << "        const auto local = getLocalBounds();\n"
           << "        if (licenseModalOpen_) {\n"
           << "            licenseModalOpen_ = false;\n"
           << "            avra::provenance::markFirstLaunchNoticeSeen(\"" << name << "\");\n"
           << "            repaint();\n"
           << "            return;\n"
           << "        }\n"
           << "        const gui::Rect<float> iconBtn = { 10.0f, 9.0f, 28.0f, 28.0f };\n"
           << "        const gui::Rect<float> provBtn = { local.width - 332.0f, 9.0f, 318.0f, 28.0f };\n"
           << "        if (iconBtn.contains(e.pos) || provBtn.contains(e.pos)) {\n"
           << "            licenseModalOpen_ = !licenseModalOpen_;\n"
           << "            repaint();\n"
           << "            return;\n"
           << "        }\n";
        if (useStageStrip && totalStages > 1) {
            const float startX = 34.0f;
            const float gap    = 8.0f;
            const float tileW  = getSignalChainTileWidthRef(totalStages);
            ss << "        if (e.pos.x >= 34.0f && e.pos.x <= 730.0f && e.pos.y >= 58.0f && e.pos.y <= 100.0f) {\n"
               << "            for (int s = 0; s < " << totalStages << "; ++s) {\n"
               << "                const float tx = " << std::format("{:.1f}f", startX) << " + static_cast<float>(s) * " << std::format("{:.2f}f", tileW + gap) << ";\n"
               << "                if (e.pos.x >= tx && e.pos.x <= tx + " << std::format("{:.2f}f", tileW) << ") {\n"
               << "                    if (e.pos.x <= tx + 19.0f && e.pos.y <= 81.0f) {\n"
               << "                        if (auto* ledP = processor_.resolveWidgetParam(s, 110)) {\n"
               << "                            ledP->setNormalized(ledP->getPlain() >= 0.5f ? 0.0f : 1.0f);\n"
               << "                        }\n"
               << "                    } else {\n"
               << "                        activeStageIdx_ = s;\n"
               << "                        syncActiveStageVisibility();\n"
               << "                    }\n"
               << "                    repaint();\n"
               << "                    return;\n"
               << "                }\n"
               << "            }\n"
               << "        }\n";
        }
        ss << "        Editor::onMouseDown(e);\n"
           << "    }\n\n"
           << "private:\n"
           << "    " << name << "& processor_;\n"
           << "    int activeStageIdx_{0};\n"
           << "    bool earsOnlyBlackoutMode_{false};\n"
           << "    bool licenseModalOpen_{false};\n"
           << "    gui::AnalogVuMeter vuMeter_;\n"
           << "    std::vector<std::unique_ptr<gui::HardwareKnob>> knobs_;\n"
           << "    std::vector<int> knobStageIdx_;\n"
           << "    std::vector<std::unique_ptr<gui::HardwareVerticalFader>> faders_;\n"
           << "    std::vector<int> faderStageIdx_;\n"
           << "    std::vector<std::unique_ptr<gui::HardwareToggleSwitch>> switches_;\n"
           << "    std::vector<int> switchStageIdx_;\n"
           << "    std::vector<std::unique_ptr<gui::HardwarePushButton>> pushButtons_;\n"
           << "    std::vector<int> pushButtonStageIdx_;\n"
           << "};\n\n"
           << "inline std::unique_ptr<Editor> " << name << "::createEditor() {\n"
           << "    return std::make_unique<" << name << "Editor>(*this);\n"
           << "}\n\n"
           << "} // namespace avra::plugins\n";

        return ss.str();
    }

    /**
     * @brief Loads `assets/avra_studio_icon.png` and encodes a compact 128x128 PNG byte array in memory
     * (cached once per session) so `EmbeddedAssets.h` embeds a genuine, zero-dependency PNG icon.
     */
    static const std::vector<uint8_t>& getEmbeddedIconBytes() {
        static std::vector<uint8_t> s_iconBytes;
        if (!s_iconBytes.empty()) return s_iconBytes;

        const std::filesystem::path iconPath = getWorkspaceRoot() / "assets" / "avra_studio_icon.png";
#if defined(_WIN32)
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        IWICImagingFactory* wicFactory = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory))) && wicFactory) {
            IWICBitmapDecoder* decoder = nullptr;
            if (SUCCEEDED(wicFactory->CreateDecoderFromFilename(
                    iconPath.wstring().c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)) && decoder) {
                IWICBitmapFrameDecode* frame = nullptr;
                if (SUCCEEDED(decoder->GetFrame(0, &frame)) && frame) {
                    IWICBitmapScaler* scaler = nullptr;
                    if (SUCCEEDED(wicFactory->CreateBitmapScaler(&scaler)) && scaler) {
                        if (SUCCEEDED(scaler->Initialize(frame, 128, 128, WICBitmapInterpolationModeFant))) {
                            IStream* memStream = nullptr;
                            if (SUCCEEDED(CreateStreamOnHGlobal(nullptr, TRUE, &memStream)) && memStream) {
                                IWICBitmapEncoder* encoder = nullptr;
                                if (SUCCEEDED(wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) && encoder) {
                                    if (SUCCEEDED(encoder->Initialize(memStream, WICBitmapEncoderNoCache))) {
                                        IWICBitmapFrameEncode* frameEncode = nullptr;
                                        if (SUCCEEDED(encoder->CreateNewFrame(&frameEncode, nullptr)) && frameEncode) {
                                            if (SUCCEEDED(frameEncode->Initialize(nullptr)) &&
                                                SUCCEEDED(frameEncode->SetSize(128, 128))) {
                                                WICPixelFormatGUID pf = GUID_WICPixelFormat32bppBGRA;
                                                frameEncode->SetPixelFormat(&pf);
                                                if (SUCCEEDED(frameEncode->WriteSource(scaler, nullptr)) &&
                                                    SUCCEEDED(frameEncode->Commit()) &&
                                                    SUCCEEDED(encoder->Commit())) {
                                                    HGLOBAL hMem = nullptr;
                                                    if (SUCCEEDED(GetHGlobalFromStream(memStream, &hMem)) && hMem) {
                                                        const SIZE_T sz = GlobalSize(hMem);
                                                        const uint8_t* ptr = static_cast<const uint8_t*>(GlobalLock(hMem));
                                                        if (ptr && sz > 0) {
                                                            s_iconBytes.assign(ptr, ptr + sz);
                                                            GlobalUnlock(hMem);
                                                        }
                                                    }
                                                }
                                            }
                                            frameEncode->Release();
                                        }
                                    }
                                    encoder->Release();
                                }
                                memStream->Release();
                            }
                        }
                        scaler->Release();
                    }
                    frame->Release();
                }
                decoder->Release();
            }
            wicFactory->Release();
        }
#endif
        if (s_iconBytes.empty()) {
            // Valid 1x1 Cyan PNG fallback if WIC or file is unavailable
            static constexpr uint8_t kFallbackPng[] = {
                0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
                0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
                0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x60, 0xF8, 0xCF, 0xC0,
                0xF0, 0x1F, 0x00, 0x05, 0x00, 0x01, 0xFF, 0x89, 0x99, 0x3D, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x49,
                0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
            };
            s_iconBytes.assign(std::begin(kFallbackPng), std::end(kFallbackPng));
        }
        return s_iconBytes;
    }

    static void appendByteArrayLiteral(std::ostringstream& ss, const uint8_t* data, size_t size) {
        if (!data || size == 0) {
            ss << "    0x00\n";
            return;
        }
        for (size_t i = 0; i < size; ++i) {
            if (i % 16 == 0) ss << "    ";
            ss << std::format("0x{:02X}", data[i]);
            if (i + 1 < size) ss << ", ";
            if (i % 16 == 15 || i + 1 == size) ss << "\n";
        }
    }

    /**
     * @brief Generates `EmbeddedAssets.h` containing `constexpr uint8_t[]` byte arrays for:
     *   1. `assets/avra_studio_icon.png` (`kEmbeddedStudioIconPng`)
     *   2. Serialized `.faceplate` layout specification (`kEmbeddedFaceplateLayoutBytes`)
     *   3. Sticker geometries, text, and any custom sticker image byte arrays (`kEmbeddedStickers`)
     *   4. Unified skin palette hex table & raw palette bytes (`kEmbeddedSkinPaletteHex`, `kEmbeddedSkinPaletteBytes`)
     * Every exported `.vst3` and `.exe` compiles into a 100% self-contained single file with zero external folders.
     */
    static std::string generateEmbeddedAssetsHeader(
        const std::string& name,
        const FaceplateLayout& faceplate) {
        std::ostringstream ss;
        ss << "#pragma once\n\n"
           << "#include <cstdint>\n"
           << "#include <cstddef>\n"
           << "#include <string_view>\n"
           << "#include \"avra/gui/Graphics.h\"\n\n"
           << "namespace avra::plugins::embedded {\n\n"
           << "// ============================================================================\n"
           << "// 1. Embedded Studio Icon PNG (`assets/avra_studio_icon.png`)\n"
           << "// ============================================================================\n"
           << "inline constexpr uint8_t kEmbeddedStudioIconPng[] = {\n";

        const auto& iconBytes = getEmbeddedIconBytes();
        appendByteArrayLiteral(ss, iconBytes.data(), iconBytes.size());
        ss << "};\n\n";

        // 2. Embedded Serialized Faceplate Layout (`constexpr uint8_t[]`)
        const std::string layoutSpecStr = serializeFaceplateLayoutToString(faceplate);
        ss << "// ============================================================================\n"
           << "// 2. Embedded Serialized Faceplate Layout (`" << name << ".faceplate`)\n"
           << "// ============================================================================\n"
           << "inline constexpr uint8_t kEmbeddedFaceplateLayoutBytes[] = {\n";
        appendByteArrayLiteral(
            ss,
            reinterpret_cast<const uint8_t*>(layoutSpecStr.data()),
            layoutSpecStr.size());
        ss << "};\n\n"
           << "[[nodiscard]] inline constexpr std::string_view getEmbeddedFaceplateLayoutSpec() noexcept {\n"
           << "    return std::string_view(reinterpret_cast<const char*>(kEmbeddedFaceplateLayoutBytes), sizeof(kEmbeddedFaceplateLayoutBytes));\n"
           << "}\n\n";

        // 3. Embedded Skin Palette (`constexpr uint32_t[]` & `constexpr uint8_t[]`)
        const uint32_t paletteWords[8] = {
            faceplate.theme.customFinishHex,
            faceplate.theme.knobBodyHex,
            faceplate.theme.knobCapHex,
            faceplate.theme.knobIndicatorHex,
            faceplate.theme.meterFaceplateHex,
            faceplate.theme.meterGlowHex,
            faceplate.theme.customFontColorHex,
            static_cast<uint32_t>(faceplate.theme.styleTemplateIdx)
        };
        ss << "// ============================================================================\n"
           << "// 3. Embedded Skin Palette & Material Hex Table\n"
           << "// ============================================================================\n"
           << "inline constexpr uint32_t kEmbeddedSkinPaletteHex[8] = {\n"
           << std::format("    0x{:06X}u, 0x{:06X}u, 0x{:06X}u, 0x{:06X}u, 0x{:06X}u, 0x{:06X}u, 0x{:06X}u, {}u\n",
                          paletteWords[0], paletteWords[1], paletteWords[2], paletteWords[3],
                          paletteWords[4], paletteWords[5], paletteWords[6], paletteWords[7])
           << "};\n\n"
           << "inline constexpr uint8_t kEmbeddedSkinPaletteBytes[] = {\n";
        appendByteArrayLiteral(ss, reinterpret_cast<const uint8_t*>(paletteWords), sizeof(paletteWords));
        ss << "};\n\n";

        // 4. Embedded Sticker Geometries, Text & Custom Sticker Image Byte Arrays
        ss << "// ============================================================================\n"
           << "// 4. Embedded Sticker Geometries, Text & Custom Decal Byte Arrays\n"
           << "// ============================================================================\n"
           << "struct EmbeddedStickerSpec {\n"
           << "    uint16_t    preset;\n"
           << "    float       x, y, w, h, rotationDeg;\n"
           << "    const char* title;\n"
           << "    const char* subtitle;\n"
           << "    const char* customImagePath;\n"
           << "    const uint8_t* customImageBytes;\n"
           << "    size_t      customImageSize;\n"
           << "};\n\n";

        struct CustomImgEntry {
            size_t widgetIdx;
            std::string path;
            std::vector<uint8_t> bytes;
        };
        std::vector<CustomImgEntry> customImgs;

        for (size_t i = 0; i < faceplate.widgets.size(); ++i) {
            const auto& w = faceplate.widgets[i];
            if (w.type == UiWidgetType::VintageSticker && !w.customImagePath.empty()) {
                std::ifstream imgFile(w.customImagePath, std::ios::binary);
                if (imgFile.is_open()) {
                    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(imgFile)), std::istreambuf_iterator<char>());
                    if (!buf.empty() && buf.size() <= 262144u) {
                        std::string normPath = w.customImagePath;
                        for (char& ch : normPath) {
                            if (ch == '\\') ch = '/';
                        }
                        ss << "inline constexpr uint8_t kEmbeddedStickerImg_" << i << "[] = {\n";
                        appendByteArrayLiteral(ss, buf.data(), buf.size());
                        ss << "};\n\n";
                        customImgs.push_back({ i, normPath, std::move(buf) });
                    }
                }
            }
        }

        ss << "inline constexpr EmbeddedStickerSpec kEmbeddedStickers[] = {\n";
        size_t stickerCount = 0;
        for (size_t i = 0; i < faceplate.widgets.size(); ++i) {
            const auto& w = faceplate.widgets[i];
            if (w.type != UiWidgetType::VintageSticker) continue;
            ++stickerCount;
            const uint16_t presetId = static_cast<uint16_t>(std::abs(static_cast<int>(w.scalePreset)) % gui::kStickerPresetCount);
            std::string normPath = w.customImagePath;
            for (char& ch : normPath) {
                if (ch == '\\') ch = '/';
            }
            bool hasCustomBytes = false;
            for (const auto& ce : customImgs) {
                if (ce.widgetIdx == i) {
                    hasCustomBytes = true;
                    break;
                }
            }
            ss << "    { " << presetId << "u, "
               << std::format("{:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f, {:.1f}f, ", w.x, w.y, w.w, w.h, w.rotationDeg)
               << "\"" << w.title << "\", \"" << w.subtitle << "\", \"" << normPath << "\", "
               << (hasCustomBytes ? std::format("kEmbeddedStickerImg_{}, sizeof(kEmbeddedStickerImg_{})", i, i) : "nullptr, 0u")
               << " },\n";
        }
        if (stickerCount == 0) {
            ss << "    { 0u, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, \"\", \"\", \"\", nullptr, 0u }\n";
        }
        ss << "};\n\n"
           << "inline constexpr size_t kEmbeddedStickerCount = " << stickerCount << "u;\n\n"
           << "inline void registerAllEmbeddedAssets() {\n"
           << "    gui::Graphics::registerEmbeddedImage(\"assets/avra_studio_icon.png\", kEmbeddedStudioIconPng, sizeof(kEmbeddedStudioIconPng));\n";
        for (const auto& ce : customImgs) {
            ss << "    gui::Graphics::registerEmbeddedImage(\"" << ce.path << "\", kEmbeddedStickerImg_" << ce.widgetIdx
               << ", sizeof(kEmbeddedStickerImg_" << ce.widgetIdx << "));\n";
        }
        ss << "}\n\n"
           << "} // namespace avra::plugins::embedded\n";

        return ss.str();
    }

private:
    std::vector<SchematicNode> chain_{};
};

} // namespace avra::studio
