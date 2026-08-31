/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/
#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "Browser_Base.h"
#include "EditViewState.h"
#include "MenuBar.h"
#include "ProjectLifecycle.h"

namespace te = tracktion_engine;

class ProjectsBrowserComponent : public BrowserBaseComponent
{
public:
    enum class Mode
    {
        normal,
        loadProject,
        saveProjectAs,
        confirmOverwrite,
        operationError,
        confirmUnsavedChanges
    };

    ProjectsBrowserComponent(EditViewState &evs, ApplicationViewState &avs);
    ~ProjectsBrowserComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress &key) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int> &rowsToDescribe) override;

    void paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent &e) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void setFileList(const juce::Array<juce::File> &fileList);
    void projectWasSaved(const juce::File &file);
    void beginLoadProject();
    void beginSaveProjectAs();
    void dismissSaveProjectAs();
    void showOperationError(const juce::String &message, const juce::File &file = {});
    void completeLoadOperation(bool succeeded, const juce::String &errorMessage = {});
    Mode getMode() const noexcept { return m_mode; }
    bool isSaveAsWorkflowActive() const noexcept { return isSaveMode(); }

private:
    void sortList(int selectedID) override;
    void sortByName(juce::Array<juce::File> &list, bool forward);
    void setMode(Mode mode);
    void configureMode();
    void cancelCurrentMode();
    void goBackFromError();
    void refreshDirectory();
    void navigateTo(const juce::File &directory, bool addToHistory = true);
    void navigateBack();
    void navigateForward();
    void updateSelectionAndValidation();
    void updateTargetPreview();
    void requestOpen(const juce::File &file, bool discardUnsavedChanges = false);
    void performPrimaryAction();
    void performSave(const juce::File &target, bool overwriteConfirmed);
    void showUnsavedConfirmation(const juce::File &file);
    void saveBeforePendingLoad();
    juce::File getSelectedBrowserFile() const;
    juce::File getSaveTarget() const;
    juce::File getInitialDirectory(bool forSave) const;
    bool isBrowserMode() const noexcept { return m_mode != Mode::normal; }
    bool isSaveMode() const noexcept;
    void setWorkingWidth(bool enabled);

    juce::DrawableButton m_loadProjectButton, m_saveProjectButton, m_saveAsProjectButton, m_newProjectButton;
    MenuBar m_projectsMenu;

    juce::Label m_modeTitle;
    juce::TextButton m_backButton{"<"}, m_forwardButton{">"};
    juce::Label m_selectedPathLabel;
    juce::Label m_projectNameLabel;
    juce::TextEditor m_projectNameEditor;
    juce::Label m_targetPathLabel;
    juce::Label m_statusLabel;
    juce::TextButton m_primaryButton{"Open"};
    juce::TextButton m_secondaryButton{"Cancel"};
    juce::TextButton m_tertiaryButton{"Back"};

    EditViewState &m_evs;
    ApplicationViewState &m_avs;
    juce::TimeSliceThread m_directoryThread{"Project directory scanner"};
    juce::DirectoryContentsList m_directoryContents{nullptr, m_directoryThread};
    Mode m_mode{Mode::normal};
    Mode m_modeBeforeError{Mode::normal};
    juce::Array<juce::File> m_normalProjectFiles;
    juce::Array<juce::File> m_navigationHistory;
    int m_navigationIndex{-1};
    juce::File m_selectedFile;
    juce::File m_displayedDirectory;
    juce::File m_pendingLoadFile;
    juce::File m_overwriteTarget;
    bool m_operationInProgress{false};
    bool m_resumeLoadAfterSave{false};
    bool m_workingWidthRequested{false};

    struct CompareNameForward
    {
        static int compareElements(const juce::File &first, const juce::File &second);
    };

    struct CompareNameBackwards
    {
        static int compareElements(const juce::File &first, const juce::File &second);
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectsBrowserComponent)
};
