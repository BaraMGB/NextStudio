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
    : m_newButton ("New", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_loadButton ("Load", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_saveButton ("Save", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_pluginsButton ("Plugins", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_stopButton ("Stop", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_recordButton ("Record", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_settingsButton ("Settings", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_playButton ("Play", DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_edit(edit)
    , m_display (edit)
{    
    addAndMakeVisible(m_newButton);
    GUIHelpers::setDrawableonButton (m_newButton, BinaryData::newbox_svg, m_btn_col);
    m_newButton.addListener(this);

    addAndMakeVisible(m_loadButton);
    GUIHelpers::setDrawableonButton (m_loadButton, BinaryData::filedownload_svg, m_btn_col);
    m_loadButton.addListener(this);

    addAndMakeVisible(m_saveButton);
    GUIHelpers::setDrawableonButton (m_saveButton, BinaryData::contentsaveedit_svg, m_btn_col);
    m_saveButton.addListener(this);

    addAndMakeVisible(m_playButton);
    GUIHelpers::setDrawableonButton (m_playButton, BinaryData::play_svg, m_btn_col);
    m_playButton.addListener(this);

    addAndMakeVisible(m_stopButton);
    GUIHelpers::setDrawableonButton (m_stopButton, BinaryData::stop_svg, m_btn_col);
    m_stopButton.addListener(this);

    addAndMakeVisible(m_recordButton);
    GUIHelpers::setDrawableonButton (m_recordButton, BinaryData::record_svg, m_btn_col);
    m_recordButton.addListener(this);

    addAndMakeVisible(m_settingsButton);
    GUIHelpers::setDrawableonButton (m_settingsButton, BinaryData::headphonessettings_svg, m_btn_col);
    m_settingsButton.addListener(this);

    addAndMakeVisible(m_pluginsButton);
    GUIHelpers::setDrawableonButton (m_pluginsButton, BinaryData::powerplug_svg, m_btn_col);
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


//    m_playButton.setImages (false, false, true,
//                            GUIHelpers::getImageFromSvg (BinaryData::Play_svg, "#000000",24,24), 0.0f, Colour(0x00000000),
//                            GUIHelpers::getImageFromSvg (BinaryData::Play_svg, "#000000",24,24), 0.0f, Colour(0x00000000),
//                            GUIHelpers::getImageFromSvg (BinaryData::Play_svg, "#000000",24,24), 0.0f, Colour(0x00000000)
//                            );

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
        const char* svgbin = m_edit.getTransport().isPlaying() ? BinaryData::pause_svg
                                                               : BinaryData::play_svg;
        GUIHelpers::setDrawableonButton (m_playButton, svgbin, m_btn_col);
       // m_playButton.setButtonText(m_edit.getTransport().isPlaying() ? "Pause" : "Play");
    }
    if (button == &m_stopButton)
    {
        m_edit.getTransport().stop(false, false, true, true);
        m_edit.getTransport().setCurrentPosition(0);
        GUIHelpers::setDrawableonButton (m_playButton, BinaryData::play_svg, m_btn_col);
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

