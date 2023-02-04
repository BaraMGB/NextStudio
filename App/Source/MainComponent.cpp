/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

MainComponent::MainComponent(ApplicationViewState &state)
    : m_applicationState(state)
{
    setLookAndFeel(&m_nextLookAndFeel);

    openValidStartEdit();

    addAndMakeVisible(m_editComponent->lowerRange());
    addAndMakeVisible (m_resizerBar);

    m_stretchableManager.setItemLayout (0, -0.05, -0.9, -0.15);
    m_stretchableManager.setItemLayout (1, 10, 10, 10);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
    startTimer (static_cast<int>(m_applicationState.m_autoSaveInterval));
}

MainComponent::~MainComponent()
{
    m_edit->state.removeListener (this);
    saveSettings();
    m_header->removeAllChangeListeners ();
    m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    setLookAndFeel(nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour(0xff555555));
    g.fillRect (getLocalBounds ());
    }

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto header = area.removeFromTop(60);


    m_header->setBounds(header);
    area.removeFromTop(10);
    auto lowerRange = area.removeFromBottom( m_editComponent->getEditViewState().m_isPianoRollVisible
                       ? m_editComponent->getEditViewState().m_midiEditorHeight
            : 250);
    Component* comps[] = {
        m_sideBarBrowser.get()
      , &m_resizerBar
      , m_editComponent.get ()};
    m_stretchableManager.layOutComponents (
                comps
              , 3
              , area.getX()
              , area.getY()
              , area.getWidth()
              , area.getHeight()
              , false, true);
    m_editComponent->lowerRange().setBounds(lowerRange);
}

bool MainComponent::keyPressed(const juce::KeyPress &key)
{
    auto scaleFactor = 0.2;

    GUIHelpers::log (key.getTextDescription ());
    if (key == juce::KeyPress::numberPadAdd)
    {
        auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
        auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
        x1 = x1 - scaleFactor;
        x2 = x2 + scaleFactor;

        return true;
    }

    if (key == juce::KeyPress::numberPadSubtract)
    {
        auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
        auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
        x1 = x1 + scaleFactor;
        x2 = x2 - scaleFactor;

        return true;
    }

    if (key == juce::KeyPress::rightKey)
    {
        auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
        auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
        x2 = x2 + (scaleFactor/5);
        x1 = x1 + (scaleFactor/5);

        return true;
    }

    if (key == juce::KeyPress::leftKey)
    {
        auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
        auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
        x2 = x2 - (scaleFactor/5);
        x1 = x1 - (scaleFactor/5);

        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (m_editComponent->getSongEditor().getTracksWithSelectedTimeRange().size() > 0)
            m_editComponent->getSongEditor().deleteSelectedTimeRange();
        else 
            EngineHelpers::deleteSelectedClips (m_editComponent->getEditViewState ());

        return true;
    }

    if (key == juce::KeyPress::returnKey)
    {
        EngineHelpers::play(m_editComponent->getEditViewState ());
    }

    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::numberPad0)
    {
        EngineHelpers::togglePlay(m_editComponent->getEditViewState ());
        return true;
    }
    if (key == juce::KeyPress::numberPadDecimalPoint)
    {
        EngineHelpers::stopPlay(m_editComponent->getEditViewState ());
        return true;
    }

    if (key == juce::KeyPress::F10Key)
    {
        std::cout << "DEBUG EDIT: " << juce::Time::getCurrentTime().toString(true, true, true, true) << std::endl;
        std::cout << "=================================================================================" << std::endl;
        auto editString = m_edit->state.toXmlString();
        std::cout << editString << std::endl;
        return true;
    }
#if JUCE_MAC
    if (key == juce::KeyPress::createFromDescription ("command + L"))
#else
    if (key == juce::KeyPress::createFromDescription ("ctrl + l"))
#endif
        m_editComponent->loopAroundSelection();

    if (key == juce::KeyPress::createFromDescription("l"))
        EngineHelpers::toggleLoop(*m_edit);

#if JUCE_MAC
    if (key == juce::KeyPress::createFromDescription ("command + D"))
#else
    if (key == juce::KeyPress::createFromDescription ("ctrl + d"))
#endif
    {
        m_editComponent->getSongEditor().duplicateSelectedClipsOrTimeRange();
        return true;
    }

#if JUCE_MAC
    if (key == juce::KeyPress::createFromDescription ("command + z"))
#else
    if (key == juce::KeyPress::createFromDescription ("ctrl + z"))
#endif
    {
        m_editComponent->getEditViewState ().m_edit.undo ();
        return true;
    }

#if JUCE_MAC
    if (key == juce::KeyPress::createFromDescription ("shift + cmd + z"))
#else
    if (key == juce::KeyPress::createFromDescription ("shift + ctrl + z"))
#endif
    {
        m_editComponent->getEditViewState ().m_edit.redo ();
        return true;
    }
    return true;
}

void MainComponent::valueTreePropertyChanged(
        juce::ValueTree &/* vt */
      , const juce::Identifier &property)
{
    if (property == te::IDs::looping)
        m_header->loopButtonClicked();

    if (property == IDs::pianorollHeight
        || property == IDs::isPianoRollVisible)
            markAndUpdate(m_updateView);

    if (property == te::IDs::source || property == te::IDs::state)
        markAndUpdate(m_updateSource);
   
    if (property == te::IDs::lastSignificantChange)
        markAndUpdate(m_saveTemp);
}
void MainComponent::handleAsyncUpdate()
{
    if (compareAndReset (m_saveTemp)  && !compareAndReset(m_updateSource))
    {
        m_hasUnsavedTemp = true;
        stopTimer();
        startTimer (static_cast<int>(m_applicationState.m_autoSaveInterval));
    }

    if (compareAndReset(m_updateView))
    {
        resized();
    }
}

void MainComponent::timerCallback()
{
    if (m_hasUnsavedTemp)
    {
        saveTempEdit();
        m_hasUnsavedTemp = false;
    }
}
void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == m_header.get ())
    {
        if (m_header->getSelectedFile ().exists ())
        {
            auto editfile = m_header->getSelectedFile ();
            setupEdit (editfile);
        }
        else
        {
            m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
            openValidStartEdit();
        }
    }
}

void MainComponent::saveTempEdit()
{
    if (!m_edit->getTransport().isPlaying())
    {
        auto temp = m_edit->getTempDirectory(false);
        auto editFile = Helpers::findRecentEdit(temp);
        auto currentFile =  te::EditFileOperations(*m_edit).getEditFile();

        EngineHelpers::refreshRelativePathsToNewEditFile(m_editComponent->getEditViewState(), editFile);
        te::EditFileOperations(*m_edit).writeToFile(editFile, true);
        EngineHelpers::refreshRelativePathsToNewEditFile(m_editComponent->getEditViewState(), currentFile);
        m_edit->sendSourceFileUpdate();
        GUIHelpers::log("Temp file saved!");
    }
}

void MainComponent::openValidStartEdit()
{
    m_tempDir = m_engine.getTemporaryFileManager().getTempDirectory();
    m_tempDir.createDirectory();

    auto f = Helpers::findRecentEdit(m_tempDir);
    if (f.existsAsFile())
    {
        auto result = juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,
                                                         "Restore crashed project?",
                                                         "It seems, NextStudio is crashed last time. Do you want to restore the last session?",
                                                         "Yes",
                                                         "No");
        if (result) 
        {
            setupEdit(f);
            return;
        }
    }
    setupEdit (m_tempDir.getNonexistentChildFile ("untitled"
                                                      , ".nextTemp"
                                                      , false));
    auto atList = te::getTracksOfType<te::AudioTrack>(*m_edit, true);
    for (auto & t : atList)
        m_edit->deleteTrack (t);
}

void MainComponent::setupSideBrowser()
{
    m_sideBarBrowser = std::make_unique<SideBarBrowser>(
                m_applicationState
              , m_editComponent->getEditViewState ());
    addAndMakeVisible (*m_sideBarBrowser);
}

void MainComponent::setupEdit(juce::File editFile)
{
    if(m_edit)
    {
        if(!handleUnsavedEdit ())
            return;
    }
    if (editFile == juce::File())
    {
        juce::FileChooser fc ("New Edit"
                              , juce::File::getSpecialLocation (
                                  juce::File::userDocumentsDirectory)
                              , "*.tracktionedit");
        if (fc.browseForFileToSave (true))
            editFile = fc.getResult();
        else
            return;
    }

    m_selectionManager.deselectAll();
    m_editComponent = nullptr;

    if (editFile.existsAsFile())
        m_edit = te::loadEditFromFile (m_engine, editFile);
    else
        m_edit = te::createEmptyEdit (m_engine, editFile);

    m_edit->playInStopEnabled = true;
    
    m_edit->setTempDirectory(m_tempDir);

    m_edit->getTransport().addChangeListener (this);

    createTracksAndAssignInputs();

    te::EditFileOperations (*m_edit).writeToFile(editFile, true);

    m_editComponent = std::make_unique<EditComponent> (
                *m_edit
              , m_applicationState
              , m_selectionManager);

    m_edit->state.addListener (this);

    m_header = std::make_unique<HeaderComponent>(m_editComponent->getEditViewState (), m_applicationState);
    m_header->addChangeListener (this);


    addAndMakeVisible (*m_editComponent);
    addAndMakeVisible(m_editComponent->lowerRange());
    addAndMakeVisible(*m_header);

    setupSideBrowser();

    resized ();
}

void MainComponent::saveSettings()
{
    m_applicationState.setBounds(getScreenBounds ());
    m_applicationState.saveState ();
}

bool MainComponent::handleUnsavedEdit()
{
    if (m_edit->hasChangedSinceSaved ())
    {
        auto result = juce::AlertWindow::showYesNoCancelBox (
                    juce::AlertWindow::QuestionIcon
                    , "Unsaved Project"
                    , "Do you want to save the project?"
                    , "Yes"
                    , "No"
                    , "Cancel");
        switch (result) {
        case 1 :
            GUIHelpers::saveEdit (m_editComponent->getEditViewState ()
                                  , juce::File::createFileWithoutCheckingPath (
                                      m_applicationState.m_workDir));
            return true;
        case 2 :
            return true;
        case 3 :
            //cancel
        default:
            return false;
        }
    }
    return true;
}
 
void MainComponent::createTracksAndAssignInputs()
{
    auto& dm = m_engine.getDeviceManager();

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice (i))
            wip->setStereoPair (false);

    for (int i = 0; i < dm.getNumWaveInDevices(); i++)
        if (auto wip = dm.getWaveInDevice (i))
        {
            wip->setEndToEnd (true);
            wip->setEnabled (true);
        }

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
        if (auto mip = dm.getMidiInDevice (i))
        {
            mip->setEndToEndEnabled (true);
            mip->setEnabled (true);
        }

    m_edit->getTransport().ensureContextAllocated();
    m_edit->restartPlayback();

}

