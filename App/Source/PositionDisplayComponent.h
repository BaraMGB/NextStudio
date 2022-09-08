
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
// #include "ApplicationViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PositionDisplayComponent  : public juce::Component
{
public:
    explicit PositionDisplayComponent(te::Edit &edit);

    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;

    void update();

private:
    te::Edit& m_edit;
    juce::Rectangle<int> m_bmpRect
                       , m_sigRect
                       , m_barBeatTickRect
                       , m_timeRect
                       , m_loopInrect
                       , m_loopOutRect;
    juce::Label          m_bpmLabel
                       , m_sigLabel
                       , m_barBeatTickLabel
                       , m_timeLabel
                       , m_loopInLabel
                       , m_loopOutLabel;

    juce::Point<int>     m_mousedownPosition;

    double               m_mousedownBPM{};
    tracktion::TimePosition 
                         m_mousedownTime;
    tracktion::BeatPosition
                         m_mousedownBeatPosition
                       , m_mousedownLoopIn
                       , m_mousedownLoopOut;


    int                  m_mousedownNumerator{}
                       , m_mousedownDenominator{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PositionDisplayComponent)
};


