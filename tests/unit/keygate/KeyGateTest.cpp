#include "vp330/keygate/KeyGate.h"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using vp330::KeyGate;

namespace {

KeyGate make_gate(int sample_rate = 48000) {
  // 1 ms attack, 50 ms release: short enough that tests finish quickly,
  // long enough that single-sample steps cannot leap straight to Sustain.
  return KeyGate{sample_rate, /*attack_seconds=*/0.001, /*release_seconds=*/0.05};
}

} // namespace

TEST_CASE("KeyGate: starts Idle", "[keygate]") {
  auto g = make_gate();
  REQUIRE(g.state() == KeyGate::State::Idle);
}

TEST_CASE("KeyGate: gate_on transitions Idle -> Attacking", "[keygate]") {
  auto g = make_gate();
  g.gate_on();
  REQUIRE(g.state() == KeyGate::State::Attacking);
}

TEST_CASE("KeyGate: reaches Sustain after attack-time samples", "[keygate]") {
  auto g = make_gate(48000);
  g.gate_on();

  // 1 ms @ 48 kHz = 48 samples. Process exactly that many.
  std::vector<float> sig(48, 1.0f), out(48);
  g.process(sig.data(), out.data(), out.size());
  REQUIRE(g.state() == KeyGate::State::Sustain);
}

TEST_CASE("KeyGate: gate_off from Sustain transitions to Releasing", "[keygate]") {
  auto g = make_gate();
  g.gate_on();
  std::vector<float> sig(48, 1.0f), out(48);
  g.process(sig.data(), out.data(), out.size());
  g.gate_off();
  REQUIRE(g.state() == KeyGate::State::Releasing);
}

TEST_CASE("KeyGate: returns to Idle after release-time samples", "[keygate]") {
  auto g = make_gate(48000);
  g.gate_on();
  std::vector<float> warm(48, 1.0f), warm_out(48);
  g.process(warm.data(), warm_out.data(), warm.size());
  g.gate_off();

  // 50 ms @ 48 kHz = 2400 samples; add 1 to absorb FP rounding in the
  // accumulator (the residual after 2400 subtractions of 1/2400 is ~5e-14).
  std::vector<float> sig(2401, 1.0f), out(2401);
  g.process(sig.data(), out.data(), out.size());
  REQUIRE(g.state() == KeyGate::State::Idle);
}

TEST_CASE("KeyGate: gate_on during Releasing returns to Attacking", "[keygate]") {
  auto g = make_gate();
  g.gate_on();
  std::vector<float> sig(48, 1.0f), out(48);
  g.process(sig.data(), out.data(), out.size());
  g.gate_off();
  REQUIRE(g.state() == KeyGate::State::Releasing);
  g.gate_on();
  REQUIRE(g.state() == KeyGate::State::Attacking);
}

TEST_CASE("KeyGate: idle output is zero regardless of input", "[keygate]") {
  auto g = make_gate();
  std::vector<float> sig(64, 1.0f), out(64, 999.0f);
  g.process(sig.data(), out.data(), out.size());
  for (auto s : out) REQUIRE(s == 0.0f);
}
