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
    setWantsKeyboardFocus (true);
}

PianoRollContentComponent::~PianoRollContentComponent() = default;

void PianoRollContentComponent::paint(juce::Graphics& g)
{
    drawKeyLines(g);

    drawBarsAndBeatLines(g, juce::Colours::black);

    for (auto& midiClip: getMidiClipsOfTrack())
    {
        drawClipRange(g, midiClip);

        auto& seq = midiClip->getSequence();

        for (auto n: seq.getNotes())
            drawNote(g, midiClip, n);
    }

    if (areNotesDragged())
        for (auto sn : getSelectedNotes())
            drawDraggedNotes(g, sn, m_selectedEvents->clipForEvent(sn));
}
void PianoRollContentComponent::resized()
{
    auto area = getLocalBounds();
    m_lassoTool.setBounds(area);
}
void PianoRollContentComponent::drawNote(juce::Graphics& g,
                                         tracktion_engine::MidiClip* const& midiClip,
                                         tracktion_engine::MidiNote* n
                                         )
{
    auto noteRect = getNoteRect(midiClip, n);
    auto noteColor = getNoteColour(midiClip, n);
    auto innerGlow = noteColor.brighter(0.5f);
    auto selectedColour = juce::Colour(0xccffffff);
    auto borderColour = juce::Colour(0xff000000);

    g.setColour(borderColour);
    g.fillRect(noteRect);

    g.setColour(innerGlow);
    noteRect.reduce(1, 1);
    g.fillRect(noteRect);

    g.setColour(noteColor);
    noteRect.reduce(1, 1);
    g.fillRect(noteRect);

    if (isSelected(n))
    {
        g.setColour(selectedColour);
        g.drawRect(noteRect.expanded(2, 2));
    }

    g.setColour(borderColour);
    drawKeyNum(g, n, noteRect);
}
void PianoRollContentComponent::drawDraggedNotes(juce::Graphics& g, te::MidiNote* n, te::MidiClip* clip)
{
    auto borderColour = juce::Colour(0xccffffff);

    const double startDelta = timeToBeat (m_draggedTimeDelta)  + timeToBeat(m_leftTimeDelta);
    const double lengthDelta = timeToBeat (m_leftTimeDelta * (-1)) + timeToBeat(m_rightTimeDelta);

    te::MidiNote mn = te::MidiNote(
        te::MidiNote::createNote(*n,
                                 tracktion::core::BeatPosition::fromBeats(n->getStartBeat().inBeats() + startDelta),
                                 tracktion::core::BeatDuration::fromBeats(n->getLengthBeats().inBeats() + lengthDelta)));
    mn.setNoteNumber(mn.getNoteNumber() + m_draggedNoteDelta, nullptr);

    auto noteRect = getNoteRect(clip, &mn);

    g.setColour(borderColour);
    g.drawRect(noteRect);

    auto innerBorderColour = juce::Colours::black;
    g.setColour(innerBorderColour);
    noteRect.reduce(1, 1);
    g.drawRect(noteRect);

    noteRect.reduce(1, 1);
    g.setColour(borderColour);
    drawKeyNum(g, &mn, noteRect);
}
void PianoRollContentComponent::drawKeyNum(juce::Graphics& g,
                                           const tracktion_engine::MidiNote* n,
                                           juce::Rectangle<float>& noteRect) const
{
    if (m_editViewState.m_pianoKeyWidth > 13)
        g.drawText(juce::MidiMessage::getMidiNoteName(
                       n->getNoteNumber() , true, true, 3),
                   noteRect, juce::Justification::centredLeft);

}
juce::Colour PianoRollContentComponent::getNoteColour(
    tracktion_engine::MidiClip* const& midiClip,
    tracktion_engine::MidiNote* n)
{
    auto s = getNoteStartBeat(midiClip, n);
    auto e = getNoteEndBeat(midiClip, n);
    bool isBeforeClipStart = s < 0;
    bool isAfterClipEnd = e > midiClip->getEndBeat().inBeats() - midiClip->getStartBeat().inBeats() + 0.00001;

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
    auto x1 = beatsToX(sBeat + midiClip->getStartBeat().inBeats());
    auto x2 = beatsToX(eBeat + midiClip->getStartBeat().inBeats()) + 1;

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
    return eBeat.inBeats();
}
double PianoRollContentComponent::getNoteStartBeat(const te::MidiClip* midiClip,
                                                   const te::MidiNote* n)
{
    auto sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat.inBeats();
}
void PianoRollContentComponent::drawClipRange(
    juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip)
{
    auto clipStartX = beatsToX(midiClip->getStartBeat().inBeats());
    auto clipEndX = juce::jmax(clipStartX, beatsToX(midiClip->getEndBeat().inBeats()));
    auto clipColour = midiClip->getColour();

    g.setColour(clipColour.brighter(0.7f));
    g.drawVerticalLine(clipStartX, 0, static_cast<float> (getHeight()));
    g.drawVerticalLine(clipEndX, 0, static_cast<float> (getHeight()));
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRect(clipStartX, 0, clipEndX - clipStartX, getHeight());
    g.setColour(midiClip->getColour().withAlpha(0.1f));
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

void PianoRollContentComponent::mouseMove(const juce::MouseEvent& e)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);

    if (auto mc = getMidiClipAt(e.x))
    {
        for (auto n: mc->getSequence().getNotes())
            n->setColour(111, nullptr);

        if (auto note = getNoteByPos(e.position))
        {
            auto startX = beatsToX(note->getStartBeat().inBeats()
                                   + mc->getStartBeat().inBeats() - mc->getOffsetInBeats().inBeats());
            auto endX = beatsToX(note->getEndBeat().inBeats()
                                 + mc->getStartBeat().inBeats() - mc->getOffsetInBeats().inBeats());
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

void PianoRollContentComponent::mouseDown(const juce::MouseEvent& e)
{
    m_noteAdding = false;
    m_clickedNote = getNoteByPos(e.position);
    m_clickedKey = getKeyForY(e.y);
    m_clickedClip = getMidiClipAt(e.x);
    m_snap = false;
    const auto clickedBeat = xToBeats(e.x);

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
        beat = getQuantisedBeat(beat);

        m_clickedNote = addNewNote (
            getNoteNumber(e.y),
            m_clickedClip,
            beat - m_clickedClip->getStartBeat().inBeats() + m_clickedClip->getOffsetInBeats().inBeats());

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
void PianoRollContentComponent::mouseDrag(const juce::MouseEvent& e)
{
    m_snap = false;
    if (!e.mods.isShiftDown())
        m_snap = true;

    if (e.mods.isLeftButtonDown() && e.mouseWasDraggedSinceMouseDown())
    {
		setMouseCursor(juce::MouseCursor::NoCursor);
        if (m_clickedNote && m_clickedClip)
        {
            if (m_expandLeft)
                m_leftTimeDelta = getDraggedTimeDelta(e, m_clickedNote->getEditStartTime(*m_clickedClip).inSeconds()); 
            else if (m_expandRight || m_noteAdding)
                m_rightTimeDelta = getDraggedTimeDelta(e, m_clickedNote->getEditEndTime(*m_clickedClip).inSeconds()); 
            else
                moveSelectedNotesToMousePos(e);
        }
        else if (m_lassoTool.isVisible())
            updateLasso(e);

        repaint();
    }
}
void PianoRollContentComponent::mouseUp(const juce::MouseEvent& e)
{
    EngineHelpers::getAudioTrack(getTrack(), m_editViewState)->turnOffGuideNotes();

    auto& um = m_editViewState.m_edit.getUndoManager();

    stopLasso();

    auto targetClip = getMidiClipAt(e.x);

    if (!e.mouseWasDraggedSinceMouseDown() && e.mods.isLeftButtonDown())
    {
        setNoteSelected(m_clickedNote, false);
    }
    else if (targetClip && (areNotesDragged()))
    {
        const double startDelta =
            timeToBeat (m_draggedTimeDelta)  + timeToBeat(m_leftTimeDelta);
        const double lengthDelta =
            timeToBeat (m_leftTimeDelta * (-1)) + timeToBeat(m_rightTimeDelta);

        juce::Array<std::pair<te::MidiNote*, te::MidiClip*>> temp;
        moveSelectedNotesToTemp(startDelta, lengthDelta, temp, e.mods.isCtrlDown ());
        //all notes remain in their clips
        if (m_expandLeft || m_expandRight || m_noteAdding || m_clickedClip == targetClip)
        {
            if (m_expandRight || m_noteAdding)
                for (auto i = temp.size() - 1; i >= 0; i--)
                    insertNote(temp[i].first, temp[i].second);
            else
                for (auto p: temp)
                    insertNote(p.first, p.second);
        }
        //all notes from all clips will go to one target clip
        else
        {
            for (auto p : temp)
            {
                auto clipDelta = targetClip->getStartBeat() - p.second->getStartBeat();
                p.first->state.setProperty(te::IDs::b, p.first->getStartBeat().inBeats() - clipDelta.inBeats(),&um); 

                insertNote(p.first, targetClip);
            }
        }

        m_editViewState.m_selectionManager.addToSelection(*m_selectedEvents);
    }

    cleanUpFlags();
	setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}
void PianoRollContentComponent::mouseExit(const juce::MouseEvent&)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
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
double PianoRollContentComponent::getDraggedTimeDelta(const juce::MouseEvent& e, double oldTime)
{
    auto scroll = beatsToTime(m_editViewState.m_pianoX1);
    auto newTime = oldTime + xToTime(e.getDistanceFromDragStartX()) - scroll;
    if (m_snap)
        newTime = getSnapedTime(newTime);

    return newTime - oldTime;
}
void PianoRollContentComponent::moveSelectedNotesToMousePos(
    const juce::MouseEvent& e)
{
    m_draggedTimeDelta = getDraggedTimeDelta(e, m_clickedNote->getEditStartTime(*m_clickedClip).inSeconds());
    m_draggedNoteDelta = getNoteNumber(e.y) - m_clickedNote->getNoteNumber();
    m_clickedClip->getAudioTrack()->turnOffGuideNotes();

    for (auto n : getSelectedNotes())
       playGuideNote(m_selectedEvents->clipForEvent(n),
                      n->getNoteNumber() + m_draggedNoteDelta,
                      n->getVelocity());
}
double PianoRollContentComponent::getQuantisedBeat(double beat, bool down) const
{
    return m_editViewState.getQuantizedBeat(beat, getBestSnapType(), down);
}
//snapes relative to clip start
double PianoRollContentComponent::getQuantisedNoteBeat(
    double beat,const te::MidiClip* c, bool down) const
{
    auto editBeat = c->getStartBeat().inBeats() + beat;
    return getQuantisedBeat(editBeat, down) - c->getStartBeat().inBeats();
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
double PianoRollContentComponent::getSnapedTime(double time)
{
    return m_editViewState.getSnapedTime(time, getBestSnapType(), false);
}

te::MidiNote* PianoRollContentComponent::addNewNote(int noteNumb,
                                                 const te::MidiClip* clip,
                                                 double beat)
{
    auto length = m_editViewState.m_lastNoteLength <= 0
                      ? 0.25
                      : m_editViewState.m_lastNoteLength;
    cleanUnderNote(noteNumb, {beat, beat + length}, clip);
    return clip->getSequence().addNote(noteNumb,
                                       tracktion::core::BeatPosition::fromBeats(beat),
                                       tracktion::core::BeatDuration::fromBeats(length),
                                       m_editViewState.m_lastVelocity,
                                       111,
                                       &m_editViewState.m_edit.getUndoManager());
}
void PianoRollContentComponent::playGuideNote(
    const te::MidiClip* clip,const int noteNumb, int vel)
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
void PianoRollContentComponent::updateLasso(const juce::MouseEvent& e)
{
    auto top = getYForKey(m_clickedKey);
    m_lassoTool.updateLasso(e.getEventRelativeTo(&m_lassoTool), top);
    updateLassoSelection();
}
void PianoRollContentComponent::stopLasso()
{
    if (m_lassoTool.isVisible())
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        m_lassoTool.stopLasso();
    }
}


void PianoRollContentComponent::moveSelectedNotesToTemp(
    const double startDelta,
    const double lengthDelta,
    juce::Array<std::pair<te::MidiNote*, te::MidiClip*>>& temp,
    bool copy)
{
    auto& um = m_editViewState.m_edit.getUndoManager();

    for (auto n: getSelectedNotes())
    {
        auto clip = m_selectedEvents->clipForEvent(n);

        te::MidiNote* mn = new te::MidiNote(
            te::MidiNote::createNote(*n,
                                     n->getStartBeat() + tracktion::core::BeatDuration::fromBeats(startDelta),
                                     n->getLengthBeats() + tracktion::core::BeatDuration::fromBeats(lengthDelta)));
        mn->setNoteNumber(n->getNoteNumber() + m_draggedNoteDelta, &um);

        std::pair<te::MidiNote*, te::MidiClip*> pair = {mn, clip};
        temp.add(pair);

        if (copy)
            m_selectedEvents->removeSelectedEvent (n);
        else
            clip->getSequence().removeNote(*n, &um);
    }
}
void PianoRollContentComponent::duplicateSelectedNotes()
{
    auto range = m_selectedEvents->getSelectedRange ();
    auto rangeLength = timeToBeat (range.getLength ().inSeconds());
    juce::Array<std::pair<te::MidiNote*, te::MidiClip*>> temp;

    moveSelectedNotesToTemp(rangeLength, 0, temp, true);

    for (auto p: temp)
        insertNote(p.first, p.second);

    m_editViewState.m_selectionManager.addToSelection (*m_selectedEvents);
}
void PianoRollContentComponent::insertNote(te::MidiNote* note, te::MidiClip* clip)
{
    auto& um = m_editViewState.m_edit.getUndoManager();

    cleanUnderNote(note->getNoteNumber(), {note->getStartBeat().inBeats(), note->getEndBeat().inBeats()}, clip);
    auto mn = clip->getSequence().addNote(note->state.getProperty(te::IDs::p),
                                          tracktion::BeatPosition::fromBeats(note->state.getProperty(te::IDs::b)),
                                          tracktion::BeatDuration::fromBeats(note->state.getProperty(te::IDs::l)),
                                          note->state.getProperty(te::IDs::v),
                                          note->state.getProperty(te::IDs::c),
                                          &um);

    m_selectedEvents->addSelectedEvent(mn, true);
	m_editViewState.m_lastNoteLength = note->getLengthBeats().inBeats();
}
void PianoRollContentComponent::snapToGrid(te::MidiNote* note,
                                           const te::MidiClip* clip) const
{
    auto& um = m_editViewState.m_edit.getUndoManager();

    if (m_expandLeft)
        note->setStartAndLength(
                                tracktion::BeatPosition::fromBeats(getQuantisedNoteBeat(note->getStartBeat().inBeats(), clip)),
                                note->getEndBeat() - tracktion::BeatPosition::fromBeats(getQuantisedNoteBeat(note->getStartBeat().inBeats(), clip)),
                                &um);
    else if (m_expandRight || m_noteAdding)
        note->setStartAndLength(note->getStartBeat(),
                                tracktion::BeatPosition::fromBeats(getQuantisedNoteBeat(note->getEndBeat().inBeats(), clip)) - note->getStartBeat(),
                                &um);
    else
        note->setStartAndLength(
            tracktion::BeatPosition::fromBeats(getQuantisedNoteBeat(note->getStartBeat().inBeats(),clip)),
                                note->getLengthBeats(),
                                &um);
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
bool PianoRollContentComponent::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::createFromDescription ("ctrl + d"))
    {
        duplicateSelectedNotes ();
        return true;
    }
    else if (key == juce::KeyPress::deleteKey
             || key == juce::KeyPress::backspaceKey)
    {
        for (auto n : getSelectedNotes ())
            m_selectedEvents->clipForEvent (n)->getSequence ().removeNote
                    (*n, &m_editViewState.m_edit.getUndoManager ());
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription ("cursor up"))
    {
        m_selectedEvents->nudge (getBestSnapType (),0, 1);
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription ("cursor down"))
    {
        if (key.getModifiers ().isCtrlDown ())
            m_selectedEvents->nudge (getBestSnapType (),0, -1);
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription ("cursor left"))
    {
        m_selectedEvents->nudge (getBestSnapType (),-1, 0);
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription ("cursor right"))
    {
        m_selectedEvents->nudge (getBestSnapType (),1, 0);
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription ("ctrl + cursor up"))
    {
        m_selectedEvents->nudge (getBestSnapType (),0, 12);
        return true;
    }
    else if (key == juce::KeyPress::createFromDescription ("ctrl + cursor down"))
    {
        m_selectedEvents->nudge (getBestSnapType (),0, -12);
        return true;
    }


    return false;
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
te::MidiNote* PianoRollContentComponent::getNoteByPos(juce::Point<float> pos)
{
    for (auto& mc: getMidiClipsOfTrack())
    {
        for (auto note: mc->getSequence().getNotes())
        {
            auto clickedBeat = xToBeats((int) pos.x) + mc->getOffsetInBeats().inBeats();
            auto clipStart = mc->getStartBeat().inBeats();
            auto isNoteNum
                = (note->getNoteNumber() == getNoteNumber(static_cast<int> (pos.y)));
            auto noteStart = note->getStartBeat().inBeats() + clipStart;
            auto noteEnd = note->getEndBeat().inBeats() + clipStart;

            if (isNoteNum
                && juce::Range<double> (noteStart, noteEnd).contains(clickedBeat))
                    return note;
        }
    }
    return nullptr;
}
tracktion_engine::MidiClip* PianoRollContentComponent::getMidiClipAt(int x)
{
    for (auto& c: getMidiClipsOfTrack())
        if ((c->getStartBeat().inBeats() < xToBeats(x)) && (c->getEndBeat().inBeats() > xToBeats(x)))
            return c;

    return nullptr;
}
tracktion_engine::Track::Ptr PianoRollContentComponent::getTrack()
{
    return m_track;
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

    return static_cast<int> (y);
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
                    && m_lassoTool.getLassoRect()
                        .m_timeRange.overlaps(midiNote->getEditTimeRange(*clip));
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
    return m_editViewState.timeToX(
        time, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
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

            if (startBeat >= beatRange.getStart() && endBeat <= beatRange.getEnd())
            {
                clip->getSequence().removeNote(*n, um);
                continue;
            }

            if (startBeat >= beatRange.getStart() && endBeat > beatRange.getEnd())
            {
                n->setStartAndLength(tracktion::BeatPosition::fromBeats(beatRange.getEnd()),
                                     tracktion::BeatPosition::fromBeats(endBeat)
                                        - tracktion::BeatPosition::fromBeats(beatRange.getEnd()), um);
                continue;
            }

            if (startBeat < beatRange.getStart() && endBeat <= beatRange.getEnd())
            {
                n->setStartAndLength(tracktion::BeatPosition::fromBeats(startBeat),
                                     tracktion::BeatDuration::fromBeats(beatRange.getStart() - startBeat), um);
                continue;
            }

            if (startBeat < beatRange.getStart() && endBeat > beatRange.getEnd())
            {
                n->setStartAndLength(tracktion::BeatPosition::fromBeats(startBeat),
                                     tracktion::BeatDuration::fromBeats(beatRange.getStart() - startBeat), um);
                clip->getSequence().addNote(noteNumb,
                                            tracktion::BeatPosition::fromBeats(beatRange.getEnd()),
                                            tracktion::BeatDuration::fromBeats(endBeat - beatRange.getEnd()),
                                            n->getVelocity(),
                                            n->getColour(),
                                            um);
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
        if (beatRange.intersects ({ n->getStartBeat().inBeats(), n->getEndBeat().inBeats()}))
                notesInRange.add(n);

    return notesInRange;
}
double PianoRollContentComponent::timeToBeat(double time)
{
    return m_editViewState.timeToBeat(time);
}
te::MidiClip* PianoRollContentComponent::getNearestClipBefore(int x)
{
    if (getMidiClipAt(x) != nullptr)
        return getMidiClipAt(x);

    auto cPtr = getMidiClipsOfTrack().getFirst();

    for (auto c : getMidiClipsOfTrack())
        if (c->getEndBeat().inBeats() < xToBeats(x))
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
        if (c->getStartBeat().inBeats() > xToBeats(x)
            && (clip == nullptr || clip->getStartBeat().inBeats() > c->getStartBeat().inBeats()))
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
    if (m_selectedEvents != nullptr)
        m_selectedEvents->deselect();

    if (getMidiClipsOfTrack().size() > 0)
        m_selectedEvents
            = std::make_unique<te::SelectedMidiEvents>(getMidiClipsOfTrack());
    else
        m_selectedEvents.reset(nullptr);
}
double PianoRollContentComponent::xToTime(const int& x) const
{
    return m_editViewState.xToTime(
        x, getWidth(), m_editViewState.m_pianoX1, m_editViewState.m_pianoX2);
}
bool PianoRollContentComponent::areNotesDragged() const
{
    return m_draggedTimeDelta != 0.0
           || m_draggedNoteDelta != 0
           || m_leftTimeDelta != 0.0
           || m_rightTimeDelta != 0.0;
}
juce::Array<te::MidiNote*> PianoRollContentComponent::getSelectedNotes()
{
    return m_selectedEvents->getSelectedNotes();
}
