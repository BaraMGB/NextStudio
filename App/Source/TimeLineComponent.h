#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"

namespace te = tracktion_engine;

class TimeLineComponent : public juce::Component
                        , public juce::ChangeBroadcaster
{
public:
    TimeLineComponent(EditViewState& state);
    ~TimeLineComponent();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    int m_posAtMouseDownY, m_posAtMouseDownX;
    double m_BeatAtMouseDown;
    bool m_mouseDown;
    int m_oldDragDistX{0}, m_oldDragDistY{0};
    double m_x1atMD, m_x2atMD;
    EditViewState& m_state;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
