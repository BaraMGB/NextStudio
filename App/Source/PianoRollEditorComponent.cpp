#include "PianoRollEditorComponent.h"

PianoRollEditorComponent::PianoRollEditorComponent(EditViewState & evs
                                                   , te::MidiClip& clip)
    : m_editViewState(evs)
    , m_clip(clip)
    , m_keyboard (getMidiClip ()->getAudioTrack ()
                  ->getMidiInputDevice ().keyboardState
                  , juce::MidiKeyboardComponent::
                    Orientation::verticalKeyboardFacingRight)
{
    getMidiClip ()->getAudioTrack ()
            ->getMidiInputDevice ().keyboardState.addListener (this);
    m_editViewState.m_state.addListener (this);
    addAndMakeVisible (m_keyboard);
    m_keyboard.setBlackNoteWidthProportion (0.5);
    m_keyboard.setBlackNoteLengthProportion (0.6);
    m_keyboard.setScrollButtonsVisible (false);
    m_keyboard.addChangeListener (this);


}

PianoRollEditorComponent::~PianoRollEditorComponent()
{
    getMidiClip ()->getAudioTrack ()
            ->getMidiInputDevice ().keyboardState.removeListener (this);
    m_keyboard.removeChangeListener (this);
}

void PianoRollEditorComponent::paint(juce::Graphics &g)
{
    int y1 = m_editViewState.m_pianoY1;
    int y2 = m_editViewState.m_pianoY2;

    float noteHeight = m_keyboard.getKeyWidth () * 7 / 12;
    float line = getHeight ();
    for (auto i = y1; i <= y2 ; i++)
    {
        line = line - noteHeight  ;
        if (juce::MidiMessage::isMidiNoteBlack (i))
        {
           g.setColour (juce::Colour(0x11ffffff));
        }
        else
        {
            g.setColour (juce::Colour(0x22ffffff));
        }
        juce::Rectangle<float> lineRect = {0.0, line, (float) getWidth (), noteHeight};
        g.fillRect(lineRect.reduced (0, 1));
    }


    if (auto mc = getMidiClip ())
    {
        auto viewWidthBeats = m_editViewState.m_pianoX2
                       - m_editViewState.m_pianoX1;
        auto& seq = mc->getSequence();
        for (auto n : seq.getNotes())
        {
            auto yOffset = n->getNoteNumber () - y1 + 1;
            auto noteY = getHeight () - (yOffset * noteHeight);
            double sBeat = n->getStartBeat() - mc->getOffsetInBeats();
            double eBeat = n->getEndBeat() - mc->getOffsetInBeats();
            if (auto p = getParentComponent())
            {
                auto x1 =  (sBeat - m_editViewState.m_pianoX1) * getWidth()
                           / viewWidthBeats;
                auto x2 =  (eBeat - m_editViewState.m_pianoX1) * getWidth()
                           / viewWidthBeats;

                g.setColour (juce::Colours::white);
                g.fillRect (float (x1), float (noteY), float (x2 - x1), float (noteHeight));
            }
        }

        auto clipStartX = (0 - m_editViewState.m_pianoX1) * getWidth()
                / viewWidthBeats;
        auto clipEndX = (mc->getLengthInBeats () - m_editViewState.m_pianoX1) * getWidth()
                / viewWidthBeats;

        g.setColour (mc->getTrack ()->getColour ().withAlpha (0.2f));
        g.fillRect (clipStartX  , 0, clipEndX - clipStartX, getHeight ());
    }



}

void PianoRollEditorComponent::mouseDown(const juce::MouseEvent &)
{

}

void PianoRollEditorComponent::mouseDrag(const juce::MouseEvent &)
{

}

void PianoRollEditorComponent::mouseUp(const juce::MouseEvent &)
{

}

void PianoRollEditorComponent::mouseWheelMove(const juce::MouseEvent &event
                                     , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown ())
    {
        auto deltaX1 = event.mods.isCtrlDown () ? wheel.deltaY : -wheel.deltaY;
        auto deltaX2 = -wheel.deltaY;

        m_editViewState.m_pianoX1 =  m_editViewState.m_pianoX1 + deltaX1;
        m_editViewState.m_pianoX2 =  m_editViewState.m_pianoX2 + deltaX2;
    }
    else
    {
        auto deltaY1 = wheel.deltaY >= 0 ? 1 : -1;
        auto deltaY2 = event.mods.isCtrlDown () ? -deltaY1 : deltaY1;

        m_editViewState.m_pianoY1 = juce::jlimit(0
                                                 ,127
                                                 , m_editViewState.m_pianoY1
                                                 + deltaY1);
        m_editViewState.m_pianoY2 = juce::jlimit(m_editViewState.m_pianoY1 + 7
                                                 , 127
                                                 , m_editViewState.m_pianoY2
                                                 + deltaY2);
    }

}

void PianoRollEditorComponent::resized()
{
    double firstVisibleNote = m_editViewState.m_pianoY1;
    double lastVisibleNote  = m_editViewState.m_pianoY2;
    double pianoRollNoteWidth = getHeight () / (lastVisibleNote - firstVisibleNote);

    m_keyboard.setKeyWidth (pianoRollNoteWidth * 12 / 7);
    m_keyboard.setBounds (0
                          , getHeight () - m_keyboard.getTotalKeyboardWidth ()
                            + (firstVisibleNote * pianoRollNoteWidth)
                          , 50
                          , m_keyboard.getTotalKeyboardWidth ());

}

void PianoRollEditorComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{

}

void PianoRollEditorComponent::handleNoteOn(juce::MidiKeyboardState *
                                            , int /*midiChannel*/
                                            , int midiNoteNumber
                                            , float v)
{
    auto midichannel = getMidiClip ()->getMidiChannel ();
    getMidiClip ()->getAudioTrack ()->playGuideNote
            (midiNoteNumber,midichannel, 127.0 * v, false, true);
}

void PianoRollEditorComponent::handleNoteOff(juce::MidiKeyboardState *
                                             , int /*midiChannel*/
                                             , int /*midiNoteNumber*/
                                             , float)
{
    getMidiClip ()->getAudioTrack ()->turnOffGuideNotes ();
}

void PianoRollEditorComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (v.hasType (IDs::EDITVIEWSTATE))
    {
        if (i == IDs::pianoX1
            || i == IDs::pianoX2
            || i == IDs::pianoY1
            || i == IDs::pianoY2)
        {
            resized ();
            repaint ();
            //markAndUpdate (updatePositions);
        }
    }
}

void PianoRollEditorComponent::showContextMenu()
{

}
