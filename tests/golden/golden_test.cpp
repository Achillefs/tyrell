#include "render.h"
#include "wav_io.h"

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

TEST_CASE("golden: single C4 choir section matches baseline", "[golden]") {
  // Phase 3: SynthesisEngine routes through ChoirSection (UpperMale8 on by
  // default). The output is a bandpass-filtered square wave — zero-crossing
  // frequency is not a meaningful check. Compare against a committed baseline
  // using a ±6 dB RMS envelope guard (coarse proxy for log-spectral distance,
  // which will be implemented as L3 infrastructure in a later phase).
  const auto wav = render_fixture("single-c4-1s.mid", 48000, 1.0);
  REQUIRE(wav.sample_rate == 48000);
  REQUIRE(wav.channels == 2);

  const auto left = left_channel(wav);
  REQUIRE_FALSE(left.empty());
  const auto level = rms(left);
  INFO("RMS: " << level);

  REQUIRE(level > 1e-4);               // choir section is producing sound
  REQUIRE(max_abs_sample(wav) < 1.0f); // no clipping

  const auto baseline = load_wav(std::string(GOLDEN_BASELINES_DIR) + "/single-c4-1s.wav");
  REQUIRE(wav.frames == baseline.frames);
  REQUIRE(wav.channels == baseline.channels);
  REQUIRE(wav.sample_rate == baseline.sample_rate);

  const auto baseline_left = left_channel(baseline);
  const auto baseline_level = rms(baseline_left);
  INFO("baseline RMS: " << baseline_level);
  // ±6 dB tolerance: catches disabled choir, gain regressions, silent output.
  // Tighter per-sample comparison is deferred until filter calibration
  // stabilises post Phase 4 (compander).
  REQUIRE(level > baseline_level * 0.5);
  REQUIRE(level < baseline_level * 2.0);
}
