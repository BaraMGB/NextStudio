/*
  ==============================================================================

    TimeLineComponent.h
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#pragma once

 #include "../JuceLibraryCode/JuceHeader.h"
 #include "EditViewState.h"

 //==============================================================================
 /*
 */
 class TimeLineComponent : public Component
                         , public ChangeBroadcaster
 {
 public:
     TimeLineComponent(EditViewState& state);
     ~TimeLineComponent();

     void paint(Graphics& g) override;
     void mouseDown(const MouseEvent& event) override;
     void mouseDrag(const MouseEvent& event) override;
     void mouseUp(const MouseEvent& event) override;

     void setScreen(int screenPosX, int screenWidth);

 private:
     int m_posAtMouseDownY, m_posAtMouseDownX;
     double m_BeatAtMouseDown;
     bool m_mouseDown;
     int m_oldDragDistX{0}, m_oldDragDistY{0};
     double m_x1atMD, m_x2atMD;
     EditViewState& m_state;
     JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
 };
