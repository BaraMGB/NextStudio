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
    m_tree(m_dirConList),
    
    m_header(getWidth(), c_headerHeight, &m_edit )

{
    setLookAndFeel(&m_nextLookAndFeel);
    //FileTree side bar
    m_thread.startThread(1);
    File file = File::getSpecialLocation(File::userHomeDirectory);
    m_dirConList.setDirectory(file, true, true);
    m_tree.addListener(this);

    addAndMakeVisible(m_tree);
    addAndMakeVisible(m_songEditor);
    

    getLookAndFeel().setColour(ScrollBar::thumbColourId, Colour(0xff2c2c2c));

    addAndMakeVisible(m_resizerBar);
    m_stretchableManager.setItemLayout(0,            // for the fileTree
        -0.1, -0.9,   // must be between 50 pixels and 90% of the available space
        -0.15);        // and its preferred size is 30% of the total available space

    m_stretchableManager.setItemLayout(1,            // for the resize bar
        5, 5, 5);     // hard limit to 5 pixels

    m_stretchableManager.setItemLayout(2,            // for the imagePreview
        -0.1, -0.9,   // size must be between 50 pixels and 90% of the available space
        -0.85);        // and its preferred size is 70% of the total available space
    // Buttons
    addAndMakeVisible(m_header);
    m_edit.tempoSequence.getTempos()[0]->setBpm(140);
    
    
    setSize(1600, 900);

    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::darkgrey);
    //g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    auto sidebarWidth = getLocalBounds().getWidth() / 5;
   
    Component* comps[] = { &m_tree, &m_resizerBar, &m_songEditor };

    // this will position the 3 components, one above the other, to fit
    // vertically into the rectangle provided.
    m_stretchableManager.layOutComponents(comps, 3,
        area.getX(), area.getY()+ c_headerHeight, area.getWidth(), area.getHeight() - (c_footerHeight + c_headerHeight),
        false, true);

    m_tree.setColour(TreeView::ColourIds::backgroundColourId, Colour(0xff2c2c2c));
    m_header.setBounds(area.getX(), area.getY(), area.getWidth(), c_headerHeight);
}

void MainComponent::buttonClicked(Button* button)
{
    
}
