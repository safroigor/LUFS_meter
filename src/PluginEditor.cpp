#include "PluginEditor.h"

#include "AboutContent.h"
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

    infoButton_.onClick = [this] { showAbout(); };
    addAndMakeVisible (infoButton_);

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
        switch (processorRef.getSettings().displayMode)
        {
            case MeterDisplayMode::vertical:
                limits->setSizeLimits (190, 300, 4000, 4000);
                break;

            case MeterDisplayMode::compact:
                limits->setSizeLimits (190, 170, 4000, 4000);
                break;

            case MeterDisplayMode::horizontal:
            default:
                limits->setSizeLimits (360, 96 + MeterComponent::secondaryRowHeight, 4000, 4000);
                break;
        }
    }
}

juce::Point<int> LufsMeterAudioProcessorEditor::getPreferredSize (const MeterSettings& s) const
{
    switch (s.displayMode)
    {
        case MeterDisplayMode::vertical:   return juce::Point<int> (224, 360);
        case MeterDisplayMode::compact:    return juce::Point<int> (220, 190);

        case MeterDisplayMode::horizontal:
        default:                           return juce::Point<int> (430, 104 + MeterComponent::secondaryRowHeight);
    }
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

    if (processorRef.getSettings().displayMode == MeterDisplayMode::compact)
    {
        // Compact: Info top-left; Reset and Settings top-right, preserving the
        // usual left-to-right order (Reset, Settings).
        infoButton_.setBounds (controls.removeFromLeft (buttonSize)
                                   .withSizeKeepingCentre (buttonSize, buttonSize));
        settingsButton_.setBounds (controls.removeFromRight (buttonSize)
                                       .withSizeKeepingCentre (buttonSize, buttonSize));
        controls.removeFromRight (buttonGap);
        resetButton_.setBounds (controls.removeFromRight (buttonSize)
                                    .withSizeKeepingCentre (buttonSize, buttonSize));
    }
    else
    {
        infoButton_.setBounds (controls.removeFromRight (buttonSize)
                                   .withSizeKeepingCentre (buttonSize, buttonSize));
        controls.removeFromRight (buttonGap);
        settingsButton_.setBounds (controls.removeFromRight (buttonSize)
                                       .withSizeKeepingCentre (buttonSize, buttonSize));
        controls.removeFromRight (buttonGap);
        resetButton_.setBounds (controls.removeFromRight (buttonSize)
                                    .withSizeKeepingCentre (buttonSize, buttonSize));
    }

    meter_.setBounds (inner);
}

void LufsMeterAudioProcessorEditor::applyPopupSettings (const MeterSettings& s)
{
    const auto previous = processorRef.getSettings();

    processorRef.setSettings (s);
    meter_.setSettings (processorRef.getSettings());

    // Only resize when the geometry changed; zone/colour edits must not move
    // the window while the user drags a slider.
    if (s.displayMode != previous.displayMode)
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

void LufsMeterAudioProcessorEditor::showAbout()
{
    auto content = std::make_unique<AboutContent>();
    content->setLookAndFeel (&MeterTheme::lookAndFeel());

    juce::CallOutBox::launchAsynchronously (std::move (content),
                                            infoButton_.getScreenBounds(),
                                            nullptr);
}
