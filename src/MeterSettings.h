#pragma once

#include <juce_core/juce_core.h>

/** Display orientation of the meter. */
enum class MeterOrientation
{
    horizontal = 0,
    vertical   = 1
};

/** Display size preset. */
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

    MeterOrientation orientation = MeterOrientation::horizontal;
    MeterSizeMode size = MeterSizeMode::compact;
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
        s.greenLow     = juce::jlimit (scaleMinLufs, scaleMaxLufs, s.greenLow);
        s.greenHigh    = juce::jlimit (s.greenLow, scaleMaxLufs, s.greenHigh);
        s.redThreshold = juce::jlimit (s.greenHigh, scaleMaxLufs, s.redThreshold);
        return s;
    }

    bool operator== (const MeterSettings& other) const noexcept
    {
        return orientation == other.orientation
            && size == other.size
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
        xml->setAttribute ("orientation", static_cast<int> (orientation));
        xml->setAttribute ("size", static_cast<int> (size));
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
        s.orientation = static_cast<MeterOrientation> (juce::jlimit (0, 1, xml.getIntAttribute ("orientation", 0)));
        s.size        = static_cast<MeterSizeMode> (juce::jlimit (0, 1, xml.getIntAttribute ("size", 1)));
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
