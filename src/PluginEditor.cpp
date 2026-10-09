#include "PluginEditor.h"

#include "PluginProcessor.h"

namespace
{
    const juce::Colour backgroundColour = juce::Colour (0xff141414);
    const juce::Colour panelColour      = juce::Colour (0xff1c1c1c);
    const juce::Colour dividerColour    = juce::Colour (0xff2c2c2c);
    const juce::Colour dimColour        = juce::Colour (0xff8f8f8f);
}

LufsMeterAudioProcessorEditor::LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      meter_ (p.getMeasurementEngine())
{
    setResizable (false, false);

    addAndMakeVisible (meter_);

    gearButton_.setButtonText (juce::String::fromUTF8 ("\xE2\x9A\x99"));
    gearButton_.onClick = [this] { toggleSettings(); };
    addAndMakeVisible (gearButton_);

    orientationHorizontal_.setClickingTogglesState (true);
    orientationVertical_.setClickingTogglesState (true);
    orientationHorizontal_.setRadioGroupId (1);
    orientationVertical_.setRadioGroupId (1);
    orientationHorizontal_.onClick = [this] { applySettings(); };
    orientationVertical_.onClick = [this] { applySettings(); };
    addAndMakeVisible (orientationHorizontal_);
    addAndMakeVisible (orientationVertical_);

    sizeCombo_.addItem ("Slim", 1);
    sizeCombo_.addItem ("Compact", 2);
    sizeCombo_.onChange = [this] { applySettings(); };
    addAndMakeVisible (sizeCombo_);

    showLabelToggle_.onClick = [this] { applySettings(); };
    showScaleToggle_.onClick = [this] { applySettings(); };
    addAndMakeVisible (showLabelToggle_);
    addAndMakeVisible (showScaleToggle_);

    resetButton_.onClick = [this] { processorRef.resetMeasurement(); };
    addAndMakeVisible (resetButton_);

    refreshControlsFromSettings();
    meter_.setSettings (processorRef.getSettings());
    setSize (getMeasuredSize().x + gearStripWidth,
             getMeasuredSize().y + (settingsVisible_ ? settingsPanelHeight : 0));

    startTimerHz (30);
}

LufsMeterAudioProcessorEditor::~LufsMeterAudioProcessorEditor()
{
    stopTimer();
}

juce::Point<int> LufsMeterAudioProcessorEditor::getMeasuredSize() const
{
    const auto s = processorRef.getSettings();

    if (s.orientation == MeterOrientation::horizontal)
        return s.size == MeterSizeMode::compact ? juce::Point<int> (360, 64)
                                                : juce::Point<int> (360, 44);

    return s.size == MeterSizeMode::compact ? juce::Point<int> (76, 300)
                                            : juce::Point<int> (60, 250);
}

void LufsMeterAudioProcessorEditor::timerCallback()
{
    meter_.repaint();
}

void LufsMeterAudioProcessorEditor::toggleSettings()
{
    settingsVisible_ = ! settingsVisible_;
    setSize (getMeasuredSize().x + gearStripWidth,
             getMeasuredSize().y + (settingsVisible_ ? settingsPanelHeight : 0));
}

void LufsMeterAudioProcessorEditor::refreshControlsFromSettings()
{
    const auto s = processorRef.getSettings();

    updatingControls_ = true;
    orientationHorizontal_.setToggleState (s.orientation == MeterOrientation::horizontal,
                                           juce::dontSendNotification);
    orientationVertical_.setToggleState (s.orientation == MeterOrientation::vertical,
                                         juce::dontSendNotification);
    sizeCombo_.setSelectedId (s.size == MeterSizeMode::slim ? 1 : 2, juce::dontSendNotification);
    showLabelToggle_.setToggleState (s.showLabel, juce::dontSendNotification);
    showScaleToggle_.setToggleState (s.showScale, juce::dontSendNotification);
    updatingControls_ = false;
}

void LufsMeterAudioProcessorEditor::applySettings()
{
    if (updatingControls_)
        return;

    auto s = processorRef.getSettings();
    s.orientation = orientationVertical_.getToggleState() ? MeterOrientation::vertical
                                                          : MeterOrientation::horizontal;
    s.size = sizeCombo_.getSelectedId() == 1 ? MeterSizeMode::slim : MeterSizeMode::compact;
    s.showLabel = showLabelToggle_.getToggleState();
    s.showScale = showScaleToggle_.getToggleState();

    processorRef.setSettings (s);
    meter_.setSettings (s);

    setSize (getMeasuredSize().x + gearStripWidth,
             getMeasuredSize().y + (settingsVisible_ ? settingsPanelHeight : 0));
}

void LufsMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    if (! settingsVisible_)
        return;

    auto panel = getLocalBounds().removeFromBottom (settingsPanelHeight);

    g.setColour (panelColour);
    g.fillRect (panel);
    g.setColour (dividerColour);
    g.fillRect (panel.getX(), panel.getY(), panel.getWidth(), 1);

    auto inner = panel.reduced (10);
    g.setColour (dimColour);
    g.setFont (12.0f);

    auto r1 = inner.removeFromTop (26);
    g.drawText ("Orientation", r1, juce::Justification::centredLeft, false);

    inner.removeFromTop (4);
    auto r2 = inner.removeFromTop (26);
    g.drawText ("Size", r2, juce::Justification::centredLeft, false);
}

void LufsMeterAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    juce::Rectangle<int> panel;

    if (settingsVisible_)
        panel = area.removeFromBottom (settingsPanelHeight);

    auto gearArea = area.removeFromRight (gearStripWidth);
    meter_.setBounds (area);
    gearButton_.setBounds (gearArea.removeFromTop (26).reduced (3));

    if (! settingsVisible_)
        return;

    auto inner = panel.reduced (10);

    auto r1 = inner.removeFromTop (26);
    orientationHorizontal_.setBounds (r1.removeFromRight (94).withSizeKeepingCentre (94, 22));
    orientationVertical_.setBounds (r1.removeFromRight (84).withSizeKeepingCentre (84, 22));

    inner.removeFromTop (4);
    auto r2 = inner.removeFromTop (26);
    sizeCombo_.setBounds (r2.removeFromRight (170).withSizeKeepingCentre (170, 22));

    auto r3 = inner.removeFromTop (26);
    showLabelToggle_.setBounds (r3.removeFromLeft (170));

    auto r4 = inner.removeFromTop (26);
    showScaleToggle_.setBounds (r4.removeFromLeft (170));

    auto r5 = inner.removeFromTop (30);
    resetButton_.setBounds (r5.removeFromLeft (94).withSizeKeepingCentre (94, 24));
}
