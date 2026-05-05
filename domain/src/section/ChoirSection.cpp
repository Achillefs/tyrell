#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirConstants.h"

#include <algorithm>

namespace vp330 {

ChoirSection::ChoirSection(int sample_rate)
    : lower_bank_{sample_rate}, upper_bank_{sample_rate} {}

void ChoirSection::set_switch(ChoirSwitch sw, bool on) {
  switches_[static_cast<std::size_t>(sw)] = on;
}

bool ChoirSection::get_switch(ChoirSwitch sw) const {
  return switches_[static_cast<std::size_t>(sw)];
}

// Mix active lower-zone sources into the lower filter bank, then accumulate
// weighted outputs for each active lower switch into left/right.
static void process_zone(ChoirFilterBank& bank, const float* in_8, const float* in_4,
                         bool sw_8, bool sw_4, int sw_8_idx, int sw_4_idx,
                         float* left, float* right, std::size_t frames,
                         std::vector<float>& scratch) {
  if (!sw_8 && !sw_4) return;

  const float* in = nullptr;
  if (sw_8 && sw_4) {
    if (scratch.size() < frames) scratch.resize(frames);
    for (std::size_t f = 0; f < frames; ++f)
      scratch[f] = in_8[f] + in_4[f];
    in = scratch.data();
  } else if (sw_8) {
    in = in_8;
  } else {
    in = in_4;
  }
  bank.process(in, frames);

  const std::array<int, 2> active_sws = {sw_8_idx, sw_4_idx};
  const std::array<bool, 2> active    = {sw_8, sw_4};
  for (int k = 0; k < 2; ++k) {
    if (!active[k]) continue;
    const auto& v = mkii::kVoiceWeights[active_sws[k]];
    for (std::size_t f = 0; f < frames; ++f) {
      float s = 0.f;
      for (int i = 0; i < 4; ++i)
        s += v.weights[i] * bank.output(v.filters[i])[f];
      left[f]  += s;
      right[f] += s;
    }
  }
}

void ChoirSection::process(const float* lower_8, const float* lower_4,
                           const float* upper_8, const float* upper_4,
                           float* left, float* right, std::size_t frames) {
  std::fill_n(left,  frames, 0.f);
  std::fill_n(right, frames, 0.f);

  // Lower zone: LowerMale8 (sw=0) uses 8′, LowerMale4 (sw=1) uses 4′.
  process_zone(lower_bank_, lower_8, lower_4,
               switches_[0], switches_[1], 0, 1,
               left, right, frames, mix_scratch_);

  // Upper zone: UpperMale8 (sw=2) uses 8′, UpperFemale4 (sw=3) uses 4′.
  process_zone(upper_bank_, upper_8, upper_4,
               switches_[2], switches_[3], 2, 3,
               left, right, frames, mix_scratch_);
}

} // namespace vp330
