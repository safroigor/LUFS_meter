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

    // EBUR128_MODE_I enables the Integrated measurement (and Momentary as a
    // prerequisite). The histogram mode is intentionally NOT used: it would
    // quantise block energies to 0.1 LU bins.
    state_ = ebur128_init (channels, sampleRate, EBUR128_MODE_I);

    if (state_ == nullptr)
        return false;

    channels_ = channels;
    sampleRate_ = sampleRate;
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

    return ebur128_add_frames_float (state_, interleaved, (size_t) frames)
           == EBUR128_SUCCESS;
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
