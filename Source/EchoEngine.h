#pragma once

// SlapTape Echo engine (v2) - independent of JUCE so it can be unit-tested on its own.
//
// Original implementation of a multi-mode echo machine:
//   modes     : Single, Dual, Ping-Pong, Rhythm (16 taps)
//   timing    : time in ms or note values (sync handled by the caller), Groove (swing/shuffle),
//               Feel (rush/drag), Accent, Prime Numbers
//   character : 28 styles, saturation types, wobble (5 shapes), diffusion (post/loop),
//               3-band echo EQ + per-repeat decay EQ, low/high cut
//
// Nothing here is copied from, or derived from the code of, any commercial product.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace echo
{
constexpr double kPi = 3.14159265358979323846;

struct Rng
{
    uint32_t s = 0x1234567u;
    double next() // [-1, 1]
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (double) s / 2147483647.5 - 1.0;
    }
};

// Soft limiter used inside the echo loops so that self-oscillation stays bounded (max about 1.6).
inline double softLimit (double x)
{
    const double a = std::abs (x);
    if (a <= 1.0) return x;
    return std::copysign (1.0 + 0.6 * std::tanh ((a - 1.0) / 0.6), x);
}

// ---------------------------------------------------------------------------
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }
    void identity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0;
        a1 = na1 / na0; a2 = na2 / na0;
    }

    static double clampF (double fs, double f) { return std::clamp (f, 10.0, fs * 0.45); }

    void lowpass (double fs, double f, double q = 0.7071)
    {
        f = clampF (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double fs, double f, double q = 0.7071)
    {
        f = clampF (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandpass (double fs, double f, double q)
    {
        f = clampF (fs, f);
        const double w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void peak (double fs, double f, double q, double gainDb)
    {
        if (std::abs (gainDb) < 0.01) { identity(); return; }
        f = clampF (fs, f);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2 * kPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double fs, double f, double gainDb)
    {
        if (std::abs (gainDb) < 0.01) { identity(); return; }
        f = clampF (fs, f);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2 * kPi * f / fs, c = std::cos (w);
        const double b = 2 * std::sqrt (A) * (std::sin (w) / std::sqrt (2.0));
        set (A * ((A + 1) - (A - 1) * c + b), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - b),
             (A + 1) + (A - 1) * c + b, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - b);
    }
    void highShelf (double fs, double f, double gainDb)
    {
        if (std::abs (gainDb) < 0.01) { identity(); return; }
        f = clampF (fs, f);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2 * kPi * f / fs, c = std::cos (w);
        const double b = 2 * std::sqrt (A) * (std::sin (w) / std::sqrt (2.0));
        set (A * ((A + 1) + (A - 1) * c + b), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - b),
             (A + 1) - (A - 1) * c + b, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - b);
    }
};

// ---------------------------------------------------------------------------
// Circular delay buffer. Call read() BEFORE write() for the current sample; read(d) returns the
// sample written d samples ago (d >= 2), with 4-point Hermite interpolation.
struct DelayBuf
{
    std::vector<float> buf;
    int mask = 0, w = 0;

    void init (int minSize)
    {
        int n = 16;
        while (n < minSize + 8) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    int capacity() const { return mask + 1; }
    void write (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }

    float read (double d) const
    {
        d = std::clamp (d, 2.0, (double) (mask - 4));
        const int i0 = (int) d;
        const float t = (float) (d - i0);
        const float sm1 = buf[(size_t) ((w - i0 + 1) & mask)];
        const float s0  = buf[(size_t) ((w - i0)     & mask)];
        const float s1  = buf[(size_t) ((w - i0 - 1) & mask)];
        const float s2  = buf[(size_t) ((w - i0 - 2) & mask)];
        const float c1 = 0.5f * (s1 - sm1);
        const float c2 = sm1 - 2.5f * s0 + 2.0f * s1 - 0.5f * s2;
        const float c3 = 0.5f * (s2 - sm1) + 1.5f * (s0 - s1);
        return ((c3 * t + c2) * t + c1) * t + s0;
    }
};

// ---------------------------------------------------------------------------
enum WobbleShape { WobTriangle = 0, WobSine, WobSquare, WobRandomWalk, WobRandomSH };

struct Wobble
{
    double phase = 0, sq = 0, walk = 0, sh = 0, shTarget = 0;
    Rng rng;

    void reset() { phase = sq = walk = sh = shTarget = 0; }

    double next (double rateHz, double fs, int shape, double phaseOffset)
    {
        phase += rateHz / fs;
        bool wrapped = false;
        if (phase >= 1.0) { phase -= 1.0; wrapped = true; }
        double p = phase + phaseOffset;
        p -= std::floor (p);

        switch (shape)
        {
            case WobTriangle: return 4.0 * std::abs (p - 0.5) - 1.0;
            case WobSine:     return std::sin (2 * kPi * p);
            case WobSquare:
            {
                const double target = p < 0.5 ? 1.0 : -1.0;
                sq += (target - sq) * (1.0 - std::exp (-2 * kPi * 40.0 / fs));
                return sq;
            }
            case WobRandomWalk:
                walk += rng.next() * 0.004 * std::sqrt (rateHz + 0.1);
                walk *= 0.9998;
                walk = std::clamp (walk, -1.0, 1.0);
                return walk;
            default:
                if (wrapped) shTarget = rng.next();
                sh += (shTarget - sh) * (1.0 - std::exp (-2 * kPi * 20.0 / fs));
                return sh;
        }
    }
};

// ---------------------------------------------------------------------------
enum SatType { SatClean = 0, SatTape, SatWarm, SatPump, SatDirt, SatHardLimit, SatMultiband };

struct Saturator
{
    double env = 0, lp = 0;
    double mb1 = 0, mb2 = 0, mbEnv[3] = { 0, 0, 0 };   // multiband compressor state

    void reset() { env = lp = 0; mb1 = mb2 = 0; mbEnv[0] = mbEnv[1] = mbEnv[2] = 0; }

    double process (double x, double amount, int type, double fs)
    {
        if (type == SatClean || amount <= 0.0001)
            return x;

        switch (type)
        {
            case SatTape:
            {
                const double g = 1.0 + amount * 6.0;
                double y = std::tanh (g * x) / g * (1.0 + amount * 0.5);
                const double a = 1.0 - std::exp (-2 * kPi * (11000.0 - 5000.0 * amount) / fs);
                lp += (y - lp) * a;
                return lp;
            }
            case SatWarm:
            {
                const double g = 1.0 + amount * 4.0, b = 0.25 * amount;
                double y = (std::tanh (g * x + b) - std::tanh (b)) / g;
                const double a = 1.0 - std::exp (-2 * kPi * (9000.0 - 3500.0 * amount) / fs);
                lp += (y - lp) * a;
                return lp * (1.0 + amount * 0.3);
            }
            case SatPump:
            {
                const double ax = std::abs (x);
                const double coef = ax > env ? 1.0 - std::exp (-1.0 / (0.003 * fs)) : 1.0 - std::exp (-1.0 / (0.12 * fs));
                env += (ax - env) * coef;
                return x / (1.0 + amount * 4.0 * env) * (1.0 + amount * 0.8);
            }
            case SatDirt:
            {
                const double g = 1.0 + amount * 10.0;
                const double v = std::clamp (x * g, -1.0, 1.0);
                return (1.5 * v - 0.5 * v * v * v) / (1.0 + amount * 2.5);
            }
            case SatMultiband:
            {
                // three-band compressor (low < 250 Hz < mid < 3 kHz < high), each band squashed on its own
                mb1 += (x - mb1) * (1.0 - std::exp (-2 * kPi * 250.0 / fs));
                mb2 += (x - mb2) * (1.0 - std::exp (-2 * kPi * 3000.0 / fs));
                const double band[3] = { mb1, mb2 - mb1, x - mb2 };
                const double att = 1.0 - std::exp (-1.0 / (0.002 * fs)), rel = 1.0 - std::exp (-1.0 / (0.15 * fs));
                double y = 0.0;
                for (int k = 0; k < 3; ++k)
                {
                    const double ax = std::abs (band[k]);
                    mbEnv[k] += (ax - mbEnv[k]) * (ax > mbEnv[k] ? att : rel);
                    y += band[k] / (1.0 + amount * 10.0 * mbEnv[k]);
                }
                return y * (1.0 + amount * 0.6);
            }
            default: // hard limit
            {
                const double c = 1.0 - 0.75 * amount;
                return std::clamp (x, -c, c);
            }
        }
    }
};

// ---------------------------------------------------------------------------
struct Allpass
{
    std::vector<float> buf;
    int idx = 0, len = 1;

    void init (int maxLen) { buf.assign ((size_t) std::max (maxLen, 2), 0.0f); idx = 0; len = std::min (len, maxLen); }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }
    void setLength (int l) { len = std::clamp (l, 1, (int) buf.size()); if (idx >= len) idx = 0; }

    double process (double x, double g)
    {
        const double d = buf[(size_t) idx];
        const double w = x + g * d;
        buf[(size_t) idx] = (float) w;
        if (++idx >= len) idx = 0;
        return -g * w + d;
    }
};

struct Diffuser
{
    static constexpr int kStages = 4;
    Allpass ap[kStages];
    double fs = 44100.0, size = -1.0;

    void init (double sampleRate)
    {
        fs = sampleRate;
        static const double baseMs[kStages] = { 3.1, 4.7, 7.3, 11.9 };
        for (int i = 0; i < kStages; ++i)
            ap[i].init ((int) (baseMs[i] * 0.001 * fs * 4.0) + 8);
        size = -1.0;
        setSize (0.5);
    }
    void setSize (double s)
    {
        s = std::clamp (s, 0.0, 1.0);
        if (std::abs (s - size) < 1e-4) return;
        size = s;
        static const double baseMs[kStages] = { 3.1, 4.7, 7.3, 11.9 };
        for (int i = 0; i < kStages; ++i)
            ap[i].setLength ((int) (baseMs[i] * 0.001 * fs * (0.4 + 3.6 * s)));
    }
    void clear() { for (auto& a : ap) a.clear(); }

    double process (double x, double amount)
    {
        if (amount <= 0.001) return x;
        const double g = 0.3 + 0.4 * amount;
        double y = x;
        for (auto& a : ap) y = a.process (y, g);
        return x * (1.0 - amount) + y * amount;
    }
};

// ---------------------------------------------------------------------------
struct StageSettings
{
    double fs = 44100;
    double lcHz = 20, hcHz = 20000;       // user low cut / high cut
    double bwLow = 20, bwHigh = 20000;    // style bandwidth
    double narrow = 0.0;                  // 0..1 band-pass emphasis (radio/telephone styles)
    int    satType = SatClean; double satAmt = 0.0;
    double decLow = 0, decMid = 0, decHigh = 0;   // dB applied at each repeat
    double midHz = 1200.0;
    double diffAmt = 0.0, diffSize = 0.5;
};

// Processing applied at every pass of the echo loop (so repeats evolve progressively).
struct LoopStage
{
    Biquad lc, hc, bwHp, bwLp, nar, dLow, dMid, dHigh;
    Saturator sat;
    Diffuser diff;
    int satType = SatClean; double satAmt = 0.0, diffAmt = 0.0, narrow = 0.0, fs = 44100.0;

    void init (double sampleRate) { fs = sampleRate; diff.init (sampleRate); }

    void reset()
    {
        lc.reset(); hc.reset(); bwHp.reset(); bwLp.reset(); nar.reset(); dLow.reset(); dMid.reset(); dHigh.reset();
        sat.reset(); diff.clear();
    }

    void configure (const StageSettings& s)
    {
        fs = s.fs;
        lc.highpass (fs, s.lcHz);
        hc.lowpass  (fs, s.hcHz);
        bwHp.highpass (fs, s.bwLow);
        bwLp.lowpass  (fs, s.bwHigh);
        nar.bandpass (fs, std::sqrt (std::max (s.bwLow, 20.0) * std::max (s.bwHigh, 20.0)), 1.2);
        narrow = s.narrow;
        dLow.lowShelf (fs, 200.0, s.decLow);
        dMid.peak (fs, s.midHz, 0.8, s.decMid);
        dHigh.highShelf (fs, 4000.0, s.decHigh);
        satType = s.satType; satAmt = s.satAmt;
        diffAmt = s.diffAmt;
        diff.setSize (s.diffSize);
    }

    double process (double x)
    {
        double y = lc.process (x);
        y = hc.process (y);
        y = bwHp.process (y);
        y = bwLp.process (y);
        if (narrow > 0.001)
            y = y * (1.0 - narrow) + nar.process (y) * narrow * 1.6;
        y = dLow.process (y);
        y = dMid.process (y);
        y = dHigh.process (y);
        y = sat.process (y, satAmt, satType, fs);
        if (diffAmt > 0.001)
            y = diff.process (y, diffAmt);
        return y;
    }
};

// ---------------------------------------------------------------------------
// Echo loop made of two delay segments (A then B). Using two segments lets Groove produce
// swing/shuffle: echo 1 arrives after tA, echo 2 after tA + tB, echo 3 after 2*(tA + tB)/... etc.
// Reads a delay line at base + mod. In "clean" mode a jump of the base time is handled by a short
// crossfade between the old and new positions (no pitch glide); otherwise the read follows base directly.
struct XRead
{
    double cur = -1.0, nxt = 0.0, x = 1.0;
    void reset() { cur = -1.0; x = 1.0; }

    double read (const DelayBuf& d, double base, double mod, bool clean, double inc)
    {
        if (! clean || cur < 0.0)
        {
            cur = base; x = 1.0;
            return d.read (base + mod);
        }
        if (x >= 1.0 && std::abs (base - cur) > 0.5)
        {
            nxt = base; x = 0.0;
        }
        if (x < 1.0)
        {
            const double g = 0.5 - 0.5 * std::cos (kPi * x);
            const double v = (1.0 - g) * d.read (cur + mod) + g * d.read (nxt + mod);
            x += inc;
            if (x >= 1.0) cur = nxt;
            return v;
        }
        return d.read (cur + mod);
    }
};

struct GrooveLoop
{
    DelayBuf a, b;
    LoopStage sa, sb;
    XRead ra, rb;
    bool clean = false;
    double fadeInc = 0.001;

    struct Out { double echoA = 0, echoB = 0, fbOut = 0; };

    void init (double fs, int maxSamples)
    {
        a.init (maxSamples); b.init (maxSamples);
        sa.init (fs); sb.init (fs);
    }
    void reset() { a.clear(); b.clear(); sa.reset(); sb.reset(); ra.reset(); rb.reset(); }

    Out tick (double in, double fbIn, double baseA, double modA, double baseB, double modB,
              double fbGain, double accGainA, double accGainB)
    {
        const double yA = ra.read (a, baseA, modA, clean, fadeInc);
        const double yB = rb.read (b, baseB, modB, clean, fadeInc);

        const double pa = sa.process (yA);
        const double pb = sb.process (yB);

        b.write ((float) softLimit (pa * fbGain * accGainA));
        a.write ((float) softLimit (in + fbIn));

        Out o;
        o.echoA = pa;
        o.echoB = pb;
        o.fbOut = pb * fbGain * accGainB;
        return o;
    }
};

// ---------------------------------------------------------------------------
enum Mode { ModeSingle = 0, ModeDual, ModePingPong, ModeRhythm };
enum RhythmShape { ShapeDecay = 0, ShapeReverse, ShapeSwell, ShapeFade, ShapeNonLin };
enum PanShape { PanCenter = 0, PanDouble, PanAlt1, PanAlt2, PanSweepL, PanSweepR, PanWide };

struct Params
{
    int    mode = ModeSingle;
    double timeMs1 = 120, timeMs2 = 180;
    double feedback = 0.25;            // 0..1.1
    double groove = 0, feel = 0;       // -1..1
    double accent1 = 0, accent2 = 0;   // -1..1
    bool   prime = false;
    double lrOffsetMs = 0, width = 0.5, balance = 0;
    double fbMix = 0, fbBal = 0;
    double inputGain = 1, outputGain = 1;
    int    timeMode = 0;               // 0 = tape (pitch glide on time changes), 1 = clean (crossfade)
    double glideMs = 40.0;             // glide time in tape mode

    StageSettings stage;               // loop stage settings (cuts, style bandwidth, decay, sat, loop diffusion)

    double eqLow = 0, eqMid = 0, eqHigh = 0, midHz = 1200;
    int    outSatType = SatClean; double outSatAmt = 0;
    double postDiffAmt = 0, diffSize = 0.5;

    double wobDepth = 0, wobRate = 0.6; int wobShape = WobSine;
    bool   wobFb = true; int wobSync = 0;

    // rhythm
    std::array<double, 16> taps {};
    double rhLenBeats = 4, bpm = 120, rhWarp = 0;
    int    rhShape = ShapeDecay, rhPan = PanCenter;
};

class Engine
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const int maxSamples = (int) (fs * 6.0);
        for (auto& l : loops) l.init (fs, maxSamples);
        rhythmBuf.init ((int) (fs * 12.0));
        for (auto& s : rhStage) s.init (fs);
        rhLoopStage.init (fs);
        for (auto& d : postDiff) d.init (fs);
        smoothCoef = 1.0 - std::exp (-1.0 / (0.04 * fs));
        wobA.rng.s = 0xA341316Cu; wobB.rng.s = 0xC8013EA4u;
        reset();
    }

    void reset()
    {
        for (auto& l : loops) l.reset();
        rhythmBuf.clear();
        for (auto& s : rhStage) s.reset();
        rhLoopStage.reset();
        for (auto& d : postDiff) d.clear();
        for (int c = 0; c < 2; ++c)
        {
            eqL[c].reset(); eqM[c].reset(); eqH[c].reset(); outSat[c].reset();
            prevFb[c] = 0.0;
        }
        wobA.reset(); wobB.reset();
        t1 = p.timeMs1; t2 = p.timeMs2;
    }

    void setParams (const Params& np)
    {
        p = np;
        smoothCoef = 1.0 - std::exp (-1.0 / (std::max (p.glideMs, 1.0) * 0.001 * fs));
        for (auto& l : loops)
        {
            l.sa.configure (p.stage); l.sb.configure (p.stage);
            l.clean = p.timeMode == 1;
            l.fadeInc = 1.0 / (0.03 * fs);   // 30 ms crossfade
        }
        for (auto& s : rhStage) { StageSettings st = p.stage; st.decLow = st.decMid = st.decHigh = 0; st.diffAmt = 0; s.configure (st); }
        rhLoopStage.configure (p.stage);
        for (int c = 0; c < 2; ++c)
        {
            eqL[c].lowShelf (fs, 200.0, p.eqLow);
            eqM[c].peak (fs, p.midHz, 0.8, p.eqMid);
            eqH[c].highShelf (fs, 4000.0, p.eqHigh);
            postDiff[c].setSize (p.diffSize);
        }
    }

    void process (float inL, float inR, float& wetL, float& wetR)
    {
        if (p.timeMode == 1)
        {
            t1 = p.timeMs1; t2 = p.timeMs2;
        }
        else
        {
            t1 += (p.timeMs1 - t1) * smoothCoef;
            t2 += (p.timeMs2 - t2) * smoothCoef;
        }

        const double mL = inL * p.inputGain, mR = inR * p.inputGain, mM = 0.5 * (mL + mR);

        // wobble
        const double wd = p.wobDepth * 0.004 * fs;
        double rateR = p.wobRate, offR = 0.0;
        if (p.wobSync == 1) offR = 0.5;
        else if (p.wobSync == 2) rateR = p.wobRate * 1.07;
        const double lfoL = p.wobDepth > 0.0005 ? wobA.next (p.wobRate, fs, p.wobShape, 0.0)  : 0.0;
        const double lfoR = p.wobDepth > 0.0005 ? wobB.next (rateR,     fs, p.wobShape, offR) : 0.0;

        const double feelF = 1.0 + 0.2 * p.feel;
        const double gv = 0.34 * p.groove;
        const double primeSamples = p.prime ? 29.0 : 0.0;
        const double gA1 = 1.0 + std::clamp (p.accent1, -0.6, 0.6), gB1 = 1.0 / gA1;
        const double gA2 = 1.0 + std::clamp (p.accent2, -0.6, 0.6), gB2 = 1.0 / gA2;
        const double fbGain = p.feedback;

        double oL = 0, oR = 0;

        if (p.mode == ModeSingle)
        {
            const double base = t1 * 0.001 * fs * feelF;
            const double offs = p.lrOffsetMs * 0.001 * fs;
            for (int c = 0; c < 2; ++c)
            {
                const double ts = base + (c == 1 ? offs : 0.0);
                const double lfo = c == 0 ? lfoL : lfoR;
                auto o = loops[c].tick (c == 0 ? mL : mR, prevFb[c],
                                        ts * (1.0 + gv), wd * lfo,
                                        ts * (1.0 - gv) + primeSamples, p.wobFb ? wd * lfo : 0.0,
                                        fbGain, gA1, gB1);
                prevFb[c] = o.fbOut;
                (c == 0 ? oL : oR) = o.echoA + o.echoB;
            }
            // stereo width (mid/side)
            const double m = 0.5 * (oL + oR), s = 0.5 * (oL - oR) * (0.4 + 2.2 * p.width);
            oL = m + s; oR = m - s;
        }
        else if (p.mode == ModeDual)
        {
            const double ts0 = t1 * 0.001 * fs * feelF, ts1 = t2 * 0.001 * fs * feelF;
            const double offs = p.lrOffsetMs * 0.001 * fs;
            const double mix = std::clamp (p.fbMix, 0.0, 1.0);
            const double balL = 1.0 - std::max (0.0,  p.fbBal), balR = 1.0 - std::max (0.0, -p.fbBal);
            const double fb0 = ((1.0 - mix) * prevFb[0] + mix * prevFb[1]) * balL;
            const double fb1 = ((1.0 - mix) * prevFb[1] + mix * prevFb[0]) * balR;

            // true stereo: left input feeds the left echo, right input the right echo
            auto o0 = loops[0].tick (mL, fb0, ts0 * (1.0 + gv), wd * lfoL,
                                     ts0 * (1.0 - gv) + primeSamples, p.wobFb ? wd * lfoL : 0.0, fbGain, gA1, gB1);
            auto o1 = loops[1].tick (mR, fb1, (ts1 + offs) * (1.0 + gv), wd * lfoR,
                                     (ts1 + offs) * (1.0 - gv) + primeSamples, p.wobFb ? wd * lfoR : 0.0, fbGain, gA2, gB2);
            prevFb[0] = o0.fbOut; prevFb[1] = o1.fbOut;
            oL = o0.echoA + o0.echoB;
            oR = o1.echoA + o1.echoB;
        }
        else if (p.mode == ModePingPong)
        {
            const double ts = t1 * 0.001 * fs * feelF;
            const double offs = p.lrOffsetMs * 0.001 * fs;
            auto o = loops[0].tick (mM, prevFb[0], (ts + offs) * (1.0 + gv), wd * lfoL,
                                    ts * (1.0 - gv) + primeSamples, p.wobFb ? wd * lfoL : 0.0, fbGain, gA1, gB1);
            prevFb[0] = o.fbOut;
            const double w = std::clamp (p.width, 0.0, 1.0);
            oL = o.echoA * (0.5 + 0.5 * w) + o.echoB * (0.5 - 0.5 * w);
            oR = o.echoB * (0.5 + 0.5 * w) + o.echoA * (0.5 - 0.5 * w);
        }
        else // rhythm
        {
            processRhythm (mM, oL, oR);
        }

        // balance
        const double bL = 1.0 - std::max (0.0, p.balance), bR = 1.0 - std::max (0.0, -p.balance);
        oL *= bL; oR *= bR;

        // output stage: EQ, saturation, post-diffusion
        double out[2] = { oL, oR };
        for (int c = 0; c < 2; ++c)
        {
            double y = out[c];
            y = eqL[c].process (y);
            y = eqM[c].process (y);
            y = eqH[c].process (y);
            y = outSat[c].process (y, p.outSatAmt, p.outSatType, fs);
            if (p.postDiffAmt > 0.001)
                y = postDiff[c].process (y, p.postDiffAmt);
            out[c] = softLimit (y * p.outputGain);
        }
        wetL = (float) out[0];
        wetR = (float) out[1];
    }

    double tailSeconds() const { return 6.0; }

private:
    void processRhythm (double in, double& oL, double& oR)
    {
        const double beatSec = 60.0 / std::clamp (p.bpm, 20.0, 400.0);
        const double len = std::clamp (p.rhLenBeats * beatSec * fs, 64.0, (double) (rhythmBuf.capacity() - 16));
        const double stepLen = len / 16.0;
        const double warpExp = std::pow (2.0, p.rhWarp);

        double sumL = 0, sumR = 0;
        for (int i = 0; i < 16; ++i)
        {
            const double lvl = p.taps[(size_t) i];
            if (lvl <= 0.001) continue;

            double pos = std::pow ((i + 1) / 16.0, warpExp);
            double d = pos * len;
            if (i % 2 == 1) d += p.groove * 0.33 * stepLen;
            d = std::clamp (d, 2.0, len);

            const double u = i / 15.0;
            double shape = 1.0;
            switch (p.rhShape)
            {
                case ShapeDecay:   shape = std::exp (-2.5 * u); break;
                case ShapeReverse: shape = std::exp (-2.5 * (1.0 - u)); break;
                case ShapeSwell:   shape = std::sin (kPi * u) * 0.9 + 0.1; break;
                case ShapeFade:    shape = 1.0 - 0.9 * u; break;
                default:           shape = std::pow (1.0 - u, 2.2) * 0.9 + 0.1; break;
            }
            const double acc = 1.0 + ((i % 2 == 0) ? -p.accent1 : p.accent1) * 0.5;
            const double g = lvl * shape * std::max (acc, 0.0);

            double pan = 0.0;
            switch (p.rhPan)
            {
                case PanDouble:  pan = (i % 2) ? 1.0 : -1.0; break;
                case PanAlt1:    { static const double a[4] = { -1, 0, 1, 0 }; pan = a[i % 4]; break; }
                case PanAlt2:    { static const double a[4] = { -1, 1, 0, 0 }; pan = a[i % 4]; break; }
                case PanSweepL:  pan = -1.0 + 2.0 * u; break;
                case PanSweepR:  pan =  1.0 - 2.0 * u; break;
                case PanWide:    { static const double a[4] = { -1, -0.33, 0.33, 1 }; pan = a[i % 4]; break; }
                default: break;
            }
            const double ang = (pan + 1.0) * kPi / 4.0;
            const double v = rhythmBuf.read (d) * g;
            sumL += v * std::cos (ang) * 1.2;
            sumR += v * std::sin (ang) * 1.2;
        }

        const double endTap = rhythmBuf.read (len);
        const double fbSig = rhLoopStage.process (endTap) * p.feedback;
        rhythmBuf.write ((float) softLimit (in + fbSig));

        oL = rhStage[0].process (sumL);
        oR = rhStage[1].process (sumR);
    }

    double fs = 44100.0, smoothCoef = 0.001;
    Params p;
    std::array<GrooveLoop, 2> loops;
    DelayBuf rhythmBuf;
    std::array<LoopStage, 2> rhStage;
    LoopStage rhLoopStage;
    std::array<Diffuser, 2> postDiff;
    Biquad eqL[2], eqM[2], eqH[2];
    Saturator outSat[2];
    Wobble wobA, wobB;
    double prevFb[2] = { 0, 0 };
    double t1 = 120, t2 = 180;
};

} // namespace echo
