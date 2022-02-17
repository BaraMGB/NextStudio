#include "HeaderComponent.h"
#include "Utilities.h"

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

    g.setColour (juce::Colour(0xff1c1c1c));
    g.fillRoundedRectangle (area.toFloat (), 5.0f);
    g.setColour (juce::Colour(0xff999999));
    g.drawRoundedRectangle (area.reduced(1).toFloat (), 5.0f, 0.5f);
}

void PositionDisplayComponent::mouseDown(const juce::MouseEvent &event)
{
    m_mousedownPosition = event.getMouseDownPosition ();
    m_mousedownBPM = m_edit.tempoSequence.getTempos ()[0]->getBpm ();
    m_mousedownBarsBeats = m_edit.tempoSequence.timeToBeats (
                m_edit.getTransport ().getCurrentPosition ());
    m_mousedownTime = m_edit.getTransport ().getCurrentPosition ();
    m_mousedownNumerator = m_edit.tempoSequence.getTimeSigAt (m_mousedownTime).numerator;
    m_mousedownDenominator = m_edit.tempoSequence.getTimeSigAt (m_mousedownTime).denominator;
    m_mousedownLoopIn = m_edit.getTransport ().getLoopRange ().getStart ();
    m_mousedownLoopOut = m_edit.getTransport ().getLoopRange ().getEnd ();
}

void PositionDisplayComponent::mouseDrag(const juce::MouseEvent &event)
{
    event.source.enableUnboundedMouseMovement (true);

    auto draggedDist = event.getDistanceFromDragStartY ();
    if (m_bmpRect.contains (m_mousedownPosition))
    {
        auto r = m_bmpRect;

        auto& tempo = m_edit.tempoSequence.getTempoAt (m_mousedownTime);
        tempo.setBpm (r.removeFromLeft (r.getWidth ()/2)
                      .contains (m_mousedownPosition)
                      ? (int) (m_mousedownBPM - (draggedDist / 10.0))
                      : m_mousedownBPM - (draggedDist / 1000.0));
        //set the Position back to the Beat Position on Mouse down
        te::TempoSequencePosition pos(m_edit.tempoSequence);
        pos.setTime (m_edit.tempoSequence.beatsToTime ( m_mousedownBarsBeats));
        m_edit.getTransport ().setCurrentPosition (pos.getTime ());
    }
    else if (m_sigRect.contains (m_mousedownPosition))
    {
        auto r = m_sigRect;
        if (r.removeFromLeft (r.getWidth ()/2)
                .contains (m_mousedownPosition))
        {
            auto newNum = juce::jlimit (1,16, m_mousedownNumerator - draggedDist);
            m_edit.tempoSequence.getTimeSigAt (m_mousedownTime).numerator
                    = newNum;
        }
        else
        {
            auto newDen = juce::jlimit ( 1,16, m_mousedownDenominator - draggedDist);
            m_edit.tempoSequence.getTimeSigAt (m_mousedownTime).denominator
                    = newDen;
        }
    }
    else if (m_barBeatTickRect.contains (m_mousedownPosition))
    {
        auto r = m_barBeatTickRect;
        auto leftRect   = r.removeFromLeft (m_barBeatTickRect.getWidth ()/3);
        auto centerRect = r.removeFromLeft (m_barBeatTickRect.getWidth ()/3);

        auto divisor = leftRect.contains (m_mousedownPosition)
                ? 0.25
                : centerRect.contains (m_mousedownPosition)
                    ? 1.0
                    : 960.0;
        m_edit.getTransport ()
                .setCurrentPosition (
                    draggedNewTime (draggedDist
                                    , m_mousedownTime
                                    , divisor
                                    , true));
    }
    else if (m_timeRect.contains (m_mousedownPosition))
    {
        auto r = m_timeRect;
        auto leftRect   = r.removeFromLeft (m_timeRect.getWidth ()/3);
        auto centerRect = r.removeFromLeft (m_timeRect.getWidth ()/3);
        auto divisor = leftRect.contains (m_mousedownPosition)
                ? 1/60.0
                : centerRect.contains (m_mousedownPosition)
                    ? 1.0
                    : 1000.0;
        m_edit.getTransport ()
                .setCurrentPosition (
                    draggedNewTime (draggedDist
                                    , m_mousedownTime
                                    , divisor
                                    , false));
    }
    else if (m_loopInrect.contains (m_mousedownPosition))
    {
        auto r = m_loopInrect;
        auto leftRect   = r.removeFromLeft (m_loopInrect.getWidth ()/3);
        auto centerRect = r.removeFromLeft (m_loopInrect.getWidth ()/3);

        auto divisor = leftRect.contains (m_mousedownPosition)
                ? 0.25
                : centerRect.contains (m_mousedownPosition)
                    ? 1.0
                    : 960.0;
        m_edit.getTransport ()
                .setLoopIn (
                    draggedNewTime (draggedDist
                                    , m_mousedownLoopIn
                                    , divisor
                                    , true));
    }
    else if (m_loopOutRect.contains (m_mousedownPosition))
    {
        auto r = m_loopOutRect;
        auto leftRect   = r.removeFromLeft (m_loopOutRect.getWidth ()/3);
        auto centerRect = r.removeFromLeft (m_loopOutRect.getWidth ()/3);

        auto divisor = leftRect.contains (m_mousedownPosition)
                ? 0.25
                : centerRect.contains (m_mousedownPosition)
                    ? 1.0
                    : 960.0;
        m_edit.getTransport ()
                .setLoopOut (
                    draggedNewTime (draggedDist
                                    , m_mousedownLoopOut
                                    , divisor
                                    , true));
    }
}

void PositionDisplayComponent::mouseUp(const juce::MouseEvent &)
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
    const auto nt = juce::NotificationType::dontSendNotification;
    PlayHeadHelpers::TimeCodeStrings positionStr(m_edit);

    m_bpmLabel.setText (positionStr.bpm, nt);
    m_sigLabel.setText (positionStr.signature, nt);
    m_barBeatTickLabel.setText (positionStr.beats, nt);
    m_timeLabel.setText (positionStr.time, nt);
    m_loopInLabel.setText (positionStr.loopIn, nt);
    m_loopOutLabel.setText (positionStr.loopOut, nt);
}

double PositionDisplayComponent::draggedNewTime(
          int draggedDistance
        , double timeAtMouseDown
        , double unitfactor
        , bool inBeat
        , int dragfactor) const
{
    te::TempoSequencePosition pos(m_edit.tempoSequence);
    pos.setTime (timeAtMouseDown);
    if (inBeat)
    {
        pos.addBeats (((double) Helpers::invert(draggedDistance) / dragfactor)
                      / unitfactor);
    }
    else
    {
        pos.setTime (timeAtMouseDown +
                ((double) Helpers::invert(draggedDistance) / dragfactor)
                     / unitfactor);
    }
    return pos.getTime ();
}



//==============================================================================

HeaderComponent::HeaderComponent(EditViewState& evs, ApplicationViewState & applicationState)
    : m_editViewState(evs)
    , m_newButton ("New", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_loadButton ("Load", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_saveButton ("Save", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_pluginsButton ("Plugins", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_stopButton ("Stop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_recordButton ("Record", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_settingsButton ("Settings", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_playButton ("Play", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_loopButton ("Loop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_clickButton ("Metronome", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_followPlayheadButton("Follow", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    , m_edit(evs.m_edit)
    , m_applicationState (applicationState)
    , m_display (m_edit)
{
    Helpers::addAndMakeVisible(*this,
                                { &m_newButton, &m_loadButton, &m_saveButton, &m_stopButton
                                , &m_playButton, &m_recordButton, &m_display, &m_clickButton, &m_loopButton
                                , &m_followPlayheadButton, &m_pluginsButton, &m_settingsButton });

    GUIHelpers::setDrawableOnButton(m_newButton, BinaryData::newbox_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(
        m_loadButton, BinaryData::filedownload_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(
        m_saveButton, BinaryData::contentsaveedit_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_playButton, BinaryData::play_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_stopButton, BinaryData::stop_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(
        m_recordButton, BinaryData::record_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(
        m_settingsButton, BinaryData::headphonessettings_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(
        m_pluginsButton, BinaryData::powerplug_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_loopButton,
                                    BinaryData::cached_svg,
                                    m_edit.getTransport().looping ? m_btn_col
                                                                  : "#666666");
    GUIHelpers::setDrawableOnButton(m_clickButton,
                                    BinaryData::metronome_svg,
                                    m_edit.clickTrackEnabled ? m_btn_col
                                                             : "#666666");
    GUIHelpers::setDrawableOnButton(m_followPlayheadButton,
                                    BinaryData::follow_svg,
                                    m_editViewState.viewFollowsPos() ? m_btn_col
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
    m_clickButton.addListener (this);
    m_followPlayheadButton.addListener (this);

    startTimer(30);
}

HeaderComponent::~HeaderComponent()
= default;

void HeaderComponent::resized()
{
    juce::Rectangle<int> area = getLocalBounds();
    auto gap = area.getHeight()/8;

    juce::FlexBox fileButtons = createFlexBox(juce::FlexBox::JustifyContent::flexStart);
    juce::FlexBox transport   = createFlexBox(juce::FlexBox::JustifyContent::flexEnd);
    juce::FlexBox position    = createFlexBox(juce::FlexBox::JustifyContent::center);
    juce::FlexBox timelineSet = createFlexBox(juce::FlexBox::JustifyContent::flexStart);
    juce::FlexBox settings    = createFlexBox(juce::FlexBox::JustifyContent::flexEnd);
    juce::FlexBox container   = createFlexBox(juce::FlexBox::JustifyContent::spaceBetween);

    area.removeFromBottom(gap);
    area.reduce(gap, 0);
    auto buttonSize = area.getHeight();

    addButtonsToFlexBox(fileButtons
                        , {&m_newButton, &m_loadButton, &m_saveButton}
                        , buttonSize, buttonSize, gap);
    addButtonsToFlexBox(transport
                        , {&m_playButton, &m_stopButton, &m_recordButton}
                        , buttonSize, buttonSize, gap);
    addButtonsToFlexBox(position, {&m_display}
                        , area.getWidth()/5 - gap * 4
                        , buttonSize
                        , gap);
    addButtonsToFlexBox(timelineSet
                        , {&m_clickButton, &m_loopButton, &m_followPlayheadButton}
                        , buttonSize, buttonSize, gap);
    addButtonsToFlexBox(settings
                        , {&m_pluginsButton, &m_settingsButton}
                        , buttonSize, buttonSize, gap);
    addFlexBoxToFlexBox( container
                        , {&fileButtons, &transport, &position, &timelineSet, &settings}
                        , area.getWidth()/5, buttonSize);

    container.performLayout(area);
}

juce::FlexBox HeaderComponent::createFlexBox(juce::FlexBox::JustifyContent justify)
{
    juce::FlexBox box;
    box.justifyContent = justify;
    box.alignContent = juce::FlexBox::AlignContent::center;
    box.flexDirection = juce::FlexBox::Direction::row;
    box.flexWrap = juce::FlexBox::Wrap::noWrap;
    return box;
}

void HeaderComponent::buttonClicked(juce::Button* button)
{
    if (button == &m_newButton)
    {
    }

    if (button == &m_playButton)
    {
        EngineHelpers::togglePlay(m_editViewState);
        const char* svgbin = m_edit.getTransport().isPlaying()
                           ? BinaryData::pause_svg
                           : BinaryData::play_svg;

        GUIHelpers::setDrawableOnButton(m_playButton, svgbin, m_btn_col);
    }
    if (button == &m_stopButton)
    {
        EngineHelpers::stopPlay(m_editViewState);
        GUIHelpers::setDrawableOnButton(
            m_playButton, BinaryData::play_svg, m_btn_col);

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
        juce::DialogWindow::LaunchOptions o;
        o.dialogTitle = TRANS("Audio Settings");
        o.dialogBackgroundColour = juce::LookAndFeel::getDefaultLookAndFeel()
                .findColour (juce::ResizableWindow::backgroundColourId);
        auto audiosettings = new AudioMidiSettings(m_edit.engine);
        o.content.setOwned (audiosettings);
        o.content->setSize (400, 600);
        o.runModal ();
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
        EngineHelpers::toggleLoop (m_edit);
        updateLoopButton();
    }
    if (button == &m_clickButton)
    {
        m_edit.clickTrackEnabled = !m_edit.clickTrackEnabled;
        GUIHelpers::setDrawableOnButton(m_clickButton,
                                        BinaryData::metronome_svg,
                                        m_edit.clickTrackEnabled ? m_btn_col
                                                                 : "#666666");
    }

    if (button == &m_followPlayheadButton)
    {
        m_editViewState.toggleFollowPlayhead();
        GUIHelpers::setDrawableOnButton(
            m_followPlayheadButton,
            BinaryData::follow_svg,
            m_editViewState.viewFollowsPos() ? m_btn_col : "#666666");
    }

    if (button == &m_saveButton)
    {
        GUIHelpers::saveEdit (m_editViewState
                              , juce::File::createFileWithoutCheckingPath (
                                  m_applicationState.m_projectsDir));
    }
    if (button == &m_loadButton)
    {
        juce::WildcardFileFilter wildcardFilter ("*.tracktionedit"
                                                 , juce::String()
                                                 , "Next Studio Project File");

        juce::FileBrowserComponent browser (juce::FileBrowserComponent::openMode
                                            + juce::FileBrowserComponent::canSelectFiles
                                            , juce::File(m_applicationState.m_projectsDir)
                                            , &wildcardFilter
                                            , nullptr);

        juce::FileChooserDialogBox dialogBox ("Load a project",
                                        "Please choose some kind of file that you want to load...",
                                        browser,
                                        true,
                                        juce::Colours::lightgrey);

        if (dialogBox.show())
        {
            m_loadingFile = browser.getSelectedFile (0);
            sendChangeMessage ();
        }
    }
}
void HeaderComponent::updateLoopButton()
{
    GUIHelpers::setDrawableOnButton(m_loopButton,
                                            BinaryData::cached_svg,
                                    m_edit.getTransport().looping ? m_btn_col
                                                                  : "#666666");
}

void HeaderComponent::timerCallback()
{
    m_display.update ();
}

juce::File HeaderComponent::loadingFile() const
{
    return m_loadingFile;
}

void HeaderComponent::addButtonsToFlexBox(juce::FlexBox& box,
                                          const juce::Array<juce::Component*>& buttons,
                                          int w, int h, int margin)
{
    for (auto b : buttons)
    {
        box.items.add(juce::FlexItem((float) w,(float) h,*b).withMargin((float) margin));
    }
}

void HeaderComponent::addFlexBoxToFlexBox(juce::FlexBox& target
                                          , const juce::Array<juce::FlexBox*>& items
                                          , int w, int h)
{
    for (auto b : items)
    {
        target.items.add(juce::FlexItem((float) w,(float) h,*b));
    }
}
