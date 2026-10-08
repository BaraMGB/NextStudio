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
#include "MidiNoteGesture.h"

namespace
{
std::optional<double> resolveSplitBeat(const juce::MouseEvent& event, MidiViewport& viewport, te::MidiNote* note)
{
    auto* clip = viewport.getSelectedEvents().clipForEvent(note);
    if (!clip)
        return {};
    auto& timeline = *viewport.getTimeLine();
    const double raw = timeline.xToBeatPos(event.position.x).inBeats();
    const double proposed = event.mods.isShiftDown() ? raw : timeline.snapBeatForMouse(raw);
    const double base = clip->getStartBeat().inBeats() - clip->getOffsetInBeats().inBeats();
    return MidiNoteGesture::validSplitBeat(note->getStartBeat().inBeats() + base, note->getEndBeat().inBeats() + base, proposed);
}
}

void KnifeTool::mouseDown(const juce::MouseEvent &event, MidiViewport &viewport)
{
    if (auto *note = viewport.getNoteByPos(event.position.toFloat()))
    {
        if (auto *clip = viewport.getSelectedEvents().clipForEvent(note))
        {
            if (const auto split = resolveSplitBeat(event, viewport, note))
            {
                const double base = clip->getStartBeat().inBeats() - clip->getOffsetInBeats().inBeats();
                const double start = note->getStartBeat().inBeats() + base;
                const double end = note->getEndBeat().inBeats() + base;
                te::MidiNote second(note->state.createCopy());
                second.setStartAndLength(tracktion::BeatPosition::fromBeats(*split - base),
                                         tracktion::BeatDuration::fromBeats(end - *split), nullptr);
                auto& um = m_evs.m_edit.getUndoManager();
                um.beginNewTransaction("Split MIDI Note");
                note->setStartAndLength(note->getStartBeat(), tracktion::BeatDuration::fromBeats(*split - start), &um);
                clip->getSequence().addNote(second, &um);
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
    viewport.getTimeLine()->clearMouseFeedback(TimelineFeedbackOwner::notes);
    m_shouldDrawSplitLine = false;
    viewport.repaint();
}

void KnifeTool::mouseMove(const juce::MouseEvent &event, MidiViewport &viewport)
{
    viewport.setMouseCursor(getCursor(viewport));

    m_shouldDrawSplitLine = false;
    m_hoveredNote = nullptr;
    viewport.getTimeLine()->clearMouseFeedback(TimelineFeedbackOwner::notes);
    if (auto* note = viewport.getNoteByPos(event.position))
        if (const auto split = resolveSplitBeat(event, viewport, note))
        {
            m_shouldDrawSplitLine = true;
            m_hoveredNote = note;
            auto& timeline = *viewport.getTimeLine();
            m_splitLineX = timeline.beatsToX(*split);
            const auto lane = viewport.getNoteLane(note->getNoteNumber());
            timeline.setMouseFeedback(TimelineInteractionFeedback{
                timeline.getMouseSnapResolver().resolveForMouse(timeline.xToBeatPos(event.position.x).inBeats(), event.mods.isShiftDown())
                    .withEffectiveBeat(*split), TimelineFeedbackOwner::notes, lane, lane.getStart() + lane.getLength() * 0.5f},
                [safe = juce::Component::SafePointer<MidiViewport>(&viewport)]
                { if (safe) safe->refreshMouseSnapContext(); });
        }

    viewport.repaint();
}

void KnifeTool::mouseDoubleClick(const juce::MouseEvent &event, MidiViewport &viewport)
{
    // No double click action
}

juce::MouseCursor KnifeTool::getCursor(MidiViewport &viewport) const { return GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Split, viewport.getCursorScale()); }

void KnifeTool::toolActivated(MidiViewport &viewport) { viewport.setMouseCursor(getCursor(viewport)); }

void KnifeTool::toolDeactivated(MidiViewport &viewport)
{
    m_shouldDrawSplitLine = false;
    viewport.getTimeLine()->clearMouseFeedback(TimelineFeedbackOwner::notes);
    viewport.setMouseCursor(juce::MouseCursor::NormalCursor);
    viewport.repaint();
}
