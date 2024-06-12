
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
#include "FileBrowser.h"
#include "SampleBrowser.h"
#include "Utilities.h"


SongEditorView::SongEditorView(EditViewState& evs, MenuBar& toolBar)
        : m_editViewState(evs)
        , m_toolBar(toolBar)
        , m_lassoComponent(evs, evs.m_viewX1, evs.m_viewX2)
{		
    setWantsKeyboardFocus (true);
    setName("SongEditorView");
    addChildComponent(m_lassoComponent);
    m_lassoComponent.setVisible(false);
    m_lassoComponent.setAlwaysOnTop (true);
    m_lassoComponent.toFront (true);
    clearSelectedTimeRange();

    m_editViewState.m_edit.getTransport().addChangeListener(this);
}

SongEditorView::~SongEditorView()
{
    m_editViewState.m_edit.getTransport().removeChangeListener(this);
}

void SongEditorView::paint(juce::Graphics& g)
{
    GUIHelpers::log("SongEditor repaint()");
    using namespace juce::Colours;
    auto &sm = m_editViewState.m_selectionManager;
    auto scroll = timeToX(0) * (-1);

    auto area = getLocalBounds();
    g.setColour(juce::Colour(0xff303030));
    g.fillRect(area);

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        if (auto ct = dynamic_cast<te::ClipTrack*>(t))
        {
            if (EngineHelpers::isTrackShowable(t))
            {
                auto x = 0;
                auto y = getYForTrack(t);
                auto w = getWidth();
                auto h = getTrackHeight(t, m_editViewState, false);

                GUIHelpers::drawTrack(g, *this, m_editViewState, {x, y, w, h}, ct, m_editViewState.getSongEditorVisibleTimeRange());

                if (m_selectedRange.selectedTracks.contains(t) && m_selectedRange.getLength().inSeconds() > 0)
                {
                    auto x = timeToX(m_selectedRange.getStart().inSeconds());
                    auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;

                    g.setColour(juce::Colour(0x50ffffff));
                    g.fillRect(juce::Rectangle<int>{x, y, w, h});
                }
            }
        }
        else if (t->isFolderTrack())
        {
            auto x = 0;
            auto y = getYForTrack(t);
            auto w = getWidth();
            auto h = getTrackHeight(t, m_editViewState, false);

            GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, m_editViewState.m_viewX1, m_editViewState.m_viewX2, {x,y,w,h});
        }
        

        for (auto& ap : t->getAllAutomatableParams())
        {
            if (GUIHelpers::isAutomationVisible(*ap))
            {
                auto rect = getAutomationRect(ap);
                    
                if (rect.getHeight() > 0)
                {
                    drawAutomationLane(g, m_editViewState.getSongEditorVisibleTimeRange(), rect, ap);

                    if (m_selectedRange.selectedAutomations.contains(ap) && m_selectedRange.getLength().inSeconds() > 0)
                    {
                        auto x = timeToX(m_selectedRange.getStart().inSeconds());
                        auto y = rect.getY();
                        auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;
                        auto h = rect.getHeight();

                        g.setColour(juce::Colour(0x50ffffff));
                        g.fillRect(juce::Rectangle<int>{x, y, w, h});
                    }
                }
            }
        }
    }

    if (m_isDraggingSelectedTimeRange)
    {
        auto selectedRangeRect = juce::Rectangle<int>();

        for (auto track : m_selectedRange.selectedTracks)
        {
            auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
            auto y = getYForTrack(track);
            auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;
            auto h = getTrackHeight(track, m_editViewState, false);
                
            x = x + timeToX(m_draggedTimeDelta) + scroll;

            juce::Rectangle<int> rect = {x, y, w, h};

            selectedRangeRect = selectedRangeRect.getUnion(rect);
            if (auto ct = dynamic_cast<te::ClipTrack*>(track))
                GUIHelpers::drawTrack(g, *this, m_editViewState, rect, ct, m_selectedRange.timeRange, true);
        }

        for (auto automation : m_selectedRange.selectedAutomations)
        {
            auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
            auto y = getAutomationRect(automation).getY();
            auto w = timeToX(m_selectedRange.getEnd().inSeconds()) - x;
            auto h = getHeightOfAutomation(automation);

            x = x + timeToX(m_draggedTimeDelta) + scroll;

            juce::Rectangle<int> rect = {x, y, w, h};

            drawAutomationLane(g, m_selectedRange.timeRange, rect, automation, true);

            selectedRangeRect = selectedRangeRect.getUnion(rect);
        }
        g.setColour(juce::Colours::yellowgreen);
        g.drawRect(selectedRangeRect);
    }

    if (m_hoveredTimeRangeLeft)
    {
        auto x = timeToX(m_selectedRange.getStart().inSeconds()); 
        auto y = getYForTrack(m_selectedRange.selectedTracks.getLast()) + getTrackHeight(m_selectedRange.selectedTracks.getLast(), m_editViewState, true);
        g.setColour(yellowgreen);
        g.drawVerticalLine(x, 0, y);
    }

    if (m_hoveredTimeRangeRight)
    {
        auto x = timeToX(m_selectedRange.getEnd().inSeconds()); 
        auto y = getYForTrack(m_selectedRange.selectedTracks.getLast()) + getTrackHeight(m_selectedRange.selectedTracks.getLast(), m_editViewState, true);
        g.setColour(yellowgreen);
        g.drawVerticalLine(x, 0, y);
    }
    if (m_draggedClip)
    {
        for (auto selectedClip : sm.getItemsOfType<te::Clip>())
        {
            if (auto targetTrack = EngineHelpers::getTargetTrack(selectedClip->getTrack(), m_draggedVerticalOffset))
            {
                auto clipRect = getClipRect(selectedClip);
                juce::Rectangle<int> targetRect = {clipRect.getX() + timeToX(m_draggedTimeDelta) + scroll,
                                                       getYForTrack(targetTrack),
                                                       clipRect.getWidth(),
                                                        getTrackHeight(targetTrack, m_editViewState, false)};
               
                if (m_leftBorderHovered)
                {
                    auto offset = selectedClip->getPosition().getOffset().inSeconds();
                    auto timeDelta = juce::jmax(0.0 - offset , m_draggedTimeDelta);
                    auto deltaX =  timeToX(timeDelta) + scroll;

                    targetRect = {clipRect.getX() + deltaX, getYForTrack(targetTrack),
                                  clipRect.getWidth() - deltaX, getTrackHeight(targetTrack, m_editViewState, false)};

                }
                else if (m_rightBorderHovered)
                {
                    targetRect = {clipRect.getX(), getYForTrack(targetTrack),
                                  clipRect.getWidth() + timeToX(m_draggedTimeDelta) + scroll, getTrackHeight(targetTrack, m_editViewState, false)};

                }

                g.setColour(white);
                g.drawRect(targetRect);

                if (EngineHelpers::trackWantsClip(selectedClip, targetTrack))
                {
                    juce::Rectangle<int> trackRect = {0, getYForTrack(targetTrack), getWidth(), getTrackHeight(targetTrack,m_editViewState, false)};
                    GUIHelpers::drawClip(g, *this, m_editViewState, targetRect, selectedClip, targetTrack->getColour().withAlpha(0.1f), trackRect, m_editViewState.m_viewX1, m_editViewState.m_viewX2);
                }
                else
                {
                    g.setColour(grey);
                    g.fillRect(targetRect.reduced(1,1));
                    g.setColour(black);
                    g.drawFittedText("not allowed",targetRect,juce::Justification::centred, 1);
                }
            }
        }
    }
    if (m_dragItemRect.visible)
    {
        if (m_dragItemRect.valid)
        {
            GUIHelpers::drawClipBody(g, m_editViewState, m_dragItemRect.name, m_dragItemRect.drawRect, false, m_dragItemRect.colour, m_dragItemRect.drawRect, m_editViewState.m_viewX1, m_editViewState.m_viewX2);
        }
        else
        {
            g.setColour(grey);
            g.fillRect(m_dragItemRect.drawRect.reduced(1,1));
            g.setColour(black);
            g.drawFittedText("not allowed",m_dragItemRect.drawRect,juce::Justification::centred, 1);
        }
    }

}

void SongEditorView::resized()
{
    m_lassoComponent.setBounds(getLocalBounds());
}


void SongEditorView::mouseMove (const juce::MouseEvent &e)
{
    //init
    m_isDragging = false;

    auto hoveredTrack = getTrackAt(e.y);
    auto hoveredAutamatableParam = getAutomatableParamAt(e.y);
    te::Clip::Ptr hoveredClip = nullptr;
    int hoveredAutomationPoint = -1;
    int hoveredCurve = -1;
    juce::Rectangle<int> hoveredRectOnAutomation;
    bool leftBorderHovered = false;
    bool rightBorderHovered = false;
    bool hoveredTimeRange = false;
    bool hoveredTimeRangeLeft = false;
    bool hoveredTimeRangeRight = false;

    auto mousePosTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
    m_timeAtMouseCursor = mousePosTime;

    bool needsRepaint = false;

    //Automation Lane hit tests 
    if (hoveredAutamatableParam)
    {
        auto cursorHitsSelectedRange = m_selectedRange.timeRange.contains(tracktion::TimePosition::fromSeconds(xtoTime(e.x)));

        if (m_selectedRange.selectedAutomations.contains(hoveredAutamatableParam) && cursorHitsSelectedRange)
        {
            int leftX = timeToX(m_selectedRange.getStart().inSeconds());
            int rightX = timeToX(m_selectedRange.getEnd().inSeconds());

            hoveredTimeRange = true;
            if (e.x < leftX + 5)
                hoveredTimeRangeLeft = true;
            else if (e.x > rightX - 5)
                hoveredTimeRangeRight = true;
        }
        else if (m_toolMode == Tool::pointer)
        {
            juce::Point<int> hoveredPointInLane = {e.x, e.y - getYForAutomatableParam(hoveredAutamatableParam)};
            const auto hoveredRectOnLane = GUIHelpers::getSensibleArea(hoveredPointInLane, getAutomationPointWidth(hoveredAutamatableParam) * 2) ;   
            auto curve = hoveredAutamatableParam->getCurve();

            for (auto i = 0; curve.getNumPoints() > i; i++)
            {
                auto time = curve.getPoint(i).time;
                auto value = curve.getPoint(i).value;

                auto pointXy = getPointOnAutomationRect(time, value, hoveredAutamatableParam, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);

                if (hoveredRectOnLane.contains(pointXy.toInt()))
                    hoveredAutomationPoint = i;
            }

            auto valueAtMouseTime = hoveredAutamatableParam->getCurve().getValueAt(mousePosTime);
            auto curvePointAtMouseTime = juce::Point<int>(e.x, getYPos(valueAtMouseTime, hoveredAutamatableParam));

            if (hoveredRectOnLane.contains(curvePointAtMouseTime) && hoveredAutomationPoint == -1)
                hoveredCurve = curve.nextIndexAfter(mousePosTime);
            
            juce::Point<int> cp = getPointOnAutomationRect(mousePosTime, valueAtMouseTime, hoveredAutamatableParam, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2).toInt();
            cp = cp.translated (0, getYForAutomatableParam(hoveredAutamatableParam));
            hoveredRectOnAutomation = GUIHelpers::getSensibleArea(cp, 8);
        }
        else if (m_toolMode == Tool::knife)
        {
            needsRepaint = true; 
        }
    }
    if (hoveredTrack && hoveredAutamatableParam == nullptr)
    {
        if (m_selectedRange.selectedTracks.contains(hoveredTrack) && m_selectedRange.timeRange.contains(tracktion::TimePosition::fromSeconds(xtoTime(e.x))))
        {
            int leftX = timeToX(m_selectedRange.getStart().inSeconds());
            int rightX = timeToX(m_selectedRange.getEnd().inSeconds());

            hoveredTimeRange = true;
            if (e.x < leftX + 5)
                hoveredTimeRangeLeft = true;
            else if (e.x > rightX - 5)
                hoveredTimeRangeRight = true;
        }
    }
    //clip hit test 
    if (auto at = dynamic_cast<te::AudioTrack*>(hoveredTrack.get()))
    {
        if (!hoveredTimeRange && hoveredAutamatableParam == nullptr)
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

    //
    // if (e.mods.isCtrlDown() && e.mods.isShiftDown())
    //     m_toolMode = Tool::knife;
    // else if (e.mods.isAltDown())
    //     m_toolMode = Tool::range;
    
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
        GUIHelpers::log("repaint needed");
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

    // repaint();
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
    m_draggedTimeDelta = 0.0;
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
        m_hoveredAutamatableParam->getCurve().removePoint(m_hoveredAutomationPoint);  
        sm.deselectAll();
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
            auto mouseTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
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
        auto beat = xToSnapedBeat(e.x);
        if ((bool) m_hoveredTrack->state.getProperty(IDs::isMidiTrack))
            createNewMidiClip(beat, m_hoveredTrack);

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

        auto startTime = xtoTime(e.getMouseDownX());
        auto targetTime = xtoTime(e.x);

        if (snap)
        {
            startTime = getSnapedTime(startTime, true);
            targetTime = getSnapedTime(targetTime, false);
        }

        m_draggedTimeDelta = targetTime - startTime; 
    }
        
    else if (m_hoveredTimeRangeLeft)
    {
        auto newStartTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
        setSelectedTimeRange({newStartTime, m_selectedRange.getEnd()}, true, false);
    }

    else if (m_hoveredTimeRangeRight)
    {
        auto newEndTime = tracktion::TimePosition::fromSeconds(xtoTime(e.x));
        setSelectedTimeRange({m_selectedRange.getStart(), newEndTime}, false, false);
    }

    else if (isDraggingClip)
    {
        sm.addToSelection(m_hoveredClip);
        m_draggedClip = m_hoveredClip;
        m_draggedVerticalOffset = getVerticalOffset(m_hoveredTrack, {e.x, e.y});

        auto screenStartTime = xtoTime(0);
        auto draggedTime = xtoTime(e.getDistanceFromDragStartX()) - screenStartTime; 

        auto startTime = m_draggedClip->getPosition().getStart().inSeconds();
        if (m_rightBorderHovered)
            startTime = m_draggedClip->getPosition().getEnd().inSeconds();

        auto targetTime = startTime + draggedTime;
        if (!e.mods.isShiftDown())
            targetTime = getSnapedTime(targetTime);

        m_draggedTimeDelta = targetTime - startTime;
    }

    if (isDraggingMultiSelectionToolSpan)
        updateLasso (e);

    if (isDraggingAutomationPoint)
    { 
        auto lockTime = e.mods.isCtrlDown();

        auto oldPos = m_timeOfHoveredAutomationPoint;
        auto newPos = tracktion::TimePosition::fromSeconds(xtoTime(e.x)); 

        if (lockTime)
            newPos = oldPos;
        else if (snap)
            newPos = tracktion::TimePosition::fromSeconds(getSnapedTime(xtoTime(e.x)));
        
        auto draggedTime = newPos - oldPos;

        for (auto p : m_selPointsAtMousedown)
        {
            auto newTime =  p->time + draggedTime;
            auto newValue = getValue(getYPos(p->value, p->param) + e.getDistanceFromDragStartY(), p->param);

            p->param.getCurve().movePoint(p->index, newTime, newValue, false);
        }
    }

    if (isChangingCurveSteepness)
    {
        m_isDragging = true;

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
        m_selectedRange.timeRange = {tracktion::TimePosition::fromSeconds(getSnapedTime(start.inSeconds(), true)),
                               tracktion::TimePosition::fromSeconds(getSnapedTime(end.inSeconds(), false))};
        stopLasso();
    }

    if (e.mods.isLeftButtonDown() &&(m_toolMode == Tool::pointer || m_toolMode==Tool::timestretch))
    {
        if (m_hoveredTimeRange && e.mouseWasDraggedSinceMouseDown() == false)
            clearSelectedTimeRange();

        if (m_isDraggingSelectedTimeRange)
        {
            moveSelectedTimeRanges(tracktion::TimeDuration::fromSeconds(m_draggedTimeDelta), e.mods.isCtrlDown());
            auto newStart = getSnapedTime(m_selectedRange.getStart().inSeconds() + m_draggedTimeDelta, true);
            setSelectedTimeRange(m_selectedRange.timeRange.movedToStartAt(tracktion::TimePosition::fromSeconds(newStart)),true, false);

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
                        setNewTempoOfClipByNewLength(wac, c->getPosition().getLength().inSeconds() + m_draggedTimeDelta);
                        m_editViewState.removeThumbnail(wac);
                    }
                }
            }
            else if (m_leftBorderHovered || m_rightBorderHovered)
            {
                EngineHelpers::resizeSelectedClips(m_leftBorderHovered, m_draggedTimeDelta, m_editViewState);
            }
            else
            {
                EngineHelpers::moveSelectedClips(e.mods.isCtrlDown(), m_draggedTimeDelta, verticalOffset, m_editViewState);
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
    m_editViewState.m_isSavingLocked = false;
            
    repaint();
    // mouseMove(e);
}

void SongEditorView::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    for (auto t : te::getAudioTracks(m_editViewState.m_edit))
    {
        buildRecordingClips(t);
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
            auto y = getYForTrack(targetTrack);
            auto w = timeToX(audioFile.getLength());
            auto h = getTrackHeight(targetTrack, m_editViewState, false);

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
            auto lastTrack = getShowedTracks().getLast(); 
            te::AudioFile audioFile (m_editViewState.m_edit.engine, f);
            auto x = timeToX(dropTime);
            auto y = lastTrack ? getYForTrack(lastTrack) + getTrackHeight(lastTrack, m_editViewState) : 0;
            auto w = timeToX(audioFile.getLength());
            auto h = static_cast<int>(m_editViewState.m_trackDefaultHeight);
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
                    addWaveFileToTrack(audioFile, dropTime, at);
                }
            }
        }
        else 
        {
            EngineHelpers::loadAudioFileOnNewTrack(m_editViewState, f, m_editViewState.m_applicationState.getRandomTrackColour(), dropTime);
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
    for (auto t : getShowedTracks())
    {
        auto s = getYForTrack(t);
        auto e = s + getTrackHeight(t, m_editViewState);
        auto vRange = juce::Range<int> (s, e);
        if (vRange.contains(y))
            return t;
    }
    
    return nullptr;
}

int SongEditorView::getYForTrack (te::Track* track)
{
    int scrollY = m_editViewState.m_viewY;
    for (auto t : getShowedTracks())
    {
        if (t.get() == track)
            return scrollY;
        
        scrollY += getTrackHeight(t, m_editViewState);
    }

    return getHeight();
}
// ToDo:  getAutomatableParamAt() is very inefficiet. It needs to be refactored
// I think it is nessesary to implement automation tracks for detecting an 
// Automation Lane at a certain y position
te::AutomatableParameter::Ptr SongEditorView::getAutomatableParamAt(int y)
{
    int scrollY = m_editViewState.m_viewY;

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
    {
        if (t->isAudioTrack() || t->isFolderTrack())
        {
            int trackHeight = getTrackHeight(t, m_editViewState, false);

            if (!t->state.getProperty(IDs::isTrackMinimized) && trackHeight > 0)
            {
                for (auto ap : t->getAllAutomatableParams())
                {
                    if (ap->getTrack() == t && GUIHelpers::isAutomationVisible(*ap))
                    {
                        int h = ap->getCurve().state.getProperty(te::IDs::height, static_cast<int>(m_editViewState.m_trackDefaultHeight));

                        if (juce::Range<int>(scrollY + trackHeight, scrollY + trackHeight + h).contains(y))
                        {
                            return ap;
                        }

                        scrollY += h;
                    }
                }
            }

            scrollY += trackHeight;
        }
    }

    return nullptr;
}

int SongEditorView::getYForAutomatableParam(te::AutomatableParameter::Ptr ap)
{
    double scrollY = m_editViewState.m_viewY;
    for (auto t : getShowedTracks())
    {
        auto trackHeight = getTrackHeight(t, m_editViewState, false); 
        scrollY += trackHeight;

        bool isMinimized = (bool) t->state.getProperty(IDs::isTrackMinimized);
        if (!isMinimized && trackHeight > 0)
        {
            for (auto p : t->getAllAutomatableParams())
            {
                if (p->getCurve().getNumPoints() > 0)
                {
                    if (p == ap.get())
                        return scrollY;

                    scrollY += getHeightOfAutomation(p);
                }   
            }
        }
    }

    jassert(false); 
    return -1;
}

juce::Rectangle<int> SongEditorView::getAutomationRect (te::AutomatableParameter::Ptr ap)
{
    auto x = getLocalBounds().getX();
    auto y = getYForAutomatableParam(ap);
    auto w = getWidth();
    auto h = getHeightOfAutomation(ap);
    return {x, y, w, h};
}

int SongEditorView::getHeightOfAutomation (te::AutomatableParameter::Ptr ap)
{
    if (ap->getTrack() == nullptr)
        return 0;
    if (getTrackHeight(ap->getTrack(), m_editViewState) == 0 || ap->getTrack()->state.getProperty(IDs::isTrackMinimized))
        return 0;

    return ap->getCurve().state.getProperty(te::IDs::height, static_cast<int>(m_editViewState.m_trackDefaultHeight));
}

void SongEditorView::addAutomationPointAt(te::AutomatableParameter::Ptr par, tracktion::TimePosition pos)
{
    auto valueAtTime = m_hoveredAutamatableParam->getCurve().getValueAt(pos);
        
    auto p = par->getCurve().addPoint(pos, valueAtTime, 0.f);
    m_hoveredCurve = -1;
    m_hoveredAutomationPoint = p;
    selectAutomationPoint(par, p, false);
    m_isDragging = true;
}

void SongEditorView::selectAutomationPoint(te::AutomatableParameter::Ptr ap,int index, bool add)
{
    if (index >= 0 && index < ap->getCurve().getNumPoints())
        m_editViewState.m_selectionManager.select(createSelectablePoint (ap, index), add);
}

SelectableAutomationPoint* SongEditorView::createSelectablePoint(te::AutomatableParameter::Ptr ap, int index)
{
    auto apoint = new SelectableAutomationPoint(index, ap->getCurve());
    return apoint;
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

juce::Array<SongEditorView::CurvePoint*> SongEditorView::getSelectedPoints()
{
    auto &sm = m_editViewState.m_selectionManager;
    juce::Array<CurvePoint*> points;
    for (auto p : sm.getItemsOfType<SelectableAutomationPoint>())
    {
        auto cp = new SongEditorView::CurvePoint(
                                 p->m_curve.getPointTime(p->index),
                                 p->m_curve.getPointValue(p->index),
                                 p->index,
                                *p->m_curve.getOwnerParameter());

        points.add(cp);
    }

    return points;
}

void SongEditorView::startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange)
{
    m_lassoComponent.startLasso({e.x, e.y}, m_editViewState.m_viewY, selectRange);
    m_isSelectingTimeRange = selectRange;
    m_lassoStartsInAutomation = fromAutomation;
    if (selectRange)
    {
        clearSelectedTimeRange();
        m_cachedSelectedAutomation.clear();
        m_cachedSelectedClips.clear();

    }
    else 
    {
        if (fromAutomation)
        {
            updateAutomationCache();
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
        m_lassoComponent.updateLasso({e.x, e.y}, m_editViewState.m_viewY);
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
    if (m_selectedRange.selectedTracks.size() == 0)
    {
        auto selectedClips = m_editViewState.m_selectionManager.getItemsOfType<te::Clip>();
        auto range = te::getTimeRangeForSelectedItems(selectedClips);
        auto delta = range.getLength().inSeconds();

        moveSelectedClips(true, delta, 0);
    }
    else 
    {
        moveSelectedTimeRanges(m_selectedRange.getLength(), true);
        setSelectedTimeRange({m_selectedRange.getStart() + m_selectedRange.getLength(), m_selectedRange.getLength()},false, false);
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
}

juce::Array<te::Track::Ptr> SongEditorView::getShowedTracks ()
{
    juce::Array<te::Track::Ptr> showedTracks;

    for (auto t : te::getAllTracks(m_editViewState.m_edit))
        if (EngineHelpers::isTrackShowable(t))
            if (getTrackHeight(t, m_editViewState) > 0)
                showedTracks.add(t);

    return showedTracks;
}

int SongEditorView::getVerticalOffset(te::Track::Ptr sourceTrack, const juce::Point<int>& dropPos)
{
    auto targetTrack = getTrackAt(dropPos.getY());

	if (targetTrack)
	{
        auto showedTracks = getShowedTracks(); 
		return showedTracks.indexOf(targetTrack) - showedTracks.indexOf(sourceTrack);
	}

	return 0;
}

void SongEditorView::resizeSelectedClips(bool snap, bool fromLeftEdge)
{
    EngineHelpers::resizeSelectedClips(fromLeftEdge, m_draggedTimeDelta, m_editViewState);
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
        GUIHelpers::centerMidiEditorToClip(m_editViewState, mc);

        return mc;
    }
    return nullptr;

}

void SongEditorView::updateClipCache()
{
    clearSelectedTimeRange();
    m_cachedSelectedAutomation.clear();
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

    for (auto t: getShowedTracks())
    {
        auto trackVRange = getVerticalRangeOfTrack(t, false);
        if (trackVRange.intersects (lassoRangeY))
            m_selectedRange.selectedTracks.add(t);
    }

    for (auto& ap : te::getAllAutomatableParameter(m_editViewState.m_edit))
    {
        if (GUIHelpers::isAutomationVisible(*ap))
        {
            auto rect = getAutomationRect(ap);
            juce::Range<int> vRange = {rect.getY(), rect.getBottom()};
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
    m_selectedRange.timeRange = {tracktion::TimePosition::fromSeconds(getSnapedTime(start.inSeconds(), snapDownAtStart)),
                                tracktion::TimePosition::fromSeconds(getSnapedTime(end.inSeconds(), snapDownAtEnd))};
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
        te::splitClips({m_hoveredClip}, tracktion::TimePosition::fromSeconds(xtoTime(x)));
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
            m_editViewState.removeThumbnail(wac);
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
            m_editViewState.removeThumbnail(wac);
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
    sm.deselectAll();

    clearSelectedTimeRange();

    for (auto t : getShowedTracks())
    {
        auto trackHeight = getTrackHeight(t, m_editViewState, false);

        if (!t->state.getProperty(IDs::isTrackMinimized) && trackHeight > 0)
        {
            for (auto ap : t->getAllAutomatableParams())
            {
                auto range = m_lassoComponent.getLassoRect().m_timeRange;
                auto firstPoint = ap->getCurve().nextIndexAfter(range.getStart());
                auto lastPoint = ap->getCurve().indexBefore(range.getEnd());
                for (auto i = firstPoint ; i <= lastPoint; i++)
                {
                    auto p = ap->getCurve().getPoint(i);
                    auto pointPos = getPointOnAutomationRect(p.time, p.value, ap, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
                    pointPos.addXY(0, getYForAutomatableParam(ap));

                    if (m_lassoComponent.getLassoRect().m_verticalRange.contains(pointPos.getY()))
                       selectAutomationPoint(ap, i, true);
                }
            }
        }
    }

   if (add)
       for (auto p : m_cachedSelectedAutomation)
           m_editViewState.m_selectionManager.addToSelection(p);
}

void SongEditorView::updateAutomationCache()
{
    m_cachedSelectedAutomation.clear();
    m_cachedSelectedClips.clear();

    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        m_cachedSelectedAutomation.add(p);
}

void SongEditorView::updateClipSelection(bool add)
{
    m_editViewState.m_selectionManager.deselectAll ();

    for (auto t : getShowedTracks())
    {
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
    auto trackHeight = getTrackHeight(track, m_editViewState, withAutomation);

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

double SongEditorView::xtoTime(int x)
{
    return m_editViewState.xToTime(x, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}

double SongEditorView::getSnapedTime(double time, bool downwards)
{
    return m_editViewState.getSnapedTime(
            time,
			m_editViewState.getBestSnapType(
                    m_editViewState.m_viewX1, m_editViewState.m_viewX2, getWidth()),
			downwards);
}

int SongEditorView::timeToX (double time)
{
    return m_editViewState.timeToX (time, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}

double SongEditorView::xToSnapedBeat (int x)
{
    auto time = xtoTime(x);
    time = getSnapedTime(time);
    return m_editViewState.timeToBeat(time);
}


juce::Rectangle<int> SongEditorView::getClipRect (te::Clip::Ptr clip)
{
    int x = timeToX(clip->getPosition().getStart().inSeconds());
    int y = getYForTrack(clip->getClipTrack());
    int w = timeToX(clip->getPosition().getEnd().inSeconds()) - x;
    int h = getTrackHeight(clip->getClipTrack(), m_editViewState, false);

    juce::Rectangle<int> clipRect = {x,y,w,h};

    return clipRect;
}



//reimplemented from te::AutomatonCurve because we 
//need to return -1 if there is no point after this position 
int SongEditorView::nextIndexAfter (tracktion::TimePosition t,te::AutomatableParameter::Ptr ap) const
{
    auto num = ap->getCurve().getNumPoints();

    for (int i = 0; i < num; ++i)
        if (ap->getCurve().getPointTime (i) >= t)
            return i;

    return -1;
}

juce::Point<float> SongEditorView::getPointOnAutomation(te::AutomatableParameter::Ptr ap, int index, juce::Rectangle<int> drawRect, double startBeat, double endBeat)
{
    auto time = ap->getCurve().getPoint(index).time;
    auto value = ap->getCurve().getPoint(index).value;
    auto point = getPointOnAutomationRect(time, value, ap, drawRect.getWidth(), startBeat, endBeat).translated((float) drawRect.getX(),(float) drawRect.getY());

    return point;
}

juce::Point<float> SongEditorView::getPointOnAutomationRect (tracktion::TimePosition t, double v, te::AutomatableParameter::Ptr ap, int w, double x1b, double x2b) 
{
   return {static_cast<float>(m_editViewState.timeToX(t.inSeconds(), w, x1b, x2b))
           , static_cast<float>(getYPos(v, ap))}; 
}

juce::Point<float> SongEditorView::getCurveControlPoint(juce::Point<float> p1, juce::Point<float> p2, float curve)
{
    auto controlX = p1.x + ( (p2.x - p1.x) * (.5f + curve) ) ;
    auto controlY = p1.y + ( (p2.y - p1.y) * (.5f - curve) ) ;

    auto curveControlPoint = juce::Point<float>(controlX, controlY);

    return curveControlPoint;
}

int SongEditorView::getYPos (double value, te::AutomatableParameter::Ptr ap)
{
    double pixelRangeStart = getAutomationPointWidth(ap) * .5;
    double pixelRangeEnd = getAutomationRect(ap).getHeight() - (getAutomationPointWidth(ap) * .5);

    double valueRangeStart = ap->valueRange.start;
    double valueRangeEnd = ap->valueRange.end;

    return static_cast<int>(juce::jmap(value, valueRangeStart, valueRangeEnd, pixelRangeEnd, pixelRangeStart));
}

double SongEditorView::getValue (int y, te::AutomatableParameter::Ptr ap)
{
    double pixelRangeStart = getAutomationPointWidth(ap) * .5;
    double pixelRangeEnd = getAutomationRect(ap).getHeight() - (getAutomationPointWidth(ap)  * .5);

    double valueRangeStart = ap->valueRange.start;
    double valueRangeEnd = ap->valueRange.end;

    return juce::jmap(static_cast<double>(y), pixelRangeStart, pixelRangeEnd, valueRangeEnd, valueRangeStart);
}

int SongEditorView::getAutomationPointWidth (te::AutomatableParameter::Ptr ap)
{
    if (getAutomationRect(ap).getHeight() <= 50)
        return 4;

    return 8;
}

void SongEditorView::drawAutomationLane (juce::Graphics& g, tracktion::TimeRange drawRange, juce::Rectangle<int> drawRect, te::AutomatableParameter::Ptr ap, bool forDragging)
{
    g.saveState();
    g.reduceClipRegion(drawRect);
    double startBeat = m_editViewState.timeToBeat(drawRange.getStart().inSeconds());
    double endBeat = m_editViewState.timeToBeat(drawRange.getEnd().inSeconds());
    g.setColour(juce::Colour(0xff252525));
    g.fillRect(drawRect);

    GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, startBeat, endBeat, drawRect);

    juce::Path pointsPath;
    juce::Path selectedPointsPath;
    juce::Path hoveredPointPath;
    juce::Path curvePath;
    juce::Path hoveredCurvePath;
    juce::Path hoveredDotOnCurvePath;
    juce::Path fillPath;

    float startX =  static_cast<float>(drawRect.getX());
    float startY =  static_cast<float>(getYPos(ap->getCurve().getValueAt(drawRange.getStart()),ap) + drawRect.getY());
    float endX = drawRect.toFloat().getRight();
    float endY = static_cast<float>(getYPos(ap->getCurve().getValueAt(drawRange.getEnd()),ap) + drawRect.getY());

    auto pointBeforeDrawRange = ap->getCurve().indexBefore(drawRange.getStart());
    auto pointAfterDrawRange = nextIndexAfter(drawRange.getEnd(), ap);
    auto numPointsInDrawRange = ap->getCurve().countPointsInRegion(drawRange);
    auto numPoints = ap->getCurve().getNumPoints();

    auto ellipseRect = juce::Rectangle<int>();
    if (numPoints < 2)
    {
        curvePath.startNewSubPath({startX, startY});
        if (numPointsInDrawRange > 0)
        {
            ellipseRect = GUIHelpers::getSensibleArea(getPointOnAutomation(ap, 0,drawRect, startBeat, endBeat).toInt(), getAutomationPointWidth(ap));
            pointsPath.addEllipse(ellipseRect.toFloat());
            if (m_hoveredAutomationPoint == 0)
                hoveredPointPath.addEllipse(ellipseRect.toFloat());
            if (isAutomationPointSelected(ap, 0))
                selectedPointsPath.addEllipse(ellipseRect.toFloat());
        }
        curvePath.lineTo({endX, endY});
    }
    else
    {
        if (pointBeforeDrawRange == -1)
        {
            curvePath.startNewSubPath({startX, startY});
            pointBeforeDrawRange = 0;
        }
        else
        {
            curvePath.startNewSubPath(getPointOnAutomation(ap, pointBeforeDrawRange, drawRect, startBeat, endBeat));
        }

        if (pointAfterDrawRange == -1)
            pointAfterDrawRange = ap->getCurve().getNumPoints() -1;

        for (auto i = pointBeforeDrawRange; i <= pointAfterDrawRange; i++)
        {
            auto pointXY = getPointOnAutomation(ap, i, drawRect, startBeat, endBeat);
            
            auto curve = juce::jlimit(-.5f, .5f, ap->getCurve().getPoint(i - 1).curve);
            auto curveControlPoint = getCurveControlPoint(curvePath.getCurrentPosition(), pointXY, curve);

            if (m_hoveredCurve == i && m_hoveredAutamatableParam == ap)
            {
                hoveredCurvePath.startNewSubPath (curvePath.getCurrentPosition());
                hoveredCurvePath.quadraticTo (curveControlPoint , pointXY);
                if (!m_isDragging) 
                    hoveredDotOnCurvePath.addEllipse (m_hoveredRectOnAutomation.toFloat());
            }

            curvePath.quadraticTo(curveControlPoint, pointXY);

            ellipseRect = GUIHelpers::getSensibleArea(getPointOnAutomation(ap, i,drawRect, startBeat, endBeat).toInt(), getAutomationPointWidth(ap));
            pointsPath.addEllipse(ellipseRect.toFloat());

            if (m_hoveredAutomationPoint == i && m_hoveredAutamatableParam == ap)
                hoveredPointPath.addEllipse(ellipseRect.toFloat());
            if (isAutomationPointSelected(ap, i))
                selectedPointsPath.addEllipse(ellipseRect.toFloat());
        }

        if (ap->getCurve().getPoint(pointAfterDrawRange).time < drawRange.getEnd())
            curvePath.lineTo({endX, endY});
    }

    float currentX = curvePath.getCurrentPosition().getX();
    float currentY = curvePath.getCurrentPosition().getY();

    if (m_hoveredCurve == ap->getCurve().getNumPoints())
    {
        hoveredCurvePath.startNewSubPath (currentX, currentY);
        hoveredCurvePath.lineTo(endX, endY);
        if (!m_isDragging) 
            hoveredDotOnCurvePath.addEllipse (m_hoveredRectOnAutomation.toFloat());
    }

    fillPath = curvePath;
    fillPath.lineTo(drawRect.getBottomRight().toFloat());
    fillPath.lineTo(drawRect.getBottomLeft().toFloat());
    fillPath.closeSubPath();

    if (getHeight () <= 50)
        g.setColour(juce::Colour(0x10ffffff));
    else
        g.setColour (ap->getTrack ()->getColour ().withAlpha (0.2f));
    g.fillPath(fillPath);

    g.setColour(juce::Colour(0xff888888));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::white);
    if (m_isDragging)
        g.setColour(m_editViewState.m_applicationState.getPrimeColour());
    g.strokePath(hoveredCurvePath, juce::PathStrokeType(2.0f));

    g.fillPath (hoveredDotOnCurvePath);

    g.setColour(juce::Colour(0xff2b2b2b));
    g.fillPath(pointsPath);

    float lineThickness = 2.0;   

    g.setColour(juce::Colour(0xff888888));
    g.strokePath(pointsPath, juce::PathStrokeType(lineThickness));

    g.setColour(juce::Colours::white);
    g.strokePath(hoveredPointPath, juce::PathStrokeType(lineThickness));

    g.setColour(juce::Colours::red);
    g.strokePath(selectedPointsPath, juce::PathStrokeType(lineThickness));
    
    g.setColour(juce::Colour(0x60ffffff));
    g.drawLine(drawRect.getX(),drawRect.getBottom(), drawRect.getRight(), drawRect.getBottom());

    g.restoreState();
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
        for (auto in: track->edit.getAllInputDevices())
        {
            if (in->isRecordingActive()
                && track == *(in->getTargetTracks().getFirst()))
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

        auto recordingClip = std::make_unique<RecordingClipComponent>(track, m_editViewState);
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

void SongEditorView::updateTrackHeights(EditViewState& evs)
{
    m_trackInfos.clear();

    for (auto* track : te::getAllTracks(m_editViewState.m_edit))
    {
        if ((track != nullptr) && (EngineHelpers::isTrackShowable(track)))
        {
            TrackHeightInfo info;
            info.m_track = track;
            info.m_isMinimized = track->state.getProperty(IDs::isTrackMinimized, false);
            info.m_height = info.m_isMinimized || track->isFolderTrack()
                ? evs.m_trackHeightMinimized
                : static_cast<int>(track->state.getProperty(tracktion_engine::IDs::height, 50));

            int automationHeight = 0;
            if (!info.m_isMinimized)
            {
                for (auto apEditItems : track->getAllAutomatableEditItems())
                {
                    for (auto ap : apEditItems->getAutomatableParameters())
                    {
                        if (GUIHelpers::isAutomationVisible(*ap))
                        {
                            auto& curveState = ap->getCurve().state;
                            automationHeight += static_cast<int>(curveState.getProperty(tracktion_engine::IDs::height, 50));
                        }
                    }
                }
            }

            auto it = track;
            while (it->isPartOfSubmix())
            {
                it = it->getParentFolderTrack();
                if (it->state.getProperty(IDs::isTrackMinimized, false))
                {
                    info.m_height = 0;
                    break;
                }
            }

            info.m_automationHeight = automationHeight;
            m_trackInfos.add(info);
        }
    }
}

int SongEditorView::getTrackHeight(tracktion_engine::Track* track, EditViewState& evs, bool withAutomation)
{
    if (track == nullptr)
        return 0;

    for (const auto& info : m_trackInfos)
    {
        if (info.m_track == track)
        {
            if (!info.m_isMinimized && withAutomation)
            {
                return info.m_height + info.m_automationHeight;
            }
            else
            {
                return info.m_height;
            }
        }
    }

    return 0; 
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

        std::cout << "Automation Y: " << getYForAutomatableParam(m_hoveredAutamatableParam) << " Height: " << getHeightOfAutomation(m_hoveredAutamatableParam) << std::endl;
        GUIHelpers::log("Point       : ", m_hoveredAutomationPoint) ;
        GUIHelpers::log("Curve       : ", m_hoveredCurve);

    }
    repaint();
}
