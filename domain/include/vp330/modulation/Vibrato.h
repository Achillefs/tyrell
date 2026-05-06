#pragma once

#include "vp330/modulation/Lfo.h"
#include "vp330/values/Hertz.h"

#include <cstddef>

namespace vp330 {

class Vibrato {
public:
  explicit Vibrato(int sample_rate);

  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1);

  // Advance LFO by `frames` samples and return the modulated master clock for the block.
  [[nodiscard]] Hertz tick(std::size_t frames = 1);

private:
  Lfo lfo_;
};

} // namespace vp330
