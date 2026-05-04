#pragma once

#include "vp330/engine/MidiEvent.h"

#include <string>
#include <vector>

namespace vp330::cli {

struct TimedMidiEvent {
  double time_seconds;
  vp330::MidiEvent event;
};

struct ParsedMidi {
  int ppqn = 0;
  std::vector<TimedMidiEvent> events;
};

ParsedMidi read_smf(const std::string& path);

} // namespace vp330::cli
