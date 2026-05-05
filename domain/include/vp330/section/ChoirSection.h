#pragma once

#include "vp330/section/ChoirFilterBank.h"
#include "vp330/section/ChoirSwitch.h"

#include <array>
#include <cstddef>
#include <vector>

namespace vp330 {

class ChoirSection {
public:
  explicit ChoirSection(int sample_rate);

  void set_switch(ChoirSwitch sw, bool on);
  [[nodiscard]] bool get_switch(ChoirSwitch sw) const;

  // Process four keyboard zone buffers (lower/upper × 8′/4′) into stereo out.
  // 8′ = natural pitch; 4′ = one octave up. Active switches determine which
  // pitch(es) are mixed into each filter bank's input.
  void process(const float* lower_8, const float* lower_4,
               const float* upper_8, const float* upper_4,
               float* left, float* right, std::size_t frames);

private:
  ChoirFilterBank lower_bank_, upper_bank_;
  std::array<bool, 4> switches_{}; // all off by default
  std::vector<float> mix_scratch_;  // scratch for pre-summing 8′+4′ when both active
};

} // namespace vp330
