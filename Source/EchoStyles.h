#pragma once

#include "EchoEngine.h"

// Echo styles: each style is a set of character settings (bandwidth, saturation, wobble, diffusion,
// EQ and per-repeat decay). The names are generic descriptions of the sound, original to this plugin.
namespace echo
{
struct StyleDef
{
    const char* name;
    double bwLow, bwHigh;        // style bandwidth (Hz)
    double narrow;               // band-pass emphasis 0..1
    int    satType; double satScale, satBase;
    int    outSatType; double outSatAmt;
    double eqLow, eqMid, eqHigh; // dB
    double decLow, decMid, decHigh; // dB per repeat
    double diff, size; bool loopDiff;
    double wobDepth, wobRate; int wobShape;
};

//                                            bw lo  bw hi  narrow satType        scale base  outSat      amt   eqL  eqM  eqH   decL  decM  decH  diff  size  loop  wDep  wRate wShape
static const StyleDef kStyles[] =
{
    { "Master Tape",        40,  16000, 0.0, SatTape,  0.5, 0.05, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -0.8,  0.0, 0.5, false, 0.02, 0.5, WobSine },
    { "Studio Tape",        60,  12000, 0.0, SatTape,  1.0, 0.15, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.2,  0.0, 0.5, false, 0.04, 0.6, WobSine },
    { "Solid-State Tape",   80,   9500, 0.0, SatTape,  1.0, 0.10, SatClean,     0.0,  0.0, 0.0, 0.0, -0.3,  0.0, -1.5,  0.0, 0.5, false, 0.06, 0.9, WobTriangle },
    { "Chamber Tape",      100,   7500, 0.0, SatWarm,  1.2, 0.25, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -2.0,  0.1, 0.4, true,  0.12, 0.4, WobRandomWalk },
    { "Tube Tape",          90,   8000, 0.0, SatWarm,  1.0, 0.30, SatClean,     0.0,  0.0, 2.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.05, 0.6, WobSine },
    { "Cheap Tape",        150,   6500, 0.0, SatTape,  1.0, 0.35, SatPump,      0.15, 0.0, 0.0, 1.0,  0.0,  0.0, -2.5,  0.0, 0.5, false, 0.18, 1.3, WobRandomSH },
    { "Bucket Chorus",     120,   5500, 0.0, SatWarm,  1.0, 0.15, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -2.2,  0.0, 0.5, false, 0.35, 0.8, WobSine },
    { "Bucket Delay",      100,   6000, 0.0, SatWarm,  1.0, 0.20, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -2.5,  0.0, 0.5, false, 0.08, 0.4, WobTriangle },
    { "Oil Can",           140,   4800, 0.0, SatWarm,  1.0, 0.25, SatClean,     0.0,  0.0, 0.0, 0.0, -0.5,  0.0, -3.0,  0.0, 0.5, false, 0.30, 0.6, WobRandomWalk },
    { "Warble Compressed", 120,   5500, 0.0, SatPump,  1.0, 0.35, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -2.0,  0.0, 0.5, false, 0.25, 1.1, WobSine },
    { "Analog Rack",        80,   8000, 0.0, SatWarm,  1.0, 0.15, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.8,  0.0, 0.5, false, 0.02, 0.5, WobSine },
    { "Digital Chorus",     60,  14000, 0.0, SatClean, 1.0, 0.00, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -0.5,  0.0, 0.5, false, 0.40, 0.6, WobTriangle },
    { "Analog Chorus",     100,   9000, 0.0, SatWarm,  1.0, 0.10, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.45, 0.5, WobSine },
    { "Vibrato",            80,  12000, 0.0, SatClean, 1.0, 0.00, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0,  0.0,  0.0, 0.5, false, 0.90, 4.5, WobSine },
    { "Saturated",          80,   9000, 0.0, SatTape,  1.0, 0.70, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.03, 0.6, WobSine },
    { "Fat",                60,   6500, 0.0, SatWarm,  1.0, 0.60, SatClean,     0.0,  2.0, 0.0, 0.0,  0.0,  0.0, -1.5,  0.0, 0.5, false, 0.03, 0.5, WobSine },
    { "Telephone",         400,   3200, 0.7, SatDirt,  1.0, 0.15, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.00, 0.5, WobSine },
    { "AM Radio",          250,   4500, 0.3, SatPump,  1.0, 0.40, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.02, 0.5, WobSine },
    { "FM Radio",           90,  14000, 0.0, SatPump,  1.0, 0.70, SatHardLimit, 0.30, 0.0, 0.0, 0.0,  0.0,  0.0, -0.5,  0.0, 0.5, false, 0.00, 0.5, WobSine },
    { "Shortwave",         500,   2800, 0.85, SatDirt, 1.0, 0.30, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.15, 1.7, WobRandomSH },
    { "Transmitter",       300,   3800, 0.5, SatDirt,  1.0, 0.50, SatClean,     0.0,  0.0, 4.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.00, 0.5, WobSine },
    { "Digital Delay",      20,  20000, 0.0, SatClean, 1.0, 0.00, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0,  0.0,  0.0, 0.5, false, 0.00, 0.5, WobSine },
    { "Distressed",        100,   8000, 0.0, SatPump,  1.0, 0.60, SatDirt,      0.35, 0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.03, 0.6, WobSine },
    { "Limited",            40,  14000, 0.0, SatClean, 1.0, 0.00, SatHardLimit, 0.50, 0.0, 0.0, 0.0,  0.0,  0.0, -0.3,  0.0, 0.5, false, 0.00, 0.5, WobSine },
    { "Distorted",          80,   9000, 0.0, SatDirt,  1.0, 0.80, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.0, 0.5, false, 0.00, 0.5, WobSine },
    { "Ambient",           100,   9000, 0.0, SatWarm,  1.0, 0.30, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.5,  0.55, 0.6, true,  0.05, 0.4, WobSine },
    { "Diffused",           60,  12000, 0.0, SatClean, 1.0, 0.00, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -0.5,  0.70, 0.5, false, 0.00, 0.5, WobSine },
    { "Splattered",        100,  10000, 0.0, SatTape,  1.0, 0.20, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -1.0,  0.90, 0.9, true,  0.00, 0.5, WobSine },
    { "Verbed",             80,  10000, 0.0, SatClean, 1.0, 0.00, SatClean,     0.0,  0.0, 0.0, 0.0,  0.0,  0.0, -0.8,  0.85, 0.8, false, 0.00, 0.5, WobSine },
    { "Ensemble Chorus",    80,  11000, 0.0, SatWarm,  0.8, 0.05, SatClean,     0.0,  0.0, 0.0, 0.5,  0.0,  0.0, -0.8,  0.0, 0.5, false, 0.55, 0.8, WobSine },
    { "Multiband Squash",   40,  15000, 0.0, SatMultiband, 1.0, 0.45, SatClean, 0.0,  0.0, 0.0, 0.5,  0.0,  0.0, -0.5,  0.0, 0.5, false, 0.03, 0.5, WobSine },
};
constexpr int kNumStyles = (int) (sizeof (kStyles) / sizeof (kStyles[0]));
}
