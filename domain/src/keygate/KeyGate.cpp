#include "vp330/keygate/KeyGate.h"

#include <cmath>

namespace vp330 {

namespace {

int samples_for(double seconds, int sample_rate) {
  return static_cast<int>(std::lround(seconds * sample_rate));
}

} // namespace

KeyGate::KeyGate(int sample_rate, double attack_seconds, double release_seconds)
    : attack_samples_{samples_for(attack_seconds, sample_rate)},
      release_samples_{samples_for(release_seconds, sample_rate)} {}

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

void KeyGate::process(const float* in, float* out, std::size_t frames) {
  for (std::size_t i = 0; i < frames; ++i) {
    float sample = in[i] * static_cast<float>(envelope_);
    if (state_ == State::Attacking && attack_samples_ > 0) {
      // Slew-rate limit during attack: the per-sample envelope step is
      // 1/attack_samples, so two consecutive worst-case (opposite-sign) output
      // samples cannot differ by more than 2/attack_samples.  This keeps the
      // note-on transient below the click-audibility threshold (~0.05) for any
      // attack longer than ~40 samples (~0.8 ms at 48 kHz).
      const float max_slew = 2.0f / static_cast<float>(attack_samples_);
      const float delta = sample - prev_out_;
      if (delta > max_slew) sample = prev_out_ + max_slew;
      else if (delta < -max_slew) sample = prev_out_ - max_slew;
    }
    out[i] = sample;
    prev_out_ = sample;
    advance_one_sample();
  }
}

} // namespace vp330
