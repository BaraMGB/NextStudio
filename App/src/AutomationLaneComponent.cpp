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

#include "AutomationLaneComponent.h"
#include "AutomationGestureLimits.h"
#include <algorithm>
#include <vector>
#include <unordered_map>
#include "SongEditorView.h"
#include "ScopedSaveLock.h"
#include "TimeUtils.h"

AutomationLaneComponent::AutomationLaneComponent(EditViewState &evs, te::AutomatableParameter::Ptr parameter, juce::String timeLineID, SongEditorView &songEditor)
    : m_editViewState(evs),
      m_parameter(parameter),
      m_timeLineID(timeLineID),
      m_songEditor(songEditor)
{
    // Enable mouse interaction
    setInterceptsMouseClicks(true, true);
    m_editViewState.m_selectionManager.addChangeListener(this);
    m_parameter->getCurve().state.addListener(this);
}

AutomationLaneComponent::~AutomationLaneComponent()
{
    m_parameter->getCurve().state.removeListener(this);
    m_editViewState.m_selectionManager.removeChangeListener(this);
}

void AutomationLaneComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_editViewState.m_selectionManager)
    {
        const auto indices = indexSelectionChildren(m_parameter->getCurve().state);
        const auto& selection = m_editViewState.m_selectionManager.getSelectedObjects();
        const std::unordered_set<te::Selectable*> selected(selection.begin(), selection.end());
        for (int i = m_selectedAutomationPoints.size(); --i >= 0;)
        {
            auto *p = m_selectedAutomationPoints.getUnchecked(i).get();
            const auto found = indices.find(p->pointState);
            p->index = found == indices.end() ? -1 : found->second;
            if (p->index < 0 || (!selected.contains(p) && p->getReferenceCount() == 1))
                m_selectedAutomationPoints.remove(i);
        }
        repaint();
    }
}

void AutomationLaneComponent::handleAsyncUpdate()
{
    changeListenerCallback(&m_editViewState.m_selectionManager);
    invalidateCurveCache();
    updateCurveCache(m_parameter->getCurve());
    repaint();
}

void AutomationLaneComponent::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) { triggerAsyncUpdate(); }

void AutomationLaneComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) { triggerAsyncUpdate(); }

void AutomationLaneComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) { triggerAsyncUpdate(); }

void AutomationLaneComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int) { triggerAsyncUpdate(); }

void AutomationLaneComponent::paint(juce::Graphics &g)
{
    drawAutomationLane(g, m_editViewState.getVisibleTimeRange(m_timeLineID, getWidth()), getLocalBounds().toFloat());

    m_needsRepaint = false;
}

//==============================================================================
// Mouse Handling
//==============================================================================

void AutomationLaneComponent::mouseMove(const juce::MouseEvent &e)
{
    // Mouse event throttling
    if (!m_mouseThrottler.shouldProcess(e))
        return;

    // Safety check: if mouse button is down, we are dragging
    if (e.mods.isAnyMouseButtonDown())
        return;

    juce::Point<float> hoveredPointInLane = e.position.toFloat();
    const auto hoveredRectOnLane = GUIHelpers::getSensibleArea(hoveredPointInLane, getAutomationPointWidth() * 2);

    auto curve = m_parameter->getCurve();

    // Cache update logic
    int currentNumPoints = curve.getNumPoints();

    if (m_lastMouseMoveNumPoints != currentNumPoints || !isCurveValid(curve))
    {
        updateCurveCache(curve);
        m_lastMouseMoveNumPoints = currentNumPoints;
    }

    auto visibleRange = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth());
    auto x1 = visibleRange.getStart().inBeats();
    auto x2 = visibleRange.getEnd().inBeats();

    int hoveredPoint = findPointUnderMouse(hoveredRectOnLane, x1, x2, getWidth());
    int hoveredCurve = -1;

    auto mousePosTime = xtoTime(e.position.x);
    auto valueAtMouseTime = curve.getValueAt(mousePosTime);
    auto curvePointAtMouseTime = juce::Point<float>((float)e.x, (float)getYPos(valueAtMouseTime));

    if (hoveredRectOnLane.contains(curvePointAtMouseTime) && hoveredPoint == -1)
        hoveredCurve = curve.nextIndexAfter(mousePosTime);

    if (m_hoveredPoint != hoveredPoint || m_hoveredCurve != hoveredCurve || m_hoveredRect != hoveredRectOnLane)
    {
        m_hoveredPoint = hoveredPoint;
        m_hoveredCurve = hoveredCurve;
        m_hoveredRect = hoveredRectOnLane.reduced(2.f, 2.f);
        repaint();
    }
}

void AutomationLaneComponent::mouseExit(const juce::MouseEvent &e)
{
    m_hoveredPoint = -1;
    m_hoveredCurve = -1;
    repaint();

    // Reset throttler when mouse leaves
    m_mouseThrottler.reset();
}

void AutomationLaneComponent::mouseDown(const juce::MouseEvent &e)
{
    m_songEditor.beginSelectionInput();
    m_mouseInput.remember(e);
    ScopedSaveLock saveLock(m_editViewState);
    m_isDragging = false;
    m_timeGesture.reset();
    m_selPointsAtMousedown.clear();

    bool leftButton = e.mods.isLeftButtonDown();
    bool rightButton = e.mods.isRightButtonDown();
    bool clickedOnPoint = m_hoveredPoint != -1;
    bool clickedOnCurve = m_hoveredCurve != -1;
    m_isLassoInteraction = false;

    if (leftButton && m_songEditor.getToolMode() == Tool::range)
    {
        auto eventInSEV = e.getEventRelativeTo(&m_songEditor);
        m_songEditor.startTimeRangeSelection(eventInSEV);
        m_isLassoInteraction = true;
        return;
    }

    if (leftButton && m_songEditor.getToolMode() == Tool::lasso)
    {
        m_songEditor.startLasso(e.getEventRelativeTo(&m_songEditor), true);
        m_isLassoInteraction = true;
        return;
    }

    // Double Click on Empty Space -> Create Point
    if (!clickedOnPoint && !clickedOnCurve && leftButton && e.getNumberOfClicks() > 1)
    {
        auto mouseTime = xtoTime(e.position.x);
        auto value = getValue(e.y);

        auto p = m_parameter->getCurve().addPoint(mouseTime, value, 0.f);
        m_hoveredCurve = -1;
        m_hoveredPoint = p;
        selectAutomationPoint(p, false);

        // Prepare Drag
        m_selPointsAtMousedown = getSelectedPoints();

        // Ensure the newly created point is included in the drag set
        if (m_selPointsAtMousedown.isEmpty())
        {
            m_selPointsAtMousedown.add(new CurvePoint(mouseTime, value, p, m_parameter));
        }

        m_timeOfHoveredAutomationPoint = mouseTime;
        m_timeGesture.begin(m_songEditor.getMouseSnapResolver().timeToBeat(mouseTime.inSeconds()),
                            e.position.x, m_songEditor.getMouseSnapResolver());
        m_isDragging = true;

        updateCurveCache(m_parameter->getCurve());

        repaint();
        return;
    }

    if (clickedOnPoint && leftButton)
    {
        if (!isAutomationPointSelected(m_hoveredPoint))
            selectAutomationPoint(m_hoveredPoint, false);

        m_timeOfHoveredAutomationPoint = m_parameter->getCurve().getPointTime(m_hoveredPoint);
        m_selPointsAtMousedown = getSelectedPoints();
        m_timeGesture.begin(m_songEditor.getMouseSnapResolver().timeToBeat(m_timeOfHoveredAutomationPoint.inSeconds()),
                            e.position.x, m_songEditor.getMouseSnapResolver());
        return;
    }

    if (clickedOnPoint && rightButton)
    {
        removeAutomationPoint(m_hoveredPoint);
        m_selPointsAtMousedown = getSelectedPoints();
        return;
    }

    if (clickedOnCurve && leftButton)
    {
        if (e.mods.isCtrlDown())
        {
            // Ctrl+Click on curve to adjust curve steepness
            m_curveSteepAtMousedown = m_parameter->getCurve().getPointCurve(m_hoveredCurve - 1);
            m_isDragging = true;            // Enable dragging for curve steepness adjustment
            m_selPointsAtMousedown.clear(); // No points being dragged
        }
        else
        {
            auto mouseTime = xtoTime(e.position.x);
            addAutomationPointAt(mouseTime);

            m_selPointsAtMousedown = getSelectedPoints();

            // Ensure the newly created point is included in the drag set
            if (m_selPointsAtMousedown.isEmpty() && m_hoveredPoint != -1)
            {
                auto time = m_parameter->getCurve().getPointTime(m_hoveredPoint);
                auto value = m_parameter->getCurve().getPointValue(m_hoveredPoint);
                m_selPointsAtMousedown.add(new CurvePoint(time, value, m_hoveredPoint, m_parameter));
            }

            m_timeOfHoveredAutomationPoint = m_parameter->getCurve().getPointTime(m_hoveredPoint);
            m_timeGesture.begin(m_songEditor.getMouseSnapResolver().timeToBeat(m_timeOfHoveredAutomationPoint.inSeconds()),
                                e.position.x, m_songEditor.getMouseSnapResolver());
        }
        return;
    }

    if (clickedOnCurve && rightButton)
    {
        m_parameter->getCurve().setCurveValue(m_hoveredCurve - 1, 0.0f);
        repaint();
        return;
    }

    // Lasso delegation (if clicking empty space single click)
    if (!clickedOnPoint && !clickedOnCurve && leftButton)
    {
        // Call startLasso on SongEditorView directly, indicating it starts from automation
        // We need to convert the event to SongEditorView coordinates
        auto eventInSEV = e.getEventRelativeTo(&m_songEditor);
        m_songEditor.startLasso(eventInSEV, true);
        m_isLassoInteraction = true;
    }
}

void AutomationLaneComponent::refreshMouseSnapContext()
{
    if (auto event = m_mouseInput.forContext(*this, juce::ModifierKeys::getCurrentModifiers()); event && event->mouseWasDraggedSinceMouseDown())
    {
        juce::ScopedValueSetter<bool> replay(m_refreshingSnapContext, true);
        mouseDrag(*event);
    }
}

void AutomationLaneComponent::modifierKeysChanged(const juce::ModifierKeys& mods)
{
    m_songEditor.modifierKeysChanged(mods);
    if (m_timeGesture.active())
        if (auto event = m_mouseInput.withModifiers(mods); event && event->mouseWasDraggedSinceMouseDown())
            mouseDrag(*event);
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent &e)
{
    if (!m_mouseInput.belongsToGesture(e)) return;
    m_mouseInput.remember(e);
    if (m_isLassoInteraction)
    {
        const auto event = e.getEventRelativeTo(&m_songEditor);
        if (m_songEditor.isSelectingTimeRange()) m_songEditor.updateTimeRangeSelection(event);
        else m_songEditor.updateLasso(event);
        return;
    }


    // Check for curve steepness change FIRST (before regular point dragging)
    // This handles Ctrl+Drag on curve segments
    auto isChangingCurveSteepness = m_hoveredPoint == -1 && m_hoveredCurve != -1 && e.mods.isCtrlDown();
    if (isChangingCurveSteepness)
    {
        m_isDragging = true;

        auto valueP1 = m_parameter->getCurve().getPointValue(m_hoveredCurve - 1);
        auto valueP2 = m_parameter->getCurve().getPointValue(m_hoveredCurve);
        auto delta = valueP1 < valueP2 ? e.getDistanceFromDragStartY() * 0.01 : e.getDistanceFromDragStartY() * -0.01;
        auto newSteep = juce::jlimit(-0.5f, 0.5f, (float)(m_curveSteepAtMousedown + delta));

        m_parameter->getCurve().setCurveValue(m_hoveredCurve - 1, newSteep);
        repaint();
        return; // Important: return here so we don't fall through to other logic
    }

    // Regular automation point dragging
    auto isDraggingAutomationPoint = (m_hoveredPoint != -1) || (m_isDragging && !m_selPointsAtMousedown.isEmpty());
    if (isDraggingAutomationPoint)
    {
        m_isDragging = true;
        auto lockTime = e.mods.isCtrlDown();

        const auto oldPos = m_timeOfHoveredAutomationPoint;
        const auto resolver = m_songEditor.getMouseSnapResolver();
        const auto newBeat = m_timeGesture.update(e.position.x, resolver, e.mods.isShiftDown());
        juce::Array<AutomationGestureLimits::Point> points;
        std::vector<CurvePoint*> ordered;
        for (auto* p : m_selPointsAtMousedown)
            if (p != nullptr && p->param != nullptr && juce::isPositiveAndBelow(p->index, p->param->getCurve().getNumPoints()))
            {
                points.add({p->param.get(), p->index, p->time});
                ordered.push_back(p);
            }
        const double seconds = AutomationGestureLimits::constrain(points, lockTime ? 0.0 : resolver.beatToTime(newBeat) - oldPos.inSeconds());
        const auto draggedTime = tracktion::TimeDuration::fromSeconds(seconds);
        m_timeGesture.setDisplayedBeat(resolver.timeToBeat((oldPos + draggedTime).inSeconds()));
        // Move outward points first, otherwise Tracktion clamps each point to
        // its still-unmoved selected neighbour and collapses group spacing.
        const double currentDelta = ordered.empty() ? 0.0 : (ordered.front()->param->getCurve().getPointTime(ordered.front()->index) - ordered.front()->time).inSeconds();
        std::sort(ordered.begin(), ordered.end(), [seconds, currentDelta](auto* a, auto* b)
        { return seconds > currentDelta ? a->time > b->time : a->time < b->time; });
        for (auto *p : ordered)
        {
            if (p == nullptr || p->param == nullptr)
                continue;
            auto &param = *p->param;
            float height = static_cast<float>(m_editViewState.m_trackHeightManager->getAutomationHeight(&param));
            int pointWidth = (height <= 50) ? 4 : 8;
            double pixelRangeStart = pointWidth * .5;
            double pixelRangeEnd = height - (pointWidth * .5);
            double valueRangeStart = param.valueRange.start;
            double valueRangeEnd = param.valueRange.end;

            // Calculate start Y for this point
            double startY = juce::jmap(p->value, valueRangeStart, valueRangeEnd, pixelRangeEnd, pixelRangeStart);
            double newY = startY + e.getDistanceFromDragStartY();

            double newValue = m_refreshingSnapContext ? param.getCurve().getPointValue(p->index)
                : juce::jmap(newY, pixelRangeStart, pixelRangeEnd, valueRangeEnd, valueRangeStart);

            auto newTime = p->time + draggedTime;

            auto newIndex = param.getCurve().movePoint(p->index, newTime, newValue, false);
            p->index = newIndex;
        }
        const auto rect = m_songEditor.getLocalArea(this, getLocalBounds()).toFloat();
        float markerY = rect.getY() + float(e.y);
        for (const auto* point : ordered)
            if (point->param == m_parameter && point->time == oldPos)
            {
                const auto& curve = m_parameter->getCurve();
                m_timeGesture.setDisplayedBeat(resolver.timeToBeat(curve.getPointTime(point->index).inSeconds()));
                const float margin = rect.getHeight() <= 50 ? 2.0f : 4.0f;
                markerY = rect.getY() + juce::jmap(float(curve.getPointValue(point->index)),
                    float(m_parameter->valueRange.start), float(m_parameter->valueRange.end), rect.getHeight() - margin, margin);
                break;
            }
        if (lockTime)
            m_songEditor.clearMouseFeedback();
        else
            m_songEditor.setMouseFeedback(m_timeGesture.feedback(), rect.getVerticalRange(), markerY,
                [safe = juce::Component::SafePointer<AutomationLaneComponent>(this)]
                { if (safe) safe->refreshMouseSnapContext(); });
        repaint();
    }
    else
    {
        // Lasso drag
        auto eventInSEV = e.getEventRelativeTo(&m_songEditor);
        m_songEditor.updateLasso(eventInSEV);
    }
}

void AutomationLaneComponent::mouseUp(const juce::MouseEvent &e)
{
    if (!m_mouseInput.belongsToGesture(e)) return;
    if (m_songEditor.takeCancelledSelection(e))
    {
        m_isLassoInteraction = false;
        m_mouseInput.reset();
        return;
    }
    if ((m_timeGesture.active() || m_isLassoInteraction) && e.mouseWasDraggedSinceMouseDown())
        mouseDrag(e);
    m_isDragging = false;
    m_timeGesture.reset();
    m_mouseInput.reset();
    m_songEditor.clearMouseFeedback();
    m_selPointsAtMousedown.clear();

    if (m_isLassoInteraction)
    {
        if (m_songEditor.isSelectingTimeRange()) m_songEditor.stopTimeRangeSelection();
        else m_songEditor.stopLasso();
        m_songEditor.repaint();
        m_isLassoInteraction = false;
    }

    repaint();
}

juce::Array<juce::ValueTree> AutomationLaneComponent::findPointsInLasso(juce::Rectangle<float> rect)
{
    juce::Array<juce::ValueTree> hits;
    if (!isShowing()) return hits;
    const auto beats = m_editViewState.getVisibleBeatRange(m_timeLineID, getWidth());
    const auto& curve = m_parameter->getCurve();
    for (int i = 0; i < curve.getNumPoints(); ++i)
    {
        const auto point = curve.getPoint(i);
        const auto local = getPointOnAutomationRect(point.time, point.value, getWidth(), beats.getStart().inBeats(), beats.getEnd().inBeats());
        const auto position = m_songEditor.getLocalPoint(this, local);
        if (LassoGesture::containsCentre(rect, position)) hits.add(curve.state.getChild(i));
    }
    return hits;
}

void AutomationLaneComponent::appendLassoSelection(const SelectionTreeSet& wanted, te::SelectableList& selected)
{
    const auto& curve = m_parameter->getCurve();
    std::unordered_map<juce::ValueTree, SelectableAutomationPoint*, SelectionIdentityHash<juce::ValueTree>> proxies;
    proxies.reserve(size_t(m_selectedAutomationPoints.size()));
    for (auto* candidate : m_selectedAutomationPoints) proxies.emplace(candidate->pointState, candidate);
    for (int i = 0; i < curve.getNumPoints(); ++i)
    {
        const auto state = curve.state.getChild(i);
        if (!wanted.contains(state)) continue;
        const auto found = proxies.find(state);
        auto* proxy = found == proxies.end() ? nullptr : found->second;
        if (proxy == nullptr)
        {
            proxy = new SelectableAutomationPoint(i, m_parameter->getCurve());
            m_selectedAutomationPoints.add(proxy);
        }
        proxy->index = i;
        selected.add(proxy);
    }
}

//==============================================================================
// Helpers Implementation
//==============================================================================

float AutomationLaneComponent::timeToX(tracktion::TimePosition time) { return TimeUtils::timeToX(time, m_editViewState, m_timeLineID, getWidth()); }

tracktion::TimePosition AutomationLaneComponent::xtoTime(float x) { return TimeUtils::xToTime(x, m_editViewState, m_timeLineID, getWidth()); }


void AutomationLaneComponent::addAutomationPointAt(tracktion::TimePosition pos)
{
    auto valueAtTime = m_parameter->getCurve().getValueAt(pos);
    auto p = m_parameter->getCurve().addPoint(pos, valueAtTime, 0.f);
    m_hoveredCurve = -1;
    m_hoveredPoint = p;
    selectAutomationPoint(p, false);
    m_isDragging = true;
    updateCurveCache(m_parameter->getCurve());
}

void AutomationLaneComponent::removeAutomationPoint(int index)
{
    m_parameter->getCurve().removePoint(index);
    m_editViewState.m_selectionManager.deselectAll();
    invalidateCurveCache();
}

void AutomationLaneComponent::selectAutomationPoint(int index, bool add)
{
    if (index >= 0 && index < m_parameter->getCurve().getNumPoints())
    {
        te::SelectableList points;
        appendLassoSelection({m_parameter->getCurve().state.getChild(index)}, points);
        if (!points.isEmpty()) m_editViewState.m_selectionManager.select(points.getFirst(), add);
    }
}

void AutomationLaneComponent::deselectAutomationPoint(int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == m_parameter->getCurve().getOwnerParameter()
            && p->pointState == m_parameter->getCurve().state.getChild(index))
            p->deselect();
}

juce::OwnedArray<AutomationLaneComponent::CurvePoint> AutomationLaneComponent::getSelectedPoints()
{
    juce::OwnedArray<CurvePoint> points;
    std::unordered_map<te::AutomationCurve*, SelectionTreeIndex> curves;
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
    {
        auto [curve, inserted] = curves.try_emplace(&p->m_curve);
        if (inserted) curve->second = indexSelectionChildren(p->m_curve.state);
        const auto found = curve->second.find(p->pointState);
        p->index = found == curve->second.end() ? -1 : found->second;
        if (p->index < 0) continue;
        auto cp = std::make_unique<CurvePoint>(p->m_curve.getPointTime(p->index), p->m_curve.getPointValue(p->index), p->index, p->m_curve.getOwnerParameter());

        points.add(std::move(cp));
    }
    return points;
}

// ... Keep existing drawing/caching methods ...

void AutomationLaneComponent::drawAutomationLane(juce::Graphics &g, tracktion::TimeRange drawRange, juce::Rectangle<float> drawRect, tracktion::TimeDuration previewDelta)
{
    if (drawRect.getWidth() <= 0 || drawRect.getHeight() <= 0)
        return;

    auto automationColour = m_editViewState.m_applicationState.getPrimeColour();
    if (auto *track = m_parameter->getTrack())
        automationColour = track->getColour();
    else if (auto *masterTrack = m_parameter->getEdit().getMasterTrack())
        automationColour = masterTrack->getColour();

    // Early exit for very small lanes
    if (getHeight() < 5)
        return;

    g.saveState();
    g.reduceClipRegion(drawRect.toNearestIntEdges());

    double startBeat = m_editViewState.timeToBeat((drawRange.getStart() + previewDelta).inSeconds());
    double endBeat = m_editViewState.timeToBeat((drawRange.getEnd() + previewDelta).inSeconds());
    const auto pointX = [&](tracktion::TimePosition t)
    {
        return drawRect.getX() + m_editViewState.timeToX((t + previewDelta).inSeconds(), drawRect.getWidth(), startBeat, endBeat);
    };
    const auto pointY = [&](double value) { return drawRect.getY() + static_cast<float>(getYPos(value)); };

    // Only draw background when visible
    if (drawRect.getHeight() > 2)
    {
        g.setColour(m_editViewState.m_applicationState.getTrackBackgroundColour());
        g.fillRect(drawRect);
        GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, startBeat, endBeat, drawRect);
    }

    const auto &curve = m_parameter->getCurve();
    const int numPoints = curve.getNumPoints();

    // Early exit when no points exist
    if (numPoints == 0)
    {
        g.restoreState();
        return;
    }

    // Create rectangles and paths only once
    juce::Path curvePath;
    juce::Path pointsPath;
    juce::Path selectedPointsPath;
    juce::Path hoveredPointPath;
    juce::Path hoveredCurvePath;
    juce::Path hoveredDotOnCurvePath;

    const float startX = drawRect.getX();
    const float endX = drawRect.getRight();
    const float pointWidth = getAutomationPointWidth();
    const float halfPointWidth = pointWidth * 0.5f;

    // Cache for visible points
    juce::Array<int> visiblePointIndices;
    visiblePointIndices.ensureStorageAllocated(juce::jmin(numPoints, 100));

    auto pointBeforeDrawRange = curve.indexBefore(drawRange.getStart());
    auto pointAfterDrawRange = nextIndexAfter(drawRange.getEnd(), m_parameter);

    if (pointBeforeDrawRange == -1)
        pointBeforeDrawRange = 0;
    if (pointAfterDrawRange == -1)
        pointAfterDrawRange = numPoints - 1;

    // Only process visible points
    const int startIdx = juce::jmax(0, pointBeforeDrawRange - 1);
    const int endIdx = juce::jmin(numPoints - 1, pointAfterDrawRange + 1);

    if (numPoints == 1)
    {
        // Single point
        const auto &point = curve.getPoint(0);
        const float x = pointX(point.time);
        const float y = pointY(point.value);

        curvePath.startNewSubPath(startX, y);
        curvePath.lineTo(endX, y);

        const juce::Rectangle<float> ellipseRect(x - halfPointWidth, y - halfPointWidth, pointWidth, pointWidth);
        pointsPath.addEllipse(ellipseRect);

        if (m_hoveredPoint == 0)
            hoveredPointPath.addEllipse(ellipseRect);
        if (isAutomationPointSelected(0))
            selectedPointsPath.addEllipse(ellipseRect);
    }
    else
    {
        // Draw curve
        const auto &firstPoint = curve.getPoint(startIdx);
        float lastX = pointX(firstPoint.time);
        float lastY = pointY(firstPoint.value);

        if (startIdx == 0 || firstPoint.time >= drawRange.getStart())
            curvePath.startNewSubPath(lastX, lastY);
        else
            curvePath.startNewSubPath(startX, pointY(curve.getValueAt(drawRange.getStart())));

        // Collect points
        for (int i = startIdx + 1; i <= endIdx; ++i)
        {
            const auto &point = curve.getPoint(i);
            const float x = pointX(point.time);
            const float y = pointY(point.value);

            if (i > 0)
            {
                const auto &prevPoint = curve.getPoint(i - 1);
                const float curveValue = juce::jlimit(-0.5f, 0.5f, prevPoint.curve);

                const float cpX = lastX + (x - lastX) * (0.5f + curveValue);
                const float cpY = lastY + (y - lastY) * (0.5f - curveValue);

                if (m_hoveredCurve == i)
                {
                    hoveredCurvePath.startNewSubPath(lastX, lastY);
                    hoveredCurvePath.quadraticTo(cpX, cpY, x, y);
                }

                curvePath.quadraticTo(cpX, cpY, x, y);
            }
            else
            {
                curvePath.lineTo(x, y);
            }

            lastX = x;
            lastY = y;

            // Only draw point if visible
            if (x >= startX - pointWidth && x <= endX + pointWidth)
            {
                const juce::Rectangle<float> ellipseRect(x - halfPointWidth, y - halfPointWidth, pointWidth, pointWidth);
                pointsPath.addEllipse(ellipseRect);

                if (m_hoveredPoint == i)
                    hoveredPointPath.addEllipse(ellipseRect);
                if (isAutomationPointSelected(i))
                    selectedPointsPath.addEllipse(ellipseRect);
            }
        }

        // Extend to the end
        if (endIdx >= 0 && curve.getPoint(endIdx).time < drawRange.getEnd())
            curvePath.lineTo(endX, pointY(curve.getValueAt(drawRange.getEnd())));
    }

    // Fill only for larger lanes
    if (getHeight() > 30)
    {
        juce::Path fillPath = curvePath;
        fillPath.lineTo(drawRect.getBottomRight());
        fillPath.lineTo(drawRect.getBottomLeft());
        fillPath.closeSubPath();

        g.setColour(automationColour.withAlpha(0.2f));
        g.fillPath(fillPath);
    }

    // Draw curve
    g.setColour(m_editViewState.m_applicationState.getTimeLineStrokeColour());
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));

    // Hover curve and dot
    if (!hoveredCurvePath.isEmpty())
    {
        g.setColour(m_editViewState.m_applicationState.getPrimeColour());
        if (m_isDragging)
            g.setColour(m_editViewState.m_applicationState.getPrimeColour().withLightness(1.0f));
        g.strokePath(hoveredCurvePath, juce::PathStrokeType(2.0f));
    }

    // Draw hover dot on curve (only when hovering over curve segment)
    if (m_hoveredCurve != -1 && !m_hoveredRect.isEmpty() && !m_isDragging && previewDelta == tracktion::TimeDuration())
    {
        // Calculate dot position on curve at mouse position
        float mouseX = drawRect.getX() + m_hoveredRect.getCentreX();

        // Convert mouse X to time using the same transformation as timeToX
        double visibleRange = endBeat - startBeat;
        double mouseBeat = startBeat + ((mouseX - drawRect.getX()) / drawRect.getWidth()) * visibleRange;
        double mouseTime = m_editViewState.beatToTime(mouseBeat);

        // Get curve value at this time
        float curveValue = curve.getValueAt(tracktion::TimePosition::fromSeconds(mouseTime));
        float curveY = pointY(curveValue);

        // Draw the hover dot
        g.setColour(m_editViewState.m_applicationState.getPrimeColour().withLightness(1.0f));
        g.fillEllipse(mouseX - 3.0f, curveY - 3.0f, 6.0f, 6.0f);
    }

    // Draw points
    const float lineThickness = 2.0f;

    if (!pointsPath.isEmpty())
    {
        g.setColour(m_editViewState.m_applicationState.getTrackBackgroundColour());
        g.fillPath(pointsPath);
        g.setColour(m_editViewState.m_applicationState.getTimeLineStrokeColour());
        g.strokePath(pointsPath, juce::PathStrokeType(lineThickness));
    }

    if (!hoveredPointPath.isEmpty())
    {
        g.setColour(m_editViewState.m_applicationState.getTimeLineStrokeColour().withLightness(1.0f));
        g.strokePath(hoveredPointPath, juce::PathStrokeType(lineThickness));
    }

    if (!selectedPointsPath.isEmpty())
    {
        g.setColour(m_editViewState.m_applicationState.getPrimeColour());
        g.strokePath(selectedPointsPath, juce::PathStrokeType(lineThickness));
    }

    g.restoreState();
}

// reimplemented from te::AutomatonCurve because we
// need to return -1 if there is no point after this position
int AutomationLaneComponent::nextIndexAfter(tracktion::TimePosition t, te::AutomatableParameter::Ptr ap) const
{
    auto num = ap->getCurve().getNumPoints();

    for (int i = 0; i < num; ++i)
        if (ap->getCurve().getPointTime(i) >= t)
            return i;

    return -1;
}

juce::Point<float> AutomationLaneComponent::getPointOnAutomation(int index, juce::Rectangle<float> drawRect, double startBeat, double endBeat)
{
    auto time = m_parameter->getCurve().getPoint(index).time;
    auto value = m_parameter->getCurve().getPoint(index).value;
    auto point = getPointOnAutomationRect(time, value, drawRect.getWidth(), startBeat, endBeat).translated(drawRect.getX(), drawRect.getY());

    return point;
}

juce::Point<float> AutomationLaneComponent::getPointOnAutomationRect(tracktion::TimePosition t, double v, int w, double x1b, double x2b) { return {static_cast<float>(m_editViewState.timeToX(t.inSeconds(), w, x1b, x2b)), static_cast<float>(getYPos(v))}; }

juce::Point<float> AutomationLaneComponent::getCurveControlPoint(juce::Point<float> p1, juce::Point<float> p2, float curve)
{
    auto controlX = p1.x + ((p2.x - p1.x) * (.5f + curve));
    auto controlY = p1.y + ((p2.y - p1.y) * (.5f - curve));

    auto curveControlPoint = juce::Point<float>(controlX, controlY);

    return curveControlPoint;
}

int AutomationLaneComponent::getYPos(double value)
{
    double pixelRangeStart = 0;         // getAutomationPointWidth() * .5;
    double pixelRangeEnd = getHeight(); // - (getAutomationPointWidth() * .5);

    double valueRangeStart = m_parameter->valueRange.start;
    double valueRangeEnd = m_parameter->valueRange.end;

    return static_cast<int>(juce::jmap(value, valueRangeStart, valueRangeEnd, pixelRangeEnd, pixelRangeStart));
}

double AutomationLaneComponent::getValue(int y)
{
    double pixelRangeStart = getAutomationPointWidth() * .5;
    double pixelRangeEnd = getHeight() - (getAutomationPointWidth() * .5);

    double valueRangeStart = m_parameter->valueRange.start;
    double valueRangeEnd = m_parameter->valueRange.end;

    return juce::jmap(static_cast<double>(y), pixelRangeStart, pixelRangeEnd, valueRangeEnd, valueRangeStart);
}

int AutomationLaneComponent::getAutomationPointWidth()
{
    if (getHeight() <= 50)
        return 4;

    return 8;
}
bool AutomationLaneComponent::isAutomationPointSelected(int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == m_parameter->getCurve().getOwnerParameter()
            && p->pointState == m_parameter->getCurve().state.getChild(index))
            return true;

    return false;
}

void AutomationLaneComponent::updateCurveCache(const tracktion::AutomationCurve &curve)
{
    const int numPoints = curve.getNumPoints();

    // Check if anything actually changed
    if (m_curveValid && m_cachedCurvePointCount == numPoints)
        return;

    m_curvePointCache.clear();

    if (numPoints == 0)
    {
        m_cachedCurvePointCount = 0;
        // Cache invalidation based on point count instead of version tracking
        m_curveValid = true;
        return;
    }

    m_curvePointCache.ensureStorageAllocated(numPoints);

    for (int i = 0; i < numPoints; i++)
    {
        auto point = curve.getPoint(i);

        CachedCurvePoint cachedPoint;
        cachedPoint.index = i;
        cachedPoint.time = point.time.inSeconds();
        cachedPoint.value = point.value;

        m_curvePointCache.add(cachedPoint);
    }

    m_cachedCurvePointCount = numPoints;
    // Cache invalidation based on point count instead of version tracking
    m_curveValid = true;
}

int AutomationLaneComponent::findPointUnderMouse(const juce::Rectangle<float> &area, double visibleStartBeat, double visibleEndBeat, int width) const
{
    if (!m_curveValid || width <= 0)
        return -1;

    // Bounding box check for early performance optimization
    const double visibleStartTime = m_editViewState.beatToTime(visibleStartBeat);
    const double visibleEndTime = m_editViewState.beatToTime(visibleEndBeat);

    // Cache screen positions for faster hit-testing
    // Pre-allocate array to avoid reallocations
    juce::Array<juce::Point<float>> cachedPositions;
    cachedPositions.ensureStorageAllocated(64);

    for (const auto &point : m_curvePointCache)
    {
        if (point.time < visibleStartTime - 0.1 || point.time > visibleEndTime + 0.1)
            continue;

        // Quick bounds check before expensive coordinate transformation
        const float expectedX = static_cast<float>(m_editViewState.timeToX(point.time, width, visibleStartBeat, visibleEndBeat));
        if (expectedX < area.getX() - 20 || expectedX > area.getRight() + 20)
            continue;

        juce::Point<float> screenPos = const_cast<AutomationLaneComponent *>(this)->getPointOnAutomationRect(tracktion::TimePosition::fromSeconds(point.time), point.value, width, visibleStartBeat, visibleEndBeat);

        if (area.contains(screenPos))
            return point.index;
    }

    return -1;
}

juce::Array<AutomationLaneComponent::CachedCurvePoint> AutomationLaneComponent::getVisiblePoints(double visibleStartBeat, double visibleEndBeat) const
{
    juce::Array<CachedCurvePoint> visiblePoints;

    if (!m_curveValid)
        return visiblePoints;

    // Use minimal buffer zone for performance optimization
    const double buffer = (visibleEndBeat - visibleStartBeat) * 0.02;
    const double extendedStartTime = m_editViewState.beatToTime(visibleStartBeat - buffer);
    const double extendedEndTime = m_editViewState.beatToTime(visibleEndBeat + buffer);

    visiblePoints.ensureStorageAllocated(juce::jmin(m_curvePointCache.size(), 32));

    for (const auto &point : m_curvePointCache)
    {
        if (point.time >= extendedStartTime && point.time <= extendedEndTime)
            visiblePoints.add(point);
    }

    return visiblePoints;
}

bool AutomationLaneComponent::isCurveValid(const tracktion::AutomationCurve &curve) const { return m_curveValid && m_cachedCurvePointCount == curve.getNumPoints(); }

void AutomationLaneComponent::invalidateCurveCache()
{
    m_curveValid = false;
    // Cache invalidation based on point count instead of version tracking
}
