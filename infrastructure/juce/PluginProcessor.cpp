#include "PluginProcessor.h"

#include "PluginEditor.h"

#include "vp330/note/MidiNote.h"
#include "vp330/section/ChoirSwitch.h"
#include "vp330/values/Hertz.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cstddef>
#include <memory>
#include <vector>

VP330Processor::VP330Processor()
    : juce::AudioProcessor(
          BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "VP330State", createParameterLayout()) {
  p_choir_lm8_ = apvts.getRawParameterValue(vp330::params::kChoirLowerMale8);
  p_choir_lm4_ = apvts.getRawParameterValue(vp330::params::kChoirLowerMale4);
  p_choir_um8_ = apvts.getRawParameterValue(vp330::params::kChoirUpperMale8);
  p_choir_uf4_ = apvts.getRawParameterValue(vp330::params::kChoirUpperFemale4);
  p_ensemble_ = apvts.getRawParameterValue(vp330::params::kEnsembleEnabled);
  p_vib_rate_ = apvts.getRawParameterValue(vp330::params::kVibratoRate);
  p_vib_depth_ = apvts.getRawParameterValue(vp330::params::kVibratoDepth);
  p_attack_ = apvts.getRawParameterValue(vp330::params::kAttack);
  p_release_ = apvts.getRawParameterValue(vp330::params::kRelease);
  p_level_ = apvts.getRawParameterValue(vp330::params::kOutputLevel);
}

juce::AudioProcessorEditor* VP330Processor::createEditor() {
  return new VP330Editor(*this, apvts);
}

juce::AudioProcessorValueTreeState::ParameterLayout VP330Processor::createParameterLayout() {
  using BoolParam = juce::AudioParameterBool;
  using FloatParam = juce::AudioParameterFloat;
  using FloatRange = juce::NormalisableRange<float>;

  std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
  p.push_back(std::make_unique<BoolParam>(vp330::params::kChoirLowerMale8, "Lower Male 8'", false));
  p.push_back(std::make_unique<BoolParam>(vp330::params::kChoirLowerMale4, "Lower Male 4'", false));
  p.push_back(std::make_unique<BoolParam>(vp330::params::kChoirUpperMale8, "Upper Male 8'", true));
  p.push_back(
      std::make_unique<BoolParam>(vp330::params::kChoirUpperFemale4, "Upper Female 4'", false));
  p.push_back(std::make_unique<BoolParam>(vp330::params::kEnsembleEnabled, "Ensemble", true));
  p.push_back(std::make_unique<FloatParam>(vp330::params::kVibratoRate, "Vibrato Rate",
                                           FloatRange{1.0f, 8.0f, 0.01f}, 3.0f));
  p.push_back(std::make_unique<FloatParam>(vp330::params::kVibratoDepth, "Vibrato Depth",
                                           FloatRange{0.0f, 1.0f, 0.001f}, 0.0f));
  p.push_back(std::make_unique<FloatParam>(vp330::params::kAttack, "Attack",
                                           FloatRange{0.005f, 2.0f, 0.001f}, 0.01f));
  p.push_back(std::make_unique<FloatParam>(vp330::params::kRelease, "Release",
                                           FloatRange{0.050f, 4.0f, 0.001f}, 0.50f));
  p.push_back(std::make_unique<FloatParam>(vp330::params::kOutputLevel, "Output Level",
                                           FloatRange{0.0f, 1.0f, 0.001f}, 0.8f));
  return {p.begin(), p.end()};
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

  // Sync APVTS params to engine at the start of each block
  engine_->set_choir_switch(vp330::ChoirSwitch::LowerMale8, *p_choir_lm8_ > 0.5f);
  engine_->set_choir_switch(vp330::ChoirSwitch::LowerMale4, *p_choir_lm4_ > 0.5f);
  engine_->set_choir_switch(vp330::ChoirSwitch::UpperMale8, *p_choir_um8_ > 0.5f);
  engine_->set_choir_switch(vp330::ChoirSwitch::UpperFemale4, *p_choir_uf4_ > 0.5f);
  engine_->set_ensemble_enabled(*p_ensemble_ > 0.5f);
  engine_->set_vibrato_rate(vp330::Hertz{*p_vib_rate_});
  engine_->set_vibrato_depth(*p_vib_depth_);
  engine_->set_attack_seconds(static_cast<double>(*p_attack_));
  engine_->set_release_seconds(static_cast<double>(*p_release_));

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
    } else if (msg.isController()) {
      const int cc = msg.getControllerNumber();
      const float val = static_cast<float>(msg.getControllerValue()) / 127.0f;
      if (cc == 1) // Mod wheel -> vibrato depth
        apvts.getParameter(vp330::params::kVibratoDepth)->setValueNotifyingHost(val);
      else if (cc == 7) // Volume -> output level
        apvts.getParameter(vp330::params::kOutputLevel)->setValueNotifyingHost(val);
      else if (cc == 80) // GP Button 1 -> ensemble on/off
        apvts.getParameter(vp330::params::kEnsembleEnabled)
            ->setValueNotifyingHost(val > 0.5f ? 1.0f : 0.0f);
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

  buffer.applyGain(*p_level_);
}

bool VP330Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto& out = layouts.getMainOutputChannelSet();
  return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VP330Processor::getStateInformation(juce::MemoryBlock& dest) {
  const auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, dest);
}

void VP330Processor::setStateInformation(const void* data, int size) {
  std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
  if (xml && xml->hasTagName(apvts.state.getType()))
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new VP330Processor();
}
