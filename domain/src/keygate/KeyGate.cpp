#include "vp330/keygate/KeyGate.h"

#include <cmath>

namespace vp330 {

namespace {

int samples_for(double seconds, int sample_rate) {
  return static_cast<int>(std::lround(seconds * sample_rate));
}

} // namespace

KeyGate::KeyGate(int sample_rate, double attack_seconds, double release_seconds)
    : sample_rate_{sample_rate},
      attack_samples_{samples_for(attack_seconds, sample_rate)},
      release_samples_{samples_for(release_seconds, sample_rate)} {}

void KeyGate::set_attack_seconds(double seconds) {
  attack_samples_ = samples_for(seconds, sample_rate_);
}

void KeyGate::set_release_seconds(double seconds) {
  release_samples_ = samples_for(seconds, sample_rate_);
}

void KeyGate::gate_on() {
  if (state_ == State::Idle || state_ == State::Releasing) {
    // Resume attack from the current envelope so retrigger does not click.
    phase_index_ = static_cast<int>(envelope_ * attack_samples_);
  }
  // If already Attacking or Sustain, leave phase_index_ alone (idempotent).
  state_ = State::Attacking;
}

void KeyGate::gate_off() {
  if (state_ == State::Attacking || state_ == State::Sustain) {
    phase_index_ = static_cast<int>((1.0 - envelope_) * release_samples_);
  }
  if (state_ != State::Idle) state_ = State::Releasing;
}

void KeyGate::advance_one_sample() {
  switch (state_) {
  case State::Idle:
    envelope_ = 0.0;
    phase_index_ = 0;
    break;
  case State::Attacking:
    ++phase_index_;
    if (phase_index_ >= attack_samples_) {
      envelope_ = 1.0;
      state_ = State::Sustain;
    } else {
      envelope_ = static_cast<double>(phase_index_) / attack_samples_;
    }
    break;
  case State::Sustain:
    envelope_ = 1.0;
    break;
  case State::Releasing:
    ++phase_index_;
    if (phase_index_ >= release_samples_) {
      envelope_ = 0.0;
      state_ = State::Idle;
    } else {
      envelope_ = 1.0 - static_cast<double>(phase_index_) / release_samples_;
    }
    break;
  }
}

void KeyGate::fill_envelope(float* gains, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    gains[i] = static_cast<float>(envelope_);
    advance_one_sample();
  }
}

void KeyGate::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    out[i] = in[i] * static_cast<float>(envelope_);
    advance_one_sample();
  }
}

} // namespace vp330
