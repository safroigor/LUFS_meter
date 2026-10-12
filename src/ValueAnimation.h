#pragma once

#include <juce_core/juce_core.h>

/** Fills an indicator up to a value instead of jumping to it.

    A measurement pass starts from "no value": Integrated Loudness only appears
    once a full gating block has been processed, and the reading then jumps
    straight to a number. Jumping reads as a glitch, so the meter sweeps up to
    that first value over a fraction of a second, and the numeric reading
    travels with it. Once the sweep is over the animation simply follows the
    measurement, so the meter never lags the value it reports.

    Time is supplied by the caller (`advance`), never read from a clock: the
    duration is then independent of the frame rate, the sequence is
    reproducible, and the offscreen preview tool can run it deterministically.
*/
class ValueAnimation
{
public:
    /** How long a fill takes: long enough to read as a sweep, short enough that
        it can never look like a meter lagging behind the measurement. */
    static constexpr double defaultDurationSeconds = 0.55;

    /** Starts a fill: `from` is where the indicator is now (the bottom of the
        scale for a fresh pass), `target` the value to reach. */
    void start (double from, double target) noexcept
    {
        from_      = from;
        target_    = target;
        value_     = from;
        elapsed_   = 0.0;
        running_   = true;
        hasValue_  = true;
    }

    /** Moves the target while the fill runs (a measurement keeps changing as a
        pass accumulates); once the fill is over this is just the value shown. */
    void setTarget (double target) noexcept
    {
        target_ = target;

        if (! running_)
            value_ = target;
    }

    /** Advances by `deltaSeconds` and returns the value to display. */
    double advance (double deltaSeconds) noexcept
    {
        if (! running_)
            return value_;

        elapsed_ += juce::jmax (0.0, deltaSeconds);

        const double progress = juce::jlimit (0.0, 1.0, elapsed_ / defaultDurationSeconds);

        if (progress >= 1.0)
        {
            running_ = false;
            value_   = target_;
        }
        else
        {
            // Ease-out cubic: quick off the mark, then settling onto the value.
            const double remaining = 1.0 - progress;
            value_ = from_ + (target_ - from_) * (1.0 - remaining * remaining * remaining);
        }

        return value_;
    }

    /** Drops any value: the indicator is empty again, and the next `start`
        begins a fresh fill. Used when the measurement has no trustworthy value
        (a reset pass, or not enough audio processed yet). */
    void clear() noexcept
    {
        running_  = false;
        hasValue_ = false;
        elapsed_  = 0.0;
        value_    = 0.0;
    }

    /** True once a fill has been started and not cleared: the value is then the
        one to display instead of a raw measurement. */
    bool hasValue() const noexcept { return hasValue_; }

    bool isRunning() const noexcept { return running_; }

    double value() const noexcept { return value_; }

private:
    double from_    = 0.0;
    double target_  = 0.0;
    double value_   = 0.0;
    double elapsed_ = 0.0;

    bool running_  = false;
    bool hasValue_ = false;
};
