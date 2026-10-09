/*
    Offscreen UI preview tool (development only).

    Renders MeterComponent into PNG images so the layout can be inspected
    without a host. It is not part of the plugin and is only built when
    -DLUFSMETER_BUILD_PREVIEW=ON.
*/

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeasurementEngine.h"
#include "MeterComponent.h"
#include "MeterSettings.h"
#include "MeterTheme.h"

namespace
{
    constexpr double pi = 3.14159265358979323846;

    // Mirrors the editor's chrome so the preview renders MeterComponent at the
    // bounds it actually receives (editor size minus padding and control strip).
    constexpr int editorOuterPadding = 8;
    constexpr int editorControlStrip = 22;

    void writePng (const juce::Image& image, const juce::File& file)
    {
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

        if (stream == nullptr)
            return;

        juce::PNGImageFormat png;
        png.writeImageToStream (image, *stream);
    }

    void render (const juce::File& dir, const juce::String& name,
                 const MeasurementEngine& engine, MeterSettings settings,
                 int width, int height)
    {
        MeterComponent component (engine);
        component.setSettings (settings);
        component.setSize (width, height);

        juce::Image image (juce::Image::ARGB, width, height, true);

        {
            juce::Graphics g (image);
            g.fillAll (MeterTheme::windowBackground);
            component.paintEntireComponent (g, true);
        }

        writePng (image, dir.getChildFile (name));
        std::printf ("wrote %s\n", name.toRawUTF8());
    }

    /** Renders MeterComponent at the bounds the editor gives it. */
    void renderEditor (const juce::File& dir, const juce::String& name,
                       const MeasurementEngine& engine, MeterSettings settings,
                       int editorWidth, int editorHeight)
    {
        render (dir, name, engine, settings,
                editorWidth - 2 * editorOuterPadding,
                editorHeight - 2 * editorOuterPadding - editorControlStrip);
    }

    void feed (MeasurementEngine& engine, double sampleRate, double seconds,
               double frequency, double peakDbfs)
    {
        const double amplitude = std::pow (10.0, peakDbfs / 20.0);
        const int total = (int) (sampleRate * seconds);
        const int chunk = 512;
        juce::AudioBuffer<float> block (2, chunk);
        int frame = 0;

        while (frame < total)
        {
            const int n = juce::jmin (chunk, total - frame);

            for (int i = 0; i < n; ++i)
            {
                const auto s = (float) (amplitude
                                        * std::sin (2.0 * pi * frequency
                                                    * (double) (frame + i) / sampleRate));
                block.setSample (0, i, s);
                block.setSample (1, i, s);
            }

            engine.pushBlock (block, n);
            frame += n;
            juce::Thread::sleep (1);
        }
    }

    void waitForShortTerm (const MeasurementEngine& engine)
    {
        for (int i = 0; i < 400 && ! engine.getReading().hasShortTerm; ++i)
            juce::Thread::sleep (10);
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    const juce::File outDir = argc > 1
                                  ? juce::File (juce::String (argv[1]))
                                  : juce::File::getCurrentWorkingDirectory().getChildFile ("docs/images");
    outDir.createDirectory();

    MeasurementEngine engine;
    engine.prepare (48000.0, 2);

    MeterSettings h;
    h.orientation = MeterOrientation::horizontal;

    MeterSettings v;
    v.orientation = MeterOrientation::vertical;

    // Unavailable state (no signal yet): placeholders.
    {
        auto s = h; s.size = MeterSizeMode::compact;
        renderEditor (outDir, "ui-v1.2-horizontal-empty.png", engine, s, 430, 144);
        auto sv = v; sv.size = MeterSizeMode::compact;
        renderEditor (outDir, "ui-v1.2-vertical-empty.png", engine, sv, 224, 360);
    }

    // Populated state: a steady -23 dBFS stereo tone.
    feed (engine, 48000.0, 4.0, 1000.0, -23.0);
    waitForShortTerm (engine);

    {
        auto sc = h; sc.size = MeterSizeMode::compact;
        renderEditor (outDir, "ui-v1.2-horizontal-compact.png", engine, sc, 430, 144);

        auto ss = h; ss.size = MeterSizeMode::slim;
        renderEditor (outDir, "ui-v1.2-horizontal-slim.png", engine, ss, 400, 122);

        renderEditor (outDir, "ui-v1.2-horizontal-min.png", engine, sc, 360, 136);

        auto vc = v; vc.size = MeterSizeMode::compact;
        renderEditor (outDir, "ui-v1.2-vertical-compact.png", engine, vc, 224, 360);

        auto vs = v; vs.size = MeterSizeMode::slim;
        renderEditor (outDir, "ui-v1.2-vertical-slim.png", engine, vs, 200, 312);

        renderEditor (outDir, "ui-v1.2-vertical-min.png", engine, vc, 190, 300);
    }

    // True Peak at/above 0 dBTP to show the red state.
    feed (engine, 48000.0, 0.5, 12000.0, 0.0);
    juce::Thread::sleep (200);

    {
        auto sc = h; sc.size = MeterSizeMode::compact;
        renderEditor (outDir, "ui-v1.2-horizontal-tp-red.png", engine, sc, 430, 144);

        auto vc = v; vc.size = MeterSizeMode::compact;
        renderEditor (outDir, "ui-v1.2-vertical-tp-red.png", engine, vc, 224, 360);
    }

    engine.release();
    return 0;
}
