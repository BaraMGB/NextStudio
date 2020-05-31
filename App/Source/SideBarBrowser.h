/*
  ==============================================================================

    SideBarBrowser.h
    Created: 2 Mar 2020 10:26:11pm
    Author:  Zehn

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class SideBarBrowser    : public FileTreeComponent
{
public:
    SideBarBrowser(DirectoryContentsList& listToShow)
        : FileTreeComponent(listToShow)
    {

    }

    void mouseDrag(const MouseEvent& /*event*/) override;
    void mouseDown(const MouseEvent& /*event*/) override
    {
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
};
