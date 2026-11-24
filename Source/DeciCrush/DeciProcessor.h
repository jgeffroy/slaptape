#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
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
    bool acceptsMidi() const override  { return false; }
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

private:
    void setParameterPlain (const char* id, float plainValue);

    std::array<deci::Channel, 2> channels;
    juce::SmoothedValue<double> preampSmoothed, mixSmoothed, outSmoothed;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeciCrushProcessor)
};
