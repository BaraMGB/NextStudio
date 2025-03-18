
/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SongEditorView.h"
#include "Browser_Base.h"
#include "Utilities.h"


SongEditorView::SongEditorView(EditViewState& evs, MenuBar& toolBar, TimeLineComponent& timeLine)
        : m_editViewState(evs)
        , m_toolBar(toolBar)
        , m_timeLine(timeLine)
        , m_lassoComponent(evs, m_timeLine.getTimeLineID())
{
    setWantsKeyboardFocus (true);
    setName("SongEditorView");
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
    clearSelectedTimeRange();

    m_editViewState.m_edit.getTransport().addChangeListener(this);
    m_editViewState.m_selectionManager.addChangeListener(this);
    m_editViewState.m_trackHeightManager->addChangeListener(this);
}

SongEditorView::~SongEditorView()
{
    m_editViewState.m_trackHeightManager->removeChangeListener(this);
    m_editViewState.m_edit.getTransport().removeChangeListener(this);
    m_editViewState.m_selectionManager.removeChangeListener(this);
}

void SongEditorView::paintOverChildren(juce::Graphics& g)
{
    
    GUIHelpers::log("SongEditor::paint() is called");

    using namespace juce::Colours;
    auto &sm = m_editViewState.m_selectionManager;
    auto scroll = timeToX(tracktion::TimePosition::fromSeconds(0)) * (-1.0f);

    const auto area = getLocalBounds().toFloat();
    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        bool isShowable = m_editViewState.m_trackHeightManager->isTrackShowable(t);
        bool isVisible = !(m_editViewState.m_trackHeightManager->isTrackInMinimizedFolderRecursive(t));

        if (isShowable && isVisible)
        {
            float x = 0.0f;
            float y = static_cast<float>(getYForTrack(t));
            float w = static_cast<float>(getWidth());
            float h = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(t, false));

            juce::Rectangle<float> trackRect(x, y, w, h);

            if (m_selectedRange.selectedTracks.contains(t) && m_selectedRange.getLength().inSeconds() > 0)
            {
                float rangeX = timeToX(m_selectedRange.getStart());
                float rangeW = timeToX(m_selectedRange.getEnd()) - rangeX;
                g.setColour(juce::Colour(0x50ffffff));
                juce::Rectangle<float> timeRangeRect(rangeX, y, rangeW, h);
                timeRangeRect = timeRangeRect.getIntersection(area);

                g.fillRect(timeRangeRect);
            }
        }
    //
        for (auto& ap : t->getAllAutomatableParams())
        {
            if (m_editViewState.m_trackHeightManager->isAutomationVisible(*ap))
            {
                auto rect = getAutomationRect(ap);

                if (rect.getHeight() > 0)
                {
                    if (m_selectedRange.selectedAutomations.contains(ap) && m_selectedRange.getLength().inSeconds() > 0)
                    {
                        float rangeX = timeToX(m_selectedRange.getStart());
                        float rangeY = rect.getY();
                        float rangeW = timeToX(m_selectedRange.getEnd()) - rangeX;
                        float rangeH = rect.getHeight();

                        g.setColour(juce::Colour(0x50ffffff));
                        juce::Rectangle<float> automationRangeRect(rangeX, rangeY, rangeW, rangeH);
                        automationRangeRect = automationRangeRect.getIntersection(area);
                        g.fillRect(automationRangeRect);
                    }
                }
            }
        }
    }
    //
    if (m_isDraggingSelectedTimeRange)
    {
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillAll();
        juce::Rectangle<float> selectedRangeRect;

        for (auto track : m_selectedRange.selectedTracks)
        {
            float x = timeToX(m_selectedRange.getStart()); 
            float y = static_cast<float>(getYForTrack(track));
            float w = timeToX(m_selectedRange.getEnd()) - x;
            float h = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(track, false));

            GUIHelpers::log("PAINT m_draggedTimeDelta: ", m_draggedTimeDelta.inSeconds());
            x = x + timeDurationToPixel(m_draggedTimeDelta);

            juce::Rectangle<float> rect(x, y, w, h);

            selectedRangeRect = selectedRangeRect.getUnion(rect);
            if (auto ct = dynamic_cast<te::ClipTrack*>(track))
            {
                GUIHelpers::drawTrack(g, *this, m_editViewState, rect, ct, m_selectedRange.timeRange, true);
            }
            else if (track->isFolderTrack())
            {
                auto beatX1 = m_editViewState.timeToBeat(m_selectedRange.getStart().inSeconds());
                auto beatX2 = m_editViewState.timeToBeat(m_selectedRange.getEnd().inSeconds());

                GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, beatX1 , beatX2, rect);
            }
        }

        for (auto automation : m_selectedRange.selectedAutomations)
        {
            auto rect = getAutomationRect(automation);

            if (rect.getHeight() > 0)
            {

                if (m_selectedRange.selectedAutomations.contains(automation) && m_selectedRange.getLength().inSeconds() > 0)
                {
                    float rangeX = timeToX(m_selectedRange.getStart());
                    float rangeY = rect.getY();
                    float rangeW = timeToX(m_selectedRange.getEnd()) - rangeX;
                    float rangeH = rect.getHeight();

                    juce::Rectangle<float> automationRangeRect(rangeX, rangeY, rangeW, rangeH);
                    automationRangeRect = automationRangeRect.getIntersection(area);
                    automationRangeRect.setX(rangeX + timeDurationToPixel(m_draggedTimeDelta));
                    getAutomationLane(automation)->drawAutomationLane(g, m_selectedRange.timeRange, automationRangeRect, automation);

                    selectedRangeRect = selectedRangeRect.getUnion(automationRangeRect);

                }
            }
        }

        g.setColour(juce::Colours::yellowgreen);
        selectedRangeRect = selectedRangeRect.getIntersection(area);
        g.drawRect(selectedRangeRect, 1.0f);
    }

    if (m_hoveredTimeRangeLeft)
    {
        auto lastSelectedTrack = m_selectedRange.selectedTracks.getLast();
        auto height = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(lastSelectedTrack, true));
        float x = timeToX(m_selectedRange.getStart()); 
        float y = static_cast<float>(getYForTrack(m_selectedRange.selectedTracks.getLast())) + height;
        g.setColour(yellowgreen);
        g.drawLine(x, 0.0f, x, y, 1.0f);
    }

    if (m_hoveredTimeRangeRight)
    {
        auto lastSelectedTrack = m_selectedRange.selectedTracks.getLast();
        auto height = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(lastSelectedTrack, true));
        float x = timeToX(m_selectedRange.getEnd()); 
        float y = static_cast<float>(getYForTrack(m_selectedRange.selectedTracks.getLast())) + height;
        g.setColour(yellowgreen);
        g.drawLine(x, 0.0f, x, y, 1.0f);
    }

    if (m_draggedClip)
    {
        for (auto selectedClip : sm.getItemsOfType<te::Clip>())
        {
            if (auto targetTrack = EngineHelpers::getTargetTrack(selectedClip->getTrack(), m_draggedVerticalOffset))
            {
                auto clipRect = getClipRect(selectedClip);
                float targetX = clipRect.getX() + timeToX(tracktion::TimePosition() + m_draggedTimeDelta) + scroll;
                float targetY = static_cast<float>(getYForTrack(targetTrack));
                float targetW = clipRect.getWidth();
                float targetH = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(targetTrack, false));

                juce::Rectangle<float> targetRect(targetX, targetY, targetW, targetH);

                if (m_leftBorderHovered)
                {
                    auto offset = selectedClip->getPosition().getOffset().inSeconds();
                    auto timeDelta = juce::jmax(0.0 - offset, m_draggedTimeDelta.inSeconds());
                    auto deltaX = timeToX(tracktion::TimePosition() + tracktion::TimeDuration::fromSeconds(timeDelta)) + scroll;

                    targetRect = juce::Rectangle<float>(clipRect.getX() + deltaX, targetY, clipRect.getWidth() - deltaX, targetH);
                }
                else if (m_rightBorderHovered)
                {
                    targetRect = juce::Rectangle<float>(clipRect.getX(), targetY, clipRect.getWidth() + timeToX(tracktion::TimePosition() + m_draggedTimeDelta) + scroll, targetH);
                }

                g.setColour(white);
                g.drawRect(targetRect, 1.0f);

                if (EngineHelpers::trackWantsClip(selectedClip, targetTrack))
                {
                    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
                    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
                    juce::Rectangle<float> trackRect(0.0f, targetY, static_cast<float>(getWidth()), targetH);
                    GUIHelpers::drawClip(g, *this, m_editViewState, targetRect, selectedClip, targetTrack->getColour().withAlpha(0.1f), trackRect, x1, x2);
                }
                else
                {
                    g.setColour(grey);
                    g.fillRect(targetRect.reduced(1.0f, 1.0f));
                    g.setColour(black);
                    g.drawFittedText("not allowed", targetRect.toNearestInt(), juce::Justification::centred, 1);
                }
            }
        }
    }

    if (m_dragItemRect.visible)
    {
        if (m_dragItemRect.valid)
        {
            auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
            auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
            juce::Rectangle<float> dragRectFloat = m_dragItemRect.drawRect.toFloat();

            GUIHelpers::drawClipBody(g, m_editViewState, m_dragItemRect.name, dragRectFloat, false, m_dragItemRect.colour, area, x1, x2);
        }
        else
        {
            g.setColour(grey);
            g.fillRect(m_dragItemRect.drawRect.toFloat().reduced(1.0f, 1.0f));
            g.setColour(black);
            g.drawFittedText("not allowed", m_dragItemRect.drawRect.toNearestIntEdges(), juce::Justification::centred, 1);
        }
    }
    m_lassoComponent.drawLasso(g);
}

void SongEditorView::resized()
{
    auto& trackHeightManager = m_editViewState.m_trackHeightManager;
    const int yScroll = juce::roundToInt (m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
    int y = yScroll;

    for (auto lane : m_trackLanes)
    {
        auto trackHeaderHeight = trackHeightManager->getTrackHeight(lane->getTrack(), true);
        auto leftEdge = 0;
        auto w = getWidth();


        lane->setBounds(leftEdge, y, w, trackHeaderHeight);
        y += trackHeaderHeight;
    }

    m_lassoComponent.setBounds(getLocalBounds());
}

void SongEditorView::mouseMove (const juce::MouseEvent &e)
{
    //init

    auto hoveredTrack = getTrackAt(e.y);
    auto scrollY = - (m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));

    auto hoveredAutamatableParam = m_editViewState.m_trackHeightManager->getAutomatableParameterForY(e.y, scrollY);
    m_isDragging = false;
    int hoveredAutomationPoint = -1;
    int hoveredCurve = -1;
    juce::Rectangle<float> hoveredRectOnAutomation;

    te::Clip::Ptr hoveredClip = nullptr;
    bool leftBorderHovered = false;
    bool rightBorderHovered = false;

    bool hoveredTimeRange = false;
    bool hoveredTimeRangeLeft = false;
    bool hoveredTimeRangeRight = false;

    auto mousePosTime = xtoTime(e.x);
    m_timeAtMouseCursor = mousePosTime;

    bool needsRepaint = false;

    //Automation Lane hit tests 
    if (hoveredAutamatableParam)
    {
        getAutomationLane(hoveredAutamatableParam)->setIsDragging(m_isDragging);
        GUIHelpers::log("----------------------------------------------------------___>>>>>>>>>>>>>>>>> AutomationLane");
        auto cursorHitsSelectedRange = m_selectedRange.timeRange.contains(xtoTime(e.x));

        if (m_selectedRange.selectedAutomations.contains(hoveredAutamatableParam) && cursorHitsSelectedRange)
        {
            int leftX = timeToX(m_selectedRange.getStart());
            int rightX = timeToX(m_selectedRange.getEnd());

            hoveredTimeRange = true;
            if (e.x < leftX + 5)
                hoveredTimeRangeLeft = true;
            else if (e.x > rightX - 5)
                hoveredTimeRangeRight = true;
        }
        else if (m_toolMode == Tool::pointer)
        {
            auto yPos = m_editViewState.m_trackHeightManager->getYForAutomatableParameter(hoveredAutamatableParam->getTrack(), hoveredAutamatableParam, scrollY);
            juce::Point<float> hoveredPointInLane = {static_cast<float>(e.x), static_cast<float>(e.y - yPos)};
            auto automationLane = getAutomationLane(hoveredAutamatableParam);
            const auto hoveredRectOnLane = GUIHelpers::getSensibleArea(hoveredPointInLane, automationLane->getAutomationPointWidth() * 2);
            auto curve = hoveredAutamatableParam->getCurve();

            auto visibleRange = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth());
            auto x1 = visibleRange.getStart().inBeats();
            auto x2 = visibleRange.getEnd().inBeats();

            if (!automationLane->isCurveValid(curve))
            {
                automationLane->updateCurveCache(curve);
            }

            hoveredAutomationPoint = automationLane->findPointUnderMouse(hoveredRectOnLane, x1, x2, getWidth());

            auto valueAtMouseTime = curve.getValueAt(mousePosTime);

            auto curvePointAtMouseTime = juce::Point<float>(e.x, automationLane->getYPos(valueAtMouseTime));

            if (hoveredRectOnLane.contains(curvePointAtMouseTime) && hoveredAutomationPoint == -1)
                hoveredCurve = curve.nextIndexAfter(mousePosTime);

            juce::Point<float> cp = automationLane->getPointOnAutomationRect(mousePosTime, valueAtMouseTime, getWidth(), x1, x2);
            cp = cp.translated(0, yPos);
            hoveredRectOnAutomation = GUIHelpers::getSensibleArea(cp, 8);

            automationLane->setHoveredCurve(hoveredCurve);
            automationLane->setHoveredPoint(hoveredAutomationPoint);
            automationLane->setHoveredRect(hoveredRectOnLane.reduced(2.f, 2.f));
            automationLane->setIsDragging(m_isDragging);
        }
        else if (m_toolMode == Tool::knife)
        {
            needsRepaint = true; 
        }
    }

    if (hoveredTrack && hoveredAutamatableParam == nullptr)
    {
        if (m_selectedRange.selectedTracks.contains(hoveredTrack) && m_selectedRange.timeRange.contains(xtoTime(e.x)))
        {
            int leftX = timeToX(m_selectedRange.getStart());
            int rightX = timeToX(m_selectedRange.getEnd());

            hoveredTimeRange = true;
            if (e.x < leftX + 5)
                hoveredTimeRangeLeft = true;
            else if (e.x > rightX - 5)
                hoveredTimeRangeRight = true;
        }
    }
    //clip hit test 
    if (!hoveredTimeRange && hoveredAutamatableParam == nullptr)
    {
        if (auto at = dynamic_cast<te::AudioTrack*>(hoveredTrack.get()))
        {
            for (auto clip : at->getClips())
            {
                if (clip->getEditTimeRange().contains(mousePosTime))
                {
                    hoveredClip  = clip;
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
                }
            }
        }
    }

    if (
            (m_hoveredTrack != hoveredTrack)
        ||  (m_hoveredAutamatableParam != hoveredAutamatableParam)
        ||  (m_hoveredClip != hoveredClip)
        ||  (m_hoveredAutomationPoint != hoveredAutomationPoint)
        ||  (m_hoveredCurve != hoveredCurve)
        ||  (m_hoveredRectOnAutomation != hoveredRectOnAutomation)
        ||  (m_leftBorderHovered != leftBorderHovered)
        ||  (m_rightBorderHovered != rightBorderHovered)
        ||  (m_hoveredTimeRange != hoveredTimeRange)
        ||  (m_hoveredTimeRangeLeft != hoveredTimeRangeLeft)
        ||  (m_hoveredTimeRangeRight != hoveredTimeRangeRight)
        )
    {
        needsRepaint = true;
    }

    if (needsRepaint)
    {
        GUIHelpers::log("Songeditor::mouseMove(): repaint needed");
        repaint();
    }

    m_hoveredTrack = hoveredTrack;
    m_hoveredAutamatableParam = hoveredAutamatableParam;
    m_hoveredClip = hoveredClip;
    m_hoveredAutomationPoint = hoveredAutomationPoint;
    m_hoveredCurve = hoveredCurve;
    m_hoveredRectOnAutomation = hoveredRectOnAutomation;
    m_leftBorderHovered = leftBorderHovered;
    m_rightBorderHovered = rightBorderHovered;
    m_hoveredTimeRange = hoveredTimeRange;
    m_hoveredTimeRangeLeft = hoveredTimeRangeLeft;
    m_hoveredTimeRangeRight = hoveredTimeRangeRight;
    updateCursor(e.mods);

    //logMousePositionInfo();
}

void SongEditorView::mouseDown(const juce::MouseEvent&e)
{
    m_editViewState.m_isSavingLocked = true;

    auto &sm = m_editViewState.m_selectionManager;

    //init
    m_draggedVerticalOffset = 0;
    m_isDraggingSelectedTimeRange = false;
    m_isSelectingTimeRange = false;
    m_draggedClip = nullptr;
    m_draggedTimeDelta = tracktion::TimeDuration();
    m_timeOfHoveredAutomationPoint = tracktion::TimePosition::fromSeconds(0.0);
    m_selPointsAtMousedown = getSelectedPoints();

    bool leftButton = e.mods.isLeftButtonDown();
    bool rightButton = e.mods.isRightButtonDown();

    bool clickedOnTrack = m_hoveredTrack != nullptr;
    bool clickedOnClip = m_hoveredClip != nullptr;
    bool clickedOnAutomationLane = m_hoveredAutamatableParam != nullptr;
    bool clickedOnPoint = m_hoveredAutomationPoint != -1;
    bool clickedOnCurve = m_hoveredCurve != -1;
    bool clickedOnTimeRange = m_hoveredTimeRange && m_toolMode == Tool::pointer;


    if (clickedOnTimeRange && leftButton)
    {
        return;
    }

    if (clickedOnClip && leftButton && (m_toolMode == Tool::pointer || m_toolMode == Tool::timestretch))
    {
        clearSelectedTimeRange();

        if ((e.getNumberOfClicks() > 1 || m_editViewState.m_isPianoRollVisible) && m_hoveredClip->isMidi())
        {
            m_editViewState.m_isPianoRollVisible = true;

            for (auto t : te::getAllTracks(m_editViewState.m_edit)) 
            {
                t->state.setProperty(IDs::showLowerRange, false, nullptr);

                if (t == m_hoveredTrack.get())
                    t->state.setProperty(IDs::showLowerRange, true, nullptr);
            }
        }

        if (!sm.isSelected(m_hoveredClip))
            sm.selectOnly(m_hoveredClip);

        m_clipPosAtMouseDown = m_hoveredClip->getPosition().getStart().inSeconds();

        repaint();

        return;
    }

    if (clickedOnClip && leftButton && m_toolMode == Tool::knife)
    {
        splitClipAt(e.x, e.y);
        return;
    }

    if (clickedOnPoint && leftButton)
    {
        if (!isAutomationPointSelected(m_hoveredAutamatableParam, m_hoveredAutomationPoint))
            selectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint, false);

        m_timeOfHoveredAutomationPoint = m_hoveredAutamatableParam->getCurve().getPointTime(m_hoveredAutomationPoint);
        m_selPointsAtMousedown = getSelectedPoints();
        return;
    }

    if (clickedOnPoint && rightButton)
    {
        removeAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint);
        m_selPointsAtMousedown = getSelectedPoints();
        return;
    }

    if (clickedOnCurve && leftButton)
    {
        if (e.mods.isCtrlDown())
        {
            m_curveSteepAtMousedown = m_hoveredAutamatableParam->getCurve().getPointCurve(m_hoveredCurve -1);
        }
        else 
        {
            auto mouseTime = xtoTime(e.x);
            addAutomationPointAt(m_hoveredAutamatableParam, mouseTime);
        }
            
        m_selPointsAtMousedown = getSelectedPoints();
        m_timeOfHoveredAutomationPoint = m_hoveredAutamatableParam->getCurve().getPointTime(m_hoveredAutomationPoint);

        return;
    }

    if (clickedOnCurve && rightButton)
    {
        m_hoveredAutamatableParam->getCurve().setCurveValue(m_hoveredCurve, .0f);
        return;
    }

    if (clickedOnTrack && leftButton && e.getNumberOfClicks() > 1)
    {
        auto beat = e.mods.isShiftDown() ? xToBeatPosition(e.x) : getSnapedBeat(xToBeatPosition(e.x));
        if ((bool) m_hoveredTrack->state.getProperty(IDs::isMidiTrack))
            createNewMidiClip(beat.inBeats(), m_hoveredTrack);

        return;
    }

    if ((clickedOnTrack || clickedOnAutomationLane) && leftButton)
    {
        startLasso(e, clickedOnAutomationLane, m_toolMode == Tool::range);
    }
}

void SongEditorView::mouseDrag(const juce::MouseEvent&e)
{
    auto &sm = m_editViewState.m_selectionManager;

    auto isDraggingTimeRange = m_hoveredTimeRange && !m_isSelectingTimeRange && !m_hoveredTimeRangeLeft && !m_hoveredTimeRangeRight;
    auto isDraggingClip = m_hoveredClip != nullptr && (m_toolMode==Tool::pointer || m_toolMode==Tool::timestretch);
    auto isDraggingMultiSelectionToolSpan = m_lassoComponent.isVisible () || m_isSelectingTimeRange;
    auto isDraggingAutomationPoint = m_hoveredAutomationPoint != -1 && e.mouseWasDraggedSinceMouseDown ();
    auto isChangingCurveSteepness = m_hoveredAutomationPoint == -1 && m_hoveredCurve != -1 && e.mods.isCtrlDown ();
    auto snap = !e.mods.isShiftDown();

    if (isDraggingTimeRange)
    {
        m_isDraggingSelectedTimeRange = true;

        auto startTime = m_selectedRange.getStart();
        auto targetTime = startTime + distanceToTime(e.getDistanceFromDragStartX());

        if (snap)
        {
            startTime = getSnapedTime(startTime, true);
            targetTime = getSnapedTime(targetTime, false);
        }

        targetTime = juce::jmax(tracktion::TimePosition::fromSeconds(0), targetTime);
        auto delta = targetTime - startTime;

        m_draggedTimeDelta = delta;
    }
        
    else if (m_hoveredTimeRangeLeft)
    {
        auto newStartTime = juce::jmax(tracktion::TimePosition::fromSeconds(0.0), xtoTime(e.x));
        setSelectedTimeRange({newStartTime, m_selectedRange.getEnd()}, true, false);
    }

    else if (m_hoveredTimeRangeRight)
    {
        auto newEndTime = juce::jmax(m_selectedRange.getStart(), xtoTime(e.x));
        setSelectedTimeRange({m_selectedRange.getStart(), newEndTime}, false, false);
    }

    else if (isDraggingClip)
    {
        sm.addToSelection(m_hoveredClip);

        m_draggedClip = m_hoveredClip;
        m_draggedVerticalOffset = getVerticalOffset(m_hoveredTrack, {e.x, e.y});

        auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - xtoTime(0);

        auto startTime = m_draggedClip->getPosition().getStart();
        if (m_rightBorderHovered)
            startTime = m_draggedClip->getPosition().getEnd();

        auto targetTime = startTime + draggedTime;
        if (!e.mods.isShiftDown())
            targetTime = getSnapedTime(targetTime);

        m_draggedTimeDelta = targetTime - startTime;


        // sm.addToSelection(m_hoveredClip);
        //
        // m_draggedClip = m_hoveredClip;
        // m_draggedVerticalOffset = getVerticalOffset(m_hoveredTrack, {e.x, e.y});
        //
        // auto screenStartTime = xtoTime(0);
        // auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - screenStartTime; 
        //
        // auto startTime = m_draggedClip->getPosition().getStart();
        // if (m_rightBorderHovered)
        //     startTime = m_draggedClip->getPosition().getEnd();
        //
        // auto targetTime = startTime + draggedTime;
        //
        // if (!e.mods.isShiftDown())
        //     targetTime = getSnapedTime(targetTime);
        //
        // auto selectedClips = sm.getItemsOfType<te::Clip>();
        // auto selectedRange = tracktion::getTimeRangeForSelectedItems(selectedClips);
        // double selectionStart = selectedRange.getStart().inSeconds();
        // double dragedTimeDelta = juce::jmax(0.0 - selectionStart, targetTime.inSeconds() - startTime.inSeconds());
        // m_draggedTimeDelta = tracktion::TimeDuration::fromSeconds(dragedTimeDelta);
    }

    if (isDraggingMultiSelectionToolSpan)
        updateLasso (e);

    if (isDraggingAutomationPoint)
    { 
        auto lockTime = e.mods.isCtrlDown();

        auto oldPos = m_timeOfHoveredAutomationPoint;
        auto newPos = xtoTime(e.x); 

        if (lockTime)
            newPos = oldPos;
        else if (snap)
            newPos = getSnapedTime(xtoTime(e.x));

        auto draggedTime = newPos - oldPos;

        for (auto p : m_selPointsAtMousedown)
        {
            auto automationLane = getAutomationLane(p->param);
            auto newTime =  p->time + draggedTime;
            auto newValue = automationLane->getValue(automationLane->getYPos(p->value) + e.getDistanceFromDragStartY());

            p->param.getCurve().movePoint(p->index, newTime, newValue, false);
        }
    }

    if (isChangingCurveSteepness)
    {
        m_isDragging = true;
        getAutomationLane(m_hoveredAutamatableParam)->setIsDragging(m_isDragging);

        auto valueP1 = m_hoveredAutamatableParam->getCurve().getPointValue(m_hoveredCurve - 1);
        auto valueP2 = m_hoveredAutamatableParam->getCurve().getPointValue(m_hoveredCurve);
        auto delta =  valueP1 < valueP2 ? e.getDistanceFromDragStartY() *  0.01 
                                : e.getDistanceFromDragStartY() * -0.01;
        auto newSteep = juce::jlimit(-0.5f, 0.5f, (float) (m_curveSteepAtMousedown + delta));

        m_hoveredAutamatableParam->getCurve().setCurveValue(m_hoveredCurve - 1, newSteep);
    }

    repaint(); 
}

void SongEditorView::mouseUp(const juce::MouseEvent& e)
{
    auto &sm = m_editViewState.m_selectionManager;

    if (m_lassoComponent.isVisible () || m_isSelectingTimeRange)
    {
        auto start = m_lassoComponent.getLassoRect().m_timeRange.getStart();
        auto end = m_lassoComponent.getLassoRect().m_timeRange.getEnd();
        m_selectedRange.timeRange = {getSnapedTime(start, true), getSnapedTime(end, false)};
        stopLasso();
    }

    if (e.mods.isLeftButtonDown() &&(m_toolMode == Tool::pointer || m_toolMode==Tool::timestretch))
    {
        if (m_hoveredTimeRange && e.mouseWasDraggedSinceMouseDown() == false)
            clearSelectedTimeRange();

        if (m_isDraggingSelectedTimeRange)
        {

            moveSelectedTimeRanges(m_draggedTimeDelta, e.mods.isCtrlDown());
            auto newStart = m_selectedRange.getStart() + m_draggedTimeDelta;
            m_selectedRange.timeRange = m_selectedRange.timeRange.movedToStartAt(newStart);

        }
        else if (m_hoveredClip && e.mouseWasDraggedSinceMouseDown())
        {
            auto verticalOffset = getVerticalOffset(m_hoveredClip->getTrack(), e.position.toInt());
    
            if ((e.mods.isCommandDown() || m_toolMode==Tool::timestretch) && m_rightBorderHovered && !m_hoveredClip->isMidi() )
            {
                for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
                {
                    if (auto wac = dynamic_cast<te::WaveAudioClip*>(c))
                    {
                        setNewTempoOfClipByNewLength(wac, c->getPosition().getLength().inSeconds() + m_draggedTimeDelta.inSeconds());
                        m_editViewState.removeThumbnail(wac->itemID);
                    }
                }
            }
            else if (m_leftBorderHovered || m_rightBorderHovered)
            {
                EngineHelpers::resizeSelectedClips(m_leftBorderHovered, m_draggedTimeDelta.inSeconds(), m_editViewState);
            }
            else
            {
                EngineHelpers::moveSelectedClips(e.mods.isCtrlDown(), m_draggedTimeDelta.inSeconds(), verticalOffset, m_editViewState);
            }
        }
        else if (m_hoveredClip && !e.mouseWasDraggedSinceMouseDown() && !e.mods.isCtrlDown())
        {
            clearSelectedTimeRange();
            sm.selectOnly(m_hoveredClip);
        }
        else if (m_hoveredClip && !e.mouseWasDraggedSinceMouseDown())
        {
            clearSelectedTimeRange();
            if (sm.isSelected(m_hoveredClip))
            {
                sm.deselect(m_hoveredClip);
            }
            else
            {
                sm.addToSelection(m_hoveredClip);
            }
        }
        else if (m_hoveredAutomationPoint != -1 && !e.mouseWasDraggedSinceMouseDown())
        {
            if (m_hoveredAutamatableParam)
            {
                clearSelectedTimeRange();
                if (e.mods.isCtrlDown())
                {
                    if (isAutomationPointSelected(m_hoveredAutamatableParam, m_hoveredAutomationPoint))
                        deselectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint);
                    else
                        selectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint, true);
                }
                else
                {
                    selectAutomationPoint(m_hoveredAutamatableParam, m_hoveredAutomationPoint, false);
                }
            }
        }
        else if (m_hoveredAutomationPoint == -1 && m_hoveredCurve == -1 && m_hoveredClip == nullptr && !e.mouseWasDraggedSinceMouseDown())
        {
            clearSelectedTimeRange();
            sm.deselectAll();
        }
    }
    if (m_toolMode == Tool::timestretch || m_toolMode == Tool::range || m_toolMode == Tool::lasso)
        for (auto b : m_toolBar.getButtons())
            if (b->getName() == "select")
                b->setToggleState(true, juce::sendNotification);

    m_draggedClip = nullptr;
    m_isDraggingSelectedTimeRange = false;
    m_hoveredTimeRangeRight = false;
    m_hoveredTimeRangeLeft = false;
    m_isDragging = false;
    if (m_hoveredAutamatableParam)
        getAutomationLane(m_hoveredAutamatableParam)->setIsDragging(false);
    m_editViewState.m_isSavingLocked = false;
            
    repaint();
    // mouseMove(e);
}

void SongEditorView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_editViewState.m_edit.getTransport())
    {
        for (auto t : te::getAudioTracks(m_editViewState.m_edit))
            buildRecordingClips(t);
    }
    else if (source == &m_editViewState.m_selectionManager)
    {
        for (auto p : m_selectedAutomationPoints)
            if (! m_editViewState.m_selectionManager.isSelected(p))
                m_selectedAutomationPoints.removeObject(p, true);
    }
    else if (source == m_editViewState.m_trackHeightManager.get())
    {
        resized();
    }
}

bool SongEditorView::isInterestedInDragSource (const SourceDetails& dragSourceDetails) 
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
        return true;
    GUIHelpers::log(dragSourceDetails.description.toString());
    if (dragSourceDetails.description == "FileBrowser")
    {
        auto f = juce::File();
        if (auto b = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
            f = b->getSelectedFile();
        if (f.existsAsFile())
        {
            if(f.getFileName().endsWith("tracktion_edit"))
                return true;
            auto af = te::AudioFile(m_editViewState.m_edit.engine, f);
            if (af.isValid())
                return true;
        }
    }
    if (dragSourceDetails.description == "SampleBrowser")
        return true;

    return false;
}

void SongEditorView::itemDragEnter (const SourceDetails& dragSourceDetails)
{
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>
        (dragSourceDetails.sourceComponent.get()))
    {
    }
}

void SongEditorView::itemDragMove (const SourceDetails& dragSourceDetails) 
{
    auto pos = dragSourceDetails.localPosition;
    bool isShiftDown = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    auto dropTime = isShiftDown ? xtoTime(pos.x) : getSnapedTime(xtoTime(pos.x)); 

    auto f = juce::File();
    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>(dragSourceDetails.sourceComponent.get()))
        f = fileTreeComp->getSelectedFile();
    else if (auto fileBrowser = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
        f = fileBrowser->getSelectedFile();

    m_dragItemRect.valid = false;
    if (f.exists())
    {
        if (auto targetTrack = getTrackAt(pos.y))
        {
            te::AudioFile audioFile (m_editViewState.m_edit.engine, f);

            auto x = timeToX(dropTime);
            auto y = (float)getYForTrack(targetTrack);
            auto w = timeDurationToPixel(tracktion::TimeDuration::fromSeconds(audioFile.getLength()));
            auto h = (float)m_editViewState.m_trackHeightManager->getTrackHeight(targetTrack, false);

            m_dragItemRect.drawRect = {x, y, w, h};
            m_dragItemRect.name = f.getFileNameWithoutExtension();
            m_dragItemRect.visible = true;

            if (auto at = dynamic_cast<te::AudioTrack*>(targetTrack.get()))
            {
                if (!at->state.getProperty(IDs::isMidiTrack))
                {
                    m_dragItemRect.valid = true;
                    m_dragItemRect.colour = targetTrack->getColour();
                }
            }
        }
        else
        {
            auto lastTrackID =  m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit).getLast(); 
            auto lastTrack = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, lastTrackID);
            te::AudioFile audioFile (m_editViewState.m_edit.engine, f);
            auto x = timeToX(dropTime);
            float y = lastTrack ? getYForTrack(lastTrack) +  m_editViewState.m_trackHeightManager->getTrackHeight(lastTrack, true) : 0;
            auto w = timeDurationToPixel(tracktion::TimeDuration::fromSeconds(audioFile.getLength()));
            float h = static_cast<int>(m_editViewState.m_trackDefaultHeight);
            m_dragItemRect.drawRect = {x, y, w, h};
            m_dragItemRect.name = f.getFileNameWithoutExtension();
            m_dragItemRect.valid = true;
            m_dragItemRect.visible = true;
            m_dragItemRect.colour = m_editViewState.m_applicationState.getPrimeColour();
        }
    }

    repaint();
}

void SongEditorView::itemDragExit (const SourceDetails& dragSourceDetails) 
{
    m_dragItemRect.visible = false;
    repaint();
}

void SongEditorView::itemDropped (const SourceDetails& dragSourceDetails) 
{
    auto pos = dragSourceDetails.localPosition;
    bool isShiftDown = juce::ModifierKeys::getCurrentModifiers().isShiftDown();
    auto dropTime = isShiftDown ? xtoTime(pos.x) : getSnapedTime(xtoTime(pos.x)); 
    auto f = juce::File();

    if (auto fileTreeComp = dynamic_cast<juce::FileTreeComponent*>(dragSourceDetails.sourceComponent.get()))
        auto f = fileTreeComp->getSelectedFile();
    else if (auto fileBrowser = dynamic_cast<BrowserListBox*>(dragSourceDetails.sourceComponent.get()))
        f = fileBrowser->getSelectedFile();

    if (f.exists())
    {
        if (auto targetTrack = getTrackAt(pos.y))
        {
            if (auto at = dynamic_cast<te::AudioTrack*>(targetTrack.get()))
            {
                if (!at->state.getProperty(IDs::isMidiTrack))
                {
                    te::AudioFile audioFile (m_editViewState.m_edit.engine, f);
                    addWaveFileToTrack(audioFile, dropTime.inSeconds(), at);
                }
            }
        }
        else 
        {
            EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, f, m_editViewState.m_applicationState.getRandomTrackColour(), dropTime.inSeconds());
        }
    }
    m_dragItemRect.visible = false;
    repaint();
}

void SongEditorView::addWaveFileToTrack(te::AudioFile audioFile, double dropTime, te::AudioTrack::Ptr track) const
{
    if (audioFile.isValid())
    {
        auto length = tracktion::TimeDuration::fromSeconds(audioFile.getLength());
        auto dropPos = tracktion::TimePosition::fromSeconds(dropTime);
        te::ClipPosition clipPos;
        clipPos.time = {dropPos,length};

        EngineHelpers::loadAudioFileToTrack(audioFile.getFile(), track, clipPos);
    }
}

te::Track::Ptr SongEditorView::getTrackAt(int y)
{
    for (auto trackID : m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        auto s = getYForTrack(t);
        auto e = s + m_editViewState.m_trackHeightManager->getTrackHeight(t, true);
        auto vRange = juce::Range<int> (s, e);
        if (vRange.contains(y))
            return t;
    }
    
    return nullptr;
}

int SongEditorView::getYForTrack (te::Track* track)
{
    int scrollY = - (m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
    auto h = m_editViewState.m_trackHeightManager->getYForTrack(track, scrollY);
    return h;
}


juce::Rectangle<float> SongEditorView::getAutomationRect (te::AutomatableParameter::Ptr ap)
{
    int scrollY = -(m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
    float x = static_cast<float>(getLocalBounds().getX());
    float y = static_cast<float>(m_editViewState.m_trackHeightManager->getYForAutomatableParameter(ap->getTrack(), ap, scrollY));
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(m_editViewState.m_trackHeightManager->getAutomationHeight(ap));
    return {x, y, w, h};
}


void SongEditorView::addAutomationPointAt(te::AutomatableParameter::Ptr par, tracktion::TimePosition pos)
{
    auto valueAtTime = m_hoveredAutamatableParam->getCurve().getValueAt(pos);
        
    auto p = par->getCurve().addPoint(pos, valueAtTime, 0.f);
    m_hoveredCurve = -1;
    m_hoveredAutomationPoint = p;
    selectAutomationPoint(par, p, false);
    m_isDragging = true;
    getAutomationLane(par)->setIsDragging(m_isDragging);
    getAutomationLane(par)->invalidateCurveCache();
}

void SongEditorView::selectAutomationPoint(te::AutomatableParameter::Ptr ap,int index, bool add)
{
    if (index >= 0 && index < ap->getCurve().getNumPoints())
    {
        auto selectablePoint = std::make_unique<SelectableAutomationPoint>(index, ap->getCurve());
        m_editViewState.m_selectionManager.select(selectablePoint.get(), add);
        m_selectedAutomationPoints.add(std::move(selectablePoint));
    }
}

bool SongEditorView::isAutomationPointSelected(te::AutomatableParameter::Ptr ap, int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == ap->getCurve().getOwnerParameter() && p->index == index)
            return true;

    return false;
}

void SongEditorView::deselectAutomationPoint(te::AutomatableParameter::Ptr ap, int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == ap->getCurve().getOwnerParameter() && p->index == index)
            p->deselect();
}

juce::OwnedArray<SongEditorView::CurvePoint> SongEditorView::getSelectedPoints()
{
    auto &sm = m_editViewState.m_selectionManager;
    juce::OwnedArray<SongEditorView::CurvePoint> points;
    for (auto p : sm.getItemsOfType<SelectableAutomationPoint>())
    {
        auto cp = std::make_unique<SongEditorView::CurvePoint>(
                                 p->m_curve.getPointTime(p->index),
                                 p->m_curve.getPointValue(p->index),
                                 p->index,
                                *p->m_curve.getOwnerParameter());

        points.add(std::move(cp));
    }

    return points;
}

void SongEditorView::startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange)
{
    m_lassoComponent.startLasso({e.x, e.y}, m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()), selectRange);
    m_isSelectingTimeRange = selectRange;
    m_lassoStartsInAutomation = fromAutomation;
    if (selectRange)
    {
        clearSelectedTimeRange();
        m_cachedSelectedClips.clear();

    }
    else 
    {
        if (fromAutomation)
        {
            m_cachedSelectedClips.clear();
        }
        else
        {
            updateClipCache();    
        }
    }
}
void SongEditorView::updateLasso(const juce::MouseEvent& e )
{
    if (m_lassoComponent.isVisible () || m_isSelectingTimeRange)
    {
        m_lassoComponent.updateLasso({e.x, e.y}, m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));
        if (m_isSelectingTimeRange)
            updateRangeSelection();
        else
            if (m_lassoStartsInAutomation)
                updateAutomationSelection(e.mods.isShiftDown());
            else
                updateClipSelection(e.mods.isShiftDown());
    }   
}

void SongEditorView::stopLasso()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
    m_lassoComponent.stopLasso();
    m_lassoStartsInAutomation = false;
    m_isSelectingTimeRange = false;  
}

void SongEditorView::duplicateSelectedClipsOrTimeRange()
{
    // This function handles duplication of either selected clips or a time range.
    // The editor allows selection of either individual clips OR a time range, but not both simultaneously.
    auto isTimeRangeSelected = m_selectedRange.selectedTracks.size() != 0;

    if (isTimeRangeSelected)
    {
        moveSelectedTimeRanges(m_selectedRange.getLength(), true);
        setSelectedTimeRange({m_selectedRange.getStart() + m_selectedRange.getLength(), m_selectedRange.getLength()},false, false);
    }
    else
    {
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        auto range = te::getTimeRangeForSelectedItems(selectedClips);
        auto delta = range.getLength().inSeconds();

        GUIHelpers::log("moving Clips.");
        moveSelectedClips(true, delta, 0);
        GUIHelpers::log("Clips moved.");
    }
}

void SongEditorView::updateCursor(juce::ModifierKeys modifierKeys)
{
    auto timeRightcursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::TimeShiftRight, *this);
    auto shiftRightcursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftRight, *this);
    auto shiftLeftcursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftLeft, *this);
    auto shiftHandCursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::ShiftHand, *this);
    auto curveSteepnesCursor = GUIHelpers::createCustomMouseCursor(GUIHelpers::CustomMouseCursor::CurveSteepnes, *this);

    if (m_hoveredTrack && m_hoveredTrack->isFolderTrack() && !m_hoveredTimeRange)
    {
    }
    else if (m_hoveredClip != nullptr && !m_hoveredTimeRange && (m_toolMode == Tool::pointer || m_toolMode == Tool::timestretch))   
    {
        if (m_leftBorderHovered)
        {
            setMouseCursor(shiftLeftcursor);
        }
        else if (m_rightBorderHovered)
        {
            if ((modifierKeys.isCommandDown() || m_toolMode == Tool::timestretch) && !m_hoveredClip->isMidi())
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
    else if (m_hoveredClip != nullptr && m_toolMode == Tool::knife)
    {
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }
    else if (m_toolMode == Tool::range)
    {
        setMouseCursor(juce::MouseCursor::IBeamCursor);
    }
    else if (m_hoveredAutamatableParam && !m_hoveredTimeRange)
    {
        if (m_hoveredAutomationPoint != -1)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        else if (m_hoveredCurve != -1 && modifierKeys.isCtrlDown())
        {
            setMouseCursor(curveSteepnesCursor);
            m_isDragging = true;
            getAutomationLane(m_hoveredAutamatableParam)->setIsDragging(m_isDragging);
        }
        else if (m_hoveredCurve != -1)
        {
            setMouseCursor(juce::MouseCursor::NoCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
        }
    }

    else if (!m_hoveredTimeRange)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    else if (m_hoveredTimeRange)
    {
        if (m_hoveredTimeRangeLeft)
           setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
        else if (m_hoveredTimeRangeRight)
           setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }
}

void SongEditorView::moveSelectedClips(bool copy,  double delta, int verticalOffset)
{
    EngineHelpers::moveSelectedClips(copy, delta, verticalOffset,m_editViewState); 
    repaint();
}


int SongEditorView::getVerticalOffset(te::Track::Ptr sourceTrack, const juce::Point<int>& dropPos)
{
    auto targetTrack = getTrackAt(dropPos.getY());

    if (targetTrack)
    {
        auto showedTracks = m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit); 
        if (showedTracks.contains(sourceTrack->itemID) && showedTracks.contains(targetTrack->itemID))
            return showedTracks.indexOf(targetTrack->itemID) - showedTracks.indexOf(sourceTrack->itemID);
    }

    return 0;
}

void SongEditorView::resizeSelectedClips(bool snap, bool fromLeftEdge)
{
    EngineHelpers::resizeSelectedClips(fromLeftEdge, m_draggedTimeDelta.inSeconds(), m_editViewState);
}


tracktion_engine::MidiClip::Ptr SongEditorView::createNewMidiClip(double beatPos, te::Track::Ptr track)
{

    if (auto at = dynamic_cast<te::AudioTrack*>(track.get()))
    {

        auto start = tracktion::core::TimePosition::fromSeconds(
            juce::jmax(0.0, m_editViewState.beatToTime(beatPos)));
        auto end = tracktion::core::TimePosition::fromSeconds(
            juce::jmax(0.0, m_editViewState.beatToTime(beatPos))
            + m_editViewState.beatToTime(4));
        tracktion::core::TimeRange newPos(start, end);
        at->deleteRegion(newPos, &m_editViewState.m_selectionManager);

        auto mc = at->insertMIDIClip(newPos, &m_editViewState.m_selectionManager);
        mc->setName(at->getName());
        GUIHelpers::centerMidiEditorToClip(m_editViewState, mc, m_timeLine.getTimeLineID(), getWidth());

        return mc;
    }
    return nullptr;

}

void SongEditorView::updateClipCache()
{
    clearSelectedTimeRange();
    m_cachedSelectedClips.clear();

    for (auto c : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        m_cachedSelectedClips.add(c);
}

void SongEditorView::updateRangeSelection()
{
    auto& sm = m_editViewState.m_selectionManager;
    sm.deselectAll();
    clearSelectedTimeRange();
   
    auto range = m_lassoComponent.getLassoRect().m_timeRange;
    juce::Range<int> lassoRangeY = m_lassoComponent.getLassoRect().m_verticalRange; 

    for (auto trackID: m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        auto trackVRange = getVerticalRangeOfTrack(t, false);
        if (trackVRange.intersects (lassoRangeY))
            m_selectedRange.selectedTracks.add(t);
    }

    for (auto& ap : m_editViewState.m_edit.getAllAutomatableParams(true))
    {
        if (m_editViewState.m_trackHeightManager->isAutomationVisible(*ap))
        {
            auto rect = getAutomationRect(ap);
            juce::Range<int> vRange = juce::Range<int>(rect.getY(), rect.getBottom());
            if (vRange.intersects(lassoRangeY))
                m_selectedRange.selectedAutomations.addIfNotAlreadyThere(ap);
        }
    }

    setSelectedTimeRange(range,true, false);
}

void SongEditorView::clearSelectedTimeRange()
{
   m_selectedRange.timeRange = tracktion::TimeRange();
   m_selectedRange.selectedTracks.clear();
    m_selectedRange.selectedAutomations.clear();
}

void SongEditorView::deleteSelectedTimeRange()
{
    for (auto t : m_selectedRange.selectedTracks)
        if (auto ct = dynamic_cast<te::ClipTrack*>(t))
            ct->deleteRegion(m_selectedRange.timeRange, &m_editViewState.m_selectionManager);

    for (auto a : m_selectedRange.selectedAutomations)
        a->getCurve().removePointsInRegion(m_selectedRange.timeRange);
}
void SongEditorView::setSelectedTimeRange(tracktion::TimeRange tr, bool snapDownAtStart, bool snapDownAtEnd)
{
    auto start = tr.getStart();
    auto end = tr.getEnd();
    m_selectedRange.timeRange = {getSnapedTime(start, snapDownAtStart), getSnapedTime(end, snapDownAtEnd)};
}

juce::Array<te::Track*> SongEditorView::getTracksWithSelectedTimeRange()
{
    return m_selectedRange.selectedTracks;
}

tracktion::TimeRange SongEditorView::getSelectedTimeRange()
{
    return m_selectedRange.timeRange;
}

void SongEditorView::splitClipAt(int x, int y)
{
    if (m_hoveredClip)
    {
        te::splitClips({m_hoveredClip}, xtoTime(x));
    }
}

void SongEditorView::reverseSelectedClips()
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        
    for (auto c : selectedClips)
    {
        if (auto wac = dynamic_cast<te::WaveAudioClip*>(c))
        {
            auto reversed = wac->getIsReversed();
            wac->setIsReversed(!reversed);
            m_editViewState.removeThumbnail(wac->itemID);
        }
    }
}
void SongEditorView::transposeSelectedClips(float pitchChange)
{
    auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        
    for (auto c : selectedClips)
    {
        if (auto wac = dynamic_cast<te::WaveAudioClip*>(c))
        {
            auto pitch = wac->getPitchChange();
            wac->setPitchChange(pitch + pitchChange);
            m_editViewState.removeThumbnail(wac->itemID);
        }
    }
}

void SongEditorView::setNewTempoOfClipByNewLength(te::WaveAudioClip::Ptr wac, double newLength)
{
    auto& engine = m_editViewState.m_edit.engine;

    wac->setTimeStretchMode(te::TimeStretcher::soundtouchBetter);

    const auto audioFileInfo = wac->getAudioFile().getInfo();

    const auto loopInfo = audioFileInfo.loopInfo;
    const auto tempo = loopInfo.getBpm (audioFileInfo);

    GUIHelpers::log("file's BPM: ", tempo);

    if (newLength != 0)
    {
        const double newSpeedRatio =  audioFileInfo.getLengthInSeconds() / newLength;

        wac->setSpeedRatio(newSpeedRatio);
        wac->setLength(tracktion::TimeDuration::fromSeconds(audioFileInfo.getLengthInSeconds()) / wac->getSpeedRatio(), true);
    }
}

void SongEditorView::updateAutomationSelection(bool add)
{
    auto& sm = m_editViewState.m_selectionManager;
    juce::Array<SelectableAutomationPoint*> previouslySelectedPoints = m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>();

    sm.deselectAll();
    clearSelectedTimeRange();
    auto& trackHeightManager = m_editViewState.m_trackHeightManager;
    auto scrollY = - (m_editViewState.getViewYScroll(m_timeLine.getTimeLineID()));

    for (auto trackID : m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        auto trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(t, false);

        if (!t->state.getProperty(IDs::isTrackMinimized) && trackHeight > 0)
        {
            for (auto ap : t->getAllAutomatableParams())
            {
                auto al = getAutomationLane(ap);
                auto range = m_lassoComponent.getLassoRect().m_timeRange;
                auto firstPoint = ap->getCurve().nextIndexAfter(range.getStart());
                auto lastPoint = ap->getCurve().indexBefore(range.getEnd());

                for (auto i = firstPoint ; i <= lastPoint; i++)
                {
                    auto p = ap->getCurve().getPoint(i);
                    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
                    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
                    auto pointPos = al->getPointOnAutomationRect(p.time, p.value, getWidth(), x1, x2);
                    pointPos.addXY(0, trackHeightManager->getYForAutomatableParameter(ap->getTrack(), ap, scrollY));

                    if (m_lassoComponent.getLassoRect().m_verticalRange.contains(pointPos.getY()))
                       selectAutomationPoint(ap, i, true);
                }
            }
        }
    }
    if (add) 
        for (auto p : previouslySelectedPoints)
            m_editViewState.m_selectionManager.addToSelection(p);
}

void SongEditorView::updateClipSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll ();

    for (auto trackID : m_editViewState.m_trackHeightManager->getShowedTracks(m_editViewState.m_edit))
    {
        auto t = m_editViewState.m_trackHeightManager->getTrackFromID(m_editViewState.m_edit, trackID);
        juce::Range<int> lassoRangeY = 
            {(int) m_lassoComponent.getLassoRect ().m_verticalRange.getStart(),
             (int) m_lassoComponent.getLassoRect ().m_verticalRange.getEnd()};
        if (getVerticalRangeOfTrack(t, false).intersects (lassoRangeY) && !(t->isFolderTrack()))
            selectCatchedClips(t);
    }

    if (add)
        for (auto c : m_cachedSelectedClips)
            m_editViewState.m_selectionManager.addToSelection(c);
}

juce::Range<int> SongEditorView::getVerticalRangeOfTrack(
     tracktion_engine::Track::Ptr track, bool withAutomation) 
{   
    auto trackY = getYForTrack(track);
    auto trackHeight = m_editViewState.m_trackHeightManager->getTrackHeight(track, withAutomation);

    return {trackY , trackY + trackHeight};
}

void SongEditorView::selectCatchedClips(const tracktion_engine::Track *track)
{
    for (auto ti = 0; ti < track->getNumTrackItems(); ti++)
    {
        auto item = track->getTrackItem(ti);
        if (m_lassoComponent.getLassoRect ().m_startTime < item->getPosition ().getEnd ().inSeconds()
                && m_lassoComponent.getLassoRect ().m_endTime > item->getPosition ().getStart ().inSeconds())
        {
            m_editViewState.m_selectionManager.addToSelection(item);
        }
    }
}

void SongEditorView::moveSelectedTimeRanges(tracktion::TimeDuration td, bool copy)
{
    for (auto t : m_selectedRange.selectedTracks)
        if (t!= nullptr)
            moveSelectedRangeOfTrack(t, td, copy);

    for (auto ap : m_selectedRange.selectedAutomations)
    {
        auto as = EngineHelpers::getTrackAutomationSection(ap, m_selectedRange.timeRange);
        EngineHelpers::moveAutomationOrCopy(as, td, copy);
    }
}

void SongEditorView::moveSelectedRangeOfTrack(te::Track::Ptr track, tracktion::TimeDuration duration, bool copy)
{
    if (auto ct = dynamic_cast<te::ClipTrack*>(track.get()))
    {
        const auto editStart = tracktion::TimePosition::fromSeconds(0.0);
        const auto viewStartTime = m_editViewState.getVisibleTimeRange(m_timeLine.getTimeLineID(), getWidth()).getLength();
        const auto targetStart =  m_selectedRange.getStart() + duration;
        const auto targetEnd = m_selectedRange.getEnd() + duration;

        te::Clipboard::getInstance()->clear();
        auto clipContent = std::make_unique<te::Clipboard::Clips>();

        for (auto& c : ct->getClips())
           if (EngineHelpers::isTrackItemInRange(c, m_selectedRange.timeRange))
               clipContent->addClip(0, c->state);

        ct->deleteRegion({targetStart, targetEnd}, &m_editViewState.m_selectionManager);

        if (!copy)
           ct->deleteRegion(m_selectedRange.timeRange, &m_editViewState.m_selectionManager);

        te::EditInsertPoint insertPoint(m_editViewState.m_edit);
        insertPoint.setNextInsertPoint(tracktion::TimePosition(), track);
        te::Clipboard::ContentType::EditPastingOptions options(m_editViewState.m_edit, insertPoint);
        options.selectionManager = &m_editViewState.m_selectionManager;
        options.startTime = editStart + duration;

        clipContent->pasteIntoEdit(options);

        for (auto& clip : m_editViewState.m_selectionManager.getItemsOfType<te::Clip>())
        {
            constrainClipInRange (clip, {targetStart, targetEnd});
            m_editViewState.m_selectionManager.deselect(clip);
        }
    }
}

void SongEditorView::constrainClipInRange(te::Clip* c, tracktion::TimeRange r)
{
    auto pos = c->getPosition();

    if (!r.intersects(c->getPosition().time))
    {
        c->removeFromParent();
    }
    else 
    {
        if (pos.getStart() < r.getStart())
            c->setStart(r.getStart(), true, false);

        if (pos.getEnd() > r.getEnd())
            c->setEnd(r.getEnd(), true);
    }
}

tracktion::TimeDuration SongEditorView::distanceToTime(int distance)
{
    auto timePerPixel = m_editViewState.getVisibleTimeRange(m_timeLine.getTimeLineID(), getWidth()).getLength().inSeconds() / getWidth();
    auto time = distance * timePerPixel;
    return tracktion::TimeDuration::fromSeconds(time);
}

tracktion::BeatPosition SongEditorView::xToBeatPosition(int x)
{
    auto visibleRange = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth());
    auto beatsPerPixel = visibleRange.getLength().inBeats() / getWidth();

    auto startBeat = visibleRange.getStart().inBeats();
    auto beatPosition = (x * beatsPerPixel) + startBeat;

    return tracktion::BeatPosition::fromBeats(beatPosition);

}
tracktion::TimePosition SongEditorView::xtoTime(int x)
{
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    auto time = m_editViewState.xToTime(x, getWidth(), x1, x2);
    return tracktion::TimePosition::fromSeconds(time);
}

tracktion::BeatPosition SongEditorView::getSnapedBeat(tracktion::BeatPosition beatPos, bool downwards)
{

    auto& ts = m_editViewState.m_edit.tempoSequence;
    auto timePos = ts.toTime(beatPos);
    auto snapTime = getSnapedTime(timePos, downwards);
    
    return ts.toBeats(snapTime);

}

tracktion::TimePosition SongEditorView::getSnapedTime(tracktion::TimePosition time, bool downwards)
{
    double x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    double x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    auto snapType = m_editViewState.getBestSnapType(x1,x2, getWidth());
    auto t = time.inSeconds();
    auto st = m_editViewState.getSnapedTime(t, snapType, downwards);

    return tracktion::TimePosition::fromSeconds(st);
}

float SongEditorView::timeDurationToPixel(tracktion::TimeDuration duration)
{
    float timePerPixel = m_editViewState.getVisibleTimeRange(m_timeLine.getTimeLineID(), getWidth()).getLength().inSeconds() / getWidth();
    return duration.inSeconds() / timePerPixel;
}
float SongEditorView::timeToX (tracktion::TimePosition time)
{
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    return m_editViewState.timeToX (time.inSeconds(), getWidth(),x1, x2);
}

float SongEditorView::beatToX (tracktion::BeatPosition beat)
{
    auto x1 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getStart().inBeats();
    auto x2 = m_editViewState.getVisibleBeatRange(m_timeLine.getTimeLineID(), getWidth()).getEnd().inBeats();
    return m_editViewState.beatsToX(beat.inBeats(), getWidth(), x1, x2);
}

double SongEditorView::xToSnapedBeat (int x)
{
    auto time = xtoTime(x);
    time = getSnapedTime(time);
    return m_editViewState.timeToBeat(time.inSeconds());
}


juce::Rectangle<float> SongEditorView::getClipRect (te::Clip::Ptr clip)
{
    float x = timeToX(clip->getPosition().getStart());
    float y = static_cast<float>(getYForTrack(clip->getClipTrack()));
    float w = timeToX(clip->getPosition().getEnd()) - x;
    float h = static_cast<float>(m_editViewState.m_trackHeightManager->getTrackHeight(clip->getClipTrack(), false));

    juce::Rectangle<float> clipRect = {x, y, w, h};
    return clipRect;
}

void SongEditorView::renderSelectedTimeRangeToNewTrack()
{
    if (getTracksWithSelectedTimeRange().size() <= 0)
        return;

    auto selectedTracks = m_selectedRange.selectedTracks;

    auto range = getSelectedTimeRange();

    EngineHelpers::renderToNewTrack(m_editViewState, selectedTracks, range);
}

void SongEditorView::buildRecordingClips(te::Track::Ptr track)
{
    bool needed = false;

    if (track->edit.getTransport().isRecording())
    {
        for (auto in : track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive() && track->itemID == in->getTargets().getFirst())
            {
                needed = true;
                break;
            }
        }
    }
    if (needed)
    {
        for (auto rc : m_recordingClips)
            if (rc->getTrack() == track)
                break;

        auto recordingClip = std::make_unique<RecordingClipComponent>(track, m_editViewState, m_timeLine);
        addAndMakeVisible(*recordingClip);
        m_recordingClips.add(std::move(recordingClip));
    }
    else
    {
        for (auto rc : m_recordingClips)
            if (rc->getTrack() == track)
                m_recordingClips.removeObject(rc, true);
    }
}



void SongEditorView::logMousePositionInfo()
{
    GUIHelpers::log("------------------------------------------------------------");
    GUIHelpers::log("ToolMode    : ", (int) m_toolMode);
    GUIHelpers::log("TimeRange   : ", m_hoveredTimeRange);
    GUIHelpers::log("TimeRange L : ", m_hoveredTimeRangeLeft);
    GUIHelpers::log("TimeRange R : ", m_hoveredTimeRangeRight);
     GUIHelpers::log("Track       : ", m_hoveredTrack != nullptr);
    if (m_hoveredTrack)
    {
        GUIHelpers::log("Track : ", m_hoveredTrack->getName());
    }
    GUIHelpers::log("Clip        : ", m_hoveredClip != nullptr);
    GUIHelpers::log("Clip      L : ", m_leftBorderHovered);
    GUIHelpers::log("Clip      R : ", m_rightBorderHovered);
    if (m_hoveredAutamatableParam)
    {
        GUIHelpers::log("Automation  : ", m_hoveredAutamatableParam != nullptr);
        GUIHelpers::log("Track: ", m_hoveredAutamatableParam->getTrack()->getName());
        std::cout << "Automation name: " << m_hoveredAutamatableParam->getParameterName() << std::endl;

        GUIHelpers::log("Point       : ", m_hoveredAutomationPoint) ;
        GUIHelpers::log("Curve       : ", m_hoveredCurve);

    }
    repaint();
}
