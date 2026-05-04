#include "PluginProcessor.h"

/**
 * @brief Constructs the VP330Processor and configures its audio buses.
 *
 * Initializes the AudioProcessor with a single enabled stereo output bus named "Output".
 */
VP330Processor::VP330Processor()
    : juce::AudioProcessor(
          BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
}

/**
 * @brief Prepare the processor for playback. Currently no resources or state are initialized.
 *
 * Called when the host prepares the audio processor for playback so it can allocate or reset
 * any resources dependent on the sample rate or block size.
 *
 * @param sampleRate The audio sample rate in Hz.
 * @param samplesPerBlockExpected The expected maximum number of samples per audio block.
 */
void VP330Processor::prepareToPlay(double, int) {
}
/**
 * @brief Releases any resources held by the audio processor.
 *
 * @note Currently a no-op; the processor does not allocate resources requiring explicit release.
 */
void VP330Processor::releaseResources() {
}

/**
 * @brief Clears the provided audio buffer for the current processing block.
 *
 * This processor removes all samples from the buffer and does not produce or modify audio beyond
 * clearing.
 *
 * @param buffer Audio buffer for the current block; its contents are cleared.
 * @param midiBuffer Unused MIDI messages for the current block (ignored).
 */
void VP330Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
  juce::ScopedNoDenormals no_denormals;
  buffer.clear();
}

/**
 * @brief Checks whether the processor supports the given buses layout.
 *
 * Examines the layout's main output channel set and determines if it is acceptable.
 *
 * @param layouts The buses layout to validate; the function checks its main output channel set.
 * @return `true` if the main output channel set is mono or stereo, `false` otherwise.
 */
bool VP330Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto& out = layouts.getMainOutputChannelSet();
  return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

/**
 * @brief Creates and returns a new VP330Processor audio plugin instance.
 *
 * The caller receives ownership of the returned pointer and is responsible for its lifetime
 * management.
 *
 * @return juce::AudioProcessor* Pointer to a newly-allocated VP330Processor instance.
 */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new VP330Processor();
}
