#include "MeterComponent.h"

#include "MeasurementEngine.h"
#include "MeterTheme.h"

#include <cmath>

namespace
{
    // Fixed number of segments: independent of size, so every segment in a
    // render has identical dimensions and spacing (the FabFilter-style look).
    constexpr int segmentCount = 48;

    // Shared layout constants, used by both orientations so the geometry can
    // never drift between paint() and the two layout functions.
    namespace Layout
    {
        constexpr int   marginX = 8;
        constexpr int   marginY = 8;
        constexpr float segmentRadius = 3.0f;
        constexpr double tickStep = 6.0;        // LUFS between scale labels
    }

    const char* const modeLabel = "LUFS-I";

    float proportion (double value)
    {
        return (float) juce::jlimit (0.0, 1.0,
                                     (value - MeterSettings::scaleMinLufs)
                                         / (MeterSettings::scaleMaxLufs - MeterSettings::scaleMinLufs));
    }

    juce::Colour colourForZone (MeterZone zone, const MeterSettings& s)
    {
        switch (zone)
        {
            case MeterZone::green: return juce::Colour (s.greenColour);
            case MeterZone::red:   return juce::Colour (s.redColour);
            case MeterZone::normal:
            default:               return juce::Colour (s.defaultColour);
        }
    }

    juce::Colour segmentColour (const MeterSettings& s, double segValue, bool filled)
    {
        return filled ? colourForZone (s.zoneForValue (segValue), s) : MeterTheme::track;
    }

    // The displayed reading is rounded to 0.1 LU. Using the *same* rounded value
    // for the numeric text and for the zone classification keeps text and meter
    // in agreement and stable around zone boundaries.
    double roundToDisplay (double value)
    {
        return std::round (value * 10.0) / 10.0;
    }

    // A plain hyphen; rendering is guaranteed regardless of the source-code
    // encoding / system code page.
    juce::String formatValue (bool hasValue, double value)
    {
        return hasValue ? juce::String (value, 1) : juce::String ("-");
    }

    juce::String tickText (double value)
    {
        // Magnitudes only: repeating minus signs beside a negative reading
        // looked like two minus signs.
        return juce::String ((int) std::lround (std::abs (value)));
    }

    /** A row of the given height whose *visual* vertical centre sits on
        `centreY`. Text drawn into it with a centred justification is centred on
        that coordinate, independent of component bounds or baselines. */
    juce::Rectangle<int> rowAtCentreY (juce::Rectangle<int> area, float centreY, int height)
    {
        return { area.getX(),
                 (int) std::lround (centreY - (float) height * 0.5f),
                 area.getWidth(),
                 height };
    }

    int textWidth (const juce::Font& font, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (font, text);
    }

    /** Picks a tick step (from `Layout::tickStep` upward) that keeps at least
        `minSpacingPx` between adjacent labels, so larger fonts never overlap. */
    double chooseTickStep (float lengthPx, int minSpacingPx)
    {
        const double range = MeterSettings::scaleMaxLufs - MeterSettings::scaleMinLufs;

        for (double step : { Layout::tickStep, 10.0, 15.0, 30.0 })
            if (lengthPx * (float) (step / range) >= (float) minSpacingPx)
                return step;

        return 30.0;
    }

    /** Fills a row/column of uniform rounded segments. `segmentPitch` is the
        distance between segment starts and `gap` the dark space between them;
        `rectFor` builds each segment rectangle from its index. */
    template <typename RectFor>
    void drawSegments (juce::Graphics& g, const MeterSettings& s, bool hasValue, double value,
                       float segmentPitch, float gap, RectFor&& rectFor)
    {
        for (int i = 0; i < segmentCount; ++i)
        {
            const double segValue = MeterSettings::scaleMinLufs
                                    + ((i + 0.5) / (double) segmentCount)
                                          * (MeterSettings::scaleMaxLufs - MeterSettings::scaleMinLufs);

            const bool filled = hasValue && value >= segValue;

            g.setColour (segmentColour (s, segValue, filled));
            g.fillRoundedRectangle (rectFor (i, segmentPitch, gap), Layout::segmentRadius);
        }
    }
}

MeterComponent::MeterComponent (const MeasurementEngine& engine)
    : engine_ (engine)
{
}

void MeterComponent::setSettings (const MeterSettings& s)
{
    settings_ = s.sanitised();
    repaint();
}

void MeterComponent::paint (juce::Graphics& g)
{
    const bool hasValue = engine_.hasValue();
    const double value = roundToDisplay (engine_.getIntegratedLufs());

    if (settings_.orientation == MeterOrientation::horizontal)
        paintHorizontal (g, getLocalBounds(), hasValue, value);
    else
        paintVertical (g, getLocalBounds(), hasValue, value);
}

void MeterComponent::paintHorizontal (juce::Graphics& g, juce::Rectangle<int> area,
                                      bool hasValue, double value)
{
    const auto s = settings_.sanitised();
    const bool compact = s.size == MeterSizeMode::compact;

    auto r = area.reduced (Layout::marginX, Layout::marginY);

    const juce::Font valueFont = MeterTheme::mediumFont (compact ? 46.0f : 32.0f);
    const float labelFont = compact ? 14.0f : 13.0f;
    const float tickFont  = compact ? 13.0f : 12.0f;

    // Size the reading to its own text (plus a small pad) so the meter can take
    // all remaining width and the gap before the reading stays small.
    const juce::String valueText = formatValue (hasValue, value);
    const int valueWidth = juce::jmax (textWidth (valueFont, valueText),
                                       textWidth (valueFont, "-00.0")) + 8;

    const int labelWidth = s.showLabel ? juce::jmin (66, juce::jmax (34, r.getWidth() / 6)) : 0;

    // Guarantee a usable meter on narrow windows.
    const int minBarWidth = 60;
    const int valueW = juce::jmin (valueWidth, juce::jmax (40, r.getWidth() - labelWidth - minBarWidth));

    juce::Rectangle<int> labelArea;
    if (s.showLabel)
        labelArea = r.removeFromLeft (labelWidth);

    auto valueArea = r.removeFromRight (valueW);
    auto barArea = r;

    // Shared visual centreline: label, meter and reading all centre on this Y.
    // The scale ruler/ticks are an independent group below the meter.
    const float height = (float) barArea.getHeight();
    const float scaleReserve = s.showScale ? tickFont + 12.0f : 0.0f;
    const float contentHeight = juce::jmax (1.0f, height - scaleReserve);
    const float centreY = (float) barArea.getY() + contentHeight * 0.5f;
    const float barThickness = juce::jlimit (12.0f, compact ? 30.0f : 22.0f, contentHeight * 0.9f);

    juce::Rectangle<float> bar ((float) barArea.getX(),
                                centreY - barThickness * 0.5f,
                                (float) barArea.getWidth(),
                                barThickness);

    const float pitch = bar.getWidth() / (float) segmentCount;
    const float gap = juce::jlimit (1.0f, 3.0f, pitch * 0.3f);

    drawSegments (g, s, hasValue, value, pitch, gap,
                  [&] (int i, float p, float g2)
                  {
                      return juce::Rectangle<float> (bar.getX() + (float) i * p + g2 * 0.5f,
                                                     bar.getY(),
                                                     juce::jmax (1.0f, p - g2),
                                                     bar.getHeight());
                  });

    if (hasValue)
    {
        const float x = bar.getX() + proportion (value) * bar.getWidth();
        g.setColour (MeterTheme::marker);
        g.fillRect (juce::Rectangle<float> (x - 1.0f, bar.getY() - 4.0f, 2.0f, bar.getHeight() + 8.0f));
    }

    if (s.showScale)
    {
        g.setFont (MeterTheme::regularFont (tickFont));

        const int tickLabelW = 34;
        const int tickLabelH = 16;
        const double step = chooseTickStep (bar.getWidth(), tickLabelW + 4);

        for (double v = MeterSettings::scaleMinLufs; v <= MeterSettings::scaleMaxLufs + 0.001; v += step)
        {
            const float x = bar.getX() + proportion (v) * bar.getWidth();

            g.setColour (MeterTheme::ruler);
            g.fillRect (juce::Rectangle<float> (x - 0.5f, bar.getBottom() + 3.0f, 1.0f, 4.0f));

            const int labelX = juce::jlimit (0, juce::jmax (0, getWidth() - tickLabelW),
                                             (int) std::lround (x) - tickLabelW / 2);
            g.setColour (MeterTheme::textSecondary);
            g.drawText (tickText (v),
                        juce::Rectangle<int> (labelX, (int) bar.getBottom() + 8, tickLabelW, tickLabelH),
                        juce::Justification::centred, false);
        }
    }

    if (s.showLabel)
    {
        g.setColour (MeterTheme::textDim);
        g.setFont (MeterTheme::regularFont (labelFont));
        g.drawText (modeLabel, rowAtCentreY (labelArea, centreY, (int) labelFont + 6),
                    juce::Justification::centredLeft, false);
    }

    g.setColour (hasValue ? colourForZone (s.zoneForValue (value), s) : MeterTheme::textDim);
    g.setFont (valueFont);
    g.drawFittedText (valueText, rowAtCentreY (valueArea, centreY, (int) std::lround (valueFont.getHeight()) + 6),
                      juce::Justification::centredRight, 1, 0.6f);
}

void MeterComponent::paintVertical (juce::Graphics& g, juce::Rectangle<int> area,
                                    bool hasValue, double value)
{
    const auto s = settings_.sanitised();
    const bool compact = s.size == MeterSizeMode::compact;

    auto r = area.reduced (Layout::marginX, Layout::marginY);

    const juce::Font valueFont = MeterTheme::mediumFont (compact ? 40.0f : 32.0f);
    const int labelFont = compact ? 15 : 14;
    const float tickFont = compact ? 13.0f : 12.0f;

    // Bottom stack, top-to-bottom: meter, small gap, reading, tiny gap, label.
    const int valueRowHeight = (int) std::lround (valueFont.getHeight());
    const int labelRowHeight = labelFont + 6;
    const int gapMeterToValue = compact ? 4 : 3;
    const int gapValueToLabel = 2;

    juce::Rectangle<int> labelArea;
    if (s.showLabel)
        labelArea = r.removeFromBottom (labelRowHeight);
    r.removeFromBottom (gapValueToLabel);
    const auto valueArea = r.removeFromBottom (valueRowHeight);
    r.removeFromBottom (gapMeterToValue);
    const auto meterArea = r;

    // The meter is centred in the content area; the ruler is an independent
    // element drawn to its right and does not shift the meter.
    const float meterCentreX = (float) meterArea.getCentreX();
    const float barThickness = juce::jlimit (40.0f, compact ? 58.0f : 46.0f,
                                             (float) meterArea.getWidth() * 0.32f);

    juce::Rectangle<float> bar (meterCentreX - barThickness * 0.5f,
                                (float) meterArea.getY(),
                                barThickness,
                                (float) meterArea.getHeight());

    const float pitch = bar.getHeight() / (float) segmentCount;
    const float gap = juce::jlimit (1.0f, 3.0f, pitch * 0.3f);

    drawSegments (g, s, hasValue, value, pitch, gap,
                  [&] (int i, float p, float g2)
                  {
                      return juce::Rectangle<float> (bar.getX(),
                                                     bar.getBottom() - (float) (i + 1) * p + g2 * 0.5f,
                                                     bar.getWidth(),
                                                     juce::jmax (1.0f, p - g2));
                  });

    if (hasValue)
    {
        const float y = bar.getBottom() - proportion (value) * bar.getHeight();
        g.setColour (MeterTheme::marker);
        g.fillRect (juce::Rectangle<float> (bar.getX() - 4.0f, y - 1.0f,
                                            bar.getWidth() + 8.0f, 2.0f));
    }

    if (s.showScale)
    {
        g.setFont (MeterTheme::regularFont (tickFont));
        const float tickX = bar.getRight() + 6.0f;
        const int tickLabelH = 16;
        const int labelW = juce::jmax (20, getWidth() - (int) tickX - 9);
        const double step = chooseTickStep (bar.getHeight(), tickLabelH + 4);

        for (double v = MeterSettings::scaleMinLufs; v <= MeterSettings::scaleMaxLufs + 0.001; v += step)
        {
            const float y = bar.getBottom() - proportion (v) * bar.getHeight();

            g.setColour (MeterTheme::ruler);
            g.fillRect (juce::Rectangle<float> (tickX, y - 0.5f, 4.0f, 1.0f));

            const int labelY = juce::jlimit (0, juce::jmax (0, getHeight() - tickLabelH),
                                             (int) std::lround (y) - tickLabelH / 2);
            g.setColour (MeterTheme::textSecondary);
            g.drawText (tickText (v),
                        juce::Rectangle<int> ((int) tickX + 5, labelY, labelW, tickLabelH),
                        juce::Justification::centredLeft, false);
        }
    }

    // Reading and label share the meter's centreline and form a compact group.
    g.setColour (hasValue ? colourForZone (s.zoneForValue (value), s) : MeterTheme::textDim);
    g.setFont (valueFont);
    g.drawFittedText (formatValue (hasValue, value), valueArea,
                      juce::Justification::centred, 1, 0.6f);

    if (s.showLabel)
    {
        g.setColour (MeterTheme::textDim);
        g.setFont (MeterTheme::regularFont ((float) labelFont));
        g.drawText (modeLabel, labelArea, juce::Justification::centred, false);
    }
}
