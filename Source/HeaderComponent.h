/*
  ==============================================================================

    HeaderComponent.h
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "SpinBoxGroup.h"

//==============================================================================
/*
*/
class HeaderComponent    : public Component
                         , public Button::Listener
                         , private Timer
{
public:
    HeaderComponent(int height, int width, tracktion_engine::Edit& edit);
    ~HeaderComponent();

    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void timerCallback() override;


private:
    TextButton m_loadButton, m_saveButton, m_playButton, m_stopButton, m_recordButton;
    tracktion_engine::Edit& m_edit;
    int m_test;
    SpinBoxGroup m_transportDisplay;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};
