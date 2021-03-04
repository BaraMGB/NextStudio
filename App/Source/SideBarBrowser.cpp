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

//==============================================================================

SideBarBrowser::SideBarBrowser(juce::ValueTree &state, tracktion_engine::Edit &edit)
    : m_applicationState(state)
    , m_edit(edit)
{
    addAndMakeVisible(m_tree);
    m_thread.startThread(1);
    juce::File file = juce::File::createFileWithoutCheckingPath (
                m_applicationState.getProperty (IDs::WorkDIR));
    if (!file.isDirectory ())
    {
        file = juce::File::getCurrentWorkingDirectory ();
    }
    m_dirConList.setDirectory(file, true, true);
    m_tree.addListener(this);

    m_tree.setColour (juce::TreeView::ColourIds::backgroundColourId
                      , juce::Colour(0xff1b1b1b));
    m_tree.setColour (juce::DirectoryContentsDisplayComponent::highlightColourId
                      , juce::Colour(0xff4b4b4b));
}

void SideBarBrowser::paint(juce::Graphics &g)
{
}

void SideBarBrowser::paintOverChildren(juce::Graphics &g)
{
    juce::Path fakeRoundedCorners;
    auto bounds = getLocalBounds (); //your component's bounds

    const float cornerSize = 10.f; //desired corner size
    fakeRoundedCorners.addRectangle(bounds); //What you start with
    fakeRoundedCorners.setUsingNonZeroWinding(false); //The secret sauce
    fakeRoundedCorners.addRoundedRectangle(bounds, cornerSize); //subtract this shape

    g.setColour(juce::Colour(0xff555555));
    g.fillPath(fakeRoundedCorners);
}

void SideBarBrowser::resized()
{
    m_tree.setBounds (getLocalBounds ());
}

void SideBarBrowser::mouseDrag(const juce::MouseEvent& /*event*/)
{
    auto dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if (!dragC->isDragAndDropActive())
    {
        dragC->startDragging("Test", this);
    }
}

void SideBarBrowser::mouseDown(const juce::MouseEvent &)
{
}

void SideBarBrowser::fileClicked(const juce::File &file, const juce::MouseEvent &event)
{
    m_tree.setDragAndDropDescription(file.getFileName());
}

void SideBarBrowser::fileDoubleClicked(const juce::File &)
{
    auto selectedFile = m_tree.getSelectedFile();
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
