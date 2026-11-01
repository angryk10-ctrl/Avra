#pragma once

/**
 * @file Vst3Plugin.h
 * @brief Zero-dependency, self-contained C++20 VST3 (3.7.x C-ABI COM) plugin wrapper for Avra.
 *
 * Allows any Avra Processor + Direct2D Editor to compile directly into a native `.vst3` binary
 * without downloading external Steinberg SDK packages or JUCE.
 */

#include "avra/Avra.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

#if defined(_WIN32)
    #define AVRA_VST3_API __stdcall
    #define AVRA_VST3_EXPORT extern "C" __declspec(dllexport)
#else
    #define AVRA_VST3_API
    #define AVRA_VST3_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace avra::vst3_bridge {

// =============================================================================
// Minimal Standard VST3 3.7.9 COM C-ABI Types & InterfaceIIDs
// =============================================================================

using int8    = int8_t;
using uint8   = uint8_t;
using int16   = int16_t;
using uint16  = uint16_t;
using int32   = int32_t;
using uint32  = uint32_t;
using int64   = int64_t;
using uint64  = uint64_t;
using tresult = int32;
using TBool   = uint8;
using char8   = char;
using char16  = char16_t;
using TUID    = uint8[16];
using ParamID = uint32;
using ParamValue = double;
using SpeakerArrangement = uint64;

enum : tresult {
    kNoInterface     = static_cast<tresult>(0x80004002L),
    kResultOk        = 0,
    kResultTrue      = kResultOk,
    kResultFalse     = 1,
    kInvalidArgument = 2,
    kNotImplemented  = 3,
    kInternalError   = 4
};

constexpr SpeakerArrangement kSpeakerArrMono   = 0x4ULL; // Mono (Center)
constexpr SpeakerArrangement kSpeakerArrStereo = 0x3ULL; // Left | Right

// =============================================================================
// Steinberg VST3 COM_COMPATIBLE TUID Byte Layout
// On Windows (_WIN32), Steinberg's VST3 ABI requires COM_COMPATIBLE = 1
// (Microsoft COM GUID layout: Data1 little-endian uint32, Data2/Data3 little-endian
// uint16s packed in l2, and Data4 big-endian 8 bytes in l3..l4).
// =============================================================================
#if defined(_WIN32)
#define AVRA_VST3_INLINE_UID(l1, l2, l3, l4) \
{ \
    static_cast<uint8>((static_cast<uint32>(l1) & 0x000000FFu)      ), \
    static_cast<uint8>((static_cast<uint32>(l1) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l1) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l1) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0x000000FFu)      ), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0x000000FFu)      ), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0x000000FFu)      )  \
}
#else
#define AVRA_VST3_INLINE_UID(l1, l2, l3, l4) \
{ \
    static_cast<uint8>((static_cast<uint32>(l1) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l1) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l1) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l1) & 0x000000FFu)      ), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l2) & 0x000000FFu)      ), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l3) & 0x000000FFu)      ), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0xFF000000u) >> 24), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0x00FF0000u) >> 16), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0x0000FF00u) >>  8), \
    static_cast<uint8>((static_cast<uint32>(l4) & 0x000000FFu)      )  \
}
#endif

#define AVRA_DECLARE_UID(name, l1, l2, l3, l4) \
    constexpr TUID name = AVRA_VST3_INLINE_UID(l1, l2, l3, l4)

constexpr void makeTuid(TUID out, uint32_t l1, uint32_t l2, uint32_t l3, uint32_t l4) noexcept {
    const TUID tmp = AVRA_VST3_INLINE_UID(l1, l2, l3, l4);
    for (size_t i = 0; i < 16; ++i) {
        out[i] = tmp[i];
    }
}

// Helper to compare two 16-byte VST3 TUIDs (accepts both COM_COMPATIBLE little-endian GUID header and big-endian 16-byte layout)
inline bool tuidMatches(const TUID a, const TUID b) noexcept {
    if (std::memcmp(a, b, 16) == 0) return true;
    return a[0] == b[3] && a[1] == b[2] && a[2] == b[1] && a[3] == b[0] &&
           a[4] == b[5] && a[5] == b[4] && a[6] == b[7] && a[7] == b[6] &&
           std::memcmp(a + 8, b + 8, 8) == 0;
}

inline bool iidEqual(const TUID a, const TUID b) noexcept {
    return tuidMatches(a, b);
}

// Helper to generate a deterministic 16-byte COM_COMPATIBLE TUID from a plugin ID string and salt
inline void deriveTuidFromString(TUID outTuid, const char* str, uint32_t salt) noexcept {
    uint64_t h1 = 0xCBF29CE484222325ULL ^ static_cast<uint64_t>(salt);
    uint64_t h2 = 0x100000001B3ULL + static_cast<uint64_t>(salt) * 0x9E3779B97F4A7C15ULL;
    if (str) {
        for (size_t i = 0; str[i] != '\0'; ++i) {
            const uint8_t c = static_cast<uint8_t>(str[i]);
            h1 ^= c;
            h1 *= 0x100000001B3ULL;
            h2 ^= (c + i);
            h2 *= 0x9E3779B97F4A7C15ULL;
        }
    }
    const uint32_t l1 = static_cast<uint32_t>(h1 >> 32);
    const uint32_t l2 = (static_cast<uint32_t>(h1 & 0xFFFFFFFFu) & 0xFFFF0FFFu) | 0x00004000u; // RFC 4122 v4
    const uint32_t l3 = (static_cast<uint32_t>(h2 >> 32) & 0x3FFFFFFFu) | 0x80000000u;         // RFC 4122 variant
    const uint32_t l4 = static_cast<uint32_t>(h2 & 0xFFFFFFFFu);
    makeTuid(outTuid, l1, l2, l3, l4);
}

inline void copyAsciiToChar8(char8* dst, size_t maxCount, const char* src) noexcept {
    if (!dst || maxCount == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t i = 0;
    while (i + 1 < maxCount && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

inline void copyAsciiToUtf16(char16* dst, size_t maxChars, const char* src) noexcept {
    if (!dst || maxChars == 0) return;
    if (!src) { dst[0] = 0; return; }
    size_t i = 0;
    while (i + 1 < maxChars && src[i] != '\0') {
        dst[i] = static_cast<char16>(static_cast<unsigned char>(src[i]));
        ++i;
    }
    dst[i] = 0;
}

inline std::string utf16ToAscii(const char16* src, size_t maxChars = 128) {
    std::string out;
    if (!src) return out;
    for (size_t i = 0; i < maxChars && src[i] != 0; ++i) {
        out.push_back(static_cast<char>(src[i] & 0x7F));
    }
    return out;
}

// Official Steinberg VST3 Interface IIDs (COM_COMPATIBLE = 1 on Windows, Big-Endian on POSIX)
AVRA_DECLARE_UID(kFUnknownIid,                     0x00000000u, 0x00000000u, 0xC0000000u, 0x00000046u);
AVRA_DECLARE_UID(kIBStreamIid,                     0xC3BF6EA2u, 0x30994752u, 0x9B6BF990u, 0x1EE33E9Bu);
AVRA_DECLARE_UID(kIPluginBaseIid,                  0x22888DDBu, 0x156E45AEu, 0x8358B348u, 0x08190625u);
AVRA_DECLARE_UID(kIPluginFactoryIid,               0x7A4D811Cu, 0x52114A1Fu, 0xAED9D2EEu, 0x0B43BF9Fu);
AVRA_DECLARE_UID(kIPluginFactory2Iid,              0x0007B650u, 0xF24B4C0Bu, 0xA464EDB9u, 0xF00B2ABBu);
AVRA_DECLARE_UID(kIPluginFactory3Iid,              0x4555A2ABu, 0xC1234E57u, 0x9B122910u, 0x36878931u);
AVRA_DECLARE_UID(kIComponentIid,                   0xE831FF31u, 0xF2D54301u, 0x928EBBEEu, 0x25697802u);
AVRA_DECLARE_UID(kIAudioProcessorIid,              0x42043F99u, 0xB7DA453Cu, 0xA569E79Du, 0x9AAEC33Du);
AVRA_DECLARE_UID(kIProcessContextRequirementsIid,  0x2A654303u, 0xEF764E3Du, 0x95B5FE83u, 0x730EF6D0u);
AVRA_DECLARE_UID(kIEditControllerIid,              0xDCD7BBE3u, 0x7742448Du, 0xA874AACCu, 0x979C759Eu);
AVRA_DECLARE_UID(kIComponentHandlerIid,            0x93A0BEA3u, 0x0BD045DBu, 0x8E890B0Cu, 0xC1E46AC6u);
AVRA_DECLARE_UID(kIPlugViewIid,                    0x5BC32507u, 0xD06049EAu, 0xA6151B52u, 0x2B755B29u);
AVRA_DECLARE_UID(kIPlugFrameIid,                   0x367FAF01u, 0xAFA94693u, 0x8D4DA2A0u, 0xED0882A3u);
AVRA_DECLARE_UID(kIParamValueQueueIid,             0x01263A18u, 0xED074F6Fu, 0x98C9D356u, 0x4686F9BAu);
AVRA_DECLARE_UID(kIParameterChangesIid,            0xA4779663u, 0x0BB64A56u, 0xB44384A8u, 0x466FEB9Du);
AVRA_DECLARE_UID(kIUnitInfoIid,                    0x3D4BD6B5u, 0x913A4FD2u, 0xA886E768u, 0xA5EB92C1u);

// =============================================================================
// Core COM Interfaces
// =============================================================================

struct FUnknown {
    virtual tresult AVRA_VST3_API queryInterface(const TUID _iid, void** obj) = 0;
    virtual uint32  AVRA_VST3_API addRef() = 0;
    virtual uint32  AVRA_VST3_API release() = 0;
};

struct IBStream : public FUnknown {
    virtual tresult AVRA_VST3_API read(void* buffer, int32 numBytes, int32* numBytesRead) = 0;
    virtual tresult AVRA_VST3_API write(void* buffer, int32 numBytes, int32* numBytesWritten) = 0;
    virtual tresult AVRA_VST3_API seek(int64 pos, int32 mode, int64* result) = 0;
    virtual tresult AVRA_VST3_API tell(int64* pos) = 0;
};

struct IPluginBase : public FUnknown {
    virtual tresult AVRA_VST3_API initialize(FUnknown* context) = 0;
    virtual tresult AVRA_VST3_API terminate() = 0;
};

#if defined(_WIN64) || defined(__LP64__)
#pragma pack(push, 16)
#else
#pragma pack(push, 8)
#endif

struct PFactoryInfo {
    enum { kURLSize = 256, kEmailSize = 128, kNameSize = 64, kUnicode = 1 << 4 };
    char8 vendor[kNameSize];
    char8 url[kURLSize];
    char8 email[kEmailSize];
    int32 flags;
};

struct PClassInfo {
    enum { kCategorySize = 32, kNameSize = 64, kManyInstances = 0x7FFFFFFF };
    TUID cid;
    int32 cardinality;
    char8 category[kCategorySize];
    char8 name[kNameSize];
};

struct PClassInfo2 {
    TUID cid;
    int32 cardinality;
    char8 category[32];
    char8 name[64];
    uint32 classFlags;
    char8 subCategories[128];
    char8 vendor[64];
    char8 version[64];
    char8 sdkVersion[64];
};

struct PClassInfoW {
    TUID cid;
    int32 cardinality;
    char8 category[32];
    char16 name[64];
    uint32 classFlags;
    char8 subCategories[128];
    char16 vendor[64];
    char16 version[64];
    char16 sdkVersion[64];
};

struct BusInfo {
    enum MediaType { kAudio = 0, kEvent = 1 };
    enum BusDirection { kInput = 0, kOutput = 1 };
    enum BusType { kMain = 0, kAux = 1 };
    enum BusFlags { kDefaultActive = 1 << 0 };
    int32 mediaType;
    int32 direction;
    int32 channelCount;
    char16 name[128];
    int32 busType;
    uint32 flags;
};

struct RoutingInfo {
    int32 mediaType;
    int32 busIndex;
    int32 channel;
};

struct ProcessSetup {
    int32 processMode;
    int32 symbolicSampleSize; // 0 = kSample32, 1 = kSample64
    int32 maxSamplesPerBlock;
    double sampleRate;
};

struct AudioBusBuffers {
    int32 numChannels;
    uint64 silenceFlags;
    union {
        float** channelBuffers32;
        double** channelBuffers64;
    };
};

struct FrameRate {
    uint32 framesPerSecond;
    uint32 flags;
};

struct Chord {
    uint8 keyNote;
    uint8 rootNote;
    int16 chordMask;
};

struct VstProcessContext {
    enum StatesAndFlags {
        kPlaying          = 1 << 1,
        kCycleActive      = 1 << 2,
        kRecording        = 1 << 3,
        kProjectTimeValid = 1 << 9,
        kTempoValid       = 1 << 10,
        kTimeSigValid     = 1 << 13
    };
    uint32 state;
    double sampleRate;
    int64 projectTimeSamples;
    int64 systemTime;
    int64 continousTimeSamples;
    double projectTimeMusic;
    double barPositionMusic;
    double cycleStartMusic;
    double cycleEndMusic;
    double tempo;
    int32 timeSigNumerator;
    int32 timeSigDenominator;
    Chord chord;
    int32 smpteOffsetSubframes;
    FrameRate frameRate;
    int32 samplesToNextClock;
};

struct IParamValueQueue : public FUnknown {
    virtual ParamID AVRA_VST3_API getParameterId() = 0;
    virtual int32   AVRA_VST3_API getPointCount() = 0;
    virtual tresult AVRA_VST3_API getPoint(int32 index, int32& sampleOffset, ParamValue& value) = 0;
    virtual tresult AVRA_VST3_API addPoint(int32 sampleOffset, ParamValue value, int32& index) = 0;
};

struct IParameterChanges : public FUnknown {
    virtual int32             AVRA_VST3_API getParameterCount() = 0;
    virtual IParamValueQueue* AVRA_VST3_API getParameterData(int32 index) = 0;
    virtual IParamValueQueue* AVRA_VST3_API addParameterData(const ParamID& id, int32& index) = 0;
};

struct ProcessData {
    int32 processMode;
    int32 symbolicSampleSize;
    int32 numSamples;
    int32 numInputs;
    int32 numOutputs;
    AudioBusBuffers* inputs;
    AudioBusBuffers* outputs;
    IParameterChanges* inputParameterChanges;
    IParameterChanges* outputParameterChanges;
    void* inputEvents;
    void* outputEvents;
    VstProcessContext* processContext;
};

struct ParameterInfo {
    enum ParameterFlags {
        kCanAutomate     = 1 << 0,
        kIsReadOnly      = 1 << 1,
        kIsWrapAround    = 1 << 2,
        kIsList          = 1 << 3,
        kIsProgramChange = 1 << 15,
        kIsBypass        = 1 << 16
    };
    ParamID id;
    char16 title[128];
    char16 shortTitle[128];
    char16 units[128];
    int32 stepCount;
    ParamValue defaultNormalizedValue;
    int32 unitId;
    int32 flags;
};

struct UnitInfo {
    int32 id;
    int32 parentUnitId;
    char16 name[128];
    int32 programListId;
};

struct ProgramListInfo {
    int32 id;
    char16 name[128];
    int32 programCount;
};

struct ViewRect {
    int32 left{0};
    int32 top{0};
    int32 right{0};
    int32 bottom{0};
    [[nodiscard]] int32 getWidth() const noexcept { return right - left; }
    [[nodiscard]] int32 getHeight() const noexcept { return bottom - top; }
};

#pragma pack(pop)

struct IPlugFrame : public FUnknown {
    virtual tresult AVRA_VST3_API resizeView(struct IPlugView* view, ViewRect* newSize) = 0;
};

struct IPlugView : public FUnknown {
    virtual tresult AVRA_VST3_API isPlatformTypeSupported(const char* type) = 0;
    virtual tresult AVRA_VST3_API attached(void* parent, const char* type) = 0;
    virtual tresult AVRA_VST3_API removed() = 0;
    virtual tresult AVRA_VST3_API onWheel(float distance) = 0;
    virtual tresult AVRA_VST3_API onKeyDown(char16 key, int16 keyCode, int16 modifiers) = 0;
    virtual tresult AVRA_VST3_API onKeyUp(char16 key, int16 keyCode, int16 modifiers) = 0;
    virtual tresult AVRA_VST3_API getSize(ViewRect* size) = 0;
    virtual tresult AVRA_VST3_API onSize(ViewRect* newSize) = 0;
    virtual tresult AVRA_VST3_API onFocus(TBool state) = 0;
    virtual tresult AVRA_VST3_API setFrame(IPlugFrame* frame) = 0;
    virtual tresult AVRA_VST3_API canResize() = 0;
    virtual tresult AVRA_VST3_API checkSizeConstraint(ViewRect* rect) = 0;
};

struct IComponentHandler : public FUnknown {
    virtual tresult AVRA_VST3_API beginEdit(ParamID id) = 0;
    virtual tresult AVRA_VST3_API performEdit(ParamID id, ParamValue valueNormalized) = 0;
    virtual tresult AVRA_VST3_API endEdit(ParamID id) = 0;
    virtual tresult AVRA_VST3_API restartComponent(int32 flags) = 0;
};

struct IComponent : public IPluginBase {
    virtual tresult AVRA_VST3_API getControllerClassId(TUID classId) = 0;
    virtual tresult AVRA_VST3_API setIoMode(int32 mode) = 0;
    virtual int32   AVRA_VST3_API getBusCount(int32 type, int32 dir) = 0;
    virtual tresult AVRA_VST3_API getBusInfo(int32 type, int32 dir, int32 index, BusInfo& bus) = 0;
    virtual tresult AVRA_VST3_API getRoutingInfo(RoutingInfo& inInfo, RoutingInfo& outInfo) = 0;
    virtual tresult AVRA_VST3_API activateBus(int32 type, int32 dir, int32 index, TBool state) = 0;
    virtual tresult AVRA_VST3_API setActive(TBool state) = 0;
    virtual tresult AVRA_VST3_API setState(IBStream* state) = 0;
    virtual tresult AVRA_VST3_API getState(IBStream* state) = 0;
};

struct IAudioProcessor : public FUnknown {
    virtual tresult AVRA_VST3_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) = 0;
    virtual tresult AVRA_VST3_API getBusArrangement(int32 dir, int32 index, SpeakerArrangement& arr) = 0;
    virtual tresult AVRA_VST3_API canProcessSampleSize(int32 symbolicSampleSize) = 0;
    virtual uint32  AVRA_VST3_API getLatencySamples() = 0;
    virtual tresult AVRA_VST3_API setupProcessing(ProcessSetup& setup) = 0;
    virtual tresult AVRA_VST3_API setProcessing(TBool state) = 0;
    virtual tresult AVRA_VST3_API process(ProcessData& data) = 0;
    virtual uint32  AVRA_VST3_API getTailSamples() = 0;
};

struct IProcessContextRequirements : public FUnknown {
    enum Flags : uint32 {
        kNeedSystemTime           = 1 << 0,
        kNeedContinousTimeSamples = 1 << 1,
        kNeedProjectTimeMusic     = 1 << 2,
        kNeedBarPositionMusic     = 1 << 3,
        kNeedCycleMusic           = 1 << 4,
        kNeedSamplesToNextClock   = 1 << 5,
        kNeedTempo                = 1 << 6,
        kNeedTimeSignature        = 1 << 7,
        kNeedChord                = 1 << 8,
        kNeedFrameRate            = 1 << 9,
        kNeedTransportState       = 1 << 10
    };
    virtual uint32 AVRA_VST3_API getProcessContextRequirements() = 0;
};

struct IEditController : public IPluginBase {
    virtual tresult    AVRA_VST3_API setComponentState(IBStream* state) = 0;
    virtual tresult    AVRA_VST3_API setState(IBStream* state) = 0;
    virtual tresult    AVRA_VST3_API getState(IBStream* state) = 0;
    virtual int32      AVRA_VST3_API getParameterCount() = 0;
    virtual tresult    AVRA_VST3_API getParameterInfo(int32 paramIndex, ParameterInfo& info) = 0;
    virtual tresult    AVRA_VST3_API getParamStringByValue(ParamID id, ParamValue valueNormalized, char16* string) = 0;
    virtual tresult    AVRA_VST3_API getParamValueByString(ParamID id, char16* string, ParamValue& valueNormalized) = 0;
    virtual ParamValue AVRA_VST3_API normalizedParamToPlain(ParamID id, ParamValue valueNormalized) = 0;
    virtual ParamValue AVRA_VST3_API plainParamToNormalized(ParamID id, ParamValue plainValue) = 0;
    virtual ParamValue AVRA_VST3_API getParamNormalized(ParamID id) = 0;
    virtual tresult    AVRA_VST3_API setParamNormalized(ParamID id, ParamValue value) = 0;
    virtual tresult    AVRA_VST3_API setComponentHandler(IComponentHandler* handler) = 0;
    virtual IPlugView* AVRA_VST3_API createView(const char* name) = 0;
};

struct IUnitInfo : public FUnknown {
    virtual int32   AVRA_VST3_API getUnitCount() = 0;
    virtual tresult AVRA_VST3_API getUnitInfo(int32 unitIndex, UnitInfo& info) = 0;
    virtual int32   AVRA_VST3_API getProgramListCount() = 0;
    virtual tresult AVRA_VST3_API getProgramListInfo(int32 listIndex, ProgramListInfo& info) = 0;
    virtual tresult AVRA_VST3_API getProgramName(int32 listId, int32 programIndex, char16 name[128]) = 0;
    virtual tresult AVRA_VST3_API getProgramInfo(int32 listId, int32 programIndex, const char* attributeId, char16 attributeValue[128]) = 0;
    virtual tresult AVRA_VST3_API hasProgramPitchNames(int32 listId, int32 programIndex) = 0;
    virtual tresult AVRA_VST3_API getProgramPitchName(int32 listId, int32 programIndex, int16 midiPitch, char16 name[128]) = 0;
    virtual int32   AVRA_VST3_API getSelectedUnit() = 0;
    virtual tresult AVRA_VST3_API selectUnit(int32 unitId) = 0;
    virtual tresult AVRA_VST3_API getUnitByBus(int32 type, int32 dir, int32 busIndex, int32 channel, int32& unitId) = 0;
    virtual tresult AVRA_VST3_API setUnitProgramData(int32 listOrUnitId, int32 programIndex, IBStream* data) = 0;
};

struct IPluginFactory3 : public FUnknown {
    virtual tresult AVRA_VST3_API getFactoryInfo(PFactoryInfo* info) = 0;
    virtual int32   AVRA_VST3_API countClasses() = 0;
    virtual tresult AVRA_VST3_API getClassInfo(int32 index, PClassInfo* info) = 0;
    virtual tresult AVRA_VST3_API createInstance(const char* cid, const char* _iid, void** obj) = 0;
    virtual tresult AVRA_VST3_API getClassInfo2(int32 index, PClassInfo2* info) = 0;
    virtual tresult AVRA_VST3_API getClassInfoUnicode(int32 index, PClassInfoW* info) = 0;
    virtual tresult AVRA_VST3_API setHostContext(FUnknown* context) = 0;
};

// =============================================================================
// Direct2D Win32 IPlugView Implementation
// =============================================================================

template <typename PluginType>
class AvraVst3PlugView final : public IPlugView {
public:
    explicit AvraVst3PlugView(PluginType& processor)
        : processor_(processor) {
        editor_ = processor_.createEditor();
    }

    ~AvraVst3PlugView() {
        if (window_) {
            window_->destroy();
            window_.reset();
        }
        editor_.reset();
    }

    tresult AVRA_VST3_API queryInterface(const TUID _iid, void** obj) override {
        if (!obj) return kInvalidArgument;
        if (iidEqual(_iid, kFUnknownIid) || iidEqual(_iid, kIPlugViewIid)) {
            addRef();
            *obj = static_cast<IPlugView*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 AVRA_VST3_API addRef() override {
        return ++refCount_;
    }

    uint32 AVRA_VST3_API release() override {
        const uint32 r = --refCount_;
        if (r == 0) delete this;
        return r;
    }

    tresult AVRA_VST3_API isPlatformTypeSupported(const char* type) override {
#if defined(_WIN32)
        if (type && std::strcmp(type, "HWND") == 0) return kResultTrue;
#else
        (void)type;
#endif
        return kResultFalse;
    }

    tresult AVRA_VST3_API attached(void* parent, const char* type) override {
        if (isPlatformTypeSupported(type) != kResultTrue || !parent || !editor_) {
            return kInvalidArgument;
        }
#if defined(_WIN32)
        if (!window_) {
            window_ = std::make_unique<gui::Window>(editor_.get());
        }
        HWND parentHwnd = reinterpret_cast<HWND>(parent);
        if (window_->create(parentHwnd, editor_->getDefaultWidth(), editor_->getDefaultHeight())) {
            window_->show();
            return kResultOk;
        }
#endif
        return kInternalError;
    }

    tresult AVRA_VST3_API removed() override {
#if defined(_WIN32)
        if (window_) {
            window_->destroy();
            window_.reset();
        }
#endif
        return kResultOk;
    }

    tresult AVRA_VST3_API onWheel(float) override { return kResultFalse; }
    tresult AVRA_VST3_API onKeyDown(char16, int16, int16) override { return kResultFalse; }
    tresult AVRA_VST3_API onKeyUp(char16, int16, int16) override { return kResultFalse; }

    tresult AVRA_VST3_API getSize(ViewRect* size) override {
        if (!size || !editor_) return kInvalidArgument;
        size->left   = 0;
        size->top    = 0;
        size->right  = static_cast<int32>(editor_->getDefaultWidth());
        size->bottom = static_cast<int32>(editor_->getDefaultHeight());
        return kResultOk;
    }

    tresult AVRA_VST3_API onSize(ViewRect* newSize) override {
        if (!newSize || !window_) return kInvalidArgument;
        window_->setSize(newSize->getWidth(), newSize->getHeight());
        return kResultOk;
    }

    tresult AVRA_VST3_API onFocus(TBool) override { return kResultOk; }
    tresult AVRA_VST3_API setFrame(IPlugFrame* frame) override { frame_ = frame; return kResultOk; }
    tresult AVRA_VST3_API canResize() override { return kResultFalse; }
    tresult AVRA_VST3_API checkSizeConstraint(ViewRect* rect) override { return getSize(rect); }

private:
    std::atomic<uint32> refCount_{1};
    PluginType& processor_;
    std::unique_ptr<Editor> editor_;
    std::unique_ptr<gui::Window> window_;
    IPlugFrame* frame_{nullptr};
};

// =============================================================================
// Unified Single-Component VST3 Processor + EditController
// =============================================================================

template <typename PluginType>
class AvraVst3SingleComponent final
    : public IComponent
    , public IAudioProcessor
    , public IProcessContextRequirements
    , public IEditController
    , public IUnitInfo {
public:
    explicit AvraVst3SingleComponent(const TUID controllerCid) {
        std::memcpy(controllerCid_, controllerCid, 16);
        processor_ = std::make_unique<PluginType>();
        inputPtrs_.resize(16, nullptr);
        outputPtrs_.resize(16, nullptr);
    }

    ~AvraVst3SingleComponent() = default;

    // --- FUnknown ---
    tresult AVRA_VST3_API queryInterface(const TUID _iid, void** obj) override {
        if (!obj) return kInvalidArgument;
        if (iidEqual(_iid, kFUnknownIid) || iidEqual(_iid, kIPluginBaseIid) || iidEqual(_iid, kIComponentIid)) {
            addRef();
            *obj = static_cast<IComponent*>(this);
            return kResultOk;
        }
        if (iidEqual(_iid, kIAudioProcessorIid)) {
            addRef();
            *obj = static_cast<IAudioProcessor*>(this);
            return kResultOk;
        }
        if (iidEqual(_iid, kIProcessContextRequirementsIid)) {
            addRef();
            *obj = static_cast<IProcessContextRequirements*>(this);
            return kResultOk;
        }
        if (iidEqual(_iid, kIEditControllerIid)) {
            addRef();
            *obj = static_cast<IEditController*>(this);
            return kResultOk;
        }
        if (iidEqual(_iid, kIUnitInfoIid)) {
            addRef();
            *obj = static_cast<IUnitInfo*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 AVRA_VST3_API addRef() override {
        return ++refCount_;
    }

    uint32 AVRA_VST3_API release() override {
        const uint32 r = --refCount_;
        if (r == 0) delete this;
        return r;
    }

    // --- IPluginBase ---
    tresult AVRA_VST3_API initialize(FUnknown*) override {
        return kResultOk;
    }

    tresult AVRA_VST3_API terminate() override {
        return kResultOk;
    }

    // --- IComponent ---
    tresult AVRA_VST3_API getControllerClassId(TUID classId) override {
        if (classId) {
            std::memset(classId, 0, 16);
        }
        // SingleComponentEffect: return kResultFalse (1) so the host queries IEditController
        // directly from this IComponent instance via queryInterface(kIEditControllerIid).
        return kResultFalse;
    }

    tresult AVRA_VST3_API setIoMode(int32) override {
        return kResultOk;
    }

    int32 AVRA_VST3_API getBusCount(int32 type, int32 dir) override {
        (void)dir;
        return (type == BusInfo::kAudio) ? 1 : 0;
    }

    tresult AVRA_VST3_API getBusInfo(int32 type, int32 dir, int32 index, BusInfo& bus) override {
        if (type != BusInfo::kAudio || index != 0) return kInvalidArgument;
        bus.mediaType    = BusInfo::kAudio;
        bus.direction    = dir;
        bus.channelCount = (busArrangement_ == kSpeakerArrMono) ? 1 : 2;
        copyAsciiToUtf16(bus.name, 128, (dir == BusInfo::kInput) ? "Stereo In" : "Stereo Out");
        bus.busType      = BusInfo::kMain;
        bus.flags        = BusInfo::kDefaultActive;
        return kResultOk;
    }

    tresult AVRA_VST3_API getRoutingInfo(RoutingInfo&, RoutingInfo&) override {
        return kNotImplemented;
    }

    tresult AVRA_VST3_API activateBus(int32, int32, int32, TBool) override {
        return kResultOk;
    }

    tresult AVRA_VST3_API setActive(TBool state) override {
        if (!state && processor_) {
            processor_->reset();
        }
        return kResultOk;
    }

    tresult AVRA_VST3_API setState(IBStream* state) override {
        if (!state || !processor_) return kInvalidArgument;
        std::string serialized;
        char buf[512];
        int32 bytesRead = 0;
        while (state->read(buf, sizeof(buf), &bytesRead) == kResultOk && bytesRead > 0) {
            serialized.append(buf, static_cast<size_t>(bytesRead));
        }
        if (serialized.empty()) return kResultOk;
        return processor_->getParameters().restoreState(serialized) ? kResultOk : kResultOk;
    }

    tresult AVRA_VST3_API getState(IBStream* state) override {
        if (!state || !processor_) return kInvalidArgument;
        std::string serialized = processor_->getParameters().serializeState();
        int32 written = 0;
        return state->write(serialized.data(), static_cast<int32>(serialized.size()), &written);
    }

    // --- IAudioProcessor ---
    tresult AVRA_VST3_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) override {
        if (numIns == 1 && numOuts == 1 && inputs && outputs) {
            if (inputs[0] == kSpeakerArrStereo && outputs[0] == kSpeakerArrStereo) {
                busArrangement_ = kSpeakerArrStereo;
                return kResultTrue;
            }
            if (inputs[0] == kSpeakerArrMono && outputs[0] == kSpeakerArrMono) {
                busArrangement_ = kSpeakerArrMono;
                return kResultTrue;
            }
        }
        return kResultFalse;
    }

    tresult AVRA_VST3_API getBusArrangement(int32, int32 index, SpeakerArrangement& arr) override {
        if (index != 0) return kInvalidArgument;
        arr = busArrangement_;
        return kResultOk;
    }

    tresult AVRA_VST3_API canProcessSampleSize(int32 symbolicSampleSize) override {
        return (symbolicSampleSize == 0) ? kResultTrue : kResultFalse; // 0 == kSample32
    }

    uint32 AVRA_VST3_API getLatencySamples() override {
        return processor_ ? processor_->getLatencySamples() : 0;
    }

    tresult AVRA_VST3_API setupProcessing(ProcessSetup& setup) override {
        const double sr = (setup.sampleRate >= 8000.0 && setup.sampleRate <= 768000.0) ? setup.sampleRate : 48000.0;
        const size_t maxFrames = static_cast<size_t>(std::max<int32>(64, setup.maxSamplesPerBlock));
        processor_->prepare(sr, maxFrames);
        return kResultOk;
    }

    tresult AVRA_VST3_API setProcessing(TBool) override {
        return kResultOk;
    }

    [[nodiscard]] static bool isInvertedBypassParam(const Parameter* p) noexcept {
        return p && p->getName() == "Master In/Out";
    }

    tresult AVRA_VST3_API process(ProcessData& data) override {
        if (!processor_) return kInternalError;
        auto& params = processor_->getParameters();

        // 1. Apply incoming VST3 parameter automation queues
        if (data.inputParameterChanges) {
            const int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
            for (int32 i = 0; i < numParamsChanged; ++i) {
                if (IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i)) {
                    const int32 numPoints = queue->getPointCount();
                    if (numPoints > 0) {
                        int32 sampleOffset = 0;
                        ParamValue normVal = 0.0;
                        if (queue->getPoint(numPoints - 1, sampleOffset, normVal) == kResultOk) {
                            if (auto* p = params.getById(queue->getParameterId())) {
                                const float v = static_cast<float>(normVal);
                                p->setNormalized(isInvertedBypassParam(p) ? (1.0f - v) : v);
                            }
                        }
                    }
                }
            }
        }

        if (data.numSamples <= 0 || data.numOutputs == 0 || !data.outputs) {
            return kResultOk;
        }

        // 2. Map input and output channel pointers
        size_t inCh = 0;
        if (data.numInputs > 0 && data.inputs && data.inputs[0].channelBuffers32) {
            inCh = std::min<size_t>(static_cast<size_t>(data.inputs[0].numChannels), inputPtrs_.size());
            for (size_t c = 0; c < inCh; ++c) {
                inputPtrs_[c] = data.inputs[0].channelBuffers32[c];
            }
        }

        size_t outCh = 0;
        if (data.numOutputs > 0 && data.outputs && data.outputs[0].channelBuffers32) {
            outCh = std::min<size_t>(static_cast<size_t>(data.outputs[0].numChannels), outputPtrs_.size());
            for (size_t c = 0; c < outCh; ++c) {
                outputPtrs_[c] = data.outputs[0].channelBuffers32[c];
            }
        }

        TransportInfo transport{};
        if (data.processContext) {
            transport.isPlaying   = (data.processContext->state & VstProcessContext::kPlaying) != 0;
            transport.isRecording = (data.processContext->state & VstProcessContext::kRecording) != 0;
            transport.isLooping   = (data.processContext->state & VstProcessContext::kCycleActive) != 0;
            if (data.processContext->state & VstProcessContext::kTempoValid) {
                transport.bpm = data.processContext->tempo;
            }
            if (data.processContext->state & VstProcessContext::kTimeSigValid) {
                transport.timeSigNumerator   = static_cast<uint16_t>(data.processContext->timeSigNumerator);
                transport.timeSigDenominator = static_cast<uint16_t>(data.processContext->timeSigDenominator);
            }
        }

        ProcessContext ctx{
            .inputs     = AudioSpan<const float>(inputPtrs_.data(), inCh, static_cast<size_t>(data.numSamples)),
            .outputs    = AudioSpan<float>(outputPtrs_.data(), outCh, static_cast<size_t>(data.numSamples)),
            .transport  = transport,
            .sampleRate = processor_->getSampleRate(),
            .numFrames  = static_cast<size_t>(data.numSamples)
        };

        processor_->process(ctx);
        if (!avra::provenance::verifyIntegrity()) {
            ctx.outputs.clear();
        }
        return kResultOk;
    }

    uint32 AVRA_VST3_API getTailSamples() override {
        return 0;
    }

    // --- IProcessContextRequirements ---
    uint32 AVRA_VST3_API getProcessContextRequirements() override {
        return kNeedTempo | kNeedTimeSignature | kNeedTransportState;
    }

    // --- IEditController ---
    tresult AVRA_VST3_API setComponentState(IBStream* state) override {
        if (!state) return kResultOk;
        return setState(state);
    }

    int32 AVRA_VST3_API getParameterCount() override {
        return processor_ ? static_cast<int32>(processor_->getParameters().size()) : 0;
    }

    tresult AVRA_VST3_API getParameterInfo(int32 paramIndex, ParameterInfo& info) override {
        if (!processor_) return kInternalError;
        const auto* p = processor_->getParameters().getByIndex(static_cast<size_t>(paramIndex));
        if (!p) return kInvalidArgument;

        std::memset(&info, 0, sizeof(ParameterInfo));
        info.id = p->getId();
        copyAsciiToUtf16(info.title, 128, p->getName().c_str());
        copyAsciiToUtf16(info.shortTitle, 128, p->getName().c_str());
        copyAsciiToUtf16(info.units, 128, p->getUnit().c_str());
        info.stepCount = (p->getTaper() == ParameterTaper::Stepped) ? 1 : 0;
        const float rawDefaultNorm = p->plainToNormalized(p->getDefaultValue());
        info.defaultNormalizedValue = static_cast<ParamValue>(isInvertedBypassParam(p) ? (1.0f - rawDefaultNorm) : rawDefaultNorm);
        info.unitId = 0;
        info.flags = ParameterInfo::kCanAutomate;
        if (isInvertedBypassParam(p) || p->getName() == "Bypass") {
            info.flags |= ParameterInfo::kIsBypass;
        } else if (p->getName() == "Preset") {
            info.stepCount = std::max<int32>(1, processor_->getProgramCount() - 1);
            info.flags     = ParameterInfo::kCanAutomate | ParameterInfo::kIsProgramChange | ParameterInfo::kIsList;
        }
        return kResultOk;
    }

    tresult AVRA_VST3_API getParamStringByValue(ParamID id, ParamValue valueNormalized, char16* string) override {
        if (!processor_ || !string) return kInvalidArgument;
        const auto* p = processor_->getParameters().getById(id);
        if (!p) return kInvalidArgument;
        if (p->getName() == "Preset" && processor_->getProgramCount() > 0) {
            const int maxIdx = std::max(1, processor_->getProgramCount() - 1);
            const int idx    = std::clamp(static_cast<int>(std::lround(valueNormalized * maxIdx)), 0, maxIdx);
            copyAsciiToUtf16(string, 128, processor_->getProgramName(idx));
            return kResultOk;
        }
        const float plain = p->normalizedToPlain(static_cast<float>(valueNormalized));
        const std::string txt = p->getText(plain);
        copyAsciiToUtf16(string, 128, txt.c_str());
        return kResultOk;
    }

    tresult AVRA_VST3_API getParamValueByString(ParamID id, char16* string, ParamValue& valueNormalized) override {
        if (!processor_ || !string) return kInvalidArgument;
        const auto* p = processor_->getParameters().getById(id);
        if (!p) return kInvalidArgument;
        const std::string ascii = utf16ToAscii(string);
        char* end = nullptr;
        const double plain = std::strtod(ascii.c_str(), &end);
        if (end == ascii.c_str()) return kInvalidArgument;
        valueNormalized = static_cast<ParamValue>(p->plainToNormalized(static_cast<float>(plain)));
        return kResultOk;
    }

    ParamValue AVRA_VST3_API normalizedParamToPlain(ParamID id, ParamValue valueNormalized) override {
        if (!processor_) return valueNormalized;
        const auto* p = processor_->getParameters().getById(id);
        return p ? static_cast<ParamValue>(p->normalizedToPlain(static_cast<float>(valueNormalized))) : valueNormalized;
    }

    ParamValue AVRA_VST3_API plainParamToNormalized(ParamID id, ParamValue plainValue) override {
        if (!processor_) return plainValue;
        const auto* p = processor_->getParameters().getById(id);
        return p ? static_cast<ParamValue>(p->plainToNormalized(static_cast<float>(plainValue))) : plainValue;
    }

    ParamValue AVRA_VST3_API getParamNormalized(ParamID id) override {
        if (!processor_) return 0.0;
        const auto* p = processor_->getParameters().getById(id);
        if (!p) return 0.0;
        const float norm = p->getNormalized();
        return static_cast<ParamValue>(isInvertedBypassParam(p) ? (1.0f - norm) : norm);
    }

    tresult AVRA_VST3_API setParamNormalized(ParamID id, ParamValue value) override {
        if (!processor_) return kInvalidArgument;
        if (auto* p = processor_->getParameters().getById(id)) {
            const float v = static_cast<float>(value);
            p->setNormalized(isInvertedBypassParam(p) ? (1.0f - v) : v);
            if (p->getName() == "Preset" && processor_->getProgramCount() > 0) {
                const int maxIdx = std::max(1, processor_->getProgramCount() - 1);
                const int idx    = std::clamp(static_cast<int>(std::lround(v * static_cast<float>(maxIdx))), 0, maxIdx);
                processor_->setCurrentProgram(idx);
            }
            return kResultOk;
        }
        return kInvalidArgument;
    }

    tresult AVRA_VST3_API setComponentHandler(IComponentHandler* handler) override {
        componentHandler_ = handler;
        return kResultOk;
    }

    IPlugView* AVRA_VST3_API createView(const char* name) override {
        if (!processor_ || !processor_->hasEditor()) return nullptr;
        if (name && std::strcmp(name, "editor") == 0) {
            return new AvraVst3PlugView<PluginType>(*processor_);
        }
        return nullptr;
    }

    // --- IUnitInfo ---
    int32 AVRA_VST3_API getUnitCount() override {
        return 1;
    }

    tresult AVRA_VST3_API getUnitInfo(int32 unitIndex, UnitInfo& info) override {
        if (unitIndex != 0) return kInvalidArgument;
        std::memset(&info, 0, sizeof(UnitInfo));
        info.id            = 0;
        info.parentUnitId  = -1;
        copyAsciiToUtf16(info.name, 128, "Root");
        info.programListId = (processor_ && processor_->getProgramCount() > 1) ? 1 : -1;
        return kResultOk;
    }

    int32 AVRA_VST3_API getProgramListCount() override {
        return (processor_ && processor_->getProgramCount() > 1) ? 1 : 0;
    }

    tresult AVRA_VST3_API getProgramListInfo(int32 listIndex, ProgramListInfo& info) override {
        if (listIndex != 0 || !processor_) return kInvalidArgument;
        std::memset(&info, 0, sizeof(ProgramListInfo));
        info.id           = 1;
        copyAsciiToUtf16(info.name, 128, "Factory Presets");
        info.programCount = processor_->getProgramCount();
        return kResultOk;
    }

    tresult AVRA_VST3_API getProgramName(int32 listId, int32 programIndex, char16 name[128]) override {
        if (listId != 1 || !processor_ || programIndex < 0 || programIndex >= processor_->getProgramCount()) {
            return kInvalidArgument;
        }
        copyAsciiToUtf16(name, 128, processor_->getProgramName(programIndex));
        return kResultOk;
    }

    tresult AVRA_VST3_API getProgramInfo(int32, int32, const char*, char16*) override {
        return kNotImplemented;
    }

    tresult AVRA_VST3_API hasProgramPitchNames(int32, int32) override {
        return kResultFalse;
    }

    tresult AVRA_VST3_API getProgramPitchName(int32, int32, int16, char16*) override {
        return kNotImplemented;
    }

    int32 AVRA_VST3_API getSelectedUnit() override {
        return 0;
    }

    tresult AVRA_VST3_API selectUnit(int32) override {
        return kResultOk;
    }

    tresult AVRA_VST3_API getUnitByBus(int32, int32, int32, int32, int32& unitId) override {
        unitId = 0;
        return kResultOk;
    }

    tresult AVRA_VST3_API setUnitProgramData(int32, int32 programIndex, IBStream*) override {
        if (processor_ && programIndex >= 0 && programIndex < processor_->getProgramCount()) {
            processor_->setCurrentProgram(programIndex);
            return kResultOk;
        }
        return kInvalidArgument;
    }

private:
    std::atomic<uint32> refCount_{1};
    TUID controllerCid_{};
    SpeakerArrangement busArrangement_{kSpeakerArrStereo};
    std::unique_ptr<PluginType> processor_;
    std::vector<const float*> inputPtrs_;
    std::vector<float*> outputPtrs_;
    IComponentHandler* componentHandler_{nullptr};
};

// =============================================================================
// VST3 Plugin Factory (IPluginFactory3)
// =============================================================================

template <typename PluginType>
class AvraVst3PluginFactory final : public IPluginFactory3 {
public:
    AvraVst3PluginFactory(
        const char* pluginId,
        const char* pluginName,
        const char* vendor,
        const char* url,
        const char* version,
        const char* subCategories)
        : pluginName_(pluginName ? pluginName : "AvraPlugin")
        , vendor_(vendor ? vendor : "Avra Audio")
        , url_(url ? url : "https://avra.audio")
        , version_(version ? version : "1.0.0")
        , subCategories_(subCategories ? subCategories : "Fx") {
        deriveTuidFromString(processorCid_, pluginId, 0xA72A0001u);
        deriveTuidFromString(controllerCid_, pluginId, 0xA72A0002u);
    }

    tresult AVRA_VST3_API queryInterface(const TUID _iid, void** obj) override {
        if (!obj) return kInvalidArgument;
        if (iidEqual(_iid, kFUnknownIid) ||
            iidEqual(_iid, kIPluginFactoryIid) ||
            iidEqual(_iid, kIPluginFactory2Iid) ||
            iidEqual(_iid, kIPluginFactory3Iid)) {
            addRef();
            *obj = static_cast<IPluginFactory3*>(this);
            return kResultOk;
        }
        *obj = nullptr;
        return kNoInterface;
    }

    uint32 AVRA_VST3_API addRef() override { return ++refCount_; }
    uint32 AVRA_VST3_API release() override { return --refCount_; }

    tresult AVRA_VST3_API getFactoryInfo(PFactoryInfo* info) override {
        if (!info) return kInvalidArgument;
        std::memset(info, 0, sizeof(PFactoryInfo));
        copyAsciiToChar8(info->vendor, PFactoryInfo::kNameSize, vendor_);
        copyAsciiToChar8(info->url, PFactoryInfo::kURLSize, url_);
        copyAsciiToChar8(info->email, PFactoryInfo::kEmailSize, "support@avra.audio");
        info->flags = PFactoryInfo::kUnicode;
        return kResultOk;
    }

    int32 AVRA_VST3_API countClasses() override {
        return 1;
    }

    tresult AVRA_VST3_API getClassInfo(int32 index, PClassInfo* info) override {
        if (index != 0 || !info) return kInvalidArgument;
        std::memset(info, 0, sizeof(PClassInfo));
        std::memcpy(info->cid, processorCid_, 16);
        info->cardinality = PClassInfo::kManyInstances;
        copyAsciiToChar8(info->category, PClassInfo::kCategorySize, "Audio Module Class");
        copyAsciiToChar8(info->name, PClassInfo::kNameSize, pluginName_);
        return kResultOk;
    }

    tresult AVRA_VST3_API getClassInfo2(int32 index, PClassInfo2* info) override {
        if (index != 0 || !info) return kInvalidArgument;
        std::memset(info, 0, sizeof(PClassInfo2));
        std::memcpy(info->cid, processorCid_, 16);
        info->cardinality = PClassInfo::kManyInstances;
        copyAsciiToChar8(info->category, 32, "Audio Module Class");
        copyAsciiToChar8(info->name, 64, pluginName_);
        info->classFlags = 0;
        copyAsciiToChar8(info->subCategories, 128, subCategories_);
        copyAsciiToChar8(info->vendor, 64, vendor_);
        copyAsciiToChar8(info->version, 64, version_);
        copyAsciiToChar8(info->sdkVersion, 64, "VST 3.7.9");
        return kResultOk;
    }

    tresult AVRA_VST3_API getClassInfoUnicode(int32 index, PClassInfoW* info) override {
        if (index != 0 || !info) return kInvalidArgument;
        std::memset(info, 0, sizeof(PClassInfoW));
        std::memcpy(info->cid, processorCid_, 16);
        info->cardinality = PClassInfo::kManyInstances;
        copyAsciiToChar8(info->category, 32, "Audio Module Class");
        copyAsciiToUtf16(info->name, 64, pluginName_);
        info->classFlags = 0;
        copyAsciiToChar8(info->subCategories, 128, subCategories_);
        copyAsciiToUtf16(info->vendor, 64, vendor_);
        copyAsciiToUtf16(info->version, 64, version_);
        copyAsciiToUtf16(info->sdkVersion, 64, "VST 3.7.9");
        return kResultOk;
    }

    tresult AVRA_VST3_API createInstance(const char* cid, const char* _iid, void** obj) override {
        if (!cid || !_iid || !obj) return kInvalidArgument;
        const auto* cidBytes = reinterpret_cast<const uint8*>(cid);
        if (!tuidMatches(cidBytes, processorCid_) && !tuidMatches(cidBytes, controllerCid_)) {
            *obj = nullptr;
            return kNoInterface;
        }
        auto* inst = new AvraVst3SingleComponent<PluginType>(controllerCid_);
        const tresult res = inst->queryInterface(reinterpret_cast<const uint8*>(_iid), obj);
        inst->release();
        return res;
    }

    tresult AVRA_VST3_API setHostContext(FUnknown*) override {
        return kResultOk;
    }

private:
    std::atomic<uint32> refCount_{1};
    TUID processorCid_{};
    TUID controllerCid_{};
    const char* pluginName_;
    const char* vendor_;
    const char* url_;
    const char* version_;
    const char* subCategories_;
};

} // namespace avra::vst3_bridge

/**
 * @brief Macro to define and export a native VST3 plugin library (.vst3) for any Avra Processor.
 */
#define AVRA_EXPORT_VST3(PluginClass, PluginId, PluginName, Vendor, Url, Version, SubCategories) \
    AVRA_VST3_EXPORT bool AVRA_VST3_API InitDll() { return avra::provenance::keepProvenanceStampAlive(); } \
    AVRA_VST3_EXPORT bool AVRA_VST3_API ExitDll() { return true; } \
    AVRA_VST3_EXPORT avra::vst3_bridge::IPluginFactory3* AVRA_VST3_API GetPluginFactory() { \
        static avra::vst3_bridge::AvraVst3PluginFactory<PluginClass> s_factory( \
            PluginId, PluginName, Vendor, Url, Version, SubCategories); \
        s_factory.addRef(); \
        return static_cast<avra::vst3_bridge::IPluginFactory3*>(&s_factory); \
    }
