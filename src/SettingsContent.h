#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "MeterSettings.h"

/**
    Popup content for the meter settings.

    Shown in a juce::CallOutBox so it never disturbs the editor layout and works
    identically in both orientations. Controls use the shared LookAndFeel, so the
    checkbox, slider, dropdown and button styling is consistent with the editor.
    It only reports changes through the supplied callbacks.
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
    void openColourSelector (int zone);
    void setZoneColour (int zone, juce::Colour colour);
    juce::Colour currentZoneColour (int zone) const;
    juce::TextButton& colourButton (int zone);
    void updateSwatch (juce::TextButton&, juce::uint32 argb);

    MeterSettings settings_;
    std::function<void (MeterSettings)> onSettingsChanged_;
    std::function<void()> onReset_;

    juce::TextButton horizontalButton_ { "Horizontal" };
    juce::TextButton verticalButton_   { "Vertical" };
    juce::ComboBox   sizeCombo_;
    juce::TextButton resetButton_ { "Reset" };

    juce::ToggleButton showLabelButton_ { "Show label (LUFS-I)" };
    juce::ToggleButton showScaleButton_ { "Show scale" };

    juce::Slider greenLowSlider_;
    juce::Slider greenHighSlider_;
    juce::Slider redThresholdSlider_;

    juce::TextButton defaultColourButton_;
    juce::TextButton greenColourButton_;
    juce::TextButton redColourButton_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsContent)
};
