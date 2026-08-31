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
constexpr auto projectWildcard = "*.tracktionedit";
const juce::DrawableButton::ButtonStyle buttonStyle{juce::DrawableButton::ButtonStyle::ImageAboveTextLabel};

void configurePathLabel(juce::Label &label)
{
    label.setMinimumHorizontalScale(0.6f);
    label.setJustificationType(juce::Justification::centredLeft);
}
} // namespace

int ProjectsBrowserComponent::CompareNameForward::compareElements(const juce::File &first, const juce::File &second)
{
    if (first.isDirectory() != second.isDirectory())
        return first.isDirectory() ? -1 : 1;
    return first.getFileName().compareNatural(second.getFileName());
}

int ProjectsBrowserComponent::CompareNameBackwards::compareElements(const juce::File &first, const juce::File &second)
{
    if (first.isDirectory() != second.isDirectory())
        return first.isDirectory() ? -1 : 1;
    return second.getFileName().compareNatural(first.getFileName());
}

ProjectsBrowserComponent::ProjectsBrowserComponent(EditViewState &evs, ApplicationViewState &avs)
    : BrowserBaseComponent(avs),
      m_newProjectButton("New", buttonStyle),
      m_loadProjectButton("Load", buttonStyle),
      m_saveProjectButton("Save", buttonStyle),
      m_saveAsProjectButton("Save As", buttonStyle),
      m_evs(evs),
      m_avs(avs)
{
    constexpr auto margin = 7;

    addAndMakeVisible(m_projectsMenu);
    m_projectsMenu.addButton(&m_newProjectButton);
    m_projectsMenu.addButton(&m_loadProjectButton);
    m_projectsMenu.addButton(&m_saveProjectButton);
    m_projectsMenu.addButton(&m_saveAsProjectButton);

    GUIHelpers::setDrawableOnButton(m_newProjectButton, BinaryData::newProjectButton_svg, avs.getProjectsColour());
    GUIHelpers::setDrawableOnButton(m_loadProjectButton, BinaryData::filedownload_svg, juce::Colours::lightcyan);
    GUIHelpers::setDrawableOnButton(m_saveProjectButton, BinaryData::contentsaveedit_svg, juce::Colours::seagreen);
    GUIHelpers::setDrawableOnButton(m_saveAsProjectButton, BinaryData::contentsaveedit_svg, juce::Colours::cornflowerblue);

    for (auto *button : {&m_newProjectButton, &m_loadProjectButton, &m_saveProjectButton, &m_saveAsProjectButton})
        button->setEdgeIndent(margin);
    m_newProjectButton.setTooltip("Create a new project");
    m_loadProjectButton.setTooltip("Open a project in the sidebar");
    m_saveProjectButton.setTooltip("Save project");
    m_saveAsProjectButton.setTooltip("Save project as a new file in the sidebar");

    m_newProjectButton.onClick = [this]
    {
        beginProjectOperation({ProjectWorkflow::OperationType::createNew, {}});
    };
    m_loadProjectButton.onClick = [this] { beginLoadProject(); };
    m_saveProjectButton.onClick = [this]
    {
        if (m_hostCallbacks.saveCurrentProject != nullptr)
            m_hostCallbacks.saveCurrentProject(false, false);
    };
    m_saveAsProjectButton.onClick = [this] { beginSaveProjectAs(); };

    const std::array<juce::Component *, 11> workflowComponents{
        &m_modeTitle, &m_backButton, &m_forwardButton, &m_selectedPathLabel,
        &m_projectNameLabel, &m_projectNameEditor, &m_targetPathLabel,
        &m_statusLabel, &m_primaryButton, &m_secondaryButton, &m_tertiaryButton};
    for (auto *component : workflowComponents)
        addChildComponent(component);

    m_modeTitle.setFont(juce::Font(18.0f, juce::Font::bold));
    m_modeTitle.setJustificationType(juce::Justification::centredLeft);
    m_projectNameLabel.setText("Project name", juce::dontSendNotification);
    configurePathLabel(m_selectedPathLabel);
    configurePathLabel(m_targetPathLabel);
    m_statusLabel.setJustificationType(juce::Justification::centredLeft);
    m_statusLabel.setMinimumHorizontalScale(0.65f);
    m_statusLabel.setColour(juce::Label::textColourId, m_avs.getTextColour());

    m_backButton.setTooltip("Previous folder");
    m_forwardButton.setTooltip("Next folder");
    m_primaryButton.setTooltip("Perform the selected project action");
    m_secondaryButton.setTooltip("Cancel or go back");
    m_tertiaryButton.setTooltip("Alternative project action");
    m_projectNameEditor.setTooltip("Project name; .tracktionedit is added automatically");
    m_projectNameEditor.setColour(juce::TextEditor::outlineColourId, m_avs.getBorderColour());
    m_projectNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, m_avs.getPrimeColour());

    m_backButton.onClick = [this] { navigateBack(); };
    m_forwardButton.onClick = [this] { navigateForward(); };
    m_primaryButton.onClick = [this] { performPrimaryAction(); };
    m_secondaryButton.onClick = [this]
    {
        if (getMode() == Mode::confirmOverwrite)
            setMode(Mode::saveProjectAs);
        else if (getMode() == Mode::operationError)
            cancelCurrentMode();
        else if (getMode() == Mode::confirmUnsavedChanges)
        {
            const auto pending = m_workflow.getPendingOperation();
            cancelCurrentMode();
            if (pending.type == ProjectWorkflow::OperationType::load)
            {
                beginLoadProject();
                if (pending.file.getParentDirectory().isDirectory())
                    navigateTo(pending.file.getParentDirectory());
                m_selectedFile = pending.file;
                updateSelectionAndValidation();
            }
        }
        else
            cancelCurrentMode();
    };
    m_tertiaryButton.onClick = [this]
    {
        if (getMode() == Mode::confirmUnsavedChanges)
            executePendingOperation(ProjectWorkflow::UnsavedResolution::discarded);
        else
            goBackFromError();
    };

    m_projectNameEditor.onTextChange = [this] { updateTargetPreview(); };
    m_projectNameEditor.onReturnKey = [this]
    {
        if (m_primaryButton.isEnabled())
            performPrimaryAction();
    };
    m_projectNameEditor.onEscapeKey = [this] { cancelCurrentMode(); };

    m_directoryContents.addChangeListener(this);
    m_directoryThread.startThread(juce::Thread::Priority::low);

    setName("ProjectBrowser");
    setWantsKeyboardFocus(true);
    m_sortingBox.addItem(GUIHelpers::translate("by Name (a - z)", m_applicationViewState), 1);
    m_sortingBox.addItem(GUIHelpers::translate("by Name (z - a)", m_applicationViewState), 2);
    m_sortingBox.setSelectedId(1, juce::dontSendNotification);
    configureMode();
}

ProjectsBrowserComponent::~ProjectsBrowserComponent()
{
    m_directoryContents.removeChangeListener(this);
    m_directoryThread.stopThread(2000);
    if (m_workingWidthRequested)
        setWorkingWidth(false);
}

void ProjectsBrowserComponent::setFileList(const juce::Array<juce::File> &fileList)
{
    m_normalProjectFiles = fileList;
    if (getMode() == Mode::normal)
        BrowserBaseComponent::setFileList(fileList);
}

void ProjectsBrowserComponent::projectWasSaved(const juce::File &file)
{
    const auto projectsRoot = juce::File(m_avs.m_projectsDir.get());
    if (!file.existsAsFile() || !(file.getParentDirectory() == projectsRoot || file.isAChildOf(projectsRoot)))
        return;

    m_normalProjectFiles.addIfNotAlreadyThere(file);
    if (getMode() == Mode::normal)
        BrowserBaseComponent::setFileList(m_normalProjectFiles);
}

void ProjectsBrowserComponent::beginLoadProject()
{
    m_projectRequest.clear();
    m_navigationHistory.clear();
    m_navigationIndex = -1;
    m_selectedFile = juce::File{};
    m_operationInProgress = false;
    m_workflow.beginLoadBrowser();
    setMode(Mode::loadProject);
    navigateTo(getInitialDirectory(false));
}

void ProjectsBrowserComponent::beginProjectOperation(ProjectWorkflow::Operation operation)
{
    if (!operation.isValid())
        return;

    if (!m_workflow.stageOperation(operation, m_evs.m_edit.hasChangedSinceSaved()))
    {
        showUnsavedConfirmation(std::move(operation));
        return;
    }

    executePendingOperation(ProjectWorkflow::UnsavedResolution::clean);
}

void ProjectsBrowserComponent::beginSaveProjectAs(bool preservePendingOperation)
{
    m_projectRequest.clear();
    m_navigationHistory.clear();
    m_navigationIndex = -1;
    m_selectedFile = juce::File{};
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
    navigateTo(getInitialDirectory(true));
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

void ProjectsBrowserComponent::paint(juce::Graphics &g)
{
    BrowserBaseComponent::paint(g);
    if (getMode() == Mode::normal)
    {
        const auto bottom = m_projectsMenu.getBottom();
        g.setColour(m_avs.getBorderColour());
        g.drawHorizontalLine(bottom, 0, getWidth());
    }
}

void ProjectsBrowserComponent::resized()
{
    auto area = getLocalBounds().reduced(4);
    if (getMode() == Mode::normal)
    {
        auto projectButtons = area.removeFromTop(66);
        auto sort = area.removeFromTop(30).reduced(2);
        auto sortLabel = sort.removeFromLeft(50);
        auto search = area.removeFromBottom(30);

        m_projectsMenu.setBounds(projectButtons);
        m_sortLabel.setBounds(sortLabel);
        m_sortingBox.setBounds(sort);
        m_searchField.setBounds(search);
        m_listBox.setBounds(area);
        return;
    }

    m_modeTitle.setBounds(area.removeFromTop(30));

    if (getMode() == Mode::confirmUnsavedChanges)
    {
        area.removeFromTop(8);
        m_statusLabel.setBounds(area.removeFromTop(100));
        auto buttons = area.removeFromBottom(34);
        const auto third = buttons.getWidth() / 3;
        m_primaryButton.setBounds(buttons.removeFromLeft(third).reduced(2));
        m_tertiaryButton.setBounds(buttons.removeFromLeft(third).reduced(2));
        m_secondaryButton.setBounds(buttons.reduced(2));
        return;
    }

    auto navigation = area.removeFromTop(32);
    m_backButton.setBounds(navigation.removeFromLeft(34).reduced(2));
    m_forwardButton.setBounds(navigation.removeFromLeft(34).reduced(2));
    m_currentPathField.setBounds(navigation);

    auto buttons = area.removeFromBottom(34);
    m_secondaryButton.setBounds(buttons.removeFromRight(90).reduced(2));
    m_primaryButton.setBounds(buttons.removeFromRight(100).reduced(2));
    if (m_tertiaryButton.isVisible())
        m_tertiaryButton.setBounds(buttons.removeFromRight(80).reduced(2));

    m_statusLabel.setBounds(area.removeFromBottom(38));
    if (isSaveMode())
    {
        m_targetPathLabel.setBounds(area.removeFromBottom(34));
        auto name = area.removeFromBottom(34);
        m_projectNameLabel.setBounds(name.removeFromLeft(95));
        m_projectNameEditor.setBounds(name.reduced(2));
    }
    else
    {
        m_selectedPathLabel.setBounds(area.removeFromBottom(38));
    }
    m_listBox.setBounds(area);
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

juce::var ProjectsBrowserComponent::getDragSourceDescription(const juce::SparseSet<int> &)
{
    return getMode() == Mode::normal ? juce::var("ProjectsBrowser") : juce::var{};
}

void ProjectsBrowserComponent::paintListBoxItem(int rowNum, juce::Graphics &g, int width, int height, bool rowIsSelected)
{
    if (!juce::isPositiveAndBelow(rowNum, m_contentList.size()))
        return;

    const juce::Rectangle<int> bounds(0, 0, width, height);
    g.setColour(rowNum % 2 == 0 ? m_avs.getBackgroundColour2() : m_avs.getBackgroundColour2().brighter(0.05f));
    g.fillRect(bounds);
    if (rowIsSelected)
    {
        g.setColour(m_avs.getPrimeColour());
        g.fillRect(bounds);
    }
    g.setColour(rowIsSelected ? m_avs.getPrimeColour().contrasting(0.7f) : m_avs.getTextColour());

    const auto &file = m_contentList.getReference(rowNum);
    juce::String text;
    if (file.isDirectory())
        text = "[Folder] " + file.getFileName();
    else if (getMode() == Mode::normal)
        text = file.getFileNameWithoutExtension();
    else
        text = file.getFileName();
    g.drawFittedText(text, bounds.reduced(5, 0), juce::Justification::centredLeft, 1, 0.75f);
}

void ProjectsBrowserComponent::listBoxItemClicked(int row, const juce::MouseEvent &event)
{
    if (!juce::isPositiveAndBelow(row, m_contentList.size()))
        return;

    const auto file = m_contentList[row];
    if (event.getNumberOfClicks() <= 1)
        return;

    if (isBrowserMode() && file.isDirectory())
    {
        navigateTo(file);
        return;
    }

    if (file.existsAsFile() && ProjectLifecycle::isPersistentProjectFile(file))
    {
        if (getMode() == Mode::saveProjectAs)
        {
            m_projectNameEditor.setText(file.getFileNameWithoutExtension());
            updateTargetPreview();
        }
        else
        {
            if (getMode() == Mode::normal)
            {
                beginLoadProject();
                navigateTo(file.getParentDirectory());
                m_selectedFile = file;
            }
            requestOpen(file);
        }
    }
}

void ProjectsBrowserComponent::selectedRowsChanged(int row)
{
    m_selectedFile = juce::isPositiveAndBelow(row, m_contentList.size()) ? m_contentList[row] : juce::File{};
    updateSelectionAndValidation();
}

void ProjectsBrowserComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (source == &m_directoryContents && isBrowserMode())
    {
        juce::Array<juce::File> entries;
        for (int index = 0; index < m_directoryContents.getNumFiles(); ++index)
        {
            const auto entry = m_directoryContents.getFile(index);
            if (entry.isDirectory() || ProjectLifecycle::isPersistentProjectFile(entry))
                entries.add(entry);
        }
        BrowserBaseComponent::setFileList(entries);
        configureMode();
        return;
    }

    if (source == &m_currentPathField && isBrowserMode())
    {
        const auto directory = m_currentPathField.getCurrentPath();
        if (directory != m_displayedDirectory)
        {
            m_displayedDirectory = directory;
            if (m_navigationIndex + 1 < m_navigationHistory.size())
                m_navigationHistory.removeRange(m_navigationIndex + 1, m_navigationHistory.size() - m_navigationIndex - 1);
            m_navigationHistory.add(directory);
            m_navigationIndex = m_navigationHistory.size() - 1;
        }
        refreshDirectory();
        return;
    }

    BrowserBaseComponent::changeListenerCallback(source);
}

void ProjectsBrowserComponent::setMode(Mode mode)
{
    m_workflow.transitionTo(mode);
    setWorkingWidth(mode != Mode::normal);
    if (m_hostCallbacks.setInteractionLocked != nullptr)
        m_hostCallbacks.setInteractionLocked(m_workflow.locksMainInteraction());
    configureMode();
    resized();
    repaint();
}

void ProjectsBrowserComponent::configureMode()
{
    const bool normal = getMode() == Mode::normal;
    const bool unsaved = getMode() == Mode::confirmUnsavedChanges;
    const bool committing = getMode() == Mode::committing;
    const bool saving = getMode() == Mode::saving;
    const bool showBrowser = !normal && !unsaved && !committing && !saving;
    const bool save = isSaveMode() && !saving;

    m_projectsMenu.setVisible(normal);
    m_sortLabel.setVisible(normal);
    m_sortingBox.setVisible(normal);
    m_searchField.setVisible(normal);
    m_modeTitle.setVisible(!normal);
    m_currentPathField.setVisible(showBrowser);
    m_backButton.setVisible(showBrowser);
    m_forwardButton.setVisible(showBrowser);
    m_listBox.setVisible(normal || showBrowser);
    m_selectedPathLabel.setVisible(getMode() == Mode::loadProject);
    m_projectNameLabel.setVisible(save);
    m_projectNameEditor.setVisible(save);
    m_targetPathLabel.setVisible(save);
    m_statusLabel.setVisible(!normal);
    m_primaryButton.setVisible(!normal);
    m_secondaryButton.setVisible(!normal);
    m_tertiaryButton.setVisible(unsaved || getMode() == Mode::operationError);

    if (getMode() != Mode::operationError)
        m_statusLabel.setColour(juce::Label::textColourId, m_avs.getTextColour());

    m_listBox.setEnabled(!m_operationInProgress && getMode() != Mode::confirmOverwrite && getMode() != Mode::operationError);
    m_currentPathField.setEnabled(m_listBox.isEnabled());
    m_projectNameEditor.setEnabled(!m_operationInProgress && getMode() == Mode::saveProjectAs);

    switch (getMode())
    {
    case Mode::normal:
        m_statusLabel.setText({}, juce::dontSendNotification);
        BrowserBaseComponent::setFileList(m_normalProjectFiles);
        break;
    case Mode::loadProject:
        m_modeTitle.setText("Open Project", juce::dontSendNotification);
        m_primaryButton.setButtonText("Open");
        m_secondaryButton.setButtonText("Cancel");
        m_statusLabel.setText(m_operationInProgress ? "Opening project..." : "Select a readable NextStudio project.", juce::dontSendNotification);
        break;
    case Mode::saveProjectAs:
        m_modeTitle.setText("Save Project As", juce::dontSendNotification);
        m_primaryButton.setButtonText("Save");
        m_secondaryButton.setButtonText("Cancel");
        m_statusLabel.setText(m_operationInProgress ? "Saving project..." : "Choose a folder and project name.", juce::dontSendNotification);
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
        m_tertiaryButton.setVisible(false);
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

    m_backButton.setEnabled(m_navigationIndex > 0 && !m_operationInProgress);
    m_forwardButton.setEnabled(m_navigationIndex >= 0 && m_navigationIndex + 1 < m_navigationHistory.size() && !m_operationInProgress);
    updateSelectionAndValidation();
}

void ProjectsBrowserComponent::cancelCurrentMode()
{
    m_projectRequest.clear();
    m_operationInProgress = false;
    m_workflow.cancel();
    setMode(Mode::normal);
}

void ProjectsBrowserComponent::goBackFromError()
{
    m_workflow.goBackFromError();
    setMode(m_workflow.getState());
}

void ProjectsBrowserComponent::refreshDirectory()
{
    const auto directory = m_currentPathField.getCurrentPath();
    if (!directory.isDirectory())
    {
        showOperationError("The selected folder is not available.", directory);
        return;
    }

    m_displayedDirectory = directory;
    if (m_directoryContents.getDirectory() != directory)
    {
        m_selectedFile = juce::File{};
        BrowserBaseComponent::setFileList({});
        m_directoryContents.setDirectory(directory, true, true);
    }

    if (isSaveMode())
        m_avs.m_projectSaveDir = directory.getFullPathName();
    else
        m_avs.m_projectLoadDir = directory.getFullPathName();
    updateTargetPreview();
    configureMode();
}

void ProjectsBrowserComponent::navigateTo(const juce::File &directory, bool addToHistory)
{
    if (!directory.isDirectory())
        return;

    if (addToHistory && (m_navigationIndex < 0 || m_navigationHistory[m_navigationIndex] != directory))
    {
        if (m_navigationIndex + 1 < m_navigationHistory.size())
            m_navigationHistory.removeRange(m_navigationIndex + 1, m_navigationHistory.size() - m_navigationIndex - 1);
        m_navigationHistory.add(directory);
        m_navigationIndex = m_navigationHistory.size() - 1;
    }

    m_displayedDirectory = directory;
    m_currentPathField.setDir(directory);
    refreshDirectory();
}

void ProjectsBrowserComponent::navigateBack()
{
    if (m_navigationIndex > 0)
    {
        --m_navigationIndex;
        navigateTo(m_navigationHistory[m_navigationIndex], false);
    }
}

void ProjectsBrowserComponent::navigateForward()
{
    if (m_navigationIndex + 1 < m_navigationHistory.size())
    {
        ++m_navigationIndex;
        navigateTo(m_navigationHistory[m_navigationIndex], false);
    }
}

void ProjectsBrowserComponent::updateSelectionAndValidation()
{
    if (getMode() == Mode::loadProject)
    {
        const bool valid = m_selectedFile.existsAsFile() && ProjectLifecycle::isPersistentProjectFile(m_selectedFile);
        m_selectedPathLabel.setText(valid ? m_selectedFile.getFullPathName() : "No project selected", juce::dontSendNotification);
        m_selectedPathLabel.setTooltip(valid ? m_selectedFile.getFullPathName() : juce::String{});
        m_primaryButton.setEnabled(valid && !m_operationInProgress);
    }
    else if (getMode() == Mode::saveProjectAs)
    {
        m_primaryButton.setEnabled(ProjectLifecycle::isValidProjectTarget(getSaveTarget()) && !m_operationInProgress);
    }
    else if (getMode() == Mode::confirmOverwrite || getMode() == Mode::confirmUnsavedChanges)
    {
        m_primaryButton.setEnabled(!m_operationInProgress);
    }
    else if (getMode() == Mode::operationError)
    {
        m_primaryButton.setEnabled(true);
    }
    else if (getMode() == Mode::saving || getMode() == Mode::committing)
    {
        m_primaryButton.setEnabled(false);
        m_secondaryButton.setEnabled(false);
    }

    if (getMode() != Mode::saving && getMode() != Mode::committing)
        m_secondaryButton.setEnabled(true);
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
    updateSelectionAndValidation();
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
    case Mode::loadProject:
        requestOpen(m_selectedFile);
        break;
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

    m_avs.m_projectSaveDir = target.getParentDirectory().getFullPathName();
    const auto continuation = m_workflow.completeSave();
    if (continuation.isValid())
    {
        executePendingOperation(ProjectWorkflow::UnsavedResolution::saved);
        return;
    }

    setMode(Mode::normal);
}

void ProjectsBrowserComponent::showUnsavedConfirmation(ProjectWorkflow::Operation)
{
    setMode(Mode::confirmUnsavedChanges);
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

    // Stopping an active recording at the workflow boundary can itself make the
    // edit dirty. Never treat the pre-lock clean decision as permission to drop it.
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

    showOperationError("The project operation handler is unavailable.", operation.file);
}

juce::File ProjectsBrowserComponent::getSelectedBrowserFile() const
{
    const auto row = m_listBox.getSelectedRow();
    return juce::isPositiveAndBelow(row, m_contentList.size()) ? m_contentList[row] : juce::File{};
}

juce::File ProjectsBrowserComponent::getSaveTarget() const
{
    const auto name = ProjectLifecycle::projectNameWithoutExtension(m_projectNameEditor.getText());
    if (!ProjectLifecycle::isValidProjectName(name))
        return {};
    return m_currentPathField.getCurrentPath().getChildFile(name + ".tracktionedit");
}

juce::File ProjectsBrowserComponent::getInitialDirectory(bool forSave) const
{
    const auto currentFile = m_evs.m_edit.editFileRetriever ? m_evs.m_edit.editFileRetriever() : juce::File{};
    if (forSave && ProjectLifecycle::isPersistentProjectFile(currentFile) && currentFile.getParentDirectory().isDirectory())
        return currentFile.getParentDirectory();

    const auto remembered = juce::File(forSave ? m_avs.m_projectSaveDir.get() : m_avs.m_projectLoadDir.get());
    if (remembered.isDirectory())
        return remembered;

    const auto projects = juce::File(m_avs.m_projectsDir.get());
    if (projects.isDirectory())
        return projects;
    return juce::File(m_avs.m_workDir.get());
}

bool ProjectsBrowserComponent::isBrowserMode() const noexcept
{
    const auto mode = getMode();
    return mode == Mode::loadProject || mode == Mode::saveProjectAs
           || mode == Mode::confirmOverwrite || mode == Mode::operationError;
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

    m_workflow.transitionTo(pending.type == ProjectWorkflow::OperationType::load ? Mode::loadProject : Mode::normal);
    showOperationError(errorMessage.isNotEmpty() ? errorMessage : "NextStudio could not complete the project operation.",
                       file != juce::File() ? file : pending.file);
}

void ProjectsBrowserComponent::sortList(int selectedID)
{
    if (m_contentList.size() > 1)
        sortByName(m_contentList, selectedID == 1);
    m_listBox.updateContent();
}

void ProjectsBrowserComponent::sortByName(juce::Array<juce::File> &list, bool forward)
{
    if (forward)
    {
        CompareNameForward compare;
        list.sort(compare);
    }
    else
    {
        CompareNameBackwards compare;
        list.sort(compare);
    }
}

void ProjectsBrowserComponent::setWorkingWidth(bool enabled)
{
    if (enabled == m_workingWidthRequested)
        return;
    m_workingWidthRequested = enabled;
    if (m_hostCallbacks.setWorkingWidth != nullptr)
        m_hostCallbacks.setWorkingWidth(enabled);
}
