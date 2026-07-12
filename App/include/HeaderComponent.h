
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
#include "PositionDisplayComponent.h"
#include "ApplicationViewState.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class PreRollCounterButton : public juce::DrawableButton
{
public:
    PreRollCounterButton()
        : juce::DrawableButton("Pre-Roll Counter", juce::DrawableButton::ButtonStyle::ImageOnButtonBackgroundOriginalSize)
    {
    }

    void setCounterText(const juce::String &text)
    {
        if (m_counterText != text)
        {
            m_counterText = text;
            repaint();
        }
    }

    void setCounterActive(bool shouldBeActive)
    {
        if (m_counterActive != shouldBeActive)
        {
            m_counterActive = shouldBeActive;
            updateImageVisibility();
            repaint();
        }
    }

    void buttonStateChanged() override
    {
        juce::DrawableButton::buttonStateChanged();
        updateImageVisibility();
    }

    void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        if (!m_counterActive)
        {
            juce::DrawableButton::paintButton(g, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
            return;
        }

        auto area = getLocalBounds().toFloat().reduced(2.0f);
        auto background = juce::Colour(0xffc43a31);

        if (shouldDrawButtonAsDown)
            background = background.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            background = background.brighter(0.15f);

        g.setColour(background);
        g.fillRoundedRectangle(area, 5.0f);
        g.setColour(juce::Colour(0x55ffffff));
        g.drawRoundedRectangle(area, 5.0f, 1.0f);
        g.setColour(juce::Colours::black);
        g.setFont(juce::FontOptions((float) getHeight() * 0.48f).withStyle("Bold"));
        g.drawFittedText(m_counterText, getLocalBounds(), juce::Justification::centred, 1);
    }

private:
    void updateImageVisibility()
    {
        for (int i = 0; i < getNumChildComponents(); ++i)
            if (auto *child = getChildComponent(i))
                child->setVisible(!m_counterActive);
    }

    juce::String m_counterText;
    bool m_counterActive = false;
};

class HeaderComponent
    : public juce::Component
    , public juce::Button::Listener
    , public juce::ValueTree::Listener
    , public juce::ChangeListener
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
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;

    juce::File getSelectedFile() const;

    void loopButtonClicked();

private:
    juce::String getCountInModeText() const;
    void showCountInMenu();
    void showFollowMenu();
    void updateCountInButton();
    void updateUndoRedoButtons(bool force = false);
    void updateCountInDisplay();
    EditViewState &m_editViewState;
    static juce::FlexBox createFlexBox(juce::FlexBox::JustifyContent justify);
    int getButtonSize();
    int getGapSize();

    void addButtonsToFlexBox(juce::FlexBox &box, const juce::Array<juce::Component *> &buttons, int width = 0);
    void addFlexBoxToFlexBox(juce::FlexBox &target, const juce::Array<juce::FlexBox *> &items);
    juce::DrawableButton m_stopButton, m_recordButton, m_playButton, m_loopButton, m_clickButton, m_followPlayheadButton, m_undoButton, m_redoButton;
    PreRollCounterButton m_countInButton;
    te::Edit &m_edit;
    ApplicationViewState &m_applicationState;

    juce::ApplicationCommandManager &m_commandManager;
    PositionDisplayComponent m_display;

    juce::Colour m_btn_col;
    bool m_lastCanUndo{false};
    bool m_lastCanRedo{false};
    bool m_undoRedoStateInitialised{false};

    juce::File m_loadingFile{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)
};
