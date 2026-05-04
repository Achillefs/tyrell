#pragma once

#include "vp330/note/MidiNote.h"

namespace vp330 {

struct MidiEvent {
  enum class Kind { NoteOn, NoteOff };

  Kind kind;
  MidiNote note;
  int velocity;
  // Populated by adapters (CLI scheduler, future MidiSource implementations).
  // Not yet consumed by the Phase 1 engine, which has a direct note_on/note_off
  // API and renders whole blocks. Phase 2 wires it through MidiSource for
  // sample-accurate event timing.
  int sample_offset;
};

} // namespace vp330
