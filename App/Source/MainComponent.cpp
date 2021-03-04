/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&m_nextLookAndFeel);

    loadApplicationSettings();
    openValidStartEdit();
    setupSideBrowser();

    addAndMakeVisible(m_tree);
    addAndMakeVisible(m_menuBar);
    addAndMakeVisible(m_editNameLabel);

    m_editNameLabel.setJustificationType (juce::Justification::centred);
    setSize(1600, 900);
}

MainComponent::~MainComponent()
{
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
    auto lowerRange = area.removeFromBottom( m_songEditor->getEditViewState().m_isPianoRollVisible
                       ? m_songEditor->getEditViewState().m_pianorollHeight
                       : 250);
    
    m_tree.setBounds (area.removeFromLeft (sidebarWidth));
    area.removeFromLeft (10);
    m_songEditor->setBounds (area);
    m_tree.setColour (juce::TreeView::ColourIds::backgroundColourId
                      , juce::Colour(0xff1b1b1b));
    m_tree.setColour (juce::DirectoryContentsDisplayComponent::highlightColourId
                      , juce::Colour(0xff4b4b4b));
    m_songEditor->lowerRange().setBounds(lowerRange);
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

void MainComponent::fileClicked(const juce::File &file, const juce::MouseEvent&/* event*/)
{
    m_tree.setDragAndDropDescription(file.getFileName());
}

void MainComponent::fileDoubleClicked(const juce::File &)
{
    auto selectedFile = m_tree.getSelectedFile();
    tracktion_engine::AudioFile audioFile(m_edit->engine, selectedFile);
    if (selectedFile.getFileExtension () == ".tracktionedit")
    {
        setupEdit (selectedFile);
    }
    else if (audioFile.isValid ())
    {
        EngineHelpers::loadAudioFileAsClip (*m_edit, selectedFile);
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
    m_thread.startThread(1);
    juce::File file = juce::File::createFileWithoutCheckingPath (
                m_state.getProperty (IDs::WorkDIR));
    if (!file.isDirectory ())
    {
        file = juce::File::getCurrentWorkingDirectory ();
    }
    m_dirConList.setDirectory(file, true, true);
    m_tree.addListener(this);
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
    m_songEditor = nullptr;

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
    m_songEditor = std::make_unique<EditComponent> (*m_edit, m_selectionManager);
    addAndMakeVisible (*m_songEditor);
    addAndMakeVisible(m_songEditor->lowerRange());
    m_header = std::make_unique<HeaderComponent>(*m_edit, m_state);
    m_header->addChangeListener (this);
    addAndMakeVisible(*m_header);
    resized ();
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
                                      m_state.getProperty (IDs::WorkDIR)));
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

void MainComponent::loadApplicationSettings()
{
    auto settingsFile = juce::File::getSpecialLocation (
                juce::File::userApplicationDataDirectory)
                .getChildFile ("NextStudio/AppSettings.xml");
    if (!settingsFile.existsAsFile ())
    {
        auto result =  juce::AlertWindow::showOkCancelBox (
                    juce::AlertWindow::AlertIconType::QuestionIcon
                    , "Working Directory"
                    , "It seems, this is your first time you started Next Studio. "
                      "A working directory can be created automaticly."
                      " You can choose a directory, if you want"
                    , "okay"
                    , "choose directory");
        juce::File workingDir;
        if (result == true)
        {
            workingDir = juce::File::getSpecialLocation (
                        juce::File::userHomeDirectory).getChildFile ("NextStudioProjects");
            workingDir.createDirectory ();
        }
        else
        {
            juce::FileChooser fc ("Working Directory"
                                  , juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                  , "");
            if (fc.browseForDirectory ())
            {
                workingDir = fc.getResult ();
            }
            else
            {
                workingDir = juce::File::getSpecialLocation (
                            juce::File::userHomeDirectory).getChildFile ("NextStudioProjects");
                workingDir.createDirectory ();
            }
        }
        workingDir.setAsCurrentWorkingDirectory ();
        juce::ValueTree settings(IDs::AppSettings);
        settings.setProperty (IDs::WorkDIR, workingDir.getFullPathName (), nullptr);
        m_state = settings.createCopy ();
        settingsFile.create ();
        auto xmlToWrite = m_state.createXml ();
        xmlToWrite->writeTo (settingsFile);
    }
    else
    {
        juce::XmlDocument xmlDoc (settingsFile);
        auto xmlToRead = xmlDoc.getDocumentElement ();
        m_state = juce::ValueTree::fromXml (*xmlToRead);
    }
}
