#include "vp330/section/ChoirCompander.h"

#include <cassert>
#include <cmath>

namespace {
double release_alpha_for(int sample_rate) {
  assert(sample_rate > 0);
  return std::exp(-1.0 / (0.103 * static_cast<double>(sample_rate)));
}
} // namespace

namespace vp330 {

ChoirCompander::ChoirCompander(int sample_rate) : alpha_release_{release_alpha_for(sample_rate)} {
}

void ChoirCompander::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    const double x = static_cast<double>(in[i]);
    const double r = std::fabs(x);
    envelope_ = (r >= envelope_) ? r : alpha_release_ * envelope_;
    if (envelope_ < 1e-15) envelope_ = 0.0;
    const double g = kGMax * kEnvTarget / (kEnvTarget + envelope_);
    out[i] = static_cast<float>(g * x);
  }
}

} // namespace vp330
