#pragma once

#include <cstddef>

namespace vp330 {

class KeyGate {
public:
  enum class State { Idle, Attacking, Sustain, Releasing };

  KeyGate(int sample_rate, double attack_seconds, double release_seconds);

  void gate_on();
  void gate_off();

  void set_attack_seconds(double seconds);
  void set_release_seconds(double seconds);

  State state() const { return state_; }

  // Multiply each input sample by the current envelope value, advancing
  // state. `in` and `out` may alias.
  void process(const float* in, float* out, std::size_t frames);

  // Write per-sample envelope gains to `gains` while advancing state.
  // Use when you need to apply the same envelope to multiple signals.
  void fill_envelope(float* gains, std::size_t frames);

private:
  void advance_one_sample();

  int sample_rate_;          // stored for set_*_seconds()
  int attack_samples_;       // total samples in Attacking phase
  int release_samples_;      // total samples in Releasing phase
  State state_ = State::Idle;
  double envelope_ = 0.0;    // current 0..1 envelope amplitude
  int phase_index_ = 0;      // sample index within current Attacking/Releasing run
};

} // namespace vp330
