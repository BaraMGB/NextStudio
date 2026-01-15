set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(BUILD_DIR_NAME "${CMAKE_BINARY_DIR}" NAME)
if(BUILD_DIR_NAME STREQUAL "jumake_build")
    set(JUCE_BINARY_DIR "${CMAKE_BINARY_DIR}/bin")
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  add_definitions(-DDEBUG_OR_RELWITHDEBINFO)
endif()

# If we are using MSVC we want static runtime linkage
if (MSVC)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# If we are compiling for macOS we want to target OS versions down to 10.11
if (APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE INTERNAL "")
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
# Verhindere Installation von Submodulen
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
add_subdirectory(../modules/tracktion_engine/modules/juce ./cmake_build_juce EXCLUDE_FROM_ALL)
add_subdirectory(../modules/tracktion_engine/modules ./cmake_build_tracktion EXCLUDE_FROM_ALL)


add_subdirectory(resources)

juce_add_gui_app(${TargetName} PRODUCT_NAME "NextStudio")
juce_generate_juce_header(${TargetName})

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_20)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        )

if(BUILD_DIR_NAME STREQUAL "jumake_build")
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${JUCE_BINARY_DIR}"
        )
endif()
target_sources(${TargetName} PRIVATE
        ../resources/icon.rc 
        Source/AutomatableSliderComponent.cpp
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MenuBar.cpp
        Source/SidebarComponent.cpp
        Source/SidebarMenu.cpp
        Source/HeaderComponent.cpp
        Source/TimeLineComponent.cpp
        Source/EditComponent.cpp
        Source/EditViewState.cpp
        Source/FourOscPluginComponent.cpp
        Source/Plugins/SimpleSynth/SimpleSynthPlugin.cpp
        Source/PresetManagerComponent.cpp
        Source/FileBrowser.cpp
        Source/HeaderComponent.cpp
        Source/InstrumentEffectChooser.cpp
        Source/PluginWindow.cpp
        Source/PluginComponent.cpp
        Source/PluginMenu.cpp
        Source/LevelMeterComponent.cpp
        Source/RackView.cpp
        Source/PianoRollEditor.cpp
        Source/PlayHeadComponent.cpp
        Source/PreviewComponent.cpp
        Source/PositionDisplayComponent.cpp
        Source/LowerRangeComponent.cpp
        Source/TrackHeadComponent.cpp
        Source/AudioMidiSettings.cpp
        Source/MidiViewport.cpp
        Source/TimelineOverlayComponent.cpp
        Source/RecordingClipComponent.cpp
        Source/VelocityEditor.cpp
        Source/KeyboardView.cpp
        Source/LassoSelectionTool.cpp
        Source/TrackListView.cpp
        Source/SongEditorView.cpp
        Source/Utilities.cpp
        Source/Thumbnail.cpp
        Source/PluginBrowser.cpp
        Source/PluginScanner.cpp
        Source/InstrumentEffectChooser.cpp
        Source/SearchFieldComponent.cpp
        Source/SampleBrowser.cpp
        Source/FileBrowser.cpp
        Source/ProjectsBrowser.cpp
        Source/Browser_Base.cpp
        Source/SplitterComponent.cpp
        Source/TrackHeightManager.cpp
        Source/TrackLaneComponent.cpp
        Source/AutomationLaneComponent.cpp
        Source/ThumbNailManager.cpp
        Source/PluginViewComponent.cpp
        Source/ToolFactory.cpp
        Source/tools/PointerTool.cpp
        Source/tools/DrawTool.cpp
        Source/tools/EraserTool.cpp
        Source/tools/KnifeTool.cpp
        Source/tools/RangeTool.cpp
        Source/tools/LassoTool.cpp
        Source/DrumPadComponent.cpp
        Source/SoundEditorPanel.cpp
        Source/DrumSamplerView.cpp
        Source/SampleDisplay.cpp
        Source/MixerComponent.cpp
        Source/LowerRangeTabBar.cpp
        Source/MixerChannelStripComponent.cpp
        )

target_compile_definitions(${TargetName} PRIVATE
        TRACKTION_JUCE7=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>"
        TRACKTION_ENABLE_TIMESTRETCH_SOUNDTOUCH=1
        TRACKTION_BUILD_RUBBERBAND=1
        )

target_compile_definitions (${TargetName} PUBLIC
        # JUCE_JACK=1
        # JUCE_WASAPI=1
#        JUCE_USE_WINRT_MIDI=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_LADSPA=1
        JUCE_PLUGINHOST_LV2=1
        JUCE_PLUGINHOST_AU=1
        JUCE_USE_FLAC=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_WINDOWS_MEDIA_FORMAT=1
        DONT_SET_USING_JUCE_NAMESPACE=1
     	JUCE_MODAL_LOOPS_PERMITTED=1
        )

if (WIN32)
    target_compile_definitions(${TargetName} PUBLIC
        JUCE_WASAPI=1
        JUCE_ASIO=0         # auf 1 setzen, wenn ASIO-SDK eingebunden ist
        JUCE_JACK=0
    )
elseif(APPLE)
    target_compile_definitions(${TargetName} PUBLIC
        JUCE_JACK=0
    )
elseif(UNIX) # Linux
    target_compile_definitions(${TargetName} PUBLIC
        JUCE_ALSA=1
        JUCE_JACK=1        # nur wenn JACK installiert/genutzt werden soll
    )
endif()


target_link_libraries(${TargetName} PRIVATE
        Pictures

        tracktion::tracktion_core
        tracktion::tracktion_engine
        tracktion::tracktion_graph
        )

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "-latomic")
endif()



