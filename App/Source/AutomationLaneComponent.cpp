//
// Created by bara on 31.12.21.
//

#include "AutomationLaneComponent.h"

AutomationLaneComponent::AutomationLaneComponent(tracktion_engine::AutomationCurve& curve, EditViewState &evs)
    : m_curve(curve)
    , m_editViewState(evs)
    , m_rangeAtMouseDown({tracktion::core::TimePosition::fromSeconds(0.0), tracktion::core::TimePosition::fromSeconds(0.0)})
{
    setWantsKeyboardFocus(true);
}
void AutomationLaneComponent::paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::white);

    auto selCol = m_editViewState.m_applicationState.getPrimeColour();
    g.setColour(selCol.withAlpha(0.3f));
    auto range = getSelectedRange();

    
    g.fillRect(juce::Rectangle<int>{ getXPos(range.getStart().inSeconds()), 0, getXPos(range.getEnd().inSeconds()) - getXPos(range.getStart().inSeconds()), getHeight() });

    auto start = 0.0;
    auto end = juce::jmax(m_curve.getPoint(m_curve.getNumPoints() -1 ).time.inSeconds() ,m_editViewState.beatToTime(m_editViewState.m_viewX2));
    paintCurves(g, start , end);

    g.setColour(juce::Colour(0x66ffffff));
    g.drawHorizontalLine(0,0,getWidth());
}
void AutomationLaneComponent::paintOverChildren(juce::Graphics &g)
{
    if (m_moveSelection && m_rangeAtMouseDown.getStart().inSeconds() > 0)
    {
        auto screenStart = m_editViewState.beatToTime(m_editViewState.m_viewX1);
        auto newStart = m_rangeAtMouseDown.getStart().inSeconds() + m_draggedTime - screenStart;
        if (m_snap)
            newStart = getSnapedTime(newStart);

        auto mrange = m_rangeAtMouseDown.movedToStartAt(tracktion::core::TimePosition::fromSeconds(newStart));
        auto x = getXPos(mrange.getStart().inSeconds());
        auto w =  getXPos(mrange.getEnd().inSeconds() - getXPos(mrange.getStart().inSeconds()));
        if (w > 0)
        {
            auto targetRect = juce::Rectangle<int>{x, 0, w, getHeight()};
            g.setColour(m_editViewState.m_applicationState.getBackgroundColour());
            g.fillRect(targetRect);
            g.drawImageAt(m_rangeImage, x, 0, false);
        }
    }
}
void AutomationLaneComponent::paintCurves(juce::Graphics &g, double start, double end)
{
    auto s = tracktion::core::TimePosition::fromSeconds(start);
    auto e = tracktion::core::TimePosition::fromSeconds(end);

    float oldX = getXPos(start);
    float oldY = (float) getYPos(m_curve.getValueAt(tracktion::core::TimePosition::fromSeconds(getTime(oldX))));
    float length = getXPos(end) - oldX;
    float lineThickness = 2.0;
    juce::Path curvePath;
    juce::Path hoveredCurve;
    juce::Path dots;
    juce::Path hoveredDot;
    juce::Path hoveredDotOnCurve;
    auto hoveredRect = m_hoveredRect.reduced(lineThickness, lineThickness).toFloat ();
    curvePath.startNewSubPath(oldX, oldY);
    for (auto i = 0; i < m_curve.getNumPoints(); i++)
    {
        if (m_curve.getPoint(i).time >= s
                && m_curve.getPoint(i).time <= e)
        {
            float x = (float) getXPos(m_curve.getPoint(i).time.inSeconds());
            float y = (float) getYPos(m_curve.getPoint(i).value);
            float curve = m_curve.getPoint(i - 1).curve;
    
            auto curveControlPoint = juce::Point<float>(
                                oldX + ((x - oldX) * (0.5f + curve)),
                                oldY + ((y - oldY) * (0.5f - curve)));
            curvePath.quadraticTo(curveControlPoint, {x, y});
    
            if (m_hoveredCurve == i)
            {
                hoveredCurve.startNewSubPath (oldX, oldY);
                hoveredCurve.quadraticTo (curveControlPoint, {x,y});
    
                hoveredDotOnCurve.addEllipse (hoveredRect);
            }
            oldX = x; oldY = y;
    
            float w = getPointWidth() - (lineThickness * 2);
            juce::Rectangle<float> ellipse = {x - w/2
                                      , y - w/2 
                                      , w 
                                      , w};
            if (m_hoveredPoint == i)
            {
                hoveredDot.addEllipse(ellipse);
            }
            dots.addEllipse(ellipse);
        }
    }

    if (m_hoveredCurve > m_curve.getNumPoints () - 1)
    {
        hoveredDotOnCurve.addEllipse (hoveredRect);
    }

    //close the path
    curvePath.lineTo((float) length + 1.f, oldY);
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
    g.strokePath(hoveredCurve, juce::PathStrokeType(2.0f));
    g.fillPath (hoveredDotOnCurve);
    g.setColour(juce::Colour(0xff2b2b2b));
    g.fillPath(dots);
    g.setColour(juce::Colour(0xff888888));
    g.strokePath(dots, juce::PathStrokeType(lineThickness));
    g.setColour(juce::Colours::white);
    g.strokePath(hoveredDot, juce::PathStrokeType(lineThickness));
}
void AutomationLaneComponent::mouseDown(const juce::MouseEvent &e)
{
    m_rangeAtMouseDown = getSelectedRange();
    if (!e.mods.isCtrlDown())
        m_editViewState.m_selectionManager.selectOnly (m_curve.getOwnerParameter ()->getTrack ());

    if (m_hoveredPoint == -1)
    {
        bool isTimePositive = m_hoveredTime > 0;
        bool leftMouseButton = e.mods.isLeftButtonDown() && !e.mods.isCtrlDown ();

        if (isTimePositive && leftMouseButton && !m_isVertical)
        {
            auto newPoint = m_curve.addPoint(
                EngineHelpers::getTimePos(m_hoveredTime)
                , m_curve.getValueAt (EngineHelpers::getTimePos(m_hoveredTime))
                    , 0.0);
            m_hoveredPoint = newPoint;
            m_hoveredCurve = -1;
            m_hoveredTime = 0;
            repaint();
        }

        if (m_hoveredCurve == -1 && e.mods.isLeftButtonDown())
        {
            auto range = juce::Range(m_curve.state.getProperty(IDs::selectedRangeStart, 0),
                                     m_curve.state.getProperty(IDs::selectedRangeEnd,0));

            if (m_hoveredPoint == -1)
            {
                if (range.contains(getTime(e.x)))
                    m_moveSelection = true;
                else
                    m_selectingTime = true;
            }
        }
           
        if (m_hoveredCurve != -1 && e.mods.isRightButtonDown())
        {
            m_curve.setCurveValue(m_hoveredCurve - 1, 0.0);
            repaint();
        }
    }
    else
    {
        if (e.mods.isRightButtonDown() && !e.mods.isCtrlDown())
        {
            m_curve.removePoint(m_hoveredPoint);
            m_hoveredPoint = -1;
            repaint();
        }
        else
        {
            m_timeAtMousedown = m_curve.getPointTime (m_hoveredPoint).inSeconds();
        }
    }
    m_curveAtMousedown = m_curve.getPoint(m_hoveredCurve - 1).curve;
    auto x = getXPos(m_curve.getPoint(m_hoveredPoint).time.inSeconds());
    auto y = getYPos(m_curve.getPoint(m_hoveredPoint).value);

    m_hovedPointXY = {x, y};

}
void AutomationLaneComponent::mouseMove(const juce::MouseEvent &e)
{
    auto timeHovered = getTime(e.x);
    auto valueHovered = (float) getValue (e.y);


    m_hoveredPoint = getIndexOfHoveredPoint (e);
    m_selectingTime = false;
    m_moveSelection = false;
    m_hoveredCurve = -1;
    if (m_hoveredPoint == -1)
    {
        auto ht = tracktion::core::TimePosition::fromSeconds(timeHovered);
        int yPosAtHoveredTime = getYPos(m_curve.getValueAt(ht));
        m_hoveredRect = {e.x - getPointWidth ()/2
                         , yPosAtHoveredTime - getPointWidth ()/2
                         , getPointWidth ()
                             , getPointWidth ()};
        auto nearestPoint = m_curve.getNearestPoint (ht, valueHovered , xToYRatio ());

        m_isVertical = m_curve.getPoint (nearestPoint).time == ht;

        if (m_hoveredRect.contains (e.x, e.y)
            || m_isVertical
            || ((isBeyondLastPoint (timeHovered, valueHovered)) && m_hoveredRect.contains (e.x, e.y)))
        {
            m_hoveredCurve = nearestPoint;
            m_hoveredTime = timeHovered;
        }
        else
        {
           m_hoveredTime = 0;
            m_hoveredRect = {0,0,0,0};
        }
    }
    if (m_hoveredCurve != -1 && e.mods.isCtrlDown ())
    {
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        m_hoveredRect = {0,0,0,0};
    }
    else{
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }

    repaint();
}
void AutomationLaneComponent::mouseDrag(const juce::MouseEvent &e)
{
    //change curve
    bool setCurveSteep = e.mods.isCtrlDown ();
    m_snap = !e.mods.isShiftDown();
    if (m_hoveredCurve != -1 && setCurveSteep)
    {
        m_hoveredRect = {0,0,0,0};
        auto factor = m_curve.getPointValue(m_hoveredCurve - 1)
                              < m_curve.getPointValue(m_hoveredCurve)
                          ? 0.01
                          : -0.01;
        m_curve.setCurveValue(
            m_hoveredCurve - 1
            , juce::jlimit(-0.5f
                         , 0.5f
                         , (float) (m_curveAtMousedown
                                  + e.getDistanceFromDragStartY() * factor)));
        repaint();
    }
    //move Point
    if (m_hoveredPoint != -1 && e.mouseWasDraggedSinceMouseDown ())
    { 
        auto snapedTime = getSnapedTime(getTime(e.x));
        bool forceValue = e.mods.isCtrlDown();
        bool snap = !e.mods.isShiftDown();
        auto draggedTime = getTime(m_hovedPointXY.getX() + e.getDistanceFromDragStartX());

        auto newTime =  forceValue
                       ? m_timeAtMousedown
                       : snap
                             ? snapedTime
                             : draggedTime;

        auto newValue = (float) getValue(m_hovedPointXY.getY()+ e.getDistanceFromDragStartY());

        double v = m_curve.getOwnerParameter()->valueRange.convertFrom0to1(newValue);
        m_curve.movePoint(m_hoveredPoint,EngineHelpers::getTimePos(newTime), v, false);

        repaint();
    }

    if (m_selectingTime)
    {
        auto screenStart = m_editViewState.beatToTime(m_editViewState.m_viewX1);

        auto clickTime = getTime(e.getMouseDownX());
        auto dragedTime = getTime(e.getDistanceFromDragStartX());

        auto s = juce::jmin(clickTime, clickTime + dragedTime);
        auto end = s + std::abs(dragedTime) - screenStart;

        s = juce::jmax(0.0, s);
        end = juce::jmax(0.0, end);

        if (!e.mods.isShiftDown())
        {
            s = getSnapedTime(s);
            end = getSnapedTime(end);
        }

        m_curve.state.setProperty(IDs::selectedRangeStart , s, nullptr);
        m_curve.state.setProperty(IDs::selectedRangeEnd , end, nullptr);
        repaint();
    }
    else if (m_moveSelection)
    {
         m_draggedTime = getTime(e.getDistanceFromDragStartX());
         updateDragImage();
         repaint();
    }

}
void AutomationLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    if (m_moveSelection)
    {
        te::TrackAutomationSection::ActiveParameters par;
        par.param = m_curve.getOwnerParameter();
        par.curve = m_curve;
        auto screenStart = m_editViewState.beatToTime(m_editViewState.m_viewX1);
        auto newStart = m_snap ? getSnapedTime(m_rangeAtMouseDown.getStart().inSeconds() + m_draggedTime - screenStart)
                               : m_rangeAtMouseDown.getStart().inSeconds() + m_draggedTime - screenStart;
        newStart = juce::jmax(0.0, newStart);
        auto range = m_rangeAtMouseDown.movedToStartAt(EngineHelpers::getTimePos(newStart));

        m_curve.state.setProperty(IDs::selectedRangeStart, range.getStart().inSeconds(), nullptr);
        m_curve.state.setProperty(IDs::selectedRangeEnd, range.getEnd().inSeconds(), nullptr);

        EngineHelpers::moveAutomation( getTrack(), par, m_rangeAtMouseDown, range.getStart().inSeconds(), e.mods.isCtrlDown());
    }
    else if (m_selectingTime && !e.mouseWasDraggedSinceMouseDown())
    {
        m_curve.state.setProperty(IDs::selectedRangeStart, 0, nullptr);
        m_curve.state.setProperty(IDs::selectedRangeEnd, 0, nullptr);
        repaint();
    }

    m_draggedTime = 0.0;
    m_snap = false;
    m_moveSelection = false;
    m_selectingTime = false;
}
void AutomationLaneComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    m_moveSelection = false;
    m_hoveredPoint = -1;
    m_hoveredCurve = -1;
    repaint();
}
bool AutomationLaneComponent::keyPressed(const juce::KeyPress &e)
{
   if (e == juce::KeyPress::createFromDescription("CTRL + D") && !getSelectedRange().isEmpty())
   {
        te::TrackAutomationSection::ActiveParameters par;
        par.param = m_curve.getOwnerParameter();
        par.curve = m_curve;
        auto range = getSelectedRange();
        m_curve.state.setProperty(IDs::selectedRangeStart, range.getEnd().inSeconds(), nullptr);
        m_curve.state.setProperty(IDs::selectedRangeEnd, range.getEnd().inSeconds() + range.getLength().inSeconds(), nullptr);
        EngineHelpers::moveAutomation( getTrack(), par, range,range.getEnd().inSeconds(), true);
        GUIHelpers::log("CTRL + D in AutomationLane");
      repaint();
      return true;
   }
   else if (e == juce::KeyPress::deleteKey)
   {
       deleteSelected();
   }
    return false;
}
void AutomationLaneComponent::deleteSelected()
{
   auto range = getSelectedRange(); 

   m_curve.removePointsInRegion(range);
}
void AutomationLaneComponent::updateDragImage()
{ 
        auto tmp = m_moveSelection;
        m_moveSelection = false;
 
        auto rs = getXPos(getSelectedRange().getStart().inSeconds());
        auto rw = getXPos(getSelectedRange().getEnd().inSeconds()) - getXPos(getSelectedRange().getStart().inSeconds());
        auto viewRect = juce::Rectangle<int> (rs, getBoundsInParent().getY(), rw, getHeight());
        m_rangeImage = getParentComponent()->createComponentSnapshot(viewRect);
        m_rangeImage.multiplyAllAlphas(.7f);

        m_moveSelection = tmp;
}
double AutomationLaneComponent::getSnapedTime(double time)
{
    auto snapType = m_editViewState.getBestSnapType (
                       m_editViewState.m_viewX1
                     , m_editViewState.m_viewX2
                     , getWidth());
    auto snapedTime = m_editViewState.getSnapedTime(time, snapType, false);

    return snapedTime;
}
int AutomationLaneComponent::getIndexOfHoveredPoint(const juce::MouseEvent &e)
{
    int p = -1;
    juce::Rectangle<int> hoveredRect = { e.x - getPointWidth ()/2
                                        , e.y - getPointWidth ()/2
                                        , getPointWidth ()
                                            , getPointWidth () };
    for (auto i = 0; i < m_curve.getNumPoints(); i++)
        if (hoveredRect.contains (getPoint (m_curve.getPoint (i))))
            p = i;
        
    return p;
}
 tracktion::core::TimeRange 
 AutomationLaneComponent::getSelectedRange()
{ 
    auto start = juce::jmax (0.0, (double) m_curve.state.getProperty(IDs::selectedRangeStart, 0));
    auto end = juce::jmax (start, (double) m_curve.state.getProperty(IDs::selectedRangeEnd, 0));

    auto spos = tracktion::core::TimePosition::fromSeconds(start);
    auto epos = tracktion::core::TimePosition::fromSeconds(end);
    return {spos,  epos};
}
te::AutomationCurve &AutomationLaneComponent::getCurve() const
{
    return m_curve;
}
double AutomationLaneComponent::getTime(int x)
{
    return m_editViewState.xToTime(x, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}
int AutomationLaneComponent::getXPos(double time)
{
    if (getWidth() == 0)
        return 0;
    return m_editViewState.timeToX(time, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}
double AutomationLaneComponent::getValue(int y)
{
    double pwh = getPointWidth() / 2;
    double lh = getLaneHeight();
    return 1.0 - juce::jmap((double)y, pwh, lh - pwh, 0.0, 1.0);
}
int AutomationLaneComponent::getYPos(double value)
{
    double lh = getLaneHeight();
    double pwh = getPointWidth()/2;

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
bool AutomationLaneComponent::isBeyondLastPoint(double time, float value)
{
    auto tp = EngineHelpers::getTimePos(time);
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
        return 8;
    return 12;
}
