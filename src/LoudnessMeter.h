#pragma once

#include <ebur128.h>

/**
    Core loudness measurement engine.

    Wraps libebur128 in the exact (non-histogram) mode and exposes a minimal,
    single-threaded API for Integrated, Momentary (400 ms), Short-Term (3 s) and
    True Peak (dBTP) measurements, all sharing one K-weighting pass. This class
    is not thread-safe: each instance must be used from a single thread only,
    which is what MeasurementEngine does.

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

    /** Returns the current Momentary Loudness (last 400 ms) in LUFS.
        Returns false until a full 400 ms window has been processed since the
        last prepare/reset, or while the window is silent. */
    bool getMomentaryLufs (double& outLufs) const;

    /** Returns the current Short-Term Loudness (last 3 s) in LUFS.
        Returns false until a full 3 s window has been processed since the last
        prepare/reset, or while the window is silent. */
    bool getShortTermLufs (double& outLufs) const;

    /** Returns the maximum True Peak (dBTP) observed across all channels since
        the last prepare/reset. Returns false when no non-zero sample has been
        measured (digital silence), so the UI never shows a fake reading. */
    bool getTruePeakDbtp (double& outDbtp) const;

    /** Loudness at or below this floor is treated as "no meaningful
        measurement" (displayed as unavailable), rather than a meaningless
        number. It is far below the -70 LUFS integrated absolute gate and below
        real programme material, but above the residual values the K-weighting
        filter can leave in an otherwise silent rolling window. This is a
        display-validity floor, NOT integrated loudness gating: M and S remain
        ungated in the standards sense. */
    static constexpr double silenceFloorLufs = -120.0;

    bool isPrepared() const noexcept { return state_ != nullptr; }
    unsigned int getChannelCount() const noexcept { return channels_; }
    unsigned long getSampleRate() const noexcept { return sampleRate_; }

private:
    ebur128_state* state_ = nullptr;
    unsigned int channels_ = 0;
    unsigned long sampleRate_ = 0;

    /** Samples that fit in 100 ms, matching libebur128's own rounding, used to
        decide when a full Momentary (x4) or Short-Term (x30) window exists. */
    unsigned long samplesIn100ms_ = 0;

    /** Frames fed since the last prepare/reset. Counts silence too, so a silent
        passage still fills the rolling windows instead of resetting them. The
        measurement itself is single-threaded; mutable only because feeding is a
        const operation on the library state. */
    mutable unsigned long long framesSinceReset_ = 0;
};
