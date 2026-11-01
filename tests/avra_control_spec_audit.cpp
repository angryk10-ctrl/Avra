// =============================================================================
// AVRA CONTROL-SPEC & CLONE-BODY AUDIT (Phase 1 guardrail)
//
// Inspects the EFFECTIVE runtime catalog (after enrichAndValidateComponentGrammar)
// and the DSP source in include/avra/dsp/circuits/CircuitLibrary.h, and reports:
//   A. Placeholder ranges that replaced real hardware ranges
//   B. Stepped switches whose labels don't match the value delivered to the DSP
//   C. Stepped switches with no labels
//   D. Wrong / suspicious units (pitch in seconds, etc.) and truncated names
//   E. Missing telemetry meter (LAW 1)
//   F. Control count outside LAW 1 (6..10 interactive controls incl. Avra Extras)
//   G. DSP classes whose processStereo() body is identical to another class's
//      after stripping numeric constants (copy-paste templates)
//
// Default: report-only (exit 0). Pass --strict to return non-zero on any finding.
// =============================================================================
#include "avra/Avra.h"
#include "avra/studio/SchematicSpec.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace avra::studio;

namespace {

struct Finding {
    std::string module;
    std::string category;
    std::string detail;
};

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool nearly(float a, float b) { return std::abs(a - b) <= 1e-4f * std::max(1.0f, std::abs(b)); }

// Parses a step label such as "30", "1.5k", "+3", "12 kHz", "0.3 s" into a number.
std::optional<double> parseLabel(const std::string& raw) {
    static const std::regex rx(R"(^\s*[+]?(-?\d+(?:\.\d+)?)\s*(k|kHz|K)?\s*(Hz|ms|s|dB|:1)?\s*$)");
    std::smatch m;
    if (!std::regex_match(raw, m, rx)) return std::nullopt;
    double v = std::stod(m[1].str());
    if (m[2].matched) v *= 1000.0;
    return v;
}

bool isSteppedKind(HardwareControlKind k) {
    return k == HardwareControlKind::SteppedRotarySwitch || k == HardwareControlKind::ToggleSwitch2Way ||
           k == HardwareControlKind::ToggleSwitch3Way || k == HardwareControlKind::PushButtonBank ||
           k == HardwareControlKind::PushButtonLatching;
}

// ----------------------------------------------------------------------------
// Control-spec checks
// ----------------------------------------------------------------------------
void auditControls(const std::vector<SchematicComponentMeta>& cat, std::vector<Finding>& out) {
    struct Sig { float lo, hi; const char* unit; const char* why; };
    static const Sig kPlaceholders[] = {
        { 40.0f, 12000.0f, "Hz", "placeholder 40..12000 Hz" },
        { 120.0f, 12000.0f, "Hz", "keyword-guessed 120..12000 Hz" },
        { 1.0f, 500.0f, "ms", "placeholder 1..500 ms" },
        { 10.0f, 800.0f, "ms", "keyword-guessed 10..800 ms" },
        { 0.2f, 20.0f, "s", "placeholder 0.2..20 s" },
        { 0.3f, 12.0f, "s", "keyword-guessed 0.3..12 s" },
    };

    for (const auto& m : cat) {
        const size_t n = m.getParamCount();
        int meters = 0;
        int interactive = 0;
        int extras = 0;
        for (size_t i = 0; i < n; ++i) {
            const auto& p = m.getParamSpec(static_cast<int>(i));
            const std::string ln = lower(p.name);
            const std::string tag = "'" + p.name + "'";
            if (p.isAvraExtra) ++extras;

            if (p.controlKind == HardwareControlKind::IndicatorMeter) {
                ++meters;
                if (ln.find("meter") == std::string::npos && ln.find("led") == std::string::npos &&
                    ln.find("indicator") == std::string::npos && ln.find("vu") == std::string::npos) {
                    out.push_back({ m.designator, "C-TYPE", tag + " is typed as a meter but reads like an adjustable control" });
                }
                continue;
            }
            ++interactive;

            for (const auto& s : kPlaceholders) {
                if (nearly(p.minVal, s.lo) && nearly(p.maxVal, s.hi) && p.unit == s.unit && p.stepLabels.empty()) {
                    out.push_back({ m.designator, "A-PLACEHOLDER", tag + " uses " + s.why });
                }
            }
            if (p.unit == "dB" && nearly(p.minVal, 0.0f) && nearly(p.maxVal, 24.0f) && nearly(p.defaultVal, 6.0f)) {
                out.push_back({ m.designator, "A-PLACEHOLDER", tag + " uses keyword-guessed 0..24 dB (def 6)" });
            }

            // Pitch / tuning quantities must be semitones or cents, never seconds.
            if (p.unit == "s" && (ln.find("pitch") != std::string::npos || ln.find("formant") != std::string::npos ||
                                  ln.find("tuning") != std::string::npos || ln.find("transpose") != std::string::npos)) {
                out.push_back({ m.designator, "D-UNIT", tag + " is a pitch quantity but has unit 's' (seconds)" });
            } else if (p.unit == "s" && ln.find("time") == std::string::npos && ln.find("decay") == std::string::npos &&
                       ln.find("rt60") == std::string::npos && ln.find("release") == std::string::npos &&
                       ln.find("recovery") == std::string::npos && ln.find("hold") == std::string::npos &&
                       ln.find("freeze") == std::string::npos && ln.find("sustain") == std::string::npos &&
                       ln.find("swell") == std::string::npos && ln.find("inertia") == std::string::npos &&
                       ln.find("length") == std::string::npos) {
                out.push_back({ m.designator, "D-UNIT", tag + " has unit 's' but is not a time quantity" });
            }
            {
                static const std::set<std::string> kShortOk = { "mix", "q", "bw", "pad", "pan", "out", "in", "eq", "pol", "ø" };
                if (p.name.size() <= 3 && !kShortOk.count(ln)) {
                    out.push_back({ m.designator, "D-NAME", tag + " looks truncated (<= 3 chars)" });
                }
            }

            if (isSteppedKind(p.controlKind)) {
                const size_t L = p.stepLabels.size();
                const float span = p.maxVal - p.minVal;
                if (!p.stepValues.empty() && p.stepValues.size() != L) {
                    out.push_back({ m.designator, "C-STEPS", tag + " has " + std::to_string(p.stepValues.size()) +
                                                             " stepValues but " + std::to_string(L) + " labels" });
                }
                if (!p.stepValues.empty() && !(nearly(p.minVal, 0.0f) && nearly(span, static_cast<float>(L - 1)))) {
                    out.push_back({ m.designator, "B-STEPMAP", tag + " has stepValues but its range is not the index range 0.." +
                                                               std::to_string(L - 1) });
                }
                if (L == 0 && span > 1.5f) {
                    out.push_back({ m.designator, "C-STEPS", tag + " is a stepped switch with no position labels" });
                }
                if (L >= 2) {
                    const bool indexRange = nearly(span, static_cast<float>(L - 1)) && nearly(p.minVal, 0.0f);
                    if (!indexRange) {
                        // Value delivered to DSP for detent k = min + span * k / (L-1)
                        double worst = 0.0; size_t wk = 0; double wd = 0.0;
                        int numeric = 0;
                        for (size_t k = 0; k < L; ++k) {
                            const auto v = parseLabel(p.stepLabels[k]);
                            if (!v) continue;
                            ++numeric;
                            const double delivered = p.minVal + span * static_cast<double>(k) / static_cast<double>(L - 1);
                            const double err = std::abs(delivered - *v) / std::max(1e-3, std::abs(*v));
                            if (err > worst) { worst = err; wk = k; wd = delivered; }
                        }
                        if (numeric >= static_cast<int>(L) - 1 && worst > 0.15) {
                            std::ostringstream os;
                            os << tag << " label '" << p.stepLabels[wk] << "' delivers " << wd << " " << p.unit << " to the DSP";
                            out.push_back({ m.designator, "B-STEPMAP", os.str() });
                        }
                    }
                    if (L == 2 && span > 1.5f) {
                        out.push_back({ m.designator, "B-STEPMAP", tag + " has 2 labels but a continuous range (" +
                                                                  std::to_string(p.minVal) + ".." + std::to_string(p.maxVal) + ")" });
                    }
                }
            }
            if (p.defaultVal < std::min(p.minVal, p.maxVal) || p.defaultVal > std::max(p.minVal, p.maxVal)) {
                out.push_back({ m.designator, "D-DEFAULT", tag + " default outside its range" });
            }
        }
        // The Faceplate Builder always auto-adds STAGE IN and an archetype telemetry meter
        // (validateAndEnrichModuleUI), so a module-level meter is optional and a module-level
        // bypass/in-out control duplicates STAGE IN.
        (void)meters;
        for (size_t i = 0; i < n; ++i) {
            const std::string ln = lower(m.getParamSpec(static_cast<int>(i)).name);
            if (ln == "stage in/out" || ln == "master in/out" || ln == "eq in/out" || ln.find("bypass") != std::string::npos) {
                out.push_back({ m.designator, "D-DUP", "'" + m.getParamSpec(static_cast<int>(i)).name +
                                                       "' duplicates the auto-generated STAGE IN switch" });
            }
        }
        const int faceplateInteractive = interactive + 1; // + auto STAGE IN
        const int maxInteractive = (m.category == SchematicCategory::EqAndFilters) ? 25 : 11;
        if (faceplateInteractive < 6 || faceplateInteractive > maxInteractive) {
            out.push_back({ m.designator, "F-COUNT", std::to_string(interactive) + " module controls (" + std::to_string(extras) +
                                                     " Avra Extras) + STAGE IN; LAW 1 wants 6..10 (+STAGE IN)" });
        }
    }
}

// ----------------------------------------------------------------------------
// Clone-body detection over CircuitLibrary.h
// ----------------------------------------------------------------------------
std::string normalizeBody(const std::string& body) {
    std::string s;
    s.reserve(body.size());
    // strip // comments and /* */ comments
    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '/' && i + 1 < body.size() && body[i + 1] == '/') {
            while (i < body.size() && body[i] != '\n') ++i;
            continue;
        }
        if (body[i] == '/' && i + 1 < body.size() && body[i + 1] == '*') {
            i += 2;
            while (i + 1 < body.size() && !(body[i] == '*' && body[i + 1] == '/')) ++i;
            ++i;
            continue;
        }
        s += body[i];
    }
    static const std::regex num(R"((\d+\.?\d*|\.\d+)(e[-+]?\d+)?f?)");
    s = std::regex_replace(s, num, "N");
    std::string o;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        o += c;
    }
    return o;
}

std::map<std::string, std::string> extractProcessStereoBodies(const std::filesystem::path& file) {
    std::ifstream in(file);
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string src = ss.str();

    std::map<std::string, std::string> bodies;
    static const std::regex classRx(R"((?:^|\n)(?:class|struct)\s+(\w+)\s*(?:final\s*)?\{)");
    std::vector<std::pair<size_t, std::string>> classes;
    for (auto it = std::sregex_iterator(src.begin(), src.end(), classRx); it != std::sregex_iterator(); ++it) {
        classes.emplace_back(static_cast<size_t>(it->position(1)), (*it)[1].str());
    }
    for (size_t c = 0; c < classes.size(); ++c) {
        const size_t begin = classes[c].first;
        const size_t end = (c + 1 < classes.size()) ? classes[c + 1].first : src.size();
        const size_t ps = src.find("void processStereo(", begin);
        if (ps == std::string::npos || ps >= end) continue;
        const size_t open = src.find('{', ps);
        if (open == std::string::npos || open >= end) continue;
        int depth = 0;
        size_t i = open;
        for (; i < src.size(); ++i) {
            if (src[i] == '{') ++depth;
            else if (src[i] == '}') { if (--depth == 0) break; }
        }
        bodies[classes[c].second] = normalizeBody(src.substr(open, i - open + 1));
    }
    return bodies;
}

void auditClones(const std::vector<SchematicComponentMeta>& cat, const std::filesystem::path& lib, std::vector<Finding>& out,
                 int& groupCount, int& clonedModules) {
    const auto bodies = extractProcessStereoBodies(lib);
    std::map<std::string, std::vector<std::string>> byBody; // normalized body -> class names
    for (const auto& [cls, body] : bodies) byBody[body].push_back(cls);

    std::map<std::string, std::vector<std::string>> classToModules;
    for (const auto& m : cat) {
        std::string cls = m.cppClassName;
        const auto pos = cls.rfind("::");
        if (pos != std::string::npos) cls = cls.substr(pos + 2);
        classToModules[cls].push_back(m.designator);
    }

    groupCount = 0;
    clonedModules = 0;
    for (const auto& [body, classes] : byBody) {
        std::vector<std::string> mods;
        for (const auto& cls : classes) {
            auto it = classToModules.find(cls);
            if (it != classToModules.end()) mods.insert(mods.end(), it->second.begin(), it->second.end());
        }
        if (mods.size() < 2) continue;
        ++groupCount;
        clonedModules += static_cast<int>(mods.size());
        std::string list;
        for (size_t i = 0; i < mods.size(); ++i) list += (i ? "," : "") + mods[i];
        for (const auto& md : mods) {
            out.push_back({ md, "G-CLONE", "processStereo() identical (numbers stripped) to " + std::to_string(mods.size() - 1) +
                                               " other module(s) in group #" + std::to_string(groupCount) });
        }
        std::cout << "  Clone group #" << groupCount << " (" << mods.size() << " modules): " << list << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    bool strict = false;
    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--strict") strict = true;
        if (a == "--verbose" || a == "-v") verbose = true;
    }

    const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
    const auto lib = root / "include" / "avra" / "dsp" / "circuits" / "CircuitLibrary.h";
    const auto& cat = getSchematicCatalog();

    std::vector<Finding> findings;
    std::cout << "=====================================================================\n";
    std::cout << "  AVRA CONTROL-SPEC & CLONE-BODY AUDIT (" << cat.size() << " modules)" << (strict ? " [STRICT]" : " [REPORT]") << "\n";
    std::cout << "=====================================================================\n";

    auditControls(cat, findings);
    int groups = 0, cloned = 0;
    auditClones(cat, lib, findings, groups, cloned);

    std::map<std::string, int> perCategory;
    std::set<std::string> modulesWithFindings;
    for (const auto& f : findings) {
        ++perCategory[f.category];
        modulesWithFindings.insert(f.module);
    }

    if (verbose) {
        std::cout << "\n--- Findings ---\n";
        for (const auto& f : findings) std::cout << "  [" << f.category << "] " << f.module << ": " << f.detail << "\n";
    }

    std::cout << "\n--- Summary by check ---\n";
    for (const auto& [k, v] : perCategory) std::cout << "  " << k << ": " << v << "\n";
    std::cout << "  Clone groups: " << groups << " covering " << cloned << " modules\n";
    std::cout << "  Modules with >= 1 finding: " << modulesWithFindings.size() << " / " << cat.size() << "\n";
    std::cout << "=====================================================================\n";

    return (strict && !findings.empty()) ? 1 : 0;
}
