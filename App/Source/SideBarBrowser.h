#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"
#include "PreviewComponent.h"

namespace te = tracktion_engine;

    
struct CategoryListBoxEntry
{
    explicit CategoryListBoxEntry(juce::String n) : name (std::move(n)){}
    virtual ~CategoryListBoxEntry()= default;
    juce::String name;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CategoryListBoxEntry)
};
struct FileListCategoryEntry
    : public CategoryListBoxEntry
    , public te::ValueTreeAllEventListener
                              , public juce::ChangeBroadcaster
{
    FileListCategoryEntry(
            juce::String en, juce::Colour c
          , juce::Identifier tag
          , juce::Array<juce::File> files, ApplicationViewState& avs)
        : CategoryListBoxEntry(std::move(en))
        , m_applicationViewState(avs)
        , colour (c)
        , m_fileList(std::move(files))
        , m_tag (std::move(tag))
    {
        m_applicationViewState.m_applicationStateValueTree.addListener (this);
    }
    ~FileListCategoryEntry() override
    {
        m_applicationViewState.m_applicationStateValueTree.removeListener (this);
    }
    void updateFileList()
    {
        m_fileList.clear ();

        for (auto &fav : m_applicationViewState.m_favorites)
        {
            if (fav->m_tag == m_tag)
            {
                m_fileList.add (fav->getFile ());
            }
        }
        //sendChangeMessage ();
    }
    ApplicationViewState& m_applicationViewState;
    juce::Colour colour;
    juce::Array<juce::File> m_fileList;
    juce::Identifier m_tag;

    void valueTreeChildAdded(
            juce::ValueTree& /*parentTree*/
          , juce::ValueTree &child) override
    {
        if(child.hasType (m_tag))
        {
            updateFileList ();
        }
    }
    void valueTreeChildRemoved(
            juce::ValueTree& /*parentTree*/
          , juce::ValueTree &child
          , int /*indexFromWhichChildWasRemoved*/) override
    {
        if(child.hasType (m_tag))
        {
            updateFileList ();
        }
    }

    void valueTreeChanged() override {}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileListCategoryEntry)
};
struct DirectoryCategoryEntry : public CategoryListBoxEntry
{
    DirectoryCategoryEntry(juce::String n, juce::File d)
        : CategoryListBoxEntry(std::move(n))
        , directory (std::move(d))
    {}
    juce::File directory;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectoryCategoryEntry)
};
struct PluginListCategoryEntry : public CategoryListBoxEntry
{
    explicit PluginListCategoryEntry(juce::String n)
        : CategoryListBoxEntry(std::move(n))
    {}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListCategoryEntry)
};


class FileListBoxComponent : public juce::Component
                           , public juce::ListBoxModel
                           , public juce::ChangeListener
{
public:
    FileListBoxComponent(ApplicationViewState& avs, SamplePreviewComponent& spc)
        : m_applicationViewState(avs)
        , m_samplePreviewComponent(spc)
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
//        g.setFont(juce::Font(
//                      m_fontTypeface->getName()
//                    , 12
//                    , juce::Font::FontStyleFlags::plain ));
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
            const juce::SparseSet<int>& /*rowsToDescribe*/) override
    {
        return {"FileListEntry"};
    }

    juce::Array<juce::File> &getFileList()
    {
        return m_fileList;
    }
    void setFileList(juce::Identifier tag, const juce::Array<juce::File> &fileList)
    {
        m_tag = std::move(tag);
        m_entries.deselectAllRows ();
        m_fileList = fileList;
        m_entries.updateContent ();
    }
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override
    {
        if (e.mods.isRightButtonDown ())
        {
            juce::PopupMenu p;
            p.addItem (1, "remove from Favorites");
            const int result = p.show();
            if(result == 1)
            {

                m_fileList = m_applicationViewState.removeFileFromFavorite (
                            m_tag
                          , getFileList ()[row]);
                m_entries.updateContent ();
              }
        }

    }
      void selectedRowsChanged(int /*lastRowSelected*/) override
      {
          previewSampleFile (m_fileList[m_entries.getSelectedRow ()]);
      }

      inline void previewSampleFile(const juce::File& file)
      {
          if (m_samplePreviewComponent.setFile (file))
          {
              m_samplePreviewComponent.rewind();
              m_samplePreviewComponent.play();
          }
      }

      void changeListenerCallback(juce::ChangeBroadcaster *source) override
      {
          if(auto entry = dynamic_cast<FileListCategoryEntry*>(source))
          {
              m_fileList = entry->m_fileList;
              m_tag = entry->m_tag;
              m_entries.updateContent ();
          }
      }

  private:
      ApplicationViewState &     m_applicationViewState;
      SamplePreviewComponent &   m_samplePreviewComponent;
      juce::ListBox              m_entries;
      juce::Array<juce::File>    m_fileList;
      juce::Identifier           m_tag;
//      juce::Typeface::Ptr        m_fontTypeface{
//          juce::Typeface::createSystemTypefaceFor(
//                      BinaryData::IBMPlexSansRegular_ttf
//                    , BinaryData::IBMPlexSansRegular_ttfSize)};
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

          if (auto entry = dynamic_cast<FileListCategoryEntry*>(m_entriesList[rowNum]))
          {
              g.setColour (entry->colour);
              auto favColourBox = bounds.removeFromLeft (20);
              favColourBox.reduce (8,8);
              g.fillRect (favColourBox);
          }

          bounds.reduce(10,0);
//          g.setFont(juce::Font(
//                        m_fontTypeface->getName()
//                        , 12
//                        , juce::Font::FontStyleFlags::plain ));
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
      void addEntry(CategoryListBoxEntry*entry)
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
      void listBoxItemClicked(int /*row*/, const juce::MouseEvent &) override
      {
          sendChangeMessage();
      }
      CategoryListBoxEntry*getSelectedEntry()
      {
          return m_entriesList[m_listBoxView.getLastRowSelected()];
      }

  private:
//      juce::Typeface::Ptr m_fontTypeface{
//          juce::Typeface::createSystemTypefaceFor(
//                      BinaryData::IBMPlexSansRegular_ttf
//                    , BinaryData::IBMPlexSansRegular_ttfSize)};
      juce::ListBox m_listBoxView;
      juce::OwnedArray<CategoryListBoxEntry> m_entriesList;
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

//          m_categoriesLabel.setFont (juce::Font(
//                                       m_fontTypeface->getName()
//                                     , 12
//                                     , juce::Font::FontStyleFlags::plain));
          m_categoriesLabel.setColour (
                      juce::Label::backgroundColourId
                    , juce::Colour(0xff171717));
//          m_placesLabel.setFont (juce::Font(
//                                       m_fontTypeface->getName()
//                                     , 12
//                                     , juce::Font::FontStyleFlags::plain));
          m_placesLabel.setColour (
                      juce::Label::backgroundColourId
                    , juce::Colour(0xff171717));
          m_placesLabel.setColour (
                      juce::Label::textColourId
                    , juce::Colour(0xffffffff));
//          m_favLabel.setFont (juce::Font(
//                                       m_fontTypeface->getName()
//                                     , 12
//                                     , juce::Font::FontStyleFlags::plain));
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
      [[maybe_unused]] void deselectAllRows()
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
//      juce::Typeface::Ptr m_fontTypeface{
//          juce::Typeface::createSystemTypefaceFor(
//                      BinaryData::IBMPlexSansRegular_ttf
//                    , BinaryData::IBMPlexSansRegular_ttfSize)};
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserPanel)
  };


  class PluginListBoxComponent : public juce::Component
                               , public juce::ListBoxModel

  {
  public:
      explicit PluginListBoxComponent(te::Edit& edit)
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

      int getNumRows() override {return m_pluginTypes.size ();}

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
//          g.setFont(juce::Font(
//                        m_fontTypeface->getName()
//                      , 12
//                      , juce::Font::FontStyleFlags::plain ));
          g.setColour(textColour);
          g.drawFittedText(
                      m_pluginTypes[rowNum].name
                    , bounds
                    , juce::Justification::left
                    , 1);
      }
      juce::var getDragSourceDescription (
              const juce::SparseSet<int>& /*rowsToDescribe*/) override
      {
          return {"PluginListEntry"};
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
//      juce::Typeface::Ptr                  m_fontTypeface{
//          juce::Typeface::createSystemTypefaceFor(
//                      BinaryData::IBMPlexSansRegular_ttf
//                    , BinaryData::IBMPlexSansRegular_ttfSize)};
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginListBoxComponent)
  };

  //------------------------------------------------------------------------------
  class SideBarBrowser : public juce::Component
                       , public juce::FileBrowserListener
                       , public juce::ChangeListener
  {
  public:

      SideBarBrowser(ApplicationViewState& state, EditViewState& evs)
          : m_applicationState(state)
          , m_editViewState(evs)
          , m_edit(evs.m_edit)
          , m_CollectedFilesListBox (state, m_samplePreviewComponent)
          , m_samplePreviewComponent(m_edit, state)
          , m_pluginListBox (m_edit)
      {
          addAndMakeVisible (m_DirTreeViewBox);
          addAndMakeVisible (m_browserSidePanel);
          addAndMakeVisible (m_resizerBar);
          addAndMakeVisible (m_CollectedFilesListBox);
          addAndMakeVisible (m_pluginListBox);
          addAndMakeVisible (m_samplePreviewComponent);

          m_DirTreeViewBox.setVisible (false);
          m_CollectedFilesListBox.setVisible (false);
          m_stretchableManager.setItemLayout (0, 120, -0.9, -0.3);
          m_stretchableManager.setItemLayout (1, 1, 1, 1);
          m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
          m_browserSidePanel.getPlacesList ().addChangeListener(this);
          m_browserSidePanel.getFavoritesList ().addChangeListener (this);
          m_browserSidePanel.getCategoriesList ().addChangeListener (this);


          //
          setupPlaces();

          //
          setupCategories();

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
          auto samplePreviewBounds = area.removeFromBottom(75);

          if (m_DirTreeViewBox.isVisible ())
          {
              Component* comps[] = {
                  &m_browserSidePanel, &m_resizerBar
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
                  &m_browserSidePanel, &m_resizerBar
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
                  &m_browserSidePanel, &m_resizerBar
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
          m_samplePreviewComponent.setBounds (samplePreviewBounds);
      }

      void changeListenerCallback (juce::ChangeBroadcaster *source) override
      {
          m_CollectedFilesListBox.setVisible (false);
          m_pluginListBox.setVisible (false);
          m_DirTreeViewBox.setVisible (false);

          if (auto chooser = dynamic_cast<CategoryChooserListBox*>(source))
          {
              if (auto fle = dynamic_cast<FileListCategoryEntry*>(chooser->getSelectedEntry ()))
              {
                  m_CollectedFilesListBox.setVisible (true);
                  m_CollectedFilesListBox.setFileList (fle->m_tag, fle->m_fileList);
              }
              else if(auto dce = dynamic_cast<DirectoryCategoryEntry*>(chooser->getSelectedEntry ()))
              {
                  m_dirConList.setDirectory (dce->directory, true, true);
                  m_DirTreeViewBox.setVisible (true);
              }
              else if (dynamic_cast<PluginListCategoryEntry*>(chooser->getSelectedEntry ()))
              {
                  m_pluginListBox.setVisible (true);
              }

              if (chooser != &m_browserSidePanel.getCategoriesList ())
              {
                  m_browserSidePanel.getCategoriesList ().deselectAllRows ();
              }
              if (chooser != &m_browserSidePanel.getFavoritesList ())
              {
                  m_browserSidePanel.getFavoritesList ().deselectAllRows ();
              }
              if (chooser != &m_browserSidePanel.getPlacesList ())
              {
                  m_browserSidePanel.getPlacesList ().deselectAllRows ();
              }
          }
          resized ();
      }
      void selectionChanged()                           override
      {
          previewSampleFile (m_DirTreeViewBox.getSelectedFile ());
      }
      inline void previewSampleFile(const juce::File& file)
      {
          if (m_samplePreviewComponent.setFile (file))
          {
              m_samplePreviewComponent.rewind();
              m_samplePreviewComponent.play();
          }
      }

      void fileClicked (
              const juce::File& file, const juce::MouseEvent& event) override
      {
          if (event.mods.isRightButtonDown ())
          {
              juce::PopupMenu p;
              auto favoriteTypes = m_applicationState.getFavoriteTypeList ();
              auto menuEntry = 1;
              for (const auto& type : favoriteTypes)
              {
                  p.addItem (menuEntry, "add to " + juce::String(
                                 type.toString ()));
                  menuEntry++;
              }
              const int result = p.show ();
              m_applicationState.addFileToFavorites (favoriteTypes[result - 1]
                                                   , file);
          }
          else
          {
              m_DirTreeViewBox.setDragAndDropDescription(file.getFileName());
          }
      }
      void fileDoubleClicked(const juce::File&) override;
      void browserRootChanged(const juce::File&) override {}


  private:
      inline void setupPlaces()
      {
          m_browserSidePanel.getPlacesList ().addEntry(
                      new DirectoryCategoryEntry("Home"
                , juce::File::getSpecialLocation (juce::File::userHomeDirectory)));
          m_browserSidePanel.getPlacesList ().addEntry(
                      new DirectoryCategoryEntry("Projects"
                , juce::File::createFileWithoutCheckingPath (
                                             m_applicationState.m_projectsDir)));
          m_browserSidePanel.getPlacesList ().addEntry (
                      new DirectoryCategoryEntry("Samples"
                , juce::File::createFileWithoutCheckingPath (
                                             m_applicationState.m_samplesDir)));
      }

      inline void setupCategories()
      {
          m_browserSidePanel.getCategoriesList ().addEntry (new PluginListCategoryEntry("Plugins"));
          m_browserSidePanel.getCategoriesList ().selectRow(0);
      }

      inline void addFavoritesEntry(
              const juce::Identifier& type
            , juce::Colour colour
            , juce::String name)
      {
          juce::Array<juce::File> favoritesList;
          for (auto &favorite : m_applicationState.m_favorites)
          {
              if (favorite->m_tag == type)
              {
                  favoritesList.add (favorite->getFile ());
              }
          }
          m_applicationState.addFavoriteType (type);
          auto entry = new FileListCategoryEntry(
                      std::move(name)
                    , colour
                    , type
                    , favoritesList
                    , m_applicationState);
          m_browserSidePanel.getFavoritesList ().addEntry (entry);
      }

      inline void setupFavorites()
      {
          addFavoritesEntry(IDs::red, juce::Colours::red, "favorite basses");
          addFavoritesEntry(IDs::green, juce::Colours::green, "my own samples");
          addFavoritesEntry (IDs::orange, juce::Colours::orange, "orange list");
      }

      inline void setupDirectoryTreeView()
      {
          m_thread.startThread();
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
      EditViewState&                    m_editViewState;
      te::Edit &                        m_edit;

      FileListBoxComponent              m_CollectedFilesListBox;
      SamplePreviewComponent            m_samplePreviewComponent;
      juce::TimeSliceThread             m_thread    {"file browser thread"};
      juce::DirectoryContentsList       m_dirConList{nullptr, m_thread};
      juce::FileTreeComponent           m_DirTreeViewBox      {m_dirConList};
      BrowserPanel                      m_browserSidePanel;
      PluginListBoxComponent            m_pluginListBox;
      juce::StretchableLayoutManager    m_stretchableManager;
      juce::StretchableLayoutResizerBar m_resizerBar
                                              {&m_stretchableManager, 1, true};
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
  };

