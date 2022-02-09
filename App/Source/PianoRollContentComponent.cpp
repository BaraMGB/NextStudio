#include "PianoRollContentComponent.h"
#include "Utilities.h"

PianoRollContentComponent::PianoRollContentComponent(
    EditViewState& evs, tracktion_engine::Track::Ptr track)
    : m_editViewState(evs)
    , m_track(std::move(track))
    , m_lassoTool(evs, evs.m_pianoX1, evs.m_pianoX2)
{
    addChildComponent(m_lassoTool);
    updateSelectedEvents();
}

PianoRollContentComponent::~PianoRollContentComponent() = default;

void PianoRollContentComponent::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();

    drawKeyLines(g);
    drawBarsAndBeatLines(g, juce::Colours::black);

    for (auto& midiClip: getMidiClipsOfTrack())
    {
        drawClipRange(g, midiClip);

        auto& seq = midiClip->getSequence();

        for (auto n: seq.getNotes())
            drawNote(g, midiClip, n);
    }

    for (auto sn : m_selectedEvents->getSelectedNotes())
        drawNote(g
                 , m_selectedEvents->clipForEvent(sn)
                 , sn
                 , m_draggedTimeDelta
                 , m_draggedNoteDelta
                 , m_leftTimeDelta
                 , m_rightTimeDelta);
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
    noteRect.removeFromBottom(1);
    auto clipCol = getNoteColour(midiClip, n);
    auto scroll = beatsToX(0) * (-1);
    const juce::Colour& borderColour = juce::Colour(0xccffffff);
    if (timeDelta != 0.0 || noteDelta != 0)
    {
        auto newX = scroll + noteRect.getX() + timeToX(timeDelta);
        if (m_snap)
            newX = beatsToX(getQuantizedBeat(xToBeats(newX), false));
        auto newY = getYForKey(getKeyForY(noteRect.getY()) + noteDelta);

        noteRect.setPosition(newX, newY);

        g.setColour(borderColour);
        g.drawRect(noteRect);

        g.setColour(juce::Colours::black);
        noteRect.reduce(1, 1);
        g.drawRect(noteRect);

        noteRect.reduce(1, 1);
        g.setColour(borderColour);
        drawKeyNum(g, n, noteDelta, noteRect);
    }
    else if (timeLeftDelta != 0.0 || timeRightDelta != 0.0)
    {
        auto left = scroll + noteRect.getX() + timeToX(timeLeftDelta);
        if (timeLeftDelta != 0.0 && m_snap)
            left =  beatsToX(getQuantizedBeat(xToBeats(left), false));
        auto right = scroll + noteRect.getRight() + timeToX(timeRightDelta);
        if (timeRightDelta != 0.0 && m_snap)
            right = beatsToX(getQuantizedBeat(xToBeats(right), false));
        noteRect.setLeft(left);
        noteRect.setRight(right);

        g.setColour(borderColour);
        g.drawRect(noteRect);

        g.setColour(juce::Colours::black);
        noteRect.reduce(1, 1);
        g.drawRect(noteRect);
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

        if (isSelected(n))
        {
            g.setColour(borderColour);
            g.drawRect(noteRect.expanded(2,2));
        }
        g.setColour(juce::Colours::black);
        drawKeyNum(g, n, noteDelta, noteRect);
    }
}
void PianoRollContentComponent::drawKeyNum(juce::Graphics& g,
                                           const tracktion_engine::MidiNote* n,
                                           int noteDelta,
                                           juce::Rectangle<float>& noteRect) const
{
    if (m_editViewState.m_pianoKeyWidth > 13)
    {
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
    bool isBeforeClipStart = s < 0;
    bool isAfterClipEnd = e > midiClip->getEndBeat() - midiClip->getStartBeat();

    if (isBeforeClipStart || isAfterClipEnd)
        return juce::Colours::grey;
    else if (n->getColour() == 127)
        return m_track->getColour().brighter(0.6);

    return m_track->getColour().darker(1.f - getVelocity(n));
}

float PianoRollContentComponent::getVelocity(const tracktion_engine::MidiNote* note)
{
    return juce::jmap((float) note->getVelocity(), 0.f, 127.f, 0.f, 1.f);
}

juce::Rectangle<float>
    PianoRollContentComponent::getNoteRect(te::MidiClip* const& midiClip,
                                           const te::MidiNote* n)
{
    double sBeat = getNoteStartBeat(midiClip, n);
    double eBeat = getNoteEndBeat(midiClip, n);
    auto x1 = beatsToX(sBeat + midiClip->getStartBeat());
    auto x2 = beatsToX(eBeat + midiClip->getStartBeat()) + 1;

    return getNoteRect(n->getNoteNumber(), x1, x2);
}

juce::Rectangle<float>
    PianoRollContentComponent::getNoteRect(const int noteNum, int x1, int x2) const
{
    auto yOffset = (float) noteNum - getStartKey() + 1;
    auto noteY = (float) getHeight() - (yOffset * getKeyWidth());
    return {float(x1), float(noteY), float(x2 - x1), float(getKeyWidth())};
}

double PianoRollContentComponent::getNoteEndBeat(const te::MidiClip* midiClip,
                                                 const te::MidiNote* n)
{
    auto eBeat = n->getEndBeat() - midiClip->getOffsetInBeats();
    return eBeat;
}
double PianoRollContentComponent::getNoteStartBeat(const te::MidiClip* midiClip,
                                                   const te::MidiNote* n)
{
    auto sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat;
}
void PianoRollContentComponent::drawClipRange(
    juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip)
{
    auto clipStartX = beatsToX(midiClip->getStartBeat()) + 1;
    auto clipEndX = juce::jmax(clipStartX, beatsToX(midiClip->getEndBeat()));

    g.setColour(midiClip->getColour());
    g.drawLine(clipStartX, 0, clipStartX, getHeight());
    g.drawLine(clipEndX, 0, clipEndX, getHeight());
    g.setColour(midiClip->getColour().withAlpha(0.2f));
    g.fillRect(clipStartX, 0, clipEndX - clipStartX, getHeight());
}

void PianoRollContentComponent::drawKeyLines(juce::Graphics& g) const
{
    int lastNote = (getHeight() / getKeyWidth()) + getStartKey();

    for (auto i = static_cast<int>(getStartKey()); i <= lastNote; i++)
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

void PianoRollContentComponent::setNoteSelected(tracktion_engine::MidiNote* n,
                                                bool addToSelection)
{
    m_selectedEvents->addSelectedEvent(n, addToSelection);
    m_editViewState.m_selectionManager.addToSelection(*m_selectedEvents);
}

void PianoRollContentComponent::mouseDown(const juce::MouseEvent& e)
{
    m_noteAdding = false;
    m_clickedNote = getNotesByPos(e.position).getLast();
    m_clickedKey = getKeyForY(e.y);
    const auto clickedBeat = xToBeats(e.x);
    m_clickedClip = getMidiClipAt(e.x);
    m_snap = false;
    if (m_clickedNote && m_clickedClip)
    {
        if (e.mods.isRightButtonDown())
        {
            if (!isSelected(m_clickedNote))
                setNoteSelected(m_clickedNote, false);
            deleteSelectedNotes();
        }
        else
        {
            if (!isSelected(m_clickedNote))
                setNoteSelected(m_clickedNote, false);

            playGuideNote(m_clickedClip,
                          m_clickedNote->getNoteNumber(),
                          m_clickedNote->getVelocity());
        }
    }
    else if (m_clickedClip
             && e.mods.isLeftButtonDown()
             && (e.getNumberOfClicks() > 1 || e.mods.isShiftDown()))
    {
        auto beat = clickedBeat;
        beat = getQuantizedBeat(beat);

        m_clickedNote = addNewNote (getNoteNumber(e.y), m_clickedClip, beat - m_clickedClip->getStartBeat() + m_clickedClip->getOffsetInBeats());
        setNoteSelected(m_clickedNote, false);

        playGuideNote(m_clickedClip,
                      m_clickedNote->getNoteNumber(),
                      m_clickedNote->getVelocity());

        m_noteAdding = true;
    }
    else
    {
        unselectAll();
        startLasso(e);
    }
}
te::MidiNote* PianoRollContentComponent::addNewNote(int noteNumb,
                                                 const te::MidiClip* clip,
                                                 double beat)
{
    auto length = m_editViewState.m_lastNoteLength == 0
                      ? 0.25
                      : m_editViewState.m_lastNoteLength;
    cleanUnderNote(noteNumb, {beat, beat + length}, clip);
    return clip->getSequence().addNote(noteNumb,
                                       beat,
                                       length,
                                       m_editViewState.m_lastVelocity,
                                       111,
                                       &m_editViewState.m_edit.getUndoManager());
}
void PianoRollContentComponent::playGuideNote(const te::MidiClip* clip,const int noteNumb, int vel)
{
        clip->getAudioTrack()->playGuideNote(
            noteNumb, clip->getMidiChannel(), vel, false, true);
        startTimer(100);
}
void PianoRollContentComponent::removeNote(te::MidiClip* clip, te::MidiNote* note)
{
    clip->getSequence().removeNote(*note,
                                   &m_editViewState.m_edit.getUndoManager());
}

void PianoRollContentComponent::updateLasso(const juce::MouseEvent& e)
{
    auto top = getYForKey(m_clickedKey);
    m_lassoTool.updateLasso(e.getEventRelativeTo(&m_lassoTool), top);
}

void PianoRollContentComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!e.mods.isShiftDown())
        m_snap = true;
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
            updateLassoSelection();
        }
        repaint();
    }
}
void PianoRollContentComponent::moveSelectedNotesToMousePos(
    const juce::MouseEvent& e)
{
    auto scrollTime = beatsToTime(m_editViewState.m_pianoX1);

    m_draggedTimeDelta = xToTime(e.getDistanceFromDragStartX());

    m_draggedTimeDelta = m_draggedTimeDelta - scrollTime;

    m_draggedNoteDelta = getNoteNumber(e.y) - m_clickedNote->getNoteNumber();

    m_clickedClip->getAudioTrack()->turnOffGuideNotes();

    for (auto n : m_selectedEvents->getSelectedNotes())
    {
        auto clip = m_selectedEvents->clipForEvent(n);

        playGuideNote(
            clip, n->getNoteNumber() + m_draggedNoteDelta, n->getVelocity());
    }
}
void PianoRollContentComponent::expandSelectedNotesRight(const juce::MouseEvent& e)
{
    auto scrollTime = beatsToTime(m_editViewState.m_pianoX1);

    m_rightTimeDelta =  xToTime(e.getDistanceFromDragStartX()) - scrollTime;
}
void PianoRollContentComponent::expandSelectedNotesLeft(const juce::MouseEvent& e)
{
    auto scrollTime = beatsToTime(m_editViewState.m_pianoX1);
    m_leftTimeDelta =  xToTime(e.getDistanceFromDragStartX()) - scrollTime;
}

double PianoRollContentComponent::getQuantizedBeat(double beat, bool down) const
{
    return  m_editViewState.getQuantizedBeat(beat, getBestSnapType(), down);
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

    if (auto mc = getMidiClipAt(e.x))
    {
        for (auto n: mc->getSequence().getNotes())
            n->setColour(111, nullptr);

        if (auto note = getNotesByPos(e.position).getLast())
        {
            auto startX = beatsToX(note->getStartBeat()
                                   + mc->getStartBeat() - mc->getOffsetInBeats());
            auto endX = beatsToX(note->getEndBeat()
                                 + mc->getStartBeat() - mc->getOffsetInBeats());
            note->setColour(127, nullptr);

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
    EngineHelpers::getAudioTrack(getTrack(), m_editViewState)->turnOffGuideNotes();

    auto& um = m_editViewState.m_edit.getUndoManager();
    stopLasso();

    auto targetClip = getMidiClipAt(e.x);

    if (!e.mouseWasDraggedSinceMouseDown() && e.mods.isLeftButtonDown())
    {
        setNoteSelected(m_clickedNote, false);
    }
    else if (targetClip && (m_draggedTimeDelta != 0.0 || m_draggedNoteDelta != 0 || m_leftTimeDelta != 0.0 || m_rightTimeDelta != 0.0))
    {
        const double startDelta = timeToBeat (m_draggedTimeDelta)  + timeToBeat(m_leftTimeDelta);
        const double lengthDelta = timeToBeat (m_leftTimeDelta * (-1)) + timeToBeat(m_rightTimeDelta);

        juce::Array<std::pair<te::MidiNote*, te::MidiClip*>> temp;
        moveSelectedNotesToTemp(startDelta, lengthDelta, temp);
        //all notes remain in their clips
        if (m_expandLeft || m_expandRight || m_clickedClip == targetClip)
        {
            if (m_expandRight)
                for (auto i = temp.size() - 1; i >= 0; i--)
                    insertNote(temp[i].first, temp[i].second);
            else
                for (auto p: temp)
                    insertNote(p.first, p.second);
        }
        //all notes will go to target clip
        else
        {
            for (auto p : temp)
            {
                auto clipDelta = targetClip->getStartBeat() - p.second->getStartBeat();
                p.first->setStartAndLength(p.first->getStartBeat() - clipDelta,
                                           p.first->getLengthBeats(),
                                           &um);
                insertNote(p.first, targetClip);
            }
        }
        m_editViewState.m_selectionManager.addToSelection(*m_selectedEvents);
    }

    cleanUpFlags();

    repaint();
}
void PianoRollContentComponent::cleanUpFlags()
{
    m_draggedNoteDelta = 0;
    m_draggedTimeDelta = 0.0;

    m_leftTimeDelta = 0.0;
    m_rightTimeDelta = 0.0;

    m_clickedNote = nullptr;
    m_clickedClip = nullptr;

    m_snap = false;
}
void PianoRollContentComponent::moveSelectedNotesToTemp(
    const double startDelta,
    const double lengthDelta,
    juce::Array<std::pair<te::MidiNote*, te::MidiClip*>>& temp)
{
    auto& um = m_editViewState.m_edit.getUndoManager();
    for (auto n: getSelectedNotes())
    {
        auto clip = m_selectedEvents->clipForEvent(n);

        te::MidiNote* mn = new te::MidiNote(
            te::MidiNote::createNote(*n,
                                     n->getStartBeat() + startDelta,
                                     n->getLengthBeats() + lengthDelta));
        mn->setNoteNumber(n->getNoteNumber() + this->m_draggedNoteDelta, &um);

        std::pair<te::MidiNote*, te::MidiClip*> pair = {mn, clip};
        temp.add(pair);

        clip->getSequence().removeNote(*n, &um);
    }
}
void PianoRollContentComponent::insertNote(te::MidiNote* note, te::MidiClip* clip)
{
    if (m_snap)
        snapToGrid(note, clip);

    auto& um = m_editViewState.m_edit.getUndoManager();
    cleanUnderNote(note->getNoteNumber(), note->getRangeBeats(), clip);
    auto mn = clip->getSequence().addNote(note->getNoteNumber(),
                                          note->getStartBeat(),
                                          note->getLengthBeats(),
                                          note->getVelocity(),
                                          note->getColour(),
                                          &um);
    m_selectedEvents->addSelectedEvent(mn, true);
}
void PianoRollContentComponent::snapToGrid(te::MidiNote* note,
                                           const te::MidiClip* clip) const
{
    auto startBeat = note->getStartBeat() + clip->getStartBeat();
    if (!m_expandRight)
        startBeat = getQuantizedBeat(startBeat, false);
    auto noteStart = startBeat - clip->getStartBeat();

    auto endBeat = note->getEndBeat() + clip->getStartBeat();
    if (m_expandRight || (!m_expandRight && !m_expandLeft))
        endBeat = getQuantizedBeat(endBeat, false);
    auto noteEnd = endBeat - clip->getStartBeat();
    auto length = noteEnd - noteStart;
    if (m_expandLeft)
        length = note->getLengthBeats() + (note->getStartBeat() - noteStart );
    note->setStartAndLength(noteStart, length, nullptr);
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
        for (auto c: at->getClips())
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
                midiClips.add(mc);

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
    auto noteNumb = (int) getKeyForY(y);
    return noteNumb;
}
juce::Array<te::MidiNote*>
    PianoRollContentComponent::getNotesByPos(juce::Point<float> pos)
{
    juce::Array<te::MidiNote*> notes;
    for (auto& mc: getMidiClipsOfTrack())
    {
        for (auto note: mc->getSequence().getNotes())
        {
            auto clickedBeat = xToBeats((int) pos.x) + mc->getOffsetInBeats();
            auto clipStart = mc->getStartBeat();
            auto isNoteNum = (note->getNoteNumber() == getNoteNumber(static_cast<int> (pos.y)));
            auto noteStart = note->getStartBeat() + clipStart;
            auto noteEnd = note->getEndBeat() + clipStart;

            if (isNoteNum && juce::Range<double> (noteStart, noteEnd).contains(clickedBeat))
                notes.add(note);
        }
    }
    return notes;
}
tracktion_engine::MidiClip* PianoRollContentComponent::getMidiClipAt(int x)
{
    for (auto& c: getMidiClipsOfTrack())
        if ((c->getStartBeat() < xToBeats(x)) && (c->getEndBeat() > xToBeats(x)))
            return c;
    return nullptr;
}
tracktion_engine::Track::Ptr PianoRollContentComponent::getTrack()
{
    return m_track;
}
juce::Array<tracktion_engine::MidiNote*> PianoRollContentComponent::getSelectedNotes()
{
    return m_selectedEvents->getSelectedNotes();
}
void PianoRollContentComponent::unselectAll()
{
    m_editViewState.m_selectionManager.deselectAll();
    repaint();
}
double PianoRollContentComponent::getKeyForY(int y)
{
    auto keyHeight = (double) m_editViewState.m_pianoKeyWidth;
    auto keyNumb = (double) (m_editViewState.m_pianoStartKey
                             + ((double) (getHeight() - y) / keyHeight));

    return keyNumb;
}
int PianoRollContentComponent::getYForKey(double key)
{
    auto keyHeight = (double) m_editViewState.m_pianoKeyWidth;
    auto firstVisibleKey = (double) m_editViewState.m_pianoStartKey;

    auto y = getHeight() - (keyHeight * (key - firstVisibleKey));

    return y;
}
void PianoRollContentComponent::updateLassoSelection()
{
    unselectAll();

    for (auto c : getMidiClipsOfTrack())
        for (auto n : c->getSequence().getNotes())
            if (isInLassoRange(c, n))
                m_selectedEvents->addSelectedEvent(n, true);

    m_editViewState.m_selectionManager.addToSelection(*m_selectedEvents);
}
bool PianoRollContentComponent::isInLassoRange(
    const te::MidiClip* clip, const tracktion_engine::MidiNote* midiNote)
{
    auto verticalKeyRange = juce::Range<double> ((double) midiNote->getNoteNumber()
        ,(double) midiNote->getNoteNumber() + 1);

    return getLassoVerticalKeyRange().intersects(verticalKeyRange)
                    && m_lassoTool.getLassoRect().m_timeRange.overlaps(midiNote->getEditTimeRange(*clip));
}
int PianoRollContentComponent::beatsToX(double beats)
{
    return m_editViewState.beatsToX(
        beats, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
void PianoRollContentComponent::deleteSelectedNotes()
{
    for (auto n : getSelectedNotes())
        m_selectedEvents->clipForEvent(n)->getSequence()
            .removeNote(*n, &m_editViewState.m_edit.getUndoManager());
}
bool PianoRollContentComponent::isSelected(tracktion_engine::MidiNote* note)
{
    return m_selectedEvents->isSelected(note);
}
double PianoRollContentComponent::timeToX(const double& time) const
{
    return m_editViewState.timeToX(time, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
double PianoRollContentComponent::beatsToTime(double beats)
{
    return m_editViewState.beatToTime(beats);
}
void PianoRollContentComponent::timerCallback()
{
    stopTimer();
    auto at = EngineHelpers::getAudioTrack(getTrack(), m_editViewState);
    at->turnOffGuideNotes();
}
void PianoRollContentComponent::cleanUnderNote(int noteNumb, juce::Range<double> beatRange,
                                               const te::MidiClip* clip)
{
    for (auto n : getNotesInRange(beatRange, clip))
    {
        if (n->getNoteNumber() == noteNumb)
        {
            double startBeat = getNoteStartBeat(clip, n);
            double endBeat = getNoteEndBeat(clip, n);

            juce::UndoManager* um = &m_editViewState.m_edit.getUndoManager();

            if (startBeat >= beatRange.getStart()
                && endBeat <= beatRange.getEnd())
            {
                clip->getSequence().removeNote(*n, um);
                continue;
            }

            if (startBeat >= beatRange.getStart()
                && endBeat > beatRange.getEnd())
            {
                n->setStartAndLength(beatRange.getEnd(), endBeat - beatRange.getEnd(), um);
                continue;
            }

            if (startBeat < beatRange.getStart()
                && endBeat <= beatRange.getEnd())
            {
                n->setStartAndLength(startBeat, beatRange.getStart() - startBeat, um);
                continue;
            }

            if (startBeat < beatRange.getStart()
                && endBeat > beatRange.getEnd())
            {
                n->setStartAndLength(startBeat, beatRange.getStart() - startBeat, um);
                clip->getSequence().addNote(noteNumb, beatRange.getEnd(), endBeat - beatRange.getEnd(),n->getVelocity(), n->getColour(), um);
            }
        }
    }
}
juce::Array<te::MidiNote*>
    PianoRollContentComponent::getNotesInRange(juce::Range<double> beatRange,
                                               const te::MidiClip* clip)
{
    juce::Array<te::MidiNote*> notesInRange;

    for(auto n : clip->getSequence().getNotes())
        if (beatRange.intersects (n->getRangeBeats()))
                notesInRange.add(n);

    return notesInRange;
}
double PianoRollContentComponent::timeToBeat(double time)
{
    return m_editViewState.timeToBeat(time);
}
te::MidiClip* PianoRollContentComponent::getNearestClipBefor(int x)
{
    if (getMidiClipAt(x) != nullptr)
        return getMidiClipAt(x);

    auto cPtr = getMidiClipsOfTrack().getFirst();

    for (auto c : getMidiClipsOfTrack())
        if (c->getEndBeat() < xToBeats(x))
            if (c->getEndBeat() > cPtr->getEndBeat())
                cPtr = c;

    return cPtr;
}
te::MidiClip* PianoRollContentComponent::getNearestClipAfter(int x)
{
    if (getMidiClipAt(x) != nullptr)
        return getMidiClipAt(x);

    te::MidiClip* clip = nullptr;

    for (auto c : getMidiClipsOfTrack())
        if (c->getStartBeat() > xToBeats(x)
            && (clip == nullptr || clip->getStartBeat() > c->getStartBeat()))
                clip = c;

    return clip;
}
juce::Range<double> PianoRollContentComponent::getLassoVerticalKeyRange()
{
    if (m_lassoTool.isVisible())
    {
        auto top = m_lassoTool.getLassoRect().m_top;
        auto bottom = m_lassoTool.getLassoRect().m_bottom;
        juce::Range<double> range (juce::jmin(getKeyForY(top), getKeyForY(bottom))
            ,juce::jmax(getKeyForY(top), getKeyForY(bottom)));
        return range;
    }
    return {0,0};
}
void PianoRollContentComponent::updateSelectedEvents()
{
    GUIHelpers::log("MidiEditor: selectedMidiEvents updated ... ");
    if (m_selectedEvents != nullptr)
        m_selectedEvents->deselect();

    if (getMidiClipsOfTrack().size() > 0)
        m_selectedEvents = std::make_unique<te::SelectedMidiEvents>(getMidiClipsOfTrack());
    else
        m_selectedEvents.reset(nullptr);
}
double PianoRollContentComponent::xToTime(const int& x) const
{
    return m_editViewState.xToTime(x, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
