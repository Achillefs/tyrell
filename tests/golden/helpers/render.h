#pragma once

#include "wav_io.h"

#include <string>

/**
 * Render a WAV from a MIDI fixture using the vp330_render CLI.
 *
 * @param fixture_filename Path or identifier of the MIDI fixture to render.
 * @param sample_rate Target sample rate for the produced WAV, in Hz.
 * @param duration_seconds Duration of the render, in seconds.
 * @returns The loaded Wav produced by rendering the fixture.
 * @throws std::runtime_error if the CLI invocation fails or the produced WAV cannot be loaded.
 */
namespace vp330::test {

// Calls the vp330_render CLI to produce a WAV from a fixture MIDI.
// Returns the loaded WAV. Throws on failure.
Wav render_fixture(const std::string& fixture_filename, int sample_rate, double duration_seconds);

} // namespace vp330::test
