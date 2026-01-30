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

#include "EraserTool.h"
#include "../Utilities.h"

void EraserTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    m_isDragging = false;
    m_notesToDelete.clear();
    m_affectedClips.clear();

    deleteNoteAtPosition(event, viewport);
}

void EraserTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (!m_isDragging && event.getDistanceFromDragStart() > 3)
    {
        m_isDragging = true;
    }

    if (m_isDragging)
    {
        deleteNoteAtPosition(event, viewport);
        viewport.repaint();
    }
}

void EraserTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (m_isDragging)
    {
        // Commit all deletions at once for better undo behavior
        commitDeletions(viewport);
    }

    // Reset state
    m_isDragging = false;
    m_notesToDelete.clear();
    m_affectedClips.clear();

    viewport.repaint();
}

void EraserTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // Clear previous highlight
    if (m_lastHoveredNote)
    {
        clearHighlights(viewport);
        m_lastHoveredNote = nullptr;
    }

    // Highlight note under cursor for potential deletion
    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        m_lastHoveredNote = note;
        highlightNoteForDeletion(note, viewport);
    }

    viewport.repaint();
}

void EraserTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // Double-click deletes all notes in the same clip at the same time position
    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note))
        {
            auto noteStartBeat = note->getStartBeat();
            auto &sequence = clip->getSequence();

            // Find all notes that start at the same beat
            juce::Array<te::MidiNote *> notesToDelete;
            for (auto *n : sequence.getNotes())
            {
                auto beatDiff = n->getStartBeat() - noteStartBeat;
                if (beatDiff.inBeats() < 0.001 && beatDiff.inBeats() > -0.001) // Small tolerance for floating point comparison
                {
                    notesToDelete.add(n);
                }
            }

            // Delete all notes at this time position
            auto &um = m_evs.m_edit.getUndoManager();
            for (auto *n : notesToDelete)
            {
                sequence.removeNote(*n, &um);
            }

            viewport.repaint();
        }
    }
}

juce::MouseCursor EraserTool::getCursor(MidiViewport &viewport) const
{
    // Use a custom eraser cursor or crosshair as fallback
    return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Erasor, viewport.getCursorScale());
}

void EraserTool::toolActivated(MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void EraserTool::toolDeactivated(MidiViewport &viewport)
{
    // Clear any highlights
    clearHighlights(viewport);

    // Reset state
    m_isDragging = false;
    m_notesToDelete.clear();
    m_affectedClips.clear();
    m_lastHoveredNote = nullptr;

    viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
}

void EraserTool::deleteNoteAtPosition(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        // Check if we already marked this note for deletion
        if (m_notesToDelete.contains(note))
            return;

        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note))
        {
            if (m_isDragging)
            {
                // During drag, collect notes for batch deletion
                m_notesToDelete.add(note);
                if (!m_affectedClips.contains(clip))
                    m_affectedClips.add(clip);
            }
            else
            {
                // Single click - delete immediately
                auto &sequence = clip->getSequence();
                sequence.removeNote(*note, &m_evs.m_edit.getUndoManager());

                viewport.repaint();
            }
        }
    }
}

void EraserTool::highlightNoteForDeletion(te::MidiNote *note, MidiViewport &viewport)
{
    // This could be implemented to show visual feedback for the note that would be deleted
    // For now, we'll just change the cursor
    viewport.setMouseCursor(getCursor(viewport));
}

void EraserTool::clearHighlights(MidiViewport &viewport)
{
    // Clear any visual highlights
    // This would remove deletion preview indicators
}

void EraserTool::commitDeletions(MidiViewport &viewport)
{
    if (m_notesToDelete.isEmpty())
        return;

    // Group deletions by clip for better undo behavior
    auto &um = m_evs.m_edit.getUndoManager();
    um.beginNewTransaction("Delete MIDI Notes");

    for (auto *clip : m_affectedClips)
    {
        auto &sequence = clip->getSequence();

        // Delete all notes in this clip that were marked for deletion
        for (auto *note : m_notesToDelete)
        {
            if (viewport.getSelectedEvents().clipForEvent(note) == clip)
            {
                sequence.removeNote(*note, &um);
            }
        }
    }
}
