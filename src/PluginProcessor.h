#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MeasurementEngine.h"
#include "MeterSettings.h"

/**
    VST3 audio processor for the LUFS Meter.

    This plugin is a measurement device only: it never modifies the audio
    signal.

    Session behaviour: while audio is playing the Integrated Loudness is
    measured continuously. When the signal is interrupted (transport stop or
    pause, or a genuine break in the audio) the displayed value is frozen. As
    soon as playback/audio resumes, a fresh measurement is started that does not
    include any previously accumulated audio.
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

    /** Restart the Integrated Loudness measurement (UI Reset button). */
    void resetMeasurement() { meter_.requestReset (true); }

    /** Persistent UI settings (message thread only). */
    MeterSettings getSettings() const noexcept { return settings_; }
    void setSettings (const MeterSettings& s) { settings_ = s; }

private:
    bool isTransportPlaying() const;

    MeasurementEngine meter_;
    MeterSettings settings_;

    // Session tracking. Only touched on the audio thread.
    bool sessionInterrupted_ = true;
    int silentFrames_ = 0;
    double lastBlockTimeMs_ = 0.0;
    double currentSampleRate_ = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessor)
};
