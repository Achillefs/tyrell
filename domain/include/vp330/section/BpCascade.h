#pragma once

#include <cstddef>

namespace vp330 {

// Pair of cascaded 2nd-order bandpass biquad filters (4th-order total).
// Uses Audio EQ Cookbook constant-0-dB-peak-gain BP form.
class BpCascade {
public:
  BpCascade(float f0_1, float q1, float f0_2, float q2, int sample_rate);

  void process(const float* in, float* out, std::size_t frames);
  void reset();

private:
  struct Stage {
    float b0, b2, a1, a2;             // b1 = 0 for BP
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    float tick(float x) noexcept;
    void reset() noexcept { x1 = x2 = y1 = y2 = 0.f; }
  };

  static Stage make_stage(float f0, float q, int sample_rate);

  Stage stage1_, stage2_;
};

} // namespace vp330
