juce_add_console_app(LufsMeterTests PRODUCT_NAME "LUFS Meter Tests")

target_sources(LufsMeterTests PRIVATE
    TestMain.cpp
    ${CMAKE_SOURCE_DIR}/src/LoudnessMeter.cpp
    ${CMAKE_SOURCE_DIR}/src/MeasurementEngine.cpp)

target_include_directories(LufsMeterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(LufsMeterTests PRIVATE
    ebur128
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

add_test(NAME lufs_meter_tests COMMAND LufsMeterTests)
