#include "vp330/engine/SynthesisEngine.h"

#include <vector>

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate)
    : sample_rate_{sample_rate}, keyboard_{sample_rate} {}

void SynthesisEngine::note_on(MidiNote note) {
  keyboard_.note_on(note);
  note_held_[static_cast<std::size_t>(note.value())] = true;
}

void SynthesisEngine::note_off(MidiNote note) {
  keyboard_.note_off(note);
  note_held_[static_cast<std::size_t>(note.value())] = false;
}

bool SynthesisEngine::is_note_active(MidiNote note) const {
  return note_held_[static_cast<std::size_t>(note.value())];
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  std::vector<float> mono(frames);
  keyboard_.render(mono.data(), frames);
  for (std::size_t i = 0; i < frames; ++i) {
    left[i] = mono[i];
    right[i] = mono[i];
  }
}

} // namespace vp330
