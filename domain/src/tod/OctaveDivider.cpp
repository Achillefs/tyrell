#include "vp330/tod/OctaveDivider.h"

#include <cassert>
#include <cmath>

namespace vp330 {

OctaveDivider::OctaveDivider(Hertz input_frequency, int sample_rate)
    : input_frequency_{input_frequency}, sample_rate_{sample_rate} {}

void OctaveDivider::render(int octave_down, float* out, std::size_t frames) {
  assert(octave_down >= 0 && octave_down <= kMaxOctavesDown);
  const double freq = input_frequency_.value() / static_cast<double>(1 << octave_down);
  const double inv_sr = 1.0 / static_cast<double>(sample_rate_);
  uint64_t& n = sample_count_[octave_down];
  for (std::size_t i = 0; i < frames; ++i) {
    ++n;
    // fmod on the integer sample count rather than += inc — eliminates per-period
    // FP drift that otherwise misses one zero-crossing per second at audio rate.
    const double p = std::fmod(static_cast<double>(n) * freq * inv_sr, 1.0);
    out[i] = (p < 0.5 ? 1.0f : -1.0f);
  }
}

} // namespace vp330
