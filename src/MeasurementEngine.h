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
    periodically publishes Integrated, Momentary, Short-Term and True Peak as one
    coherent snapshot.

    See docs/architecture.md, docs/decisions/0003-real-time-architecture.md and
    docs/decisions/0008-coherent-reading-snapshot.md.
*/
class MeasurementEngine final : private juce::Thread
{
public:
    /** A coherent set of the latest measurements. `has*` is false when no
        trustworthy value exists for that measurement (see LoudnessMeter). */
    struct Reading
    {
        bool   hasIntegrated = false; double integrated = 0.0;
        bool   hasMomentary  = false; double momentary  = 0.0;
        bool   hasShortTerm  = false; double shortTerm  = 0.0;
        bool   hasTruePeak   = false; double truePeak   = 0.0; // dBTP
    };

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

    /** Runs on the worker thread. Starts a new measurement. When
        discardQueued is true, any already-queued audio is dropped first (used
        by the UI Reset button); otherwise queued samples are kept and fed into
        the fresh measurement (used when playback resumes after an
        interruption). */
    void requestReset (bool discardQueued = true) noexcept;

    /** Latest coherent snapshot for the UI. All four values belong to the same
        publication and are never a mix of two different updates. */
    Reading getReading() const noexcept;

    /** Convenience accessors over getReading() (used by the meter UI and
        tests). hasValue() == false means "no trustworthy Integrated value yet"
        and getIntegratedLufs() must not be displayed. */
    bool hasValue() const noexcept { return getReading().hasIntegrated; }
    double getIntegratedLufs() const noexcept { return getReading().integrated; }

private:
    void run() override;
    void publish();

    /** Single-writer / multi-reader coherent publication. The writer (worker
        thread) is the only thread that ever mutates the snapshot; the reader
        (UI thread) retries if it observes a write in progress. */
    void storeReading (const Reading&) noexcept;
    Reading loadReading() const noexcept;

    LoudnessMeter meter_;
    juce::AbstractFifo fifo_ { 1 };
    juce::HeapBlock<float> interleaved_;
    int capacityFrames_ = 0;
    int channels_ = 0;

    std::atomic<bool> resetRequested_ { false };
    std::atomic<bool> resetDiscardQueued_ { true };

    // Coherent snapshot. The payload fields are atomic so concurrent reads and
    // writes are free of data races; sequence_ makes a multi-field read
    // consistent (a seqlock). Payload accesses are relaxed, ordered by the
    // release/acquire fences around sequence_. See ADR 0008.
    std::atomic<unsigned long long> sequence_ { 0 };
    std::atomic<bool>   hasIntegrated_ { false };
    std::atomic<double> integrated_ { 0.0 };
    std::atomic<bool>   hasMomentary_ { false };
    std::atomic<double> momentary_ { 0.0 };
    std::atomic<bool>   hasShortTerm_ { false };
    std::atomic<double> shortTerm_ { 0.0 };
    std::atomic<bool>   hasTruePeak_ { false };
    std::atomic<double> truePeak_ { 0.0 };

    static constexpr double bufferSeconds = 4.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeasurementEngine)
};
