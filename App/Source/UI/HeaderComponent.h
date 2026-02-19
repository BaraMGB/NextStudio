
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

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "UI/PositionDisplayComponent.h"
#include "Utilities/ApplicationViewState.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;

class HeaderComponent
    : public juce::Component
    , public juce::Button::Listener
    , public juce::Timer
    , public juce::ChangeBroadcaster
{
public:
    HeaderComponent(EditViewState &, ApplicationViewState &applicationState, juce::ApplicationCommandManager &commandManager);
    ~HeaderComponent() override;

    void updateIcons();
    void paint(juce::Graphics &g) override;
    void resized() override;
    void buttonClicked(juce::Button *button) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void timerCallback() override;

    juce::File getSelectedFile() const;

    void loopButtonClicked();

private:
    void showFollowMenu();
    EditViewState &m_editViewState;
    static juce::FlexBox createFlexBox(juce::FlexBox::JustifyContent justify);
    int getButtonSize();
    int getGapSize();

    void addButtonsToFlexBox(juce::FlexBox &box, const juce::Array<juce::Component *> &buttons, int width = 0);
    void addFlexBoxToFlexBox(juce::FlexBox &target, const juce::Array<juce::FlexBox *> &items);
    juce::DrawableButton m_stopButton, m_recordButton, m_playButton, m_loopButton, m_clickButton, m_followPlayheadButton;
    te::Edit &m_edit;
    ApplicationViewState &m_applicationState;

    juce::ApplicationCommandManager &m_commandManager;
    PositionDisplayComponent m_display;

    juce::Colour m_btn_col;

    juce::File m_loadingFile{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)
};
