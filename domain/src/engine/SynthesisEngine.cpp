#include "vp330/engine/SynthesisEngine.h"

#include "vp330/values/Pitch.h"

#include <cstddef>

namespace vp330 {

namespace {
constexpr float kPerVoiceGain = 0.05f; // Phase 1 placeholder; Phase 3 replaces with section gain.
}

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
  const double sr = static_cast<double>(sample_rate_);
  for (std::size_t i = 0; i < frames; ++i) {
    float sum = 0.0f;
    for (int n = 0; n < 128; ++n) {
      const auto idx = static_cast<std::size_t>(n);
      if (!active_[idx]) continue;
      const auto note = MidiNote{n};
      const double freq = Pitch::from_midi_note(note).to_hertz().value();
      const double inc = freq / sr;
      auto& p = phase_[idx];
      sum += (p < 0.5 ? 1.0f : -1.0f);
      p += inc;
      if (p >= 1.0) p -= 1.0;
    }
    const float out = sum * kPerVoiceGain;
    left[i] = out;
    right[i] = out;
  }
}

} // namespace vp330
