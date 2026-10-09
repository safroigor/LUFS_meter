#include "MeterTheme.h"

namespace
{
    juce::String pickFamily()
    {
        // A clean sans-serif with distinct numerals on Windows; JUCE falls back
        // to its default when the family is unavailable (e.g. on another OS).
        return "Segoe UI";
    }
}

namespace MeterTheme
{
    juce::String sansFamily()
    {
        return pickFamily();
    }

    juce::Font regularFont (float height)
    {
        return juce::Font (juce::FontOptions (sansFamily(), "Regular", height));
    }

    juce::Font mediumFont (float height)
    {
        // Semibold reads as a confident focal value without the weight of a full
        // bold face.
        return juce::Font (juce::FontOptions (sansFamily(), "Semibold", height));
    }

    juce::LookAndFeel& lookAndFeel()
    {
        static juce::SharedResourcePointer<MeterLookAndFeel> shared;
        return *shared;
    }
}

MeterLookAndFeel::MeterLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, MeterTheme::panelBackground);
    setColour (juce::PopupMenu::backgroundColourId, MeterTheme::panelBackground);
    setColour (juce::PopupMenu::textColourId, MeterTheme::textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, MeterTheme::controlHover);
    setColour (juce::PopupMenu::highlightedTextColourId, MeterTheme::textPrimary);
}

juce::Font MeterLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return MeterTheme::regularFont (juce::jmin (16.0f, (float) buttonHeight * 0.5f));
}

juce::Font MeterLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return MeterTheme::regularFont (juce::jmin (16.0f, (float) box.getHeight() * 0.58f));
}

juce::Font MeterLookAndFeel::getLabelFont (juce::Label& label)
{
    return MeterTheme::regularFont (juce::jmin (16.0f, (float) label.getHeight() * 0.62f));
}

void MeterLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto fill = backgroundColour;

    if (down)
        fill = fill.brighter (0.12f);
    else if (highlighted)
        fill = fill.brighter (0.06f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 4.0f);

    if (button.hasKeyboardFocus (false))
    {
        g.setColour (MeterTheme::accent.withAlpha (0.55f));
        g.drawRoundedRectangle (bounds.reduced (0.75f), 4.0f, 1.0f);
    }
}

void MeterLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                            : juce::TextButton::textColourOffId)
                    .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));

    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().reduced (4, 0),
                      juce::Justification::centred, 1, 0.9f);
}

void MeterLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool highlighted, bool down)
{
    const auto& b = button;
    const float boxSize = juce::jmin (16.0f, (float) b.getHeight() - 6.0f);
    auto box = juce::Rectangle<float> ((float) b.getWidth(), (float) b.getHeight())
                   .withSizeKeepingCentre (boxSize, boxSize)
                   .withX (2.0f);
    box.setY ((b.getHeight() - boxSize) * 0.5f);

    const bool ticked = b.getToggleState();
    const auto tickColour = b.findColour (juce::ToggleButton::tickColourId);

    if (ticked)
    {
        g.setColour (tickColour);
        g.fillRoundedRectangle (box, 3.5f);

        juce::Path tick;
        tick.startNewSubPath (box.getX() + boxSize * 0.28f, box.getCentreY());
        tick.lineTo (box.getCentreX() - boxSize * 0.06f, box.getBottom() - boxSize * 0.30f);
        tick.lineTo (box.getRight() - boxSize * 0.24f, box.getY() + boxSize * 0.30f);
        g.setColour (juce::Colours::white);
        g.strokePath (tick, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
    else
    {
        g.setColour (highlighted ? MeterTheme::controlHover : MeterTheme::control);
        g.fillRoundedRectangle (box, 3.5f);

        g.setColour (down ? MeterTheme::accent.withAlpha (0.7f) : MeterTheme::controlBorder);
        g.drawRoundedRectangle (box.reduced (0.5f), 3.5f, 1.0f);
    }

    g.setColour (b.isEnabled() ? MeterTheme::textSecondary : MeterTheme::textDim);
    g.setFont (MeterTheme::regularFont (juce::jmin (15.5f, (float) b.getHeight() * 0.55f)));
    g.drawFittedText (b.getButtonText(),
                      b.getLocalBounds().withTrimmedLeft ((int) box.getRight() + 8)
                          .withTrimmedRight (2),
                      juce::Justification::centredLeft, 1, 0.9f);
}

void MeterLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);

    g.setColour (MeterTheme::control);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (box.hasKeyboardFocus (false) ? MeterTheme::accent.withAlpha (0.7f)
                                              : MeterTheme::controlBorder);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) width - 12.0f;
    const float cy = (float) height * 0.5f;
    arrow.startNewSubPath (cx - 3.5f, cy - 1.8f);
    arrow.lineTo (cx, cy + 2.2f);
    arrow.lineTo (cx + 3.5f, cy - 1.8f);

    g.setColour (MeterTheme::textSecondary);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void MeterLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 0, box.getWidth() - 26, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

int MeterLookAndFeel::getSliderThumbRadius (juce::Slider&)
{
    return 6;
}

void MeterLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float minSliderPos, float maxSliderPos,
                                         juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearBar)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                                minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const float cy = (float) y + (float) height * 0.5f;
    const float start = (float) x + 2.0f;
    const float end = juce::jmax (start + 1.0f, maxSliderPos);

    g.setColour (slider.findColour (juce::Slider::backgroundColourId));
    g.fillRoundedRectangle (start, cy - 1.5f, end - start, 3.0f, 1.5f);

    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (start, cy - 1.5f, juce::jmax (0.0f, sliderPos - start), 3.0f, 1.5f);

    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (sliderPos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
}

void MeterLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted,
                                          bool isTicked, bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColour)
{
    juce::ignoreUnused (isTicked, hasSubMenu, icon, textColour);

    if (isSeparator)
    {
        g.setColour (MeterTheme::controlBorder);
        g.fillRect (area.reduced (6, 0).withSizeKeepingCentre (area.getWidth() - 12, 1));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour (MeterTheme::controlHover);
        g.fillRoundedRectangle (area.reduced (3, 1).toFloat(), 3.0f);
    }

    g.setColour (isActive ? MeterTheme::textPrimary : MeterTheme::textDim);
    g.setFont (MeterTheme::regularFont (15.0f));
    g.drawFittedText (text, area.reduced (10, 0), juce::Justification::centredLeft, 1, 0.9f);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (MeterTheme::textDim);
        g.drawFittedText (shortcutKeyText, area.reduced (10, 0), juce::Justification::centredRight, 1, 0.9f);
    }
}
