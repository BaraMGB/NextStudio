#include "HeaderComponent.h"

PositionDisplayComponent::PositionDisplayComponent(te::Edit &edit)
    : m_edit(edit)
{
    Helpers::addAndMakeVisible (*this, {   &m_bpmLabel,
                                           &m_sigLabel,
                                           &m_barBeatTickLabel,
                                           &m_timeLabel,
                                           &m_loopInLabel,
                                           &m_loopOutLabel  });
    m_bpmLabel.setJustificationType (juce::Justification::centred);
    m_sigLabel.setJustificationType (juce::Justification::centred);
    m_barBeatTickLabel.setJustificationType (juce::Justification::centred);
    m_barBeatTickLabel.setFont (28);
    m_timeLabel.setJustificationType (juce::Justification::centred);
    m_loopInLabel.setJustificationType (juce::Justification::centred);
    m_loopOutLabel.setJustificationType (juce::Justification::centred);

    m_bpmLabel.setInterceptsMouseClicks (false, false);
    m_sigLabel.setInterceptsMouseClicks (false, false);
    m_barBeatTickLabel.setInterceptsMouseClicks (false, false);
    m_timeLabel.setInterceptsMouseClicks (false, false);
    m_loopInLabel.setInterceptsMouseClicks (false, false);
    m_loopOutLabel.setInterceptsMouseClicks (false, false);

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
    m_barsBeatsAtMd = m_edit.tempoSequence.timeToBeats (
                m_edit.getTransport ().getCurrentPosition ());
    m_timeAtMouseDown = m_edit.getTransport ().getCurrentPosition ();
    m_numAtMouseDown = m_edit.tempoSequence.getTimeSig(0)->numerator;
    m_denAtMouseDown = m_edit.tempoSequence.getTimeSig(0)->denominator;
    m_loopInAtMouseDown = m_edit.getTransport ().getLoopRange ().getStart ();
    m_loopOutAtMouseDown = m_edit.getTransport ().getLoopRange ().getEnd ();
    te::TempoSequencePosition pos(m_edit.tempoSequence);
}

void PositionDisplayComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (m_bmpRect.contains (m_MouseDownPosition))
    {
        //m_edit.getTransport().setUserDragging (true);

        auto r = m_bmpRect;
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
            tempo->setBpm (m_bpmAtMd
                           - (event.getDistanceFromDragStartY () /100.0));
        }
        //set the Position back to the Beat Position on Mouse down
        te::TempoSequencePosition pos(m_edit.tempoSequence);
        pos.setTime (m_edit.tempoSequence.beatsToTime ( m_barsBeatsAtMd));
        m_edit.getTransport ().setCurrentPosition (pos.getTime ());
    }
    else if (m_sigRect.contains (m_MouseDownPosition))
    {
    }
    else if (m_barBeatTickRect.contains (m_MouseDownPosition))
    {
        auto r = m_barBeatTickRect;
        if (r.removeFromLeft (
                m_barBeatTickRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY () * 4));
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else if(r.removeFromLeft (
                   m_barBeatTickRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY ()));
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY () / 960.0));
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
    }
    else if (m_timeRect.contains (m_MouseDownPosition))
    {
        auto r = m_timeRect;
        if (r.removeFromLeft (
                    m_timeRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_timeAtMouseDown
                         - (event.getDistanceFromDragStartY () * 60));
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else if (r.removeFromLeft (
                     m_timeRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_timeAtMouseDown
                          - event.getDistanceFromDragStartY ());
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_timeAtMouseDown
                         - (event.getDistanceFromDragStartY () * 0.001));
            m_edit.getTransport ().setCurrentPosition (pos.getTime ());
        }
    }
    else if (m_loopInrect.contains (m_MouseDownPosition))
    {
        auto r = m_loopInrect;
        if (r.removeFromLeft (
                    m_loopInrect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_loopInAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY () * 4));
            m_edit.getTransport ().setLoopIn (pos.getTime ());
        }
        else if(r.removeFromLeft (
                    m_loopInrect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopInAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY ()));
            m_edit.getTransport ().setLoopIn (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopInAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY () / 960.0));
            m_edit.getTransport ().setLoopIn (pos.getTime ());
        }
    }
    else if (m_loopOutRect.contains (m_MouseDownPosition))
    {
        auto r = m_loopOutRect;
        if (r.removeFromLeft (
                    m_loopOutRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime (m_loopOutAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY () * 4));
            m_edit.getTransport ().setLoopOut (pos.getTime ());
        }
        else if(r.removeFromLeft (
                    m_loopOutRect.getWidth ()/3).contains (m_MouseDownPosition))
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopOutAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY ()));
            m_edit.getTransport ().setLoopOut (pos.getTime ());
        }
        else
        {
            event.source.enableUnboundedMouseMovement (true);
            te::TempoSequencePosition pos(m_edit.tempoSequence);
            pos.setTime ( m_loopOutAtMouseDown);
            pos.setPPQTime (pos.getPPQTime ()
                            - (event.getDistanceFromDragStartY () / 960.0));
            m_edit.getTransport ().setLoopOut (pos.getTime ());
        }
    }
}

void PositionDisplayComponent::mouseUp(const juce::MouseEvent &/*e*/)
{
    m_edit.getTransport ().setUserDragging (false);
}

void PositionDisplayComponent::resized()
{
    auto area = getLocalBounds ();
    auto leftColumb = area.removeFromLeft (getWidth ()/4);

    m_bmpRect = leftColumb.removeFromTop (leftColumb.getHeight ()/2);
    m_sigRect = leftColumb;

    auto rightColumb = area.removeFromRight (getWidth ()/4);

    m_loopInrect = rightColumb.removeFromTop (rightColumb.getHeight ()/2);
    m_loopOutRect = rightColumb;
    m_barBeatTickRect = area.removeFromTop ( (getHeight ()/3) * 2);
    m_timeRect = area;

    m_bpmLabel.setBounds (m_bmpRect);
    m_sigLabel.setBounds (m_sigRect);
    m_barBeatTickLabel.setBounds (m_barBeatTickRect);
    m_timeLabel.setBounds (m_timeRect);
    m_loopInLabel.setBounds (m_loopInrect);
    m_loopOutLabel.setBounds (m_loopOutRect);
}

void PositionDisplayComponent::update()
{
    const auto pos = te::getCurrentPositionInfo (m_edit);
    const auto nt = juce::NotificationType::dontSendNotification;
    PlayHeadHelpers::TimeCodeStrings positionStr(pos);

    m_bpmLabel.setText (positionStr.bpm, nt);
    m_sigLabel.setText (positionStr.signature, nt);
    m_barBeatTickLabel.setText (positionStr.beats, nt);
    m_timeLabel.setText (positionStr.time, nt);
    m_loopInLabel.setText (positionStr.loopIn, nt);
    m_loopOutLabel.setText (positionStr.loopOut, nt);
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
    , m_loopButton ("Loop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
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
    addAndMakeVisible (m_loopButton);
    addAndMakeVisible (m_display);

    GUIHelpers::setDrawableonButton (m_newButton
                                     , BinaryData::newbox_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_loadButton
                                     , BinaryData::filedownload_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_saveButton
                                     , BinaryData::contentsaveedit_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_playButton
                                     , BinaryData::play_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_stopButton
                                     , BinaryData::stop_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_recordButton
                                     , BinaryData::record_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_settingsButton
                                     , BinaryData::headphonessettings_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_pluginsButton
                                     , BinaryData::powerplug_svg
                                     , m_btn_col);
    GUIHelpers::setDrawableonButton (m_loopButton
                                     , BinaryData::cached_svg
                                     , m_edit.getTransport ().looping
                                       ? m_btn_col
                                       : "#666666");

    m_newButton.addListener(this);
    m_loadButton.addListener(this);
    m_saveButton.addListener(this);
    m_playButton.addListener(this);
    m_stopButton.addListener(this);
    m_recordButton.addListener(this);
    m_settingsButton.addListener(this);
    m_pluginsButton.addListener(this);
    m_loopButton.addListener (this);

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
    auto displayRect = juce::Rectangle<int> ((area.getX ()
                                              + (area.getWidth ()/2)
                                              - (displayWidth/2))
                                             , area.getY ()
                                             , displayWidth
                                             , area.getHeight ());

    m_settingsButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));
    area.removeFromRight(gap/4);
    m_pluginsButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));

    area.removeFromLeft(gap/4);
    m_newButton.setBounds(area.removeFromLeft(area.getHeight() + gap/2));
    area.removeFromLeft(gap/4);
    m_loadButton.setBounds(area.removeFromLeft(area.getHeight() + gap/2));
    area.removeFromLeft(gap/4);
    m_saveButton.setBounds(area.removeFromLeft(area.getHeight() + gap/2));


    area = juce::Rectangle<int>(getLocalBounds ().getX ()
                                , getLocalBounds ().getY ()
                                , (getLocalBounds ().getWidth ()/2)
                                    - (displayWidth/2)
                                , getLocalBounds ().getHeight ());

    area.removeFromBottom(gap/4);
    area.removeFromRight(gap);
    m_recordButton.setBounds(area.removeFromRight(area.getHeight()+gap/2));
    area.removeFromRight(gap/4);
    m_stopButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));
    area.removeFromRight(gap/4);
    m_playButton.setBounds(area.removeFromRight(area.getHeight() + gap/2));
    m_loopButton.setBounds ({displayRect.getTopRight ().x + gap
                             , 0
                             , area.getHeight ()
                             , area.getHeight ()
                            });
    m_display.setBounds (displayRect);
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
        {
            transport.stop(false, false);
        }
        else
        {
            transport.play(true);
        }
        const char* svgbin = m_edit.getTransport().isPlaying()
                           ? BinaryData::pause_svg
                           : BinaryData::play_svg;

        GUIHelpers::setDrawableonButton (m_playButton, svgbin, m_btn_col);
    }
    if (button == &m_stopButton)
    {
        m_edit.getTransport().stop(false, false, true, true);
        m_edit.getTransport().setCurrentPosition(0);
        GUIHelpers::setDrawableonButton (m_playButton
                                         , BinaryData::play_svg
                                         , m_btn_col);
    }
    if (button == &m_recordButton)
    {
        bool wasRecording = m_edit.getTransport().isRecording();
        EngineHelpers::toggleRecord (m_edit);
        if (wasRecording)
        {
            te::EditFileOperations (m_edit).save (true, true, false);
        }
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

        auto v = new juce::PluginListComponent (
                      m_edit.engine.getPluginManager().pluginFormatManager
                    , m_edit.engine.getPluginManager().knownPluginList
                    , m_edit.engine.getTemporaryFileManager()
                        .getTempFile ("PluginScanDeadMansPedal")
                    , te::getApplicationSettings());
        v->setSize (800, 600);
        o.content.setOwned (v);
        o.launchAsync();
    }
    if (button == &m_loopButton)
    {
        std::cout << "loopbutton" << std::endl;
        EngineHelpers::toggleLoop (m_edit);
        GUIHelpers::setDrawableonButton (m_loopButton
                                         , BinaryData::cached_svg
                                         , m_edit.getTransport ().looping
                                           ? m_btn_col
                                           : "#666666");
    }
}

void HeaderComponent::timerCallback()
{
    m_display.update ();
}
