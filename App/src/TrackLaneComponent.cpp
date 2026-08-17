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

#include "TrackLaneComponent.h"
#include "ClipOverwriteCommand.h"
#include "SongEditorView.h"
#include "ScopedSaveLock.h"
#include "TimeUtils.h"

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
    else
    {
        float trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(m_track, false);
        auto area = getLocalBounds().removeFromTop(trackHeight).toFloat();
        auto x1beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getStart().inBeats();
        auto x2beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth()).getEnd().inBeats();
        g.setColour(m_editViewState.m_applicationState.getTrackBackgroundColour().darker(0.1f));
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

    m_editViewState.m_trackHeightManager->regenerateTrackHeightsFromEdit(m_track->edit);

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

    const auto toolMode = m_songEditor.getToolMode();
    const bool allowFadeHandles = toolMode == Tool::pointer || toolMode == Tool::timestretch;

    bool needsRepaint = false;
    auto hoverState = getClipHoverState(e.position, allowFadeHandles);

    if (m_hoveredClip != hoverState.clip || m_leftBorderHovered != hoverState.leftBorder || m_rightBorderHovered != hoverState.rightBorder || m_hoveredFadeZone != hoverState.fadeZone)
        needsRepaint = true;

    m_hoveredClip = hoverState.clip;
    m_leftBorderHovered = hoverState.leftBorder;
    m_rightBorderHovered = hoverState.rightBorder;
    m_hoveredFadeZone = hoverState.fadeZone;

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
        m_hoveredFadeZone = FadeHitZone::none;
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
    const auto toolMode = m_songEditor.getToolMode();
    const bool allowFadeHandles = toolMode == Tool::pointer || toolMode == Tool::timestretch;

    bool leftButton = e.mods.isLeftButtonDown();
    bool rightButton = e.mods.isRightButtonDown();
    auto hoverState = getClipHoverState(e.position, allowFadeHandles);
    m_hoveredClip = hoverState.clip;
    m_leftBorderHovered = hoverState.leftBorder;
    m_rightBorderHovered = hoverState.rightBorder;
    m_hoveredFadeZone = hoverState.fadeZone;
    bool isClipClicked = m_hoveredClip != nullptr;

    // 1. Clip Interaction
    if (isClipClicked && rightButton && isFadeZone(m_hoveredFadeZone))
    {
        if (auto audioClip = te::AudioClipBase::Ptr(dynamic_cast<te::AudioClipBase *>(m_hoveredClip.get())))
        {
            showFadeCurveMenu(audioClip, m_hoveredFadeZone == FadeHitZone::fadeInHandle, e.getScreenPosition());
            return;
        }
    }

    if (isClipClicked && leftButton)
    {
        if (toolMode == Tool::pointer || toolMode == Tool::timestretch)
        {
            if (auto audioClip = te::AudioClipBase::Ptr(dynamic_cast<te::AudioClipBase *>(m_hoveredClip.get())); audioClip != nullptr && isFadeHandle(m_hoveredFadeZone))
            {
                m_songEditor.startDrag(DragType::Clip, xtoTime(e.x), e.getPosition(), m_hoveredClip->itemID);
                auto &dragState = m_songEditor.getDragState();
                dragState.draggedClip = m_hoveredClip;
                dragState.isFadeIn = m_hoveredFadeZone == FadeHitZone::fadeInHandle;
                dragState.isFadeOut = m_hoveredFadeZone == FadeHitZone::fadeOutHandle;
                audioClip->setAutoCrossfade(false);
                repaint();
                return;
            }

            // Double Click -> Piano Roll
            if ((e.getNumberOfClicks() > 1 || m_editViewState.getLowerRangeView() == LowerRangeView::midiEditor) && m_hoveredClip->isMidi())
            {
                EngineHelpers::setLowerRangeTrack(m_editViewState, m_track.get(), static_cast<int>(LowerRangeView::midiEditor));

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
                m_pendingCtrlToggleClip = m_hoveredClip;
            }

            // Start clip drag
            m_songEditor.startDrag(DragType::Clip, xtoTime(e.x), e.getPosition(), m_hoveredClip->itemID);
            auto &dragState = m_songEditor.getDragState();
            dragState.draggedClip = m_hoveredClip;
            dragState.isLeftEdge = m_leftBorderHovered;
            dragState.isRightEdge = m_rightBorderHovered;
            dragState.isTimeStretching = dragState.isRightEdge && !m_hoveredClip->isMidi() && (toolMode == Tool::timestretch || e.mods.isCommandDown());

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
    // The range tool must be able to draw a time range no matter what is under the
    // cursor, so an active range tool also enters this branch even when a clip was hit.
    if (leftButton && (!isClipClicked || toolMode == Tool::range))
    {
        // Double Click -> Create MIDI Clip (only on actual empty space)
        if (!isClipClicked && e.getNumberOfClicks() > 1)
        {
            auto beat = e.mods.isShiftDown() ? m_editViewState.timeToBeat(xtoTime(e.x).inSeconds()) : m_songEditor.xToSnapedBeat(e.getEventRelativeTo(&m_songEditor).x);

            if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
            {
                if ((bool)m_track->state.getProperty(IDs::isMidiTrack))
                {
                    auto start = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime(beat)));
                    auto end = tracktion::core::TimePosition::fromSeconds(juce::jmax(0.0, m_editViewState.beatToTime(beat)) + m_editViewState.beatToTime(4));

                    ClipEditing::Placement placement;
                    placement.mode = ClipEditing::PlacementMode::insertMidi;
                    placement.destination = at;
                    placement.finalPosition = {{start, end}, {}};
                    placement.name = at->getName();

                    ClipEditing::Options options;
                    options.undoName = "Create MIDI clip";
                    auto result = ClipEditing::applyOverwrite(m_editViewState, {std::move(placement)}, options);
                    if (!result.succeeded)
                        GUIHelpers::log("MIDI clip creation failed: " + result.error);
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
    auto &dragState = m_songEditor.getDragState();

    if (dragState.draggedClip && (toolMode == Tool::pointer || toolMode == Tool::timestretch))
    {
        if (dragState.isFadeIn || dragState.isFadeOut)
        {
            if (auto audioClip = te::AudioClipBase::Ptr(dynamic_cast<te::AudioClipBase *>(dragState.draggedClip.get())))
            {
                const auto clipPos = audioClip->getPosition();
                const auto clipLength = clipPos.getLength();
                const auto mouseTime = xtoTime(e.x);

                if (dragState.isFadeIn)
                {
                    auto fadeLength = mouseTime - clipPos.getStart();
                    fadeLength = juce::jlimit(tracktion::TimeDuration(), clipLength, fadeLength);
                    audioClip->setFadeIn(fadeLength);
                }
                else if (dragState.isFadeOut)
                {
                    auto fadeLength = clipPos.getEnd() - mouseTime;
                    fadeLength = juce::jlimit(tracktion::TimeDuration(), clipLength, fadeLength);
                    audioClip->setFadeOut(fadeLength);
                }

                repaint();
                m_songEditor.repaint();
            }

            return;
        }

        auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - xtoTime(0);
        auto startTime = dragState.draggedClip->getPosition().getStart();
        if (dragState.isRightEdge)
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
        if (dragState.isFadeIn || dragState.isFadeOut)
        {
            m_pendingCtrlToggleClip = nullptr;
            m_songEditor.endDrag();
            m_songEditor.updateDragGhost(nullptr, {}, 0);
            m_songEditor.repaint();
            repaint();
            return;
        }

        // Finalize Move/Resize
        auto globalEvent = e.getEventRelativeTo(&m_songEditor);
        int verticalOffset = m_songEditor.getVerticalOffset(m_track, globalEvent.position.toInt());

        m_songEditor.updateDragGhost(dragState.draggedClip, dragState.timeDelta, verticalOffset);

        // Only apply changes if the mouse was actually dragged and there is real movement
        // to avoid unnecessary processing and audio graph rebuilds on simple clicks.
        if (e.mouseWasDraggedSinceMouseDown())
        {
            if (dragState.isLeftEdge || dragState.isRightEdge)
            {
                if (std::abs(dragState.timeDelta.inSeconds()) > 1.0e-9)
                {
                    if (dragState.isTimeStretching)
                        EngineHelpers::timeStretchSelectedClips(dragState.timeDelta.inSeconds(), m_editViewState);
                    else
                        EngineHelpers::resizeSelectedClips(dragState.isLeftEdge, dragState.timeDelta.inSeconds(), m_editViewState);
                }
            }
            else
            {
                if (std::abs(dragState.timeDelta.inSeconds()) > 1.0e-9 || verticalOffset != 0)
                {
                    EngineHelpers::moveSelectedClips(e.mods.isCtrlDown(), dragState.timeDelta.inSeconds(), verticalOffset, m_editViewState);
                }
            }
        }
        else if (m_pendingCtrlToggleClip != nullptr && m_pendingCtrlToggleClip == dragState.draggedClip)
        {
            m_editViewState.m_selectionManager.deselect(m_pendingCtrlToggleClip);
        }

        m_pendingCtrlToggleClip = nullptr;
    }
    else
    {
        m_pendingCtrlToggleClip = nullptr;

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

TrackLaneComponent::ClipHoverState TrackLaneComponent::getClipHoverState(juce::Point<float> point, bool allowFadeHandles)
{
    ClipHoverState result;
    const auto mousePosTime = xtoTime(juce::roundToInt(point.x));

    if (auto at = dynamic_cast<te::AudioTrack *>(m_track.get()))
    {
        for (auto clip : at->getClips())
        {
            if (!clip->getEditTimeRange().contains(mousePosTime))
                continue;

            result.clip = clip;
            if (allowFadeHandles)
                result.fadeZone = getFadeHitZone(clip, point);

            if (!isFadeZone(result.fadeZone))
            {
                auto clipRect = getClipRect(clip);
                auto borderWidth = clipRect.getWidth() > 30.0f ? 10.0f : clipRect.getWidth() / 3.0f;

                if (point.x < clipRect.getX() + borderWidth)
                    result.leftBorder = true;
                else if (point.x > clipRect.getRight() - borderWidth)
                    result.rightBorder = true;
            }

            break;
        }
    }

    return result;
}

TrackLaneComponent::FadeHitZone TrackLaneComponent::getFadeHitZone(te::Clip::Ptr clip, juce::Point<float> point)
{
    auto audioClip = te::AudioClipBase::Ptr(dynamic_cast<te::AudioClipBase *>(clip.get()));
    if (audioClip == nullptr)
        return FadeHitZone::none;

    const auto clipRect = getClipRect(clip);
    if (!canUseFadeHandles(clipRect) || !clipRect.contains(point))
        return FadeHitZone::none;

    auto fadeArea = clipRect.withTrimmedTop(static_cast<float>(m_editViewState.m_clipHeaderHeight)).reduced(2.0f, 3.0f);

    if (fadeArea.isEmpty())
        return FadeHitZone::none;

    const auto clipPos = audioClip->getPosition();
    const auto fadeInEndX = timeToX(clipPos.getStart() + audioClip->getFadeIn());
    const auto fadeOutStartX = timeToX(clipPos.getEnd() - audioClip->getFadeOut());
    const auto handleSize = juce::jlimit(6.0f, 12.0f, clipRect.getWidth() * 0.18f);
    const auto handleY = fadeArea.getY() + juce::jmin(5.0f, fadeArea.getHeight() * 0.25f);

    auto fadeInHandle = juce::Rectangle<float>(fadeInEndX, handleY - handleSize, handleSize, handleSize).expanded(2.0f);
    auto fadeOutHandle = juce::Rectangle<float>(fadeOutStartX - handleSize, handleY - handleSize, handleSize, handleSize).expanded(2.0f);

    if (fadeInHandle.contains(point))
        return FadeHitZone::fadeInHandle;

    if (fadeOutHandle.contains(point))
        return FadeHitZone::fadeOutHandle;

    return FadeHitZone::none;
}

bool TrackLaneComponent::canUseFadeHandles(const juce::Rectangle<float> &clipRect) const
{
    const auto minimizedHeight = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackMinimizedHeight());
    return getHeight() > minimizedHeight && clipRect.getHeight() > minimizedHeight;
}

bool TrackLaneComponent::isFadeHandle(FadeHitZone zone) const
{
    return zone == FadeHitZone::fadeInHandle || zone == FadeHitZone::fadeOutHandle;
}

bool TrackLaneComponent::isFadeZone(FadeHitZone zone) const
{
    return zone != FadeHitZone::none;
}

void TrackLaneComponent::showFadeCurveMenu(te::AudioClipBase::Ptr clip, bool isFadeIn, juce::Point<int> screenPosition)
{
    if (clip == nullptr)
        return;

    const auto currentType = isFadeIn ? clip->getFadeInType() : clip->getFadeOutType();

    juce::PopupMenu menu;
    menu.addItem(1, "Linear", true, currentType == te::AudioFadeCurve::linear);
    menu.addItem(2, "Convex", true, currentType == te::AudioFadeCurve::convex);
    menu.addItem(3, "Concave", true, currentType == te::AudioFadeCurve::concave);
    menu.addItem(4, "S-Curve", true, currentType == te::AudioFadeCurve::sCurve);

    juce::Component::SafePointer<TrackLaneComponent> safeThis(this);
    const auto targetArea = juce::Rectangle<int>(screenPosition.x, screenPosition.y, 1, 1);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(targetArea), [safeThis, clip, isFadeIn](int result)
    {
        if (safeThis == nullptr || result == 0 || clip == nullptr)
            return;

        auto type = te::AudioFadeCurve::linear;
        if (result == 2)
            type = te::AudioFadeCurve::convex;
        else if (result == 3)
            type = te::AudioFadeCurve::concave;
        else if (result == 4)
            type = te::AudioFadeCurve::sCurve;

        ScopedSaveLock saveLock(safeThis->m_editViewState);

        if (isFadeIn)
            clip->setFadeInType(type);
        else
            clip->setFadeOutType(type);

        safeThis->repaint();
    });
}

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
    auto curveCursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::CurveSteepnes, m_editViewState.m_applicationState.m_mouseCursorScale);

    // Clip cursor handling
    if (m_hoveredClip != nullptr && (toolMode == Tool::pointer || toolMode == Tool::timestretch))
    {
        if (isFadeZone(m_hoveredFadeZone))
        {
            setMouseCursor(curveCursor);
        }
        else if (m_leftBorderHovered)
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
