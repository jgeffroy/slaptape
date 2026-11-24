#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DeciProcessor.h"

class DeciCrushEditor : public juce::AudioProcessorEditor
{
public:
    explicit DeciCrushEditor (DeciCrushProcessor&);
    ~DeciCrushEditor() override = default;

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

    struct Combo
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void setupKnob   (Knob&, const char* paramId, const char* text);
    void setupToggle (Toggle&, const char* paramId, const char* text);
    void setupCombo  (Combo&, const char* paramId, const char* text, const juce::StringArray& items);
    static void placeKnob (Knob&, juce::Rectangle<int>);

    DeciCrushProcessor& processorRef;

    Knob preamp, bits, dither, freq, jitter, approxDev, imagesShift, cutoff, reso, mix, output;
    Toggle quantOn, dcShift, resOn, approxOn, imagesOn;
    Combo filterType, filterOrder;

    juce::ComboBox presetBox;
    juce::Label titleLabel;

    juce::Rectangle<int> panelInput, panelQuant, panelRes, panelFilter, panelMaster;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeciCrushEditor)
};
