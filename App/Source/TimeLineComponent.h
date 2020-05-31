/*
  ==============================================================================

    TimeLineComponent.h
    Created: 20 Feb 2020 12:06:14am
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SongEditorState.h"

//==============================================================================
/*
*/
class TimeLineComponent : public Component
                        , public ChangeBroadcaster
{
public:
    TimeLineComponent(SongEditorViewState& state, Viewport& pos);
    ~TimeLineComponent();

    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseUp(const MouseEvent& event) override;

    void setScreen(int screenPosX, int screenWidth);

private:
    Point<int> m_posAtMouseDown;
    int m_distanceX{ 0 };
    int m_screenX, m_screenW;
    SongEditorViewState& m_state;
    Viewport& m_viewPort;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeLineComponent)
};
