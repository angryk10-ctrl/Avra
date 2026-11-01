#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <windows.h>
#include <clap/clap.h>

#define CLAP_TEST(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "[FAILED] " << msg << " (" #expr ") at line " << __LINE__ << std::endl; \
            return 1; \
        } \
    } while (0)

int main(int argc, char* argv[]) {
    std::cout << "===========================================" << std::endl;
    std::cout << "Avra CLAP Host Validation Harness" << std::endl;
    std::cout << "===========================================" << std::endl;

    const char* pluginPath = (argc > 1) ? argv[1] : "AvraGain.clap";
    std::cout << "Loading plugin: " << pluginPath << std::endl;

    HMODULE hModule = LoadLibraryA(pluginPath);
    CLAP_TEST(hModule != nullptr, "LoadLibrary failed to load CLAP binary");

    auto* entry = reinterpret_cast<const clap_plugin_entry_t*>(GetProcAddress(hModule, "clap_entry"));
    CLAP_TEST(entry != nullptr, "Failed to locate 'clap_entry' export symbol");
    CLAP_TEST(entry->init(pluginPath) == true, "entry->init() failed");

    auto* factory = static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    CLAP_TEST(factory != nullptr, "Failed to retrieve CLAP_PLUGIN_FACTORY_ID");

    uint32_t count = factory->get_plugin_count(factory);
    CLAP_TEST(count >= 1, "Plugin factory count must be >= 1");

    const clap_plugin_descriptor_t* desc = factory->get_plugin_descriptor(factory, 0);
    CLAP_TEST(desc != nullptr, "Failed to get descriptor");
    std::cout << "  Plugin Name: " << desc->name << std::endl;
    std::cout << "  Plugin ID:   " << desc->id << std::endl;
    std::cout << "  Vendor:      " << desc->vendor << std::endl;
    std::cout << "  Version:     " << desc->version << std::endl;

    // Dummy host definition
    static clap_host_t dummyHost = {
        .clap_version = CLAP_VERSION_INIT,
        .host_data = nullptr,
        .name = "AvraTestHost",
        .vendor = "Avra Audio",
        .url = "https://github.com/angryk10-ctrl/Avra",
        .version = "1.0.0",
        .get_extension = [](const clap_host_t*, const char*) -> const void* { return nullptr; },
        .request_restart = [](const clap_host_t*) {},
        .request_process = [](const clap_host_t*) {},
        .request_callback = [](const clap_host_t*) {}
    };

    const clap_plugin_t* plugin = factory->create_plugin(factory, &dummyHost, desc->id);
    CLAP_TEST(plugin != nullptr, "factory->create_plugin() returned null");

    CLAP_TEST(plugin->init(plugin) == true, "plugin->init() failed");
    CLAP_TEST(plugin->activate(plugin, 48000.0, 32, 512) == true, "plugin->activate() failed");

    // Check extensions
    auto* paramsExt = static_cast<const clap_plugin_params_t*>(plugin->get_extension(plugin, CLAP_EXT_PARAMS));
    CLAP_TEST(paramsExt != nullptr, "Plugin must support CLAP_EXT_PARAMS");
    uint32_t numParams = paramsExt->count(plugin);
    std::cout << "  Parameters reported by plugin: " << numParams << std::endl;
    CLAP_TEST(numParams >= 2, "Plugin must have parameters");

    clap_param_info_t param0{};
    CLAP_TEST(paramsExt->get_info(plugin, 0, &param0) == true, "params->get_info(0) failed");
    std::cout << "    Param 0: " << param0.name << " (ID: " << param0.id << ", Min: " << param0.min_value << ", Max: " << param0.max_value << ")" << std::endl;
    CLAP_TEST((param0.flags & CLAP_PARAM_IS_MODULATABLE) != 0, "All exported CLAP parameters must advertise CLAP_PARAM_IS_MODULATABLE");

    // Verify CLAP_EXT_LATENCY extension for PDC reporting
    auto* latencyExt = static_cast<const clap_plugin_latency_t*>(plugin->get_extension(plugin, CLAP_EXT_LATENCY));
    CLAP_TEST(latencyExt != nullptr, "Plugin must support CLAP_EXT_LATENCY for Plugin Delay Compensation (PDC)");
    std::cout << "  Reported CLAP PDC Latency: " << latencyExt->get(plugin) << " samples" << std::endl;

    // Verify non-destructive CLAP_EVENT_PARAM_MOD via params->flush()
    double baseValBefore = 0.0;
    CLAP_TEST(paramsExt->get_value(plugin, param0.id, &baseValBefore) == true, "params->get_value(param0) failed");
    static clap_event_param_mod_t modEv = {
        .header = {
            .size = sizeof(clap_event_param_mod_t),
            .time = 0,
            .space_id = CLAP_CORE_EVENT_SPACE_ID,
            .type = CLAP_EVENT_PARAM_MOD,
            .flags = 0
        },
        .param_id = param0.id,
        .cookie = param0.cookie,
        .note_id = -1,
        .port_index = -1,
        .channel = -1,
        .key = -1,
        .amount = 3.0
    };
    clap_input_events_t modInEvents = {
        .ctx = &modEv,
        .size = [](const clap_input_events_t*) -> uint32_t { return 1; },
        .get = [](const clap_input_events_t* list, uint32_t index) -> const clap_event_header_t* {
            return (index == 0) ? reinterpret_cast<const clap_event_header_t*>(list->ctx) : nullptr;
        }
    };
    clap_output_events_t dummyOutEvents = {
        .ctx = nullptr,
        .try_push = [](const clap_output_events_t*, const clap_event_header_t*) -> bool { return true; }
    };
    paramsExt->flush(plugin, &modInEvents, &dummyOutEvents);
    double baseValAfter = 0.0;
    CLAP_TEST(paramsExt->get_value(plugin, param0.id, &baseValAfter) == true, "params->get_value(param0) after mod failed");
    CLAP_TEST(std::abs(baseValAfter - baseValBefore) < 1e-6, "CLAP_EVENT_PARAM_MOD must modulate non-destructively without overwriting base parameter value");

    // Test audio processing through CLAP
    constexpr uint32_t kFrames = 64;
    std::vector<float> inL(kFrames, 1.0f);
    std::vector<float> inR(kFrames, 1.0f);
    std::vector<float> outL(kFrames, 0.0f);
    std::vector<float> outR(kFrames, 0.0f);

    float* inBuffers[2] = { inL.data(), inR.data() };
    float* outBuffers[2] = { outL.data(), outR.data() };

    clap_audio_buffer_t audioIn = {
        .data32 = inBuffers,
        .data64 = nullptr,
        .channel_count = 2,
        .latency = 0,
        .constant_mask = 0
    };

    clap_audio_buffer_t audioOut = {
        .data32 = outBuffers,
        .data64 = nullptr,
        .channel_count = 2,
        .latency = 0,
        .constant_mask = 0
    };

    clap_process_t processCtx = {
        .steady_time = 0,
        .frames_count = kFrames,
        .transport = nullptr,
        .audio_inputs = &audioIn,
        .audio_outputs = &audioOut,
        .audio_inputs_count = 1,
        .audio_outputs_count = 1,
        .in_events = nullptr,
        .out_events = nullptr
    };

    CLAP_TEST(plugin->start_processing(plugin) == true, "start_processing failed");
    clap_process_status status = plugin->process(plugin, &processCtx);
    CLAP_TEST(status == CLAP_PROCESS_CONTINUE, "process returned unexpected status");

    // Output audio rendered check (non-zero)
    CLAP_TEST(std::abs(outL[kFrames - 1]) > 0.01f, "CLAP audio render non-zero verify");
    std::cout << "  Audio render verified: Input 1.0f -> Output " << outL[kFrames - 1] << std::endl;

    // -------------------------------------------------------------
    // Test GUI Extension (Direct2D Win32)
    // -------------------------------------------------------------
    auto* guiExt = static_cast<const clap_plugin_gui_t*>(plugin->get_extension(plugin, CLAP_EXT_GUI));
    CLAP_TEST(guiExt != nullptr, "Plugin must support CLAP_EXT_GUI");
    CLAP_TEST(guiExt->is_api_supported(plugin, CLAP_WINDOW_API_WIN32, false) == true, "Win32 GUI API must be supported");

    uint32_t guiW = 0, guiH = 0;
    CLAP_TEST(guiExt->create(plugin, CLAP_WINDOW_API_WIN32, false) == true, "gui->create() failed");
    CLAP_TEST(guiExt->get_size(plugin, &guiW, &guiH) == true, "gui->get_size() failed");
    std::cout << "  GUI Extension verified: Default size " << guiW << "x" << guiH << std::endl;
    CLAP_TEST(guiW > 0 && guiH > 0, "GUI dimensions must be non-zero");

    // Test parent window embedding
    HWND dummyHostWnd = CreateWindowExW(0, L"STATIC", L"HostWindow", WS_OVERLAPPEDWINDOW, 0, 0, 1024, 768, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    clap_window_t hostWindow = {
        .api = CLAP_WINDOW_API_WIN32,
        .win32 = dummyHostWnd
    };

    CLAP_TEST(guiExt->set_parent(plugin, &hostWindow) == true, "gui->set_parent() failed");
    CLAP_TEST(guiExt->show(plugin) == true, "gui->show() failed");
    CLAP_TEST(guiExt->hide(plugin) == true, "gui->hide() failed");
    guiExt->destroy(plugin);
    DestroyWindow(dummyHostWnd);
    std::cout << "  GUI Win32 embedding, Direct2D creation, and lifecycle verified!" << std::endl;


    plugin->stop_processing(plugin);
    plugin->deactivate(plugin);
    plugin->destroy(plugin);

    FreeLibrary(hModule);

    std::cout << "\n>>> CLAP HOST & GUI VALIDATION COMPLETED 100% SUCCESSFULLY! <<<" << std::endl;
    return 0;
}

