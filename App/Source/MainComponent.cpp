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

    addAndMakeVisible(m_resizerBar);
    m_stretchableManager.setItemLayout(0,            // for the fileTree
        -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
        -0.15);        // and its preferred size is 30% of the total available space

    m_stretchableManager.setItemLayout(1,            // for the resize bar
        5, 5, 5);     // hard limit to 5 pixels

    m_stretchableManager.setItemLayout(2,            // for the imagePreview
        -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
        -0.85);        // and its preferred size is 70% of the total available space


    //Edit stuff
    auto d = File::getSpecialLocation (File::tempDirectory).getChildFile ("EmptyEdit");
    d.createDirectory();

    auto f = Helpers::findRecentEdit (d);
    if (f.existsAsFile())
    {
        std::cout << "last" << std::endl;
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
    menu.reduce(5, 5);
    m_menuBar.setBounds(menu);
    m_editNameLabel.setBounds(menu);
    
    m_header.get()->setBounds(header);
    area.removeFromTop(10);
//    auto sidebarWidth = getLocalBounds().getWidth() / 5;
    Component* comps[] = { &m_tree, &m_resizerBar,m_songEditor.get()};

    // this will position the 3 components, one above the other, to fit
    // vertically into the rectangle provided.
    m_stretchableManager.layOutComponents(comps, 3,
        area.getX(), area.getY(), area.getWidth(), area.getHeight() - c_footerHeight,
        false, true);

    m_tree.setColour(TreeView::ColourIds::backgroundColourId, Colour(0xff2c2c2c));
}

void MainComponent::buttonClicked(Button* /*button*/)
{
    
}

void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
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
        m_edit = std::make_unique<te::Edit> (m_engine, ValueTree::fromXml (editFile.loadFileAsString()), te::Edit::forEditing, nullptr, 0);
    else
        m_edit = std::make_unique<te::Edit> (m_engine, te::createEmptyEdit (m_engine), te::Edit::forEditing, nullptr, 0);

    m_edit->editFileRetriever = [editFile] { return editFile; };
    m_edit->playInStopEnabled = true;

    auto& transport = m_edit->getTransport();
    transport.addChangeListener (this);

    m_editNameLabel.setText (editFile.getFileNameWithoutExtension(), dontSendNotification);


    createTracksAndAssignInputs();

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

    int trackNum = 0;
    for (auto instance : m_edit->getAllInputDevices())
    {
        if (instance->getInputDevice().getDeviceType() == te::InputDevice::waveDevice)
        {
            if (auto t = EngineHelpers::getOrInsertAudioTrackAt (*m_edit, trackNum))
            {
                instance->setTargetTrack (*t, 0, true);
                instance->setRecordingEnabled (*t, true);

                trackNum++;
            }
        }
    }

    m_edit->restartPlayback();
}
