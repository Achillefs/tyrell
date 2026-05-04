#include "vp330/engine/SynthesisEngine.h"

#include <catch2/catch_test_macros.hpp>

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
