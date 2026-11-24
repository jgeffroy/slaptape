#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SatEngine.h"
#include <atomic>

/**
    HeatSat - an original multi-style saturator.

    Five styles (A, E, N, T, P), Drive 0-10, Punish (+20 dB), Auto gain, Low Cut with Thump,
    Tone, High Cut with Steep switch, Output and Mix. Saturation runs 4x oversampled.
    Written from scratch; not derived from any third-party product.
*/
class HeatSatProcessor : public juce::AudioProcessor
{
public:
    HeatSatProcessor();
    ~HeatSatProcessor() override = default;

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

    // Attitude meter: level hitting the saturation stage (after Low Cut, Tone and Drive), in dB
    float getAttitudeDb() const { return attitudeDb.load(); }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void setParameterPlain (const char* id, float plainValue);
    heat::Settings readSettings() const;

    static constexpr int kOsStages = 2; // 2^2 = 4x oversampling

    std::array<heat::Channel, 2> channels;
    juce::dsp::Oversampling<float> oversampling { 2, kOsStages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 };
    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float> gainSmoothed, autoSmoothed, outSmoothed, mixSmoothed;
    int currentProgram = 0;
    std::atomic<float> attitudeDb { -60.0f };
    float meterState = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeatSatProcessor)
};
