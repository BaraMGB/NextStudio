/*
  ==============================================================================

    MenuBar.h
    Created: 23 Feb 2020 5:36:47pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class MenuBar    : public Component
{
public:
    MenuBar();
    ~MenuBar();

    void paint (Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuBar)
};
