#include "PluginEditor.h"

#include "AboutContent.h"
#include "MeterTheme.h"
#include "PluginProcessor.h"
#include "SettingsContent.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX 1
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN 1
 #endif
 #include <windows.h>
#endif

namespace
{
    /** The real mouse button state, straight from the OS.

        JUCE only learns about clicks that arrive in its own windows, so a click
        in the host's UI — the same process, but not a JUCE window — is invisible
        to it, and that is exactly the click that should close a popup. */
    bool isMouseButtonDownAnywhere()
    {
       #if JUCE_WINDOWS
        return (::GetAsyncKeyState (VK_LBUTTON) & 0x8000) != 0
            || (::GetAsyncKeyState (VK_MBUTTON) & 0x8000) != 0
            || (::GetAsyncKeyState (VK_RBUTTON) & 0x8000) != 0;
       #else
        return juce::ComponentPeer::getCurrentModifiersRealtime().isAnyMouseButtonDown();
       #endif
    }
}

LufsMeterAudioProcessorEditor::LufsMeterAudioProcessorEditor (LufsMeterAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      meter_ (p.getMeasurementEngine())
{
    setLookAndFeel (&MeterTheme::lookAndFeel());

    setResizable (true, true);
    updateSizeLimits();

    addAndMakeVisible (meter_);

    resetButton_.onClick = [this]
    {
        processorRef.resetMeasurement();
        meter_.resetDisplay();
    };
    addAndMakeVisible (resetButton_);

    settingsButton_.onClick = [this] { showSettings(); };
    addAndMakeVisible (settingsButton_);

    infoButton_.onClick = [this] { showAbout(); };
    addAndMakeVisible (infoButton_);

    meter_.setSettings (processorRef.getSettings());
    updateControlInsets (processorRef.getSettings().displayMode);

    const auto restored = getRestoredSize (processorRef.getSettings().displayMode);
    applyWindowSize (restored);

    processorRef.addChangeListener (this);
    juce::Desktop::getInstance().addGlobalMouseListener (this);

    startTimerHz (30);
}

LufsMeterAudioProcessorEditor::~LufsMeterAudioProcessorEditor()
{
    stopTimer();
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
    processorRef.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void LufsMeterAudioProcessorEditor::updateSizeLimits()
{
    const auto size = EditorLayout::forMode (processorRef.getSettings().displayMode);

    if (auto* limits = getConstrainer())
        limits->setSizeLimits (size.minWidth, size.minHeight,
                               EditorLayout::maxWindowSize, EditorLayout::maxWindowSize);
}

juce::Point<int> LufsMeterAudioProcessorEditor::getRestoredSize (MeterDisplayMode mode) const
{
    const auto remembered = processorRef.getEditorWindowSize (mode);

    if (! remembered.isUnset())
        return { remembered.width, remembered.height };

    const auto size = EditorLayout::forMode (mode);
    return { size.preferredWidth, size.preferredHeight };
}

void LufsMeterAudioProcessorEditor::applyWindowSize (juce::Point<int> size)
{
    const juce::ScopedValueSetter<bool> programmaticSize (applyingProgrammaticSize_, true);

    setSize (size.x, size.y);
}

void LufsMeterAudioProcessorEditor::updateControlInsets (MeterDisplayMode mode)
{
    // The buttons are overlaid on the meter's own area, so the meter has to lay
    // out the rows it draws beside them — the Horizontal TP / LS / LM row and
    // the Compact reading capsule — inside what the buttons leave free. The
    // Vertical readouts sit beside the bar, clear of the corner buttons.
    const int inset = EditorLayout::rowInsetFor (mode);
    meter_.setControlInsets (inset, inset);
}

void LufsMeterAudioProcessorEditor::timerCallback()
{
    // Real elapsed time: the fill animation keeps its duration even when the
    // host delays a frame, and the meter follows the measurement between frames.
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double deltaSeconds = lastFrameTimeMs_ > 0.0
                                    ? (nowMs - lastFrameTimeMs_) * 0.001
                                    : 0.0;

    lastFrameTimeMs_ = nowMs;

    meter_.advanceAnimation (deltaSeconds);
    watchOpenPopup();
    meter_.repaint();
}

void LufsMeterAudioProcessorEditor::mouseDown (const juce::MouseEvent&)
{
    lastJuceMouseDownMs_ = juce::Time::getMillisecondCounterHiRes();
}

void LufsMeterAudioProcessorEditor::watchOpenPopup()
{
    if (openPopup_ == nullptr)
    {
        lastRealMouseDown_ = false;
        return;
    }

    const bool realDown = isMouseButtonDownAnywhere();

    if (realDown && ! lastRealMouseDown_)
    {
        // A fresh press. If JUCE did not deliver it within the last couple of
        // frames, it landed outside every JUCE window: the host's UI, or another
        // application. That is a click outside the popup, so close it. Clicks
        // JUCE did see are handled by the popup's own modality.
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const bool juceSawIt = (nowMs - lastJuceMouseDownMs_) < 150.0;

        if (! juceSawIt)
            openPopup_->dismiss();
    }

    lastRealMouseDown_ = realDown;
}

void LufsMeterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (MeterTheme::windowBackground);
}

void LufsMeterAudioProcessorEditor::resized()
{
    const auto mode = processorRef.getSettings().displayMode;
    const auto inner = getLocalBounds().reduced (EditorLayout::outerPadding);

    // The meter fills everything except the strip row the buttons take — except
    // in Horizontal, where the buttons are overlaid on the meter's own empty top
    // corners and the meter therefore keeps the whole area.
    meter_.setBounds (EditorLayout::overlaysControls (mode)
                          ? inner
                          : inner.withTrimmedTop (EditorLayout::controlStripHeight));

    // One control arrangement in every mode: About in the top-left corner,
    // Settings in the top-right one, and Reset out of their way — bottom-left in
    // Vertical and Horizontal, left of the reading in Compact. The positions
    // come from EditorLayout, so the preview tool draws the same chrome.
    const auto placement = EditorLayout::controlsIn (inner.getWidth(), inner.getHeight(), mode);

    const auto place = [&] (juce::Component& button, int x, int y)
    {
        button.setBounds (inner.getX() + x, inner.getY() + y,
                          EditorLayout::buttonSize, EditorLayout::buttonSize);
    };

    place (infoButton_, placement.aboutX, placement.aboutY);
    place (resetButton_, placement.resetX, placement.resetY);
    place (settingsButton_, placement.settingsX, placement.settingsY);

    // A size the host gave the window (the user dragged its edge) is the size
    // the mode should reopen with; a size the editor applied itself is not.
    if (! applyingProgrammaticSize_)
        processorRef.setEditorWindowSize (mode, getWidth(), getHeight());
}

void LufsMeterAudioProcessorEditor::applyPopupSettings (const MeterSettings& s)
{
    const auto previous = processorRef.getSettings();

    processorRef.setSettings (s);
    meter_.setSettings (processorRef.getSettings());

    // Only resize when the geometry changed; zone/colour edits must not move
    // the window while the user drags a slider.
    if (s.displayMode != previous.displayMode)
    {
        updateSizeLimits();
        updateControlInsets (s.displayMode);
        applyWindowSize (getRestoredSize (s.displayMode));
    }

    repaint();
}

void LufsMeterAudioProcessorEditor::popupResetWindowSize()
{
    const auto mode = processorRef.getSettings().displayMode;

    processorRef.resetEditorWindowSize (mode);
    applyWindowSize (getRestoredSize (mode));
    repaint();
}

void LufsMeterAudioProcessorEditor::popupReset()
{
    processorRef.resetMeasurement();

    // Clearing the accumulated value also clears the display: the meter must not
    // hold the reading it was showing (see MeterComponent::resetDisplay).
    meter_.resetDisplay();
    meter_.repaint();
}

void LufsMeterAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshFromProcessor();
}

void LufsMeterAudioProcessorEditor::refreshFromProcessor()
{
    const auto settings = processorRef.getSettings();

    meter_.setSettings (settings);
    updateSizeLimits();
    updateControlInsets (settings.displayMode);
    applyWindowSize (getRestoredSize (settings.displayMode));
    repaint();
}

void LufsMeterAudioProcessorEditor::showSettings()
{
    auto safe = juce::Component::SafePointer<LufsMeterAudioProcessorEditor> (this);

    auto content = std::make_unique<SettingsContent> (
        processorRef.getSettings(),
        [safe] (MeterSettings s) mutable
        {
            if (auto* editor = safe.getComponent())
                editor->applyPopupSettings (s);
        },
        [safe]
        {
            if (auto* editor = safe.getComponent())
                editor->popupReset();
        },
        [safe]
        {
            if (auto* editor = safe.getComponent())
                editor->popupResetWindowSize();
        });

    content->setLookAndFeel (&MeterTheme::lookAndFeel());

    addPopup (std::move (content), settingsButton_.getScreenBounds());
}

void LufsMeterAudioProcessorEditor::showAbout()
{
    auto content = std::make_unique<AboutContent>();
    content->setLookAndFeel (&MeterTheme::lookAndFeel());

    addPopup (std::move (content), infoButton_.getScreenBounds());
}

juce::CallOutBox& LufsMeterAudioProcessorEditor::addPopup (std::unique_ptr<juce::Component> content,
                                                           juce::Rectangle<int> anchorArea)
{
    // A desktop window rather than a child of the editor: the settings panel is
    // taller than the smallest windows, and must not be clipped by them.
    auto& box = juce::CallOutBox::launchAsynchronously (std::move (content), anchorArea, nullptr);

    // A click outside the box closes it — and is swallowed, so it cannot also
    // hit the meter underneath or re-open the popup through its own button.
    box.setDismissalMouseClicksAreAlwaysConsumed (true);

    openPopup_ = &box;
    lastRealMouseDown_ = true; // the click that opened it is still held

    return box;
}
