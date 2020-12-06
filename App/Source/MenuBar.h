/*
  ==============================================================================

    MenuBar.h
    Created: 23 Feb 2020 5:36:47pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class MenuBar    : public juce::Component
{
public:
    MenuBar();
    ~MenuBar();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuBar)
};
