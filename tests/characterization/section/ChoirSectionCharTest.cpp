#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirSwitch.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::ChoirSection;
using vp330::ChoirSwitch;

namespace {

double rms(const std::vector<float>& v, int start) {
  double s = 0;
  for (int i = start; i < static_cast<int>(v.size()); ++i) {
    s += static_cast<double>(v[i]) * v[i];
  }
  return std::sqrt(s / static_cast<double>(v.size() - static_cast<std::size_t>(start)));
}

} // namespace

TEST_CASE("ChoirSection L2: UpperMale8 produces non-silent output from upper zone",
          "[choir_section][L2]") {
  ChoirSection cs{48000};
  cs.set_switch(ChoirSwitch::UpperMale8, true);

  const int n = 48000;
  std::vector<float> lower(n, 0.f), upper(n), left(n), right(n);
  // Square wave at ~261 Hz (C4 region).
  for (int i = 0; i < n; ++i) {
    upper[i] = (i % 184 < 92) ? 0.05f : -0.05f;
  }
  cs.process(lower.data(), lower.data(), upper.data(), upper.data(),
             left.data(), right.data(), n);

  REQUIRE(rms(left, n / 2) > 1e-5);
}

TEST_CASE("ChoirSection L2: both lower switches sum louder than one alone",
          "[choir_section][L2]") {
  ChoirSection cs1{48000};
  ChoirSection cs2{48000};
  cs1.set_switch(ChoirSwitch::LowerMale8, true);
  cs2.set_switch(ChoirSwitch::LowerMale8, true);
  cs2.set_switch(ChoirSwitch::LowerMale4, true);

  const int n = 48000;
  std::vector<float> lower(n), upper(n, 0.f);
  std::vector<float> l1(n), r1(n), l2(n), r2(n);
  for (int i = 0; i < n; ++i) {
    lower[i] = (i % 100 < 50) ? 0.05f : -0.05f;
  }

  cs1.process(lower.data(), lower.data(), upper.data(), upper.data(),
              l1.data(), r1.data(), n);
  cs2.process(lower.data(), lower.data(), upper.data(), upper.data(),
              l2.data(), r2.data(), n);

  REQUIRE(rms(l2, n / 2) > rms(l1, n / 2));
}
