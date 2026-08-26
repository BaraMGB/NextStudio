
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "ArpeggiatorPlugin.h"
#include "ClipOverwriteCommand.h"
#include "DebugSessionEnvironment.h"
#include "InitialContentSetup.h"
#include "NextChorusPlugin.h"
#include "NextDelayPlugin.h"
#include "NextFilterPlugin.h"
#include "NextPhaserPlugin.h"
#include "NextSaturationPlugin.h"
#include "PeakLimiterPlugin.h"
#include "ProjectsBrowser.h"
#include "SetupWizard.h"
#include "SidebarComponent.h"
#include "SimpleSynthPlugin.h"
#include "SoundFontPlugin.h"
#include "SpectrumAnalyzerPlugin.h"
#include "ThemeHelpers.h"
#include "Utilities.h"
#include "WineRendererFallback.h"

MainComponent::MainComponent(ApplicationViewState &state, NextStudio::WineRendererFallback &wineRendererFallback, bool debugMode, const juce::File &debugSessionDirectory)
    : m_applicationState(state),
      m_wineRendererFallback(wineRendererFallback),
      m_nextLookAndFeel(state),
      m_sidebarSplitter(false),
      m_debugMode(debugMode)
{
    if (m_debugMode)
    {
        const auto debugTempDir = debugSessionDirectory == juce::File() ? NextStudio::Debug::SessionEnvironment::createDebugSessionTempDirectory() : debugSessionDirectory;
        if (m_engine.getTemporaryFileManager().setTempDirectory(debugTempDir))
        {
            NS_LOG_INFO(app, "debug shell temp directory: " + debugTempDir.getFullPathName());
        }
        else
        {
            NS_LOG_ERROR(app, "failed to set debug shell temp directory: " + debugTempDir.getFullPathName());
        }
    }

    const auto configuredWorkDir = juce::File(m_applicationState.m_workDir.get());
    const auto defaultWorkDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("NextStudio");
    const bool configuredWorkDirExists = configuredWorkDir.exists() && configuredWorkDir.isDirectory();
    const bool needsSetupWizard = !m_applicationState.m_setupComplete || !configuredWorkDirExists;

    // Keep the current UX goal: preselect the fallback path, but don't create it until setup is resolved.
    if (needsSetupWizard && !configuredWorkDirExists)
        m_applicationState.setRootFolder(defaultWorkDir);

    float scale = m_applicationState.m_appScale;
    scale = juce::jlimit(0.2f, 3.f, scale);
    juce::Desktop::getInstance().setGlobalScaleFactor(scale);

    setWantsKeyboardFocus(true);
    juce::LookAndFeel::setDefaultLookAndFeel(&m_nextLookAndFeel);
    NextStudio::WineRendererFallback::configureFontFallback(m_nextLookAndFeel);
    updateTheme();

    if (!needsSetupWizard)
        ensureUserDirectoriesAndSamples();

    addAndMakeVisible(m_sidebarSplitter);
    m_sidebarSplitter.onMouseDown = [this]() { handleSidebarSplitterMouseDown(); };
    m_sidebarSplitter.onDrag = [this](int dragDistance) { handleSidebarSplitterDrag(dragDistance); };

    m_engine.getPluginManager().createBuiltInType<SimpleSynthPlugin>();
    m_engine.getPluginManager().createBuiltInType<ArpeggiatorPlugin>();
    m_engine.getPluginManager().createBuiltInType<SpectrumAnalyzerPlugin>();
    m_engine.getPluginManager().createBuiltInType<PeakLimiterPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextDelayPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextChorusPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextPhaserPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextSaturationPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextFilterPlugin>();
    m_engine.getPluginManager().createBuiltInType<SoundFontPlugin>();

    // Always start with the Projects sidebar expanded at its default width.
    m_applicationState.m_sidebarWidth = SidebarLayout::defaultExpandedWidth;
    m_applicationState.m_sidebarCollapsed = false;
    openValidStartEdit();

    m_commandManager.registerAllCommandsForTarget(this);
    m_commandManager.registerAllCommandsForTarget(m_editComponent.get());
    m_commandManager.registerAllCommandsForTarget(&m_editComponent->getTrackListView());
    m_commandManager.registerAllCommandsForTarget(&m_lowerRange->getPianoRollEditor());

    m_selectionManager.addChangeListener(this);
    m_applicationState.m_applicationStateValueTree.addListener(this);

    if (needsSetupWizard)
    {
        NS_LOG_INFO(setup, "setup wizard scheduled");
        launchSetupWizardAsync();
    }
}

MainComponent::~MainComponent()
{
    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
        uiBehaviour->setFocusedEdit(nullptr);

    m_applicationState.m_applicationStateValueTree.removeListener(this);
    m_selectionManager.removeChangeListener(this);
    if (m_edit)
        m_edit->state.removeListener(this);

    if (m_header)
        m_header->removeAllChangeListeners();

    if (m_editComponent)
        m_editComponent->getSongEditor().clear();

    // Explicitly destroy UI components and then the Edit to ensure correct shutdown order
    m_editorContainer = nullptr;
    m_header = nullptr;
    m_editComponent = nullptr;
    m_lowerRange = nullptr;
    m_sideBarBrowser = nullptr;
    m_editViewState = nullptr;
    m_edit = nullptr;

    saveSettings();
    if (!m_debugMode)
        m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics &g)
{
    g.setColour(m_applicationState.getMainFrameColour());
    g.fillRect(getLocalBounds());
}

int MainComponent::getPreferredSidebarWidth() const
{
    return SidebarLayout::getPreferredWidth((int)m_applicationState.m_sidebarWidth);
}

void MainComponent::handleSidebarSplitterMouseDown()
{
    const bool collapsed = m_applicationState.m_sidebarCollapsed;
    m_sidebarWidthAtMousedown = getPreferredSidebarWidth();

    m_sidebarSplitterCollapseController.beginDrag(collapsed, SidebarLayout::getTransitionDistance(m_sidebarWidthAtMousedown, collapsed));
}

void MainComponent::handleSidebarSplitterDrag(int dragDistance)
{
    const bool collapsed = m_applicationState.m_sidebarCollapsed;
    const bool requestedCollapsed = m_sidebarSplitterCollapseController.getCollapsedState(-dragDistance, collapsed);

    if (requestedCollapsed != collapsed)
    {
        m_applicationState.m_sidebarWidth = requestedCollapsed ? SidebarLayout::minimumExpandedWidth : m_sidebarWidthAtMousedown;
        m_applicationState.m_sidebarCollapsed = requestedCollapsed;
        resized();
        return;
    }

    if (m_sidebarSplitterCollapseController.startedCollapsed() || collapsed)
        return;

    const auto resizedWidth = SidebarLayout::getResizedWidth(m_sidebarWidthAtMousedown, dragDistance);
    if (resizedWidth != (int)m_applicationState.m_sidebarWidth)
    {
        m_applicationState.m_sidebarWidth = resizedWidth;
        resized();
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto &evs = m_editComponent->getEditViewState();
    auto lowerRangeHeight = LowerRangeComponent::collapsedHeight;
    if (!m_applicationState.m_lowerRangeCollapsed)
    {
        if (evs.getLowerRangeView() == LowerRangeView::midiEditor)
            lowerRangeHeight = evs.m_midiEditorHeight;
        else if (evs.getLowerRangeView() == LowerRangeView::pluginRack || evs.getLowerRangeView() == LowerRangeView::mixer)
            lowerRangeHeight = LowerRangeComponent::defaultExpandedHeight;
    }

    auto lowerRange = area.removeFromBottom(lowerRangeHeight);
    const auto sidebarWidth = m_applicationState.m_sidebarCollapsed
                                ? SidebarLayout::collapsedWidth
                                : getPreferredSidebarWidth();
    m_sideBarBrowser->setBounds(area.removeFromLeft(sidebarWidth));
    m_sidebarSplitter.setBounds(area.removeFromLeft(10));
    m_editorContainer->setBounds(area);
    m_lowerRange->setBounds(lowerRange);
}

bool MainComponent::keyStateChanged(bool isKeyDown)

{
    juce::ignoreUnused(isKeyDown);

    int rootNote = 48;
    int gap = 0;

    for (auto kp : m_pressedKeysForMidiKeyboard)
        if (!kp.isCurrentlyDown())
        {
            m_pressedKeysForMidiKeyboard.removeFirstMatchingValue(kp);
            // send noteOff
            auto command = m_commandManager.getKeyMappings()->findCommandForKeyPress(kp);
            if (command >= KeyPressCommandIDs::midiNoteC && command <= KeyPressCommandIDs::midiNoteTopC)
                gap = (int)command - 1;

            if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(m_editViewState->m_edit))
                EngineHelpers::getVirtualMidiInputDevice(*m_edit)->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1, rootNote + gap), 0);
        }
    return true;
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID> &commands)
{
    juce::Array<juce::CommandID> ids{KeyPressCommandIDs::midiNoteC, KeyPressCommandIDs::midiNoteCsharp, KeyPressCommandIDs::midiNoteD, KeyPressCommandIDs::midiNoteDsharp, KeyPressCommandIDs::midiNoteE, KeyPressCommandIDs::midiNoteF, KeyPressCommandIDs::midiNoteFsharp, KeyPressCommandIDs::midiNoteG, KeyPressCommandIDs::midiNoteGsharp, KeyPressCommandIDs::midiNoteA, KeyPressCommandIDs::midiNoteAsharp, KeyPressCommandIDs::midiNoteB, KeyPressCommandIDs::midiNoteUpperC, KeyPressCommandIDs::midiNoteUpperCsharp, KeyPressCommandIDs::midiNoteUpperD, KeyPressCommandIDs::midiNoteUpperDsharp, KeyPressCommandIDs::midiNoteUpperE, KeyPressCommandIDs::midiNoteUpperF, KeyPressCommandIDs::midiNoteUpperFsharp, KeyPressCommandIDs::midiNoteUpperG, KeyPressCommandIDs::midiNoteUpperGsharp, KeyPressCommandIDs::midiNoteUpperA, KeyPressCommandIDs::midiNoteUpperAsharp, KeyPressCommandIDs::midiNoteUpperB, KeyPressCommandIDs::midiNoteTopC,

                                     KeyPressCommandIDs::togglePlay, KeyPressCommandIDs::toggleRecord, KeyPressCommandIDs::play, KeyPressCommandIDs::stop,

                                     KeyPressCommandIDs::loopAroundSelection,
                                     // KeyPressCommandIDs::loopOn,
                                     // KeyPressCommandIDs::loopOff,
                                     KeyPressCommandIDs::loopAroundAll, KeyPressCommandIDs::loopToggle,

                                     // KeyPressCommandIDs::toggleSnap,
                                     KeyPressCommandIDs::toggleMetronome,
                                     // KeyPressCommandIDs::snapToBar,
                                     // KeyPressCommandIDs::snapToBeat,
                                     // KeyPressCommandIDs::snapToGrid,
                                     // KeyPressCommandIDs::snapToTime,
                                     // KeyPressCommandIDs::snapToOff,

                                     KeyPressCommandIDs::undo, KeyPressCommandIDs::redo,

                                     KeyPressCommandIDs::debugOutputEdit, KeyPressCommandIDs::saveProject};

    commands.addArray(ids);
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result)
{
    switch (commandID)
    {
    case KeyPressCommandIDs::midiNoteC:
        result.setInfo("note C", "set MIDI note C", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("y").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteCsharp:
        result.setInfo("note C#", "set MIDI note C#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("s").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteD:
        result.setInfo("note D", "set MIDI note D", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteDsharp:
        result.setInfo("note D#", "set MIDI note D#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteE:
        result.setInfo("note E", "set MIDI note E", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("c").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteF:
        result.setInfo("note F", "set MIDI note F", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("v").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteFsharp:
        result.setInfo("note F#", "set MIDI note F#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("g").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteG:
        result.setInfo("note G", "set MIDI note G", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("b").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteGsharp:
        result.setInfo("note G#", "set MIDI note G#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("h").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteA:
        result.setInfo("note A", "set MIDI note A", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("n").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteAsharp:
        result.setInfo("note A#", "set MIDI note A#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("j").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteB:
        result.setInfo("note B", "set MIDI note B", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("m").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperC:
        result.setInfo("noteUpper C", "set MIDI noteUpper C", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("q").getKeyCode(), 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription(",").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperCsharp:
        result.setInfo("noteUpper C#", "set MIDI noteUpper C#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("2").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperD:
        result.setInfo("noteUpper D", "set MIDI noteUpper D", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("w").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperDsharp:
        result.setInfo("noteUpper D#", "set MIDI noteUpper D#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("3").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperE:
        result.setInfo("noteUpper E", "set MIDI noteUpper E", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("e").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperF:
        result.setInfo("noteUpper F", "set MIDI noteUpper F", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("r").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperFsharp:
        result.setInfo("noteUpper F#", "set MIDI noteUpper F#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("5").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperG:
        result.setInfo("noteUpper G", "set MIDI noteUpper G", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("t").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperGsharp:
        result.setInfo("noteUpper G#", "set MIDI noteUpper G#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("6").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperA:
        result.setInfo("noteUpper A", "set MIDI noteUpper A", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperAsharp:
        result.setInfo("noteUpper A#", "set MIDI noteUpper A#", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("7").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteUpperB:
        result.setInfo("noteUpper B", "set MIDI noteUpper B", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("u").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::midiNoteTopC:
        result.setInfo("noteUpper C", "set MIDI noteUpper C", "virtual Midi keyboard", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("i").getKeyCode(), 0);
        break;
    case KeyPressCommandIDs::togglePlay:
        result.setInfo("Play/Pause", "Toggle play", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
        result.addDefaultKeypress(juce::KeyPress::numberPad0, 0);
        break;
    case KeyPressCommandIDs::play:
        result.setInfo("Play", "Play", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::returnKey, 0);
        break;
    case KeyPressCommandIDs::toggleRecord:
        result.setInfo("Record", "Record", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::numberPadMultiply, 0);
        break;
    case KeyPressCommandIDs::stop:
        result.setInfo("Stop", "Stop", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
        result.addDefaultKeypress(juce::KeyPress::numberPadDecimalPoint, 0);
        break;
    case KeyPressCommandIDs::loopToggle:
        result.setInfo("Loop", "Loop", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::loopAroundAll:
        result.setInfo("Loop around all", "Loop around all", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier | juce::ModifierKeys::altModifier);
        break;
    case KeyPressCommandIDs::toggleMetronome:
        result.setInfo("Metronome", "Metronome", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("m").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::loopAroundSelection:
        result.setInfo("Loop around selection", "Loop around selection", "Selection", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case KeyPressCommandIDs::debugOutputEdit:
        result.setInfo("Debug output edit", "Debug output edit", "Debug", 0);
        result.addDefaultKeypress(juce::KeyPress::F10Key, 0);
        break;
    case KeyPressCommandIDs::undo:
        result.setInfo("Undo last action", "Undo", "Song Editor", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::redo:
        result.setInfo("Redo last action", "Redo", "Song Editor", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case KeyPressCommandIDs::saveProject:
        result.setInfo("Save project", "Save the current project", "Project", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("s").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    default:
        break;
    }
}

bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo &info)
{
    NS_LOG_DEBUG(workflow, "command invoked: id=" + juce::String(static_cast<int>(info.commandID)));
    int rootNote = 48;
    switch (info.commandID)
    {
    // send NoteOn
    case KeyPressCommandIDs::midiNoteC:

        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
        {
            NS_LOG_DEBUG(engine, "virtual MIDI note-on routed to device=" + virMidiIn->getName());
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        }
        break;
    case KeyPressCommandIDs::midiNoteCsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteD:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteDsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteE:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteF:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteFsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteG:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteGsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteA:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteAsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteB:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperC:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperCsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperD:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperDsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperE:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperF:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperFsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperG:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperGsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperA:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperAsharp:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteUpperB:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::midiNoteTopC:
        if (auto virMidiIn = EngineHelpers::getVirtualMidiInputDevice(*m_edit))
            virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f), 0);
        m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
        break;
    case KeyPressCommandIDs::togglePlay:
        EngineHelpers::togglePlay(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::play:
        if (m_lowerRange == nullptr || !m_lowerRange->getPianoRollEditor().confirmPendingPasteIfActive())
            EngineHelpers::play(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::stop:
        EngineHelpers::stopPlay(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::toggleRecord:
        NS_LOG_INFO(transport, "toggle record requested");
        EngineHelpers::toggleRecord(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::loopToggle:
        EngineHelpers::toggleLoop(*m_edit);
        break;
    case KeyPressCommandIDs::loopAroundSelection:
        EngineHelpers::loopAroundSelection(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::loopOff:
        EngineHelpers::loopOff(*m_edit);
        break;
    case KeyPressCommandIDs::loopOn:
        EngineHelpers::loopOn(*m_edit);
        break;
    case KeyPressCommandIDs::loopAroundAll:
        EngineHelpers::loopAroundAll(*m_edit);
        break;
    case KeyPressCommandIDs::toggleSnap:
        EngineHelpers::toggleSnap(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::toggleMetronome:
        EngineHelpers::toggleMetronome(*m_edit);
        break;
    case KeyPressCommandIDs::debugOutputEdit:
    {
        auto editString = m_edit->state.toXmlString();
        NS_LOG_DEBUG(edit, "edit state dump requested\n" + editString);

        break;
    }

    case KeyPressCommandIDs::undo:
        m_edit->undo();
        break;
    case KeyPressCommandIDs::redo:
        m_edit->redo();
        break;
    case KeyPressCommandIDs::saveProject:
        saveCurrentProject();
        break;
    // case KeyPressCommandIDs::snapToBar:
    //     EngineHelpers::snapToBar(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToBeat:
    //     EngineHelpers::snapToBeat(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToGrid:
    //     EngineHelpers::snapToGrid(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToTime:
    //     EngineHelpers::snapToTime(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToOff:
    //     EngineHelpers::snapToOff(m_editComponent->getEditViewState());
    //     break;
    default:
        return false;
    }
    return true;
}

void MainComponent::valueTreePropertyChanged(juce::ValueTree &vt, const juce::Identifier &property)
{
    if (property == te::IDs::looping)
        m_header->loopButtonClicked();

    if (property == IDs::pianorollHeight || property == IDs::lowerRangeView || property == IDs::LowerRangeCollapsed
        || property == IDs::SidebarWidth || property == IDs::SidebarCollapsed)
        markAndUpdate(m_updateView);

    if (property == te::IDs::source || property == te::IDs::state)
        markAndUpdate(m_updateSource);

    if (vt.hasType(IDs::ThemeState))
        markAndUpdate(m_updateTheme);

    if (property == te::IDs::lastSignificantChange)
        markAndUpdate(m_saveTemp);
}
void MainComponent::handleAsyncUpdate()
{
    if (compareAndReset(m_saveTemp) && !compareAndReset(m_updateSource))
        m_hasUnsavedTemp = true;

    if (compareAndReset(m_updateView))
        resized();

    if (compareAndReset(m_updateTheme))
        updateTheme();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto *browser = dynamic_cast<BrowserBaseComponent *>(source))
    {
        const auto request = browser->m_projectRequest.take();

        juce::Component::SafePointer<MainComponent> safeThis(this);
        if (request.action == ProjectLifecycle::ProjectAction::loadProject)
        {
            juce::MessageManager::callAsync(
                [safeThis, editFile = request.file]
                {
                    if (safeThis != nullptr)
                        safeThis->setupEdit(editFile);
                });
        }
        else if (request.action == ProjectLifecycle::ProjectAction::newProject)
        {
            juce::MessageManager::callAsync(
                [safeThis]
                {
                    if (safeThis != nullptr)
                        safeThis->setupEdit(juce::File());
                });
        }
    }

    if (source == &m_selectionManager && m_editViewState)
    {
        if (m_editViewState->m_applicationState.m_exclusiveMidiFocusEnabled)
        {
            NS_LOG_DEBUG(selection, "selection changed; updating exclusive MIDI focus");
            EngineHelpers::setMidiInputFocusToSelection(*m_editViewState);
        }
    }
}

void MainComponent::openValidStartEdit()
{
    m_tempDir = m_engine.getTemporaryFileManager().getTempDirectory();
    m_tempDir.createDirectory();

    auto f = Helpers::findRecentEdit(m_tempDir);
    if (f.existsAsFile())
    {
        NS_LOG_WARN(autosave, "recovery file found: " + f.getFullPathName());
        auto result = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon, "Restore crashed project?", "It seems, NextStudio is crashed last time. Do you want to restore the last session?", "Yes", "No");
        if (result)
        {
            setupEdit(f);
            return;
        }
        else
        {
            m_tempDir.deleteRecursively();
            m_tempDir.createDirectory();
        }
    }

    setupEdit(juce::File());
}

void MainComponent::setupSideBrowser()
{
    m_sideBarBrowser = std::make_unique<SidebarComponent>(*m_editViewState, m_commandManager);
    addAndMakeVisible(*m_sideBarBrowser);
    m_sideBarBrowser->updateParentsListener();
}

void MainComponent::ensureUserDirectoriesAndSamples() { InitialContentSetup::populateBundledContent(juce::File(m_applicationState.m_workDir.get())); }

void MainComponent::launchSetupWizardAsync()
{
    juce::Component::SafePointer<MainComponent> safeThis(this);

    juce::MessageManager::callAsync(
        [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->runSetupWizard();
        });
}

void MainComponent::runSetupWizard()
{
    auto wizard = std::make_unique<SetupWizard>(m_applicationState, m_engine);
    wizard->setSize(1400, 1000);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(wizard.release());
    options.componentToCentreAround = this;
    options.dialogTitle = "NextStudio Setup Wizard";
    options.dialogBackgroundColour = m_applicationState.getBackgroundColour1();
    options.escapeKeyTriggersCloseButton = false;
    options.useNativeTitleBar = true;
    options.resizable = false;

    auto *dialog = options.create();
    m_wineRendererFallback.applyTo(*dialog);
    dialog->enterModalState(true, nullptr, true);
    const auto wizardResult = dialog->runModalLoop();

    if (wizardResult != 1)
    {
        // Aborting setup falls back to ~/NextStudio by product decision.
        const auto defaultWorkDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("NextStudio");
        m_applicationState.setRootFolder(defaultWorkDir);
        ThemeHelpers::applyBuiltInTheme(m_applicationState, ThemeHelpers::getDefaultBuiltInThemeName());
        m_applicationState.m_setupComplete = true;
        m_applicationState.saveState();
    }

    handleContentPathChangedFromSettings();

    juce::Component::SafePointer<juce::Component> mainWindow(getTopLevelComponent());
    juce::MessageManager::callAsync(
        [mainWindow]
        {
            if (mainWindow == nullptr)
                return;

            mainWindow->setVisible(true);
            mainWindow->toFront(true);
            mainWindow->repaint();
        });
}

void MainComponent::handleContentPathChangedFromSettings()
{
    ensureUserDirectoriesAndSamples();
    if (m_sideBarBrowser)
        m_sideBarBrowser->refreshBrowsersFromAppState();
    resized();
}

void MainComponent::setupEdit(juce::File editFile)
{
    m_tempDir.createDirectory();

    const bool isNewEdit = (editFile == juce::File());
    const bool isRecoveryEdit = !isNewEdit && editFile.getParentDirectory() == m_tempDir;

    if (!isNewEdit)
    {
        const auto loadStatus = ProjectLifecycle::inspectLoadFile(editFile, isRecoveryEdit);
        if (loadStatus != ProjectLifecycle::LoadFileStatus::valid)
        {
            juce::String reason;
            switch (loadStatus)
            {
            case ProjectLifecycle::LoadFileStatus::missing:
                reason = "The selected project file no longer exists.";
                break;
            case ProjectLifecycle::LoadFileStatus::unsupportedExtension:
                reason = "The selected file is not a supported project file.";
                break;
            case ProjectLifecycle::LoadFileStatus::empty:
                reason = "The selected project file is empty.";
                break;
            case ProjectLifecycle::LoadFileStatus::invalidData:
                reason = "The selected project file is damaged or invalid.";
                break;
            case ProjectLifecycle::LoadFileStatus::valid:
                break;
            }

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Project could not be loaded", reason + "\n\n" + editFile.getFullPathName());
            return;
        }
    }

    if (m_edit && !handleUnsavedEdit())
        return;

    if (isNewEdit)
        editFile = m_tempDir.getNonexistentChildFile("autosave", ".nextTemp", false);

    // Validate and construct the replacement before destroying the current project or its recovery file.
    std::unique_ptr<te::Edit> replacementEdit;
    try
    {
        replacementEdit = isNewEdit ? te::createEmptyEdit(m_engine, editFile) : te::loadEditFromFile(m_engine, editFile);
    }
    catch (const std::exception &e)
    {
        GUIHelpers::log("ERROR: Exception while loading project: " + juce::String(e.what()));
    }

    if (!replacementEdit)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Project could not be loaded", "NextStudio could not read the selected project:\n\n" + editFile.getFullPathName());
        return;
    }

    m_selectionManager.deselectAll();

    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
        uiBehaviour->setFocusedEdit(nullptr);

    if (m_edit)
    {
        m_edit->state.removeListener(this);
        m_edit->getTransport().removeChangeListener(this);
    }

    // Destroy every object that refers to the old Edit before replacing it.
    m_editorContainer = nullptr;
    m_header = nullptr;
    m_editComponent = nullptr;
    m_lowerRange = nullptr;
    m_sideBarBrowser = nullptr;
    m_editViewState = nullptr;
    m_edit = nullptr;

    // A recovery project must keep its source file. For other switches, remove only old
    // recovery snapshots; the replacement Edit may already have created other temp resources.
    if (!isRecoveryEdit)
    {
        const auto oldRecoveryFiles = m_tempDir.findChildFiles(juce::File::findFiles, false, "*.nextTemp");
        for (const auto &oldRecoveryFile : oldRecoveryFiles)
            oldRecoveryFile.deleteFile();
    }
    m_tempDir.createDirectory();

    m_edit = std::move(replacementEdit);

    for (auto *track : te::getAudioTracks(*m_edit))
        if (ClipEditing::hasOverlaps(*track))
            GUIHelpers::log("WARNING: Loaded track contains overlapping clips: " + track->getName() + " (" + ClipEditing::describeOverlaps(*track) + ")");

    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
        uiBehaviour->setFocusedEdit(m_edit.get());

    if (isNewEdit)
        clearAudioTracks();

    m_edit->setTempDirectory(m_tempDir);

    if (auto *w = dynamic_cast<juce::DocumentWindow *>(getParentComponent()))
        w->setName(isNewEdit ? "Untitled" : editFile.getFileNameWithoutExtension());

    m_edit->playInStopEnabled = true;
    m_edit->getTransport().addChangeListener(this);

    createTracksAndAssignInputs();

    if (!editFile.existsAsFile())
        te::EditFileOperations(*m_edit).writeToFile(editFile, true);

    m_editViewState = std::make_unique<EditViewState>(*m_edit, m_selectionManager, m_applicationState);
    m_editComponent = std::make_unique<EditComponent>(*m_edit, *m_editViewState, m_applicationState, m_selectionManager, m_commandManager);
    m_lowerRange = std::make_unique<LowerRangeComponent>(*m_editViewState);
    m_editViewState->setLowerRangeView(LowerRangeView::mixer);

    m_edit->state.addListener(this);

    m_header = std::make_unique<HeaderComponent>(m_editComponent->getEditViewState(), m_applicationState, m_commandManager);
    m_editorContainer = std::make_unique<EditorContainer>(*m_header, *m_editComponent);

    addAndMakeVisible(*m_editorContainer);
    addAndMakeVisible(*m_lowerRange);

    setupSideBrowser();

    addKeyListener(m_commandManager.getKeyMappings());
    resized();

    // Startup housekeeping should not appear in the user's undo history.
    m_edit->getUndoManager().clearUndoHistory();
    m_edit->resetChangedStatus();
}

void MainComponent::saveSettings()
{
    m_applicationState.setBounds(getScreenBounds());
    m_applicationState.saveState();
}

GUIHelpers::ProjectSaveResult MainComponent::saveCurrentProject(bool saveAs)
{
    if (!m_editViewState)
        return GUIHelpers::ProjectSaveResult::failed;

    const auto result = GUIHelpers::saveEdit(*m_editViewState, juce::File(m_applicationState.m_projectsDir.get()), saveAs);
    if (result == GUIHelpers::ProjectSaveResult::saved)
    {
        if (m_editComponent)
            m_editComponent->projectSaved();

        const auto projectFile = m_edit->editFileRetriever ? m_edit->editFileRetriever() : juce::File{};
        if (auto *w = dynamic_cast<juce::DocumentWindow *>(getParentComponent()))
            w->setName(projectFile.getFileNameWithoutExtension());

        if (m_sideBarBrowser)
            m_sideBarBrowser->refreshBrowsersFromAppState();
    }

    return result;
}

bool MainComponent::handleUnsavedEdit()
{
    if (m_edit->hasChangedSinceSaved())
    {
        const auto result = juce::AlertWindow::showYesNoCancelBox(juce::AlertWindow::QuestionIcon, "Unsaved Project", "Do you want to save the project?", "Yes", "No", "Cancel");

        switch (result)
        {
        case 1:
            return ProjectLifecycle::shouldProceedAfterUnsavedChoice(ProjectLifecycle::UnsavedChoice::save, saveCurrentProject());
        case 2:
            return ProjectLifecycle::shouldProceedAfterUnsavedChoice(ProjectLifecycle::UnsavedChoice::discard, GUIHelpers::ProjectSaveResult::cancelled);
        case 3:
        default:
            return ProjectLifecycle::shouldProceedAfterUnsavedChoice(ProjectLifecycle::UnsavedChoice::cancel, GUIHelpers::ProjectSaveResult::cancelled);
        }
    }
    return true;
}

void MainComponent::createTracksAndAssignInputs()
{
    auto &dm = m_engine.getDeviceManager();

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice(i))
            wip->setStereoPair(false);

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice(i))
        {
            wip->setMonitorMode(te::InputDevice::MonitorMode::off);
            wip->setEnabled(false);
        }

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
        if (auto mip = dm.getMidiInDevice(i))
        {
            mip->setMonitorMode(te::InputDevice::MonitorMode::on);
            mip->setEnabled(true);
        }

    m_edit->getTransport().ensureContextAllocated();
    m_edit->restartPlayback();
}
