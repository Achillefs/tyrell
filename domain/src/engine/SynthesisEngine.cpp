#include "vp330/engine/SynthesisEngine.h"

#include <algorithm>

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate) : sample_rate_{sample_rate} {
}

void SynthesisEngine::note_on(MidiNote note) {
  const auto idx = static_cast<std::size_t>(note.value());
  active_[idx] = true;
  phase_[idx] = 0.0;
}

void SynthesisEngine::note_off(MidiNote note) {
  active_[static_cast<std::size_t>(note.value())] = false;
}

bool SynthesisEngine::is_note_active(MidiNote note) const {
  return active_[static_cast<std::size_t>(note.value())];
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  // Task 8 fills this in. For now write silence so the state-machine tests
  // can verify is_note_active without depending on audio output.
  std::fill_n(left, frames, 0.0f);
  std::fill_n(right, frames, 0.0f);
}

} // namespace vp330
