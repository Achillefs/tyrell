#pragma once

#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vp330 {

class TopOctaveDivider {
public:
  TopOctaveDivider(Hertz master_clock, std::array<int, 12> divider_ratios, int sample_rate);

  void set_master_clock_hz(Hertz hz);

  Hertz pitch_class_frequency(int pitch_class) const;

  // Append `frames` samples of the requested pitch class's square output to
  // `out`. Output is ±1.0 50%-duty. State persists across calls.
  void render_pitch_class(int pitch_class, float* out, std::size_t frames);

private:
  Hertz master_clock_;
  std::array<int, 12> divider_ratios_;
  int sample_rate_;
  // Per-pitch-class sample counter; phase is derived via fmod to avoid
  // accumulation error.
  std::array<uint64_t, 12> sample_count_{};
};

} // namespace vp330
