#include "vp330/modulation/Lfo.h"

#include <cmath>
#include <numbers>

namespace vp330 {

Lfo::Lfo(int sample_rate) : sample_rate_{sample_rate} {}

void Lfo::set_rate(Hertz rate) {
  phase_inc_ = 2.0 * std::numbers::pi * rate.value() / static_cast<double>(sample_rate_);
}

void Lfo::set_depth(float depth) { depth_ = depth; }

float Lfo::value() const {
  return depth_ * static_cast<float>(std::sin(phase_));
}

float Lfo::tick() {
  const float v = value();
  phase_ += phase_inc_;
  if (phase_ >= 2.0 * std::numbers::pi) {
    phase_ -= 2.0 * std::numbers::pi;
  }
  return v;
}

void Lfo::advance(int n) {
  phase_ = std::fmod(phase_ + (phase_inc_ * static_cast<double>(n)), 2.0 * std::numbers::pi);
}

} // namespace vp330
