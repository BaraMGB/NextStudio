/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/
#pragma once

#include "Browser_Base.h"

#include <functional>

/** Reusable, asynchronous directory browser.

    The browser owns navigation, scanning, sorting and search. Domain-specific
    behaviour is supplied through callbacks, so browsing files does not depend
    on project lifecycle or sample-preview classes.
*/
class DirectoryBrowserComponent : public BrowserBaseComponent
{
public:
    using FilePredicate = std::function<bool(const juce::File &)>;
    using FileCallback = std::function<void(const juce::File &)>;

    explicit DirectoryBrowserComponent(ApplicationViewState &avs);
    ~DirectoryBrowserComponent() override;

    void resized() override;
    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent &event) override;
    void selectedRowsChanged(int lastRowSelected) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int> &rowsToDescribe) override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void setDirectory(const juce::File &directory, bool addToHistory = true);
    juce::File getCurrentDirectory() const { return m_currentPathField.getCurrentPath(); }
    juce::File getSelectedFile() const;
    void refresh();

    void setFilePredicate(FilePredicate predicate);
    void setSelectionChangedCallback(FileCallback callback) { m_selectionChanged = std::move(callback); }
    void setFileActivatedCallback(FileCallback callback) { m_fileActivated = std::move(callback); }
    void setDirectoryChangedCallback(FileCallback callback) { m_directoryChanged = std::move(callback); }
    void setDragSourceDescription(juce::String description) { m_dragSourceDescription = std::move(description); }

    void setBrowserEnabled(bool enabled);

private:
    void sortList(int selectedID) override;
    void navigateBack();
    void navigateForward();
    void addDirectoryToHistory(const juce::File &directory);
    void updateNavigationButtons();
    void publishDirectoryContents();

    juce::TextButton m_backButton{"<"};
    juce::TextButton m_forwardButton{">"};
    juce::TimeSliceThread m_directoryThread{"Directory browser scanner"};
    juce::DirectoryContentsList m_directoryContents{nullptr, m_directoryThread};
    juce::Array<juce::File> m_navigationHistory;
    int m_navigationIndex{-1};
    juce::File m_displayedDirectory;
    juce::File m_selectionToRestore;
    FilePredicate m_acceptsFile;
    FileCallback m_selectionChanged;
    FileCallback m_fileActivated;
    FileCallback m_directoryChanged;
    juce::String m_dragSourceDescription;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectoryBrowserComponent)
};
