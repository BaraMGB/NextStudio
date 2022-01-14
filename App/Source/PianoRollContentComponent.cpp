#include "PianoRollContentComponent.h"
#include "Utilities.h"

PianoRollContentComponent::PianoRollContentComponent(EditViewState & evs
                                   , tracktion_engine::Track::Ptr track)
    : m_editViewState(evs)
    , m_track (std::move(track))
{
}

PianoRollContentComponent::~PianoRollContentComponent()
= default;

void PianoRollContentComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds ();

    drawKeyLines(g, area);
    drawBarsAndBeatLines(g, juce::Colours::black);

    for (auto & midiClip : getMidiClipsOfTrack())
    {
        drawClipRange(g, midiClip);

        auto& seq = midiClip->getSequence();
        for (auto n : seq.getNotes())
        {
            drawNote(g, midiClip, n);
        }
    }
}
void PianoRollContentComponent::drawNote(juce::Graphics& g,
                                         tracktion_engine::MidiClip* const& midiClip,
                                         const tracktion_engine::MidiNote* n) const
{
    auto noteRect = getNoteRect(midiClip, n);
    auto clipCol = getNoteColour(midiClip, n);

    g.setColour(juce::Colours::black);
    g.fillRect (noteRect);

    g.setColour(clipCol.brighter(0.5f));
    noteRect.reduce(1, 1);
    g.fillRect(noteRect);

    g.setColour(clipCol);
    noteRect.reduce(1, 1);
    g.fillRect (noteRect);

    if (m_editViewState.m_pianoKeyWidth > 13)
    {
        g.setColour(juce::Colours::black);
        g.drawText(juce::MidiMessage::getMidiNoteName(n->getNoteNumber(),true, true, 3)
            , noteRect, juce::Justification::centredLeft);
    }
}
juce::Colour PianoRollContentComponent::getNoteColour(
    tracktion_engine::MidiClip* const& midiClip,
    const tracktion_engine::MidiNote* n) const
{
    auto s = getNoteStartBeat(midiClip, n);
    auto e = getNoteEndBeat(midiClip, n);

    if (isBeforeClipStart(s) || isAfterClipEnd(midiClip, e))
        return juce::Colours::grey;
    else if (n->getColour () == 127)
        return juce::Colours::white;

    return m_track->getColour().darker(1.f - getVelocity(n));
}

float PianoRollContentComponent::getVelocity(
    const tracktion_engine::MidiNote* note)
{
    return juce::jmap((float)note->getVelocity()
                        , 0.f, 127.f, 0.f, 1.f);
}

bool PianoRollContentComponent::isAfterClipEnd(
    tracktion_engine::MidiClip* const& midiClip, double beats)
{
    bool endAfterClipEnd = beats > midiClip->getEndBeat () - midiClip->getStartBeat ();
    return endAfterClipEnd;
}

bool PianoRollContentComponent::isBeforeClipStart(double beats)
{
    bool startBeforeClipStart = beats < 0;
    return startBeforeClipStart;
}

juce::Rectangle<float> PianoRollContentComponent::getNoteRect(
    te::MidiClip* const& midiClip, const te::MidiNote* n) const
{
    double sBeat = getNoteStartBeat(midiClip, n);
    double eBeat = getNoteEndBeat(midiClip, n);
    auto x1 = m_editViewState.beatsToX (sBeat + midiClip->getStartBeat (),
                                       getWidth(),
                                       m_editViewState.m_pianoX1,
                                       m_editViewState.m_pianoX2);
    auto x2 = m_editViewState.beatsToX (eBeat + midiClip->getStartBeat (),
                                       getWidth(),
                                       m_editViewState.m_pianoX1,
                                       m_editViewState.m_pianoX2) + 1;

    return getNoteRect(n->getNoteNumber(), x1, x2);
}

juce::Rectangle<float> PianoRollContentComponent::getNoteRect(const int noteNum,
                                                              int x1,
                                                              int x2) const
{
    auto yOffset = (float) noteNum - getStartKey() + 1;
    auto noteY = (float) getHeight() - (yOffset * getKeyWidth());
    return {
        float (x1), float (noteY)
                       , float (x2 - x1), float (getKeyWidth())};
}

double PianoRollContentComponent::getNoteEndBeat(te::MidiClip* const& midiClip,
                                                 const te::MidiNote* n)
{
    auto eBeat= n->getEndBeat() - midiClip->getOffsetInBeats();
    return eBeat;
}

double PianoRollContentComponent::getNoteStartBeat(te::MidiClip* const& midiClip,
                                                   const te::MidiNote* n)
{
    auto sBeat= n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat;
}

void PianoRollContentComponent::drawClipRange(
    juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip) const
{
    auto clipStartX = m_editViewState.beatsToX (midiClip->getStartBeat (),
                                               getWidth(),
                                               m_editViewState.m_pianoX1,
                                               m_editViewState.m_pianoX2) + 1;
    auto clipEndX = m_editViewState.beatsToX (midiClip->getEndBeat (),
                                                getWidth(),
                                                m_editViewState.m_pianoX1,
                                                m_editViewState.m_pianoX2);
    g.setColour (midiClip->getColour ());
    g.drawLine(clipStartX, 0, clipStartX, getHeight());
    g.drawLine(clipEndX, 0, clipEndX, getHeight());
    g.setColour (midiClip->getColour ().withAlpha (0.2f));
    g.fillRect (clipStartX, 0, clipEndX - clipStartX, getHeight());
}

void PianoRollContentComponent::drawKeyLines(juce::Graphics& g,
                                              juce::Rectangle<int>& area) const
{
    auto lastNote= ((float) getHeight () / getKeyWidth()) + getStartKey();

    for (auto i = (int) getStartKey() ; i <= (int) lastNote; i++)
    {
        g.setColour (juce::MidiMessage::isMidiNoteBlack (i)
                ? juce::Colour(0x11ffffff)
                : juce::Colour(0x22ffffff));
        g.fillRect (getNoteRect(i, 0, getWidth()).reduced(0, 1));
    }
}

float PianoRollContentComponent::getKeyWidth() const
{
    return  (float) m_editViewState.m_pianoKeyWidth;
}

float PianoRollContentComponent::getStartKey() const
{
    return (float) m_editViewState.m_pianoStartKey;
}

void PianoRollContentComponent::mouseDown(const juce::MouseEvent &e)
{
    m_noteAdding = false;
    m_clickedNote = getNoteByPos (e.position);
    m_clickedPos = e.position;
    auto clickedBeat = xToBeats (e.x);
    auto clickedClip = getMidiClipByPos(e.x);
    if (m_clickedNote && clickedClip)
    {
        if (e.mods.isRightButtonDown ())
        {
            removeNote(clickedClip, m_clickedNote);
        }
        else
        {
            m_clickOffsetBeats = m_clickedNote->getStartBeat () - clickedBeat;
            playNote(clickedClip, m_clickedNote);
        }
    }
    else if (clickedClip && (e.getNumberOfClicks () > 1 || e.mods.isShiftDown ()))
    {
        auto beat = clickedBeat
                - clickedClip->getStartBeat ()
                + clickedClip->getOffsetInBeats ();

        beat = getQuantizedBeat(beat);
        std::cout << getNoteNumber(e.y) << std::endl;
        m_clickedNote = addNote (getNoteNumber(e.y), clickedClip, beat);
        playNote(clickedClip, m_clickedNote);
        m_clickOffsetBeats = m_clickedNote->getStartBeat () - clickedBeat;

        m_noteAdding = true;
    }

}
te::MidiNote* PianoRollContentComponent::addNote(int noteNumb,
                                                 const te::MidiClip* clip,
                                                 double beat)
{
    return clip->getSequence ().addNote(
        noteNumb
        , beat
        , m_editViewState.m_lastNoteLength == 0
            ? 0.25
            : m_editViewState.m_lastNoteLength
        , m_editViewState.m_lastVelocity
        , 111
        , &m_editViewState.m_edit.getUndoManager ());
}
void PianoRollContentComponent::playNote(const te::MidiClip* clip,
                                         te::MidiNote* note) const
{
    clip->getAudioTrack ()->playGuideNote (
        note->getNoteNumber ()
        , clip->getMidiChannel ()
        , 127
        , false
        , true);
}
void PianoRollContentComponent::removeNote(te::MidiClip* clip,
                                           te::MidiNote* note)
{
    if (clipContains(clip, note))
        clip->getSequence().removeNote(*m_clickedNote,
                                       &m_editViewState.m_edit.getUndoManager());
}

void PianoRollContentComponent::mouseDrag(const juce::MouseEvent &e)
{
    if(e.mods.isLeftButtonDown () && e.mouseWasDraggedSinceMouseDown ())
    {
        auto um = &m_editViewState.m_edit.getUndoManager ();
        auto clickedClip = getMidiClipByPos((int) m_clickedPos.x);
        if (m_clickedNote && clickedClip)
        {
            if (m_expandLeft)
            {
                expandClickedNoteLeft(e.x, e.mods.isShiftDown());
            }
            else if (m_expandRight || m_noteAdding)
            {
                auto oldStart = m_clickedNote->getStartBeat ();
                auto newEnd = xToBeats (e.x)
                            + clickedClip->getOffsetInBeats ()
                            - clickedClip->getStartBeat ()
                            - oldStart;

                m_clickedNote->setStartAndLength (oldStart
                                                  , e.mods.isShiftDown ()
                                                  ? newEnd
                                                  : getQuantizedBeat(newEnd)
                                                  , um);
                m_editViewState.m_lastNoteLength = e.mods.isShiftDown ()
                        ? newEnd
                        : getQuantizedBeat(newEnd);
            }
            else
            {
                auto length = m_clickedNote->getLengthBeats ();
                auto newBeat = xToBeats (e.x) + m_clickOffsetBeats;

                m_clickedNote->setStartAndLength (e.mods.isShiftDown ()
                                                  ? newBeat
                                                  : getQuantizedBeat(newBeat)
                                                    , length
                                                    , um);
                if (m_clickedNote->getNoteNumber () != getNoteNumber (e.y))
                {
                    clickedClip->getAudioTrack ()->turnOffGuideNotes ();
                    m_clickedNote->setNoteNumber (getNoteNumber (e.y), um);
                    clickedClip->getAudioTrack ()->playGuideNote (
                                m_clickedNote->getNoteNumber ()
                              , clickedClip->getMidiChannel ()
                              , 127
                              , false
                              , true);
                }
                //update displayed note number under cursor
                //getParentComponent ()->mouseMove (e);
            }
        }
        repaint ();
    }
}
void PianoRollContentComponent::expandClickedNoteLeft(int targetX, bool snap)
{
    auto oldEndBeat = m_clickedNote->getEndBeat ();
    auto newRawStartBeat = xToBeats(targetX) + m_clickOffsetBeats;

    auto newStart = snap ? newRawStartBeat
                                          : getQuantizedBeat(newRawStartBeat);
    m_clickedNote->setStartAndLength (newStart
                                     , oldEndBeat - newStart
                                     , &m_editViewState.m_edit.getUndoManager ());
}

double PianoRollContentComponent::getQuantizedBeat(double beat) const
{
    auto snapedBeat = m_editViewState.getQuantizedBeat(beat, getBestSnapType());
    return snapedBeat;
}

te::TimecodeSnapType PianoRollContentComponent::getBestSnapType() const
{
    auto snapType = m_editViewState.getBestSnapType (
        m_editViewState.m_pianoX1
              , m_editViewState.m_pianoX2
              , getWidth());
    return snapType;
}

double PianoRollContentComponent::xToBeats(const int& x) const
{
    return m_editViewState.xToBeats (x, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}

void PianoRollContentComponent::mouseMove(const juce::MouseEvent &e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);

    if (auto mc = getMidiClipByPos(e.x))
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
            if (e.x < startX + 10)
            {
                setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
                m_expandLeft = true;
                m_expandRight = false;
            }
            else if (e.x > endX - 10)
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
    //repaint ();
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
    else
    {
        scrollPianoRoll((float) wheel.deltaY * 5 );
    }
}

void PianoRollContentComponent::scrollPianoRoll(float delta)
{
    m_editViewState.m_pianoStartKey =
            juce::jlimit(0.f
                       , 127.f - (float) (getHeight() / m_editViewState.m_pianoKeyWidth)
                       , (float) m_editViewState.m_pianoStartKey + delta);
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

void PianoRollContentComponent::drawBarsAndBeatLines(juce::Graphics &g, juce::Colour colour)
{
    g.setColour(colour);
    double x1 = m_editViewState.m_pianoX1;
    double x2 = m_editViewState.m_pianoX2;
    GUIHelpers::drawBarsAndBeatLines (g, m_editViewState, x1, x2, getBounds ());
}

int PianoRollContentComponent::getNoteNumber(int y)
{
    auto noteHeight = (double) m_editViewState.m_pianoKeyWidth;
    auto noteNumb = (int)(m_editViewState.m_pianoStartKey
                           + ((double)(getHeight () - y) / noteHeight));
    return noteNumb;
}

tracktion_engine::MidiNote* PianoRollContentComponent::getNoteByPos(juce::Point<float> pos)
{
    for (auto& mc : getMidiClipsOfTrack ())
    {
        for (auto note : mc->getSequence ().getNotes ())
        {
            if (note->getNoteNumber () == getNoteNumber ((int) pos.y))
            {
                auto clickedBeat = xToBeats ((int) pos.x)
                        + mc->getOffsetInBeats ();
                auto clipStart = mc->getStartBeat ();
                if ( clickedBeat > note->getStartBeat () + clipStart
                    && clickedBeat < note->getEndBeat () + clipStart)
                {
                    return note;
                }
            }
        }
    }
    return nullptr;
}

tracktion_engine::MidiClip* PianoRollContentComponent::getMidiClipByPos(int y)
{
    for (auto & clip : getMidiClipsOfTrack ())
    {
        if ((clip->getStartBeat () < xToBeats (y))
        &&  (clip->getEndBeat () > xToBeats (y)))
        {
            return clip;
        }
    }
    return nullptr;
}

tracktion_engine::Track::Ptr PianoRollContentComponent::getTrack()
{
    return m_track;
}
bool PianoRollContentComponent::clipContains(const te::MidiClip* clip
                                             , te::MidiNote* note)
{
    return clip->getSequence ().getNotes ().contains (note);
}
