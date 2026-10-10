#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "IconButton.h"
#include "MeterComponent.h"
#include "MeterSettings.h"

class LufsMeterAudioProcessor;

/**
    Compact, resizable dark editor for the LUFS Meter.

    Layout: a thin control strip in the upper-right holds the Reset, Settings and
    About vector-icon buttons; the meter fills the rest. Settings open in a
    CallOutBox, so they work identically in every display mode and never disturb
    the layout. The three display modes — Vertical, Horizontal and Compact — are
    mutually exclusive; changing the mode never resets the measurement.
*/
class LufsMeterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor&);
    ~LufsMeterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Called by the settings popup. */
    void applyPopupSettings (const MeterSettings&);
    void popupReset();

private:
    void timerCallback() override;
    void showSettings();
    void showAbout();
    void updateSizeLimits();
    juce::Point<int> getPreferredSize (const MeterSettings&) const;

    LufsMeterAudioProcessor& processorRef;
    MeterComponent meter_;
    IconButton resetButton_    { IconButton::IconType::reset,    "Reset measurement" };
    IconButton settingsButton_ { IconButton::IconType::settings, "Settings" };
    IconButton infoButton_     { IconButton::IconType::info,     "About LUFS Meter" };
    juce::TooltipWindow tooltipWindow_ { this, 700 };

    static constexpr int outerPadding = 8;
    static constexpr int controlStripHeight = 22;
    static constexpr int buttonSize = 22;
    static constexpr int buttonGap = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessorEditor)
};
