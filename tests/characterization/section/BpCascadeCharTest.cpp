#include "vp330/section/BpCascade.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <numbers>
#include <vector>

using vp330::BpCascade;

namespace {

float rms_at(BpCascade& bp, float freq, int sample_rate = 48000) {
  bp.reset();
  const int n = sample_rate;
  std::vector<float> in(n), out(n);
  const float k = 2.f * std::numbers::pi_v<float> * freq / static_cast<float>(sample_rate);
  for (int i = 0; i < n; ++i) {
    in[i] = std::sin(k * static_cast<float>(i));
  }
  bp.process(in.data(), out.data(), n);
  // Measure second half (settled state).
  float sum = 0;
  for (int i = n / 2; i < n; ++i) {
    sum += out[i] * out[i];
  }
  return std::sqrt(2.f * sum / static_cast<float>(n));
}

} // namespace

TEST_CASE("BpCascade L2: passes signal near centre, rejects extremes", "[bp_cascade][L2]") {
  // F0: stages at 175 Hz and 216 Hz — centre region ~190 Hz.
  BpCascade bp{175.f, 6.79f, 216.f, 6.83f, 48000};

  const float rms_centre = rms_at(bp, 190.f);
  const float rms_lo     = rms_at(bp, 30.f);
  const float rms_hi     = rms_at(bp, 5000.f);

  // At least 10 dB more gain in the passband than at the extremes.
  REQUIRE(rms_centre > rms_lo  * 3.f);
  REQUIRE(rms_centre > rms_hi  * 3.f);
}

TEST_CASE("BpCascade L2: higher-frequency filter (F5) centred around 2700 Hz", "[bp_cascade][L2]") {
  BpCascade bp{2532.f, 6.83f, 3098.f, 6.83f, 48000};

  const float rms_centre = rms_at(bp, 2700.f);
  const float rms_lo     = rms_at(bp, 400.f);
  const float rms_hi     = rms_at(bp, 12000.f);

  REQUIRE(rms_centre > rms_lo * 3.f);
  REQUIRE(rms_centre > rms_hi * 3.f);
}
