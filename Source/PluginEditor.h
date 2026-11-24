#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Common/UserPresets.h"

class SlapTapeEchoAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit SlapTapeEchoAudioProcessorEditor (SlapTapeEchoAudioProcessor&);
    ~SlapTapeEchoAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum Kind { KnobK, ToggleK, ComboK };

    struct Control
    {
        juce::String id, label;
        Kind kind;
        int page;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::ComboBox> combo;
        juce::Label labelComp;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
        juce::Component* comp() const
        {
            if (slider) return slider.get();
            if (toggle) return toggle.get();
            return combo.get();
        }
    };

    void add (const char* id, const char* label, Kind kind, int page);
    void showPage (int page);
    void timerCallback() override;
    void updateEnabled();

    SlapTapeEchoAudioProcessor& processorRef;
    std::vector<std::unique_ptr<Control>> controls;

    juce::TextButton pageButtons[3] { juce::TextButton ("Main"), juce::TextButton ("Style Edit"), juce::TextButton ("Rhythm") };
    juce::TextButton tapButton { "Tap Tempo" };
    juce::Label titleLabel, bpmLabel;
    juce::ComboBox factoryBox;
    atelier::PresetBar userPresets;
    int currentPage = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapTapeEchoAudioProcessorEditor)
};
