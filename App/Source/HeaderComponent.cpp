/*
  ==============================================================================

    HeaderComponent.cpp
    Created: 7 Jan 2020 8:31:11pm
    Author:  Zehn

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "HeaderComponent.h"
#include "Utilities.h"


//==============================================================================
HeaderComponent::HeaderComponent(tracktion_engine::Edit& edit)
    : m_edit(edit)
    , m_display (edit)
{
    addAndMakeVisible(m_newButton);
    m_newButton.setButtonText("New");
    m_newButton.addListener(this);

    addAndMakeVisible(m_loadButton);
    m_loadButton.setButtonText("Load");
    m_loadButton.addListener(this);

    addAndMakeVisible(m_saveButton);
    m_saveButton.setButtonText("Save");
    m_saveButton.addListener(this);

    addAndMakeVisible(m_playButton);
    m_playButton.setButtonText("Play");
    m_playButton.addListener(this);

    addAndMakeVisible(m_stopButton);
    m_stopButton.setButtonText("Stop");
    m_stopButton.addListener(this);

    addAndMakeVisible(m_recordButton);
    m_recordButton.setButtonText("Record");
    m_recordButton.addListener(this);

    addAndMakeVisible(m_settingsButton);
    m_settingsButton.setButtonText("Settings");
    m_settingsButton.addListener(this);

    addAndMakeVisible(m_pluginsButton);
    m_pluginsButton.setButtonText("Plugins");
    m_pluginsButton.addListener(this);

    addAndMakeVisible (m_display);

    startTimer(30);
}

HeaderComponent::~HeaderComponent()
{
}

void HeaderComponent::paint (Graphics& g)
{
    juce::Rectangle<int> area = getLocalBounds();
}

void HeaderComponent::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    auto gap = area.getHeight()/2;

    area.removeFromRight(gap/4);
    area.removeFromBottom(gap/4);

    auto displayWidth = 300;
    auto displayRect = juce::Rectangle<int> ((area.getX () + (area.getWidth ()/2) - (displayWidth/2)),
                                             area.getY (),
                                             displayWidth,
                                             area.getHeight ());


    m_settingsButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));
    area.removeFromRight(gap/4);
    m_pluginsButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));

    area.removeFromLeft(gap/4);
    m_newButton.setBounds(area.removeFromLeft(area.getHeight() + gap/2));
    area.removeFromLeft(gap/4);
    m_loadButton.setBounds(area.removeFromLeft(area.getHeight() + gap/2));
    area.removeFromLeft(gap/4);
    m_saveButton.setBounds(area.removeFromLeft(area.getHeight() + gap/2));


    area = juce::Rectangle<int>(getLocalBounds ().getX (),
                                getLocalBounds ().getY (),
                                (getLocalBounds ().getWidth ()/2) - (displayWidth/2),
                                getLocalBounds ().getHeight ()
                                );

    area.removeFromBottom(gap/4);
    area.removeFromRight(gap);
    m_recordButton.setBounds(area.removeFromRight(area.getHeight()+gap/2));
    area.removeFromRight(gap/4);
    m_stopButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));
    area.removeFromRight(gap/4);
    m_playButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));
    m_display.setBounds (displayRect);

}

void HeaderComponent::buttonClicked(Button* button)
{
    if (button == &m_newButton)
    {

    }


    if (button == &m_playButton)
    {
        auto& transport = m_edit.getTransport();

        if (transport.isPlaying())
            transport.stop(false, false);
        else
            transport.play(true);

        m_playButton.setButtonText(m_edit.getTransport().isPlaying() ? "Pause" : "Play");
    }
    if (button == &m_stopButton)
    {
        m_edit.getTransport().stop(false, false, true, true);
        m_edit.getTransport().setCurrentPosition(0);
        m_playButton.setButtonText("Play");
    }
    if (button == &m_recordButton)
    {
        bool wasRecording = m_edit.getTransport().isRecording();
                    EngineHelpers::toggleRecord (m_edit);
                    if (wasRecording)
                        te::EditFileOperations (m_edit).save (true, true, false);
    }
    if (button == &m_settingsButton)
    {
        EngineHelpers::showAudioDeviceSettings (m_edit.engine);
    }
    if (button == &m_pluginsButton)
    {
        DialogWindow::LaunchOptions o;
            o.dialogTitle                   = TRANS("Plugins");
            o.dialogBackgroundColour        = Colours::black;
            o.escapeKeyTriggersCloseButton  = true;
            o.useNativeTitleBar             = true;
            o.resizable                     = true;
            o.useBottomRightCornerResizer   = true;
            
            auto v = new PluginListComponent (m_edit.engine.getPluginManager().pluginFormatManager,
                                              m_edit.engine.getPluginManager().knownPluginList,
                                              m_edit.engine.getTemporaryFileManager().getTempFile ("PluginScanDeadMansPedal"),
                                              te::getApplicationSettings());
            v->setSize (800, 600);
            o.content.setOwned (v);
            o.launchAsync();
    }
}

void HeaderComponent::timerCallback()
{
    m_display.update ();
}

