#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class LufsMeterAudioProcessor;

/**
    Provisional editor for Stage 2: it shows the current Integrated Loudness so
    the measurement can be verified in a host. The full meter UI (scale,
    orientation, display options) is implemented in Stage 3.
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
    juce::String getDisplayValue() const;

    LufsMeterAudioProcessor& processorRef;
    juce::TextButton resetButton_ { "Reset" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessorEditor)
};
