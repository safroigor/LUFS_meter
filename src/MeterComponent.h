#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeterSettings.h"

class MeasurementEngine;

/**
    Draws the Integrated Loudness value and its scale.

    Horizontal and vertical layouts share the same visual language: a slim
    track, a green→yellow gradient up to the current value, tick marks and the
    numeric reading. The component only reads the atomic snapshot from the
    engine; it performs no DSP and never blocks the audio thread.
*/
class MeterComponent final : public juce::Component
{
public:
    explicit MeterComponent (const MeasurementEngine& engine);

    void setSettings (const MeterSettings& s);
    const MeterSettings& getSettings() const noexcept { return settings_; }

    void paint (juce::Graphics&) override;

private:
    void paintHorizontal (juce::Graphics&, juce::Rectangle<int> area,
                          bool hasValue, double value);
    void paintVertical (juce::Graphics&, juce::Rectangle<int> area,
                        bool hasValue, double value);

    const MeasurementEngine& engine_;
    MeterSettings settings_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterComponent)
};
