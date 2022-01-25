#include "PianoRollContentComponent.h"
#include "Utilities.h"

PianoRollContentComponent::PianoRollContentComponent(
    EditViewState& evs, tracktion_engine::Track::Ptr track)
    : m_editViewState(evs)
    , m_track(std::move(track))
    , m_lassoTool(evs, evs.m_pianoX1, evs.m_pianoX2)

{
    addChildComponent(m_lassoTool);
    m_selectedEvents = std::make_unique<te::SelectedMidiEvents>(getMidiClipsOfTrack());
}

PianoRollContentComponent::~PianoRollContentComponent() = default;

void PianoRollContentComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();

    drawKeyLines(g, area);
    drawBarsAndBeatLines(g, juce::Colours::black);

    for (auto& midiClip: getMidiClipsOfTrack())
    {
        drawClipRange(g, midiClip);

        auto& seq = midiClip->getSequence();

        for (auto n: seq.getNotes())
        {
            drawNote(g, midiClip, n);
        }
    }

    for (auto sn : m_selectedEvents->getSelectedNotes())
    {
        drawNote(g
                 , m_selectedEvents->clipForEvent(sn)
                 , sn
                 , m_draggedTimeDelta
                 , m_draggedNoteDelta
                 , m_leftTimeDelta
                 , m_rightTimeDelta);
    }
}

void PianoRollContentComponent::resized()
{
    auto area = getLocalBounds();
    m_lassoTool.setBounds(area);
}
void PianoRollContentComponent::drawNote(juce::Graphics& g,
                                         tracktion_engine::MidiClip* const& midiClip,
                                         tracktion_engine::MidiNote* n,
                                         double timeDelta,
                                         int noteDelta,
                                         double timeLeftDelta,
                                         double timeRightDelta
                                         )
{
    auto noteRect = getNoteRect(midiClip, n);
    auto clipCol = getNoteColour(midiClip, n);
    auto scroll = beatsToX(0) * (-1);
    if (timeDelta != 0.0 || noteDelta != 0)
    {

        auto newX = scroll + noteRect.getX() + m_editViewState.timeToX(timeDelta,getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
        auto newY = getYfromKey(getKeyFromY(noteRect.getY()) + noteDelta);

        noteRect.setPosition(newX, newY);

        clipCol = midiClip->getTrack()->getColour().withAlpha(0.2f);

        g.setColour(clipCol);
        noteRect.reduce(1, 1);
        g.fillRect(noteRect);
    }
    else if (timeLeftDelta != 0.0 || timeRightDelta != 0.0)
    {
        noteRect.setLeft(scroll + noteRect.getX() + timeToX(timeLeftDelta));
        noteRect.setRight(scroll + noteRect.getRight() + timeToX(timeRightDelta));
        clipCol = midiClip->getTrack()->getColour().withAlpha(0.7f);

        g.setColour(clipCol.brighter(0.4f));
        noteRect.reduce(1, 1);
        g.fillRect(noteRect);
    }
    else
    {
        g.setColour(juce::Colours::black);
        g.fillRect(noteRect);

        g.setColour(clipCol.brighter(0.5f));
        noteRect.reduce(1, 1);
        g.fillRect(noteRect);

        g.setColour(clipCol);
        noteRect.reduce(1, 1);
        g.fillRect(noteRect);
    }

    if (m_editViewState.m_pianoKeyWidth > 13)
    {
        g.setColour(juce::Colours::black);
        g.drawText(
            juce::MidiMessage::getMidiNoteName(n->getNoteNumber() + noteDelta, true, true, 3),
            noteRect,
            juce::Justification::centredLeft);
    }
}
juce::Colour PianoRollContentComponent::getNoteColour(
    tracktion_engine::MidiClip* const& midiClip,
    tracktion_engine::MidiNote* n)
{
    auto s = getNoteStartBeat(midiClip, n);
    auto e = getNoteEndBeat(midiClip, n);


    if (isBeforeClipStart(s) || isAfterClipEnd(midiClip, e))
        return juce::Colours::grey;
    else if (n->getColour() == 127)
        return juce::Colours::white;

    if (isSelected(n))
        return juce::Colours::yellow;

    return m_track->getColour().darker(1.f - getVelocity(n));
}

float PianoRollContentComponent::getVelocity(const tracktion_engine::MidiNote* note)
{
    return juce::jmap((float) note->getVelocity(), 0.f, 127.f, 0.f, 1.f);
}

bool PianoRollContentComponent::isAfterClipEnd(
    tracktion_engine::MidiClip* const& midiClip, double beats)
{
    bool endAfterClipEnd = beats > midiClip->getEndBeat() - midiClip->getStartBeat();
    return endAfterClipEnd;
}

bool PianoRollContentComponent::isBeforeClipStart(double beats)
{
    bool startBeforeClipStart = beats < 0;
    return startBeforeClipStart;
}

juce::Rectangle<float>
    PianoRollContentComponent::getNoteRect(te::MidiClip* const& midiClip,
                                           const te::MidiNote* n) const
{
    double sBeat = getNoteStartBeat(midiClip, n);
    double eBeat = getNoteEndBeat(midiClip, n);
    auto x1 = m_editViewState.beatsToX(sBeat + midiClip->getStartBeat(),
                                       getWidth(),
                                       m_editViewState.m_pianoX1,
                                       m_editViewState.m_pianoX2);
    auto x2 = m_editViewState.beatsToX(eBeat + midiClip->getStartBeat(),
                                       getWidth(),
                                       m_editViewState.m_pianoX1,
                                       m_editViewState.m_pianoX2)
              + 1;

    return getNoteRect(n->getNoteNumber(), x1, x2);
}

juce::Rectangle<float>
    PianoRollContentComponent::getNoteRect(const int noteNum, int x1, int x2) const
{
    auto yOffset = (float) noteNum - getStartKey() + 1;
    auto noteY = (float) getHeight() - (yOffset * getKeyWidth());
    return {float(x1), float(noteY), float(x2 - x1), float(getKeyWidth())};
}

double PianoRollContentComponent::getNoteEndBeat(te::MidiClip* const& midiClip,
                                                 const te::MidiNote* n)
{
    auto eBeat = n->getEndBeat() - midiClip->getOffsetInBeats();
    return eBeat;
}

double PianoRollContentComponent::getNoteStartBeat(te::MidiClip* const& midiClip,
                                                   const te::MidiNote* n)
{
    auto sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat;
}

void PianoRollContentComponent::drawClipRange(
    juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip) const
{
    auto clipStartX = m_editViewState.beatsToX(midiClip->getStartBeat(),
                                               getWidth(),
                                               m_editViewState.m_pianoX1,
                                               m_editViewState.m_pianoX2)
                      + 1;
    auto clipEndX = m_editViewState.beatsToX(midiClip->getEndBeat(),
                                             getWidth(),
                                             m_editViewState.m_pianoX1,
                                             m_editViewState.m_pianoX2);
    g.setColour(midiClip->getColour());
    g.drawLine(clipStartX, 0, clipStartX, getHeight());
    g.drawLine(clipEndX, 0, clipEndX, getHeight());
    g.setColour(midiClip->getColour().withAlpha(0.2f));
    g.fillRect(clipStartX, 0, clipEndX - clipStartX, getHeight());
}

void PianoRollContentComponent::drawKeyLines(juce::Graphics& g,
                                             juce::Rectangle<int>& area) const
{
    auto lastNote = ((float) getHeight() / getKeyWidth()) + getStartKey();

    for (auto i = (int) getStartKey(); i <= (int) lastNote; i++)
    {
        g.setColour(juce::MidiMessage::isMidiNoteBlack(i)
                        ? juce::Colour(0x11ffffff)
                        : juce::Colour(0x22ffffff));
        g.fillRect(getNoteRect(i, 0, getWidth()).reduced(0, 1));
    }
}

float PianoRollContentComponent::getKeyWidth() const
{
    return (float) m_editViewState.m_pianoKeyWidth;
}

float PianoRollContentComponent::getStartKey() const
{
    return (float) m_editViewState.m_pianoStartKey;
}

void PianoRollContentComponent::startLasso(const juce::MouseEvent& e)
{
    m_lassoTool.startLasso(e.getEventRelativeTo(&m_lassoTool));
}

void PianoRollContentComponent::setNoteSelected(tracktion_engine::MidiNote& n,
                                                tracktion_engine::MidiClip& c)
{
    m_selectedEvents->addSelectedEvent(&n, true);
    m_editViewState.m_selectionManager.addToSelection(*m_selectedEvents);
}

void PianoRollContentComponent::mouseDown(const juce::MouseEvent& e)
{
    m_noteAdding = false;
    m_clickedNote = getNoteByPos(e.position);
    m_clickedPos = e.position;
    m_clickedKey = getKeyFromY(e.y);
    auto clickedBeat = xToBeats(e.x);
    m_clickedClip = getMidiClipByPos(e.x);
    if (m_clickedNote && m_clickedClip)
    {
        if (e.mods.isRightButtonDown())
        {
            deleteSelectedNotes();
        }
        else
        {
            m_clickOffsetBeats = m_clickedNote->getStartBeat() - clickedBeat;

            if (!isSelected(m_clickedNote))
            {
                unselectAll();
                setNoteSelected(*m_clickedNote, *m_clickedClip);
            }

            playNote(m_clickedClip, m_clickedNote);
        }
    }
    else if (m_clickedClip && (e.getNumberOfClicks() > 1 || e.mods.isShiftDown()))
    {
        unselectAll();

        auto beat = clickedBeat - m_clickedClip->getStartBeat()
                    + m_clickedClip->getOffsetInBeats();

        beat = getQuantizedBeat(beat);

        m_clickedNote = addNote(getNoteNumber(e.y), m_clickedClip, beat);
        setNoteSelected(*m_clickedNote, *m_clickedClip);

        m_clickOffsetBeats = m_clickedNote->getStartBeat() - clickedBeat;
        playNote(m_clickedClip, m_clickedNote);

        m_noteAdding = true;
    }
    else
    {
        unselectAll();
        startLasso(e);
    }
    std::cout << m_editViewState.m_selectionManager.getNumObjectsSelected() << std::endl;
}
te::MidiNote* PianoRollContentComponent::addNote(int noteNumb,
                                                 const te::MidiClip* clip,
                                                 double beat)
{
    return clip->getSequence().addNote(noteNumb,
                                       beat,
                                       m_editViewState.m_lastNoteLength == 0
                                           ? 0.25
                                           : m_editViewState.m_lastNoteLength,
                                       m_editViewState.m_lastVelocity,
                                       111,
                                       &m_editViewState.m_edit.getUndoManager());
}
void PianoRollContentComponent::playNote(const te::MidiClip* clip,
                                         te::MidiNote* note) const
{
    clip->getAudioTrack()->playGuideNote(
        note->getNoteNumber(), clip->getMidiChannel(), 127, false, true);
}
void PianoRollContentComponent::removeNote(te::MidiClip* clip, te::MidiNote* note)
{
    clip->getSequence().removeNote(*note,
                                   &m_editViewState.m_edit.getUndoManager());
}

void PianoRollContentComponent::updateLasso(const juce::MouseEvent& e)
{
    auto top = getYfromKey(m_clickedKey);
    m_lassoTool.updateLasso(e.getEventRelativeTo(&m_lassoTool), top);
}

void PianoRollContentComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() && e.mouseWasDraggedSinceMouseDown())
    {
        if (m_clickedNote && m_clickedClip)
        {
            if (m_expandLeft)
                expandSelectedNotesLeft(e);
            else if (m_expandRight || m_noteAdding)
                expandSelectedNotesRight(e);
            else
                moveSelectedNotesToMousePos(e);
        }
        else if (m_lassoTool.isVisible())
        {
            updateLasso(e);
            updateSelection();
        }
        repaint();
    }
}
void PianoRollContentComponent::moveSelectedNotesToMousePos(
    const juce::MouseEvent& e)
{
    auto newStartTime = m_editViewState.beatToTime(
        e.mods.isShiftDown()
            ? xToBeats(e.x) + m_clickOffsetBeats
                             : getQuantizedBeat(xToBeats(e.x) + m_clickOffsetBeats));
    m_draggedTimeDelta = newStartTime - m_editViewState.beatToTime(m_clickedNote->getStartBeat());
    m_draggedNoteDelta = getNoteNumber(e.y) - m_clickedNote->getNoteNumber();
}
void PianoRollContentComponent::expandSelectedNotesRight(const juce::MouseEvent& e)
{
    auto oldStart = m_clickedNote->getStartBeat();
    auto newLenght = xToBeats(e.x) + m_clickedClip->getOffsetInBeats()
                  - m_clickedClip->getStartBeat() - oldStart;
    newLenght = e.mods.isShiftDown() ? newLenght : getQuantizedBeat(newLenght);

    m_rightTimeDelta = beatsToTime(newLenght - m_clickedNote->getLengthBeats());

    m_editViewState.m_lastNoteLength = (newLenght);
}
void PianoRollContentComponent::expandSelectedNotesLeft(const juce::MouseEvent& e)
{
    auto newRawStartBeat = xToBeats(e.x) + m_clickOffsetBeats;
    auto newStart = e.mods.isShiftDown() ? newRawStartBeat : getQuantizedBeat(newRawStartBeat);

    m_leftTimeDelta = m_editViewState.beatToTime(newStart - m_clickedNote->getStartBeat());
}

double PianoRollContentComponent::getQuantizedBeat(double beat) const
{
    return  m_editViewState.getQuantizedBeat(beat, getBestSnapType());
}

te::TimecodeSnapType PianoRollContentComponent::getBestSnapType() const
{
    return m_editViewState.getBestSnapType(
        m_editViewState.m_pianoX1, m_editViewState.m_pianoX2, getWidth());
}

double PianoRollContentComponent::xToBeats(const int& x) const
{
    return m_editViewState.xToBeats(
        x, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}

void PianoRollContentComponent::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);

    if (auto mc = getMidiClipByPos(e.x))
    {
        for (auto n: mc->getSequence().getNotes())
        {
            n->setColour(111, &m_editViewState.m_edit.getUndoManager());
        }
        if (auto note = getNoteByPos(e.position))
        {
            auto startX = beatsToX(
                note->getStartBeat() + mc->getStartBeat() - mc->getOffsetInBeats());

            auto endX = beatsToX(
                note->getEndBeat() + mc->getStartBeat() - mc->getOffsetInBeats());
            note->setColour(127, &m_editViewState.m_edit.getUndoManager());
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
    getParentComponent()->mouseMove(e);
    //repaint ();
}

void PianoRollContentComponent::mouseExit(const juce::MouseEvent&)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void PianoRollContentComponent::stopLasso()
{
    if (m_lassoTool.isVisible())
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        m_lassoTool.stopLasso();
    }
}

void PianoRollContentComponent::mouseUp(const juce::MouseEvent&e)
{
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
        at->turnOffGuideNotes();

    stopLasso();

    if (m_draggedTimeDelta != 0.0 || m_draggedNoteDelta != 0)
    {
        m_selectedEvents->moveEvents (m_draggedTimeDelta, 0, m_draggedNoteDelta);

        m_draggedNoteDelta = 0;
        m_draggedTimeDelta = 0.0;
    }

    if (m_leftTimeDelta != 0.0 || m_rightTimeDelta != 0.0)
    {
        m_selectedEvents->moveEvents(m_leftTimeDelta, m_leftTimeDelta * (-1.0) , 0);
        m_selectedEvents->moveEvents(0, m_rightTimeDelta, 0);

        m_leftTimeDelta = 0.0;
        m_rightTimeDelta = 0.0;
    }

    if (!e.mouseWasDraggedSinceMouseDown())
    {
        unselectAll();
        setNoteSelected(*m_clickedNote, *m_clickedClip);
    }

    m_clickedNote = nullptr;
    m_clickedClip = nullptr;

    repaint();
}

void PianoRollContentComponent::mouseWheelMove(const juce::MouseEvent& event,
                                               const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isShiftDown())
    {
        auto deltaX1 = event.mods.isCtrlDown() ? wheel.deltaY : -wheel.deltaY;
        auto deltaX2 = -wheel.deltaY;

        m_editViewState.m_pianoX1 =
            juce::jmax(0.0, m_editViewState.m_pianoX1 + deltaX1);
        m_editViewState.m_pianoX2 = m_editViewState.m_pianoX2 + deltaX2;
    }
    else
    {
        scrollPianoRoll((float) wheel.deltaY * 5);
    }
}

void PianoRollContentComponent::scrollPianoRoll(float delta)
{
    m_editViewState.m_pianoStartKey =
        juce::jlimit(0.f,
                     127.f - (float) (getHeight() / m_editViewState.m_pianoKeyWidth),
                     (float) m_editViewState.m_pianoStartKey + delta);
}

juce::Array<te::MidiClip*> PianoRollContentComponent::getMidiClipsOfTrack()
{
    juce::Array<te::MidiClip*> midiClips;
    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
    {
        for (auto c: at->getClips())
        {
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
            {
                midiClips.add(mc);
            }
        }
    }
    return midiClips;
}

void PianoRollContentComponent::drawBarsAndBeatLines(juce::Graphics& g,
                                                     juce::Colour colour)
{
    g.setColour(colour);
    double x1 = m_editViewState.m_pianoX1;
    double x2 = m_editViewState.m_pianoX2;
    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, x1, x2, getBounds());
}
int PianoRollContentComponent::getNoteNumber(int y)
{
    auto noteNumb = (int) getKeyFromY(y);
    return noteNumb;
}

tracktion_engine::MidiNote*
    PianoRollContentComponent::getNoteByPos(juce::Point<float> pos)
{
    for (auto& mc: getMidiClipsOfTrack())
    {
        for (auto note: mc->getSequence().getNotes())
        {
            if (note->getNoteNumber() == getNoteNumber((int) pos.y))
            {
                auto clickedBeat = xToBeats((int) pos.x) + mc->getOffsetInBeats();
                auto clipStart = mc->getStartBeat();
                if (clickedBeat > note->getStartBeat() + clipStart
                    && clickedBeat < note->getEndBeat() + clipStart)
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
    for (auto& clip: getMidiClipsOfTrack())
    {
        if ((clip->getStartBeat() < xToBeats(y))
            && (clip->getEndBeat() > xToBeats(y)))
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
bool PianoRollContentComponent::clipContains(const te::MidiClip* clip,
                                             te::MidiNote* note)
{
    return clip->getSequence().getNotes().contains(note);
}

juce::Array<tracktion_engine::MidiNote*> PianoRollContentComponent::getSelectedNotes()
{
 /*   juce::Array<tracktion_engine::MidiNote*> selNotes;
    for (auto se : m_editViewState.m_selectionManager.getItemsOfType<te::SelectedMidiEvents>())
        for (auto n : se->getSelectedNotes())
            selNotes.add(n);*/
    //return selNotes;
 return m_selectedEvents->getSelectedNotes();
}
void PianoRollContentComponent::unselectAll()
{
    m_editViewState.m_selectionManager.deselectAll();
    repaint();
}
double PianoRollContentComponent::getKeyFromY(int y)
{
    auto keyHeight = (double) m_editViewState.m_pianoKeyWidth;
    auto keyNumb = (double) (m_editViewState.m_pianoStartKey
                             + ((double) (getHeight() - y) / keyHeight));

    return keyNumb;
}
int PianoRollContentComponent::getYfromKey(double key)
{
    auto keyHeight = (double) m_editViewState.m_pianoKeyWidth;
    auto firstVisibleKey = (double) m_editViewState.m_pianoStartKey;

    auto y = getHeight() - (keyHeight * (key - firstVisibleKey));

    return y;
}
void PianoRollContentComponent::updateSelection()
{
    unselectAll();
    for (auto c : getMidiClipsOfTrack())
    {
        for (auto n : c->getSequence().getNotes())
        {
            if (getLassoVerticalRange().intersects(
                juce::Range<double>((double) n->getNoteNumber(),(double) n->getNoteNumber() + 1)))
            {
                if (m_lassoTool.getLassoRect().m_timeRange.overlaps(n->getEditTimeRange(*c)))
                {
                    m_selectedEvents->addSelectedEvent(n, true);
                }
            }
        }
    }
    m_editViewState.m_selectionManager.addToSelection(*m_selectedEvents);
}
int PianoRollContentComponent::beatsToX(double beats)
{
    return m_editViewState.beatsToX(
        beats, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
void PianoRollContentComponent::deleteSelectedNotes()
{
    for (auto c: getMidiClipsOfTrack())
        for (auto n: getSelectedNotes())
            removeNote(c, n);
}
bool PianoRollContentComponent::isSelected(tracktion_engine::MidiNote* note)
{
    for (auto se : m_editViewState.m_selectionManager.getItemsOfType<te::SelectedMidiEvents>())
        if(se->getSelectedNotes().contains(note))
            return true;
    return false;
}
double PianoRollContentComponent::timeToX(const double& time) const
{
    return m_editViewState.timeToX(time, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
double PianoRollContentComponent::beatsToTime(double beats)
{
    return m_editViewState.beatToTime(beats);
}
