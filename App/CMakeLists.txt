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

set(HAS_RUBBERBAND 0)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../modules/rubberband/")
    message(STATUS "Found rubberband, enabling")
    set(HAS_RUBBERBAND 1)
endif()


add_subdirectory(resources)

juce_add_gui_app(${TargetName}
        PRODUCT_NAME "NextStudio"
        VERSION "${NEXTSTUDIO_VERSION}")
set_target_properties(${TargetName} PROPERTIES JUCE_VERSION "${NEXTSTUDIO_VERSION}")
juce_generate_juce_header(${TargetName})

# ---------------------------------------------------------------------------
# Focused console unit tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    juce_add_console_app(NextStudioTests
            PRODUCT_NAME "NextStudioTests")
    juce_generate_juce_header(NextStudioTests)

    target_sources(NextStudioTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PositionDisplayHelpers.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/PositionDisplayTests.cpp)

    target_include_directories(NextStudioTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(NextStudioTests PRIVATE
            juce::juce_core
            tracktion::tracktion_core)

    target_compile_definitions(NextStudioTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME PositionDisplayHelpers
            COMMAND NextStudioTests)

    add_executable(PluginChainLayoutTests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/PluginChainLayoutTests.cpp)

    target_include_directories(PluginChainLayoutTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_test(NAME PluginChainLayout
            COMMAND PluginChainLayoutTests)

    add_executable(EffectEditorLayoutTests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/EffectEditorLayoutTests.cpp)

    target_include_directories(EffectEditorLayoutTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_test(NAME EffectEditorLayout
            COMMAND EffectEditorLayoutTests)

    add_executable(SplitterCollapseControllerTests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/SplitterCollapseControllerTests.cpp)

    target_include_directories(SplitterCollapseControllerTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_test(NAME SplitterCollapseController
            COMMAND SplitterCollapseControllerTests)

    add_executable(MainInteractionStateTests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/MainInteractionStateTests.cpp)

    target_include_directories(MainInteractionStateTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_test(NAME MainInteractionState
            COMMAND MainInteractionStateTests)

    juce_add_console_app(DebugProtocolTests
            PRODUCT_NAME "DebugProtocolTests")
    juce_generate_juce_header(DebugProtocolTests)

    target_sources(DebugProtocolTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/DebugProtocol.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DebugProtocolTests.cpp)

    target_include_directories(DebugProtocolTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(DebugProtocolTests PRIVATE
            juce::juce_core)

    target_compile_definitions(DebugProtocolTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME DebugProtocol
            COMMAND DebugProtocolTests)

    juce_add_console_app(DebugSnapshotWriterTests
            PRODUCT_NAME "DebugSnapshotWriterTests")
    juce_generate_juce_header(DebugSnapshotWriterTests)

    target_sources(DebugSnapshotWriterTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/DebugSnapshotWriter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DebugSnapshotWriterTests.cpp)

    target_include_directories(DebugSnapshotWriterTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(DebugSnapshotWriterTests PRIVATE
            juce::juce_graphics)

    target_compile_definitions(DebugSnapshotWriterTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME DebugSnapshotWriter
            COMMAND DebugSnapshotWriterTests)

    juce_add_console_app(DebugStateFilterTests
            PRODUCT_NAME "DebugStateFilterTests")
    juce_generate_juce_header(DebugStateFilterTests)

    target_sources(DebugStateFilterTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/DebugStateFilter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DebugStateFilterTests.cpp)

    target_include_directories(DebugStateFilterTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(DebugStateFilterTests PRIVATE
            juce::juce_core)

    target_compile_definitions(DebugStateFilterTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME DebugStateFilter
            COMMAND DebugStateFilterTests)

    juce_add_console_app(DebugSettingsIsolationTests
            PRODUCT_NAME "DebugSettingsIsolationTests")
    juce_generate_juce_header(DebugSettingsIsolationTests)

    target_sources(DebugSettingsIsolationTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DebugSettingsIsolationTests.cpp)

    target_include_directories(DebugSettingsIsolationTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(DebugSettingsIsolationTests PRIVATE
            juce::juce_graphics
            juce::juce_data_structures)

    target_compile_definitions(DebugSettingsIsolationTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME DebugSettingsIsolation
            COMMAND DebugSettingsIsolationTests)

    juce_add_console_app(DebugAppControllerTests
            PRODUCT_NAME "DebugAppControllerTests")
    juce_generate_juce_header(DebugAppControllerTests)

    target_sources(DebugAppControllerTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/DebugAppController.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/DebugProtocol.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/DebugAppControllerTests.cpp)

    target_include_directories(DebugAppControllerTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(DebugAppControllerTests PRIVATE
            tracktion::tracktion_engine
            tracktion::tracktion_core
            juce::juce_gui_basics)

    target_compile_definitions(DebugAppControllerTests PRIVATE
            TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME DebugAppController
            COMMAND DebugAppControllerTests)

    juce_add_console_app(ProjectLifecycleTests
            PRODUCT_NAME "ProjectLifecycleTests")
    juce_generate_juce_header(ProjectLifecycleTests)

    target_sources(ProjectLifecycleTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ProjectLifecycle.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ProjectLifecycleTests.cpp)

    target_include_directories(ProjectLifecycleTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(ProjectLifecycleTests PRIVATE
            juce::juce_core
            juce::juce_data_structures)

    target_compile_definitions(ProjectLifecycleTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME ProjectLifecycle
            COMMAND ProjectLifecycleTests)

    juce_add_console_app(ProjectWorkflowTests
            PRODUCT_NAME "ProjectWorkflowTests")
    juce_generate_juce_header(ProjectWorkflowTests)

    target_sources(ProjectWorkflowTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ProjectWorkflow.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ProjectWorkflowTests.cpp)

    target_include_directories(ProjectWorkflowTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(ProjectWorkflowTests PRIVATE
            juce::juce_core)

    target_compile_definitions(ProjectWorkflowTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME ProjectWorkflow
            COMMAND ProjectWorkflowTests)

    juce_add_console_app(MidiNoteOverlapTests
            PRODUCT_NAME "MidiNoteOverlapTests")
    juce_generate_juce_header(MidiNoteOverlapTests)

    target_sources(MidiNoteOverlapTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/MidiNoteOverlap.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/MidiNoteOverlapTests.cpp)

    target_include_directories(MidiNoteOverlapTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(MidiNoteOverlapTests PRIVATE
            juce::juce_core)

    target_compile_definitions(MidiNoteOverlapTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME MidiNoteOverlap
            COMMAND MidiNoteOverlapTests)

    juce_add_console_app(MidiPendingPasteTests
            PRODUCT_NAME "MidiPendingPasteTests")
    juce_generate_juce_header(MidiPendingPasteTests)

    target_sources(MidiPendingPasteTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/MidiPendingPaste.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/MidiPendingPasteTests.cpp)

    target_include_directories(MidiPendingPasteTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(MidiPendingPasteTests PRIVATE
            juce::juce_core)

    target_compile_definitions(MidiPendingPasteTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME MidiPendingPaste
            COMMAND MidiPendingPasteTests)

    juce_add_console_app(PianoRollNoteLengthTests
            PRODUCT_NAME "PianoRollNoteLengthTests")
    juce_generate_juce_header(PianoRollNoteLengthTests)

    target_sources(PianoRollNoteLengthTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PianoRollNoteLength.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/PianoRollNoteLengthTests.cpp)

    target_include_directories(PianoRollNoteLengthTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(PianoRollNoteLengthTests PRIVATE
            juce::juce_core)

    target_compile_definitions(PianoRollNoteLengthTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME PianoRollNoteLength
            COMMAND PianoRollNoteLengthTests)

    juce_add_console_app(ComputerMidiKeyboardLayoutTests
            PRODUCT_NAME "ComputerMidiKeyboardLayoutTests")
    juce_generate_juce_header(ComputerMidiKeyboardLayoutTests)

    target_sources(ComputerMidiKeyboardLayoutTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ComputerMidiKeyboardLayout.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Logging.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ComputerMidiKeyboardLayoutTests.cpp)

    target_include_directories(ComputerMidiKeyboardLayoutTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(ComputerMidiKeyboardLayoutTests PRIVATE
            juce::juce_audio_utils)

    target_compile_definitions(ComputerMidiKeyboardLayoutTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME ComputerMidiKeyboardLayout
            COMMAND ComputerMidiKeyboardLayoutTests)

    juce_add_console_app(ClipOverwriteCommandTests
            PRODUCT_NAME "ClipOverwriteCommandTests")
    juce_generate_juce_header(ClipOverwriteCommandTests)

    target_sources(ClipOverwriteCommandTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ClipOverwriteCommand.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ClipOverwriteCommandTests.cpp)

    target_include_directories(ClipOverwriteCommandTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(ClipOverwriteCommandTests PRIVATE
            tracktion::tracktion_engine
            tracktion::tracktion_core
            juce::juce_gui_basics)

    target_compile_definitions(ClipOverwriteCommandTests PRIVATE
            TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME ClipOverwriteCommand
            COMMAND ClipOverwriteCommandTests)

    juce_add_console_app(MidiInputRoutingTests
            PRODUCT_NAME "MidiInputRoutingTests")
    juce_generate_juce_header(MidiInputRoutingTests)

    target_sources(MidiInputRoutingTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/MidiInputRouting.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/MidiInputRoutingTests.cpp)

    target_include_directories(MidiInputRoutingTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(MidiInputRoutingTests PRIVATE
            tracktion::tracktion_engine
            tracktion::tracktion_core
            juce::juce_gui_basics)

    target_compile_definitions(MidiInputRoutingTests PRIVATE
            TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME MidiInputRouting
            COMMAND MidiInputRoutingTests)

    juce_add_console_app(MetronomeSampleManagerTests
            PRODUCT_NAME "MetronomeSampleManagerTests")
    juce_generate_juce_header(MetronomeSampleManagerTests)

    target_sources(MetronomeSampleManagerTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/MetronomeSampleManager.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/MetronomeSampleManagerTests.cpp)

    target_include_directories(MetronomeSampleManagerTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(MetronomeSampleManagerTests PRIVATE
            juce::juce_audio_formats)

    target_compile_definitions(MetronomeSampleManagerTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME MetronomeSampleManager
            COMMAND MetronomeSampleManagerTests)

    juce_add_console_app(ThemePresetModelTests
            PRODUCT_NAME "ThemePresetModelTests")
    juce_generate_juce_header(ThemePresetModelTests)

    target_sources(ThemePresetModelTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/InlineColourEditor.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ThemePresetBrowser.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/ThemePresetModel.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ThemePresetModelTests.cpp)

    target_include_directories(ThemePresetModelTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(ThemePresetModelTests PRIVATE
            juce::juce_gui_basics
            juce::juce_data_structures)

    target_compile_definitions(ThemePresetModelTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME ThemePresetModel
            COMMAND ThemePresetModelTests)

    juce_add_console_app(EqBandResetTests
            PRODUCT_NAME "EqBandResetTests")
    juce_generate_juce_header(EqBandResetTests)

    target_sources(EqBandResetTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/EqBandReset.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/EqBandResetTests.cpp)

    target_include_directories(EqBandResetTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(EqBandResetTests PRIVATE
            tracktion::tracktion_engine
            tracktion::tracktion_core
            juce::juce_gui_basics)

    target_compile_definitions(EqBandResetTests PRIVATE
            TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME EqBandReset
            COMMAND EqBandResetTests)

    juce_add_console_app(PitchShiftDragTests
            PRODUCT_NAME "PitchShiftDragTests")
    juce_generate_juce_header(PitchShiftDragTests)

    target_sources(PitchShiftDragTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PitchShiftDrag.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/PitchShiftDragTests.cpp)

    target_include_directories(PitchShiftDragTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(PitchShiftDragTests PRIVATE
            tracktion::tracktion_engine
            tracktion::tracktion_core
            juce::juce_gui_basics)

    target_compile_definitions(PitchShiftDragTests PRIVATE
            TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME PitchShiftDrag
            COMMAND PitchShiftDragTests)

    juce_add_console_app(PluginBypassPresentationTests
            PRODUCT_NAME "PluginBypassPresentationTests")
    juce_generate_juce_header(PluginBypassPresentationTests)

    target_sources(PluginBypassPresentationTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/PluginBypassPresentation.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/PluginBypassPresentationTests.cpp)

    target_include_directories(PluginBypassPresentationTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(PluginBypassPresentationTests PRIVATE
            juce::juce_gui_basics
            juce::juce_data_structures)

    target_compile_definitions(PluginBypassPresentationTests PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_MODAL_LOOPS_PERMITTED=1)

    add_test(NAME PluginBypassPresentation
            COMMAND PluginBypassPresentationTests)

    juce_add_console_app(PluginMenuOrderingTests
            PRODUCT_NAME "PluginMenuOrderingTests")
    juce_generate_juce_header(PluginMenuOrderingTests)

    target_sources(PluginMenuOrderingTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/PluginMenuOrderingTests.cpp)

    target_include_directories(PluginMenuOrderingTests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(PluginMenuOrderingTests PRIVATE
            tracktion::tracktion_engine
            juce::juce_gui_basics)

    target_compile_definitions(PluginMenuOrderingTests PRIVATE
            TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    add_test(NAME PluginMenuOrdering
            COMMAND PluginMenuOrderingTests)

    juce_add_console_app(ClipFrameDrawingTests PRODUCT_NAME "ClipFrameDrawingTests")
    target_sources(ClipFrameDrawingTests PRIVATE tests/ClipFrameDrawingTests.cpp)
    target_include_directories(ClipFrameDrawingTests PRIVATE include)
    target_link_libraries(ClipFrameDrawingTests PRIVATE juce::juce_graphics)
    target_compile_definitions(ClipFrameDrawingTests PRIVATE JUCE_USE_CURL=0)
    add_test(NAME ClipFrameDrawing COMMAND ClipFrameDrawingTests)

    juce_add_console_app(TimelineViewGeometryTests PRODUCT_NAME "TimelineViewGeometryTests")
    target_sources(TimelineViewGeometryTests PRIVATE
            src/TimelineViewGeometry.cpp tests/TimelineViewGeometryTests.cpp)
    target_include_directories(TimelineViewGeometryTests PRIVATE include)
    target_compile_features(TimelineViewGeometryTests PRIVATE cxx_std_20)
    target_link_libraries(TimelineViewGeometryTests PRIVATE juce::juce_gui_basics)
    target_compile_definitions(TimelineViewGeometryTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    add_test(NAME TimelineViewGeometry COMMAND TimelineViewGeometryTests)

    juce_add_console_app(TimelineViewStateTests PRODUCT_NAME "TimelineViewStateTests")
    juce_generate_juce_header(TimelineViewStateTests)
    target_sources(TimelineViewStateTests PRIVATE
            src/EditViewState.cpp src/TimelineViewGeometry.cpp src/TrackHeightManager.cpp
            src/ThumbNailManager.cpp src/Logging.cpp tests/TimelineViewStateTests.cpp)
    target_include_directories(TimelineViewStateTests PRIVATE include)
    target_compile_features(TimelineViewStateTests PRIVATE cxx_std_20)
    target_link_libraries(TimelineViewStateTests PRIVATE tracktion::tracktion_engine juce::juce_gui_basics)
    target_compile_definitions(TimelineViewStateTests PRIVATE TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    add_test(NAME TimelineViewState COMMAND TimelineViewStateTests)

    juce_add_console_app(SelectionGestureTests PRODUCT_NAME "SelectionGestureTests")
    juce_generate_juce_header(SelectionGestureTests)
    target_sources(SelectionGestureTests PRIVATE src/LassoSelectionComponent.cpp tests/SelectionGestureTests.cpp)
    target_include_directories(SelectionGestureTests PRIVATE include)
    target_compile_features(SelectionGestureTests PRIVATE cxx_std_20)
    target_link_libraries(SelectionGestureTests PRIVATE tracktion::tracktion_engine juce::juce_gui_basics)
    target_compile_definitions(SelectionGestureTests PRIVATE TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    add_test(NAME SelectionGestures COMMAND SelectionGestureTests)

    juce_add_console_app(TimelineSoftSnapTests PRODUCT_NAME "TimelineSoftSnapTests")
    target_sources(TimelineSoftSnapTests PRIVATE src/TimelineSoftSnap.cpp tests/TimelineSoftSnapTests.cpp)
    target_include_directories(TimelineSoftSnapTests PRIVATE include)
    target_compile_features(TimelineSoftSnapTests PRIVATE cxx_std_20)
    target_link_libraries(TimelineSoftSnapTests PRIVATE juce::juce_core)
    target_compile_definitions(TimelineSoftSnapTests PRIVATE JUCE_USE_CURL=0)
    add_test(NAME TimelineSoftSnap COMMAND TimelineSoftSnapTests)

    juce_add_console_app(TimelineSnappingTests PRODUCT_NAME "TimelineSnappingTests")
    juce_generate_juce_header(TimelineSnappingTests)
    target_sources(TimelineSnappingTests PRIVATE
            src/TimelineSoftSnap.cpp src/TimelineSnapResolver.cpp src/TimelineViewGeometry.cpp src/PianoRollDrawGesture.cpp
            src/PianoRollNoteLength.cpp src/MidiNoteGesture.cpp src/MidiNoteCreation.cpp src/MidiNoteOverlap.cpp
            src/ClipGestureLimits.cpp src/AutomationGestureLimits.cpp tests/TimelineSnappingTests.cpp)
    target_include_directories(TimelineSnappingTests PRIVATE include)
    target_compile_features(TimelineSnappingTests PRIVATE cxx_std_20)
    target_link_libraries(TimelineSnappingTests PRIVATE tracktion::tracktion_engine juce::juce_gui_basics)
    target_compile_definitions(TimelineSnappingTests PRIVATE TRACKTION_JUCE7=1
            JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    add_test(NAME TimelineSnapping COMMAND TimelineSnappingTests)
endif()

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

target_include_directories(${TargetName} PRIVATE include)
target_include_directories(${TargetName} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../modules/tinysoundfont")

if(HAS_RUBBERBAND)
    target_include_directories(${TargetName} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/../modules")
endif()

file(GLOB NEXTSTUDIO_SOURCES src/*.cpp)
target_sources(${TargetName} PRIVATE
        ../resources/icon.rc
        ${NEXTSTUDIO_SOURCES}
        )

target_compile_definitions(${TargetName} PRIVATE
        TRACKTION_JUCE7=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_PROJECT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${TargetName},JUCE_VERSION>"
        TRACKTION_ENABLE_TIMESTRETCH_SOUNDTOUCH=1
        TRACKTION_ENABLE_TIMESTRETCH_RUBBERBAND=${HAS_RUBBERBAND}
        TRACKTION_BUILD_RUBBERBAND=${HAS_RUBBERBAND}
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
        ThemePresets
        Samples707
        Samples808
        Samples909

        tracktion::tracktion_core
        tracktion::tracktion_engine
        tracktion::tracktion_graph
        )

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
    # gcc 15: nanorange.hpp (tracktion 3rd-party) has a template-body error
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-template-body")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "-latomic")
endif()
