cmake_minimum_required(VERSION 3.25)

project(ChiengSaturation VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE lives in ./JUCE (shallow clone of tag 8.0.13)
add_subdirectory(JUCE)

juce_add_plugin(ChiengSaturation
    COMPANY_NAME            "Chieng"
    PRODUCT_NAME            "Chieng Saturation"
    BUNDLE_ID               com.chieng.chiengsaturation
    PLUGIN_MANUFACTURER_CODE Chng     # 4 chars, >=1 uppercase
    PLUGIN_CODE             Sat1       # 4 chars, >=1 uppercase
    FORMATS                 AU Standalone   # AU for Logic; Standalone for quick testing
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE)       # auto-installs to ~/Library/Audio/Plug-Ins/Components

juce_generate_juce_header(ChiengSaturation)

target_sources(ChiengSaturation PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ChiengSaturation PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ChiengSaturation
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
