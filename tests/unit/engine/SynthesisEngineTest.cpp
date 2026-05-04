#include "vp330/engine/SynthesisEngine.h"

#include <catch2/catch_approx.hpp>
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

// Counts sign changes in `buf` and reports the equivalent fundamental in Hz.
// For a clean square at frequency f, this returns ~f because a square has two
// sign changes per cycle.
double zero_crossing_frequency(const std::vector<float>& buf, int sample_rate) {
  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    const bool prev_pos = buf[i - 1] >= 0.0f;
    const bool curr_pos = buf[i] >= 0.0f;
    if (prev_pos != curr_pos) ++crossings;
  }
  const double seconds = static_cast<double>(buf.size()) / sample_rate;
  return static_cast<double>(crossings) / (2.0 * seconds);
}

double rms(const std::vector<float>& buf) {
  double acc = 0.0;
  for (auto s : buf)
    acc += static_cast<double>(s) * s;
  return std::sqrt(acc / static_cast<double>(buf.size()));
}

} // namespace

TEST_CASE("SynthesisEngine: single C4 renders a square at ~261.63 Hz", "[engine][audio]") {
  constexpr int sample_rate = 48000;
  constexpr std::size_t frames = sample_rate; // 1.0 s of audio
  SynthesisEngine engine{sample_rate};
  engine.note_on(vp330::kMidiC4);

  std::vector<float> left(frames), right(frames);
  engine.render(left.data(), right.data(), frames);

  const double f = zero_crossing_frequency(left, sample_rate);
  REQUIRE(f == Catch::Approx(261.63).margin(2.0));

  REQUIRE(rms(left) == Catch::Approx(0.05).margin(0.005));

  for (std::size_t i = 0; i < frames; ++i)
    REQUIRE(left[i] == right[i]);
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
