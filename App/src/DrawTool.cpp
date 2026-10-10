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

bool DrawTool::clipIsValid(MidiViewport& viewport) const
{
    return m_clickedClip != nullptr && viewport.getCachedMidiClips().contains(m_clickedClip.get())
        && m_clickedClip->state.isAChildOf(viewport.getTimeLine()->getEditViewState().m_edit.state)
        && m_gesture.startBeat() >= m_clickedClip->getStartBeat().inBeats() - m_clickedClip->getOffsetInBeats().inBeats();
}

void DrawTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    cancel(viewport);
    if (!event.mods.isLeftButtonDown())
        return;
    m_clickedClip = viewport.getClipAt(event.position.x);
    if (m_clickedClip == nullptr)
        return;
    auto& timeline = *viewport.getTimeLine();
    const bool bypass = event.mods.isShiftDown();
    viewport.setSnap(timeline.isSnappingEnabled() && !bypass);
    double start = timeline.xToBeatPos(event.position.x).inBeats();
    if (viewport.isSnapping())
        start = timeline.getMouseSnapResolver().startAtOrBefore(start);
    // The engine stores nonnegative internal MIDI positions.
    start = std::max(start, m_clickedClip->getStartBeat().inBeats() - m_clickedClip->getOffsetInBeats().inBeats());
    m_gesture.begin(start, timeline.getNoteInsertLength(), event.position.x, timeline.getMouseSnapResolver(), bypass);
    m_drawNoteNumber = viewport.getNoteNumber(event.y);
    mouseDrag(event, viewport);
}

void DrawTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.setSnap(viewport.getTimeLine()->isSnappingEnabled() && !event.mods.isShiftDown());

    if (!isDrawing())
        return;
    if (!clipIsValid(viewport))
    {
        cancel(viewport);
        return;
    }
    m_gesture.update(event.position.x, viewport.getTimeLine()->getMouseSnapResolver(), event.mods.isShiftDown(),
                     event.mouseWasDraggedSinceMouseDown() && event.position.x != event.getMouseDownPosition().x);
    const auto lane = viewport.getNoteLane(m_drawNoteNumber);
    viewport.publishNoteInteractionPreview({m_gesture.startBeat(), m_gesture.endBeat() - m_gesture.startBeat(),
        m_drawNoteNumber, m_evs.m_lastVelocity, viewport.getSelectedNotes().size(), true}, m_gesture.feedback(),
        lane, lane.getStart() + lane.getLength() * 0.5f, m_clickedClip->state);
    viewport.repaint();
}

void DrawTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (!isDrawing())
        return;
    mouseDrag(event, viewport); // final position/modifiers use the same preview calculation
    if (!isDrawing() || !clipIsValid(viewport))
    {
        cancel(viewport);
        return;
    }
    MidiViewport::InteractionCommitScope commitScope(viewport);
    const double base = m_clickedClip->getStartBeat().inBeats() - m_clickedClip->getOffsetInBeats().inBeats();
    if (auto* note = viewport.addNewNote(m_drawNoteNumber, m_clickedClip, m_gesture.startBeat() - base,
                                        m_gesture.endBeat() - m_gesture.startBeat()))
    {
        viewport.unselectAll();
        viewport.setNoteSelected(note, false);
    }
    cancel(viewport);
}

void DrawTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // MidiViewport already forwards mouseDown for this event. The normal
    // mouseUp commits that gesture once; do not insert a second note here.
    juce::ignoreUnused(event, viewport);
}

juce::MouseCursor DrawTool::getCursor(MidiViewport &viewport) const { return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Draw, viewport.getCursorScale()); }

void DrawTool::cancel(MidiViewport& viewport)
{
    m_gesture.reset();
    m_clickedClip = nullptr;
    m_drawNoteNumber = 0;
    viewport.clearNoteInteractionPreview();
    viewport.repaint();
}

void DrawTool::toolDeactivated(MidiViewport &viewport)
{
    cancel(viewport);
    viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
}
