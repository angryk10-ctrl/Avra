cmake_minimum_required(VERSION 3.20)
project(Avra VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable MSVC UTF-8 and strict warning flags if on MSVC
if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive- $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi> $<$<CONFIG:Release>:/Ot> $<$<CONFIG:Release>:/fp:fast> $<$<CONFIG:Release>:/arch:AVX2>)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# Avra Core Header-Only Library
add_library(Avra_Core INTERFACE)
add_library(Avra::Core ALIAS Avra_Core)

target_include_directories(Avra_Core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(Avra_Core INTERFACE cxx_std_20)

# Use bundled CLAP C-API headers (MIT licensed) when present, falling back to FetchContent
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/clap/clap.h")
    add_library(clap INTERFACE)
    target_include_directories(clap INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
else()
    include(FetchContent)
    FetchContent_Declare(
        clap
        GIT_REPOSITORY https://github.com/free-audio/clap.git
        GIT_TAG 1.2.9
    )
    FetchContent_MakeAvailable(clap)
endif()

target_link_libraries(Avra_Core INTERFACE clap)

if(WIN32)
    target_link_libraries(Avra_Core INTERFACE d2d1 dwrite user32 gdi32 shell32 comdlg32 winmm)
endif()


# Helper CMake function to declare an Avra CLAP plugin (.clap)
function(avra_add_clap_plugin target_name)
    cmake_parse_arguments(AVRA "" "OUTPUT_NAME" "SOURCES" ${ARGN})

    add_library(${target_name} MODULE ${AVRA_SOURCES})
    target_link_libraries(${target_name} PRIVATE Avra::Core)

    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        SUFFIX ".clap"
        ARCHIVE_OUTPUT_NAME "${target_name}"
        COMPILE_PDB_NAME "${target_name}"
        PDB_NAME "${target_name}"
    )

    if(AVRA_OUTPUT_NAME)
        set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_NAME "${AVRA_OUTPUT_NAME}" RUNTIME_OUTPUT_NAME "${AVRA_OUTPUT_NAME}")
    endif()
endfunction()

# Helper CMake function to declare an Avra VST3 plugin (.vst3)
function(avra_add_vst3_plugin target_name)
    cmake_parse_arguments(AVRA "" "OUTPUT_NAME" "SOURCES" ${ARGN})

    add_library(${target_name} MODULE ${AVRA_SOURCES})
    target_link_libraries(${target_name} PRIVATE Avra::Core)

    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        SUFFIX ".vst3"
        ARCHIVE_OUTPUT_NAME "${target_name}"
        COMPILE_PDB_NAME "${target_name}"
        PDB_NAME "${target_name}"
    )

    if(AVRA_OUTPUT_NAME)
        set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_NAME "${AVRA_OUTPUT_NAME}" RUNTIME_OUTPUT_NAME "${AVRA_OUTPUT_NAME}")
    endif()
endfunction()

# Helper CMake function to declare an Avra Standalone Desktop App (.exe)
function(avra_add_standalone_plugin target_name)
    cmake_parse_arguments(AVRA "" "OUTPUT_NAME" "SOURCES" ${ARGN})

    set(AVRA_STANDALONE_RC "")
    if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/avra_studio.rc")
        set(AVRA_STANDALONE_RC "${CMAKE_CURRENT_SOURCE_DIR}/assets/avra_studio.rc")
    endif()
    add_executable(${target_name} WIN32 ${AVRA_SOURCES} ${AVRA_STANDALONE_RC})
    target_link_libraries(${target_name} PRIVATE Avra::Core)

    if(AVRA_OUTPUT_NAME)
        set_target_properties(${target_name} PROPERTIES OUTPUT_NAME "${AVRA_OUTPUT_NAME}_Standalone")
    endif()
endfunction()

# Example: Avra Gain (CLAP + VST3 + Standalone .exe — Included Initial Demo Plugin)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin/GainPluginClap.cpp")
    avra_add_clap_plugin(AvraGain_Clap
        OUTPUT_NAME "AvraGain"
        SOURCES
            examples/GainPlugin/GainPluginClap.cpp
    )
    target_include_directories(AvraGain_Clap PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )

    avra_add_vst3_plugin(AvraGain_Vst3
        OUTPUT_NAME "AvraGain"
        SOURCES
            examples/GainPlugin/GainPluginVst3.cpp
    )
    target_include_directories(AvraGain_Vst3 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )

    avra_add_standalone_plugin(AvraGain_Standalone
        OUTPUT_NAME "AvraGain"
        SOURCES
            examples/GainPlugin/GainPluginStandalone.cpp
    )
    target_include_directories(AvraGain_Standalone PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/AvraGain/AvraGainClap.cpp")
    avra_add_clap_plugin(AvraGain_Clap
        OUTPUT_NAME "AvraGain"
        SOURCES
            plugins/AvraGain/AvraGainClap.cpp
    )
    target_include_directories(AvraGain_Clap PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/AvraGain
    )

    avra_add_vst3_plugin(AvraGain_Vst3
        OUTPUT_NAME "AvraGain"
        SOURCES
            plugins/AvraGain/AvraGainVst3.cpp
    )
    target_include_directories(AvraGain_Vst3 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/AvraGain
    )

    avra_add_standalone_plugin(AvraGain_Standalone
        OUTPUT_NAME "AvraGain"
        SOURCES
            plugins/AvraGain/AvraGainStandalone.cpp
    )
    target_include_directories(AvraGain_Standalone PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins/AvraGain
    )
endif()



# MasteringProcessor (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/MasteringProcessor/MasteringProcessorClap.cpp")
avra_add_clap_plugin(AvraMasteringProcessor_Clap
    OUTPUT_NAME "MasteringProcessor"
    SOURCES
        plugins/MasteringProcessor/MasteringProcessorClap.cpp
)
target_include_directories(AvraMasteringProcessor_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/MasteringProcessor
)

avra_add_vst3_plugin(AvraMasteringProcessor_Vst3
    OUTPUT_NAME "MasteringProcessor"
    SOURCES
        plugins/MasteringProcessor/MasteringProcessorVst3.cpp
)
target_include_directories(AvraMasteringProcessor_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/MasteringProcessor
)

avra_add_standalone_plugin(AvraMasteringProcessor_Standalone
    OUTPUT_NAME "MasteringProcessor"
    SOURCES
        plugins/MasteringProcessor/MasteringProcessorStandalone.cpp
)
target_include_directories(AvraMasteringProcessor_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/MasteringProcessor
)
endif()


# DrumBusMaster (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/DrumBusMaster/DrumBusMasterClap.cpp")
avra_add_clap_plugin(AvraDrumBusMaster_Clap
    OUTPUT_NAME "DrumBusMaster"
    SOURCES
        plugins/DrumBusMaster/DrumBusMasterClap.cpp
)
target_include_directories(AvraDrumBusMaster_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DrumBusMaster
)

avra_add_vst3_plugin(AvraDrumBusMaster_Vst3
    OUTPUT_NAME "DrumBusMaster"
    SOURCES
        plugins/DrumBusMaster/DrumBusMasterVst3.cpp
)
target_include_directories(AvraDrumBusMaster_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DrumBusMaster
)

avra_add_standalone_plugin(AvraDrumBusMaster_Standalone
    OUTPUT_NAME "DrumBusMaster"
    SOURCES
        plugins/DrumBusMaster/DrumBusMasterStandalone.cpp
)
target_include_directories(AvraDrumBusMaster_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DrumBusMaster
)
endif()


# BassStudioChain (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/BassStudioChain/BassStudioChainClap.cpp")
avra_add_clap_plugin(AvraBassStudioChain_Clap
    OUTPUT_NAME "BassStudioChain"
    SOURCES
        plugins/BassStudioChain/BassStudioChainClap.cpp
)
target_include_directories(AvraBassStudioChain_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/BassStudioChain
)

avra_add_vst3_plugin(AvraBassStudioChain_Vst3
    OUTPUT_NAME "BassStudioChain"
    SOURCES
        plugins/BassStudioChain/BassStudioChainVst3.cpp
)
target_include_directories(AvraBassStudioChain_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/BassStudioChain
)

avra_add_standalone_plugin(AvraBassStudioChain_Standalone
    OUTPUT_NAME "BassStudioChain"
    SOURCES
        plugins/BassStudioChain/BassStudioChainStandalone.cpp
)
target_include_directories(AvraBassStudioChain_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/BassStudioChain
)
endif()


# CleanGuitarStudio (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanGuitarStudio/CleanGuitarStudioClap.cpp")
avra_add_clap_plugin(AvraCleanGuitarStudio_Clap
    OUTPUT_NAME "CleanGuitarStudio"
    SOURCES
        plugins/CleanGuitarStudio/CleanGuitarStudioClap.cpp
)
target_include_directories(AvraCleanGuitarStudio_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanGuitarStudio
)

avra_add_vst3_plugin(AvraCleanGuitarStudio_Vst3
    OUTPUT_NAME "CleanGuitarStudio"
    SOURCES
        plugins/CleanGuitarStudio/CleanGuitarStudioVst3.cpp
)
target_include_directories(AvraCleanGuitarStudio_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanGuitarStudio
)

avra_add_standalone_plugin(AvraCleanGuitarStudio_Standalone
    OUTPUT_NAME "CleanGuitarStudio"
    SOURCES
        plugins/CleanGuitarStudio/CleanGuitarStudioStandalone.cpp
)
target_include_directories(AvraCleanGuitarStudio_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/CleanGuitarStudio
)
endif()


# DistGuitarStudio (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/DistGuitarStudio/DistGuitarStudioClap.cpp")
avra_add_clap_plugin(AvraDistGuitarStudio_Clap
    OUTPUT_NAME "DistGuitarStudio"
    SOURCES
        plugins/DistGuitarStudio/DistGuitarStudioClap.cpp
)
target_include_directories(AvraDistGuitarStudio_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DistGuitarStudio
)

avra_add_vst3_plugin(AvraDistGuitarStudio_Vst3
    OUTPUT_NAME "DistGuitarStudio"
    SOURCES
        plugins/DistGuitarStudio/DistGuitarStudioVst3.cpp
)
target_include_directories(AvraDistGuitarStudio_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DistGuitarStudio
)

avra_add_standalone_plugin(AvraDistGuitarStudio_Standalone
    OUTPUT_NAME "DistGuitarStudio"
    SOURCES
        plugins/DistGuitarStudio/DistGuitarStudioStandalone.cpp
)
target_include_directories(AvraDistGuitarStudio_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/DistGuitarStudio
)
endif()


# StudioVocalChain (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/StudioVocalChain/StudioVocalChainClap.cpp")
avra_add_clap_plugin(AvraStudioVocalChain_Clap
    OUTPUT_NAME "StudioVocalChain"
    SOURCES
        plugins/StudioVocalChain/StudioVocalChainClap.cpp
)
target_include_directories(AvraStudioVocalChain_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/StudioVocalChain
)

avra_add_vst3_plugin(AvraStudioVocalChain_Vst3
    OUTPUT_NAME "StudioVocalChain"
    SOURCES
        plugins/StudioVocalChain/StudioVocalChainVst3.cpp
)
target_include_directories(AvraStudioVocalChain_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/StudioVocalChain
)

avra_add_standalone_plugin(AvraStudioVocalChain_Standalone
    OUTPUT_NAME "StudioVocalChain"
    SOURCES
        plugins/StudioVocalChain/StudioVocalChainStandalone.cpp
)
target_include_directories(AvraStudioVocalChain_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/StudioVocalChain
)
endif()


# SynthKeysStudio (CLAP + VST3 + Standalone .exe — Generated by Avra Studio)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugins/SynthKeysStudio/SynthKeysStudioClap.cpp")
avra_add_clap_plugin(AvraSynthKeysStudio_Clap
    OUTPUT_NAME "SynthKeysStudio"
    SOURCES
        plugins/SynthKeysStudio/SynthKeysStudioClap.cpp
)
target_include_directories(AvraSynthKeysStudio_Clap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/SynthKeysStudio
)

avra_add_vst3_plugin(AvraSynthKeysStudio_Vst3
    OUTPUT_NAME "SynthKeysStudio"
    SOURCES
        plugins/SynthKeysStudio/SynthKeysStudioVst3.cpp
)
target_include_directories(AvraSynthKeysStudio_Vst3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/SynthKeysStudio
)

avra_add_standalone_plugin(AvraSynthKeysStudio_Standalone
    OUTPUT_NAME "SynthKeysStudio"
    SOURCES
        plugins/SynthKeysStudio/SynthKeysStudioStandalone.cpp
)
target_include_directories(AvraSynthKeysStudio_Standalone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/plugins/SynthKeysStudio
)
endif()



# Avra Studio Desktop Application (Avra.exe)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/studio/main.cpp")
    set(AVRA_STUDIO_RC "")
    if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/avra_studio.rc")
        set(AVRA_STUDIO_RC "${CMAKE_CURRENT_SOURCE_DIR}/assets/avra_studio.rc")
    endif()
    add_executable(Avra_Studio WIN32
        studio/main.cpp
        ${AVRA_STUDIO_RC}
    )

    if(WIN32)
        set_target_properties(Avra_Studio PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()

    set_target_properties(Avra_Studio PROPERTIES
        OUTPUT_NAME "Avra"
    )

    target_include_directories(Avra_Studio PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/studio
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )

    target_link_libraries(Avra_Studio PRIVATE Avra::Core)

    add_custom_command(TARGET Avra_Studio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/Debug"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Avra_Studio>" "${CMAKE_CURRENT_SOURCE_DIR}/bin/Avra.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Avra_Studio>" "${CMAKE_CURRENT_SOURCE_DIR}/Avra.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Avra_Studio>" "${CMAKE_CURRENT_BINARY_DIR}/Debug/Avra.exe"
        COMMENT "Syncing Avra.exe to bin/Avra.exe, root Avra.exe, and build/Debug/Avra.exe"
    )
endif()

# Avra Core Test Suite (Development Workspace Only)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestCore.cpp")
    add_executable(Avra_Tests
        tests/TestCore.cpp
    )

    target_include_directories(Avra_Tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )

    target_link_libraries(Avra_Tests PRIVATE Avra::Core)

    # CLAP Host Dynamic Loading Validation Harness
    add_executable(Avra_Clap_Host_Test
        tests/TestClapHost.cpp
    )
    target_link_libraries(Avra_Clap_Host_Test PRIVATE Avra::Core)
    if(TARGET AvraGain_Clap)
        add_dependencies(Avra_Clap_Host_Test AvraGain_Clap)
    endif()

    # Commercial DSP & 4-Tier UI Integrity Verification Harness
    add_executable(dsp_integrity_harness
        tests/dsp_integrity_harness.cpp
    )
    target_include_directories(dsp_integrity_harness PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/studio
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )
    target_link_libraries(dsp_integrity_harness PRIVATE Avra::Core)

    enable_testing()
    add_test(NAME Avra_Core_Tests COMMAND Avra_Tests)
    if(TARGET AvraGain_Clap)
        add_test(NAME Avra_Clap_Host_Tests COMMAND Avra_Clap_Host_Test $<TARGET_FILE:AvraGain_Clap>)
    endif()
    add_test(NAME Avra_DSP_Integrity_Harness COMMAND dsp_integrity_harness)

    # Forensic Anti-Cookie-Cutter DSP Verification Suite
    add_executable(avra_forensic_dsp_audit
        tests/avra_forensic_dsp_audit.cpp
    )
    target_include_directories(avra_forensic_dsp_audit PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/studio
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/GainPlugin
    )
    target_link_libraries(avra_forensic_dsp_audit PRIVATE Avra::Core)
    add_test(NAME Avra_Forensic_DSP_Audit COMMAND avra_forensic_dsp_audit)

    # Control-Spec & Clone-Body Audit (report mode; pass --strict to gate)
    add_executable(avra_control_spec_audit
        tests/avra_control_spec_audit.cpp
    )
    target_link_libraries(avra_control_spec_audit PRIVATE Avra::Core)
    add_test(NAME Avra_Control_Spec_Audit COMMAND avra_control_spec_audit)

    # Per-module behaviour tests (each control does what its label says, measured)
    add_executable(avra_module_behavior_tests
        tests/avra_module_behavior_tests.cpp
    )
    target_link_libraries(avra_module_behavior_tests PRIVATE Avra::Core)
    add_test(NAME Avra_Module_Behavior_Tests COMMAND avra_module_behavior_tests)
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/installer/AvraSetupMain.cpp")
    add_executable(Avra_Setup_Stub WIN32
        installer/AvraSetupMain.cpp
        assets/avra_studio.rc
    )
    set_target_properties(Avra_Setup_Stub PROPERTIES
        OUTPUT_NAME "Avra_Setup_Stub"
    )
endif()
