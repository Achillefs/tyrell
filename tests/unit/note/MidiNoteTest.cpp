#include "vp330/note/MidiNote.h"

#include <catch2/catch_test_macros.hpp>

using vp330::MidiNote;

TEST_CASE("MidiNote: try_from_int accepts 0..127", "[midi-note]") {
  REQUIRE(MidiNote::try_from_int(0).has_value());
  REQUIRE(MidiNote::try_from_int(60).has_value());
  REQUIRE(MidiNote::try_from_int(127).has_value());
}

TEST_CASE("MidiNote: try_from_int rejects out of range", "[midi-note]") {
  REQUIRE_FALSE(MidiNote::try_from_int(-1).has_value());
  REQUIRE_FALSE(MidiNote::try_from_int(128).has_value());
  REQUIRE_FALSE(MidiNote::try_from_int(1000).has_value());
}

TEST_CASE("MidiNote: value() round-trips the constructor", "[midi-note]") {
  REQUIRE(MidiNote::try_from_int(60)->value() == 60);
  REQUIRE(MidiNote::try_from_int(0)->value() == 0);
  REQUIRE(MidiNote::try_from_int(127)->value() == 127);
}

TEST_CASE("MidiNote: equality and ordering", "[midi-note]") {
  REQUIRE(*MidiNote::try_from_int(60) == *MidiNote::try_from_int(60));
  REQUIRE(*MidiNote::try_from_int(60) != *MidiNote::try_from_int(61));
  REQUIRE(*MidiNote::try_from_int(60) < *MidiNote::try_from_int(61));
}

TEST_CASE("MidiNote: well-known constants", "[midi-note]") {
  REQUIRE(vp330::kMidiC4.value() == 60);
  REQUIRE(vp330::kMidiA4.value() == 69);
}
