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
                    , int leftSpace);
    ~TimeLineComponent();

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    te::TimecodeSnapType getBestSnapType();
    int beatsToX (double);
    double xToBeats(int);
    EditViewState& getEditViewState();
    int getTimeLineWidth();

private:
    EditViewState & m_editViewState;
    double m_cachedBeat;
    bool m_isMouseDown;
    double m_cachedX1, m_cachedX2;
    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;
    int m_leftSpace;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
