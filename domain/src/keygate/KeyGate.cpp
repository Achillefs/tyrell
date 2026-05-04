#include "vp330/keygate/KeyGate.h"

namespace vp330 {

KeyGate::KeyGate(int sample_rate, double attack_seconds, double release_seconds)
    : attack_step_{1.0 / (attack_seconds * sample_rate)},
      release_step_{1.0 / (release_seconds * sample_rate)} {}

void KeyGate::gate_on() {
  state_ = State::Attacking;
}

void KeyGate::gate_off() {
  if (state_ != State::Idle) state_ = State::Releasing;
}

void KeyGate::advance_one_sample() {
  switch (state_) {
  case State::Idle:
    envelope_ = 0.0;
    break;
  case State::Attacking:
    envelope_ += attack_step_;
    if (envelope_ >= 1.0) {
      envelope_ = 1.0;
      state_ = State::Sustain;
    }
    break;
  case State::Sustain:
    envelope_ = 1.0;
    break;
  case State::Releasing:
    envelope_ -= release_step_;
    if (envelope_ <= 0.0) {
      envelope_ = 0.0;
      state_ = State::Idle;
    }
    break;
  }
}

void KeyGate::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = in[i] * static_cast<float>(envelope_);
    advance_one_sample();
  }
}

} // namespace vp330
