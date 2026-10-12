#include "TestSupport.h"

#include "EditorLayout.h"
#include "EditorWindowSizes.h"
#include "LoudnessMeter.h"
#include "MeasurementEngine.h"
#include "MeterSettings.h"
#include "TransportTracker.h"
#include "ValueAnimation.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>
#include <thread>
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

    /** Feeds interleaved data into an existing prepared meter in chunks. */
    void feedInterleaved (LoudnessMeter& meter, int channels,
                          const std::vector<float>& data, int chunkFrames = 1024)
    {
        const int totalFrames = (int) (data.size() / (size_t) channels);

        for (int pos = 0; pos < totalFrames; pos += chunkFrames)
        {
            const int n = std::min (chunkFrames, totalFrames - pos);
            meter.processInterleaved (data.data() + (size_t) pos * (size_t) channels,
                                      (unsigned long) n);
        }
    }

    /** Interleaved sine whose phase is offset, identical in every channel. */
    std::vector<float> makeSinePhase (int channels, double sampleRate, double seconds,
                                      double frequency, double peakDbfs, double phaseRadians)
    {
        const int frames = (int) std::llround (sampleRate * seconds);
        std::vector<float> data ((size_t) frames * (size_t) channels);
        const double amplitude = std::pow (10.0, peakDbfs / 20.0);

        for (int i = 0; i < frames; ++i)
        {
            const auto s = (float) (amplitude * std::sin (2.0 * pi * frequency
                                                              * (double) i / sampleRate
                                                          + phaseRadians));

            for (int c = 0; c < channels; ++c)
                data[(size_t) i * (size_t) channels + (size_t) c] = s;
        }

        return data;
    }

    /** Maximum absolute sample value in an interleaved buffer. */
    double samplePeak (const std::vector<float>& data)
    {
        double peak = 0.0;

        for (float s : data)
            peak = std::max (peak, (double) std::abs (s));

        return peak;
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
    // Momentary / Short-Term / True Peak
    // -----------------------------------------------------------------------

    void testMomentaryReference()
    {
        std::printf ("testMomentaryReference\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, makeSine (2, 48000.0, 5.0, 1000.0, -23.0));

        double v = 0.0;
        TEST_CHECK (m.getMomentaryLufs (v));
        TEST_CHECK_NEAR (v, -23.0, 0.1);
    }

    void testMomentaryUnavailableBeforeWindow()
    {
        std::printf ("testMomentaryUnavailableBeforeWindow\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));

        // Less than one 400 ms window -> no Momentary value yet.
        feedInterleaved (m, 2, makeSine (2, 48000.0, 0.2, 1000.0, -23.0));

        double v = 0.0;
        TEST_CHECK (! m.getMomentaryLufs (v));
    }

    void testMomentarySilentWindow()
    {
        std::printf ("testMomentarySilentWindow\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, makeSine (2, 48000.0, 10.0, 1000.0, -23.0));

        double v = 0.0;
        TEST_CHECK (m.getMomentaryLufs (v));

        // A fully silent 400 ms window has no meaningful loudness and must be
        // reported as unavailable, but must not disturb the integrated result.
        feedInterleaved (m, 2, makeSilence (2, 48000.0, 0.5));
        TEST_CHECK (! m.getMomentaryLufs (v));

        double i = 0.0;
        TEST_CHECK (m.getIntegratedLufs (i));
        TEST_CHECK_NEAR (i, -23.0, 0.15);
    }

    void testShortTermReference()
    {
        std::printf ("testShortTermReference\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, makeSine (2, 48000.0, 5.0, 1000.0, -23.0));

        double v = 0.0;
        TEST_CHECK (m.getShortTermLufs (v));
        TEST_CHECK_NEAR (v, -23.0, 0.1);
    }

    void testShortTermUnavailableBeforeWindow()
    {
        std::printf ("testShortTermUnavailableBeforeWindow\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));

        // 2 s is not enough for the 3 s window.
        feedInterleaved (m, 2, makeSine (2, 48000.0, 2.0, 1000.0, -23.0));

        double v = 0.0;
        TEST_CHECK (! m.getShortTermLufs (v));

        // Cross the 3 s boundary.
        feedInterleaved (m, 2, makeSine (2, 48000.0, 1.5, 1000.0, -23.0));
        TEST_CHECK (m.getShortTermLufs (v));
        TEST_CHECK_NEAR (v, -23.0, 0.1);
    }

    void testShortTermRollingTransition()
    {
        std::printf ("testShortTermRollingTransition\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, makeSine (2, 48000.0, 5.0, 1000.0, -23.0));

        double v = 0.0;
        TEST_CHECK (m.getShortTermLufs (v));
        TEST_CHECK_NEAR (v, -23.0, 0.1);

        // After a full 3 s at the new level the rolling window is entirely -33.
        feedInterleaved (m, 2, makeSine (2, 48000.0, 5.0, 1000.0, -33.0));
        TEST_CHECK (m.getShortTermLufs (v));
        TEST_CHECK_NEAR (v, -33.0, 0.1);
    }

    void testShortTermSilenceAfterActive()
    {
        std::printf ("testShortTermSilenceAfterActive\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, makeSine (2, 48000.0, 5.0, 1000.0, -23.0));

        // One second of silence: the 3 s window still contains audio, so S stays
        // valid and simply drops.
        feedInterleaved (m, 2, makeSilence (2, 48000.0, 1.0));

        double v = 0.0;
        TEST_CHECK (m.getShortTermLufs (v));
        TEST_CHECK (v < -23.0);

        // A fully silent 3 s window -> unavailable, without resetting anything.
        feedInterleaved (m, 2, makeSilence (2, 48000.0, 2.5));
        TEST_CHECK (! m.getShortTermLufs (v));
    }

    void testShortTermLowLevelUngated()
    {
        std::printf ("testShortTermLowLevelUngated\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, makeSine (2, 48000.0, 5.0, 1000.0, -75.0));

        // Short-term is ungated, so it keeps a value below the -70 LUFS
        // integrated gate.
        double s = 0.0;
        TEST_CHECK (m.getShortTermLufs (s));
        TEST_CHECK_NEAR (s, -75.0, 0.1);

        double i = 0.0;
        TEST_CHECK (! m.getIntegratedLufs (i));
    }

    void testMomentaryShortTermSampleRates()
    {
        std::printf ("testMomentaryShortTermSampleRates\n");

        for (double sr : { 44100.0, 88200.0, 96000.0, 192000.0 })
        {
            LoudnessMeter m;
            TEST_CHECK (m.prepare (2, (unsigned long) sr));
            feedInterleaved (m, 2, makeSine (2, sr, 5.0, 1000.0, -23.0));

            double mv = 0.0, sv = 0.0;
            TEST_CHECK (m.getMomentaryLufs (mv));
            TEST_CHECK (m.getShortTermLufs (sv));
            TEST_CHECK_NEAR (mv, -23.0, 0.1);
            TEST_CHECK_NEAR (sv, -23.0, 0.1);
        }
    }

    void testMomentaryShortTermBlockSize()
    {
        std::printf ("testMomentaryShortTermBlockSize\n");

        const auto data = makeSine (2, 48000.0, 5.0, 1000.0, -23.0);

        for (int chunk : { 1, 512, 65536 })
        {
            LoudnessMeter m;
            TEST_CHECK (m.prepare (2, 48000));
            feedInterleaved (m, 2, data, chunk);

            double mv = 0.0, sv = 0.0;
            TEST_CHECK (m.getMomentaryLufs (mv));
            TEST_CHECK (m.getShortTermLufs (sv));
            TEST_CHECK_NEAR (mv, -23.0, 0.1);
            TEST_CHECK_NEAR (sv, -23.0, 0.1);
        }
    }

    void testMomentaryShortTermMono()
    {
        std::printf ("testMomentaryShortTermMono\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (1, 48000));
        feedInterleaved (m, 1, makeSine (1, 48000.0, 5.0, 1000.0, -23.0));

        double mv = 0.0, sv = 0.0;
        TEST_CHECK (m.getMomentaryLufs (mv));
        TEST_CHECK (m.getShortTermLufs (sv));
        TEST_CHECK_NEAR (mv, -26.0, 0.15); // single channel: half the energy
        TEST_CHECK_NEAR (sv, -26.0, 0.15);
    }

    void testTruePeakInterSample()
    {
        std::printf ("testTruePeakInterSample\n");

        const double sr = 48000.0;
        const int frames = (int) sr;

        // fs/4 sine with a 45 degree phase: every sample is +/- A/sqrt(2)
        // (sample peak -3.01 dBFS) while the reconstructed peak is A (0 dBTP).
        auto data = makeSinePhase (2, sr, 1.0, sr / 4.0, 0.0, pi / 4.0);

        const double samplePeakDb = 20.0 * std::log10 (samplePeak (data));
        TEST_CHECK_NEAR (samplePeakDb, -3.01, 0.1);

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, data);

        double tp = 0.0;
        TEST_CHECK (m.getTruePeakDbtp (tp));

        // True peak clearly exceeds the sample peak and is close to 0 dBTP.
        TEST_CHECK (tp > samplePeakDb + 2.0);
        TEST_CHECK_NEAR (tp, 0.0, 0.5);

        juce::ignoreUnused (frames);
    }

    void testTruePeakSampleRates()
    {
        std::printf ("testTruePeakSampleRates\n");

        // libebur128 oversamples 4x below 96 kHz, 2x for 96-192 kHz and not at
        // all at >= 192 kHz. An fs/4 sine reaches its true peak exactly halfway
        // between samples, so the 2x/4x interpolators recover it; at 192 kHz the
        // reported "true peak" is the sample peak (-3.01 dBFS).
        for (double sr : { 44100.0, 96000.0, 192000.0 })
        {
            auto data = makeSinePhase (2, sr, 1.0, sr / 4.0, 0.0, pi / 4.0);

            LoudnessMeter m;
            TEST_CHECK (m.prepare (2, (unsigned long) sr));
            feedInterleaved (m, 2, data);

            double tp = 0.0;
            TEST_CHECK (m.getTruePeakDbtp (tp));

            if (sr < 192000.0)
                TEST_CHECK_NEAR (tp, 0.0, 0.5);
            else
                TEST_CHECK_NEAR (tp, -3.01, 0.1);
        }
    }

    void testTruePeakAcrossChannels()
    {
        std::printf ("testTruePeakAcrossChannels\n");

        const double sr = 48000.0;
        const int frames = (int) sr;
        const auto tone = makeSinePhase (2, sr, 1.0, sr / 4.0, -6.02, pi / 4.0);

        // Left silent, right carries the tone: the reported peak is the maximum
        // across channels, i.e. the right channel.
        std::vector<float> data ((size_t) frames * 2, 0.0f);

        for (int i = 0; i < frames; ++i)
            data[(size_t) i * 2 + 1] = tone[(size_t) i * 2 + 1];

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));
        feedInterleaved (m, 2, data);

        double tp = 0.0;
        TEST_CHECK (m.getTruePeakDbtp (tp));
        TEST_CHECK_NEAR (tp, -6.02, 0.5);
    }

    void testTruePeakSilenceAndReset()
    {
        std::printf ("testTruePeakSilenceAndReset\n");

        LoudnessMeter m;
        TEST_CHECK (m.prepare (2, 48000));

        double tp = 0.0;
        TEST_CHECK (! m.getTruePeakDbtp (tp)); // digital silence -> unavailable

        feedInterleaved (m, 2, makeSine (2, 48000.0, 1.0, 1000.0, -6.0));
        TEST_CHECK (m.getTruePeakDbtp (tp));
        TEST_CHECK (tp > -7.0);

        // Reset must clear the accumulated maximum.
        TEST_CHECK (m.reset());
        TEST_CHECK (! m.getTruePeakDbtp (tp));
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

    void testScaleProportion()
    {
        std::printf ("testScaleProportion\n");

        // The meter bar and the Compact fill share this mapping: 0 at the scale
        // minimum, 1 at the maximum, linear in between. The probes are expressed
        // against the constants, so widening the scale (currently -50 … 0) does
        // not silently invalidate the test.
        const double min = MeterSettings::scaleMinLufs;
        const double max = MeterSettings::scaleMaxLufs;
        const double range = max - min;

        TEST_CHECK_NEAR (MeterSettings::scaleProportion (min), 0.0, 1.0e-9);
        TEST_CHECK_NEAR (MeterSettings::scaleProportion (max), 1.0, 1.0e-9);
        TEST_CHECK_NEAR (MeterSettings::scaleProportion (min + range * 0.25), 0.25, 1.0e-9);
        TEST_CHECK_NEAR (MeterSettings::scaleProportion (min + range * 0.5), 0.5, 1.0e-9);
        TEST_CHECK_NEAR (MeterSettings::scaleProportion (min + range * 0.75), 0.75, 1.0e-9);

        // The scale covers the whole practical loudness range, and typical
        // programme material (-14 … -16 LUFS) sits in its upper third.
        TEST_CHECK (min <= -50.0);
        TEST_CHECK (MeterSettings::scaleProportion (-16.0) > 0.66);
        TEST_CHECK (MeterSettings::scaleProportion (-14.0) > 0.66);

        // Values outside the scale clamp to [0, 1] (no fill below, full above).
        TEST_CHECK_NEAR (MeterSettings::scaleProportion (min - 10.0), 0.0, 1.0e-9);
        TEST_CHECK_NEAR (MeterSettings::scaleProportion (max + 5.0), 1.0, 1.0e-9);
    }

    void testValueAnimation()
    {
        std::printf ("testValueAnimation\n");

        constexpr double duration = ValueAnimation::defaultDurationSeconds;
        constexpr double from = -50.0;
        constexpr double target = -23.0;

        // Nothing is displayed until a fill has been started.
        ValueAnimation animation;
        TEST_CHECK (! animation.hasValue());
        TEST_CHECK (! animation.isRunning());

        // A fill starts at its starting point and reaches the target exactly at
        // the end of the duration, moving monotonically in between.
        animation.start (from, target);
        TEST_CHECK (animation.hasValue());
        TEST_CHECK (animation.isRunning());
        TEST_CHECK_NEAR (animation.value(), from, 1.0e-9);

        double previous = from;

        for (int frame = 0; frame < 10; ++frame)
        {
            const double value = animation.advance (duration / 10.0);
            TEST_CHECK (value > previous);
            TEST_CHECK (value <= target + 1.0e-9);
            previous = value;
        }

        TEST_CHECK (! animation.isRunning());
        TEST_CHECK_NEAR (animation.value(), target, 1.0e-9);

        // The frame rate does not change the result: the same duration reached
        // in one step or in many lands on the same value.
        ValueAnimation coarse;
        coarse.start (from, target);
        coarse.advance (duration * 0.5);

        ValueAnimation fine;
        fine.start (from, target);

        for (int frame = 0; frame < 50; ++frame)
            fine.advance (duration * 0.01);

        TEST_CHECK_NEAR (coarse.value(), fine.value(), 1.0e-9);

        // Once the fill is over, the meter follows the measurement exactly.
        fine.advance (duration * 0.5);
        TEST_CHECK (! fine.isRunning());
        fine.setTarget (-16.0);
        TEST_CHECK_NEAR (fine.value(), -16.0, 1.0e-9);

        // A target that moves mid-fill is eased towards, then reached exactly.
        ValueAnimation moving;
        moving.start (from, -40.0);
        moving.advance (duration * 0.25);
        const double quarter = moving.value();

        TEST_CHECK (quarter > from && quarter < -40.0);

        moving.setTarget (-20.0);
        moving.advance (duration * 0.25);

        TEST_CHECK (moving.value() > quarter);            // still climbing
        TEST_CHECK (moving.value() < -20.0);              // not yet there
        TEST_CHECK (moving.isRunning());

        moving.advance (duration * 0.5);                  // past the duration
        TEST_CHECK (! moving.isRunning());
        TEST_CHECK_NEAR (moving.value(), -20.0, 1.0e-9);  // landed exactly

        // Dropping the value empties the indicator again, so the next pass
        // starts a fresh fill rather than resuming the old one.
        moving.clear();
        TEST_CHECK (! moving.hasValue());
        TEST_CHECK (! moving.isRunning());

        moving.advance (duration);
        TEST_CHECK (! moving.hasValue());

        moving.start (from, target);
        TEST_CHECK (moving.isRunning());
        TEST_CHECK_NEAR (moving.value(), from, 1.0e-9);

        // A new pass starts from the bottom of the scale, not from whatever was
        // on screen before: the reset has to be visible.
        ValueAnimation newPass;
        newPass.start (from, -23.0);
        newPass.advance (duration * 0.5);

        const double midway = newPass.value();
        TEST_CHECK (midway > from && midway < -23.0);

        newPass.clear();
        TEST_CHECK (! newPass.hasValue());

        newPass.start (from, -16.0);
        TEST_CHECK_NEAR (newPass.value(), from, 1.0e-9);
        TEST_CHECK (newPass.advance (duration) > from);
        TEST_CHECK_NEAR (newPass.value(), -16.0, 1.0e-9);
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
        s.displayMode = MeterDisplayMode::compact;
        s.showLabel = false;
        s.showScale = false;
        s.greenLow = -25.0;
        s.greenHigh = -17.0;
        s.redThreshold = -12.0;
        s.truePeakRedThresholdDbtp = -2.5;
        s.defaultColour = 0xff112233;
        s.greenColour   = 0xff00ff00;
        s.redColour     = 0xffff0000;

        const auto xml = s.toXml();
        TEST_CHECK (xml != nullptr);
        TEST_CHECK (xml->hasAttribute ("displayMode"));

        const auto restored = MeterSettings::fromXml (*xml);
        TEST_CHECK (restored == s);
        TEST_CHECK (restored.displayMode == MeterDisplayMode::compact);
        TEST_CHECK_NEAR (restored.truePeakRedThresholdDbtp, -2.5, 1.0e-9);
        TEST_CHECK (restored.defaultColour == 0xff112233);
        TEST_CHECK (restored.redColour == 0xffff0000);

        // Saving and loading repeatedly must never change the selected mode.
        auto round = s;
        for (int i = 0; i < 5; ++i)
        {
            const auto x = round.toXml();
            round = MeterSettings::fromXml (*x);
            TEST_CHECK (round.displayMode == MeterDisplayMode::compact);
        }

        TEST_CHECK (round == s);

        // Every mode survives a round trip.
        for (const auto mode : { MeterDisplayMode::vertical,
                                 MeterDisplayMode::horizontal,
                                 MeterDisplayMode::compact })
        {
            MeterSettings m;
            m.displayMode = mode;
            TEST_CHECK (MeterSettings::fromXml (*m.toXml()).displayMode == mode);
        }

        // An out-of-range mode is clamped by sanitise(), not left invalid.
        MeterSettings bogus;
        bogus.displayMode = static_cast<MeterDisplayMode> (9);
        TEST_CHECK (bogus.sanitised().displayMode == MeterDisplayMode::compact);
        bogus.displayMode = static_cast<MeterDisplayMode> (-3);
        TEST_CHECK (bogus.sanitised().displayMode == MeterDisplayMode::vertical);

        // The True Peak warning threshold is clamped to its own range, so a state
        // written by another build cannot put the warning outside the scale.
        TEST_CHECK_NEAR (MeterSettings().truePeakRedThresholdDbtp, -1.0, 1.0e-9);

        MeterSettings loud;
        loud.truePeakRedThresholdDbtp = 5.0;
        TEST_CHECK_NEAR (loud.sanitised().truePeakRedThresholdDbtp,
                         MeterSettings::truePeakRedMaxDbtp, 1.0e-9);

        MeterSettings quiet;
        quiet.truePeakRedThresholdDbtp = -100.0;
        TEST_CHECK_NEAR (quiet.sanitised().truePeakRedThresholdDbtp,
                         MeterSettings::truePeakRedMinDbtp, 1.0e-9);

        // A state saved before the threshold existed keeps the default.
        juce::XmlElement legacyState ("LUFSMeterSettings");
        legacyState.setAttribute ("displayMode", 0);
        TEST_CHECK_NEAR (MeterSettings::fromXml (legacyState).truePeakRedThresholdDbtp,
                         -1.0, 1.0e-9);
    }

    void testLegacyDisplayModeMigration()
    {
        std::printf ("testLegacyDisplayModeMigration\n");

        // A legacy state has only `orientation` and `size`. Slim and Compact
        // both collapse to their orientation's single display mode.
        auto legacy = [] (int orientation, int size)
        {
            juce::XmlElement xml ("LUFSMeterSettings");
            xml.setAttribute ("orientation", orientation);
            xml.setAttribute ("size", size);
            return MeterSettings::fromXml (xml);
        };

        TEST_CHECK (legacy (0, 0).displayMode == MeterDisplayMode::horizontal); // H slim
        TEST_CHECK (legacy (0, 1).displayMode == MeterDisplayMode::horizontal); // H compact
        TEST_CHECK (legacy (1, 0).displayMode == MeterDisplayMode::vertical);   // V slim
        TEST_CHECK (legacy (1, 1).displayMode == MeterDisplayMode::vertical);   // V compact

        // A state with no orientation at all defaults to Horizontal.
        juce::XmlElement bare ("LUFSMeterSettings");
        TEST_CHECK (MeterSettings::fromXml (bare).displayMode == MeterDisplayMode::horizontal);

        // `displayMode` takes precedence over a conflicting legacy pair.
        juce::XmlElement mixed ("LUFSMeterSettings");
        mixed.setAttribute ("displayMode", 2); // Compact
        mixed.setAttribute ("orientation", 1); // legacy Vertical
        mixed.setAttribute ("size", 0);
        TEST_CHECK (MeterSettings::fromXml (mixed).displayMode == MeterDisplayMode::compact);

        // An older build cannot know Compact: the legacy attributes written for
        // a Compact state must describe Horizontal, and the new field must
        // still restore Compact.
        MeterSettings compact;
        compact.displayMode = MeterDisplayMode::compact;
        const auto compactXml = compact.toXml();
        TEST_CHECK (compactXml->getIntAttribute ("orientation", -1) == 0);
        TEST_CHECK (compactXml->getIntAttribute ("size", -1) == 1);
        TEST_CHECK (MeterSettings::fromXml (*compactXml).displayMode == MeterDisplayMode::compact);
    }

    // -----------------------------------------------------------------------
    // Editor geometry and remembered window sizes
    // -----------------------------------------------------------------------

    void testEditorLayoutTable()
    {
        std::printf ("testEditorLayoutTable\n");

        for (const auto mode : { MeterDisplayMode::vertical,
                                 MeterDisplayMode::horizontal,
                                 MeterDisplayMode::compact })
        {
            const auto size = EditorLayout::forMode (mode);

            TEST_CHECK (size.preferredWidth >= size.minWidth);
            TEST_CHECK (size.preferredHeight >= size.minHeight);
            TEST_CHECK (size.minWidth > 2 * EditorLayout::outerPadding + EditorLayout::buttonSize * 2);
            TEST_CHECK (size.minHeight > 2 * EditorLayout::outerPadding + EditorLayout::controlStripHeight);
        }

        // The cross-axis compaction: Vertical is the slim one (height kept),
        // Horizontal the short one (width kept).
        const auto vertical = EditorLayout::forMode (MeterDisplayMode::vertical);
        const auto horizontal = EditorLayout::forMode (MeterDisplayMode::horizontal);
        TEST_CHECK (vertical.preferredWidth < horizontal.preferredWidth);
        TEST_CHECK (horizontal.preferredHeight < vertical.preferredHeight);

        // Only Horizontal overlays the buttons on the meter's corners.
        TEST_CHECK (EditorLayout::overlaysControls (MeterDisplayMode::horizontal));
        TEST_CHECK (! EditorLayout::overlaysControls (MeterDisplayMode::vertical));
        TEST_CHECK (! EditorLayout::overlaysControls (MeterDisplayMode::compact));

        // The minimum Horizontal width has to leave the whole TP / LS / LM row
        // clear of the overlaid buttons: About at the top-left, Settings at the
        // top-right. The row measures about 260 px at the 0.9 readout scale
        // (MeterComponent), and each button owns one end of it.
        const int horizontalInset = EditorLayout::rowInsetFor (MeterDisplayMode::horizontal);
        const int readoutRoom = horizontal.minWidth - 2 * EditorLayout::outerPadding
                                    - 2 * horizontalInset;
        TEST_CHECK (horizontalInset > EditorLayout::buttonSize);
        TEST_CHECK (readoutRoom >= 290);

        // Vertical's readouts sit beside the bar, clear of the corner buttons,
        // so it reserves nothing for them.
        TEST_CHECK (EditorLayout::rowInsetFor (MeterDisplayMode::vertical) == 0);
        TEST_CHECK (EditorLayout::rowInsetFor (MeterDisplayMode::compact)
                        > EditorLayout::buttonSize);

        // About sits in the top-left corner and Settings in the top-right one in
        // every mode, and no two buttons overlap.
        const int innerWidth = horizontal.minWidth - 2 * EditorLayout::outerPadding;
        const int innerHeight = horizontal.minHeight - 2 * EditorLayout::outerPadding;

        for (const auto mode : { MeterDisplayMode::vertical,
                                 MeterDisplayMode::horizontal,
                                 MeterDisplayMode::compact })
        {
            const auto placement = EditorLayout::controlsIn (innerWidth, innerHeight, mode);

            TEST_CHECK (placement.aboutX == 0);
            TEST_CHECK (placement.settingsX + EditorLayout::buttonSize == innerWidth);
            TEST_CHECK (placement.aboutY >= 0);
            TEST_CHECK (placement.aboutY + EditorLayout::buttonSize
                            <= EditorLayout::controlStripHeight);
            TEST_CHECK (placement.settingsY == placement.aboutY);

            TEST_CHECK (placement.resetX == 0);
            TEST_CHECK (placement.resetY >= 0);
            TEST_CHECK (placement.resetY + EditorLayout::buttonSize <= innerHeight);

            // Reset never shares a row with the top buttons, and never overlaps
            // the Settings button.
            TEST_CHECK (placement.resetY != placement.aboutY);
            TEST_CHECK (placement.resetX + EditorLayout::buttonSize <= placement.settingsX);
        }

        // Reset is a corner button in the segmented modes and sits at half
        // height in Compact, where the reading is centred.
        const auto verticalPlacement = EditorLayout::controlsIn (innerWidth, innerHeight,
                                                                 MeterDisplayMode::vertical);
        const auto horizontalPlacement = EditorLayout::controlsIn (innerWidth, innerHeight,
                                                                   MeterDisplayMode::horizontal);
        const auto compactPlacement = EditorLayout::controlsIn (innerWidth, innerHeight,
                                                                MeterDisplayMode::compact);

        TEST_CHECK (verticalPlacement.resetY == innerHeight - EditorLayout::buttonSize);
        TEST_CHECK (horizontalPlacement.resetY == innerHeight - EditorLayout::buttonSize);
        TEST_CHECK (compactPlacement.resetY == (innerHeight - EditorLayout::buttonSize) / 2);
    }

    void testEditorWindowSizes()
    {
        std::printf ("testEditorWindowSizes\n");

        // Never resized: every mode is "unset", so the editor opens each of them
        // at its preferred size.
        EditorWindowSizes sizes;

        for (const auto mode : { MeterDisplayMode::vertical,
                                 MeterDisplayMode::horizontal,
                                 MeterDisplayMode::compact })
            TEST_CHECK (sizes.sizeFor (mode).isUnset());

        sizes.setSize (MeterDisplayMode::vertical, 180, 344);
        sizes.setSize (MeterDisplayMode::horizontal, 400, 130);
        sizes.setSize (MeterDisplayMode::compact, 210, 180);

        const EditorWindowSizes::Size expectedVertical { 180, 344 };
        TEST_CHECK (sizes.sizeFor (MeterDisplayMode::vertical) == expectedVertical);

        // Each mode keeps its own size through a state round trip.
        juce::XmlElement root ("LUFSMeterSettings");
        sizes.toXml (root);

        const auto restored = EditorWindowSizes::fromXml (root);
        TEST_CHECK (restored == sizes);
        TEST_CHECK (restored.sizeFor (MeterDisplayMode::vertical) == expectedVertical);
        TEST_CHECK (restored.sizeFor (MeterDisplayMode::horizontal).width == 400);
        TEST_CHECK (restored.sizeFor (MeterDisplayMode::compact).height == 180);

        // Saving and loading repeatedly never drifts.
        auto round = restored;

        for (int i = 0; i < 5; ++i)
        {
            juce::XmlElement xml ("LUFSMeterSettings");
            round.toXml (xml);
            round = EditorWindowSizes::fromXml (xml);
        }

        TEST_CHECK (round == sizes);

        // A mode that was never resized stays unset across the round trip.
        EditorWindowSizes partial;
        partial.setSize (MeterDisplayMode::vertical, 176, 360);

        juce::XmlElement partialXml ("LUFSMeterSettings");
        partial.toXml (partialXml);

        const auto partialRestored = EditorWindowSizes::fromXml (partialXml);
        TEST_CHECK (partialRestored.sizeFor (MeterDisplayMode::vertical).width == 176);
        TEST_CHECK (partialRestored.sizeFor (MeterDisplayMode::horizontal).isUnset());
        TEST_CHECK (partialRestored.sizeFor (MeterDisplayMode::compact).isUnset());

        // An untouched editor adds no node to the state chunk at all.
        juce::XmlElement untouched ("LUFSMeterSettings");
        EditorWindowSizes().toXml (untouched);
        TEST_CHECK (untouched.getChildByName (EditorWindowSizes::elementName) == nullptr);

        // A size outside the mode's range is clamped to it, so a state restored
        // from another build can never produce an unusable window.
        const auto horizontalLimits = EditorLayout::forMode (MeterDisplayMode::horizontal);

        EditorWindowSizes clamped;
        clamped.setSize (MeterDisplayMode::horizontal, 10, 10);
        TEST_CHECK (clamped.sizeFor (MeterDisplayMode::horizontal).width == horizontalLimits.minWidth);
        TEST_CHECK (clamped.sizeFor (MeterDisplayMode::horizontal).height == horizontalLimits.minHeight);

        clamped.setSize (MeterDisplayMode::vertical, 90000, 90000);
        TEST_CHECK (clamped.sizeFor (MeterDisplayMode::vertical).width == EditorLayout::maxWindowSize);
        TEST_CHECK (clamped.sizeFor (MeterDisplayMode::vertical).height == EditorLayout::maxWindowSize);

        // A degenerate size (a host reporting 0 before the view is laid out)
        // never erases a valid memory.
        EditorWindowSizes kept;
        kept.setSize (MeterDisplayMode::compact, 200, 176);
        kept.setSize (MeterDisplayMode::compact, 0, 0);
        kept.setSize (MeterDisplayMode::compact, -5, -5);
        TEST_CHECK (kept.sizeFor (MeterDisplayMode::compact).width == 200);
        TEST_CHECK (kept.sizeFor (MeterDisplayMode::compact).height == 176);

        // Reset forgets the mode, so it opens at its preferred size again.
        kept.reset (MeterDisplayMode::compact);
        TEST_CHECK (kept.sizeFor (MeterDisplayMode::compact).isUnset());

        // A damaged chunk (missing or nonsense values) degrades to "unset"
        // instead of a broken window.
        juce::XmlElement damaged ("LUFSMeterSettings");
        auto* node = damaged.createNewChildElement (EditorWindowSizes::elementName);
        node->createNewChildElement ("vertical");
        auto* compact = node->createNewChildElement ("compact");
        compact->setAttribute ("width", -40);
        compact->setAttribute ("height", -40);

        const auto damagedRestored = EditorWindowSizes::fromXml (damaged);
        TEST_CHECK (damagedRestored.sizeFor (MeterDisplayMode::vertical).isUnset());
        TEST_CHECK (damagedRestored.sizeFor (MeterDisplayMode::compact).isUnset());

        // An out-of-range mode index (a corrupt chunk) cannot read past the
        // array or report a size for a mode that does not exist.
        const EditorWindowSizes empty;
        TEST_CHECK (empty.sizeFor (static_cast<MeterDisplayMode> (-4)).isUnset());
        TEST_CHECK (empty.sizeFor (static_cast<MeterDisplayMode> (9)).isUnset());
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

        const auto reading = engine.getReading();
        TEST_CHECK (reading.hasIntegrated);
        TEST_CHECK_NEAR (reading.integrated, -23.0, 0.1);

        // The 5 s tone also fills the Momentary and Short-Term windows; the
        // true peak of a -23 dBFS sine is about -23 dBTP.
        TEST_CHECK (reading.hasMomentary);
        TEST_CHECK_NEAR (reading.momentary, -23.0, 0.1);
        TEST_CHECK (reading.hasShortTerm);
        TEST_CHECK_NEAR (reading.shortTerm, -23.0, 0.1);
        TEST_CHECK (reading.hasTruePeak);
        TEST_CHECK_NEAR (reading.truePeak, -23.0, 0.5);

        // Manual reset clears every measurement coherently.
        engine.requestReset (true);
        TEST_CHECK (waitFor ([&] { return ! engine.getReading().hasIntegrated; }, 2000));

        const auto cleared = engine.getReading();
        TEST_CHECK (! cleared.hasIntegrated);
        TEST_CHECK (! cleared.hasMomentary);
        TEST_CHECK (! cleared.hasShortTerm);
        TEST_CHECK (! cleared.hasTruePeak);

        engine.release();
    }

    void testSnapshotConcurrentRead()
    {
        std::printf ("testSnapshotConcurrentRead\n");

        // Stress the coherent publication: a reader thread reads the snapshot
        // while the audio side pushes audio, and must never observe a torn
        // (non-finite) value presented as valid.
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);

        std::atomic<bool> stop { false };
        std::atomic<bool> torn { false };

        std::thread reader ([&]
        {
            while (! stop.load())
            {
                const auto r = engine.getReading();

                if ((r.hasIntegrated && ! std::isfinite (r.integrated))
                    || (r.hasMomentary && ! std::isfinite (r.momentary))
                    || (r.hasShortTerm && ! std::isfinite (r.shortTerm))
                    || (r.hasTruePeak  && ! std::isfinite (r.truePeak)))
                    torn.store (true);
            }
        });

        const auto sine = makeSine (2, 48000.0, 3.0, 1000.0, -23.0);
        const int channels = 2;
        const int chunk = 512;
        juce::AudioBuffer<float> block (channels, chunk);

        for (int pos = 0; pos < 48000 * 3; pos += chunk)
        {
            const int n = std::min (chunk, 48000 * 3 - pos);

            for (int c = 0; c < channels; ++c)
                for (int i = 0; i < n; ++i)
                    block.setSample (c, i, sine[(size_t) (pos + i) * (size_t) channels + (size_t) c]);

            engine.pushBlock (block, n);
            juce::Thread::sleep (1);
        }

        stop.store (true);
        reader.join();

        TEST_CHECK (! torn.load());

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
    testMomentaryReference();
    testMomentaryUnavailableBeforeWindow();
    testMomentarySilentWindow();
    testShortTermReference();
    testShortTermUnavailableBeforeWindow();
    testShortTermRollingTransition();
    testShortTermSilenceAfterActive();
    testShortTermLowLevelUngated();
    testMomentaryShortTermSampleRates();
    testMomentaryShortTermBlockSize();
    testMomentaryShortTermMono();
    testTruePeakInterSample();
    testTruePeakSampleRates();
    testTruePeakAcrossChannels();
    testTruePeakSilenceAndReset();
    testZoneClassification();
    testZoneSanitisation();
    testScaleProportion();
    testValueAnimation();
    testZoneBoundaryConsistency();
    testSettingsPersistence();
    testLegacyDisplayModeMigration();
    testEditorLayoutTable();
    testEditorWindowSizes();
    testLifecycleContinuousPlayback();
    testLifecycleSilenceDoesNotReset();
    testLifecycleSeekResets();
    testLifecycleLoopWrapResets();
    testLifecycleStopAndRestart();
    testLifecycleGapResets();
    testLifecycleWithoutPositionStillPlays();
    testEnginePipeline();
    testSnapshotConcurrentRead();

    std::printf ("\n%d checks, %d failures\n", test::checkCount(), test::failureCount());
    return test::failureCount() == 0 ? 0 : 1;
}
