
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
class ProjectsBrowserComponent;
class ProjectsListBox : public juce::ListBox
{
public:
ProjectsListBox(ProjectsBrowserComponent& sbc, ApplicationViewState& appState)
    : m_projectsBrowser(sbc)
    , m_appState(appState)
{}


    
    void paintOverChildren (juce::Graphics& g) override
    {
        g.setColour(m_appState.getBorderColour());
        g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
    }
    juce::File getSelectedProject();

private:
    ProjectsBrowserComponent& m_projectsBrowser;
    ApplicationViewState& m_appState;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectsListBox)
};
// ----------------------------------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------------------------------
class ProjectsBrowserComponent : public juce::Component
        , public juce::ListBoxModel
        , public juce::ChangeListener
{
public:
    ProjectsBrowserComponent(ApplicationViewState& avs);
    void resized() override;
    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;
    int getNumRows() override { return m_contentList.size (); }

    juce::var getDragSourceDescription (const juce::SparseSet<int>& /*rowsToDescribe*/) override;

    juce::Array<juce::File> &getContentList() { return m_contentList; }
    void setFileList(const juce::Array<juce::File> &fileList);
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;
    void selectedRowsChanged(int /*lastRowSelected*/) override;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    juce::ListBox& getListBox(){ return m_listBox; }

private:
    void updateContentList();

    ApplicationViewState &     m_applicationViewState;
    ProjectsListBox              m_listBox;
    juce::Array<juce::File>    m_fileList;
    juce::Array<juce::File>    m_contentList;
    SearchFieldComponent       m_searchField;

    juce::String                m_searchTerm;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectsBrowserComponent)
};
