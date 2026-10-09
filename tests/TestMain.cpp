#include "TestSupport.h"

#include "LoudnessMeter.h"
#include "MeasurementEngine.h"
#include "MeterSettings.h"
#include "TransportTracker.h"

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
    // Integrated loudness across silent passages
    // -----------------------------------------------------------------------

    void testSilenceInMiddleKeepsValue()
    {
        std::printf ("testSilenceInMiddleKeepsValue\n");

        // tone - silence - tone. Silent blocks are below the absolute gate and
        // must not disturb the accumulated integrated value.
        auto data = makeSine (2, 48000.0, 20.0, 1000.0, -23.0);
        const auto gap = makeSilence (2, 48000.0, 15.0);
        const auto tone = makeSine (2, 48000.0, 20.0, 1000.0, -23.0);
        data.insert (data.end(), gap.begin(), gap.end());
        data.insert (data.end(), tone.begin(), tone.end());

        TEST_CHECK_NEAR (measure (2, 48000.0, data), -23.0, 0.1);
    }

    void testMultipleTakesSeparatedBySilence()
    {
        std::printf ("testMultipleTakesSeparatedBySilence\n");

        // Several "takes" at the same level, separated by long silent gaps,
        // must read as one continuous integrated value.
        std::vector<float> data;
        for (int take = 0; take < 4; ++take)
        {
            const auto tone = makeSine (2, 48000.0, 10.0, 1000.0, -23.0);
            data.insert (data.end(), tone.begin(), tone.end());

            if (take < 3)
            {
                const auto gap = makeSilence (2, 48000.0, 8.0);
                data.insert (data.end(), gap.begin(), gap.end());
            }
        }

        TEST_CHECK_NEAR (measure (2, 48000.0, data), -23.0, 0.1);
    }

    // -----------------------------------------------------------------------
    // Colour zones / configuration model
    // -----------------------------------------------------------------------

    void testZoneClassification()
    {
        std::printf ("testZoneClassification\n");

        const MeterSettings s; // green [-24, -14], red >= -14

        TEST_CHECK (s.zoneForValue (-30.0) == MeterZone::normal);
        TEST_CHECK (s.zoneForValue (-24.0) == MeterZone::green);
        TEST_CHECK (s.zoneForValue (-20.0) == MeterZone::green);
        TEST_CHECK (s.zoneForValue (-14.0) == MeterZone::red);   // red wins at the shared boundary
        TEST_CHECK (s.zoneForValue (-6.0)  == MeterZone::red);
        TEST_CHECK (s.zoneForValue (-24.01) == MeterZone::normal);

        // A gap between the green band and the red threshold is normal, not red.
        MeterSettings g;
        g.greenLow = -24.0;
        g.greenHigh = -16.0;
        g.redThreshold = -14.0;
        TEST_CHECK (g.zoneForValue (-15.0) == MeterZone::normal);
    }

    void testZoneSanitisation()
    {
        std::printf ("testZoneSanitisation\n");

        // Reversed / out-of-range values must be ordered and clamped.
        MeterSettings bad;
        bad.greenLow = -10.0;
        bad.greenHigh = -20.0;
        bad.redThreshold = -25.0;

        const auto fixed = bad.sanitised();
        TEST_CHECK (fixed.greenLow <= fixed.greenHigh);
        TEST_CHECK (fixed.greenHigh <= fixed.redThreshold);
        TEST_CHECK (fixed.greenLow >= MeterSettings::scaleMinLufs);
        TEST_CHECK (fixed.redThreshold <= MeterSettings::scaleMaxLufs);

        MeterSettings extreme;
        extreme.greenLow = -1000.0;
        extreme.greenHigh = 1000.0;
        extreme.redThreshold = 1000.0;

        const auto clamped = extreme.sanitised();
        TEST_CHECK_NEAR (clamped.greenLow, MeterSettings::scaleMinLufs, 1.0e-9);
        TEST_CHECK_NEAR (clamped.greenHigh, MeterSettings::scaleMaxLufs, 1.0e-9);
        TEST_CHECK_NEAR (clamped.redThreshold, MeterSettings::scaleMaxLufs, 1.0e-9);
    }

    void testZoneBoundaryConsistency()
    {
        std::printf ("testZoneBoundaryConsistency\n");

        // Reproduces the reported case: greenHigh = -14.0, redThreshold = -13.9,
        // reading = -13.9. The reading must be red, not default.
        MeterSettings s;
        s.greenLow = -16.0;
        s.greenHigh = -14.0;
        s.redThreshold = -13.9;

        TEST_CHECK (s.zoneForValue (-13.9) == MeterZone::red);
        TEST_CHECK (s.zoneForValue (-13.8) == MeterZone::red);
        TEST_CHECK (s.zoneForValue (-13.9000001) == MeterZone::red); // epsilon
        TEST_CHECK (s.zoneForValue (-13.95) == MeterZone::normal);   // gap is normal
        TEST_CHECK (s.zoneForValue (-14.0) == MeterZone::green);     // reaches green
        TEST_CHECK (s.zoneForValue (-16.0) == MeterZone::green);
        TEST_CHECK (s.zoneForValue (-16.01) == MeterZone::normal);

        // Same shared rule for meter and reading: green zone -> green, at/over
        // the red threshold -> red, otherwise default.
        MeterSettings d;
        TEST_CHECK (d.zoneForValue (-20.0) == MeterZone::green);
        TEST_CHECK (d.zoneForValue (-14.0) == MeterZone::red);
        TEST_CHECK (d.zoneForValue (-28.0) == MeterZone::normal);

        const auto c = s.sanitised();
        TEST_CHECK (c.zoneForValue (-13.9) == MeterZone::red);
        TEST_CHECK (c.zoneForValue (-14.0) == MeterZone::green);
    }

    void testSettingsPersistence()
    {
        std::printf ("testSettingsPersistence\n");

        MeterSettings s;
        s.orientation = MeterOrientation::vertical;
        s.size = MeterSizeMode::slim;
        s.showLabel = false;
        s.showScale = false;
        s.greenLow = -25.0;
        s.greenHigh = -17.0;
        s.redThreshold = -12.0;
        s.defaultColour = 0xff112233;
        s.greenColour   = 0xff00ff00;
        s.redColour     = 0xffff0000;

        const auto xml = s.toXml();
        TEST_CHECK (xml != nullptr);

        const auto restored = MeterSettings::fromXml (*xml);
        TEST_CHECK (restored == s);
        TEST_CHECK (restored.defaultColour == 0xff112233);
        TEST_CHECK (restored.redColour == 0xffff0000);

        // A legacy state (only the original four attributes) must load with the
        // new fields defaulted, not corrupted.
        juce::XmlElement legacy ("LUFSMeterSettings");
        legacy.setAttribute ("orientation", 1);
        legacy.setAttribute ("size", 0);
        legacy.setAttribute ("showLabel", false);
        legacy.setAttribute ("showScale", false);

        const auto fromLegacy = MeterSettings::fromXml (legacy);
        TEST_CHECK (fromLegacy.orientation == MeterOrientation::vertical);
        TEST_CHECK (fromLegacy.size == MeterSizeMode::slim);
        TEST_CHECK (fromLegacy.greenHigh >= fromLegacy.greenLow);
        TEST_CHECK (fromLegacy.redThreshold >= fromLegacy.greenHigh);
    }

    // -----------------------------------------------------------------------
    // Measurement-pass lifecycle (transport + position)
    // -----------------------------------------------------------------------

    TransportTracker::BlockContext playing (double positionSeconds, double nowMs,
                                            int numSamples = 480, double sampleRate = 48000.0)
    {
        TransportTracker::BlockContext b;
        b.transportPlaying = true;
        b.hasPosition = true;
        b.positionSeconds = positionSeconds;
        b.numSamples = numSamples;
        b.sampleRate = sampleRate;
        b.nowMs = nowMs;
        return b;
    }

    void testLifecycleContinuousPlayback()
    {
        std::printf ("testLifecycleContinuousPlayback\n");

        TransportTracker t;
        t.reset();

        // First playing block always starts a pass.
        TEST_CHECK (t.update (playing (10.0, 0.0)));

        // Continuous, jitter-free playback: no further resets.
        TEST_CHECK (! t.update (playing (10.01, 10.0)));
        TEST_CHECK (! t.update (playing (10.02, 20.0)));

        // Small timing jitter must not be mistaken for a seek.
        TEST_CHECK (! t.update (playing (10.02 + 0.01, 30.0)));
        TEST_CHECK (! t.update (playing (10.03 + 0.02, 40.0)));
    }

    void testLifecycleSilenceDoesNotReset()
    {
        std::printf ("testLifecycleSilenceDoesNotReset\n");

        // The tracker has no audio input at all, so a "silent" block cannot be
        // distinguished from any other block: silence can never reset.
        TransportTracker t;
        t.reset();
        TEST_CHECK (t.update (playing (5.0, 0.0)));

        for (int i = 1; i <= 50; ++i)
            TEST_CHECK (! t.update (playing (5.0 + 0.01 * i, 10.0 * i)));
    }

    void testLifecycleSeekResets()
    {
        std::printf ("testLifecycleSeekResets\n");

        TransportTracker t;
        t.reset();
        t.update (playing (10.0, 0.0));
        t.update (playing (10.01, 10.0));

        // Forward seek -> new pass.
        TEST_CHECK (t.update (playing (500.0, 20.0)));

        // ...and playback continues cleanly afterwards.
        TEST_CHECK (! t.update (playing (500.01, 30.0)));
    }

    void testLifecycleLoopWrapResets()
    {
        std::printf ("testLifecycleLoopWrapResets\n");

        TransportTracker t;
        t.reset();
        t.update (playing (99.98, 0.0));

        // Loop wraps back to its start -> new pass.
        TEST_CHECK (t.update (playing (10.00, 10.0)));
        TEST_CHECK (! t.update (playing (10.01, 20.0)));
    }

    void testLifecycleStopAndRestart()
    {
        std::printf ("testLifecycleStopAndRestart\n");

        TransportTracker t;
        t.reset();
        t.update (playing (10.0, 0.0));
        t.update (playing (10.01, 10.0));

        // Transport stopped: no new pass, value stays frozen.
        auto stopped = TransportTracker::BlockContext{};
        stopped.transportPlaying = false;
        stopped.hasPosition = true;
        stopped.positionSeconds = 10.02;
        stopped.nowMs = 20.0;
        TEST_CHECK (! t.update (stopped));

        // Play again from the *same* position -> new pass.
        TEST_CHECK (t.update (playing (10.02, 30.0)));

        // Repeated play/stop cycles each start a fresh pass.
        for (int i = 0; i < 3; ++i)
        {
            stopped.nowMs = 100.0 + (double) i * 100.0;
            TEST_CHECK (! t.update (stopped));
            TEST_CHECK (t.update (playing (10.02, stopped.nowMs + 10.0)));
        }
    }

    void testLifecycleGapResets()
    {
        std::printf ("testLifecycleGapResets\n");

        TransportTracker t;
        t.reset();
        t.update (playing (10.0, 0.0));

        // Host claims to be playing but stopped calling processBlock for > 1 s.
        TEST_CHECK (t.update (playing (10.01, 5000.0)));
    }

    void testLifecycleWithoutPositionStillPlays()
    {
        std::printf ("testLifecycleWithoutPositionStillPlays\n");

        // Hosts that report playing but no position accumulate continuously.
        TransportTracker t;
        t.reset();

        auto b = playing (0.0, 0.0);
        b.hasPosition = false;
        TEST_CHECK (t.update (b));

        for (int i = 1; i <= 20; ++i)
        {
            b.nowMs = 10.0 * i;
            TEST_CHECK (! t.update (b));
        }
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
    testSilenceInMiddleKeepsValue();
    testMultipleTakesSeparatedBySilence();
    testZoneClassification();
    testZoneSanitisation();
    testZoneBoundaryConsistency();
    testSettingsPersistence();
    testLifecycleContinuousPlayback();
    testLifecycleSilenceDoesNotReset();
    testLifecycleSeekResets();
    testLifecycleLoopWrapResets();
    testLifecycleStopAndRestart();
    testLifecycleGapResets();
    testLifecycleWithoutPositionStillPlays();
    testEnginePipeline();

    std::printf ("\n%d checks, %d failures\n", test::checkCount(), test::failureCount());
    return test::failureCount() == 0 ? 0 : 1;
}
