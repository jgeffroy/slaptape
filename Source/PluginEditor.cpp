#include "PluginEditor.h"

SlapTapeEchoAudioProcessorEditor::SlapTapeEchoAudioProcessorEditor (SlapTapeEchoAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    titleLabel.setText ("SlapTape Echo", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    setupKnob (mixKnob,      SlapTapeEchoAudioProcessor::paramMix,      "Mix");
    setupKnob (timeKnob,     SlapTapeEchoAudioProcessor::paramTime,     "Time");
    setupKnob (feedbackKnob, SlapTapeEchoAudioProcessor::paramFeedback, "Feedback");
    setupKnob (toneKnob,     SlapTapeEchoAudioProcessor::paramTone,     "Tone");
    setupKnob (hpfKnob,      SlapTapeEchoAudioProcessor::paramHpf,      "Low Cut");
    setupKnob (modRateKnob,  SlapTapeEchoAudioProcessor::paramModRate,  "Mod Rate");
    setupKnob (modDepthKnob, SlapTapeEchoAudioProcessor::paramModDepth, "Mod Depth");
    setupKnob (driveKnob,    SlapTapeEchoAudioProcessor::paramDrive,    "Drive");

    addAndMakeVisible (slapButton);
    slapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.apvts, SlapTapeEchoAudioProcessor::paramSlap, slapButton);

    setSize (620, 380);
}

void SlapTapeEchoAudioProcessorEditor::setupKnob (KnobWithLabel& knob, const juce::String& paramId, const juce::String& text)
{
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (knob.slider);

    knob.label.setText (text, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, paramId, knob.slider);
}

void SlapTapeEchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2b2d31));
}

void SlapTapeEchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    titleLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);

    auto slapArea = area.removeFromTop (24);
    slapButton.setBounds (slapArea.withSizeKeepingCentre (140, 24));
    area.removeFromTop (8);

    auto layoutRow = [] (juce::Rectangle<int> row, std::initializer_list<KnobWithLabel*> knobs)
    {
        int w = row.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
        {
            auto slot = row.removeFromLeft (w);
            k->label.setBounds (slot.removeFromTop (18));
            k->slider.setBounds (slot);
        }
    };

    auto row1 = area.removeFromTop (area.getHeight() / 2);
    layoutRow (row1, { &mixKnob, &timeKnob, &feedbackKnob, &toneKnob });
    layoutRow (area, { &hpfKnob, &modRateKnob, &modDepthKnob, &driveKnob });
}
