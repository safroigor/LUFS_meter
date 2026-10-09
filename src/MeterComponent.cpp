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

    // Shared typography and spacing for the secondary readouts (TP, LS, LM),
    // used identically in both orientations: values are deliberately larger
    // than their labels, and each label sits close to its own value.
    namespace Readout
    {
        constexpr float labelFontHeight = 14.0f;
        constexpr float valueFontHeight = 22.0f;
        constexpr int   labelGap = 6;   // label -> value, horizontal row
        constexpr int   itemGap  = 18;  // group -> group, horizontal row
        constexpr int   lineGap  = 2;   // label line -> value line, vertical
        constexpr int   groupGap = 18;  // group -> group, vertical
        constexpr int   horizontalTopInset = 1; // row top -> first text line
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

    // An em dash (U+2014) for an unavailable secondary measurement. Encoded as
    // UTF-8 bytes so the source file's encoding cannot corrupt it.
    const juce::String& placeholderText()
    {
        static const juce::String dash (juce::CharPointer_UTF8 ("\xe2\x80\x94"));
        return dash;
    }

    juce::String formatSecondary (bool hasValue, double value)
    {
        return hasValue ? juce::String (value, 1) : placeholderText();
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
        return (int) juce::GlyphArrangement::getStringWidth (font, text);
    }

    /** Vertical shift that moves `text` so its *visual* (ink) centre lands on
        the centre of the drawing rect. JUCE vertically centres the font's
        ascent/descent line box; for digit-only strings (no descenders) the
        visible digits then sit below the centre. We measure a digit's real ink
        centre from the typeface (normalised to point size 1.0) and add the
        font's line-box offset. */
    float glyphCentreShift (const juce::Font& font, const juce::String& text)
    {
        juce::ignoreUnused (text);

        if (auto typeface = font.getTypefacePtr())
            if (auto glyph = typeface->getNominalGlyphForCodepoint ('0'))
            {
                const float inkCentre = typeface->getGlyphBounds ((int) *glyph)
                                            .getCentreY() * font.getHeightInPoints();
                return (font.getAscent() - font.getDescent()) * 0.5f + inkCentre;
            }

        return 0.0f;
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
    const auto reading = engine_.getReading();

    if (settings_.orientation == MeterOrientation::horizontal)
        paintHorizontal (g, getLocalBounds(), reading);
    else
        paintVertical (g, getLocalBounds(), reading);
}

void MeterComponent::paintHorizontal (juce::Graphics& g, juce::Rectangle<int> area,
                                      const MeasurementEngine::Reading& reading)
{
    const auto s = settings_.sanitised();
    const bool compact = s.size == MeterSizeMode::compact;

    const bool hasValue = reading.hasIntegrated;
    const double value = roundToDisplay (reading.integrated);

    auto r = area.reduced (Layout::marginX, Layout::marginY);

    // The compact secondary readouts sit as a row above the segmented scale.
    // The row is drawn below, centred on the segmented scale's own horizontal
    // bounds, so it stays aligned with the meter at any window width.
    juce::Rectangle<int> readoutArea;
    const bool showReadouts = r.getHeight() > secondaryRowHeight + 8;

    if (showReadouts)
        readoutArea = r.removeFromTop (secondaryRowHeight);

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

    // Centre the whole secondary group on the segmented scale's centre; the
    // group stays inside the full row so it cannot clip at small widths.
    if (showReadouts)
        paintReadouts (g, readoutArea, reading, true, barArea.getCentreX());

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
    // Align the digits' visual (ink) centre with the meter's centre, not the
    // font's ascent/descent line box.
    const float valueCentreY = centreY - glyphCentreShift (valueFont, valueText);
    g.drawFittedText (valueText, rowAtCentreY (valueArea, valueCentreY, (int) std::lround (valueFont.getHeight()) + 6),
                      juce::Justification::centredRight, 1, 0.6f);
}

void MeterComponent::paintVertical (juce::Graphics& g, juce::Rectangle<int> area,
                                    const MeasurementEngine::Reading& reading)
{
    const auto s = settings_.sanitised();
    const bool compact = s.size == MeterSizeMode::compact;

    const bool hasValue = reading.hasIntegrated;
    const double value = roundToDisplay (reading.integrated);

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

    // Secondary readouts in a compact column to the left of the segmented meter.
    // The meter keeps its centred position; the column uses the empty space that
    // already exists to its left.
    const int readoutGap = 6;
    const int readoutWidth = juce::jmax (0, (int) std::lround (bar.getX()) - readoutGap
                                              - meterArea.getX());
    paintReadouts (g, juce::Rectangle<int> (meterArea.getX(), meterArea.getY(),
                                            readoutWidth, meterArea.getHeight()),
                   reading, false, 0);
}

void MeterComponent::paintReadouts (juce::Graphics& g, juce::Rectangle<int> area,
                                    const MeasurementEngine::Reading& reading,
                                    bool horizontalRow, int referenceCentreX)
{
    if (area.getWidth() <= 0 || area.getHeight() <= 0)
        return;

    struct Item
    {
        const char* label;
        juce::String value;
        juce::Colour colour;
    };

    const auto normal = MeterTheme::textSecondary;
    const auto red = juce::Colour (settings_.redColour);

    // Display order is TP, LS, LM. LS maps to the Short-Term measurement and LM
    // to the Momentary measurement; only the display labels differ from the
    // internal names.
    const Item items[] =
    {
        { "TP", formatSecondary (reading.hasTruePeak,  reading.truePeak),
                (reading.hasTruePeak && reading.truePeak >= 0.0) ? red : normal },
        { "LS", formatSecondary (reading.hasShortTerm,  reading.shortTerm), normal },
        { "LM", formatSecondary (reading.hasMomentary,  reading.momentary), normal },
    };

    constexpr int count = 3;

    const juce::Font labelFont = MeterTheme::regularFont (Readout::labelFontHeight);
    const juce::Font valueFont = MeterTheme::mediumFont (Readout::valueFontHeight);

    if (horizontalRow)
    {
        // One line per group: a smaller label followed by a larger value, all
        // vertically centred on a shared centreline. The whole group is centred
        // horizontally within `area` (which is the segmented scale's x-range).
        // Hug the top of the reserved row so the space above the readouts is no
        // larger than the space below the scale, leaving a clear, balanced gap
        // between the readout row and the meter.
        const int valueRowH = (int) std::lround (valueFont.getHeight()) + 4;
        const float centreY = (float) area.getY() + (float) valueRowH * 0.5f
                                  + (float) Readout::horizontalTopInset;
        const int valueW = textWidth (valueFont, "-000.0");

        int totalW = Readout::itemGap * (count - 1);

        for (int i = 0; i < count; ++i)
            totalW += textWidth (labelFont, items[i].label) + Readout::labelGap + valueW;

        // Centre the group on the scale's centre, but never let it leave `area`.
        const int maxX = juce::jmax (area.getX(), area.getRight() - totalW);
        int x = juce::jlimit (area.getX(), maxX, referenceCentreX - totalW / 2);

        for (int i = 0; i < count; ++i)
        {
            const int labelW = textWidth (labelFont, items[i].label);

            g.setFont (labelFont);
            g.setColour (MeterTheme::textDim);
            g.drawFittedText (items[i].label,
                              rowAtCentreY ({ x, area.getY(), labelW, area.getHeight() },
                                            centreY, (int) std::lround (labelFont.getHeight()) + 2),
                              juce::Justification::centredLeft, 1, 0.7f);

            g.setFont (valueFont);
            g.setColour (items[i].colour);
            g.drawFittedText (items[i].value,
                              rowAtCentreY ({ x + labelW + Readout::labelGap, area.getY(),
                                              valueW, area.getHeight() },
                                            centreY, (int) std::lround (valueFont.getHeight()) + 4),
                              juce::Justification::centredLeft, 1, 0.8f);

            x += labelW + Readout::labelGap + valueW + Readout::itemGap;
        }

        return;
    }

    // Vertical: three compact two-line groups (label directly above a larger
    // value), stacked TP, LS, LM and centred in the left-hand column. The line
    // heights come from the fonts themselves rather than being stretched to fill
    // the column, so each label stays visually attached to its own value; the
    // whole stack is scaled down only if it would not otherwise fit.
    const float naturalLabel    = (float) labelFont.getHeight();
    const float naturalValue    = (float) valueFont.getHeight();
    const float naturalLineGap  = (float) Readout::lineGap;
    const float naturalGroupGap = (float) Readout::groupGap;
    const float naturalUnit     = naturalLabel + naturalLineGap + naturalValue;
    const float naturalTotal    = naturalUnit * count + naturalGroupGap * (count - 1);

    const float k = naturalTotal > (float) area.getHeight()
                        ? (float) area.getHeight() / naturalTotal
                        : 1.0f;

    const int labelLine = (int) std::lround (naturalLabel * k);
    const int valueLine = (int) std::lround (naturalValue * k);
    const int lineGap   = (int) std::lround (naturalLineGap * k);
    const int groupGap  = (int) std::lround (naturalGroupGap * k);
    const int groupH    = labelLine + lineGap + valueLine;
    const int totalH    = groupH * count + groupGap * (count - 1);

    int y = area.getY() + juce::jmax (0, (area.getHeight() - totalH) / 2);

    for (int i = 0; i < count; ++i)
    {
        g.setFont (labelFont);
        g.setColour (MeterTheme::textDim);
        g.drawFittedText (items[i].label,
                          { area.getX(), y, area.getWidth(), labelLine },
                          juce::Justification::centred, 1, 0.7f);

        g.setFont (valueFont);
        g.setColour (items[i].colour);
        g.drawFittedText (items[i].value,
                          { area.getX(), y + labelLine + lineGap,
                            area.getWidth(), valueLine },
                          juce::Justification::centred, 1, 0.8f);

        y += groupH + groupGap;
    }
}
