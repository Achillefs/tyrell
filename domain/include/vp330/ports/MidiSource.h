#pragma once

#include "vp330/engine/MidiEvent.h"

namespace vp330 {

class MidiSource {
public:
  virtual ~MidiSource() = default;
  MidiSource(const MidiSource&) = delete;
  MidiSource& operator=(const MidiSource&) = delete;
  MidiSource(MidiSource&&) = delete;
  MidiSource& operator=(MidiSource&&) = delete;

  // Pulls the next event for the current block. Returns false when no further
  // events fit in the current block; the caller advances the block boundary
  // before calling again. The returned event's sample_offset is in the
  // current-block local frame.
  virtual bool next(MidiEvent& out) = 0;
};

} // namespace vp330
