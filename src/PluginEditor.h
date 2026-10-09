#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "MeterComponent.h"
#include "MeterSettings.h"

class LufsMeterAudioProcessor;

/**
    Compact dark editor for the LUFS Meter.

    Shows the numeric Integrated Loudness and a slim scale, with a settings
    panel (orientation, size, label/scale visibility) and a Reset button. The
    editor only reads an atomic snapshot; changing orientation/size never resets
    the measurement.
*/
class LufsMeterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor&);
    ~LufsMeterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void toggleSettings();
    void applySettings();
    void refreshControlsFromSettings();
    juce::Point<int> getMeasuredSize() const;

    LufsMeterAudioProcessor& processorRef;

    MeterComponent meter_;
    juce::TextButton gearButton_;

    juce::TextButton orientationHorizontal_ { "Horizontal" };
    juce::TextButton orientationVertical_   { "Vertical" };
    juce::ComboBox sizeCombo_;
    juce::ToggleButton showLabelToggle_ { "Show label" };
    juce::ToggleButton showScaleToggle_ { "Show scale" };
    juce::TextButton resetButton_ { "Reset" };

    bool settingsVisible_ = false;
    bool updatingControls_ = false;

    static constexpr int gearStripWidth = 30;
    static constexpr int settingsPanelHeight = 150;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessorEditor)
};
