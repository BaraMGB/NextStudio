/*
 * Copyright 2025 Steffen Baranowsky
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

#include "AutomationLaneComponent.h"
#include "SongEditorView.h"

AutomationLaneComponent::AutomationLaneComponent(EditViewState& evs, te::AutomatableParameter::Ptr parameter, juce::String timeLineID)
    : m_editViewState(evs), m_parameter(parameter), m_timeLineID(timeLineID)
{
    setInterceptsMouseClicks(false, false);
}

void AutomationLaneComponent::paint(juce::Graphics& g)
{
    drawAutomationLane(g, m_editViewState.getVisibleTimeRange(m_timeLineID, getWidth()), getLocalBounds().toFloat(), m_parameter);

    m_needsRepaint = false;
}
void AutomationLaneComponent::drawAutomationLane (juce::Graphics& g, tracktion::TimeRange drawRange, juce::Rectangle<float> drawRect, te::AutomatableParameter::Ptr ap)
{
    g.saveState();
    g.reduceClipRegion(drawRect.toNearestIntEdges());
    double startBeat = m_editViewState. timeToBeat(drawRange.getStart().inSeconds());
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

    float startX = drawRect.getX();
    float endX = drawRect.getRight();

    float startValue = ap->getCurve().getValueAt(drawRange.getStart());
    float endValue = ap->getCurve().getValueAt(drawRange.getEnd());

    float startY = getYPos(startValue);
    float endY = getYPos(endValue);

    auto pointBeforeDrawRange = ap->getCurve().indexBefore(drawRange.getStart());
    auto pointAfterDrawRange = nextIndexAfter(drawRange.getEnd(), ap);
    auto numPointsInDrawRange = ap->getCurve().countPointsInRegion(drawRange);
    auto numPoints = ap->getCurve().getNumPoints();

    auto ellipseRect = juce::Rectangle<float>();
    if (numPoints < 2)
    {
        curvePath.startNewSubPath({startX, startY});
        if (numPointsInDrawRange > 0)
        {
            ellipseRect = GUIHelpers::getSensibleArea(getPointOnAutomation(0,drawRect, startBeat, endBeat), getAutomationPointWidth());
            pointsPath.addEllipse(ellipseRect.toFloat());
            if (m_hoveredPoint == 0)
                hoveredPointPath.addEllipse(ellipseRect.toFloat());
            if (isAutomationPointSelected(0))
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
            curvePath.startNewSubPath(getPointOnAutomation(pointBeforeDrawRange, drawRect, startBeat, endBeat));
        }

        if (pointAfterDrawRange == -1)
            pointAfterDrawRange = ap->getCurve().getNumPoints() -1;

        for (auto i = pointBeforeDrawRange; i <= pointAfterDrawRange; i++)
        {
            auto pointXY = getPointOnAutomation(i, drawRect, startBeat, endBeat);

            auto curve = juce::jlimit(-.5f, .5f, ap->getCurve().getPoint(i - 1).curve);
            auto curveControlPoint = getCurveControlPoint(curvePath.getCurrentPosition(), pointXY, curve);

            if (m_hoveredCurve == i)
            {
                hoveredCurvePath.startNewSubPath (curvePath.getCurrentPosition());
                hoveredCurvePath.quadraticTo (curveControlPoint , pointXY);
                if (!m_isDragging) 
                    hoveredDotOnCurvePath.addEllipse (m_hoveredRect.toFloat());
            }

            curvePath.quadraticTo(curveControlPoint, pointXY);

            ellipseRect = GUIHelpers::getSensibleArea(getPointOnAutomation(i,drawRect, startBeat, endBeat), getAutomationPointWidth());
            pointsPath.addEllipse(ellipseRect.toFloat());

            if (m_hoveredPoint == i)
                hoveredPointPath.addEllipse(ellipseRect.toFloat());
            if (isAutomationPointSelected(i))
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
            hoveredDotOnCurvePath.addEllipse (m_hoveredRect.toFloat());
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
    double pixelRangeStart = getAutomationPointWidth() * .5;
    double pixelRangeEnd = getHeight() - (getAutomationPointWidth() * .5);

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
    m_curvePointCache.clear();

    m_curvePointCache.ensureStorageAllocated(curve.getNumPoints());

    for (int i = 0; i < curve.getNumPoints(); i++)
    {
        auto point = curve.getPoint(i);

        CachedCurvePoint cachedPoint;
        cachedPoint.index = i;
        cachedPoint.time = point.time.inSeconds();
        cachedPoint.value = point.value;

        m_curvePointCache.add(cachedPoint);
    }

    m_cachedCurvePointCount = curve.getNumPoints();
    m_curveValid = true;
}

int AutomationLaneComponent::findPointUnderMouse(const juce::Rectangle<float>& area, 
                                                 double visibleStartBeat, double visibleEndBeat, 
                                                 int width) const
{
    if (!m_curveValid)
        return -1;

    double visibleStartTime = m_editViewState.beatToTime(visibleStartBeat);
    double visibleEndTime = m_editViewState.beatToTime(visibleEndBeat);

    for (const auto& point : m_curvePointCache)
    {
        if (point.time >= visibleStartTime && point.time <= visibleEndTime)
        {
            juce::Point<float> screenPos = point.getScreenPosition(
                const_cast<AutomationLaneComponent*>(this), width, visibleStartBeat, visibleEndBeat);

            if (area.contains(screenPos))
            {
                return point.index;
            }
        }
    }

    return -1;
}

juce::Array<AutomationLaneComponent::CachedCurvePoint> 
AutomationLaneComponent::getVisiblePoints(double visibleStartBeat, double visibleEndBeat) const
{
    juce::Array<CachedCurvePoint> visiblePoints;

    if (!m_curveValid)
        return visiblePoints;

    const double buffer = (visibleEndBeat - visibleStartBeat) * 0.05;
    double extendedStartBeat = visibleStartBeat - buffer;
    double extendedEndBeat = visibleEndBeat + buffer;

    double extendedStartTime = m_editViewState.beatToTime(extendedStartBeat);
    double extendedEndTime = m_editViewState.beatToTime(extendedEndBeat);

    for (const auto& point : m_curvePointCache)
    {
        if (point.time >= extendedStartTime && point.time <= extendedEndTime)
        {
            visiblePoints.add(point);
        }
    }

    return visiblePoints;
}

bool AutomationLaneComponent::isCurveValid(const tracktion::AutomationCurve& curve) const
{
    return m_curveValid && m_cachedCurvePointCount == curve.getNumPoints();
}

void AutomationLaneComponent::invalidateCurveCache()
{
    m_curveValid = false;
}

