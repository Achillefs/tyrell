#pragma once

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class VP330LookAndFeel : public juce::LookAndFeel_V4 {
public:
  VP330LookAndFeel();
  void drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn, bool isHighlighted,
                        bool isDown) override;
};

class VP330Editor : public juce::AudioProcessorEditor {
public:
  VP330Editor(VP330Processor& p, juce::AudioProcessorValueTreeState& apvts);
  ~VP330Editor() override;

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  VP330LookAndFeel laf_;

  juce::ToggleButton lm8_btn_{"LOWER 8'"};
  juce::ToggleButton lm4_btn_{"LOWER 4'"};
  juce::ToggleButton um8_btn_{"UPPER 8'"};
  juce::ToggleButton uf4_btn_{"UPPER 4'"};
  juce::ToggleButton ensemble_btn_{"ENS"};

  juce::Slider vib_rate_knob_;
  juce::Slider vib_depth_knob_;
  juce::Slider attack_knob_;
  juce::Slider release_knob_;
  juce::Slider level_knob_;

  using BtnAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using SldAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

  BtnAtt lm8_att_, lm4_att_, um8_att_, uf4_att_, ensemble_att_;
  SldAtt vib_rate_att_, vib_depth_att_, attack_att_, release_att_, level_att_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VP330Editor)
};
