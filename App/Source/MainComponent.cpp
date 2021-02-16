/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "Utilities.h"
namespace IDs
{
    #define DECLARE_ID(name)  const juce::Identifier name (#name);
    DECLARE_ID (AppSettings)
    DECLARE_ID (WorkDIR)
}
//==============================================================================
MainComponent::MainComponent()
{
    setLookAndFeel(&m_nextLookAndFeel);


    //Edit stuff
    auto directory = juce::File::getSpecialLocation (
                juce::File::tempDirectory).getChildFile ("temporary");
    directory.createDirectory();
    auto f = Helpers::findRecentEdit (directory);
    setupEdit (f.existsAsFile () ? f
                                 : directory.getNonexistentChildFile ("Untitled"
                                       , ".tracktionedit"
                                       , false ));
    loadApplicationSettings();
    //FileTree side bar
    m_thread.startThread(1);
    juce::File file = juce::File::createFileWithoutCheckingPath (
                m_state.getProperty (IDs::WorkDIR));
    if (!file.isDirectory ())
    {
        file = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    }
    m_dirConList.setDirectory(file, true, true);
    m_tree.addListener(this);

    addAndMakeVisible(m_tree);
    addAndMakeVisible(m_menuBar);
    addAndMakeVisible(m_editNameLabel);

    m_editNameLabel.setJustificationType (juce::Justification::centred);
    setSize(1600, 900);
}

MainComponent::~MainComponent()
{
    m_header->removeAllChangeListeners ();
    te::EditFileOperations (*m_edit).save (true, true, false);
    m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    setLookAndFeel(nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::darkgrey);
    auto area = getLocalBounds();
    g.fillRect (area);

    area.reduce(10, 10);
    auto header = area.removeFromTop(c_headerHeight);
    g.setColour(juce::Colour(0xff242424));
    g.fillRect(header);
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
    m_tree.setBounds (area.removeFromLeft (sidebarWidth));
    area.removeFromLeft (10);
    m_songEditor->setBounds (area);
    m_tree.setColour (juce::TreeView::ColourIds::backgroundColourId
                      , juce::Colour(0xff1b1b1b));
    m_tree.setColour (juce::DirectoryContentsDisplayComponent::highlightColourId
                      , juce::Colour(0xff4b4b4b));
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

void MainComponent::fileClicked(const juce::File &file, const juce::MouseEvent &event)
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
        auto red = juce::Random::getSystemRandom().nextInt(juce::Range<int>(0, 255));
        auto gre = juce::Random::getSystemRandom().nextInt(juce::Range<int>(0, 255));
        auto blu = juce::Random::getSystemRandom().nextInt(juce::Range<int>(0, 255));

        if (auto track = EngineHelpers::getOrInsertAudioTrackAt (*m_edit, tracktion_engine::getAudioTracks(*m_edit).size()))
        {
            track->setName("Track " + juce::String(tracktion_engine::getAudioTracks(*m_edit).size()));
            track->setColour(juce::Colour(red, gre, blu));
            EngineHelpers::removeAllClips(*track);
            // Add a new clip to this track
            if (auto newClip = track->insertWaveClip(selectedFile.getFileNameWithoutExtension(), selectedFile,
            { { 0.0, 0.0 + audioFile.getLength() }, 0.0 }, false))
            {
                newClip->setColour(track->getColour());
            }
        }
    }
}

void MainComponent::setupEdit(juce::File editFile)
{
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

    m_editNameLabel.setText (editFile.getFileNameWithoutExtension(), juce::dontSendNotification);


    createTracksAndAssignInputs();

    te::EditFileOperations (*m_edit).save (true, true, false);

    m_songEditor = std::make_unique<EditComponent> (*m_edit, m_selectionManager);
    addAndMakeVisible (*m_songEditor);
    m_header = std::make_unique<HeaderComponent>(*m_edit);
    m_header->addChangeListener (this);
    addAndMakeVisible(*m_header);
    resized ();
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
        juce::WildcardFileFilter wcf("","","");
        juce::FileBrowserComponent chooseDir(juce::FileBrowserComponent::canSelectDirectories
                                             , juce::File()
                                             ,&wcf
                                             , nullptr);
        juce::FileChooserDialogBox dialog ("Working Directory"
                                           , "Please select the working directory"
                                           , chooseDir,
                                           false,
                                           juce::Colours::black);
        juce::File workingDir;
        if (dialog.show ())
        {
            workingDir = chooseDir.getSelectedFile (0);
        }
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
