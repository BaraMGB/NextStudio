/*
  ==============================================================================

    ArrangerComponent.h
    Created: 7 Jan 2020 8:29:38pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class ArrangerComponent    : public Component
{
public:
    ArrangerComponent();
    ~ArrangerComponent();

    void paint (Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArrangerComponent)
};
