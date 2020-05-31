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
    if(!event.mouseWasDraggedSinceMouseDown())
    {
        if (event.mods.isRightButtonDown())
        {
            PopupMenu m;
            m.addItem(1, "delete Clip");
            m.addItem(2, "item 2");

            const int result = m.show();

            if (result == 0)
            {
                // user dismissed the menu without picking anything
            }
            else if (result == 1)
            {
                m_engineClip.removeFromParentTrack();
                return;
                // user picked item 1
            }
            else if (result == 2)
            {
                // user picked item 2
            }
        }
    }
    m_ClipPosAtMouseDown = m_engineClip.edit.tempoSequence.timeToBeats(m_engineClip.getPosition().getStart());
}

void ClipComponent::mouseDrag(const MouseEvent& event)
{
    //if (event.getDistanceFromDragStartY() > 50 || event.getDistanceFromDragStartY() < -50)
    {

        DragAndDropContainer* dragC = DragAndDropContainer::findParentDragContainerFor(this);
        if (!dragC->isDragAndDropActive())
        {
            dragC->startDragging("Clip", this,juce::Image(Image::ARGB,1,1,true), 
                false);
        }
    }
    //else if (m_state.m_pixelPerBeat > 1.0)
    {
        auto newPos = jmax(
            0.0, 
            m_engineClip.edit.tempoSequence.beatsToTime(
                m_ClipPosAtMouseDown 
                + event.getDistanceFromDragStartX() 
                / m_state.m_pixelPerBeat 
            )
        );
        if (!event.mods.isCtrlDown())
        {
            newPos = m_engineClip.edit.getTimecodeFormat().getSnapType(9).roundTimeNearest(newPos, m_engineClip.edit.tempoSequence);
        }
        m_engineClip.setStart(newPos, false, true);
    }
}

double ClipComponent::getLength()
{
    return m_engineClip.state[tracktion_engine::IDs::length];
}

double ClipComponent::getStart()
{
    return m_engineClip.state[tracktion_engine::IDs::start];
}
