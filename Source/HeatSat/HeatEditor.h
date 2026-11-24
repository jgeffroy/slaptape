#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "HeatProcessor.h"
#include "../Common/UserPresets.h"

class HeatSatEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit HeatSatEditor (HeatSatProcessor&);
    ~HeatSatEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void setupKnob (Knob&, const char* paramId, const char* text);
    void setupToggle (Toggle&, const char* paramId, const char* text);
    void updateStyleButtons();
    void timerCallback() override;

    HeatSatProcessor& processorRef;

    Knob drive, lowCut, tone, highCut, output, mix;
    Toggle punish, autoGain, thump, steep;

    std::array<juce::TextButton, 5> styleButtons;
    std::unique_ptr<juce::ParameterAttachment> styleAttachment;

    juce::ComboBox factoryBox, stereoBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stereoAttachment;
    juce::Rectangle<int> meterArea, styleLabelArea;
    float shownDb = -60.0f;
    atelier::PresetBar presetBar;
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeatSatEditor)
};
