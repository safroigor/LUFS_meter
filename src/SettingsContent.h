#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeterSettings.h"

/**
    Popup content for the meter settings.

    Shown in a juce::CallOutBox so it never disturbs the editor layout and works
    identically in both orientations. It only reports changes through the
    supplied callbacks.
*/
class SettingsContent final : public juce::Component
{
public:
    SettingsContent (MeterSettings initial,
                     std::function<void (MeterSettings)> onSettingsChanged,
                     std::function<void()> onReset);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void emitSettings();
    void refreshControls();

    MeterSettings settings_;
    std::function<void (MeterSettings)> onSettingsChanged_;
    std::function<void()> onReset_;

    juce::TextButton horizontalButton_ { "Horizontal" };
    juce::TextButton verticalButton_   { "Vertical" };
    juce::ComboBox   sizeCombo_;
    juce::ToggleButton showLabelToggle_ { "Show label" };
    juce::ToggleButton showScaleToggle_ { "Show scale" };
    juce::TextButton resetButton_ { "Reset" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsContent)
};
