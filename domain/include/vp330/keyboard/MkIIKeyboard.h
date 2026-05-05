#pragma once

#include "vp330/keygate/KeyGate.h"
#include "vp330/note/MidiNote.h"
#include "vp330/tod/OctaveDivider.h"
#include "vp330/tod/TopOctaveDivider.h"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace vp330 {

class MkIIKeyboard {
public:
  explicit MkIIKeyboard(int sample_rate);

  void note_on(MidiNote note);
  void note_off(MidiNote note);

  // Render `frames` mono samples summed across every open KeyGate.
  // Caller duplicates to stereo. `out` is overwritten.
  void render(float* out, std::size_t frames);

  // Split render: four buffers for lower/upper × 8′/4′ pitch.
  // 8′ = natural pitch (octave_down stages); 4′ = one octave up (octave_down−1).
  void render_zones(float* lower_8, float* lower_4,
                    float* upper_8, float* upper_4, std::size_t frames);

  void set_master_clock_hz(Hertz hz);
  void set_attack_seconds(double seconds);
  void set_release_seconds(double seconds);

private:
  struct KeyTopology {
    int pitch_class;
    int octave_down;
  };
  std::optional<KeyTopology> topology_for(MidiNote note) const;

  int sample_rate_;
  TopOctaveDivider tod_;
  // One OctaveDivider per pitch class (C..B), fed by the corresponding TOD
  // output. Each chain produces five descending octaves through the
  // flip-flop ÷2 stages.
  std::array<OctaveDivider, 12> octave_dividers_;
  // 49 KeyGates indexed by (note - kKeyboardLowestNote).
  std::vector<KeyGate> keygates_;
};

} // namespace vp330
