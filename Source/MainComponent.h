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

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/


class MainComponent   : public AudioAppComponent
                      , public FileBrowserListener
                      , public Button::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* button) override;
    
private:
    void selectionChanged()                           override {}
    void fileClicked (const File&, const MouseEvent&) override {}
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
    TimeSliceThread m_thread;
    DirectoryContentsList m_dirConList;
    FileTreeComponent m_tree;
    StretchableLayoutManager m_stretchableManager;
    StretchableLayoutResizerBar m_resizerBar{ &m_stretchableManager, 1, true };
    std::unique_ptr<HeaderComponent> m_header;
    std::unique_ptr<SongEditorComponent> m_songEditor;
    NextLookAndFeel m_nextLookAndFeel;

    tracktion_engine::Engine m_engine{ ProjectInfo::projectName };
    std::unique_ptr<tracktion_engine::Edit> m_edit;



    const int c_headerHeight = 50;
    const int c_footerHeight = 50;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
