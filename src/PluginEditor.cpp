#include "PluginEditor.h"

#include "PluginProcessor.h"

LufsMeterAudioProcessorEditor::LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p)
{
    setSize (360, 120);
}

LufsMeterAudioProcessorEditor::~LufsMeterAudioProcessorEditor() = default;

void LufsMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawFittedText ("LUFS Meter", getLocalBounds(), juce::Justification::centred, 1);
}

void LufsMeterAudioProcessorEditor::resized()
{
}
