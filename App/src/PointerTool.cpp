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

#include "PointerTool.h"
#include "LassoTool.h"

#include <map>

void PointerTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    resetDrag(viewport);
    m_pendingLassoStart = false;
    m_dragStartPos = event.getPosition();
    m_lastDragPos = m_dragStartPos;
    m_isDragging = false;
    m_hasPlayedDragGuideNote = false;
    m_lastGuideNoteDelta = 0;

    viewport.setClickedNote(nullptr);
    viewport.setClickedClip(nullptr);

    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note))
        {
            viewport.setClickedNote(note);
            viewport.setClickedClip(clip);

            auto noteRect = viewport.getNoteRect(clip, note);
            auto borderWidth = noteRect.getWidth() > 30 ? 10.0f : noteRect.getWidth() / 3.0f;

            if (std::abs(event.x - noteRect.getX()) < borderWidth)
                m_currentDragMode = DragMode::resizeLeft;
            else if (std::abs(event.x - noteRect.getRight()) < borderWidth)
                m_currentDragMode = DragMode::resizeRight;
            else
                m_currentDragMode = DragMode::moveNotes;

            m_dragClip = clip;
            m_pitchAnchorKey = viewport.getNoteNumber(event.y);
            m_pitchAnchorDelta = 0;
            m_originalEdgeBeat = clip->getStartBeat().inBeats() - clip->getOffsetInBeats().inBeats()
                + (m_currentDragMode == DragMode::resizeRight ? note->getEndBeat().inBeats() : note->getStartBeat().inBeats());
            m_timeGesture.begin(m_originalEdgeBeat, event.position.x, viewport.getTimeLine()->getMouseSnapResolver());

            if (!event.mods.isShiftDown() && !viewport.isSelected(note))
                viewport.unselectAll();

            if (!viewport.isSelected(note))
                viewport.setNoteSelected(note, event.mods.isShiftDown());
        }
    }
    else
    {
        // Empty space - don't immediately switch to LassoTool. Defer starting lasso until
        // the user drags the mouse to allow double-clicks to be detected by PointerTool.
        viewport.unselectAll();
        m_pendingLassoStart = true;
    }
}

void PointerTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.setClickedClip(viewport.getClipAt(event.position.x));
    insertNoteAtPosition(event, viewport);
}

void PointerTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (!m_isDragging && event.getDistanceFromDragStart() > 5)
        m_isDragging = true;

    if (m_isDragging)
    {
        // If we had a pending lasso start (clicked empty space before dragging), start the lasso now.
        if (m_pendingLassoStart)
        {
            m_pendingLassoStart = false;
            viewport.setTool(Tool::lasso);
            if (auto *lassoTool = dynamic_cast<LassoTool *>(viewport.getCurrentTool()))
            {
                lassoTool->mouseDown(event, viewport);
                lassoTool->mouseDrag(event, viewport);
                return; // LassoTool now handles dragging
            }
        }

        viewport.setSnap(viewport.getTimeLine()->isSnappingEnabled() && !event.mods.isShiftDown());

        auto* clickedNote = viewport.getClickedNote();
        if (m_dragClip == nullptr || !viewport.getCachedMidiClips().contains(m_dragClip.get())
            || !m_dragClip->state.isAChildOf(m_evs.m_edit.state)
            || !m_dragClip->getSequence().getNotes().contains(clickedNote))
        {
            resetDrag(viewport);
            return;
        }
        const auto resolver = viewport.getTimeLine()->getMouseSnapResolver();
        const double newBeat = m_timeGesture.update(event.position.x, resolver, event.mods.isShiftDown());
        juce::Array<MidiNoteGesture::Item> items;
        for (auto* note : viewport.getSelectedNotes())
            if (auto* clip = viewport.getSelectedEvents().clipForEvent(note))
                items.add({clip, note});
        const double sourceEdgeTime = MidiNoteGesture::edgeTime({m_dragClip.get(), clickedNote}, gestureKind());
        const double delta = MidiNoteGesture::constrain(items, gestureKind(), resolver.beatToTime(newBeat) - sourceEdgeTime);
        m_timeGesture.setDisplayedBeat(resolver.timeToBeat(sourceEdgeTime + delta));
        m_draggedTimeDelta = m_leftTimeDelta = m_rightTimeDelta = 0;
        if (m_currentDragMode == DragMode::resizeLeft)
            m_leftTimeDelta = delta;
        else if (m_currentDragMode == DragMode::resizeRight)
            m_rightTimeDelta = delta;
        else if (m_currentDragMode == DragMode::moveNotes)
        {
            m_draggedTimeDelta = delta;
            if (viewport.isRefreshingMouseSnapContext())
            {
                m_pitchAnchorKey = viewport.getNoteNumber(event.y);
                m_pitchAnchorDelta = m_draggedNoteDelta;
            }
            else
                m_draggedNoteDelta = m_pitchAnchorDelta + viewport.getNoteNumber(event.y) - m_pitchAnchorKey;
            for (const auto& item : items)
                m_draggedNoteDelta = juce::jlimit(-item.note->getNoteNumber(), 127 - item.note->getNoteNumber(), m_draggedNoteDelta);
            if (!m_hasPlayedDragGuideNote || m_draggedNoteDelta != m_lastGuideNoteDelta)
            {
                if (auto* track = m_dragClip->getAudioTrack())
                    track->turnOffGuideNotes();
                for (const auto& item : items)
                    viewport.playGuideNote(item.clip, item.note->getNoteNumber() + m_draggedNoteDelta, item.note->getVelocity());
                m_hasPlayedDragGuideNote = true;
                m_lastGuideNoteDelta = m_draggedNoteDelta;
            }
        }
        const auto timing = previewTiming(m_dragClip, clickedNote);
        const auto lane = viewport.getNoteLane(clickedNote->getNoteNumber() + m_draggedNoteDelta);
        viewport.publishNoteInteractionPreview({timing.startBeat + m_dragClip->getStartBeat().inBeats()
            - m_dragClip->getOffsetInBeats().inBeats(), timing.lengthBeats, clickedNote->getNoteNumber() + m_draggedNoteDelta,
            clickedNote->getVelocity(), items.size()}, m_timeGesture.feedback(), lane, lane.getStart() + lane.getLength() * 0.5f, clickedNote->state);
        viewport.repaint();
    }

    m_lastDragPos = event.getPosition();
}

void PointerTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (m_isDragging)
        mouseDrag(event, viewport);
    if (!m_isDragging)
    {
        // Click without dragging - might be a single click on empty space
        if (viewport.getNoteByPos(event.position.toFloat()) == nullptr)
        {
            if (!event.mods.isShiftDown())
            {
                viewport.unselectAll();
            }
        }

        // If we had a pending lasso start but the user didn't drag (i.e. just clicked), clear it.
        m_pendingLassoStart = false;
    }
    else
    {
        // A local helper structure to hold all necessary information for a pending note creation.
        struct NoteOperationInfo
        {
            te::MidiClip *targetClip;
            tracktion::BeatPosition startBeat;
            tracktion::BeatDuration length;
            int noteNumber;
            juce::ValueTree noteState;
        };

        if (viewport.getSelectedEvents().getNumSelected() > 0)
        {
            MidiViewport::InteractionCommitScope commitScope(viewport);
            auto &um = m_evs.m_edit.getUndoManager();
            const bool copy = event.mods.isCtrlDown();
            um.beginNewTransaction(copy ? "Copy MIDI Notes" : "Move MIDI Notes");

            juce::Array<NoteOperationInfo> plannedNotes;
            auto selectedNotes = viewport.getSelectedNotes();

            // --- PHASE 1: Collect Info & Prepare ---
            for (auto *note : selectedNotes)
            {
                auto *clip = viewport.getSelectedEvents().clipForEvent(note);
                if (clip == nullptr)
                    continue;

                const auto timing = previewTiming(clip, note);
                plannedNotes.add({clip,
                                  tracktion::BeatPosition::fromBeats(timing.startBeat),
                                  tracktion::BeatDuration::fromBeats(timing.lengthBeats),
                                  note->getNoteNumber() + m_draggedNoteDelta,
                                  note->state.createCopy()});

                if (!copy)
                    clip->getSequence().removeNote(*note, &um);
            }

            viewport.unselectAll();

            // --- PHASE 2: Clear Target Area ---
            std::map<std::pair<te::MidiClip *, int>, juce::Array<tracktion::BeatRange>> rangesByPitch;
            for (const auto &noteInfo : plannedNotes)
                rangesByPitch[{noteInfo.targetClip, noteInfo.noteNumber}].add({noteInfo.startBeat, noteInfo.startBeat + noteInfo.length});

            for (auto &[key, ranges] : rangesByPitch)
                viewport.cleanUnderNoteRanges(key.second, ranges, key.first);

            // --- PHASE 3: Create New Notes & Update Selection ---
            for (const auto &noteInfo : plannedNotes)
            {
                auto newState = noteInfo.noteState.createCopy();
                newState.setProperty(te::IDs::p, noteInfo.noteNumber, nullptr);
                newState.setProperty(te::IDs::b, noteInfo.startBeat.inBeats(), nullptr);
                newState.setProperty(te::IDs::l, noteInfo.length.inBeats(), nullptr);

                auto *newNote = noteInfo.targetClip->getSequence().addNote(te::MidiNote(newState), &um);
                viewport.setNoteSelected(newNote, true);
            }

        }
    }

    resetDrag(viewport);
}

MidiNoteGesture::Kind PointerTool::gestureKind() const
{
    return m_currentDragMode == DragMode::resizeLeft ? MidiNoteGesture::Kind::resizeLeft
         : m_currentDragMode == DragMode::resizeRight ? MidiNoteGesture::Kind::resizeRight : MidiNoteGesture::Kind::move;
}

MidiNoteGesture::Timing PointerTool::previewTiming(te::MidiClip* clip, te::MidiNote* note) const
{
    return MidiNoteGesture::resolve({clip, note}, gestureKind(), m_draggedTimeDelta + m_leftTimeDelta + m_rightTimeDelta);
}

void PointerTool::resetDrag(MidiViewport& viewport)
{
    m_currentDragMode = DragMode::none;
    m_isDragging = false;
    m_draggedTimeDelta = m_leftTimeDelta = m_rightTimeDelta = 0;
    m_draggedNoteDelta = 0;
    m_hasPlayedDragGuideNote = false;
    m_lastGuideNoteDelta = 0;
    m_timeGesture.reset();
    m_dragClip = nullptr;
    viewport.clearNoteInteractionPreview();
    viewport.cleanUpFlags();
    viewport.repaint();
}

void PointerTool::toolDeactivated(MidiViewport& viewport)
{
    resetDrag(viewport);
    m_pendingLassoStart = false;
}

void PointerTool::insertNoteAtPosition(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto clip = viewport.getClickedClip())
    {
        viewport.unselectAll();
        if (auto note = viewport.addNewNoteAt(event.x, event.y, clip))
        {
            viewport.setNoteSelected(note, false);
            auto noteNumber = note->getNoteNumber();
            // Play the new note as guide note
            viewport.playGuideNote(clip, noteNumber);
        }
    }
}

void PointerTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note))
        {
            auto noteRect = viewport.getNoteRect(clip, note);
            auto borderWidth = noteRect.getWidth() > 30 ? 10.0f : noteRect.getWidth() / 3.0f;

            if (std::abs(event.x - noteRect.getX()) < borderWidth)
            {
                viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, viewport.getCursorScale()));
                return;
            }
            else if (std::abs(event.x - noteRect.getRight()) < borderWidth)
            {
                viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, viewport.getCursorScale()));
                return;
            }
        }

        // Over a note, but not an edge
        viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, viewport.getCursorScale()));
    }
    else
    {
        // Not over any note
        viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

juce::MouseCursor PointerTool::getCursor(MidiViewport &viewport) const
{
    switch (m_currentDragMode)
    {
    case DragMode::resizeLeft:
        return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, viewport.getCursorScale());

    case DragMode::resizeRight:
        return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, viewport.getCursorScale());

    case DragMode::moveNotes:
        return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, viewport.getCursorScale());

    case DragMode::none:
    default:
        return juce::MouseCursor::NormalCursor;
    }
}

void PointerTool::updateCursor(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        auto noteRect = viewport.getNoteRect(viewport.getSelectedEvents().clipForEvent(note), note);
        float edgeTolerance = 3.0f;

        if (std::abs(event.x - noteRect.getX()) < edgeTolerance || std::abs(event.x - noteRect.getRight()) < edgeTolerance)
        {
            // Over note edge - show resize cursor
            return;
        }
    }
}
