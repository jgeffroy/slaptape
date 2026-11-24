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

    addFloat ("mix",    "FX Mix",     Range (0.0f, 100.0f, 0.1f), 100.0f, "%");
    addFloat ("out",    "Output",     Range (-24.0f, 12.0f, 0.1f), 0.0f, "dB");

    return { p.begin(), p.end() };
}

void DeciCrushProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& c : channels)
        c.reset();

    preampSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.reset    (sampleRate, 0.02);
    outSmoothed.reset    (sampleRate, 0.02);

    preampSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain ((double) apvts.getRawParameterValue ("preamp")->load()));
    mixSmoothed.setCurrentAndTargetValue    ((double) apvts.getRawParameterValue ("mix")->load() * 0.01);
    outSmoothed.setCurrentAndTargetValue    (juce::Decibels::decibelsToGain ((double) apvts.getRawParameterValue ("out")->load()));
}

bool DeciCrushProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void DeciCrushProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    deci::Params p;
    p.sampleRate     = getSampleRate();
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

    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
        channels[(size_t) ch].updateCoefficients (p);

    for (int i = 0; i < numSamples; ++i)
    {
        const double pre = preampSmoothed.getNextValue();
        const double mix = mixSmoothed.getNextValue();
        const double out = outSmoothed.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const double dry = (double) data[i];
            const double wet = channels[(size_t) ch].process (dry, p, pre);
            data[i] = (float) ((dry * (1.0 - mix) + wet * mix) * out);
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
    setParameterPlain ("mix",    s.mix);
    setParameterPlain ("out",    s.out);
}

void DeciCrushProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DeciCrushProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DeciCrushProcessor();
}
