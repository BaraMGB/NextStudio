/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "HeaderComponent.h"
#include "NextLookAndFeel.h"
#include "MenuBar.h"
#include "SideBarBrowser.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "EditComponent.h"
#include "Utilities.h"
#include "PluginWindow.h"

namespace te = tracktion_engine;

class MainComponent   : public juce::Component
                      , public juce::DragAndDropContainer
                      , public juce::ChangeListener
                      , public te::ValueTreeAllEventListener
                      , private FlaggedAsyncUpdater
                      , private juce::Timer
{
public:
    explicit MainComponent(ApplicationViewState& state);
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress &key) override;

    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged, const juce::Identifier &property) override;
    void valueTreeChanged() override {}

    void setupEdit (juce::File = {});
    bool handleUnsavedEdit();

private:
    void handleAsyncUpdate () override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void saveSettings();
    void createTracksAndAssignInputs();
    void saveTempEdit();
    void openValidStartEdit();
    void setupSideBrowser();

    void timerCallback() override;

    std::unique_ptr<HeaderComponent>                    m_header;
    NextLookAndFeel                                     m_nextLookAndFeel;

    tracktion_engine::Engine                            m_engine
                                { ProjectInfo::projectName
                                , std::make_unique<ExtendedUIBehaviour>()
                                , nullptr };
    tracktion_engine::SelectionManager                  m_selectionManager{ m_engine };
    std::unique_ptr<tracktion_engine::Edit>             m_edit;
    std::unique_ptr<EditComponent>                      m_editComponent;
    ApplicationViewState &                              m_applicationState;
    std::unique_ptr<SideBarBrowser>                     m_sideBarBrowser;
    juce::StretchableLayoutManager                      m_stretchableManager;
    juce::StretchableLayoutResizerBar                   m_resizerBar
                                {&m_stretchableManager, 1, true};
    [[maybe_unused]] bool                               m_settingsLoaded {false};
    bool                                                m_saveTemp{false}, m_updateView{false}, m_updateSource{false};
    bool m_hasUnsavedTemp {true};
    juce::File m_tempDir;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
