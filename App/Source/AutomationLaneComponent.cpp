//
// Created by bara on 31.12.21.
//

#include "AutomationLaneComponent.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "SongEditorView.h"


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

            if (m_hoveredPoint == i)
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
void AutomationLaneComponent::mouseMove(const juce::MouseEvent &e)
{
    auto timeHovered = getTimePosFromX(e.x);
    auto valueOnMousePos = (float) getValueAt (e.y);
    auto valueAtHoveredTime = m_curve.getValueAt(timeHovered); 

    m_hoveredPoint = getIndexOfHoveredPoint (e);
    m_hoveredCurve = -1;

    if (m_hoveredPoint == -1)
    {
        m_hoveredRect = getRectFromPoint({e.x, getYPos(valueAtHoveredTime)}).expanded(2, 2);
        auto isCurveHovered = getHoveredRect(e).contains (e.x, getYPos(valueAtHoveredTime));
        
        if (isCurveHovered
         || ((isBeyondLastPoint (timeHovered, valueOnMousePos)) && isCurveHovered))
            m_hoveredCurve = m_curve.indexBefore(timeHovered) + 1;

        m_hoveredTime = timeHovered;

        if (m_hoveredCurve != -1 && e.mods.isCtrlDown ())
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        else
            setMouseCursor (juce::MouseCursor::NormalCursor);
    }
    repaint();
}

void AutomationLaneComponent::mouseDown(const juce::MouseEvent &e)
{
    bool leftMouseButton = e.mods.isLeftButtonDown();
    bool rightMouseButton = e.mods.isRightButtonDown();
    bool isCtrlDown = e.mods.isCtrlDown ();


    //no automation point clicked
    if (m_hoveredPoint == -1)
    {        
        m_selectionManager.deselectAll();
        if (m_hoveredCurve != -1 && leftMouseButton && !isCtrlDown)
        {
            auto newPoint = m_curve.addPoint(m_hoveredTime, m_curve.getValueAt (m_hoveredTime), 0.0);
            m_hoveredPoint = newPoint;
            m_hoveredCurve = -1;
            m_timeAtMousedown = m_curve.getPointTime (m_hoveredPoint);
            selectPoint(m_hoveredPoint, false);
            repaint();
        }
        else //reset curve steepness to 0
        if (m_hoveredCurve != -1 && rightMouseButton)
        {
            m_curve.setCurveValue(m_hoveredCurve - 1, 0.0);
            m_hoveredCurve = -1;
            repaint();
        }
        else //curve clicked / select both automation points
        if (m_hoveredCurve != -1 && leftMouseButton)
        {
            selectPoint(m_hoveredCurve, false);
            selectPoint(m_hoveredCurve - 1, true);
        }
        else //no curve clicked / start Lasso
        if (m_hoveredCurve == -1 && e.mods.isLeftButtonDown())
        {
            auto se = dynamic_cast<SongEditorView*>(this->getParentComponent()->getParentComponent());
            if (se)
            {
                GUIHelpers::log("start Lasso in AutomationLane");
                se->startLasso(e.getEventRelativeTo(se), true, e.mods.isAltDown());
            }
        }
    }//automation point clicked
    else
    {
        if (rightMouseButton && !isCtrlDown)
        {
            m_curve.removePoint(m_hoveredPoint);
            m_hoveredPoint = -1;
            repaint();
        }
        else
        {
            m_timeAtMousedown = m_curve.getPointTime (m_hoveredPoint);
        }

        if(leftMouseButton ||!rightMouseButton)
        {
            selectPoint(m_hoveredPoint, isCtrlDown);
        }
    }

    m_curveAtMousedown = m_curve.getPoint(m_hoveredCurve - 1).curve;

    auto t = m_curve.getPoint(m_hoveredPoint).time;
    auto v = m_curve.getPoint(m_hoveredPoint).value;

    m_hovedPointXY = getPointXY(t, v);

    m_selPointsAtMousedown.clear();
    for (auto p : m_selectionManager.getItemsOfType<AutomationPoint>())
    {
        auto cp = new CurvePoint(p->m_curve.getPointTime(p->index),
                                 p->m_curve.getOwnerParameter()->valueRange.convertTo0to1(p->m_curve.getPointValue(p->index)),
                                 p->index,
                                 *p->m_curve.getOwnerParameter());
        m_selPointsAtMousedown.add(cp);
    }
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent &e)
{
    //change curve steepness
    if (m_hoveredCurve != -1 && e.mods.isCtrlDown ())
    { 
        m_isDragging = true;

        auto valueP1 = m_curve.getPointValue(m_hoveredCurve - 1);
        auto valueP2 = m_curve.getPointValue(m_hoveredCurve);
        auto delta =  valueP1 < valueP2 ? e.getDistanceFromDragStartY() *  0.01 
                                    : e.getDistanceFromDragStartY() * -0.01;
        auto newSteep = juce::jlimit(-0.5f, 0.5f, (float) (m_curveAtMousedown + delta));

        m_curve.setCurveValue(m_hoveredCurve - 1, newSteep);
  
        repaint();
    }
           
    //move Point
    else if (m_hoveredPoint != -1 && e.mouseWasDraggedSinceMouseDown ())
    { 
        auto screenTime = tracktion::TimeDuration::fromSeconds(xToTime(0));
        auto draggedTime = tracktion::TimeDuration::fromSeconds(xToTime(e.getDistanceFromDragStartX()));

        //lock time
        if (e.mods.isShiftDown())
        {
            draggedTime = tracktion::TimeDuration::fromSeconds(0.0);
        }
        //snap
        else if (!e.mods.isCtrlDown())
        {
            auto oldPos = tracktion::TimePosition::fromSeconds(xToTime(m_hovedPointXY.getX()));
            draggedTime = getSnapedTime(oldPos + draggedTime, false) - oldPos;
        }

        for (auto p : m_selPointsAtMousedown)
        {
            auto newTime =  p->time + draggedTime - screenTime;

            auto newValue = p->value - getValue(getHeight() - e.getDistanceFromDragStartY());

            newValue = p->param.valueRange.convertFrom0to1(newValue);
            p->param.getCurve().movePoint(p->index, newTime, newValue, false);
        }

        getParentComponent()->getParentComponent()->repaint();
    }
    else
    {
        auto se = dynamic_cast<SongEditorView*>(this->getParentComponent()->getParentComponent());
        if (se)
        {
            se->updateLasso(e.getEventRelativeTo(se));
        }
    }

}


void AutomationLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    m_isDragging = false;
    m_hoveredPoint = -1;
    m_hoveredCurve = -1;
    m_draggedTime = tracktion::TimeDuration::fromSeconds(0.0);
    if (m_hoveredCurve == -1 && m_hoveredPoint == -1 && e.mods.isLeftButtonDown())
    {
        auto se = dynamic_cast<SongEditorView*>(this->getParentComponent()->getParentComponent());
        if (se)
        {
            se->stopLasso();
        }
    }
}
void AutomationLaneComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    m_hoveredPoint = -1;
    m_hoveredCurve = -1;
    repaint();
}
bool AutomationLaneComponent::keyPressed(const juce::KeyPress &e)
{
   return false;
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
    GUIHelpers::log("select Automation point in AutomationLane");
    if (index >= 0 && index < m_curve.getNumPoints())
        m_selectionManager.select(createSelectablePoint (index), add);
    GUIHelpers::log("selected Automation Points: ", m_selectionManager.getItemsOfType<AutomationPoint>().size());
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

void AutomationLaneComponent::setSelectedTimeRange(tracktion::TimeRange timerange, bool snap)
{
    auto s = timerange.getStart();
    auto e = timerange.getEnd();
    if (snap)
    {
        s = getSnapedTime(s, true);
        e = getSnapedTime(e, false);
    }

    m_curve.state.setProperty(IDs::selectedRangeStart,s.inSeconds() , &m_editViewState.m_edit.getUndoManager());
    m_curve.state.setProperty(IDs::selectedRangeEnd, e.inSeconds(), &m_editViewState.m_edit.getUndoManager());
}

tracktion::TimeRange AutomationLaneComponent::getSelectedTimeRange()
{
    auto start = tracktion::TimePosition::fromSeconds((double) m_curve.state.getProperty(IDs::selectedRangeStart));
    auto end = tracktion::TimePosition::fromSeconds((double) m_curve.state.getProperty(IDs::selectedRangeEnd));

    return {start, end};
}


void AutomationLaneComponent::clearSelectedTimeRange()
{
    setSelectedTimeRange(tracktion::TimeRange());
}

te::AutomationCurve &AutomationLaneComponent::getCurve() const
{
    return m_curve;
}

tracktion::TimePosition AutomationLaneComponent::getTimePosFromX(int x)
{
    auto time = m_editViewState.xToTime(x, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    return tracktion::TimePosition::fromSeconds(time);
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
double AutomationLaneComponent::getValue(int y)
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
