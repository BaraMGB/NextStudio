project(NextStudio VERSION 0.1.0)

set (TargetName ${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# If we are using MSVC we want static runtime linkage
if (MSVC)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# If we are compiling for macOS we want to target OS versions down to 10.11
if (APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "")
    set (CMAKE_OSX_ARCHITECTURES "arm64")
endif()

# Compile with werror in release builds
#if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
#    CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
#elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
#    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /WX")
#endif()


# Adds all the module sources so they appear correctly in the IDE
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable Module Source Groups" ON)


add_subdirectory(../modules/tracktion_engine/modules/juce ./cmake_build_juce)                                   
add_subdirectory(../modules/tracktion_engine/modules ./cmake_build_tracktion)


add_subdirectory(resources)

juce_add_gui_app(${TargetName} PRODUCT_NAME "NextStudio")
juce_generate_juce_header(${TargetName})

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
 
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
     C_VISIBILITY_PRESET hidden
     CXX_VISIBILITY_PRESET hidden)

target_sources(${TargetName} PRIVATE
        Source/AutomatableSliderComponent.cpp
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MenuBar.cpp
        Source/SideBarBrowser.cpp
        Source/HeaderComponent.cpp
        Source/TimeLineComponent.cpp
        Source/EditComponent.cpp
        Source/PluginWindow.cpp
        Source/PluginComponent.cpp
        Source/PluginMenu.cpp
        Source/LevelMeterComponent.cpp
        Source/PluginRackComponent.cpp
        Source/PianoRollEditorComponent.cpp
        Source/PlayHeadComponent.cpp
        Source/PositionDisplayComponent.cpp
        Source/LowerRangeComponent.cpp
        Source/TrackHeadComponent.cpp
        Source/AudioMidiSettings.cpp
        Source/PianoRollContentComponent.cpp
        Source/TimelineOverlayComponent.cpp
        Source/RecordingClipComponent.cpp
        Source/VelocityEditor.cpp
        Source/KeyboardView.cpp
        Source/LassoSelectionTool.cpp
        Source/TrackListView.cpp
        Source/SongEditorView.cpp
        Source/Utilities.cpp
        )

target_compile_definitions(${TargetName} PRIVATE
        TRACKTION_JUCE7=1
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
        JUCE_PLUGINHOST_AU=1
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_WINDOWS_MEDIA_FORMAT=1
        DONT_SET_USING_JUCE_NAMESPACE=1
     	JUCE_MODAL_LOOPS_PERMITTED=1
        )



target_link_libraries(${TargetName} PRIVATE
        Pictures
        #     juce::juce_recommended_config_flags
        #     juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_utils

        tracktion::tracktion_core
        tracktion::tracktion_engine
        tracktion::tracktion_graph
	)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "-latomic")
  target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "-m64")
endif()
