#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

class LufsMeterAudioProcessor;

/**
    Stage 1 placeholder editor. The functional meter UI (numeric value, scale,
    orientation, reset) is implemented in a later stage.
*/
class LufsMeterAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor&);
    ~LufsMeterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    LufsMeterAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessorEditor)
};
