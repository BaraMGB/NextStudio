#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"

namespace te = tracktion_engine;

class TimeLineComponent : public juce::Component
                        , public juce::ChangeBroadcaster
{
public:
    TimeLineComponent(EditViewState&, juce::CachedValue<double> & x1
                      , juce::CachedValue<double> & x2);
    ~TimeLineComponent();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    int beatsToX (double);
    double xToBeats(int);
private:
    EditViewState & m_state;
    int m_posAtMouseDownY, m_posAtMouseDownX;
    double m_BeatAtMouseDown;
    bool m_mouseDown;
    int m_oldDragDistX{0}, m_oldDragDistY{0};
    double m_x1atMD, m_x2atMD;
    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
