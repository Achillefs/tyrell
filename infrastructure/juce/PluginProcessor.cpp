#include "PluginProcessor.h"

#include "vp330/note/MidiNote.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>
#include <vector>

VP330Processor::VP330Processor()
    : juce::AudioProcessor(
          BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
}

void VP330Processor::prepareToPlay(double sample_rate, int /*samples_per_block*/) {
  engine_ = std::make_unique<vp330::SynthesisEngine>(static_cast<int>(sample_rate));
}

void VP330Processor::releaseResources() {
  engine_.reset();
}

void VP330Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals no_denormals;

  if (!engine_) {
    buffer.clear();
    return;
  }

  for (const auto meta : midi) {
    const auto& msg = meta.getMessage();
    if (msg.isNoteOn()) {
      const auto note = vp330::MidiNote::try_from_int(msg.getNoteNumber());
      if (note) {
        engine_->note_on(*note);
      }
    } else if (msg.isNoteOff()) {
      const auto note = vp330::MidiNote::try_from_int(msg.getNoteNumber());
      if (note) {
        engine_->note_off(*note);
      }
    }
  }

  const auto num_samples = static_cast<std::size_t>(buffer.getNumSamples());
  auto* left = buffer.getWritePointer(0);
  if (buffer.getNumChannels() >= 2) {
    auto* right = buffer.getWritePointer(1);
    engine_->render(left, right, num_samples);
  } else {
    std::vector<float> right(num_samples, 0.0f);
    engine_->render(left, right.data(), num_samples);
  }
}

bool VP330Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto& out = layouts.getMainOutputChannelSet();
  return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new VP330Processor();
}
