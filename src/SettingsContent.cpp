#include "SettingsContent.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include "MeterTheme.h"

namespace
{
    constexpr int padding = 18;
    constexpr int labelWidth = 122;
    constexpr int headingHeight = 20;
    constexpr int rowHeight = 30;
    constexpr int toggleHeight = 30;
    constexpr int sliderRowHeight = 34;
    constexpr int popupWidth = 372;

    /** ColourSelector that reports live changes through a std::function. */
    class ZoneColourSelector final : public juce::ColourSelector,
                                     private juce::ChangeListener
    {
    public:
        std::function<void (juce::Colour)> onColourChange;

        ZoneColourSelector()
            : juce::ColourSelector (juce::ColourSelector::showColourAtTop
                                    | juce::ColourSelector::editableColour
                                    | juce::ColourSelector::showSliders
                                    | juce::ColourSelector::showColourspace)
        {
            addChangeListener (this);
        }

        ~ZoneColourSelector() override
        {
            removeChangeListener (this);
        }

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override
        {
            if (onColourChange)
                onColourChange (getCurrentColour());
        }
    };

    /** Shared row layout so paint() and resized() can never drift apart. */
    struct Rows
    {
        juce::Rectangle<int> title;
        juce::Rectangle<int> display;
        juce::Rectangle<int> showLabel;
        juce::Rectangle<int> showScale;
        juce::Rectangle<int> zonesHeading;
        juce::Rectangle<int> greenLow;
        juce::Rectangle<int> greenHigh;
        juce::Rectangle<int> redThreshold;
        juce::Rectangle<int> coloursHeading;
        juce::Rectangle<int> defaultColour;
        juce::Rectangle<int> greenColour;
        juce::Rectangle<int> redColour;
        juce::Rectangle<int> reset;
    };

    Rows computeRows (juce::Rectangle<int> bounds)
    {
        Rows r;
        auto inner = bounds.reduced (padding);

        r.title = inner.removeFromTop (24);
        inner.removeFromTop (12);
        r.display = inner.removeFromTop (rowHeight);
        inner.removeFromTop (18);
        r.showLabel = inner.removeFromTop (toggleHeight);
        r.showScale = inner.removeFromTop (toggleHeight);
        inner.removeFromTop (12);
        r.zonesHeading = inner.removeFromTop (headingHeight);
        inner.removeFromTop (8);
        r.greenLow = inner.removeFromTop (sliderRowHeight);
        inner.removeFromTop (8);
        r.greenHigh = inner.removeFromTop (sliderRowHeight);
        inner.removeFromTop (8);
        r.redThreshold = inner.removeFromTop (sliderRowHeight);
        inner.removeFromTop (16);
        r.coloursHeading = inner.removeFromTop (headingHeight);
        inner.removeFromTop (8);
        r.defaultColour = inner.removeFromTop (sliderRowHeight);
        inner.removeFromTop (8);
        r.greenColour = inner.removeFromTop (sliderRowHeight);
        inner.removeFromTop (8);
        r.redColour = inner.removeFromTop (sliderRowHeight);
        inner.removeFromTop (16);
        r.reset = inner.removeFromTop (36);
        return r;
    }

    /** Height needed by computeRows at `popupWidth`, derived from the layout
        itself so the panel can never clip its content. */
    int popupHeight()
    {
        juce::Rectangle<int> bounds (0, 0, popupWidth, 100000);
        return computeRows (bounds).reset.getBottom() + padding;
    }

    void styleSlider (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 78, 26);
        s.setRange (MeterSettings::scaleMinLufs, MeterSettings::scaleMaxLufs, 0.1);
        s.setTextValueSuffix (" LUFS");
        s.setColour (juce::Slider::backgroundColourId, MeterTheme::track);
        s.setColour (juce::Slider::trackColourId, MeterTheme::textDim);
        s.setColour (juce::Slider::thumbColourId, MeterTheme::textPrimary);
        s.setColour (juce::Slider::textBoxTextColourId, MeterTheme::textPrimary);
        s.setColour (juce::Slider::textBoxBackgroundColourId, MeterTheme::control);
        s.setColour (juce::Slider::textBoxOutlineColourId, MeterTheme::controlBorder);
    }

    void styleFlatButton (juce::TextButton& b, bool toggling)
    {
        if (toggling)
            b.setClickingTogglesState (true);

        b.setColour (juce::TextButton::buttonColourId, MeterTheme::control);
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3a4a3e));
        b.setColour (juce::TextButton::textColourOffId, MeterTheme::textSecondary);
        b.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    // Single source of truth for the three-option display selector.
    int displayModeToId (MeterDisplayMode mode)
    {
        switch (mode)
        {
            case MeterDisplayMode::vertical:   return 1;
            case MeterDisplayMode::horizontal: return 2;
            case MeterDisplayMode::compact:    return 3;
        }

        return 2;
    }

    MeterDisplayMode idToDisplayMode (int id)
    {
        switch (id)
        {
            case 1: return MeterDisplayMode::vertical;
            case 3: return MeterDisplayMode::compact;
            default: return MeterDisplayMode::horizontal;
        }
    }
}

SettingsContent::SettingsContent (MeterSettings initial,
                                  std::function<void (MeterSettings)> onSettingsChanged,
                                  std::function<void()> onReset)
    : settings_ (initial.sanitised()),
      onSettingsChanged_ (std::move (onSettingsChanged)),
      onReset_ (std::move (onReset))
{
    setLookAndFeel (&MeterTheme::lookAndFeel());

    // One mutually exclusive three-option selector replaces the old
    // orientation + size pair.
    displayCombo_.addItem ("Vertical", 1);
    displayCombo_.addItem ("Horizontal", 2);
    displayCombo_.addItem ("Compact", 3);
    displayCombo_.onChange = [this] { emitSettings(); };
    addAndMakeVisible (displayCombo_);

    showLabelButton_.onClick = [this] { emitSettings(); };
    showScaleButton_.onClick = [this] { emitSettings(); };
    addAndMakeVisible (showLabelButton_);
    addAndMakeVisible (showScaleButton_);

    for (auto* slider : { &greenLowSlider_, &greenHighSlider_, &redThresholdSlider_ })
    {
        styleSlider (*slider);
        slider->onValueChange = [this] { emitSettings(); };
        addAndMakeVisible (*slider);
    }

    for (auto* button : { &defaultColourButton_, &greenColourButton_, &redColourButton_ })
    {
        styleFlatButton (*button, false);
        button->setTriggeredOnMouseDown (false);
        addAndMakeVisible (*button);
    }

    defaultColourButton_.onClick = [this] { openColourSelector (0); };
    greenColourButton_.onClick   = [this] { openColourSelector (1); };
    redColourButton_.onClick     = [this] { openColourSelector (2); };

    styleFlatButton (resetButton_, false);
    resetButton_.onClick = [this] { if (onReset_) onReset_(); };
    addAndMakeVisible (resetButton_);

    refreshControls();
    setSize (popupWidth, popupHeight());
}

juce::Colour SettingsContent::currentZoneColour (int zone) const
{
    if (zone == 1)
        return juce::Colour (settings_.greenColour);
    if (zone == 2)
        return juce::Colour (settings_.redColour);
    return juce::Colour (settings_.defaultColour);
}

juce::TextButton& SettingsContent::colourButton (int zone)
{
    if (zone == 1)
        return greenColourButton_;
    if (zone == 2)
        return redColourButton_;
    return defaultColourButton_;
}

void SettingsContent::updateSwatch (juce::TextButton& button, juce::uint32 argb)
{
    const juce::Colour colour (argb);
    const float brightness = colour.getBrightness();

    button.setColour (juce::TextButton::buttonColourId, colour);
    button.setColour (juce::TextButton::buttonOnColourId, colour);
    button.setColour (juce::TextButton::textColourOffId,
                      brightness > 0.6f ? juce::Colour (0xff202020) : juce::Colours::white);
    button.setButtonText (juce::String::toHexString ((int) argb).substring (2).toUpperCase());
}

void SettingsContent::refreshControls()
{
    settings_ = settings_.sanitised();

    displayCombo_.setSelectedId (displayModeToId (settings_.displayMode),
                                 juce::dontSendNotification);

    showLabelButton_.setToggleState (settings_.showLabel, juce::dontSendNotification);
    showScaleButton_.setToggleState (settings_.showScale, juce::dontSendNotification);

    greenLowSlider_.setValue (settings_.greenLow, juce::dontSendNotification);
    greenHighSlider_.setValue (settings_.greenHigh, juce::dontSendNotification);
    redThresholdSlider_.setValue (settings_.redThreshold, juce::dontSendNotification);

    const auto accentColour = juce::Colour (settings_.greenColour);
    showLabelButton_.setColour (juce::ToggleButton::tickColourId, accentColour);
    showScaleButton_.setColour (juce::ToggleButton::tickColourId, accentColour);

    updateSwatch (defaultColourButton_, settings_.defaultColour);
    updateSwatch (greenColourButton_, settings_.greenColour);
    updateSwatch (redColourButton_, settings_.redColour);
}

void SettingsContent::emitSettings()
{
    settings_.displayMode = idToDisplayMode (displayCombo_.getSelectedId());
    settings_.showLabel = showLabelButton_.getToggleState();
    settings_.showScale = showScaleButton_.getToggleState();
    settings_.greenLow = greenLowSlider_.getValue();
    settings_.greenHigh = greenHighSlider_.getValue();
    settings_.redThreshold = redThresholdSlider_.getValue();

    refreshControls();

    if (onSettingsChanged_)
        onSettingsChanged_ (settings_);

    repaint();
}

void SettingsContent::setZoneColour (int zone, juce::Colour colour)
{
    const auto argb = colour.getARGB();

    if (zone == 1)
        settings_.greenColour = argb;
    else if (zone == 2)
        settings_.redColour = argb;
    else
        settings_.defaultColour = argb;

    updateSwatch (colourButton (zone), argb);

    if (zone == 1)
    {
        showLabelButton_.setColour (juce::ToggleButton::tickColourId, colour);
        showScaleButton_.setColour (juce::ToggleButton::tickColourId, colour);
    }

    if (onSettingsChanged_)
        onSettingsChanged_ (settings_);

    repaint();
}

void SettingsContent::openColourSelector (int zone)
{
    auto safe = juce::Component::SafePointer<SettingsContent> (this);

    auto selector = std::make_unique<ZoneColourSelector>();
    selector->setCurrentColour (currentZoneColour (zone), juce::dontSendNotification);
    selector->setSize (300, 320);
    selector->onColourChange = [safe, zone] (juce::Colour colour)
    {
        if (auto* self = safe.getComponent())
            self->setZoneColour (zone, colour);
    };

    juce::CallOutBox::launchAsynchronously (std::move (selector),
                                            colourButton (zone).getScreenBounds(),
                                            nullptr);
}

void SettingsContent::paint (juce::Graphics& g)
{
    g.fillAll (MeterTheme::panelBackground);

    const auto rows = computeRows (getLocalBounds());

    g.setColour (MeterTheme::textPrimary);
    g.setFont (MeterTheme::mediumFont (17.0f));
    g.drawText ("Settings", rows.title, juce::Justification::centredLeft, false);

    g.setColour (MeterTheme::textSecondary);
    g.setFont (MeterTheme::regularFont (13.0f));
    g.drawText ("ZONES (LUFS)", rows.zonesHeading, juce::Justification::centredLeft, false);
    g.drawText ("COLOURS", rows.coloursHeading, juce::Justification::centredLeft, false);

    g.setColour (MeterTheme::textSecondary);
    g.setFont (MeterTheme::regularFont (14.5f));
    g.drawText ("Display", rows.display, juce::Justification::centredLeft, false);
    g.drawText ("Green low", rows.greenLow, juce::Justification::centredLeft, false);
    g.drawText ("Green high", rows.greenHigh, juce::Justification::centredLeft, false);
    g.drawText ("Red threshold", rows.redThreshold, juce::Justification::centredLeft, false);
    g.drawText ("Default", rows.defaultColour, juce::Justification::centredLeft, false);
    g.drawText ("Green", rows.greenColour, juce::Justification::centredLeft, false);
    g.drawText ("Red", rows.redColour, juce::Justification::centredLeft, false);
}

void SettingsContent::resized()
{
    auto rows = computeRows (getLocalBounds());

    displayCombo_.setBounds (rows.display.withTrimmedLeft (labelWidth));

    showLabelButton_.setBounds (rows.showLabel);
    showScaleButton_.setBounds (rows.showScale);

    greenLowSlider_.setBounds (rows.greenLow.withTrimmedLeft (labelWidth));
    greenHighSlider_.setBounds (rows.greenHigh.withTrimmedLeft (labelWidth));
    redThresholdSlider_.setBounds (rows.redThreshold.withTrimmedLeft (labelWidth));

    const int swatchWidth = 132;
    defaultColourButton_.setBounds (rows.defaultColour.withTrimmedLeft (labelWidth)
                                        .removeFromRight (swatchWidth));
    greenColourButton_.setBounds (rows.greenColour.withTrimmedLeft (labelWidth)
                                      .removeFromRight (swatchWidth));
    redColourButton_.setBounds (rows.redColour.withTrimmedLeft (labelWidth)
                                    .removeFromRight (swatchWidth));

    resetButton_.setBounds (rows.reset.removeFromLeft (110));
}
