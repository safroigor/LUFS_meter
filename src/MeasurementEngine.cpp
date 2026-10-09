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
    storeReading ({});
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

void MeasurementEngine::requestReset (bool discardQueued) noexcept
{
    resetDiscardQueued_.store (discardQueued, std::memory_order_relaxed);
    resetRequested_.store (true, std::memory_order_relaxed);
}

void MeasurementEngine::run()
{
    while (! threadShouldExit())
    {
        if (resetRequested_.exchange (false, std::memory_order_relaxed))
        {
            // Optionally discard anything queued from before the reset, then
            // restart the measurement so stale data can never be shown as a new
            // result.
            if (resetDiscardQueued_.load (std::memory_order_relaxed))
            {
                const int pending = fifo_.getNumReady();

                if (pending > 0)
                {
                    int s1 = 0, z1 = 0, s2 = 0, z2 = 0;
                    fifo_.prepareToRead (pending, s1, z1, s2, z2);
                    fifo_.finishedRead (z1 + z2);
                }
            }

            meter_.reset();
            storeReading ({});
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
    Reading r;
    double v = 0.0;

    if (meter_.getIntegratedLufs (v)) { r.hasIntegrated = true; r.integrated = v; }
    if (meter_.getMomentaryLufs (v))  { r.hasMomentary  = true; r.momentary  = v; }
    if (meter_.getShortTermLufs (v))  { r.hasShortTerm  = true; r.shortTerm  = v; }
    if (meter_.getTruePeakDbtp (v))   { r.hasTruePeak   = true; r.truePeak   = v; }

    storeReading (r);
}

void MeasurementEngine::storeReading (const Reading& r) noexcept
{
    const auto start = sequence_.load (std::memory_order_relaxed);
    sequence_.store (start + 1, std::memory_order_relaxed); // odd: writing
    std::atomic_thread_fence (std::memory_order_release);

    hasIntegrated_.store (r.hasIntegrated, std::memory_order_relaxed);
    integrated_.store    (r.integrated,    std::memory_order_relaxed);
    hasMomentary_.store  (r.hasMomentary,  std::memory_order_relaxed);
    momentary_.store     (r.momentary,     std::memory_order_relaxed);
    hasShortTerm_.store  (r.hasShortTerm,  std::memory_order_relaxed);
    shortTerm_.store     (r.shortTerm,     std::memory_order_relaxed);
    hasTruePeak_.store   (r.hasTruePeak,   std::memory_order_relaxed);
    truePeak_.store      (r.truePeak,      std::memory_order_relaxed);

    std::atomic_thread_fence (std::memory_order_release);
    sequence_.store (start + 2, std::memory_order_relaxed); // even: done
}

MeasurementEngine::Reading MeasurementEngine::loadReading() const noexcept
{
    Reading r;

    for (;;)
    {
        const auto s1 = sequence_.load (std::memory_order_acquire);

        if ((s1 & 1ull) != 0ull)
            continue; // a write is in progress

        r.hasIntegrated = hasIntegrated_.load (std::memory_order_relaxed);
        r.integrated    = integrated_.load    (std::memory_order_relaxed);
        r.hasMomentary  = hasMomentary_.load  (std::memory_order_relaxed);
        r.momentary     = momentary_.load     (std::memory_order_relaxed);
        r.hasShortTerm  = hasShortTerm_.load  (std::memory_order_relaxed);
        r.shortTerm     = shortTerm_.load     (std::memory_order_relaxed);
        r.hasTruePeak   = hasTruePeak_.load   (std::memory_order_relaxed);
        r.truePeak      = truePeak_.load      (std::memory_order_relaxed);

        std::atomic_thread_fence (std::memory_order_acquire);

        if (s1 == sequence_.load (std::memory_order_relaxed))
            break; // no write happened during the read
    }

    return r;
}

MeasurementEngine::Reading MeasurementEngine::getReading() const noexcept
{
    return loadReading();
}
