#include "vp330/ensemble/BbdLine.h"

#include <cassert>
#include <cmath>
#include <numbers>

namespace vp330 {

BbdLine::BbdLine(int sample_rate, int n_stages, float max_delay_seconds)
    : sample_rate_{sample_rate}, n_stages_{n_stages}, max_delay_seconds_{max_delay_seconds},
      delay_seconds_{max_delay_seconds * 0.5f} {
  assert(sample_rate > 0);
  assert(n_stages > 0);
  assert(max_delay_seconds > 0.f);
  const int buf_size = static_cast<int>(max_delay_seconds * static_cast<float>(sample_rate)) + 4;
  buffer_.assign(buf_size, 0.f);
  set_delay(delay_seconds_);
}

void BbdLine::set_delay(float seconds) {
  assert(seconds > 0.f && seconds <= max_delay_seconds_);
  delay_seconds_ = seconds;
  // S&H-accurate: clock frequency determines bandwidth.
  const float clock_hz = static_cast<float>(n_stages_) / seconds;
  const float fc = 0.3f * clock_hz;
  const float w = 2.f * std::numbers::pi_v<float> * fc / static_cast<float>(sample_rate_);
  lp_coeff_ = (w >= std::numbers::pi_v<float>) ? 0.f : std::exp(-w);
}

void BbdLine::process(const float* in, float* out, std::size_t frames) {
  const int buf_size = static_cast<int>(buffer_.size());
  const float delay_samples = delay_seconds_ * static_cast<float>(sample_rate_);
  const int delay_int = static_cast<int>(delay_samples);
  const float frac = delay_samples - static_cast<float>(delay_int);
  const float c = lp_coeff_;
  const float a = 1.f - c;

  for (std::size_t i = 0; i < frames; ++i) {
    lp_pre_z1_ = a * in[i] + c * lp_pre_z1_;
    buffer_[write_pos_] = lp_pre_z1_;

    const int r0 = (write_pos_ - delay_int + buf_size) % buf_size;
    const int r1 = (r0 - 1 + buf_size) % buf_size;
    const float delayed = buffer_[r0] + frac * (buffer_[r1] - buffer_[r0]);

    lp_post_z1_ = a * delayed + c * lp_post_z1_;
    out[i] = lp_post_z1_;

    write_pos_ = (write_pos_ + 1) % buf_size;
  }
}

} // namespace vp330
