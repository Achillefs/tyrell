#pragma once

#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/note/MidiNote.h"

#include <array>
#include <cstddef>

namespace vp330 {

class SynthesisEngine {
public:
  explicit SynthesisEngine(int sample_rate);

  int sample_rate() const { return sample_rate_; }

  void note_on(MidiNote note);
  void note_off(MidiNote note);

  bool is_note_active(MidiNote note) const;

  void render(float* left, float* right, std::size_t frames);

private:
  int sample_rate_;
  MkIIKeyboard keyboard_;
  // Tracks logical hold state — distinct from KeyGate's audible state, which
  // can still be Releasing after note_off.
  std::array<bool, 128> note_held_{};
};

} // namespace vp330
