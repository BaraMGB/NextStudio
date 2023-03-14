#include "HeaderComponent.h"


HeaderComponent::HeaderComponent(EditViewState& evs, ApplicationViewState & applicationState, juce::ApplicationCommandManager& commandManager)
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
    , m_commandManager(commandManager)
    , m_display (m_edit)
{
    Helpers::addAndMakeVisible(*this,
        { &m_newButton, &m_loadButton, &m_saveButton, &m_stopButton
        , &m_playButton, &m_recordButton, &m_display, &m_clickButton, &m_loopButton
        , &m_followPlayheadButton, &m_pluginsButton, &m_settingsButton });

    GUIHelpers::setDrawableOnButton(m_newButton, BinaryData::newbox_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_loadButton, BinaryData::filedownload_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_saveButton, BinaryData::contentsaveedit_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_playButton, BinaryData::play_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_stopButton, BinaryData::stop_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_recordButton, BinaryData::record_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_settingsButton, BinaryData::headphonessettings_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_pluginsButton, BinaryData::powerplug_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_loopButton, BinaryData::cached_svg,
                                    m_edit.getTransport().looping ? m_btn_col : "#666666");
    GUIHelpers::setDrawableOnButton(m_clickButton, BinaryData::metronome_svg,
                                    m_edit.clickTrackEnabled ? m_btn_col : "#666666");
    GUIHelpers::setDrawableOnButton(m_followPlayheadButton, BinaryData::follow_svg,
                                    m_editViewState.viewFollowsPos() ? m_btn_col : "#666666");
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
{
    m_newButton.removeListener(this);
    m_loadButton.removeListener(this);
    m_saveButton.removeListener(this);
    m_playButton.removeListener(this);
    m_stopButton.removeListener(this);
    m_recordButton.removeListener(this);
    m_settingsButton.removeListener(this);
    m_pluginsButton.removeListener(this);
    m_loopButton.removeListener (this);
    m_clickButton.removeListener (this);
    m_followPlayheadButton.removeListener (this);
}

void HeaderComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();
    g.setColour(juce::Colour(0xff242424));
    g.fillRoundedRectangle (area.toFloat(), 10);
}

void HeaderComponent::resized()
{
    auto area = getLocalBounds();

    auto fileButtonsBox = createFlexBox(juce::FlexBox::JustifyContent::flexStart);
    auto transportBox   = createFlexBox(juce::FlexBox::JustifyContent::flexEnd);
    auto positionBox    = createFlexBox(juce::FlexBox::JustifyContent::center);
    auto timelineSetBox = createFlexBox(juce::FlexBox::JustifyContent::flexStart);
    auto settingsBox    = createFlexBox(juce::FlexBox::JustifyContent::flexEnd);

    auto container      = createFlexBox(juce::FlexBox::JustifyContent::spaceBetween);

    auto fileButtons      = {&m_newButton, &m_loadButton, &m_saveButton};
    auto transportButtons = {&m_playButton, &m_stopButton, &m_recordButton};
    auto timeLineButtons  = {&m_clickButton, &m_loopButton, &m_followPlayheadButton};
    auto SettingsButtons  = {&m_pluginsButton, &m_settingsButton};


    auto displayWidth =  (area.getWidth()/5) - (getGapSize() * 4) ;

    addButtonsToFlexBox(fileButtonsBox, fileButtons);
    addButtonsToFlexBox(transportBox, transportButtons);
    addButtonsToFlexBox(positionBox, {&m_display}, displayWidth);
    addButtonsToFlexBox(timelineSetBox, timeLineButtons);
    addButtonsToFlexBox(settingsBox, SettingsButtons);

    auto containers = {&fileButtonsBox, &transportBox, &positionBox, &timelineSetBox, &settingsBox};
    addFlexBoxToFlexBox(container, containers);

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
        auto audiosettings = new SettingsView(m_edit.engine, m_commandManager);
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
        loopButtonClicked();
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
                                        juce::Colour::fromString(m_btn_col));

        if (dialogBox.show())
        {
            m_loadingFile = browser.getSelectedFile (0);
            sendChangeMessage ();
        }
    }
    if (button == &m_newButton)
    {
        m_loadingFile = juce::File();
        sendChangeMessage();
    }
}

void HeaderComponent::loopButtonClicked()
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

juce::File HeaderComponent::getSelectedFile() const
{
    return m_loadingFile;
}

void HeaderComponent::addButtonsToFlexBox(juce::FlexBox& box,
                                          const juce::Array<juce::Component*>& buttons,
                                          int width)
{
    auto w = (width == 0) ?     getButtonSize() : width;
    auto h = getButtonSize();
    auto margin = getGapSize();

    for (auto b : buttons)
        box.items.add(juce::FlexItem((float) w,(float) h,*b).withMargin((float) margin));
}

void HeaderComponent::addFlexBoxToFlexBox(juce::FlexBox& target
                                          , const juce::Array<juce::FlexBox*>& items)
{
    auto w = getWidth() / items.size();
    auto h = getButtonSize();
     
    for (auto b : items)
        target.items.add(juce::FlexItem((float) w,(float) h,*b));
}

int HeaderComponent::getButtonSize()
{
    auto h = getLocalBounds().getHeight();
    auto margin = getGapSize() * 2;

    return h - margin;
}

int HeaderComponent::getGapSize()
{
    auto h = getLocalBounds().getHeight();
    const auto div = 8;

    return h / div;
}
