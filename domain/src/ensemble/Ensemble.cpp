#include "vp330/ensemble/Ensemble.h"

#include "vp330/ensemble/EnsembleConstants.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

namespace vp330 {

Ensemble::Ensemble(int sample_rate)
    : sample_rate_{sample_rate}, lines_{BbdLine{sample_rate, 256, mkii::kMaxDelayShort},
                                        BbdLine{sample_rate, 256, mkii::kMaxDelayShort},
                                        BbdLine{sample_rate, 512, mkii::kMaxDelayLong},
                                        BbdLine{sample_rate, 512, mkii::kMaxDelayLong}} {
}

void Ensemble::set_rate(Hertz rate) {
  assert(rate.value() > 0.0);
  rate_hz_ = static_cast<float>(rate.value());
}

void Ensemble::set_depth(float depth_0_to_1) {
  assert(depth_0_to_1 >= 0.f && depth_0_to_1 <= 1.f);
  depth_short_ = depth_0_to_1 * mkii::kDepthShort;
  depth_long_ = depth_0_to_1 * mkii::kDepthLong;
}

void Ensemble::set_enabled(bool on) {
  enabled_ = on;
}

void Ensemble::process(const float* in, float* left, float* right, std::size_t frames) {
  if (!enabled_) {
    std::copy(in, in + frames, left);
    std::copy(in, in + frames, right);
    return;
  }

  for (auto& s : line_scratch_)
    if (s.size() < frames) s.resize(frames);

  // Set delays for this block from current LFO phase.
  // Quadrature phases: line0=0°, line1=90°, line2=180°, line3=270°.
  const float sn = std::sin(phase_);
  const float cs = std::cos(phase_); // sin(phase + 90°)

  lines_[0].set_delay(mkii::kCentreDelayShort + depth_short_ * sn);
  lines_[1].set_delay(mkii::kCentreDelayShort + depth_short_ * cs);
  lines_[2].set_delay(mkii::kCentreDelayLong - depth_long_ * sn); // sin(phase + 180°)
  lines_[3].set_delay(mkii::kCentreDelayLong - depth_long_ * cs); // sin(phase + 270°)

  for (int i = 0; i < 4; ++i)
    lines_[i].process(in, line_scratch_[i].data(), frames);

  // Mix: Left  = dry×0.5 + line0×0.25 + line2×0.25
  //      Right = dry×0.5 + line1×0.25 + line3×0.25
  const float* l0 = line_scratch_[0].data();
  const float* l1 = line_scratch_[1].data();
  const float* l2 = line_scratch_[2].data();
  const float* l3 = line_scratch_[3].data();
  for (std::size_t i = 0; i < frames; ++i) {
    left[i] = mkii::kDryGain * in[i] + mkii::kWetGain * l0[i] + mkii::kWetGain * l2[i];
    right[i] = mkii::kDryGain * in[i] + mkii::kWetGain * l1[i] + mkii::kWetGain * l3[i];
  }

  // Advance phase by one block (per-block LFO update as spec'd).
  const float block_seconds = static_cast<float>(frames) / static_cast<float>(sample_rate_);
  phase_ += 2.f * std::numbers::pi_v<float> * rate_hz_ * block_seconds;
  if (phase_ >= 2.f * std::numbers::pi_v<float>) phase_ -= 2.f * std::numbers::pi_v<float>;
}

} // namespace vp330
