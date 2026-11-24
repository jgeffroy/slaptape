#include "HeatProcessor.h"
#include "HeatEditor.h"

namespace
{
struct HeatPreset
{
    const char* name;
    int style; float drive; bool punish, autoGain; float out, mix, lowCut; bool thump; float tone, highCut; bool steep;
    int stereo = 0; // 0 stereo, 1 mid only, 2 side only
};

const HeatPreset kPresets[] =
{
    { "Init",                 0, 3.0f, false, true,  0.0f, 100.0f,   20.0f, false,  0.0f, 20000.0f, false },
    { "Warm Tape Glue",       0, 4.0f, false, true,  0.0f, 100.0f,   30.0f, false, -0.2f, 12000.0f, false },
    { "Console Bite",         1, 5.0f, false, true,  0.0f, 100.0f,   60.0f, false,  0.2f, 16000.0f, false },
    { "Fat Transformer Bass", 2, 6.0f, false, true,  0.0f, 100.0f,   40.0f, true,   0.0f,  9000.0f, false },
    { "Triode Vocal Warmth",  3, 4.0f, false, true,  0.0f,  70.0f,  100.0f, false,  0.1f, 14000.0f, false },
    { "Pentode Fuzz",         4, 7.0f, false, true, -3.0f, 100.0f,  120.0f, false,  0.3f,  8000.0f, true  },
    { "Drum Smash",           3, 6.0f, true,  true, -4.0f,  45.0f,   80.0f, true,   0.2f, 11000.0f, true  },
    { "Parallel Heat",        1, 8.0f, false, true,  0.0f,  30.0f,  150.0f, false,  0.3f, 15000.0f, false },
    { "Lo-Fi Crunch",         4, 9.0f, true,  true, -6.0f, 100.0f,  300.0f, false, -0.4f,  5000.0f, true  },
    { "Vocal Glass Softener", 0, 3.5f, false, true,  0.0f,  60.0f,   80.0f, false, -0.3f, 12000.0f, false },
    { "Bass Translation",     2, 6.5f, false, true,  0.0f,  50.0f,   30.0f, true,   0.1f, 10000.0f, false },
    { "Guitar Cab (Steep)",   4, 6.0f, false, true,  0.0f, 100.0f,   90.0f, false,  0.0f,  4500.0f, true  },
    { "AM Radio Voice",       1, 6.0f, false, true,  0.0f, 100.0f,  450.0f, false,  0.4f,  3200.0f, true  },
    { "Snare Weight",         0, 5.0f, false, true,  0.0f,  70.0f,  120.0f, true,   0.0f, 14000.0f, false },
    { "Bus Kiss",             1, 2.0f, false, true,  0.0f,  35.0f,   25.0f, false,  0.1f, 18000.0f, false },
    { "Mid Warmth (M/S)",     3, 4.0f, false, true,  0.0f,  80.0f,   40.0f, false,  0.0f, 16000.0f, false, 1 },
    { "Wide Side Sizzle (M/S)",1, 5.0f, false, true, 0.0f,  50.0f,  200.0f, false,  0.3f, 14000.0f, false, 2 },
    { "Punish Sound Design",  4, 8.0f, true,  true, -6.0f, 100.0f,   60.0f, true,   0.2f,  9000.0f, true  },
};
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
}

HeatSatProcessor::HeatSatProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HEATSAT", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HeatSatProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style",
                                                         StringArray { "A", "E", "N", "T", "P" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive",
                                                        NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "punish", 1 }, "Punish", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 }, "Auto Gain", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                                                        NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                                                        AudioParameterFloatAttributes().withLabel ("dB")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                                        AudioParameterFloatAttributes().withLabel ("%")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "lowcut", 1 }, "Low Cut",
                                                        NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.4f), 20.0f,
                                                        AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "thump", 1 }, "Thump", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone",
                                                        NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "highcut", 1 }, "High Cut",
                                                        NormalisableRange<float> (1000.0f, 20000.0f, 1.0f, 0.35f), 20000.0f,
                                                        AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "steep", 1 }, "Steep", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "stereo", 1 }, "Stereo Mode",
                                                         StringArray { "Stereo", "Mid only", "Side only" }, 0));

    return { p.begin(), p.end() };
}

bool HeatSatProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == layouts.getMainInputChannelSet();
}

heat::Settings HeatSatProcessor::readSettings() const
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    heat::Settings s;
    s.fs        = getSampleRate();
    s.style     = (int) std::lround (get ("style"));
    s.drive     = get ("drive");
    s.punish    = get ("punish") > 0.5f;
    s.lowCutHz  = get ("lowcut");
    s.thump     = get ("thump") > 0.5f;
    s.tone      = get ("tone");
    s.highCutHz = get ("highcut");
    s.steep     = get ("steep") > 0.5f;
    return s;
}

void HeatSatProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int block = juce::jmax (samplesPerBlock, 16);
    oversampling.initProcessing ((size_t) block);
    oversampling.reset();

    const int latency = (int) std::ceil (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) block, 2 };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuffer.setSize (2, block);

    const auto s = readSettings();
    for (auto& c : channels)
    {
        c.configure (s, 4.0);
        c.reset();
    }

    const float g = (float) heat::driveGain (s);
    gainSmoothed.reset (sampleRate, 0.02);  gainSmoothed.setCurrentAndTargetValue (g);
    autoSmoothed.reset (sampleRate, 0.02);  autoSmoothed.setCurrentAndTargetValue (1.0f);
    outSmoothed.reset  (sampleRate, 0.02);  outSmoothed.setCurrentAndTargetValue (1.0f);
    mixSmoothed.reset  (sampleRate, 0.02);  mixSmoothed.setCurrentAndTargetValue (1.0f);
}

void HeatSatProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);
    if (n == 0 || numCh == 0)
        return;

    auto s = readSettings();
    s.fs = getSampleRate();
    const bool autoOn = apvts.getRawParameterValue ("auto")->load() > 0.5f;
    for (int c = 0; c < numCh; ++c)
        channels[(size_t) c].configure (s, 4.0);

    gainSmoothed.setTargetValue ((float) heat::driveGain (s));
    autoSmoothed.setTargetValue (autoOn ? (float) heat::autoGain (s) : 1.0f);
    outSmoothed.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    mixSmoothed.setTargetValue (apvts.getRawParameterValue ("mix")->load() * 0.01f);

    // Dry path, delayed to match the oversampling latency
    if (dryBuffer.getNumSamples() < n)
        dryBuffer.setSize (2, n, false, false, true);
    for (int c = 0; c < numCh; ++c)
        for (int i = 0; i < n; ++i)
        {
            dryDelay.pushSample (c, buffer.getSample (c, i));
            dryBuffer.setSample (c, i, dryDelay.popSample (c));
        }

    // Mid/side: encode, process only the chosen component, decode at the end
    const int stereoMode = numCh == 2 ? (int) std::lround (apvts.getRawParameterValue ("stereo")->load()) : 0;
    if (stereoMode > 0)
    {
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]), sd = 0.5f * (L[i] - R[i]);
            L[i] = m; R[i] = sd;
        }
    }

    // Pre filters and drive at the base rate
    std::vector<float> gains ((size_t) n);
    for (int i = 0; i < n; ++i)
        gains[(size_t) i] = gainSmoothed.getNextValue();

    for (int c = 0; c < numCh; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        auto& ch = channels[(size_t) c];
        for (int i = 0; i < n; ++i)
            d[i] = (float) ch.pre ((double) d[i], (double) gains[(size_t) i]);
    }

    // Attitude meter (VU-like ballistics on the signal entering the saturation stage)
    {
        const int mc = stereoMode == 2 ? 1 : 0;
        double sum = 0.0;
        for (int c = 0; c < numCh; ++c)
            if (stereoMode == 0 || c == mc)
            {
                const auto* d = buffer.getReadPointer (c);
                for (int i = 0; i < n; ++i) sum += (double) d[i] * d[i];
            }
        const int chans = stereoMode == 0 ? numCh : 1;
        const float rms = (float) std::sqrt (sum / (double) juce::jmax (1, n * chans));
        const float coef = 1.0f - std::exp (-(float) n / (0.3f * (float) getSampleRate()));
        meterState += (rms - meterState) * coef;
        attitudeDb.store (juce::Decibels::gainToDecibels (meterState, -60.0f));
    }

    // Saturation, 4x oversampled
    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh).getSubBlock (0, (size_t) n);
    auto up = oversampling.processSamplesUp (sub);
    for (int c = 0; c < numCh; ++c)
    {
        auto* d = up.getChannelPointer ((size_t) c);
        auto& sh = channels[(size_t) c].shaper;
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            d[i] = (float) sh.process ((double) d[i]);
    }
    oversampling.processSamplesDown (sub);

    // Post filters, auto gain, output, mix
    for (int i = 0; i < n; ++i)
    {
        const float comp = autoSmoothed.getNextValue() * outSmoothed.getNextValue();
        const float mix = mixSmoothed.getNextValue();
        float wet[2] = { 0.0f, 0.0f };
        for (int c = 0; c < numCh; ++c)
            wet[c] = (float) channels[(size_t) c].post ((double) buffer.getSample (c, i)) * comp;

        if (stereoMode > 0)
        {
            const float dl = dryBuffer.getSample (0, i), dr = dryBuffer.getSample (1, i);
            const float m  = stereoMode == 1 ? wet[0] : 0.5f * (dl + dr);   // unprocessed part uses the
            const float sd = stereoMode == 2 ? wet[1] : 0.5f * (dl - dr);   // latency-aligned dry signal
            wet[0] = m + sd;
            wet[1] = m - sd;
        }

        for (int c = 0; c < numCh; ++c)
            buffer.setSample (c, i, dryBuffer.getSample (c, i) * (1.0f - mix) + wet[c] * mix);
    }
}

int HeatSatProcessor::getNumPrograms() { return kNumPresets; }
const juce::String HeatSatProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? juce::String (kPresets[index].name) : juce::String();
}

void HeatSatProcessor::setParameterPlain (const char* id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void HeatSatProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets))
        return;
    currentProgram = index;
    const auto& p = kPresets[index];
    setParameterPlain ("style",   (float) p.style);
    setParameterPlain ("drive",   p.drive);
    setParameterPlain ("punish",  p.punish ? 1.0f : 0.0f);
    setParameterPlain ("auto",    p.autoGain ? 1.0f : 0.0f);
    setParameterPlain ("output",  p.out);
    setParameterPlain ("mix",     p.mix);
    setParameterPlain ("lowcut",  p.lowCut);
    setParameterPlain ("thump",   p.thump ? 1.0f : 0.0f);
    setParameterPlain ("tone",    p.tone);
    setParameterPlain ("highcut", p.highCut);
    setParameterPlain ("steep",   p.steep ? 1.0f : 0.0f);
    setParameterPlain ("stereo",  (float) p.stereo);
}

void HeatSatProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HeatSatProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* HeatSatProcessor::createEditor() { return new HeatSatEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HeatSatProcessor(); }
