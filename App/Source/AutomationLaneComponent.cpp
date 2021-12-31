//
// Created by bara on 31.12.21.
//

#include "AutomationLaneComponent.h"

AutomationLaneComponent::AutomationLaneComponent(tracktion_engine::AutomationCurve &curve, EditViewState &evs)
    : m_curve(curve)
    , m_editViewState(evs)
{
}

void AutomationLaneComponent::paint(juce::Graphics &g)
{
    g.setColour(juce::Colours::white);
    auto tr = te::EditTimeRange( m_editViewState.beatToTime(m_editViewState.m_viewX1)
                                    , m_editViewState.beatToTime(m_editViewState.m_viewX2));

    float oldX = 0.f, oldY = (float) getYPos(m_curve.getValueAt(0.0));

    juce::Path curvePath;
    juce::Path hoveredCurve;
    juce::Path dots;
    juce::Path hoveredDot;
    juce::Path hoveredDotOnCurve;

    curvePath.startNewSubPath(oldX, oldY);
    for (auto i = 0; i < m_curve.getNumPoints(); i++)
    {
        float x = (float) getXPos(m_curve.getPoint(i).time);
        float y = (float) getYPos(m_curve.getPoint(i).value);
        float curve = m_curve.getPoint(i - 1).curve;

        auto curveControlPoint = juce::Point<float>(
            oldX + ((x - oldX) * (0.5f + curve))
                , oldY + ((y - oldY) * (0.5f - curve)));
        curvePath.quadraticTo(curveControlPoint, {x, y});

        if (m_hoveredCurve == i)
        {
            hoveredCurve.startNewSubPath (oldX, oldY);
            hoveredCurve.quadraticTo (curveControlPoint, {x,y});

            hoveredDotOnCurve.addEllipse (m_hoveredRect.toFloat ());
        }



        oldX = x; oldY = y;

        if (m_hoveredPoint == i)
        {
            hoveredDot.addEllipse(x - (float) getPointWidth ()/2
                                  , y - (float) getPointWidth ()/2
                                  , (float) getPointWidth ()
                                      , (float) getPointWidth ());
        }
        dots.addEllipse(x - (float) getPointWidth ()/2
                        , y - (float) getPointWidth ()/2
                        , (float) getPointWidth ()
                            , (float) getPointWidth ());
    }

    if (m_hoveredCurve > m_curve.getNumPoints () - 1)
    {
        hoveredDotOnCurve.addEllipse (m_hoveredRect.toFloat ());
    }

    //close the path
    curvePath.lineTo((float) getWidth() + 1.f, oldY);
    curvePath.lineTo((float) getWidth() + 1.f, (float) getHeight() + 1.f);
    curvePath.lineTo(-1.f, (float) getHeight() + 1.f);
    curvePath.lineTo (-1 , (float) getYPos(m_curve.getValueAt(0.0)));
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
    g.strokePath(dots, juce::PathStrokeType(2.0f));
    g.setColour(juce::Colours::white);
    g.strokePath(hoveredDot, juce::PathStrokeType(2.0f));
}

void AutomationLaneComponent::mouseMove(const juce::MouseEvent &e)
{
    auto timeHovered = getTime(e.x);
    auto valueHovered = (float) getValue (e.y);


    m_hoveredPoint = getIndexOfHoveredPoint (e);

    m_hoveredCurve = -1;
    if (m_hoveredPoint == -1)
    {
        int yPosAtHoveredTime = getYPos(m_curve.getValueAt(timeHovered));
        m_hoveredRect = {e.x - getPointWidth ()/2
                         , yPosAtHoveredTime - getPointWidth ()/2
                         , getPointWidth ()
                             , getPointWidth ()};
        auto nearestPoint = m_curve.getNearestPoint (timeHovered, valueHovered , xToYRatio ());

        m_isVertical = m_curve.getPoint (nearestPoint).time == timeHovered;

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

void AutomationLaneComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    m_hoveredPoint = -1;
    m_hoveredCurve = -1;
    repaint();
}

void AutomationLaneComponent::mouseDown(const juce::MouseEvent &e)
{
    m_editViewState.m_selectionManager.selectOnly (m_curve.getOwnerParameter ()->getTrack ());
    if (m_hoveredPoint == -1)
    {
        bool isTimePositive = m_hoveredTime > 0;
        bool leftMouseButton = e.mods.isLeftButtonDown() && !e.mods.isCtrlDown ();

        if (isTimePositive && leftMouseButton && !m_isVertical)
        {
            auto newPoint = m_curve.addPoint(
                m_hoveredTime
                , m_curve.getValueAt (m_hoveredTime)
                    , 0.0);
            m_hoveredPoint = newPoint;
            m_hoveredCurve = -1;
            m_hoveredTime = 0;
            repaint();
        }

        if (m_hoveredCurve != -1 && e.mods.isRightButtonDown())
        {
            m_curve.setCurveValue(m_hoveredCurve - 1, 0.0);
            repaint();
        }
    }
    else
    {
        if (e.mods.isRightButtonDown())
        {
            m_curve.removePoint(m_hoveredPoint);
            m_hoveredPoint = -1;
            repaint();
        }
        else
        {
            m_timeAtMousedown = m_curve.getPointTime (m_hoveredPoint);
        }
    }
    m_curveAtMousedown = m_curve.getPoint(m_hoveredCurve - 1).curve;
}

double AutomationLaneComponent::getNewTime(const juce::MouseEvent &e)
{
    auto snapType = m_editViewState.getBestSnapType (
        m_editViewState.m_viewX1
        , m_editViewState.m_viewX2
        , getWidth());
    auto snapedTime = m_editViewState.getSnapedTime(
        getTime(e.x)
            , snapType);
    auto newTime = e.mods.isCtrlDown ()
                       ? m_timeAtMousedown
                       : e.mods.isShiftDown()
                             ? getTime(e.x)
                             : snapedTime;

    return newTime;
}

int AutomationLaneComponent::getIndexOfHoveredPoint(const juce::MouseEvent &e)
{
    int p = -1;
    juce::Rectangle<int> hoveredRect = { e.x - getPointWidth ()/2
                                        , e.y - getPointWidth ()/2
                                        , getPointWidth ()
                                            , getPointWidth () };
    for (auto i = 0; i < m_curve.getNumPoints(); i++)
    {
        if (hoveredRect.contains (getPoint (m_curve.getPoint (i))))
        {
            p = i;
        }
    }
    return p;
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent &e)
{
    //change curve
    bool setCurveSteep = e.mods.isCtrlDown ();
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
        auto newTime = getNewTime(e);
        m_curve.movePoint(m_hoveredPoint
                          , newTime
                          , (float) getValue(e.y)
                              , false);
        repaint();
    }
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
    return m_editViewState.timeToX(time, getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
}

double AutomationLaneComponent::getValue(int y)
{
    return 1.0 - (double)(y + (getPointWidth ()/2.0+1) +1)
                     / (double) (getLaneHeight());
}

int AutomationLaneComponent::getYPos(double value)
{
    double lh = getLaneHeight();
    double pw = getPointWidth();
    return juce::roundToInt((lh - value * lh) + (pw/2.0) + 1.0);
}

juce::Point<int> AutomationLaneComponent::getPoint(const tracktion_engine::AutomationCurve::AutomationPoint &ap)
{
    return {getXPos (ap.time), getYPos (ap.value)};
}

double AutomationLaneComponent::xToYRatio()
{
    //1 screen unit in value / 1 screen unit in time
    double screenUnitInValue = 1.0/getLaneHeight();
    double screenUnitInTime =
        (m_editViewState.beatToTime(m_editViewState.m_viewX2)
         - m_editViewState.beatToTime(m_editViewState.m_viewX1))
        / getWidth();
    return screenUnitInValue / screenUnitInTime;
}

int AutomationLaneComponent::getLaneHeight()
{
    return getHeight() - getPointWidth () - 3;
}

bool AutomationLaneComponent::isBeyondLastPoint(double time, float value)
{
    auto nearestPoint = m_curve.getNearestPoint (time, value , xToYRatio ());
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

//==============================================================================