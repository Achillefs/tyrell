#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/note/MidiNote.h"
#include "vp330/tod/MkIIConstants.h"
#include "vp330/values/Pitch.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using vp330::MidiNote;
using vp330::MkIIKeyboard;

namespace {

constexpr int kSampleRate = 48000;

double measure_frequency_hz(MkIIKeyboard& kb, MidiNote n) {
  kb.note_on(n);
  // Skip 10 ms attack window.
  std::vector<float> warm(kSampleRate / 100), warm_buf(warm.size());
  kb.render(warm_buf.data(), warm_buf.size());
  std::vector<float> buf(kSampleRate);
  kb.render(buf.data(), buf.size());
  kb.note_off(n);

  // Measure frequency from the time delta between first and last sign change.
  // Counting crossings and dividing by 2 only resolves to ±0.5 Hz in a 1-second
  // window — at 65 Hz that's ±13 ¢, far too coarse for an ET test. Using the
  // interval between bracketing crossings gives precision ≈ freq / interval,
  // which is sub-cent across the keyboard's range.
  std::size_t first_xing = 0;
  std::size_t last_xing = 0;
  std::size_t crossings = 0;
  for (std::size_t i = 1; i < buf.size(); ++i) {
    if ((buf[i - 1] >= 0.0f) != (buf[i] >= 0.0f)) {
      if (crossings == 0) first_xing = i;
      last_xing = i;
      ++crossings;
    }
  }
  if (crossings < 2) return 0.0;
  const double interval_samples = static_cast<double>(last_xing - first_xing);
  const double half_cycles = static_cast<double>(crossings - 1);
  return (half_cycles * kSampleRate) / (2.0 * interval_samples);
}

double cents_deviation(double measured_hz, double et_hz) {
  return 1200.0 * std::log2(measured_hz / et_hz);
}

double expected_tod_frequency(MidiNote n) {
  const int absolute_midi = n.value();
  const int pitch_class = absolute_midi % 12;
  const int octave = absolute_midi / 12 - 1;
  const int octave_down = 8 - octave;
  const double tod_freq = vp330::mkii::kMasterClockHz.value() /
                          static_cast<double>(vp330::mkii::kDividerRatios[pitch_class]);
  return tod_freq / static_cast<double>(1 << octave_down);
}

} // namespace

TEST_CASE("MkIIKeyboard L2: every key matches master/(N*2^k) within 0.1 cents", "[keyboard][L2]") {
  // Tight tolerance — verifies the keyboard topology and TOD/divider chain
  // produce exactly the integer-divider frequency for every key. Any
  // mis-mapping (wrong pitch_class, wrong octave_down) would surface as a
  // multi-cent deviation here.
  for (int i = 0; i < vp330::mkii::kKeyCount; ++i) {
    MkIIKeyboard kb{kSampleRate};
    const auto note = MidiNote{vp330::mkii::kKeyboardLowestNote.value() + i};
    const double measured = measure_frequency_hz(kb, note);
    const double expected = expected_tod_frequency(note);
    const double cents = cents_deviation(measured, expected);
    INFO("key " << note.value() << " measured " << measured << " expected " << expected);
    REQUIRE(std::abs(cents) < 0.1);
  }
}

TEST_CASE("MkIIKeyboard L2: every key tunes within ±1.5 cents of equal temperament",
          "[keyboard][L2]") {
  // The integer-divider design has intrinsic deviations from ET. The
  // canonical AY-3-0214 series with 2.000272 MHz master clock keeps every
  // pitch within ±1.5 ¢ of ET — see docs/tod-intrinsic-detuning.md for the
  // full per-pitch-class table. This test guards the envelope.
  for (int i = 0; i < vp330::mkii::kKeyCount; ++i) {
    MkIIKeyboard kb{kSampleRate};
    const auto note = MidiNote{vp330::mkii::kKeyboardLowestNote.value() + i};
    const double measured = measure_frequency_hz(kb, note);
    const double et = vp330::Pitch::from_midi_note(note).to_hertz().value();
    const double cents = cents_deviation(measured, et);
    INFO("key " << note.value() << " measured " << measured << " ET " << et << " cents " << cents);
    REQUIRE(std::abs(cents) < 1.5);
  }
}
