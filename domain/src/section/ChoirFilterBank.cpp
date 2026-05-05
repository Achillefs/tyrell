#include "vp330/section/ChoirFilterBank.h"

#include "vp330/section/ChoirConstants.h"

#include <algorithm>

namespace vp330 {

namespace {

std::array<BpCascade, 7> make_filters(int sr) {
  const auto& p = mkii::kChoirFilters;
  return {
      BpCascade{p[0].f0_1, p[0].q1, p[0].f0_2, p[0].q2, sr, p[0].gain},
      BpCascade{p[1].f0_1, p[1].q1, p[1].f0_2, p[1].q2, sr, p[1].gain},
      BpCascade{p[2].f0_1, p[2].q1, p[2].f0_2, p[2].q2, sr, p[2].gain},
      BpCascade{p[3].f0_1, p[3].q1, p[3].f0_2, p[3].q2, sr, p[3].gain},
      BpCascade{p[4].f0_1, p[4].q1, p[4].f0_2, p[4].q2, sr, p[4].gain},
      BpCascade{p[5].f0_1, p[5].q1, p[5].f0_2, p[5].q2, sr, p[5].gain},
      BpCascade{p[6].f0_1, p[6].q1, p[6].f0_2, p[6].q2, sr, p[6].gain},
  };
}

} // namespace

ChoirFilterBank::ChoirFilterBank(int sample_rate) : filters_{make_filters(sample_rate)} {
}

void ChoirFilterBank::process(const float* in, std::size_t frames) {
  for (int i = 0; i < 7; ++i) {
    if (scratch_[i].size() < frames) {
      scratch_[i].resize(frames);
    }
    filters_[i].process(in, scratch_[i].data(), frames);
  }
}

const float* ChoirFilterBank::output(int filter_idx) const {
  return scratch_[filter_idx].data();
}

void ChoirFilterBank::reset() {
  for (int i = 0; i < 7; ++i) {
    filters_[i].reset();
    std::fill(scratch_[i].begin(), scratch_[i].end(), 0.f);
  }
}

} // namespace vp330
