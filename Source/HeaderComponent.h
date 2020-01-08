/*
  ==============================================================================

    HeaderComponent.h
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "TransportDisplayComponent.h"

//==============================================================================
/*
*/
class HeaderComponent    : public Component
                         , public Button::Listener
                         , private Timer
{
public:
    HeaderComponent::HeaderComponent(int height, int width, tracktion_engine::Edit* edit);
    ~HeaderComponent();

    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked(Button* button);
    void timerCallback() override;
    double getPPQPos(double time)
    {
        double ppqPerSecond = 120/*bpm*/ / 60 * 480;
        return time * ppqPerSecond;
    }


private:
    TextButton m_loadButton, m_saveButton, m_playButton, m_stopButton, m_recordButton;
    TransportDisplayComponent m_transportDisplay;
    tracktion_engine::Edit* m_edit;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};
