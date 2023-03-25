/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

MainComponent::MainComponent(ApplicationViewState &state)
    : m_applicationState(state)
{
    setLookAndFeel(&m_nextLookAndFeel);

    openValidStartEdit();

    addAndMakeVisible(m_editComponent->lowerRange());
    addAndMakeVisible (m_resizerBar);

    m_stretchableManager.setItemLayout (0, -0.05, -0.9, -0.15);
    m_stretchableManager.setItemLayout (1, 10, 10, 10);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
    m_commandManager.registerAllCommandsForTarget(this);
    juce::Timer::callAfterDelay (300, [this] { grabKeyboardFocus(); }); // ensure that key presses are sent to the KeyPressTarget object
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

// bool MainComponent::keyPressed(const juce::KeyPress &key)
// {
//     auto scaleFactor = 0.2;
//
//     GUIHelpers::log (key.getTextDescription ());
//     if (key == juce::KeyPress::numberPadAdd)
//     {
//         auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
//         auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
//         x1 = x1 - scaleFactor;
//         x2 = x2 + scaleFactor;
//
//         return true;
//     }
//
//     if (key == juce::KeyPress::numberPadSubtract)
//     {
//         auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
//         auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
//         x1 = x1 + scaleFactor;
//         x2 = x2 - scaleFactor;
//
//         return true;
//     }
//
//     if (key == juce::KeyPress::rightKey)
//     {
//         auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
//         auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
//         x2 = x2 + (scaleFactor/5);
//         x1 = x1 + (scaleFactor/5);
//
//         return true;
//     }
//
//     if (key == juce::KeyPress::leftKey)
//     {
//         auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
//         auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
//         x2 = x2 - (scaleFactor/5);
//         x1 = x1 - (scaleFactor/5);
//
//         return true;
//     }
//
//     if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
//     {
//         if (m_editComponent->getSongEditor().getTracksWithSelectedTimeRange().size() > 0)
//             m_editComponent->getSongEditor().deleteSelectedTimeRange();
//         else 
//             EngineHelpers::deleteSelectedClips (m_editComponent->getEditViewState ());
//
//         return true;
//     }
//
//     if (key == juce::KeyPress::returnKey)
//     {
//         EngineHelpers::play(m_editComponent->getEditViewState ());
//     }
//
//     if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::numberPad0)
//     {
//         EngineHelpers::togglePlay(m_editComponent->getEditViewState ());
//         return true;
//     }
//     if (key == juce::KeyPress::numberPadDecimalPoint)
//     {
//         EngineHelpers::stopPlay(m_editComponent->getEditViewState ());
//         return true;
//     }
//
//     if (key == juce::KeyPress::F10Key)
//     {
//         std::cout << "DEBUG EDIT: " << juce::Time::getCurrentTime().toString(true, true, true, true) << std::endl;
//         std::cout << "=================================================================================" << std::endl;
//         auto editString = m_edit->state.toXmlString();
//         std::cout << editString << std::endl;
//         return true;
//     }
// #if JUCE_MAC
//     if (key == juce::KeyPress::createFromDescription ("command + L"))
// #else
//     if (key == juce::KeyPress::createFromDescription ("ctrl + l"))
// #endif
//         m_editComponent->loopAroundSelection();
//
//     if (key == juce::KeyPress::createFromDescription("l"))
//         EngineHelpers::toggleLoop(*m_edit);
//
// #if JUCE_MAC
//     if (key == juce::KeyPress::createFromDescription ("command + D"))
// #else
//     if (key == juce::KeyPress::createFromDescription ("ctrl + d"))
// #endif
//     {
//         m_editComponent->getSongEditor().duplicateSelectedClipsOrTimeRange();
//         return true;
//     }
//
// #if JUCE_MAC
//     if (key == juce::KeyPress::createFromDescription ("command + z"))
// #else
//     if (key == juce::KeyPress::createFromDescription ("ctrl + z"))
// #endif
//     {
//         m_editComponent->getEditViewState ().m_edit.undo ();
//         return true;
//     }
//
// #if JUCE_MAC
//     if (key == juce::KeyPress::createFromDescription ("shift + cmd + z"))
// #else
//     if (key == juce::KeyPress::createFromDescription ("shift + ctrl + z"))
// #endif
//     {
//         m_editComponent->getEditViewState ().m_edit.redo ();
//         return true;
//     }
//     return true;
// }

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
            if (command == KeyPressCommandIDs::midiNoteC)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteCsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteD)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteDsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteE)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteF)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteFsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteG)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteGsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteA)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteAsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteB)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperC)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperCsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperD)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperDsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperE)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperF)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperFsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperG)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperGsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperA)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperAsharp)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteUpperB)
                gap = (int) command - 1;
            else if (command == KeyPressCommandIDs::midiNoteTopC)
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
            KeyPressCommandIDs::midiNoteTopC
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
        default:
            break;
        }

}

bool MainComponent::perform (const juce::ApplicationCommandTarget::InvocationInfo& info) 
{

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

