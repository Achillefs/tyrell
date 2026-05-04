#pragma once

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
  std::array<bool, 128> active_{};
  std::array<double, 128> phase_{};
};

} // namespace vp330
