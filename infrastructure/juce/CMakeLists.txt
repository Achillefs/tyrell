juce_add_plugin(VP330
    PRODUCT_NAME              "VP330"
    COMPANY_NAME              "VP330 Project"
    BUNDLE_ID                 "org.vp330.VP330"
    PLUGIN_MANUFACTURER_CODE  Vp33
    PLUGIN_CODE               Vprc
    FORMATS                   VST3 AU CLAP Standalone
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
)

target_sources(VP330 PRIVATE
    PluginProcessor.cpp
)

target_compile_definitions(VP330 PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VP330 PRIVATE
    vp330_domain
    juce::juce_audio_utils
    juce::juce_audio_processors
)
