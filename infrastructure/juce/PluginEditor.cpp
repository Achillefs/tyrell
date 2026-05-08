#include "PluginEditor.h"

#include "parameters/ParameterIds.h"

namespace {
constexpr juce::uint32 kBg = 0xff1a1a1a;
constexpr juce::uint32 kHeader = 0xff0f0f0f;
constexpr juce::uint32 kAmber = 0xffff8c00;
constexpr juce::uint32 kDivider = 0xff2d2d2d;
constexpr juce::uint32 kLabel = 0xff888888;
constexpr juce::uint32 kBtnOff = 0xff2a2a2a;
constexpr juce::uint32 kBtnBdr = 0xff444444;
constexpr int kW = 700, kH = 200, kHdr = 28;
} // namespace

VP330LookAndFeel::VP330LookAndFeel() {
  setColour(juce::Slider::thumbColourId, juce::Colour(kAmber));
  setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(kAmber));
  setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333333));
  setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffcccccc));
  setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(kBg));
  setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void VP330LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
                                        bool /*isHighlighted*/, bool /*isDown*/) {
  const auto r = btn.getLocalBounds().toFloat().reduced(2.0f);
  g.setColour(btn.getToggleState() ? juce::Colour(kAmber) : juce::Colour(kBtnOff));
  g.fillRoundedRectangle(r, 4.0f);
  g.setColour(juce::Colour(kBtnBdr));
  g.drawRoundedRectangle(r, 4.0f, 1.0f);
  g.setColour(btn.getToggleState() ? juce::Colour(kBg) : juce::Colour(0xffcccccc));
  g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f).withStyle("Bold")));
  g.drawText(btn.getButtonText(), r.toNearestInt(), juce::Justification::centred);
}

VP330Editor::VP330Editor(VP330Processor& p, juce::AudioProcessorValueTreeState& apvts)
    : juce::AudioProcessorEditor(p), lm8_att_(apvts, vp330::params::kChoirLowerMale8, lm8_btn_),
      lm4_att_(apvts, vp330::params::kChoirLowerMale4, lm4_btn_),
      um8_att_(apvts, vp330::params::kChoirUpperMale8, um8_btn_),
      uf4_att_(apvts, vp330::params::kChoirUpperFemale4, uf4_btn_),
      ensemble_att_(apvts, vp330::params::kEnsembleEnabled, ensemble_btn_),
      vib_rate_att_(apvts, vp330::params::kVibratoRate, vib_rate_knob_),
      vib_depth_att_(apvts, vp330::params::kVibratoDepth, vib_depth_knob_),
      attack_att_(apvts, vp330::params::kAttack, attack_knob_),
      release_att_(apvts, vp330::params::kRelease, release_knob_),
      level_att_(apvts, vp330::params::kOutputLevel, level_knob_) {
  setLookAndFeel(&laf_);
  setSize(kW, kH);

  for (auto* s : {&vib_rate_knob_, &vib_depth_knob_, &attack_knob_, &release_knob_, &level_knob_}) {
    s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(s);
  }
  lm8_btn_.setButtonText("LM8");
  lm4_btn_.setButtonText("LM4");
  um8_btn_.setButtonText("UM8");
  uf4_btn_.setButtonText("UF4");
  ensemble_btn_.setButtonText("ENS");
  for (auto* b : {&lm8_btn_, &lm4_btn_, &um8_btn_, &uf4_btn_, &ensemble_btn_}) {
    addAndMakeVisible(b);
  }
}

VP330Editor::~VP330Editor() {
  setLookAndFeel(nullptr);
}

void VP330Editor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(kBg));

  g.setColour(juce::Colour(kHeader));
  g.fillRect(0, 0, kW, kHdr);
  g.setColour(juce::Colour(kAmber));
  g.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
  g.drawText("VP-330 MkII", 12, 0, 220, kHdr, juce::Justification::centredLeft);
  g.setColour(juce::Colour(0xff555555));
  g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
  g.drawText("CHOIR", kW - 70, 0, 65, kHdr, juce::Justification::centredRight);

  g.setColour(juce::Colour(kDivider));
  for (int x : {210, 315, 492, 652}) {
    g.drawLine(static_cast<float>(x), static_cast<float>(kHdr), static_cast<float>(x),
               static_cast<float>(kH), 1.0f);
  }

  g.setColour(juce::Colour(kLabel));
  g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f).withStyle("Bold")));
  g.drawText("CHOIR VOICE", 5, kHdr + 4, 200, 13, juce::Justification::centred);
  g.drawText("ENSEMBLE", 215, kHdr + 4, 95, 13, juce::Justification::centred);
  g.drawText("VIBRATO", 320, kHdr + 4, 167, 13, juce::Justification::centred);
  g.drawText("ENVELOPE", 497, kHdr + 4, 150, 13, juce::Justification::centred);
  g.drawText("LEVEL", 657, kHdr + 4, 38, 13, juce::Justification::centred);

  g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
  g.drawText("RATE", 322, 138, 70, 12, juce::Justification::centred);
  g.drawText("DEPTH", 402, 138, 70, 12, juce::Justification::centred);
  g.drawText("ATK", 497, 133, 65, 12, juce::Justification::centred);
  g.drawText("REL", 572, 133, 65, 12, juce::Justification::centred);
  g.drawText("LVL", 655, 133, 42, 12, juce::Justification::centred);
}

void VP330Editor::resized() {
  lm8_btn_.setBounds(8, 55, 96, 34);
  lm4_btn_.setBounds(109, 55, 96, 34);
  um8_btn_.setBounds(8, 95, 96, 34);
  uf4_btn_.setBounds(109, 95, 96, 34);

  ensemble_btn_.setBounds(222, 62, 78, 52);

  vib_rate_knob_.setBounds(322, 50, 72, 82);
  vib_depth_knob_.setBounds(402, 50, 72, 82);

  attack_knob_.setBounds(497, 50, 67, 78);
  release_knob_.setBounds(572, 50, 67, 78);

  level_knob_.setBounds(655, 50, 40, 78);
}
