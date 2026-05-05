#include "vp330/section/ChoirFilterBank.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::ChoirFilterBank;

namespace {

float filter_rms(ChoirFilterBank& bank, int idx, float freq) {
  bank.reset();
  const int n = 48000;
  std::vector<float> in(n);
  const float k = 2.f * std::numbers::pi_v<float> * freq / 48000.f;
  for (int i = 0; i < n; ++i) {
    in[i] = std::sin(k * static_cast<float>(i));
  }
  bank.process(in.data(), n);
  const float* out = bank.output(idx);
  float sum = 0;
  for (int i = n / 2; i < n; ++i) {
    sum += out[i] * out[i];
  }
  return std::sqrt(2.f * sum / static_cast<float>(n));
}

} // namespace

TEST_CASE("ChoirFilterBank L2: F2 (559/671 Hz) passes 600 Hz, rejects 100 Hz",
          "[choir_fb][L2]") {
  ChoirFilterBank bank{48000};
  REQUIRE(filter_rms(bank, 2, 600.f) > filter_rms(bank, 2, 100.f) * 3.f);
}

TEST_CASE("ChoirFilterBank L2: F5 (2532/3098 Hz) passes 2700 Hz, rejects 500 Hz",
          "[choir_fb][L2]") {
  ChoirFilterBank bank{48000};
  REQUIRE(filter_rms(bank, 5, 2700.f) > filter_rms(bank, 5, 500.f) * 3.f);
}

TEST_CASE("ChoirFilterBank L2: F0 (175/216 Hz) passes 190 Hz, rejects 2000 Hz",
          "[choir_fb][L2]") {
  ChoirFilterBank bank{48000};
  REQUIRE(filter_rms(bank, 0, 190.f) > filter_rms(bank, 0, 2000.f) * 3.f);
}
