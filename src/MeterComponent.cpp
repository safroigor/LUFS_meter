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
        constexpr int   marginX = 6;
        constexpr int   marginY = 6;
        constexpr float segmentRadius = 3.0f;
        constexpr double tickStep = 10.0;       // LUFS between scale labels

        // Compact mode: the continuous fill spans the whole main display area.
        constexpr float compactFillRadius = 3.0f;
        constexpr float compactFillAlpha  = 0.18f; // substantially subtler than the old segmented fill
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

    // Compact mode arranges TP / LS / LM as three label-over-value columns. The
    // value type starts a little larger than Vertical's readouts (targeting a
    // noticeably larger secondary value) and only shrinks if a column would not
    // fit the value safely.
    namespace CompactReadout
    {
        constexpr float maxFontScale = 1.10f;
        constexpr float minFontScale = 0.62f;
    }

    // Compact mode's focal reading sits on a capsule in the window colour, so
    // the fill's moving edge passes behind the digits instead of slicing
    // through them. The capsule is sized to a full "-00.0" so it keeps the same
    // width as the digits change, with a visible margin around them.
    namespace CompactReading
    {
        constexpr int   pillPadX = 10;
        constexpr int   pillPadY = 5;
        constexpr float pillAlpha = 0.9f;
        constexpr float minFontHeight = 28.0f;
        constexpr float maxFontHeight = 54.0f;
    }

    const char* const modeLabel = "LUFS-I";

    float proportion (double value)
    {
        return (float) MeterSettings::scaleProportion (value);
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

    /** Ruler label for a scale mark. Horizontal prints the sign, so the axis
        under the bar reads unambiguously ("-50 … 0"); the vertical column keeps
        magnitudes only, where a repeated minus beside the meter reads as noise.
        Zero never takes a sign. */
    juce::String tickText (double value, bool withSign)
    {
        const int magnitude = (int) std::lround (std::abs (value));

        if (! withSign || magnitude == 0)
            return juce::String (magnitude);

        return juce::String (value < 0.0 ? "-" : "") + juce::String (magnitude);
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
        `minSpacingPx` between adjacent labels, so larger fonts never overlap and
        a narrow window simply shows a sparser ruler. */
    double chooseTickStep (float lengthPx, int minSpacingPx)
    {
        const double range = MeterSettings::scaleMaxLufs - MeterSettings::scaleMinLufs;

        for (double step : { Layout::tickStep, 15.0, 25.0, 50.0 })
            if (lengthPx * (float) (step / range) >= (float) minSpacingPx)
                return step;

        return 50.0;
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

    // --- Compact mode: continuous moving fill --------------------------------

    /** Fills one continuous bottom-anchored shape whose top edge is at `top`.
        Only the top corners are rounded, matching the panel language, and the
        radius collapses on very short fills so the shape never distorts. */
    void fillContinuousFill (juce::Graphics& g, juce::Rectangle<float> area, float top)
    {
        const float radius = juce::jlimit (0.0f, Layout::compactFillRadius,
                                           (area.getBottom() - top) * 0.5f);

        juce::Path path;
        path.startNewSubPath (area.getX(), area.getBottom());
        path.lineTo (area.getX(), top + radius);
        path.quadraticTo (area.getX(), top, area.getX() + radius, top);
        path.lineTo (area.getRight() - radius, top);
        path.quadraticTo (area.getRight(), top, area.getRight(), top + radius);
        path.lineTo (area.getRight(), area.getBottom());
        path.closeSubPath();

        g.fillPath (path);
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

void MeterComponent::setControlInsets (int leftInset, int rightInset)
{
    const int left  = juce::jmax (0, leftInset);
    const int right = juce::jmax (0, rightInset);

    if (left == controlInsetLeft_ && right == controlInsetRight_)
        return;

    controlInsetLeft_  = left;
    controlInsetRight_ = right;
    repaint();
}

void MeterComponent::advanceAnimation (double deltaSeconds)
{
    const auto reading = engine_.getReading();

    if (! reading.hasIntegrated)
    {
        // A pass with nothing measured yet — a seek, playback restarting, or the
        // very first block. Drop back to the start of the scale, so a new pass
        // is unmistakable: the indicator visibly begins again from nothing, and
        // the first value of the pass then fills up to it.
        animation_.clear();
        hadValue_ = false;
        return;
    }

    if (! hadValue_)
    {
        // The first value of the pass — or the first frame after the editor
        // opened in the middle of one: fill up from the bottom of the scale.
        animation_.start (MeterSettings::scaleMinLufs, reading.integrated);
        hadValue_ = true;
    }
    else
    {
        animation_.setTarget (reading.integrated);
    }

    animation_.advance (deltaSeconds);
}

void MeterComponent::settleAnimation()
{
    // Well past the fill duration: whatever the measurement is now, this lands
    // the animation on it without drawing a frame of the sweep.
    advanceAnimation (ValueAnimation::defaultDurationSeconds * 4.0);
}

void MeterComponent::resetDisplay()
{
    animation_.clear();
    hadValue_ = false;
    repaint();
}

void MeterComponent::paint (juce::Graphics& g)
{
    auto reading = engine_.getReading();

    // A pass fills in rather than jumping, and holds the last reading until it
    // has one of its own (see ValueAnimation). Everything drawn below —
    // segmented bar, Compact fill, marker, reading and its zone colour — uses
    // this one value, so they can never disagree.
    if (animation_.hasValue())
    {
        reading.hasIntegrated = true;
        reading.integrated = animation_.value();
    }

    switch (settings_.displayMode)
    {
        case MeterDisplayMode::vertical:
            paintVertical (g, getLocalBounds(), reading);
            break;

        case MeterDisplayMode::compact:
            paintCompact (g, getLocalBounds(), reading);
            break;

        case MeterDisplayMode::horizontal:
        default:
            paintHorizontal (g, getLocalBounds(), reading);
            break;
    }
}

void MeterComponent::paintHorizontal (juce::Graphics& g, juce::Rectangle<int> area,
                                      const MeasurementEngine::Reading& reading)
{
    const auto s = settings_.sanitised();

    const bool hasValue = reading.hasIntegrated;
    const double value = roundToDisplay (reading.integrated);

    auto r = area.reduced (Layout::marginX, Layout::marginY);

    // The secondary readouts sit as a row above the segmented scale. In this
    // mode the editor overlays its icon buttons on the window's top corners, so
    // the row is laid out inside the insets those buttons reserve. The row is
    // much wider than the scale, so centring it on its own (already inset) area
    // — rather than on the scale — is what keeps it clear of the buttons at
    // every allowed width.
    juce::Rectangle<int> readoutArea;
    const bool showReadouts = r.getHeight() > secondaryRowHeight + 8;

    if (showReadouts)
        readoutArea = r.removeFromTop (secondaryRowHeight)
                          .withTrimmedLeft (controlInsetLeft_)
                          .withTrimmedRight (controlInsetRight_);

    const juce::Font valueFont = MeterTheme::mediumFont (46.0f);
    const float labelFont = 13.0f;
    const float tickFont  = 12.0f;

    // The readout row's type is a little smaller than the v1.2.0 sizes: the
    // window is shorter now, and this keeps both the scale and the reading
    // their room without competing with the button row beside them.
    constexpr float readoutScale = 0.9f;

    // Size the reading to its own text (plus a small pad) so the meter can take
    // all remaining width and the gap before the reading stays small.
    const juce::String valueText = formatValue (hasValue, value);
    const int valueWidth = juce::jmax (textWidth (valueFont, valueText),
                                       textWidth (valueFont, "-00.0")) + 8;

    const int labelWidth = s.showLabel ? juce::jmin (56, juce::jmax (32, r.getWidth() / 7)) : 0;

    // Guarantee a usable meter on narrow windows.
    const int minBarWidth = 60;
    const int valueW = juce::jmin (valueWidth, juce::jmax (40, r.getWidth() - labelWidth - minBarWidth));

    juce::Rectangle<int> labelArea;
    if (s.showLabel)
        labelArea = r.removeFromLeft (labelWidth);

    auto valueArea = r.removeFromRight (valueW);
    auto barArea = r;

    // Centre the whole secondary group on its own row; the group stays inside
    // the insets so it can neither clip at small widths nor meet the buttons.
    if (showReadouts)
        paintReadouts (g, readoutArea, reading, ReadoutLayout::horizontalRow,
                       readoutArea.getCentreX(), readoutScale, MeterTheme::textDim);

    // Shared visual centreline: label, meter and reading all centre on this Y.
    // The scale ruler/ticks are an independent group below the meter.
    const float height = (float) barArea.getHeight();
    const float scaleReserve = s.showScale ? tickFont + 12.0f : 0.0f;
    const float contentHeight = juce::jmax (1.0f, height - scaleReserve);
    const float centreY = (float) barArea.getY() + contentHeight * 0.5f;
    const float barThickness = juce::jlimit (12.0f, 30.0f, contentHeight * 0.9f);

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

        // Wide enough for a signed label ("-50").
        const int tickLabelW = 38;
        const int tickLabelH = 16;
        const double step = chooseTickStep (bar.getWidth(), tickLabelW + 4);

        // Counted down from the top, so the maximum is always labelled and the
        // marks stay on round values at every step.
        for (double v = MeterSettings::scaleMaxLufs; v >= MeterSettings::scaleMinLufs - 0.001; v -= step)
        {
            const float x = bar.getX() + proportion (v) * bar.getWidth();

            g.setColour (MeterTheme::ruler);
            g.fillRect (juce::Rectangle<float> (x - 0.5f, bar.getBottom() + 3.0f, 1.0f, 4.0f));

            const int labelX = juce::jlimit (0, juce::jmax (0, getWidth() - tickLabelW),
                                             (int) std::lround (x) - tickLabelW / 2);
            g.setColour (MeterTheme::textSecondary);
            g.drawText (tickText (v, true),
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

    const bool hasValue = reading.hasIntegrated;
    const double value = roundToDisplay (reading.integrated);

    auto r = area.reduced (Layout::marginX, Layout::marginY);

    // Same focal size as Horizontal: the vertical meter has the width for it.
    const juce::Font valueFont = MeterTheme::mediumFont (46.0f);
    const int labelFont = 15;
    const float tickFont = 12.0f;

    // Bottom stack, top-to-bottom: meter, small gap, reading, tiny gap, label.
    const int valueRowHeight = (int) std::lround (valueFont.getHeight());
    const int labelRowHeight = labelFont + 6;
    const int gapMeterToValue = 4;
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
    // Slimmer than v1.2.0: this is what lets the slim window keep its narrow
    // TP / LS / LM column beside the meter instead of stealing width from it.
    const float barThickness = juce::jlimit (30.0f, 44.0f,
                                             (float) meterArea.getWidth() * 0.30f);

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

        // Counted down from the top, so the maximum is always labelled and the
        // marks stay on round values at every step.
        for (double v = MeterSettings::scaleMaxLufs; v >= MeterSettings::scaleMinLufs - 0.001; v -= step)
        {
            const float y = bar.getBottom() - proportion (v) * bar.getHeight();

            g.setColour (MeterTheme::ruler);
            g.fillRect (juce::Rectangle<float> (tickX, y - 0.5f, 4.0f, 1.0f));

            const int labelY = juce::jlimit (0, juce::jmax (0, getHeight() - tickLabelH),
                                             (int) std::lround (y) - tickLabelH / 2);
            g.setColour (MeterTheme::textSecondary);
            g.drawText (tickText (v, false),
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
    constexpr float readoutScale = 0.9f;
    const int readoutGap = 6;
    const int readoutWidth = juce::jmax (0, (int) std::lround (bar.getX()) - readoutGap
                                              - meterArea.getX());
    paintReadouts (g, juce::Rectangle<int> (meterArea.getX(), meterArea.getY(),
                                            readoutWidth, meterArea.getHeight()),
                   reading, ReadoutLayout::verticalColumn, 0, readoutScale, MeterTheme::textDim);
}

void MeterComponent::paintCompact (juce::Graphics& g, juce::Rectangle<int> area,
                                   const MeasurementEngine::Reading& reading)
{
    const auto s = settings_.sanitised();
    const bool hasValue = reading.hasIntegrated;
    const double value = roundToDisplay (reading.integrated);

    // 1. No panel backdrop: the meter draws on the window background, so the
    //    only thing tinting the area is the measurement's own fill. (A grey
    //    rectangle behind everything read as a dirty wash across the whole
    //    area.)

    // 2. One continuous, bottom-anchored fill: the main display background. Its
    //    top edge moves with the Integrated value using the shared scale mapping
    //    and the configured zone colours. No fill when there is no valid value.
    if (hasValue)
    {
        const float fillHeight = (float) MeterSettings::scaleProportion (value)
                                     * (float) area.getHeight();

        if (fillHeight > 0.5f)
        {
            g.setColour (colourForZone (s.zoneForValue (value), s)
                             .withMultipliedAlpha (Layout::compactFillAlpha));

            fillContinuousFill (g, area.toFloat(), (float) area.getBottom() - fillHeight);
        }
    }

    // 3/4. Text on top of the fill. The secondary row is three two-line columns
    //      (label above value); choose the largest value type that fits one
    //      column, then give the LUFS-I reading the remaining height.
    const int slot = juce::jmax (1, area.getWidth() / 3);
    const int valueBudget = slot - 6;

    auto compactValueWidth = [] (float fontScale)
    {
        return textWidth (MeterTheme::mediumFont (Readout::valueFontHeight * fontScale), "-00.0");
    };

    float readoutScale = CompactReadout::maxFontScale;

    while (readoutScale > CompactReadout::minFontScale
               && compactValueWidth (readoutScale) > valueBudget)
        readoutScale -= 0.02f;

    const juce::Font readoutValueFont = MeterTheme::mediumFont (Readout::valueFontHeight * readoutScale);
    const juce::Font readoutLabelFont = MeterTheme::regularFont (Readout::labelFontHeight * readoutScale);
    const int readoutBand = (int) std::lround (readoutLabelFont.getHeight())
                            + juce::jmax (1, juce::roundToInt (Readout::lineGap * readoutScale))
                            + (int) std::lround (readoutValueFont.getHeight())
                            + 4;

    auto textArea = area;
    auto readoutArea = textArea.removeFromBottom (readoutBand);

    if (s.showLabel)
    {
        auto labelArea = textArea.removeFromTop (juce::jmin (14, juce::jmax (10, textArea.getHeight() / 6)));
        g.setColour (MeterTheme::textDim);
        g.setFont (MeterTheme::regularFont (11.5f));
        g.drawText (modeLabel, labelArea, juce::Justification::centred, false);
    }

    // The main reading stays centred in the space above the secondary row, in
    // the capsule described above. Its type comes from the available height and
    // only shrinks if the digits would not fit the capsule's margin budget — the
    // editor's Reset button reserves the sides at the smallest window sizes.
    const auto readingArea = textArea.withTrimmedLeft (controlInsetLeft_)
                                     .withTrimmedRight (controlInsetRight_);

    const int textBudget = juce::jmax (40, readingArea.getWidth()
                                              - CompactReading::pillPadX * 2);
    float fontHeight = juce::jlimit (CompactReading::minFontHeight,
                                     CompactReading::maxFontHeight,
                                     (float) textArea.getHeight() * 0.64f);

    while (fontHeight > CompactReading::minFontHeight
               && textWidth (MeterTheme::mediumFont (fontHeight), "-00.0") > textBudget)
        fontHeight -= 1.0f;

    const juce::Font valueFont = MeterTheme::mediumFont (fontHeight);
    const juce::String valueText = formatValue (hasValue, value);

    if (readingArea.getWidth() > 0 && readingArea.getHeight() > 0)
    {
        const int pillW = juce::jmin (readingArea.getWidth(),
                                      textWidth (valueFont, "-00.0")
                                          + CompactReading::pillPadX * 2);
        const int pillH = juce::jmin (readingArea.getHeight(),
                                      (int) std::lround (valueFont.getHeight())
                                          + CompactReading::pillPadY * 2);
        const auto pill = readingArea.withSizeKeepingCentre (pillW, pillH);

        g.setColour (MeterTheme::windowBackground.withAlpha (CompactReading::pillAlpha));
        g.fillRoundedRectangle (pill.toFloat(), (float) pill.getHeight() * 0.5f);

        g.setColour (hasValue ? colourForZone (s.zoneForValue (value), s) : MeterTheme::textDim);
        g.setFont (valueFont);
        g.drawFittedText (valueText, pill, juce::Justification::centred, 1, 0.5f);
    }

    paintReadouts (g, readoutArea, reading, ReadoutLayout::columns,
                   readoutArea.getCentreX(), readoutScale, MeterTheme::textSecondary);
}

void MeterComponent::paintReadouts (juce::Graphics& g, juce::Rectangle<int> area,
                                    const MeasurementEngine::Reading& reading,
                                    ReadoutLayout layout, int referenceCentreX,
                                    float fontScale, juce::Colour labelColour)
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
    const auto truePeakRedAt = settings_.truePeakRedThresholdDbtp;

    // Display order is TP, LS, LM. LS maps to the Short-Term measurement and LM
    // to the Momentary measurement; only the display labels differ from the
    // internal names.
    const Item items[] =
    {
        { "TP", formatSecondary (reading.hasTruePeak,  reading.truePeak),
                (reading.hasTruePeak && reading.truePeak >= truePeakRedAt) ? red : normal },
        { "LS", formatSecondary (reading.hasShortTerm,  reading.shortTerm), normal },
        { "LM", formatSecondary (reading.hasMomentary,  reading.momentary), normal },
    };

    constexpr int count = 3;

    const juce::Font labelFont = MeterTheme::regularFont (Readout::labelFontHeight * fontScale);
    const juce::Font valueFont = MeterTheme::mediumFont (Readout::valueFontHeight * fontScale);

    if (layout == ReadoutLayout::horizontalRow)
    {
        // One line per group: a smaller label followed by a larger value, all
        // vertically centred on a shared centreline. The whole group is centred
        // horizontally within `area` (which is the segmented scale's x-range).
        // Hug the top of the reserved row so the space above the readouts is no
        // larger than the space below the scale, leaving a clear, balanced gap
        // between the readout row and the meter.
        // The gaps shrink with the type, so a smaller readout row stays a
        // compact group instead of a row of loosely spaced values.
        const int labelGap = juce::jmax (1, juce::roundToInt (Readout::labelGap * fontScale));
        const int itemGap  = juce::jmax (1, juce::roundToInt (Readout::itemGap * fontScale));
        const int valueRowH = (int) std::lround (valueFont.getHeight()) + 4;
        const float centreY = (float) area.getY() + (float) valueRowH * 0.5f
                                  + (float) Readout::horizontalTopInset;
        const int valueW = textWidth (valueFont, "-000.0");

        int totalW = itemGap * (count - 1);

        for (int i = 0; i < count; ++i)
            totalW += textWidth (labelFont, items[i].label) + labelGap + valueW;

        // Centre the group on its row, but never let it leave `area`.
        const int maxX = juce::jmax (area.getX(), area.getRight() - totalW);
        int x = juce::jlimit (area.getX(), maxX, referenceCentreX - totalW / 2);

        for (int i = 0; i < count; ++i)
        {
            const int labelW = textWidth (labelFont, items[i].label);

            g.setFont (labelFont);
            g.setColour (labelColour);
            g.drawFittedText (items[i].label,
                              rowAtCentreY ({ x, area.getY(), labelW, area.getHeight() },
                                            centreY, (int) std::lround (labelFont.getHeight()) + 2),
                              juce::Justification::centredLeft, 1, 0.7f);

            g.setFont (valueFont);
            g.setColour (items[i].colour);
            g.drawFittedText (items[i].value,
                              rowAtCentreY ({ x + labelW + labelGap, area.getY(),
                                              valueW, area.getHeight() },
                                            centreY, (int) std::lround (valueFont.getHeight()) + 4),
                              juce::Justification::centredLeft, 1, 0.8f);

            x += labelW + labelGap + valueW + itemGap;
        }

        return;
    }

    if (layout == ReadoutLayout::columns)
    {
        // Compact: three equal columns, each a two-line group with the label
        // directly above its larger value, both centred in the column. This gives
        // the values the full column width (so they can be larger than in an
        // inline row) while keeping the groups evenly spread and collision-free.
        const int labelLine = (int) std::lround (labelFont.getHeight());
        const int valueLine = (int) std::lround (valueFont.getHeight());
        const int lineGap   = juce::jmax (1, juce::roundToInt (Readout::lineGap * fontScale));
        const int groupH    = labelLine + lineGap + valueLine;

        const int top = area.getY() + juce::jmax (0, (area.getHeight() - groupH) / 2);
        const float slot = (float) area.getWidth() / (float) count;

        for (int i = 0; i < count; ++i)
        {
            const int colX = area.getX() + (int) std::lround ((float) i * slot);
            const int colW = (int) std::lround (slot);

            g.setFont (labelFont);
            g.setColour (labelColour);
            g.drawFittedText (items[i].label, { colX, top, colW, labelLine },
                              juce::Justification::centred, 1, 0.7f);

            g.setFont (valueFont);
            g.setColour (items[i].colour);
            g.drawFittedText (items[i].value,
                              { colX, top + labelLine + lineGap, colW, valueLine },
                              juce::Justification::centred, 1, 0.8f);
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
        g.setColour (labelColour);
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
