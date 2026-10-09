#include "PluginProcessor.h"

#include "PluginEditor.h"

LufsMeterAudioProcessor::LufsMeterAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

LufsMeterAudioProcessor::~LufsMeterAudioProcessor() = default;

void LufsMeterAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
}

void LufsMeterAudioProcessor::releaseResources()
{
}

bool LufsMeterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    // Metering is pass-through, so the input and output layouts must match.
    if (mainIn != mainOut)
        return false;

    if (mainOut.isDisabled())
        return false;

    return true;
}

void LufsMeterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // The plugin is a meter and must not alter the incoming audio signal.
    juce::ignoreUnused (buffer);
}

juce::AudioProcessorEditor* LufsMeterAudioProcessor::createEditor()
{
    return new LufsMeterAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LufsMeterAudioProcessor();
}
