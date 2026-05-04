#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * VP330Processor constructor.
 */

/**
 * Prepare internal resources for processing at the given sample rate and block size.
 * @param sample_rate Target audio sample rate in Hz.
 * @param samples_per_block Expected maximum number of samples per audio block.
 */

/**
 * Release any resources allocated for audio processing.
 */

/**
 * Process an audio block and its incoming MIDI messages in-place.
 * @param buffer Audio buffer containing interleaved channel data to be processed.
 * @param midi MIDI messages received for the current block.
 */

/**
 * Return the editor component for this processor.
 * @returns `nullptr` (no editor is provided).
 */

/**
 * Query whether the processor provides a GUI editor.
 * @returns `false` (the processor has no editor).
 */

/**
 * Get the plugin name.
 * @returns The name "VP330".
 */

/**
 * Query whether this processor accepts MIDI input.
 * @returns `true` if MIDI input is accepted, `false` otherwise.
 */

/**
 * Query whether this processor produces MIDI output.
 * @returns `true` if MIDI output is produced, `false` otherwise.
 */

/**
 * Query whether this processor is a pure MIDI effect (no audio processing).
 * @returns `true` if the processor is a MIDI-only effect, `false` otherwise.
 */

/**
 * Get the processor's tail length in seconds (sustain/release time after input stops).
 * @returns Tail length in seconds.
 */

/**
 * Get the number of programs/presets exposed by the processor.
 * @returns The number of programs.
 */

/**
 * Get the index of the currently selected program.
 * @returns The current program index.
 */

/**
 * Set the current program by index.
 * @param index Program index to select.
 */

/**
 * Get the name of a program by index.
 * @param index Program index.
 * @returns Program name as a juce::String (may be empty).
 */

/**
 * Change the name of a program.
 * @param index Program index.
 * @param newName New program name.
 */

/**
 * Save the processor state into the provided memory block.
 * @param destData Memory block to receive serialized state (may remain empty).
 */

/**
 * Restore the processor state from the provided memory blob.
 * @param data Pointer to serialized state data.
 * @param sizeInBytes Size of the serialized state in bytes.
 */

/**
 * Determine whether the given bus/channel layout is supported by this processor.
 * @param layouts Buses layout to validate.
 * @returns `true` if the layout is supported, `false` otherwise.
 */
class VP330Processor : public juce::AudioProcessor {
public:
  VP330Processor();
  ~VP330Processor() override = default;

  void prepareToPlay(double sample_rate, int samples_per_block) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

  juce::AudioProcessorEditor* createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return "VP330"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock&) override {}
  void setStateInformation(const void*, int) override {}

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VP330Processor)
};
