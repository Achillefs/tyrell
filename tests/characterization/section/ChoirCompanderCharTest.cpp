#include "vp330/section/ChoirCompander.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirCompander;

TEST_CASE("ChoirCompander L2: output does not clip for high-amplitude input",
          "[choir_compander][L2]") {
  ChoirCompander cc{48000};
  const int n = 48000;
  std::vector<float> in(n, 1.0f), out(n);
  cc.process(in.data(), out.data(), n);

  float peak = 0;
  for (auto s : out)
    peak = std::max(peak, std::fabs(s));
  // g = kGMax * env / (kEnvTarget + env) < kGMax always, so output < kGMax * input
  REQUIRE(peak < static_cast<float>(vp330::kGMax));
}

TEST_CASE("ChoirCompander L2: gain monotonically increases with envelope level",
          "[choir_compander][L2]") {
  auto steady_gain = [](float amplitude) {
    ChoirCompander cc{48000};
    std::vector<float> in(48000, amplitude), out(48000);
    cc.process(in.data(), out.data(), 48000);
    return std::fabs(out.back()) / amplitude;
  };

  const float g_low = steady_gain(0.05f);
  const float g_mid = steady_gain(0.1f);
  const float g_high = steady_gain(1.0f);

  REQUIRE(g_low < g_mid);
  REQUIRE(g_mid < g_high);
  REQUIRE(g_mid == Catch::Approx(static_cast<float>(vp330::kGMax) / 2.f).margin(0.01f));
}
