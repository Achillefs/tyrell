#include "vp330/values/Pitch.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <type_traits>

using Catch::Approx;
using vp330::Hertz;
using vp330::MidiNote;
using vp330::Pitch;

TEST_CASE("Pitch::from_midi_note carries the integer semitone", "[pitch]") {
  REQUIRE(Pitch::from_midi_note(*MidiNote::try_from_int(60)).semitones() == 60.0);
  REQUIRE(Pitch::from_midi_note(*MidiNote::try_from_int(69)).semitones() == 69.0);
}

TEST_CASE("Pitch::to_hertz: A4 round-trip", "[pitch]") {
  const auto a4 = Pitch::from_midi_note(*MidiNote::try_from_int(69)).to_hertz();
  REQUIRE(a4.value() == Approx(440.0).margin(1e-9));
}

TEST_CASE("Pitch::to_hertz: octaves halve / double", "[pitch]") {
  const auto a3 = Pitch::from_midi_note(*MidiNote::try_from_int(57)).to_hertz();
  const auto a5 = Pitch::from_midi_note(*MidiNote::try_from_int(81)).to_hertz();
  REQUIRE(a3.value() == Approx(220.0).margin(1e-9));
  REQUIRE(a5.value() == Approx(880.0).margin(1e-9));
}

TEST_CASE("Pitch::to_hertz: middle C (C4)", "[pitch]") {
  const auto c4 = Pitch::from_midi_note(vp330::kMidiC4).to_hertz();
  // 12-TET C4 with A4=440: 440 * 2^((60-69)/12) ≈ 261.6255653...
  REQUIRE(c4.value() == Approx(261.6255653005986).margin(1e-9));
}

TEST_CASE("Pitch::from_hertz inverts to_hertz", "[pitch]") {
  const auto p = Pitch::from_hertz(Hertz{261.6255653005986});
  REQUIRE(p.semitones() == Approx(60.0).margin(1e-9));
}

TEST_CASE("Pitch: equality and ordering", "[pitch]") {
  REQUIRE(Pitch{60.0} == Pitch{60.0});
  REQUIRE(Pitch{60.0} < Pitch{60.5});
}

TEST_CASE("Pitch: forbids implicit conversion to double", "[pitch]") {
  STATIC_REQUIRE(!std::is_convertible_v<Pitch, double>);
  STATIC_REQUIRE(!std::is_convertible_v<double, Pitch>);
}
