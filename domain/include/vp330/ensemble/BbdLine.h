#pragma once

#include <cstddef>
#include <vector>

namespace vp330 {

class BbdLine {
public:
  // n_stages: 256 (MN3009-style) or 512 (MN3004-style)
  // max_delay_seconds: sets circular-buffer capacity
  BbdLine(int sample_rate, int n_stages, float max_delay_seconds);

  // Called per-block by Ensemble before process().
  void set_delay(float seconds);

  void process(const float* in, float* out, std::size_t frames);

private:
  int sample_rate_;
  int n_stages_;
  float max_delay_seconds_;
  float delay_seconds_;

  std::vector<float> buffer_;
  int write_pos_ = 0;

  // Tracking 1-pole IIR: fc = 0.3 × (n_stages / delay_seconds).
  // Coefficient recomputed in set_delay(). 0 when fc > Nyquist (bypass).
  float lp_coeff_ = 0.f;
  float lp_pre_z1_ = 0.f;
  float lp_post_z1_ = 0.f;
};

} // namespace vp330
