#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "EditorLayout.h"
#include "IconButton.h"
#include "MeterComponent.h"
#include "MeterSettings.h"

class LufsMeterAudioProcessor;

/**
    Compact, resizable dark editor for the LUFS Meter.

    Layout: a thin control strip holds the About button on the left and the
    Reset and Settings buttons on the right, in every display mode. Horizontal
    mode overlays those buttons on the window's empty top corners instead of
    taking a strip row of its own, which is what lets that window stay short;
    the meter is told how much room the buttons reserve there, so the TP / LS /
    LM row can never run under them.

    Settings open in a CallOutBox, so they work identically in every display
    mode and never disturb the layout. The three display modes — Vertical,
    Horizontal and Compact — are mutually exclusive; changing the mode never
    resets the measurement, and each mode reopens at the window size it was last
    dragged to (docs/decisions/0009-editor-window-size-persistence.md).
*/
class LufsMeterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer,
                                            private juce::ChangeListener
{
public:
    explicit LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor&);
    ~LufsMeterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Called by the settings popup. */
    void applyPopupSettings (const MeterSettings&);
    void popupReset();

    /** Called by the settings popup: forget this mode's window size and return
        to the size the mode is designed for. */
    void popupResetWindowSize();

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    /** Records that JUCE delivered a mouse-down, so the outside-click watcher
        can tell a click in the host's own UI (which JUCE never sees) from one
        inside the plug-in window. */
    void mouseDown (const juce::MouseEvent&) override;

    void showSettings();
    void showAbout();

    /** Dismisses the open popup when a mouse-down happens outside every JUCE
        window (the host's UI, or another application). Clicks inside the plug-in
        window are handled by the popup's own modality. */
    void watchOpenPopup();

    /** Shows `content` in a CallOutBox anchored to `anchorArea` (screen
        coordinates) and remembers it as the open popup. */
    juce::CallOutBox& addPopup (std::unique_ptr<juce::Component> content,
                                juce::Rectangle<int> anchorArea);

    /** Applies the current display mode's limits to the resize constrainer. */
    void updateSizeLimits();

    /** The size to open `mode` with: the size the user last dragged it to, or
        the size the mode is designed for when it has never been resized. */
    juce::Point<int> getRestoredSize (MeterDisplayMode) const;

    /** Sizes the window without treating it as a user resize, so opening the
        editor, switching a mode or restoring a state never overwrites a
        remembered size. */
    void applyWindowSize (juce::Point<int>);

    /** Tells the meter how much horizontal room the overlaid buttons take from
        its own rows (the Horizontal TP / LS / LM row and the Compact reading
        capsule). */
    void updateControlInsets (MeterDisplayMode);

    /** Follows a state the host restored (project load or preset): display
        settings, mode and window size. */
    void refreshFromProcessor();

    LufsMeterAudioProcessor& processorRef;
    MeterComponent meter_;
    IconButton resetButton_    { IconButton::IconType::reset,    "Reset measurement" };
    IconButton settingsButton_ { IconButton::IconType::settings, "Settings" };
    IconButton infoButton_     { IconButton::IconType::info,     "About LUFS Meter" };
    juce::TooltipWindow tooltipWindow_ { this, 700 };

    /** True while the editor sizes the window itself, so resized() does not
        record a programmatic size as if the user had dragged it. */
    bool applyingProgrammaticSize_ = false;

    /** Timestamp of the previous frame, so the meter's fill animation advances
        by real elapsed time rather than by a nominal frame duration. */
    double lastFrameTimeMs_ = 0.0;

    /** The popup currently on screen: clicking its button again (or anywhere
        outside it) closes it. */
    juce::Component::SafePointer<juce::CallOutBox> openPopup_;
    double lastJuceMouseDownMs_ = 0.0;
    bool lastRealMouseDown_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LufsMeterAudioProcessorEditor)
};
