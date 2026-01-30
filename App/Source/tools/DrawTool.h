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

#include "../MidiViewport.h"
#include "../ToolStrategy.h"

namespace te = tracktion_engine;

/**
 * Draw tool for creating new MIDI notes.
 * Creates notes by clicking and dragging to set length.
 */
class DrawTool : public ToolStrategy
{
public:
    explicit DrawTool(EditViewState &evs)
        : ToolStrategy(evs)
    {
    }
    ~DrawTool() override = default;

    void mouseDown(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseUp(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseMove(const juce::MouseEvent &event, MidiViewport &viewport) override;
    void mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport) override;

    juce::MouseCursor getCursor(MidiViewport &viewport) const override;

    void toolActivated(MidiViewport &viewport) override;
    void toolDeactivated(MidiViewport &viewport) override;
    Tool getToolId() override { return Tool::draw; }

    // Public getters for MidiViewport's paint method
    bool isDrawing() const { return m_isDrawingNote; }
    int getDrawStartPos() const { return m_drawStartPos; }
    int getDrawCurrentPos() const { return m_drawCurrentPos; }
    int getDrawNoteNumber() const { return m_drawNoteNumber; }
    te::MidiClip *getClickedClip() const { return m_clickedClip; }

private:
    bool m_isDrawingNote{false};
    int m_drawStartPos{0};
    int m_drawCurrentPos{0};
    int m_intervalX{0};
    int m_drawNoteNumber{0};
    te::MidiClip *m_clickedClip{nullptr};
};
