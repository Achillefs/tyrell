#include "vp330/ensemble/Ensemble.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

using vp330::Ensemble;

TEST_CASE("Ensemble L1: enabled produces different L and R", "[ensemble][L1]") {
  Ensemble ens{48000};
  const int n = 48000; // one full second covers > one LFO period
  std::vector<float> in(n, 0.1f), left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  bool any_diff = false;
  for (int i = 0; i < n; ++i)
    if (left[i] != right[i]) {
      any_diff = true;
      break;
    }
  REQUIRE(any_diff);
}

TEST_CASE("Ensemble L1: bypass produces L == R == dry (bit-exact)", "[ensemble][L1]") {
  Ensemble ens{48000};
  ens.set_enabled(false);

  const int n = 512;
  std::vector<float> in(n);
  for (int i = 0; i < n; ++i)
    in[i] = static_cast<float>(i) * 0.001f; // non-trivial signal
  std::vector<float> left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  for (int i = 0; i < n; ++i) {
    REQUIRE(left[i] == in[i]);
    REQUIRE(right[i] == in[i]);
  }
}

TEST_CASE("Ensemble L1: LFO phase does not advance while bypassed", "[ensemble][L1]") {
  Ensemble ens{48000};
  std::vector<float> in(256, 0.1f), l(256), r(256);

  ens.process(in.data(), l.data(), r.data(), 256); // one enabled block advances phase
  const float phase_before_bypass = ens.test_phase();

  ens.set_enabled(false);
  for (int k = 0; k < 10; ++k)
    ens.process(in.data(), l.data(), r.data(), 256); // 10 bypass blocks

  // Phase must be identical to what it was before the bypass blocks.
  REQUIRE(ens.test_phase() == phase_before_bypass);
}
