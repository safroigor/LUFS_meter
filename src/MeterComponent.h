#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeasurementEngine.h"
#include "MeterSettings.h"

/**
    Draws the Integrated Loudness value as a segmented bar plus a numeric reading
    (Vertical / Horizontal modes), or as a nearly square digital meter with one
    continuous bottom-anchored fill that rises and falls with the value (Compact
    mode). The True Peak / Short-Term / Momentary values (labelled TP, LS, LM)
    are drawn as a compact numeric block.

    Vertical and Horizontal share the segmented visual language: uniformly sized
    rounded segments, filled up to the current value. Segment and fill colour are
    decided by the zone configuration (default / green / red), not by the drawing
    code. The component only reads the coherent atomic snapshot from the engine;
    it performs no DSP and never blocks the audio thread.
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

    /** Compact mode: a nearly square digital meter whose main display area is a
        single continuous bottom-anchored fill that rises and falls with the
        Integrated value, with the large centred reading and the TP / LS / LM row
        drawn on top. */
    void paintCompact (juce::Graphics&, juce::Rectangle<int> area,
                       const MeasurementEngine::Reading& reading);

    /** How the three TP / LS / LM groups are arranged. */
    enum class ReadoutLayout
    {
        horizontalRow,   // label left of value, one shared centreline (Horizontal)
        verticalColumn,  // label above value, groups stacked (Vertical)
        columns          // label above value, three groups side by side (Compact)
    };

    /** Draws the TP / LS / LM block inside `area` using `layout`. In the two
        horizontal arrangements the group is centred on `referenceCentreX` and kept
        within `area`, so it can never clip at small widths. `fontScale` scales the
        readout type and `labelColour` sets the label colour; both let Compact
        adjust the row without affecting Horizontal or Vertical. */
    void paintReadouts (juce::Graphics&, juce::Rectangle<int> area,
                        const MeasurementEngine::Reading& reading,
                        ReadoutLayout layout, int referenceCentreX,
                        float fontScale, juce::Colour labelColour);

    const MeasurementEngine& engine_;
    MeterSettings settings_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterComponent)
};
