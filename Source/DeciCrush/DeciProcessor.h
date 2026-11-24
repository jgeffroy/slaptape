#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "DeciEngine.h"

/**
    DeciCrush - an original bit-crusher / sample-rate reducer.

    Modules: Preamp, Quantizer (bits, DC shift, dither), Resampler (frequency, jitter,
    approximative filter, images filter), multimode Filter (pre or post), FX mix, output.
    Written from scratch; not derived from any third-party product.
*/
class DeciCrushProcessor : public juce::AudioProcessor
{
public:
    DeciCrushProcessor();
    ~DeciCrushProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // --- basic MIDI learn (CC -> parameter) -------------------------------------------------
    static const juce::StringArray& learnableIds();
    void startMidiLearn (const juce::String& paramId) { learnTarget.store (learnableIds().indexOf (paramId)); }
    void cancelMidiLearn()                            { learnTarget.store (-1); }
    bool isLearning (const juce::String& paramId) const { return learnTarget.load() == learnableIds().indexOf (paramId); }
    void clearMidiMapping (const juce::String& paramId);
    int  getMappedCc (const juce::String& paramId) const;

    // --- editor scale (saved with the plugin state, not with presets) ---------------------
    float getGuiScale() const { return guiScale; }
    void  setGuiScale (float s) { guiScale = s; }

private:
    void setParameterPlain (const char* id, float plainValue);

    void applyQuality (int q, int maxQ);
    int  latencyFor (int q) const;
    double lfoValue (int shape, double phase);
    juce::String midiMapToString() const;
    void midiMapFromString (const juce::String&);

    std::array<deci::Channel, 2> channels;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers; // x2, x4, x8
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 }, wetDelay { 8192 };
    juce::AudioBuffer<float> dryBuffer;
    int currentQuality = -1, currentMaxQuality = -1, currentLatency = 0, maxBlock = 512;
    double lfoPhase = 0.0, lfoRandom = 0.0, lfoLastPhase = 0.0;
    juce::Random lfoRng;

    std::array<std::atomic<int>, 128> ccMap;      // cc -> index in learnableIds(), or -1
    std::atomic<int> learnTarget { -1 };
    float guiScale = 1.0f;
    juce::SmoothedValue<double> preampSmoothed, mixSmoothed, outSmoothed;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeciCrushProcessor)
};
