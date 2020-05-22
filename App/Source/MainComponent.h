/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "HeaderComponent.h"
#include "NextLookAndFeel.h"
#include "SongEditorComponent.h"
#include "MenuBar.h"
#include "SideBarBrowser.h"

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
    void fileClicked (const File& file, const MouseEvent& event) override 
    {
        m_tree.setDragAndDropDescription(file.getFileName());
    }
    void fileDoubleClicked(const File&)               override
    {
        auto selectedFile = m_tree.getSelectedFile();
        if (selectedFile.existsAsFile())
        {
            m_songEditor->addTrack(selectedFile);
        }
    }

    void removeAllClips(tracktion_engine::AudioTrack& track)
    {
        auto clips = track.getClips();

        for (int i = clips.size(); --i >= 0;)
            clips.getUnchecked(i)->removeFromParentTrack();
    }

    tracktion_engine::AudioTrack* getOrInsertAudioTrackAt(tracktion_engine::Edit& edit, int index)
    {
        edit.ensureNumberOfAudioTracks(index + 1);
        return tracktion_engine::getAudioTracks(edit)[index];
    }

    void browserRootChanged(const File&) override {}

    void setupEdit();


    TimeSliceThread m_thread;
    DirectoryContentsList m_dirConList;
    FileTreeComponent m_tree;
    MenuBar m_menuBar;
    StretchableLayoutManager m_stretchableManager;
    StretchableLayoutResizerBar m_resizerBar{ &m_stretchableManager, 1, true };
    std::unique_ptr<HeaderComponent> m_header;
    NextLookAndFeel m_nextLookAndFeel;

    tracktion_engine::Engine m_engine{ ProjectInfo::projectName };
    tracktion_engine::SelectionManager m_selectionManager{ m_engine };
    std::unique_ptr<tracktion_engine::Edit> m_edit;
    std::unique_ptr<SongEditorComponent> m_songEditor;


    const int c_headerHeight = 100;
    const int c_footerHeight = 50;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
