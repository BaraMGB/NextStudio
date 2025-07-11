
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

#include "HeaderComponent.h"
#include "LowerRangeComponent.h"
#include "NextLookAndFeel.h"
#include "SidebarComponent.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "EditComponent.h"
#include "Utilities.h"
#include "ExtendedUIBehavior.h"
#include "PluginWindow.h"

namespace te = tracktion_engine;

class EditorContainer : public juce::Component
{
public:
    EditorContainer(HeaderComponent& hc, EditComponent& ec)
        : m_header(hc)
        , m_editComp(ec)
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
    HeaderComponent& m_header;
    EditComponent& m_editComp;
};

class MainComponent   : public juce::Component
                      , public juce::ApplicationCommandTarget
                      , public juce::DragAndDropContainer
                      , public juce::ChangeListener
                      , public te::ValueTreeAllEventListener
                      , private FlaggedAsyncUpdater
{
public:
    explicit MainComponent(ApplicationViewState& state);
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    bool keyStateChanged(bool isKeyDown) override ;
    ApplicationCommandTarget* getNextCommandTarget() override   { return nullptr; }
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;

    void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    
    bool perform (const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
    void valueTreeChanged() override {}

    void setupEdit (juce::File = {});
    bool handleUnsavedEdit();

private:


    void handleAsyncUpdate () override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void saveSettings();
    void createTracksAndAssignInputs();
    void openValidStartEdit();
    void setupSideBrowser();

    void clearAudioTracks()
    {
        auto atList = te::getTracksOfType<te::AudioTrack>(*m_edit, true);

        for (auto & t : atList)
            m_edit->deleteTrack (t);
    }

    void updateTheme()
    {
        GUIHelpers::log("update theme");
        getLookAndFeel().setColour(juce::AlertWindow::backgroundColourId, m_applicationState.getBackgroundColour1());
        getLookAndFeel().setColour(juce::AlertWindow::textColourId, m_applicationState.getTextColour());
        getLookAndFeel().setColour(juce::AlertWindow::outlineColourId, m_applicationState.getBorderColour());
        getLookAndFeel().setColour(juce::Label::textColourId, m_applicationState.getTextColour());
        getLookAndFeel().setColour(juce::ComboBox::ColourIds::textColourId, m_applicationState.getTextColour());
        getLookAndFeel().setColour(juce::TabbedButtonBar::ColourIds::tabTextColourId, m_applicationState.getTextColour());
        getLookAndFeel().setColour(juce::TabbedButtonBar::ColourIds::frontTextColourId, m_applicationState.getPrimeColour());
        getLookAndFeel().setColour(juce::TableHeaderComponent::ColourIds::textColourId , m_applicationState.getTextColour());
        getLookAndFeel().setColour(juce::TableHeaderComponent::ColourIds::backgroundColourId, m_applicationState.getBackgroundColour2());
        getLookAndFeel().setColour(juce::TableHeaderComponent::ColourIds::outlineColourId , m_applicationState.getBorderColour());
        getLookAndFeel().setColour(juce::TableHeaderComponent::ColourIds::highlightColourId , m_applicationState.getPrimeColour());
        getLookAndFeel().setColour(juce::TableListBox::textColourId, m_applicationState.getTextColour());
        getLookAndFeel().setColour(juce::DrawableButton::textColourId , m_applicationState.getButtonTextColour());
        getLookAndFeel().setColour(juce::DrawableButton::textColourOnId, m_applicationState.getButtonTextColour());

        if (m_editComponent)
            m_editComponent->updateButtonIcons();
        if (m_header)
            m_header->updateIcons();

        repaint();
    }

    ApplicationViewState &                              m_applicationState;
    NextLookAndFeel                                     m_nextLookAndFeel;

    tracktion_engine::Engine                            m_engine
                                { ProjectInfo::projectName
                                , std::make_unique<ExtendedUIBehaviour>()
                                , nullptr };
    juce::ApplicationCommandManager                     m_commandManager;

    tracktion_engine::SelectionManager                  m_selectionManager{ m_engine };
    std::unique_ptr<tracktion_engine::Edit>             m_edit;
    std::unique_ptr<EditViewState>                      m_editViewState;
    std::unique_ptr<EditComponent>                      m_editComponent;
    std::unique_ptr<HeaderComponent>                    m_header;
    std::unique_ptr<EditorContainer>                    m_editorContainer;
    std::unique_ptr<LowerRangeComponent>                m_lowerRange;
    std::unique_ptr<SidebarComponent>                   m_sideBarBrowser;
    SplitterComponent                                   m_sidebarSplitter;

    [[maybe_unused]] bool                               m_settingsLoaded {false};
    bool                                                m_saveTemp{false}, m_updateView{false},
                                                        m_updateSource{false}, m_updateTheme{false};
    bool m_hasUnsavedTemp {true};

    int m_sidebarWidthAtMousedown; 

    juce::File m_tempDir;
    juce::Array<juce::KeyPress> m_pressedKeysForMidiKeyboard;
    juce::TooltipWindow tooltipWindow{this, 500};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
