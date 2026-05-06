#pragma once

#include "parameters/ParameterIds.h"
#include "vp330/engine/SynthesisEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
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

  void getStateInformation(juce::MemoryBlock& dest) override;
  void setStateInformation(const void* data, int size) override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  juce::AudioProcessorValueTreeState apvts;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  std::unique_ptr<vp330::SynthesisEngine> engine_;

  std::atomic<float>* p_choir_lm8_{};
  std::atomic<float>* p_choir_lm4_{};
  std::atomic<float>* p_choir_um8_{};
  std::atomic<float>* p_choir_uf4_{};
  std::atomic<float>* p_ensemble_{};
  std::atomic<float>* p_vib_rate_{};
  std::atomic<float>* p_vib_depth_{};
  std::atomic<float>* p_attack_{};
  std::atomic<float>* p_release_{};
  std::atomic<float>* p_level_{};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VP330Processor)
};
