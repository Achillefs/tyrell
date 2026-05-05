#include "vp330/tod/OctaveDivider.h"

#include <cassert>
#include <cmath>

namespace vp330 {

OctaveDivider::OctaveDivider(Hertz input_frequency, int sample_rate)
    : input_frequency_{input_frequency}, sample_rate_{sample_rate} {}

void OctaveDivider::set_input_frequency(Hertz freq) {
  const double inv_sr = 1.0 / static_cast<double>(sample_rate_);
  for (int od = 0; od <= kMaxOctavesDown; ++od) {
    const double scale = 1.0 / static_cast<double>(1 << od);
    const double old_f = input_frequency_.value() * scale;
    const double new_f = freq.value() * scale;
    const double n     = static_cast<double>(sample_count_[od]);
    phase_offsets_[od] = std::fmod(phase_offsets_[od] + n * (old_f - new_f) * inv_sr, 1.0);
    if (phase_offsets_[od] < 0.0) phase_offsets_[od] += 1.0;
  }
  input_frequency_ = freq;
}

void OctaveDivider::render(int octave_down, float* out, std::size_t frames) {
  assert(octave_down >= 0 && octave_down <= kMaxOctavesDown);
  const double freq = input_frequency_.value() / static_cast<double>(1 << octave_down);
  const double inv_sr = 1.0 / static_cast<double>(sample_rate_);
  uint64_t& n = sample_count_[octave_down];
  for (std::size_t i = 0; i < frames; ++i) {
    ++n;
    // fmod on the integer sample count rather than += inc — eliminates per-period
    // FP drift that otherwise misses one zero-crossing per second at audio rate.
    const double p_raw = std::fmod(static_cast<double>(n) * freq * inv_sr + phase_offsets_[octave_down], 1.0);
    const double p     = p_raw < 0.0 ? p_raw + 1.0 : p_raw;
    out[i] = (p < 0.5 ? 1.0f : -1.0f);
  }
}

} // namespace vp330
