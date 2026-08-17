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

#include "DrawTool.h"
#include "Utilities.h"

void DrawTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    m_clickedClip = viewport.getClipAt(event.x);
    if (m_clickedClip == nullptr)
        return;

    viewport.setSnap(viewport.getTimeLine()->isSnappingEnabled() && !event.mods.isShiftDown());

    m_insertLengthBeats = viewport.getTimeLine()->getNoteInsertLength();
    m_intervalX = juce::jmax(1, juce::roundToInt(m_insertLengthBeats / viewport.getTimeLine()->getBeatsPerPixel()));

    m_isDrawingNote = true;
    m_drawStartPos = event.getPosition().x;
    m_drawCurrentPos = m_drawStartPos + m_intervalX;
    m_drawNoteNumber = viewport.getNoteNumber(event.y);

    viewport.repaint();
}

void DrawTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.setSnap(viewport.getTimeLine()->isSnappingEnabled() && !event.mods.isShiftDown());

    if (!m_isDrawingNote)
        return;

    m_drawCurrentPos = juce::jmax(event.getPosition().x, m_drawStartPos + m_intervalX);
    viewport.repaint();
}

void DrawTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (!m_isDrawingNote)
        return;

    auto startBeat = viewport.getTimeLine()->xToBeatPos(m_drawStartPos).inBeats() - m_clickedClip->getStartBeat().inBeats();
    if (viewport.isSnapping())
        startBeat = viewport.getTimeLine()->getQuantisedNoteBeat(startBeat, m_clickedClip);

    auto endBeat = viewport.getTimeLine()->xToBeatPos(m_drawCurrentPos).inBeats() - m_clickedClip->getStartBeat().inBeats();
    if (viewport.isSnapping())
        endBeat = viewport.getTimeLine()->getQuantisedNoteBeat(endBeat, m_clickedClip);
    endBeat = PianoRollNoteLength::applyMinimum(startBeat, endBeat, m_insertLengthBeats);

    const auto length = endBeat - startBeat;
    auto newNote = viewport.addNewNote(m_drawNoteNumber, m_clickedClip, startBeat, length);

    viewport.unselectAll();
    viewport.setNoteSelected(newNote, false);

    // Reset state
    m_clickedClip = nullptr;
    m_isDrawingNote = false;
    m_drawStartPos = 0;
    m_drawCurrentPos = 0;
    m_drawNoteNumber = 0;
    m_intervalX = 0;
    m_insertLengthBeats = PianoRollNoteLength::defaultLengthBeats;

    viewport.repaint();
}

void DrawTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // Update cursor based on context
    if (viewport.getClipAt(event.x))
    {
        viewport.setMouseCursor(getCursor(viewport));
    }
    else
    {
        viewport.setMouseCursor(juce::MouseCursor::NoCursor);
    }
}

void DrawTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // A double-click could create a note with a default length.
    mouseDown(event, viewport);
    mouseUp(event, viewport);
}

juce::MouseCursor DrawTool::getCursor(MidiViewport &viewport) const { return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Draw, viewport.getCursorScale()); }

void DrawTool::toolActivated(MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void DrawTool::toolDeactivated(MidiViewport &viewport)
{
    // Ensure any pending drawing operation is cancelled
    m_isDrawingNote = false;
    m_clickedClip = nullptr;
    m_drawNoteNumber = 0;
    m_drawCurrentPos = 0;
    m_drawNoteNumber = 0;
    m_intervalX = 0;
    m_insertLengthBeats = PianoRollNoteLength::defaultLengthBeats;

    viewport.repaint();
    viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
}
