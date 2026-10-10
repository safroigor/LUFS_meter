#pragma once

#include <juce_core/juce_core.h>

/** The three mutually exclusive display layouts.

    - vertical:   the existing vertical meter.
    - horizontal: the existing horizontal meter.
    - compact:    a small horizontal numeric layout whose entire background is a
                  segmented loudness indicator.

    This is the authoritative display setting. `MeterOrientation` and
    `MeterSizeMode` below are kept only as the saved-state vocabulary so old
    sessions still load (see `MeterSettings::fromXml`). */
enum class MeterDisplayMode
{
    vertical   = 0,
    horizontal = 1,
    compact    = 2
};

/** Legacy display orientation. Kept only as the saved-state vocabulary; the
    authoritative setting is `MeterDisplayMode`. Do not branch on this in new
    code. */
enum class MeterOrientation
{
    horizontal = 0,
    vertical   = 1
};

/** Legacy display size preset. Kept only as the saved-state vocabulary (see
    `MeterOrientation`). Slim and Compact both restore to their orientation's
    single `MeterDisplayMode`. */
enum class MeterSizeMode
{
    slim    = 0,
    compact = 1
};

/** Which colour zone a measured value falls into. */
enum class MeterZone
{
    normal = 0,
    green  = 1,
    red    = 2
};

/** Persistent UI settings of the meter.

    Colours are stored as plain ARGB integers (0xAARRGGBB) so this header has no
    graphics dependency and the zone logic can be unit-tested directly. The UI
    converts them to juce::Colour at the edge. */
struct MeterSettings
{
    /** Display scale in LUFS, shared by the meter drawing and the settings
        ranges. */
    static constexpr double scaleMinLufs = -30.0;
    static constexpr double scaleMaxLufs = 0.0;

    MeterDisplayMode displayMode = MeterDisplayMode::horizontal;
    bool showLabel = true;
    bool showScale = true;

    /** Colour zones in LUFS. Values below greenLow or above greenHigh (and below
        redThreshold) use the default colour; values >= redThreshold are red. */
    double greenLow     = -24.0;
    double greenHigh    = -14.0;
    double redThreshold = -14.0;

    juce::uint32 defaultColour = 0xffe6e6e6; // light gray
    juce::uint32 greenColour   = 0xff34c759; // green
    juce::uint32 redColour     = 0xffff3b30; // red

    /** Maps a LUFS value to its position on the display scale, clamped to
        [0, 1]. Shared by the segmented bar (Vertical/Horizontal) and the
        continuous Compact fill, so both use exactly the same mapping. */
    static double scaleProportion (double lufs) noexcept
    {
        return juce::jlimit (0.0, 1.0,
                             (lufs - scaleMinLufs) / (scaleMaxLufs - scaleMinLufs));
    }

    /** Classifies a measured value. Red takes precedence, so the zones stay
        well-defined even if the boundaries overlap. A tiny epsilon makes values
        that sit exactly on a boundary (after rounding to the displayed 0.1 LU)
        classify deterministically, so the numeric reading and the meter can
        never disagree because of floating-point noise. */
    MeterZone zoneForValue (double lufs) const noexcept
    {
        constexpr double epsilon = 1.0e-6;

        if (lufs >= redThreshold - epsilon)
            return MeterZone::red;

        if (lufs >= greenLow - epsilon && lufs <= greenHigh + epsilon)
            return MeterZone::green;

        return MeterZone::normal;
    }

    /** Returns a copy with the zone boundaries ordered (normal < green < red)
        and clamped to the display scale, so an inconsistent configuration can
        never be produced. */
    MeterSettings sanitised() const
    {
        MeterSettings s = *this;
        s.displayMode  = static_cast<MeterDisplayMode> (
            juce::jlimit (0, 2, static_cast<int> (s.displayMode)));
        s.greenLow     = juce::jlimit (scaleMinLufs, scaleMaxLufs, s.greenLow);
        s.greenHigh    = juce::jlimit (s.greenLow, scaleMaxLufs, s.greenHigh);
        s.redThreshold = juce::jlimit (s.greenHigh, scaleMaxLufs, s.redThreshold);
        return s;
    }

    bool operator== (const MeterSettings& other) const noexcept
    {
        return displayMode == other.displayMode
            && showLabel == other.showLabel
            && showScale == other.showScale
            && juce::approximatelyEqual (greenLow, other.greenLow)
            && juce::approximatelyEqual (greenHigh, other.greenHigh)
            && juce::approximatelyEqual (redThreshold, other.redThreshold)
            && defaultColour == other.defaultColour
            && greenColour == other.greenColour
            && redColour == other.redColour;
    }

    bool operator!= (const MeterSettings& other) const noexcept
    {
        return ! (*this == other);
    }

    std::unique_ptr<juce::XmlElement> toXml() const
    {
        auto xml = std::make_unique<juce::XmlElement> ("LUFSMeterSettings");
        xml->setAttribute ("displayMode", static_cast<int> (displayMode));

        // Legacy attributes, derived from the display mode, so an older build
        // that has never heard of `displayMode` still loads a sensible layout.
        // Compact is written as Horizontal, the closest legacy layout.
        const bool vertical = displayMode == MeterDisplayMode::vertical;
        xml->setAttribute ("orientation", vertical ? 1 : 0);
        xml->setAttribute ("size", static_cast<int> (MeterSizeMode::compact));

        xml->setAttribute ("showLabel", showLabel);
        xml->setAttribute ("showScale", showScale);
        xml->setAttribute ("greenLow", greenLow);
        xml->setAttribute ("greenHigh", greenHigh);
        xml->setAttribute ("redThreshold", redThreshold);
        xml->setAttribute ("defaultColour", colourToString (defaultColour));
        xml->setAttribute ("greenColour", colourToString (greenColour));
        xml->setAttribute ("redColour", colourToString (redColour));
        return xml;
    }

    static MeterSettings fromXml (const juce::XmlElement& xml)
    {
        MeterSettings s;

        // `displayMode` takes precedence whenever present. Repeated save/load
        // therefore round-trips the selected mode unchanged.
        if (xml.hasAttribute ("displayMode"))
        {
            s.displayMode = static_cast<MeterDisplayMode> (
                juce::jlimit (0, 2, xml.getIntAttribute ("displayMode", 1)));
        }
        else
        {
            // Migrate from the legacy orientation/size pair. Slim and Compact
            // both collapse to their orientation's single display mode; the size
            // attribute is intentionally ignored so the migration is
            // deterministic.
            const int legacyOrientation = xml.getIntAttribute ("orientation", 0);
            s.displayMode = legacyOrientation == static_cast<int> (MeterOrientation::vertical)
                                ? MeterDisplayMode::vertical
                                : MeterDisplayMode::horizontal;
        }

        s.showLabel   = xml.getBoolAttribute ("showLabel", true);
        s.showScale   = xml.getBoolAttribute ("showScale", true);
        s.greenLow     = xml.getDoubleAttribute ("greenLow", s.greenLow);
        s.greenHigh    = xml.getDoubleAttribute ("greenHigh", s.greenHigh);
        s.redThreshold = xml.getDoubleAttribute ("redThreshold", s.redThreshold);
        s.defaultColour = colourFromString (xml.getStringAttribute ("defaultColour"), s.defaultColour);
        s.greenColour   = colourFromString (xml.getStringAttribute ("greenColour"), s.greenColour);
        s.redColour     = colourFromString (xml.getStringAttribute ("redColour"), s.redColour);
        return s.sanitised();
    }

private:
    static juce::String colourToString (juce::uint32 argb)
    {
        // 8 hex digits: getHexValue32() reads the leading 8 digits only.
        return juce::String::toHexString (static_cast<int> (argb));
    }

    static juce::uint32 colourFromString (const juce::String& text, juce::uint32 fallback)
    {
        if (text.isEmpty())
            return fallback;

        return static_cast<juce::uint32> (text.getHexValue32());
    }
};
