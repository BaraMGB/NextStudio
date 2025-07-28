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



void PointerTool::mouseDown(const juce::MouseEvent& event, MidiViewport& viewport)
{
    m_dragStartPos = event.getPosition();
    m_lastDragPos = m_dragStartPos;
    m_isDragging = false;

    viewport.setClickedNote(nullptr);
    viewport.setClickedClip(nullptr);

    if (auto* note = viewport.getNoteByPos(event.position.toFloat()))
    {
        if (auto* clip = viewport.getSelectedEvents().clipForEvent(note))
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

            if (!event.mods.isShiftDown() && !viewport.isSelected(note))
                viewport.unselectAll();

            if (!viewport.isSelected(note))
                viewport.setNoteSelected(note, event.mods.isShiftDown());
        }
    }
    else
    {
        m_currentDragMode = DragMode::selectNotes;
        if (!event.mods.isShiftDown())
        {
            viewport.unselectAll();
        }
        viewport.startLasso(event);
    }
}

void PointerTool::mouseDoubleClick(const juce::MouseEvent& event, MidiViewport& viewport)
{
    GUIHelpers::log("PointerTRool Mouse doublel click");
    viewport.setClickedClip(viewport.getClipAt(event.x));
    insertNoteAtPosition(event, viewport);
}

void PointerTool::mouseDrag(const juce::MouseEvent& event, MidiViewport& viewport)
{
    if (!m_isDragging && event.getDistanceFromDragStart() > 5)
        m_isDragging = true;
    
    if (m_isDragging)
    {
        viewport.setSnap(true);
        if (event.mods.isShiftDown())
            viewport.setSnap(false);

        int deltaX = event.x - m_lastDragPos.x;
        int deltaY = event.y - m_lastDragPos.y;
        
        switch (m_currentDragMode)
        {
            case DragMode::moveNotes:
                // Only update view if we have a clicked note
                if (viewport.getClickedNote() != nullptr)
                {
                    viewport.updateViewOfMoveSelectedNotes(event);
                }
                break;
                
            case DragMode::resizeLeft:
                if (viewport.getClickedNote() != nullptr)
                {
                    viewport.setLeftEdgeDraggingTime(event);

                }
                break;

                
            case DragMode::resizeRight:
                if (viewport.getClickedNote() != nullptr)
                {
                    viewport.setRightEdgeDraggingTime(event);
                }
                // Handle note resizing
                // This would need to be implemented in MidiViewport
                break;
                
            case DragMode::selectNotes:
                viewport.updateLasso(event);
                break;
                
            case DragMode::none:
                break;
        }
    }
    
    m_lastDragPos = event.getPosition();
}

void PointerTool::mouseUp(const juce::MouseEvent& event, MidiViewport& viewport)
{
    if (m_currentDragMode == DragMode::selectNotes)
    {
        viewport.stopLasso();
        if (!m_isDragging)
        {
            // Single click - select note under cursor
            if (auto* note = viewport.getNoteByPos(event.position.toFloat()))
            {
                if (!event.mods.isShiftDown())
                {
                    viewport.unselectAll();
                }
                viewport.setNoteSelected(note, event.mods.isShiftDown());
            }
        }
    }
    else if (!m_isDragging)
    {
        // Click without dragging - might be a single click on empty space
        if (viewport.getNoteByPos(event.position.toFloat()) == nullptr)
        {
            if (!event.mods.isShiftDown())
            {
                viewport.unselectAll();
            }
        }
    }
    else
    {
        viewport.performNoteMoveOrCopy(event.mods.isCtrlDown());
    }
    
    m_currentDragMode = DragMode::none;
    m_isDragging = false;

    viewport.cleanUpFlags();
}

void PointerTool::insertNoteAtPosition(const juce::MouseEvent& event, MidiViewport& viewport)
{
    if (auto clip = viewport.getClickedClip())
    {
        if (auto note = viewport.addNewNoteAt(event.x, event.y, clip))
        {
            auto noteNumber = note->getNoteNumber();
            // Play the new note as guide note
            // viewport.playGuideNote(clip, noteNumber);
        }
    }
}

void PointerTool::mouseMove(const juce::MouseEvent& event, MidiViewport& viewport)
{
    if (auto* note = viewport.getNoteByPos(event.position.toFloat()))
    {
        if (auto* clip = viewport.getSelectedEvents().clipForEvent(note))
        {
            auto noteRect = viewport.getNoteRect(clip, note);
            auto borderWidth = noteRect.getWidth() > 30 ? 10.0f : noteRect.getWidth() / 3.0f;

            if (std::abs(event.x - noteRect.getX()) < borderWidth)
            {
                viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, viewport));
                return;
            }
            else if (std::abs(event.x - noteRect.getRight()) < borderWidth)
            {
                viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, viewport));
                return;
            }
        }
        
        // Over a note, but not an edge
        viewport.setMouseCursor(GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, viewport));
    }
    else
    {
        // Not over any note
        viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

juce::MouseCursor PointerTool::getCursor() const
{
    switch (m_currentDragMode)
    {
        case DragMode::resizeLeft:
        case DragMode::resizeRight:
            return juce::MouseCursor::LeftRightResizeCursor;
            
        case DragMode::moveNotes:
            return juce::MouseCursor::DraggingHandCursor;
            
        case DragMode::selectNotes:
            return juce::MouseCursor::CrosshairCursor;
            
        case DragMode::none:
        default:
            return juce::MouseCursor::NormalCursor;
    }
}

void PointerTool::updateCursor(const juce::MouseEvent& event, MidiViewport& viewport)
{
    if (auto* note = viewport.getNoteByPos(event.position.toFloat()))
    {
        auto noteRect = viewport.getNoteRect(viewport.getSelectedEvents().clipForEvent(note), note);
        float edgeTolerance = 3.0f;
        
        if (std::abs(event.x - noteRect.getX()) < edgeTolerance ||
            std::abs(event.x - noteRect.getRight()) < edgeTolerance)
        {
            // Over note edge - show resize cursor
            return;
        }
    }
    
    // Default cursor
}
