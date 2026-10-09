#include "SettingsContent.h"

namespace
{
    constexpr int padding = 16;
    constexpr int rowHeight = 26;
    constexpr int rowGap = 10;
    constexpr int labelWidth = 92;
}

SettingsContent::SettingsContent (MeterSettings initial,
                                  std::function<void (MeterSettings)> onSettingsChanged,
                                  std::function<void()> onReset)
    : settings_ (initial),
      onSettingsChanged_ (std::move (onSettingsChanged)),
      onReset_ (std::move (onReset))
{
    horizontalButton_.setClickingTogglesState (true);
    verticalButton_.setClickingTogglesState (true);
    horizontalButton_.setRadioGroupId (2);
    verticalButton_.setRadioGroupId (2);
    horizontalButton_.onClick = [this] { emitSettings(); };
    verticalButton_.onClick = [this] { emitSettings(); };
    addAndMakeVisible (horizontalButton_);
    addAndMakeVisible (verticalButton_);

    sizeCombo_.addItem ("Slim", 1);
    sizeCombo_.addItem ("Compact", 2);
    sizeCombo_.onChange = [this] { emitSettings(); };
    addAndMakeVisible (sizeCombo_);

    showLabelToggle_.onClick = [this] { emitSettings(); };
    showScaleToggle_.onClick = [this] { emitSettings(); };
    addAndMakeVisible (showLabelToggle_);
    addAndMakeVisible (showScaleToggle_);

    resetButton_.onClick = [this] { if (onReset_) onReset_(); };
    addAndMakeVisible (resetButton_);

    refreshControls();
    setSize (280, padding * 2 + rowHeight * 2 + rowGap * 3 + 34 + rowHeight);
}

void SettingsContent::refreshControls()
{
    horizontalButton_.setToggleState (settings_.orientation == MeterOrientation::horizontal,
                                      juce::dontSendNotification);
    verticalButton_.setToggleState (settings_.orientation == MeterOrientation::vertical,
                                    juce::dontSendNotification);
    sizeCombo_.setSelectedId (settings_.size == MeterSizeMode::slim ? 1 : 2,
                              juce::dontSendNotification);
    showLabelToggle_.setToggleState (settings_.showLabel, juce::dontSendNotification);
    showScaleToggle_.setToggleState (settings_.showScale, juce::dontSendNotification);
}

void SettingsContent::emitSettings()
{
    settings_.orientation = verticalButton_.getToggleState() ? MeterOrientation::vertical
                                                             : MeterOrientation::horizontal;
    settings_.size = sizeCombo_.getSelectedId() == 1 ? MeterSizeMode::slim
                                                     : MeterSizeMode::compact;
    settings_.showLabel = showLabelToggle_.getToggleState();
    settings_.showScale = showScaleToggle_.getToggleState();

    if (onSettingsChanged_)
        onSettingsChanged_ (settings_);
}

void SettingsContent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff232323));

    g.setColour (juce::Colour (0xffffffff).withAlpha (0.92f));
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawText ("Settings", juce::Rectangle<int> (padding, padding, getWidth() - padding * 2, 20),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xff9a9a9a));
    g.setFont (12.5f);

    auto inner = getLocalBounds().reduced (padding);
    inner.removeFromTop (26);

    auto r1 = inner.removeFromTop (rowHeight);
    g.drawText ("Orientation", r1, juce::Justification::centredLeft, false);
    inner.removeFromTop (rowGap);

    auto r2 = inner.removeFromTop (rowHeight);
    g.drawText ("Size", r2, juce::Justification::centredLeft, false);
}

void SettingsContent::resized()
{
    auto inner = getLocalBounds().reduced (padding);
    inner.removeFromTop (26);

    auto r1 = inner.removeFromTop (rowHeight);
    r1.removeFromLeft (labelWidth);
    const int half = (r1.getWidth() - 6) / 2;
    horizontalButton_.setBounds (r1.removeFromLeft (half));
    r1.removeFromLeft (6);
    verticalButton_.setBounds (r1.removeFromLeft (half));
    inner.removeFromTop (rowGap);

    auto r2 = inner.removeFromTop (rowHeight);
    r2.removeFromLeft (labelWidth);
    sizeCombo_.setBounds (r2);

    inner.removeFromTop (rowGap + 4);

    showLabelToggle_.setBounds (inner.removeFromTop (rowHeight));
    showScaleToggle_.setBounds (inner.removeFromTop (rowHeight));
    inner.removeFromTop (rowGap);

    resetButton_.setBounds (inner.removeFromTop (34).removeFromLeft (110));
}
