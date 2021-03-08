#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PlacesList : public juce::Component
                 , public juce::ListBoxModel
                 , public juce::ChangeBroadcaster
{
public:
    struct PlacesListEntry
    {
        juce::String name;
        juce::File directory;
    };
    PlacesList();
    void resized() override;
    void paintListBoxItem(
        int rowNum
      , juce::Graphics& g
      , int width
      , int height
      , bool rowIsSelected) override;
    int getNumRows() override;
    void addEntry(const PlacesListEntry& entry);
    void selectRow(int row);
    void selectedRowsChanged(int row) override;
    void listBoxItemClicked(int row, const juce::MouseEvent &) override;
    juce::File getCurrentDir();

private: 
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
    juce::ListBox m_entries;
    juce::Array<PlacesListEntry> m_entrysList;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlacesList)
};

class BrowserPanel : public juce::Component
{
public:
    BrowserPanel()
    {
        addAndMakeVisible(m_placesList);
        addAndMakeVisible (m_placesLabel);
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
    }
    void resized() override
    {
        auto area = getLocalBounds ();
        m_placesLabel.setBounds (area.removeFromTop (20));
        m_placesList.setBounds (area);
    }

    PlacesList& getPlacesList()
    {
        return m_placesList;
    }
private:
    PlacesList                        m_placesList;
    juce::Label                       m_placesLabel{"Places", "Places"};
    juce::Typeface::Ptr m_fontTypeface{
        juce::Typeface::createSystemTypefaceFor(
                    BinaryData::IBMPlexSansRegular_ttf
                  , BinaryData::IBMPlexSansRegular_ttfSize)};
};

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
    BrowserPanel                      m_panel;
    juce::StretchableLayoutManager    m_stretchableManager;
    juce::StretchableLayoutResizerBar m_resizerBar
                                            {&m_stretchableManager, 1, true};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SideBarBrowser)
};
