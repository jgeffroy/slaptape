#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "EchoEngine.h"

/**
    SlapTapeEchoAudioProcessor

    An original tape/slap-style delay effect:
      - delay with feedback and a tone filter (LP + HP) in the feedback loop
      - optional LFO modulation of the delay time (tape wow/flutter)
      - soft saturation on the repeats
      - a "Slap" mode that tightens the feedback range for clean single-repeat
        slapback delay sounds.

    This is original work; it is not a copy of any third-party plugin.
*/
class SlapTapeEchoAudioProcessor : public juce::AudioProcessor
{
public:
    SlapTapeEchoAudioProcessor();
    ~SlapTapeEchoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return EchoVoice::kMaxDelaySeconds; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Parameter IDs
    static constexpr auto paramMix      = "mix";
    static constexpr auto paramTime     = "time";
    static constexpr auto paramFeedback = "feedback";
    static constexpr auto paramTone     = "tone";
    static constexpr auto paramHpf      = "hpf";
    static constexpr auto paramModRate  = "modRate";
    static constexpr auto paramModDepth = "modDepth";
    static constexpr auto paramDrive    = "drive";
    static constexpr auto paramSlap     = "slapMode";

private:
    std::array<EchoVoice, 2> voices;

    std::atomic<float>* mixParam      = nullptr;
    std::atomic<float>* timeParam     = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* toneParam     = nullptr;
    std::atomic<float>* hpfParam      = nullptr;
    std::atomic<float>* modRateParam  = nullptr;
    std::atomic<float>* modDepthParam = nullptr;
    std::atomic<float>* driveParam    = nullptr;
    std::atomic<float>* slapParam     = nullptr;

    float lastTone = -1.0f, lastHpf = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapTapeEchoAudioProcessor)
};
