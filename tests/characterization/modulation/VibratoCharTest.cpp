#include "vp330/modulation/Vibrato.h"
#include "vp330/values/Hertz.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using vp330::Hertz;
using vp330::Vibrato;

TEST_CASE("Vibrato L2: clock oscillates at the set LFO rate", "[vibrato][L2]") {
  Vibrato v{48000};
  v.set_rate(Hertz{5.0});
  v.set_depth(0.5f);

  // Sample once per 256-sample block over ~1 second (187 blocks).
  const int block    = 256;
  const int n_blocks = 48000 / block;
  std::vector<double> clocks(n_blocks);
  for (int i = 0; i < n_blocks; ++i) {
    clocks[i] = v.tick(block).value();
  }

  // Count sign changes of clock relative to its mean.
  double mean = 0;
  for (auto c : clocks) {
    mean += c;
  }
  mean /= n_blocks;

  int crossings = 0;
  for (int i = 1; i < n_blocks; ++i) {
    const bool prev = (clocks[i - 1] - mean) >= 0;
    const bool curr = (clocks[i]     - mean) >= 0;
    if (prev != curr) {
      ++crossings;
    }
  }
  // 5 Hz × 2 sign-changes/cycle × 1 s ≈ 10 crossings.
  REQUIRE(crossings >= 8);
  REQUIRE(crossings <= 12);
}
