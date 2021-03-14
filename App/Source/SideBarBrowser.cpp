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

void FileListBoxComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_entries.deselectAllRows ();
    m_fileList = fileList;
    m_entries.updateContent ();

}

//------------------------------------------------------------------------------
CategoryChooserListBox::CategoryChooserListBox()
{

    addAndMakeVisible(m_listBoxView);
    m_listBoxView.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff171717));
    m_listBoxView.setModel(this);
    m_listBoxView.setRowHeight(20);
}

void CategoryChooserListBox::resized()
{
    m_listBoxView.setBounds(getLocalBounds());
}

void CategoryChooserListBox::paintListBoxItem(
        int rowNum
      , juce::Graphics& g
      , int width
      , int height
      , bool rowIsSelected)
{
    if (rowNum < 0|| rowNum >= getNumRows())
    {
        return;
    }

    juce::Rectangle<int> bounds (0,0, width, height);
    auto textColour = juce::Colours::white;
    g.setColour(juce::Colour(0xff171717));
    g.fillRect(bounds);


    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xff555555));
        g.fillRect(bounds);
    }

    if (auto entry = dynamic_cast<FileListEntry*>(m_entriesList[rowNum]))
    {
        g.setColour (entry->colour);
        auto favColourBox = bounds.removeFromLeft (20);
        favColourBox.reduce (8,8);
        g.fillRect (favColourBox);
    }

    bounds.reduce(10,0);
    g.setFont(juce::Font( m_fontTypeface->getName(), 12, juce::Font::FontStyleFlags::plain ));
    g.setColour(textColour);
    g.drawFittedText(m_entriesList[rowNum]->name, bounds, juce::Justification::left, 1);
}

void CategoryChooserListBox::addEntry(Entry* entry)
{
    m_listBoxView.deselectAllRows();
    m_entriesList.add(entry);
    m_listBoxView.updateContent();
}

void CategoryChooserListBox::selectRow(int row)
{
    if (row < 0|| row >= getNumRows())
    {
        return;
    }
    m_listBoxView.selectRow(row);
}

void CategoryChooserListBox::deselectAllRows()
{
    m_listBoxView.deselectAllRows ();
    m_listBoxView.updateContent ();
}

int CategoryChooserListBox::getNumRows()
{
    return m_entriesList.size();
}

void CategoryChooserListBox::listBoxItemClicked(int row, const juce::MouseEvent &)
{
    sendChangeMessage();
}

Entry* CategoryChooserListBox::getSelectedEntry()
{
    return m_entriesList[m_listBoxView.getLastRowSelected()];
}



//==============================================================================

SideBarBrowser::SideBarBrowser(juce::ValueTree &state, tracktion_engine::Edit &edit)
    : m_applicationState(state)
    , m_edit(edit)
    , m_fileList ()
    , m_pluginList (m_edit)
{
    addAndMakeVisible (m_tree);
    addAndMakeVisible (m_panel);
    addAndMakeVisible (m_resizerBar);
    addAndMakeVisible (m_fileList);
    addAndMakeVisible (m_pluginList);
    m_tree.setVisible (false);
    m_fileList.setVisible (false);
    m_stretchableManager.setItemLayout (0, 120, -0.9, -0.3);
    m_stretchableManager.setItemLayout (1, 1, 1, 1);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
    m_panel.getPlacesList ().addChangeListener(this);
    m_panel.getFavoritesList ().addChangeListener (this);
    m_panel.getCategoriesList ().addChangeListener (this);

    m_thread.startThread(1);
    juce::File file = juce::File::createFileWithoutCheckingPath (
                m_applicationState.getProperty (IDs::WorkDIR));
    if (!file.isDirectory ())
    {
        file = juce::File::getCurrentWorkingDirectory ();
    }
    m_panel.getPlacesList ().addEntry(
                new DirectoryEntry("Home"
              , juce::File::getSpecialLocation (juce::File::userHomeDirectory)));
    m_panel.getPlacesList ().addEntry(
                new DirectoryEntry("Projects"
              , juce::File::createFileWithoutCheckingPath (
                             m_applicationState.getProperty (IDs::ProjectsDIR))));
    m_panel.getPlacesList ().addEntry (
                new DirectoryEntry("Samples"
              , juce::File::createFileWithoutCheckingPath (
                                       m_applicationState.getProperty (IDs::ProjectsDIR))));
    m_panel.getCategoriesList ().addEntry (new PluginListEntry("Plugins"));
    m_panel.getCategoriesList ().selectRow(0);
    juce::Array<juce::File> red;
    for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator (file, false))
    {
        red.add (entry.getFile ());
    }
    auto kick = juce::File::createFileWithoutCheckingPath ("/Users/baramgb/NextStudioProjects/Samples/Drum Hits Megapack/Sample Tools by Cr2 - Deep House Drum Hits/Kicks/01 DH Kick C.wav");
    if (kick.existsAsFile ())
    {
        red.add (kick);
    }
    m_panel.getFavoritesList ().addEntry (new FileListEntry(
                                              "red",juce::Colours::red, red));
    m_dirConList.setDirectory(file, true, true);

    m_tree.addListener(this);

    m_tree.setColour (juce::TreeView::ColourIds::backgroundColourId
                      , juce::Colour(0xff171717));
    m_tree.setColour (juce::DirectoryContentsDisplayComponent::highlightColourId
                      , juce::Colour(0xff555555));
    m_tree.setItemHeight (20);
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
    auto area = getLocalBounds();

    if (m_tree.isVisible ())
    {
        Component* comps[] = {
            &m_panel
          , &m_resizerBar
          , &m_tree};
        m_stretchableManager.layOutComponents (
                    comps
                  , 3
                  , area.getX()
                  , area.getY()
                  , area.getWidth()
                  , area.getHeight()
                  , false, true);
    }
    else if (m_fileList.isVisible ())
    {
        Component* comps[] = {
            &m_panel
          , &m_resizerBar
          , &m_fileList};
        m_stretchableManager.layOutComponents (
                    comps
                  , 3
                  , area.getX()
                  , area.getY()
                  , area.getWidth()
                  , area.getHeight()
                  , false, true);
    }
    else if (m_pluginList.isVisible ())
    {
        Component* comps[] = {
            &m_panel
          , &m_resizerBar
          , &m_pluginList};
        m_stretchableManager.layOutComponents (
                    comps
                  , 3
                  , area.getX()
                  , area.getY()
                  , area.getWidth()
                  , area.getHeight()
                  , false, true);
      }
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

void SideBarBrowser::changeListenerCallback (juce::ChangeBroadcaster *source)
{
    std::cout << "changeListenerCallBAck" << std::endl;
        m_fileList.setVisible (false);
        m_pluginList.setVisible (false);
        m_tree.setVisible (false);

        if (auto chooser = dynamic_cast<CategoryChooserListBox*>(source))
        {
            if (auto entry = dynamic_cast<FileListEntry*>(chooser->getSelectedEntry ()))
            {
                std::cout << "FileList clicked " << entry->m_fileList.getFirst ().getFileName ()<< std::endl;
                m_fileList.setVisible (true);
                m_fileList.setFileList (entry->m_fileList);
            }
            else if(auto entry = dynamic_cast<DirectoryEntry*>(chooser->getSelectedEntry ()))
            {
                std::cout << "Directory clicked" << std::endl;
                m_dirConList.setDirectory (entry->directory, true, true);
                m_tree.setVisible (true);
            }
            else if (auto entry = dynamic_cast<PluginListEntry*>(chooser->getSelectedEntry ()))
            {
                std::cout << "PluginList clicked" << std::endl;
                m_pluginList.setVisible (true);
            }
            if (chooser != &m_panel.getCategoriesList ())
            {
                std::cout << "not category" << std::endl;
                m_panel.getCategoriesList ().deselectAllRows ();
            }
            if (chooser != &m_panel.getFavoritesList ())
            {
                std::cout << "not Fav" << std::endl;
                m_panel.getFavoritesList ().deselectAllRows ();
            }
            if (chooser != &m_panel.getPlacesList ())
            {
                std::cout << "not places" << std::endl;
                m_panel.getPlacesList ().deselectAllRows ();
            }
        }

        resized ();

}

