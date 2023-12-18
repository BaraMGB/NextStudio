/*
 * Copyright 2023 Steffen Baranowsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "MenuBar.h"
#include "Utilities.h"
#include "PreviewComponent.h"

class SidebarMenu : public MenuBar 
{
public:
    SidebarMenu(ApplicationViewState& appstate) : MenuBar(Alignment::Left, true)
    , m_projectsButton("Projects", juce::DrawableButton::ButtonStyle::ImageFitted)// ImageOnButtonBackground)
    , m_pluginsButton("Plugins", juce::DrawableButton::ButtonStyle::ImageFitted)
    , m_samplesButton("Samples", juce::DrawableButton::ButtonStyle::ImageFitted)
    , m_presetButton("Preset", juce::DrawableButton::ButtonStyle::ImageFitted)
    , m_homeButton("Home", juce::DrawableButton::ButtonStyle::ImageFitted)
    , m_settingsButton("Settings", juce::DrawableButton::ButtonStyle::ImageFitted)
    , m_appState(appstate)
    {
        addButton(&m_projectsButton);
        GUIHelpers::setDrawableOnButton(m_projectsButton, BinaryData::projectsButton_svg, juce::Colour(0xffffffff));
        m_projectsButton.setTooltip(GUIHelpers::translate("opens projects", m_appState));

        addButton(&m_pluginsButton);
        GUIHelpers::setDrawableOnButton(m_pluginsButton, BinaryData::pluginsButton_svg, juce::Colour(0xffffffff));
        m_pluginsButton.setTooltip(GUIHelpers::translate("opens plugins", m_appState));

        addButton(&m_samplesButton);
        GUIHelpers::setDrawableOnButton(m_samplesButton, BinaryData::samplesButton_svg, juce::Colour(0xffffffff));
        m_samplesButton.setTooltip(GUIHelpers::translate("opens samples", m_appState));

        addButton(&m_presetButton);
        GUIHelpers::setDrawableOnButton(m_presetButton, BinaryData::presetsButton_svg, juce::Colour(0xffffffff));
        m_presetButton.setTooltip(GUIHelpers::translate("opens presets", m_appState));

        addButton(&m_homeButton);
        GUIHelpers::setDrawableOnButton(m_homeButton, BinaryData::homeButton_svg, juce::Colour(0xffffffff));
        m_homeButton.setTooltip(GUIHelpers::translate("returns to home", m_appState));

        addButton(&m_settingsButton);
        GUIHelpers::setDrawableOnButton(m_settingsButton, BinaryData::settingsButton_svg, juce::Colour(0xffffffff));
        m_settingsButton.setTooltip(GUIHelpers::translate("opens settings", m_appState));

        setButtonGap(15);
    }

private:
    juce::DrawableButton m_projectsButton
                       , m_pluginsButton
                       , m_samplesButton
                       , m_presetButton
                       , m_homeButton
                       , m_settingsButton;
    ApplicationViewState& m_appState;
};

