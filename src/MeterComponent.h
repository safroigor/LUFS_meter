#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeasurementEngine.h"
#include "MeterSettings.h"
#include "ValueAnimation.h"

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

    /** Horizontal space the editor's overlaid buttons reserve at the sides of
        the meter's own rows: the Horizontal TP / LS / LM row and the Compact
        reading capsule are both laid out — and centred — inside what is left,
        so neither can run under a button. Vertical leaves both insets at zero
        (its readouts sit beside the bar, and its buttons are in the corners). */
    void setControlInsets (int leftInset, int rightInset);

    /** Advances the fill animation by the elapsed frame time. The first valid
        value of a pass (or the first frame after the editor opened mid-pass) is
        filled up to from the bottom of the scale, with the reading travelling
        with it; after that the meter follows the measurement directly. Called
        by the editor's repaint timer — painting itself changes nothing, so an
        offscreen render is reproducible. */
    void advanceAnimation (double deltaSeconds);

    /** Ends any running fill and shows the settled value (offscreen renders and
        tests, which have no frame clock). */
    void settleAnimation();

    /** Drops the held reading (the editor's Reset button): the meter clears to
        the "no value yet" placeholder instead of holding the last reading while
        a new pass is measured. */
    void resetDisplay();

    void paint (juce::Graphics&) override;

    /** Extra height the horizontal layout reserves above the segmented scale for
        the secondary readout row (TP, LS, LM). The editor adds the same amount
        to its preferred/minimum height so the primary composition is unchanged. */
    static constexpr int secondaryRowHeight = 32;

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
    int controlInsetLeft_ = 0;
    int controlInsetRight_ = 0;

    /** Filled-in value of the current pass. See `advanceAnimation`. */
    ValueAnimation animation_;
    bool hadValue_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeterComponent)
};
