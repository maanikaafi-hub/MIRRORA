cmake_minimum_required(VERSION 3.22)

project(MIRRORA VERSION 1.1.0 LANGUAGES C CXX)

#===============================================================================
# JUCE
#   Look for JUCE in this order:
#     1. $JUCE_DIR
#     2. ../JUCE  or  ./JUCE
#     3. download it automatically with FetchContent
#===============================================================================
set(MIRRORA_JUCE_TAG "8.0.8" CACHE STRING "JUCE version to fetch when no local copy is found")

if(DEFINED ENV{JUCE_DIR} AND EXISTS "$ENV{JUCE_DIR}/CMakeLists.txt")
    message(STATUS "MIRRORA: using JUCE from \$JUCE_DIR ($ENV{JUCE_DIR})")
    add_subdirectory("$ENV{JUCE_DIR}" JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
    message(STATUS "MIRRORA: using JUCE from ../JUCE")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(STATUS "MIRRORA: using JUCE from ./JUCE")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/JUCE" JUCE)
else()
    message(STATUS "MIRRORA: JUCE not found locally, fetching ${MIRRORA_JUCE_TAG}")
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${MIRRORA_JUCE_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

#===============================================================================
# The plug-in
#   VST3 everywhere, AU only on macOS (AU cannot be built on Windows/Linux).
#===============================================================================
set(MIRRORA_FORMATS VST3)

if(APPLE)
    list(APPEND MIRRORA_FORMATS AU)
endif()

juce_add_plugin(MIRRORA
    COMPANY_NAME              "Plugin Store"
    BUNDLE_ID                 "com.pluginstore.mirrora"
    PLUGIN_MANUFACTURER_CODE  PlSt
    PLUGIN_CODE               Mrra
    FORMATS                   ${MIRRORA_FORMATS}
    PRODUCT_NAME              "MIRRORA"
    DESCRIPTION               "Real-time reverse effect"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Fx
    AU_MAIN_TYPE              kAudioUnitType_Effect)

juce_generate_juce_header(MIRRORA)

target_sources(MIRRORA PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/MirroraTheme.h
    Source/MirroraControls.cpp
    Source/MirroraControls.h
    Source/WaveformDisplay.cpp
    Source/WaveformDisplay.h)

target_compile_features(MIRRORA PRIVATE cxx_std_17)

target_compile_definitions(MIRRORA PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(MIRRORA
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

#===============================================================================
# Optional: off-screen editor renderer used by the "GUI preview" workflow.
#   cmake -DMIRRORA_BUILD_PREVIEW=ON ...
#===============================================================================
option(MIRRORA_BUILD_PREVIEW "Build the off-screen GUI renderer" OFF)

if(MIRRORA_BUILD_PREVIEW)
    juce_add_console_app(MIRRORA_Preview PRODUCT_NAME "MIRRORA_Preview")

    juce_generate_juce_header(MIRRORA_Preview)

    target_sources(MIRRORA_Preview PRIVATE
        tools/preview/Preview.cpp
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/MirroraTheme.h
        Source/MirroraControls.cpp
        Source/MirroraControls.h
        Source/WaveformDisplay.cpp
        Source/WaveformDisplay.h)

    target_include_directories(MIRRORA_Preview PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

    target_compile_features(MIRRORA_Preview PRIVATE cxx_std_17)

    target_compile_definitions(MIRRORA_Preview PRIVATE
        JucePlugin_Name="MIRRORA"
        JucePlugin_VersionString="1.1.0"
        JucePlugin_VersionCode=0x10100
        JucePlugin_Desc="Real-time reverse effect"
        JucePlugin_Manufacturer="Plugin Store"
        JucePlugin_ManufacturerCode=0x506c5374
        JucePlugin_PluginCode=0x4d727261
        JucePlugin_IsSynth=0
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_EditorRequiresKeyboardFocus=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1)

    target_link_libraries(MIRRORA_Preview
        PRIVATE
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_gui_basics)
endif()

#===============================================================================
# Optional: offline engine self-test (used by CI to catch DSP regressions).
#   cmake -DMIRRORA_BUILD_TESTS=ON ...
#===============================================================================
option(MIRRORA_BUILD_TESTS "Build the offline engine self-test" OFF)

if(MIRRORA_BUILD_TESTS)
    juce_add_console_app(MIRRORA_AudioTest PRODUCT_NAME "MIRRORA_AudioTest")

    juce_generate_juce_header(MIRRORA_AudioTest)

    target_sources(MIRRORA_AudioTest PRIVATE
        tools/audiotest/AudioTest.cpp
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/MirroraTheme.h
        Source/MirroraControls.cpp
        Source/MirroraControls.h
        Source/WaveformDisplay.cpp
        Source/WaveformDisplay.h)

    target_include_directories(MIRRORA_AudioTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

    target_compile_features(MIRRORA_AudioTest PRIVATE cxx_std_17)

    target_compile_definitions(MIRRORA_AudioTest PRIVATE
        JucePlugin_Name="MIRRORA"
        JucePlugin_VersionString="1.1.0"
        JucePlugin_VersionCode=0x10100
        JucePlugin_Desc="Real-time reverse effect"
        JucePlugin_Manufacturer="Plugin Store"
        JucePlugin_ManufacturerCode=0x506c5374
        JucePlugin_PluginCode=0x4d727261
        JucePlugin_IsSynth=0
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_EditorRequiresKeyboardFocus=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1)

    target_link_libraries(MIRRORA_AudioTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_gui_basics)
endif()
