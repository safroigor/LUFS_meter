#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MeasurementEngine.h"
#include "MeterSettings.h"
#include "TransportTracker.h"

/**
    VST3 audio processor for the LUFS Meter.

    This plugin is a measurement device only: it never modifies the audio
    signal.

    Measurement lifecycle: while the transport is playing the Integrated
    Loudness is measured continuously, including silent passages. A new
    measurement pass — which clears the accumulated value — starts only on a
    transport/position event (play after stop, a seek, a loop wrap or a gap in
    processing), never because a block merely contains silence. See
    `TransportTracker` and docs/decisions/0007-transport-lifecycle.md.
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

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    /** UI access to the latest measurement snapshot (thread-safe). */
    MeasurementEngine& getMeasurementEngine() noexcept { return meter_; }

    /** Restart the Integrated Loudness measurement (UI Reset button). Clears
        the accumulated and displayed value; playback is not interrupted, and a
        fresh measurement begins from the current point. */
    void resetMeasurement() { meter_.requestReset (true); }

    /** Persistent UI settings (message thread only). */
    MeterSettings getSettings() const noexcept { return settings_; }
    void setSettings (const MeterSettings& s) { settings_ = s.sanitised(); }

private:
    MeasurementEngine meter_;
    MeterSettings settings_;

    // Measurement-pass lifecycle. Only touched on the audio thread.
    TransportTracker transportTracker_;
    double currentSampleRate_ = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessor)
};
