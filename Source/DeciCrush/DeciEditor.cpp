#include "DeciEditor.h"

DeciCrushEditor::DeciCrushEditor (DeciCrushProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    titleLabel.setText ("DeciCrush", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetBox.addItem (processorRef.getProgramName (i), i + 1);
    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { processorRef.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setupKnob (preamp,      "preamp", "Preamp");

    setupToggle (quantOn,   "qOn",    "Quantizer");
    setupKnob   (bits,      "bits",   "Resolution");
    setupKnob   (dither,    "dither", "Dithering");
    setupToggle (dcShift,   "dc",     "DC Shift");

    setupToggle (resOn,     "rOn",    "Resampler");
    setupKnob   (freq,      "freq",   "Frequency");
    setupKnob   (jitter,    "jitter", "Jitter");
    setupToggle (approxOn,  "aOn",    "Approx. Filter");
    setupKnob   (approxDev, "aDev",   "Frq. Deviation");
    setupToggle (imagesOn,  "iOn",    "Images Filter");
    setupKnob   (imagesShift, "iShift", "Frq. Shift");

    setupCombo (filterType,  "fType",  "Type",  { "Off", "LP", "BP", "HP", "BR" });
    setupCombo (filterOrder, "fOrder", "Order", { "Pre", "Post" });
    setupKnob  (cutoff,      "fCut",   "Cutoff");
    setupKnob  (reso,        "fReso",  "Reso / Width");

    setupKnob (mix,    "mix", "FX");
    setupKnob (output, "out", "Output");

    setSize (960, 420);
}

void DeciCrushEditor::setupKnob (Knob& k, const char* paramId, const char* text)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    addAndMakeVisible (k.slider);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, paramId, k.slider);
}

void DeciCrushEditor::setupToggle (Toggle& t, const char* paramId, const char* text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processorRef.apvts, paramId, t.button);
}

void DeciCrushEditor::setupCombo (Combo& c, const char* paramId, const char* text, const juce::StringArray& items)
{
    c.box.addItemList (items, 1);
    addAndMakeVisible (c.box);
    c.label.setText (text, juce::dontSendNotification);
    c.label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (c.label);
    c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processorRef.apvts, paramId, c.box);
}

void DeciCrushEditor::placeKnob (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (16));
    k.slider.setBounds (r);
}

void DeciCrushEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff25272b));

    auto panel = [&g] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff32353b));
        g.fillRoundedRectangle (r.toFloat(), 8.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (title, r.withHeight (22).reduced (10, 0), juce::Justification::centredLeft);
    };

    panel (panelInput,  "INPUT");
    panel (panelQuant,  "QUANTIZER");
    panel (panelRes,    "RESAMPLER");
    panel (panelFilter, "FILTER");
    panel (panelMaster, "MASTER");
}

void DeciCrushEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto header = area.removeFromTop (34);
    titleLabel.setBounds (header.removeFromLeft (200));
    presetBox.setBounds (header.removeFromRight (260).reduced (0, 4));
    area.removeFromTop (8);

    const int gap = 8;
    panelInput  = area.removeFromLeft (110);  area.removeFromLeft (gap);
    panelQuant  = area.removeFromLeft (200);  area.removeFromLeft (gap);
    panelRes    = area.removeFromLeft (330);  area.removeFromLeft (gap);
    panelFilter = area.removeFromLeft (200);  area.removeFromLeft (gap);
    panelMaster = area;

    // Input
    {
        auto r = panelInput.reduced (8); r.removeFromTop (28);
        placeKnob (preamp, r.removeFromTop (150));
    }

    // Quantizer
    {
        auto r = panelQuant.reduced (8); r.removeFromTop (22);
        quantOn.button.setBounds (r.removeFromTop (24));
        dcShift.button.setBounds (r.removeFromTop (24));
        r.removeFromTop (6);
        auto row = r.removeFromTop (150);
        placeKnob (bits,   row.removeFromLeft (row.getWidth() / 2));
        placeKnob (dither, row);
    }

    // Resampler
    {
        auto r = panelRes.reduced (8); r.removeFromTop (22);
        resOn.button.setBounds (r.removeFromTop (24));
        r.removeFromTop (4);
        auto top = r.removeFromTop (130);
        placeKnob (freq,   top.removeFromLeft (top.getWidth() / 2));
        placeKnob (jitter, top);

        auto bottom = r.removeFromTop (150);
        auto left = bottom.removeFromLeft (bottom.getWidth() / 2);
        approxOn.button.setBounds (left.removeFromTop (24));
        placeKnob (approxDev, left);
        imagesOn.button.setBounds (bottom.removeFromTop (24));
        placeKnob (imagesShift, bottom);
    }

    // Filter
    {
        auto r = panelFilter.reduced (8); r.removeFromTop (24);
        auto typeRow = r.removeFromTop (26);
        filterType.label.setBounds (typeRow.removeFromLeft (50));
        filterType.box.setBounds (typeRow);
        r.removeFromTop (4);
        auto orderRow = r.removeFromTop (26);
        filterOrder.label.setBounds (orderRow.removeFromLeft (50));
        filterOrder.box.setBounds (orderRow);
        r.removeFromTop (8);
        auto row = r.removeFromTop (150);
        placeKnob (cutoff, row.removeFromLeft (row.getWidth() / 2));
        placeKnob (reso,   row);
    }

    // Master
    {
        auto r = panelMaster.reduced (8); r.removeFromTop (28);
        placeKnob (mix,    r.removeFromTop (r.getHeight() / 2));
        placeKnob (output, r);
    }
}
