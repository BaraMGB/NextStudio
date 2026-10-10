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

#include "VelocityEditor.h"

VelocityEditor::~VelocityEditor()
{
    if (m_viewport && m_viewport->ownsLasso(*this))
        m_viewport->cancelSelectionGesture(false);
}

void VelocityEditor::resized()
{
    if (m_viewport && m_viewport->ownsLasso(*this))
        m_viewport->refreshMouseSnapContext();
}

void VelocityEditor::modifierKeysChanged(const juce::ModifierKeys &mods)
{
    if (m_viewport && m_viewport->ownsLasso(*this))
        m_viewport->modifierKeysChanged(mods);
}

void VelocityEditor::updateToolCursor()
{
    const bool lasso = m_viewport && (m_viewport->ownsLasso(*this) || m_viewport->getCurrentToolType() == Tool::lasso);
    setMouseCursor(lasso ? GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::Lasso, m_viewport->getCursorScale()) : juce::MouseCursor::NormalCursor);
}

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
    if (m_viewport)
        m_viewport->drawLasso(g, *this);
}

void VelocityEditor::setNotePropertyPreview(const juce::Array<MidiNotePropertyEdit> &preview)
{
    m_notePropertyPreview = preview;
    repaint();
}

void VelocityEditor::mouseDown(const juce::MouseEvent &e)
{
    m_dragVelocityStates.clear();
    m_dragReferenceNote = nullptr;
    m_pressedMarker = {};
    m_markerDragged = false;
    m_pressInput.reset();
    if (!m_viewport || !e.mods.isLeftButtonDown())
        return;
    m_viewport->cancelActiveInteraction();
    // Complete property text before selection or marker hit/drag origins change,
    // including focus loss that JUCE has queued but not delivered yet.
    if (auto *timeline = m_viewport->getTimeLine(); timeline->onNoteInteractionBeginning)
        timeline->onNoteInteractionBeginning();
    m_viewport->finishPendingPasteOnDeselect();
    m_pressInput.remember(e);
    grabKeyboardFocus();

    auto *hoveredNote = getNote(e.position);
    const auto tool = m_viewport->getCurrentToolType();
    if (hoveredNote == nullptr && tool != Tool::pointer && tool != Tool::lasso)
        return;
    if (tool == Tool::lasso || hoveredNote == nullptr)
    {
        // Source geometry only: the viewport retains selection and cancellation ownership.
        m_viewport->startLasso(e, {this, [this](juce::Point<float> p) { return juce::Point<double>{m_editViewState.xToBeats(p.x, m_timeLineID, getWidth()), VelocityMarkerGeometry::velocityAt(p.y, getHeight())}; }, [this](juce::Point<double> p) { return juce::Point<float>{m_editViewState.beatsToX(p.x, m_timeLineID, getWidth()), VelocityMarkerGeometry::projectVelocity(p.y, getHeight())}; },
                                   [this](juce::Rectangle<float> rect)
                                   {
                                       MidiSelectionSnapshot::Items hits;
                                       for (auto *clip : m_viewport->getCachedMidiClips())
                                           for (auto *note : clip->getSequence().getNotes())
                                               if (LassoGesture::containsCentre(rect, getMarkerCentre(clip, note)))
                                                   hits.add(note->state);
                                       return hits;
                                   }});
        updateToolCursor();
        return;
    }

    if (hoveredNote != nullptr)
    {
        m_dragReferenceNote = hoveredNote;
        m_pressedMarker = hoveredNote->state;

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
    if (!m_pressInput.belongsToGesture(e) || !m_viewport)
        return;
    if (m_viewport->ownsLasso(*this))
    {
        m_viewport->updateLasso(e);
        return;
    }
    if (m_dragVelocityStates.isEmpty())
        return;

    // Even a sub-threshold motion may edit velocity. Never treat that release
    // as a selection click, or change the group midway through a marker drag.
    m_markerDragged = m_markerDragged || e.getDistanceFromDragStart() > 0;
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
        note->state.setProperty(IDs::isHovered, true, nullptr);
    }
    updateToolCursor();
    repaint();
}

void VelocityEditor::mouseExit(const juce::MouseEvent &)
{
    clearNotesFlags();
    repaint();
}

void VelocityEditor::mouseUp(const juce::MouseEvent &e)
{
    if (!m_pressInput.belongsToGesture(e))
        return;
    if (m_viewport && m_viewport->ownsLasso(*this))
    {
        if (e.mouseWasDraggedSinceMouseDown())
            m_viewport->updateLasso(e);
        const bool oneShot = m_viewport->getCurrentToolType() == Tool::lasso;
        m_viewport->stopLasso();
        if (oneShot)
            m_viewport->setTool(Tool::pointer);
    }
    else if (m_viewport && m_pressedMarker.isValid() && !m_markerDragged && !e.mouseWasDraggedSinceMouseDown())
    {
        // Resolve the press identity against live clips, never dereference a
        // note that may have been deleted/recreated since mouseDown.
        const auto &clips = m_viewport->getCachedMidiClips();
        const MidiSelectionSnapshot::Items hit{m_pressedMarker};
        if (!MidiSelectionSnapshot::resolve(hit, clips).isEmpty())
        {
            auto &selected = m_viewport->getSelectedEvents();
            const auto original = MidiSelectionSnapshot::capture(selected, clips);
            MidiSelectionSnapshot::apply(combineLassoSelection(original, hit, lassoSelectionMode(e.mods)),
                                         selected, m_editViewState.m_selectionManager, clips);
        }
    }
    m_pressedMarker = {};
    m_markerDragged = false;
    m_pressInput.reset();
    m_dragVelocityStates.clear();
    m_dragReferenceNote = nullptr;
    updateToolCursor();
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
    if ((m_viewport && m_viewport->isSelected(n)) || n->state.getPropertyAsValue(IDs::isHovered, nullptr, false) == true)
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

juce::Point<float> VelocityEditor::getMarkerCentre(te::MidiClip *const &clip, const te::MidiNote *note) const
{
    // Use the same X/Y geometry as the painted head, not its stem or duration.
    return {getXLineRange(clip, note).getStart(), float(getVelocityPixel(note))};
}

int VelocityEditor::getVelocityPixel(const te::MidiNote *n) const { return int(VelocityMarkerGeometry::markerY(getDisplayedVelocity(n), getHeight())); }

int VelocityEditor::getDisplayedVelocity(const te::MidiNote *n) const
{
    for (const auto &edit : m_notePropertyPreview)
        if (edit.sourceNote == n)
            return edit.velocity;

    return n->getVelocity();
}

tracktion_engine::MidiNote *VelocityEditor::getNote(juce::Point<float> p)
{
    for (auto &mc : EngineHelpers::getMidiClipsOfTrack(*m_track))
    {
        for (auto note : mc->getSequence().getNotes())
        {
            if (GUIHelpers::getSensibleArea(p, 10).contains(getMarkerCentre(mc, note)))
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
            n->state.setProperty(IDs::isHovered, false, nullptr);
        }
    }
}
