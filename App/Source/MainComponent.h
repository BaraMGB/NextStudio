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

enum KeyPressCommandIDs
{
    midiNoteC = 1,
    midiNoteCsharp ,
    midiNoteD,
    midiNoteDsharp ,
    midiNoteE,
    midiNoteF,
    midiNoteFsharp ,
    midiNoteG,
    midiNoteGsharp ,
    midiNoteA,
    midiNoteAsharp ,
    midiNoteB,
    midiNoteUpperC,
    midiNoteUpperCsharp ,
    midiNoteUpperD,
    midiNoteUpperDsharp ,
    midiNoteUpperE,
    midiNoteUpperF,
    midiNoteUpperFsharp ,
    midiNoteUpperG,
    midiNoteUpperGsharp ,
    midiNoteUpperA,
    midiNoteUpperAsharp ,
    midiNoteUpperB,
    midiNoteTopC,


    togglePlay
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
    void saveTempEdit();
    void openValidStartEdit();
    void setupSideBrowser();

    


    std::unique_ptr<HeaderComponent>                    m_header;
    NextLookAndFeel                                     m_nextLookAndFeel;

    tracktion_engine::Engine                            m_engine
                                { ProjectInfo::projectName
                                , std::make_unique<ExtendedUIBehaviour>()
                                , nullptr };
    juce::ApplicationCommandManager                     m_commandManager;

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
    juce::Array<juce::KeyPress> m_pressedKeysForMidiKeyboard;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
