cmake_minimum_required(VERSION 3.22)

project(LUFSMeter VERSION 1.2.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Dependencies (pinned git submodules)
# ---------------------------------------------------------------------------
# JUCE 8.0.15 (AGPLv3) - provides VST3 support and the plugin framework.
add_subdirectory(third_party/JUCE)

# libebur128 v1.2.6 (MIT) - EBU R128 / ITU-R BS.1770 loudness measurement.
# We compile the single C source directly instead of adding the library's own
# CMake project: libebur128 declares cmake_minimum_required(VERSION 2.8.12),
# which is no longer accepted by modern CMake, and its test target is not needed.
# The bundled ebur128/queue/sys/queue.h supplies <sys/queue.h> on MSVC.
add_library(ebur128 STATIC third_party/libebur128/ebur128/ebur128.c)
set_source_files_properties(third_party/libebur128/ebur128/ebur128.c
    PROPERTIES LANGUAGE C)
target_include_directories(ebur128 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libebur128/ebur128
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libebur128/ebur128/queue)
if(MSVC)
    target_compile_definitions(ebur128 PRIVATE _USE_MATH_DEFINES)
    target_compile_options(ebur128 PRIVATE /W3)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(LUFSMeter
    COMPANY_NAME              "SafroAudio"
    PLUGIN_MANUFACTURER_CODE  Safr
    PLUGIN_CODE               Lufs
    PRODUCT_NAME              "LUFS Meter"
    DESCRIPTION               "Accurate Loudness and True Peak meter (EBU R128 / ITU-R BS.1770)"
    VERSION                   1.2.0
    FORMATS                   VST3
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Analyzer")

target_sources(LUFSMeter PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/MeterComponent.cpp
    src/MeterTheme.cpp
    src/IconButton.cpp
    src/SettingsContent.cpp
    src/LoudnessMeter.cpp
    src/MeasurementEngine.cpp)

target_include_directories(LUFSMeter PRIVATE src)

target_compile_definitions(LUFSMeter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(LUFSMeter
    PRIVATE
        ebur128
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Offscreen UI preview tool (development only, off by default)
# ---------------------------------------------------------------------------
option(LUFSMETER_BUILD_PREVIEW "Build the offscreen UI preview tool" OFF)

if(LUFSMETER_BUILD_PREVIEW)
    add_subdirectory(tools/preview)
endif()
