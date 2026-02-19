
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
#include "SideBrowser/SearchFieldComponent.h"
#include "UI/PreviewComponent.h"
#include "Utilities/ApplicationViewState.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

class PathComponent
    : public juce::Component
    , public juce::ChangeBroadcaster
{
public:
    PathComponent(juce::File dir, ApplicationViewState &appState);

    void paint(juce::Graphics &g) override
    {
        g.fillAll(m_appState.getBackgroundColour2());
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

    ApplicationViewState &m_appState;
};
// ----------------------------------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------------------------------

class BrowserBaseComponent;
class BrowserListBox : public juce::ListBox
{
public:
    BrowserListBox(BrowserBaseComponent &sbc, ApplicationViewState &appState)
        : m_browser(sbc),
          m_appState(appState)
    {
    }

    juce::File getSelectedFile();

private:
    BrowserBaseComponent &m_browser;
    ApplicationViewState &m_appState;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserListBox)
};
// ----------------------------------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------------------------------
class BrowserBaseComponent
    : public juce::Component
    , public juce::ChangeBroadcaster
    , private juce::ComboBox::Listener
    , public juce::ListBoxModel
    , public juce::ChangeListener
{
public:
    BrowserBaseComponent(ApplicationViewState &avs);
    ~BrowserBaseComponent() override;

    void paint(juce::Graphics &g) override
    {
        g.fillAll(m_applicationViewState.getBackgroundColour2());
        g.setColour(m_applicationViewState.getBorderColour());
        g.drawHorizontalLine(m_sortingBox.getY() + m_sortingBox.getHeight(), 0, getWidth());
        g.drawHorizontalLine(m_searchField.getY(), 0, getWidth());
    }
    int getNumRows() override { return m_contentList.size(); }

    juce::Array<juce::File> &getContentList() { return m_contentList; }
    void setFileList(const juce::Array<juce::File> &fileList);

    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    juce::ListBox &getListBox() { return m_listBox; }

    juce::File m_projectToLoad{};

protected:
    virtual void sortList(int selectedID) = 0;
    void updateContentList();
    BrowserListBox m_listBox;
    juce::Array<juce::File> m_fileList;
    juce::Array<juce::File> m_contentList;
    SearchFieldComponent m_searchField;
    juce::ComboBox m_sortingBox;
    juce::Label m_sortLabel;
    PathComponent m_currentPathField;
    ApplicationViewState &m_applicationViewState;

    juce::String m_searchTerm;

private:
    void comboBoxChanged(juce::ComboBox *box) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserBaseComponent)
};
