
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
#include "ProjectsBrowser.h"
#include "SidebarComponent.h"
#include "Utilities.h"

MainComponent::MainComponent(ApplicationViewState &state)
    : m_applicationState(state)
    , m_nextLookAndFeel(state)
    , m_sidebarSplitter(false)
{

    float scale = m_applicationState.m_appScale;
    scale = juce::jlimit(0.2f, 3.f, scale);
    juce::Desktop::getInstance().setGlobalScaleFactor(scale);

    setWantsKeyboardFocus(true);
    juce::LookAndFeel::setDefaultLookAndFeel(&m_nextLookAndFeel);
    updateTheme();

    addAndMakeVisible(m_sidebarSplitter);
    m_sidebarSplitter.onMouseDown = [this] ()
    {
        m_sidebarWidthAtMousedown = m_applicationState.m_sidebarWidth;
    };

    m_sidebarSplitter.onDrag = [this] (int dragDistance)
    {
        if (m_applicationState.m_sidebarCollapsed == false)
        {
            m_applicationState.m_sidebarWidth = juce::jmax(m_applicationState.m_minSidebarWidth , m_sidebarWidthAtMousedown + dragDistance);
            resized();
        }
    };

    openValidStartEdit();

    m_commandManager.registerAllCommandsForTarget(this);
    m_commandManager.registerAllCommandsForTarget(m_editComponent.get());
    m_commandManager.registerAllCommandsForTarget(&m_editComponent->getTrackListView());
    m_commandManager.registerAllCommandsForTarget(&m_lowerRange->getPianoRollEditor());

    m_selectionManager.addChangeListener(this);
    m_applicationState.m_applicationStateValueTree.addListener(this);
}

MainComponent::~MainComponent()
{
    m_applicationState.m_applicationStateValueTree.removeListener(this);
    m_selectionManager.removeChangeListener(this);
    if (m_edit)
        m_edit->state.removeListener (this);

    if (m_header)
        m_header->removeAllChangeListeners();

    // Explicitly destroy UI components and then the Edit to ensure correct shutdown order
    m_editorContainer = nullptr;
    m_header = nullptr;
    m_editComponent = nullptr;
    m_lowerRange = nullptr;
    m_sideBarBrowser = nullptr;
    m_editViewState = nullptr;
    m_edit = nullptr;

    saveSettings();
    m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    setLookAndFeel(nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.setColour (m_applicationState.getMainFrameColour());
    g.fillRect (getLocalBounds ());
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto lowerRange = area.removeFromBottom( m_editComponent->getEditViewState().m_isPianoRollVisible
                       ? m_editComponent->getEditViewState().m_midiEditorHeight
            : 250);
    m_sideBarBrowser->setBounds(area.removeFromLeft(m_applicationState.m_sidebarWidth));
    m_sidebarSplitter.setBounds(area.removeFromLeft(10));
    m_editorContainer->setBounds(area);
    m_lowerRange->setBounds(lowerRange);
}

bool MainComponent::keyStateChanged(bool isKeyDown)  

{
    int rootNote = 48;
    int gap = 0;

    for (auto kp : m_pressedKeysForMidiKeyboard)
        if (!kp.isCurrentlyDown())
        {
            m_pressedKeysForMidiKeyboard.removeFirstMatchingValue(kp);
            //send noteOff
            auto command = m_commandManager.getKeyMappings()->findCommandForKeyPress(kp);
            if (command >= KeyPressCommandIDs::midiNoteC && command <= KeyPressCommandIDs::midiNoteTopC)
                gap = (int) command - 1;

            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(m_editViewState->m_edit))
                EngineHelpers::getVirtuelMidiInputDevice(*m_edit)->handleIncomingMidiMessage(juce::MidiMessage::noteOff(1,rootNote + gap),0);
        }
    return true;
}

void MainComponent::getAllCommands (juce::Array<juce::CommandID>& commands) 
{
    juce::Array<juce::CommandID> ids {
            KeyPressCommandIDs::midiNoteC,
            KeyPressCommandIDs::midiNoteCsharp ,
            KeyPressCommandIDs::midiNoteD,
            KeyPressCommandIDs::midiNoteDsharp ,
            KeyPressCommandIDs::midiNoteE,
            KeyPressCommandIDs::midiNoteF,
            KeyPressCommandIDs::midiNoteFsharp ,
            KeyPressCommandIDs::midiNoteG,
            KeyPressCommandIDs::midiNoteGsharp ,
            KeyPressCommandIDs::midiNoteA,
            KeyPressCommandIDs::midiNoteAsharp ,
            KeyPressCommandIDs::midiNoteB,
            KeyPressCommandIDs::midiNoteUpperC,
            KeyPressCommandIDs::midiNoteUpperCsharp ,
            KeyPressCommandIDs::midiNoteUpperD,
            KeyPressCommandIDs::midiNoteUpperDsharp ,
            KeyPressCommandIDs::midiNoteUpperE,
            KeyPressCommandIDs::midiNoteUpperF,
            KeyPressCommandIDs::midiNoteUpperFsharp ,
            KeyPressCommandIDs::midiNoteUpperG,
            KeyPressCommandIDs::midiNoteUpperGsharp ,
            KeyPressCommandIDs::midiNoteUpperA,
            KeyPressCommandIDs::midiNoteUpperAsharp ,
            KeyPressCommandIDs::midiNoteUpperB,
            KeyPressCommandIDs::midiNoteTopC,

            KeyPressCommandIDs::togglePlay,
            KeyPressCommandIDs::toggleRecord,
            KeyPressCommandIDs::play,
            KeyPressCommandIDs::stop,

            KeyPressCommandIDs::loopAroundSelection,
            // KeyPressCommandIDs::loopOn,
            // KeyPressCommandIDs::loopOff,
            KeyPressCommandIDs::loopAroundAll,
            KeyPressCommandIDs::loopToggle,

            // KeyPressCommandIDs::toggleSnap,
            KeyPressCommandIDs::toggleMetronome,
            // KeyPressCommandIDs::snapToBar,
            // KeyPressCommandIDs::snapToBeat,
            // KeyPressCommandIDs::snapToGrid,
            // KeyPressCommandIDs::snapToTime,
            // KeyPressCommandIDs::snapToOff,

            KeyPressCommandIDs::undo,
            KeyPressCommandIDs::redo,

            KeyPressCommandIDs::debugOutputEdit
        };

    commands.addArray(ids);
}

void MainComponent::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) 
{
    switch (commandID)
    { 
case KeyPressCommandIDs::midiNoteC :
            result.setInfo("note C", "set MIDI note C", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("y").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteCsharp :
            result.setInfo("note C#", "set MIDI note C#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("s").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteD:
            result.setInfo("note D", "set MIDI note D", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("x").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteDsharp :
            result.setInfo("note D#", "set MIDI note D#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("d").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteE:
            result.setInfo("note E", "set MIDI note E", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("c").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteF :
            result.setInfo("note F", "set MIDI note F", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("v").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteFsharp :
            result.setInfo("note F#", "set MIDI note F#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("g").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteG:
            result.setInfo("note G", "set MIDI note G", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("b").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteGsharp :
            result.setInfo("note G#", "set MIDI note G#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("h").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteA:
            result.setInfo("note A", "set MIDI note A", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("n").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteAsharp:
            result.setInfo("note A#", "set MIDI note A#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("j").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteB:
            result.setInfo("note B", "set MIDI note B", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("m").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperC :
            result.setInfo("noteUpper C", "set MIDI noteUpper C", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("q").getKeyCode() , 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription(",").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperCsharp :
            result.setInfo("noteUpper C#", "set MIDI noteUpper C#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("2").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperD:
            result.setInfo("noteUpper D", "set MIDI noteUpper D", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("w").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperDsharp :
            result.setInfo("noteUpper D#", "set MIDI noteUpper D#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("3").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperE:
            result.setInfo("noteUpper E", "set MIDI noteUpper E", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("e").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperF :
            result.setInfo("noteUpper F", "set MIDI noteUpper F", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("r").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperFsharp :
            result.setInfo("noteUpper F#", "set MIDI noteUpper F#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("5").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperG:
            result.setInfo("noteUpper G", "set MIDI noteUpper G", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("t").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperGsharp :
            result.setInfo("noteUpper G#", "set MIDI noteUpper G#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("6").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperA:
            result.setInfo("noteUpper A", "set MIDI noteUpper A", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperAsharp:
            result.setInfo("noteUpper A#", "set MIDI noteUpper A#", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("7").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteUpperB:
            result.setInfo("noteUpper B", "set MIDI noteUpper B", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("u").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::midiNoteTopC :
            result.setInfo("noteUpper C", "set MIDI noteUpper C", "virtual Midi keyboard", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("i").getKeyCode() , 0);
            break;
        case KeyPressCommandIDs::togglePlay :
            result.setInfo("Play/Pause", "Toggle play", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::spaceKey , 0);
            result.addDefaultKeypress(juce::KeyPress::numberPad0 , 0);
            break;
        case KeyPressCommandIDs::play :
            result.setInfo("Play", "Play", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::returnKey, 0);
            break;
        case KeyPressCommandIDs::toggleRecord :
            result.setInfo("Record", "Record", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::numberPadMultiply , 0);
            break;
        case KeyPressCommandIDs::stop :
            result.setInfo("Stop", "Stop", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::numberPadDecimalPoint , 0);
            break;
        case KeyPressCommandIDs::loopToggle :
            result.setInfo("Loop", "Loop", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode() , juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::loopAroundAll :
            result.setInfo("Loop around all", "Loop around all", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode() , juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier | juce::ModifierKeys::altModifier);
            break;
        case KeyPressCommandIDs::toggleMetronome :
            result.setInfo("Metronome", "Metronome", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("m").getKeyCode() , juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::loopAroundSelection :
            result.setInfo("Loop around selection", "Loop around selection", "Selection", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode() , juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case KeyPressCommandIDs::debugOutputEdit :
            result.setInfo("Debug output edit", "Debug output edit", "Debug", 0);
            result.addDefaultKeypress(juce::KeyPress::F10Key, 0);
            break;
        case KeyPressCommandIDs::undo :
            result.setInfo("Undo last action","Undo", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), juce::ModifierKeys::commandModifier);
            break;
        case KeyPressCommandIDs::redo :
            result.setInfo("Redo last action","Redo", "Song Editor", 0);
            result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        default:
            break;
        }

}

bool MainComponent::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{

    GUIHelpers::log("MainComponent::perform() (ApplicationCommandTarget)");
    int rootNote = 48;
    switch (info.commandID)
    { 
        //send NoteOn
        case KeyPressCommandIDs::midiNoteC :

            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
            {
                GUIHelpers::log("NAME: ", virMidiIn->getName());
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
                m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            }
            break;
        case KeyPressCommandIDs::midiNoteCsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteD:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteDsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteE:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteF:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteFsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteG:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteGsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteA:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteAsharp:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteB:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperC :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperCsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperD:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperDsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperE:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperF:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperFsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperG:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperGsharp :
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperA:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperAsharp:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperB:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteTopC:
            if (auto virMidiIn = EngineHelpers::getVirtuelMidiInputDevice(*m_edit))
                virMidiIn->handleIncomingMidiMessage(juce::MidiMessage::noteOn(1, rootNote + info.commandID - 1, .8f),0);
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::togglePlay:
            EngineHelpers::togglePlay(m_editComponent->getEditViewState());
            break;
        case KeyPressCommandIDs::play:
            EngineHelpers::play(m_editComponent->getEditViewState());
            break;
        case KeyPressCommandIDs::stop:
            EngineHelpers::stopPlay(m_editComponent->getEditViewState());
            break;
        case KeyPressCommandIDs::toggleRecord:
            std::cout << "toggleRecord" << std::endl;
            EngineHelpers::toggleRecord(m_editComponent->getEditViewState().m_edit);
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
            std::cout << "DEBUG EDIT: " << juce::Time::getCurrentTime().toString(true, true, true, true) << std::endl;
            std::cout << "=================================================================================" << std::endl;
            auto editString = m_edit->state.toXmlString();
            std::cout << editString << std::endl;
            
            break;
        }
        
        case KeyPressCommandIDs::undo:
            m_edit->undo();
            break;
        case KeyPressCommandIDs::redo:
            m_edit->redo();
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

void MainComponent::valueTreePropertyChanged(
        juce::ValueTree & vt
      , const juce::Identifier &property)
{
    if (property == te::IDs::looping)
        m_header->loopButtonClicked();

    if (property == IDs::pianorollHeight
        || property == IDs::isPianoRollVisible)
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
    if (compareAndReset (m_saveTemp)  && !compareAndReset(m_updateSource))
        m_hasUnsavedTemp = true;

    if (compareAndReset(m_updateView))
        resized();

    if (compareAndReset(m_updateTheme))
        updateTheme();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (auto pro = dynamic_cast<BrowserBaseComponent*>(source))
    {
        if (pro->m_projectToLoad.exists ())
        {
            auto editfile = pro->m_projectToLoad;
            setupEdit (editfile);
        }
        else
        {
            setupEdit (juce::File());
            clearAudioTracks();
        }
    }

    if (source == &m_selectionManager && m_editViewState)
    {
        if (m_editViewState->m_applicationState.m_exclusiveMidiFocusEnabled)
            EngineHelpers::updateMidiInputTargets(*m_editViewState);
    }
}

void MainComponent::openValidStartEdit()
{
    m_tempDir = m_engine.getTemporaryFileManager().getTempDirectory();
    m_tempDir.createDirectory();

    auto f = Helpers::findRecentEdit(m_tempDir);
    if (f.existsAsFile())
    {
        GUIHelpers::log("MainComponent: found Temp file:" + f.getFullPathName());
        auto result = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                                                         "Restore crashed project?",
                                                         "It seems, NextStudio is crashed last time. Do you want to restore the last session?",
                                                         "Yes",
                                                         "No");
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

    setupEdit (juce::File());
}

void MainComponent::setupSideBrowser()
{
    m_sideBarBrowser = std::make_unique<SidebarComponent>(*m_editViewState, m_commandManager);
    addAndMakeVisible (*m_sideBarBrowser);
    m_sideBarBrowser->updateParentsListener();
}

void MainComponent::setupEdit(juce::File editFile)
{
     if (m_edit)
    {
        if (!handleUnsavedEdit())
            return;
    }
    if (m_tempDir.exists() && (editFile.getParentDirectory() != m_tempDir))
        m_tempDir.deleteRecursively();

    m_tempDir.createDirectory();

    const bool isNewEdit = (editFile == juce::File());

    if (isNewEdit)
        editFile = m_tempDir.getNonexistentChildFile("autosave", ".nextTemp", false);

    m_lowerRange = nullptr;
    m_selectionManager.deselectAll();
    m_editComponent = nullptr;

    if (editFile.existsAsFile())
        m_edit = te::loadEditFromFile(m_engine, editFile);
    else
        m_edit = te::createEmptyEdit(m_engine, editFile);

    if (isNewEdit)
        clearAudioTracks();

    m_edit->setTempDirectory(m_tempDir);

    if (auto w = dynamic_cast<juce::DocumentWindow*>(getParentComponent()))
    {
        w->setName(editFile.getFileNameWithoutExtension());
    }

    m_edit->playInStopEnabled = true;

    m_edit->getTransport().addChangeListener(this);

    createTracksAndAssignInputs();

    if (!editFile.existsAsFile())
        te::EditFileOperations(*m_edit).writeToFile(editFile, true);

    m_editViewState = std::make_unique<EditViewState>(*m_edit, m_selectionManager, m_applicationState);
    m_editComponent = std::make_unique<EditComponent>(*m_edit, *m_editViewState, m_applicationState, m_selectionManager, m_commandManager);
    m_lowerRange = std::make_unique<LowerRangeComponent>(*m_editViewState);

    m_edit->state.addListener(this);

    m_header = std::make_unique<HeaderComponent>(m_editComponent->getEditViewState(), m_applicationState, m_commandManager);
    m_editorContainer = std::make_unique<EditorContainer>(*m_header, *m_editComponent);

    addAndMakeVisible(*m_editorContainer);
    addAndMakeVisible(*m_lowerRange);

    setupSideBrowser();

    addKeyListener(m_commandManager.getKeyMappings());
    resized();
}

void MainComponent::saveSettings()
{
    m_applicationState.setBounds(getScreenBounds ());
    m_applicationState.saveState ();
}

bool MainComponent::handleUnsavedEdit()
{
    if (m_edit->hasChangedSinceSaved ())
    {
        auto result = juce::AlertWindow::showYesNoCancelBox (
                    juce::AlertWindow::QuestionIcon
                    , "Unsaved Project"
                    , "Do you want to save the project?"
                    , "Yes"
                    , "No"
                    , "Cancel");
        switch (result) {
        case 1 :
            GUIHelpers::saveEdit (m_editComponent->getEditViewState ()
                                  , juce::File::createFileWithoutCheckingPath (
                                      m_applicationState.m_workDir));
            return true;
        case 2 :
            return true;
        case 3 :
            //cancel
        default:
            return false;
        }
    }
    return true;
}
 
void MainComponent::createTracksAndAssignInputs()
{
    auto& dm = m_engine.getDeviceManager();

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice (i))
            wip->setStereoPair (false);

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice (i))
        {
            wip->setMonitorMode(te::InputDevice::MonitorMode::off);
            wip->setEnabled (false);
        }

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
        if (auto mip = dm.getMidiInDevice (i))
        {
            mip->setMonitorMode (te::InputDevice::MonitorMode::on);
            mip->setEnabled (true);
        }

    m_edit->getTransport().ensureContextAllocated();
    m_edit->restartPlayback();

}

