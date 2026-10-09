#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MeasurementEngine.h"

/**
    VST3 audio processor for the LUFS Meter.

    This plugin is a measurement device only: it never modifies the audio
    signal.
*/
class LufsMeterAudioProcessor final : public juce::AudioProcessor
{
public:
    LufsMeterAudioProcessor();
    ~LufsMeterAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    /** UI access to the latest measurement snapshot. */
    MeasurementEngine& getMeasurementEngine() noexcept { return meter_; }

    /** Restart the Integrated Loudness measurement. */
    void resetMeasurement() { meter_.requestReset(); }

private:
    MeasurementEngine meter_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessor)
};
