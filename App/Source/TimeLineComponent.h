#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"

namespace te = tracktion_engine;

class TimeLineComponent : public juce::Component
{
public:
    TimeLineComponent(EditViewState&
                    , juce::CachedValue<double> & x1
                    , juce::CachedValue<double> & x2
                    );
    ~TimeLineComponent() override;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& event) override;

    te::TimecodeSnapType    getBestSnapType();
    EditViewState&          getEditViewState();

private:

    void                    drawLoopRange(juce::Graphics& g);
    double                  getMovedTime(const juce::MouseEvent& e, double oldTime);
    void                    moveLoopRange(const juce::MouseEvent& e);
    void                    updateViewRange(const juce::MouseEvent& e);
    juce::Rectangle<int>    getLoopRangeRect();

    int                     timeToX (double time);
    int                     beatsToX (double beats);
    double                  xToTime (int x);
    double                  beatToTime(double beats);

    EditViewState &         m_editViewState;
    double                  m_cachedBeat{};
    bool                    m_isMouseDown,
                            m_leftResized,
                            m_rightResized,
                            m_loopRangeClicked;
    double                  m_cachedX1{}, m_cachedX2{};

    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;

    double                  m_cachedL1;
    double                  m_cachedL2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
