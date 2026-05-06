#include "vp330/keyboard/MkIIKeyboard.h"

#include "vp330/note/MidiNote.h"
#include "vp330/tod/MkIIConstants.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::MidiNote;
using vp330::MkIIKeyboard;

namespace {

double rms(const std::vector<float>& buf) {
  double a = 0.0;
  for (auto s : buf)
    a += static_cast<double>(s) * s;
  return std::sqrt(a / static_cast<double>(buf.size()));
}

} // namespace

TEST_CASE("MkIIKeyboard: silent with no keys held", "[keyboard]") {
  MkIIKeyboard kb{48000};
  std::vector<float> out(1024, 0.5f);
  kb.render(out.data(), out.size());
  for (auto s : out)
    REQUIRE(s == 0.0f);
}

TEST_CASE("MkIIKeyboard: notes outside the 49-key range are ignored", "[keyboard]") {
  MkIIKeyboard kb{48000};
  const auto below = MidiNote{vp330::mkii::kKeyboardLowestNote.value() - 1};
  const auto above = MidiNote{vp330::mkii::kKeyboardLowestNote.value() + vp330::mkii::kKeyCount};
  kb.note_on(below);
  kb.note_on(above);
  std::vector<float> out(1024);
  kb.render(out.data(), out.size());
  for (auto s : out)
    REQUIRE(s == 0.0f);
}

TEST_CASE("MkIIKeyboard: lowest key produces audible output", "[keyboard]") {
  MkIIKeyboard kb{48000};
  kb.note_on(vp330::mkii::kKeyboardLowestNote);

  // Skip 10 ms attack window.
  std::vector<float> warm(480), warm_out(480);
  kb.render(warm_out.data(), warm_out.size());

  std::vector<float> out(48000);
  kb.render(out.data(), out.size());
  REQUIRE(rms(out) > 0.03);
}

TEST_CASE("MkIIKeyboard: paraphonic — two keys produce a louder sum", "[keyboard]") {
  MkIIKeyboard kb_one{48000};
  kb_one.note_on(vp330::mkii::kKeyboardLowestNote);
  std::vector<float> warm1(480), warm1_out(480);
  kb_one.render(warm1_out.data(), warm1_out.size());
  std::vector<float> one(48000);
  kb_one.render(one.data(), one.size());

  MkIIKeyboard kb_two{48000};
  kb_two.note_on(vp330::mkii::kKeyboardLowestNote);
  kb_two.note_on(MidiNote{vp330::mkii::kKeyboardLowestNote.value() + 7}); // a fifth
  std::vector<float> warm2(480), warm2_out(480);
  kb_two.render(warm2_out.data(), warm2_out.size());
  std::vector<float> two(48000);
  kb_two.render(two.data(), two.size());

  REQUIRE(rms(two) > rms(one));
}
