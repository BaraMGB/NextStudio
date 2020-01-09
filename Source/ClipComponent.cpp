/*
  ==============================================================================

    ClipComponent.cpp
    Created: 8 Jan 2020 11:53:16pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ClipComponent.h"

//==============================================================================





void ClipComponent::paint (Graphics& g)
{
    /* This demo code just fills the component's background and
       draws some placeholder text to get you started.

       You should replace everything in this method with your own
       drawing code..
    */

    g.fillAll (m_colour);   // clear the background

    g.setColour (Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (Colours::white);
    g.setFont (14.0f);
    g.drawText ("ClipComponent", getLocalBounds(),
                Justification::centred, true);   // draw some placeholder text
}

void ClipComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}
