#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

struct CategorieListBoxEntry
{
    CategorieListBoxEntry(juce::String n) : name (n){}
    virtual ~CategorieListBoxEntry(){}
    juce::String name;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CategorieListBoxEntry)
};
struct FileListCategorieEntry : public CategorieListBoxEntry
                              , public te::ValueTreeAllEventListener
                              , public juce::ChangeBroadcaster
{
    FileListCategorieEntry(
            juce::String entryname
          , juce::Colour c
          , juce::Array<juce::File> filelist, ApplicationViewState& avs)
        : CategorieListBoxEntry (entryname)
        , colour (c)
        , m_fileList(filelist)
        , m_applicationViewState(avs)
    {
        m_applicationViewState.m_applicationStateValueTree.addListener (this);
    }
    void updateFileList()
    {
        m_fileList.clear ();

        for (auto &redFavs : m_applicationViewState.m_red)
        {
            m_fileList.add (redFavs->getFile ());
        }
        sendChangeMessage ();
    }
    ApplicationViewState& m_applicationViewState;
    juce::Colour colour;
    juce::Array<juce::File> m_fileList;

    void valueTreeChildAdded(
            juce::ValueTree &parentTree
          , juce::ValueTree &child)
    {
        if(child.hasType (IDs::red))
        {
            updateFileList ();
        }
    }
    void valueTreeChildRemoved(
            juce::ValueTree &parentTree
          , juce::ValueTree &child
          , int indexFromWhichChildWasRemoved)
    {
        if(child.hasType (IDs::red))
        {
            updateFileList ();
        }
    }

    void valueTreeChanged(){}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileListCategorieEntry)
};
struct DirectoryCategorieEntry : public CategorieListBoxEntry
{
    DirectoryCategorieEntry(juce::String n, juce::File d)
        : CategorieListBoxEntry (n)
        , directory (d)
    {}
    juce::File directory;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectoryCategorieEntry)
};
struct PluginListCategorieEntry : public CategorieListBoxEntry
{
    PluginListCategorieEntry(juce::String n)
        : CategorieListBoxEntry (n)
    {}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListCategorieEntry)
};


class FileListBoxComponent : public juce::Component
                           , public juce::ListBoxModel
                           , public juce::ChangeListener
{
public:
    FileListBoxComponent(ApplicationViewState& avs)
        : m_applicationViewState(avs)
    {
        addAndMakeVisible (m_entries);
        m_entries.setModel (this);
        m_entries.setRowHeight (20);
        m_entries.setColour (
                    juce::ListBox::ColourIds::backgroundColourId
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
        g.setFont(juce::Font(
                      m_fontTypeface->getName()
                    , 12
                    , juce::Font::FontStyleFlags::plain ));
        g.setColour(textColour);
        g.drawFittedText(
                    m_fileList[rowNum].getFileName ()
                  , bounds, juce::Justification::left, 1);
    }
    int getNumRows() override
    {
        return m_fileList.size ();
    }

    juce::var getDragSourceDescription (
            const juce::SparseSet<int>& rowsToDescribe) override
    {
        return juce::var ("FileListEntry");
    }

    juce::Array<juce::File> &getFileList()
    {
        return m_fileList;
    }
    void setFileList(const juce::Array<juce::File> &fileList)
    {
        m_entries.deselectAllRows ();
        m_fileList = fileList;
        m_entries.updateContent ();
    }
    void listBoxItemClicked(int row, const juce::MouseEvent &e)
    {
        if (e.mods.isRightButtonDown ())
        {
            juce::PopupMenu p;
            p.addItem (1, "remove from Favorites");
            const int result = p.show();
            if(result == 1)
            {
                m_applicationViewState.removeFileFromFavorite (getFileList ()[row]);

                m_entries.updateContent ();
            }
        }
    }

    void changeListenerCallback(juce::ChangeBroadcaster *source)
    {
        if(auto entry = dynamic_cast<FileListCategorieEntry*>(source))
        {
            m_fileList = entry->m_fileList;
            m_entries.updateContent ();
        }
    }
private:
    ApplicationViewState& m_applicationViewState;
    juce::ListBox m_entries;
    juce::Array<juce::File> m_fileList;
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileListBoxComponent)

};
//------------------------------------------------------------------------------
class CategoryChooserListBox : public juce::Component
                 , public juce::ListBoxModel
                 , public juce::ChangeBroadcaster
{
public:
    CategoryChooserListBox()
    {

        addAndMakeVisible(m_listBoxView);
        m_listBoxView.setColour(
                    juce::ListBox::backgroundColourId
                    , juce::Colour(0xff171717));
        m_listBoxView.setModel(this);
        m_listBoxView.setRowHeight(20);
    }
    void resized() override
    {
        m_listBoxView.setBounds(getLocalBounds());
    }
    void paintListBoxItem(
            int rowNum
            , juce::Graphics& g
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

        if (auto entry = dynamic_cast<FileListCategorieEntry*>(m_entriesList[rowNum]))
        {
            g.setColour (entry->colour);
            auto favColourBox = bounds.removeFromLeft (20);
            favColourBox.reduce (8,8);
            g.fillRect (favColourBox);
        }

        bounds.reduce(10,0);
        g.setFont(juce::Font(
                      m_fontTypeface->getName()
                      , 12
                      , juce::Font::FontStyleFlags::plain ));
        g.setColour(textColour);
        g.drawFittedText(
                    m_entriesList[rowNum]->name
                    , bounds
                    , juce::Justification::left
                    , 1);
    }
    int getNumRows() override
    {
        return m_entriesList.size();
    }
    void addEntry(CategorieListBoxEntry *entry)
    {
        m_listBoxView.deselectAllRows();
        m_entriesList.add(entry);
        m_listBoxView.updateContent();
    }
    void selectRow(int row)
    {
        if (row < 0|| row >= getNumRows())
        {
            return;
        }
        m_listBoxView.selectRow(row);
    }
    void deselectAllRows()
    {
        m_listBoxView.deselectAllRows ();
        m_listBoxView.updateContent ();
    }
    void listBoxItemClicked(int row, const juce::MouseEvent &) override
    {
        sendChangeMessage();
    }
    CategorieListBoxEntry *getSelectedEntry()
    {
        return m_entriesList[m_listBoxView.getLastRowSelected()];
    }

private: 
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
    juce::ListBox m_listBoxView;
    juce::OwnedArray<CategorieListBoxEntry> m_entriesList;
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
        m_categoriesList.setBounds (area.removeFromTop(
                                    15 * (m_categoriesList.getNumRows () + 1)));
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
    CategoryChooserListBox        m_categoriesList;
    juce::Label                   m_categoriesLabel{"Categories", "Categories"};
    CategoryChooserListBox        m_placesList;
    juce::Label                   m_placesLabel{"Places", "Places"};
    CategoryChooserListBox        m_favList;
    juce::Label                   m_favLabel{"Favorites", "Favorites"};
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserPanel)
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
    void paintListBoxItem(
            int rowNum
          , juce::Graphics &g
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
        bounds.reduce(10,0);
        g.setFont(juce::Font(
                      m_fontTypeface->getName()
                    , 12
                    , juce::Font::FontStyleFlags::plain ));
        g.setColour(textColour);
        g.drawFittedText(
                    m_pluginTypes[rowNum].name
                  , bounds
                  , juce::Justification::left
                  , 1);
    }
    juce::var getDragSourceDescription (
            const juce::SparseSet<int>& rowsToDescribe) override
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxComponent)
};

//------------------------------------------------------------------------------
class SideBarBrowser : public juce::Component
                     , public juce::FileBrowserListener
                     , public juce::ChangeListener
{
public:

    SideBarBrowser(ApplicationViewState& state, te::Edit& edit)
        : m_applicationState(state)
        , m_edit(edit)
        , m_CollectedFilesListBox (state)
        , m_pluginListBox (m_edit)
    {
        addAndMakeVisible (m_DirTreeViewBox);
        addAndMakeVisible (m_browserSidepanel);
        addAndMakeVisible (m_resizerBar);
        addAndMakeVisible (m_CollectedFilesListBox);
        addAndMakeVisible (m_pluginListBox);

        m_DirTreeViewBox.setVisible (false);
        m_CollectedFilesListBox.setVisible (false);
        m_stretchableManager.setItemLayout (0, 120, -0.9, -0.3);
        m_stretchableManager.setItemLayout (1, 1, 1, 1);
        m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
        m_browserSidepanel.getPlacesList ().addChangeListener(this);
        m_browserSidepanel.getFavoritesList ().addChangeListener (this);
        m_browserSidepanel.getCategoriesList ().addChangeListener (this);


        //
        setupPlaces();

        //
        setuCategories();

        //
        setupFavorites();

        //
        setupDirectoryTreeView();
    }
    void paintOverChildren(juce::Graphics& g) override
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
    void resized () override
    {
        auto area = getLocalBounds();

        if (m_DirTreeViewBox.isVisible ())
        {
            Component* comps[] = {
                &m_browserSidepanel
                , &m_resizerBar
                , &m_DirTreeViewBox};
            m_stretchableManager.layOutComponents (
                        comps
                        , 3
                        , area.getX()
                        , area.getY()
                        , area.getWidth()
                        , area.getHeight()
                        , false, true);
        }
        else if (m_CollectedFilesListBox.isVisible ())
        {
            Component* comps[] = {
                &m_browserSidepanel
                , &m_resizerBar
                , &m_CollectedFilesListBox};
            m_stretchableManager.layOutComponents (
                        comps
                        , 3
                        , area.getX()
                        , area.getY()
                        , area.getWidth()
                        , area.getHeight()
                        , false, true);
        }
        else if (m_pluginListBox.isVisible ())
        {
            Component* comps[] = {
                &m_browserSidepanel
                , &m_resizerBar
                , &m_pluginListBox};
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
    void mouseDrag(const juce::MouseEvent& /*event*/) override
    {
        auto dragC = juce::DragAndDropContainer::findParentDragContainerFor(this);
        if (!dragC->isDragAndDropActive())
        {
            dragC->startDragging("Test", this);
        }
    }

    void changeListenerCallback (juce::ChangeBroadcaster *source) override
    {
        m_CollectedFilesListBox.setVisible (false);
        m_pluginListBox.setVisible (false);
        m_DirTreeViewBox.setVisible (false);

        if (auto chooser = dynamic_cast<CategoryChooserListBox*>(source))
        {
            if (auto entry = dynamic_cast<FileListCategorieEntry*>(chooser->getSelectedEntry ()))
            {
                m_CollectedFilesListBox.setVisible (true);
                m_CollectedFilesListBox.setFileList (entry->m_fileList);
            }
            else if(auto entry = dynamic_cast<DirectoryCategorieEntry*>(chooser->getSelectedEntry ()))
            {
                m_dirConList.setDirectory (entry->directory, true, true);
                m_DirTreeViewBox.setVisible (true);
            }
            else if (auto entry = dynamic_cast<PluginListCategorieEntry*>(chooser->getSelectedEntry ()))
            {
                std::cout << "PluginList clicked" << std::endl;
                m_pluginListBox.setVisible (true);
            }
            if (chooser != &m_browserSidepanel.getCategoriesList ())
            {
                m_browserSidepanel.getCategoriesList ().deselectAllRows ();
            }
            if (chooser != &m_browserSidepanel.getFavoritesList ())
            {
                m_browserSidepanel.getFavoritesList ().deselectAllRows ();
            }
            if (chooser != &m_browserSidepanel.getPlacesList ())
            {
                m_browserSidepanel.getPlacesList ().deselectAllRows ();
            }
        }
        resized ();
    }
    void selectionChanged()                           override {}
    void fileClicked (
            const juce::File& file, const juce::MouseEvent& event) override
    {
        if (event.mods.isRightButtonDown ())
        {
            juce::PopupMenu p;
            p.addItem (1, "add to Red");

            const int result = p.show ();

            if (result == 1)
            {
                m_applicationState.addFileToFavorites (file);
            }
        }
        m_DirTreeViewBox.setDragAndDropDescription(file.getFileName());
    }
    void fileDoubleClicked(const juce::File&) override;
    void browserRootChanged(const juce::File&) override {}

private:
    inline void setupPlaces()
    {
        m_browserSidepanel.getPlacesList ().addEntry(
                    new DirectoryCategorieEntry("Home"
              , juce::File::getSpecialLocation (juce::File::userHomeDirectory)));
        m_browserSidepanel.getPlacesList ().addEntry(
                    new DirectoryCategorieEntry("Projects"
              , juce::File::createFileWithoutCheckingPath (
                                           m_applicationState.m_projectsDir)));
        m_browserSidepanel.getPlacesList ().addEntry (
                    new DirectoryCategorieEntry("Samples"
              , juce::File::createFileWithoutCheckingPath (
                                           m_applicationState.m_samplesDir)));
    }

    inline void setuCategories()
    {
        m_browserSidepanel.getCategoriesList ().addEntry (new PluginListCategorieEntry("Plugins"));
        m_browserSidepanel.getCategoriesList ().selectRow(0);
    }

    inline void setupFavorites()
    {
        juce::Array<juce::File> red;
        for (auto &redFavs : m_applicationState.m_red)
        {
            red.add (redFavs->getFile ());
        }
        auto redEntry = new FileListCategorieEntry(
                    "red",juce::Colours::red, red, m_applicationState);
        redEntry->addChangeListener (&m_CollectedFilesListBox);
        m_browserSidepanel.getFavoritesList ().addEntry (redEntry);
    }

    inline void setupDirectoryTreeView()
    {
        m_thread.startThread(1);
        juce::File file = juce::File::createFileWithoutCheckingPath (
                    m_applicationState.m_workDir);
        if (!file.isDirectory ())
        {
            file = juce::File::getCurrentWorkingDirectory ();
        }
        m_dirConList.setDirectory(file, true, true);
        m_DirTreeViewBox.addListener(this);
        m_DirTreeViewBox.setColour (juce::TreeView::ColourIds::backgroundColourId
                          , juce::Colour(0xff171717));
        m_DirTreeViewBox.setColour (juce::DirectoryContentsDisplayComponent::highlightColourId
                          , juce::Colour(0xff555555));
        m_DirTreeViewBox.setItemHeight (20);
    }

    ApplicationViewState &            m_applicationState;
    te::Edit &                        m_edit;
    juce::TimeSliceThread             m_thread    {"file browser thread"};
    juce::DirectoryContentsList       m_dirConList{nullptr, m_thread};
    juce::FileTreeComponent           m_DirTreeViewBox      {m_dirConList};
    FileListBoxComponent              m_CollectedFilesListBox;
    BrowserPanel                      m_browserSidepanel;
    PluginListBoxComponent            m_pluginListBox;
    juce::StretchableLayoutManager    m_stretchableManager;
    juce::StretchableLayoutResizerBar m_resizerBar
                                            {&m_stretchableManager, 1, true};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
};
