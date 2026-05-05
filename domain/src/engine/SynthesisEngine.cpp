#include "vp330/engine/SynthesisEngine.h"

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate)
    : sample_rate_{sample_rate},
      keyboard_{sample_rate},
      choir_{sample_rate},
      vibrato_{sample_rate} {
  choir_.set_switch(ChoirSwitch::UpperMale8, true); // default: upper male 8' on
}

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

void SynthesisEngine::set_choir_switch(ChoirSwitch sw, bool on) { choir_.set_switch(sw, on); }
void SynthesisEngine::set_vibrato_rate(Hertz r)    { vibrato_.set_rate(r); }
void SynthesisEngine::set_vibrato_depth(float d)   { vibrato_.set_depth(d); }
void SynthesisEngine::set_attack_seconds(double s)  { keyboard_.set_attack_seconds(s); }
void SynthesisEngine::set_release_seconds(double s) { keyboard_.set_release_seconds(s); }

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  if (lower_8_.size() < frames) lower_8_.resize(frames);
  if (lower_4_.size() < frames) lower_4_.resize(frames);
  if (upper_8_.size() < frames) upper_8_.resize(frames);
  if (upper_4_.size() < frames) upper_4_.resize(frames);

  keyboard_.set_master_clock_hz(vibrato_.tick(frames));
  keyboard_.render_zones(lower_8_.data(), lower_4_.data(),
                         upper_8_.data(), upper_4_.data(), frames);
  choir_.process(lower_8_.data(), lower_4_.data(),
                 upper_8_.data(), upper_4_.data(), left, right, frames);
}

} // namespace vp330
