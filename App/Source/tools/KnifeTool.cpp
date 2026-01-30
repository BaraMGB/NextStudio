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

#include "KnifeTool.h"

void KnifeTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto *note = viewport.getNoteByPos(event.position.toFloat())) {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note)) {
            auto &um = m_evs.m_edit.getUndoManager();
            um.beginNewTransaction("Split MIDI Note");

            auto time = viewport.getTimeLine()->xToTimePos(event.x).inSeconds();
            if (viewport.isSnapping() && !event.mods.isShiftDown())
                time = viewport.getTimeLine()->getSnappedTime(time);

            auto splitBeat = m_evs.timeToBeat(time);
            auto noteStartBeat = note->getStartBeat().inBeats() + clip->getStartBeat().inBeats();
            auto noteEndBeat = note->getEndBeat().inBeats() + clip->getStartBeat().inBeats();

            if (splitBeat > noteStartBeat && splitBeat < noteEndBeat) {
                auto originalEndBeat = noteEndBeat; // Absolute Endbeat-Position verwenden

                // 1. Truncate the original note
                note->setStartAndLength(note->getStartBeat(), tracktion::BeatDuration::fromBeats(splitBeat - noteStartBeat), &um);

                // 2. Create the new note for the second part
                clip->getSequence().addNote(
                    note->getNoteNumber(),
                    tracktion::BeatPosition::fromBeats(splitBeat - clip->getStartBeat().inBeats()), // Relative Position zum Clip
                    tracktion::BeatDuration::fromBeats(originalEndBeat - splitBeat),                // Korrekte Länge berechnen
                    note->getVelocity(),
                    note->getColour(),
                    &um);
            }
        }
    }
}

void KnifeTool::mouseDrag(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // No drag action for the knife tool
}

void KnifeTool::mouseUp(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // No special action needed on mouse up
}

void KnifeTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.setMouseCursor(getCursor(viewport));

    // Check if hovering over a note and update split line position
    if (auto *note = viewport.getNoteByPos(event.position.toFloat())) {
        m_shouldDrawSplitLine = true;
        m_hoveredNote = note;

        // Calculate snapped position for the line
        auto time = viewport.getTimeLine()->xToTimePos(event.x).inSeconds();
        if (viewport.isSnapping() && !event.mods.isShiftDown())
            time = viewport.getTimeLine()->getSnappedTime(time);

        auto splitBeat = m_evs.timeToBeat(time);
        m_splitLineX = viewport.getTimeLine()->beatsToX(splitBeat);
    }
    else {
        m_shouldDrawSplitLine = false;
        m_hoveredNote = nullptr;
    }

    viewport.repaint();
}

void KnifeTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // No double click action
}

juce::MouseCursor KnifeTool::getCursor(MidiViewport &viewport) const
{
    return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Split, viewport.getCursorScale());
}

void KnifeTool::toolActivated(MidiViewport &viewport)
{
    viewport.setMouseCursor(getCursor(viewport));
}

void KnifeTool::toolDeactivated(MidiViewport &viewport)
{
    m_shouldDrawSplitLine = false;
    viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
    viewport.repaint();
}
