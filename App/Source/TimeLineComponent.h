#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "tracktion_core/utilities/tracktion_TimeRange.h"

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

    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;

private:

    void                    drawLoopRange(juce::Graphics& g);
    void                    moveOrResizeLoopRange();
    tracktion::TimeRange    getLoopRangeToBeMovedOrResized();
    void                    updateViewRange(const juce::MouseEvent& e);

    juce::Rectangle<int>    getTimeRangeRect(tracktion::TimeRange tr);
    juce::Rectangle<int>    getLoopRangeRect();

    int                     timeToX (double time);
    int                     beatsToX (double beats);
    tracktion::TimeDuration xToTimeDuration (int x);
    tracktion::TimePosition beatToTime(tracktion::BeatPosition beats);

    EditViewState &         m_editViewState;
    double                  m_cachedBeat{};
    bool                    m_isMouseDown,
                            m_isSnapping {true},
                            m_leftResized,
                            m_rightResized,
                            m_changeLoopRange{false},
                            m_loopRangeClicked;
    double                  m_cachedX1{}, m_cachedX2{};

    tracktion::TimeRange    m_cachedLoopRange; 
    tracktion::TimeDuration m_draggedTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
