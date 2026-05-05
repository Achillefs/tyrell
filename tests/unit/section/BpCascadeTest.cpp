#include "vp330/section/BpCascade.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::BpCascade;

TEST_CASE("BpCascade: rejects DC input after settling", "[bp_cascade][L1]") {
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};
  // F0 (175 Hz, Q=6.79) has a pole radius ~0.9983 → ~591-sample time constant.
  // 10000 samples gives ~16 time constants: transient decays to <1e-7 of initial.
  std::vector<float> in(10000, 1.0f), out(10000);
  bp.process(in.data(), out.data(), 10000);
  for (int i = 9000; i < 10000; ++i)
    REQUIRE(std::abs(out[i]) < 0.01f);
}

TEST_CASE("BpCascade: BIBO stable at resonance", "[bp_cascade][L1]") {
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};
  std::vector<float> in(10000), out(10000);
  for (int i = 0; i < 10000; ++i)
    in[i] = std::sin(2.f * 3.14159265f * 190.f * i / 48000.f);
  bp.process(in.data(), out.data(), 10000);
  for (auto s : out) {
    REQUIRE(std::isfinite(s));
    REQUIRE(std::abs(s) < 50.f); // peak transient ≈ Q² ≈ 46 for amplitude-1 input near resonance
  }
}

TEST_CASE("BpCascade: reset zeroes output on silence", "[bp_cascade][L1]") {
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};
  // Drive filter into a non-zero state.
  std::vector<float> in(500, 1.0f), out(500);
  bp.process(in.data(), out.data(), 500);
  bp.reset();
  // After reset, silence in → silence out.
  std::vector<float> zeros(100, 0.0f), zeros_out(100);
  bp.process(zeros.data(), zeros_out.data(), 100);
  for (auto s : zeros_out)
    REQUIRE(s == 0.0f);
}
