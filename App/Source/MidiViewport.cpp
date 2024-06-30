
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MidiViewport.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "juce_core/system/juce_PlatformDefs.h"

MidiViewport::MidiViewport(
    EditViewState& evs, tracktion_engine::Track::Ptr track)
    : m_evs(evs)
    , m_track(std::move(track))
    , m_lassoTool(evs, evs.m_pianoX1, evs.m_pianoX2)
{
    addChildComponent(m_lassoTool);
    updateSelectedEvents();
    // setWantsKeyboardFocus (true);
}

MidiViewport::~MidiViewport() = default;

void MidiViewport::paint(juce::Graphics& g)
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
void MidiViewport::resized()
{
    auto area = getLocalBounds();
    m_lassoTool.setBounds(area);
}
void MidiViewport::drawNote(juce::Graphics& g,
                                         tracktion_engine::MidiClip* const& midiClip,
                                         tracktion_engine::MidiNote* n
                                         )
{
    
    auto noteRect = getNoteRect(midiClip, n);
    auto visibleRect = noteRect;
    auto leftInvisible = std::abs(noteRect.getX()) - 2;
    auto rightOffset = noteRect.getRight() - getWidth() - 2;
    
    if (visibleRect.getX() < -2)
        visibleRect.removeFromLeft(leftInvisible);

    if (visibleRect.getRight() > getWidth() + 2)
        visibleRect.removeFromRight(rightOffset);

    if (m_evs.m_editNotesOutsideClipRange == false)
    {
        auto clipRect = getClipRect(midiClip);
        // GUIHelpers::log("cipRect Right: ", clipRect.getRight());
        // GUIHelpers::log("visibleRect: ", visibleRect.getRight());
        visibleRect = visibleRect.getIntersection(getClipRect(midiClip));
    }

    auto noteColor = getNoteColour(midiClip, n);
    auto innerGlow = noteColor.brighter(0.5f);
    auto selectedColour = juce::Colour(0xccffffff);
    auto borderColour = juce::Colour(0xff000000);

    g.setColour(borderColour);
    g.fillRect(visibleRect );

    g.setColour(innerGlow);
    visibleRect.reduce(1, 1);
    g.fillRect(visibleRect );

    g.setColour(noteColor);
    visibleRect.reduce(1, 1);
    g.fillRect(visibleRect );

    if (isSelected(n))
    {
        g.setColour(selectedColour);
        g.drawRect(visibleRect .expanded(2, 2));
    }

    if (isHovered(n) && m_toolMode == Tool::knife)
    {
        auto time = m_snap ? m_hoveredTime : getSnapedTime(m_hoveredTime);
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(timeToX(time), visibleRect.getY(), visibleRect.getBottom());
    }

    noteRect.reduce(2, 2);
    g.setColour(borderColour);
    drawKeyNum(g, n, noteRect);
}
void MidiViewport::drawDraggedNotes(juce::Graphics& g, te::MidiNote* n, te::MidiClip* clip)
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
void MidiViewport::drawKeyNum(juce::Graphics& g,
                                           const tracktion_engine::MidiNote* n,
                                           juce::Rectangle<float>& noteRect) const
{
    if (m_evs.m_pianoKeyWidth > 13)
        g.drawText(juce::MidiMessage::getMidiNoteName(
                       n->getNoteNumber() , true, true, 3),
                   noteRect, juce::Justification::centredLeft);

}
juce::Colour MidiViewport::getNoteColour(
    tracktion_engine::MidiClip* const& midiClip,
    tracktion_engine::MidiNote* n)
{
    auto s = getNoteStartBeat(midiClip, n);
    auto e = getNoteEndBeat(midiClip, n);
    bool isBeforeClipStart = s < 0;
    bool isAfterClipEnd = e > midiClip->getEndBeat().inBeats() - midiClip->getStartBeat().inBeats() + 0.00001;

    if (isBeforeClipStart || isAfterClipEnd)
        return juce::Colours::grey;
    else if (isHovered(n))
    {
        GUIHelpers::log(juce::MidiMessage::getMidiNoteName(n->getNoteNumber(), true, true, 3));
        return m_track->getColour().brighter(0.6);
    }

    return m_track->getColour().darker(1.f - getVelocity(n));
}

float MidiViewport::getVelocity(const tracktion_engine::MidiNote* note)
{
    return juce::jmap((float) note->getVelocity(), 0.f, 127.f, 0.f, 1.f);
}

juce::Rectangle<float>
    MidiViewport::getNoteRect(te::MidiClip* const& midiClip,
                                           const te::MidiNote* n)
{
    double sBeat = getNoteStartBeat(midiClip, n);
    double eBeat = getNoteEndBeat(midiClip, n);
    auto x1 = beatsToX(sBeat + midiClip->getStartBeat().inBeats());
    auto x2 = beatsToX(eBeat + midiClip->getStartBeat().inBeats()) + 1;

    return getNoteRect(n->getNoteNumber(), x1, x2);
}

juce::Rectangle<float>
    MidiViewport::getNoteRect(const int noteNum, int x1, int x2) const
{
    auto yOffset = (float) noteNum - getStartKey() + 1;
    auto noteY = (float) getHeight() - (yOffset * getKeyWidth());
    return {float(x1), float(noteY), float(x2 - x1), float(getKeyWidth())};
}

double MidiViewport::getNoteEndBeat(const te::MidiClip* midiClip,
                                                 const te::MidiNote* n)
{
    auto eBeat = n->getEndBeat() - midiClip->getOffsetInBeats();
    return eBeat.inBeats();
}
double MidiViewport::getNoteStartBeat(const te::MidiClip* midiClip,
                                                   const te::MidiNote* n)
{
    auto sBeat = n->getStartBeat() - midiClip->getOffsetInBeats();
    return sBeat.inBeats();
}
void MidiViewport::drawClipRange(
    juce::Graphics& g, tracktion_engine::MidiClip* const& midiClip)
{
    auto clipRect = getClipRect(midiClip);
    auto clipStartX = static_cast<int>(clipRect.getX());
    auto clipEndX = static_cast<int>(clipRect.getRight());
    auto clipColour = midiClip->getTrack()->getColour();

    g.setColour(clipColour);
    g.drawVerticalLine(clipStartX, 0, static_cast<float> (getHeight()));
    g.drawVerticalLine(clipEndX, 0, static_cast<float> (getHeight()));
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRect(clipStartX + 1, 0, clipEndX - clipStartX - 2, getHeight());
    g.setColour(midiClip->getColour().withAlpha(0.1f));
    g.fillRect(clipStartX + 1, 0, clipEndX - clipStartX - 2, getHeight());
}
void MidiViewport::drawKeyLines(juce::Graphics& g) const
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

void MidiViewport::mouseMove(const juce::MouseEvent& e)
{
    for (auto c : getMidiClipsOfTrack())
        for (auto n : c->getSequence().getNotes())
             setHovered(n, false);



    setMouseCursor(getRecommendedMouseCursor()) ;

    m_snap = false;
    if (e.mods.isShiftDown())
        m_snap = true;
    m_hoveredTime = xToTime(e.x);

    if (auto mc = getMidiClipAt(e.x))
    {

        if (auto note = getNoteByPos(e.position))
        {
            setHovered(note, true);

            auto startX = beatsToX(note->getStartBeat().inBeats()
                                   + mc->getStartBeat().inBeats() - mc->getOffsetInBeats().inBeats());
            auto endX = beatsToX(note->getEndBeat().inBeats()
                                 + mc->getStartBeat().inBeats() - mc->getOffsetInBeats().inBeats());

            if (m_toolMode == Tool::pointer)
            {
                auto borderWidth = getNoteRect(mc, note).getWidth() > 30 ? 10 : getNoteRect(mc, note).getWidth() / 3;

                if (e.x < startX + borderWidth)
                {
                    setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, *this));
                    m_expandLeft = true;
                    m_expandRight = false;
                }
                else if (e.x > endX - borderWidth)
                {
                    setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, *this));
                    m_expandLeft = false;
                    m_expandRight = true;
                }
                else
                {
                    setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand,*this));
                    m_expandLeft = false;
                    m_expandRight = false;
                }
            }
        }
    }

    getParentComponent()->mouseMove(e);
}

void MidiViewport::mouseDown(const juce::MouseEvent& e)
{
    m_noteAdding = false;

    m_clickedNote = getNoteByPos(e.position);
    m_clickedKey = getKeyForY(e.y);
    m_clickedClip = getMidiClipAt(e.x);
    
    
    auto isDrawMode = m_clickedClip
                    && e.mods.isLeftButtonDown()
                    && (e.getNumberOfClicks() > 1 || e.mods.isShiftDown() || m_toolMode == Tool::draw);

    const auto clickedBeat = xToBeats(e.x);

    if (m_clickedNote && m_clickedClip)
    {
        if (e.mods.isRightButtonDown() || m_toolMode == Tool::eraser)
        {
            if (!isSelected(m_clickedNote))
                setNoteSelected(m_clickedNote, false);
            setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Erasor, *this));
            deleteSelectedNotes();
        }
        else if (m_toolMode == Tool::knife)
        {
            auto time = m_snap ? m_hoveredTime : getSnapedTime(m_hoveredTime);
            splitNoteAt(m_clickedClip, m_clickedNote, time);
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
    else if (isDrawMode)
    {
        setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Draw, *this));
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
void MidiViewport::mouseDrag(const juce::MouseEvent& e)
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
                updateViewOfMoveSelectedNotes(e);
        }
        else if (m_lassoTool.isVisible())
            updateLasso(e);

        repaint();
    }
}
void MidiViewport::mouseUp(const juce::MouseEvent& e)
{
    EngineHelpers::getAudioTrack(getTrack(), m_evs)->turnOffGuideNotes();

    auto& um = m_evs.m_edit.getUndoManager();

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

        moveSelectedNotesToTemp(startDelta, lengthDelta, e.mods.isCtrlDown ());
        for (auto n : m_temp)
            insertNote(*n);

        m_evs.m_selectionManager.addToSelection(*m_selectedEvents);
    }

    cleanUpFlags();
    setMouseCursor(getRecommendedMouseCursor());
    repaint();
}
void MidiViewport::mouseExit(const juce::MouseEvent&)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiViewport::cleanUpFlags()
{
    m_draggedNoteDelta = 0;
    m_draggedTimeDelta = 0.0;

    m_leftTimeDelta = 0.0;
    m_rightTimeDelta = 0.0;

    m_clickedNote = nullptr;
    m_clickedClip = nullptr;

    m_snap = false;
}
double MidiViewport::getDraggedTimeDelta(const juce::MouseEvent& e, double oldTime)
{
    auto scroll = beatsToTime(m_evs.m_pianoX1);
    auto newTime = oldTime + xToTime(e.getDistanceFromDragStartX()) - scroll;
    if (m_snap)
        newTime = getSnapedTime(newTime);

    return newTime - oldTime;
}
void MidiViewport::updateViewOfMoveSelectedNotes(
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
double MidiViewport::getQuantisedBeat(double beat, bool down) const
{
    return m_evs.getQuantizedBeat(beat, getBestSnapType(), down);
}
//snapes relative to clip start
double MidiViewport::getQuantisedNoteBeat(
    double beat,const te::MidiClip* c, bool down) const
{
    auto editBeat = c->getStartBeat().inBeats() + beat;
    return getQuantisedBeat(editBeat, down) - c->getStartBeat().inBeats();
}
te::TimecodeSnapType MidiViewport::getBestSnapType() const
{
    return m_evs.getBestSnapType(
        m_evs.m_pianoX1, m_evs.m_pianoX2, getWidth());
}
double MidiViewport::xToBeats(const int& x) const
{
    return m_evs.xToBeats(
        x, getWidth(), m_evs.m_pianoX1, m_evs.m_pianoX2);
}
double MidiViewport::getSnapedTime(double time)
{
    return m_evs.getSnapedTime(time, getBestSnapType(), false);
}
te::MidiClip* MidiViewport::getMidiClipForNote(MidiNote note)
{
    for (int i = 0; i < m_track->getNumTrackItems(); ++i)
    {
        if (m_track->getTrackItem(i)->itemID == note.clip)
        {
            if (auto mc = dynamic_cast<te::MidiClip*>(m_track->getTrackItem(i)))
            {
                return mc;
            }
        }
    }
    return nullptr;
}

te::MidiNote* MidiViewport::addNewNote(int noteNumb, const te::MidiClip* clip, double beat, double length)
{
    if (length == -1)
        length = m_evs.m_lastNoteLength <= 0
                          ? 0.25
                          : m_evs.m_lastNoteLength;

    cleanUnderNote(noteNumb, {beat, beat + length}, clip);
    return clip->getSequence().addNote(noteNumb,
                                       tracktion::core::BeatPosition::fromBeats(beat),
                                       tracktion::core::BeatDuration::fromBeats(length),
                                       m_evs.m_lastVelocity,
                                       111,
                                       &m_evs.m_edit.getUndoManager());
}

void MidiViewport::playGuideNote(
    const te::MidiClip* clip,const int noteNumb, int vel)
{
        clip->getAudioTrack()->playGuideNote(
            noteNumb, clip->getMidiChannel(), vel, false, true);
        startTimer(100);
}

void MidiViewport::removeNote(te::MidiClip* clip, te::MidiNote* note)
{
    clip->getSequence().removeNote(*note,
                                   &m_evs.m_edit.getUndoManager());
}


void MidiViewport::splitNoteAt(te::MidiClip* clip, te::MidiNote* note, double time)
{
    auto newNoteLength = timeToBeat(note->getEditEndTime(*clip).inSeconds() - time);
    addNewNote(note->getNoteNumber(), clip,  timeToBeat(time) - clip->getStartBeat().inBeats(), newNoteLength);
}

float MidiViewport::getKeyWidth() const
{
    return (float) m_evs.m_pianoKeyWidth;
}
float MidiViewport::getStartKey() const
{
    return (float) m_evs.m_pianoStartKey;
}
void MidiViewport::startLasso(const juce::MouseEvent& e, bool isRangeTool)
{
    m_lassoTool.startLasso({e.x, e.y}, (m_evs.m_pianoStartKey * m_evs.m_pianoKeyWidth), isRangeTool);
}
void MidiViewport::setNoteSelected(tracktion_engine::MidiNote* n,
                                                bool addToSelection)
{
    m_selectedEvents->addSelectedEvent(n, addToSelection);
    m_evs.m_selectionManager.addToSelection(*m_selectedEvents);
}
void MidiViewport::updateLasso(const juce::MouseEvent& e)
{
    m_lassoTool.updateLasso({e.x, e.y}, (m_evs.m_pianoStartKey * m_evs.m_pianoKeyWidth));
    updateLassoSelection();
}
void MidiViewport::stopLasso()
{
    if (m_lassoTool.isVisible())
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        m_lassoTool.stopLasso();
    }
}


void MidiViewport::moveSelectedNotesToTemp(
    const double startDelta,
    const double lengthDelta,
    bool copy)
{
    auto& um = m_evs.m_edit.getUndoManager();

    m_temp.clear();

    for (auto n: getSelectedNotes())
    {
        auto clip = m_selectedEvents->clipForEvent(n)->itemID;

        auto midiC = std::make_unique<MidiNote>();
        midiC->startBeat = n->getStartBeat() + tracktion::core::BeatDuration::fromBeats(startDelta);
        midiC->Lenght = n->getLengthBeats() + tracktion::core::BeatDuration::fromBeats(lengthDelta);
        midiC->clip = clip;
        midiC->noteNumber = n->getNoteNumber() + m_draggedNoteDelta;
        midiC->colourIndex = n->state.getProperty(tracktion::IDs::c);
        midiC->velocity = n->state.getProperty(tracktion::IDs::v);

        m_temp.add(std::move(midiC));

        if (copy)
            m_selectedEvents->removeSelectedEvent (n);
        else
            m_selectedEvents->clipForEvent(n)->getSequence().removeNote(*n, &um);
    }
}
void MidiViewport::duplicateSelectedNotes()
{
    auto range = m_selectedEvents->getSelectedRange ();
    auto rangeLength = timeToBeat (range.getLength ().inSeconds());

    moveSelectedNotesToTemp(rangeLength, 0, true);

    for (auto n: m_temp)
        insertNote(*n);

    m_evs.m_selectionManager.addToSelection (*m_selectedEvents);
}
void MidiViewport::insertNote(MidiNote note)
{
    auto& um = m_evs.m_edit.getUndoManager();

    if (auto clip = getMidiClipForNote(note))
    {
        auto clipStartBeat = clip->getStartBeat().inBeats();
        auto clipOffset = clip->getOffsetInBeats().inBeats();

        auto noteStartBeat = note.startBeat.inBeats();
        auto noteEndBeat = note.startBeat.inBeats() + note.Lenght.inBeats();

        auto clipStartBoundary = 0.0;

        if (noteStartBeat < clipStartBoundary)
        {
            auto newNoteLength = noteEndBeat - clipStartBoundary;
            noteStartBeat = clipStartBoundary;

            if (newNoteLength < 0)
            {
                return;
            }

            cleanUnderNote(note.noteNumber, {noteStartBeat, noteStartBeat + newNoteLength}, clip);
            auto mn = clip->getSequence().addNote(note.noteNumber,
                                                  tracktion::BeatPosition::fromBeats(noteStartBeat),
                                                  tracktion::BeatDuration::fromBeats(newNoteLength),
                                                  note.velocity,
                                                  note.colourIndex,
                                                  &um);
            m_selectedEvents->addSelectedEvent(mn, true);
            m_evs.m_lastNoteLength = newNoteLength;
        }
        else
        {
            cleanUnderNote(note.noteNumber, {noteStartBeat, noteEndBeat}, clip);
            auto mn = clip->getSequence().addNote(note.noteNumber,
                                                  tracktion::BeatPosition::fromBeats(noteStartBeat),
                                                  note.Lenght,
                                                  note.velocity,
                                                  note.colourIndex,
                                                  &um);
            m_selectedEvents->addSelectedEvent(mn, true);
            m_evs.m_lastNoteLength = note.Lenght.inBeats();
        }
    }
 }
void MidiViewport::snapToGrid(te::MidiNote* note,
                                           const te::MidiClip* clip) const
{
    auto& um = m_evs.m_edit.getUndoManager();

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
void MidiViewport::mouseWheelMove(const juce::MouseEvent& event,
                                               const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isShiftDown())
    {
        auto deltaX1 = event.mods.isCtrlDown() ? wheel.deltaY : -wheel.deltaY;
        auto deltaX2 = -wheel.deltaY;

        m_evs.m_pianoX1 =
            juce::jmax(0.0, m_evs.m_pianoX1 + deltaX1);
        m_evs.m_pianoX2 = m_evs.m_pianoX2 + deltaX2;
    }
    else
    {
        scrollPianoRoll((float) wheel.deltaY * 5);
    }
}

void MidiViewport::scrollPianoRoll(float delta)
{
    m_evs.m_pianoStartKey =
        juce::jlimit(0.f,
                     127.f - (float) (getHeight() / m_evs.m_pianoKeyWidth),
                     (float) m_evs.m_pianoStartKey + delta);
}
juce::Array<te::MidiClip*> MidiViewport::getMidiClipsOfTrack()
{
    juce::Array<te::MidiClip*> midiClips;

    if (auto at = dynamic_cast<te::AudioTrack*>(&(*m_track)))
        for (auto c: at->getClips())
            if (auto mc = dynamic_cast<te::MidiClip*>(c))
                midiClips.add(mc);

    return midiClips;
}
void MidiViewport::drawBarsAndBeatLines(juce::Graphics& g,
                                                     juce::Colour colour)
{
    g.setColour(colour);
    double x1 = m_evs.m_pianoX1;
    double x2 = m_evs.m_pianoX2;
    GUIHelpers::drawBarsAndBeatLines(g, m_evs, x1, x2, getLocalBounds());
}
int MidiViewport::getNoteNumber(int y)
{
    auto noteNumb = (int) getKeyForY(y);
    return noteNumb;
}
te::MidiNote* MidiViewport::getNoteByPos(juce::Point<float> pos)
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
tracktion_engine::MidiClip* MidiViewport::getMidiClipAt(int x)
{
    for (auto& c: getMidiClipsOfTrack())
        if ((c->getStartBeat().inBeats() < xToBeats(x)) && (c->getEndBeat().inBeats() > xToBeats(x)))
            return c;

    return nullptr;
}
tracktion_engine::Track::Ptr MidiViewport::getTrack()
{
    return m_track;
}
void MidiViewport::unselectAll()
{
    m_evs.m_selectionManager.deselectAll();
    repaint();
}
double MidiViewport::getKeyForY(int y)
{
    auto keyHeight = (double) m_evs.m_pianoKeyWidth;
    auto keyNumb = (double) (m_evs.m_pianoStartKey
                             + ((double) (getHeight() - y) / keyHeight));

    return keyNumb;
}
int MidiViewport::getYForKey(double key)
{
    auto keyHeight = (double) m_evs.m_pianoKeyWidth;
    auto firstVisibleKey = (double) m_evs.m_pianoStartKey;

    auto y = getHeight() - (keyHeight * (key - firstVisibleKey));

    return static_cast<int> (y);
}
void MidiViewport::updateLassoSelection()
{
    unselectAll();

    for (auto c : getMidiClipsOfTrack())
        for (auto n : c->getSequence().getNotes())
            if (isInLassoRange(c, n))
                m_selectedEvents->addSelectedEvent(n, true);

    m_evs.m_selectionManager.addToSelection(*m_selectedEvents);
}
bool MidiViewport::isInLassoRange(
    const te::MidiClip* clip, const tracktion_engine::MidiNote* midiNote)
{
    auto verticalKeyRange = juce::Range<double> ((double) midiNote->getNoteNumber()
        ,(double) midiNote->getNoteNumber() + 1);

    return getLassoVerticalKeyRange().intersects(verticalKeyRange)
                    && m_lassoTool.getLassoRect()
                        .m_timeRange.overlaps(midiNote->getEditTimeRange(*clip));
}
int MidiViewport::beatsToX(double beats)
{
    return m_evs.beatsToX(
        beats, getWidth(), m_evs.m_pianoX1, m_evs.m_pianoX2);
}
void MidiViewport::deleteSelectedNotes()
{
    for (auto n : getSelectedNotes())
        m_selectedEvents->clipForEvent(n)->getSequence()
            .removeNote(*n, &m_evs.m_edit.getUndoManager());
}
bool MidiViewport::isSelected(tracktion_engine::MidiNote* note)
{
    return m_selectedEvents->isSelected(note);
}
double MidiViewport::timeToX(const double& time) const
{
    return m_evs.timeToX(
        time, getWidth(), m_evs.m_pianoX1, m_evs.m_pianoX2);
}
double MidiViewport::beatsToTime(double beats)
{
    return m_evs.beatToTime(beats);
}
void MidiViewport::timerCallback()
{
    stopTimer();
    auto at = EngineHelpers::getAudioTrack(getTrack(), m_evs);
    at->turnOffGuideNotes();
}
void MidiViewport::cleanUnderNote(int noteNumb, juce::Range<double> beatRange,
                                               const te::MidiClip* clip)
{
    for (auto n : getNotesInRange(beatRange, clip))
    {
        if (n->getNoteNumber() == noteNumb)
        {
            double startBeat = getNoteStartBeat(clip, n);
            double endBeat = getNoteEndBeat(clip, n);

            juce::UndoManager* um = &m_evs.m_edit.getUndoManager();

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
    MidiViewport::getNotesInRange(juce::Range<double> beatRange,
                                               const te::MidiClip* clip)
{
    juce::Array<te::MidiNote*> notesInRange;

    for(auto n : clip->getSequence().getNotes())
        if (beatRange.intersects ({ n->getStartBeat().inBeats(), n->getEndBeat().inBeats()}))
                notesInRange.add(n);

    return notesInRange;
}
double MidiViewport::timeToBeat(double time)
{
    return m_evs.timeToBeat(time);
}
te::MidiClip* MidiViewport::getNearestClipBefore(int x)
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
te::MidiClip* MidiViewport::getNearestClipAfter(int x)
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

juce::Rectangle<float> MidiViewport::getClipRect(te::Clip* clip)
{
    auto clipX = static_cast<float>(beatsToX(clip->getStartBeat().inBeats()));
    auto clipW = static_cast<float>(beatsToX(clip->getEndBeat().inBeats()) - clipX);

    auto clipY = static_cast<float>(getYForKey(127.0));
    auto clipH = static_cast<float>(getYForKey(0.0) - clipY);

    return {clipX, clipY, clipW, clipH};
}

juce::Range<double> MidiViewport::getLassoVerticalKeyRange()
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
void MidiViewport::updateSelectedEvents()
{
    if (m_selectedEvents != nullptr)
        m_selectedEvents->deselect();

    if (getMidiClipsOfTrack().size() > 0)
        m_selectedEvents
            = std::make_unique<te::SelectedMidiEvents>(getMidiClipsOfTrack());
    else
        m_selectedEvents.reset(nullptr);
}



// -----------------------------------------------------------------------------
double MidiViewport::xToTime(const int& x) const
{
    return m_evs.xToTime(
        x, getWidth(), m_evs.m_pianoX1, m_evs.m_pianoX2);
}
bool MidiViewport::areNotesDragged() const
{
    return m_draggedTimeDelta != 0.0
           || m_draggedNoteDelta != 0
           || m_leftTimeDelta != 0.0
           || m_rightTimeDelta != 0.0;
}

bool MidiViewport::isHovered(te::MidiNote* note)
{
    return static_cast<bool>(note->state.getProperty(IDs::isHovered));
}

void MidiViewport::setHovered(te::MidiNote* note, bool hovered)
{
    note->state.setProperty(IDs::isHovered, hovered, nullptr);
}

juce::Array<te::MidiNote*> MidiViewport::getSelectedNotes()
{
    return m_selectedEvents->getSelectedNotes();
}
juce::MouseCursor MidiViewport::getRecommendedMouseCursor()
{
    juce::MouseCursor cursor = juce::MouseCursor::CrosshairCursor;

    if (m_toolMode == Tool::draw)
        cursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Draw, *this);
    else if (m_toolMode == Tool::knife)
        cursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Split, *this);
    else if (m_toolMode == Tool::lasso)
        cursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Lasso, *this);
    else if (m_toolMode == Tool::range)
        cursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Range, *this);
    else if (m_toolMode == Tool::eraser)
        cursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Erasor, *this);

    return cursor;
}
