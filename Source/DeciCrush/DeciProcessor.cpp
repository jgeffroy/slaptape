#include "DeciProcessor.h"
#include "DeciEditor.h"

namespace
{
struct Preset
{
    const char* name;
    float preamp; bool qOn; int bits; bool dc; float dither;
    bool rOn; float freq; float jitter; bool aOn; float aDev; bool iOn; float iShift;
    int fType; float fCut; float fReso; int fOrder;
    float mix; float out;
    // optional LFO (left at zero = off)
    bool lfoOn = false; int lfoShape = 0; float lfoRate = 0.0f; bool lfoSync = false; int lfoNote = 4; float lfoFreq = 0.0f, lfoBits = 0.0f, lfoJit = 0.0f;
};

// fType: 0 Off, 1 LP, 2 BP, 3 HP, 4 BR   |   fOrder: 0 Pre, 1 Post
const Preset kPresets[] =
{
    { "Init (clean)",            0.0f, false, 16, true,   0.0f,  false, 44100.0f,  0.0f, true,  0.0f, false,  0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, 0.0f },
    { "12-bit Crunch",           3.0f, true,  12, true,   0.0f,  true,  26000.0f,  5.0f, true,  0.0f, false,  0.0f, 1, 12000.0f, 5.0f, 1, 100.0f, 0.0f },
    { "8-bit Grit",              6.0f, true,   8, true,  20.0f,  true,  16000.0f,  0.0f, false, 0.0f, false,  0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, -2.0f },
    { "Dusty Sampler",           2.0f, true,  10, true,   5.0f,  true,  22050.0f, 15.0f, true,  0.0f, true,   0.3f, 1, 9000.0f, 20.0f, 1, 100.0f, 0.0f },
    { "Telephone",               6.0f, true,   8, true,   0.0f,  true,   8000.0f,  3.0f, true,  0.0f, true,   0.0f, 2, 1800.0f, 45.0f, 1, 100.0f, 0.0f },
    { "Alias-free Brickwall LP", 0.0f, false, 16, true,   0.0f,  true,   6000.0f,  0.0f, true,  0.0f, true,   0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, 0.0f },
    { "Aliasing Metal",          9.0f, true,   6, false, 10.0f,  true,  11025.0f,  8.0f, false, 0.0f, false,  0.0f, 3, 300.0f,  25.0f, 0, 100.0f, -4.0f },
    { "Parallel Grit (50%)",     6.0f, true,   6, true,   0.0f,  true,  14000.0f,  0.0f, false, 0.0f, false,  0.0f, 0, 8000.0f,  0.0f, 1,  50.0f, 0.0f },
    { "Total Destruction",      12.0f, true,   3, true,   0.0f,  true,   2200.0f, 40.0f, false, 0.0f, false,  0.0f, 1, 3000.0f, 60.0f, 1, 100.0f, -6.0f },
    { "Vintage 12-bit Sampler 26k", 2.0f, true, 12, true,  0.0f,  true,  26040.0f,  3.0f, true,  0.0f, true,   0.0f, 1, 13000.0f, 8.0f, 1, 100.0f, 0.0f },
    { "Early 8-bit Sampler 22k",    3.0f, true,  8, true,  0.0f,  true,  22050.0f,  2.0f, true,  0.2f, true,   0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, -1.0f },
    { "Lo-Fi Hip-Hop Drums",        4.0f, true, 10, true,  3.0f,  true,  18000.0f,  6.0f, true,  0.0f, true,  -0.3f, 1, 10000.0f, 10.0f, 1, 100.0f, 0.0f },
    { "Game Console Chip",          6.0f, true,  4, false, 0.0f,  true,  11025.0f, 20.0f, false, 0.0f, false,  0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, -6.0f },
    { "Radio Static Voice",         8.0f, true,  6, true, 40.0f,  true,   9000.0f, 12.0f, true,  0.0f, true,   0.0f, 2, 1500.0f, 35.0f, 1, 100.0f, -3.0f },
    { "Sub Crush Bass (parallel)",  9.0f, true,  5, true,  0.0f,  true,   6000.0f,  0.0f, false, 0.0f, true,   0.5f, 1, 4000.0f, 20.0f, 1,  40.0f, 0.0f },
    { "Soft Grain Vocal",           1.0f, true, 11, true, 10.0f,  true,  30000.0f,  4.0f, true,  0.0f, true,   0.0f, 0, 8000.0f,  0.0f, 1,  60.0f, 0.0f },
    { "Resampler Sweep (LFO)",      3.0f, true, 10, true,  0.0f,  true,   8000.0f,  0.0f, true,  0.0f, true,   0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, -2.0f,
      true, 0, 0.25f, true, 2, 2.0f, 0.0f, 0.0f },
    { "Bit Wobble (LFO)",           4.0f, true,  8, true,  0.0f,  false, 44100.0f,  0.0f, true,  0.0f, false,  0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, -2.0f,
      true, 1, 2.0f, false, 4, 0.0f, 4.0f, 0.0f },
    { "Glitch Jitter (LFO)",        5.0f, true,  9, true,  0.0f,  true,  12000.0f, 10.0f, false, 0.0f, true,   0.0f, 0, 8000.0f,  0.0f, 1, 100.0f, -3.0f,
      true, 5, 4.0f, true, 6, 1.0f, 2.0f, 60.0f },
};
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
}

DeciCrushProcessor::DeciCrushProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    channels[0].seed (0x1234567u);
    channels[1].seed (0x7654321u);
    for (auto& c : ccMap)
        c.store (-1);
    for (size_t i = 0; i < oversamplers.size(); ++i)
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, (size_t) i + 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false);
}

const juce::StringArray& DeciCrushProcessor::learnableIds()
{
    static const juce::StringArray ids { "preamp", "qOn", "bits", "dc", "dither", "rOn", "freq", "jitter", "aOn", "aDev",
                                         "iOn", "iShift", "fType", "fCut", "fReso", "fOrder", "mix", "out",
                                         "lfoOn", "lfoShape", "lfoRate", "lfoSync", "lfoNote", "lfoFreq", "lfoBits", "lfoJit", "input" };
    return ids;
}

void DeciCrushProcessor::clearMidiMapping (const juce::String& paramId)
{
    const int idx = learnableIds().indexOf (paramId);
    for (auto& c : ccMap)
        if (c.load() == idx)
            c.store (-1);
}

int DeciCrushProcessor::getMappedCc (const juce::String& paramId) const
{
    const int idx = learnableIds().indexOf (paramId);
    for (size_t cc = 0; cc < ccMap.size(); ++cc)
        if (ccMap[cc].load() == idx)
            return (int) cc;
    return -1;
}

juce::String DeciCrushProcessor::midiMapToString() const
{
    juce::StringArray parts;
    for (size_t cc = 0; cc < ccMap.size(); ++cc)
    {
        const int idx = ccMap[cc].load();
        if (idx >= 0)
            parts.add (juce::String ((int) cc) + ":" + learnableIds()[idx]);
    }
    return parts.joinIntoString (",");
}

void DeciCrushProcessor::midiMapFromString (const juce::String& str)
{
    for (auto& c : ccMap)
        c.store (-1);
    for (auto& part : juce::StringArray::fromTokens (str, ",", ""))
    {
        const int cc = part.upToFirstOccurrenceOf (":", false, false).getIntValue();
        const int idx = learnableIds().indexOf (part.fromFirstOccurrenceOf (":", false, false));
        if (cc >= 0 && cc < 128 && idx >= 0)
            ccMap[(size_t) cc].store (idx);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout DeciCrushProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addFloat = [&p] (const char* id, const char* name, Range r, float def, const char* unit)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                                                                  juce::AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto addBool = [&p] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };

    addFloat ("preamp", "Preamp",     Range (0.0f, 48.0f, 0.1f), 0.0f, "dB");

    addBool  ("qOn",    "Quantizer On", true);
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "bits", 1 }, "Resolution", 1, 24, 12));
    addBool  ("dc",     "DC Shift",   true);
    addFloat ("dither", "Dithering",  Range (0.0f, 100.0f, 0.1f), 0.0f, "%");

    addBool  ("rOn",    "Resampler On", true);
    addFloat ("freq",   "Frequency",  Range (44.0f, 44100.0f, 1.0f, 0.3f), 22050.0f, "Hz");
    addFloat ("jitter", "Jitter",     Range (0.0f, 100.0f, 0.1f), 0.0f, "%");
    addBool  ("aOn",    "Approx. Filter", true);
    addFloat ("aDev",   "Approx. Deviation", Range (-2.0f, 2.0f, 0.01f), 0.0f, "oct");
    addBool  ("iOn",    "Images Filter", false);
    addFloat ("iShift", "Images Shift", Range (-2.0f, 2.0f, 0.01f), 0.0f, "oct");

    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "fType", 1 }, "Filter Type",
                                                               juce::StringArray { "Off", "LP", "BP", "HP", "BR" }, 0));
    addFloat ("fCut",   "Filter Cutoff", Range (20.0f, 20000.0f, 1.0f, 0.3f), 8000.0f, "Hz");
    addFloat ("fReso",  "Reso / Width",  Range (0.0f, 100.0f, 0.1f), 0.0f, "%");
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "fOrder", 1 }, "Filter Order",
                                                               juce::StringArray { "Pre", "Post" }, 1));

    // Quality: Draft = no oversampling, Normal = 2x, High = 4x, Ultra = 8x. Render quality is used for offline bounces.
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "quality", 1 }, "Quality",
                                                               juce::StringArray { "Draft", "Normal (2x)", "High (4x)", "Ultra (8x)" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "renderQ", 1 }, "Render Quality",
                                                               juce::StringArray { "Same as real-time", "Draft", "Normal (2x)", "High (4x)", "Ultra (8x)" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "input", 1 }, "Input",
                                                               juce::StringArray { "Stereo", "Left only", "Right only", "Mono (L+R)" }, 0));

    // LFO (modulates resampler frequency, bit depth and jitter)
    addBool  ("lfoOn",   "LFO On", false);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfoShape", 1 }, "LFO Shape",
                                                               juce::StringArray { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Random" }, 0));
    addFloat ("lfoRate", "LFO Rate",   Range (0.02f, 20.0f, 0.01f, 0.3f), 1.0f, "Hz");
    addBool  ("lfoSync", "LFO Sync", false);
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfoNote", 1 }, "LFO Note",
                                                               juce::StringArray { "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32" }, 4));
    addFloat ("lfoFreq", "LFO > Frequency", Range (0.0f, 4.0f, 0.01f), 0.0f, "oct");
    addFloat ("lfoBits", "LFO > Resolution", Range (0.0f, 12.0f, 0.1f), 0.0f, "bits");
    addFloat ("lfoJit",  "LFO > Jitter", Range (0.0f, 100.0f, 0.1f), 0.0f, "%");

    addFloat ("mix",    "FX Mix",     Range (0.0f, 100.0f, 0.1f), 100.0f, "%");
    addFloat ("out",    "Output",     Range (-24.0f, 12.0f, 0.1f), 0.0f, "dB");

    return { p.begin(), p.end() };
}

int DeciCrushProcessor::latencyFor (int q) const
{
    return q > 0 ? (int) std::ceil (oversamplers[(size_t) q - 1]->getLatencyInSamples()) : 0;
}

// The reported latency is the larger of the real-time and render settings, so that switching to the
// render quality during an offline bounce does not change the latency seen by the host.
void DeciCrushProcessor::applyQuality (int q, int maxQ)
{
    currentQuality = q;
    currentMaxQuality = maxQ;
    currentLatency = latencyFor (maxQ);
    setLatencySamples (currentLatency);
    dryDelay.setDelay ((float) currentLatency);
    wetDelay.setDelay ((float) (currentLatency - latencyFor (q)));
    dryDelay.reset();
    wetDelay.reset();
    if (q > 0)
        oversamplers[(size_t) q - 1]->reset();
    for (auto& c : channels)
        c.reset();
}

void DeciCrushProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlock = juce::jmax (samplesPerBlock, 16);
    for (auto& o : oversamplers)
        o->initProcessing ((size_t) maxBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
    dryDelay.prepare (spec);
    wetDelay.prepare (spec);
    dryBuffer.setSize (2, maxBlock);

    const int rtQ = (int) std::lround (apvts.getRawParameterValue ("quality")->load());
    const int rq  = (int) std::lround (apvts.getRawParameterValue ("renderQ")->load());
    const int q   = (isNonRealtime() && rq > 0) ? rq - 1 : rtQ;
    applyQuality (q, juce::jmax (rtQ, rq > 0 ? rq - 1 : rtQ));

    preampSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.reset    (sampleRate, 0.02);
    outSmoothed.reset    (sampleRate, 0.02);

    preampSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain ((double) apvts.getRawParameterValue ("preamp")->load()));
    mixSmoothed.setCurrentAndTargetValue    ((double) apvts.getRawParameterValue ("mix")->load() * 0.01);
    outSmoothed.setCurrentAndTargetValue    (juce::Decibels::decibelsToGain ((double) apvts.getRawParameterValue ("out")->load()));
    lfoPhase = 0.0;
}

double DeciCrushProcessor::lfoValue (int shape, double ph)
{
    switch (shape)
    {
        case 0:  return std::sin (juce::MathConstants<double>::twoPi * ph);
        case 1:  return 1.0 - 4.0 * std::abs (ph - 0.5);
        case 2:  return 2.0 * ph - 1.0;
        case 3:  return 1.0 - 2.0 * ph;
        case 4:  return ph < 0.5 ? 1.0 : -1.0;
        default:
            if (ph < lfoLastPhase)   // new cycle: new random value
                lfoRandom = lfoRng.nextDouble() * 2.0 - 1.0;
            lfoLastPhase = ph;
            return lfoRandom;
    }
}

bool DeciCrushProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void DeciCrushProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    // MIDI learn / CC control
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (! msg.isController())
            continue;
        const int cc = msg.getControllerNumber();
        const int target = learnTarget.load();
        if (target >= 0)
        {
            for (auto& c : ccMap)
                if (c.load() == target)
                    c.store (-1);
            ccMap[(size_t) cc].store (target);
            learnTarget.store (-1);
        }
        const int idx = ccMap[(size_t) cc].load();
        if (idx >= 0)
            if (auto* prm = apvts.getParameter (learnableIds()[idx]))
                prm->setValueNotifyingHost ((float) msg.getControllerValue() / 127.0f);
    }

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0)
        return;

    // Input selection (applies to the dry signal too)
    const int inMode = (int) std::lround (get ("input"));
    if (numChannels == 2 && inMode > 0)
    {
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
        {
            const float l = L[i], r = R[i];
            const float v = inMode == 1 ? l : inMode == 2 ? r : 0.5f * (l + r);
            L[i] = R[i] = v;
        }
    }

    // Quality (real-time or render)
    const int rtQ = (int) std::lround (get ("quality"));
    const int rq  = (int) std::lround (get ("renderQ"));
    const int quality = (isNonRealtime() && rq > 0) ? rq - 1 : rtQ;
    const int maxQ = juce::jmax (rtQ, rq > 0 ? rq - 1 : rtQ);
    if (quality != currentQuality || maxQ != currentMaxQuality)
        applyQuality (quality, maxQ);
    const int factor = 1 << quality;

    deci::Params p;
    p.sampleRate     = getSampleRate() * factor;
    p.quantizerOn    = get ("qOn") > 0.5f;
    p.bits           = (int) std::lround (get ("bits"));
    p.dcShift        = get ("dc") > 0.5f;
    p.dither         = (double) get ("dither") * 0.01;
    p.resamplerOn    = get ("rOn") > 0.5f;
    p.frequency      = (double) get ("freq");
    p.jitter         = (double) get ("jitter") * 0.01;
    p.approxOn       = get ("aOn") > 0.5f;
    p.approxDevOct   = (double) get ("aDev");
    p.imagesOn       = get ("iOn") > 0.5f;
    p.imagesShiftOct = (double) get ("iShift");
    p.filterType     = (int) std::lround (get ("fType"));
    p.filterCutoff   = (double) get ("fCut");
    p.filterReso     = (double) get ("fReso") * 0.01;
    p.filterPost     = (int) std::lround (get ("fOrder")) == 1;

    preampSmoothed.setTargetValue (juce::Decibels::decibelsToGain ((double) get ("preamp")));
    mixSmoothed.setTargetValue    ((double) get ("mix") * 0.01);
    outSmoothed.setTargetValue    (juce::Decibels::decibelsToGain ((double) get ("out")));

    // LFO setup
    const bool lfoOn = get ("lfoOn") > 0.5f;
    const int lfoShape = (int) std::lround (get ("lfoShape"));
    const double dFreq = (double) get ("lfoFreq"), dBits = (double) get ("lfoBits"), dJit = (double) get ("lfoJit") * 0.01;
    double lfoHz = (double) get ("lfoRate");
    if (lfoOn && get ("lfoSync") > 0.5f)
    {
        static const double beats[8] = { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 };
        const double cycleBeats = beats[juce::jlimit (0, 7, (int) std::lround (get ("lfoNote")))];
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) bpm = *b;
                if (pos->getIsPlaying())
                    if (auto ppq = pos->getPpqPosition())
                        lfoPhase = std::fmod (*ppq / cycleBeats, 1.0);   // lock the phase to the song position
            }
        lfoHz = bpm / 60.0 / cycleBeats;
    }

    auto modulated = [&] () -> deci::Params
    {
        deci::Params m = p;
        if (! lfoOn)
            return m;
        const double v = lfoValue (lfoShape, lfoPhase);
        m.frequency = juce::jlimit (44.0, 44100.0, p.frequency * std::pow (2.0, v * dFreq));
        m.bits      = juce::jlimit (1, 24, (int) std::lround ((double) p.bits + v * dBits));
        m.jitter    = juce::jlimit (0.0, 1.0, p.jitter + (0.5 + 0.5 * v) * dJit);
        return m;
    };

    // Dry path, delayed by the reported latency
    if (dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (2, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
        {
            dryDelay.pushSample (ch, buffer.getSample (ch, i));
            dryBuffer.setSample (ch, i, dryDelay.popSample (ch));
        }

    std::vector<double> pre ((size_t) numSamples);
    for (int i = 0; i < numSamples; ++i)
        pre[(size_t) i] = preampSmoothed.getNextValue();

    constexpr int kChunk = 32;   // the LFO updates the coefficients every 32 host samples
    const double fs = getSampleRate();

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numChannels).getSubBlock (0, (size_t) numSamples);
    auto work = quality > 0 ? oversamplers[(size_t) quality - 1]->processSamplesUp (sub) : sub;

    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int len = juce::jmin (kChunk, numSamples - start);
        const deci::Params m = modulated();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& chan = channels[(size_t) ch];
            chan.updateCoefficients (m);
            auto* d = work.getChannelPointer ((size_t) ch);
            for (int k = start * factor; k < (start + len) * factor; ++k)
                d[k] = (float) chan.process ((double) d[k], m, pre[(size_t) (k / factor)]);
        }
        if (lfoOn)
        {
            lfoPhase += lfoHz * (double) len / fs;
            lfoPhase -= std::floor (lfoPhase);
        }
    }

    if (quality > 0)
        oversamplers[(size_t) quality - 1]->processSamplesDown (sub);

    for (int i = 0; i < numSamples; ++i)
    {
        const double mix = mixSmoothed.getNextValue();
        const double out = outSmoothed.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            wetDelay.pushSample (ch, buffer.getSample (ch, i));
            const double wet = (double) wetDelay.popSample (ch);
            const double dry = (double) dryBuffer.getSample (ch, i);
            buffer.setSample (ch, i, (float) ((dry * (1.0 - mix) + wet * mix) * out));
        }
    }
}

juce::AudioProcessorEditor* DeciCrushProcessor::createEditor()
{
    return new DeciCrushEditor (*this);
}

int DeciCrushProcessor::getNumPrograms() { return kNumPresets; }

const juce::String DeciCrushProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? juce::String (kPresets[index].name) : juce::String();
}

void DeciCrushProcessor::setParameterPlain (const char* id, float plainValue)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

void DeciCrushProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets))
        return;

    currentProgram = index;
    const auto& s = kPresets[index];

    setParameterPlain ("preamp", s.preamp);
    setParameterPlain ("qOn",    s.qOn ? 1.0f : 0.0f);
    setParameterPlain ("bits",   (float) s.bits);
    setParameterPlain ("dc",     s.dc ? 1.0f : 0.0f);
    setParameterPlain ("dither", s.dither);
    setParameterPlain ("rOn",    s.rOn ? 1.0f : 0.0f);
    setParameterPlain ("freq",   s.freq);
    setParameterPlain ("jitter", s.jitter);
    setParameterPlain ("aOn",    s.aOn ? 1.0f : 0.0f);
    setParameterPlain ("aDev",   s.aDev);
    setParameterPlain ("iOn",    s.iOn ? 1.0f : 0.0f);
    setParameterPlain ("iShift", s.iShift);
    setParameterPlain ("fType",  (float) s.fType);
    setParameterPlain ("fCut",   s.fCut);
    setParameterPlain ("fReso",  s.fReso);
    setParameterPlain ("fOrder", (float) s.fOrder);
    setParameterPlain ("lfoOn",    s.lfoOn ? 1.0f : 0.0f);
    setParameterPlain ("lfoShape", (float) s.lfoShape);
    setParameterPlain ("lfoRate",  s.lfoRate > 0.0f ? s.lfoRate : 1.0f);
    setParameterPlain ("lfoSync",  s.lfoSync ? 1.0f : 0.0f);
    setParameterPlain ("lfoNote",  s.lfoOn ? (float) s.lfoNote : 4.0f);
    setParameterPlain ("lfoFreq",  s.lfoFreq);
    setParameterPlain ("lfoBits",  s.lfoBits);
    setParameterPlain ("lfoJit",   s.lfoJit);
    setParameterPlain ("mix",    s.mix);
    setParameterPlain ("out",    s.out);
}

void DeciCrushProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("DECISTATE");
    if (auto xml = apvts.copyState().createXml())
        root.addChildElement (xml.release());
    root.setAttribute ("midimap", midiMapToString());
    root.setAttribute ("guiScale", (double) guiScale);
    copyXmlToBinary (root, destData);
}

void DeciCrushProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    if (xml->hasTagName ("DECISTATE"))
    {
        midiMapFromString (xml->getStringAttribute ("midimap"));
        guiScale = (float) xml->getDoubleAttribute ("guiScale", 1.0);
        if (auto* child = xml->getFirstChildElement())
            if (child->hasTagName (apvts.state.getType()))
                apvts.replaceState (juce::ValueTree::fromXml (*child));
    }
    else if (xml->hasTagName (apvts.state.getType())) // older saves
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeciCrushProcessor();
}
