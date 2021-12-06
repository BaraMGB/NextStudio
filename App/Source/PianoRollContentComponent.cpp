#include "PianoRollContentComponent.h"
#include "Utilities.h"

PianoRollContentComponent::PianoRollContentComponent(EditViewState & evs
                                   , tracktion_engine::Track::Ptr track)
    : m_editViewState(evs)
    , m_track (track)
{
}

PianoRollContentComponent::~PianoRollContentComponent()
{
}

void PianoRollContentComponent::paint(juce::Graphics &g)
{
    float firstNote = m_editViewState.m_pianoY1;
    //float noteHeight = m_keyWidth * 7 / 12;
    auto noteHeight = (double) m_editViewState.m_pianorollNoteWidth;
    //draw horizontal Lines
    float lastNote = ((float) getHeight () / noteHeight) + firstNote;
    auto area = getLocalBounds ();
    auto firstNoteHeight = noteHeight - (noteHeight * (firstNote - (int) firstNote));
    auto firstNoteRect = area.removeFromBottom (firstNoteHeight);
    g.setColour (juce::MidiMessage::isMidiNoteBlack (firstNote)
            ? juce::Colour(0x11ffffff)
            : juce::Colour(0x22ffffff));
    g.fillRect (firstNoteRect.reduced(0,1));

    for (auto i = (int) firstNote + 1; i <= lastNote; i++)
    {
        g.setColour (juce::MidiMessage::isMidiNoteBlack (i)
                ? juce::Colour(0x11ffffff)
                : juce::Colour(0x22ffffff));
        g.fillRect (area.removeFromBottom (noteHeight).reduced (0, 1));
    }

    g.setColour (juce::Colours::black);
    drawVerticalLines (g);

    
        for (auto & midiClip : getMidiClipsOfTrack())
        {
            
                auto& seq = midiClip->getSequence();

                //draw Notes
                for (auto n : seq.getNotes())
                {
                    auto yOffset = n->getNoteNumber () - firstNote + 1;
                    auto noteY = getHeight () - (yOffset * noteHeight);
                    double sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
                    double eBeat = n->getEndBeat() - midiClip->getOffsetInBeats();

                    auto x1 = m_editViewState.beatsToX (sBeat + midiClip->getStartBeat (), getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
                    auto x2 = m_editViewState.beatsToX (eBeat + midiClip->getStartBeat (), getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);

                    if ( sBeat < 0 || eBeat > midiClip->getEndBeat ()
                                              - midiClip->getStartBeat ())
                    {
                        g.setColour (juce::Colours::grey);
                    }
                    else if (n->getColour () == 127)
                    {
                        g.setColour (juce::Colours::green);
                    }
                    else
                    {
                        g.setColour (juce::Colours::white);
                    }
                    juce::Rectangle<float> noteRect
                                (float (x1)     , float (noteY)
                               , float (x2 - x1), float (noteHeight));
                    g.fillRect (noteRect.reduced (1,1));
                }
                //draw ClipRange
                auto clipStartX = m_editViewState.beatsToX (midiClip->getStartBeat (), getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
                auto clipLengthX = m_editViewState.beatsToX (midiClip->getEndBeat (), getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
                g.setColour (midiClip->getColour ());
                g.drawRect (clipStartX  , 0, clipLengthX - clipStartX, getHeight ());
                g.setColour (midiClip->getColour ().withAlpha (0.2f));
                g.fillRect (clipStartX  , 0, clipLengthX - clipStartX, getHeight ());
                  //visualize clip Offset
//                auto clipOffset = beatsToX (midiClip->getStartBeat () - midiClip->getOffsetInBeats ());
//                auto clipEndOffsetX = beatsToX (midiClip->getStartBeat ()
//                                                - midiClip->getOffsetInBeats ()
//                                                + seq.getLastBeatNumber ());
//                clipEndOffsetX = juce::jmax(clipEndOffsetX, clipLengthX);

//                g.setColour (midiClip->getTrack ()->getColour ());
//                g.fillRect (clipOffset, 0, clipEndOffsetX - clipOffset, 30);
            }
}

void PianoRollContentComponent::mouseDown(const juce::MouseEvent &e)
{
    m_noteAdding = false;
    m_clickedNote = getNoteByPos (e.position);
    m_clickedPos = e.position;
    auto clickedBeat = m_editViewState.xToBeats (e.position.x, getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
    auto clickedClip = getMidiclipByPos (e.x);
    if (m_clickedNote && clickedClip)
    {
        if (e.mods.isRightButtonDown ())
        {
            if (clickedClip->getSequence ()
                    .getNotes ().contains (m_clickedNote))
            {
                clickedClip->getSequence ()
                        .removeNote (*m_clickedNote
                                     , &m_editViewState
                                     .m_edit.getUndoManager ());
                repaint();
            }
        }
        else
        {
            m_clickOffsetBeats = m_clickedNote->getStartBeat () - clickedBeat;
            clickedClip->getAudioTrack ()->playGuideNote (
                        m_clickedNote->getNoteNumber ()
                      , clickedClip->getMidiChannel ()
                      , 127
                      , false
                      , true);
        }
    }
    else if (clickedClip
             && (e.getNumberOfClicks () > 1
                 || e.mods.isShiftDown ()))
    {
        auto beat = clickedBeat
                - clickedClip->getStartBeat ()
                + clickedClip->getOffsetInBeats ();

        auto snapType = m_editViewState.getBestSnapType (
                    m_editViewState.m_pianoX1
                  , m_editViewState.m_pianoX2
                  , getWidth ());
        beat = m_editViewState.getSnapedBeat (beat, snapType, true);
        m_clickedNote = clickedClip->getSequence ().addNote
                (getNoteNumber (e.position.y)
                 , beat
                 , m_editViewState.m_lastNoteLenght == 0
                    ? 0.25 : m_editViewState.m_lastNoteLenght
                 , 127
                 , 111
                 , &m_editViewState.m_edit.getUndoManager ());
        clickedClip->getAudioTrack ()->playGuideNote (
                    m_clickedNote->getNoteNumber ()
                  , clickedClip->getMidiChannel ()
                  , 127
                  , false
                  , true);
        m_clickOffsetBeats = m_clickedNote->getStartBeat () - clickedBeat;
        repaint();
        m_noteAdding = true;
    }

}

void PianoRollContentComponent::mouseDrag(const juce::MouseEvent &e)
{
    if(e.mods.isLeftButtonDown () && e.mouseWasDraggedSinceMouseDown ())
    {
        auto um = &m_editViewState.m_edit.getUndoManager ();
        auto clickedClip = getMidiclipByPos (m_clickedPos.x);
        if (m_clickedNote && clickedClip)
        {

            if (m_expandLeft)
            {
                auto oldEndBeat = m_clickedNote->getEndBeat ();
                auto newRawStartBeat = m_editViewState.xToBeats (e.position.x, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2)
                        + m_clickOffsetBeats;
                auto snapType = m_editViewState.getBestSnapType (
                            m_editViewState.m_pianoX1
                          , m_editViewState.m_pianoX2
                          , getWidth ());
                auto snapedStart = m_editViewState.getSnapedBeat (
                            newRawStartBeat
                          , snapType);
                auto newStart = e.mods.isShiftDown ()
                              ? newRawStartBeat
                              : snapedStart;
                m_clickedNote->setStartAndLength (newStart
                                                  , oldEndBeat - newStart
                                                  , um);
            }
            else if (m_expandRight || m_noteAdding)
            {
                auto oldStart = m_clickedNote->getStartBeat ();
                auto newEnd = m_editViewState.xToBeats (e.position.x, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2)
                            + clickedClip->getOffsetInBeats ()
                            - clickedClip->getStartBeat ()
                            - oldStart;
                auto snapType = m_editViewState.getBestSnapType (
                            m_editViewState.m_pianoX1
                          , m_editViewState.m_pianoX2
                          , getWidth ());
                auto snapedEnd = m_editViewState.getSnapedBeat (newEnd, snapType);
                m_clickedNote->setStartAndLength (oldStart
                                                  , e.mods.isShiftDown ()
                                                  ? newEnd
                                                  : snapedEnd
                                                  , um);
                m_editViewState.m_lastNoteLenght = e.mods.isShiftDown ()
                        ? newEnd
                        : snapedEnd;
            }
            else
            {
                auto length = m_clickedNote->getLengthBeats ();
                auto newBeat = m_editViewState.xToBeats (e.position.x, getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2) + m_clickOffsetBeats;
                auto snapType = m_editViewState.getBestSnapType (
                            m_editViewState.m_pianoX1
                          , m_editViewState.m_pianoX2
                          , getWidth ());
                auto snapedBeat = m_editViewState.getSnapedBeat (newBeat, snapType);
                m_clickedNote->setStartAndLength (e.mods.isShiftDown ()
                                                  ? newBeat
                                                  : snapedBeat
                                                    , length
                                                    , um);
                if (m_clickedNote->getNoteNumber () != getNoteNumber (e.position.y))
                {
                    clickedClip->getAudioTrack ()->turnOffGuideNotes ();
                    m_clickedNote->setNoteNumber (getNoteNumber (e.position.y), um);
                    clickedClip->getAudioTrack ()->playGuideNote (
                                m_clickedNote->getNoteNumber ()
                              , clickedClip->getMidiChannel ()
                              , 127
                              , false
                              , true);
                }
                //update displayed Notenumber under cursor
                getParentComponent ()->mouseMove (e);
            }
        }

        repaint ();
    }
}

void PianoRollContentComponent::mouseMove(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);

    if (auto mc = getMidiclipByPos (e.position.x))
    {
        for (auto n : mc->getSequence ().getNotes ())
        {
            n->setColour (111, &m_editViewState.m_edit.getUndoManager ());
        }
        if (auto note = getNoteByPos (e.position))
        {
            auto startX = m_editViewState.beatsToX (note->getStartBeat ()
                                             + mc->getStartBeat ()
                                             - mc->getOffsetInBeats ()
                                               , getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
            auto endX = m_editViewState.beatsToX (note->getEndBeat ()
                                             + mc->getStartBeat ()
                                             - mc->getOffsetInBeats ()
                                             , getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
            note->setColour (127, &m_editViewState.m_edit.getUndoManager ());
            if (e.position.x < startX + 10)
            {
                setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
                m_expandLeft = true;
                m_expandRight = false;
            }
            else if (e.position.x > endX - 10)
            {
                setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
                m_expandLeft = false;
                m_expandRight = true;
            }
            else
            {
                m_expandLeft = false;
                m_expandRight = false;
            }
        }
    }
    getParentComponent ()->mouseMove (e);
    repaint ();
}

void PianoRollContentComponent::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void PianoRollContentComponent::mouseUp(const juce::MouseEvent &)
{
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
    {
        at->turnOffGuideNotes ();
    }
}

void PianoRollContentComponent::mouseWheelMove(const juce::MouseEvent &event
                                     , const juce::MouseWheelDetails &wheel)
{
    if (event.mods.isShiftDown ())
    {
        auto deltaX1 = event.mods.isCtrlDown () ? wheel.deltaY : -wheel.deltaY;
        auto deltaX2 = -wheel.deltaY;

        m_editViewState.m_pianoX1 =  juce::jmax(0.0
                                                , m_editViewState.m_pianoX1
                                                    + deltaX1);
        m_editViewState.m_pianoX2 =  m_editViewState.m_pianoX2 + deltaX2;
    }
//    else if (event.mods.isCtrlDown ())
//    {
//        auto deltaY1 = wheel.deltaY >= 0 ? 1 : -1;
//        m_editViewState.m_pianorollNoteWidth =
//                juce::jlimit(1.0
//                           , 30.0
//                           , m_editViewState.m_pianorollNoteWidth - deltaY1);

//    }
    else
    {
        double deltaY1 = wheel.deltaY >= 0 ? 3 : -3;
        m_editViewState.m_pianoY1 =
                juce::jlimit(0.0
                           , 127.0 - (getHeight ()
                                / m_editViewState.m_pianorollNoteWidth)
                           , (double) m_editViewState.m_pianoY1 + deltaY1);
    }
}

std::vector<te::MidiClip*> PianoRollContentComponent::getMidiClipsOfTrack()
{
    std::vector<te::MidiClip*> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
    {
        for (auto c : at->getClips ())
        {
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
            {
                midiClips.push_back (mc);
            }
        }
    }
    return midiClips;
}

void PianoRollContentComponent::drawVerticalLines(juce::Graphics &g)
{
    double x1 = m_editViewState.m_pianoX1;
    double x2 = m_editViewState.m_pianoX2;
    GUIHelpers::drawBarsAndBeatLines (g, m_editViewState, x1, x2, getBounds ());
}

int PianoRollContentComponent::getNoteNumber(int y)
{
    double noteHeight = m_keyWidth * 7 / 12;
    double noteNumb = m_editViewState.m_pianoY1 + ((getHeight () - y )/ noteHeight);
    return noteNumb;
}

tracktion_engine::MidiNote *PianoRollContentComponent::getNoteByPos(juce::Point<float> pos)
{
    for (auto& mc : getMidiClipsOfTrack ())
    {
        for (auto note : mc->getSequence ().getNotes ())
        {
            if (note->getNoteNumber () == getNoteNumber (pos.y))
            {
                auto clickedBeat = m_editViewState.xToBeats (pos.x, getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2)
                        + mc->getOffsetInBeats ();
                auto clipstart = mc->getStartBeat ();
                if ( clickedBeat > note->getStartBeat () + clipstart
                 && clickedBeat < note->getEndBeat () + clipstart)
                {
                    return note;
                }
            }
        }
    }
    return nullptr;
}

tracktion_engine::MidiClip *PianoRollContentComponent::getMidiclipByPos(int y)
{
    for (auto & clip : getMidiClipsOfTrack ())
    {
        if (clip->getStartBeat () < m_editViewState.xToBeats (y, getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2)
        &&  clip->getEndBeat () > m_editViewState.xToBeats (y, getWidth (), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2))
        {
            return clip;
        }
    }
    return nullptr;
}

void PianoRollContentComponent::setKeyWidth(float noteHeight)
{
    m_keyWidth = noteHeight;
    repaint ();
}

tracktion_engine::Track::Ptr PianoRollContentComponent::getTrack()
{
    return m_track;
}

