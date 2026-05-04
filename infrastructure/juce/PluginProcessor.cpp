#include "PluginProcessor.h"

VP330Processor::VP330Processor()
    : juce::AudioProcessor(BusesProperties()
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void VP330Processor::prepareToPlay(double, int) {}
void VP330Processor::releaseResources() {}

void VP330Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals no_denormals;
    buffer.clear();
}

bool VP330Processor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VP330Processor();
}
