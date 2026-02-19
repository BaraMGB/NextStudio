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

target_include_directories(${TargetName} PRIVATE Source)

target_sources(${TargetName} PRIVATE
        ../resources/icon.rc 
        Source/LowerRange/LowerRangeComponent.cpp
        Source/LowerRange/LowerRangeTabBar.cpp
        Source/LowerRange/Mixer/MixerChannelStripComponent.cpp
        Source/LowerRange/Mixer/MixerComponent.cpp
        Source/LowerRange/PianoRoll/KeyboardView.cpp
        Source/LowerRange/PianoRoll/MidiViewport.cpp
        Source/LowerRange/PianoRoll/PianoRollEditor.cpp
        Source/LowerRange/PianoRoll/VelocityEditor.cpp
        Source/LowerRange/PluginChain/ModifierViewComponent.cpp
        Source/LowerRange/PluginChain/PluginViewComponent.cpp
        Source/LowerRange/PluginChain/PresetHelpers.cpp
        Source/LowerRange/PluginChain/PresetManagerComponent.cpp
        Source/LowerRange/PluginChain/PluginChainItemView.cpp
        Source/LowerRange/PluginChain/PluginChainView.cpp
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/Plugins/Arpeggiator/ArpeggiatorPlugin.cpp
        Source/Plugins/Arpeggiator/ArpeggiatorPluginComponent.cpp
        Source/Plugins/Chorus/ChorusPluginComponent.cpp
        Source/Plugins/Chorus/NextChorusPlugin.cpp
        Source/Plugins/Compressor/CompressorPluginComponent.cpp
        Source/Plugins/Delay/DelayPluginComponent.cpp
        Source/Plugins/Delay/NextDelayPlugin.cpp
        Source/Plugins/DrumSampler/DrumPadComponent.cpp
        Source/Plugins/DrumSampler/DrumSamplerView.cpp
        Source/Plugins/DrumSampler/SoundEditorPanel.cpp
        Source/Plugins/EQ/EqPluginComponent.cpp
        Source/Plugins/Filter/FilterPluginComponent.cpp
        Source/Plugins/Filter/NextFilterPlugin.cpp
        Source/Plugins/FourOscPlugin/FourOscPluginComponent.cpp
        Source/Plugins/Phaser/NextPhaserPlugin.cpp
        Source/Plugins/Phaser/PhaserPluginComponent.cpp
        Source/Plugins/Reverb/ReverbPluginComponent.cpp
        Source/Plugins/Saturation/NextSaturationPlugin.cpp
        Source/Plugins/Saturation/SaturationPluginComponent.cpp
        Source/SideBrowser/InstrumentEffectChooser.cpp
        Source/Plugins/PluginComponent.cpp
        Source/UI/PluginMenu.cpp
        Source/UI/PluginScanner.cpp
        Source/LowerRange/PluginChain/PluginViewComponent.cpp
        Source/UI/PluginWindow.cpp
        Source/Plugins/SimpleSynth/SimpleSynthPlugin.cpp
        Source/Plugins/SimpleSynth/SimpleSynthPluginComponent.cpp
        Source/Plugins/SpectrumAnalyzer/SpectrumAnalyzerPlugin.cpp
        Source/Plugins/SpectrumAnalyzer/SpectrumAnalyzerPluginComponent.cpp
        Source/Plugins/VST/VstPluginComponent.cpp
        Source/Plugins/Volume/VolumePluginComponent.cpp
        Source/SideBrowser/AudioMidiSettings.cpp
        Source/SideBrowser/Browser_Base.cpp
        Source/SideBrowser/FileBrowser.cpp
        Source/SideBrowser/PluginBrowser.cpp
        Source/SideBrowser/ProjectsBrowser.cpp
        Source/SideBrowser/SampleBrowser.cpp
        Source/SideBrowser/SearchFieldComponent.cpp
        Source/SideBrowser/SidebarComponent.cpp
        Source/SideBrowser/SidebarMenu.cpp
        Source/SongEditor/AutomationLaneComponent.cpp
        Source/SongEditor/EditComponent.cpp
        Source/SongEditor/PlayHeadComponent.cpp
        Source/SongEditor/RecordingClipComponent.cpp
        Source/SongEditor/SongEditorView.cpp
        Source/SongEditor/Thumbnail.cpp
        Source/SongEditor/TimeLineComponent.cpp
        Source/SongEditor/TimelineOverlayComponent.cpp
        Source/SongEditor/TrackHeadComponent.cpp
        Source/SongEditor/TrackLaneComponent.cpp
        Source/SongEditor/TrackListView.cpp
        Source/Tools/ToolFactory.cpp
        Source/Tools/tools/DrawTool.cpp
        Source/Tools/tools/EraserTool.cpp
        Source/Tools/tools/KnifeTool.cpp
        Source/Tools/tools/LassoSelectionTool.cpp
        Source/Tools/tools/LassoTool.cpp
        Source/Tools/tools/PointerTool.cpp
        Source/Tools/tools/RangeTool.cpp
        Source/UI/Controls/AutomatableComboBox.cpp
        Source/UI/Controls/AutomatableEnvelopeParameter.cpp
        Source/UI/Controls/AutomatableParameter.cpp
        Source/UI/Controls/AutomatableSlider.cpp
        Source/UI/Controls/AutomatableToggle.cpp
        Source/UI/Controls/NonAutomatableParameter.cpp
        Source/UI/Controls/ParameterComponent.cpp
        Source/UI/HeaderComponent.cpp
        Source/UI/LevelMeterComponent.cpp
        Source/UI/MenuBar.cpp
        Source/UI/ModifierDetailPanel.cpp
        Source/UI/ModifierSidebar.cpp
        Source/UI/PositionDisplayComponent.cpp
        Source/UI/PreviewComponent.cpp
        Source/UI/SampleDisplay.cpp
        Source/UI/SetupWizard.cpp
        Source/UI/SplitterComponent.cpp
        Source/Utilities/EditViewState.cpp
        Source/Utilities/ThumbNailManager.cpp
        Source/Utilities/TrackHeightManager.cpp
        Source/Utilities/Utilities.cpp
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
        Samples707
        Samples808
        Samples909

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
