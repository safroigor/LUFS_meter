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

/** Persistent UI settings of the meter. */
struct MeterSettings
{
    MeterOrientation orientation = MeterOrientation::horizontal;
    MeterSizeMode size = MeterSizeMode::compact;
    bool showLabel = true;
    bool showScale = true;

    bool operator== (const MeterSettings& other) const noexcept
    {
        return orientation == other.orientation
            && size == other.size
            && showLabel == other.showLabel
            && showScale == other.showScale;
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
        return xml;
    }

    static MeterSettings fromXml (const juce::XmlElement& xml)
    {
        MeterSettings s;
        s.orientation = static_cast<MeterOrientation> (juce::jlimit (0, 1, xml.getIntAttribute ("orientation", 0)));
        s.size        = static_cast<MeterSizeMode> (juce::jlimit (0, 1, xml.getIntAttribute ("size", 1)));
        s.showLabel   = xml.getBoolAttribute ("showLabel", true);
        s.showScale   = xml.getBoolAttribute ("showScale", true);
        return s;
    }
};
