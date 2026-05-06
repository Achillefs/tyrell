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

  // 50 ms @ 48 kHz = 2400 samples; after exactly that many, gate is Idle.
  std::vector<float> sig(2400, 1.0f), out(2400);
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
  for (auto s : out)
    REQUIRE(s == 0.0f);
}

TEST_CASE("KeyGate: set_attack_seconds takes effect on next gate_on", "[keygate][L1]") {
  KeyGate gate{48000, /*attack=*/0.001, /*release=*/0.05}; // 1 ms = 48 samples

  gate.set_attack_seconds(0.002); // 2 ms = 96 samples
  gate.gate_on();

  std::vector<float> sig(48, 1.0f), out(48);
  gate.process(sig.data(), out.data(), 48);
  REQUIRE(gate.state() == KeyGate::State::Attacking); // not yet done at 48 samples

  gate.process(sig.data(), out.data(), 48);
  REQUIRE(gate.state() == KeyGate::State::Sustain); // done at 96 samples
}

TEST_CASE("KeyGate: set_release_seconds takes effect on next gate_off", "[keygate][L1]") {
  KeyGate gate{48000, 0.001, /*release=*/0.05}; // 50 ms = 2400 samples
  gate.gate_on();
  std::vector<float> warm(48, 1.f), wo(48);
  gate.process(warm.data(), wo.data(), 48);

  gate.set_release_seconds(0.01); // 10 ms = 480 samples
  gate.gate_off();

  std::vector<float> sig(480, 1.f), out(480);
  gate.process(sig.data(), out.data(), 480);
  REQUIRE(gate.state() == KeyGate::State::Idle);
}
