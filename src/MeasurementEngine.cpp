#include "MeasurementEngine.h"

MeasurementEngine::MeasurementEngine()
    : juce::Thread ("LUFS measurement")
{
}

MeasurementEngine::~MeasurementEngine()
{
    release();
}

void MeasurementEngine::prepare (double sampleRate, int numChannels)
{
    release();

    if (sampleRate <= 0.0 || numChannels <= 0)
        return;

    channels_ = numChannels;
    capacityFrames_ = (int) (sampleRate * bufferSeconds);

    if (capacityFrames_ < 1)
        capacityFrames_ = 1;

    fifo_.setTotalSize (capacityFrames_);
    interleaved_.allocate ((size_t) capacityFrames_ * (size_t) channels_, true);

    if (! meter_.prepare ((unsigned int) channels_, (unsigned long) sampleRate))
        return;

    startThread (juce::Thread::Priority::normal);
}

void MeasurementEngine::release()
{
    stopThread (1000);

    meter_.release();
    interleaved_.free();

    fifo_.setTotalSize (1);
    fifo_.reset();

    capacityFrames_ = 0;
    channels_ = 0;

    resetRequested_.store (false);
    hasValue_.store (false);
    value_.store (0.0);
}

void MeasurementEngine::pushBlock (const juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    if (channels_ == 0 || numSamples <= 0)
        return;

    const int numCh = channels_;

    if (buffer.getNumChannels() < numCh)
        return;

    const int framesToWrite = juce::jmin (numSamples, capacityFrames_);

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo_.prepareToWrite (framesToWrite, start1, size1, start2, size2);

    auto* dst = interleaved_.getData();
    int sourceFrame = 0;

    auto writeRange = [&] (int destStart, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            float* frame = dst + (size_t) (destStart + i) * (size_t) numCh;

            for (int c = 0; c < numCh; ++c)
                frame[c] = buffer.getReadPointer (c)[sourceFrame + i];
        }

        sourceFrame += count;
    };

    writeRange (start1, size1);
    writeRange (start2, size2);

    fifo_.finishedWrite (size1 + size2);
}

void MeasurementEngine::requestReset() noexcept
{
    resetRequested_.store (true, std::memory_order_relaxed);
}

void MeasurementEngine::run()
{
    while (! threadShouldExit())
    {
        if (resetRequested_.exchange (false, std::memory_order_relaxed))
        {
            // Discard anything queued from before the reset, then restart the
            // measurement so stale data can never be shown as a new result.
            const int pending = fifo_.getNumReady();

            if (pending > 0)
            {
                int s1 = 0, z1 = 0, s2 = 0, z2 = 0;
                fifo_.prepareToRead (pending, s1, z1, s2, z2);
                fifo_.finishedRead (z1 + z2);
            }

            meter_.reset();
            hasValue_.store (false, std::memory_order_relaxed);
            value_.store (0.0, std::memory_order_relaxed);
        }

        const int ready = fifo_.getNumReady();

        if (ready > 0)
        {
            int s1 = 0, z1 = 0, s2 = 0, z2 = 0;
            fifo_.prepareToRead (ready, s1, z1, s2, z2);

            auto* data = interleaved_.getData();

            if (z1 > 0)
                meter_.processInterleaved (data + (size_t) s1 * (size_t) channels_, (unsigned long) z1);

            if (z2 > 0)
                meter_.processInterleaved (data + (size_t) s2 * (size_t) channels_, (unsigned long) z2);

            fifo_.finishedRead (z1 + z2);
            publish();
        }

        wait (20);
    }
}

void MeasurementEngine::publish()
{
    double v = 0.0;

    if (meter_.getIntegratedLufs (v))
    {
        value_.store (v, std::memory_order_relaxed);
        hasValue_.store (true, std::memory_order_relaxed);
    }
    else
    {
        hasValue_.store (false, std::memory_order_relaxed);
    }
}
