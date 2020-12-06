#include "HeaderComponent.h"

PositionDisplayComponent::PositionDisplayComponent(te::Edit &edit)
    : m_edit(edit)
{
    Helpers::addAndMakeVisible (*this, {   &bpmLabel,
                                           &sigLabel,
                                           &barBeatTickLabel,
                                           &timeLabel,
                                           &loopInLabel,
                                           &loopOutLabel  });
    bpmLabel.setJustificationType (juce::Justification::centred);
    sigLabel.setJustificationType (juce::Justification::centred);
    barBeatTickLabel.setJustificationType (juce::Justification::centred);
    barBeatTickLabel.setFont (28);
    timeLabel.setJustificationType (juce::Justification::centred);
    loopInLabel.setJustificationType (juce::Justification::centred);
    loopOutLabel.setJustificationType (juce::Justification::centred);

    bpmLabel.setInterceptsMouseClicks (false, false);
    sigLabel.setInterceptsMouseClicks (false, false);
    barBeatTickLabel.setInterceptsMouseClicks (false, false);
    timeLabel.setInterceptsMouseClicks (false, false);
    loopInLabel.setInterceptsMouseClicks (false, false);
    loopOutLabel.setInterceptsMouseClicks (false, false);

    update ();
}

void PositionDisplayComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds ();
    g.setGradientFill({juce::Colour(0xff2b2b2b),
                       0.0f,
                       0.0f,
                       juce::Colour(0xff3b3b3b),
                       0.0f,
                       static_cast<float>(getHeight()),
                       false});

    g.fillRect (area);
    area.reduce (1, 1);
    g.setColour (juce::Colour(0xff1b1b1b));
    g.fillRect (area);
    area.reduce (2, 2);
    g.setColour (juce::Colour (0xff202020));
    g.fillRect (area);
}

void PositionDisplayComponent::mouseDown(const juce::MouseEvent &event)
{
    m_MouseDownPosition = event.getMouseDownPosition ();
    m_bpmAtMd = m_edit.tempoSequence.getTempos ()[0]->getBpm ();
    m_barsBeatsAtMd = m_edit.tempoSequence.timeToBeats (m_edit.getTransport ().getCurrentPosition ());
    m_timeAtMouseDown = m_edit.getTransport ().getCurrentPosition ();
    m_numAtMouseDown = m_edit.tempoSequence.getTimeSig(0)->numerator;
    m_denAtMouseDown = m_edit.tempoSequence.getTimeSig(0)->denominator;
    m_loopInAtMouseDown = m_edit.getTransport ().getLoopRange ().getStart ();
    m_loopOutAtMouseDown = m_edit.getTransport ().getLoopRange ().getEnd ();
    te::TempoSequencePosition pos(m_edit.tempoSequence);
}

void PositionDisplayComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (bmpRect.contains (m_MouseDownPosition))
    {
        //m_edit.getTransport().setUserDragging (true);

        auto r = bmpRect;
        if (r.removeFromLeft (r.getWidth ()/2).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            auto tempo = m_edit.tempoSequence.getTempos ()[0];
            tempo->setBpm ( m_bpmAtMd - (event.getDistanceFromDragStartY ()));

        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            auto tempo = m_edit.tempoSequence.getTempos ()[0];
            tempo->setBpm (m_bpmAtMd - (event.getDistanceFromDragStartY () /100.0));
        }
        //set the Position back to the Beat Position on Mouse down
        te::TempoSequencePosition pos(m_edit.tempoSequence);
        pos.setTime (m_edit.tempoSequence.beatsToTime ( m_barsBeatsAtMd));
        m_edit.getTransport ().setCurrentPosition (pos.getTime ());
    }
    else if (sigRect.contains (m_MouseDownPosition))
    {

    }
    else if (barBeatTickRect.contains (m_MouseDownPosition))
    {
        auto r = barBeatTickRect;
        if (r.removeFromLeft (barBeatTickRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -(event.getDistanceFromDragStartY () * 4));

            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else if(r.removeFromLeft (barBeatTickRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () - (event.getDistanceFromDragStartY ()));

            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY () / 960.0));

            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
    }
    else if (timeRect.contains (m_MouseDownPosition))
    {
        auto r = timeRect;
        if (r.removeFromLeft (timeRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_timeAtMouseDown - (event.getDistanceFromDragStartY () * 60));


            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else if (r.removeFromLeft (timeRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown - event.getDistanceFromDragStartY ());


            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_timeAtMouseDown - (event.getDistanceFromDragStartY () * 0.001));


            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
    }
    else if (loopInrect.contains (m_MouseDownPosition))
    {
        auto r = loopInrect;
        if (r.removeFromLeft (loopInrect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_loopInAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -(event.getDistanceFromDragStartY () * 4));

            m_edit.getTransport ().setLoopIn (pos.getTime ());

        }
        else if(r.removeFromLeft (loopInrect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopInAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY ()));

            m_edit.getTransport ().setLoopIn (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopInAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY () / 960.0));

            m_edit.getTransport ().setLoopIn (pos.getTime ());
        }
    }
    else if (loopOutRect.contains (m_MouseDownPosition))
    {
        auto r = loopOutRect;
        if (r.removeFromLeft (loopOutRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_loopOutAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -(event.getDistanceFromDragStartY () * 4));

            m_edit.getTransport ().setLoopOut (pos.getTime ());

        }
        else if(r.removeFromLeft (loopOutRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopOutAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY ()));

            m_edit.getTransport ().setLoopOut (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopOutAtMouseDown);
            pos.setPPQTime (pos.getPPQTime () -( event.getDistanceFromDragStartY () / 960.0));

            m_edit.getTransport ().setLoopOut (pos.getTime ());
        }
    }
}

void PositionDisplayComponent::mouseUp(const juce::MouseEvent &event)
{
    m_edit.getTransport ().setUserDragging (false);
}

void PositionDisplayComponent::resized()
{
    auto area = getLocalBounds ();
    auto leftColumb = area.removeFromLeft (getWidth ()/4);

    bmpRect = leftColumb.removeFromTop (leftColumb.getHeight ()/2);
    sigRect = leftColumb;

    auto rightColumb = area.removeFromRight (getWidth ()/4);

    loopInrect = rightColumb.removeFromTop (rightColumb.getHeight ()/2);
    loopOutRect = rightColumb;
    barBeatTickRect = area.removeFromTop ( (getHeight ()/3) * 2);
    timeRect = area;

    bpmLabel.setBounds (bmpRect);
    sigLabel.setBounds (sigRect);
    barBeatTickLabel.setBounds (barBeatTickRect);
    timeLabel.setBounds (timeRect);
    loopInLabel.setBounds (loopInrect);
    loopOutLabel.setBounds (loopOutRect);
}

void PositionDisplayComponent::update()
{
    auto pos = te::getCurrentPositionInfo (m_edit);
    PlayHeadHelpers::TimeCodeStrings positionStr(pos);
    bpmLabel.setText (positionStr.bpm, juce::NotificationType::dontSendNotification);
    sigLabel.setText (positionStr.signature, juce::NotificationType::dontSendNotification);
    barBeatTickLabel.setText (positionStr.beats, juce::NotificationType::dontSendNotification);
    timeLabel.setText (positionStr.time, juce::NotificationType::dontSendNotification);
    loopInLabel.setText (positionStr.loopIn, juce::NotificationType::dontSendNotification);
    loopOutLabel.setText (positionStr.loopOut, juce::NotificationType::dontSendNotification);
}

//==============================================================================

HeaderComponent::HeaderComponent(te::Edit& edit)
    : m_newButton ("New", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_loadButton ("Load", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_saveButton ("Save", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_pluginsButton ("Plugins", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_stopButton ("Stop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_recordButton ("Record", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_settingsButton ("Settings", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_playButton ("Play", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_edit(edit)
    , m_display (edit)
{    
    addAndMakeVisible(m_newButton);
    addAndMakeVisible(m_loadButton);
    addAndMakeVisible(m_saveButton);
    addAndMakeVisible(m_playButton);
    addAndMakeVisible(m_stopButton);
    addAndMakeVisible(m_recordButton);
    addAndMakeVisible(m_settingsButton);
    addAndMakeVisible(m_pluginsButton);
    addAndMakeVisible (m_display);

    GUIHelpers::setDrawableonButton (m_newButton, BinaryData::newbox_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_loadButton, BinaryData::filedownload_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_saveButton, BinaryData::contentsaveedit_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_playButton, BinaryData::play_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_stopButton, BinaryData::stop_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_recordButton, BinaryData::record_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_settingsButton, BinaryData::headphonessettings_svg, m_btn_col);
    GUIHelpers::setDrawableonButton (m_pluginsButton, BinaryData::powerplug_svg, m_btn_col);

    m_newButton.addListener(this);
    m_loadButton.addListener(this);
    m_saveButton.addListener(this);
    m_playButton.addListener(this);
    m_stopButton.addListener(this);
    m_recordButton.addListener(this);
    m_settingsButton.addListener(this);
    m_pluginsButton.addListener(this);

    startTimer(30);
}

HeaderComponent::~HeaderComponent()
{
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

void HeaderComponent::buttonClicked(juce::Button* button)
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
        m_edit.engine.getDeviceManager().saveSettings();
    }
    if (button == &m_pluginsButton)
    {
        juce::DialogWindow::LaunchOptions o;
        o.dialogTitle                   = TRANS("Plugins");
        o.dialogBackgroundColour        = juce::Colours::black;
        o.escapeKeyTriggersCloseButton  = true;
        o.useNativeTitleBar             = true;
        o.resizable                     = true;
        o.useBottomRightCornerResizer   = true;

        auto v = new juce::PluginListComponent (m_edit.engine.getPluginManager().pluginFormatManager,
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


