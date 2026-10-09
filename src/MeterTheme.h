#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
    Shared visual language for the meter UI: palette, typography and a custom
    LookAndFeel.

    All text and control styling goes through here so the editor and the
    settings popup cannot drift apart. Colours that the user can configure (the
    zone colours) stay in MeterSettings; this namespace only owns the fixed
    chrome.
*/
namespace MeterTheme
{
    inline const juce::Colour windowBackground { 0xff161616 };
    inline const juce::Colour panelBackground  { 0xff1e1e1e };
    inline const juce::Colour control          { 0xff2a2a2a };
    inline const juce::Colour controlHover     { 0xff343434 };
    inline const juce::Colour controlBorder    { 0xff3c3c3c };
    inline const juce::Colour track            { 0xff262626 };
    inline const juce::Colour ruler            { 0xff5f5f5f };
    inline const juce::Colour marker           { 0xffffffff };
    inline const juce::Colour accent           { 0xff34c759 };
    inline const juce::Colour textPrimary      { 0xfff2f2f2 };
    inline const juce::Colour textSecondary    { 0xff9a9a9a };
    inline const juce::Colour textDim          { 0xff6e6e6e };

    /** Preferred sans-serif family with a safe fallback. */
    juce::String sansFamily();

    juce::Font regularFont (float height);
    juce::Font mediumFont (float height);

    /** Process-wide, reference-counted LookAndFeel shared by the editor and any
        popups it opens. */
    juce::LookAndFeel& lookAndFeel();
}

/** Custom LookAndFeel implementing the plugin's control design language. */
class MeterLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MeterLookAndFeel();

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    int getSliderThumbRadius (juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                            bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
};
