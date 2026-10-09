#include "PluginEditor.h"

#include "PluginProcessor.h"
#include "SettingsContent.h"

namespace
{
    const juce::Colour backgroundColour = juce::Colour (0xff1a1a1a);
}

LufsMeterAudioProcessorEditor::LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      meter_ (p.getMeasurementEngine())
{
    setResizable (true, true);
    if (auto* limits = getConstrainer())
        limits->setSizeLimits (170, 90, 4000, 4000);

    addAndMakeVisible (meter_);

    settingsButton_.setButtonText (juce::String::fromUTF8 ("\xE2\x9A\x99"));
    settingsButton_.onClick = [this] { showSettings(); };
    addAndMakeVisible (settingsButton_);

    meter_.setSettings (processorRef.getSettings());
    const auto preferred = getPreferredSize (processorRef.getSettings());
    setSize (preferred.x, preferred.y);

    startTimerHz (30);
}

LufsMeterAudioProcessorEditor::~LufsMeterAudioProcessorEditor()
{
    stopTimer();
}

juce::Point<int> LufsMeterAudioProcessorEditor::getPreferredSize (const MeterSettings& s) const
{
    if (s.orientation == MeterOrientation::horizontal)
        return s.size == MeterSizeMode::compact ? juce::Point<int> (410, 78)
                                                : juce::Point<int> (410, 58);

    return s.size == MeterSizeMode::compact ? juce::Point<int> (126, 330)
                                            : juce::Point<int> (100, 290);
}

void LufsMeterAudioProcessorEditor::timerCallback()
{
    meter_.repaint();
}

void LufsMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);
}

void LufsMeterAudioProcessorEditor::resized()
{
    auto inner = getLocalBounds().reduced (outerPadding);

    const auto orientation = processorRef.getSettings().orientation;

    if (orientation == MeterOrientation::horizontal)
    {
        auto buttonArea = inner.removeFromRight (buttonSize);
        settingsButton_.setBounds (buttonArea.withSizeKeepingCentre (buttonSize, buttonSize));
    }
    else
    {
        auto buttonRow = inner.removeFromTop (buttonSize);
        settingsButton_.setBounds (buttonRow.removeFromRight (buttonSize));
    }

    meter_.setBounds (inner);
}

void LufsMeterAudioProcessorEditor::applyPopupSettings (const MeterSettings& s)
{
    processorRef.setSettings (s);
    meter_.setSettings (s);
    const auto preferred = getPreferredSize (s);
    setSize (preferred.x, preferred.y);
    repaint();
}

void LufsMeterAudioProcessorEditor::popupReset()
{
    processorRef.resetMeasurement();
}

void LufsMeterAudioProcessorEditor::showSettings()
{
    auto safe = juce::Component::SafePointer<LufsMeterAudioProcessorEditor> (this);

    auto content = std::make_unique<SettingsContent> (
        processorRef.getSettings(),
        [safe] (MeterSettings s) mutable
        {
            if (auto* editor = safe.getComponent())
                editor->applyPopupSettings (s);
        },
        [safe]
        {
            if (auto* editor = safe.getComponent())
                editor->popupReset();
        });

    juce::CallOutBox::launchAsynchronously (std::move (content),
                                            settingsButton_.getScreenBounds(),
                                            nullptr);
}
