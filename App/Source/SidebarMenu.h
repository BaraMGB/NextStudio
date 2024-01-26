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
    const juce::DrawableButton::ButtonStyle bs {juce::DrawableButton::ButtonStyle::ImageAboveTextLabel};
    SidebarMenu(ApplicationViewState& appstate) : MenuBar(Alignment::Left, true)
    , m_projectsButton("Projects", bs)// ImageOnButtonBackground)
    , m_instrumentsButton("Instruments", bs)
    , m_samplesButton("Samples",bs)
    , m_effectsButton("Effects",bs)
    , m_homeButton("Home",bs)
    , m_settingsButton("Settings", bs)
    , m_appState(appstate)
    {
        addButton(&m_projectsButton);
        GUIHelpers::setDrawableOnButton(m_projectsButton, BinaryData::projectsButton_svg, m_appState.getProjectsColour());
        m_projectsButton.setTooltip(GUIHelpers::translate("handle projects", m_appState));

        addButton(&m_instrumentsButton);
        GUIHelpers::setDrawableOnButton(m_instrumentsButton, BinaryData::presetsButton_svg,m_appState.getInstrumentsColour());
        m_instrumentsButton.setTooltip(GUIHelpers::translate("instrument plugins", m_appState));

        addButton(&m_samplesButton);
        GUIHelpers::setDrawableOnButton(m_samplesButton, BinaryData::samplesButton_svg,m_appState.getSamplesColour());
        m_samplesButton.setTooltip(GUIHelpers::translate("samples", m_appState));

        addButton(&m_effectsButton);
        GUIHelpers::setDrawableOnButton(m_effectsButton, BinaryData::pluginsButton_svg, m_appState.getEffectsColour());
        m_effectsButton.setTooltip(GUIHelpers::translate("effect plugins", m_appState));

        addButton(&m_homeButton);
        GUIHelpers::setDrawableOnButton(m_homeButton, BinaryData::homeButton_svg, m_appState.getHomeColour());
        m_homeButton.setTooltip(GUIHelpers::translate("home folder file browser", m_appState));

        addButton(&m_settingsButton);
        GUIHelpers::setDrawableOnButton(m_settingsButton, BinaryData::settingsButton_svg, m_appState.getSettingsColour());
        m_settingsButton.setTooltip(GUIHelpers::translate("opens settings", m_appState));

        setButtonGap(15);
    }

private:
    juce::DrawableButton m_projectsButton
                       , m_instrumentsButton
                       , m_samplesButton
                       , m_effectsButton
                       , m_homeButton
                       , m_settingsButton;
    ApplicationViewState& m_appState;
};

