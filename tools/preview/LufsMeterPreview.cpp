/*
    Offscreen UI preview tool (development only).

    Renders MeterComponent into PNG images so the layout can be inspected
    without a host. It is not part of the plugin and is only built when
    -DLUFSMETER_BUILD_PREVIEW=ON.
*/

#include <juce_gui_basics/juce_gui_basics.h>

#include "AboutContent.h"
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

    /** Renders an arbitrary component (e.g. the About panel) on the window
        background, at its own size. */
    void renderComponent (const juce::File& dir, const juce::String& name, juce::Component& component)
    {
        juce::Image image (juce::Image::ARGB, component.getWidth(), component.getHeight(), true);

        {
            juce::Graphics g (image);
            g.fillAll (MeterTheme::windowBackground);
            component.paintEntireComponent (g, true);
        }

        writePng (image, dir.getChildFile (name));
        std::printf ("wrote %s\n", name.toRawUTF8());
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

    // Unavailable state (no signal yet): placeholders in all three modes.
    {
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);

        MeterSettings s;
        s.displayMode = MeterDisplayMode::vertical;
        renderEditor (outDir, "ui-vertical-empty.png", engine, s, 224, 360);

        s.displayMode = MeterDisplayMode::horizontal;
        renderEditor (outDir, "ui-horizontal-empty.png", engine, s, 430, 144);

        s.displayMode = MeterDisplayMode::compact;
        renderEditor (outDir, "ui-compact-empty.png", engine, s, 220, 190);
    }

    // Populated state: a steady -23 dBFS stereo tone.
    {
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);
        feed (engine, 48000.0, 4.0, 1000.0, -23.0);
        waitForShortTerm (engine);

        MeterSettings s;
        s.displayMode = MeterDisplayMode::vertical;
        renderEditor (outDir, "ui-mode-vertical.png", engine, s, 224, 360);

        s.displayMode = MeterDisplayMode::horizontal;
        renderEditor (outDir, "ui-mode-horizontal.png", engine, s, 430, 144);

        // Compact mode at the preferred and minimum sizes.
        s.displayMode = MeterDisplayMode::compact;
        renderEditor (outDir, "ui-compact.png", engine, s, 220, 190);
        renderEditor (outDir, "ui-compact-min.png", engine, s, 190, 170);

        engine.release();
    }

    // Compact background fill / zone colours at several LUFS-I levels. The
    // default zones are green [-24, -14] and red >= -14, so these cover the
    // default, green and red zones with low, mid and high fills.
    struct Level { double dbfs; const char* name; };

    for (const Level level : { Level { -27.0, "ui-compact-normal.png" },
                               Level { -18.0, "ui-compact-green.png" },
                               Level {  -8.0, "ui-compact-red.png" } })
    {
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);
        feed (engine, 48000.0, 4.0, 1000.0, level.dbfs);
        waitForShortTerm (engine);

        MeterSettings s;
        s.displayMode = MeterDisplayMode::compact;
        renderEditor (outDir, level.name, engine, s, 220, 190);

        engine.release();
    }

    // The About popup, rendered on its own.
    {
        AboutContent about;
        renderComponent (outDir, "ui-about.png", about);
    }

    return 0;
}
