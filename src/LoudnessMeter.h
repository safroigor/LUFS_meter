#pragma once

#include <ebur128.h>

/**
    Core Integrated Loudness measurement engine.

    Wraps libebur128 in the exact (non-histogram) mode and exposes a minimal,
    single-threaded API. This class is not thread-safe: each instance must be
    used from a single thread only, which is what MeasurementEngine does.

    It has no dependency on JUCE or the GUI, so it can be unit-tested directly.
*/
class LoudnessMeter
{
public:
    LoudnessMeter() = default;
    ~LoudnessMeter();

    LoudnessMeter (const LoudnessMeter&) = delete;
    LoudnessMeter& operator= (const LoudnessMeter&) = delete;

    /** (Re)creates the measurement state. Allocates, so call it outside the
        audio callback. No-op if already prepared for the same configuration. */
    bool prepare (unsigned int channels, unsigned long sampleRate);

    /** Releases all resources. */
    void release();

    /** Clears the accumulated measurement, keeping the configuration. */
    bool reset();

    /** Feeds interleaved frames (frames * channels floats). */
    bool processInterleaved (const float* interleaved, unsigned long frames) const;

    /** Returns the current Integrated Loudness in LUFS.
        Returns false when no trustworthy value exists yet (silence or less
        than one full gating block). */
    bool getIntegratedLufs (double& outLufs) const;

    bool isPrepared() const noexcept { return state_ != nullptr; }
    unsigned int getChannelCount() const noexcept { return channels_; }
    unsigned long getSampleRate() const noexcept { return sampleRate_; }

private:
    ebur128_state* state_ = nullptr;
    unsigned int channels_ = 0;
    unsigned long sampleRate_ = 0;
};
