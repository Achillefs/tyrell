#include "render.h"
#include "wav_io.h"

#include "vp330/tod/MkIIConstants.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#ifndef GOLDEN_BASELINES_DIR
#error "GOLDEN_BASELINES_DIR must be defined by CMake"
#endif

using namespace vp330::test;

namespace {

// Measure fundamental from the time delta between first and last sign change.
// Sub-cent precision across the keyboard's range — the naive crossings/(2*seconds)
// formulation only resolves to ±13 ¢ at low frequencies.
double zero_crossing_frequency(const std::vector<float>& buf, int sample_rate) {
  std::size_t first_xing = 0;
  std::size_t last_xing = 0;
  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) {
      if (crossings == 0) first_xing = i;
      last_xing = i;
      ++crossings;
    }
  }
  if (crossings < 2) return 0.0;
  const double interval_samples = static_cast<double>(last_xing - first_xing);
  const double half_cycles = static_cast<double>(crossings - 1);
  return (half_cycles * sample_rate) / (2.0 * interval_samples);
}

double rms(const std::vector<float>& buf) {
  double acc = 0.0;
  for (auto s : buf)
    acc += static_cast<double>(s) * s;
  return std::sqrt(acc / static_cast<double>(buf.size()));
}

std::vector<float> left_channel(const Wav& w) {
  std::vector<float> out;
  out.reserve(w.frames);
  for (std::size_t i = 0; i < w.frames; ++i)
    out.push_back(w.samples[i * w.channels]);
  return out;
}

} // namespace

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

TEST_CASE("golden: single C4 renders the MkII C-divider frequency", "[golden]") {
  // C4 comes through as TOD's C output (divider 478) divided down 4 octaves
  // (octave_down = 8 - 4 = 4 → /16). Tight tolerance around master/N/16
  // catches any regression in the TOD/divider math; ±0.05 Hz is well below
  // perceptual threshold and well within the measurement precision of the
  // interval-based zero-crossing method.
  constexpr double kMkIIC4Hz =
      vp330::mkii::kMasterClockHz.value() / vp330::mkii::kDividerRatios[0] / 16.0;

  const auto wav = render_fixture("single-c4-1s.mid", 48000, 1.0);
  REQUIRE(wav.sample_rate == 48000);
  REQUIRE(wav.channels == 2);
  REQUIRE(wav.samples.size() == wav.frames * static_cast<std::size_t>(wav.channels));

  const auto left = left_channel(wav);
  const auto f = zero_crossing_frequency(left, wav.sample_rate);
  INFO("estimated fundamental: " << f << " Hz; expected MkII C4: " << kMkIIC4Hz);
  REQUIRE(f == Catch::Approx(kMkIIC4Hz).margin(0.05));

  const auto level = rms(left);
  INFO("RMS: " << level);
  // Slightly lower envelope-shaped RMS than Phase 1's flat square (5 ms attack
  // at the start of a 1 s render), but still well inside the band.
  REQUIRE(level == Catch::Approx(0.05).margin(0.005));
}
