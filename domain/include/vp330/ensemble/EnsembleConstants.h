#pragma once

// VP-330 MkII ETH16 BBD ensemble constants.
// Source: service manual page 14 (IC16–IC19 chip parameters), page 9 (LFO range).
// Mix weights are L4 calibration targets; adjust here without code changes.

namespace vp330::mkii {

// MN3009-style: 256 stages, delay range 0.64–12.8ms
inline constexpr float kMaxDelayShort = 12.8e-3f;
inline constexpr float kCentreDelayShort = 6.0e-3f;
inline constexpr float kDepthShort = 1.5e-3f; // ±1.5ms default modulation depth

// MN3004-style: 512 stages, delay range 2.56–25.6ms
inline constexpr float kMaxDelayLong = 25.6e-3f;
inline constexpr float kCentreDelayLong = 20.0e-3f;
inline constexpr float kDepthLong = 3.0e-3f; // ±3ms default modulation depth

// LFO: 0.45–1.4 Hz range (service manual page 9), default 0.7 Hz
inline constexpr float kLfoRateHz = 0.7f;

// Stereo mix: Left = dry×kDryGain + line0×kWetGain + line2×kWetGain
//             Right = dry×kDryGain + line1×kWetGain + line3×kWetGain
inline constexpr float kDryGain = 0.5f;
inline constexpr float kWetGain = 0.25f;

} // namespace vp330::mkii
