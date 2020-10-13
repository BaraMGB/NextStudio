project(NextStudio VERSION 0.1.0)

set (TargetName ${PROJECT_NAME})

juce_add_gui_app(${TargetName} PRODUCT_NAME "NextStudio")
juce_generate_juce_header(${TargetName})

target_sources(${TargetName} PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MenuBar.cpp
        Source/SpinBox.cpp
        Source/SpinBoxGroup.cpp
        Source/SideBarBrowser.cpp
        Source/HeaderComponent.cpp
        Source/TimeLineComponent.cpp
        Source/EditComponent.cpp
        )

target_compile_definitions(${TargetName} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>")

target_compile_definitions (${TargetName} PUBLIC
        JUCE_JACK=1
        JUCE_WASAPI=1
#        JUCE_USE_WINRT_MIDI=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_LADSPA=1
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_WINDOWS_MEDIA_FORMAT=1)



target_link_libraries(${TargetName} PRIVATE
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_gui_extra
        tracktion_engine
	tracktion_graph)

