#pragma once

// HeatSat saturation engine - independent of JUCE, so it can be unit-tested on its own.
//
// Chain (per channel):
//   Low Cut (+ optional Thump bump) -> Tone (tilt, so it changes what gets distorted) -> Drive/Punish gain
//   -> [5 saturation styles, run oversampled] -> High Cut (6 dB/oct or steep 30 dB/oct) -> Output (+ auto-gain)
//
// The five styles are original saturation algorithms. Their names (A, E, N, T, P) only mirror the
// kinds of character documented for this type of effect: tape-style preamp, console-style,
// heavy low-end transformer-style, triode (even harmonics) and pentode (odd harmonics).

#include "../EchoEngine.h"   // Biquad
#include <algorithm>
#include <cmath>

namespace heat
{
using echo::Biquad;
constexpr double kPi = echo::kPi;

enum Style { StyleA = 0, StyleE, StyleN, StyleT, StyleP };

// Stateless approximation of each style's curve (used for auto-gain calibration and tests).
inline double staticCurve (int style, double x)
{
    switch (style)
    {
        case StyleA: return std::tanh (x + 0.06) - std::tanh (0.06);
        case StyleE: return x >= 0 ? std::tanh (1.3 * x) / 1.3 : std::tanh (0.8 * x) / 0.8;
        case StyleN: return std::tanh (x + 0.05) - std::tanh (0.05);
        case StyleT: { const double b = 0.6, t = std::tanh (b); return (std::tanh (x + b) - t) / (1.0 - t * t); }
        default:     return x / std::pow (1.0 + std::pow (std::abs (x), 3.0), 1.0 / 3.0);
    }
}

// Stateful shaper, run at the oversampled rate.
struct Shaper
{
    Biquad presence, air, lowShelf, midFocus, lowSplit;
    double lp = 0, dcX = 0, dcY = 0, fs = 44100.0, dcR = 0.999;
    int style = StyleA;

    void configure (int s, double sampleRate)
    {
        style = s; fs = sampleRate;
        presence.peak (fs, 3200.0, 1.0, 1.0);
        air.highShelf (fs, 9000.0, 2.5);       // E: smooth, airy top
        lowShelf.lowShelf (fs, 90.0, 2.0);     // E: full low end
        midFocus.peak (fs, 900.0, 0.7, 1.5);   // N: focused (not narrow) mids
        lowSplit.lowpass (fs, 150.0);
        dcR = 1.0 - 2.0 * kPi * 8.0 / fs;
    }
    void reset() { presence.reset(); air.reset(); lowShelf.reset(); midFocus.reset(); lowSplit.reset(); lp = dcX = dcY = 0; }

    double dcBlock (double x)
    {
        dcY = x - dcX + dcR * dcY;
        dcX = x;
        return dcY;
    }

    double process (double x)
    {
        switch (style)
        {
            case StyleA:
            {
                const double y = staticCurve (StyleA, x);
                lp += (y - lp) * (1.0 - std::exp (-2.0 * kPi * 16000.0 / fs));
                return dcBlock (lp);
            }
            case StyleE:
                return dcBlock (staticCurve (StyleE, air.process (lowShelf.process (presence.process (x)))));
            case StyleN:
            {
                const double xf   = midFocus.process (x);
                const double low  = lowSplit.process (xf);
                const double rest = xf - low;
                const double restSat = std::tanh (rest + 0.05) - std::tanh (0.05);
                const double lowSat  = std::tanh (2.2 * low) / 2.2 * 1.3;
                return dcBlock (restSat + lowSat);
            }
            case StyleT:
                return dcBlock (staticCurve (StyleT, x));
            default:
                return staticCurve (StyleP, x);
        }
    }
};

struct OnePole
{
    double a = 1.0, z = 0.0;
    void lowpass (double fs, double f) { a = 1.0 - std::exp (-2.0 * kPi * std::clamp (f, 20.0, fs * 0.45) / fs); }
    void reset() { z = 0.0; }
    double process (double x) { z += (x - z) * a; return z; }
};

struct Settings
{
    double fs = 44100.0;
    int    style = StyleA;
    double drive = 0.0;       // 0..10
    bool   punish = false;    // +20 dB
    double lowCutHz = 20.0;
    bool   thump = false;
    double tone = 0.0;        // -1 (dark) .. +1 (bright)
    double highCutHz = 20000.0;
    bool   steep = false;
};

inline double driveGain (const Settings& s)
{
    const double db = s.drive * 3.0 + (s.punish ? 20.0 : 0.0);
    return std::pow (10.0, db / 20.0);
}

// Gain that keeps the output level of a -12 dBFS-peak sine roughly constant as drive increases.
inline double autoGain (const Settings& s)
{
    const double g = driveGain (s), amp = 0.25;
    double sum = 0.0;
    const int n = 128;
    for (int i = 0; i < n; ++i)
    {
        const double y = staticCurve (s.style, g * amp * std::sin (2.0 * kPi * i / n));
        sum += y * y;
    }
    const double rmsOut = std::sqrt (sum / n);
    if (rmsOut < 1e-9) return 1.0;
    return std::clamp ((amp / std::sqrt (2.0)) / rmsOut, 0.05, 4.0);
}

// Pre/post filters (base rate). The shaper itself runs at the oversampled rate.
struct Channel
{
    Biquad lowCut, thumpBump, tiltLow, tiltHigh, hc1, hc2;
    OnePole hcSimple, hcFirst;
    Shaper shaper;
    bool steep = false, lowCutActive = false;

    void configure (const Settings& s, double osFactor)
    {
        lowCutActive = s.lowCutHz > 21.0;
        if (lowCutActive) lowCut.highpass (s.fs, s.lowCutHz); else lowCut.identity();
        if (s.thump && lowCutActive) thumpBump.peak (s.fs, s.lowCutHz, 1.0, 4.0); else thumpBump.identity();

        tiltLow.lowShelf   (s.fs, 350.0,  -7.0 * s.tone);
        tiltHigh.highShelf (s.fs, 2500.0,  7.0 * s.tone);

        steep = s.steep;
        hcSimple.lowpass (s.fs, s.highCutHz);
        hcFirst.lowpass  (s.fs, s.highCutHz);
        hc1.lowpass (s.fs, s.highCutHz, 0.618);
        hc2.lowpass (s.fs, s.highCutHz, 1.618);

        shaper.configure (s.style, s.fs * osFactor);
    }

    void reset()
    {
        lowCut.reset(); thumpBump.reset(); tiltLow.reset(); tiltHigh.reset(); hc1.reset(); hc2.reset();
        hcSimple.reset(); hcFirst.reset(); shaper.reset();
    }

    double pre (double x, double gain)
    {
        double y = lowCut.process (x);
        y = thumpBump.process (y);
        y = tiltLow.process (y);      // Tone acts before saturation
        y = tiltHigh.process (y);
        return y * gain;
    }

    double post (double x)
    {
        double y = x;
        if (steep)
        {
            y = hcFirst.process (y);     // 5th order: one pole + two biquads (about 30 dB/oct)
            y = hc1.process (y);
            y = hc2.process (y);
        }
        else
            y = hcSimple.process (y);    // 6 dB/oct
        return y;
    }
};

} // namespace heat
