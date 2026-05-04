#pragma once

#include "wav_io.h"

#include <string>

namespace vp330::test {

// Calls the vp330_render CLI to produce a WAV from a fixture MIDI.
// Returns the loaded WAV. Throws on failure.
Wav render_fixture(const std::string& fixture_filename, int sample_rate, double duration_seconds);

} // namespace vp330::test
