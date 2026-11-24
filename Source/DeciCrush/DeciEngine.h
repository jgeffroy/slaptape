#pragma once

// DeciCrush DSP engine - independent of JUCE so it can be unit-tested on its own.
//
// Signal flow (per channel):
//   Preamp -> [Filter (Pre)] -> Approximative filter -> Resampler (hold + jitter)
//          -> Quantizer (bit depth, mid-tread/mid-riser, dither) -> Images filter
//          -> [Filter (Post)] -> DC blocker
//
// Original implementation, written from the publicly described *behaviour* of
// this kind of effect (bit-depth reduction, sample-rate reduction, anti-alias
// and anti-image filtering). It contains no third-party code.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace deci
{
constexpr double kPi = 3.14159265358979323846;

// Zero-delay-feedback state-variable filter (double precision, stable at very low cutoffs).
struct SVF
{
    double g = 0.0, k = 1.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    double ic1 = 0.0, ic2 = 0.0;
    double lp = 0.0, bp = 0.0, hp = 0.0, notch = 0.0;

    void set (double cutoff, double q, double sampleRate)
    {
        cutoff = std::clamp (cutoff, 5.0, sampleRate * 0.49);
        g  = std::tan (kPi * cutoff / sampleRate);
        k  = 1.0 / std::max (q, 0.05);
        a1 = 1.0 / (1.0 + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void reset() { ic1 = ic2 = 0.0; lp = bp = hp = notch = 0.0; }

    void tick (double v0)
    {
        const double v3 = v0 - ic2;
        const double v1 = a1 * ic1 + a2 * v3;
        const double v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;
        lp    = v2;
        bp    = k * v1;            // unity-gain band-pass
        hp    = v0 - k * v1 - v2;
        notch = v0 - k * v1;
    }
};

// Butterworth low-pass built from N second-order sections.
template <int N>
struct LowpassCascade
{
    SVF stage[N];
    const double* qs = nullptr;

    void configure (const double* qValues) { qs = qValues; }

    void set (double cutoff, double sampleRate)
    {
        for (int i = 0; i < N; ++i)
            stage[i].set (cutoff, qs[i], sampleRate);
    }

    void reset() { for (auto& s : stage) s.reset(); }

    double process (double x)
    {
        for (int i = 0; i < N; ++i)
        {
            stage[i].tick (x);
            x = stage[i].lp;
        }
        return x;
    }
};

static constexpr double kQ6[3] = { 0.5176, 0.7071, 1.9319 };                 // 6th order Butterworth
static constexpr double kQ8[4] = { 0.5098, 0.6013, 0.8999, 2.5629 };         // 8th order Butterworth

enum FilterType { FilterOff = 0, FilterLP, FilterBP, FilterHP, FilterBR };

struct Params
{
    double sampleRate = 44100.0;

    bool   quantizerOn = true;
    int    bits        = 12;
    bool   dcShift     = true;      // true = mid-tread (zero level exists), false = mid-riser
    double dither      = 0.0;       // 0..1 (1 = 1 LSB TPDF)

    bool   resamplerOn = true;
    double frequency   = 22050.0;   // target sample rate in Hz
    double jitter      = 0.0;       // 0..1
    bool   approxOn    = true;
    double approxDevOct = 0.0;      // octaves, offset from target Nyquist
    bool   imagesOn    = false;
    double imagesShiftOct = 0.0;    // octaves, offset from target Nyquist

    int    filterType  = FilterOff;
    double filterCutoff = 8000.0;
    double filterReso   = 0.0;      // 0..1
    bool   filterPost  = false;     // false = before crushing, true = after

    bool   dcBlock     = true;      // output DC blocker (disabled only in unit tests)

    bool resamplerActive() const { return resamplerOn && frequency < sampleRate * 0.999; }
};

struct Channel
{
    LowpassCascade<3> approx;
    LowpassCascade<4> images;
    SVF               filter;

    double   hold = 0.0, counter = 0.0;
    double   dcX = 0.0, dcY = 0.0;
    uint32_t rng = 0x12345678u;

    Channel()
    {
        approx.configure (kQ6);
        images.configure (kQ8);
    }

    void seed (uint32_t s) { rng = s ? s : 0x9E3779B9u; }

    void reset()
    {
        approx.reset();
        images.reset();
        filter.reset();
        hold = counter = dcX = dcY = 0.0;
    }

    double noise() // uniform in [-1, 1]
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (double) rng / 2147483647.5 - 1.0;
    }

    // Recompute filter coefficients (call once per block).
    void updateCoefficients (const Params& p)
    {
        if (p.resamplerActive())
        {
            const double nyquist = p.frequency * 0.5;
            approx.set (nyquist * std::pow (2.0, p.approxDevOct), p.sampleRate);
            images.set (nyquist * std::pow (2.0, p.imagesShiftOct), p.sampleRate);
        }

        double q;
        if (p.filterType == FilterLP || p.filterType == FilterHP)
            q = 0.7071 * std::pow (30.0, p.filterReso);              // resonance
        else
            q = 0.5 * std::pow (40.0, 1.0 - p.filterReso);           // bandwidth: higher = wider

        filter.set (p.filterCutoff, q, p.sampleRate);
    }

    double runFilter (double x, int type)
    {
        filter.tick (x);
        switch (type)
        {
            case FilterLP: return filter.lp;
            case FilterBP: return filter.bp;
            case FilterHP: return filter.hp;
            case FilterBR: return filter.notch;
            default:       return x;
        }
    }

    double process (double x, const Params& p, double preampGain)
    {
        x *= preampGain;

        if (p.filterType != FilterOff && ! p.filterPost)
            x = runFilter (x, p.filterType);

        const bool resActive = p.resamplerActive();

        if (resActive)
        {
            if (p.approxOn)
                x = approx.process (x);

            counter -= 1.0;
            if (counter <= 0.0)
            {
                hold = x;
                double period = p.sampleRate / p.frequency;
                if (p.jitter > 0.0)
                    period *= 1.0 + p.jitter * 0.6 * noise();
                counter += std::max (period, 0.25);
            }
            x = hold;
        }

        if (p.quantizerOn)
        {
            const double levels = std::ldexp (1.0, std::clamp (p.bits, 1, 24));
            const double step   = 2.0 / levels;

            x = std::clamp (x, -1.0, 1.0);

            if (p.dither > 0.0)
                x += p.dither * step * 0.5 * (noise() + noise());   // TPDF, +/- 1 LSB at 100 %

            double y = p.dcShift ? step * std::round (x / step)
                                 : step * (std::floor (x / step) + 0.5);

            const double limit = p.dcShift ? 1.0 : 1.0 - step * 0.5;
            x = std::clamp (y, -limit, limit);
        }

        if (resActive && p.imagesOn)
            x = images.process (x);

        if (p.filterType != FilterOff && p.filterPost)
            x = runFilter (x, p.filterType);

        // DC blocker (mid-riser quantization leaves a DC offset at silence).
        if (! p.dcBlock)
            return x;

        const double r = 1.0 - 2.0 * kPi * 5.0 / p.sampleRate;
        dcY = x - dcX + r * dcY;
        dcX = x;
        return dcY;
    }
};

} // namespace deci
