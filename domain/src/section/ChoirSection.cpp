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

void ChoirSection::process(const float* lower, const float* upper,
                           float* left, float* right, std::size_t frames) {
  std::fill_n(left,  frames, 0.f);
  std::fill_n(right, frames, 0.f);

  lower_bank_.process(lower, frames);
  upper_bank_.process(upper, frames);

  // Switches 0,1 draw from the lower bank; switches 2,3 from the upper bank.
  const std::array<const ChoirFilterBank*, 4> banks = {
    &lower_bank_, &lower_bank_, &upper_bank_, &upper_bank_
  };

  for (int sw = 0; sw < 4; ++sw) {
    if (!switches_[static_cast<std::size_t>(sw)]) {
      continue;
    }
    const auto& v    = mkii::kVoiceWeights[sw];
    const auto* bank = banks[sw];
    for (std::size_t f = 0; f < frames; ++f) {
      float s = 0.f;
      for (int i = 0; i < 4; ++i) {
        s += v.weights[i] * bank->output(v.filters[i])[f];
      }
      left[f]  += s;
      right[f] += s;
    }
  }
}

} // namespace vp330
