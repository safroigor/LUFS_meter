/*
    Offscreen UI preview tool (development only).

    Renders MeterComponent into PNG images so the layout can be inspected
    without a host. It is not part of the plugin and is only built when
    -DLUFSMETER_BUILD_PREVIEW=ON.
*/

#include <juce_gui_basics/juce_gui_basics.h>

#include "AboutContent.h"
#include "EditorLayout.h"
#include "IconButton.h"
#include "MeasurementEngine.h"
#include "MeterComponent.h"
#include "MeterSettings.h"
#include "MeterTheme.h"
#include "SettingsContent.h"

namespace
{
    constexpr double pi = 3.14159265358979323846;

    void writePng (const juce::Image& image, const juce::File& file)
    {
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());

        if (stream == nullptr)
            return;

        juce::PNGImageFormat png;
        png.writeImageToStream (image, *stream);
    }

    /** Paints a component into `g` at its own position. paintEntireComponent
        draws into the *current* coordinate space, so the origin has to be moved
        to the component's position first — otherwise every component lands in
        the image's top-left corner. */
    void paintAt (juce::Graphics& g, juce::Component& component)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.setOrigin (component.getPosition());
        component.paintEntireComponent (g, true);
    }

    void render (const juce::File& dir, const juce::String& name,
                 const MeasurementEngine& engine, MeterSettings settings,
                 int width, int height,
                 double animationSeconds = -1.0)
    {
        MeterComponent component (engine);
        component.setSettings (settings);
        component.setSize (width, height);

        // The meter fills its first value in over a fraction of a second; a
        // still render wants either the settled value (-1) or one frame of the
        // sweep, which is what `animationSeconds` asks for.
        if (animationSeconds >= 0.0)
            component.advanceAnimation (animationSeconds);
        else
            component.settleAnimation();

        juce::Image image (juce::Image::ARGB, width, height, true);

        {
            juce::Graphics g (image);
            g.fillAll (MeterTheme::windowBackground);
            paintAt (g, component);
        }

        writePng (image, dir.getChildFile (name));
        std::printf ("wrote %s\n", name.toRawUTF8());
    }

    /** Renders the editor's whole composition — the meter at the bounds the
        editor gives it, plus the three icon buttons in their shared positions
        (EditorLayout) — so a preview shows what a host window shows. */
    void renderEditor (const juce::File& dir, const juce::String& name,
                       const MeasurementEngine& engine, MeterSettings settings,
                       int editorWidth, int editorHeight,
                       double animationSeconds = -1.0)
    {
        const bool overlays = EditorLayout::overlaysControls (settings.displayMode);
        const auto inner = juce::Rectangle<int> (EditorLayout::outerPadding,
                                                 EditorLayout::outerPadding,
                                                 editorWidth - 2 * EditorLayout::outerPadding,
                                                 editorHeight - 2 * EditorLayout::outerPadding);

        MeterComponent component (engine);
        component.setSettings (settings);

        // Mirrors the editor: the overlaid buttons take room from the row the
        // meter draws beside them (Horizontal's readouts, Compact's reading).
        const int inset = EditorLayout::rowInsetFor (settings.displayMode);
        component.setControlInsets (inset, inset);

        // Horizontal overlays the buttons on the meter's own top corners and
        // gives it the whole area; the other modes give the strip row up.
        component.setBounds (overlays ? inner : inner.withTrimmedTop (EditorLayout::controlStripHeight));

        // As in `render`: a still shows the settled value unless a frame of the
        // fill sweep was asked for.
        if (animationSeconds >= 0.0)
            component.advanceAnimation (animationSeconds);
        else
            component.settleAnimation();

        juce::Image image (juce::Image::ARGB, editorWidth, editorHeight, true);

        {
            juce::Graphics g (image);
            g.fillAll (MeterTheme::windowBackground);
            paintAt (g, component);

            IconButton info (IconButton::IconType::info, "About");
            IconButton reset (IconButton::IconType::reset, "Reset");
            IconButton settingsButton (IconButton::IconType::settings, "Settings");

            const auto placement = EditorLayout::controlsIn (inner.getWidth(),
                                                             inner.getHeight(),
                                                             settings.displayMode);

            IconButton* const buttons[] = { &info, &reset, &settingsButton };
            const int buttonX[] = { inner.getX() + placement.aboutX,
                                    inner.getX() + placement.resetX,
                                    inner.getX() + placement.settingsX };
            const int buttonY[] = { inner.getY() + placement.aboutY,
                                    inner.getY() + placement.resetY,
                                    inner.getY() + placement.settingsY };

            for (int i = 0; i < 3; ++i)
            {
                buttons[i]->setBounds (buttonX[i], buttonY[i],
                                       EditorLayout::buttonSize, EditorLayout::buttonSize);
                paintAt (g, *buttons[i]);
            }
        }

        writePng (image, dir.getChildFile (name));
        std::printf ("wrote %s\n", name.toRawUTF8());
    }

    /** Renders an arbitrary component (e.g. the About panel) on the window
        background, at its own size. */
    void renderComponent (const juce::File& dir, const juce::String& name, juce::Component& component)
    {
        juce::Image image (juce::Image::ARGB, component.getWidth(), component.getHeight(), true);

        {
            juce::Graphics g (image);
            g.fillAll (MeterTheme::windowBackground);
            paintAt (g, component);
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

namespace
{
    /** One display mode and the renders the documentation uses for it. The
        sizes come from EditorLayout, so a render can never show a stale size. */
    struct ModeCase
    {
        MeterDisplayMode mode;
        const char* emptyName;
        const char* preferredName;
        const char* minName;
        const char* plainName;
    };

    const ModeCase modeCases[] =
    {
        { MeterDisplayMode::vertical,   "ui-vertical-empty.png",   "ui-mode-vertical.png",
          "ui-mode-vertical-min.png",   "ui-mode-vertical-plain.png" },
        { MeterDisplayMode::horizontal, "ui-horizontal-empty.png", "ui-mode-horizontal.png",
          "ui-mode-horizontal-min.png", "ui-mode-horizontal-plain.png" },
        { MeterDisplayMode::compact,    "ui-compact-empty.png",    "ui-compact.png",
          "ui-compact-min.png",         "ui-compact-plain.png" },
    };

    MeterSettings settingsFor (MeterDisplayMode mode)
    {
        MeterSettings s;
        s.displayMode = mode;
        return s;
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

        for (const auto& modeCase : modeCases)
        {
            const auto size = EditorLayout::forMode (modeCase.mode);
            renderEditor (outDir, modeCase.emptyName, engine, settingsFor (modeCase.mode),
                          size.preferredWidth, size.preferredHeight);
        }
    }

    // Populated state: a steady -23 dBFS stereo tone, at the preferred and the
    // minimum size of every mode.
    {
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);
        feed (engine, 48000.0, 4.0, 1000.0, -23.0);
        waitForShortTerm (engine);

        for (const auto& modeCase : modeCases)
        {
            const auto size = EditorLayout::forMode (modeCase.mode);

            renderEditor (outDir, modeCase.preferredName, engine, settingsFor (modeCase.mode),
                          size.preferredWidth, size.preferredHeight);
            renderEditor (outDir, modeCase.minName, engine, settingsFor (modeCase.mode),
                          size.minWidth, size.minHeight);
        }

        // One frame of the fill sweep: a still cannot show motion, but it can
        // show that the value arrives by filling up from the bottom of the scale
        // (0.15 s into the 0.55 s fill, so the bar is clearly part-way).
        constexpr double fillingFrameSeconds = 0.15;

        for (const auto mode : { MeterDisplayMode::vertical, MeterDisplayMode::horizontal })
        {
            const auto size = EditorLayout::forMode (mode);
            const juce::String name = mode == MeterDisplayMode::vertical
                                          ? "ui-mode-vertical-filling.png"
                                          : "ui-mode-horizontal-filling.png";

            renderEditor (outDir, name, engine, settingsFor (mode),
                          size.preferredWidth, size.preferredHeight, fillingFrameSeconds);
        }

        // The tightest composition: no LUFS-I label and no scale marks.
        MeterSettings plain = settingsFor (MeterDisplayMode::vertical);
        plain.showLabel = false;
        plain.showScale = false;

        for (const auto& modeCase : modeCases)
        {
            auto settings = plain;
            settings.displayMode = modeCase.mode;
            const auto size = EditorLayout::forMode (modeCase.mode);

            renderEditor (outDir, modeCase.plainName, engine, settings,
                          size.preferredWidth, size.preferredHeight);
        }

        engine.release();
    }

    // Compact background fill / zone colours at several LUFS-I levels. The
    // default zones are green [-24, -14] and red >= -14, so these cover the
    // default, green and red zones with low, mid and high fills.
    struct Level { double dbfs; const char* name; };

    const auto compactSize = EditorLayout::forMode (MeterDisplayMode::compact);

    for (const Level level : { Level { -27.0, "ui-compact-normal.png" },
                               Level { -18.0, "ui-compact-green.png" },
                               Level {  -8.0, "ui-compact-red.png" } })
    {
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);
        feed (engine, 48000.0, 4.0, 1000.0, level.dbfs);
        waitForShortTerm (engine);

        renderEditor (outDir, level.name, engine, settingsFor (MeterDisplayMode::compact),
                      compactSize.preferredWidth, compactSize.preferredHeight);

        engine.release();
    }

    // True Peak above its warning threshold (default -1.0 dBTP): the TP readout
    // turns red, everything else stays as it is.
    {
        MeasurementEngine engine;
        engine.prepare (48000.0, 2);
        feed (engine, 48000.0, 4.0, 1000.0, -0.5);
        waitForShortTerm (engine);

        const auto size = EditorLayout::forMode (MeterDisplayMode::horizontal);

        renderEditor (outDir, "ui-mode-horizontal-tp-red.png", engine,
                      settingsFor (MeterDisplayMode::horizontal),
                      size.preferredWidth, size.preferredHeight);

        engine.release();
    }

    // The About popup, rendered on its own.
    {
        AboutContent about;
        renderComponent (outDir, "ui-about.png", about);
    }

    // The settings panel, rendered on its own (it opens in a CallOutBox).
    {
        SettingsContent settings (MeterSettings(),
                                  [] (MeterSettings) {},
                                  [] {},
                                  [] {});
        renderComponent (outDir, "ui-settings.png", settings);
    }

    return 0;
}
