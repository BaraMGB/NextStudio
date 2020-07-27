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
    HeaderComponent(tracktion_engine::Edit& edit);
    ~HeaderComponent();

    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    void timerCallback() override;


private:
    TextButton m_loadButton, m_saveButton, m_playButton, m_stopButton, m_recordButton, m_settingsButton;
    tracktion_engine::Edit& m_edit;
    Colour m_mainColour{ Colour(0xff57cdff) };
    SpinBoxGroup m_transportDisplay;
    SpinBoxGroup m_BpmDisplay;
    SpinBoxGroup m_signatureDisplay;
    SpinBoxGroup m_LoopBeginDisplay;
    SpinBoxGroup m_LoopEndDisplay;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};
