cmake_minimum_required(VERSION 3.22)

project(AtelierDSPPlugins VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# ---------------------------------------------------------------------------
# Plugin 1: SlapTape Echo (slap / tape delay)
# ---------------------------------------------------------------------------
juce_add_plugin(SlapTapeEcho
    COMPANY_NAME "AtelierDSP"
    PLUGIN_MANUFACTURER_CODE Atdp
    PLUGIN_CODE Slpt
    FORMATS VST3
    PRODUCT_NAME "SlapTape Echo"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Delay"
)

target_sources(SlapTapeEcho
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(SlapTapeEcho
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SlapTapeEcho
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Plugin 2: DeciCrush (bit crusher / sample-rate reducer)
# ---------------------------------------------------------------------------
juce_add_plugin(DeciCrush
    COMPANY_NAME "AtelierDSP"
    PLUGIN_MANUFACTURER_CODE Atdp
    PLUGIN_CODE Dcsh
    FORMATS VST3
    PRODUCT_NAME "DeciCrush"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Fx" "Distortion"
)

target_sources(DeciCrush
    PRIVATE
        Source/DeciCrush/DeciProcessor.cpp
        Source/DeciCrush/DeciEditor.cpp
)

target_compile_definitions(DeciCrush
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DeciCrush
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
