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
    const juce::Colour rulerColour  = juce::Colour (0xff565656);
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

    // A plain hyphen; rendering is guaranteed regardless of the source-code
    // encoding / system code page.
    juce::String formatValue (bool hasValue, double value)
    {
        return hasValue ? juce::String (value, 1) : juce::String ("-");
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

    void drawTicksHorizontal (juce::Graphics& g, juce::Rectangle<float> bar, int componentWidth,
                              int labelTop, float fontHeight)
    {
        g.setFont (fontHeight);
        g.setColour (rulerColour);

        for (int v = (int) minLufs; v <= (int) maxLufs; v += 3)
        {
            const float x = bar.getX() + proportion ((double) v) * bar.getWidth();
            g.fillRect (juce::Rectangle<float> (x - 0.5f, (float) labelTop - 5.0f, 1.0f, 4.0f));

            // Magnitudes only: repeating minus signs beside a negative reading
            // looked like two minus signs.
            const int labelX = juce::jlimit (0, juce::jmax (0, componentWidth - 32),
                                             (int) x - 16);
            g.drawText (juce::String (v < 0 ? -v : v),
                        juce::Rectangle<int> (labelX, labelTop, 32, 12),
                        juce::Justification::centred, false);
        }
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

    auto r = area.reduced (innerPadding, innerPadding / 2);
    const float centreY = (float) r.getCentreY();

    const int numFont = compact ? 34 : 26;
    const int unitFont = compact ? 12 : 10;
    const int unitWidth = compact ? 42 : 34;
    const int valueWidth = compact ? 120 : 100;
    const int labelWidth = compact ? 52 : 44;

    juce::Rectangle<int> labelArea;
    if (settings_.showLabel)
        labelArea = r.removeFromLeft (labelWidth);

    auto valueArea = r.removeFromRight (valueWidth);
    auto barArea = r;

    const int barThickness = compact ? 18 : 12;
    const float radius = (float) barThickness * 0.5f;
    juce::Rectangle<float> bar ((float) barArea.getX(),
                                centreY - (float) barThickness * 0.5f,
                                (float) barArea.getWidth(),
                                (float) barThickness);

    g.setColour (trackColour);
    g.fillRoundedRectangle (bar, radius);

    if (hasValue)
    {
        const float x = bar.getX() + proportion (value) * bar.getWidth();

        drawFilledTrack (g, bar, radius,
                         { bar.getX(), bar.getY(), x - bar.getX(), bar.getHeight() },
                         makeGradient ({ bar.getX(), bar.getCentreY() },
                                       { bar.getRight(), bar.getCentreY() }));

        g.setColour (markerColour);
        g.fillRect (juce::Rectangle<float> (x - 1.0f, bar.getY() - 4.0f,
                                            2.0f, bar.getHeight() + 8.0f));
    }

    if (settings_.showScale)
        drawTicksHorizontal (g, bar, getWidth(), (int) bar.getBottom() + 6,
                             compact ? 10.5f : 9.5f);

    if (settings_.showLabel)
    {
        g.setColour (textColour);
        g.setFont (compact ? 14.0f : 12.0f);
        auto labelRect = labelArea.withSizeKeepingCentre (labelArea.getWidth(), 20);
        labelRect.translate (0, (int) centreY - labelRect.getCentreY());
        g.drawText ("LUFS", labelRect, juce::Justification::centredLeft, false);
    }

    // Value and unit share the horizontal axis with the bar.
    const int gap = compact ? 6 : 5;

    g.setColour (hasValue ? textColour : dimColour);
    g.setFont ((float) numFont);
    auto numRect = valueArea.withTrimmedRight (unitWidth + gap);
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
    const float centreX = (float) getWidth() * 0.5f;

    juce::Rectangle<int> labelArea;
    if (settings_.showLabel)
        labelArea = r.removeFromTop (18);

    auto valueArea = r.removeFromBottom (compact ? 54 : 44);

    const int barThickness = compact ? 14 : 11;
    const int gap = 5;
    const int rulerWidth = compact ? 22 : 18;
    const bool showScale = settings_.showScale;

    // Centre the bar and the ruler together, so the window can be narrow.
    const int groupWidth = barThickness + (showScale ? gap + rulerWidth : 0);
    const float groupX = centreX - (float) groupWidth * 0.5f;

    const float radius = (float) barThickness * 0.5f;
    juce::Rectangle<float> bar (groupX, (float) r.getY(),
                                (float) barThickness, (float) r.getHeight());

    g.setColour (trackColour);
    g.fillRoundedRectangle (bar, radius);

    if (hasValue)
    {
        const float y = bar.getBottom() - proportion (value) * bar.getHeight();

        drawFilledTrack (g, bar, radius,
                         { bar.getX(), y, bar.getWidth(), bar.getBottom() - y },
                         makeGradient ({ bar.getCentreX(), bar.getBottom() },
                                       { bar.getCentreX(), bar.getY() }));

        g.setColour (markerColour);
        g.fillRect (juce::Rectangle<float> (bar.getX() - 4.0f, y - 1.0f,
                                            bar.getWidth() + 8.0f, 2.0f));
    }

    if (showScale)
    {
        g.setFont (compact ? 10.0f : 9.0f);
        g.setColour (rulerColour);

        const int tickX = (int) bar.getRight() + gap;

        for (int v = (int) minLufs; v <= (int) maxLufs; v += 3)
        {
            const float y = bar.getBottom() - proportion ((double) v) * bar.getHeight();
            g.fillRect (juce::Rectangle<float> ((float) tickX, y - 0.5f, 4.0f, 1.0f));

            const int labelY = juce::jlimit (0, juce::jmax (0, getHeight() - 12), (int) y - 6);
            g.drawText (juce::String (v < 0 ? -v : v),
                        juce::Rectangle<int> (tickX + 6, labelY, rulerWidth, 12),
                        juce::Justification::centredLeft, false);
        }
    }

    if (settings_.showLabel)
    {
        g.setColour (textColour);
        g.setFont (12.0f);
        g.drawText ("LUFS", labelArea, juce::Justification::centred, false);
    }

    {
        const auto valueText = formatValue (hasValue, value);
        juce::Font valueFont (juce::FontOptions ((float) (compact ? 28 : 22)));
        g.setFont (valueFont);
        g.setColour (hasValue ? textColour : dimColour);

        auto valueRect = valueArea.withTrimmedBottom (compact ? 18 : 14);

        // Centre the digits, not the leading minus: every LUFS reading is
        // negative, so including the sign would shift the digits to the right.
        if (valueText.startsWithChar ('-'))
        {
            const float signWidth = juce::GlyphArrangement::getStringWidth (valueFont, "-");
            valueRect.translate ((int) (-signWidth * 0.5f), 0);
        }

        g.drawText (valueText, valueRect, juce::Justification::centred, false);
    }

    g.setColour (dimColour);
    g.setFont (compact ? 11.0f : 10.0f);
    g.drawText ("LUFS", valueArea.removeFromBottom (compact ? 18 : 14),
                juce::Justification::centred, false);
}
