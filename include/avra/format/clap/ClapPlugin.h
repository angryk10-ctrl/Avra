#pragma once

#include "avra/Avra.h"
#include <clap/clap.h>
#include <vector>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace avra::clap_bridge {

inline void safeStringCopy(char* dst, size_t dstSize, const char* src) {
    if (!dst || dstSize == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t i = 0;
    while (i < dstSize - 1 && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

template <typename PluginType>
struct AvraClapInstance {
    clap_plugin_t clapPlugin{};
    const clap_host_t* host{nullptr};
    std::unique_ptr<PluginType> processor{};
    std::unique_ptr<Editor> editor{};
    std::unique_ptr<gui::Window> window{};

    // Scratch buffers for real-time channel pointer routing (zero allocations during process)
    std::vector<const float*> inputPointers;
    std::vector<float*> outputPointers;
    std::vector<MidiEvent> midiEvents;
};


// =============================================================================
// CLAP Plugin Core Callbacks
// =============================================================================

template <typename PluginType>
bool CLAP_ABI pluginInit(const clap_plugin_t* plugin) {
    return (plugin && plugin->plugin_data);
}

template <typename PluginType>
void CLAP_ABI pluginDestroy(const clap_plugin_t* plugin) {
    if (!plugin) return;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    delete inst;
}

template <typename PluginType>
bool CLAP_ABI pluginActivate(const clap_plugin_t* plugin, double sample_rate, uint32_t min_frames, uint32_t max_frames) {
    (void)min_frames;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);

    inst->inputPointers.resize(16, nullptr);
    inst->outputPointers.resize(16, nullptr);
    inst->midiEvents.reserve(128);

    const double safeRate = (sample_rate >= 8000.0 && sample_rate <= 768000.0) ? sample_rate : 48000.0;
    const size_t safeMaxFrames = std::max<size_t>(64, max_frames);
    inst->processor->prepare(safeRate, safeMaxFrames);
    return true;
}

template <typename PluginType>
void CLAP_ABI pluginDeactivate(const clap_plugin_t* plugin) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    auto& params = inst->processor->getParameters();
    for (size_t i = 0; i < params.size(); ++i) {
        if (auto* p = params.getByIndex(i)) p->setModulationOffset(0.0f);
    }
    inst->processor->reset();
}

template <typename PluginType>
bool CLAP_ABI pluginStartProcessing(const clap_plugin_t* plugin) {
    (void)plugin;
    return true;
}

template <typename PluginType>
void CLAP_ABI pluginStopProcessing(const clap_plugin_t* plugin) {
    (void)plugin;
}


template <typename PluginType>
void CLAP_ABI pluginReset(const clap_plugin_t* plugin) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    auto& params = inst->processor->getParameters();
    for (size_t i = 0; i < params.size(); ++i) {
        if (auto* p = params.getByIndex(i)) p->setModulationOffset(0.0f);
    }
    inst->processor->reset();
}

// =============================================================================
// Real-Time Audio & Event Processing
// =============================================================================

template <typename PluginType>
clap_process_status CLAP_ABI pluginProcess(const clap_plugin_t* plugin, const clap_process_t* process) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    auto& params = inst->processor->getParameters();

    // 1. Process incoming parameter automation & MIDI events
    inst->midiEvents.clear();
    if (process->in_events) {
        const uint32_t numEvents = process->in_events->size(process->in_events);
        for (uint32_t i = 0; i < numEvents; ++i) {
            const clap_event_header_t* header = process->in_events->get(process->in_events, i);
            if (!header) continue;

            if (header->space_id == CLAP_CORE_EVENT_SPACE_ID) {
                if (header->type == CLAP_EVENT_PARAM_VALUE) {
                    auto* paramVal = reinterpret_cast<const clap_event_param_value_t*>(header);
                    auto* param = static_cast<Parameter*>(paramVal->cookie);
                    if (!param) {
                        param = params.getById(paramVal->param_id);
                    }
                    if (param) {
                        param->setPlain(static_cast<float>(paramVal->value));
                    }
                } else if (header->type == CLAP_EVENT_PARAM_MOD) {
                    auto* paramMod = reinterpret_cast<const clap_event_param_mod_t*>(header);
                    auto* param = static_cast<Parameter*>(paramMod->cookie);
                    if (!param) {
                        param = params.getById(paramMod->param_id);
                    }
                    if (param) {
                        // Non-destructive CLAP parameter modulation offset
                        param->setModulationOffset(static_cast<float>(paramMod->amount));
                    }
                } else if (header->type == CLAP_EVENT_MIDI) {
                    auto* midi = reinterpret_cast<const clap_event_midi_t*>(header);
                    inst->midiEvents.push_back(MidiEvent{
                        .sampleOffset = midi->header.time,
                        .status = midi->data[0],
                        .data1 = midi->data[1],
                        .data2 = midi->data[2]
                    });
                }
            }
        }
    }

    // 2. Set up Input AudioSpan
    size_t inChannels = 0;
    if (process->audio_inputs_count > 0 && process->audio_inputs) {
        const auto& inPort = process->audio_inputs[0];
        inChannels = std::min<size_t>(inPort.channel_count, inst->inputPointers.size());
        for (size_t ch = 0; ch < inChannels; ++ch) {
            inst->inputPointers[ch] = inPort.data32 ? inPort.data32[ch] : nullptr;
        }
    }
    AudioSpan<const Sample> inputSpan(inst->inputPointers.data(), inChannels, process->frames_count);

    // 3. Set up Output AudioSpan
    size_t outChannels = 0;
    if (process->audio_outputs_count > 0 && process->audio_outputs) {
        const auto& outPort = process->audio_outputs[0];
        outChannels = std::min<size_t>(outPort.channel_count, inst->outputPointers.size());
        for (size_t ch = 0; ch < outChannels; ++ch) {
            inst->outputPointers[ch] = outPort.data32 ? outPort.data32[ch] : nullptr;
        }
    }
    AudioSpan<Sample> outputSpan(inst->outputPointers.data(), outChannels, process->frames_count);

    // 4. Transport Info
    TransportInfo transport{};
    if (process->transport) {
        transport.isPlaying = (process->transport->flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
        transport.isRecording = (process->transport->flags & CLAP_TRANSPORT_IS_RECORDING) != 0;
        transport.isLooping = (process->transport->flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE) != 0;
        if (process->transport->flags & CLAP_TRANSPORT_HAS_TEMPO) {
            transport.bpm = process->transport->tempo;
        }
        if (process->transport->flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE) {
            transport.timeSigNumerator = process->transport->tsig_num;
            transport.timeSigDenominator = process->transport->tsig_denom;
        }
        if (process->transport->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) {
            transport.songPositionBeats = static_cast<double>(process->transport->song_pos_beats) / static_cast<double>(CLAP_BEATTIME_FACTOR);
        }
        if (process->transport->flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE) {
            transport.songPositionSamples = static_cast<int64_t>(process->transport->song_pos_seconds * inst->processor->getSampleRate());
        }
    }

    // 5. Invoke Processor
    ProcessContext ctx{
        .inputs = inputSpan,
        .outputs = outputSpan,
        .midiInputs = MidiSpan(inst->midiEvents.data(), inst->midiEvents.size()),
        .transport = transport,
        .sampleRate = inst->processor->getSampleRate(),
        .numFrames = process->frames_count
    };

    inst->processor->process(ctx);
    if (!avra::provenance::verifyIntegrity()) {
        ctx.outputs.clear();
    }

    return CLAP_PROCESS_CONTINUE;
}

// =============================================================================
// Extension: Audio Ports
// =============================================================================

template <typename PluginType>
uint32_t CLAP_ABI audioPortsCount(const clap_plugin_t* plugin, bool is_input) {
    (void)plugin;
    (void)is_input;
    return 1; // 1 Main stereo input, 1 Main stereo output
}

template <typename PluginType>
bool CLAP_ABI audioPortsGet(const clap_plugin_t* plugin, uint32_t index, bool is_input, clap_audio_port_info_t* info) {
    (void)plugin;
    if (index != 0 || !info) return false;

    info->id = 0;
    safeStringCopy(info->name, sizeof(info->name), is_input ? "Stereo In" : "Stereo Out");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = is_input ? 0 : CLAP_INVALID_ID;
    return true;
}

template <typename PluginType>
const clap_plugin_audio_ports_t s_audioPortsExt = {
    audioPortsCount<PluginType>,
    audioPortsGet<PluginType>
};

// =============================================================================
// Extension: Parameters
// =============================================================================

template <typename PluginType>
uint32_t CLAP_ABI paramsCount(const clap_plugin_t* plugin) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    return static_cast<uint32_t>(inst->processor->getParameters().size());
}

template <typename PluginType>
bool CLAP_ABI paramsGetInfo(const clap_plugin_t* plugin, uint32_t param_index, clap_param_info_t* param_info) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    const auto* param = inst->processor->getParameters().getByIndex(param_index);
    if (!param || !param_info) return false;

    param_info->id = param->getId();
    param_info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE;
    if (param->getTaper() == ParameterTaper::Stepped) {
        param_info->flags |= CLAP_PARAM_IS_STEPPED;
    }

    param_info->cookie = const_cast<Parameter*>(param);
    safeStringCopy(param_info->name, sizeof(param_info->name), param->getName().c_str());
    param_info->module[0] = '\0';

    param_info->min_value = param->getMinValue();
    param_info->max_value = param->getMaxValue();
    param_info->default_value = param->getDefaultValue();

    return true;
}

template <typename PluginType>
bool CLAP_ABI paramsGetValue(const clap_plugin_t* plugin, clap_id param_id, double* out_value) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    const auto* param = inst->processor->getParameters().getById(param_id);
    if (!param || !out_value) return false;

    *out_value = static_cast<double>(param->getPlain());
    return true;
}

template <typename PluginType>
bool CLAP_ABI paramsValueToText(const clap_plugin_t* plugin, clap_id param_id, double value, char* out_buffer, uint32_t out_buffer_capacity) {
    (void)param_id;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    const auto* param = inst->processor->getParameters().getById(param_id);
    if (!param || !out_buffer || out_buffer_capacity == 0) return false;

    std::string text = param->getText(static_cast<float>(value));
    safeStringCopy(out_buffer, out_buffer_capacity, text.c_str());
    return true;
}

template <typename PluginType>
bool CLAP_ABI paramsTextToValue(const clap_plugin_t* plugin, clap_id param_id, const char* param_value_text, double* out_value) {
    (void)plugin;
    (void)param_id;
    if (!param_value_text || !out_value) return false;
    char* end = nullptr;
    *out_value = std::strtod(param_value_text, &end);
    return end != param_value_text;
}

template <typename PluginType>
void CLAP_ABI paramsFlush(const clap_plugin_t* plugin, const clap_input_events_t* in_events, const clap_output_events_t* out_events) {
    (void)out_events;
    if (!in_events) return;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    auto& params = inst->processor->getParameters();

    const uint32_t numEvents = in_events->size(in_events);
    for (uint32_t i = 0; i < numEvents; ++i) {
        const clap_event_header_t* header = in_events->get(in_events, i);
        if (!header) continue;

        if (header->space_id == CLAP_CORE_EVENT_SPACE_ID) {
            if (header->type == CLAP_EVENT_PARAM_VALUE) {
                auto* paramVal = reinterpret_cast<const clap_event_param_value_t*>(header);
                auto* param = static_cast<Parameter*>(paramVal->cookie);
                if (!param) {
                    param = params.getById(paramVal->param_id);
                }
                if (param) {
                    param->setPlain(static_cast<float>(paramVal->value));
                }
            } else if (header->type == CLAP_EVENT_PARAM_MOD) {
                auto* paramMod = reinterpret_cast<const clap_event_param_mod_t*>(header);
                auto* param = static_cast<Parameter*>(paramMod->cookie);
                if (!param) {
                    param = params.getById(paramMod->param_id);
                }
                if (param) {
                    param->setModulationOffset(static_cast<float>(paramMod->amount));
                }
            }
        }
    }
}


template <typename PluginType>
const clap_plugin_params_t s_paramsExt = {
    paramsCount<PluginType>,
    paramsGetInfo<PluginType>,
    paramsGetValue<PluginType>,
    paramsValueToText<PluginType>,
    paramsTextToValue<PluginType>,
    paramsFlush<PluginType>
};

// =============================================================================
// Extension: State Save & Restore
// =============================================================================

template <typename PluginType>
bool CLAP_ABI stateSave(const clap_plugin_t* plugin, const clap_ostream_t* stream) {
    if (!plugin || !stream) return false;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    std::string state = inst->processor->getParameters().serializeState();
    int64_t written = stream->write(stream, state.data(), state.size());
    return written == static_cast<int64_t>(state.size());
}

template <typename PluginType>
bool CLAP_ABI stateLoad(const clap_plugin_t* plugin, const clap_istream_t* stream) {
    if (!plugin || !stream) return false;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);

    std::string state;
    char buffer[512];
    int64_t bytesRead = 0;
    while ((bytesRead = stream->read(stream, buffer, sizeof(buffer))) > 0) {
        state.append(buffer, static_cast<size_t>(bytesRead));
    }

    const bool ok = inst->processor->getParameters().restoreState(state);
    if (ok && inst->editor) {
        inst->editor->repaint();
    }
    return ok;
}

template <typename PluginType>
const clap_plugin_state_t s_stateExt = {
    stateSave<PluginType>,
    stateLoad<PluginType>
};

// =============================================================================
// Extension: GUI (Direct2D Win32 / Cocoa)
// =============================================================================

template <typename PluginType>
bool CLAP_ABI guiIsApiSupported(const clap_plugin_t* plugin, const char* api, bool is_floating) {
    (void)plugin;
#if defined(_WIN32)
    return (std::strcmp(api, CLAP_WINDOW_API_WIN32) == 0 && !is_floating);
#else
    (void)api; (void)is_floating;
    return false;
#endif
}

template <typename PluginType>
bool CLAP_ABI guiGetPreferredApi(const clap_plugin_t* plugin, const char** api, bool* is_floating) {
    (void)plugin;
#if defined(_WIN32)
    *api = CLAP_WINDOW_API_WIN32;
    *is_floating = false;
    return true;
#else
    (void)api; (void)is_floating;
    return false;
#endif
}

template <typename PluginType>
bool CLAP_ABI guiCreate(const clap_plugin_t* plugin, const char* api, bool is_floating) {
    (void)api; (void)is_floating;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    if (!inst->processor->hasEditor()) return false;
    inst->editor = inst->processor->createEditor();
    if (!inst->editor) return false;

    inst->window = std::make_unique<gui::Window>(inst->editor.get());
    return true;
}

template <typename PluginType>
void CLAP_ABI guiDestroy(const clap_plugin_t* plugin) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    inst->window.reset();
    inst->editor.reset();
}

template <typename PluginType>
bool CLAP_ABI guiSetScale(const clap_plugin_t* plugin, double scale) {
    (void)plugin; (void)scale;
    return false;
}

template <typename PluginType>
bool CLAP_ABI guiGetSize(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    if (!inst->editor || !width || !height) return false;
    *width = static_cast<uint32_t>(inst->editor->getDefaultWidth());
    *height = static_cast<uint32_t>(inst->editor->getDefaultHeight());
    return true;
}

template <typename PluginType>
bool CLAP_ABI guiCanResize(const clap_plugin_t* plugin) {
    (void)plugin;
    return false;
}

template <typename PluginType>
bool CLAP_ABI guiGetResizeHints(const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints) {
    (void)plugin; (void)hints;
    return false;
}

template <typename PluginType>
bool CLAP_ABI guiAdjustSize(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height) {
    return guiGetSize<PluginType>(plugin, width, height);
}

template <typename PluginType>
bool CLAP_ABI guiSetSize(const clap_plugin_t* plugin, uint32_t width, uint32_t height) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    if (inst->window) {
        inst->window->setSize(static_cast<int>(width), static_cast<int>(height));
        return true;
    }
    return false;
}

template <typename PluginType>
bool CLAP_ABI guiSetParent(const clap_plugin_t* plugin, const clap_window_t* window) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    if (!inst->window || !inst->editor || !window) return false;

#if defined(_WIN32)
    HWND parentHwnd = reinterpret_cast<HWND>(window->win32);
    return inst->window->create(parentHwnd, inst->editor->getDefaultWidth(), inst->editor->getDefaultHeight());
#else
    return false;
#endif
}

template <typename PluginType>
bool CLAP_ABI guiSetTransient(const clap_plugin_t* plugin, const clap_window_t* window) {
    (void)plugin; (void)window;
    return false;
}

template <typename PluginType>
void CLAP_ABI guiSuggestTitle(const clap_plugin_t* plugin, const char* title) {
    (void)plugin; (void)title;
}

template <typename PluginType>
bool CLAP_ABI guiShow(const clap_plugin_t* plugin) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    if (inst->window) {
        inst->window->show();
        return true;
    }
    return false;
}

template <typename PluginType>
bool CLAP_ABI guiHide(const clap_plugin_t* plugin) {
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    if (inst->window) {
        inst->window->hide();
        return true;
    }
    return false;
}

template <typename PluginType>
const clap_plugin_gui_t s_guiExt = {
    guiIsApiSupported<PluginType>,
    guiGetPreferredApi<PluginType>,
    guiCreate<PluginType>,
    guiDestroy<PluginType>,
    guiSetScale<PluginType>,
    guiGetSize<PluginType>,
    guiCanResize<PluginType>,
    guiGetResizeHints<PluginType>,
    guiAdjustSize<PluginType>,
    guiSetSize<PluginType>,
    guiSetParent<PluginType>,
    guiSetTransient<PluginType>,
    guiSuggestTitle<PluginType>,
    guiShow<PluginType>,
    guiHide<PluginType>
};

// =============================================================================
// Extension: Latency (Plugin Delay Compensation / PDC)
// =============================================================================

template <typename PluginType>
uint32_t CLAP_ABI latencyGet(const clap_plugin_t* plugin) {
    if (!plugin || !plugin->plugin_data) return 0;
    auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
    return inst->processor ? inst->processor->getLatencySamples() : 0;
}

template <typename PluginType>
const clap_plugin_latency_t s_latencyExt = {
    latencyGet<PluginType>
};

// =============================================================================
// Extension Dispatcher
// =============================================================================

template <typename PluginType>
const void* CLAP_ABI pluginGetExtension(const clap_plugin_t* plugin, const char* id) {
    (void)plugin;
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &s_audioPortsExt<PluginType>;
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0) return &s_paramsExt<PluginType>;
    if (std::strcmp(id, CLAP_EXT_STATE) == 0) return &s_stateExt<PluginType>;
    if (std::strcmp(id, CLAP_EXT_LATENCY) == 0) return &s_latencyExt<PluginType>;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0) {
        auto* inst = static_cast<AvraClapInstance<PluginType>*>(plugin->plugin_data);
        if (inst && inst->processor && inst->processor->hasEditor()) {
            return &s_guiExt<PluginType>;
        }
    }
    return nullptr;
}


// =============================================================================
// Plugin Factory Instantiation
// =============================================================================

template <typename PluginType>
const clap_plugin_t* createPlugin(const clap_host_t* host, const clap_plugin_descriptor_t* descriptor) {
    auto* inst = new AvraClapInstance<PluginType>();
    inst->host = host;
    inst->processor = std::make_unique<PluginType>();

    inst->clapPlugin.desc = descriptor;
    inst->clapPlugin.plugin_data = inst;
    inst->clapPlugin.init = pluginInit<PluginType>;
    inst->clapPlugin.destroy = pluginDestroy<PluginType>;
    inst->clapPlugin.activate = pluginActivate<PluginType>;
    inst->clapPlugin.deactivate = pluginDeactivate<PluginType>;
    inst->clapPlugin.start_processing = pluginStartProcessing<PluginType>;
    inst->clapPlugin.stop_processing = pluginStopProcessing<PluginType>;
    inst->clapPlugin.reset = pluginReset<PluginType>;
    inst->clapPlugin.process = pluginProcess<PluginType>;
    inst->clapPlugin.get_extension = pluginGetExtension<PluginType>;
    inst->clapPlugin.on_main_thread = nullptr;

    return &inst->clapPlugin;
}

} // namespace avra::clap_bridge

/**
 * @brief Macro to define and export a CLAP plugin library for any Avra Processor.
 */
#define AVRA_EXPORT_CLAP(PluginClass, PluginId, PluginName, Vendor, Url, Version, Description, ...) \
    static const char* s_features[] = { __VA_ARGS__, nullptr }; \
    static const clap_plugin_descriptor_t s_descriptor = { \
        .clap_version = CLAP_VERSION_INIT, \
        .id = PluginId, \
        .name = PluginName, \
        .vendor = Vendor, \
        .url = Url, \
        .manual_url = Url, \
        .support_url = Url, \
        .version = Version, \
        .description = Description, \
        .features = s_features \
    }; \
    \
    static uint32_t CLAP_ABI avra_factory_get_plugin_count(const clap_plugin_factory_t*) { return 1; } \
    static const clap_plugin_descriptor_t* CLAP_ABI avra_factory_get_plugin_descriptor(const clap_plugin_factory_t*, uint32_t index) { \
        return (index == 0) ? &s_descriptor : nullptr; \
    } \
    static const clap_plugin_t* CLAP_ABI avra_factory_create_plugin(const clap_plugin_factory_t*, const clap_host_t* host, const char* plugin_id) { \
        if (!plugin_id || std::strcmp(plugin_id, s_descriptor.id) != 0) return nullptr; \
        return avra::clap_bridge::createPlugin<PluginClass>(host, &s_descriptor); \
    } \
    \
    static const clap_plugin_factory_t s_factory = { \
        avra_factory_get_plugin_count, \
        avra_factory_get_plugin_descriptor, \
        avra_factory_create_plugin \
    }; \
    \
    static bool CLAP_ABI avra_entry_init(const char*) { return avra::provenance::keepProvenanceStampAlive(); } \
    static void CLAP_ABI avra_entry_deinit() {} \
    static const void* CLAP_ABI avra_entry_get_factory(const char* factory_id) { \
        if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) return &s_factory; \
        return nullptr; \
    } \
    \
    CLAP_EXPORT const clap_plugin_entry_t clap_entry = { \
        .clap_version = CLAP_VERSION_INIT, \
        .init = avra_entry_init, \
        .deinit = avra_entry_deinit, \
        .get_factory = avra_entry_get_factory \
    };
