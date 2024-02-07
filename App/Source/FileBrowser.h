//
// /*
//  * Copyright 2023 Steffen Baranowsky
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  * GNU General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program. If not, see <http://www.gnu.org/licenses/>.
//  */
//
#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "SearchFieldComponent.h"
#include "Utilities.h"
#include "PreviewComponent.h"

namespace te = tracktion_engine;
class FileBrowserComponent;

#include <JuceHeader.h>

class PathComponent : public juce::Component
                    , public juce::ChangeBroadcaster
{
public:
    PathComponent(juce::File dir, ApplicationViewState& appState) ;

    void paint (juce::Graphics& g) override
    {
        g.fillAll(m_appState.getBackgroundColour());
        g.setColour(m_appState.getBorderColour());
        g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
    }

    void resized() override;

    void setDir(juce::File file);
    juce::File getCurrentPath();

private:


    juce::TextEditor m_currentPathField;
    juce::TextButton m_button{"^"};
    juce::File m_currentPath;

    ApplicationViewState& m_appState;
};
// ----------------------------------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------------------------------
class FileListBox : public juce::ListBox
{
public:
FileListBox(FileBrowserComponent& sbc, ApplicationViewState& appState)
    : m_fileBrowser(sbc)
    , m_appState(appState)
{}


    juce::File getSelectedFile();


    void paintOverChildren (juce::Graphics& g) override
    {
        juce::ListBox::paintOverChildren(g);
        g.setColour(m_appState.getBorderColour());
        g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
    }

private:
    FileBrowserComponent& m_fileBrowser;
    ApplicationViewState& m_appState;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileListBox)
};
// ----------------------------------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------------------------------
class FileBrowserComponent : public juce::Component
        , public juce::ListBoxModel
        , public juce::ChangeListener
{
public:
    FileBrowserComponent(ApplicationViewState& avs, te::Engine&);
    ~FileBrowserComponent() override;
    void resized() override;
    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;
    int getNumRows() override { return m_contentList.size (); }

    juce::var getDragSourceDescription (const juce::SparseSet<int>& /*rowsToDescribe*/) override;

    juce::Array<juce::File> &getContentList() { return m_contentList; }
    void setDirecory(const juce::File& dir);
    void setFileList(const juce::Array<juce::File> &fileList);
    void sortList(bool forward=true);
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &e) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;

    void previewSampleFile(const juce::File& file);

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    juce::ListBox& getListBox(){ return m_listBox; }

private:
    struct CompareNameForward{
        static int compareElements (const juce::File& first, 
                                              const juce::File& second)
        {   
            return first.getFileName().compareNatural(second.getFileName());
        }
    };

    struct CompareNameBackwards{
        static int compareElements(const juce::File& first, 
                                               const juce::File& second)
        {
            return second.getFileName().compareNatural(first.getFileName());
        }
    };
    void sortByName(juce::Array<juce::File>& list, bool forward);
    void updateContentList();

    ApplicationViewState&       m_applicationViewState;
    SamplePreviewComponent      m_samplePreviewComponent;
    FileListBox                 m_listBox;
    juce::Array<juce::File>     m_fileList;
    juce::Array<juce::File>     m_contentList;
    SearchFieldComponent        m_searchField;

    juce::String                m_searchTerm;
    PathComponent               m_currentPathField; 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileBrowserComponent)
};
