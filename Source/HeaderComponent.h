/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 5.4.5

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2017 - ROLI Ltd.

  ==============================================================================
*/

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "TransportDisplayComponent.h"

//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class HeaderComponent : public Component
    , public Button::Listener
{
public:
    HeaderComponent (int height, int width);
    ~HeaderComponent() override;

    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;

private:
    TextButton m_loadButton, m_saveButton, m_playButton, m_stopButton, m_recordButton;
    TransportDisplayComponent m_transportDisplay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeaderComponent)
};

