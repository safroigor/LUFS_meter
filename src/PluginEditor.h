#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "MeterComponent.h"
#include "MeterSettings.h"

class LufsMeterAudioProcessor;

/**
    Compact, resizable dark editor for the LUFS Meter.

    The meter fills the padded content area except for a small settings button.
    Settings open in a CallOutBox, so they work identically in horizontal and
    vertical orientation and never disturb the layout. Changing orientation or
    size never resets the measurement.
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
    juce::Point<int> getPreferredSize (const MeterSettings&) const;

    LufsMeterAudioProcessor& processorRef;
    MeterComponent meter_;
    juce::TextButton settingsButton_;

    static constexpr int outerPadding = 8;
    static constexpr int buttonSize = 24;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessorEditor)
};
