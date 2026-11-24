#include "PluginProcessor.h"
#include "PluginEditor.h"

SlapTapeEchoAudioProcessor::SlapTapeEchoAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    mixParam      = apvts.getRawParameterValue (paramMix);
    timeParam     = apvts.getRawParameterValue (paramTime);
    feedbackParam = apvts.getRawParameterValue (paramFeedback);
    toneParam     = apvts.getRawParameterValue (paramTone);
    hpfParam      = apvts.getRawParameterValue (paramHpf);
    modRateParam  = apvts.getRawParameterValue (paramModRate);
    modDepthParam = apvts.getRawParameterValue (paramModDepth);
    driveParam    = apvts.getRawParameterValue (paramDrive);
    slapParam     = apvts.getRawParameterValue (paramSlap);

    voices[1].setPhaseOffset (juce::MathConstants<float>::halfPi);
}

juce::AudioProcessorValueTreeState::ParameterLayout SlapTapeEchoAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramMix, "Mix", Range (0.0f, 100.0f, 0.1f), 35.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramTime, "Time", Range (1.0f, 1200.0f, 0.1f, 0.4f), 120.0f, "ms"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramFeedback, "Feedback", Range (0.0f, 95.0f, 0.1f), 25.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramTone, "Tone", Range (500.0f, 18000.0f, 1.0f, 0.3f), 8000.0f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramHpf, "Low Cut", Range (20.0f, 2000.0f, 1.0f, 0.3f), 120.0f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramModRate, "Mod Rate", Range (0.02f, 6.0f, 0.01f, 0.4f), 0.6f, "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramModDepth, "Mod Depth", Range (0.0f, 100.0f, 0.1f), 15.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        paramDrive, "Drive", Range (0.0f, 100.0f, 0.1f), 20.0f, "%"));

    params.push_back (std::make_unique<juce::AudioParameterBool>(
        paramSlap, "Slap Mode", false));

    return { params.begin(), params.end() };
}

void SlapTapeEchoAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& v : voices)
    {
        v.prepare (sampleRate, samplesPerBlock);
        v.reset();
    }
    lastTone = -1.0f;
    lastHpf  = -1.0f;
}

void SlapTapeEchoAudioProcessor::releaseResources() {}

bool SlapTapeEchoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();
    const auto& mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == mainIn;
}

void SlapTapeEchoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    const float mix       = mixParam->load() * 0.01f;
    const float timeMs    = timeParam->load();
    const bool  slapMode  = slapParam->load() > 0.5f;
    const float rawFeedback = feedbackParam->load() * 0.01f;
    const float feedback  = slapMode ? juce::jmin (rawFeedback, 0.28f) : rawFeedback;
    const float tone      = toneParam->load();
    const float hpf       = hpfParam->load();
    const float modRate   = modRateParam->load();
    const float rawModDepthMs = (modDepthParam->load() * 0.01f) * 12.0f; // up to 12 ms of wobble
    const float modDepthMs = slapMode ? rawModDepthMs * 0.15f : rawModDepthMs;
    const float drive      = driveParam->load() * 0.01f;

    if (std::abs (tone - lastTone) > 1.0f || std::abs (hpf - lastHpf) > 1.0f)
    {
        for (auto& v : voices)
            v.updateToneFilters (tone, hpf);
        lastTone = tone;
        lastHpf  = hpf;
    }

    for (int ch = 0; ch < numChannels && ch < 2; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& voice = voices[(size_t) ch];

        for (int i = 0; i < numSamples; ++i)
        {
            float dry = data[i];
            float wet = voice.processSample (dry, timeMs, feedback, modRate, modDepthMs, drive);
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

juce::AudioProcessorEditor* SlapTapeEchoAudioProcessor::createEditor()
{
    return new SlapTapeEchoAudioProcessorEditor (*this);
}

void SlapTapeEchoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void SlapTapeEchoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SlapTapeEchoAudioProcessor();
}
