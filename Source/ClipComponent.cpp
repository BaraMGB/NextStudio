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
    g.fillAll (m_engineClip.getColour());

    g.setColour (Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (Colours::white);
    g.setFont (14.0f);
    g.drawText (m_engineClip.getName(), getLocalBounds(),
                Justification::centred, true);   // draw some placeholder text
}

void ClipComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}



void ClipComponent::mouseDown(const MouseEvent& event)
{
    m_ClipPosAtMouseDown =  m_engineClip.edit.tempoSequence.timeToBeats(m_engineClip.getPosition().getStart());
}

void ClipComponent::mouseDrag(const MouseEvent& event)
{
    m_engineClip.setStart(m_engineClip.edit.tempoSequence.beatsToTime(m_ClipPosAtMouseDown + event.getDistanceFromDragStartX() / 10),false, true);
}

double ClipComponent::getLength()
{
    return m_engineClip.state[tracktion_engine::IDs::length];
}

double ClipComponent::getStart()
{
    return m_engineClip.state[tracktion_engine::IDs::start];
}
