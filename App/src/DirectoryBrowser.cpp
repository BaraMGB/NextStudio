/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "DirectoryBrowser.h"
#include "Utilities.h"

namespace
{
struct CompareEntries
{
    bool forwards{true};

    int compareElements(const juce::File &first, const juce::File &second) const
    {
        if (first.isDirectory() != second.isDirectory())
            return first.isDirectory() ? -1 : 1;

        const auto comparison = first.getFileName().compareNatural(second.getFileName());
        return forwards ? comparison : -comparison;
    }
};
} // namespace

DirectoryBrowserComponent::DirectoryBrowserComponent(ApplicationViewState &avs)
    : BrowserBaseComponent(avs)
{
    addAndMakeVisible(m_backButton);
    addAndMakeVisible(m_forwardButton);
    m_backButton.setTooltip("Previous folder");
    m_forwardButton.setTooltip("Next folder");
    m_backButton.onClick = [this] { navigateBack(); };
    m_forwardButton.onClick = [this] { navigateForward(); };

    m_sortingBox.addItem(GUIHelpers::translate("by Name (a - z)", m_applicationViewState), 1);
    m_sortingBox.addItem(GUIHelpers::translate("by Name (z - a)", m_applicationViewState), 2);
    m_sortingBox.setSelectedId(1, juce::dontSendNotification);

    m_directoryContents.addChangeListener(this);
    m_directoryThread.startThread(juce::Thread::Priority::low);
    updateNavigationButtons();
}

DirectoryBrowserComponent::~DirectoryBrowserComponent()
{
    m_directoryContents.removeChangeListener(this);
    m_directoryThread.stopThread(2000);
}

void DirectoryBrowserComponent::resized()
{
    auto area = getLocalBounds();
    auto sort = area.removeFromTop(30).reduced(2);
    m_sortLabel.setBounds(sort.removeFromLeft(50));
    m_sortingBox.setBounds(sort);

    auto navigation = area.removeFromTop(30);
    m_backButton.setBounds(navigation.removeFromLeft(32).reduced(2));
    m_forwardButton.setBounds(navigation.removeFromLeft(32).reduced(2));
    m_currentPathField.setBounds(navigation);

    m_searchField.setBounds(area.removeFromBottom(30));
    m_listBox.setBounds(area);
}

void DirectoryBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (!juce::isPositiveAndBelow(rowNum, m_contentList.size()))
        return;

    const juce::Rectangle<int> rowBounds(0, 0, width, height);
    const auto file = m_contentList[rowNum];
    g.setColour(rowNum % 2 == 0 ? m_applicationViewState.getBackgroundColour1()
                               : m_applicationViewState.getBackgroundColour1().brighter(0.05f));
    g.fillRect(rowBounds);
    g.setColour(m_applicationViewState.getBorderColour().withAlpha(0.3f));
    g.drawHorizontalLine(height - 1, 0, width);

    if (rowIsSelected)
    {
        g.setColour(m_applicationViewState.getPrimeColour());
        g.fillRect(rowBounds);
    }

    auto content = rowBounds.reduced(10, 0);
    auto icon = content.removeFromLeft(height).toFloat().reduced(2);
    content.removeFromLeft(10);
    GUIHelpers::drawFromSvg(g,
                            file.isDirectory() ? BinaryData::folder_svg : BinaryData::file_svg,
                            file.isDirectory() ? m_applicationViewState.getPrimeColour() : m_applicationViewState.getTextColour(),
                            icon);

    const auto colour = rowIsSelected ? m_applicationViewState.getPrimeColour().contrasting(0.7f)
                                      : m_applicationViewState.getTextColour();
    const auto text = file.getFileName();
    const auto matchStart = text.indexOfIgnoreCase(m_searchTerm);
    if (m_searchTerm.isEmpty() || matchStart < 0)
    {
        g.setColour(colour);
        g.drawFittedText(text, content, juce::Justification::centredLeft, 1);
        return;
    }

    const auto before = text.substring(0, matchStart);
    const auto match = text.substring(matchStart, matchStart + m_searchTerm.length());
    const auto after = text.substring(matchStart + m_searchTerm.length());
    const auto font = g.getCurrentFont().withHeight((float)height * 0.7f).withStyle(juce::Font::bold);
    g.setFont(font);
    const auto beforeWidth = juce::GlyphArrangement::getStringWidthInt(font, before);
    const auto matchWidth = juce::GlyphArrangement::getStringWidthInt(font, match);

    g.setColour(colour);
    g.drawText(before, content, juce::Justification::centredLeft, false);
    g.setColour(juce::Colours::coral);
    g.drawText(match, content.withTrimmedLeft(beforeWidth), juce::Justification::centredLeft, false);
    g.setColour(colour);
    g.drawText(after, content.withTrimmedLeft(beforeWidth + matchWidth), juce::Justification::centredLeft, false);
}

void DirectoryBrowserComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent &event)
{
    if (!event.mods.isLeftButtonDown() || !juce::isPositiveAndBelow(row, m_contentList.size()))
        return;

    const auto file = m_contentList[row];
    if (file.isDirectory())
    {
        setDirectory(file);
        return;
    }

    if (m_fileActivated != nullptr)
        m_fileActivated(file);
}

void DirectoryBrowserComponent::selectedRowsChanged(int)
{
    const auto selectedFile = getSelectedFile();
    if (m_directoryContents.isStillLoading() && selectedFile != juce::File())
        m_selectionToRestore = selectedFile;

    if (m_selectionChanged != nullptr)
        m_selectionChanged(selectedFile);
}

juce::var DirectoryBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return m_dragSourceDescription.isNotEmpty() && getSelectedFile().existsAsFile()
             ? juce::var(m_dragSourceDescription)
             : juce::var{};
}

void DirectoryBrowserComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_directoryContents)
    {
        publishDirectoryContents();
        return;
    }

    if (source == &m_currentPathField)
    {
        const auto directory = m_currentPathField.getCurrentPath();
        if (directory == m_displayedDirectory)
            return;
        addDirectoryToHistory(directory);
        refresh();
        return;
    }

    BrowserBaseComponent::changeListenerCallback(source);
}

void DirectoryBrowserComponent::setDirectory(const juce::File &directory, bool addToHistory)
{
    if (!directory.isDirectory())
        return;

    if (addToHistory)
        addDirectoryToHistory(directory);

    m_displayedDirectory = directory;
    m_currentPathField.setDir(directory);
    refresh();
}

juce::File DirectoryBrowserComponent::getSelectedFile() const
{
    const auto row = m_listBox.getSelectedRow();
    return juce::isPositiveAndBelow(row, m_contentList.size()) ? m_contentList[row] : juce::File{};
}

void DirectoryBrowserComponent::refresh()
{
    const auto directory = m_currentPathField.getCurrentPath();
    if (!directory.isDirectory())
        return;

    m_displayedDirectory = directory;
    if (m_directoryContents.getDirectory() != directory)
    {
        m_selectionToRestore = juce::File{};
        BrowserBaseComponent::setFileList({});
        m_directoryContents.setDirectory(directory, true, true);
    }
    else
    {
        // DirectoryContentsList caches its scan. Re-publishing that cache would
        // miss files created by Save As, so explicitly restart the scan.
        m_selectionToRestore = getSelectedFile();
        m_directoryContents.refresh();
    }

    if (m_directoryChanged != nullptr)
        m_directoryChanged(directory);
    updateNavigationButtons();
}

void DirectoryBrowserComponent::setFilePredicate(FilePredicate predicate)
{
    m_acceptsFile = std::move(predicate);
    publishDirectoryContents();
}

void DirectoryBrowserComponent::setBrowserEnabled(bool enabled)
{
    m_backButton.setEnabled(enabled && m_navigationIndex > 0);
    m_forwardButton.setEnabled(enabled && m_navigationIndex >= 0 && m_navigationIndex + 1 < m_navigationHistory.size());
    m_currentPathField.setEnabled(enabled);
    m_sortingBox.setEnabled(enabled);
    m_searchField.setEnabled(enabled);
    m_listBox.setEnabled(enabled);
}

void DirectoryBrowserComponent::sortList(int selectedID)
{
    if (m_contentList.size() > 1)
    {
        CompareEntries compare{selectedID != 2};
        m_contentList.sort(compare);
    }
    m_listBox.updateContent();
}

void DirectoryBrowserComponent::navigateBack()
{
    while (m_navigationIndex > 0)
    {
        const auto candidateIndex = m_navigationIndex - 1;
        const auto candidate = m_navigationHistory[candidateIndex];
        if (candidate.isDirectory())
        {
            m_navigationIndex = candidateIndex;
            setDirectory(candidate, false);
            return;
        }

        m_navigationHistory.remove(candidateIndex);
        --m_navigationIndex;
    }
    updateNavigationButtons();
}

void DirectoryBrowserComponent::navigateForward()
{
    while (m_navigationIndex >= 0 && m_navigationIndex + 1 < m_navigationHistory.size())
    {
        const auto candidateIndex = m_navigationIndex + 1;
        const auto candidate = m_navigationHistory[candidateIndex];
        if (candidate.isDirectory())
        {
            m_navigationIndex = candidateIndex;
            setDirectory(candidate, false);
            return;
        }

        m_navigationHistory.remove(candidateIndex);
    }
    updateNavigationButtons();
}

void DirectoryBrowserComponent::addDirectoryToHistory(const juce::File &directory)
{
    if (!directory.isDirectory()
        || (m_navigationIndex >= 0 && m_navigationHistory[m_navigationIndex] == directory))
        return;

    if (m_navigationIndex + 1 < m_navigationHistory.size())
        m_navigationHistory.removeRange(m_navigationIndex + 1, m_navigationHistory.size() - m_navigationIndex - 1);
    m_navigationHistory.add(directory);
    m_navigationIndex = m_navigationHistory.size() - 1;
    updateNavigationButtons();
}

void DirectoryBrowserComponent::updateNavigationButtons()
{
    const bool browserEnabled = m_listBox.isEnabled();
    m_backButton.setEnabled(browserEnabled && m_navigationIndex > 0);
    m_forwardButton.setEnabled(browserEnabled && m_navigationIndex >= 0 && m_navigationIndex + 1 < m_navigationHistory.size());
}

void DirectoryBrowserComponent::publishDirectoryContents()
{
    juce::Array<juce::File> entries;
    for (int index = 0; index < m_directoryContents.getNumFiles(); ++index)
    {
        const auto entry = m_directoryContents.getFile(index);
        if (entry.isDirectory() || m_acceptsFile == nullptr || m_acceptsFile(entry))
            entries.add(entry);
    }

    if (m_directoryContents.isStillLoading() && m_selectionToRestore != juce::File()
        && (m_selectionToRestore.isDirectory()
            || (m_selectionToRestore.existsAsFile()
                && (m_acceptsFile == nullptr || m_acceptsFile(m_selectionToRestore)))))
        entries.addIfNotAlreadyThere(m_selectionToRestore);

    BrowserBaseComponent::setFileList(entries);
    if (getSelectedFile() == juce::File() && m_selectionToRestore != juce::File())
        restoreSelectedFile(m_selectionToRestore);

    if (!m_directoryContents.isStillLoading())
        m_selectionToRestore = juce::File{};
}
