/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "SideBrowser/Browser_Base.h"
#include "SideBrowser/SearchFieldComponent.h"
#include "Utilities/Utilities.h"

PathComponent::PathComponent(juce::File dir, ApplicationViewState &appState)
    : m_currentPath(dir),
      m_appState(appState)
{
    addAndMakeVisible(m_currentPathField);
    m_currentPathField.setText(m_currentPath.getFullPathName());
    m_currentPathField.onReturnKey = [this] { setDir(juce::File(m_currentPathField.getText())); };
    addAndMakeVisible(m_button);
    m_button.onClick = [this]
    {
        GUIHelpers::log("button");
        setDir(m_currentPath.getParentDirectory());
    };
    m_currentPathField.setColour(juce::TextEditor::ColourIds::backgroundColourId, m_appState.getBackgroundColour1());
    m_currentPathField.setColour(juce::TextEditor::ColourIds::shadowColourId, m_appState.getBackgroundColour1().darker(0.3f));
    m_currentPathField.setColour(juce::TextEditor::ColourIds::outlineColourId, m_appState.getBorderColour());
    m_currentPathField.setColour(juce::TextEditor::ColourIds::textColourId, m_appState.getTextColour());
    m_currentPathField.setColour(juce::TextEditor::ColourIds::highlightColourId, m_appState.getPrimeColour());
}

void PathComponent::resized()
{
    auto area = getLocalBounds();
    auto upButton = area.removeFromRight(getHeight());
    upButton.reduce(3, 3);
    area.reduce(3, 3);
    m_currentPathField.setBounds(area);
    m_button.setBounds(upButton);
}

void PathComponent::setDir(juce::File file)
{
    if (!file.exists() || !file.isDirectory())
        return;

    if (file == m_currentPath)
        return;

    m_currentPath = file;
    m_currentPathField.setText(m_currentPath.getFullPathName());
    sendChangeMessage();
}

juce::File PathComponent::getCurrentPath() { return m_currentPath; }

// ----------------------------------------------------------------------------------------------------
//
//
// ----------------------------------------------------------------------------------------------------

juce::File BrowserListBox::getSelectedFile()
{
    auto row = getSelectedRows()[0];
    return m_browser.getContentList()[row];
}
BrowserBaseComponent::BrowserBaseComponent(ApplicationViewState &appState)
    : m_applicationViewState(appState),
      m_listBox(*this, appState),
      m_searchField(appState),
      m_currentPathField(juce::File(appState.m_workDir), appState)
{
    addAndMakeVisible(m_listBox);
    m_listBox.setModel(this);
    m_listBox.setRowHeight(20);
    addAndMakeVisible(m_searchField);
    m_searchField.addChangeListener(this);

    addAndMakeVisible(m_sortingBox);
    m_sortingBox.setWantsKeyboardFocus(false);
    m_sortingBox.addListener(this);
    addAndMakeVisible(m_sortLabel);
    m_sortLabel.setText(GUIHelpers::translate("Sort: ", m_applicationViewState), juce::dontSendNotification);
    addAndMakeVisible(m_currentPathField);
    m_currentPathField.addChangeListener(this);
}
BrowserBaseComponent::~BrowserBaseComponent()
{
    m_searchField.removeChangeListener(this);
    m_currentPathField.removeChangeListener(this);
}

void BrowserBaseComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_listBox.deselectAllRows();
    m_fileList = fileList;

    updateContentList();
}

void BrowserBaseComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto sf = dynamic_cast<SearchFieldComponent *>(source))
    {
        m_searchTerm = sf->getText();
        updateContentList();
    }
}
void BrowserBaseComponent::comboBoxChanged(juce::ComboBox *box)
{
    if (box == &m_sortingBox)
    {
        auto selectedId = m_sortingBox.getSelectedId();
        sortList(selectedId);
    }
    m_listBox.grabKeyboardFocus();
}
void BrowserBaseComponent::updateContentList()
{
    m_contentList.clear();

    for (const auto &entry : m_fileList)
        if (entry.getFileName().containsIgnoreCase(m_searchTerm))
            m_contentList.add(entry);

    auto selectedId = m_sortingBox.getSelectedId();
    sortList(selectedId);
    m_listBox.updateContent();
    repaint();
}
