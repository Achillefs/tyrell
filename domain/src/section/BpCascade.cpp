#include "vp330/section/BpCascade.h"

#include <cmath>
#include <numbers>

namespace vp330 {

BpCascade::Stage BpCascade::make_stage(float f0, float q, int sample_rate) {
  const float w0    = 2.f * std::numbers::pi_v<float> * f0 / static_cast<float>(sample_rate);
  const float sinw0 = std::sin(w0);
  const float cosw0 = std::cos(w0);
  const float alpha = sinw0 / (2.f * q);
  const float a0    = 1.f + alpha;
  // Constant 0 dB peak-gain BP: b0 = sin(w0)/2, b1=0, b2=-sin(w0)/2, normalised by a0.
  return Stage{
      .b0 =  sinw0 / (2.f * a0),
      .b2 = -sinw0 / (2.f * a0),
      .a1 = -2.f * cosw0 / a0,
      .a2 = (1.f - alpha) / a0,
  };
}

BpCascade::BpCascade(float f0_1, float q1, float f0_2, float q2, int sample_rate)
    : stage1_{make_stage(f0_1, q1, sample_rate)},
      stage2_{make_stage(f0_2, q2, sample_rate)} {}

float BpCascade::Stage::tick(float x) noexcept {
  const float y = (b0 * x) + (b2 * x2) - (a1 * y1) - (a2 * y2);
  x2 = x1;
  x1 = x;
  y2 = y1;
  y1 = y;
  return y;
}

void BpCascade::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = stage2_.tick(stage1_.tick(in[i]));
  }
}

void BpCascade::reset() {
  stage1_.reset();
  stage2_.reset();
}

} // namespace vp330
