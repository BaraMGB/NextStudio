juce_add_binary_data(Pictures SOURCES
    metronome.svg
    folder.svg
    file.svg
    file-music.svg
    pause.svg
    play.svg
    render.svg
    record.svg
    stop.svg
    headphones-settings.svg
    new-box.svg
    power-plug.svg
    file-download.svg
    content-save-edit.svg
    cached.svg
    piano.svg
    piano5.svg
    wavetest5.svg
    arrowright18.svg
    arrowdown18.svg
    automation.svg
    follow.svg
    folderopen.svg
    folderclosed.svg
    expandPluginPlain18.svg
    collapse.svg
    expand.svg
    timeShiftCursor.svg
    curveSteepnessCursor.svg
    shiftCursorLeftEdge.svg
    shiftCursorRightEdge.svg
    timeShiftCursorRightEdge.svg
    shiftHandCursor.svg
    select_icon.svg
    rubberband.svg
    rubber.svg
    select_timerange.svg
    split.svg
    time_stretch_button.svg
    pitch.svg
    reverse_clip.svg
    delete_icon.svg
    pencil.svg
    projectsButton.svg
    pluginsButton.svg
    presetsButton.svg
    homeButton.svg
    samplesButton.svg
    settingsButton.svg
    newProjectButton.svg
    vst3Icon.svg
    ladspaIcon.svg
    la2Icon.svg
    AUIcon.svg
    logo.svg
    INTIcon.svg
    menu.svg
    renderButton.svg
    sine.svg
    saw.svg
    rnd.svg
    tri.svg
    square.svg
    
    )

file(GLOB_RECURSE SAMPLES_707 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Samples/707/*)
file(GLOB_RECURSE SAMPLES_808 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Samples/808/*)
file(GLOB_RECURSE SAMPLES_909 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Samples/909/*)

juce_add_binary_data(Samples707 SOURCES ${SAMPLES_707} NAMESPACE Samples707 HEADER_NAME Samples707.h)
juce_add_binary_data(Samples808 SOURCES ${SAMPLES_808} NAMESPACE Samples808 HEADER_NAME Samples808.h)
juce_add_binary_data(Samples909 SOURCES ${SAMPLES_909} NAMESPACE Samples909 HEADER_NAME Samples909.h)
