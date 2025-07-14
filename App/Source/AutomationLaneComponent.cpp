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
#include "SongEditorView.h"

AutomationLaneComponent::AutomationLaneComponent(EditViewState& evs, te::AutomatableParameter::Ptr parameter, juce::String timeLineID)
    : m_editViewState(evs), m_parameter(parameter), m_timeLineID(timeLineID)
{
    setInterceptsMouseClicks(false, false);
}

void AutomationLaneComponent::paint(juce::Graphics& g)
{
    drawAutomationLane(g, m_editViewState.getVisibleTimeRange(m_timeLineID, getWidth()), getLocalBounds().toFloat());

    m_needsRepaint = false;
}
void AutomationLaneComponent::drawAutomationLane (juce::Graphics& g, tracktion::TimeRange drawRange, juce::Rectangle<float> drawRect)
{
    if (m_parameter->getTrack() == nullptr || drawRect.getWidth() <= 0 || drawRect.getHeight() <= 0)
        return;
        
    // Early exit for very small lanes
    if (getHeight() < 5)
        return;
        
    g.saveState();
    g.reduceClipRegion(drawRect.toNearestIntEdges());
    
    double startBeat = m_editViewState.timeToBeat(drawRange.getStart().inSeconds());
    double endBeat = m_editViewState.timeToBeat(drawRange.getEnd().inSeconds());
    
    // Only draw background when visible
    if (drawRect.getHeight() > 2)
    {
        g.setColour(m_editViewState.m_applicationState.getTrackBackgroundColour());
        g.fillRect(drawRect);
        GUIHelpers::drawBarsAndBeatLines(g, m_editViewState, startBeat, endBeat, drawRect);
    }

    const auto& curve = m_parameter->getCurve();
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
    
    if (pointBeforeDrawRange == -1) pointBeforeDrawRange = 0;
    if (pointAfterDrawRange == -1) pointAfterDrawRange = numPoints - 1;
    
    // Only process visible points
    const int startIdx = juce::jmax(0, pointBeforeDrawRange - 1);
    const int endIdx = juce::jmin(numPoints - 1, pointAfterDrawRange + 1);
    
    if (numPoints == 1)
    {
        // Single point
        const auto& point = curve.getPoint(0);
        const float x = static_cast<float>(m_editViewState.timeToX(point.time.inSeconds(), drawRect.getWidth(), startBeat, endBeat));
        const float y = static_cast<float>(getYPos(point.value));
        
        curvePath.startNewSubPath(startX, y);
        curvePath.lineTo(endX, y);
        
        const juce::Rectangle<float> ellipseRect(x - halfPointWidth, y - halfPointWidth, pointWidth, pointWidth);
        pointsPath.addEllipse(ellipseRect);
        
        if (m_hoveredPoint == 0) hoveredPointPath.addEllipse(ellipseRect);
        if (isAutomationPointSelected(0)) selectedPointsPath.addEllipse(ellipseRect);
    }
    else
    {
        // Draw curve
        const auto& firstPoint = curve.getPoint(startIdx);
        float lastX = static_cast<float>(m_editViewState.timeToX(firstPoint.time.inSeconds(), drawRect.getWidth(), startBeat, endBeat));
        float lastY = static_cast<float>(getYPos(firstPoint.value));
        
        if (startIdx == 0 || firstPoint.time >= drawRange.getStart())
            curvePath.startNewSubPath(lastX, lastY);
        else
            curvePath.startNewSubPath(startX, static_cast<float>(getYPos(curve.getValueAt(drawRange.getStart()))));
        
        // Collect points
        for (int i = startIdx + 1; i <= endIdx; ++i)
        {
            const auto& point = curve.getPoint(i);
            const float x = static_cast<float>(m_editViewState.timeToX(point.time.inSeconds(), drawRect.getWidth(), startBeat, endBeat));
            const float y = static_cast<float>(getYPos(point.value));
            
            if (i > 0)
            {
                const auto& prevPoint = curve.getPoint(i - 1);
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
                
                if (m_hoveredPoint == i) hoveredPointPath.addEllipse(ellipseRect);
                if (isAutomationPointSelected(i)) selectedPointsPath.addEllipse(ellipseRect);
            }
        }
        
        // Extend to the end
        if (endIdx >= 0 && curve.getPoint(endIdx).time < drawRange.getEnd())
            curvePath.lineTo(endX, static_cast<float>(getYPos(curve.getValueAt(drawRange.getEnd()))));
    }
    
    // Fill only for larger lanes
    if (getHeight() > 30)
    {
        juce::Path fillPath = curvePath;
        fillPath.lineTo(drawRect.getBottomRight());
        fillPath.lineTo(drawRect.getBottomLeft());
        fillPath.closeSubPath();
        
        g.setColour(m_parameter->getTrack()->getColour().withAlpha(0.2f));
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
    if (m_hoveredCurve != -1 && !m_hoveredRect.isEmpty() && !m_isDragging)
    {
        // Calculate dot position on curve at mouse position
        float mouseX = m_hoveredRect.getCentreX();
        
        // Convert mouse X to time using the same transformation as timeToX
        double visibleRange = endBeat - startBeat;
        double mouseBeat = startBeat + ((mouseX - drawRect.getX()) / drawRect.getWidth()) * visibleRange;
        double mouseTime = m_editViewState.beatToTime(mouseBeat);
        
        // Get curve value at this time
        float curveValue = curve.getValueAt(tracktion::TimePosition::fromSeconds(mouseTime));
        float curveY = static_cast<float>(getYPos(curveValue));
        
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

//reimplemented from te::AutomatonCurve because we 
//need to return -1 if there is no point after this position 
int AutomationLaneComponent::nextIndexAfter (tracktion::TimePosition t,te::AutomatableParameter::Ptr ap) const
{
    auto num = ap->getCurve().getNumPoints();

    for (int i = 0; i < num; ++i)
        if (ap->getCurve().getPointTime (i) >= t)
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

juce::Point<float> AutomationLaneComponent::getPointOnAutomationRect (tracktion::TimePosition t, double v, int w, double x1b, double x2b) 
{
   return {static_cast<float>(m_editViewState.timeToX(t.inSeconds(), w, x1b, x2b))
           , static_cast<float>(getYPos(v))}; 
}

juce::Point<float> AutomationLaneComponent::getCurveControlPoint(juce::Point<float> p1, juce::Point<float> p2, float curve)
{
    auto controlX = p1.x + ( (p2.x - p1.x) * (.5f + curve) ) ;
    auto controlY = p1.y + ( (p2.y - p1.y) * (.5f - curve) ) ;

    auto curveControlPoint = juce::Point<float>(controlX, controlY);

    return curveControlPoint;
}

int AutomationLaneComponent::getYPos (double value)
{
    double pixelRangeStart = 0;//getAutomationPointWidth() * .5;
    double pixelRangeEnd = getHeight();// - (getAutomationPointWidth() * .5);

    double valueRangeStart = m_parameter->valueRange.start;
    double valueRangeEnd = m_parameter->valueRange.end;

    return static_cast<int>(juce::jmap(value, valueRangeStart, valueRangeEnd, pixelRangeEnd, pixelRangeStart));
}

double AutomationLaneComponent::getValue (int y)
{
    double pixelRangeStart = getAutomationPointWidth() * .5;
    double pixelRangeEnd = getHeight() - (getAutomationPointWidth()  * .5);

    double valueRangeStart = m_parameter->valueRange.start;
    double valueRangeEnd = m_parameter->valueRange.end;

    return juce::jmap(static_cast<double>(y), pixelRangeStart, pixelRangeEnd, valueRangeEnd, valueRangeStart);
}

int AutomationLaneComponent::getAutomationPointWidth ()
{
    if (getHeight() <= 50)
        return 4;

    return 8;
}
bool AutomationLaneComponent::isAutomationPointSelected(int index)
{
    for (auto p : m_editViewState.m_selectionManager.getItemsOfType<SelectableAutomationPoint>())
        if (p->m_curve.getOwnerParameter() == m_parameter->getCurve().getOwnerParameter() && p->index == index)
            return true;

    return false;
}

void AutomationLaneComponent::updateCurveCache(const tracktion::AutomationCurve& curve)
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

int AutomationLaneComponent::findPointUnderMouse(const juce::Rectangle<float>& area, 
                                                 double visibleStartBeat, double visibleEndBeat, 
                                                 int width) const
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
    
    for (const auto& point : m_curvePointCache)
    {
        if (point.time < visibleStartTime - 0.1 || point.time > visibleEndTime + 0.1)
            continue;
            
        // Quick bounds check before expensive coordinate transformation
        const float expectedX = static_cast<float>(m_editViewState.timeToX(point.time, width, visibleStartBeat, visibleEndBeat));
        if (expectedX < area.getX() - 20 || expectedX > area.getRight() + 20)
            continue;
            
        juce::Point<float> screenPos = const_cast<AutomationLaneComponent*>(this)->getPointOnAutomationRect(
            tracktion::TimePosition::fromSeconds(point.time), point.value, width, visibleStartBeat, visibleEndBeat);
        
        if (area.contains(screenPos))
            return point.index;
    }
    
    return -1;
}

juce::Array<AutomationLaneComponent::CachedCurvePoint> 
AutomationLaneComponent::getVisiblePoints(double visibleStartBeat, double visibleEndBeat) const
{
    juce::Array<CachedCurvePoint> visiblePoints;

    if (!m_curveValid)
        return visiblePoints;

    // Use minimal buffer zone for performance optimization
    const double buffer = (visibleEndBeat - visibleStartBeat) * 0.02;
    const double extendedStartTime = m_editViewState.beatToTime(visibleStartBeat - buffer);
    const double extendedEndTime = m_editViewState.beatToTime(visibleEndBeat + buffer);

    visiblePoints.ensureStorageAllocated(juce::jmin(m_curvePointCache.size(), 32));

    for (const auto& point : m_curvePointCache)
    {
        if (point.time >= extendedStartTime && point.time <= extendedEndTime)
            visiblePoints.add(point);
    }

    return visiblePoints;
}

bool AutomationLaneComponent::isCurveValid(const tracktion::AutomationCurve& curve) const
{
    return m_curveValid && 
           m_cachedCurvePointCount == curve.getNumPoints();
}

void AutomationLaneComponent::invalidateCurveCache()
{
    m_curveValid = false;
    // Cache invalidation based on point count instead of version tracking
}

