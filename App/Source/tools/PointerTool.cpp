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

void PointerTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    m_dragStartPos = event.getPosition();
    m_lastDragPos = m_dragStartPos;
    m_isDragging = false;

    viewport.setClickedNote(nullptr);
    viewport.setClickedClip(nullptr);

    if (auto *note = viewport.getNoteByPos(event.position.toFloat())) {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note)) {
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

            if (!event.mods.isShiftDown() && !viewport.isSelected(note))
                viewport.unselectAll();

            if (!viewport.isSelected(note))
                viewport.setNoteSelected(note, event.mods.isShiftDown());
        }
    }
    else {
        // Empty space - don't immediately switch to LassoTool. Defer starting lasso until
        // the user drags the mouse to allow double-clicks to be detected by PointerTool.
        m_evs.m_selectionManager.deselectAll();
        m_pendingLassoStart = true;
    }
}

void PointerTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.setClickedClip(viewport.getClipAt(event.x));
    insertNoteAtPosition(event, viewport);
}

void PointerTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (!m_isDragging && event.getDistanceFromDragStart() > 5)
        m_isDragging = true;

    if (m_isDragging) {
        // If we had a pending lasso start (clicked empty space before dragging), start the lasso now.
        if (m_pendingLassoStart) {
            m_pendingLassoStart = false;
            viewport.setTool(Tool::lasso);
            if (auto *lassoTool = dynamic_cast<LassoTool *>(viewport.getCurrentTool())) {
                lassoTool->mouseDown(event, viewport);
                lassoTool->mouseDrag(event, viewport);
                return; // LassoTool now handles dragging
            }
        }

        viewport.setSnap(true);
        if (event.mods.isShiftDown())
            viewport.setSnap(false);

        switch (m_currentDragMode) {
        case DragMode::moveNotes:
            if (auto *clickedNote = viewport.getClickedNote()) {
                auto oldTime = clickedNote->getEditStartTime(*viewport.getClickedClip()).inSeconds();
                auto scroll = viewport.getTimeLine()->getCurrentTimeRange().getStart().inSeconds();
                auto newTime = oldTime + viewport.getTimeLine()->xToTimePos(event.getDistanceFromDragStartX()).inSeconds() - scroll;
                if (viewport.isSnapping())
                    newTime = viewport.getTimeLine()->getSnappedTime(newTime);

                m_draggedTimeDelta = newTime - oldTime;
                m_draggedNoteDelta = viewport.getNoteNumber(event.y) - clickedNote->getNoteNumber();

                viewport.getClickedClip()->getAudioTrack()->turnOffGuideNotes();

                for (auto n : viewport.getSelectedNotes())
                    viewport.playGuideNote(viewport.getSelectedEvents().clipForEvent(n),
                                           n->getNoteNumber() + m_draggedNoteDelta,
                                           n->getVelocity());
            }
            break;

        case DragMode::resizeLeft:
            if (auto *clickedNote = viewport.getClickedNote()) {
                auto oldTime = clickedNote->getEditStartTime(*viewport.getClickedClip()).inSeconds();
                auto scroll = viewport.getTimeLine()->getCurrentTimeRange().getStart().inSeconds();
                auto newTime = oldTime + viewport.getTimeLine()->xToTimePos(event.getDistanceFromDragStartX()).inSeconds() - scroll;
                if (viewport.isSnapping())
                    newTime = viewport.getTimeLine()->getSnappedTime(newTime);

                m_leftTimeDelta = newTime - oldTime;
                m_draggedTimeDelta = 0;
            }
            break;

        case DragMode::resizeRight:
            if (auto *clickedNote = viewport.getClickedNote()) {
                auto oldTime = clickedNote->getEditEndTime(*viewport.getClickedClip()).inSeconds();
                auto scroll = viewport.getTimeLine()->getCurrentTimeRange().getStart().inSeconds();
                auto newTime = oldTime + viewport.getTimeLine()->xToTimePos(event.getDistanceFromDragStartX()).inSeconds() - scroll;
                if (viewport.isSnapping())
                    newTime = viewport.getTimeLine()->getSnappedTime(newTime);

                m_rightTimeDelta = newTime - oldTime;
                m_draggedTimeDelta = 0;
            }
            break;

        case DragMode::none:
            break;
        }
        viewport.repaint();
    }

    m_lastDragPos = event.getPosition();
}

void PointerTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (!m_isDragging) {
        // Click without dragging - might be a single click on empty space
        if (viewport.getNoteByPos(event.position.toFloat()) == nullptr) {
            if (!event.mods.isShiftDown()) {
                viewport.unselectAll();
            }
        }

        // If we had a pending lasso start but the user didn't drag (i.e. just clicked), clear it.
        m_pendingLassoStart = false;
    }
    else {
        // A local helper structure to hold all necessary information for a pending note creation.
        struct NoteOperationInfo
        {
            te::MidiClip *targetClip;
            tracktion::BeatPosition startBeat;
            tracktion::BeatDuration length;
            int noteNumber;
            int velocity;
            int colour;
        };

        if (viewport.getSelectedEvents().getNumSelected() > 0) {
            auto &um = m_evs.m_edit.getUndoManager();
            const bool copy = event.mods.isCtrlDown();
            um.beginNewTransaction(copy ? "Copy MIDI Notes" : "Move MIDI Notes");

            juce::Array<NoteOperationInfo> plannedNotes;
            auto selectedNotes = viewport.getSelectedNotes();
            auto &tempoSequence = m_evs.m_edit.tempoSequence;

            // --- PHASE 1: Collect Info & Prepare ---
            for (auto *note : selectedNotes) {
                auto *clip = viewport.getSelectedEvents().clipForEvent(note);
                if (clip == nullptr)
                    continue;

                auto originalNoteStartTime = tempoSequence.toTime(note->getStartBeat());
                auto newNoteStartTime = originalNoteStartTime + tracktion::TimeDuration::fromSeconds(m_draggedTimeDelta);
                auto newNoteStartBeat = tempoSequence.toBeats(newNoteStartTime);
                auto beatDelta = newNoteStartBeat - note->getStartBeat();
                auto lengthDelta = m_evs.timeToBeat(m_leftTimeDelta * (-1) + (m_rightTimeDelta));

                plannedNotes.add({clip,
                                  note->getStartBeat() + beatDelta + tracktion::BeatDuration::fromBeats(m_evs.timeToBeat(m_leftTimeDelta)),
                                  note->getLengthBeats() + tracktion::BeatDuration::fromBeats(lengthDelta),
                                  note->getNoteNumber() + m_draggedNoteDelta,
                                  note->getVelocity(),
                                  note->getColour()});

                if (!copy) {
                    clip->getSequence().removeNote(*note, &um);
                }
            }

            viewport.unselectAll();

            // --- PHASE 2: Clear Target Area ---
            for (const auto &noteInfo : plannedNotes) {
                tracktion::BeatRange targetBeatRange(noteInfo.startBeat, noteInfo.startBeat + noteInfo.length);
                viewport.cleanUnderNote(noteInfo.noteNumber, targetBeatRange, noteInfo.targetClip);
            }

            // --- PHASE 3: Create New Notes & Update Selection ---
            for (const auto &noteInfo : plannedNotes) {
                auto *newNote = noteInfo.targetClip->getSequence().addNote(
                    noteInfo.noteNumber,
                    noteInfo.startBeat,
                    noteInfo.length,
                    noteInfo.velocity,
                    noteInfo.colour,
                    &um);
                viewport.setNoteSelected(newNote, true);
            }
        }
    }

    m_currentDragMode = DragMode::none;
    m_isDragging = false;
    m_draggedTimeDelta = 0.0;
    m_draggedNoteDelta = 0;
    m_leftTimeDelta = 0.0;
    m_rightTimeDelta = 0.0;

    viewport.cleanUpFlags();
    viewport.repaint();
}

void PointerTool::insertNoteAtPosition(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto clip = viewport.getClickedClip()) {
        if (auto note = viewport.addNewNoteAt(event.x, event.y, clip)) {
            auto noteNumber = note->getNoteNumber();
            // Play the new note as guide note
            viewport.playGuideNote(clip, noteNumber);
        }
    }
}

void PointerTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto *note = viewport.getNoteByPos(event.position.toFloat())) {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note)) {
            auto noteRect = viewport.getNoteRect(clip, note);
            auto borderWidth = noteRect.getWidth() > 30 ? 10.0f : noteRect.getWidth() / 3.0f;

            if (std::abs(event.x - noteRect.getX()) < borderWidth) {
                viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, viewport.getCursorScale()));
                return;
            }
            else if (std::abs(event.x - noteRect.getRight()) < borderWidth) {
                viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, viewport.getCursorScale()));
                return;
            }
        }

        // Over a note, but not an edge
        viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, viewport.getCursorScale()));
    }
    else {
        // Not over any note
        viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

juce::MouseCursor PointerTool::getCursor(MidiViewport &viewport) const
{
    switch (m_currentDragMode) {
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
    if (auto *note = viewport.getNoteByPos(event.position.toFloat())) {
        auto noteRect = viewport.getNoteRect(viewport.getSelectedEvents().clipForEvent(note), note);
        float edgeTolerance = 3.0f;

        if (std::abs(event.x - noteRect.getX()) < edgeTolerance ||
            std::abs(event.x - noteRect.getRight()) < edgeTolerance) {
            // Over note edge - show resize cursor
            return;
        }
    }
}
