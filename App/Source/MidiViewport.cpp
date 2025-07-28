/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/


#include "MidiViewport.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "ToolStrategy.h"

MidiViewport::MidiViewport(
    EditViewState& evs, tracktion_engine::Track::Ptr track, TimeLineComponent& timeLine)
    : m_evs(evs)
    , m_track(std::move(track))
    , m_timeLine(timeLine)
    , m_lassoTool(evs, m_timeLine.getTimeLineID())
{
    m_currentTool = ToolFactory::createTool(Tool::pointer);
    addChildComponent(m_lassoTool);
    updateSelectedEvents();
    
    // Register as listener for ValueTree changes to invalidate clip cache when needed
    if (m_track != nullptr)
        m_track->state.addListener(this);
    
    // setWantsKeyboardFocus (true);
}

MidiViewport::~MidiViewport()
{
    if (m_track != nullptr)
        m_track->state.removeListener(this);
}

void MidiViewport::paintOverChildren(juce::Graphics& g)
{
    m_lassoTool.drawLasso(g);
}

void MidiViewport::paint(juce::Graphics& g)
{
    g.fillAll(m_evs.m_applicationState.getTrackBackgroundColour());
    drawKeyLines(g);

    drawBarsAndBeatLines(g, juce::Colours::black);

    for (auto& midiClip: getCachedMidiClips())
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
void MidiViewport::drawKeyLines(juce::Graphics& g) const
{
    int lastNote = (getHeight() / getKeyWidth()) + getStartKey();

    for (auto i = static_cast<int>(getStartKey()); i <= lastNote; i++)
    {
        g.setColour(juce::MidiMessage::isMidiNoteBlack(i)
                        ? juce::Colour(0x22000000)
                        : juce::Colour(0x22ffffff));
        g.fillRect(getNoteRect(i, 0, getWidth()).reduced(0, 1));
    }
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

    

    noteRect.reduce(2, 2);
    g.setColour(borderColour);
    drawKeyNum(g, n, noteRect);
}
void MidiViewport::drawDraggedNotes(juce::Graphics& g, te::MidiNote* n, te::MidiClip* clip)
{
    auto borderColour = juce::Colour(0xccffffff);

    const double startDelta = m_evs.timeToBeat (m_draggedTimeDelta)  + m_evs.timeToBeat(m_leftTimeDelta);
    const double lengthDelta = m_evs.timeToBeat (m_leftTimeDelta * (-1)) + m_evs.timeToBeat(m_rightTimeDelta);

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
    if (m_evs.getViewYScale(m_timeLine.getTimeLineID()) > 13)
        g.drawText(juce::MidiMessage::getMidiNoteName(
                       n->getNoteNumber() , true, true, 3),
                   noteRect, juce::Justification::centredLeft);

}
juce::Colour MidiViewport::getNoteColour(
    tracktion_engine::MidiClip* const& midiClip,
    tracktion_engine::MidiNote* n)
{
    auto s = EngineHelpers::getNoteStartBeat(midiClip, n);
    auto e = EngineHelpers::getNoteEndBeat(midiClip, n);
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
                                           const tracktion_engine::MidiNote* n)
{
    double sBeat = EngineHelpers::getNoteStartBeat(midiClip, n);
    double eBeat = EngineHelpers::getNoteEndBeat(midiClip, n);
    auto x1 = m_evs.beatsToX(sBeat + midiClip->getStartBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth());
    auto x2 = m_evs.beatsToX(eBeat + midiClip->getStartBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth()) + 1;

    return getNoteRect(n->getNoteNumber(), x1, x2);
}

juce::Rectangle<float>
    MidiViewport::getNoteRect(const int noteNum, int x1, int x2) const
{
    auto yOffset = (float) noteNum - getStartKey() + 1;
    auto noteY = (float) getHeight() - (yOffset * getKeyWidth());
    return {float(x1), float(noteY), float(x2 - x1), float(getKeyWidth())};
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

void MidiViewport::mouseMove(const juce::MouseEvent& e)
{
    if (m_currentTool)
        m_currentTool->mouseMove(e, *this);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiViewport::mouseDown(const juce::MouseEvent& e)
{
    if (m_currentTool)
        m_currentTool->mouseDown(e, *this);
        
    // Handle double click
    if (e.getNumberOfClicks() == 2)
    {
        if (m_currentTool)
            m_currentTool->mouseDoubleClick(e, *this);
    }
}
void MidiViewport::mouseDrag(const juce::MouseEvent& e)
{
    if (m_currentTool)
        m_currentTool->mouseDrag(e, *this);

    repaint();
}
void MidiViewport::mouseUp(const juce::MouseEvent& e)
{
    if (m_currentTool)
        m_currentTool->mouseUp(e, *this);

    repaint();
}
void MidiViewport::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    // Only invalidate cache if a clip was added
    if (parent.getType() == te::IDs::TRACK && child.hasType(te::IDs::MIDICLIP))
    {
        invalidateClipCache();
        repaint();
    }
}

void MidiViewport::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int)
{
    // Only invalidate cache if a clip was removed
    if (parent.getType() == te::IDs::TRACK && child.hasType(te::IDs::MIDICLIP))
    {
        invalidateClipCache();
        repaint();
    }
}

void MidiViewport::cleanUpFlags()
{
    m_draggedNoteDelta = 0;
    m_draggedTimeDelta = 0.0;

    m_leftTimeDelta = 0.0;
    m_rightTimeDelta = 0.0;

    m_clickedNote = nullptr;
    m_clickedClip = nullptr;

    // Clear hover state when cleaning up
    if (m_hoveredNote != nullptr)
    {
        setHovered(m_hoveredNote, false);
        m_hoveredNote = nullptr;
    }

    m_snap = false;
}

void MidiViewport::mouseExit(const juce::MouseEvent &)
{
    // Clear hover state when mouse leaves the component
    if (m_hoveredNote != nullptr)
    {
        setHovered(m_hoveredNote, false);
        m_hoveredNote = nullptr;
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiViewport::setLeftEdgeDraggingTime(const juce::MouseEvent& e)
{
    auto oldTime = m_clickedNote->getEditStartTime(*m_clickedClip).inSeconds();
    auto scroll = m_evs.beatToTime(m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats());
    auto newTime = oldTime + m_evs.xToTime(e.getDistanceFromDragStartX(), m_timeLine.getTimeLineID(), getWidth()) - scroll;
    if (m_snap)
        newTime = getSnapedTime(newTime);

    m_leftTimeDelta = newTime - oldTime;
    m_draggedTimeDelta = 0;
}

void MidiViewport::setRightEdgeDraggingTime(const juce::MouseEvent& e)
{
    auto oldTime = m_clickedNote->getEditEndTime(*m_clickedClip).inSeconds();
    auto scroll = m_evs.beatToTime(m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats());
    auto newTime = oldTime + m_evs.xToTime(e.getDistanceFromDragStartX(), m_timeLine.getTimeLineID(), getWidth()) - scroll;
    if (m_snap)
        newTime = getSnapedTime(newTime);

    m_rightTimeDelta = newTime - oldTime;
    m_draggedTimeDelta = 0;
}

void MidiViewport::updateViewOfMoveSelectedNotes(
    const juce::MouseEvent& e)
{
    auto oldTime = m_clickedNote->getEditStartTime(*m_clickedClip).inSeconds();
    auto scroll = m_evs.beatToTime(m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats());
    auto newTime = oldTime + m_evs.xToTime(e.getDistanceFromDragStartX(), m_timeLine.getTimeLineID(), getWidth()) - scroll;
    if (m_snap)
        newTime = getSnapedTime(newTime);

    //set member
    m_draggedTimeDelta = newTime - oldTime;
    m_draggedNoteDelta = getNoteNumber(e.y) - m_clickedNote->getNoteNumber();

    m_clickedClip->getAudioTrack()->turnOffGuideNotes();

    for (auto n : getSelectedNotes())
       playGuideNote(m_selectedEvents->clipForEvent(n),
                      n->getNoteNumber() + m_draggedNoteDelta,
                      n->getVelocity());

    repaint();
}
double MidiViewport::getQuantisedBeat(double beat, bool down) const
{
    auto snapType = getBestSnapType();
    auto time = m_evs.beatToTime(beat);
    auto snapedTime = m_evs.getSnapedTime(time, snapType, down);
    auto quantisedBeat = m_evs.timeToBeat (snapedTime);

    return quantisedBeat;
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
    auto x1 = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();

    return m_evs.getBestSnapType(x1, x2, getWidth());
}
double MidiViewport::getSnapedTime(double time)
{
    return m_evs.getSnapedTime(time, getBestSnapType(), false);
}

te::MidiNote* MidiViewport::addNewNoteAt(int x, int y, te::MidiClip* clip)
{
    auto noteNum = getKeyForY(y);
    auto beat = m_timeLine.xToBeatPos(x).inBeats();
    // auto beat = m_evs.xToBeats(x, m_timeLine.getTimeLineID(), m_timeLine.getWidth());

    return addNewNote(noteNum, clip, beat - clip->getStartBeat().inBeats());
}

te::MidiNote* MidiViewport::addNewNote(int noteNumb, const te::MidiClip* clip, double beat, double length)
{
    if (length == -1)
        length = m_evs.m_lastNoteLength <= 0
                          ? 0.25
                          : m_evs.m_lastNoteLength;

    cleanUnderNote(noteNumb, {tracktion::BeatPosition::fromBeats(beat), tracktion::BeatDuration::fromBeats(length)}, clip);
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
    auto newNoteLength = m_evs.timeToBeat(note->getEditEndTime(*clip).inSeconds() - time);
    addNewNote(note->getNoteNumber(), clip,  m_evs.timeToBeat(time) - clip->getStartBeat().inBeats(), newNoteLength);
}

float MidiViewport::getKeyWidth() const
{
    return (float) m_evs.getViewYScale(m_timeLine.getTimeLineID());
}
float MidiViewport::getStartKey() const
{
    return (float) m_evs.getViewYScroll(m_timeLine.getTimeLineID());
}
void MidiViewport::startLasso(const juce::MouseEvent& e, bool isRangeTool)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyWidth = m_evs.getViewYScale(m_timeLine.getTimeLineID());

    m_lassoTool.startLasso({e.x, e.y}, (startKey * keyWidth), isRangeTool);
}
void MidiViewport::setNoteSelected(tracktion_engine::MidiNote* n,
                                                bool addToSelection)
{
    m_selectedEvents->addSelectedEvent(n, addToSelection);
    m_evs.m_selectionManager.addToSelection(*m_selectedEvents);
}
void MidiViewport::updateLasso(const juce::MouseEvent& e)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyWidth = m_evs.getViewYScale(m_timeLine.getTimeLineID());
    m_lassoTool.updateLasso({e.x, e.y}, (startKey * keyWidth));
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

void MidiViewport::duplicateSelectedNotes()
{
    auto range = m_selectedEvents->getSelectedRange ();
    auto rangeLength = range.getLength ().inSeconds();

    m_draggedTimeDelta = rangeLength;

    performNoteMoveOrCopy(true);

    cleanUpFlags();
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
        int viewStartX = 0 -
                #if JUCE_MAC
                static_cast<int>(wheel.deltaX * 300);
                #else
                static_cast<int>(wheel.deltaY * 300);
                #endif

        auto newViewStartBeats = m_evs.xToBeats(viewStartX, m_timeLine.getTimeLineID(), m_timeLine.getWidth());
        m_evs.setNewStartAndZoom(m_timeLine.getTimeLineID(), newViewStartBeats);
    }
    else if (event.mods.isCommandDown())
    {
        const float wheelDelta =
                #if JUCE_MAC
                wheel.deltaX * -(m_editViewState.getTimeLineZoomUnit());
                #else
                wheel.deltaY * -(m_evs.getTimeLineZoomUnit());
                #endif

        const auto startBeat = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getStart().inBeats();
        const auto endBeat = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), m_timeLine.getWidth()).getEnd().inBeats();
        const auto xPos = event.getPosition().getX();
        const auto mouseBeat = m_timeLine.xToBeatPos(xPos).inBeats();
        const auto scaleFactor = wheelDelta > 0 ? 1.1 : 0.9;
        const auto newVisibleLengthBeats = juce::jlimit(0.05, 100240.0, (endBeat - startBeat) * scaleFactor);
        const auto newBeatsPerPixel = newVisibleLengthBeats / m_timeLine.getWidth();
        const auto viewCorrect =  (xPos * m_timeLine.getBeatsPerPixel()) - (xPos * newBeatsPerPixel);
        const auto newStartPos = startBeat + viewCorrect;

        m_evs.setNewStartAndZoom(m_timeLine.getTimeLineID(), newStartPos, newBeatsPerPixel);
    }
    else
    {
        scrollPianoRoll((float) wheel.deltaY * 5);
    }
}

void MidiViewport::scrollPianoRoll(float delta)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyWidth = m_evs.getViewYScale(m_timeLine.getTimeLineID());
    m_evs.setYScroll(m_timeLine.getTimeLineID(),
        juce::jlimit(0.f,
                     127.f - (float) (getHeight() / keyWidth),
                     (float) startKey + delta));
}

void MidiViewport::drawBarsAndBeatLines(juce::Graphics& g,
                                                     juce::Colour colour)
{
    auto x1 = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    GUIHelpers::drawBarsAndBeatLines(g, m_evs, x1, x2, getLocalBounds().toFloat());
}
int MidiViewport::getNoteNumber(int y)
{
    auto noteNumb = (int) getKeyForY(y);
    return noteNumb;
}
te::MidiNote* MidiViewport::getNoteByPos(juce::Point<float> pos)
{
    for (auto& mc: getCachedMidiClips())
    {
        for (auto note: mc->getSequence().getNotes())
        {
            auto clickedBeat = m_evs.xToBeats((int) pos.x, m_timeLine.getTimeLineID(), getWidth()) + mc->getOffsetInBeats().inBeats();
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
    for (auto& c: getCachedMidiClips())
        if ((c->getStartBeat().inBeats() < m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth())) && (c->getEndBeat().inBeats() > m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth())))
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
    m_evs.m_selectionManager.addToSelection(m_track);
    repaint();
}
double MidiViewport::getKeyForY(int y)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyHeight = m_evs.getViewYScale(m_timeLine.getTimeLineID());
    auto keyNumb = (double) (startKey
                             + ((double) (getHeight() - y) / keyHeight));

    return keyNumb;
}
int MidiViewport::getYForKey(double key)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyHeight = m_evs.getViewYScale(m_timeLine.getTimeLineID());

    auto y = getHeight() - (keyHeight * (key - startKey));

    return static_cast<int> (y);
}
void MidiViewport::updateLassoSelection()
{
    unselectAll();

    for (auto c : getCachedMidiClips())
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

void MidiViewport::timerCallback()
{
    stopTimer();
    auto at = EngineHelpers::getAudioTrack(getTrack(), m_evs);
    at->turnOffGuideNotes();
}
void MidiViewport::performNoteMoveOrCopy(bool copy)
{
    // A local helper structure to hold all necessary information for a pending note creation.
    struct NoteOperationInfo
    {
        te::MidiClip* targetClip;
        tracktion::BeatPosition startBeat;
        tracktion::BeatDuration length;
        int noteNumber;
        int velocity;
        int colour;
    };

    if (m_selectedEvents == nullptr || m_selectedEvents->getNumSelected() == 0)
        return;

    auto& um = m_evs.m_edit.getUndoManager();
    um.beginNewTransaction(copy ? "Copy MIDI Notes" : "Move MIDI Notes");

    juce::Array<NoteOperationInfo> plannedNotes;
    auto selectedNotes = getSelectedNotes();
    auto& tempoSequence = m_evs.m_edit.tempoSequence;

    // --- PHASE 1: Collect Info & Prepare ---
    for (auto* note : selectedNotes)
    {
        auto* clip = m_selectedEvents->clipForEvent(note);
        if (clip == nullptr) continue;

        // --- KORRIGIERTE DELTA-BERECHNUNG ---
        // A duration in beats depends on the tempo at that point in time.
        // We calculate the delta relative to the note's original start time.
        auto originalNoteStartTime = tempoSequence.toTime(note->getStartBeat());
        auto newNoteStartTime = originalNoteStartTime + tracktion::TimeDuration::fromSeconds(m_draggedTimeDelta);
        auto newNoteStartBeat = tempoSequence.toBeats(newNoteStartTime);
        auto beatDelta = newNoteStartBeat - note->getStartBeat();
        auto lengthDelta = m_evs.timeToBeat(m_leftTimeDelta * (-1) + (m_rightTimeDelta));
        // --- ENDE DER KORREKTUR ---

        plannedNotes.add({
            clip,
            note->getStartBeat() + beatDelta + tracktion::BeatDuration::fromBeats(m_evs.timeToBeat(m_leftTimeDelta)),
            note->getLengthBeats() + tracktion::BeatDuration::fromBeats(lengthDelta), 
            note->getNoteNumber() + m_draggedNoteDelta,
            note->getVelocity(),
            note->getColour()
        });

        if (!copy)
        {
            clip->getSequence().removeNote(*note, &um);
        }
    }

    unselectAll();

    // --- PHASE 2: Clear Target Area ---
    for (const auto& noteInfo : plannedNotes)
    {
        tracktion::BeatRange targetBeatRange(noteInfo.startBeat, noteInfo.startBeat + noteInfo.length);
        cleanUnderNote(noteInfo.noteNumber, targetBeatRange, noteInfo.targetClip);
    }

    // --- PHASE 3: Create New Notes & Update Selection ---
    for (const auto& noteInfo : plannedNotes)
    {
        auto* newNote = noteInfo.targetClip->getSequence().addNote(
            noteInfo.noteNumber,
            noteInfo.startBeat,
            noteInfo.length,
            noteInfo.velocity,
            noteInfo.colour,
            &um
        );
        setNoteSelected(newNote, true);
    }
}

void MidiViewport::cleanUnderNote(int noteNumb, tracktion::BeatRange beatRange, const te::MidiClip* clip)
{
    if (clip == nullptr || beatRange.isEmpty())
        return;

    auto& um = m_evs.m_edit.getUndoManager();
    auto& sequence = clip->getSequence();
    
    // We must iterate over a copy, as we might modify the sequence during the loop.
    auto allNotesInClip = sequence.getNotes();

    for (auto* note : allNotesInClip)
    {
        if (note->getNoteNumber() == noteNumb)
        {
            tracktion::BeatRange noteBeatRange(note->getStartBeat(), note->getEndBeat());

            if (noteBeatRange.intersects(beatRange))
            {
                // To avoid floating point inaccuracies, we can add a tiny epsilon.
                constexpr double epsilon = 0.00001;

                // Case 1: The existing note is completely contained within the clear area.
                if (beatRange.contains(noteBeatRange))
                {
                    sequence.removeNote(*note, &um);
                }
                // Case 2: The clear area splits the existing note.
                else if (noteBeatRange.getStart() < beatRange.getStart() && noteBeatRange.getEnd() > beatRange.getEnd())
                {
                    auto oldEndBeat = note->getEndBeat();

                    // Trim the original note to end where the clear area begins.
                    auto newLength1 = beatRange.getStart() - noteBeatRange.getStart();
                    note->setStartAndLength(note->getStartBeat(), newLength1, &um);


                    // Create a new note for the part after the clear area.
                    sequence.addNote(noteNumb, 
                                     beatRange.getEnd(), 
                                     oldEndBeat - beatRange.getEnd(),
                                     note->getVelocity(), note->getColour(), &um);
                }
                // Case 3: The clear area trims the end of the existing note.
                else if (noteBeatRange.getStart() < beatRange.getStart())
                {
                    auto newLength = beatRange.getStart() - noteBeatRange.getStart();

                    if (newLength.inBeats() > epsilon)
                        note->setStartAndLength(note->getStartBeat(), newLength, &um);
                    else
                        sequence.removeNote(*note, &um);
                }
                // Case 4: The clear area trims the start of the existing note.
                else if (noteBeatRange.getEnd() > beatRange.getEnd())
                {
                    auto newStart = beatRange.getEnd();
                    auto newLength = noteBeatRange.getEnd() - newStart;

                    if (newLength.inBeats() > epsilon)
                        note->setStartAndLength(newStart, newLength, &um);
                    else
                        sequence.removeNote(*note, &um);
                }
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
te::MidiClip* MidiViewport::getNearestClipBefore(int x)
{
    if (getMidiClipAt(x) != nullptr)
        return getMidiClipAt(x);

    auto cPtr = getCachedMidiClips().getFirst();

    for (auto c : getCachedMidiClips())
        if (c->getEndBeat().inBeats() < m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth()))
            if (c->getEndBeat() > cPtr->getEndBeat())
                cPtr = c;

    return cPtr;
}
te::MidiClip* MidiViewport::getNearestClipAfter(int x)
{
    if (getMidiClipAt(x) != nullptr)
        return getMidiClipAt(x);

    te::MidiClip* clip = nullptr;

    for (auto c : getCachedMidiClips())
        if (c->getStartBeat().inBeats() > m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth())
            && (clip == nullptr || clip->getStartBeat().inBeats() > c->getStartBeat().inBeats()))
                clip = c;

    return clip;
}

juce::Rectangle<float> MidiViewport::getClipRect(te::Clip* clip)
{
    auto clipX = static_cast<float>(m_evs.beatsToX(clip->getStartBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth()));
    auto clipW = static_cast<float>(m_evs.beatsToX(clip->getEndBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth()) - clipX);

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

    if (getCachedMidiClips().size() > 0)
        m_selectedEvents
            = std::make_unique<te::SelectedMidiEvents>(getCachedMidiClips());
    else
        m_selectedEvents.reset(nullptr);
}

void MidiViewport::refreshClipCache()
{
    GUIHelpers::log("MidiViewComponent: Clip Cache refreshed!");
    m_cachedClips.clear();
    if (m_track != nullptr)
    {
        m_cachedClips = EngineHelpers::getMidiClipsOfTrack(*m_track);
        m_clipCacheValid = true;
    }
    else
    {
        m_clipCacheValid = false;
    }
}

const juce::Array<te::MidiClip*>& MidiViewport::getCachedMidiClips()
{
    if (!m_clipCacheValid || m_track == nullptr)
    {
        refreshClipCache();
    }
    return m_cachedClips;
}

void MidiViewport::invalidateClipCache()
{
    m_clipCacheValid = false;
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
void MidiViewport::setTool(Tool tool)
{
    if (m_currentTool)
        m_currentTool->toolDeactivated(*this);

    m_currentTool = ToolFactory::createTool(tool);

    if (m_currentTool)
        m_currentTool->toolActivated(*this);
}


