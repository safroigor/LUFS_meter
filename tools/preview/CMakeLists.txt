# Offscreen UI preview tool (development only). Built only when
# -DLUFSMETER_BUILD_PREVIEW=ON so normal plugin/test builds are unaffected.
juce_add_console_app(LufsMeterPreview PRODUCT_NAME "LUFS Meter UI Preview")

target_sources(LufsMeterPreview PRIVATE
    LufsMeterPreview.cpp
    ${CMAKE_SOURCE_DIR}/src/LoudnessMeter.cpp
    ${CMAKE_SOURCE_DIR}/src/MeasurementEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/MeterComponent.cpp
    ${CMAKE_SOURCE_DIR}/src/AboutContent.cpp
    ${CMAKE_SOURCE_DIR}/src/IconButton.cpp
    ${CMAKE_SOURCE_DIR}/src/SettingsContent.cpp
    ${CMAKE_SOURCE_DIR}/src/MeterTheme.cpp)

target_include_directories(LufsMeterPreview PRIVATE
    ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(LufsMeterPreview PRIVATE
    ebur128
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
