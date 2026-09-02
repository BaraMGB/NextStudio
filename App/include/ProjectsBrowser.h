/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/
#pragma once

#include "ApplicationViewState.h"
#include "DirectoryBrowser.h"
#include "EditViewState.h"
#include "MenuBar.h"
#include "ProjectLifecycle.h"
#include "ProjectWorkflow.h"

namespace te = tracktion_engine;

/** Project actions and lifecycle workflow around a reusable directory browser. */
class ProjectsBrowserComponent : public juce::Component
{
public:
    using Mode = ProjectWorkflow::State;
    using OperationHandler = std::function<void(const ProjectWorkflow::Operation &, ProjectWorkflow::UnsavedResolution)>;

    struct HostCallbacks
    {
        OperationHandler executeOperation;
        std::function<void(bool restore)> resolveRecovery;
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

    void setProjectsDirectory(const juce::File &directory);
    void projectWasSaved(const juce::File &file);
    void setHostCallbacks(HostCallbacks callbacks) { m_hostCallbacks = std::move(callbacks); }
    void beginProjectOperation(ProjectWorkflow::Operation operation);
    void beginRecovery(const juce::String &errorMessage = {});
    void beginSaveProjectAs(bool preservePendingOperation = false);
    void dismissSaveProjectAs();
    void showOperationError(const juce::String &message, const juce::File &file = {});
    void completeProjectOperation(bool succeeded, const juce::String &errorMessage = {}, const juce::File &file = {});
    Mode getMode() const noexcept { return m_workflow.getState(); }
    bool isSaveAsWorkflowActive() const noexcept { return m_workflow.isSavePath(); }
    bool isInteractionLocked() const noexcept { return m_workflow.locksMainInteraction(); }

private:
    void setMode(Mode mode);
    void configureMode();
    void cancelCurrentMode();
    void goBackFromError();
    void updateTargetPreview();
    void updateActionValidation();
    void requestOpen(const juce::File &file);
    void performPrimaryAction();
    void performSave(const juce::File &target, bool overwriteConfirmed);
    void saveBeforePendingOperation();
    void executePendingOperation(ProjectWorkflow::UnsavedResolution resolution);
    void resolveRecovery(bool restore);
    juce::File getSaveTarget() const;
    juce::File getInitialDirectory() const;
    bool isSaveMode() const noexcept { return m_workflow.isSavePath(); }
    void setWorkingWidth(bool enabled);

    juce::DrawableButton m_saveProjectButton, m_saveAsProjectButton, m_newProjectButton;
    MenuBar m_projectsMenu;
    DirectoryBrowserComponent m_directoryBrowser;

    juce::Label m_modeTitle;
    juce::Label m_projectNameLabel;
    juce::TextEditor m_projectNameEditor;
    juce::Label m_targetPathLabel;
    juce::Label m_statusLabel;
    juce::TextButton m_primaryButton{"Save"};
    juce::TextButton m_secondaryButton{"Cancel"};
    juce::TextButton m_tertiaryButton{"Discard && Continue"};

    EditViewState &m_evs;
    ApplicationViewState &m_avs;
    ProjectWorkflow::Controller m_workflow;
    HostCallbacks m_hostCallbacks;
    juce::File m_overwriteTarget;
    bool m_operationInProgress{false};
    bool m_workingWidthRequested{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectsBrowserComponent)
};
