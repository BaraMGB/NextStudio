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


    addAndMakeVisible(m_menuBar);
    addAndMakeVisible(m_editNameLabel);
    addAndMakeVisible (m_resizerBar);

    m_editNameLabel.setJustificationType (juce::Justification::centred);

    m_stretchableManager.setItemLayout (0, -0.05, -0.9, -0.15);
    m_stretchableManager.setItemLayout (1, 10, 10, 10);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
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
    auto area = getLocalBounds();
    area.reduce(10, 10);
    auto header = area.removeFromTop(c_headerHeight);
    g.setColour(juce::Colour(0xff242424));
    g.fillRoundedRectangle (
                header.getX ()
              , header.getY ()
              , header.getWidth()
              , header.getHeight()
              , 10);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto header = area.removeFromTop(c_headerHeight);
    auto menu = header.removeFromTop(header.getHeight() / 2);
    menu.reduce(5, 5);

    m_menuBar.setBounds(menu);
    m_editNameLabel.setBounds(menu);

    m_header.get()->setBounds(header);
    area.removeFromTop(10);
    auto lowerRange = area.removeFromBottom( m_editComponent->getEditViewState().m_isPianoRollVisible
                       ? m_editComponent->getEditViewState().m_pianorollHeight
                       : 250);
    Component* comps[] = {
        m_sideBarBrowser.get ()
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
    GUIHelpers::log("MainComponent: keypressed");
    auto scaleFactor = 0.2;

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
        x1 = x1 + (scaleFactor/5);
        x2 = x2 + (scaleFactor/5);

        return true;
    }

    if (key == juce::KeyPress::leftKey)
    {
        auto &x1 = m_editComponent->getEditViewState ().m_viewX1;
        auto &x2 = m_editComponent->getEditViewState ().m_viewX2;
        x1 = x1 - (scaleFactor/5);
        x2 = x2 - (scaleFactor/5);

        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
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
    if (key == juce::KeyPress::createFromDescription ("command + D"))
#else
    if (key == juce::KeyPress::createFromDescription ("ctrl + d"))
#endif
    {
        EngineHelpers::duplicateSelectedClips (
                    m_editComponent->getEditViewState ().m_edit
                  , m_selectionManager
                  , m_editComponent->getEditViewState ().m_automationFollowsClip);
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
    GUIHelpers::log (key.getTextDescription ());
    return true;
}

void MainComponent::valueTreePropertyChanged(
        juce::ValueTree &vt
      , const juce::Identifier &property)
{
    if (vt.hasType (IDs::EDITVIEWSTATE))
    {
        if (property == te::IDs::name)
        {
            m_editNameLabel.setText (m_editComponent->getEditViewState ().m_editName
                                   , juce::dontSendNotification);
        }
        if (property == IDs::pianorollHeight
        || property == IDs::isPianoRollVisible)
        {
            resized ();
        }
    }
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == m_header.get ())
    {
        if (m_header->loadingFile ().exists ())
        {
            auto editfile = m_header->loadingFile ();
            setupEdit (editfile);
        }
    }
}

void MainComponent::openValidStartEdit()
{
    auto tempDirectory = m_engine.getTemporaryFileManager().getTempDirectory();
    tempDirectory.createDirectory();
    auto f = Helpers::findRecentEdit (tempDirectory);
    if (f.existsAsFile ())
    {
        setupEdit (f);
    }
    else
    {
        setupEdit (tempDirectory.getNonexistentChildFile ("Untitled"
                         , ".tracktionedit"
                         , false));
        auto atList = te::getTracksOfType<te::AudioTrack>(*m_edit, true);
        for (auto & t : atList)
        {
            m_edit->deleteTrack (t);
        }
    }
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
        {
            return;
        }
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
    auto& transport = m_edit->getTransport();
    transport.addChangeListener (this);


    createTracksAndAssignInputs();
    te::EditFileOperations (*m_edit).save (true, true, false);
    m_editComponent = std::make_unique<EditComponent> (
                *m_edit
              , m_selectionManager
              , m_applicationState.m_trackColours);
    m_edit->state.addListener (this);
    m_editNameLabel.setText (m_editComponent->getEditViewState ().m_editName
                             , juce::dontSendNotification);
    addAndMakeVisible (*m_editComponent);
    addAndMakeVisible(m_editComponent->lowerRange());
    m_header = std::make_unique<HeaderComponent>(m_editComponent->getEditViewState (), m_applicationState);
    m_header->addChangeListener (this);
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
                                  , juce::File().createFileWithoutCheckingPath (
                                      m_applicationState.m_workDir));
            return true;
            break;
        case 2 :
            return true;
            break;
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
    {
        if (auto wip = dm.getWaveInDevice (i))
        {
            wip->setEndToEnd (true);
            wip->setEnabled (true);
        }
    }

    for (int i = 0; i < dm.getNumMidiInDevices(); i++)
    {
        if (auto mip = dm.getMidiInDevice (i))
        {
            mip->setEndToEndEnabled (true);
            mip->setEnabled (true);
        }
    }

    m_edit->getTransport().ensureContextAllocated();
    m_edit->restartPlayback();

}
