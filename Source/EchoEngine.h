#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
    EchoVoice
    ---------
    Mono signal path for one channel of an original slap/tape-style echo effect:

        input -> [delay line] -> tone filter (LP + HP) -> saturation -> feedback
                                         |
                                         +--> wet output (mixed with dry outside this class)

    The delay time can be modulated by a slow LFO to emulate tape wow/flutter.
    This is an original implementation (not a derivative of any commercial product).
*/
class EchoVoice
{
public:
    void prepare (double sampleRate, int maximumBlockSize)
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maximumBlockSize, 1 };

        delayLine.prepare (spec);
        delayLine.setMaximumDelayInSamples ((int) (sampleRate * kMaxDelaySeconds) + 4);

        lowpass.reset();
        highpass.reset();

        updateToneFilters (8000.0f, 120.0f);

        lfoPhase = phaseOffset;
    }

    void reset()
    {
        delayLine.reset();
        lowpass.reset();
        highpass.reset();
        lfoPhase = phaseOffset;
    }

    void setPhaseOffset (float radians) { phaseOffset = radians; }

    void updateToneFilters (float lowpassHz, float highpassHz)
    {
        if (sr <= 0.0)
            return;

        lowpass.coefficients  = juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, juce::jlimit (200.0f, 20000.0f, lowpassHz));
        highpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, juce::jlimit (20.0f,  2000.0f,  highpassHz));
    }

    /** Processes one sample. All parameters are already smoothed/resolved by the caller. */
    float processSample (float inputSample,
                          float delayTimeMs,
                          float feedbackAmount,
                          float modRateHz,
                          float modDepthMs,
                          float driveAmount)
    {
        // --- LFO for tape-style wobble on the delay time ---
        float modSamples = 0.0f;
        if (modDepthMs > 0.0001f)
        {
            modSamples = std::sin (lfoPhase) * (modDepthMs * 0.001f) * (float) sr;
            lfoPhase += (float) (2.0 * juce::MathConstants<double>::pi * modRateHz / sr);
            if (lfoPhase > juce::MathConstants<float>::twoPi)
                lfoPhase -= juce::MathConstants<float>::twoPi;
        }

        float delaySamples = juce::jlimit (1.0f,
                                            (float) delayLine.getMaximumDelayInSamples() - 2.0f,
                                            (delayTimeMs * 0.001f) * (float) sr + modSamples);
        delayLine.setDelay (delaySamples);

        float delayed = delayLine.popSample (0);

        // Tone shaping inside the feedback path (classic analogue-echo behaviour:
        // repeats get progressively darker and lose sub-bass as they recirculate).
        float toned = lowpass.processSample (delayed);
        toned = highpass.processSample (toned);

        // Soft saturation, emulating tape/preamp coloration on the repeats.
        float driven = toned;
        if (driveAmount > 0.0001f)
        {
            float driveGain = 1.0f + driveAmount * 6.0f;
            driven = std::tanh (toned * driveGain) / std::tanh (driveGain);
        }

        // Feed input + feedback back into the line.
        float toWrite = inputSample + driven * feedbackAmount;
        delayLine.pushSample (0, toWrite);

        return driven; // wet signal returned; caller mixes with dry
    }

    static constexpr double kMaxDelaySeconds = 2.5;

private:
    double sr = 44100.0;
    float lfoPhase = 0.0f;
    float phaseOffset = 0.0f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine { (int) (44100 * kMaxDelaySeconds) };

    juce::dsp::IIR::Filter<float> lowpass;
    juce::dsp::IIR::Filter<float> highpass;
};
