#pragma once

#include <array>

// PD-derived VP-330 MkII Choir filter constants.
// Source: community circuit analysis; calibrate against author's MkII at L4.
// To recalibrate: update kChoirFilters / kVoiceWeights here only.

namespace vp330::mkii {

struct BpCascadeParams {
  float f0_1, q1; // first 2-pole stage
  float f0_2, q2; // second 2-pole stage
  // Post-cascade scalar. Target is 8.72× (+18.8 dB) to match the hardware's
  // ~+10 dB net Deliyannis gain at Fc, but without the HV-330 compander
  // (Phase 4) the transients clip at that level. 2.7× is the maximum that
  // keeps a single-note output below 0 dBFS with kPerKeyGain=0.05; it places
  // the formant peak at ~0 dB (passes formant harmonics at unity, attenuates
  // everything else). Raise to 8.72 once the compander is implemented.
  float gain = 8.72f; // raised from 2.7×; ChoirCompander now prevents clipping
};

// clang-format off
inline constexpr std::array<BpCascadeParams, 7> kChoirFilters{{
    { 175.f, 6.79f,  216.f, 6.83f}, // F0
    { 209.f, 6.79f,  257.f, 6.83f}, // F1
    { 559.f, 6.96f,  671.f, 6.83f}, // F2 — present in all 4 voices
    { 845.f, 7.00f, 1007.f, 6.83f}, // F3 — present in all 4 voices
    {1202.f, 6.81f, 1473.f, 6.83f}, // F4
    {2532.f, 6.83f, 3098.f, 6.83f}, // F5
    {2974.f, 6.78f, 3660.f, 6.83f}, // F6
}};
// clang-format on

struct VoiceWeights {
  std::array<int, 4> filters;
  std::array<float, 4> weights;
};

// Order MUST match ChoirSwitch enum: LowerMale8=0, LowerMale4=1, UpperMale8=2, UpperFemale4=3.
// clang-format off
inline constexpr std::array<VoiceWeights, 4> kVoiceWeights{{
    {{0, 2, 3, 5}, {0.3125f, 1.f, 1.f,    0.3125f}}, // LowerMale8
    {{1, 2, 3, 5}, {0.125f,  1.f, 1.f,    0.3125f}}, // LowerMale4
    {{1, 2, 3, 5}, {0.1875f, 1.f, 1.f,    0.3125f}}, // UpperMale8
    {{2, 3, 4, 6}, {0.1875f, 1.f, 0.6875f, 0.1875f}}, // UpperFemale4
}};
// clang-format on

inline constexpr double kVibratoMaxClockOffsetHz = 50.0; // calibrate vs capture #4

} // namespace vp330::mkii
