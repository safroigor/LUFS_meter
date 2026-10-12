#include "IconButton.h"

#include "MeterTheme.h"

#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    constexpr float twoPi = 2.0f * pi;

    // The reset glyph's arc leaves this gap at the top-right for the arrow head.
    constexpr float resetStartAngle = 0.52f;
    constexpr float resetEndAngle   = twoPi - 0.53f;
}

IconButton::IconButton (IconType type, const juce::String& tooltip)
    : juce::Button (tooltip),
      type_ (type)
{
    setTooltip (tooltip);
    setWantsKeyboardFocus (true);
}

juce::Path IconButton::makeResetArc (juce::Rectangle<float> area, float strokeWidth)
{
    juce::Path path;

    const auto centre = area.getCentre();
    // The stroke is centred on the path, so pulling the radius in by half a
    // stroke puts the drawn arc's outer edge exactly on the glyph box — the same
    // optical size as the filled gear and the ring.
    const float radius = juce::jmax (1.0f, area.getWidth() * 0.5f - strokeWidth * 0.5f);

    path.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                        resetStartAngle, resetEndAngle, true);
    return path;
}

juce::Path IconButton::makeResetHead (juce::Rectangle<float> area, float strokeWidth)
{
    juce::Path path;

    const auto centre = area.getCentre();
    const float radius = juce::jmax (1.0f, area.getWidth() * 0.5f - strokeWidth * 0.5f);

    const float ax = centre.x + radius * std::sin (resetEndAngle);
    const float ay = centre.y - radius * std::cos (resetEndAngle);
    const float tx = std::cos (resetEndAngle);
    const float ty = std::sin (resetEndAngle);

    // A solid head that continues the arc along its tangent.
    const float head = area.getWidth() * 0.42f;
    const float half = area.getWidth() * 0.24f;

    path.startNewSubPath (ax + tx * head, ay + ty * head);
    path.lineTo (ax - ty * half, ay + tx * half);
    path.lineTo (ax + ty * half, ay - tx * half);
    path.closeSubPath();
    return path;
}

juce::Path IconButton::makeSettingsPath (juce::Rectangle<float> area)
{
    juce::Path path;

    const auto centre = area.getCentre();
    const float outer = area.getWidth() * 0.5f;
    const float valley = outer * 0.72f;
    const float hole = outer * 0.38f;

    // Six teeth: at a 20 px button a denser gear loses its gaps and reads as a
    // blob (it used to be drawn at half this size with ten teeth).
    constexpr int teeth = 6;

    for (int i = 0; i < teeth * 2; ++i)
    {
        const float angle = -pi * 0.5f + (float) i * pi / (float) teeth;
        const float radius = (i % 2 == 0) ? outer : valley;
        const float x = centre.x + radius * std::cos (angle);
        const float y = centre.y + radius * std::sin (angle);

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    path.closeSubPath();

    // Even-odd fill turns the inner circle into a transparent hole.
    path.addEllipse (centre.x - hole, centre.y - hole, hole * 2.0f, hole * 2.0f);
    path.setUsingNonZeroWinding (false);
    return path;
}

juce::Path IconButton::makeInfoRing (juce::Rectangle<float> area)
{
    juce::Path path;
    path.addEllipse (area);
    return path;
}

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto area = getLocalBounds().toFloat();
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    auto iconArea = area.withSizeKeepingCentre (size, size);

    // Subtle state background (never a heavy border).
    if (down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillEllipse (iconArea.reduced (1.0f));
    }
    else if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillEllipse (iconArea.reduced (1.0f));
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (MeterTheme::accent.withAlpha (0.35f));
        g.drawEllipse (iconArea.reduced (1.0f), 1.0f);
    }

    // One glyph box for all three icons. They used to be drawn at three
    // different optical sizes (a wide ring, a tiny arc, a dense gear), which
    // made Reset and Settings unreadable at the 20 px button size.
    const auto glyphBox = iconArea.reduced (size * 0.18f);
    const float stroke = juce::jmax (1.6f, size * 0.085f);

    // Dim chrome: the icons stay readable without competing with the meter
    // (see MeterTheme's icon colours).
    const auto colour = down ? MeterTheme::iconPressed
                             : (highlighted ? MeterTheme::iconHover : MeterTheme::iconIdle);
    g.setColour (colour);

    if (type_ == IconType::reset)
    {
        g.strokePath (makeResetArc (glyphBox, stroke),
                      juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
        g.fillPath (makeResetHead (glyphBox, stroke));
    }
    else if (type_ == IconType::settings)
    {
        g.fillPath (makeSettingsPath (glyphBox));
    }
    else // info / About
    {
        // A stroked ring with a solid lowercase "i": dot above a short stem, in
        // the same box and with the same stroke weight as the other two glyphs.
        const auto ringArea = glyphBox.reduced (stroke * 0.5f);

        g.strokePath (makeInfoRing (ringArea),
                      juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        const auto c = ringArea.getCentre();
        const float r = ringArea.getWidth() * 0.5f;
        const float dotR = juce::jmax (1.1f, size * 0.055f);

        g.fillEllipse (c.x - dotR, c.y - r * 0.44f - dotR, dotR * 2.0f, dotR * 2.0f);

        const float stemW = juce::jmax (1.8f, size * 0.09f);
        const float stemTop = c.y - r * 0.12f;
        const float stemBottom = c.y + r * 0.46f;
        g.fillRoundedRectangle (c.x - stemW * 0.5f, stemTop, stemW, stemBottom - stemTop,
                                stemW * 0.5f);
    }
}
