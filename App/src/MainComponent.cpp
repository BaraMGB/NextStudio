
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

/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "ArpeggiatorPlugin.h"
#include "ClipOverwriteCommand.h"
#include "DebugSessionEnvironment.h"
#include "InitialContentSetup.h"
#include "NextChorusPlugin.h"
#include "NextDelayPlugin.h"
#include "NextFilterPlugin.h"
#include "NextPhaserPlugin.h"
#include "NextSaturationPlugin.h"
#include "PeakLimiterPlugin.h"
#include "ProjectsBrowser.h"
#include "SetupWizard.h"
#include "SidebarComponent.h"
#include "SimpleSynthPlugin.h"
#include "SoundFontPlugin.h"
#include "SpectrumAnalyzerPlugin.h"
#include "ThemeHelpers.h"
#include "Utilities.h"
#include "WineRendererFallback.h"

MainComponent::MainComponent(ApplicationViewState &state, NextStudio::WineRendererFallback &wineRendererFallback, bool debugMode, const juce::File &debugSessionDirectory)
    : m_applicationState(state),
      m_wineRendererFallback(wineRendererFallback),
      m_nextLookAndFeel(state),
      m_sidebarSplitter(false),
      m_debugMode(debugMode)
{
    if (m_debugMode)
    {
        const auto debugTempDir = debugSessionDirectory == juce::File() ? NextStudio::Debug::SessionEnvironment::createDebugSessionTempDirectory() : debugSessionDirectory;
        if (m_engine.getTemporaryFileManager().setTempDirectory(debugTempDir))
        {
            NS_LOG_INFO(app, "debug shell temp directory: " + debugTempDir.getFullPathName());
        }
        else
        {
            NS_LOG_ERROR(app, "failed to set debug shell temp directory: " + debugTempDir.getFullPathName());
        }
    }

    const auto configuredWorkDir = juce::File(m_applicationState.m_workDir.get());
    const auto defaultWorkDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("NextStudio");
    const bool configuredWorkDirExists = configuredWorkDir.exists() && configuredWorkDir.isDirectory();
    const bool needsSetupWizard = !m_applicationState.m_setupComplete || !configuredWorkDirExists;

    // Keep the current UX goal: preselect the fallback path, but don't create it until setup is resolved.
    if (needsSetupWizard && !configuredWorkDirExists)
        m_applicationState.setRootFolder(defaultWorkDir);

    setWantsKeyboardFocus(true);
    m_computerMidiKeyboard.setLayout(ComputerMidiKeyboardLayout::loadFrom(m_applicationState));
    m_computerMidiKeyboard.attachTo(*this);
    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
        uiBehaviour->setComputerMidiKeyboardController(&m_computerMidiKeyboard);

    juce::LookAndFeel::setDefaultLookAndFeel(&m_nextLookAndFeel);
    NextStudio::WineRendererFallback::configureFontFallback(m_nextLookAndFeel);
    updateTheme();

    if (!needsSetupWizard)
        ensureUserDirectoriesAndSamples();

    addAndMakeVisible(m_sidebarSplitter);
    addChildComponent(m_projectSaveInteractionBlocker);
    m_projectSaveInteractionBlocker.onClickOutside = [this]
    {
        if (m_sideBarBrowser)
            m_sideBarBrowser->dismissProjectSaveAs();
    };
    m_sidebarSplitter.onMouseDown = [this]() { handleSidebarSplitterMouseDown(); };
    m_sidebarSplitter.onDrag = [this](int dragDistance) { handleSidebarSplitterDrag(dragDistance); };

    m_engine.getPluginManager().createBuiltInType<SimpleSynthPlugin>();
    m_engine.getPluginManager().createBuiltInType<ArpeggiatorPlugin>();
    m_engine.getPluginManager().createBuiltInType<SpectrumAnalyzerPlugin>();
    m_engine.getPluginManager().createBuiltInType<PeakLimiterPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextDelayPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextChorusPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextPhaserPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextSaturationPlugin>();
    m_engine.getPluginManager().createBuiltInType<NextFilterPlugin>();
    m_engine.getPluginManager().createBuiltInType<SoundFontPlugin>();

    // Always start with the Projects sidebar expanded at its default width.
    m_applicationState.m_sidebarWidth = SidebarLayout::defaultExpandedWidth;
    m_applicationState.m_sidebarCollapsed = false;
    openValidStartEdit();

    m_commandManager.registerAllCommandsForTarget(this);
    m_commandManager.registerAllCommandsForTarget(m_editComponent.get());
    m_commandManager.registerAllCommandsForTarget(&m_editComponent->getTrackListView());
    m_commandManager.registerAllCommandsForTarget(&m_lowerRange->getPianoRollEditor());

    m_selectionManager.addChangeListener(this);
    m_applicationState.m_applicationStateValueTree.addListener(this);

    if (needsSetupWizard)
    {
        NS_LOG_INFO(setup, "setup wizard scheduled");
        launchSetupWizardAsync();
    }
}

MainComponent::~MainComponent()
{
    m_computerMidiKeyboard.setKeyboardState(nullptr);

    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
    {
        uiBehaviour->setFocusedEdit(nullptr);
        uiBehaviour->setComputerMidiKeyboardController(nullptr);
    }

    m_applicationState.m_applicationStateValueTree.removeListener(this);
    m_selectionManager.removeChangeListener(this);
    if (m_edit)
        m_edit->state.removeListener(this);

    if (m_header)
        m_header->removeAllChangeListeners();

    if (m_editComponent)
        m_editComponent->getSongEditor().clear();

    // Explicitly destroy UI components and then the Edit to ensure correct shutdown order
    m_editorContainer = nullptr;
    m_header = nullptr;
    m_editComponent = nullptr;
    m_lowerRange = nullptr;
    m_sideBarBrowser = nullptr;
    m_editViewState = nullptr;
    m_edit = nullptr;

    saveSettings();
    if (!m_debugMode)
        m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    m_computerMidiKeyboard.detachFrom(*this);
    setLookAndFeel(nullptr);
}

void MainComponent::paint(juce::Graphics &g)
{
    g.setColour(m_applicationState.getMainFrameColour());
    g.fillRect(getLocalBounds());
}

int MainComponent::getPreferredSidebarWidth() const
{
    return SidebarLayout::getPreferredWidth((int)m_applicationState.m_sidebarWidth);
}

int MainComponent::getMaximumLowerRangeHeight() const
{
    if (m_editViewState == nullptr)
        return LowerRangeLayout::defaultExpandedHeight;

    return LowerRangeLayout::getMaximumExpandedHeight(getHeight(), (int)m_editViewState->m_timeLineHeight);
}

void MainComponent::handleSidebarSplitterMouseDown()
{
    const bool collapsed = m_applicationState.m_sidebarCollapsed;
    m_sidebarWidthAtMousedown = getPreferredSidebarWidth();

    m_sidebarSplitterCollapseController.beginDrag(collapsed, SidebarLayout::getTransitionDistance(m_sidebarWidthAtMousedown, collapsed));
}

void MainComponent::handleSidebarSplitterDrag(int dragDistance)
{
    const bool collapsed = m_applicationState.m_sidebarCollapsed;

    if (m_projectSaveAsInteractionBlocked)
    {
        const auto resizedWidth = SidebarLayout::getResizedWidth(m_sidebarWidthAtMousedown, dragDistance);
        if (resizedWidth != (int)m_applicationState.m_sidebarWidth)
        {
            m_applicationState.m_sidebarWidth = resizedWidth;
            resized();
        }
        return;
    }

    const bool requestedCollapsed = m_sidebarSplitterCollapseController.getCollapsedState(-dragDistance, collapsed);

    if (requestedCollapsed != collapsed)
    {
        m_applicationState.m_sidebarWidth = requestedCollapsed ? SidebarLayout::minimumExpandedWidth : m_sidebarWidthAtMousedown;
        m_applicationState.m_sidebarCollapsed = requestedCollapsed;
        resized();
        return;
    }

    if (m_sidebarSplitterCollapseController.startedCollapsed() || collapsed)
        return;

    const auto resizedWidth = SidebarLayout::getResizedWidth(m_sidebarWidthAtMousedown, dragDistance);
    if (resizedWidth != (int)m_applicationState.m_sidebarWidth)
    {
        m_applicationState.m_sidebarWidth = resizedWidth;
        resized();
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto &evs = m_editComponent->getEditViewState();
    auto lowerRangeHeight = LowerRangeComponent::collapsedHeight;
    if (!m_applicationState.m_lowerRangeCollapsed)
    {
        if (evs.getLowerRangeView() == LowerRangeView::midiEditor)
            lowerRangeHeight = LowerRangeLayout::clampExpandedHeight((int)evs.m_midiEditorHeight, getMaximumLowerRangeHeight());
        else if (evs.getLowerRangeView() == LowerRangeView::pluginRack || evs.getLowerRangeView() == LowerRangeView::mixer)
            lowerRangeHeight = LowerRangeComponent::defaultExpandedHeight;
    }

    auto lowerRange = area.removeFromBottom(lowerRangeHeight);
    const auto sidebarWidth = m_applicationState.m_sidebarCollapsed
                                ? SidebarLayout::collapsedWidth
                                : getPreferredSidebarWidth();
    m_sideBarBrowser->setBounds(area.removeFromLeft(sidebarWidth));
    m_sidebarSplitter.setBounds(area.removeFromLeft(10));
    m_editorContainer->setBounds(area);
    m_lowerRange->setBounds(lowerRange);

    m_projectSaveInteractionBlocker.setBounds(getLocalBounds());
    if (m_projectSaveAsInteractionBlocked)
    {
        m_projectSaveInteractionBlocker.toFront(false);
        m_sidebarSplitter.toFront(false);
        m_sideBarBrowser->toFront(false);
    }
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID> &commands)
{
    juce::Array<juce::CommandID> ids{KeyPressCommandIDs::togglePlay, KeyPressCommandIDs::toggleRecord, KeyPressCommandIDs::play, KeyPressCommandIDs::stop,

                                     KeyPressCommandIDs::loopAroundSelection,
                                     // KeyPressCommandIDs::loopOn,
                                     // KeyPressCommandIDs::loopOff,
                                     KeyPressCommandIDs::loopAroundAll, KeyPressCommandIDs::loopToggle,

                                     // KeyPressCommandIDs::toggleSnap,
                                     KeyPressCommandIDs::toggleMetronome,
                                     // KeyPressCommandIDs::snapToBar,
                                     // KeyPressCommandIDs::snapToBeat,
                                     // KeyPressCommandIDs::snapToGrid,
                                     // KeyPressCommandIDs::snapToTime,
                                     // KeyPressCommandIDs::snapToOff,

                                     KeyPressCommandIDs::undo, KeyPressCommandIDs::redo,

                                     KeyPressCommandIDs::debugOutputEdit, KeyPressCommandIDs::saveProject};

    commands.addArray(ids);
}

void MainComponent::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result)
{
    switch (commandID)
    {
    case KeyPressCommandIDs::togglePlay:
        result.setInfo("Play/Pause", "Toggle play", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
        result.addDefaultKeypress(juce::KeyPress::numberPad0, 0);
        break;
    case KeyPressCommandIDs::play:
        result.setInfo("Play", "Play", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::returnKey, 0);
        break;
    case KeyPressCommandIDs::toggleRecord:
        result.setInfo("Record", "Record", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::numberPadMultiply, 0);
        break;
    case KeyPressCommandIDs::stop:
        result.setInfo("Stop", "Stop", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::spaceKey, juce::ModifierKeys::shiftModifier);
        result.addDefaultKeypress(juce::KeyPress::numberPadDecimalPoint, 0);
        break;
    case KeyPressCommandIDs::loopToggle:
        result.setInfo("Loop", "Loop", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::loopAroundAll:
        result.setInfo("Loop around all", "Loop around all", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier | juce::ModifierKeys::altModifier);
        break;
    case KeyPressCommandIDs::toggleMetronome:
        result.setInfo("Metronome", "Metronome", "Transport", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("m").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::loopAroundSelection:
        result.setInfo("Loop around selection", "Loop around selection", "Selection", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("l").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case KeyPressCommandIDs::debugOutputEdit:
        result.setInfo("Debug output edit", "Debug output edit", "Debug", 0);
        result.addDefaultKeypress(juce::KeyPress::F10Key, 0);
        break;
    case KeyPressCommandIDs::undo:
        result.setInfo("Undo last action", "Undo", "Song Editor", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    case KeyPressCommandIDs::redo:
        result.setInfo("Redo last action", "Redo", "Song Editor", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("z").getKeyCode(), juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
        break;
    case KeyPressCommandIDs::saveProject:
        result.setInfo("Save project", "Save the current project", "Project", 0);
        result.addDefaultKeypress(juce::KeyPress::createFromDescription("s").getKeyCode(), juce::ModifierKeys::commandModifier);
        break;
    default:
        break;
    }
}

bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo &info)
{
    if (m_projectSaveAsInteractionBlocked)
        return false;

    NS_LOG_DEBUG(workflow, "command invoked: id=" + juce::String(static_cast<int>(info.commandID)));
    switch (info.commandID)
    {
    case KeyPressCommandIDs::togglePlay:
        EngineHelpers::togglePlay(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::play:
        if (m_lowerRange == nullptr || !m_lowerRange->getPianoRollEditor().confirmPendingPasteIfActive())
            EngineHelpers::play(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::stop:
        EngineHelpers::stopPlay(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::toggleRecord:
        NS_LOG_INFO(transport, "toggle record requested");
        EngineHelpers::toggleRecord(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::loopToggle:
        EngineHelpers::toggleLoop(*m_edit);
        break;
    case KeyPressCommandIDs::loopAroundSelection:
        EngineHelpers::loopAroundSelection(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::loopOff:
        EngineHelpers::loopOff(*m_edit);
        break;
    case KeyPressCommandIDs::loopOn:
        EngineHelpers::loopOn(*m_edit);
        break;
    case KeyPressCommandIDs::loopAroundAll:
        EngineHelpers::loopAroundAll(*m_edit);
        break;
    case KeyPressCommandIDs::toggleSnap:
        EngineHelpers::toggleSnap(m_editComponent->getEditViewState());
        break;
    case KeyPressCommandIDs::toggleMetronome:
        EngineHelpers::toggleMetronome(*m_edit);
        break;
    case KeyPressCommandIDs::debugOutputEdit:
    {
        auto editString = m_edit->state.toXmlString();
        NS_LOG_DEBUG(edit, "edit state dump requested\n" + editString);

        break;
    }

    case KeyPressCommandIDs::undo:
        m_edit->undo();
        break;
    case KeyPressCommandIDs::redo:
        m_edit->redo();
        break;
    case KeyPressCommandIDs::saveProject:
        saveCurrentProject();
        break;
    // case KeyPressCommandIDs::snapToBar:
    //     EngineHelpers::snapToBar(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToBeat:
    //     EngineHelpers::snapToBeat(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToGrid:
    //     EngineHelpers::snapToGrid(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToTime:
    //     EngineHelpers::snapToTime(m_editComponent->getEditViewState());
    //     break;
    // case KeyPressCommandIDs::snapToOff:
    //     EngineHelpers::snapToOff(m_editComponent->getEditViewState());
    //     break;
    default:
        return false;
    }
    return true;
}

void MainComponent::valueTreePropertyChanged(juce::ValueTree &vt, const juce::Identifier &property)
{
    if (property == te::IDs::looping)
        m_header->loopButtonClicked();

    if (property == IDs::pianorollHeight || property == IDs::lowerRangeView || property == IDs::LowerRangeCollapsed
        || property == IDs::SidebarWidth || property == IDs::SidebarCollapsed)
        markAndUpdate(m_updateView);

    if (property == te::IDs::source || property == te::IDs::state)
        markAndUpdate(m_updateSource);

    if (vt.hasType(IDs::ThemeState))
        markAndUpdate(m_updateTheme);

    if (vt.hasType(IDs::ComputerMidiKeyboard))
        m_computerMidiKeyboard.setLayout(ComputerMidiKeyboardLayout::loadFrom(m_applicationState));

    if (property == te::IDs::lastSignificantChange)
        markAndUpdate(m_saveTemp);
}
void MainComponent::handleAsyncUpdate()
{
    if (compareAndReset(m_saveTemp) && !compareAndReset(m_updateSource))
        m_hasUnsavedTemp = true;

    if (compareAndReset(m_updateView))
        resized();

    if (compareAndReset(m_updateTheme))
        updateTheme();
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster *source)
{
    if (auto *browser = dynamic_cast<BrowserBaseComponent *>(source))
    {
        const auto request = browser->m_projectRequest.take();

        juce::Component::SafePointer<MainComponent> safeThis(this);
        if (request.action == ProjectLifecycle::ProjectAction::loadProject)
        {
            juce::Component::SafePointer<ProjectsBrowserComponent> safeProjectBrowser(dynamic_cast<ProjectsBrowserComponent *>(browser));
            juce::MessageManager::callAsync(
                [safeThis, safeProjectBrowser, editFile = request.file, unsavedChangesHandled = request.unsavedChangesHandled]
                {
                    if (safeThis == nullptr)
                        return;

                    juce::String errorMessage;
                    const bool loaded = safeThis->setupEdit(editFile, unsavedChangesHandled, &errorMessage);
                    if (!loaded && safeProjectBrowser != nullptr)
                        safeProjectBrowser->completeLoadOperation(false, errorMessage);
                });
        }
        else if (request.action == ProjectLifecycle::ProjectAction::newProject)
        {
            juce::MessageManager::callAsync(
                [safeThis]
                {
                    if (safeThis != nullptr)
                        safeThis->setupEdit(juce::File());
                });
        }
    }

    if (source == &m_selectionManager && m_editViewState)
    {
        if (m_editViewState->m_applicationState.m_exclusiveMidiFocusEnabled)
        {
            NS_LOG_DEBUG(selection, "selection changed; updating exclusive MIDI focus");
            EngineHelpers::setMidiInputFocusToSelection(*m_editViewState);
        }
    }
}

void MainComponent::openValidStartEdit()
{
    m_tempDir = m_engine.getTemporaryFileManager().getTempDirectory();
    m_tempDir.createDirectory();

    auto f = Helpers::findRecentEdit(m_tempDir);
    if (f.existsAsFile())
    {
        NS_LOG_WARN(autosave, "recovery file found: " + f.getFullPathName());
        auto result = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon, "Restore crashed project?", "It seems, NextStudio is crashed last time. Do you want to restore the last session?", "Yes", "No");
        if (result)
        {
            setupEdit(f);
            return;
        }
        else
        {
            m_tempDir.deleteRecursively();
            m_tempDir.createDirectory();
        }
    }

    setupEdit(juce::File());
}

void MainComponent::setupSideBrowser()
{
    m_sideBarBrowser = std::make_unique<SidebarComponent>(*m_editViewState, m_commandManager);
    addAndMakeVisible(*m_sideBarBrowser);
    m_sideBarBrowser->updateParentsListener();
}

void MainComponent::ensureUserDirectoriesAndSamples() { InitialContentSetup::populateBundledContent(juce::File(m_applicationState.m_workDir.get())); }

void MainComponent::launchSetupWizardAsync()
{
    juce::Component::SafePointer<MainComponent> safeThis(this);

    juce::MessageManager::callAsync(
        [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->runSetupWizard();
        });
}

void MainComponent::runSetupWizard()
{
    auto wizard = std::make_unique<SetupWizard>(m_applicationState, m_engine);
    wizard->setSize(1400, 1000);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(wizard.release());
    options.componentToCentreAround = this;
    options.dialogTitle = "NextStudio Setup Wizard";
    options.dialogBackgroundColour = m_applicationState.getBackgroundColour1();
    options.escapeKeyTriggersCloseButton = false;
    options.useNativeTitleBar = true;
    options.resizable = false;

    auto *dialog = options.create();
    m_wineRendererFallback.applyTo(*dialog);
    dialog->enterModalState(true, nullptr, true);
    const auto wizardResult = dialog->runModalLoop();

    if (wizardResult != 1)
    {
        // Aborting setup falls back to ~/NextStudio by product decision.
        const auto defaultWorkDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("NextStudio");
        m_applicationState.setRootFolder(defaultWorkDir);
        ThemeHelpers::applyBuiltInTheme(m_applicationState, ThemeHelpers::getDefaultBuiltInThemeName());
        m_applicationState.m_setupComplete = true;
        m_applicationState.saveState();
    }

    handleContentPathChangedFromSettings();

    juce::Component::SafePointer<juce::Component> mainWindow(getTopLevelComponent());
    juce::MessageManager::callAsync(
        [mainWindow]
        {
            if (mainWindow == nullptr)
                return;

            mainWindow->setVisible(true);
            mainWindow->toFront(true);
            mainWindow->repaint();
        });
}

void MainComponent::handleContentPathChangedFromSettings()
{
    ensureUserDirectoriesAndSamples();
    if (m_sideBarBrowser)
        m_sideBarBrowser->refreshBrowsersFromAppState();
    resized();
}

bool MainComponent::setupEdit(juce::File editFile, bool unsavedChangesHandled, juce::String *errorMessage)
{
    const auto fail = [errorMessage](const juce::String &message)
    {
        if (errorMessage != nullptr)
            *errorMessage = message;
        NS_LOG_ERROR(project, message);
        return false;
    };
    m_tempDir.createDirectory();

    const bool isNewEdit = (editFile == juce::File());
    const bool isRecoveryEdit = !isNewEdit && editFile.getParentDirectory() == m_tempDir;

    if (!isNewEdit)
    {
        const auto loadStatus = ProjectLifecycle::inspectLoadFile(editFile, isRecoveryEdit);
        if (loadStatus != ProjectLifecycle::LoadFileStatus::valid)
        {
            juce::String reason;
            switch (loadStatus)
            {
            case ProjectLifecycle::LoadFileStatus::missing:
                reason = "The selected project file no longer exists.";
                break;
            case ProjectLifecycle::LoadFileStatus::unsupportedExtension:
                reason = "The selected file is not a supported project file.";
                break;
            case ProjectLifecycle::LoadFileStatus::empty:
                reason = "The selected project file is empty.";
                break;
            case ProjectLifecycle::LoadFileStatus::invalidData:
                reason = "The selected project file is damaged or invalid.";
                break;
            case ProjectLifecycle::LoadFileStatus::valid:
                break;
            }

            return fail(reason + "\n" + editFile.getFullPathName());
        }
    }

    if (m_edit && !unsavedChangesHandled && !handleUnsavedEdit())
        return fail("Opening the project was cancelled because the current project has unsaved changes.");

    if (isNewEdit)
        editFile = m_tempDir.getNonexistentChildFile("autosave", ".nextTemp", false);

    // Validate and construct the replacement before destroying the current project or its recovery file.
    std::unique_ptr<te::Edit> replacementEdit;
    try
    {
        replacementEdit = isNewEdit ? te::createEmptyEdit(m_engine, editFile) : te::loadEditFromFile(m_engine, editFile);
    }
    catch (const std::exception &e)
    {
        GUIHelpers::log("ERROR: Exception while loading project: " + juce::String(e.what()));
    }

    if (!replacementEdit)
        return fail("NextStudio could not read the selected project.\n" + editFile.getFullPathName());

    // Restore a temporarily enlarged sidebar before its old component hierarchy is replaced.
    if (m_projectBrowserExpandedSidebar)
        setProjectBrowserWorkingMode(false);

    m_selectionManager.deselectAll();

    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
        uiBehaviour->setFocusedEdit(nullptr);

    m_computerMidiKeyboard.setKeyboardState(nullptr);

    if (m_edit)
    {
        m_edit->state.removeListener(this);
        m_edit->getTransport().removeChangeListener(this);
    }

    // Destroy every object that refers to the old Edit before replacing it.
    m_editorContainer = nullptr;
    m_header = nullptr;
    m_editComponent = nullptr;
    m_lowerRange = nullptr;
    m_sideBarBrowser = nullptr;
    m_editViewState = nullptr;
    m_edit = nullptr;

    // A recovery project must keep its source file. For other switches, remove only old
    // recovery snapshots; the replacement Edit may already have created other temp resources.
    if (!isRecoveryEdit)
    {
        const auto oldRecoveryFiles = m_tempDir.findChildFiles(juce::File::findFiles, false, "*.nextTemp");
        for (const auto &oldRecoveryFile : oldRecoveryFiles)
            oldRecoveryFile.deleteFile();
    }
    m_tempDir.createDirectory();

    m_edit = std::move(replacementEdit);

    for (auto *track : te::getAudioTracks(*m_edit))
        if (ClipEditing::hasOverlaps(*track))
            GUIHelpers::log("WARNING: Loaded track contains overlapping clips: " + track->getName() + " (" + ClipEditing::describeOverlaps(*track) + ")");

    if (auto *uiBehaviour = dynamic_cast<ExtendedUIBehaviour *>(&m_engine.getUIBehaviour()))
        uiBehaviour->setFocusedEdit(m_edit.get());

    if (isNewEdit)
        clearAudioTracks();

    m_edit->setTempDirectory(m_tempDir);

    if (auto *w = dynamic_cast<juce::DocumentWindow *>(getParentComponent()))
        w->setName(isNewEdit ? "Untitled" : editFile.getFileNameWithoutExtension());

    m_edit->playInStopEnabled = true;
    m_edit->getTransport().addChangeListener(this);

    createTracksAndAssignInputs();

    bindComputerMidiKeyboard(m_edit.get());

    if (!editFile.existsAsFile())
        te::EditFileOperations(*m_edit).writeToFile(editFile, true);

    m_editViewState = std::make_unique<EditViewState>(*m_edit, m_selectionManager, m_applicationState);
    m_editComponent = std::make_unique<EditComponent>(*m_edit, *m_editViewState, m_applicationState, m_selectionManager, m_commandManager);
    m_lowerRange = std::make_unique<LowerRangeComponent>(*m_editViewState);
    m_editViewState->setLowerRangeView(LowerRangeView::mixer);

    m_edit->state.addListener(this);

    m_header = std::make_unique<HeaderComponent>(m_editComponent->getEditViewState(), m_applicationState, m_commandManager);
    m_editorContainer = std::make_unique<EditorContainer>(*m_header, *m_editComponent);

    addAndMakeVisible(*m_editorContainer);
    addAndMakeVisible(*m_lowerRange);

    setupSideBrowser();

    addKeyListener(m_commandManager.getKeyMappings());
    resized();

    // Startup housekeeping should not appear in the user's undo history.
    m_edit->getUndoManager().clearUndoHistory();
    m_edit->resetChangedStatus();
    return true;
}

void MainComponent::saveSettings()
{
    if (auto *window = dynamic_cast<juce::ResizableWindow *>(getTopLevelComponent()))
        m_applicationState.setWindowGeometry(window->getWindowStateAsString(), window->getBounds());
    else
        m_applicationState.setBounds(getScreenBounds());

    m_applicationState.saveState();
}

GUIHelpers::ProjectSaveResult MainComponent::saveCurrentProject(bool saveAs)
{
    if (!m_editViewState)
        return GUIHelpers::ProjectSaveResult::failed;

    const auto currentFile = m_edit->editFileRetriever ? m_edit->editFileRetriever() : juce::File{};
    if (ProjectLifecycle::shouldChooseSaveTarget(currentFile, saveAs))
    {
        if (m_sideBarBrowser)
            m_sideBarBrowser->beginProjectSaveAs();
        return GUIHelpers::ProjectSaveResult::cancelled;
    }

    const auto result = saveCurrentProjectTo(currentFile);
    if (result == GUIHelpers::ProjectSaveResult::failed && m_sideBarBrowser)
        m_sideBarBrowser->showProjectError("NextStudio could not save the project.", currentFile);
    return result;
}

GUIHelpers::ProjectSaveResult MainComponent::saveCurrentProjectTo(const juce::File &targetFile)
{
    if (!m_editViewState)
        return GUIHelpers::ProjectSaveResult::failed;

    const auto result = GUIHelpers::saveEditToFile(*m_editViewState, targetFile);
    if (result != GUIHelpers::ProjectSaveResult::saved)
        return result;

    if (m_editComponent)
        m_editComponent->projectSaved();

    const auto projectFile = m_edit->editFileRetriever ? m_edit->editFileRetriever() : juce::File{};
    if (auto *window = dynamic_cast<juce::DocumentWindow *>(getParentComponent()))
        window->setName(projectFile.getFileNameWithoutExtension());

    if (m_sideBarBrowser)
        m_sideBarBrowser->projectWasSaved(projectFile);
    return result;
}

void MainComponent::setProjectSaveAsInteractionBlocked(bool blocked)
{
    if (m_projectSaveAsInteractionBlocked == blocked)
        return;

    m_projectSaveAsInteractionBlocked = blocked;
    if (blocked)
        m_computerMidiKeyboard.detachFrom(*this);
    else
        m_computerMidiKeyboard.attachTo(*this);

    m_projectSaveInteractionBlocker.setVisible(blocked);
    if (blocked)
    {
        m_projectSaveInteractionBlocker.setBounds(getLocalBounds());
        m_projectSaveInteractionBlocker.toFront(false);
        m_sidebarSplitter.toFront(false);
        if (m_sideBarBrowser)
            m_sideBarBrowser->toFront(false);
    }

    if (m_sideBarBrowser)
        m_sideBarBrowser->repaint();
}

void MainComponent::setProjectBrowserWorkingMode(bool enabled)
{
    constexpr int workingWidth = SidebarLayout::defaultExpandedWidth;
    if (enabled)
    {
        if (m_projectBrowserExpandedSidebar)
            return;
        m_sidebarWidthBeforeProjectBrowser = getPreferredSidebarWidth();
        if (m_sidebarWidthBeforeProjectBrowser < workingWidth)
        {
            m_applicationState.m_sidebarWidth = workingWidth;
            m_projectBrowserExpandedSidebar = true;
            resized();
        }
        return;
    }

    if (!m_projectBrowserExpandedSidebar)
        return;

    // Restore only if the user has not resized the sidebar during the operation.
    if ((int)m_applicationState.m_sidebarWidth == workingWidth && m_sidebarWidthBeforeProjectBrowser > 0)
        m_applicationState.m_sidebarWidth = m_sidebarWidthBeforeProjectBrowser;
    m_projectBrowserExpandedSidebar = false;
    m_sidebarWidthBeforeProjectBrowser = -1;
    resized();
}

bool MainComponent::handleUnsavedEdit()
{
    if (m_edit->hasChangedSinceSaved())
    {
        const auto result = juce::AlertWindow::showYesNoCancelBox(juce::AlertWindow::QuestionIcon, "Unsaved Project", "Do you want to save the project?", "Yes", "No", "Cancel");

        switch (result)
        {
        case 1:
            return ProjectLifecycle::shouldProceedAfterUnsavedChoice(ProjectLifecycle::UnsavedChoice::save, saveCurrentProject());
        case 2:
            return ProjectLifecycle::shouldProceedAfterUnsavedChoice(ProjectLifecycle::UnsavedChoice::discard, GUIHelpers::ProjectSaveResult::cancelled);
        case 3:
        default:
            return ProjectLifecycle::shouldProceedAfterUnsavedChoice(ProjectLifecycle::UnsavedChoice::cancel, GUIHelpers::ProjectSaveResult::cancelled);
        }
    }
    return true;
}

void MainComponent::bindComputerMidiKeyboard(te::Edit *expectedEdit, int attemptsRemaining)
{
    if (expectedEdit == nullptr || m_edit.get() != expectedEdit)
        return;

    if (auto *virtualMidiInput = EngineHelpers::getVirtualMidiInputDevice(*expectedEdit))
    {
        m_computerMidiKeyboard.setKeyboardState(&virtualMidiInput->keyboardState);
        return;
    }

    m_engine.getDeviceManager().dispatchPendingUpdates();
    expectedEdit->dispatchPendingUpdatesSynchronously();

    if (auto *virtualMidiInput = EngineHelpers::getVirtualMidiInputDevice(*expectedEdit))
    {
        m_computerMidiKeyboard.setKeyboardState(&virtualMidiInput->keyboardState);
        return;
    }

    if (attemptsRemaining <= 0)
    {
        NS_LOG_ERROR(engine, "virtual MIDI input did not become available for the computer keyboard");
        return;
    }

    juce::Timer::callAfterDelay(50,
                                [safeThis = juce::Component::SafePointer<MainComponent>(this), expectedEdit, attemptsRemaining]
                                {
                                    if (safeThis != nullptr)
                                        safeThis->bindComputerMidiKeyboard(expectedEdit, attemptsRemaining - 1);
                                });
}

void MainComponent::createTracksAndAssignInputs()
{
    auto &dm = m_engine.getDeviceManager();

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice(i))
            wip->setStereoPair(false);

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice(i))
        {
            wip->setMonitorMode(te::InputDevice::MonitorMode::off);
            wip->setEnabled(false);
        }

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
        if (auto mip = dm.getMidiInDevice(i))
        {
            mip->setMonitorMode(te::InputDevice::MonitorMode::on);
            mip->setEnabled(true);
        }

    m_edit->getTransport().ensureContextAllocated();
    m_edit->restartPlayback();
}
