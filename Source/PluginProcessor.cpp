#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const juce::StringArray kModes      { "Single", "Dual", "Ping-Pong", "Rhythm" };
const juce::StringArray kNotes      { "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
const juce::StringArray kTempoSrc   { "Host", "Manual" };
const juce::StringArray kDiffLoop   { "Style", "Post", "Loop" };
const juce::StringArray kSatTypes   { "Style", "Clean", "Tape", "Warm", "Pump", "Dirt", "Hard Limit", "Multiband" };
const juce::StringArray kWobShapes  { "Style", "Triangle", "Sine", "Square", "Random Walk", "Random S/H" };
const juce::StringArray kWobSync    { "In Phase", "Out of Phase", "Drift" };
const juce::StringArray kRhShapes   { "Decay", "Reverse", "Swell", "Fade", "NonLin" };
const juce::StringArray kRhPans     { "Center", "Double", "Alt 1", "Alt 2", "Sweep L", "Sweep R", "Wide" };

struct FactoryPreset
{
    const char* name;
    int mode, style; float time; bool sync; int note; bool dot; bool trip;
    float fb, mix, sat, groove, feel;
};

const FactoryPreset kPresets[] =
{
    { "Slapback 110 ms",        0,  1, 110.0f, false, 2, false, false,  6.0f, 30.0f, 30.0f,   0.0f, 0.0f },
    { "Slapback Tape Warm",     0,  3, 95.0f,  false, 2, false, false, 10.0f, 28.0f, 55.0f,   0.0f, 0.0f },
    { "Slapback Rockabilly",    0,  2, 130.0f, false, 2, false, false, 18.0f, 35.0f, 70.0f,   0.0f, 0.0f },
    { "1/8 Tape Echo",          0,  1, 250.0f, true,  2, false, false, 32.0f, 30.0f, 40.0f,   0.0f, 0.0f },
    { "Dotted 1/8 Ping-Pong",   2,  1, 250.0f, true,  2, true,  false, 38.0f, 32.0f, 35.0f,   0.0f, 0.0f },
    { "Swing 1/8",              0,  1, 250.0f, true,  2, false, false, 35.0f, 30.0f, 40.0f,  55.0f, 0.0f },
    { "Dub Chamber",            0,  3, 375.0f, true,  1, true,  false, 62.0f, 38.0f, 60.0f,   0.0f, 0.0f },
    { "Dual 1/4 and 1/8",       1,  1, 500.0f, true,  1, false, false, 30.0f, 32.0f, 35.0f,   0.0f, 0.0f },
    { "Rhythm Gallop",          3,  1, 250.0f, true,  2, false, false, 25.0f, 35.0f, 30.0f,   0.0f, 0.0f },
    { "Ambient Wash",           0, 25, 420.0f, true,  1, false, false, 55.0f, 40.0f, 30.0f,   0.0f, 0.0f },
    { "Telephone Echo",         0, 16, 180.0f, false, 2, false, false, 30.0f, 45.0f, 30.0f,   0.0f, 0.0f },
    { "Lo-fi Bucket Chorus",    0,  6, 60.0f,  false, 2, false, false, 20.0f, 45.0f, 40.0f,   0.0f, 0.0f },
    { "Vocal Throw 1/4",        0,  1, 500.0f, true,  1, false, false, 42.0f, 25.0f, 35.0f,   0.0f, 0.0f },
    { "Vocal Slap Doubler",     0,  0, 75.0f,  false, 2, false, false,  0.0f, 22.0f, 25.0f,   0.0f, 0.0f },
    { "Hi-Hat Shuffle 1/16",    0, 21, 125.0f, true,  3, false, false, 18.0f, 30.0f, 10.0f,  60.0f, 0.0f },
    { "Drum Splash Diffused",   0, 26, 250.0f, true,  2, false, false, 45.0f, 30.0f, 30.0f,   0.0f, 0.0f },
    { "Ensemble Chorus",        0, 29, 18.0f,  false, 2, false, false,  0.0f, 50.0f, 20.0f,   0.0f, 0.0f },
    { "Hyper Feedback Squash",  0, 30, 375.0f, true,  1, true,  false, 95.0f, 30.0f, 30.0f,   0.0f, 0.0f },
    { "Laid-back 1/8 (Drag)",   0,  1, 250.0f, true,  2, false, false, 30.0f, 28.0f, 35.0f,   0.0f, 40.0f },
};
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
}

SlapTapeEchoAudioProcessor::SlapTapeEchoAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SlapTapeEchoAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto F = [&p] (const char* id, const char* name, float lo, float hi, float step, float skew, float def, const char* unit)
    {
        Range r (lo, hi, step, skew);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                                                                  juce::AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto B = [&p] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    auto C = [&p] (const char* id, const char* name, const juce::StringArray& items, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };

    // --- main ---
    C ("mode", "Echo Mode", kModes, 0);
    {
        juce::StringArray styleNames;
        for (int i = 0; i < echo::kNumStyles; ++i) styleNames.add (echo::kStyles[i].name);
        C ("style", "Style", styleNames, 1);
    }
    F ("inGain",  "Input",       -24.0f, 24.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("outGain", "Output",      -24.0f, 24.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("sat",     "Saturation",    0.0f, 100.0f, 0.1f, 1.0f, 30.0f, "%");
    F ("fb",      "Feedback",      0.0f, 110.0f, 0.1f, 1.0f, 30.0f, "%");
    F ("mix",     "Mix",           0.0f, 100.0f, 0.1f, 1.0f, 35.0f, "%");
    B ("prime",   "Prime Numbers", false);
    F ("lowcut",  "Low Cut",      20.0f, 2000.0f, 1.0f, 0.3f, 20.0f, "Hz");
    F ("highcut", "High Cut",    500.0f, 20000.0f, 1.0f, 0.3f, 20000.0f, "Hz");
    F ("groove",  "Groove",     -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("feel",    "Feel",       -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    C ("timeMode","Time Change", juce::StringArray { "Tape (glide)", "Clean (crossfade)" }, 0);
    F ("glide",   "Glide Time",    5.0f, 2000.0f, 1.0f, 0.4f, 40.0f, "ms");

    // --- timing ---
    B ("sync",    "Sync",         false);
    C ("note",    "Note",         kNotes, 2);
    B ("dot",     "Dot",          false);
    B ("trip",    "Trip",         false);
    F ("time",    "Echo Time",    1.0f, 2500.0f, 0.1f, 0.4f, 120.0f, "ms");
    C ("note2",   "Note 2",       kNotes, 3);
    F ("time2",   "Echo Time 2",  1.0f, 2500.0f, 0.1f, 0.4f, 180.0f, "ms");
    C ("tempoSrc","Tempo Source", kTempoSrc, 0);
    F ("bpm",     "Tempo",       40.0f, 300.0f, 0.1f, 1.0f, 120.0f, "BPM");

    // --- mode controls ---
    F ("accent",  "Accent",     -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("accent2", "Accent 2",   -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("width",   "Width",         0.0f, 100.0f, 0.1f, 1.0f, 50.0f, "%");
    F ("lrOffset","L/R Offset",    0.0f, 25.0f, 0.1f, 1.0f, 0.0f, "ms");
    F ("balance", "Balance",    -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("fbMix",   "FB Mix",        0.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("fbBal",   "FB Bal",     -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");

    // --- style editor (all values are added to the selected style) ---
    F ("eqLow",   "EQ Low",      -12.0f, 12.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("eqMid",   "EQ Mid",      -12.0f, 12.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("eqHigh",  "EQ High",     -12.0f, 12.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("midFreq", "Mid Freq",    200.0f, 8000.0f, 1.0f, 0.4f, 1200.0f, "Hz");
    F ("decLow",  "Decay Low",    -6.0f, 6.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("decMid",  "Decay Mid",    -6.0f, 6.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("decHigh", "Decay High",   -6.0f, 6.0f, 0.1f, 1.0f, 0.0f, "dB");
    F ("diff",    "Diffusion",     0.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("size",    "Size",          0.0f, 100.0f, 0.1f, 1.0f, 50.0f, "%");
    C ("diffLoop","Diffusion Place", kDiffLoop, 0);
    C ("decaySat","Decay Sat Type",  kSatTypes, 0);
    C ("outSatType","Out Sat Type",  kSatTypes, 0);
    F ("outSat",  "Out Sat Amount",  0.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("wobDepth","Wobble Depth",    0.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    F ("wobRate", "Wobble Rate x",   0.25f, 4.0f, 0.01f, 0.5f, 1.0f, "x");
    C ("wobShape","Wobble Shape",    kWobShapes, 0);
    B ("wobFb",   "Wobble FB",       true);
    C ("wobSync", "Wobble Sync",     kWobSync, 0);

    // --- rhythm ---
    F ("rhLen",   "Pattern Length",  1.0f, 8.0f, 0.25f, 1.0f, 4.0f, "beats");
    C ("rhShape", "Shape",           kRhShapes, 0);
    C ("rhPan",   "Pan Shape",       kRhPans, 0);
    F ("rhWarp",  "Warp",         -100.0f, 100.0f, 0.1f, 1.0f, 0.0f, "%");
    for (int i = 0; i < 16; ++i)
    {
        const auto id   = juce::String ("tap") + juce::String (i + 1);
        const auto name = juce::String ("Tap ") + juce::String (i + 1);
        const float def = (i % 4 == 0) ? 100.0f : (i % 4 == 2 ? 60.0f : 0.0f);
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, Range (0.0f, 100.0f, 0.1f), def));
    }

    return { p.begin(), p.end() };
}

void SlapTapeEchoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setParams (buildParams (120.0));
    engine.reset();
}

bool SlapTapeEchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

echo::Params SlapTapeEchoAudioProcessor::buildParams (double bpm) const
{
    using namespace echo;
    Params q;
    const double fs = getSampleRate() > 0 ? getSampleRate() : 44100.0;

    const int styleIdx = juce::jlimit (0, kNumStyles - 1, (int) std::lround (get ("style")));
    const StyleDef& st = kStyles[styleIdx];

    q.mode = juce::jlimit (0, 3, (int) std::lround (get ("mode")));

    // time (ms): note values when synced, otherwise the knob
    auto noteMs = [&] (int idx, bool dot, bool trip)
    {
        static const double beats[6] = { 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
        double b = beats[juce::jlimit (0, 5, idx)];
        if (dot) b *= 1.5; else if (trip) b *= 2.0 / 3.0;
        return std::min (b * 60000.0 / bpm, 4500.0);
    };
    const bool sync = get ("sync") > 0.5f, dot = get ("dot") > 0.5f, trip = get ("trip") > 0.5f;
    q.timeMs1 = sync ? noteMs ((int) std::lround (get ("note")), dot, trip)  : (double) get ("time");
    q.timeMs2 = sync ? noteMs ((int) std::lround (get ("note2")), dot, trip) : (double) get ("time2");

    q.feedback = (double) get ("fb") * 0.01;
    q.groove   = (double) get ("groove") * 0.01;
    q.feel     = (double) get ("feel") * 0.01;
    q.accent1  = (double) get ("accent")  * 0.01;
    q.accent2  = (double) get ("accent2") * 0.01;
    q.prime    = get ("prime") > 0.5f;
    q.lrOffsetMs = (double) get ("lrOffset");
    q.width    = (double) get ("width") * 0.01;
    q.balance  = (double) get ("balance") * 0.01;
    q.fbMix    = (double) get ("fbMix") * 0.01;
    q.fbBal    = (double) get ("fbBal") * 0.01;
    q.inputGain  = juce::Decibels::decibelsToGain ((double) get ("inGain"));
    q.outputGain = juce::Decibels::decibelsToGain ((double) get ("outGain"));

    // loop stage: user cuts + style bandwidth, saturation, per-repeat decay, diffusion
    StageSettings& s = q.stage;
    s.fs = fs;
    s.lcHz = (double) get ("lowcut");
    s.hcHz = (double) get ("highcut");
    s.bwLow = st.bwLow; s.bwHigh = st.bwHigh; s.narrow = st.narrow;

    const int decaySat = (int) std::lround (get ("decaySat"));
    s.satType = decaySat == 0 ? st.satType : decaySat - 1;
    s.satAmt  = juce::jlimit (0.0, 1.0, st.satBase + (double) get ("sat") * 0.01 * st.satScale);
    if (decaySat != 0 && st.satType == SatClean)
        s.satAmt = juce::jlimit (0.0, 1.0, (double) get ("sat") * 0.01 + 0.05);

    s.decLow  = st.decLow  + (double) get ("decLow");
    s.decMid  = st.decMid  + (double) get ("decMid");
    s.decHigh = st.decHigh + (double) get ("decHigh");
    s.midHz   = (double) get ("midFreq");

    const int place = (int) std::lround (get ("diffLoop"));
    const bool loopDiff = place == 0 ? st.loopDiff : place == 2;
    const double diffAmt = juce::jlimit (0.0, 1.0, st.diff + (double) get ("diff") * 0.01);
    const double sizeV   = juce::jlimit (0.0, 1.0, (st.size + (double) get ("size") * 0.01) * 0.5 + 0.0);
    q.diffSize = sizeV;
    s.diffSize = sizeV;
    s.diffAmt  = loopDiff ? diffAmt : 0.0;
    q.postDiffAmt = loopDiff ? 0.0 : diffAmt;

    // output stage
    q.eqLow  = st.eqLow  + (double) get ("eqLow");
    q.eqMid  = st.eqMid  + (double) get ("eqMid");
    q.eqHigh = st.eqHigh + (double) get ("eqHigh");
    q.midHz  = (double) get ("midFreq");
    const int outSat = (int) std::lround (get ("outSatType"));
    q.outSatType = outSat == 0 ? st.outSatType : outSat - 1;
    q.outSatAmt  = juce::jlimit (0.0, 1.0, (outSat == 0 ? st.outSatAmt : 0.0) + (double) get ("outSat") * 0.01);

    // wobble
    q.wobDepth = juce::jlimit (0.0, 1.0, st.wobDepth + (double) get ("wobDepth") * 0.01);
    q.wobRate  = st.wobRate * (double) get ("wobRate");
    const int shape = (int) std::lround (get ("wobShape"));
    q.wobShape = shape == 0 ? st.wobShape : shape - 1;
    q.wobFb    = get ("wobFb") > 0.5f;
    q.wobSync  = (int) std::lround (get ("wobSync"));

    // rhythm
    for (int i = 0; i < 16; ++i)
        q.taps[(size_t) i] = (double) get ((juce::String ("tap") + juce::String (i + 1)).toRawUTF8()) * 0.01;
    q.rhLenBeats = (double) get ("rhLen");
    q.rhShape = (int) std::lround (get ("rhShape"));
    q.rhPan   = (int) std::lround (get ("rhPan"));
    q.rhWarp  = (double) get ("rhWarp") * 0.01;
    q.bpm = bpm;
    q.timeMode = (int) std::lround (get ("timeMode"));
    q.glideMs  = (double) get ("glide");
    return q;
}

void SlapTapeEchoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    double bpm = (double) get ("bpm");
    if ((int) std::lround (get ("tempoSrc")) == 0)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm())
                    if (*b > 20.0 && *b < 400.0)
                        bpm = *b;
    lastBpm.store (bpm);

    engine.setParams (buildParams (bpm));

    const double mix = (double) get ("mix") * 0.01;
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    float* l = buffer.getWritePointer (0);
    float* r = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float dl = l[i], dr = r ? r[i] : dl;
        float wl, wr;
        engine.process (dl, dr, wl, wr);
        l[i] = (float) (dl * (1.0 - mix) + wl * mix);
        if (r) r[i] = (float) (dr * (1.0 - mix) + wr * mix);
    }
}

juce::AudioProcessorEditor* SlapTapeEchoAudioProcessor::createEditor()
{
    return new SlapTapeEchoAudioProcessorEditor (*this);
}

void SlapTapeEchoAudioProcessor::tapTempo()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (lastTapMs > 0.0)
    {
        const double dt = now - lastTapMs;
        if (dt > 200.0 && dt < 1500.0)
        {
            tapIntervals.push_back (dt);
            if (tapIntervals.size() > 4)
                tapIntervals.erase (tapIntervals.begin());
            double sum = 0.0;
            for (double v : tapIntervals) sum += v;
            const double bpm = 60000.0 / (sum / (double) tapIntervals.size());
            setPlain ("bpm", (float) juce::jlimit (40.0, 300.0, bpm));
            setPlain ("tempoSrc", 1.0f);
        }
        else
            tapIntervals.clear();
    }
    lastTapMs = now;
}

void SlapTapeEchoAudioProcessor::setPlain (const char* id, float v)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

int SlapTapeEchoAudioProcessor::getNumPrograms() { return kNumPresets; }

const juce::String SlapTapeEchoAudioProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, kNumPresets) ? juce::String (kPresets[i].name) : juce::String();
}

void SlapTapeEchoAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets))
        return;
    currentProgram = index;
    const auto& s = kPresets[index];

    // start from a neutral state so that presets do not inherit stray settings
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            const juce::String id = rp->getParameterID();
            if (id != "tempoSrc" && id != "bpm")
                rp->setValueNotifyingHost (rp->getDefaultValue());
        }

    setPlain ("mode",  (float) s.mode);
    setPlain ("style", (float) s.style);
    setPlain ("time",  s.time);
    setPlain ("sync",  s.sync ? 1.0f : 0.0f);
    setPlain ("note",  (float) s.note);
    setPlain ("dot",   s.dot ? 1.0f : 0.0f);
    setPlain ("trip",  s.trip ? 1.0f : 0.0f);
    setPlain ("fb",    s.fb);
    setPlain ("mix",   s.mix);
    setPlain ("sat",   s.sat);
    setPlain ("groove", s.groove);
    setPlain ("feel",   s.feel);
    if (s.mode == 1) setPlain ("note2", 2.0f);
}

void SlapTapeEchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SlapTapeEchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlapTapeEchoAudioProcessor();
}
