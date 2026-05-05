#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirSwitch.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using vp330::ChoirSection;
using vp330::ChoirSwitch;

TEST_CASE("ChoirSection: all switches off → silence", "[choir_section][L1]") {
  ChoirSection cs{48000};
  std::vector<float> lower(256, 0.5f), upper(256, 0.5f);
  std::vector<float> left(256), right(256);
  cs.process(lower.data(), lower.data(), upper.data(), upper.data(),
             left.data(), right.data(), 256);
  for (auto s : left) {
    REQUIRE(s == 0.0f);
  }
  for (auto s : right) {
    REQUIRE(s == 0.0f);
  }
}

TEST_CASE("ChoirSection: lower zone silence when only upper switch is on", "[choir_section][L1]") {
  ChoirSection cs{48000};
  cs.set_switch(ChoirSwitch::UpperMale8, true);
  // Feed signal in lower zone only, upper zone silent.
  std::vector<float> lower(256, 0.5f), upper(256, 0.0f);
  std::vector<float> left(256), right(256);
  cs.process(lower.data(), lower.data(), upper.data(), upper.data(),
             left.data(), right.data(), 256);
  float max_left = 0.0f;
  float max_right = 0.0f;
  for (auto s : left) {
    max_left = std::max(max_left, std::abs(s));
  }
  for (auto s : right) {
    max_right = std::max(max_right, std::abs(s));
  }
  REQUIRE(max_left < 1e-6f);
  REQUIRE(max_right < 1e-6f);
}

TEST_CASE("ChoirSection: left and right outputs are equal (mono duplication)",
          "[choir_section][L1]") {
  ChoirSection cs{48000};
  cs.set_switch(ChoirSwitch::UpperMale8, true);
  std::vector<float> lower(256, 0.0f), upper(256, 0.1f);
  std::vector<float> left(256), right(256);
  // Settle the filter first.
  for (int i = 0; i < 10; ++i) {
    cs.process(lower.data(), lower.data(), upper.data(), upper.data(),
               left.data(), right.data(), 256);
  }
  for (std::size_t i = 0; i < 256; ++i) {
    REQUIRE(left[i] == right[i]);
  }
}
