#include "vp330/engine/SynthesisEngine.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::MidiNote;
using vp330::SynthesisEngine;

TEST_CASE("SynthesisEngine: stores its sample rate", "[engine]") {
  SynthesisEngine engine{48000};
  REQUIRE(engine.sample_rate() == 48000);
}

TEST_CASE("SynthesisEngine: tracks active notes via note_on / note_off", "[engine]") {
  SynthesisEngine engine{48000};
  const auto c4 = vp330::kMidiC4;

  REQUIRE_FALSE(engine.is_note_active(c4));

  engine.note_on(c4);
  REQUIRE(engine.is_note_active(c4));

  engine.note_off(c4);
  REQUIRE_FALSE(engine.is_note_active(c4));
}

TEST_CASE("SynthesisEngine: independent state per note", "[engine]") {
  SynthesisEngine engine{48000};
  const auto c4 = vp330::kMidiC4;
  const auto a4 = vp330::kMidiA4;

  engine.note_on(c4);
  engine.note_on(a4);
  REQUIRE(engine.is_note_active(c4));
  REQUIRE(engine.is_note_active(a4));

  engine.note_off(c4);
  REQUIRE_FALSE(engine.is_note_active(c4));
  REQUIRE(engine.is_note_active(a4));
}

TEST_CASE("SynthesisEngine: render returns silence with no notes active", "[engine]") {
  SynthesisEngine engine{48000};
  std::vector<float> left(128, 0.5f);
  std::vector<float> right(128, 0.5f);

  engine.render(left.data(), right.data(), left.size());

  for (auto v : left)
    REQUIRE(v == 0.0f);
  for (auto v : right)
    REQUIRE(v == 0.0f);
}

namespace {

double rms(const std::vector<float>& buf) {
  double acc = 0.0;
  for (auto s : buf)
    acc += static_cast<double>(s) * s;
  return std::sqrt(acc / static_cast<double>(buf.size()));
}

} // namespace

TEST_CASE("SynthesisEngine: single C4 produces non-silent stereo output", "[engine][audio]") {
  // Phase 4: ChoirCompander + Ensemble in chain; output is true stereo (L ≠ R).
  constexpr int sample_rate = 48000;
  constexpr std::size_t frames = sample_rate;
  SynthesisEngine engine{sample_rate};
  engine.note_on(vp330::kMidiC4);

  std::vector<float> left(frames), right(frames);
  engine.render(left.data(), right.data(), frames);

  REQUIRE(rms(left) > 1e-4);
  REQUIRE(rms(right) > 1e-4);

  bool any_stereo_difference = false;
  for (std::size_t i = 0; i < frames; ++i) {
    if (std::fabs(left[i] - right[i]) > 1e-6f) {
      any_stereo_difference = true;
      break;
    }
  }
  REQUIRE(any_stereo_difference);

  float max_abs = 0.0f;
  for (std::size_t i = 0; i < frames; ++i) {
    if (std::fabs(left[i]) > max_abs) max_abs = std::fabs(left[i]);
    if (std::fabs(right[i]) > max_abs) max_abs = std::fabs(right[i]);
  }
  // Compander is a gain stage (not a limiter); kGMax=4.0 × filter peak ≈ 8× is expected.
  // Bound is intentionally loose; tighter constraint lives in the ChoirCompander L2 test.
  REQUIRE(max_abs < 20.0f);
}

TEST_CASE("SynthesisEngine: silence after note_off", "[engine][audio]") {
  constexpr int sample_rate = 48000;
  SynthesisEngine engine{sample_rate};
  engine.note_on(vp330::kMidiC4);
  engine.note_off(vp330::kMidiC4);

  std::vector<float> left(1024), right(1024);
  engine.render(left.data(), right.data(), left.size());

  REQUIRE(rms(left) == 0.0);
}
