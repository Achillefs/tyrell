#include "vp330/keygate/KeyGate.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using vp330::KeyGate;

namespace {

constexpr int kSampleRate = 48000;

std::vector<float> dc_signal(std::size_t n, float value = 1.0f) {
  return std::vector<float>(n, value);
}

} // namespace

TEST_CASE("KeyGate L2: attack ramp is monotonic non-decreasing", "[keygate][L2]") {
  KeyGate gate{kSampleRate, /*attack=*/0.005, /*release=*/0.05}; // 5 ms attack
  gate.gate_on();
  auto in = dc_signal(kSampleRate / 100); // 10 ms — covers full attack
  std::vector<float> out(in.size());
  gate.process(in.data(), out.data(), out.size());
  for (std::size_t i = 1; i < out.size(); ++i)
    REQUIRE(out[i] >= out[i - 1]);
}

TEST_CASE("KeyGate L2: release ramp is monotonic non-increasing", "[keygate][L2]") {
  KeyGate gate{kSampleRate, 0.005, 0.05};
  gate.gate_on();
  auto warm = dc_signal(kSampleRate / 50); // 20 ms — settle to Sustain
  std::vector<float> warm_out(warm.size());
  gate.process(warm.data(), warm_out.data(), warm.size());

  gate.gate_off();
  auto in = dc_signal(kSampleRate / 10); // 100 ms >= release time
  std::vector<float> out(in.size());
  gate.process(in.data(), out.data(), out.size());
  for (std::size_t i = 1; i < out.size(); ++i)
    REQUIRE(out[i] <= out[i - 1]);
}

TEST_CASE("KeyGate L2: Sustain output equals input (unity gain)", "[keygate][L2]") {
  KeyGate gate{kSampleRate, 0.001, 0.05};
  gate.gate_on();
  auto warm = dc_signal(kSampleRate / 100); // 10 ms — well past 1 ms attack
  std::vector<float> warm_out(warm.size());
  gate.process(warm.data(), warm_out.data(), warm.size());

  auto in = dc_signal(1024, 0.7f);
  std::vector<float> out(1024);
  gate.process(in.data(), out.data(), out.size());
  for (auto s : out) REQUIRE(s == Catch::Approx(0.7f).margin(1e-6));
}

TEST_CASE("KeyGate L2: click suppression on note-on (DC input)", "[keygate][L2]") {
  // The click that "click suppression" prevents is the abrupt onset at note-on.
  // For a held DC input (the cleanest analytical case), the output sample-to-
  // sample step is exactly envelope_step * input — i.e. 1/N for a 5 ms attack
  // at 48 kHz (N = 240). The bound 0.05 is the click-audibility threshold.
  // Real audio input (square waves at pitch rates << sample rate) produces
  // steps dominated by this envelope contribution at note-on.
  KeyGate gate{kSampleRate, /*attack_seconds=*/0.005, /*release_seconds=*/0.05};
  gate.gate_on();

  std::vector<float> sig(240, 1.0f);
  std::vector<float> out(sig.size());
  gate.process(sig.data(), out.data(), out.size());

  float max_step = 0.0f;
  for (std::size_t i = 1; i < out.size(); ++i)
    max_step = std::max(max_step, std::abs(out[i] - out[i - 1]));
  REQUIRE(max_step < 0.05f);
}

TEST_CASE("KeyGate L2: idempotent retrigger does not click on Attacking", "[keygate][L2]") {
  // Calling gate_on() while already Attacking should not jump the envelope.
  KeyGate gate{kSampleRate, 0.005, 0.05};
  gate.gate_on();

  std::vector<float> sig(120, 1.0f), out(120);
  gate.process(sig.data(), out.data(), out.size()); // half-attack
  const float mid = out.back();
  gate.gate_on();
  std::vector<float> sig2(1, 1.0f), out2(1);
  gate.process(sig2.data(), out2.data(), 1);
  REQUIRE(std::abs(out2[0] - mid) < 0.01f);
}
