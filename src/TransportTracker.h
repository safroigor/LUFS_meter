#pragma once

#include <algorithm>
#include <cmath>

/**
    Pure, dependency-free transport / playback-position tracker.

    It answers one question for the audio thread: *should a new measurement pass
    start on this block?* It deliberately ignores the audio signal itself, so a
    silent passage can never start a new pass.

    A new pass begins when:

      * the host transport transitions from "not playing" to "playing"
        (stop -> play, including resuming from the same position; pause -> play
        is treated the same way because hosts do not reliably distinguish the
        two, see docs/decisions/0007-transport-lifecycle.md);
      * while playing, the playback position jumps by more than a small
        tolerance (a seek, or a loop wrapping back to its start);
      * the transport claims to be playing but the audio callback is not called
        for longer than `gapResetSeconds` (a host that stops processing without
        reporting it).

    The logic is time-based and independent of block size; minor timing jitter is
    absorbed by the position tolerance. There is intentionally no JUCE dependency
    so the whole lifecycle can be unit-tested directly.
*/
class TransportTracker
{
public:
    /** Everything the tracker needs from one audio block. */
    struct BlockContext
    {
        bool   transportPlaying = false; ///< host says the transport is rolling
        bool   hasPosition      = false; ///< host provided a playback position
        double positionSeconds  = 0.0;   ///< playback position at the block start
        int    numSamples       = 0;
        double sampleRate       = 48000.0;
        double nowMs            = 0.0;   ///< monotonic callback timestamp
    };

    /** Advances the tracker by one block and returns true when a new
        measurement pass must start. */
    bool update (const BlockContext& block) noexcept
    {
        const double blockSeconds = (block.sampleRate > 0.0 && block.numSamples > 0)
                                        ? (double) block.numSamples / block.sampleRate
                                        : 0.0;

        if (! block.transportPlaying)
        {
            // The pass is over, but the last published value is left frozen for
            // the UI. Remember where the transport came to rest.
            if (block.hasPosition)
                lastPosition_ = block.positionSeconds;

            wasPlaying_       = false;
            havePosition_     = block.hasPosition;
            lastBlockSeconds_ = 0.0;
            lastCallbackMs_   = block.nowMs;
            return false;
        }

        bool newPass = false;

        if (! wasPlaying_)
        {
            // Transport just started rolling: always a new pass, even if the
            // position did not move (e.g. stop -> play from the same position).
            newPass = true;
        }
        else
        {
            if (havePosition_ && block.hasPosition)
            {
                // Where would this block start if playback had been continuous?
                const double expected  = lastPosition_ + lastBlockSeconds_;
                const double tolerance = std::max (minPositionToleranceSeconds,
                                                   lastBlockSeconds_ + blockSeconds + jitterSeconds);

                if (std::abs (block.positionSeconds - expected) > tolerance)
                    newPass = true; // seek or loop wrap
            }

            if (block.nowMs - lastCallbackMs_ > gapResetSeconds * 1000.0)
                newPass = true; // host stopped processing while "playing"
        }

        wasPlaying_       = true;
        lastCallbackMs_   = block.nowMs;
        lastBlockSeconds_ = blockSeconds;

        if (block.hasPosition)
        {
            lastPosition_ = block.positionSeconds;
            havePosition_ = true;
        }
        else
        {
            havePosition_ = false;
        }

        return newPass;
    }

    /** Forgets all history, so the next playing block starts a new pass. */
    void reset() noexcept
    {
        wasPlaying_       = false;
        havePosition_     = false;
        lastPosition_     = 0.0;
        lastBlockSeconds_ = 0.0;
        lastCallbackMs_   = 0.0;
    }

    // Tunables (seconds). Kept public so the rationale is visible and testable.
    static constexpr double minPositionToleranceSeconds = 0.05;
    static constexpr double jitterSeconds               = 0.02;
    static constexpr double gapResetSeconds             = 1.0;

private:
    bool   wasPlaying_       = false;
    bool   havePosition_     = false;
    double lastPosition_     = 0.0;
    double lastBlockSeconds_ = 0.0;
    double lastCallbackMs_   = 0.0;
};
