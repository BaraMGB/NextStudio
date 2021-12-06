#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"


namespace te = tracktion_engine;

class PlayheadComponent : public juce::Component
                        , private juce::Timer
{
public:
    PlayheadComponent (te::Edit&
                       , EditViewState&
                       , juce::CachedValue<double>& x1
                       , juce::CachedValue<double>& x2);

    void paint (juce::Graphics& g) override;
    bool hitTest (int x, int y) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    te::Edit& m_edit;
    EditViewState& m_editViewState;
    juce::CachedValue<double> & m_X1;
    juce::CachedValue<double> & m_X2;

    int m_xPosition = 0;
    bool m_firstTimer = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlayheadComponent)
};
