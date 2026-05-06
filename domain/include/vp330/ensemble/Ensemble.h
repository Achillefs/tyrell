#pragma once

#include "vp330/ensemble/BbdLine.h"
#include "vp330/ensemble/EnsembleConstants.h"
#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>
#include <vector>

namespace vp330 {

class Ensemble {
public:
  explicit Ensemble(int sample_rate);

  void set_rate(Hertz rate);
  void set_depth(float depth_0_to_1); // scales default mkii::kDepthShort/Long
  void set_enabled(bool on);

  // mono in → stereo out (overwrites left and right)
  void process(const float* in, float* left, float* right, std::size_t frames);

  // --- test instrumentation (not part of production API) ---
  float test_phase() const { return phase_; }
  // ---

private:
  int sample_rate_;
  std::array<BbdLine, 4> lines_; // [0,1] = MN3009-style; [2,3] = MN3004-style
  float phase_ = 0.f;
  float rate_hz_ = mkii::kLfoRateHz;
  float depth_short_ = mkii::kDepthShort;
  float depth_long_ = mkii::kDepthLong;
  bool enabled_ = true;
  std::array<std::vector<float>, 4> line_scratch_;
};

} // namespace vp330
