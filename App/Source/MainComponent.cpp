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
    setupSideBrowser();

    addAndMakeVisible(m_menuBar);
    addAndMakeVisible(m_editNameLabel);
    addAndMakeVisible (m_resizerBar);

    m_editNameLabel.setJustificationType (juce::Justification::centred);

    m_stretchableManager.setItemLayout (0, -0.05, -0.9, -0.15);
    m_stretchableManager.setItemLayout (1, 10, 10, 10);
    m_stretchableManager.setItemLayout (2, -0.1, -0.9, -0.85);
    //setSize(1600, 1000);
}

MainComponent::~MainComponent()
{
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
    auto sidebarWidth = getLocalBounds().getWidth() / 7;

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
    //Settings
    auto bounds = getScreenBounds ();
//    m_applicationState.m_windowXpos = bounds.getX();
//    m_applicationState.m_windowYpos = bounds.getY();
//    m_applicationState.m_windowWidth = bounds.getWidth();
//    m_applicationState.m_windowHeight = bounds.getHeight ();
}

bool MainComponent::keyPressed(const juce::KeyPress &key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        EngineHelpers::togglePlay(* m_edit);
        return true;
    }
    return false;
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
              , *m_edit);
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

    m_edit->editFileRetriever = [editFile] { return editFile; };
    m_edit->playInStopEnabled = true;

    auto& transport = m_edit->getTransport();
    transport.addChangeListener (this);

    m_editNameLabel.setText (editFile.getFileNameWithoutExtension()
                             , juce::dontSendNotification);
    createTracksAndAssignInputs();
    te::EditFileOperations (*m_edit).save (true, true, false);
    m_editComponent = std::make_unique<EditComponent> (*m_edit, m_selectionManager);
    addAndMakeVisible (*m_editComponent);
    addAndMakeVisible(m_editComponent->lowerRange());
    m_header = std::make_unique<HeaderComponent>(*m_edit, m_applicationState);
    m_header->addChangeListener (this);
    addAndMakeVisible(*m_header);
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
            GUIHelpers::saveEdit (*m_edit
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

//void MainComponent::loadApplicationSettings()
//{
//    auto settingsFile = juce::File::getSpecialLocation (
//                juce::File::userApplicationDataDirectory)
//                .getChildFile ("NextStudio/AppSettings.xml");
//    if (!settingsFile.existsAsFile ())
//    {
//        auto result =  juce::AlertWindow::showOkCancelBox (
//                    juce::AlertWindow::AlertIconType::QuestionIcon
//                    , "Working Directory"
//                    , "It seems, this is your first time you started Next Studio. "
//                      "A working directory can be created automaticly."
//                      " You can choose a directory, if you want"
//                    , "okay"
//                    , "choose directory");
//        juce::File workingDir;
//        if (result == true)
//        {
//            workingDir = juce::File::getSpecialLocation (
//                        juce::File::userHomeDirectory).getChildFile ("NextStudio");
//            workingDir.createDirectory ();
//        }
//        else
//        {
//            juce::FileChooser fc ("Working Directory"
//                                  , juce::File::getSpecialLocation (juce::File::userHomeDirectory)
//                                  , "");
//            if (fc.browseForDirectory ())
//            {
//                workingDir = fc.getResult ();
//            }
//            else
//            {
//                workingDir = juce::File::getSpecialLocation (
//                            juce::File::userHomeDirectory).getChildFile ("NextStudio");
//                workingDir.createDirectory ();
//            }
//        }
//        juce::File presetDir = workingDir.getChildFile ("Presets");
//        juce::File projectsDir = workingDir.getChildFile ("Projects");
//        juce::File samplesDir = workingDir.getChildFile ("Samples");
//        juce::File clipsDir = workingDir.getChildFile ("Clips");
//        presetDir.createDirectory ();
//        projectsDir.createDirectory ();
//        samplesDir.createDirectory ();
//        clipsDir.createDirectory ();

//        juce::ValueTree settings(IDs::AppSettings);
//        settings.setProperty (IDs::WorkDIR, workingDir.getFullPathName (), nullptr);
//        settings.setProperty (IDs::PresetDIR, presetDir.getFullPathName (), nullptr);
//        settings.setProperty (IDs::ProjectsDIR, projectsDir.getFullPathName (), nullptr);
//        settings.setProperty (IDs::SamplesDIR, samplesDir.getFullPathName (), nullptr);
//        settings.setProperty (IDs::ClipsDIR, clipsDir.getFullPathName (), nullptr);

//        auto bounds = getLocalBounds ();
//        settings.setProperty (IDs::WindowX, bounds.getX(), nullptr);
//        settings.setProperty (IDs::WindowY, bounds.getY(), nullptr);
//        settings.setProperty (IDs::WindowWidth, bounds.getWidth(), nullptr);
//        settings.setProperty (IDs::WindowHeight, bounds.getHeight (), nullptr);
//        m_applicationState = settings.createCopy ();
//        settingsFile.create ();
//        auto xmlToWrite = m_applicationState.createXml ();
//        xmlToWrite->writeTo (settingsFile);
//    }
//    else
//    {
//        juce::XmlDocument xmlDoc (settingsFile);
//        auto xmlToRead = xmlDoc.getDocumentElement ();
//        m_applicationState = juce::ValueTree::fromXml (*xmlToRead);
//        m_settingsLoaded = true;
//    }
//}
