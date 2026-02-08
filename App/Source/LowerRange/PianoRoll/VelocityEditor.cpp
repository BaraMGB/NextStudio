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

#include "LowerRange/PianoRoll/VelocityEditor.h"
void VelocityEditor::paint(juce::Graphics &g)
{
    drawBarsAndBeatLines(g, juce::Colour(0x77ffffff));
    g.setColour(juce::Colour(0x77ffffff));

    for (auto &midiClip : EngineHelpers::getMidiClipsOfTrack(*m_track))
    {
        auto &seq = midiClip->getSequence();
        for (auto n : seq.getNotes())
        {
            drawVelocityRuler(g, midiClip, n);
        }
    }
}

void VelocityEditor::mouseDown(const juce::MouseEvent &)
{
    m_dragVelocityStates.clear();
    m_dragReferenceNote = nullptr;

    if (auto *hoveredNote = getHoveredNote())
    {
        m_dragReferenceNote = hoveredNote;

        if (auto *selectedEvents = m_editViewState.m_selectionManager.getFirstItemOfType<te::SelectedMidiEvents>(); selectedEvents != nullptr && selectedEvents->isSelected(hoveredNote))
        {
            for (auto *note : selectedEvents->getSelectedNotes())
                m_dragVelocityStates.add({note, note->getVelocity()});
        }

        if (m_dragVelocityStates.isEmpty())
            m_dragVelocityStates.add({hoveredNote, hoveredNote->getVelocity()});
    }
}

void VelocityEditor::mouseDrag(const juce::MouseEvent &e)
{
    if (m_dragVelocityStates.isEmpty())
        return;

    const int velocityDelta = -e.getDistanceFromDragStartY();
    int lastVelocity = m_editViewState.m_lastVelocity;
    bool updatedReferenceVelocity = false;

    for (const auto &state : m_dragVelocityStates)
    {
        if (state.note == nullptr)
            continue;

        const int velocity = juce::jlimit(0, 127, state.startVelocity + velocityDelta);
        state.note->setVelocity(velocity, &m_editViewState.m_edit.getUndoManager());

        if (!updatedReferenceVelocity && state.note == m_dragReferenceNote)
        {
            lastVelocity = velocity;
            updatedReferenceVelocity = true;
        }
        else if (!updatedReferenceVelocity)
        {
            lastVelocity = velocity;
        }
    }

    m_editViewState.m_lastVelocity = lastVelocity;
    repaint();
}

void VelocityEditor::mouseMove(const juce::MouseEvent &e)
{
    clearNotesFlags();
    if (auto note = getNote(e.position))
    {
        note->state.setProperty(IDs::isHovered, true, &m_editViewState.m_edit.getUndoManager());
    }
    repaint();
}

void VelocityEditor::mouseExit(const juce::MouseEvent &)
{
    clearNotesFlags();
    repaint();
}

void VelocityEditor::mouseUp(const juce::MouseEvent &)
{
    m_dragVelocityStates.clear();
    m_dragReferenceNote = nullptr;
}

void VelocityEditor::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) {}

void VelocityEditor::drawBarsAndBeatLines(juce::Graphics &g, juce::Colour colour)
{
    g.setColour(colour);
    double beatX1 = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
    double beatX2 = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, beatX1, beatX2, getBounds().toFloat());
}

void VelocityEditor::drawVelocityRuler(juce::Graphics &g, tracktion_engine::MidiClip *&midiClip, tracktion_engine::MidiNote *n)
{
    auto noteRangeX = getXLineRange(midiClip, n);
    auto velocityY = getVelocityPixel(n);

    g.setColour(juce::Colour(0xff666666));

    g.fillRect(juce::Rectangle<int>(noteRangeX.getStart() - 1, velocityY, 2, getHeight() - velocityY));
    g.setColour(juce::Colour(0xff181818));
    g.fillEllipse(noteRangeX.getStart() - 3, velocityY - 3, 6, 6);
    if (n->state.getPropertyAsValue(IDs::isHovered, &m_editViewState.m_edit.getUndoManager(), false) == true)
    {
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(m_track->getColour());
    }
    g.drawEllipse(noteRangeX.getStart() - 3, velocityY - 3, 6, 6, 2);
}

juce::Range<float> VelocityEditor::getXLineRange(te::MidiClip *const &midiClip, const te::MidiNote *n) const
{
    double sBeat = EngineHelpers::getNoteStartBeat(midiClip, n);
    double eBeat = EngineHelpers::getNoteEndBeat(midiClip, n);

    auto x1 = m_editViewState.beatsToX(sBeat + midiClip->getStartBeat().inBeats(), m_timeLineID, getWidth());
    auto x2 = m_editViewState.beatsToX(eBeat + midiClip->getStartBeat().inBeats(), m_timeLineID, getWidth()) + 1;

    return {x1, x2};
}

int VelocityEditor::getVelocity(int y) { return juce::jmap((getHeight() - 4) - y, 0, getHeight() - 8, 0, 127); }

int VelocityEditor::getVelocityPixel(const te::MidiNote *n) const { return (getHeight() - 4) - juce::jmap(n->getVelocity(), 0, 127, 0, getHeight() - 8); }

tracktion_engine::MidiNote *VelocityEditor::getNote(juce::Point<float> p)
{
    for (auto &mc : EngineHelpers::getMidiClipsOfTrack(*m_track))
    {
        for (auto note : mc->getSequence().getNotes())
        {
            auto y = getVelocityPixel(note);
            auto x = m_editViewState.beatsToX(EngineHelpers::getNoteStartBeat(mc, note) + mc->getStartBeat().inBeats(), m_timeLineID, getWidth());

            if (GUIHelpers::getSensibleArea(p, 10).contains(x, y))
            {
                return note;
            }
        }
    }
    return nullptr;
}

void VelocityEditor::clearNotesFlags()
{
    for (auto mc : EngineHelpers::getMidiClipsOfTrack(*m_track))
    {
        for (auto n : mc->getSequence().getNotes())
        {
            n->state.setProperty(IDs::isHovered, false, &m_editViewState.m_edit.getUndoManager());
        }
    }
}
te::MidiNote *VelocityEditor::getHoveredNote()
{
    for (auto mc : EngineHelpers::getMidiClipsOfTrack(*m_track))
    {
        for (auto n : mc->getSequence().getNotes())
        {
            if (n->state.getProperty(IDs::isHovered, false))
                return n;
        }
    }
    return nullptr;
}
