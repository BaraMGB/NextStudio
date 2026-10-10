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
#include "MidiNoteCreation.h"
#include "MidiNoteOverlap.h"
#include "ToolStrategy.h"
#include "DrawTool.h"
#include "KnifeTool.h"
#include "PointerTool.h"
#include "EditViewState.h"
#include "PianoRollNoteLength.h"
#include "Utilities.h"

#include <map>

MidiViewport::MidiViewport(EditViewState &evs, tracktion_engine::Track::Ptr track, TimeLineComponent &timeLine)
    : m_evs(evs),
      m_track(std::move(track)),
      m_timeLine(timeLine)
{
    setWantsKeyboardFocus(true);
    m_currentTool = ToolFactory::createTool(Tool::pointer, m_evs);
    m_lassoComponent.setProjection([this](juce::Point<double> point)
    {
        return juce::Point<float>{m_timeLine.beatsToX(point.x),
            float(getHeight() - (point.y - getStartKey()) * getKeyWidth())};
    });
    updateSelectedEvents();

    // Register as listener for ValueTree changes to invalidate clip cache when needed
    if (m_track != nullptr)
        m_track->state.addListener(this);
}

MidiViewport::~MidiViewport()
{
    clearNoteInteractionPreview();
    if (m_selectedEvents != nullptr)
        m_selectedEvents->removeChangeListener(this);

    if (m_track != nullptr)
        m_track->state.removeListener(this);
}

void MidiViewport::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == m_selectedEvents.get())
    {
        if (m_noteInteractionActive && !m_committingInteraction && m_clickedNote != nullptr
            && (!m_noteInteractionSource.isAChildOf(m_track->state)
                || !m_selectedEvents->isSelected(m_clickedNote)))
            m_currentTool->toolDeactivated(*this);
        sendChangeMessage();
    }
}

void MidiViewport::paintOverChildren(juce::Graphics &g)
{
    m_lassoComponent.drawLasso(g);
    if (m_timeRangeGesture.active())
    {
        const auto beats = m_timeRangeGesture.interval();
        const auto pitches = m_timeRangeGesture.lanes();
        const auto x1 = m_timeLine.beatsToX(beats.getStart()), x2 = m_timeLine.beatsToX(beats.getEnd());
        const auto y1 = float(getHeight() - (pitches.getEnd() - getStartKey()) * getKeyWidth());
        const auto y2 = float(getHeight() - (pitches.getStart() - getStartKey()) * getKeyWidth());
        g.setColour(m_evs.m_applicationState.getPrimeColour().withAlpha(0.15f));
        g.fillRect(juce::Rectangle<float>{x1, y1, x2 - x1, y2 - y1});
    }
    m_timeLine.drawMouseFeedback(g, TimelineFeedbackOwner::notes);
}

void MidiViewport::publishNoteInteractionPreview(NoteTimingPreview preview, const TimelineSnapResult& snap,
                                                juce::Range<float> vertical, float markerY, juce::ValueTree source)
{
    m_noteInteractionActive = true;
    m_noteInteractionSource = source;
    if (m_timeLine.onNoteInteractionPreview)
        m_timeLine.onNoteInteractionPreview(preview);
    m_timeLine.setMouseFeedback(TimelineInteractionFeedback{snap, TimelineFeedbackOwner::notes, vertical, markerY},
        [safe = juce::Component::SafePointer<MidiViewport>(this)]
        { if (safe) safe->refreshMouseSnapContext(); });
}

void MidiViewport::clearNoteInteractionPreview()
{
    if (m_noteInteractionActive)
    {
        m_noteInteractionActive = false;
        m_noteInteractionSource = {};
        if (m_timeLine.onNoteInteractionPreview)
            m_timeLine.onNoteInteractionPreview({});
    }
    m_timeLine.clearMouseFeedback(TimelineFeedbackOwner::notes);
}

void MidiViewport::paint(juce::Graphics &g)
{
    g.fillAll(m_evs.m_applicationState.getTrackBackgroundColour());
    drawKeyLines(g);

    drawBarsAndBeatLines(g, juce::Colours::black);

    for (auto &midiClip : getCachedMidiClips())
    {
        drawClipRange(g, midiClip);

        auto &seq = midiClip->getSequence();

        for (auto n : seq.getNotes())
            drawNote(g, midiClip, n);
    }

    drawPendingPasteNotes(g);
    drawNotePropertyPreview(g);

    if (auto *pointerTool = dynamic_cast<PointerTool *>(m_currentTool.get()))
    {
        if (pointerTool->isDragging())
        {
            auto selectedNotes = getSelectedNotes();
            for (auto sn : selectedNotes)
            {
                if (m_selectedEvents)
                    drawDraggedNotes(g, sn, m_selectedEvents->clipForEvent(sn));
            }
        }
    }

    if (auto *drawTool = dynamic_cast<DrawTool *>(m_currentTool.get()))
    {
        if (drawTool->isDrawing() && getCachedMidiClips().contains(drawTool->getClickedClip()))
        {
            g.setColour(juce::Colours::white.withAlpha(0.5f));

            const auto startX = m_timeLine.beatsToX(drawTool->getDrawStartBeat());
            const auto endX = m_timeLine.beatsToX(drawTool->getDrawEndBeat());

            auto noteRect = getNoteRect(drawTool->getDrawNoteNumber(), startX, endX);
            g.drawRect(noteRect, 1.0f);
        }
    }

    if (auto *knifeTool = dynamic_cast<KnifeTool *>(m_currentTool.get()))
    {
        if (knifeTool->shouldDrawSplitLine())
        {
            // Use the note that was already detected in the KnifeTool
            if (auto *note = knifeTool->getHoveredNote())
            {
                if (auto *clip = m_selectedEvents->clipForEvent(note))
                {
                    auto noteRect = getNoteRect(clip, note);

                    g.setColour(juce::Colours::white);
                    auto lineX = knifeTool->getSplitLineX();
                    g.drawLine(lineX, noteRect.getY(), lineX, noteRect.getBottom(), 1.0f);
                }
            }
        }
    }
}

void MidiViewport::drawKeyLines(juce::Graphics &g) const
{
    int lastNote = (getHeight() / getKeyWidth()) + getStartKey();

    for (auto i = static_cast<int>(getStartKey()); i <= lastNote; i++)
    {
        g.setColour(juce::MidiMessage::isMidiNoteBlack(i) ? juce::Colour(0x22000000) : juce::Colour(0x22ffffff));
        g.fillRect(getNoteRect(i, 0, getWidth()).reduced(0, 1));
    }
}

void MidiViewport::resized()
{
    auto area = getLocalBounds();
    m_lassoComponent.setViewBounds(area);
    refreshMouseSnapContext();
    updateNoteUnderMouse();
    m_timeLine.mouseFeedbackGeometryChanged();
}

void MidiViewport::drawNote(juce::Graphics &g, tracktion_engine::MidiClip *const &midiClip, tracktion_engine::MidiNote *n)
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
        visibleRect = visibleRect.getIntersection(getClipRect(midiClip));
    }

    auto noteColor = getNoteColour(midiClip, n);
    auto innerGlow = noteColor.brighter(0.5f);
    auto selectedColour = juce::Colour(0xccffffff);
    auto borderColour = juce::Colour(0xff000000);

    g.setColour(borderColour);
    g.fillRect(visibleRect);

    g.setColour(innerGlow);
    visibleRect.reduce(1, 1);
    g.fillRect(visibleRect);

    g.setColour(noteColor);
    visibleRect.reduce(1, 1);
    g.fillRect(visibleRect);

    if (isSelected(n))
    {
        g.setColour(selectedColour);
        g.drawRect(visibleRect.expanded(2, 2));
    }

    noteRect.reduce(2, 2);
    g.setColour(borderColour);
    drawKeyNum(g, n, noteRect);
}

void MidiViewport::drawDraggedNotes(juce::Graphics &g, te::MidiNote *n, te::MidiClip *clip)
{
    if (auto *pointerTool = dynamic_cast<PointerTool *>(m_currentTool.get()))
    {
        auto borderColour = juce::Colour(0xccffffff);

        if (clip == nullptr)
            return;
        const auto timing = pointerTool->previewTiming(clip, n);
        te::MidiNote mn(te::MidiNote::createNote(*n, tracktion::BeatPosition::fromBeats(timing.startBeat),
                                               tracktion::BeatDuration::fromBeats(timing.lengthBeats)));
        mn.setNoteNumber(mn.getNoteNumber() + pointerTool->getDraggedNoteDelta(), nullptr);

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
}

void MidiViewport::drawNotePropertyPreview(juce::Graphics &g)
{
    for (const auto &edit : m_notePropertyPreview)
    {
        if (edit.clip == nullptr || !edit.sourceState.isValid())
            continue;

        auto state = edit.sourceState.createCopy();
        state.setProperty(te::IDs::p, edit.noteNumber, nullptr);
        state.setProperty(te::IDs::b, edit.startBeat.inBeats(), nullptr);
        state.setProperty(te::IDs::l, edit.length.inBeats(), nullptr);
        te::MidiNote previewNote(state);
        auto noteRect = getNoteRect(edit.clip, &previewNote);

        if (!m_evs.m_editNotesOutsideClipRange)
            noteRect = noteRect.getIntersection(getClipRect(edit.clip));
        if (noteRect.isEmpty())
            continue;

        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.fillRect(noteRect);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawRect(noteRect, 2.0f);
    }
}

void MidiViewport::drawPendingPasteNotes(juce::Graphics &g)
{
    if (!m_pendingPasteState.isActive())
        return;

    for (const auto &clipboardNote : m_pendingPasteNotes)
    {
        auto *clip = findClipboardClip(clipboardNote.clipID);
        if (clip == nullptr)
            continue;

        auto previewState = clipboardNote.copiedState.createCopy();
        const int pitch = juce::jlimit(0, 127, static_cast<int>(previewState.getProperty(te::IDs::p)) + m_pendingPasteState.getPitchDelta());
        const double startBeat = static_cast<double>(previewState.getProperty(te::IDs::b)) + m_pendingPasteState.getBeatDelta();
        previewState.setProperty(te::IDs::p, pitch, nullptr);
        previewState.setProperty(te::IDs::b, startBeat, nullptr);

        te::MidiNote previewNote(previewState);
        auto noteRect = getNoteRect(clip, &previewNote);

        if (!m_evs.m_editNotesOutsideClipRange)
            noteRect = noteRect.getIntersection(getClipRect(clip));

        if (noteRect.isEmpty())
            continue;

        g.setColour(juce::Colours::white.withAlpha(0.18f));
        g.fillRect(noteRect);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawRect(noteRect, 2.0f);
    }
}

void MidiViewport::drawKeyNum(juce::Graphics &g, const tracktion_engine::MidiNote *n, juce::Rectangle<float> &noteRect) const
{
    if (m_evs.getViewYScale(m_timeLine.getTimeLineID()) > 13)
        g.drawText(GUIHelpers::getMidiNoteName(n->getNoteNumber()), noteRect, juce::Justification::centredLeft);
}

juce::Colour MidiViewport::getNoteColour(tracktion_engine::MidiClip *const &midiClip, tracktion_engine::MidiNote *n)
{
    auto s = EngineHelpers::getNoteStartBeat(midiClip, n);
    auto e = EngineHelpers::getNoteEndBeat(midiClip, n);
    bool isBeforeClipStart = s < 0;
    bool isAfterClipEnd = e > midiClip->getEndBeat().inBeats() - midiClip->getStartBeat().inBeats() + 0.00001;

    if (isBeforeClipStart || isAfterClipEnd)
        return juce::Colours::grey;
    else if (isHovered(n))
    {
        return m_track->getColour().brighter(0.6);
    }

    return m_track->getColour().darker(1.f - getVelocity(n));
}

float MidiViewport::getVelocity(const tracktion_engine::MidiNote *note) { return juce::jmap((float)note->getVelocity(), 0.f, 127.f, 0.f, 1.f); }

juce::Rectangle<float> MidiViewport::getNoteRect(te::MidiClip *const &midiClip, const tracktion_engine::MidiNote *n)
{
    double sBeat = EngineHelpers::getNoteStartBeat(midiClip, n);
    double eBeat = EngineHelpers::getNoteEndBeat(midiClip, n);
    auto x1 = m_evs.beatsToX(sBeat + midiClip->getStartBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth());
    auto x2 = m_evs.beatsToX(eBeat + midiClip->getStartBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth()) + 1;

    return getNoteRect(n->getNoteNumber(), x1, x2);
}

juce::Rectangle<float> MidiViewport::getNoteRect(const int noteNum, float x1, float x2) const
{
    auto yOffset = (float)noteNum - getStartKey() + 1;
    auto noteY = (float)getHeight() - (yOffset * getKeyWidth());
    return {float(x1), float(noteY), float(x2 - x1), float(getKeyWidth())};
}

void MidiViewport::drawClipRange(juce::Graphics &g, tracktion_engine::MidiClip *const &midiClip)
{
    auto clipRect = getClipRect(midiClip);
    auto clipStartX = static_cast<int>(clipRect.getX());
    auto clipEndX = static_cast<int>(clipRect.getRight());
    auto clipColour = midiClip->getTrack()->getColour();

    g.setColour(clipColour);
    g.drawVerticalLine(clipStartX, 0, static_cast<float>(getHeight()));
    g.drawVerticalLine(clipEndX, 0, static_cast<float>(getHeight()));
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRect(clipStartX + 1, 0, clipEndX - clipStartX - 2, getHeight());
    g.setColour(midiClip->getColour().withAlpha(0.1f));
    g.fillRect(clipStartX + 1, 0, clipEndX - clipStartX - 2, getHeight());
}

void MidiViewport::updateToolCursor()
{
    if (!m_currentTool)
        return;
    switch (m_currentTool->getToolId())
    {
        case Tool::draw:
        case Tool::knife:
        case Tool::eraser:
        {
            getCachedMidiClips();
            const auto point = getLocalPoint(nullptr, juce::Desktop::getInstance().getMainMouseSource().getScreenPosition());
            setMouseCursor(getClipAt(point.x) ? m_currentTool->getCursor(*this) : juce::MouseCursor::NormalCursor);
            break;
        }
        case Tool::lasso:
        case Tool::range: setMouseCursor(m_currentTool->getCursor(*this)); break;
        default: break; // Pointer keeps its existing note-body/edge feedback.
    }
}

void MidiViewport::mouseEnter(const juce::MouseEvent& e) { mouseMove(e); }

void MidiViewport::mouseMove(const juce::MouseEvent &e)
{
    m_lastMousePosition = e.getPosition();
    m_mouseInside = true;
    updateNoteUnderMouse();

    m_mouseInput.remember(e);
    if (m_currentTool)
        m_currentTool->mouseMove(e, *this);
    else
        setMouseCursor(juce::MouseCursor::NormalCursor);
    updateToolCursor();
}

bool MidiViewport::cancelActiveDraw()
{
    if (auto* draw = dynamic_cast<DrawTool*>(m_currentTool.get()); draw != nullptr && draw->isDrawing())
    {
        draw->cancel(*this);
        m_pressInput.reset();
        m_mouseInput.reset();
        updateToolCursor();
        return true;
    }
    return false;
}

bool MidiViewport::cancelActiveInteraction()
{
    if (cancelSelectionGesture())
        return true;
    if (cancelActiveDraw())
        return true;
    if (auto* pointer = dynamic_cast<PointerTool*>(m_currentTool.get()); pointer && pointer->isDragging())
    {
        pointer->toolDeactivated(*this);
        m_pressInput.reset();
        m_mouseInput.reset();
        return true;
    }
    return false;
}

void MidiViewport::refreshMouseSnapContext()
{
    if (!isShowing())
    {
        cancelActiveInteraction();
        return;
    }
    if (auto event = m_mouseInput.forContext(*this, juce::ModifierKeys::getCurrentModifiers()); event && m_currentTool)
    {
        m_mouseInput.remember(*event);
        juce::ScopedValueSetter<bool> replay(m_refreshingSnapContext, true);
        auto* pointer = dynamic_cast<PointerTool*>(m_currentTool.get());
        auto* draw = dynamic_cast<DrawTool*>(m_currentTool.get());
        if (m_lassoComponent.active())
            updateLasso(*event);
        else if (m_timeRangeGesture.active())
            updateTimeRangeSelection(*event);
        else if ((pointer && pointer->isDragging()) || (draw && draw->isDrawing()))
            m_currentTool->mouseDrag(*event, *this);
        else if (m_currentTool->getToolId() == Tool::knife)
            m_currentTool->mouseMove(*event, *this);
    }
}

void MidiViewport::modifierKeysChanged(const juce::ModifierKeys& mods)
{
    if (auto event = m_mouseInput.withModifiers(mods); event && m_currentTool)
    {
        auto* draw = dynamic_cast<DrawTool*>(m_currentTool.get());
        auto* pointer = dynamic_cast<PointerTool*>(m_currentTool.get());
        if (m_lassoComponent.active())
            updateLasso(*event);
        else if (m_timeRangeGesture.active())
            updateTimeRangeSelection(*event);
        else if ((draw && draw->isDrawing()) || (pointer && pointer->isDragging()))
            m_currentTool->mouseDrag(*event, *this);
        else if (m_currentTool->getToolId() == Tool::knife)
            m_currentTool->mouseMove(*event, *this);
    }
}

void MidiViewport::mouseDown(const juce::MouseEvent &e)
{
    cancelSelectionGesture();
    m_pressInput.remember(e);
    m_mouseInput.remember(e);
    if (m_timeLine.onNoteInteractionBeginning)
        m_timeLine.onNoteInteractionBeginning();
    finishPendingPasteOnDeselect();

    if (m_currentTool)
        m_currentTool->mouseDown(e, *this);

    // Handle double click
    if (e.getNumberOfClicks() == 2)
    {
        if (m_currentTool)
            m_currentTool->mouseDoubleClick(e, *this);
    }
}
void MidiViewport::mouseDrag(const juce::MouseEvent &e)
{
    if (!m_pressInput.belongsToGesture(e)) return;
    m_mouseInput.remember(e);
    if (m_currentTool)
        m_currentTool->mouseDrag(e, *this);

    repaint();
}
void MidiViewport::mouseUp(const juce::MouseEvent &e)
{
    if (!m_pressInput.belongsToGesture(e)) return;
    if (m_currentTool)
        m_currentTool->mouseUp(e, *this);
    m_pressInput.reset();
    m_mouseInput.reset();
    updateToolCursor();
    repaint();
}
void MidiViewport::valueTreeChildAdded(juce::ValueTree &parent, juce::ValueTree &child)
{
    // Only invalidate cache if a clip was added
    if (parent.getType() == te::IDs::TRACK && child.hasType(te::IDs::MIDICLIP))
    {
        invalidateClipCache();
        repaint();
    }
}

void MidiViewport::valueTreeChildRemoved(juce::ValueTree &parent, juce::ValueTree &child, int)
{
    if (m_noteInteractionActive && !m_committingInteraction
        && (child == m_noteInteractionSource || m_noteInteractionSource.isAChildOf(child)))
    {
        // Engine list listeners may already have destroyed the removed note.
        m_hoveredNote = m_clickedNote = nullptr;
        m_currentTool->toolDeactivated(*this);
    }
    // Only invalidate cache if a clip was removed
    if (parent.getType() == te::IDs::TRACK && child.hasType(te::IDs::MIDICLIP))
    {
        invalidateClipCache();
        repaint();
    }
}

void MidiViewport::cleanUpFlags()
{
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
    m_mouseInside = false;
    if (!m_noteInteractionActive)
        m_timeLine.clearMouseFeedback(TimelineFeedbackOwner::notes);
    updateNoteUnderMouse();

    // Clear hover state when mouse leaves the component
    if (m_hoveredNote != nullptr)
    {
        setHovered(m_hoveredNote, false);
        m_hoveredNote = nullptr;
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

te::MidiNote *MidiViewport::addNewNoteAt(int x, int y, te::MidiClip *clip)
{
    auto noteNum = getKeyForY(y);
    if (clip == nullptr)
        return nullptr;
    const bool bypass = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    double start = m_timeLine.xToBeatPos(x).inBeats();
    if (m_timeLine.isSnappingEnabled() && !bypass)
        start = m_timeLine.getMouseSnapResolver().startAtOrBefore(start);
    const double base = clip->getStartBeat().inBeats() - clip->getOffsetInBeats().inBeats();
    start = std::max(start, base);
    PianoRollDrawGesture gesture;
    if (!gesture.begin(start, m_timeLine.getNoteInsertLength(), x, m_timeLine.getMouseSnapResolver(), bypass))
        return nullptr;
    return addNewNote(noteNum, clip, gesture.startBeat() - base, gesture.endBeat() - gesture.startBeat());
}

te::MidiNote *MidiViewport::addNewNote(int noteNumb, const te::MidiClip *clip, double beat, double length)
{
    if (length <= 0)
        length = m_timeLine.getNoteInsertLength();

    return MidiNoteCreation::add(clip, noteNumb, beat, length, m_evs.m_lastVelocity,
        [this](auto* note) { if (m_selectedEvents) m_selectedEvents->removeSelectedEvent(note); },
        [this](double actualLength) { m_timeLine.setLastNoteLength(actualLength); });
}

void MidiViewport::playGuideNote(const te::MidiClip *clip, const int noteNumb, int vel)
{
    clip->getAudioTrack()->playGuideNote(noteNumb, clip->getMidiChannel(), vel, false, true);
    startTimer(100);
}

void MidiViewport::removeNote(te::MidiClip *clip, te::MidiNote *note) { clip->getSequence().removeNote(*note, &m_evs.m_edit.getUndoManager()); }

float MidiViewport::getKeyWidth() const { return (float)m_evs.getViewYScale(m_timeLine.getTimeLineID()); }

float MidiViewport::getStartKey() const { return (float)m_evs.getViewYScroll(m_timeLine.getTimeLineID()); }

void MidiViewport::startLasso(const juce::MouseEvent &e)
{
    if (!e.mods.isLeftButtonDown()) return;
    m_sharedOriginalSelection.capture(m_evs.m_selectionManager);
    m_selectionAtGestureStart = MidiSelectionSnapshot::capture(getSelectedEvents(), getCachedMidiClips());
    const auto start = e.mouseDownPosition;
    m_lassoComponent.begin({m_timeLine.xToBeatPos(start.x).inBeats(),
        getStartKey() + (getHeight() - start.y) / getKeyWidth()});
    replaceSelection(combineLassoSelection(m_selectionAtGestureStart, MidiSelectionSnapshot::Items{}, lassoSelectionMode(e.mods)));
}

void MidiViewport::setNoteSelected(tracktion_engine::MidiNote *n, bool addToSelection)
{
    if (!m_selectedEvents)
        m_selectedEvents = std::make_unique<te::SelectedMidiEvents>(juce::Array<te::MidiClip *>());

    m_selectedEvents->addSelectedEvent(n, addToSelection);
    m_evs.m_selectionManager.addToSelection(*m_selectedEvents);
}

void MidiViewport::updateLasso(const juce::MouseEvent &e)
{
    if (!m_lassoComponent.active() || !m_pressInput.belongsToGesture(e)) return;
    m_lassoComponent.update({m_timeLine.xToBeatPos(e.position.x).inBeats(),
        getStartKey() + (getHeight() - e.position.y) / getKeyWidth()});
    updateLassoSelection(e.mods);
    repaint();
}

void MidiViewport::stopLasso()
{
    m_lassoComponent.end();
    m_selectionAtGestureStart.clear();
    m_sharedOriginalSelection.clear();
    repaint();
}

void MidiViewport::startTimeRangeSelection(const juce::MouseEvent& e)
{
    if (!e.mods.isLeftButtonDown()) return;
    m_sharedOriginalSelection.capture(m_evs.m_selectionManager);
    m_selectionAtGestureStart = MidiSelectionSnapshot::capture(getSelectedEvents(), getCachedMidiClips());
    m_timeRangeGesture.begin(m_timeLine.xToBeatPos(e.position.x).inBeats(),
        getStartKey() + (getHeight() - e.position.y) / getKeyWidth());
    replaceSelection({});
}

void MidiViewport::updateTimeRangeSelection(const juce::MouseEvent& e)
{
    if (!m_timeRangeGesture.active() || !m_pressInput.belongsToGesture(e)) return;
    m_timeRangeGesture.update(m_timeLine.xToBeatPos(e.position.x).inBeats(),
        getStartKey() + (getHeight() - e.position.y) / getKeyWidth());
    updateRangeNoteSelection();
    repaint();
}

void MidiViewport::stopTimeRangeSelection()
{
    m_timeRangeGesture.end();
    m_selectionAtGestureStart.clear();
    m_sharedOriginalSelection.clear();
    repaint();
}

bool MidiViewport::cancelSelectionGesture(bool restore)
{
    if (!m_lassoComponent.active() && !m_timeRangeGesture.active()) return false;
    m_lassoComponent.end();
    m_timeRangeGesture.end();
    if (restore) m_sharedOriginalSelection.restore(m_evs.m_edit, m_evs.m_selectionManager);
    m_selectionAtGestureStart.clear();
    m_sharedOriginalSelection.clear();
    m_pressInput.reset();
    m_mouseInput.reset();
    repaint();
    return true;
}

MidiViewport::MidiClipboard MidiViewport::copySelectedNotesToClipboard()
{
    MidiClipboard clipboard;

    if (m_selectedEvents == nullptr)
        return clipboard;

    for (auto *note : getSelectedNotes())
    {
        if (auto *clip = m_selectedEvents->clipForEvent(note))
            clipboard.add({clip->itemID, note->state.createCopy()});
    }

    return clipboard;
}

bool MidiViewport::beginPendingPaste(const MidiClipboard &clipboard)
{
    cancelPendingPaste();

    for (const auto &clipboardNote : clipboard)
    {
        if (!clipboardNote.copiedState.hasType(te::IDs::NOTE))
            continue;

        if (findClipboardClip(clipboardNote.clipID) != nullptr)
            m_pendingPasteNotes.add(clipboardNote);
    }

    if (m_pendingPasteNotes.isEmpty())
        return false;

    m_pendingPasteState.begin();
    deselectActualNotes();
    repaint();
    sendChangeMessage();
    return true;
}

bool MidiViewport::nudgePendingPaste(int leftRight, int upDown)
{
    if (!m_pendingPasteState.isActive())
        return false;

    double beatDelta = 0.0;
    int pitchDelta = 0;

    if (leftRight != 0)
    {
        for (const auto &clipboardNote : m_pendingPasteNotes)
        {
            auto *clip = findClipboardClip(clipboardNote.clipID);
            if (clip == nullptr)
                continue;

            auto state = clipboardNote.copiedState.createCopy();
            state.setProperty(te::IDs::b,
                              static_cast<double>(state.getProperty(te::IDs::b)) + m_pendingPasteState.getBeatDelta(),
                              nullptr);
            te::MidiNote referenceNote(state);
            const auto start = referenceNote.getEditStartTime(*clip);
            beatDelta = m_timeLine.getNudgeDeltaBeats(m_evs.m_edit.tempoSequence.toBeats(start).inBeats(), leftRight);
            break;
        }
    }

    if (upDown != 0)
    {
        int minimumPitch = 127;
        int maximumPitch = 0;

        for (const auto &clipboardNote : m_pendingPasteNotes)
        {
            const int pitch = static_cast<int>(clipboardNote.copiedState.getProperty(te::IDs::p))
                              + m_pendingPasteState.getPitchDelta();
            minimumPitch = juce::jmin(minimumPitch, pitch);
            maximumPitch = juce::jmax(maximumPitch, pitch);
        }

        pitchDelta = juce::jlimit(-minimumPitch, 127 - maximumPitch, upDown);
    }

    if (m_pendingPasteState.nudge(beatDelta, pitchDelta))
        repaint();

    return true;
}

void MidiViewport::nudgeSelectedNotes(int leftRight, int upDown)
{
    if (m_selectedEvents == nullptr)
        return;

    if (!m_timeLine.isUsingFixedSnap() && m_timeLine.isSnappingEnabled())
    {
        m_selectedEvents->nudge(m_timeLine.getBestSnapType(), leftRight, upDown);
        return;
    }

    if (upDown != 0)
        m_selectedEvents->nudge(m_timeLine.getBestSnapType(), 0, upDown);

    if (leftRight == 0)
        return;

    const auto selectedNotes = getSelectedNotes();
    if (selectedNotes.isEmpty())
        return;

    auto *firstNote = selectedNotes.getFirst();
    if (firstNote == nullptr)
        return;

    auto *firstClip = m_selectedEvents->clipForEvent(firstNote);
    if (firstClip == nullptr)
        return;

    const auto globalBeat = m_evs.m_edit.tempoSequence.toBeats(firstNote->getEditStartTime(*firstClip)).inBeats();
    const auto beatDelta = m_timeLine.getNudgeDeltaBeats(globalBeat, leftRight);
    auto &undoManager = m_evs.m_edit.getUndoManager();
    for (auto *note : selectedNotes)
        note->setStartAndLength(note->getStartBeat() + tracktion::BeatDuration::fromBeats(beatDelta),
                                note->getLengthBeats(), &undoManager);
}

bool MidiViewport::confirmPendingPaste(bool keepSelection)
{
    return resolvePendingPaste(m_pendingPasteState.confirm(), keepSelection);
}

bool MidiViewport::finishPendingPasteOnDeselect()
{
    return resolvePendingPaste(m_pendingPasteState.finishOnDeselect(), false);
}

bool MidiViewport::cancelPendingPaste()
{
    return resolvePendingPaste(m_pendingPasteState.cancel(), false);
}

void MidiViewport::setNotePropertyPreview(const juce::Array<MidiNotePropertyEdit> &preview)
{
    m_notePropertyPreview = preview;
    repaint();
}

void MidiViewport::commitNotePropertyEdit(const juce::Array<MidiNotePropertyEdit> &requestedEdits,
                                          bool resolveOverlaps)
{
    m_notePropertyPreview.clearQuick();

    juce::Array<MidiNotePropertyEdit> edits;
    juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> unchangedNotes;
    for (const auto &edit : requestedEdits)
    {
        if (edit.clip == nullptr || edit.sourceNote == nullptr
            || !edit.clip->getSequence().getNotes().contains(edit.sourceNote))
            continue;

        const bool changed = std::abs(edit.startBeat.inBeats() - edit.sourceNote->getStartBeat().inBeats()) >= 1.0e-7
                             || std::abs(edit.length.inBeats() - edit.sourceNote->getLengthBeats().inBeats()) >= 1.0e-7
                             || edit.noteNumber != edit.sourceNote->getNoteNumber()
                             || edit.velocity != edit.sourceNote->getVelocity();
        if (changed)
            edits.add(edit);
        else
            unchangedNotes.add({edit.clip, edit.sourceNote});
    }

    if (edits.isEmpty())
    {
        repaint();
        return;
    }

    auto &undo = m_evs.m_edit.getUndoManager();
    if (!resolveOverlaps)
    {
        for (const auto &edit : edits)
            edit.sourceNote->setVelocity(edit.velocity, &undo);
        repaint();
        return;
    }

    // The destination notes have priority. Remove all edited sources first,
    // clear the complete destination ranges, then recreate from full state copies.
    unselectAll();
    for (const auto &edit : edits)
        edit.clip->getSequence().removeNote(*edit.sourceNote, &undo);

    std::map<std::pair<te::MidiClip *, int>, juce::Array<tracktion::BeatRange>> rangesByPitch;
    for (const auto &edit : edits)
        rangesByPitch[{edit.clip, edit.noteNumber}].add({edit.startBeat, edit.startBeat + edit.length});

    for (auto &[key, ranges] : rangesByPitch)
        cleanUnderNoteRanges(key.second, ranges, key.first);

    juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> createdNotes;
    for (const auto &edit : edits)
    {
        // Absolute property edits can make selected destinations overlap each
        // other. Resolve those conflicts in selection order as well.
        cleanUnderNote(edit.noteNumber, {edit.startBeat, edit.length}, edit.clip);

        auto state = edit.sourceState.createCopy();
        state.setProperty(te::IDs::p, edit.noteNumber, nullptr);
        state.setProperty(te::IDs::b, edit.startBeat.inBeats(), nullptr);
        state.setProperty(te::IDs::l, edit.length.inBeats(), nullptr);
        auto *newNote = edit.clip->getSequence().addNote(te::MidiNote(state), &undo);
        createdNotes.add({edit.clip, newNote});
    }

    for (const auto &[clip, note] : createdNotes)
        if (note != nullptr && clip->getSequence().getNotes().contains(note))
            setNoteSelected(note, true);

    // Notes clamped at a pitch/velocity boundary may have had no effective
    // change. Restore their selection if they survived destination clearing.
    for (const auto &[clip, note] : unchangedNotes)
        if (clip->getSequence().getNotes().contains(note))
            setNoteSelected(note, true);

    cleanUpFlags();
    repaint();
    sendChangeMessage();
}

bool MidiViewport::resolvePendingPaste(MidiPendingPaste::Resolution resolution, bool keepSelection)
{
    if (resolution.action == MidiPendingPaste::Action::none)
        return false;

    if (resolution.action == MidiPendingPaste::Action::cancel)
    {
        m_pendingPasteNotes.clear();
        repaint();
        sendChangeMessage();
        return true;
    }

    struct PlannedNote
    {
        te::MidiClip *clip{};
        tracktion::BeatPosition startBeat;
        tracktion::BeatDuration length;
        int pitch{};
        juce::ValueTree state;
    };

    juce::Array<PlannedNote> plannedNotes;

    for (const auto &clipboardNote : m_pendingPasteNotes)
    {
        auto *clip = findClipboardClip(clipboardNote.clipID);
        if (clip == nullptr)
            continue;

        const auto startBeat = tracktion::BeatPosition::fromBeats(
            static_cast<double>(clipboardNote.copiedState.getProperty(te::IDs::b)) + resolution.beatDelta);
        const auto length = tracktion::BeatDuration::fromBeats(
            static_cast<double>(clipboardNote.copiedState.getProperty(te::IDs::l)));
        const int pitch = juce::jlimit(0, 127,
                                      static_cast<int>(clipboardNote.copiedState.getProperty(te::IDs::p))
                                          + resolution.pitchDelta);

        if (length.inBeats() <= 0.0)
            continue;

        plannedNotes.add({clip, startBeat, length, pitch, clipboardNote.copiedState.createCopy()});
    }

    m_pendingPasteNotes.clear();

    if (plannedNotes.isEmpty())
    {
        repaint();
        sendChangeMessage();
        return true;
    }

    auto &undoManager = m_evs.m_edit.getUndoManager();
    undoManager.beginNewTransaction("Paste MIDI Notes");

    std::map<std::pair<te::MidiClip *, int>, juce::Array<tracktion::BeatRange>> rangesByPitch;
    for (const auto &note : plannedNotes)
        rangesByPitch[{note.clip, note.pitch}].add({note.startBeat, note.startBeat + note.length});

    for (auto &[key, ranges] : rangesByPitch)
        cleanUnderNoteRanges(key.second, ranges, key.first);

    for (const auto &note : plannedNotes)
    {
        auto state = note.state.createCopy();
        state.setProperty(te::IDs::p, note.pitch, nullptr);
        state.setProperty(te::IDs::b, note.startBeat.inBeats(), nullptr);
        state.setProperty(te::IDs::l, note.length.inBeats(), nullptr);

        auto *newNote = note.clip->getSequence().addNote(te::MidiNote(state), &undoManager);
        if (keepSelection)
            setNoteSelected(newNote, true);
    }

    repaint();
    sendChangeMessage();
    return true;
}

te::MidiClip *MidiViewport::findClipboardClip(te::EditItemID clipID) const
{
    auto *clip = dynamic_cast<te::MidiClip *>(te::findClipForID(m_evs.m_edit, clipID));
    return clip != nullptr && clip->getTrack() == m_track.get() ? clip : nullptr;
}

void MidiViewport::deselectActualNotes()
{
    if (m_selectedEvents != nullptr)
        m_evs.m_selectionManager.deselect(m_selectedEvents.get());

    if (m_track != nullptr && !m_evs.m_selectionManager.isSelected(*m_track))
        m_evs.m_selectionManager.addToSelection(m_track);
}

void MidiViewport::duplicateSelectedNotes()
{
    if (m_selectedEvents == nullptr || m_selectedEvents->getNumSelected() == 0)
        return;

    const auto duplicateOffset = m_selectedEvents->getSelectedRange().getLength();
    if (duplicateOffset.inSeconds() <= 0.0)
        return;

    struct NoteCopy
    {
        te::MidiClip *clip;
        tracktion::BeatPosition startBeat;
        tracktion::BeatDuration length;
        int noteNumber;
        juce::ValueTree noteState;
    };

    juce::Array<NoteCopy> copies;
    for (auto *note : getSelectedNotes())
    {
        auto *clip = m_selectedEvents->clipForEvent(note);
        if (clip == nullptr)
            continue;

        const auto destinationTime = note->getEditStartTime(*clip) + duplicateOffset;
        const auto destinationBeat = clip->getContentBeatAtTime(destinationTime) + toDuration(clip->getLoopStartBeats());
        copies.add({clip, destinationBeat, note->getLengthBeats(), note->getNoteNumber(), note->state.createCopy()});
    }

    if (copies.isEmpty())
        return;

    auto &undoManager = m_evs.m_edit.getUndoManager();
    undoManager.beginNewTransaction("Duplicate MIDI Notes");

    unselectAll();

    // Clear all destinations before creating notes so duplicated notes cannot
    // erase each other when several selected notes share a pitch.
    std::map<std::pair<te::MidiClip *, int>, juce::Array<tracktion::BeatRange>> rangesByPitch;
    for (const auto &copy : copies)
        rangesByPitch[{copy.clip, copy.noteNumber}].add({copy.startBeat, copy.startBeat + copy.length});

    for (auto &[key, ranges] : rangesByPitch)
        cleanUnderNoteRanges(key.second, ranges, key.first);

    for (const auto &copy : copies)
    {
        auto newState = copy.noteState.createCopy();
        newState.setProperty(te::IDs::p, copy.noteNumber, nullptr);
        newState.setProperty(te::IDs::b, copy.startBeat.inBeats(), nullptr);
        newState.setProperty(te::IDs::l, copy.length.inBeats(), nullptr);

        auto *newNote = copy.clip->getSequence().addNote(te::MidiNote(newState), &undoManager);
        setNoteSelected(newNote, true);
    }

    cleanUpFlags();
    repaint();
}

void MidiViewport::snapToGrid(te::MidiNote *note, const te::MidiClip *clip) const
{
    auto &um = m_evs.m_edit.getUndoManager();

    if (m_expandLeft)
        note->setStartAndLength(tracktion::BeatPosition::fromBeats(m_timeLine.getQuantisedNoteBeat(note->getStartBeat().inBeats(), clip)), note->getEndBeat() - tracktion::BeatPosition::fromBeats(m_timeLine.getQuantisedNoteBeat(note->getStartBeat().inBeats(), clip)), &um);
    else if (m_expandRight || m_noteAdding)
        note->setStartAndLength(note->getStartBeat(), tracktion::BeatPosition::fromBeats(m_timeLine.getQuantisedNoteBeat(note->getEndBeat().inBeats(), clip)) - note->getStartBeat(), &um);
    else
        note->setStartAndLength(tracktion::BeatPosition::fromBeats(m_timeLine.getQuantisedNoteBeat(note->getStartBeat().inBeats(), clip)), note->getLengthBeats(), &um);
}

void MidiViewport::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel)
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
            wheel.deltaX * -(m_evs.getTimeLineZoomUnit());
#else
            wheel.deltaY * -(m_evs.getTimeLineZoomUnit());
#endif

        if (wheelDelta != 0)
            m_timeLine.zoomByFactor(wheelDelta > 0 ? 1.1 : 0.9, event.getPosition().getX());
    }
    else
    {
        scrollPianoRoll((float)wheel.deltaY * 5);
    }
}

void MidiViewport::scrollPianoRoll(float delta)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyWidth = m_evs.getViewYScale(m_timeLine.getTimeLineID());
    m_evs.setYScroll(m_timeLine.getTimeLineID(), juce::jlimit(0.f, 127.f - (float)(getHeight() / keyWidth), (float)startKey + delta));
    refreshNoteUnderMouse();
}

void MidiViewport::setNoteUnderMouseHandler(NoteUnderMouseHandler handler)
{
    m_noteUnderMouseHandler = std::move(handler);
    refreshNoteUnderMouse();
}

void MidiViewport::refreshNoteUnderMouse()
{
    updateNoteUnderMouse();
    if (!juce::Desktop::getInstance().getMainMouseSource().isDragging())
        updateToolCursor();
}

void MidiViewport::updateNoteUnderMouse()
{
    std::optional<int> note;
    if (m_mouseInside && getLocalBounds().contains(m_lastMousePosition))
        note = getNoteNumber(m_lastMousePosition.y);

    if (note == m_noteUnderMouse)
        return;

    m_noteUnderMouse = note;
    if (m_noteUnderMouseHandler)
        m_noteUnderMouseHandler(note);
}

void MidiViewport::drawBarsAndBeatLines(juce::Graphics &g, juce::Colour colour)
{
    auto x1 = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_evs.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    GUIHelpers::drawBarsAndBeatLines(g, m_evs, x1, x2, getLocalBounds().toFloat());
}

int MidiViewport::getNoteNumber(int y)
{
    return juce::jlimit(0, 127, static_cast<int>(getKeyForY(y)));
}

tracktion::MidiClip *MidiViewport::getClipAt(float x)
{
    auto time = m_timeLine.xToTimePos(x);
    for (auto clip : m_cachedClips)
        if (clip->getEditTimeRange().contains(time))
            return dynamic_cast<tracktion::MidiClip *>(clip);

    return nullptr;
}

te::MidiNote *MidiViewport::getNoteByPos(juce::Point<float> pos)
{
    for (auto &mc : getCachedMidiClips())
    {
        for (auto note : mc->getSequence().getNotes())
        {
            auto clickedBeat = m_evs.xToBeats(pos.x, m_timeLine.getTimeLineID(), getWidth()) + mc->getOffsetInBeats().inBeats();
            auto clipStart = mc->getStartBeat().inBeats();
            auto isNoteNum = (note->getNoteNumber() == getNoteNumber(static_cast<int>(pos.y)));
            auto noteStart = note->getStartBeat().inBeats() + clipStart;
            auto noteEnd = note->getEndBeat().inBeats() + clipStart;

            if (isNoteNum && juce::Range<double>(noteStart, noteEnd).contains(clickedBeat))
                return note;
        }
    }
    return nullptr;
}

tracktion_engine::MidiClip *MidiViewport::getMidiClipAt(float x)
{
    for (auto &c : getCachedMidiClips())
        if ((c->getStartBeat().inBeats() < m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth())) && (c->getEndBeat().inBeats() > m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth())))
            return c;

    return nullptr;
}

tracktion_engine::Track::Ptr MidiViewport::getTrack() { return m_track; }

void MidiViewport::unselectAll()
{
    finishPendingPasteOnDeselect();
    deselectActualNotes();
    repaint();
}

double MidiViewport::getKeyForY(int y)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyHeight = m_evs.getViewYScale(m_timeLine.getTimeLineID());
    auto keyNumb = (double)(startKey + ((double)(getHeight() - y) / keyHeight));

    return keyNumb;
}

int MidiViewport::getYForKey(double key)
{
    auto startKey = m_evs.getViewYScroll(m_timeLine.getTimeLineID());
    auto keyHeight = m_evs.getViewYScale(m_timeLine.getTimeLineID());

    auto y = getHeight() - (keyHeight * (key - startKey));

    return static_cast<int>(y);
}

void MidiViewport::replaceSelection(const MidiSelectionSnapshot::Items& items)
{
    MidiSelectionSnapshot::apply(items, getSelectedEvents(), m_evs.m_selectionManager, getCachedMidiClips());
    if (m_selectedEvents->getSelectedNotes().isEmpty()) deselectActualNotes();
    repaint();
}

void MidiViewport::updateLassoSelection(juce::ModifierKeys modifiers)
{
    MidiSelectionSnapshot::Items hits;
    const auto rect = m_lassoComponent.viewBounds();
    for (auto* clip : getCachedMidiClips())
        for (auto* note : clip->getSequence().getNotes())
        {
            auto noteRect = getNoteRect(clip, note);
            if (!m_evs.m_editNotesOutsideClipRange)
                noteRect = noteRect.getIntersection(getClipRect(clip));
            if (rect.intersects(noteRect)) hits.add(note->state);
        }
    replaceSelection(combineLassoSelection(m_selectionAtGestureStart, hits, lassoSelectionMode(modifiers)));
}

void MidiViewport::updateRangeNoteSelection()
{
    MidiSelectionSnapshot::Items hits;
    const auto beats = m_timeRangeGesture.interval(), pitches = m_timeRangeGesture.lanes();
    for (auto* clip : getCachedMidiClips())
        for (auto* note : clip->getSequence().getNotes())
        {
            const double start = clip->getStartBeat().inBeats() + EngineHelpers::getNoteStartBeat(clip, note);
            const double end = clip->getStartBeat().inBeats() + EngineHelpers::getNoteEndBeat(clip, note);
            if (beats.intersects({start, end}) && pitches.intersects({double(note->getNoteNumber()), double(note->getNoteNumber()) + 1.0}))
                hits.add(note->state);
        }
    replaceSelection(hits);
}

void MidiViewport::deleteSelectedNotes()
{
    juce::Array<std::pair<te::MidiClip *, te::MidiNote *>> notesToDelete;
    for (auto *note : getSelectedNotes())
        if (auto *clip = m_selectedEvents->clipForEvent(note))
            notesToDelete.add({clip, note});

    if (notesToDelete.isEmpty())
        return;

    unselectAll();

    for (const auto &[clip, note] : notesToDelete)
        clip->getSequence().removeNote(*note, &m_evs.m_edit.getUndoManager());
}

bool MidiViewport::isSelected(tracktion_engine::MidiNote *note) { return m_selectedEvents->isSelected(note); }

void MidiViewport::timerCallback()
{
    stopTimer();
    auto at = EngineHelpers::getAudioTrack(getTrack(), m_evs);
    at->turnOffGuideNotes();
}

void MidiViewport::cleanUnderNote(int noteNumb, tracktion::BeatRange beatRange, const te::MidiClip *clip)
{
    juce::Array<tracktion::BeatRange> ranges;
    ranges.add(beatRange);
    cleanUnderNoteRanges(noteNumb, ranges, clip);
}

void MidiViewport::cleanUnderNoteRanges(int noteNumb, const juce::Array<tracktion::BeatRange> &ranges, const te::MidiClip *clip)
{
    MidiNoteCreation::clear(clip, noteNumb, ranges,
        [this](auto* note) { if (m_selectedEvents) m_selectedEvents->removeSelectedEvent(note); });
}

te::MidiClip *MidiViewport::getNearestClipBefore(int x)
{
    if (auto clipAt = getMidiClipAt(x))
        return clipAt;

    const auto &clips = getCachedMidiClips();
    if (clips.isEmpty())
        return nullptr;

    te::MidiClip *best = nullptr;
    double targetBeat = m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth());

    for (auto c : clips)
    {
        double endBeat = c->getEndBeat().inBeats();
        if (endBeat < targetBeat)
        {
            if (best == nullptr || endBeat > best->getEndBeat().inBeats())
                best = c;
        }
    }

    return best;
}

te::MidiClip *MidiViewport::getNearestClipAfter(int x)
{
    if (getMidiClipAt(x) != nullptr)
        return getMidiClipAt(x);

    te::MidiClip *clip = nullptr;

    for (auto c : getCachedMidiClips())
        if (c->getStartBeat().inBeats() > m_evs.xToBeats(x, m_timeLine.getTimeLineID(), getWidth()) && (clip == nullptr || clip->getStartBeat().inBeats() > c->getStartBeat().inBeats()))
            clip = c;

    return clip;
}

juce::Rectangle<float> MidiViewport::getClipRect(te::Clip *clip)
{
    auto clipX = static_cast<float>(m_evs.beatsToX(clip->getStartBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth()));
    auto clipW = static_cast<float>(m_evs.beatsToX(clip->getEndBeat().inBeats(), m_timeLine.getTimeLineID(), getWidth()) - clipX);

    auto clipY = static_cast<float>(getYForKey(127.0));
    auto clipH = static_cast<float>(getYForKey(0.0) - clipY);

    return {clipX, clipY, clipW, clipH};
}

void MidiViewport::updateSelectedEvents()
{
    cancelSelectionGesture(false);
    finishPendingPasteOnDeselect();

    if (m_selectedEvents != nullptr)
    {
        m_selectedEvents->removeChangeListener(this);
        m_selectedEvents->deselect();
    }

    if (getCachedMidiClips().size() > 0)
        m_selectedEvents = std::make_unique<te::SelectedMidiEvents>(getCachedMidiClips());
    else
        m_selectedEvents = std::make_unique<te::SelectedMidiEvents>(juce::Array<te::MidiClip *>());

    m_selectedEvents->addChangeListener(this);
}

void MidiViewport::refreshClipCache()
{
    // Clip cache refreshed (removed noisy debug log)
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

const juce::Array<te::MidiClip *> &MidiViewport::getCachedMidiClips()
{
    if (!m_clipCacheValid || m_track == nullptr)
    {
        refreshClipCache();
    }
    return m_cachedClips;
}

void MidiViewport::invalidateClipCache() { m_clipCacheValid = false; }

bool MidiViewport::isHovered(te::MidiNote *note) { return static_cast<bool>(note->state.getProperty(IDs::isHovered)); }

void MidiViewport::setHovered(te::MidiNote *note, bool hovered) { note->state.setProperty(IDs::isHovered, hovered, nullptr); }

juce::Array<te::MidiNote *> MidiViewport::getSelectedNotes()
{
    if (!m_selectedEvents)
        return {};
    return m_selectedEvents->getSelectedNotes();
}

te::SelectedMidiEvents &MidiViewport::getSelectedEvents()
{
    if (!m_selectedEvents)
        m_selectedEvents = std::make_unique<te::SelectedMidiEvents>(juce::Array<te::MidiClip *>());
    return *m_selectedEvents;
}

void MidiViewport::setTool(Tool tool)
{
    finishPendingPasteOnDeselect();

    if (m_currentTool)
        m_currentTool->toolDeactivated(*this);
    m_pressInput.reset();
    m_mouseInput.reset();

    m_currentTool = ToolFactory::createTool(tool, m_evs);

    if (m_currentTool)
        m_currentTool->toolActivated(*this);
    updateToolCursor();

    // Notify listeners about the tool change
    sendChangeMessage();
}
