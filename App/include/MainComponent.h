
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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ApplicationViewState.h"
#include "ComputerMidiKeyboardController.h"
#include "EditComponent.h"
#include "EditViewState.h"
#include "ExtendedUIBehavior.h"
#include "HeaderComponent.h"
#include "LowerRangeComponent.h"
#include "NextLookAndFeel.h"
#include "PluginWindow.h"
#include "SidebarComponent.h"
#include "SplitterCollapseController.h"
#include "ThemeHelpers.h"
#include "Utilities.h"

namespace te = tracktion_engine;
namespace NextStudio
{
class WineRendererFallback;
}

namespace NextStudio::Debug
{
class MainComponentDebugHost;
}

class EditorContainer : public juce::Component
{
public:
    EditorContainer(HeaderComponent &hc, EditComponent &ec)
        : m_header(hc),
          m_editComp(ec)
    {
        addAndMakeVisible(hc);
        addAndMakeVisible(ec);
    }
    void resized() override
    {
        auto area = getLocalBounds();
        m_header.setBounds(area.removeFromTop(60));
        area.removeFromTop(10);
        m_editComp.setBounds(area);
    }

private:
    HeaderComponent &m_header;
    EditComponent &m_editComp;
};

class MainComponent
    : public juce::Component
    , public juce::ApplicationCommandTarget
    , public juce::DragAndDropContainer
    , public juce::ChangeListener
    , public te::ValueTreeAllEventListener
    , private FlaggedAsyncUpdater
{
public:
    explicit MainComponent(ApplicationViewState &state, NextStudio::WineRendererFallback &wineRendererFallback, bool debugMode = false, const juce::File &debugSessionDirectory = {});
    ~MainComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    ApplicationCommandTarget *getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID> &commands) override;

    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo &result) override;

    bool perform(const juce::ApplicationCommandTarget::InvocationInfo &info) override;

    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
    void valueTreeChanged() override {}

    void setupEdit(juce::File = {});
    bool handleUnsavedEdit();
    GUIHelpers::ProjectSaveResult saveCurrentProject(bool saveAs = false);
    void handleContentPathChangedFromSettings();

private:
    void handleAsyncUpdate() override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    void saveSettings();
    void createTracksAndAssignInputs();
    void bindComputerMidiKeyboard(te::Edit *expectedEdit, int attemptsRemaining = 100);
    void openValidStartEdit();
    void setupSideBrowser();
    int getPreferredSidebarWidth() const;
    void handleSidebarSplitterMouseDown();
    void handleSidebarSplitterDrag(int dragDistance);
    void ensureUserDirectoriesAndSamples();
    void launchSetupWizardAsync();
    void runSetupWizard();

    void clearAudioTracks()
    {
        auto atList = te::getTracksOfType<te::AudioTrack>(*m_edit, true);

        for (auto &t : atList)
            m_edit->deleteTrack(t);
    }

    void updateTheme()
    {
        NS_LOG_DEBUG(viewstate, "updating application theme");
        ThemeHelpers::applyLookAndFeelColours(getLookAndFeel(), m_applicationState);
        if (m_editComponent)
            m_editComponent->updateButtonIcons();
        if (m_header)
            m_header->updateIcons();
        if (m_sideBarBrowser)
            m_sideBarBrowser->refreshThemeFromAppState();

        repaint();
    }

    ApplicationViewState &m_applicationState;
    NextStudio::WineRendererFallback &m_wineRendererFallback;
    NextLookAndFeel m_nextLookAndFeel;

    tracktion_engine::Engine m_engine{ProjectInfo::projectName, std::make_unique<ExtendedUIBehaviour>(), nullptr};
    juce::ApplicationCommandManager m_commandManager;

    tracktion_engine::SelectionManager m_selectionManager{m_engine};
    std::unique_ptr<tracktion_engine::Edit> m_edit;
    std::unique_ptr<EditViewState> m_editViewState;
    std::unique_ptr<EditComponent> m_editComponent;
    std::unique_ptr<HeaderComponent> m_header;
    std::unique_ptr<EditorContainer> m_editorContainer;
    std::unique_ptr<LowerRangeComponent> m_lowerRange;
    std::unique_ptr<SidebarComponent> m_sideBarBrowser;
    SplitterComponent m_sidebarSplitter;
    ComputerMidiKeyboardController m_computerMidiKeyboard;

    [[maybe_unused]] bool m_settingsLoaded{false};
    bool m_debugMode{false};
    bool m_saveTemp{false}, m_updateView{false}, m_updateSource{false}, m_updateTheme{false};
    bool m_hasUnsavedTemp{true};

    SplitterCollapseController m_sidebarSplitterCollapseController;
    int m_sidebarWidthAtMousedown{};

    juce::File m_tempDir;
    juce::TooltipWindow tooltipWindow{this, 500};

    friend class NextStudio::Debug::MainComponentDebugHost;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
