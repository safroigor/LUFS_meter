#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeterSettings.h"

/**
    Popup content for the meter settings.

    Shown in a juce::CallOutBox so it never disturbs the editor layout and works
    identically in both orientations. Controls use the shared LookAndFeel, so the
    checkbox, slider, dropdown and button styling is consistent with the editor.
    It only reports changes through the supplied callbacks: settings edits, a
    measurement Reset, and a Reset of the window size (which is remembered per
    display mode; see docs/decisions/0009-editor-window-size-persistence.md).
*/
class SettingsContent final : public juce::Component
{
public:
    SettingsContent (MeterSettings initial,
                     std::function<void (MeterSettings)> onSettingsChanged,
                     std::function<void()> onReset,
                     std::function<void()> onResetWindowSize);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void emitSettings();
    void refreshControls();
    void openColourSelector (int zone);
    void setZoneColour (int zone, juce::Colour colour);
    juce::Colour currentZoneColour (int zone) const;
    juce::TextButton& colourButton (int zone);
    void updateSwatch (juce::TextButton&, juce::uint32 argb);

    MeterSettings settings_;
    std::function<void (MeterSettings)> onSettingsChanged_;
    std::function<void()> onReset_;
    std::function<void()> onResetWindowSize_;

    juce::ComboBox   displayCombo_;
    juce::TextButton resetButton_ { "Reset" };
    juce::TextButton resetWindowSizeButton_ { "Reset window size" };

    juce::ToggleButton showLabelButton_ { "Show label (LUFS-I)" };
    juce::ToggleButton showScaleButton_ { "Show scale" };

    juce::Slider greenLowSlider_;
    juce::Slider greenHighSlider_;
    juce::Slider redThresholdSlider_;
    juce::Slider truePeakRedSlider_;

    juce::TextButton defaultColourButton_;
    juce::TextButton greenColourButton_;
    juce::TextButton redColourButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsContent)
};
