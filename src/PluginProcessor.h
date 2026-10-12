#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "EditorWindowSizes.h"
#include "MeasurementEngine.h"
#include "MeterSettings.h"
#include "TransportTracker.h"

/**
    VST3 audio processor for the LUFS Meter.

    This plugin is a measurement device only: it never modifies the audio
    signal.

    Besides the measurement, the processor owns the persistent UI state: the
    display settings and the window size the user dragged each display mode to
    (see docs/decisions/0009-editor-window-size-persistence.md). Restoring a
    state broadcasts a change, so an editor that is already open follows it.

    Measurement lifecycle: while the transport is playing the Integrated
    Loudness is measured continuously, including silent passages. A new
    measurement pass — which clears the accumulated value — starts only on a
    transport/position event (play after stop, a seek, a loop wrap or a gap in
    processing), never because a block merely contains silence. See
    `TransportTracker` and docs/decisions/0007-transport-lifecycle.md.
*/
class LufsMeterAudioProcessor final : public juce::AudioProcessor,
                                      public juce::ChangeBroadcaster
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

    /** Window size the user last dragged `mode` to; a size of 0 means the mode
        has never been resized and opens at its preferred size. Thread-safe: the
        editor writes it from the message thread, and the host may read it from
        any thread when it saves the state. */
    EditorWindowSizes::Size getEditorWindowSize (MeterDisplayMode mode) const noexcept;
    void setEditorWindowSize (MeterDisplayMode mode, int width, int height) noexcept;
    void resetEditorWindowSize (MeterDisplayMode mode) noexcept;

private:
    struct AtomicWindowSize
    {
        std::atomic<int> width { 0 };
        std::atomic<int> height { 0 };
    };

    static constexpr int numDisplayModes = EditorWindowSizes::numModes;

    EditorWindowSizes getEditorWindowSizes() const noexcept;
    void applyEditorWindowSizes (const EditorWindowSizes&) noexcept;

    MeasurementEngine meter_;
    MeterSettings settings_;

    // Window size per display mode (vertical = 0, horizontal = 1, compact = 2).
    // See docs/decisions/0009-editor-window-size-persistence.md.
    AtomicWindowSize editorWindowSizes_[numDisplayModes];

    // Measurement-pass lifecycle. Only touched on the audio thread.
    TransportTracker transportTracker_;
    double currentSampleRate_ = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessor)
};
