#include "PianoRollEditorComponent.h"



//------------------------------------------------------------------------------

PianoRollComponent::PianoRollComponent(EditViewState & evs)
    : m_editViewState(evs)
    , m_keyboard (m_keybordstate
                  , juce::MidiKeyboardComponent::
                    Orientation::verticalKeyboardFacingRight)
    , m_timeline (evs, evs.m_pianoX1, evs.m_pianoX2)    
    , m_playhead (evs.m_edit, evs, evs.m_pianoX1, evs.m_pianoX2)
{
    m_editViewState.m_edit.state.addListener (this);

    m_keyboard.setBlackNoteWidthProportion (0.5);
    m_keyboard.setBlackNoteLengthProportion (0.6);
    m_keyboard.setScrollButtonsVisible (false);

    addAndMakeVisible (m_keyboard);
    addAndMakeVisible (m_timeline);
    addAndMakeVisible (m_playhead);
    m_playhead.setAlwaysOnTop (true);
}

PianoRollComponent::~PianoRollComponent()
{
    m_editViewState.m_edit.state.removeListener (this);
}

void PianoRollComponent::focusLost(juce::Component::FocusChangeType cause)
{
}

void PianoRollComponent::focusGained(juce::Component::FocusChangeType cause)
{
}

void PianoRollComponent::resized()
{
    auto area = getLocalBounds ();
    auto keyboard = area.removeFromLeft (50);
    auto timeline = area.removeFromTop (20);

    double firstVisibleNote = m_editViewState.m_pianoY1;
    double lastVisibleNote  = m_editViewState.m_pianoY2;
    double pianoRollNoteWidth = getHeight () / (lastVisibleNote - firstVisibleNote);

    m_keyboard.setKeyWidth (juce::jmax(0.1, pianoRollNoteWidth * 12 / 7));
    m_keyboard.setBounds (0
                          , getHeight () - m_keyboard.getTotalKeyboardWidth ()
                            + (firstVisibleNote * pianoRollNoteWidth)
                          , keyboard.getWidth ()
                          , m_keyboard.getTotalKeyboardWidth ());
    m_timeline.setBounds (timeline);
    if (m_pianoRollClip != nullptr)
    {
        m_pianoRollClip->setBounds (area);
        m_pianoRollClip->setKeyWidth (m_keyboard.getKeyWidth ());
        std::cout << "gesehen?" << std::endl;
    }

    m_playhead.setBounds (area);
}

void PianoRollComponent::valueTreePropertyChanged(juce::ValueTree &v
                                                  , const juce::Identifier &i)
{
    if (v.hasType (tracktion_engine::IDs::MIDICLIP))
    {
        resized ();
        repaint ();
    }
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::pianoX1
            || i == IDs::pianoX2
            || i == IDs::pianoY1
            || i == IDs::pianoY2)
        {
            resized ();
            repaint ();
        }
    }
}

void PianoRollComponent::handleNoteOn(juce::MidiKeyboardState *
                                      , int /*midiChannel*/
                                      , int midiNoteNumber
                                      , float v)
{
    if (m_pianoRollClip)
    {
        auto mc = m_pianoRollClip->getMidiClip ();
        auto midichannel = mc->getMidiChannel ();
        mc->getAudioTrack ()->playGuideNote
                          (midiNoteNumber,midichannel, 127.0 * v, false, true);
    }

}

void PianoRollComponent::handleNoteOff(juce::MidiKeyboardState *
                                       , int /*midiChannel*/
                                       , int /*midiNoteNumber*/
                                       , float)
{
    if (m_pianoRollClip)
    {
        auto mc = m_pianoRollClip->getMidiClip ();
        mc->getAudioTrack ()->turnOffGuideNotes ();
    }
}

void PianoRollComponent::centerView()
{
    //center view of clip in horizontal
    if (m_pianoRollClip)
    {
        auto mc = m_pianoRollClip->getMidiClip ();
        auto width = m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1;
        m_editViewState.m_pianoX1 = juce::jmax(0.0, mc->getStartBeat () - 1);
        m_editViewState.m_pianoX2 = m_editViewState.m_pianoX1 + width;
    }
    //in vertical
//    auto noteRange = getMidiClip ()->getSequence ().getNoteNumberRange ()
//            .expanded (10);
//    m_editViewState.m_pianoY1 = juce::jmax(0, noteRange.getStart ());
//    m_editViewState.m_pianoY2 = juce::jmin(127, noteRange.getEnd ());
}

juce::MidiKeyboardComponent &PianoRollComponent::getKeyboard()
{
    return m_keyboard;
}

