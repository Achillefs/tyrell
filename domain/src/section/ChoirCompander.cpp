#include "vp330/section/ChoirCompander.h"

#include <cassert>
#include <cmath>

namespace vp330 {

ChoirCompander::ChoirCompander(int sample_rate)
    : alpha_release_{std::exp(-1.0 / (0.103 * static_cast<double>(sample_rate)))} {
  assert(sample_rate > 0);
}

void ChoirCompander::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    const double x = static_cast<double>(in[i]);
    const double r = std::fabs(x);
    envelope_ = (r >= envelope_) ? r : alpha_release_ * envelope_;
    const double g = kGMax * envelope_ / (kEnvTarget + envelope_);
    out[i] = static_cast<float>(g * x);
  }
}

} // namespace vp330
