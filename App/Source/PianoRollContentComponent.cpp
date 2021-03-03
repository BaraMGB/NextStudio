#include "PianoRollContentComponent.h"

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
    int firstNote = m_editViewState.m_pianoY1;
    float noteHeight = m_keyWidth * 7 / 12;

    //draw horizontal Lines

    float line = getHeight ();
    int lastNote = (getHeight () / noteHeight) + firstNote;

    for (auto i = firstNote; i <= lastNote; i++)
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

                    auto x1 = beatsToX (sBeat + midiClip->getStartBeat ());
                    auto x2 = beatsToX (eBeat + midiClip->getStartBeat ());

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
                auto clipStartX = beatsToX (midiClip->getStartBeat ());
                auto clipLengthX = beatsToX (midiClip->getEndBeat ());
                g.setColour (midiClip->getTrack ()->getColour ());
                g.drawRect (clipStartX  , 0, clipLengthX - clipStartX, getHeight ());
                g.setColour (midiClip->getTrack ()->getColour ().withAlpha (0.2f));
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
    auto clickedBeat = xToBeats (e.position.x);
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
            m_clickOffset = m_clickedNote->getStartBeat () - clickedBeat;
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

        m_editViewState.m_snapType = 7;
        beat = m_editViewState.getSnapedBeat (beat, true);
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
        m_clickOffset = m_clickedNote->getStartBeat () - clickedBeat;
        repaint();
        m_noteAdding = true;
    }

}

void PianoRollContentComponent::mouseDrag(const juce::MouseEvent &e)
{
    if(e.mods.isLeftButtonDown ())
    {
        auto um = &m_editViewState.m_edit.getUndoManager ();
        auto clickedClip = getMidiclipByPos (m_clickedPos.x);
        if (m_clickedNote && clickedClip)
        {

            if (m_expandLeft)
            {
                auto oldEndBeat = m_clickedNote->getEndBeat ();
                auto newRawStartBeat = xToBeats (e.position.x)
                        + m_clickOffset;
                auto snapedStart = m_editViewState.getSnapedBeat (newRawStartBeat);
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
                auto newEnd = xToBeats (e.position.x)
                            + clickedClip->getOffsetInBeats ()
                            - clickedClip->getStartBeat ()
                            - oldStart;
                auto snapedEnd = m_editViewState.getSnapedBeat (newEnd);
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
                auto newBeat = xToBeats (e.position.x) + m_clickOffset;
                auto snapedBeat = m_editViewState.getSnapedBeat (newBeat);
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
            auto startX = beatsToX (note->getStartBeat ()
                                             + mc->getStartBeat ()
                                             - mc->getOffsetInBeats ()
                                               );
            auto endX = beatsToX (note->getEndBeat ()
                                             + mc->getStartBeat ()
                                             - mc->getOffsetInBeats ()
                                             );
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
//    for (auto &mc : getMidiClipsOfTrack ())
//    {

//    }
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
        auto deltaY1 = wheel.deltaY >= 0 ? 1 : -1;
        m_editViewState.m_pianoY1 =
                juce::jlimit(0
                           , 127 - (int) (getHeight ()
                                / m_editViewState.m_pianorollNoteWidth)
                           , m_editViewState.m_pianoY1 + deltaY1);
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
    double zoom = x2 - x1;
    int firstBeat = static_cast<int>(x1);
    if(beatsToX(firstBeat) < 0)
    {
        firstBeat++;
    }
    auto pixelPerBeat = getWidth() / zoom;
    for (int beat = firstBeat - 1; beat <= x2; beat++)
    {
        const int BeatX = beatsToX(beat) - 1;
        auto zBars = 16;
        if (zoom < 240)
        {
            zBars /= 2;
        }
        if (zoom < 120)
        {
            zBars /=2;
        }
        if (beat % zBars == 0)
        {
            g.drawLine(BeatX, 0, BeatX, getHeight());
        }
        if (zoom < 60)
        {
            g.drawLine(BeatX,0, BeatX, getHeight());
        }
        if (zoom < 25)
        {
            auto quarterBeat = pixelPerBeat / 4;
            auto i = 1;
            while ( i < 5)
            {
                g.drawLine(BeatX + quarterBeat * i ,0,
                           BeatX + quarterBeat * i ,getHeight());
                i++;
            }
        }
    }
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
                auto clickedBeat = xToBeats (pos.x)
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
        if (clip->getStartBeat () < xToBeats (y)
        &&  clip->getEndBeat () > xToBeats (y))
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

int PianoRollContentComponent::beatsToX(double beats)
{
    return juce::roundToInt (((beats - m_editViewState.m_pianoX1)
                              *  getWidth())
                             / (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1));
}

double PianoRollContentComponent::xToBeats(int x)
{
    return (double (x) / getWidth())
            * (m_editViewState.m_pianoX2 - m_editViewState.m_pianoX1)
            + m_editViewState.m_pianoX1;
}


