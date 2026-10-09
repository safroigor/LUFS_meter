#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeasurementEngine.h"
#include "MeterSettings.h"

/**
    Draws the Integrated Loudness value as a segmented bar plus a numeric reading,
    and the True Peak / Short-Term / Momentary values (labelled TP, LS, LM) as a
    compact numeric block.

    Horizontal and vertical layouts share the same visual language: a row/column
    of uniformly sized rounded segments, filled up to the current value. Segment
    colour is decided by the zone configuration (default / green / red), not by
    the drawing code. The component only reads the coherent atomic snapshot from
    the engine; it performs no DSP and never blocks the audio thread.
*/
class MeterComponent final : public juce::Component
{
public:
    explicit MeterComponent (const MeasurementEngine& engine);

    void setSettings (const MeterSettings& s);
    const MeterSettings& getSettings() const noexcept { return settings_; }

    void paint (juce::Graphics&) override;

    /** Extra height the horizontal layout reserves above the segmented scale for
        the secondary readout row (TP, LS, LM). The editor adds the same amount
        to its preferred/minimum height so the primary composition is unchanged. */
    static constexpr int secondaryRowHeight = 40;

private:
    void paintHorizontal (juce::Graphics&, juce::Rectangle<int> area,
                          const MeasurementEngine::Reading& reading);
    void paintVertical (juce::Graphics&, juce::Rectangle<int> area,
                        const MeasurementEngine::Reading& reading);

    /** Draws the TP / LS / LM block either as a single row (horizontal) or a
        stacked column (vertical) inside `area`. In the horizontal case the group
        is centred on `referenceCentreX` (the segmented scale's centre) and kept
        within `area`, so it can never clip at small widths. */
    void paintReadouts (juce::Graphics&, juce::Rectangle<int> area,
                        const MeasurementEngine::Reading& reading,
                        bool horizontalRow, int referenceCentreX);

    const MeasurementEngine& engine_;
    MeterSettings settings_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterComponent)
};
