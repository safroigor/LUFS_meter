#include "SettingsContent.h"

namespace
{
    constexpr int padding = 16;
    constexpr int rowHeight = 26;
    constexpr int toggleRowHeight = 28;
    constexpr int rowGap = 10;
    constexpr int labelWidth = 96;

    const juce::Colour backgroundColour = juce::Colour (0xff232323);
    const juce::Colour labelColour      = juce::Colour (0xff9a9a9a);
    const juce::Colour textColour       = juce::Colour (0xffe6e6e6);
    const juce::Colour boxColour        = juce::Colour (0xff2e2e2e);
    const juce::Colour boxBorderColour  = juce::Colour (0xff585858);
    const juce::Colour accentColour     = juce::Colour (0xff18c96a);
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

    resetButton_.onClick = [this] { if (onReset_) onReset_(); };
    addAndMakeVisible (resetButton_);

    refreshControls();
    setSize (280, padding * 2 + 26 + rowHeight + rowGap + rowHeight + rowGap * 2
                      + toggleRowHeight * 2 + 12 + 34);
}

void SettingsContent::refreshControls()
{
    horizontalButton_.setToggleState (settings_.orientation == MeterOrientation::horizontal,
                                      juce::dontSendNotification);
    verticalButton_.setToggleState (settings_.orientation == MeterOrientation::vertical,
                                    juce::dontSendNotification);
    sizeCombo_.setSelectedId (settings_.size == MeterSizeMode::slim ? 1 : 2,
                              juce::dontSendNotification);
}

void SettingsContent::emitSettings()
{
    settings_.orientation = verticalButton_.getToggleState() ? MeterOrientation::vertical
                                                             : MeterOrientation::horizontal;
    settings_.size = sizeCombo_.getSelectedId() == 1 ? MeterSizeMode::slim
                                                     : MeterSizeMode::compact;

    if (onSettingsChanged_)
        onSettingsChanged_ (settings_);

    repaint();
}

void SettingsContent::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (showLabelRect_.contains (pos))
    {
        settings_.showLabel = ! settings_.showLabel;
        emitSettings();
    }
    else if (showScaleRect_.contains (pos))
    {
        settings_.showScale = ! settings_.showScale;
        emitSettings();
    }
}

void SettingsContent::drawCheckboxRow (juce::Graphics& g, juce::Rectangle<int> row,
                                       const juce::String& label, bool on) const
{
    auto box = row.removeFromLeft (row.getHeight()).reduced (0, 3).toFloat();

    g.setColour (on ? accentColour : boxColour);
    g.fillRoundedRectangle (box, 4.0f);

    if (on)
    {
        juce::Path tick;
        tick.startNewSubPath (box.getX() + 4.0f, box.getCentreY());
        tick.lineTo (box.getCentreX() - 1.0f, box.getBottom() - 4.5f);
        tick.lineTo (box.getRight() - 4.0f, box.getY() + 4.5f);
        g.setColour (juce::Colours::white);
        g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
    else
    {
        g.setColour (boxBorderColour);
        g.drawRoundedRectangle (box.reduced (0.75f), 4.0f, 1.5f);
    }

    g.setColour (textColour);
    g.setFont (13.5f);
    g.drawText (label, row.withTrimmedLeft (8), juce::Justification::centredLeft, false);
}

void SettingsContent::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    g.setColour (juce::Colour (0xfff2f2f2));
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawText ("Settings",
                juce::Rectangle<int> (padding, padding, getWidth() - padding * 2, 20),
                juce::Justification::centredLeft, false);

    g.setColour (labelColour);
    g.setFont (12.5f);

    auto inner = getLocalBounds().reduced (padding);
    inner.removeFromTop (26);

    auto r1 = inner.removeFromTop (rowHeight);
    g.drawText ("Orientation", r1, juce::Justification::centredLeft, false);
    inner.removeFromTop (rowGap);

    auto r2 = inner.removeFromTop (rowHeight);
    g.drawText ("Size", r2, juce::Justification::centredLeft, false);
    inner.removeFromTop (rowGap * 2);

    drawCheckboxRow (g, inner.removeFromTop (toggleRowHeight), "Show label", settings_.showLabel);
    drawCheckboxRow (g, inner.removeFromTop (toggleRowHeight), "Show scale", settings_.showScale);
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
    inner.removeFromTop (rowGap * 2);

    showLabelRect_ = inner.removeFromTop (toggleRowHeight);
    showScaleRect_ = inner.removeFromTop (toggleRowHeight);
    inner.removeFromTop (12);

    resetButton_.setBounds (inner.removeFromTop (34).removeFromLeft (110));
}
