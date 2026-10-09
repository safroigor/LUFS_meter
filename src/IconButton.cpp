#include "IconButton.h"

#include "MeterTheme.h"

#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    constexpr float twoPi = 2.0f * pi;
}

IconButton::IconButton (IconType type, const juce::String& tooltip)
    : juce::Button (tooltip),
      type_ (type)
{
    setTooltip (tooltip);
    setWantsKeyboardFocus (true);
}

juce::Path IconButton::makeResetArc (juce::Rectangle<float> area)
{
    juce::Path path;

    const auto centre = area.getCentre();
    const float radius = area.getWidth() * 0.32f;

    // A ~280 degree arc, leaving a gap at the top-right for the arrow head.
    const float startAngle = 0.65f;
    const float endAngle = twoPi - 0.35f;

    path.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                        startAngle, endAngle, true);
    return path;
}

juce::Path IconButton::makeResetHead (juce::Rectangle<float> area)
{
    juce::Path path;

    const auto centre = area.getCentre();
    const float radius = area.getWidth() * 0.32f;
    const float endAngle = twoPi - 0.35f;

    const float ax = centre.x + radius * std::sin (endAngle);
    const float ay = centre.y - radius * std::cos (endAngle);
    const float tx = std::cos (endAngle);
    const float ty = std::sin (endAngle);

    const float head = radius * 0.60f;
    const float half = radius * 0.46f;

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
    const float outer = area.getWidth() * 0.42f;
    const float valley = outer * 0.78f;
    const float hole = outer * 0.36f;
    constexpr int teeth = 10;

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

void IconButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto area = getLocalBounds().toFloat();
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    auto iconArea = area.withSizeKeepingCentre (size, size);

    // Subtle state background (never a heavy border).
    if (down)
    {
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.fillEllipse (iconArea.reduced (1.0f));
    }
    else if (highlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillEllipse (iconArea.reduced (1.0f));
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (MeterTheme::accent.withAlpha (0.65f));
        g.drawEllipse (iconArea.reduced (1.0f), 1.0f);
    }

    const auto glyphArea = iconArea.reduced (size * 0.24f);

    const auto colour = down ? MeterTheme::textPrimary
                             : (highlighted ? juce::Colour (0xffcfcfcf) : MeterTheme::textSecondary);
    g.setColour (colour);

    if (type_ == IconType::reset)
    {
        const float stroke = juce::jmax (1.4f, size * 0.075f);
        g.strokePath (makeResetArc (glyphArea),
                      juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
        g.fillPath (makeResetHead (glyphArea));
    }
    else
    {
        g.fillPath (makeSettingsPath (glyphArea));
    }
}
