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

#include "LowerRange/PianoRoll/MidiViewport.h"
#include "Tools/ToolStrategy.h"

/**
 * Pointer tool for selecting and moving notes.
 * Handles single note selection, multi-note selection, and note dragging.
 */
class PointerTool : public ToolStrategy
{
public:
    explicit PointerTool(EditViewState &evs)
        : ToolStrategy(evs)
    {
    }
    ~PointerTool() override = default;

    void mouseDown(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseUp(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseMove(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport) override;

    juce::MouseCursor getCursor(MidiViewport &viewport) const override;

    Tool getToolId() override { return Tool::pointer; }
    // Public getters for MidiViewport's paint method
    bool isDragging() const { return m_isDragging; }
    double getDraggedTimeDelta() const { return m_draggedTimeDelta; }
    int getDraggedNoteDelta() const { return m_draggedNoteDelta; }
    double getLeftTimeDelta() const { return m_leftTimeDelta; }
    double getRightTimeDelta() const { return m_rightTimeDelta; }

private:
    enum class DragMode
    {
        none,
        moveNotes,
        resizeLeft,
        resizeRight
    };

    DragMode m_currentDragMode = DragMode::none;
    juce::Point<int> m_dragStartPos;
    juce::Point<int> m_lastDragPos;
    bool m_isDragging = false;

    // Drag state
    double m_draggedTimeDelta{0.0};
    int m_draggedNoteDelta{0};
    double m_leftTimeDelta{0.0};
    double m_rightTimeDelta{0.0};
    bool m_hasPlayedDragGuideNote{false};
    int m_lastGuideNoteDelta{0};

    // Helper methods
    void updateCursor(const juce::MouseEvent &event, MidiViewport &viewport);
    void insertNoteAtPosition(const juce::MouseEvent &event, MidiViewport &viewport);

    // When clicking on empty space we defer starting the Lasso until the user drags.
    // This flag indicates that a mouseDown on empty space occurred and a subsequent
    // mouseDrag should start the LassoTool. It must persist across mouse events.
    bool m_pendingLassoStart{false};
};
