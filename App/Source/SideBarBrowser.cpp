/*
  ==============================================================================

    SideBarBrowser.cpp
    Created: 2 Mar 2020 10:26:11pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "SideBarBrowser.h"
#include "MainComponent.h"

void SideBarBrowser::fileDoubleClicked(const juce::File &)
{
    auto selectedFile = m_DirTreeViewBox.getSelectedFile();
    tracktion_engine::AudioFile audioFile(m_edit.engine, selectedFile);
    if (selectedFile.getFileExtension () == ".tracktionedit")
    {
        if (auto mc = dynamic_cast<MainComponent*>(getParentComponent ()))
        {
            mc->setupEdit(selectedFile);
        }
    }
    else if (audioFile.isValid ())
    {
        EngineHelpers::loadAudioFileAsClip (m_edit, selectedFile);
    }
}
