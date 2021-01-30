#include "PianoRollClipComponent.h"

PianoRollClipComponent::PianoRollClipComponent(EditViewState & evs
                                   , tracktion_engine::Clip::Ptr clip)
    : m_editViewState(evs)
    , m_clip(clip)
{
}

PianoRollClipComponent::~PianoRollClipComponent()
{


}

void PianoRollClipComponent::paint(juce::Graphics &g)
{
    int y1 = m_editViewState.m_pianoY1;
    int y2 = m_editViewState.m_pianoY2;

    //draw horizontal Lines
    float line = getHeight ();
    for (auto i = y1; i <= y2 ; i++)
    {
        line = line - m_keyWidth  ;
        if (juce::MidiMessage::isMidiNoteBlack (i))
        {
           g.setColour (juce::Colour(0x11ffffff));
        }
        else
        {
            g.setColour (juce::Colour(0x22ffffff));
        }
        juce::Rectangle<float> lineRect = {0.0, line, (float) getWidth (), m_keyWidth};
        g.fillRect(lineRect.reduced (0, 1));
    }
    g.setColour (juce::Colours::black);
    drawVerticalLines (g);
    if (auto mc = getMidiClip () && getMidiClip ()->getAudioTrack () != nullptr)
    {
        for (auto & trackClip : getMidiClip ()->getAudioTrack ()->getClips ())
        {
            if (auto midiClip = dynamic_cast<te::MidiClip*>(trackClip))
            {
                auto& seq = midiClip->getSequence();
                //draw Notes
                for (auto n : seq.getNotes())
                {
                    auto yOffset = n->getNoteNumber () - y1 + 1;
                    auto noteY = getHeight () - (yOffset * m_keyWidth);
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
                               , float (x2 - x1), float (m_keyWidth));
                    g.fillRect (noteRect.reduced (1,1));
                }
                //draw ClipRange
                auto clipStartX = beatsToX (midiClip->getStartBeat ());
                auto clipEndX = beatsToX (midiClip->getEndBeat ());
                g.setColour (midiClip->getTrack ()->getColour ());
                g.drawRect (clipStartX  , 0, clipEndX - clipStartX, getHeight ());
                g.setColour (midiClip->getTrack ()->getColour ().withAlpha (0.2f));
                g.fillRect (clipStartX  , 0, clipEndX - clipStartX, getHeight ());
            }
        }
    }
}

void PianoRollClipComponent::mouseDown(const juce::MouseEvent &e)
{
    m_clickedNote = getNoteByPos (e.position);
    auto clickedBeat = xToBeats (e.position.x);

    if (!e.mouseWasDraggedSinceMouseDown ())
    {

        if (m_clickedNote)
        {
            if (e.mods.isRightButtonDown ())
            {
                getMidiClip ()->getSequence ()
                        .removeNote (*m_clickedNote
                                     , &m_editViewState.m_edit.getUndoManager ());
                repaint ();
            }
            else
            {
                m_clickOffset = m_clickedNote->getStartBeat () - clickedBeat;
            }
        }
        else if (e.getNumberOfClicks () > 1 || e.mods.isShiftDown ())
        {



            auto beat = clickedBeat
                      - getMidiClip ()->getStartBeat ()
                      + getMidiClip ()->getOffsetInBeats ();

            m_editViewState.m_snapType = 7;
            beat = m_editViewState.getSnapedBeat (beat, true);
            getMidiClip ()->getSequence ().addNote
                    (getNoteNumber (e.position.y)
                     , beat
                     , 0.25
                     , 127
                     , 111
                     , &m_editViewState.m_edit.getUndoManager ());
            repaint();
        }
    }
}

void PianoRollClipComponent::mouseDrag(const juce::MouseEvent &e)
{
    if(e.mods.isLeftButtonDown ())
    {
        auto um = &m_editViewState.m_edit.getUndoManager ();
        if (m_clickedNote != nullptr)
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
            else if (m_expandRight)
            {
                auto oldStart = m_clickedNote->getStartBeat ();
                auto newEnd = xToBeats (e.position.x)
                            + getMidiClip ()->getOffsetInBeats ()
                            - getMidiClip ()->getStartBeat ()
                            - oldStart;
                auto snapedEnd = m_editViewState.getSnapedBeat (newEnd);
                m_clickedNote->setStartAndLength (oldStart
                                                  , e.mods.isShiftDown ()
                                                  ? newEnd
                                                  : snapedEnd
                                                  , um);
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
                m_clickedNote->setNoteNumber (getNoteNumber (e.position.y), um);
            }
        }
        repaint ();
    }
}

void PianoRollClipComponent::mouseMove(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    for (auto n : getMidiClip ()->getSequence ().getNotes ())
    {
        n->setColour (111, &m_editViewState.m_edit.getUndoManager ());
    }
    if (auto note = getNoteByPos (e.position))
    {
        auto startX = beatsToX (note->getStartBeat ()
                                           + getMidiClip ()->getStartBeat ()
                                         - getMidiClip ()->getOffsetInBeats ()
                                           );
        auto endX = beatsToX (note->getEndBeat ()
                                           + getMidiClip ()->getStartBeat ()
                                         - getMidiClip ()->getOffsetInBeats ()
                                         );
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
        note->setColour (127, &m_editViewState.m_edit.getUndoManager ());
    }
    repaint ();
}

void PianoRollClipComponent::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void PianoRollClipComponent::mouseUp(const juce::MouseEvent &)
{
}

void PianoRollClipComponent::mouseWheelMove(const juce::MouseEvent &event
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

void PianoRollClipComponent::drawVerticalLines(juce::Graphics &g)
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

int PianoRollClipComponent::getNoteNumber(int y)
{
    double noteHeight = m_keyWidth * 7 / 12;
    double noteNumb = m_editViewState.m_pianoY1 + ((getHeight () - y )/ noteHeight);
    return noteNumb;
}

tracktion_engine::MidiNote *PianoRollClipComponent::getNoteByPos(juce::Point<float> pos)
{
    for (auto note : getMidiClip ()->getSequence ().getNotes ())
    {
        if (note->getNoteNumber () == getNoteNumber (pos.y))
        {
            auto clickedBeat = xToBeats (pos.x)
                    + getMidiClip ()->getOffsetInBeats ();
            auto clipstart = getMidiClip ()->getStartBeat ();
            if ( clickedBeat > note->getStartBeat () + clipstart
             && clickedBeat < note->getEndBeat () + clipstart)
            {
                return note;
            }
        }
    }
    return nullptr;
}

void PianoRollClipComponent::setKeyWidth(float noteHeight)
{
    m_keyWidth = noteHeight;
    repaint ();
}


