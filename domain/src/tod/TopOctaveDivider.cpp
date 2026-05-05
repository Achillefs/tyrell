#include "vp330/tod/TopOctaveDivider.h"

#include <cassert>
#include <cmath>

namespace vp330 {

TopOctaveDivider::TopOctaveDivider(Hertz master_clock,
                                   std::array<int, 12> divider_ratios,
                                   int sample_rate)
    : master_clock_{master_clock},
      divider_ratios_{divider_ratios},
      sample_rate_{sample_rate} {}

void TopOctaveDivider::set_master_clock_hz(Hertz hz) {
  const double inv_sr = 1.0 / static_cast<double>(sample_rate_);
  for (int pc = 0; pc < 12; ++pc) {
    const double old_f = pitch_class_frequency(pc).value();
    const double new_f = hz.value() / static_cast<double>(divider_ratios_[pc]);
    const double n     = static_cast<double>(sample_count_[pc]);
    phase_offsets_[pc] = std::fmod(phase_offsets_[pc] + n * (old_f - new_f) * inv_sr, 1.0);
    if (phase_offsets_[pc] < 0.0) phase_offsets_[pc] += 1.0;
  }
  master_clock_ = hz;
}

Hertz TopOctaveDivider::pitch_class_frequency(int pitch_class) const {
  assert(pitch_class >= 0 && pitch_class < 12);
  const double ratio = static_cast<double>(divider_ratios_[pitch_class]);
  // Real TOS chips (e.g. AY-3-0214) use master/N divide-by-N semantics with
  // possibly odd N — not the master/(2N) flip-flop convention. With odd N the
  // chip's hardware square is slightly asymmetric (e.g. 225/451 vs 226/451);
  // our fmod model idealises to 50% duty, which is fine at audio.
  return Hertz{master_clock_.value() / ratio};
}

void TopOctaveDivider::render_pitch_class(int pitch_class, float* out, std::size_t frames) {
  assert(pitch_class >= 0 && pitch_class < 12);
  const double freq = pitch_class_frequency(pitch_class).value();
  const double inv_sr = 1.0 / static_cast<double>(sample_rate_);
  uint64_t& n = sample_count_[pitch_class];
  for (std::size_t i = 0; i < frames; ++i) {
    ++n;
    // fmod on the integer sample count rather than += inc — eliminates per-period
    // FP drift that otherwise misses one zero-crossing per second at audio rate.
    const double p_raw = std::fmod(static_cast<double>(n) * freq * inv_sr + phase_offsets_[pitch_class], 1.0);
    const double p     = p_raw < 0.0 ? p_raw + 1.0 : p_raw;
    out[i] = (p < 0.5 ? 1.0f : -1.0f);
  }
}

} // namespace vp330
