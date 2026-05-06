#include "vp330/section/ChoirCompander.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirCompander;

TEST_CASE("ChoirCompander L1: instantaneous attack — envelope reaches step in ≤ 2 samples",
          "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  // Feed step: 0 → 0.5
  std::vector<float> in{0.f, 0.5f, 0.5f, 0.5f};
  std::vector<float> out(4);
  cc.process(in.data(), out.data(), 4);

  // Instantaneous attack: g[1] should already reflect envelope=0.5
  // g = kGMax * 0.5 / (kEnvTarget + 0.5) ≈ 3.33; out[1] ≈ 1.67
  REQUIRE(out[1] > out[0]);
  REQUIRE(out[2] == Catch::Approx(out[1]).margin(1e-5f));
}

TEST_CASE("ChoirCompander L1: steady-state gain at env_target is g_max/2",
          "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  // DC at kEnvTarget: g = kGMax * kEnvTarget / (kEnvTarget + kEnvTarget) = kGMax/2
  const float probe = static_cast<float>(vp330::kEnvTarget);
  std::vector<float> in(4, probe), out(4);
  cc.process(in.data(), out.data(), 4);
  const float expected = static_cast<float>(vp330::kGMax) / 2.f * probe;
  REQUIRE(out[3] == Catch::Approx(expected).margin(1e-4f));
}

TEST_CASE("ChoirCompander L1: release τ within ±10% of 103ms", "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  float x = 1.0f, y;
  cc.process(&x, &y, 1);

  // Tiny probe so envelope decays freely without re-triggering attack.
  const float probe = 1e-6f;
  const int tau_nominal = static_cast<int>(std::round(0.103 * 48000));
  std::vector<float> in_decay(tau_nominal, probe), out_decay(tau_nominal);
  cc.process(in_decay.data(), out_decay.data(), tau_nominal);

  // Back-compute envelope from gain: env = kEnvTarget * g / (kGMax - g)
  const float g_max = static_cast<float>(vp330::kGMax);
  const float env_target = static_cast<float>(vp330::kEnvTarget);
  auto env_from_g = [g_max, env_target](float g) { return env_target * g / (g_max - g); };
  const float g0 = out_decay[0] / probe;
  const float g_tau = out_decay.back() / probe;
  const float env0 = env_from_g(g0);
  const float env_tau = env_from_g(g_tau);

  REQUIRE(env0 > 0.9f);
  // exp(−1/0.9) ≈ 0.329 (τ ≥ 92.7ms), exp(−1/1.1) ≈ 0.403 (τ ≤ 113.3ms)
  REQUIRE(env_tau > std::exp(-1.0f / 0.9f));
  REQUIRE(env_tau < std::exp(-1.0f / 1.1f));
}

TEST_CASE("ChoirCompander L1: process is in-place safe", "[choir_compander][L1]") {
  ChoirCompander cc{48000};
  std::vector<float> buf(64, 0.3f);
  cc.process(buf.data(), buf.data(), 64);
  for (auto s : buf)
    REQUIRE(std::isfinite(s));
}
