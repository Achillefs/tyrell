#pragma once

#include "vp330/note/MidiNote.h"
#include "vp330/values/Hertz.h"

#include <cmath>
#include <compare>

namespace vp330 {

class Pitch {
public:
  static constexpr Pitch from_midi_note(MidiNote n) {
    return Pitch{static_cast<double>(n.value())};
  }

  static Pitch from_hertz(Hertz f, Hertz a4 = Hertz{440.0}) {
    return Pitch{69.0 + 12.0 * std::log2(f.value() / a4.value())};
  }

  constexpr explicit Pitch(double semitones) : semitones_{semitones} {}

  constexpr double semitones() const { return semitones_; }

  Hertz to_hertz(Hertz a4 = Hertz{440.0}) const {
    return a4 * std::pow(2.0, (semitones_ - 69.0) / 12.0);
  }

  constexpr bool operator==(const Pitch&) const = default;
  constexpr auto operator<=>(const Pitch&) const = default;

private:
  double semitones_;
};

} // namespace vp330
