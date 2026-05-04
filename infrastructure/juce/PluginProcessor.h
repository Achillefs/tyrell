#pragma once

#include "vp330/engine/SynthesisEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

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
  std::unique_ptr<vp330::SynthesisEngine> engine_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VP330Processor)
};
