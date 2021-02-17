/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "HeaderComponent.h"
#include "NextLookAndFeel.h"
#include "MenuBar.h"
#include "SideBarBrowser.h"
#include "EditViewState.h"
#include "EditComponent.h"
#include "Utilities.h"
#include "PluginWindow.h"

namespace te = tracktion_engine;

class MainComponent   : public juce::Component
                      , public juce::FileBrowserListener
                      , public juce::DragAndDropContainer
                      , public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress &key) override;

    bool handleUnsavedEdit();

    juce::ValueTree& state();

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source);
    void selectionChanged()                           override {}
    void fileClicked (const juce::File& file, const juce::MouseEvent& event) override;
    void fileDoubleClicked(const juce::File&) override;
    void browserRootChanged(const juce::File&) override {}

    void setupEdit (juce::File = {});
    void createTracksAndAssignInputs();
    void loadApplicationSettings();
    void openValidStartEdit();
    void setupSideBrowser();

    juce::TimeSliceThread       m_thread    {"file browser thread"};
    juce::DirectoryContentsList m_dirConList{nullptr, m_thread};
    juce::FileTreeComponent     m_tree      {m_dirConList};
    MenuBar                     m_menuBar;

    std::unique_ptr<HeaderComponent> m_header;
    juce::Label                      m_editNameLabel { "No Edit Loaded" };
    NextLookAndFeel                  m_nextLookAndFeel;

    tracktion_engine::Engine m_engine
                                { ProjectInfo::projectName, std::make_unique<ExtendedUIBehaviour>(), nullptr };
    tracktion_engine::SelectionManager      m_selectionManager{ m_engine };
    std::unique_ptr<tracktion_engine::Edit> m_edit;
    std::unique_ptr<EditComponent>          m_songEditor;
    juce::ValueTree m_state;

// todo : into settings
    const int c_headerHeight = 100;
    const int c_footerHeight = 50;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
