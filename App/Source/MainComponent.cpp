/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() :
    m_thread("Tread"),
    m_dirConList(nullptr, m_thread),
    m_tree(m_dirConList)
{
    setLookAndFeel(&m_nextLookAndFeel);

    //FileTree side bar
    m_thread.startThread(1);
    File file = File::getSpecialLocation(File::userHomeDirectory);
    m_dirConList.setDirectory(file, true, true);
    m_tree.addListener(this);




    addAndMakeVisible(m_tree);
    addAndMakeVisible(m_menuBar);

    //Edit stuff
    auto d = File::getSpecialLocation (File::tempDirectory).getChildFile ("EmptyEdit");
    d.createDirectory();

    auto f = Helpers::findRecentEdit (d);
    if (f.existsAsFile())
    {

        setupEdit (f);
    }
    else
    {
        setupEdit (d.getNonexistentChildFile ("Untitled", ".tracktionedit", false));
    }


    m_header = std::make_unique<HeaderComponent>(*m_edit);
    addAndMakeVisible(*m_header);
    addAndMakeVisible(m_editNameLabel);
    m_editNameLabel.setJustificationType (Justification::centred);
    setSize(1600, 900);
}

MainComponent::~MainComponent()
{
    te::EditFileOperations (*m_edit).save (true, true, false);
    m_engine.getTemporaryFileManager().getTempDirectory().deleteRecursively();
    setLookAndFeel(nullptr);
}

void MainComponent::paint (Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto header = area.removeFromTop(c_headerHeight);
    g.setColour(Colour(0xff242424));
    g.fillRect(header);
    //g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    area.reduce(10, 10);

    auto header = area.removeFromTop(c_headerHeight);
    auto menu = header.removeFromTop(header.getHeight() / 2);
    auto sidebarWidth = getLocalBounds().getWidth() / 7;

    menu.reduce(5, 5);
    m_menuBar.setBounds(menu);
    m_editNameLabel.setBounds(menu);
    
    m_header.get()->setBounds(header);
    area.removeFromTop(10);
    m_tree.setBounds (area.removeFromLeft (sidebarWidth));
    area.removeFromLeft (10);
    m_songEditor->setBounds (area);
    m_tree.setColour(TreeView::ColourIds::backgroundColourId, Colour(0xff1b1b1b));
    m_tree.setColour (DirectoryContentsDisplayComponent::highlightColourId,Colour(0xff4b4b4b));
}

void MainComponent::buttonClicked(Button* /*button*/)
{
    
}

void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
}

void MainComponent::fileClicked(const File &file, const MouseEvent &event)
{
    m_tree.setDragAndDropDescription(file.getFileName());
}

void MainComponent::fileDoubleClicked(const File &)
{
    auto selectedFile = m_tree.getSelectedFile();
    if (selectedFile.existsAsFile())
    {
        auto red = Random::getSystemRandom().nextInt(Range<int>(0, 255));
        auto gre = Random::getSystemRandom().nextInt(Range<int>(0, 255));
        auto blu = Random::getSystemRandom().nextInt(Range<int>(0, 255));

        if (auto track = EngineHelpers::getOrInsertAudioTrackAt (*m_edit, tracktion_engine::getAudioTracks(*m_edit).size()))
        {

            track->setName("Track " + String(tracktion_engine::getAudioTracks(*m_edit).size()));
            track->setColour(Colour(red, gre, blu));
            EngineHelpers::removeAllClips(*track);
            // Add a new clip to this track
            tracktion_engine::AudioFile audioFile(m_edit->engine, selectedFile);
            if (audioFile.isValid())
                if (auto newClip = track->insertWaveClip(selectedFile.getFileNameWithoutExtension(), selectedFile,
                { { 0.0, 0.0 + audioFile.getLength() }, 0.0 }, false))
                {
                    newClip->setColour(track->getColour());
                }
        }
    }
}

void MainComponent::setupEdit(File editFile = {})
{
    if (editFile == File())
    {
        FileChooser fc ("New Edit", File::getSpecialLocation (File::userDocumentsDirectory), "*.tracktionedit");
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

    m_editNameLabel.setText (editFile.getFileNameWithoutExtension(), dontSendNotification);


    //createTracksAndAssignInputs();

    te::EditFileOperations (*m_edit).save (true, true, false);

    m_songEditor = std::make_unique<EditComponent> (*m_edit, m_selectionManager);
    addAndMakeVisible (*m_songEditor);
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

    m_edit->getTransport().ensureContextAllocated();

//    int trackNum = 0;
//    for (auto instance : m_edit->getAllInputDevices())
//    {
//        if (instance->getInputDevice().getDeviceType() == te::InputDevice::waveDevice)
//        {
//            if (auto t = EngineHelpers::getOrInsertAudioTrackAt (*m_edit, trackNum))
//            {
//                instance->setTargetTrack (*t, 0, true);
//                instance->setRecordingEnabled (*t, true);

//                trackNum++;
//            }
//        }
//    }

    m_edit->restartPlayback();
}
