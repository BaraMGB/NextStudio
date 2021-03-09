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

void FavFileList::setFileList(const juce::Array<juce::File> &fileList)
{
    m_entries.deselectAllRows ();
    m_fileList = fileList;
    m_entries.updateContent ();

}

//------------------------------------------------------------------------------
LeftListBox::LeftListBox()
{

    addAndMakeVisible(m_entries);
    m_entries.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff171717));
    m_entries.setModel(this);
    m_entries.setRowHeight(20);
}

void LeftListBox::resized()
{
    m_entries.setBounds(getLocalBounds());
}

void LeftListBox::paintListBoxItem(
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

    if (auto entry = dynamic_cast<FavoritesEntry*>(m_entrysL[rowNum]))
    {
        g.setColour (entry->colour);
        auto favColourBox = bounds.removeFromLeft (20);
        favColourBox.reduce (8,8);
        g.fillRect (favColourBox);
    }

    bounds.reduce(10,0);
    g.setFont(juce::Font( m_fontTypeface->getName(), 12, juce::Font::FontStyleFlags::plain ));
    g.setColour(textColour);
    g.drawFittedText(m_entrysL[rowNum]->name, bounds, juce::Justification::left, 1);
}

void LeftListBox::addEntry(Entry entry)
{
    m_entries.deselectAllRows();
    m_entrysL.add(&entry);
    m_entries.updateContent();
}

void LeftListBox::selectRow(int row)
{
    if (row < 0|| row >= getNumRows())
    {
        return;
    }
    m_entries.selectRow(row);
}

void LeftListBox::deselectAllRows()
{
    m_entries.deselectAllRows ();
    m_entries.updateContent ();
}

int LeftListBox::getNumRows()
{
    return m_entrysL.size();
}

void LeftListBox::listBoxItemClicked(int row, const juce::MouseEvent &)
{
    sendChangeMessage();
}

Entry* LeftListBox::getSelectedEntry()
{
    return m_entrysL[m_entries.getLastRowSelected()];
}



//==============================================================================

SideBarBrowser::SideBarBrowser(juce::ValueTree &state, tracktion_engine::Edit &edit)
    : m_applicationState(state)
    , m_edit(edit)
{
    addAndMakeVisible (m_tree);
    addAndMakeVisible (m_panel);
    addAndMakeVisible (m_resizerBar);
    addAndMakeVisible (m_favList);
    m_stretchableManager.setItemLayout (0, 120, -0.9, -0.3);
    m_stretchableManager.setItemLayout (1, 1, 1, 1);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
    m_panel.getPlacesList ().addChangeListener(this);
    m_panel.getFavoritesList ().addChangeListener (this);

    m_thread.startThread(1);
    juce::File file = juce::File::createFileWithoutCheckingPath (
                m_applicationState.getProperty (IDs::WorkDIR));
    if (!file.isDirectory ())
    {
        file = juce::File::getCurrentWorkingDirectory ();
    }
    m_panel.getPlacesList ().addEntry(PlacesListEntry("Home",file));
    m_panel.getPlacesList ().addEntry(PlacesListEntry
                ("Documents", juce::File::getSpecialLocation(
                                    juce::File::commonDocumentsDirectory)));
    m_panel.getPlacesList ().selectRow(0);
    juce::Array<juce::File> red;
    for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator (file, false))
    {
        red.add (entry.getFile ());
    }

    m_panel.getFavoritesList ().addEntry (FavoritesEntry(
                                              "red",juce::Colours::red, red));
    m_dirConList.setDirectory(file, true, true);

    m_tree.addListener(this);

    m_tree.setColour (juce::TreeView::ColourIds::backgroundColourId
                      , juce::Colour(0xff171717));
    m_tree.setColour (juce::DirectoryContentsDisplayComponent::highlightColourId
                      , juce::Colour(0xff555555));
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
    else
    {
        Component* comps[] = {
            &m_panel
          , &m_resizerBar
          , &m_favList};
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
    if (source == &m_panel.getPlacesList ())
    {
        m_favList.setVisible (false);
        m_panel.getFavoritesList ().deselectAllRows ();
        if (auto entry  = dynamic_cast<PlacesListEntry*>(
                    m_panel.getPlacesList ().getSelectedEntry ()))
        {
            m_dirConList.setDirectory(entry->directory, true, true);
        }
        m_tree.setVisible (true);
    }
    if (source == &m_panel.getFavoritesList ())
    {
        m_panel.getPlacesList ().deselectAllRows();
        if (auto entry  = dynamic_cast<FavoritesEntry*>(
                    m_panel.getFavoritesList ().getSelectedEntry ()))
        {
            m_favList.setFileList (entry->m_fileList);
        }
        m_tree.setVisible (false);
        m_favList.setVisible (true);
        resized ();
    }
}

