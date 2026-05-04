#pragma once

#include <cstddef>

namespace vp330 {

class KeyGate {
public:
  enum class State { Idle, Attacking, Sustain, Releasing };

  KeyGate(int sample_rate, double attack_seconds, double release_seconds);

  void gate_on();
  void gate_off();

  State state() const { return state_; }

  // Multiply each input sample by the current envelope value, advancing
  // state. `in` and `out` may alias.
  void process(const float* in, float* out, std::size_t frames);

private:
  void advance_one_sample();

  State state_ = State::Idle;
  double envelope_ = 0.0;
  double attack_step_;
  double release_step_;
};

} // namespace vp330
