#include "TestSupport.h"

#include "LoudnessMeter.h"
#include "MeasurementEngine.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <vector>

// ---------------------------------------------------------------------------
// Signal helpers
// ---------------------------------------------------------------------------

namespace
{
    constexpr double pi = 3.14159265358979323846;
    constexpr double kNoValue = std::numeric_limits<double>::quiet_NaN();

    /** Interleaved 1 kHz sine, identical in every channel (in-phase). */
    std::vector<float> makeSine (int channels, double sampleRate, double seconds,
                                 double frequency, double peakDbfs)
    {
        const int frames = (int) std::llround (sampleRate * seconds);
        std::vector<float> data ((size_t) frames * (size_t) channels);
        const double amplitude = std::pow (10.0, peakDbfs / 20.0);

        for (int i = 0; i < frames; ++i)
        {
            const auto s = (float) (amplitude
                                    * std::sin (2.0 * pi * frequency * (double) i / sampleRate));

            for (int c = 0; c < channels; ++c)
                data[(size_t) i * (size_t) channels + (size_t) c] = s;
        }

        return data;
    }

    std::vector<float> makeSilence (int channels, double sampleRate, double seconds)
    {
        const int frames = (int) std::llround (sampleRate * seconds);
        return std::vector<float> ((size_t) frames * (size_t) channels, 0.0f);
    }

    /** Feeds interleaved data in chunks and returns the Integrated LUFS,
        or NaN when the meter reports no trustworthy value. */
    double measure (int channels, double sampleRate, const std::vector<float>& data,
                    int chunkFrames = 1024)
    {
        LoudnessMeter meter;

        if (! meter.prepare ((unsigned int) channels, (unsigned long) sampleRate))
            return kNoValue;

        const int totalFrames = (int) (data.size() / (size_t) channels);

        for (int pos = 0; pos < totalFrames; pos += chunkFrames)
        {
            const int n = std::min (chunkFrames, totalFrames - pos);
            meter.processInterleaved (data.data() + (size_t) pos * (size_t) channels,
                                      (unsigned long) n);
        }

        double value = 0.0;
        return meter.getIntegratedLufs (value) ? value : kNoValue;
    }

    // -----------------------------------------------------------------------
    // Core measurement tests
    // -----------------------------------------------------------------------

    void testStereoReference()
    {
        std::printf ("testStereoReference\n");
        // EBU Tech 3341 test case 1: stereo 1 kHz sine, -23 dBFS per-channel
        // peak, in phase -> I = -23.0 LUFS (+/- 0.1).
        const auto data = makeSine (2, 48000.0, 20.0, 1000.0, -23.0);
        TEST_CHECK_NEAR (measure (2, 48000.0, data), -23.0, 0.1);
    }

    void testLevelScaling()
    {
        std::printf ("testLevelScaling\n");
        TEST_CHECK_NEAR (measure (2, 48000.0, makeSine (2, 48000.0, 10.0, 1000.0, -33.0)),
                         -33.0, 0.1);
        TEST_CHECK_NEAR (measure (2, 48000.0, makeSine (2, 48000.0, 10.0, 1000.0, -18.0)),
                         -18.0, 0.1);
    }

    void testSilence()
    {
        std::printf ("testSilence\n");
        // Digital silence must not produce a fake 0 LUFS value.
        TEST_CHECK_NOT_A_NUMBER (measure (2, 48000.0, makeSilence (2, 48000.0, 2.0)));
    }

    void testLowLevelAboveGate()
    {
        std::printf ("testLowLevelAboveGate\n");
        TEST_CHECK_NEAR (measure (2, 48000.0, makeSine (2, 48000.0, 10.0, 1000.0, -60.0)),
                         -60.0, 0.1);
    }

    void testBelowAbsoluteGate()
    {
        std::printf ("testBelowAbsoluteGate\n");
        // Everything below the -70 LUFS absolute gate -> no value.
        TEST_CHECK_NOT_A_NUMBER (measure (2, 48000.0, makeSine (2, 48000.0, 10.0, 1000.0, -75.0)));
    }

    void testMonoReference()
    {
        std::printf ("testMonoReference\n");
        // One channel carries half the energy of the in-phase stereo case, so
        // the reading is ~3.01 LU lower: -26.0 LUFS.
        TEST_CHECK_NEAR (measure (1, 48000.0, makeSine (1, 48000.0, 10.0, 1000.0, -23.0)),
                         -26.0, 0.15);
    }

    void testSampleRates()
    {
        std::printf ("testSampleRates\n");

        for (double sr : { 44100.0, 88200.0, 96000.0, 192000.0 })
            TEST_CHECK_NEAR (measure (2, sr, makeSine (2, sr, 10.0, 1000.0, -23.0)),
                             -23.0, 0.1);
    }

    void testBlockSizeIndependence()
    {
        std::printf ("testBlockSizeIndependence\n");
        const auto data = makeSine (2, 48000.0, 10.0, 1000.0, -23.0);

        TEST_CHECK_NEAR (measure (2, 48000.0, data, 1),    -23.0, 0.1);
        TEST_CHECK_NEAR (measure (2, 48000.0, data, 512),  -23.0, 0.1);
        TEST_CHECK_NEAR (measure (2, 48000.0, data, 65536), -23.0, 0.1);
    }

    void testNoValueBeforeOneBlock()
    {
        std::printf ("testNoValueBeforeOneBlock\n");

        LoudnessMeter meter;
        TEST_CHECK (meter.prepare (2, 48000));

        // Less than one full 400 ms gating block of audio -> no value yet.
        const auto shortBlock = makeSine (2, 48000.0, 0.2, 1000.0, -23.0);
        meter.processInterleaved (shortBlock.data(), (unsigned long) (shortBlock.size() / 2));

        double v = 0.0;
        TEST_CHECK (! meter.getIntegratedLufs (v));
    }

    void testReset()
    {
        std::printf ("testReset\n");

        LoudnessMeter meter;
        TEST_CHECK (meter.prepare (2, 48000));

        const auto loud  = makeSine (2, 48000.0, 5.0, 1000.0, -23.0);
        const auto quiet = makeSine (2, 48000.0, 5.0, 1000.0, -33.0);

        auto feed = [&] (const std::vector<float>& data)
        {
            const int total = (int) (data.size() / 2);
            for (int pos = 0; pos < total; pos += 1024)
            {
                const int n = std::min (1024, total - pos);
                meter.processInterleaved (data.data() + (size_t) pos * 2, (unsigned long) n);
            }
        };

        feed (loud);

        double v = 0.0;
        TEST_CHECK (meter.getIntegratedLufs (v));
        TEST_CHECK_NEAR (v, -23.0, 0.1);

        // A reset must discard the previous measurement entirely.
        TEST_CHECK (meter.reset());
        TEST_CHECK (! meter.getIntegratedLufs (v)); // no trustworthy value yet

        feed (quiet);
        TEST_CHECK (meter.getIntegratedLufs (v));
        TEST_CHECK_NEAR (v, -33.0, 0.1);
    }

    void testRelativeGating()
    {
        std::printf ("testRelativeGating\n");

        // 10 s at -23 LUFS followed by 10 s at -40 LUFS. The quiet part is more
        // than 10 LU below the loud part, so the relative gate excludes it and
        // the integrated value stays ~-23.0 LUFS.
        auto data = makeSine (2, 48000.0, 10.0, 1000.0, -23.0);
        const auto quiet = makeSine (2, 48000.0, 10.0, 1000.0, -40.0);
        data.insert (data.end(), quiet.begin(), quiet.end());

        TEST_CHECK_NEAR (measure (2, 48000.0, data), -23.0, 0.1);
    }

    void testSilenceThenSignal()
    {
        std::printf ("testSilenceThenSignal\n");

        // Leading silence is below the absolute gate and must be ignored. A
        // long tone keeps the contribution of the (correctly included) partial
        // transition blocks negligible.
        auto data = makeSilence (2, 48000.0, 5.0);
        const auto tone = makeSine (2, 48000.0, 20.0, 1000.0, -23.0);
        data.insert (data.end(), tone.begin(), tone.end());

        TEST_CHECK_NEAR (measure (2, 48000.0, data), -23.0, 0.1);
    }

    // -----------------------------------------------------------------------
    // Threaded pipeline test
    // -----------------------------------------------------------------------

    bool waitFor (const std::function<bool()>& predicate, int timeoutMs)
    {
        const int step = 10;
        for (int waited = 0; waited < timeoutMs; waited += step)
        {
            if (predicate())
                return true;

            juce::Thread::sleep (step);
        }

        return predicate();
    }

    void testEnginePipeline()
    {
        std::printf ("testEnginePipeline\n");

        MeasurementEngine engine;
        engine.prepare (48000.0, 2);

        const int channels = 2;
        const int frames = 48000 * 5;
        const auto sine = makeSine (channels, 48000.0, 5.0, 1000.0, -23.0);

        const int chunk = 512;
        juce::AudioBuffer<float> block (channels, chunk);

        for (int pos = 0; pos < frames; pos += chunk)
        {
            const int n = std::min (chunk, frames - pos);

            for (int c = 0; c < channels; ++c)
                for (int i = 0; i < n; ++i)
                    block.setSample (c, i, sine[(size_t) (pos + i) * (size_t) channels + (size_t) c]);

            engine.pushBlock (block, n);
            juce::Thread::sleep (2);
        }

        TEST_CHECK (waitFor ([&] { return engine.hasValue(); }, 2000));
        TEST_CHECK_NEAR (engine.getIntegratedLufs(), -23.0, 0.1);

        engine.release();
    }
}

int main()
{
    std::printf ("=== LUFS Meter tests ===\n");

    testStereoReference();
    testLevelScaling();
    testSilence();
    testLowLevelAboveGate();
    testBelowAbsoluteGate();
    testMonoReference();
    testSampleRates();
    testBlockSizeIndependence();
    testNoValueBeforeOneBlock();
    testReset();
    testRelativeGating();
    testSilenceThenSignal();
    testEnginePipeline();

    std::printf ("\n%d checks, %d failures\n", test::checkCount(), test::failureCount());
    return test::failureCount() == 0 ? 0 : 1;
}
