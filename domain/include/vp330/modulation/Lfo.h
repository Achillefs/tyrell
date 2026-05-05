#pragma once

#include "vp330/values/Hertz.h"

namespace vp330 {

class Lfo {
public:
  explicit Lfo(int sample_rate);

  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1);

  float tick();                      // advance one sample, return scaled output
  void advance(int n);               // advance n samples without returning output
  [[nodiscard]] float value() const; // current output without advancing

private:
  double phase_ = 0.0;
  double phase_inc_ = 0.0;
  float depth_ = 0.0f;
  int sample_rate_;
};

} // namespace vp330
