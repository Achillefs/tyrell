#include "vp330/section/ChoirFilterBank.h"

#include "vp330/section/ChoirConstants.h"
#include "vp330/section/ChoirSwitch.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>

using vp330::ChoirSwitch;
using namespace vp330::mkii;

TEST_CASE("ChoirFilterBank: LowerMale8 uses filters 0,2,3,5 with correct weights",
          "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::LowerMale8)];
  REQUIRE(v.filters == std::array<int, 4>{0, 2, 3, 5});
  REQUIRE(v.weights[0] == Catch::Approx(0.3125f));
  REQUIRE(v.weights[1] == Catch::Approx(1.f));
  REQUIRE(v.weights[2] == Catch::Approx(1.f));
  REQUIRE(v.weights[3] == Catch::Approx(0.3125f));
}

TEST_CASE("ChoirFilterBank: LowerMale4 uses filters 1,2,3,5 with correct weights",
          "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::LowerMale4)];
  REQUIRE(v.filters == std::array<int, 4>{1, 2, 3, 5});
  REQUIRE(v.weights[0] == Catch::Approx(0.125f));
}

TEST_CASE("ChoirFilterBank: UpperMale8 uses filters 1,2,3,5 with correct weights",
          "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::UpperMale8)];
  REQUIRE(v.filters == std::array<int, 4>{1, 2, 3, 5});
  REQUIRE(v.weights[0] == Catch::Approx(0.1875f));
}

TEST_CASE("ChoirFilterBank: UpperFemale4 uses filters 2,3,4,6 with correct weights",
          "[choir_fb][L1]") {
  const auto& v = kVoiceWeights[static_cast<int>(ChoirSwitch::UpperFemale4)];
  REQUIRE(v.filters == std::array<int, 4>{2, 3, 4, 6});
  REQUIRE(v.weights[0] == Catch::Approx(0.1875f));
  REQUIRE(v.weights[2] == Catch::Approx(0.6875f));
  REQUIRE(v.weights[3] == Catch::Approx(0.1875f));
}

TEST_CASE("ChoirFilterBank: silence in → silence out for all 7 outputs", "[choir_fb][L1]") {
  vp330::ChoirFilterBank bank{48000};
  std::vector<float> in(256, 0.0f);
  bank.process(in.data(), 256);
  for (int i = 0; i < 7; ++i) {
    for (std::size_t f = 0; f < 256; ++f) {
      REQUIRE(bank.output(i)[f] == 0.0f);
    }
  }
}
