#include "PluginEditor.h"

#include "PluginProcessor.h"

LufsMeterAudioProcessorEditor::LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p)
{
    resetButton_.onClick = [this] { processorRef.resetMeasurement(); };
    addAndMakeVisible (resetButton_);

    setSize (360, 120);
    startTimerHz (30);
}

LufsMeterAudioProcessorEditor::~LufsMeterAudioProcessorEditor()
{
    stopTimer();
}

void LufsMeterAudioProcessorEditor::timerCallback()
{
    repaint();
}

juce::String LufsMeterAudioProcessorEditor::getDisplayValue() const
{
    const auto& engine = processorRef.getMeasurementEngine();

    if (! engine.hasValue())
        return juce::String::fromUTF8 ("\u2014"); // em dash: no trustworthy value

    return juce::String (engine.getIntegratedLufs(), 1);
}

void LufsMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));

    auto area = getLocalBounds().reduced (12);
    area.removeFromBottom (34);

    g.setColour (juce::Colours::white);
    g.setFont (34.0f);
    g.drawFittedText (getDisplayValue(), area, juce::Justification::centred, 1);

    g.setColour (juce::Colours::grey);
    g.setFont (13.0f);
    g.drawText ("Integrated  LUFS", getLocalBounds().reduced (12).removeFromBottom (30),
                juce::Justification::centredLeft);
}

void LufsMeterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    resetButton_.setBounds (area.removeFromBottom (28).removeFromRight (90));
}
