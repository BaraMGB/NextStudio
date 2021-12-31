//
// Created by bara on 31.12.21.
//
#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class AutomationLaneComponent : public juce::Component
{
public:
    AutomationLaneComponent(te::AutomationCurve& curve, EditViewState& evs);
    void paint (juce::Graphics& g) override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent &e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    [[nodiscard]] te::AutomationCurve &getCurve() const;
private:

    double getTime (int x);
    int getXPos (double time);
    double getValue(int y);
    int getYPos (double value);
    double xToYRatio();

    juce::Point<int> getPoint(const te::AutomationCurve::AutomationPoint& ap);
    int getLaneHeight();
    bool isBeyondLastPoint(double time, float value);
    int getPointWidth();

    double getNewTime(const juce::MouseEvent &e);
    int getIndexOfHoveredPoint(const juce::MouseEvent &e);

    te::AutomationCurve&        m_curve;
    int                         m_hoveredPoint{};
    int                         m_hoveredCurve = -1;
    double                      m_hoveredTime{};
    juce::Rectangle<int>        m_hoveredRect;
    double                      m_curveAtMousedown{};
    double                      m_timeAtMousedown{};
    EditViewState&              m_editViewState;
    bool                        m_isVertical {false};
};


