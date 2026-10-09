#include "PluginEditor.h"

#include "MeterTheme.h"
#include "PluginProcessor.h"
#include "SettingsContent.h"

LufsMeterAudioProcessorEditor::LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      meter_ (p.getMeasurementEngine())
{
    setLookAndFeel (&MeterTheme::lookAndFeel());

    setResizable (true, true);
    updateSizeLimits();

    addAndMakeVisible (meter_);

    resetButton_.onClick = [this]
    {
        processorRef.resetMeasurement();
        meter_.repaint();
    };
    addAndMakeVisible (resetButton_);

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
    setLookAndFeel (nullptr);
}

void LufsMeterAudioProcessorEditor::updateSizeLimits()
{
    if (auto* limits = getConstrainer())
    {
        if (processorRef.getSettings().orientation == MeterOrientation::horizontal)
            limits->setSizeLimits (210, 96, 4000, 4000);
        else
            limits->setSizeLimits (150, 240, 4000, 4000);
    }
}

juce::Point<int> LufsMeterAudioProcessorEditor::getPreferredSize (const MeterSettings& s) const
{
    if (s.orientation == MeterOrientation::horizontal)
        return s.size == MeterSizeMode::compact ? juce::Point<int> (430, 104)
                                                : juce::Point<int> (400, 82);

    return s.size == MeterSizeMode::compact ? juce::Point<int> (188, 360)
                                            : juce::Point<int> (166, 312);
}

void LufsMeterAudioProcessorEditor::timerCallback()
{
    meter_.repaint();
}

void LufsMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (MeterTheme::windowBackground);
}

void LufsMeterAudioProcessorEditor::resized()
{
    auto inner = getLocalBounds().reduced (outerPadding);

    // One consistent control strip in the upper-right, shared by both
    // orientations, so the buttons can never overlap the scale or the reading.
    auto controls = inner.removeFromTop (controlStripHeight);
    settingsButton_.setBounds (controls.removeFromRight (buttonSize)
                                   .withSizeKeepingCentre (buttonSize, buttonSize));
    controls.removeFromRight (buttonGap);
    resetButton_.setBounds (controls.removeFromRight (buttonSize)
                                .withSizeKeepingCentre (buttonSize, buttonSize));

    meter_.setBounds (inner);
}

void LufsMeterAudioProcessorEditor::applyPopupSettings (const MeterSettings& s)
{
    const auto previous = processorRef.getSettings();

    processorRef.setSettings (s);
    meter_.setSettings (processorRef.getSettings());

    // Only resize when the geometry changed; zone/colour edits must not move
    // the window while the user drags a slider.
    if (s.orientation != previous.orientation || s.size != previous.size)
    {
        updateSizeLimits();
        const auto preferred = getPreferredSize (processorRef.getSettings());
        setSize (preferred.x, preferred.y);
    }

    repaint();
}

void LufsMeterAudioProcessorEditor::popupReset()
{
    processorRef.resetMeasurement();
    meter_.repaint();
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

    content->setLookAndFeel (&MeterTheme::lookAndFeel());

    juce::CallOutBox::launchAsynchronously (std::move (content),
                                            settingsButton_.getScreenBounds(),
                                            nullptr);
}
