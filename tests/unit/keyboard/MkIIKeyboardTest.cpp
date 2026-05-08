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

TEST_CASE("MkIIKeyboard: default onset attack takes >= 50ms", "[keyboard][L1]") {
  // kAttackSeconds must be ~0.05s: the 0-50ms RMS must be below sustain (attack
  // still rising) but above 40% of sustain (attack completes within 50ms, not
  // later). This pins both the floor (>= 50ms) and ceiling (< instant).
  const int sr = 48000;
  const int window = sr / 20; // 50ms

  MkIIKeyboard early{sr};
  early.note_on(vp330::mkii::kKeyboardLowestNote);
  std::vector<float> early_buf(window);
  early.render(early_buf.data(), early_buf.size()); // 0-50ms

  MkIIKeyboard late{sr};
  late.note_on(vp330::mkii::kKeyboardLowestNote);
  std::vector<float> skip_buf(sr / 5);
  late.render(skip_buf.data(), skip_buf.size()); // skip 200ms
  std::vector<float> late_buf(window);
  late.render(late_buf.data(), late_buf.size()); // 200-250ms (sustain)

  const double r_late = rms(late_buf);
  REQUIRE(rms(early_buf) > r_late * 0.40); // attack settles within ~50ms
  REQUIRE(rms(early_buf) < r_late * 0.80); // attack is not instantaneous
}

TEST_CASE("MkIIKeyboard: lowest key produces audible output", "[keyboard]") {
  MkIIKeyboard kb{48000};
  kb.note_on(vp330::mkii::kKeyboardLowestNote);

  // Skip 100 ms default attack window.
  std::vector<float> warm_out(4800);
  kb.render(warm_out.data(), warm_out.size());

  std::vector<float> out(48000);
  kb.render(out.data(), out.size());
  REQUIRE(rms(out) > 0.03);
}

TEST_CASE("MkIIKeyboard: paraphonic — two keys produce a louder sum", "[keyboard]") {
  MkIIKeyboard kb_one{48000};
  kb_one.note_on(vp330::mkii::kKeyboardLowestNote);
  std::vector<float> warm1_out(4800);
  kb_one.render(warm1_out.data(), warm1_out.size()); // skip 100ms attack
  std::vector<float> one(48000);
  kb_one.render(one.data(), one.size());

  MkIIKeyboard kb_two{48000};
  kb_two.note_on(vp330::mkii::kKeyboardLowestNote);
  kb_two.note_on(MidiNote{vp330::mkii::kKeyboardLowestNote.value() + 7}); // a fifth
  std::vector<float> warm2_out(4800);
  kb_two.render(warm2_out.data(), warm2_out.size()); // skip 100ms attack
  std::vector<float> two(48000);
  kb_two.render(two.data(), two.size());

  REQUIRE(rms(two) > rms(one));
}
