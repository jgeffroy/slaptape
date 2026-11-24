#include "DeciEditor.h"

DeciCrushEditor::DeciCrushEditor (DeciCrushProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), presetBar (p.apvts, "DeciCrush")
{
    titleLabel.setText ("DeciCrush", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetBox.addItem (processorRef.getProgramName (i), i + 1);
    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected ("Factory presets");
    presetBox.setSelectedId (0, juce::dontSendNotification);
    presetBox.onChange = [this] { processorRef.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    addAndMakeVisible (presetBar);

    sizeBox.addItemList ({ "75%", "100%", "125%", "150%" }, 1);
    const float sc = processorRef.getGuiScale();
    sizeBox.setSelectedId (sc < 0.9f ? 1 : sc < 1.1f ? 2 : sc < 1.4f ? 3 : 4, juce::dontSendNotification);
    sizeBox.onChange = [this]
    {
        static const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        const float v = scales[juce::jlimit (0, 3, sizeBox.getSelectedId() - 1)];
        processorRef.setGuiScale (v);
        setScaleFactor (v);
    };
    addAndMakeVisible (sizeBox);

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

    setupCombo (quality,   "quality", "Quality", { "Draft", "Normal (2x)", "High (4x)", "Ultra (8x)" });
    setupCombo (renderQ,   "renderQ", "Render",  { "Same as real-time", "Draft", "Normal (2x)", "High (4x)", "Ultra (8x)" });
    setupCombo (inputMode, "input",   "Input",   { "Stereo", "Left only", "Right only", "Mono (L+R)" });

    setupToggle (lfoOn,   "lfoOn",   "LFO");
    setupToggle (lfoSync, "lfoSync", "Sync");
    setupCombo  (lfoShape, "lfoShape", "Shape", { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random" });
    setupCombo  (lfoNote,  "lfoNote",  "Note",  { "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32" });
    setupKnob   (lfoRate, "lfoRate", "Rate");
    setupKnob   (lfoFreq, "lfoFreq", "> Frequency");
    setupKnob   (lfoBits, "lfoBits", "> Resolution");
    setupKnob   (lfoJit,  "lfoJit",  "> Jitter");

    setupKnob (mix,    "mix", "FX");
    setupKnob (output, "out", "Output");

    setSize (960, 560);
    setScaleFactor (processorRef.getGuiScale());
    startTimerHz (4);
}

void DeciCrushEditor::registerLearnTarget (juce::Component& c, const char* paramId)
{
    learnTargets.push_back ({ &c, paramId });
    c.addMouseListener (this, false);
}

void DeciCrushEditor::mouseDown (const juce::MouseEvent& e)
{
    // right-click, or shift-click for trackpads without a secondary button
    if (! (e.mods.isPopupMenu() || e.mods.isShiftDown()))
        return;

    for (auto& t : learnTargets)
    {
        if (t.first != e.eventComponent)
            continue;

        const juce::String id = t.second;
        const int cc = processorRef.getMappedCc (id);
        juce::PopupMenu m;
        m.addItem (1, "MIDI Learn (move a controller now)");
        m.addItem (2, cc >= 0 ? "Clear MIDI mapping (CC " + juce::String (cc) + ")" : juce::String ("Clear MIDI mapping"), cc >= 0);
        m.showMenuAsync (juce::PopupMenu::Options(), [this, id] (int result)
        {
            if (result == 1) processorRef.startMidiLearn (id);
            if (result == 2) processorRef.clearMidiMapping (id);
        });
        return;
    }
}

void DeciCrushEditor::timerCallback()
{
    juce::String hint;
    for (auto& t : learnTargets)
        if (processorRef.isLearning (t.second))
            hint = "MIDI learn: move a controller to assign " + t.second;
    if (hint != learnHint)
    {
        learnHint = hint;
        repaint();
    }
}

void DeciCrushEditor::setupKnob (Knob& k, const char* paramId, const char* text)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    addAndMakeVisible (k.slider);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, paramId, k.slider);
    registerLearnTarget (k.slider, paramId);
}

void DeciCrushEditor::setupToggle (Toggle& t, const char* paramId, const char* text)
{
    t.button.setButtonText (text);
    addAndMakeVisible (t.button);
    t.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processorRef.apvts, paramId, t.button);
    registerLearnTarget (t.button, paramId);
}

void DeciCrushEditor::setupCombo (Combo& c, const char* paramId, const char* text, const juce::StringArray& items)
{
    c.box.addItemList (items, 1);
    addAndMakeVisible (c.box);
    c.label.setText (text, juce::dontSendNotification);
    c.label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (c.label);
    c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processorRef.apvts, paramId, c.box);
    if (juce::String (paramId) != "quality" && juce::String (paramId) != "renderQ")
        registerLearnTarget (c.box, paramId);
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
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (title, r.withHeight (22).reduced (10, 0), juce::Justification::centredLeft);
    };

    panel (panelInput,  "INPUT");
    panel (panelQuant,  "QUANTIZER");
    panel (panelRes,    "RESAMPLER");
    panel (panelFilter, "FILTER");
    panel (panelMaster, "MASTER");
    panel (panelLfo,    "LFO  (modulates resampler frequency, resolution and jitter)");
    panel (panelOptions,"OPTIONS");

    if (learnHint.isNotEmpty())
    {
        g.setColour (juce::Colour (0xffe8743b));
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (learnHint, getLocalBounds().removeFromBottom (18).reduced (14, 0), juce::Justification::centredLeft);
    }
}

void DeciCrushEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto header = area.removeFromTop (34);
    titleLabel.setBounds (header.removeFromLeft (150));
    {
        auto bottom = area.removeFromBottom (130);
        area.removeFromBottom (8);
        panelOptions = bottom.removeFromRight (300);
        bottom.removeFromRight (8);
        panelLfo = bottom;

        auto r = panelLfo.reduced (8); r.removeFromTop (22);
        auto col = r.removeFromLeft (80);
        lfoOn.button.setBounds (col.removeFromTop (28));
        lfoSync.button.setBounds (col.removeFromTop (28));
        r.removeFromLeft (6);
        auto combos = r.removeFromLeft (150);
        auto row1 = combos.removeFromTop (30);
        lfoShape.label.setBounds (row1.removeFromLeft (44)); lfoShape.box.setBounds (row1.reduced (0, 3));
        auto row2 = combos.removeFromTop (30);
        lfoNote.label.setBounds (row2.removeFromLeft (44)); lfoNote.box.setBounds (row2.reduced (0, 3));
        r.removeFromLeft (6);
        const int kw = r.getWidth() / 4;
        placeKnob (lfoRate, r.removeFromLeft (kw));
        placeKnob (lfoFreq, r.removeFromLeft (kw));
        placeKnob (lfoBits, r.removeFromLeft (kw));
        placeKnob (lfoJit,  r);

        auto o = panelOptions.reduced (8); o.removeFromTop (24);
        for (auto* c : { &inputMode, &quality, &renderQ })
        {
            auto row = o.removeFromTop (30);
            c->label.setBounds (row.removeFromLeft (60));
            c->box.setBounds (row.reduced (0, 3));
        }
    }
    sizeBox.setBounds (header.removeFromRight (70).reduced (0, 4));
    header.removeFromRight (6);
    presetBar.setBounds (header.removeFromRight (230).reduced (0, 4));
    header.removeFromRight (6);
    presetBox.setBounds (header.removeFromRight (190).reduced (0, 4));
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
