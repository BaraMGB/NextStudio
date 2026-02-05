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

#include "SongEditor/TrackLaneComponent.h"
#include "SongEditor/SongEditorView.h"
#include "Utilities/ScopedSaveLock.h"
#include "Utilities/TimeUtils.h"

TrackLaneComponent::TrackLaneComponent(EditViewState &evs, te::Track::Ptr track, juce::String timelineID, SongEditorView &owner)
    : m_editViewState(evs),
      m_track(track),
      m_timeLineID(timelineID),
      m_songEditor(owner)
{
    // Enable mouse events for this component and its children
    setInterceptsMouseClicks(true, true);
    buildAutomationLanes();
}

void TrackLaneComponent::paint(juce::Graphics &g)
{
    if (m_track == nullptr)
        return;

    auto visibleTimeRange = m_editViewState.getVisibleTimeRange(m_timeLineID, getWidth());
    if (auto clipTrack = dynamic_cast<te::ClipTrack *>(m_track.get()))
    {
        float clipTrackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        auto clipArea = getLocalBounds().removeFromTop(clipTrackHeight).toFloat();
        GUIHelpers::drawTrack(g, *this, m_editViewState, clipArea, clipTrack, visibleTimeRange);
    }
    else if (m_track->isFolderTrack())
    {
        float trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        auto area = getLocalBounds().removeFromTop(trackHeight).toFloat();
        auto x1beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        auto x2beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
        g.setColour(m_editViewState.m_applicationState.getTrackBackgroundColour());
        g.fillRect(area);
        GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, x1beats, x2beats, area);
    }
}

void TrackLaneComponent::resized()
{
    auto *trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    const int minimizedHeigth = 30;

    auto rect = getLocalBounds();
    float trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
    rect.removeFromTop(trackHeight);

    for (auto *al : m_automationLanes)
    {
        auto ap = al->getParameter();
        int automationHeight = trackInfo->automationParameterHeights[ap];
        automationHeight = automationHeight < minimizedHeigth ? minimizedHeigth : automationHeight;
        automationHeight = trackInfo->isMinimized ? 0 : automationHeight;
        al->setBounds(rect.removeFromTop(automationHeight));
    }
}

void TrackLaneComponent::buildAutomationLanes()
{
    m_automationLanes.clear(true);

    m_editViewState.m_trackHeightManager->regenerateTrackHeightsFromStates(tracktion::getAllTracks(m_track->edit));

    auto *trackInfo = m_editViewState.m_trackHeightManager->getTrackInfoForTrack(m_track);
    if (trackInfo == nullptr)
        return;

    juce::Array<te::AutomatableParameter *> params;
    for (const auto &[ap, height] : trackInfo->automationParameterHeights)
        if (ap && ap->getCurve().getNumPoints() > 0)
            params.add(ap);

    // Sort the parameters by their ID string to ensure a consistent order
    std::sort(params.begin(), params.end(), [](const auto *a, const auto *b) { return a->paramID < b->paramID; });

    for (auto *ap : params)
    {
        m_automationLanes.add(new AutomationLaneComponent(m_editViewState, ap, m_timeLineID, m_songEditor));
        addAndMakeVisible(m_automationLanes.getLast());
    }

    resized();
}

AutomationLaneComponent *TrackLaneComponent::getAutomationLane(tracktion::AutomatableParameter::Ptr ap)
{
    for (auto al : m_automationLanes)
        if (al->getParameter() == ap)
            return al;

    return nullptr;
}

//==============================================================================
// Mouse Handling
//==============================================================================

void TrackLaneComponent::mouseMove(const juce::MouseEvent &e)
{
    // Mouse event throttling
    if (!m_mouseThrottler.shouldProcess(e))
        return;

    auto mousePosTime = xtoTime(e.x);
    auto toolMode = m_songEditor.getToolMode();

    te::Clip::Ptr hoveredClip = nullptr;
    bool leftBorderHovered = false;
    bool rightBorderHovered = false;
    bool needsRepaint = false;

    // Clip hit test
    if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
    {
        for (auto clip : at->getClips())
        {
            if (clip->getEditTimeRange().contains(mousePosTime))
            {
                hoveredClip = clip;
                auto clipRect = getClipRect(clip);
                auto borderWidth = clipRect.getWidth() > 30 ? 10 : clipRect.getWidth() / 3;

                if (e.getPosition().getX() < clipRect.getX() + borderWidth)
                {
                    leftBorderHovered = true;
                }
                else if (e.getPosition().getX() > clipRect.getRight() - borderWidth)
                {
                    rightBorderHovered = true;
                }
                break; // Found the top-most clip
            }
        }
    }

    if (m_hoveredClip != hoveredClip || m_leftBorderHovered != leftBorderHovered || m_rightBorderHovered != rightBorderHovered)
    {
        needsRepaint = true;
    }

    m_hoveredClip = hoveredClip;
    m_leftBorderHovered = leftBorderHovered;
    m_rightBorderHovered = rightBorderHovered;

    updateCursor(e.mods);

    if (needsRepaint)
        repaint();
}

void TrackLaneComponent::mouseExit(const juce::MouseEvent &e)
{
    if (m_hoveredClip != nullptr)
    {
        m_hoveredClip = nullptr;
        m_leftBorderHovered = false;
        m_rightBorderHovered = false;
        repaint();
    }
    setMouseCursor(juce::MouseCursor::NormalCursor);

    // Reset throttler when mouse leaves
    m_mouseThrottler.reset();
}

void TrackLaneComponent::mouseDown(const juce::MouseEvent &e)
{
    ScopedSaveLock saveLock(m_editViewState);
    auto &sm = m_editViewState.m_selectionManager;
    auto toolMode = m_songEditor.getToolMode();

    bool leftButton = e.mods.isLeftButtonDown();
    bool isClipClicked = m_hoveredClip != nullptr;

    // 1. Clip Interaction
    if (isClipClicked && leftButton)
    {
        if (toolMode == Tool::pointer || toolMode == Tool::timestretch)
        {
            // Double Click -> Piano Roll
            if ((e.getNumberOfClicks() > 1 || m_editViewState.getLowerRangeView() == LowerRangeView::midiEditor) && m_hoveredClip->isMidi())
            {
                m_editViewState.setLowerRangeView(LowerRangeView::midiEditor);
                for (auto t : te::getAllTracks(m_editViewState.m_edit))
                {
                    t->state.setProperty(IDs::showLowerRange, false, nullptr);
                    if (t == m_track.get())
                        t->state.setProperty(IDs::showLowerRange, true, nullptr);
                }

                if (e.getNumberOfClicks() > 1 && m_track->itemID.isValid())
                {
                    auto trackTimeLineID = "ID" + m_track->itemID.toString().removeCharacters("{}-)");
                    GUIHelpers::centerMidiEditorToClip(m_editViewState, m_hoveredClip, trackTimeLineID, getWidth());
                }
            }

            // Selection Logic
            if (!sm.isSelected(m_hoveredClip))
            {
                if (e.mods.isCtrlDown())
                    sm.addToSelection(m_hoveredClip);
                else
                    sm.selectOnly(m_hoveredClip);
            }
            else if (e.mods.isCtrlDown())
            {
                sm.deselect(m_hoveredClip);
            }

            // Start clip drag
            m_songEditor.startDrag(DragType::Clip, xtoTime(e.x), e.getPosition(), m_hoveredClip->itemID);
            auto &dragState = m_songEditor.getDragState();
            dragState.draggedClip = m_hoveredClip;
            dragState.isLeftEdge = m_leftBorderHovered;
            dragState.isRightEdge = m_rightBorderHovered;

            repaint();
            return;
        }
        else if (toolMode == Tool::knife)
        {
            te::splitClips({m_hoveredClip}, xtoTime(e.x));
            return;
        }
    }

    // 2. Empty Space Interaction
    if (!isClipClicked && leftButton)
    {
        // Double Click -> Create MIDI Clip
        if (e.getNumberOfClicks() > 1)
        {
            auto beat = e.mods.isShiftDown() ? m_editViewState.timeToBeat(xtoTime(e.x).inSeconds()) : m_songEditor.xToSnapedBeat(e.getEventRelativeTo(&m_songEditor).x);

            if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
            {
                if ((bool)m_track->state.getProperty(IDs::isMidiTrack))
                {
                    auto start = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime(beat)));
                    auto end = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime(beat)) + m_editViewState.beatToTime(4));

                    at->insertMIDIClip({start, end}, &sm);
                }
            }
        }
        else
        {
            // Start Lasso
            auto globalEvent = e.getEventRelativeTo(&m_songEditor);
            m_songEditor.startLasso(globalEvent, false, toolMode == Tool::range);
        }
    }
}

void TrackLaneComponent::mouseDrag(const juce::MouseEvent &e)
{
    auto toolMode = m_songEditor.getToolMode();
    auto &sm = m_editViewState.m_selectionManager;
    auto &dragState = m_songEditor.getDragState();

    if (dragState.draggedClip && (toolMode == Tool::pointer || toolMode == Tool::timestretch))
    {
        auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - xtoTime(0);
        auto startTime = dragState.draggedClip->getPosition().getStart();
        if (m_rightBorderHovered)
            startTime = dragState.draggedClip->getPosition().getEnd();

        auto targetTime = startTime + draggedTime;
        if (!e.mods.isShiftDown())
            targetTime = getSnappedTime(targetTime);

        dragState.timeDelta = targetTime - startTime;

        auto globalEvent = e.getEventRelativeTo(&m_songEditor);
        int verticalOffset = m_songEditor.getVerticalOffset(m_track, globalEvent.position.toInt());

        m_songEditor.updateDragGhost(dragState.draggedClip, dragState.timeDelta, verticalOffset);
    }
    else
    {
        // Lasso Drag
        auto globalEvent = e.getEventRelativeTo(&m_songEditor);
        m_songEditor.updateLasso(globalEvent);
    }

    repaint();
}

void TrackLaneComponent::mouseUp(const juce::MouseEvent &e)
{
    auto &dragState = m_songEditor.getDragState();

    if (dragState.isClipDrag() && dragState.draggedClip)
    {
        // Finalize Move/Resize
        auto globalEvent = e.getEventRelativeTo(&m_songEditor);
        int verticalOffset = m_songEditor.getVerticalOffset(m_track, globalEvent.position.toInt());

        m_songEditor.updateDragGhost(dragState.draggedClip, dragState.timeDelta, verticalOffset);

        // Only apply changes if the mouse was actually dragged and there is real movement
        // to avoid unnecessary processing and audio graph rebuilds on simple clicks.
        if (e.mouseWasDraggedSinceMouseDown())
        {
            if (m_leftBorderHovered || m_rightBorderHovered)
            {
                if (std::abs(dragState.timeDelta.inSeconds()) > 1.0e-9)
                    EngineHelpers::resizeSelectedClips(m_leftBorderHovered, dragState.timeDelta.inSeconds(), m_editViewState);
            }
            else
            {
                if (std::abs(dragState.timeDelta.inSeconds()) > 1.0e-9 || verticalOffset != 0)
                {
                    EngineHelpers::moveSelectedClips(e.mods.isCtrlDown(), dragState.timeDelta.inSeconds(), verticalOffset, m_editViewState);
                }
            }
        }
    }
    else
    {
        // Finish Lasso
        m_songEditor.stopLasso();

        auto &sm = m_editViewState.m_selectionManager;
        auto toolMode = m_songEditor.getToolMode();

        if (!dragState.draggedClip && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isShiftDown() && !e.mods.isCommandDown() && e.getNumberOfClicks() == 1 && toolMode != Tool::knife)
        {
            sm.deselectAll();
            m_songEditor.clearSelectedTimeRange();
        }
    }

    m_songEditor.endDrag();
    m_songEditor.updateDragGhost(nullptr, {}, 0);
    m_songEditor.repaint();
    repaint();
}

//==============================================================================
// Helpers
//==============================================================================

float TrackLaneComponent::timeToX(tracktion::TimePosition time) { return TimeUtils::timeToX(time, m_editViewState, m_timeLineID, getWidth()); }

tracktion::TimePosition TrackLaneComponent::xtoTime(int x) { return TimeUtils::xToTime(x, m_editViewState, m_timeLineID, getWidth()); }

tracktion::TimePosition TrackLaneComponent::getSnappedTime(tracktion::TimePosition time, bool downwards) { return TimeUtils::getSnappedTime(time, m_editViewState, m_timeLineID, getWidth(), downwards); }

juce::Rectangle<float> TrackLaneComponent::getClipRect(te::Clip::Ptr clip)
{
    float x = timeToX(clip->getPosition().getStart());
    float y = 0.0f; // Relative to TrackLaneComponent top
    float w = timeToX(clip->getPosition().getEnd()) - x;
    float h = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(clip->getClipTrack(), false));

    juce::Rectangle<float> clipRect = {x, y, w, h};
    return clipRect;
}

void TrackLaneComponent::updateCursor(juce::ModifierKeys modifierKeys)
{
    auto toolMode = m_songEditor.getToolMode();

    auto timeRightcursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::TimeShiftRight, m_editViewState.m_applicationState.m_mouseCursorScale);
    auto shiftRightcursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, m_editViewState.m_applicationState.m_mouseCursorScale);
    auto shiftLeftcursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, m_editViewState.m_applicationState.m_mouseCursorScale);
    auto shiftHandCursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, m_editViewState.m_applicationState.m_mouseCursorScale);

    // Clip cursor handling
    if (m_hoveredClip != nullptr && (toolMode == Tool::pointer || toolMode == Tool::timestretch))
    {
        if (m_leftBorderHovered)
        {
            setMouseCursor(shiftLeftcursor);
        }
        else if (m_rightBorderHovered)
        {
            if ((modifierKeys.isCommandDown() || toolMode == Tool::timestretch) && !m_hoveredClip->isMidi())
            {
                setMouseCursor(timeRightcursor);
            }
            else
            {
                setMouseCursor(shiftRightcursor);
            }
        }
        else
        {
            setMouseCursor(shiftHandCursor);
        }
    }
    else if (m_hoveredClip != nullptr && toolMode == Tool::knife)
    {
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}