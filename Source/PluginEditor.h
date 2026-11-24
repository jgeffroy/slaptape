#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class SlapTapeEchoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SlapTapeEchoAudioProcessorEditor (SlapTapeEchoAudioProcessor&);
    ~SlapTapeEchoAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobWithLabel
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    SlapTapeEchoAudioProcessor& processorRef;

    KnobWithLabel mixKnob, timeKnob, feedbackKnob, toneKnob, hpfKnob, modRateKnob, modDepthKnob, driveKnob;

    juce::ToggleButton slapButton { "Slap Mode" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> slapAttachment;

    juce::Label titleLabel;

    void setupKnob (KnobWithLabel& knob, const juce::String& paramId, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapTapeEchoAudioProcessorEditor)
};
