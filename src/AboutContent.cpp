#include "AboutContent.h"

#include "MeterTheme.h"

// The plugin target defines this; the fallback keeps the file self-contained for
// any tooling that compiles it outside a JUCE plugin target.
#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "1.3.0"
#endif

namespace
{
    constexpr int padding = 18;
    constexpr int contentWidth = 348;
}

AboutContent::AboutContent()
{
    const auto body = MeterTheme::regularFont (13.0f);
    const auto heading = MeterTheme::regularFont (11.5f);

    auto add = [this] (const juce::String& text, const juce::Font& font,
                       juce::Colour colour, float spaceBefore)
    {
        Paragraph p;
        p.text = juce::AttributedString (text);
        p.text.setFont (font);
        p.text.setColour (colour);
        p.spaceBefore = spaceBefore;

        juce::TextLayout layout;
        layout.createLayout (p.text, (float) contentWidth);
        p.height = layout.getHeight();

        paragraphs_.push_back (std::move (p));
    };

    add ("LUFS Meter", MeterTheme::mediumFont (19.0f), MeterTheme::textPrimary, 0.0f);
    add ("SafroAudio  |  Version " JucePlugin_VersionString,
         body, MeterTheme::textSecondary, 3.0f);
    add ("VST3  |  Windows x64", body, MeterTheme::textSecondary, 1.0f);
    add ("Loudness and True Peak measurement to EBU R 128 and ITU-R BS.1770. "
         "A measurement device: it never modifies the audio signal.",
         body, MeterTheme::textPrimary, 9.0f);

    add ("MEASUREMENTS", heading, MeterTheme::textDim, 11.0f);
    add ("Integrated (LUFS-I) - gated loudness of the current pass.",
         body, MeterTheme::textSecondary, 3.0f);
    add ("Momentary (LM) - 400 ms window.", body, MeterTheme::textSecondary, 2.0f);
    add ("Short-Term (LS) - 3 s window.", body, MeterTheme::textSecondary, 2.0f);
    add ("True Peak (TP) - maximum inter-sample peak, in dBTP.",
         body, MeterTheme::textSecondary, 2.0f);

    add ("THIRD-PARTY", heading, MeterTheme::textDim, 11.0f);
    add ("JUCE 8.0.15 - AGPLv3 / commercial.", body, MeterTheme::textSecondary, 3.0f);
    add ("libebur128 v1.2.6 - MIT.", body, MeterTheme::textSecondary, 2.0f);

    float h = (float) padding;
    for (const auto& p : paragraphs_)
        h += p.spaceBefore + p.height;
    h += (float) padding;

    setSize (contentWidth + 2 * padding, juce::roundToInt (h));
}

void AboutContent::paint (juce::Graphics& g)
{
    g.fillAll (MeterTheme::panelBackground);

    float y = (float) padding;

    for (const auto& p : paragraphs_)
    {
        y += p.spaceBefore;

        juce::TextLayout layout;
        layout.createLayout (p.text, (float) contentWidth);
        layout.draw (g, juce::Rectangle<float> ((float) padding, y,
                                                (float) contentWidth, p.height));

        y += p.height;
    }
}
