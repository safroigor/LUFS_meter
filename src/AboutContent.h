#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

/**
    Popup content for the About / information icon.

    Static, read-only panel shown in a juce::CallOutBox from the editor's info
    button, so it appears identically in every display mode. It states the plugin
    metadata, the measurements it performs and the third-party attributions,
    using the shared theme.
*/
class AboutContent final : public juce::Component
{
public:
    AboutContent();

    void paint (juce::Graphics&) override;

private:
    struct Paragraph
    {
        juce::AttributedString text;
        float height = 0.0f;
        float spaceBefore = 0.0f;
    };

    std::vector<Paragraph> paragraphs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AboutContent)
};
