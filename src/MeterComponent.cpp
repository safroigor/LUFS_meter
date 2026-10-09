#include "MeterComponent.h"

#include "MeasurementEngine.h"

namespace
{
    constexpr double minLufs = -30.0;
    constexpr double maxLufs = 0.0;

    const juce::Colour trackColour  = juce::Colour (0xff2b2b2b);
    const juce::Colour greenColour  = juce::Colour (0xff18c96a);
    const juce::Colour yellowColour = juce::Colour (0xfff2d90b);
    const juce::Colour textColour   = juce::Colour (0xffececec);
    const juce::Colour dimColour    = juce::Colour (0xff8f8f8f);
    const juce::Colour markerColour = juce::Colour (0xffffffff);

    float proportion (double value)
    {
        return (float) juce::jlimit (0.0, 1.0, (value - minLufs) / (maxLufs - minLufs));
    }

    /** Gradient that runs green (at minLufs) to yellow (at -14 LUFS). */
    juce::ColourGradient makeGradient (juce::Point<float> from, juce::Point<float> to)
    {
        const auto yellowProportion = proportion (-14.0);
        juce::ColourGradient gradient (greenColour, from, yellowColour, to, false);
        gradient.addColour (juce::jlimit (0.0, 1.0, (double) yellowProportion), yellowColour);
        return gradient;
    }

    juce::String formatValue (bool hasValue, double value)
    {
        if (! hasValue)
            return juce::String::fromUTF8 ("\u2014");

        return juce::String (value, 1);
    }
}

MeterComponent::MeterComponent (const MeasurementEngine& engine)
    : engine_ (engine)
{
}

void MeterComponent::setSettings (const MeterSettings& s)
{
    settings_ = s;
    repaint();
}

void MeterComponent::paint (juce::Graphics& g)
{
    const bool hasValue = engine_.hasValue();
    const double value = engine_.getIntegratedLufs();

    if (settings_.orientation == MeterOrientation::horizontal)
        paintHorizontal (g, getLocalBounds(), hasValue, value);
    else
        paintVertical (g, getLocalBounds(), hasValue, value);
}

void MeterComponent::paintHorizontal (juce::Graphics& g, juce::Rectangle<int> area,
                                      bool hasValue, double value)
{
    const bool compact = settings_.size == MeterSizeMode::compact;
    const int valueWidth = compact ? 116 : 96;

    auto valueArea = area.removeFromRight (valueWidth);
    juce::Rectangle<int> labelArea;

    if (settings_.showLabel)
        labelArea = area.removeFromLeft (compact ? 58 : 50);

    auto scaleArea = settings_.showScale ? area.removeFromBottom (16)
                                         : juce::Rectangle<int>();

    const int thickness = compact ? 20 : 12;
    auto bar = area.withSizeKeepingCentre (area.getWidth(), thickness).toFloat();

    // Track.
    g.setColour (trackColour);
    g.fillRoundedRectangle (bar, (float) thickness * 0.5f);

    if (hasValue)
    {
        const float t = proportion (value);
        const float x = bar.getX() + t * bar.getWidth();
        auto filled = bar.withRight (x);

        g.setGradientFill (makeGradient ({ bar.getX(), bar.getCentreY() },
                                         { bar.getRight(), bar.getCentreY() }));
        g.fillRect (filled);

        g.setColour (markerColour);
        g.fillRect (juce::Rectangle<float> (x - 1.0f, bar.getY() - 3.0f, 2.0f, bar.getHeight() + 6.0f));
    }

    // Scale ticks.
    if (settings_.showScale)
    {
        g.setFont (compact ? 10.0f : 9.0f);
        g.setColour (dimColour);

        for (int v = (int) minLufs; v <= (int) maxLufs; v += 3)
        {
            const float x = bar.getX() + proportion ((double) v) * bar.getWidth();

            g.fillRect (juce::Rectangle<float> (x - 0.5f, (float) scaleArea.getY(), 1.0f, 4.0f));
            g.drawText (juce::String (v),
                        juce::Rectangle<int> ((int) x - 16, scaleArea.getY() + 4, 32, 12),
                        juce::Justification::centred, false);
        }
    }

    // Label.
    if (settings_.showLabel)
    {
        g.setColour (textColour);
        g.setFont (compact ? 14.0f : 12.0f);
        g.drawText ("LUFS", labelArea, juce::Justification::centredLeft, false);
    }

    // Numeric value.
    g.setColour (hasValue ? textColour : dimColour);
    g.setFont (compact ? 32.0f : 24.0f);
    g.drawText (formatValue (hasValue, value),
                valueArea.withTrimmedRight (compact ? 42 : 34),
                juce::Justification::centredRight, false);

    g.setColour (dimColour);
    g.setFont (compact ? 12.0f : 10.0f);
    g.drawText ("LUFS",
                valueArea.removeFromRight (compact ? 42 : 34),
                juce::Justification::centredLeft, false);
}

void MeterComponent::paintVertical (juce::Graphics& g, juce::Rectangle<int> area,
                                    bool hasValue, double value)
{
    const bool compact = settings_.size == MeterSizeMode::compact;

    juce::Rectangle<int> labelArea;

    if (settings_.showLabel)
        labelArea = area.removeFromTop (18);

    auto valueArea = area.removeFromBottom (compact ? 56 : 44);
    auto scaleArea = settings_.showScale ? area.removeFromRight (34)
                                         : juce::Rectangle<int>();

    const int thickness = compact ? 18 : 12;
    auto bar = area.withSizeKeepingCentre (thickness, area.getHeight()).toFloat();

    g.setColour (trackColour);
    g.fillRoundedRectangle (bar, (float) thickness * 0.5f);

    if (hasValue)
    {
        const float t = proportion (value);
        const float y = bar.getBottom() - t * bar.getHeight();
        auto filled = bar.withTop (y);

        g.setGradientFill (makeGradient ({ bar.getCentreX(), bar.getBottom() },
                                         { bar.getCentreX(), bar.getY() }));
        g.fillRect (filled);

        g.setColour (markerColour);
        g.fillRect (juce::Rectangle<float> (bar.getX() - 3.0f, y - 1.0f, bar.getWidth() + 6.0f, 2.0f));
    }

    if (settings_.showScale)
    {
        g.setFont (compact ? 10.0f : 9.0f);
        g.setColour (dimColour);

        for (int v = (int) minLufs; v <= (int) maxLufs; v += 3)
        {
            const float y = bar.getBottom() - proportion ((double) v) * bar.getHeight();

            g.fillRect (juce::Rectangle<float> ((float) scaleArea.getX(), y - 0.5f, 4.0f, 1.0f));
            g.drawText (juce::String (v),
                        juce::Rectangle<int> (scaleArea.getX() + 5, (int) y - 6, 28, 12),
                        juce::Justification::centredLeft, false);
        }
    }

    if (settings_.showLabel)
    {
        g.setColour (textColour);
        g.setFont (12.0f);
        g.drawText ("LUFS", labelArea, juce::Justification::centred, false);
    }

    g.setColour (hasValue ? textColour : dimColour);
    g.setFont (compact ? 26.0f : 20.0f);
    g.drawText (formatValue (hasValue, value),
                valueArea.withTrimmedBottom (compact ? 16 : 13),
                juce::Justification::centred, false);

    g.setColour (dimColour);
    g.setFont (compact ? 11.0f : 10.0f);
    g.drawText ("LUFS", valueArea.removeFromBottom (compact ? 16 : 13),
                juce::Justification::centred, false);
}
