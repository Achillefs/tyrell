#pragma once

#include "vp330/keyboard/MkIIKeyboard.h"
#include "vp330/modulation/Vibrato.h"
#include "vp330/note/MidiNote.h"
#include "vp330/section/ChoirSection.h"
#include "vp330/section/ChoirSwitch.h"
#include "vp330/values/Hertz.h"

#include <array>
#include <cstddef>
#include <vector>

namespace vp330 {

class SynthesisEngine {
public:
  explicit SynthesisEngine(int sample_rate);

  int sample_rate() const { return sample_rate_; }

  void note_on(MidiNote note);
  void note_off(MidiNote note);
  bool is_note_active(MidiNote note) const;

  void set_choir_switch(ChoirSwitch sw, bool on);
  void set_vibrato_rate(Hertz rate);
  void set_vibrato_depth(float depth_0_to_1);
  void set_attack_seconds(double seconds);
  void set_release_seconds(double seconds);

  void render(float* left, float* right, std::size_t frames);

private:
  int sample_rate_;
  MkIIKeyboard keyboard_;
  ChoirSection choir_;
  Vibrato vibrato_;
  std::vector<float> lower_8_, lower_4_, upper_8_, upper_4_;
  // Tracks logical hold state — distinct from KeyGate's audible state, which
  // can still be Releasing after note_off.
  std::array<bool, 128> note_held_{};
};

} // namespace vp330
