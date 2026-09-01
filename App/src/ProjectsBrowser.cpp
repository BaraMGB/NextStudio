/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ProjectsBrowser.h"
#include "Utilities.h"

#include <array>

namespace
{
const juce::DrawableButton::ButtonStyle buttonStyle{juce::DrawableButton::ButtonStyle::ImageAboveTextLabel};
}

ProjectsBrowserComponent::ProjectsBrowserComponent(EditViewState &evs, ApplicationViewState &avs)
    : m_saveProjectButton("Save", buttonStyle),
      m_saveAsProjectButton("Save As", buttonStyle),
      m_newProjectButton("New", buttonStyle),
      m_directoryBrowser(avs),
      m_evs(evs),
      m_avs(avs)
{
    constexpr auto margin = 7;

    addAndMakeVisible(m_projectsMenu);
    m_projectsMenu.addButton(&m_newProjectButton);
    m_projectsMenu.addButton(&m_saveProjectButton);
    m_projectsMenu.addButton(&m_saveAsProjectButton);

    GUIHelpers::setDrawableOnButton(m_newProjectButton, BinaryData::newProjectButton_svg, avs.getProjectsColour());
    GUIHelpers::setDrawableOnButton(m_saveProjectButton, BinaryData::contentsaveedit_svg, juce::Colours::seagreen);
    GUIHelpers::setDrawableOnButton(m_saveAsProjectButton, BinaryData::contentsaveedit_svg, juce::Colours::cornflowerblue);

    for (auto *button : {&m_newProjectButton, &m_saveProjectButton, &m_saveAsProjectButton})
        button->setEdgeIndent(margin);
    m_newProjectButton.setTooltip("Create a new project");
    m_saveProjectButton.setTooltip("Save project");
    m_saveAsProjectButton.setTooltip("Save project as a new file in the sidebar");

    m_newProjectButton.onClick = [this]
    {
        beginProjectOperation({ProjectWorkflow::OperationType::createNew, {}});
    };
    m_saveProjectButton.onClick = [this]
    {
        if (m_hostCallbacks.saveCurrentProject != nullptr)
            m_hostCallbacks.saveCurrentProject(false, false);
    };
    m_saveAsProjectButton.onClick = [this] { beginSaveProjectAs(); };

    addAndMakeVisible(m_directoryBrowser);
    m_directoryBrowser.setName("ProjectDirectoryBrowser");
    m_directoryBrowser.setDragSourceDescription("ProjectsBrowser");
    m_directoryBrowser.setFilePredicate(ProjectLifecycle::isProjectBrowserEntry);
    m_directoryBrowser.setFileActivatedCallback(
        [this](const juce::File &file)
        {
            if (getMode() == Mode::saveProjectAs)
            {
                m_projectNameEditor.setText(ProjectLifecycle::projectNameWithoutExtension(file.getFileName()));
                return;
            }

            if (getMode() == Mode::normal)
                requestOpen(file);
        });
    m_directoryBrowser.setDirectoryChangedCallback(
        [this](const juce::File &directory)
        {
            m_avs.m_projectLoadDir = directory.getFullPathName();
            updateTargetPreview();
        });

    const std::array<juce::Component *, 8> workflowComponents{
        &m_modeTitle, &m_projectNameLabel, &m_projectNameEditor, &m_targetPathLabel,
        &m_statusLabel, &m_primaryButton, &m_secondaryButton, &m_tertiaryButton};
    for (auto *component : workflowComponents)
        addChildComponent(component);

    m_modeTitle.setFont(juce::Font(18.0f, juce::Font::bold));
    m_modeTitle.setJustificationType(juce::Justification::centredLeft);
    m_projectNameLabel.setText("Project name", juce::dontSendNotification);
    m_targetPathLabel.setMinimumHorizontalScale(0.6f);
    m_targetPathLabel.setJustificationType(juce::Justification::centredLeft);
    m_statusLabel.setJustificationType(juce::Justification::centredLeft);
    m_statusLabel.setMinimumHorizontalScale(0.65f);
    m_statusLabel.setColour(juce::Label::textColourId, m_avs.getTextColour());
    m_projectNameEditor.setTooltip("Project name; .tracktionedit is added automatically");
    m_projectNameEditor.setColour(juce::TextEditor::outlineColourId, m_avs.getBorderColour());
    m_projectNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, m_avs.getPrimeColour());

    m_primaryButton.onClick = [this] { performPrimaryAction(); };
    m_secondaryButton.onClick = [this]
    {
        if (getMode() == Mode::confirmOverwrite)
            setMode(Mode::saveProjectAs);
        else
            cancelCurrentMode();
    };
    m_tertiaryButton.onClick = [this]
    {
        if (getMode() == Mode::confirmUnsavedChanges)
            executePendingOperation(ProjectWorkflow::UnsavedResolution::discarded);
    };
    m_projectNameEditor.onTextChange = [this] { updateTargetPreview(); };
    m_projectNameEditor.onReturnKey = [this]
    {
        if (m_primaryButton.isEnabled())
            performPrimaryAction();
    };
    m_projectNameEditor.onEscapeKey = [this] { cancelCurrentMode(); };

    setName("ProjectBrowser");
    setWantsKeyboardFocus(true);
    m_directoryBrowser.setDirectory(getInitialDirectory());
    configureMode();
}

ProjectsBrowserComponent::~ProjectsBrowserComponent()
{
    if (m_workingWidthRequested)
        setWorkingWidth(false);
}

void ProjectsBrowserComponent::paint(juce::Graphics &g)
{
    g.fillAll(m_avs.getBackgroundColour2());
    if (getMode() == Mode::normal)
    {
        g.setColour(m_avs.getBorderColour());
        g.drawHorizontalLine(m_projectsMenu.getBottom(), 0, getWidth());
    }
}

void ProjectsBrowserComponent::resized()
{
    auto area = getLocalBounds().reduced(4);
    const auto mode = getMode();

    if (mode == Mode::normal)
    {
        m_projectsMenu.setBounds(area.removeFromTop(66));
        m_directoryBrowser.setBounds(area);
        return;
    }

    m_modeTitle.setBounds(area.removeFromTop(30));
    if (mode == Mode::confirmUnsavedChanges)
    {
        area.removeFromTop(8);
        m_statusLabel.setBounds(area.removeFromTop(110));
        auto buttons = area.removeFromBottom(34);
        const auto third = buttons.getWidth() / 3;
        m_primaryButton.setBounds(buttons.removeFromLeft(third).reduced(2));
        m_tertiaryButton.setBounds(buttons.removeFromLeft(third).reduced(2));
        m_secondaryButton.setBounds(buttons.reduced(2));
        return;
    }

    auto buttons = area.removeFromBottom(34);
    m_secondaryButton.setBounds(buttons.removeFromRight(90).reduced(2));
    m_primaryButton.setBounds(buttons.removeFromRight(100).reduced(2));
    m_statusLabel.setBounds(area.removeFromBottom(42));

    if (isSaveMode() && mode != Mode::saving)
    {
        m_targetPathLabel.setBounds(area.removeFromBottom(34));
        auto name = area.removeFromBottom(34);
        m_projectNameLabel.setBounds(name.removeFromLeft(95));
        m_projectNameEditor.setBounds(name.reduced(2));
    }

    if (m_directoryBrowser.isVisible())
        m_directoryBrowser.setBounds(area);
}

bool ProjectsBrowserComponent::keyPressed(const juce::KeyPress &key)
{
    if (getMode() == Mode::normal)
        return false;
    if (getMode() == Mode::committing || getMode() == Mode::saving)
        return true;

    if (key == juce::KeyPress::escapeKey)
    {
        cancelCurrentMode();
        return true;
    }
    if (key == juce::KeyPress::returnKey && m_primaryButton.isEnabled())
    {
        performPrimaryAction();
        return true;
    }
    return false;
}

void ProjectsBrowserComponent::setProjectsDirectory(const juce::File &directory)
{
    if (directory.isDirectory())
        m_directoryBrowser.setDirectory(directory);
}

void ProjectsBrowserComponent::projectWasSaved(const juce::File &file)
{
    if (file.existsAsFile() && file.getParentDirectory() == m_directoryBrowser.getCurrentDirectory())
        m_directoryBrowser.refresh();
}

void ProjectsBrowserComponent::beginProjectOperation(ProjectWorkflow::Operation operation)
{
    if (!operation.isValid() || m_operationInProgress)
        return;

    if (!m_workflow.stageOperation(operation, m_evs.m_edit.hasChangedSinceSaved()))
    {
        setMode(Mode::confirmUnsavedChanges);
        return;
    }

    executePendingOperation(ProjectWorkflow::UnsavedResolution::clean);
}

void ProjectsBrowserComponent::beginSaveProjectAs(bool preservePendingOperation)
{
    m_overwriteTarget = juce::File{};
    m_operationInProgress = false;

    auto suggestedName = m_evs.m_editName.get().trim();
    const auto currentFile = m_evs.m_edit.editFileRetriever ? m_evs.m_edit.editFileRetriever() : juce::File{};
    if (ProjectLifecycle::isPersistentProjectFile(currentFile))
        suggestedName = currentFile.getFileNameWithoutExtension();
    if (suggestedName.isEmpty() || suggestedName.equalsIgnoreCase("unknown"))
        suggestedName = "Untitled";

    m_projectNameEditor.setText(ProjectLifecycle::projectNameWithoutExtension(suggestedName), false);
    m_workflow.beginSaveAs(preservePendingOperation);
    setMode(Mode::saveProjectAs);
    updateTargetPreview();

    juce::MessageManager::callAsync(
        [safeThis = juce::Component::SafePointer<ProjectsBrowserComponent>(this)]
        {
            if (safeThis != nullptr)
            {
                safeThis->m_projectNameEditor.grabKeyboardFocus();
                safeThis->m_projectNameEditor.selectAll();
            }
        });
}

void ProjectsBrowserComponent::dismissSaveProjectAs()
{
    if (isSaveAsWorkflowActive() && getMode() != Mode::saving)
        cancelCurrentMode();
}

void ProjectsBrowserComponent::setMode(Mode mode)
{
    m_workflow.transitionTo(mode);
    setWorkingWidth(m_workflow.isSavePath());
    if (m_hostCallbacks.setInteractionLocked != nullptr)
        m_hostCallbacks.setInteractionLocked(m_workflow.locksMainInteraction());
    configureMode();
    resized();
    repaint();
}

void ProjectsBrowserComponent::configureMode()
{
    const auto mode = getMode();
    const bool normal = mode == Mode::normal;
    const bool unsaved = mode == Mode::confirmUnsavedChanges;
    const bool busy = mode == Mode::saving || mode == Mode::committing;
    const bool showDirectory = normal || mode == Mode::saveProjectAs || mode == Mode::confirmOverwrite || mode == Mode::operationError;
    const bool showSaveFields = isSaveMode() && mode != Mode::saving;

    m_projectsMenu.setVisible(normal);
    m_directoryBrowser.setVisible(showDirectory);
    m_directoryBrowser.setBrowserEnabled(normal || mode == Mode::saveProjectAs);
    m_modeTitle.setVisible(!normal);
    m_projectNameLabel.setVisible(showSaveFields);
    m_projectNameEditor.setVisible(showSaveFields);
    m_projectNameEditor.setEnabled(mode == Mode::saveProjectAs && !m_operationInProgress);
    m_targetPathLabel.setVisible(showSaveFields);
    m_statusLabel.setVisible(!normal);
    m_primaryButton.setVisible(!normal);
    m_secondaryButton.setVisible(!normal);
    m_tertiaryButton.setVisible(unsaved);

    if (mode != Mode::operationError)
        m_statusLabel.setColour(juce::Label::textColourId, m_avs.getTextColour());

    switch (mode)
    {
    case Mode::normal:
        m_statusLabel.setText({}, juce::dontSendNotification);
        break;
    case Mode::saveProjectAs:
        m_modeTitle.setText("Save Project As", juce::dontSendNotification);
        m_primaryButton.setButtonText("Save");
        m_secondaryButton.setButtonText("Cancel");
        updateTargetPreview();
        break;
    case Mode::confirmOverwrite:
        m_modeTitle.setText("File Already Exists", juce::dontSendNotification);
        m_primaryButton.setButtonText("Overwrite");
        m_secondaryButton.setButtonText("Back");
        m_statusLabel.setText("Warning: Overwrite the existing project?\n" + m_overwriteTarget.getFullPathName(), juce::dontSendNotification);
        break;
    case Mode::saving:
        m_modeTitle.setText("Saving Project", juce::dontSendNotification);
        m_primaryButton.setButtonText("Saving...");
        m_secondaryButton.setButtonText("Cancel");
        m_statusLabel.setText("Saving the current project...", juce::dontSendNotification);
        break;
    case Mode::committing:
        m_modeTitle.setText("Project Operation", juce::dontSendNotification);
        m_primaryButton.setButtonText("Working...");
        m_secondaryButton.setButtonText("Cancel");
        m_statusLabel.setText("Completing the project operation...", juce::dontSendNotification);
        break;
    case Mode::operationError:
        m_modeTitle.setText("Project Operation Failed", juce::dontSendNotification);
        m_primaryButton.setButtonText("Back");
        m_secondaryButton.setButtonText("Close");
        break;
    case Mode::confirmUnsavedChanges:
    {
        const auto &pending = m_workflow.getPendingOperation();
        const auto action = pending.type == ProjectWorkflow::OperationType::load
                              ? "opening:\n" + pending.file.getFullPathName()
                              : pending.type == ProjectWorkflow::OperationType::createNew
                                  ? juce::String("creating a new project.")
                                  : juce::String("quitting NextStudio.");
        m_modeTitle.setText("Unsaved Project", juce::dontSendNotification);
        m_primaryButton.setButtonText("Save && Continue");
        m_tertiaryButton.setButtonText("Discard && Continue");
        m_secondaryButton.setButtonText("Back");
        m_statusLabel.setText("The current project has unsaved changes. Save them before " + action, juce::dontSendNotification);
        break;
    }
    }

    m_primaryButton.setEnabled(!busy);
    m_secondaryButton.setEnabled(!busy);
    updateActionValidation();
}

void ProjectsBrowserComponent::cancelCurrentMode()
{
    m_operationInProgress = false;
    m_workflow.cancel();
    setMode(Mode::normal);
}

void ProjectsBrowserComponent::goBackFromError()
{
    m_workflow.goBackFromError();
    setMode(m_workflow.getState());
}

void ProjectsBrowserComponent::updateTargetPreview()
{
    if (!isSaveMode())
        return;

    const auto target = getSaveTarget();
    const auto validName = ProjectLifecycle::isValidProjectName(m_projectNameEditor.getText());
    m_targetPathLabel.setText(target == juce::File() ? "Invalid project name" : target.getFullPathName(), juce::dontSendNotification);
    m_targetPathLabel.setTooltip(target == juce::File() ? juce::String{} : target.getFullPathName());
    if (getMode() == Mode::saveProjectAs)
    {
        m_statusLabel.setText(validName ? "The .tracktionedit extension is added automatically."
                                        : "Enter a non-empty name without < > : \" / \\ | ? *.",
                              juce::dontSendNotification);
        m_statusLabel.setColour(juce::Label::textColourId, validName ? m_avs.getTextColour() : juce::Colours::orange);
    }
    updateActionValidation();
}

void ProjectsBrowserComponent::updateActionValidation()
{
    const auto mode = getMode();
    if (mode == Mode::saveProjectAs)
        m_primaryButton.setEnabled(ProjectLifecycle::isValidProjectTarget(getSaveTarget()) && !m_operationInProgress);
    else if (mode == Mode::confirmOverwrite || mode == Mode::confirmUnsavedChanges || mode == Mode::operationError)
        m_primaryButton.setEnabled(!m_operationInProgress);
    else if (mode == Mode::saving || mode == Mode::committing)
    {
        m_primaryButton.setEnabled(false);
        m_secondaryButton.setEnabled(false);
    }
}

void ProjectsBrowserComponent::requestOpen(const juce::File &file)
{
    if (!file.existsAsFile() || !ProjectLifecycle::isPersistentProjectFile(file))
    {
        showOperationError("The selected file is not a readable NextStudio project.", file);
        return;
    }

    beginProjectOperation({ProjectWorkflow::OperationType::load, file});
}

void ProjectsBrowserComponent::performPrimaryAction()
{
    switch (getMode())
    {
    case Mode::saveProjectAs:
    {
        const auto target = getSaveTarget();
        if (!ProjectLifecycle::isValidProjectTarget(target))
        {
            showOperationError("The project name or target folder is not writable.", target);
            return;
        }
        if (target.existsAsFile())
        {
            m_overwriteTarget = target;
            setMode(Mode::confirmOverwrite);
        }
        else
        {
            performSave(target, false);
        }
        break;
    }
    case Mode::confirmOverwrite:
        performSave(m_overwriteTarget, true);
        break;
    case Mode::operationError:
        goBackFromError();
        break;
    case Mode::confirmUnsavedChanges:
        saveBeforePendingOperation();
        break;
    case Mode::saving:
    case Mode::committing:
    case Mode::normal:
        break;
    }
}

void ProjectsBrowserComponent::performSave(const juce::File &target, bool overwriteConfirmed)
{
    if (target.existsAsFile() && !overwriteConfirmed)
    {
        m_overwriteTarget = target;
        setMode(Mode::confirmOverwrite);
        return;
    }

    const auto returnStateOnFailure = getMode();
    m_operationInProgress = true;
    m_workflow.markSaving();
    setMode(Mode::saving);

    auto result = GUIHelpers::ProjectSaveResult::failed;
    if (m_hostCallbacks.saveProjectTo != nullptr)
        result = m_hostCallbacks.saveProjectTo(target);

    m_operationInProgress = false;
    if (result != GUIHelpers::ProjectSaveResult::saved)
    {
        m_workflow.transitionTo(returnStateOnFailure);
        showOperationError("NextStudio could not save the project.", target);
        return;
    }

    const auto continuation = m_workflow.completeSave();
    if (continuation.isValid())
    {
        executePendingOperation(ProjectWorkflow::UnsavedResolution::saved);
        return;
    }

    setMode(Mode::normal);
}

void ProjectsBrowserComponent::saveBeforePendingOperation()
{
    const auto currentFile = m_evs.m_edit.editFileRetriever ? m_evs.m_edit.editFileRetriever() : juce::File{};
    const bool saveTargetRequired = ProjectLifecycle::shouldChooseSaveTarget(currentFile, false);
    m_workflow.beginSaveBeforePending(saveTargetRequired);
    if (!saveTargetRequired)
        setMode(Mode::saving);

    auto result = GUIHelpers::ProjectSaveResult::failed;
    if (saveTargetRequired && m_hostCallbacks.saveCurrentProject != nullptr)
        result = m_hostCallbacks.saveCurrentProject(false, true);
    else if (!saveTargetRequired && m_hostCallbacks.saveProjectTo != nullptr)
        result = m_hostCallbacks.saveProjectTo(currentFile);

    if (result == GUIHelpers::ProjectSaveResult::saved)
    {
        m_workflow.completeSave();
        executePendingOperation(ProjectWorkflow::UnsavedResolution::saved);
    }
    else if (result == GUIHelpers::ProjectSaveResult::failed)
    {
        m_workflow.transitionTo(Mode::confirmUnsavedChanges);
        showOperationError("NextStudio could not save the current project.", currentFile);
    }
    // cancelled means the embedded Save As workflow owns the continuation.
}

void ProjectsBrowserComponent::executePendingOperation(ProjectWorkflow::UnsavedResolution resolution)
{
    ProjectWorkflow::Operation operation;
    if (resolution == ProjectWorkflow::UnsavedResolution::discarded)
        operation = m_workflow.confirmDiscard();
    else
        operation = m_workflow.getPendingOperation();

    if (!operation.isValid())
        return;

    m_workflow.markCommitting();
    m_operationInProgress = true;
    setMode(Mode::committing);

    if (resolution == ProjectWorkflow::UnsavedResolution::clean && m_evs.m_edit.hasChangedSinceSaved())
    {
        m_operationInProgress = false;
        m_workflow.transitionTo(Mode::confirmUnsavedChanges);
        setMode(Mode::confirmUnsavedChanges);
        return;
    }

    if (m_hostCallbacks.executeOperation != nullptr)
    {
        m_hostCallbacks.executeOperation(operation, resolution);
        return;
    }

    m_workflow.completeOperation();
    showOperationError("The project operation handler is unavailable.", operation.file);
}

juce::File ProjectsBrowserComponent::getSaveTarget() const
{
    const auto name = ProjectLifecycle::projectNameWithoutExtension(m_projectNameEditor.getText());
    if (!ProjectLifecycle::isValidProjectName(name))
        return {};
    return m_directoryBrowser.getCurrentDirectory().getChildFile(name + ".tracktionedit");
}

juce::File ProjectsBrowserComponent::getInitialDirectory() const
{
    const auto remembered = juce::File(m_avs.m_projectLoadDir.get());
    if (remembered.isDirectory())
        return remembered;

    const auto projects = juce::File(m_avs.m_projectsDir.get());
    if (projects.isDirectory())
        return projects;
    return juce::File(m_avs.m_workDir.get());
}

void ProjectsBrowserComponent::showOperationError(const juce::String &message, const juce::File &file)
{
    m_workflow.showError();
    m_operationInProgress = false;
    auto text = message;
    if (file != juce::File())
        text << "\n" << file.getFullPathName();
    m_statusLabel.setText(text, juce::dontSendNotification);
    m_statusLabel.setTooltip(text);
    m_statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    setMode(m_workflow.getState());
    m_statusLabel.setText(text, juce::dontSendNotification);
}

void ProjectsBrowserComponent::completeProjectOperation(bool succeeded, const juce::String &errorMessage, const juce::File &file)
{
    m_operationInProgress = false;
    const auto pending = m_workflow.getPendingOperation();
    if (succeeded)
    {
        m_workflow.completeOperation();
        setMode(Mode::normal);
        return;
    }

    m_workflow.completeOperation();
    showOperationError(errorMessage.isNotEmpty() ? errorMessage : "NextStudio could not complete the project operation.",
                       file != juce::File() ? file : pending.file);
}

void ProjectsBrowserComponent::setWorkingWidth(bool enabled)
{
    if (enabled == m_workingWidthRequested)
        return;
    m_workingWidthRequested = enabled;
    if (m_hostCallbacks.setWorkingWidth != nullptr)
        m_hostCallbacks.setWorkingWidth(enabled);
}
