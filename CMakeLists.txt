cmake_minimum_required(VERSION 3.22)
project(MIRRORA VERSION 0.1.0)
if(DEFINED ENV{JUCE_DIR})
  add_subdirectory("$ENV{JUCE_DIR}" JUCE)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" JUCE)
else()
  message(FATAL_ERROR "JUCE not found. Set JUCE_DIR to your JUCE source folder.")
endif()
juce_add_plugin(MIRRORA
  COMPANY_NAME "Plugin Store"
  BUNDLE_ID "com.pluginstore.mirrora"
  PLUGIN_MANUFACTURER_CODE PlSt PLUGIN_CODE Mrra
  FORMATS VST3 AU PRODUCT_NAME "MIRRORA"
  IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(MIRRORA)
target_sources(MIRRORA PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_features(MIRRORA PRIVATE cxx_std_17)
target_compile_definitions(MIRRORA PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MIRRORA PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
