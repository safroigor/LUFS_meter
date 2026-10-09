#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeterSettings.h"

/**
    Popup content for the meter settings.

    Shown in a juce::CallOutBox so it never disturbs the editor layout and works
    identically in both orientations. The label/scale options are custom-drawn
    checkboxes so their size is guaranteed to be identical. It only reports
    changes through the supplied callbacks.
*/
class SettingsContent final : public juce::Component
{
public:
    SettingsContent (MeterSettings initial,
                     std::function<void (MeterSettings)> onSettingsChanged,
                     std::function<void()> onReset);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void emitSettings();
    void refreshControls();
    void drawCheckboxRow (juce::Graphics&, juce::Rectangle<int> row,
                          const juce::String& label, bool on) const;

    MeterSettings settings_;
    std::function<void (MeterSettings)> onSettingsChanged_;
    std::function<void()> onReset_;

    juce::TextButton horizontalButton_ { "Horizontal" };
    juce::TextButton verticalButton_   { "Vertical" };
    juce::ComboBox   sizeCombo_;
    juce::TextButton resetButton_ { "Reset" };

    juce::Rectangle<int> showLabelRect_;
    juce::Rectangle<int> showScaleRect_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsContent)
};
