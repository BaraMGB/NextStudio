/*
  ==============================================================================

    TimeLineComponent.h
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class TimeLineComponent : public Component
                        , public ChangeBroadcaster
{
public:
    TimeLineComponent::TimeLineComponent(int& pixelPerBeat, Viewport& pos);
    ~TimeLineComponent();

    void paint(Graphics& g);
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

    void setScreenXPosition(int screenPosX);

private:
    Point<int> m_posAtMouseDown;
    int& m_pixelPerBeat;
    int m_ppbAtMouseDown{ 0 };
    int m_distanceX{ 0 };
    int m_screenX;
    int m_oldWidth;
    Viewport& m_viewPort;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
