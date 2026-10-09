#include "LoudnessMeter.h"

#include <cmath>

LoudnessMeter::~LoudnessMeter()
{
    release();
}

bool LoudnessMeter::prepare (unsigned int channels, unsigned long sampleRate)
{
    if (channels == 0 || sampleRate == 0)
        return false;

    if (state_ != nullptr && channels == channels_ && sampleRate == sampleRate_)
        return true;

    release();

    // EBUR128_MODE_I enables the Integrated measurement, EBUR128_MODE_S the
    // rolling Short-Term (3 s) and Momentary (400 ms) measurements, and
    // EBUR128_MODE_TRUE_PEAK the inter-sample true peak. All three share the
    // same K-weighting pass and state. The histogram mode is intentionally NOT
    // used: it would quantise block energies to 0.1 LU bins.
    state_ = ebur128_init (channels, sampleRate,
                           EBUR128_MODE_I | EBUR128_MODE_S | EBUR128_MODE_TRUE_PEAK);

    if (state_ == nullptr)
        return false;

    channels_ = channels;
    sampleRate_ = sampleRate;
    samplesIn100ms_ = (sampleRate + 5) / 10;
    framesSinceReset_ = 0;
    return true;
}

void LoudnessMeter::release()
{
    if (state_ != nullptr)
    {
        ebur128_destroy (&state_);
        state_ = nullptr;
    }

    channels_ = 0;
    sampleRate_ = 0;
    samplesIn100ms_ = 0;
    framesSinceReset_ = 0;
}

bool LoudnessMeter::reset()
{
    if (channels_ == 0 || sampleRate_ == 0)
        return false;

    const unsigned int ch = channels_;
    const unsigned long sr = sampleRate_;

    release();
    return prepare (ch, sr);
}

bool LoudnessMeter::processInterleaved (const float* interleaved, unsigned long frames) const
{
    if (state_ == nullptr || interleaved == nullptr || frames == 0)
        return false;

    const bool ok = ebur128_add_frames_float (state_, interleaved, (size_t) frames)
                    == EBUR128_SUCCESS;

    if (ok)
        framesSinceReset_ += frames;

    return ok;
}

bool LoudnessMeter::getIntegratedLufs (double& outLufs) const
{
    if (state_ == nullptr)
        return false;

    double value = 0.0;

    if (ebur128_loudness_global (state_, &value) != EBUR128_SUCCESS)
        return false;

    // libebur128 returns -HUGE_VAL when no block passes the absolute gate
    // (e.g. digital silence). That is not a trustworthy reading.
    if (! std::isfinite (value))
        return false;

    outLufs = value;
    return true;
}

bool LoudnessMeter::getMomentaryLufs (double& outLufs) const
{
    if (state_ == nullptr)
        return false;

    // libebur128 zero-fills the not-yet-populated part of the rolling buffer, so
    // before a full window exists it would return a value dragged down by those
    // zeros. Stay unavailable until the window is genuinely full.
    if (framesSinceReset_ < (unsigned long long) samplesIn100ms_ * 4ull)
        return false;

    double value = 0.0;

    if (ebur128_loudness_momentary (state_, &value) != EBUR128_SUCCESS)
        return false;

    if (! std::isfinite (value) || value <= silenceFloorLufs)
        return false;

    outLufs = value;
    return true;
}

bool LoudnessMeter::getShortTermLufs (double& outLufs) const
{
    if (state_ == nullptr)
        return false;

    if (framesSinceReset_ < (unsigned long long) samplesIn100ms_ * 30ull)
        return false;

    double value = 0.0;

    if (ebur128_loudness_shortterm (state_, &value) != EBUR128_SUCCESS)
        return false;

    if (! std::isfinite (value) || value <= silenceFloorLufs)
        return false;

    outLufs = value;
    return true;
}

bool LoudnessMeter::getTruePeakDbtp (double& outDbtp) const
{
    if (state_ == nullptr)
        return false;

    double maxPeak = 0.0;

    for (unsigned int c = 0; c < channels_; ++c)
    {
        double peak = 0.0;

        if (ebur128_true_peak (state_, c, &peak) != EBUR128_SUCCESS)
            return false;

        if (peak > maxPeak)
            maxPeak = peak;
    }

    // Digital silence: no non-zero sample has ever been measured. Do not report
    // an infinite dB value. A linear amplitude of 1.0 is 0 dBTP.
    if (maxPeak <= 0.0)
        return false;

    outDbtp = 20.0 * std::log10 (maxPeak);
    return true;
}
