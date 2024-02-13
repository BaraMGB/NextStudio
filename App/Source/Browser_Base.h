
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
class BrowserBaseComponent;
class BrowserListBox : public juce::ListBox
{
public:
BrowserListBox(BrowserBaseComponent& sbc, ApplicationViewState& appState)
    : m_browser(sbc)
    , m_appState(appState)
{}


    
    void paintOverChildren (juce::Graphics& g) override
    {
        g.setColour(m_appState.getBorderColour());
        g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
    }
    juce::File getSelectedFile();

private:
    BrowserBaseComponent& m_browser;
    ApplicationViewState& m_appState;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserListBox)
};
// ----------------------------------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------------------------------
class BrowserBaseComponent : public juce::Component
        , public juce::ListBoxModel
        , public juce::ChangeListener
{
public:
    BrowserBaseComponent(ApplicationViewState& avs);
    ~BrowserBaseComponent() { m_searchField.removeChangeListener(this); }
    void resized() override;
    int getNumRows() override { return m_contentList.size (); }

    juce::Array<juce::File> &getContentList() { return m_contentList; }
    void setFileList(const juce::Array<juce::File> &fileList);

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    juce::ListBox& getListBox(){ return m_listBox; }

protected:
    virtual void sortList(bool forward=true) = 0;
    BrowserListBox             m_listBox;
    juce::Array<juce::File>    m_fileList;
    juce::Array<juce::File>    m_contentList;
    SearchFieldComponent       m_searchField;
    ApplicationViewState &     m_applicationViewState;

    juce::String                m_searchTerm;
private:

    void updateContentList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrowserBaseComponent)
};
