#include "vp330/engine/MidiEvent.h"

#include <catch2/catch_test_macros.hpp>

using vp330::MidiEvent;
using vp330::MidiNote;

TEST_CASE("MidiEvent: aggregate-initialisable", "[midi-event]") {
  const auto note = *MidiNote::try_from_int(60);
  MidiEvent ev{MidiEvent::Kind::NoteOn, note, 100, 0};
  REQUIRE(ev.kind == MidiEvent::Kind::NoteOn);
  REQUIRE(ev.note == note);
  REQUIRE(ev.velocity == 100);
  REQUIRE(ev.sample_offset == 0);
}

TEST_CASE("MidiEvent: NoteOff carries 0 velocity convention", "[midi-event]") {
  const auto note = *MidiNote::try_from_int(60);
  MidiEvent ev{MidiEvent::Kind::NoteOff, note, 0, 128};
  REQUIRE(ev.kind == MidiEvent::Kind::NoteOff);
  REQUIRE(ev.sample_offset == 128);
}
