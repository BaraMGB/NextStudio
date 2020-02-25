/*
  ==============================================================================

    ClipComponent.cpp
    Created: 8 Jan 2020 11:53:16pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "ClipComponent.h"
#include "ArrangerComponent.h"

//==============================================================================





void ClipComponent::paint (Graphics& g)
{
    g.setColour(m_engineClip.getColour());
    g.fillRect(getLocalBounds());

    g.setColour (Colours::grey);
    g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

    g.setColour (Colours::white);
    g.setFont (14.0f);
    g.drawText (
        m_engineClip.getName()
        , getLocalBounds()
        , Justification::topLeft
        , true
    );
}

void ClipComponent::resized()
{
}



void ClipComponent::mouseDown(const MouseEvent& event)
{
    m_ClipPosAtMouseDown = m_engineClip.edit.tempoSequence.timeToBeats(m_engineClip.getPosition().getStart());
    //std::cout << m_engineClip.state.toXmlString();
}

void ClipComponent::mouseDrag(const MouseEvent& event)
{
    m_engineClip.setStart(m_engineClip.edit.tempoSequence.beatsToTime(
        m_ClipPosAtMouseDown
        + event.getDistanceFromDragStartX()
        / m_pixelPerBeat
    ), false, true);
}

double ClipComponent::getLength()
{
    return m_engineClip.state[tracktion_engine::IDs::length];
}

double ClipComponent::getStart()
{
    return m_engineClip.state[tracktion_engine::IDs::start];
}
