#include "vp330/engine/SynthesisEngine.h"

namespace vp330 {

SynthesisEngine::SynthesisEngine(int sample_rate)
    : sample_rate_{sample_rate}, keyboard_{sample_rate}, choir_{sample_rate},
      choir_compander_{sample_rate}, ensemble_{sample_rate}, vibrato_{sample_rate} {
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

void SynthesisEngine::set_choir_switch(ChoirSwitch sw, bool on) {
  choir_.set_switch(sw, on);
}
void SynthesisEngine::set_vibrato_rate(Hertz r) {
  vibrato_.set_rate(r);
}
void SynthesisEngine::set_vibrato_depth(float d) {
  vibrato_.set_depth(d);
}
void SynthesisEngine::set_attack_seconds(double s) {
  keyboard_.set_attack_seconds(s);
}
void SynthesisEngine::set_release_seconds(double s) {
  keyboard_.set_release_seconds(s);
}
void SynthesisEngine::set_ensemble_enabled(bool on) {
  ensemble_.set_enabled(on);
}
void SynthesisEngine::set_ensemble_rate(Hertz rate) {
  ensemble_.set_rate(rate);
}
void SynthesisEngine::set_ensemble_depth(float d) {
  ensemble_.set_depth(d);
}

void SynthesisEngine::render(float* left, float* right, std::size_t frames) {
  auto ensure = [frames](std::vector<float>& v) {
    if (v.size() < frames) v.resize(frames);
  };
  ensure(lower_8_);
  ensure(lower_4_);
  ensure(upper_8_);
  ensure(upper_4_);
  ensure(choir_mono_);
  ensure(choir_scratch_);

  keyboard_.set_master_clock_hz(vibrato_.tick(frames));
  keyboard_.render_zones(lower_8_.data(), lower_4_.data(), upper_8_.data(), upper_4_.data(),
                         frames);
  // ChoirSection produces mono; pass choir_mono_ as left, choir_scratch_ as unused right.
  choir_.process(lower_8_.data(), lower_4_.data(), upper_8_.data(), upper_4_.data(),
                 choir_mono_.data(), choir_scratch_.data(), frames);
  choir_compander_.process(choir_mono_.data(), choir_mono_.data(), frames);
  ensemble_.process(choir_mono_.data(), left, right, frames);
}

} // namespace vp330
