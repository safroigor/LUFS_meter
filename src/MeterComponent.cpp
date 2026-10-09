#include "MeterComponent.h"

#include "MeasurementEngine.h"

namespace
{
    constexpr double minLufs = -30.0;
    constexpr double maxLufs = 0.0;
    constexpr int    innerPadding = 8;

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

    juce::ColourGradient makeGradient (juce::Point<float> from, juce::Point<float> to)
    {
        const auto yellowProportion = proportion (-14.0);
        juce::ColourGradient gradient (greenColour, from, yellowColour, to, false);
        gradient.addColour (juce::jlimit (0.0, 1.0, (double) yellowProportion), yellowColour);
        return gradient;
    }

    juce::String formatValue (bool hasValue, double value)
    {
        return hasValue ? juce::String (value, 1)
                        : juce::String::fromUTF8 ("\u2014");
    }

    void drawFilledTrack (juce::Graphics& g, juce::Rectangle<float> bar, float radius,
                          juce::Rectangle<float> filled, juce::ColourGradient gradient)
    {
        juce::Path clip;
        clip.addRoundedRectangle (bar, radius);

        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (clip);
        g.setGradientFill (gradient);
        g.fillRect (filled);
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

    auto r = area.reduced (innerPadding, 0);
    const int numFont = compact ? 34 : 26;
    const int unitFont = compact ? 12 : 10;
    const int unitWidth = compact ? 44 : 36;
    const int valueWidth = (compact ? 120 : 100);

    auto valueArea = r.removeFromRight (valueWidth);
    juce::Rectangle<int> labelArea;

    if (settings_.showLabel)
        labelArea = r.removeFromLeft (compact ? 56 : 48);

    const int barThickness = compact ? 18 : 12;
    const int gap = 6;
    const int scaleHeight = 14;
    const int blockHeight = barThickness + (settings_.showScale ? gap + scaleHeight : 0);

    auto block = r.withSizeKeepingCentre (r.getWidth(), blockHeight);
    auto barArea = block.removeFromTop (barThickness).toFloat();

    juce::Rectangle<int> scaleArea;

    if (settings_.showScale)
    {
        block.removeFromTop (gap);
        scaleArea = block;
    }

    const float radius = (float) barThickness * 0.5f;

    g.setColour (trackColour);
    g.fillRoundedRectangle (barArea, radius);

    if (hasValue)
    {
        const float x = barArea.getX() + proportion (value) * barArea.getWidth();

        drawFilledTrack (g, barArea, radius,
                         { barArea.getX(), barArea.getY(), x - barArea.getX(), barArea.getHeight() },
                         makeGradient ({ barArea.getX(), barArea.getCentreY() },
                                       { barArea.getRight(), barArea.getCentreY() }));

        g.setColour (markerColour);
        g.fillRect (juce::Rectangle<float> (x - 1.0f, barArea.getY() - 4.0f,
                                            2.0f, barArea.getHeight() + 8.0f));
    }

    if (settings_.showScale)
    {
        g.setFont (compact ? 10.5f : 9.5f);
        g.setColour (dimColour);

        for (int v = (int) minLufs; v <= (int) maxLufs; v += 3)
        {
            const float x = barArea.getX() + proportion ((double) v) * barArea.getWidth();
            g.fillRect (juce::Rectangle<float> (x - 0.5f, (float) scaleArea.getY(), 1.0f, 4.0f));
            g.drawText (juce::String (v),
                        juce::Rectangle<int> ((int) x - 16, scaleArea.getY() + 5, 32, 12),
                        juce::Justification::centred, false);
        }
    }

    if (settings_.showLabel)
    {
        g.setColour (textColour);
        g.setFont (compact ? 14.0f : 12.0f);
        g.drawText ("LUFS", labelArea, juce::Justification::centredLeft, false);
    }

    // Numeric value and unit, vertically aligned with the centre of the bar.
    const auto centreY = barArea.getCentreY();

    g.setColour (hasValue ? textColour : dimColour);
    g.setFont ((float) numFont);
    auto numRect = valueArea.withTrimmedRight (unitWidth);
    numRect = numRect.withSizeKeepingCentre (numRect.getWidth(), numFont)
                     .translated (0, (int) centreY - numRect.getCentreY());
    g.drawText (formatValue (hasValue, value), numRect, juce::Justification::centredRight, false);

    g.setColour (dimColour);
    g.setFont ((float) unitFont);
    auto unitRect = valueArea.removeFromRight (unitWidth);
    unitRect = unitRect.withSizeKeepingCentre (unitRect.getWidth(), unitFont)
                       .translated (0, (int) centreY - unitRect.getCentreY());
    g.drawText ("LUFS", unitRect, juce::Justification::centredLeft, false);
}

void MeterComponent::paintVertical (juce::Graphics& g, juce::Rectangle<int> area,
                                    bool hasValue, double value)
{
    const bool compact = settings_.size == MeterSizeMode::compact;

    auto r = area.reduced (innerPadding, 6);

    juce::Rectangle<int> labelArea;

    if (settings_.showLabel)
        labelArea = r.removeFromTop (18);

    auto valueArea = r.removeFromBottom (compact ? 58 : 46);
    auto scaleArea = settings_.showScale ? r.removeFromRight (32)
                                         : juce::Rectangle<int>();

    const int barThickness = compact ? 16 : 12;
    auto barArea = r.withSizeKeepingCentre (barThickness, r.getHeight()).toFloat();
    const float radius = (float) barThickness * 0.5f;

    g.setColour (trackColour);
    g.fillRoundedRectangle (barArea, radius);

    if (hasValue)
    {
        const float y = barArea.getBottom() - proportion (value) * barArea.getHeight();

        drawFilledTrack (g, barArea, radius,
                         { barArea.getX(), y, barArea.getWidth(), barArea.getBottom() - y },
                         makeGradient ({ barArea.getCentreX(), barArea.getBottom() },
                                       { barArea.getCentreX(), barArea.getY() }));

        g.setColour (markerColour);
        g.fillRect (juce::Rectangle<float> (barArea.getX() - 4.0f, y - 1.0f,
                                            barArea.getWidth() + 8.0f, 2.0f));
    }

    if (settings_.showScale)
    {
        g.setFont (compact ? 10.5f : 9.5f);
        g.setColour (dimColour);

        for (int v = (int) minLufs; v <= (int) maxLufs; v += 3)
        {
            const float y = barArea.getBottom() - proportion ((double) v) * barArea.getHeight();
            g.fillRect (juce::Rectangle<float> ((float) scaleArea.getX(), y - 0.5f, 4.0f, 1.0f));
            g.drawText (juce::String (v),
                        juce::Rectangle<int> (scaleArea.getX() + 5, (int) y - 6, 26, 12),
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
    g.setFont (compact ? 28.0f : 22.0f);
    g.drawText (formatValue (hasValue, value),
                valueArea.withTrimmedBottom (compact ? 18 : 14),
                juce::Justification::centred, false);

    g.setColour (dimColour);
    g.setFont (compact ? 11.0f : 10.0f);
    g.drawText ("LUFS", valueArea.removeFromBottom (compact ? 18 : 14),
                juce::Justification::centred, false);
}
