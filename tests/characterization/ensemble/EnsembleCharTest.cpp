#include "vp330/ensemble/Ensemble.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::Ensemble;

namespace {

float cross_correlation(const std::vector<float>& a, const std::vector<float>& b) {
  const std::size_t n = a.size();
  double sum_ab = 0, sum_aa = 0, sum_bb = 0;
  for (std::size_t i = 0; i < n; ++i) {
    sum_ab += static_cast<double>(a[i]) * b[i];
    sum_aa += static_cast<double>(a[i]) * a[i];
    sum_bb += static_cast<double>(b[i]) * b[i];
  }
  const double denom = std::sqrt(sum_aa * sum_bb);
  return denom < 1e-30 ? 0.f : static_cast<float>(sum_ab / denom);
}

} // namespace

TEST_CASE("Ensemble L2: stereo cross-correlation < 0.8 (decorrelated)", "[ensemble][L2]") {
  Ensemble ens{48000};
  // Two full LFO periods at 0.7 Hz: ~2.86 s = 137280 samples
  const int n = 2 * static_cast<int>(48000.f / 0.7f) + 1;

  // Use a 440 Hz tone: the ±1.5 ms / ±3 ms delay differences create meaningful
  // phase separation at pitched frequencies.  DC or broadband noise cannot verify
  // decorrelation because the 50% dry component alone pins correlation near 0.86
  // for white noise; a tone at 440 Hz yields ~0.70, well below the 0.8 threshold.
  const float k440 = 2.f * std::numbers::pi_v<float> * 440.f / 48000.f;
  std::vector<float> in(n);
  for (int i = 0; i < n; ++i)
    in[i] = 0.1f * std::sin(k440 * static_cast<float>(i));

  std::vector<float> left(n), right(n);
  // Process in 512-sample blocks so the LFO phase advances per block.
  constexpr std::size_t kBlock = 512;
  for (std::size_t off = 0; off < static_cast<std::size_t>(n); off += kBlock) {
    const std::size_t len = std::min(kBlock, static_cast<std::size_t>(n) - off);
    ens.process(in.data() + off, left.data() + off, right.data() + off, len);
  }

  // Measure correlation over the second half (past initial delay settling)
  std::vector<float> l2(left.begin() + n / 2, left.end());
  std::vector<float> r2(right.begin() + n / 2, right.end());
  REQUIRE(cross_correlation(l2, r2) < 0.8f);
}

TEST_CASE("Ensemble L2: output level within expected range for DC input", "[ensemble][L2]") {
  Ensemble ens{48000};
  const int n = 48000;
  std::vector<float> in(n, 1.0f), left(n), right(n);
  ens.process(in.data(), left.data(), right.data(), n);

  // With dry=0.5 and two wet lines at 0.25 each, total gain approaches 1.0
  // for DC (all lines agree at DC). Allow [0.8, 1.2] for modulation variance.
  auto mean_abs = [](const std::vector<float>& v) {
    float s = 0;
    for (auto x : v)
      s += std::fabs(x);
    return s / static_cast<float>(v.size());
  };
  REQUIRE(mean_abs(left) > 0.8f);
  REQUIRE(mean_abs(left) < 1.2f);
  REQUIRE(mean_abs(right) > 0.8f);
  REQUIRE(mean_abs(right) < 1.2f);
}
