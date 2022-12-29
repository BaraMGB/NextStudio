//
// Created by bara on 31.12.21.
//

#include "AutomationLaneComponent.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "SongEditorView.h"
#include "tracktion_core/utilities/tracktion_Time.h"


AutomationLaneComponent::AutomationLaneComponent(tracktion_engine::AutomationCurve& curve, EditViewState &evs)
    : m_curve(curve)
    , m_editViewState(evs)
    , m_rangeAtMouseDown({tracktion::core::TimePosition::fromSeconds(0.0), tracktion::core::TimePosition::fromSeconds(0.0)})
{

    setName(m_curve.getOwnerParameter()->getFullName());
    //setWantsKeyboardFocus(true);
}
void AutomationLaneComponent::paint(juce::Graphics &g)
{
    paintCurves(g, getCurveTimeRangeToDraw());

    g.setColour(juce::Colour(0x60ffffff));
    g.drawLine(0, 0, getWidth(), 0);

}
void AutomationLaneComponent::paintOverChildren(juce::Graphics &g)
{
    if (getSelectedTimeRange() != tracktion::TimeRange())
    {
        auto s = getXPos(getSelectedTimeRange().getStart().inSeconds());
        auto e = getXPos(getSelectedTimeRange().getEnd().inSeconds());
    
        auto sc = juce::Colours::white;
        g.setColour(sc);
        g.drawVerticalLine(s, 0, getHeight());
        g.drawVerticalLine(e, 0, getHeight());
        g.setColour(sc.withAlpha(0.5f));
        g.fillRect(s,0,e-s,getHeight());
    }

}
void AutomationLaneComponent::paintCurves(juce::Graphics &g, tracktion::TimeRange drawRange)
{
    auto oldPoint = getPointXY(drawRange.getStart(), m_curve.getValueAt(drawRange.getStart()));
    float length = getXPos(drawRange.getEnd().inSeconds()) - oldPoint.x;
    float lineThickness = 2.0;

    juce::Path curvePath;
    juce::Path hoveredCurvePath;
    juce::Path dotsPath;
    juce::Path selectedPath;
    juce::Path hoveredDotPath;
    juce::Path hoveredDotOnCurvePath;

    auto hoveredRect = m_hoveredRect.reduced(lineThickness, lineThickness).toFloat ();
    curvePath.startNewSubPath(oldPoint.toFloat());
    for (auto i = 0; i < m_curve.getNumPoints(); i++)
    {
        if (m_curve.getPoint(i).time >= drawRange.getStart()
         && m_curve.getPoint(i).time <= drawRange.getEnd())
        {
            juce::Point<int> pointPos = getPointXY(m_curve.getPoint(i).time, m_curve.getPoint(i).value);
   
            auto curve = juce::jlimit(-.5f, .5f, m_curve.getPoint(i - 1).curve);
            auto x = oldPoint.x + ( (pointPos.x - oldPoint.x) * (.5f + curve) );
            auto y = oldPoint.y + ( (pointPos.y - oldPoint.y) * (.5f - curve) );
            auto curveControlPoint = juce::Point<float>(x, y);

            curvePath.quadraticTo(curveControlPoint, {pointPos.toFloat().x, pointPos.toFloat().y});
    
            if (m_hoveredCurve == i)
            {
                hoveredCurvePath.startNewSubPath (oldPoint.toFloat());
                hoveredCurvePath.quadraticTo (curveControlPoint , pointPos.toFloat());
                if (!m_isDragging) 
                    hoveredDotOnCurvePath.addEllipse (m_hoveredRect.reduced(1, 1).toFloat());
            }
    
            oldPoint = pointPos;

            juce::Rectangle<float> ellipse = getRectFromPoint(pointPos.toInt());

            if (m_hoveredAutomationPoint == i)
            {
                hoveredDotPath.addEllipse(ellipse);
            }

            if (isPointSelected(i))
            {
                selectedPath.addEllipse(ellipse);
            }


            dotsPath.addEllipse(ellipse);
        }
    }

    if (m_hoveredCurve > m_curve.getNumPoints () - 1)
    {
        hoveredDotOnCurvePath.addEllipse (m_hoveredRect.reduced(1, 1).toFloat());
    }

    //close the path
    curvePath.lineTo((float) length + 1.f, oldPoint.y);
    curvePath.lineTo((float) length + 1.f, (float) getHeight() + 1.f);
    curvePath.lineTo(-1.f, (float) getHeight() + 1.f);
    curvePath.lineTo (-1 , (float) getYPos(m_curve.getValueAt(tracktion::core::TimePosition::fromSeconds(0.0))));
    curvePath.closeSubPath();

    if (getHeight () <= 50)
        g.setColour(juce::Colour(0x10ffffff));
    else
        g.setColour (m_curve.getOwnerParameter ()->getTrack ()->getColour ().withAlpha (0.2f));
    g.fillPath(curvePath);
    g.setColour(juce::Colour(0xff888888));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f));
    g.setColour(juce::Colours::white);
    if (m_isDragging)
        g.setColour(m_editViewState.m_applicationState.getPrimeColour());
    g.strokePath(hoveredCurvePath, juce::PathStrokeType(2.0f));
    g.fillPath (hoveredDotOnCurvePath);
    g.setColour(juce::Colour(0xff2b2b2b));
    g.fillPath(dotsPath);
    g.setColour(juce::Colour(0xff888888));
    g.strokePath(dotsPath, juce::PathStrokeType(lineThickness));
    g.setColour(juce::Colours::white);
    g.strokePath(hoveredDotPath, juce::PathStrokeType(lineThickness));
    g.setColour(juce::Colours::red);
    g.strokePath(selectedPath, juce::PathStrokeType(lineThickness));
}
tracktion::TimeRange AutomationLaneComponent::getCurveTimeRangeToDraw()
{
    auto lastPointTime = m_curve.getPoint(m_curve.getNumPoints() - 1).time;
    auto screenEndTime = tracktion::TimePosition::fromSeconds(m_editViewState.beatToTime(m_editViewState.m_viewX2));
    auto lastTime = juce::jmax(lastPointTime, screenEndTime);

    return {tracktion::TimePosition::fromSeconds(0.0), lastTime};
}

double AutomationLaneComponent::getValueAt(int x)
{
    auto pos = getTimePosFromX(x);
    return m_curve.getValueAt(pos);
}
tracktion::TimePosition AutomationLaneComponent::getSnapedTime(tracktion::TimePosition time, bool down=false)
{
    auto snapType = m_editViewState.getBestSnapType (
                       m_editViewState.m_viewX1
                     , m_editViewState.m_viewX2
                     , getWidth());
    auto snapedTime = m_editViewState.getSnapedTime(time.inSeconds(), snapType, down);

    return tracktion::TimePosition::fromSeconds(snapedTime);
}
int AutomationLaneComponent::getIndexOfHoveredPoint(const juce::MouseEvent &e)
{
    int p = -1;

    for (auto i = 0; i < m_curve.getNumPoints(); i++)
        if (getHoveredRect(e).contains (getPoint (m_curve.getPoint (i))))
            p = i;
        
    return p;
}

juce::Rectangle<int> AutomationLaneComponent::getHoveredRect(const juce::MouseEvent &e)
{
     return getRectFromPoint({e.x, e.y}).toNearestInt();  
}

juce::Rectangle<float> AutomationLaneComponent::getRectFromPoint(juce::Point<int> p)
{
    juce::Rectangle<float> rect = { p.getX() - static_cast<float> (getPointWidth ())/2
                                        , p.getY() - static_cast<float>(getPointWidth ())/2
                                        , static_cast<float>(getPointWidth ())
                                        , static_cast<float>(getPointWidth ()) };

    return rect;  
}



void AutomationLaneComponent::selectPoint(int index, bool add)
{
    if (index >= 0 && index < m_curve.getNumPoints())
        m_selectionManager.select(createSelectablePoint (index), add);
}

AutomationPoint* AutomationLaneComponent::createSelectablePoint(int index)
{
    auto ap = new AutomationPoint(index, m_curve);
    return ap;
}

bool AutomationLaneComponent::isPointSelected(int index)
{
    for (auto p : m_selectionManager.getItemsOfType<AutomationPoint>())
        if (p->m_curve.getOwnerParameter() == m_curve.getOwnerParameter() && p->index == index)
            return true;

    return false;
}

void AutomationLaneComponent::deselectPoint(int index)
{
    for (auto p : m_selectionManager.getItemsOfType<AutomationPoint>())
        if (p->m_curve.getOwnerParameter() == m_curve.getOwnerParameter() && p->index == index)
            p->deselect();
}
tracktion::TimeRange AutomationLaneComponent::getSelectedTimeRange()
{
    if (auto tc = dynamic_cast<TrackComponent*>(getParentComponent()))
       return tc->getSelectedTimeRange();

    return tracktion::TimeRange();
}

te::AutomationCurve &AutomationLaneComponent::getCurve() const
{
    return m_curve;
}

tracktion::TimePosition AutomationLaneComponent::getTimePosFromX(int x)
{
    return tracktion::TimePosition::fromSeconds(xToTime(x));
}


double AutomationLaneComponent::xToTime(const int x)
{
    return m_editViewState.xToTime (x, getWidth (), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}
juce::Point<int> AutomationLaneComponent::getPointXY (tracktion::TimePosition t, double v)
{
    return {getXPos(t.inSeconds()), getYPos(v)};
}
int AutomationLaneComponent::getXPos(double time)
{
    if (getWidth() == 0)
        return 0;
    return m_editViewState.timeToX(time, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}
double AutomationLaneComponent::getValue(double y)
{
    double pwh = (double) getPointWidth() / 2;
    double lh = getLaneHeight();

    return 1.0 - juce::jmap((double)y, pwh, lh - pwh, 0.0, 1.0);
}
int AutomationLaneComponent::getYPos(double value)
{
    double lh = getLaneHeight();
    double pwh = (double) getPointWidth() / 2;

    double v = m_curve.getOwnerParameter()->valueRange.convertTo0to1(value);
    return getHeight() - juce::jmap(v, 0.0, 1.0, pwh, lh - pwh);
}
juce::Point<int> AutomationLaneComponent::getPoint(const tracktion_engine::AutomationCurve::AutomationPoint &ap)
{
    return {getXPos (ap.time.inSeconds()), getYPos (ap.value)};
}
double AutomationLaneComponent::xToYRatio()
{
    //1 screen unit in value / 1 screen unit in time
    double screenUnitInValue = 1.0/(getLaneHeight() - getPointWidth());
    double screenUnitInTime =
        (m_editViewState.beatToTime(m_editViewState.m_viewX2)
         - m_editViewState.beatToTime(m_editViewState.m_viewX1))
        / getWidth();
    return screenUnitInValue / screenUnitInTime;
}
int AutomationLaneComponent::getLaneHeight()
{
    return getHeight() ;
}
bool AutomationLaneComponent::isBeyondLastPoint(tracktion::TimePosition time, float value)
{
    auto tp = time;
    auto nearestPoint = m_curve.getNearestPoint (tp, value , xToYRatio ());
    if (nearestPoint > m_curve.getNumPoints () - 1)
    {
        return true;
    }
    return false;
}
int AutomationLaneComponent::getPointWidth()
{
    if (getHeight () <= 50)
        return 4;
    return 8;
}
