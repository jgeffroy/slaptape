#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DeciProcessor.h"
#include "../Common/UserPresets.h"

class DeciCrushEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit DeciCrushEditor (DeciCrushProcessor&);
    ~DeciCrushEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

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

    void timerCallback() override;
    void registerLearnTarget (juce::Component&, const char* paramId);
    void setupKnob   (Knob&, const char* paramId, const char* text);
    void setupToggle (Toggle&, const char* paramId, const char* text);
    void setupCombo  (Combo&, const char* paramId, const char* text, const juce::StringArray& items);
    static void placeKnob (Knob&, juce::Rectangle<int>);

    DeciCrushProcessor& processorRef;

    Knob preamp, bits, dither, freq, jitter, approxDev, imagesShift, cutoff, reso, mix, output;
    Toggle quantOn, dcShift, resOn, approxOn, imagesOn;
    Combo filterType, filterOrder;

    Combo quality, renderQ, inputMode, lfoShape, lfoNote;
    Knob lfoRate, lfoFreq, lfoBits, lfoJit;
    Toggle lfoOn, lfoSync;
    juce::ComboBox presetBox, sizeBox;
    atelier::PresetBar presetBar;
    std::vector<std::pair<juce::Component*, juce::String>> learnTargets;
    juce::String learnHint;
    juce::Label titleLabel;

    juce::Rectangle<int> panelInput, panelQuant, panelRes, panelFilter, panelMaster, panelLfo, panelOptions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeciCrushEditor)
};
