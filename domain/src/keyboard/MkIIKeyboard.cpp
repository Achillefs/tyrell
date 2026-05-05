#include "vp330/keyboard/MkIIKeyboard.h"

#include "vp330/tod/MkIIConstants.h"

#include <algorithm>
#include <vector>

namespace vp330 {

namespace {

std::array<OctaveDivider, 12> make_octave_dividers(const TopOctaveDivider& tod, int sr) {
  return {
      OctaveDivider{tod.pitch_class_frequency(0), sr},
      OctaveDivider{tod.pitch_class_frequency(1), sr},
      OctaveDivider{tod.pitch_class_frequency(2), sr},
      OctaveDivider{tod.pitch_class_frequency(3), sr},
      OctaveDivider{tod.pitch_class_frequency(4), sr},
      OctaveDivider{tod.pitch_class_frequency(5), sr},
      OctaveDivider{tod.pitch_class_frequency(6), sr},
      OctaveDivider{tod.pitch_class_frequency(7), sr},
      OctaveDivider{tod.pitch_class_frequency(8), sr},
      OctaveDivider{tod.pitch_class_frequency(9), sr},
      OctaveDivider{tod.pitch_class_frequency(10), sr},
      OctaveDivider{tod.pitch_class_frequency(11), sr},
  };
}

constexpr float kPerKeyGain = 0.05f;
constexpr double kAttackSeconds = 0.005;
constexpr double kReleaseSeconds = 0.05;

} // namespace

MkIIKeyboard::MkIIKeyboard(int sample_rate)
    : sample_rate_{sample_rate},
      tod_{mkii::kMasterClockHz, mkii::kDividerRatios, sample_rate},
      octave_dividers_{make_octave_dividers(tod_, sample_rate)} {
  keygates_.reserve(mkii::kKeyCount);
  for (int i = 0; i < mkii::kKeyCount; ++i)
    keygates_.emplace_back(sample_rate, kAttackSeconds, kReleaseSeconds);
}

std::optional<MkIIKeyboard::KeyTopology>
MkIIKeyboard::topology_for(MidiNote note) const {
  const int n = note.value() - mkii::kKeyboardLowestNote.value();
  if (n < 0 || n >= mkii::kKeyCount) return std::nullopt;
  const int absolute_midi = note.value();
  // The TOD produces the C8..B8 octave (one octave above the highest playable
  // key C6). For each key, octave_down is the count of ÷2 stages from C8 down
  // to that key's octave. MIDI octave numbering: 60 = C4 → octave 4.
  const int octave = absolute_midi / 12 - 1;
  const int octave_down = 8 - octave;
  const int pitch_class = absolute_midi % 12;
  return KeyTopology{pitch_class, octave_down};
}

void MkIIKeyboard::note_on(MidiNote note) {
  const auto t = topology_for(note);
  if (!t) return;
  const int gate_idx = note.value() - mkii::kKeyboardLowestNote.value();
  keygates_[gate_idx].gate_on();
}

void MkIIKeyboard::note_off(MidiNote note) {
  const auto t = topology_for(note);
  if (!t) return;
  const int gate_idx = note.value() - mkii::kKeyboardLowestNote.value();
  keygates_[gate_idx].gate_off();
}

void MkIIKeyboard::set_master_clock_hz(Hertz hz) {
  tod_.set_master_clock_hz(hz);
  for (int p = 0; p < 12; ++p)
    octave_dividers_[p].set_input_frequency(tod_.pitch_class_frequency(p));
}

void MkIIKeyboard::set_attack_seconds(double s) {
  for (auto& g : keygates_)
    g.set_attack_seconds(s);
}

void MkIIKeyboard::set_release_seconds(double s) {
  for (auto& g : keygates_)
    g.set_release_seconds(s);
}

void MkIIKeyboard::render_zones(float* lower, float* upper, std::size_t frames) {
  std::fill_n(lower, frames, 0.f);
  std::fill_n(upper, frames, 0.f);
  std::vector<float> scratch(frames), gated(frames);

  for (int i = 0; i < mkii::kKeyCount; ++i) {
    if (keygates_[i].state() == KeyGate::State::Idle) continue;
    const auto note = MidiNote{mkii::kKeyboardLowestNote.value() + i};
    const auto t = topology_for(note);
    octave_dividers_[t->pitch_class].render(t->octave_down, scratch.data(), frames);
    keygates_[i].process(scratch.data(), gated.data(), frames);
    const int midi = mkii::kKeyboardLowestNote.value() + i;
    float* zone = (midi < mkii::kSplitNote.value()) ? lower : upper;
    for (std::size_t f = 0; f < frames; ++f)
      zone[f] += gated[f] * kPerKeyGain;
  }
}

void MkIIKeyboard::render(float* out, std::size_t frames) {
  std::vector<float> lower(frames), upper(frames);
  render_zones(lower.data(), upper.data(), frames);
  for (std::size_t i = 0; i < frames; ++i)
    out[i] = lower[i] + upper[i];
}

} // namespace vp330
