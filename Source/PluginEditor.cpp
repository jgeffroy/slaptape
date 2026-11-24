#include "PluginEditor.h"

SlapTapeEchoAudioProcessorEditor::SlapTapeEchoAudioProcessorEditor (SlapTapeEchoAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), userPresets (p.apvts, "SlapTapeEcho")
{
    titleLabel.setText ("SlapTape Echo", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        factoryBox.addItem (processorRef.getProgramName (i), i + 1);
    factoryBox.setTextWhenNothingSelected ("Factory presets");
    factoryBox.onChange = [this] { processorRef.setCurrentProgram (factoryBox.getSelectedId() - 1); };
    addAndMakeVisible (factoryBox);
    addAndMakeVisible (userPresets);

    for (int i = 0; i < 3; ++i)
    {
        pageButtons[i].setClickingTogglesState (false);
        pageButtons[i].onClick = [this, i] { showPage (i); };
        addAndMakeVisible (pageButtons[i]);
    }
    tapButton.onClick = [this] { processorRef.tapTempo(); };
    addAndMakeVisible (tapButton);
    bpmLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (bpmLabel);

    // ---- page 0: main ----
    add ("mode",    "Mode",       ComboK, 0);
    add ("style",   "Style",      ComboK, 0);
    add ("sync",    "Sync",       ToggleK, 0);
    add ("note",    "Note",       ComboK, 0);
    add ("dot",     "Dot",        ToggleK, 0);
    add ("trip",    "Trip",       ToggleK, 0);
    add ("tempoSrc","Tempo Source", ComboK, 0);
    add ("bpm",     "Tempo",      KnobK, 0);

    add ("time",    "Echo Time",  KnobK, 0);
    add ("time2",   "Echo Time 2", KnobK, 0);
    add ("note2",   "Note 2",     ComboK, 0);
    add ("fb",      "Feedback",   KnobK, 0);
    add ("mix",     "Mix",        KnobK, 0);
    add ("sat",     "Saturation", KnobK, 0);
    add ("inGain",  "Input",      KnobK, 0);
    add ("outGain", "Output",     KnobK, 0);

    add ("lowcut",  "Low Cut",    KnobK, 0);
    add ("highcut", "High Cut",   KnobK, 0);
    add ("groove",  "Groove",     KnobK, 0);
    add ("feel",    "Feel",       KnobK, 0);
    add ("accent",  "Accent",     KnobK, 0);
    add ("accent2", "Accent 2",   KnobK, 0);
    add ("prime",   "Prime Numbers", ToggleK, 0);
    add ("width",   "Width",      KnobK, 0);

    add ("lrOffset","L/R Offset", KnobK, 0);
    add ("balance", "Balance",    KnobK, 0);
    add ("fbMix",   "FB Mix",     KnobK, 0);
    add ("fbBal",   "FB Bal",     KnobK, 0);
    add ("timeMode","Time Change", ComboK, 0);
    add ("glide",   "Glide Time", KnobK, 0);

    // ---- page 1: style edit ----
    add ("eqLow",   "EQ Low",     KnobK, 1);
    add ("eqMid",   "EQ Mid",     KnobK, 1);
    add ("midFreq", "Mid Freq",   KnobK, 1);
    add ("eqHigh",  "EQ High",    KnobK, 1);
    add ("decLow",  "Decay Low",  KnobK, 1);
    add ("decMid",  "Decay Mid",  KnobK, 1);
    add ("decHigh", "Decay High", KnobK, 1);
    add ("diff",    "Diffusion",  KnobK, 1);
    add ("size",    "Size",       KnobK, 1);
    add ("diffLoop","Diffusion Place", ComboK, 1);
    add ("decaySat","Decay Sat Type",  ComboK, 1);
    add ("outSatType","Out Sat Type",  ComboK, 1);
    add ("outSat",  "Out Sat Amount",  KnobK, 1);
    add ("wobDepth","Wobble Depth", KnobK, 1);
    add ("wobRate", "Wobble Rate x", KnobK, 1);
    add ("wobShape","Wobble Shape", ComboK, 1);
    add ("wobFb",   "Wobble FB",    ToggleK, 1);
    add ("wobSync", "Wobble Sync",  ComboK, 1);

    // ---- page 2: rhythm ----
    add ("rhLen",   "Length (beats)", KnobK, 2);
    add ("rhShape", "Shape",     ComboK, 2);
    add ("rhPan",   "Pan Shape", ComboK, 2);
    add ("rhWarp",  "Warp",      KnobK, 2);
    for (int i = 1; i <= 16; ++i)
        add (("tap" + juce::String (i)).toRawUTF8(), ("Tap " + juce::String (i)).toRawUTF8(), KnobK, 2);

    showPage (0);
    setSize (940, 600);
    startTimerHz (8);
}

void SlapTapeEchoAudioProcessorEditor::add (const char* id, const char* label, Kind kind, int page)
{
    auto c = std::make_unique<Control>();
    c->id = id; c->label = label; c->kind = kind; c->page = page;

    if (kind == KnobK)
    {
        c->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        c->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 16);
        c->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, id, *c->slider);
    }
    else if (kind == ToggleK)
    {
        c->toggle = std::make_unique<juce::ToggleButton> (label);
        c->bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processorRef.apvts, id, *c->toggle);
    }
    else
    {
        c->combo = std::make_unique<juce::ComboBox>();
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processorRef.apvts.getParameter (id)))
            c->combo->addItemList (choice->choices, 1);
        c->cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processorRef.apvts, id, *c->combo);
    }

    if (kind != ToggleK)
    {
        c->labelComp.setText (label, juce::dontSendNotification);
        c->labelComp.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (c->labelComp);
    }
    addAndMakeVisible (*c->comp());
    controls.push_back (std::move (c));
}

void SlapTapeEchoAudioProcessorEditor::showPage (int page)
{
    currentPage = page;
    for (auto& c : controls)
    {
        const bool vis = c->page == page;
        c->comp()->setVisible (vis);
        c->labelComp.setVisible (vis && c->kind != ToggleK);
    }
    for (int i = 0; i < 3; ++i)
        pageButtons[i].setColour (juce::TextButton::buttonColourId,
                                  i == page ? juce::Colour (0xff4a7bd0) : juce::Colour (0xff3a3d43));
    resized();
    updateEnabled();
}

void SlapTapeEchoAudioProcessorEditor::updateEnabled()
{
    const int mode = (int) std::lround (processorRef.apvts.getRawParameterValue ("mode")->load());
    const bool sync = processorRef.apvts.getRawParameterValue ("sync")->load() > 0.5f;
    const bool rhythm = mode == 3;

    for (auto& c : controls)
    {
        bool en = true;
        const auto& id = c->id;
        if (id == "time2" || id == "accent2" || id == "fbMix" || id == "fbBal") en = mode == 1;
        else if (id == "note2") en = mode == 1 && sync;
        else if (id == "width") en = mode == 0 || mode == 2;
        else if (id == "lrOffset") en = mode <= 2;
        else if (id == "balance") en = true;
        else if (id == "time") en = ! sync && ! rhythm;
        else if (id == "note" || id == "dot" || id == "trip") en = sync && ! rhythm;
        else if (id == "tempoSrc") en = sync || rhythm;
        else if (id == "bpm") en = (sync || rhythm);
        else if (id == "timeMode") en = ! rhythm;
        else if (id == "glide") en = ! rhythm && processorRef.apvts.getRawParameterValue ("timeMode")->load() < 0.5f;
        c->comp()->setEnabled (en);
        c->comp()->setAlpha (en ? 1.0f : 0.4f);
    }

    juce::String txt = "Tempo: " + juce::String (processorRef.getCurrentBpm(), 1) + " BPM";
    if (rhythm) txt += "  (Rhythm follows the tempo)";
    bpmLabel.setText (txt, juce::dontSendNotification);
}

void SlapTapeEchoAudioProcessorEditor::timerCallback() { updateEnabled(); }

void SlapTapeEchoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff25272b));
    g.setColour (juce::Colour (0xff32353b));
    g.fillRoundedRectangle (getLocalBounds().reduced (8).withTrimmedTop (84).toFloat(), 8.0f);
}

void SlapTapeEchoAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto header = area.removeFromTop (34);
    titleLabel.setBounds (header.removeFromLeft (170));
    userPresets.setBounds (header.removeFromRight (250).reduced (0, 4));
    header.removeFromRight (8);
    factoryBox.setBounds (header.removeFromRight (220).reduced (0, 4));

    auto bar = area.removeFromTop (34);
    for (int i = 0; i < 3; ++i)
    {
        pageButtons[i].setBounds (bar.removeFromLeft (100).reduced (2, 4));
    }
    bar.removeFromLeft (20);
    tapButton.setBounds (bar.removeFromLeft (100).reduced (2, 4));
    bpmLabel.setBounds (bar.removeFromLeft (340).reduced (6, 4));

    area.removeFromTop (10);
    area = area.reduced (6);

    // flow layout on a grid of 8 columns
    const int cols = 8;
    const int cellW = area.getWidth() / cols;
    const int cellH = 118;
    int n = 0;
    for (auto& c : controls)
    {
        if (c->page != currentPage) continue;
        const int col = n % cols, row = n / cols;
        auto cell = juce::Rectangle<int> (area.getX() + col * cellW, area.getY() + row * cellH, cellW, cellH).reduced (4);
        ++n;

        if (c->kind == KnobK)
        {
            c->labelComp.setBounds (cell.removeFromTop (16));
            c->slider->setBounds (cell);
        }
        else if (c->kind == ComboK)
        {
            c->labelComp.setBounds (cell.removeFromTop (16));
            c->combo->setBounds (cell.removeFromTop (28).reduced (2, 0));
        }
        else
        {
            c->toggle->setBounds (cell.removeFromTop (28).translated (0, 22));
        }
    }
}
