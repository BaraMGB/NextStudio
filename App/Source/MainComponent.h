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


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/




class MainComponent   : public Component
                      , public FileBrowserListener
                      , public Button::Listener
                      , public DragAndDropContainer
                      , public ChangeListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    
private:
    void changeListenerCallback(ChangeBroadcaster* source);
    void selectionChanged()                           override {}
    void fileClicked (const File& file, const MouseEvent& event) override;
    void fileDoubleClicked(const File&) override;

    void browserRootChanged(const File&) override {}

    void setupEdit(File);
    void createTracksAndAssignInputs();


    TimeSliceThread m_thread;
    DirectoryContentsList m_dirConList;
    FileTreeComponent m_tree;
    MenuBar m_menuBar;

    std::unique_ptr<HeaderComponent> m_header;
    Label m_editNameLabel { "No Edit Loaded" };
    NextLookAndFeel m_nextLookAndFeel;

    tracktion_engine::Engine m_engine{ ProjectInfo::projectName, std::make_unique<ExtendedUIBehaviour>(), nullptr };
    tracktion_engine::SelectionManager m_selectionManager{ m_engine };
    std::unique_ptr<tracktion_engine::Edit> m_edit;
    std::unique_ptr<EditComponent> m_songEditor;


    const int c_headerHeight = 100;
    const int c_footerHeight = 50;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
