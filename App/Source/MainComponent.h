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
    void fileDoubleClicked(const File&) override
    {
        auto selectedFile = m_tree.getSelectedFile();
        if (selectedFile.existsAsFile())
        {
            auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
            auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
            auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));

            if (auto track = EngineHelpers::getOrInsertAudioTrackAt (*m_edit, tracktion_engine::getAudioTracks(*m_edit).size()))
            {

                 track->setName("Track " + String(tracktion_engine::getAudioTracks(*m_edit).size()));
                 track->setColour(Colour(red, gre, blu));
                 EngineHelpers::removeAllClips(*track);
                 // Add a new clip to this track
                 tracktion_engine::AudioFile audioFile(m_edit->engine, selectedFile);
                 if (audioFile.isValid())
                     if (auto newClip = track->insertWaveClip(selectedFile.getFileNameWithoutExtension(), selectedFile,
                              { { 0.0, 0.0 + audioFile.getLength() }, 0.0 }, false))
                     {
                         newClip->setColour(track->getColour());
                     }
            }
        }
    }

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

    tracktion_engine::Engine m_engine{ ProjectInfo::projectName };
    tracktion_engine::SelectionManager m_selectionManager{ m_engine };
    std::unique_ptr<tracktion_engine::Edit> m_edit;
    std::unique_ptr<EditComponent> m_songEditor;


    const int c_headerHeight = 100;
    const int c_footerHeight = 50;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
