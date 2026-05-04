#include "SmfReader.h"

#include "vp330/engine/MidiEvent.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using vp330::MidiEvent;
using vp330::cli::ParsedMidi;
using vp330::cli::read_smf;

namespace {

std::filesystem::path write_temp_midi(const std::vector<std::uint8_t>& bytes,
                                      const std::string& tag) {
  auto p = std::filesystem::temp_directory_path() / ("vp330_smf_" + tag + ".mid");
  std::ofstream f(p, std::ios::binary);
  f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  return p;
}

// Format 0, PPQN 480, 120 BPM, NoteOn C4 vel=100 at t=0, NoteOff C4 at +960 ticks (1.0 s).
std::vector<std::uint8_t> simple_c4_smf() {
  return {
      'M',  'T',  'h',  'd', 0,   0,    0,    6,    0,    0,    0,    1,    0x01, 0xE0,
      'M',  'T',  'r',  'k', 0,   0,    0,    20,   0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1,
      0x20, 0x00, 0x90, 60,  100, 0x87, 0x40, 0x80, 60,   0,    0x00, 0xFF, 0x2F, 0x00,
  };
}

// Same as simple_c4_smf but the NoteOff is encoded as NoteOn vel=0 (running status).
std::vector<std::uint8_t> running_status_c4_smf() {
  return {
      'M',  'T',  'h',  'd', 0,   0,    0,    6,  0,    0,    0,    1,    0x01, 0xE0,
      'M',  'T',  'r',  'k', 0,   0,    0,    19, 0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1,
      0x20, 0x00, 0x90, 60,  100, 0x87, 0x40, 60, 0,    0x00, 0xFF, 0x2F, 0x00,
  };
}

} // namespace

TEST_CASE("SmfReader: parses simple format-0 NoteOn/NoteOff pair", "[smf]") {
  const auto path = write_temp_midi(simple_c4_smf(), "simple");
  const auto parsed = read_smf(path.string());

  REQUIRE(parsed.ppqn == 480);
  REQUIRE(parsed.events.size() == 2);

  REQUIRE(parsed.events[0].event.kind == MidiEvent::Kind::NoteOn);
  REQUIRE(parsed.events[0].event.note.value() == 60);
  REQUIRE(parsed.events[0].event.velocity == 100);
  REQUIRE(parsed.events[0].time_seconds == Catch::Approx(0.0).margin(1e-9));

  REQUIRE(parsed.events[1].event.kind == MidiEvent::Kind::NoteOff);
  REQUIRE(parsed.events[1].event.note.value() == 60);
  REQUIRE(parsed.events[1].time_seconds == Catch::Approx(1.0).margin(1e-9));
}

TEST_CASE("SmfReader: handles running status and NoteOn vel=0 → NoteOff", "[smf]") {
  const auto path = write_temp_midi(running_status_c4_smf(), "running");
  const auto parsed = read_smf(path.string());

  REQUIRE(parsed.events.size() == 2);
  REQUIRE(parsed.events[1].event.kind == MidiEvent::Kind::NoteOff);
  REQUIRE(parsed.events[1].time_seconds == Catch::Approx(1.0).margin(1e-9));
}

TEST_CASE("SmfReader: rejects missing MThd", "[smf]") {
  const std::vector<std::uint8_t> bad = {'X', 'Y', 'Z', 'Z'};
  const auto path = write_temp_midi(bad, "bad");
  REQUIRE_THROWS_AS(read_smf(path.string()), std::runtime_error);
}
