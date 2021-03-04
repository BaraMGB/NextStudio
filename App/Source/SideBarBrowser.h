#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class SideBarBrowser : public juce::Component
                     , public juce::FileBrowserListener
{
public:
    SideBarBrowser(juce::ValueTree& state, te::Edit& edit);
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized () override;
    void mouseDrag(const juce::MouseEvent& /*event*/) override;
    void mouseDown(const juce::MouseEvent& /*event*/) override;

    void selectionChanged()                           override {}
    void fileClicked (const juce::File& file, const juce::MouseEvent& event) override;
    void fileDoubleClicked(const juce::File&) override;
    void browserRootChanged(const juce::File&) override {}

private:
    juce::ValueTree &           m_applicationState;
    te::Edit &                  m_edit;
    juce::TimeSliceThread       m_thread    {"file browser thread"};
    juce::DirectoryContentsList m_dirConList{nullptr, m_thread};
    juce::FileTreeComponent     m_tree      {m_dirConList};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
};
