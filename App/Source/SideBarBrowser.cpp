/*
  ==============================================================================

    SideBarBrowser.cpp
    Created: 2 Mar 2020 10:26:11pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SideBarBrowser.h"

//==============================================================================

void SideBarBrowser::mouseDrag(const juce::MouseEvent& /*event*/)
{
    auto dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC->isDragAndDropActive())
    {
        dragC->startDragging("Test", this);
    }
}
