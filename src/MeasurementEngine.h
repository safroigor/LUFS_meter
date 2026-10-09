#pragma once

#include <atomic>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include "LoudnessMeter.h"

/**
    Real-time-safe bridge between the audio thread and the measurement core.

    The audio thread only copies interleaved samples into a lock-free SPSC ring
    buffer (juce::AbstractFifo) and sets an atomic reset flag. A dedicated
    worker thread owns the LoudnessMeter (libebur128 is not thread-safe) and
    periodically publishes the Integrated Loudness as an atomic snapshot.

    See docs/architecture.md and docs/decisions/0003-real-time-architecture.md.
*/
class MeasurementEngine final : private juce::Thread
{
public:
    MeasurementEngine();
    ~MeasurementEngine() override;

    /** Allocates and starts the worker. Call from prepareToPlay (not the audio
        callback). Safe to call again; reconfigures on sample-rate/channel
        changes. */
    void prepare (double sampleRate, int numChannels);

    /** Stops the worker and frees resources. */
    void release();

    /** Audio-thread entry point. Copies the buffer into the ring buffer.
        Never allocates or blocks. Excess input is dropped if the worker falls
        more than the ring-buffer duration behind. */
    void pushBlock (const juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    /** Requests a fresh measurement. Executed on the worker thread. */
    void requestReset() noexcept;

    /** Latest snapshot for the UI. hasValue() == false means "no trustworthy
        value yet" and getIntegratedLufs() must not be displayed. */
    bool hasValue() const noexcept { return hasValue_.load (std::memory_order_relaxed); }
    double getIntegratedLufs() const noexcept { return value_.load (std::memory_order_relaxed); }

private:
    void run() override;
    void publish();

    LoudnessMeter meter_;
    juce::AbstractFifo fifo_ { 1 };
    juce::HeapBlock<float> interleaved_;
    int capacityFrames_ = 0;
    int channels_ = 0;

    std::atomic<bool> resetRequested_ { false };
    std::atomic<bool> hasValue_ { false };
    std::atomic<double> value_ { 0.0 };

    static constexpr double bufferSeconds = 4.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeasurementEngine)
};
