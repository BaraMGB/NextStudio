
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "Utilities.h"

MainComponent::MainComponent(ApplicationViewState &state)
    : m_applicationState(state)
{
    setWantsKeyboardFocus(true);
    setLookAndFeel(&m_nextLookAndFeel);
    m_nextLookAndFeel.setColour(juce::TooltipWindow::outlineColourId, juce::Colours::white);

    openValidStartEdit();

    addAndMakeVisible(m_editComponent->lowerRange());
    addAndMakeVisible (m_resizerBar);

    m_stretchableManager.setItemLayout (0, -0.05, -0.9, -0.15);
    m_stretchableManager.setItemLayout (1, 10, 10, 10);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
    m_commandManager.registerAllCommandsForTarget(this);
    m_commandManager.registerAllCommandsForTarget(&m_editComponent->getSongEditor());
    m_commandManager.registerAllCommandsForTarget(&m_editComponent->getTrackListView());
    m_commandManager.registerAllCommandsForTarget(&m_editComponent->getPianoRollEditor());
}

MainComponent::~MainComponent()
{
    m_edit->state.removeListener (this);
    saveSettings();
    m_header->removeAllChangeListeners ();
    m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    setLookAndFeel(nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour(0xff555555));
    g.fillRect (getLocalBounds ());
    }

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto header = area.removeFromTop(60);


    m_header->setBounds(header);
    area.removeFromTop(10);
    auto lowerRange = area.removeFromBottom( m_editComponent->getEditViewState().m_isPianoRollVisible
                       ? m_editComponent->getEditViewState().m_midiEditorHeight
            : 250);
    Component* comps[] = {
        m_sideBarBrowser.get()
      , &m_resizerBar
      , m_editComponent.get ()};
    m_stretchableManager.layOutComponents (
                comps
              , 3
              , area.getX()
              , area.getY()
              , area.getWidth()
              , area.getHeight()
              , false, true);
    m_editComponent->lowerRange().setBounds(lowerRange);
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

        EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOff(0,rootNote + gap));
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
        default:
            break;
        }

}

bool MainComponent::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{

    GUIHelpers::log("MainComponent perform");
    int rootNote = 48;
    switch (info.commandID)
    { 
        //send NoteOn
        case KeyPressCommandIDs::midiNoteC :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteCsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteD:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteDsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteE:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteF:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteFsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteG:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteGsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteA:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteAsharp:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteB:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperC :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperCsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperD:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperDsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperE:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperF:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperFsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperG:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperGsharp :
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperA:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperAsharp:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteUpperB:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
            m_pressedKeysForMidiKeyboard.addIfNotAlreadyThere(info.keyPress);
            break;
        case KeyPressCommandIDs::midiNoteTopC:
            EngineHelpers::getVirtuelMidiInputDevice(m_engine).handleIncomingMidiMessage(juce::MidiMessage::noteOn(0, rootNote + info.commandID - 1, .8f));
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
        juce::ValueTree &/* vt */
      , const juce::Identifier &property)
{
    if (property == te::IDs::looping)
        m_header->loopButtonClicked();

    if (property == IDs::pianorollHeight
        || property == IDs::isPianoRollVisible)
            markAndUpdate(m_updateView);

    if (property == te::IDs::source || property == te::IDs::state)
        markAndUpdate(m_updateSource);
   
    if (property == te::IDs::lastSignificantChange)
        markAndUpdate(m_saveTemp);
}
void MainComponent::handleAsyncUpdate()
{
    if (compareAndReset (m_saveTemp)  && !compareAndReset(m_updateSource))
    {
        m_hasUnsavedTemp = true;
    }

    if (compareAndReset(m_updateView))
    {
        resized();
    }
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == m_header.get ())
    {
        if (m_header->getSelectedFile ().exists ())
        {
            auto editfile = m_header->getSelectedFile ();
            setupEdit (editfile);
        }
        else
        {
            m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
            openValidStartEdit();
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
    }
    setupEdit (m_tempDir.getNonexistentChildFile ("untitled"
                                                      , ".nextTemp"
                                                      , false));
    auto atList = te::getTracksOfType<te::AudioTrack>(*m_edit, true);
    for (auto & t : atList)
        m_edit->deleteTrack (t);
}

void MainComponent::setupSideBrowser()
{
    m_sideBarBrowser = std::make_unique<SideBarBrowser>(
                m_applicationState
              , m_editComponent->getEditViewState ());
    addAndMakeVisible (*m_sideBarBrowser);
}

void MainComponent::setupEdit(juce::File editFile)
{
    if(m_edit)
    {
        if(!handleUnsavedEdit ())
            return;
    }
    if (editFile == juce::File())
    {
        juce::FileChooser fc ("New Edit"
                              , juce::File::getSpecialLocation (
                                  juce::File::userDocumentsDirectory)
                              , "*.tracktionedit");
        if (fc.browseForFileToSave (true))
            editFile = fc.getResult();
        else
            return;
    }

    m_selectionManager.deselectAll();
    m_editComponent = nullptr;

    if (editFile.existsAsFile())
        m_edit = te::loadEditFromFile (m_engine, editFile);
    else
        m_edit = te::createEmptyEdit (m_engine, editFile);

    if (auto w = dynamic_cast<juce::DocumentWindow*>(getParentComponent()))
    {
        w->setName(editFile.getFileNameWithoutExtension());
    }
    m_edit->playInStopEnabled = true;
    
    m_edit->setTempDirectory(m_tempDir);

    m_edit->getTransport().addChangeListener (this);

    createTracksAndAssignInputs();

    te::EditFileOperations (*m_edit).writeToFile(editFile, true);

    m_editComponent = std::make_unique<EditComponent> (
                *m_edit
              , m_applicationState
              , m_selectionManager);

    m_edit->state.addListener (this);

    m_header = std::make_unique<HeaderComponent>(m_editComponent->getEditViewState (), m_applicationState, m_commandManager);
    m_header->addChangeListener (this);


    addAndMakeVisible (*m_editComponent);
    addAndMakeVisible(m_editComponent->lowerRange());
    addAndMakeVisible(*m_header);

    setupSideBrowser();

    addKeyListener(m_commandManager.getKeyMappings());
    resized ();
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
            wip->setEndToEnd (true);
            wip->setEnabled (true);
        }

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
        if (auto mip = dm.getMidiInDevice (i))
        {
            mip->setEndToEndEnabled (true);
            mip->setEnabled (true);
        }

    m_edit->getTransport().ensureContextAllocated();
    m_edit->restartPlayback();

}

