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

//[Headers]     -- You can add your own extra header files here --
#include "../JuceLibraryCode/JuceHeader.h"
#include "DraggableLabel.h"
//[/Headers]


//==============================================================================
/**
                                                                    //[Comments]
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
                                                                    //[/Comments]
*/
class TransportDisplayComponent  : public Component
{
public:
    //==============================================================================
    TransportDisplayComponent ();
    ~TransportDisplayComponent() override;

    void paint (Graphics& g) override;
    void resized() override;


private:
    DraggableLabel m_bars;
    DraggableLabel m_beat;
    DraggableLabel m_quat;
    DraggableLabel m_cent;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportDisplayComponent)
};

//[EndFile] You can add extra defines here...
//[/EndFile]

