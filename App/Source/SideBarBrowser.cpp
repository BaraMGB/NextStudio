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

void SideBarBrowser::mouseDrag(const MouseEvent& event)
{
    Logger::outputDebugString("DRAG");
    DragAndDropContainer* dragC = DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC->isDragAndDropActive())
    {
        dragC->startDragging("Test", this);
    }
}
