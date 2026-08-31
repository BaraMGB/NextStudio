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
#include "ProjectWorkflow.h"

namespace te = tracktion_engine;

class ProjectsBrowserComponent : public BrowserBaseComponent
{
public:
    using Mode = ProjectWorkflow::State;
    using OperationHandler = std::function<void(const ProjectWorkflow::Operation &, ProjectWorkflow::UnsavedResolution)>;

    struct HostCallbacks
    {
        OperationHandler executeOperation;
        std::function<GUIHelpers::ProjectSaveResult(bool saveAs, bool preservePendingOperation)> saveCurrentProject;
        std::function<GUIHelpers::ProjectSaveResult(const juce::File &)> saveProjectTo;
        std::function<void(bool)> setInteractionLocked;
        std::function<void(bool)> setWorkingWidth;
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
    void setHostCallbacks(HostCallbacks callbacks) { m_hostCallbacks = std::move(callbacks); }
    void beginLoadProject();
    void beginProjectOperation(ProjectWorkflow::Operation operation);
    void beginSaveProjectAs(bool preservePendingOperation = false);
    void dismissSaveProjectAs();
    void showOperationError(const juce::String &message, const juce::File &file = {});
    void completeProjectOperation(bool succeeded, const juce::String &errorMessage = {}, const juce::File &file = {});
    Mode getMode() const noexcept { return m_workflow.getState(); }
    bool isSaveAsWorkflowActive() const noexcept { return m_workflow.isSavePath(); }
    bool isInteractionLocked() const noexcept { return m_workflow.locksMainInteraction(); }

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
    void requestOpen(const juce::File &file);
    void performPrimaryAction();
    void performSave(const juce::File &target, bool overwriteConfirmed);
    void showUnsavedConfirmation(ProjectWorkflow::Operation operation);
    void saveBeforePendingOperation();
    void executePendingOperation(ProjectWorkflow::UnsavedResolution resolution);
    juce::File getSelectedBrowserFile() const;
    juce::File getSaveTarget() const;
    juce::File getInitialDirectory(bool forSave) const;
    bool isBrowserMode() const noexcept;
    bool isSaveMode() const noexcept { return m_workflow.isSavePath(); }
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
    ProjectWorkflow::Controller m_workflow;
    HostCallbacks m_hostCallbacks;
    juce::Array<juce::File> m_normalProjectFiles;
    juce::Array<juce::File> m_navigationHistory;
    int m_navigationIndex{-1};
    juce::File m_selectedFile;
    juce::File m_displayedDirectory;
    juce::File m_overwriteTarget;
    bool m_operationInProgress{false};
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
