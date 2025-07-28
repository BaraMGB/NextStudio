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

#pragma once

#include "../ToolStrategy.h"
#include "../MidiViewport.h"

/**
 * Pointer tool for selecting and moving notes.
 * Handles single note selection, multi-note selection, and note dragging.
 */
class PointerTool : public ToolStrategy
{
public:
    PointerTool() = default;
    ~PointerTool() override = default;
    
    void mouseDown(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseDrag(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseUp(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseMove(const juce::MouseEvent& event, MidiViewport& viewport) override;
    void mouseDoubleClick(const juce::MouseEvent& event, MidiViewport& viewport) override;
    
    juce::MouseCursor getCursor() const override;
    
private:
    enum class DragMode
    {
        none,
        moveNotes,
        resizeLeft,
        resizeRight,
        selectNotes
    };
    
    DragMode m_currentDragMode = DragMode::none;
    juce::Point<int> m_dragStartPos;
    juce::Point<int> m_lastDragPos;
    bool m_isDragging = false;
    
    // Helper methods
    void updateCursor(const juce::MouseEvent& event, MidiViewport& viewport);
    void insertNoteAtPosition(const juce::MouseEvent& event, MidiViewport& viewport);
};