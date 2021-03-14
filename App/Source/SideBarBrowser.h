#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class FileListBoxComponent : public juce::Component
                  , public juce::ListBoxModel

{
public:
    FileListBoxComponent()
    {
        addAndMakeVisible (m_entries);
        m_entries.setModel (this);
        m_entries.setRowHeight (20);
        m_entries.setColour (juce::ListBox::ColourIds::backgroundColourId
                             , juce::Colour(0xff171717));
    }
    void resized() override
    {
        m_entries.setBounds (getLocalBounds ());
    }
    void paintListBoxItem(
            int rowNum
          , juce::Graphics &g
          , int width
          , int height
          , bool rowIsSelected) override
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
        bounds.reduce(10,0);
        g.setFont(juce::Font( m_fontTypeface->getName(), 12, juce::Font::FontStyleFlags::plain ));
        g.setColour(textColour);
        g.drawFittedText(m_fileList[rowNum].getFileName ()
                         , bounds, juce::Justification::left, 1);
    }
    int getNumRows() override
    {
        return m_fileList.size ();
    }

    juce::var getDragSourceDescription (const juce::SparseSet<int>& rowsToDescribe) override
    {
        return juce::var ("FileListEntry");
    }

    juce::Array<juce::File> getFileList()
    {
        return m_fileList;
    }
    void setFileList(const juce::Array<juce::File> &fileList);
private:
    juce::ListBox m_entries;
    juce::Array<juce::File> m_fileList;
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};

};

//------------------------------------------------------------------------------
struct Entry
{
    Entry(juce::String n) : name (n){}
    virtual ~Entry(){}
    juce::String name;

};
struct FileListEntry : public Entry
{
    FileListEntry(juce::String entryname, juce::Colour c, juce::Array<juce::File> filelist)
        : Entry (entryname)
        , colour (c)
        , m_fileList(filelist)
    {}
    juce::Colour colour;
    juce::Array<juce::File> m_fileList;
};
struct DirectoryEntry : public Entry
{
    DirectoryEntry(juce::String n, juce::File d)
        : Entry (n)
        , directory (d)
    {}
    juce::File directory;
};
struct PluginListEntry : public Entry
{
    PluginListEntry(juce::String n)
        : Entry (n)
    {}
};

//------------------------------------------------------------------------------
class CategoryChooserListBox : public juce::Component
                 , public juce::ListBoxModel
                 , public juce::ChangeBroadcaster
{
public:

    CategoryChooserListBox();
    void resized() override;
    void paintListBoxItem(
        int rowNum
      , juce::Graphics& g
      , int width
      , int height
      , bool rowIsSelected) override;
    int getNumRows() override;
    void addEntry(Entry *entry);
    void selectRow(int row);
    void deselectAllRows();
    void listBoxItemClicked(int row, const juce::MouseEvent &) override;
    Entry *getSelectedEntry();

private: 
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
    juce::ListBox m_listBoxView;
    juce::OwnedArray<Entry> m_entriesList;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CategoryChooserListBox)
};

//------------------------------------------------------------------------------
class BrowserPanel : public juce::Component
{
public:
    BrowserPanel()
    {
        addAndMakeVisible (m_placesList);
        addAndMakeVisible (m_placesLabel);
        addAndMakeVisible (m_favLabel);
        addAndMakeVisible (m_favList);
        addAndMakeVisible (m_categoriesLabel);
        addAndMakeVisible (m_categoriesList);

        m_categoriesLabel.setFont (juce::Font(
                                     m_fontTypeface->getName()
                                   , 12
                                   , juce::Font::FontStyleFlags::plain));
        m_categoriesLabel.setColour (
                    juce::Label::backgroundColourId
                  , juce::Colour(0xff171717));
        m_placesLabel.setFont (juce::Font(
                                     m_fontTypeface->getName()
                                   , 12
                                   , juce::Font::FontStyleFlags::plain));
        m_placesLabel.setColour (
                    juce::Label::backgroundColourId
                  , juce::Colour(0xff171717));
        m_placesLabel.setColour (
                    juce::Label::textColourId
                  , juce::Colour(0xffffffff));
        m_favLabel.setFont (juce::Font(
                                     m_fontTypeface->getName()
                                   , 12
                                   , juce::Font::FontStyleFlags::plain));
        m_favLabel.setColour (
                    juce::Label::backgroundColourId
                  , juce::Colour(0xff171717));
        m_favLabel.setColour (
                    juce::Label::textColourId
                  , juce::Colour(0xffffffff));
    }
    void paint(juce::Graphics& g) override
    {
        g.setColour (juce::Colour(0xff171717));
        g.fillRect (getLocalBounds ());
    }
    void resized() override
    {
        auto area = getLocalBounds ();
        m_categoriesLabel.setBounds (area.removeFromTop (20));
        m_categoriesList.setBounds (area.removeFromTop
                                    (15 * (m_categoriesList.getNumRows () + 1)));
        m_favLabel.setBounds (area.removeFromTop (20));
        m_favList.setBounds (area.removeFromTop
                             (15 * (m_favList.getNumRows () + 1)));
        m_placesLabel.setBounds (area.removeFromTop (20));
        m_placesList.setBounds (area.removeFromTop
                                (15 * (m_placesList.getNumRows () + 1)));
    }
    void deselectAllRows()
    {
        getCategoriesList ().deselectAllRows ();
        getFavoritesList ().deselectAllRows ();
        getPlacesList ().deselectAllRows ();
    }
    CategoryChooserListBox& getCategoriesList()
    {
        return m_categoriesList;
    }
    CategoryChooserListBox& getPlacesList()
    {
        return m_placesList;
    }
    CategoryChooserListBox& getFavoritesList()
    {
        return m_favList;
    }
private:
    CategoryChooserListBox             m_categoriesList;
    juce::Label                        m_categoriesLabel{"Categories", "Categories"};
    CategoryChooserListBox             m_placesList;
    juce::Label                        m_placesLabel{"Places", "Places"};
    CategoryChooserListBox             m_favList;
    juce::Label                        m_favLabel{"Favorites", "Favorites"};
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
};


class PluginListBoxComponent : public juce::Component
                             , public juce::ListBoxModel

{
public:
    PluginListBoxComponent(te::Edit& edit)
        : m_pluginTypes(edit.engine.getPluginManager()
                        .knownPluginList.getTypes ())
        , m_edit(edit)
    {
        addAndMakeVisible (m_entries);
        m_entries.setModel (this);
        m_entries.setRowHeight (20);
        m_entries.setColour (juce::ListBox::ColourIds::backgroundColourId
                             , juce::Colour(0xff171717));
    }

    void resized() override
    {
        m_entries.setBounds (getLocalBounds ());
    }
    int getNumRows(){return m_pluginTypes.size ();}
    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
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
        bounds.reduce(10,0);
        g.setFont(juce::Font( m_fontTypeface->getName(), 12, juce::Font::FontStyleFlags::plain ));
        g.setColour(textColour);
        g.drawFittedText(m_pluginTypes[rowNum].name
                             , bounds, juce::Justification::left, 1);
    }
    juce::var getDragSourceDescription (const juce::SparseSet<int>& rowsToDescribe) override
    {
        return juce::var ("PluginListEntry");
    }

    te::Plugin::Ptr getSelectedPlugin()
    {
        return m_edit.getPluginCache ().createNewPlugin(
                    te::ExternalPlugin::xmlTypeName
                  , m_pluginTypes[m_entries.getLastRowSelected ()]);
    }
private:
    juce::Array<juce::PluginDescription> m_pluginTypes;
    te::Edit &                           m_edit;
    juce::ListBox                        m_entries;
    juce::Typeface::Ptr                  m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
};

//------------------------------------------------------------------------------
class SideBarBrowser : public juce::Component
                     , public juce::FileBrowserListener
                     , public juce::ChangeListener
{
public:
    SideBarBrowser(juce::ValueTree& state, te::Edit& edit);
    void paintOverChildren(juce::Graphics& g) override;
    void resized () override;
    void mouseDrag(const juce::MouseEvent& /*event*/) override;
    void mouseDown(const juce::MouseEvent& /*event*/) override;

    void changeListenerCallback (juce::ChangeBroadcaster *source) override;
    void selectionChanged()                           override {}
    void fileClicked (
            const juce::File& file, const juce::MouseEvent& event) override;
    void fileDoubleClicked(const juce::File&) override;
    void browserRootChanged(const juce::File&) override {}

private:

    juce::ValueTree &                 m_applicationState;
    te::Edit &                        m_edit;
    juce::TimeSliceThread             m_thread    {"file browser thread"};
    juce::DirectoryContentsList       m_dirConList{nullptr, m_thread};
    juce::FileTreeComponent           m_tree      {m_dirConList};
    FileListBoxComponent              m_fileList;
    BrowserPanel                      m_panel;
    PluginListBoxComponent            m_pluginList;
    juce::StretchableLayoutManager    m_stretchableManager;
    juce::StretchableLayoutResizerBar m_resizerBar
                                            {&m_stretchableManager, 1, true};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
};
