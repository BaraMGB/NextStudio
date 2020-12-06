#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

class SideBarBrowser    : public juce::FileTreeComponent
{
public:
    SideBarBrowser(juce::DirectoryContentsList& listToShow)
        : juce::FileTreeComponent(listToShow)
    {

    }

    void mouseDrag(const juce::MouseEvent& /*event*/) override;
    void mouseDown(const juce::MouseEvent& /*event*/) override
    {
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
};
