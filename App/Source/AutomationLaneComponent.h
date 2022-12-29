//
// Created by bara on 31.12.21.
//
#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_TimeRange.h"

namespace te = tracktion_engine;
class AutomationPoint  : public te::Selectable
{
public:
    AutomationPoint (int i, te::AutomationCurve& c)  : index (i), m_curve (c) {}
    ~AutomationPoint() override {notifyListenersOfDeletion();}

    juce::String getSelectableDescription() override {return juce::String("AutomationPoint");}
    static bool arePointsConsecutive (const te::SelectableList&);
    static bool arePointsOnSameCurve (const te::SelectableList&);

    int index = 0;
    te::AutomationCurve&        m_curve;
};

class AutomationLaneComponent : public juce::Component
{
public:
    AutomationLaneComponent(te::AutomationCurve& curve, EditViewState& evs);
    void paint (juce::Graphics& g) override;
    void paintCurves(juce::Graphics &g, tracktion::TimeRange drawRange);
    void paintOverChildren(juce::Graphics &g) override;

    [[nodiscard]] te::AutomationCurve &getCurve() const;

    juce::Point<int> getPointXY (tracktion::TimePosition t, double v);

    void selectPoint(int index, bool add);
    bool isPointSelected(int index);
    void deselectPoint(int index);
    
    tracktion::TimeRange getSelectedTimeRange();

    juce::Rectangle<int> getHoveredRect(const juce::MouseEvent &e);
    juce::Point<int> getPoint(const te::AutomationCurve::AutomationPoint& ap);

    tracktion::TimePosition getSnapedTime(tracktion::TimePosition time, bool down);
    tracktion::TimePosition getTimePosFromX (int x);
    int getXPos (double time);
    double getValue(double y);
    int getYPos (double value);
    double xToYRatio();

    void setHover (int hoveredPoint, int hoveredCurve, juce::Rectangle<int> hoveredRect)
    {
        m_hoveredAutomationPoint = hoveredPoint;
        m_hoveredCurve = hoveredCurve;
        m_hoveredRect = hoveredRect.toFloat();
    }

    juce::Rectangle<float> getRectFromPoint(juce::Point<int> p);

    struct CurvePoint
    {
        CurvePoint(tracktion::TimePosition t, double v, int i, te::AutomatableParameter& p)
                : time(t), value(v), index(i), param(p) {}
        ~CurvePoint() {}
        tracktion::TimePosition   time;
        double                    value;
        int                       index;
        te::AutomatableParameter& param;
    };

    void setIsDragging(bool isDragging)
    {
        m_isDragging = isDragging;
    }

    te::AutomatableParameter * getParameter() { return m_curve.getOwnerParameter(); }
private:



    AutomationPoint* createSelectablePoint(int index);
    tracktion::TimeRange getCurveTimeRangeToDraw();
    double getValueAt(int x);
    double xToTime(const int x);
    int getLaneHeight();
    bool isBeyondLastPoint(tracktion::TimePosition time, float value);
    int getPointWidth();
    void deleteSelected();


    int getIndexOfHoveredPoint(const juce::MouseEvent &e);


	te::Track* getTrack()
	{
		return m_curve.getOwnerParameter()->getTrack();
	}

    juce::Array<CurvePoint*>     m_selPointsAtMousedown;
    te::AutomationCurve&        m_curve;
    int                         m_hoveredAutomationPoint = -1;
    int                         m_hoveredCurve = -1;
    tracktion::TimePosition     m_hoveredTime;
    juce::Rectangle<float>      m_hoveredRect;
    double                      m_curveAtMousedown{};
    tracktion::TimePosition     m_timeAtMousedown;
    juce::Point<int>            m_hovedPointXY;
    EditViewState&              m_editViewState;
    te::SelectionManager &      m_selectionManager {m_editViewState.m_selectionManager};
    bool                        m_isDragging {false};
    tracktion::core::TimeRange  m_rangeAtMouseDown;
    tracktion::TimeDuration     m_draggedTime;
    juce::Image                 m_rangeImage;
};
