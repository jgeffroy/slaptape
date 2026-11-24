#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "EchoEngine.h"
#include "EchoStyles.h"

/**
    SlapTape Echo - an original multi-mode echo machine.
    Modes: Single, Dual, Ping-Pong, Rhythm. Tempo sync (host or manual, note/dotted/triplet),
    Groove, Feel, Accent, Prime Numbers, 29 styles, saturation types, wobble, diffusion, EQ and decay EQ.
*/
class SlapTapeEchoAudioProcessor : public juce::AudioProcessor
{
public:
    SlapTapeEchoAudioProcessor();
    ~SlapTapeEchoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Tap tempo: call on each tap; sets the manual tempo and switches the tempo source to Manual. */
    void tapTempo();
    double getCurrentBpm() const { return lastBpm.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void  setPlain (const char* id, float v);
    echo::Params buildParams (double bpm) const;

    echo::Engine engine;
    int currentProgram = 0;
    std::atomic<double> lastBpm { 120.0 };
    double lastTapMs = 0.0;
    std::vector<double> tapIntervals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlapTapeEchoAudioProcessor)
};
