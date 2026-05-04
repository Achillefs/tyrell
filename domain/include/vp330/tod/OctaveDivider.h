#pragma once

#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vp330 {

class OctaveDivider {
public:
  static constexpr int kMaxOctavesDown = 6; // 4-octave keyboard with margin

  OctaveDivider(Hertz input_frequency, int sample_rate);

  // Append `frames` samples of (input / 2^octave_down) square output to `out`.
  // Output is ±1.0 50%-duty. State persists across calls.
  void render(int octave_down, float* out, std::size_t frames);

private:
  Hertz input_frequency_;
  int sample_rate_;
  // Per-octave sample counter; phase is derived via fmod to avoid accumulation error.
  std::array<uint64_t, kMaxOctavesDown + 1> sample_count_{};
};

} // namespace vp330
