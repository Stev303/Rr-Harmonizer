cmake_minimum_required(VERSION 3.22)
project(RrHarmonizer VERSION 1.0.0)

# VST3 only: do not add VST2 targets or legacy SDK support.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(RrHarmonizer
    COMPANY_NAME "Rr Audio"
    BUNDLE_ID "com.rraudio.rrharmonizer"
    PLUGIN_MANUFACTURER_CODE RrAu
    PLUGIN_CODE RrH1
    PRODUCT_NAME "Rr Harmonizer"
    DESCRIPTION "Real-time scale-aware vocal harmonizer"
    FORMATS VST3
    VST3_CATEGORIES Fx
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)
juce_generate_juce_header(RrHarmonizer)
target_sources(RrHarmonizer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)
target_compile_features(RrHarmonizer PRIVATE cxx_std_17)
target_compile_definitions(RrHarmonizer PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(RrHarmonizer PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags
)
