#include "HeatEditor.h"

namespace
{
const juce::Colour kBg   (0xff1d1a18);
const juce::Colour kHot  (0xffe8743b);
const juce::Colour kText (0xffe9e2da);
}

HeatSatEditor::HeatSatEditor (HeatSatProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), presetBar (p.apvts, "HeatSat")
{
    setupKnob (drive,   "drive",   "DRIVE");
    setupKnob (lowCut,  "lowcut",  "LOW CUT");
    setupKnob (tone,    "tone",    "TONE");
    setupKnob (highCut, "highcut", "HIGH CUT");
    setupKnob (output,  "output",  "OUTPUT");
    setupKnob (mix,     "mix",     "MIX");

    setupToggle (punish,   "punish", "PUNISH");
    setupToggle (autoGain, "auto",   "AUTO GAIN");
    setupToggle (thump,    "thump",  "THUMP");
    setupToggle (steep,    "steep",  "STEEP");

    static const char* names[] = { "A", "E", "N", "T", "P" };
    auto* styleParam = processorRef.apvts.getParameter ("style");
    styleAttachment = std::make_unique<juce::ParameterAttachment> (*styleParam, [this] (float) { updateStyleButtons(); });
    for (size_t i = 0; i < styleButtons.size(); ++i)
    {
        auto& b = styleButtons[i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (false);
        b.onClick = [this, i] { styleAttachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    updateStyleButtons();
    styleAttachment->sendInitialUpdate();

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        factoryBox.addItem (processorRef.getProgramName (i), i + 1);
    factoryBox.setTextWhenNothingSelected ("Factory presets");
    factoryBox.onChange = [this] { processorRef.setCurrentProgram (factoryBox.getSelectedId() - 1); };
    addAndMakeVisible (factoryBox);
    addAndMakeVisible (presetBar);

    stereoBox.addItemList ({ "Stereo", "Mid only", "Side only" }, 1);
    addAndMakeVisible (stereoBox);
    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processorRef.apvts, "stereo", stereoBox);

    titleLabel.setText ("HEATSAT", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, kHot);
    addAndMakeVisible (titleLabel);

    setSize (720, 460);
    startTimerHz (30);
}

void HeatSatEditor::setupKnob (Knob& k, const char* id, const char* text)
{
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, kHot);
    k.slider.setColour (juce::Slider::thumbColourId, kText);
    k.slider.setColour (juce::Slider::textBoxTextColourId, kText);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, kText);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, id, k.slider);
}

void HeatSatEditor::setupToggle (Toggle& t, const char* id, const char* text)
{
    t.button.setButtonText (text);
    t.button.setColour (juce::ToggleButton::textColourId, kText);
    t.button.setColour (juce::ToggleButton::tickColourId, kHot);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processorRef.apvts, id, t.button);
}

void HeatSatEditor::updateStyleButtons()
{
    const int current = (int) std::lround (processorRef.apvts.getRawParameterValue ("style")->load());
    for (size_t i = 0; i < styleButtons.size(); ++i)
    {
        const bool on = (int) i == current;
        styleButtons[i].setColour (juce::TextButton::buttonColourId, on ? kHot : juce::Colour (0xff3a342f));
        styleButtons[i].setColour (juce::TextButton::textColourOffId, on ? juce::Colours::black : kText);
        styleButtons[i].repaint();
    }
}

void HeatSatEditor::timerCallback()
{
    const float db = processorRef.getAttitudeDb();
    if (std::abs (db - shownDb) > 0.2f)
    {
        shownDb = db;
        repaint (meterArea);
    }
}

void HeatSatEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    g.setColour (juce::Colour (0xff2b2623));
    g.fillRoundedRectangle (getLocalBounds().reduced (10).withTrimmedTop (50).toFloat(), 8.0f);
    g.setColour (kText.withAlpha (0.6f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("STYLE", styleLabelArea, juce::Justification::centredLeft);

    // Attitude meter: how hard the signal hits the saturation stage
    if (! meterArea.isEmpty())
    {
        auto m = meterArea.toFloat();
        g.setColour (kText.withAlpha (0.6f));
        g.drawText ("ATTITUDE", meterArea.withWidth (80), juce::Justification::centredLeft);
        auto bar = m.withTrimmedLeft (84.0f).reduced (0.0f, 6.0f);
        g.setColour (juce::Colour (0xff3a342f));
        g.fillRoundedRectangle (bar, 3.0f);
        const float norm = juce::jlimit (0.0f, 1.0f, (shownDb + 40.0f) / 46.0f); // -40..+6 dB
        auto fill = bar.withWidth (bar.getWidth() * norm);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6fb36a), bar.getX(), 0.0f,
                                                 juce::Colour (0xffe03b2e), bar.getRight(), 0.0f, false));
        g.fillRoundedRectangle (fill, 3.0f);
        g.setColour (kText.withAlpha (0.8f));
        const float zeroX = bar.getX() + bar.getWidth() * (40.0f - 18.0f) / 46.0f; // -18 dBFS RMS ~ "0 VU"
        g.drawVerticalLine ((int) zeroX, bar.getY() - 3.0f, bar.getBottom() + 3.0f);
    }
}

void HeatSatEditor::resized()
{
    auto r = getLocalBounds().reduced (14);

    auto top = r.removeFromTop (36);
    titleLabel.setBounds (top.removeFromLeft (170));
    presetBar.setBounds (top.removeFromRight (230));
    top.removeFromRight (8);
    factoryBox.setBounds (top.removeFromRight (180).reduced (0, 4));

    r.removeFromTop (22);
    {
        auto meterRow = r.removeFromTop (30).reduced (12, 2);
        stereoBox.setBounds (meterRow.removeFromRight (120).reduced (0, 2));
        meterRow.removeFromRight (12);
        meterArea = meterRow;
    }
    auto styleRow = r.removeFromTop (44).reduced (6, 4);
    styleLabelArea = styleRow.removeFromLeft (60);
    const int bw = 52;
    for (auto& b : styleButtons)
    {
        b.setBounds (styleRow.removeFromLeft (bw));
        styleRow.removeFromLeft (6);
    }

    auto toggles = styleRow.removeFromRight (360);
    const int tw = toggles.getWidth() / 4;
    punish.button.setBounds   (toggles.removeFromLeft (tw));
    autoGain.button.setBounds (toggles.removeFromLeft (tw));
    thump.button.setBounds    (toggles.removeFromLeft (tw));
    steep.button.setBounds    (toggles.removeFromLeft (tw));

    auto row = r.reduced (6, 8);
    const int w = row.getWidth() / 6;
    Knob* knobs[] = { &drive, &lowCut, &tone, &highCut, &output, &mix };
    for (auto* k : knobs)
    {
        auto cell = row.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell.reduced (4));
    }
}
