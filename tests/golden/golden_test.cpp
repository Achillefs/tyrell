#include "render.h"
#include "wav_io.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <string>

#ifndef GOLDEN_BASELINES_DIR
#error "GOLDEN_BASELINES_DIR must be defined by CMake"
#endif

using namespace vp330::test;

TEST_CASE("walking skeleton: silence renders to silence", "[golden]") {
  const auto wav = render_fixture("silence-1s.mid", 48000, 1.0);
  const auto baseline = load_wav(std::string(GOLDEN_BASELINES_DIR) + "/silence-1s.wav");

  REQUIRE(wav.frames == baseline.frames);
  REQUIRE(wav.channels == baseline.channels);
  REQUIRE(wav.sample_rate == baseline.sample_rate);
  REQUIRE(wav.samples.size() == baseline.samples.size());

  // Element-wise comparison so a non-silent regression is caught on the actual
  // diff, not just on a "still-silent overall" guard. Tolerance matches the
  // silence band used below.
  constexpr float kTolerance = 1e-6f;
  float worst_delta = 0.0f;
  std::size_t worst_index = 0;
  for (std::size_t i = 0; i < wav.samples.size(); ++i) {
    const float delta = std::fabs(wav.samples[i] - baseline.samples[i]);
    if (delta > worst_delta) {
      worst_delta = delta;
      worst_index = i;
    }
  }
  INFO("worst sample delta " << worst_delta << " at frame " << (worst_index / 2) << " channel "
                             << (worst_index % 2));
  REQUIRE(worst_delta < kTolerance);

  // Sanity: both sides really are near-silence.
  REQUIRE(max_abs_sample(wav) < kTolerance);
  REQUIRE(max_abs_sample(baseline) < kTolerance);
}
