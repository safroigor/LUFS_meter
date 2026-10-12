#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
    Small, quietly styled vector icon button used for Reset, Settings and About.

    The visible glyph is drawn as a vector path (crisp at any scale) while the
    component keeps a comfortable clickable area. Hover, pressed and focus
    states are all drawn by the button itself.
*/
class IconButton final : public juce::Button
{
public:
    enum class IconType
    {
        reset,
        settings,
        info
    };

    IconButton (IconType type, const juce::String& tooltip);

    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    static juce::Path makeResetArc (juce::Rectangle<float> area, float strokeWidth);
    static juce::Path makeResetHead (juce::Rectangle<float> area, float strokeWidth);
    static juce::Path makeSettingsPath (juce::Rectangle<float> area);
    static juce::Path makeInfoRing (juce::Rectangle<float> area);

    IconType type_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconButton)
};
