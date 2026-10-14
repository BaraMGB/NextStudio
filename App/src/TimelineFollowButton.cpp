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

#include "TimelineFollowButton.h"
#include "TimeLineComponent.h"
#include "Utilities.h"

TimelineFollowButton::TimelineFollowButton(EditViewState &editViewState, TimeLineComponent &timeLine)
    : juce::DrawableButton("Follow", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize),
      m_editViewState(editViewState),
      m_timeLine(timeLine)
{
    setButtonText("Follow Playhead");
    setTooltip(GUIHelpers::translate("View follows playhead on/off", m_editViewState.m_applicationState));
    onClick = [this]
    {
        m_editViewState.toggleFollowPlayhead(m_timeLine.getTimeLineID());
        refresh();
    };

    m_editViewState.m_edit.state.addListener(this);
    m_editViewState.m_applicationState.m_applicationStateValueTree.addListener(this);
    refresh();
}

TimelineFollowButton::~TimelineFollowButton()
{
    m_editViewState.m_applicationState.m_applicationStateValueTree.removeListener(this);
    m_editViewState.m_edit.state.removeListener(this);
}

void TimelineFollowButton::refresh()
{
    const auto active = m_editViewState.viewFollowsPos(m_timeLine.getTimeLineID());
    const auto colour = active ? m_editViewState.m_applicationState.getButtonTextColour()
                               : juce::Colour(0xff666666);
    GUIHelpers::setDrawableOnButton(*this, BinaryData::follow_svg, colour);
}

void TimelineFollowButton::mouseDown(const juce::MouseEvent &event)
{
    if (event.mods.isRightButtonDown())
    {
        showFollowMenu();
        return;
    }

    juce::DrawableButton::mouseDown(event);
}

void TimelineFollowButton::showFollowMenu()
{
    const auto timeLineID = m_timeLine.getTimeLineID();
    const auto currentMode = m_editViewState.getFollowMode(timeLineID);
    const auto isFollowOn = m_editViewState.viewFollowsPos(timeLineID);

    juce::PopupMenu menu;
    menu.addItem(1, "Follow Playhead", true, isFollowOn);
    menu.addSeparator();
    menu.addItem(2, "Page Scroll (Smooth)", true, currentMode == EditViewState::FollowMode::Page);
    menu.addItem(3, "Continuous Scroll (Center)", true, currentMode == EditViewState::FollowMode::Continuous);

    juce::Component::SafePointer<TimelineFollowButton> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                       [safeThis, timeLineID](int result)
                       {
                           if (safeThis == nullptr)
                               return;

                           if (result == 1)
                               safeThis->m_editViewState.toggleFollowPlayhead(timeLineID);
                           else if (result == 2)
                               safeThis->m_editViewState.setFollowMode(timeLineID, EditViewState::FollowMode::Page);
                           else if (result == 3)
                               safeThis->m_editViewState.setFollowMode(timeLineID, EditViewState::FollowMode::Continuous);

                           safeThis->refresh();
                       });
}

void TimelineFollowButton::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property)
{
    if ((tree.hasType(m_timeLine.getTimeLineID())
         && (property == IDs::followsPlayhead || property == IDs::followMode))
        || tree.hasType(IDs::ThemeState))
        refresh();
}
