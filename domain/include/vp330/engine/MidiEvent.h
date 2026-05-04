#pragma once

#include "vp330/note/MidiNote.h"

namespace vp330 {

struct MidiEvent {
  enum class Kind { NoteOn, NoteOff };

  Kind kind;
  MidiNote note;
  int velocity;
  int sample_offset;
};

} // namespace vp330
