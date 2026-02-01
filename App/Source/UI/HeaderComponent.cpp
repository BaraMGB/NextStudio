
/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "UI/HeaderComponent.h"
#include "SideBrowser/RenderDialog.h"
#include "Utilities/Utilities.h"

HeaderComponent::HeaderComponent(EditViewState &evs, ApplicationViewState &applicationState, juce::ApplicationCommandManager &commandManager)
    : m_editViewState(evs),
      m_btn_col(evs.m_applicationState.getButtonTextColour()),
      m_stopButton("Stop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_recordButton("Record", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_playButton("Play", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_loopButton("Loop", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_clickButton("Metronome", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_followPlayheadButton("Follow", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_edit(evs.m_edit),
      m_applicationState(applicationState),
      m_commandManager(commandManager),
      m_display(m_edit)
{
    Helpers::addAndMakeVisible(*this, {&m_stopButton, &m_playButton, &m_recordButton, &m_display, &m_clickButton, &m_loopButton, &m_followPlayheadButton});

    updateIcons();

    m_playButton.addListener(this);
    m_stopButton.addListener(this);
    m_recordButton.addListener(this);
    m_loopButton.addListener(this);
    m_clickButton.addListener(this);
    m_followPlayheadButton.addListener(this);
    m_followPlayheadButton.addMouseListener(this, false);

    m_playButton.setTooltip(GUIHelpers::translate("Play", m_editViewState.m_applicationState));
    m_stopButton.setTooltip(GUIHelpers::translate("Stop", m_editViewState.m_applicationState));
    m_recordButton.setTooltip(GUIHelpers::translate("Recording", m_editViewState.m_applicationState));
    m_loopButton.setTooltip(GUIHelpers::translate("Toggle loop on/off", m_editViewState.m_applicationState));
    m_clickButton.setTooltip(GUIHelpers::translate("Toggle metronome on/off", m_editViewState.m_applicationState));
    m_followPlayheadButton.setTooltip(GUIHelpers::translate("View follows playhead on/off", m_editViewState.m_applicationState));

    startTimer(30);
}

HeaderComponent::~HeaderComponent()
{
    m_playButton.removeListener(this);
    m_stopButton.removeListener(this);
    m_recordButton.removeListener(this);
    m_loopButton.removeListener(this);
    m_clickButton.removeListener(this);
    m_followPlayheadButton.removeListener(this);
}

void HeaderComponent::updateIcons()
{

    m_btn_col = m_editViewState.m_applicationState.getButtonTextColour();
    GUIHelpers::setDrawableOnButton(m_playButton, BinaryData::play_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_stopButton, BinaryData::stop_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_recordButton, BinaryData::record_svg, m_btn_col);
    GUIHelpers::setDrawableOnButton(m_loopButton, BinaryData::cached_svg, m_edit.getTransport().looping ? m_btn_col : juce::Colour(0xff666666));
    GUIHelpers::setDrawableOnButton(m_clickButton, BinaryData::metronome_svg, m_edit.clickTrackEnabled ? m_btn_col : juce::Colour(0xff666666));
    GUIHelpers::setDrawableOnButton(m_followPlayheadButton, BinaryData::follow_svg, m_editViewState.viewFollowsPos() ? m_btn_col : juce::Colour(0xff666666));
}
void HeaderComponent::paint(juce::Graphics &g)
{
    auto area = getLocalBounds();
    g.setColour(m_applicationState.getBackgroundColour1());
    g.fillRect(area);

    GUIHelpers::drawFakeRoundCorners(g, area.toFloat(), m_applicationState.getMainFrameColour(), m_applicationState.getBorderColour());
}

void HeaderComponent::resized()
{
    auto area = getLocalBounds();

    auto transportBox = createFlexBox(juce::FlexBox::JustifyContent::flexEnd);
    auto positionBox = createFlexBox(juce::FlexBox::JustifyContent::center);
    auto timelineSetBox = createFlexBox(juce::FlexBox::JustifyContent::flexStart);

    auto container = createFlexBox(juce::FlexBox::JustifyContent::spaceBetween);

    auto transportButtons = {&m_playButton, &m_stopButton, &m_recordButton};
    auto timeLineButtons = {&m_clickButton, &m_loopButton, &m_followPlayheadButton};

    auto displayWidth = (area.getWidth() / 5) - (getGapSize() * 4);

    addButtonsToFlexBox(transportBox, transportButtons);
    addButtonsToFlexBox(positionBox, {&m_display}, displayWidth);
    addButtonsToFlexBox(timelineSetBox, timeLineButtons);

    auto containers = {&transportBox, &positionBox, &timelineSetBox};
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

void HeaderComponent::buttonClicked(juce::Button *button)
{
    if (button == &m_playButton)
    {
        EngineHelpers::togglePlay(m_editViewState);
        const char *svgbin = m_edit.getTransport().isPlaying() ? BinaryData::pause_svg : BinaryData::play_svg;

        GUIHelpers::setDrawableOnButton(m_playButton, svgbin, m_btn_col);
    }
    if (button == &m_stopButton)
    {
        EngineHelpers::stopPlay(m_editViewState);
        GUIHelpers::setDrawableOnButton(m_playButton, BinaryData::play_svg, m_btn_col);
    }
    if (button == &m_recordButton)
    {
        bool wasRecording = m_edit.getTransport().isRecording();
        EngineHelpers::toggleRecord(m_edit);
        if (wasRecording)
        {
            te::EditFileOperations(m_edit).save(true, true, false);
        }
    }
    if (button == &m_loopButton)
    {
        EngineHelpers::toggleLoop(m_edit);
        loopButtonClicked();
    }
    if (button == &m_clickButton)
    {
        m_edit.clickTrackEnabled = !m_edit.clickTrackEnabled;
        GUIHelpers::setDrawableOnButton(m_clickButton, BinaryData::metronome_svg, m_edit.clickTrackEnabled ? m_btn_col : juce::Colour(0xff666666));
    }

    if (button == &m_followPlayheadButton)
    {
        m_editViewState.toggleFollowPlayhead();
        updateIcons();
    }
}

void HeaderComponent::mouseDown(const juce::MouseEvent &e)
{
    if (e.eventComponent == &m_followPlayheadButton && e.mods.isRightButtonDown())
    {
        showFollowMenu();
    }
}

void HeaderComponent::showFollowMenu()
{
    juce::PopupMenu m;
    auto currentMode = m_editViewState.getFollowMode();
    auto isFollowOn = m_editViewState.viewFollowsPos();

    m.addItem(1, "Follow Playhead", true, isFollowOn);
    m.addSeparator();
    m.addItem(2, "Page Scroll (Smooth)", true, currentMode == EditViewState::FollowMode::Page);
    m.addItem(3, "Continuous Scroll (Center)", true, currentMode == EditViewState::FollowMode::Continuous);

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&m_followPlayheadButton),
                    [this](int result)
                    {
                        if (result == 1)
                        {
                            m_editViewState.toggleFollowPlayhead();
                        }
                        else if (result == 2)
                        {
                            m_editViewState.setFollowMode(EditViewState::FollowMode::Page);
                        }
                        else if (result == 3)
                        {
                            m_editViewState.setFollowMode(EditViewState::FollowMode::Continuous);
                        }
                        updateIcons();
                    });
}

void HeaderComponent::loopButtonClicked() { GUIHelpers::setDrawableOnButton(m_loopButton, BinaryData::cached_svg, m_edit.getTransport().looping ? m_btn_col : juce::Colour(0xff666666)); }

void HeaderComponent::timerCallback() { m_display.update(); }

juce::File HeaderComponent::getSelectedFile() const { return m_loadingFile; }

void HeaderComponent::addButtonsToFlexBox(juce::FlexBox &box, const juce::Array<juce::Component *> &buttons, int width)
{
    auto w = (width == 0) ? getButtonSize() : width;
    auto h = getButtonSize();
    auto margin = getGapSize();

    for (auto b : buttons)
        box.items.add(juce::FlexItem((float)w, (float)h, *b).withMargin((float)margin));
}

void HeaderComponent::addFlexBoxToFlexBox(juce::FlexBox &target, const juce::Array<juce::FlexBox *> &items)
{
    auto w = getWidth() / items.size();
    auto h = getButtonSize();

    for (auto b : items)
        target.items.add(juce::FlexItem((float)w, (float)h, *b));
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
