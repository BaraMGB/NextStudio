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

    const juce::DrawableButton::ButtonStyle buttonStyle {juce::DrawableButton::ButtonStyle::ImageAboveTextLabel};

    SidebarMenu(ApplicationViewState& appstate) : MenuBar(Alignment::Left, true)
    , m_projectsButton("Projects", buttonStyle)// ImageOnButtonBackground)
    , m_instrumentsButton("Instruments", buttonStyle)
    , m_samplesButton("Samples",buttonStyle)
    , m_effectsButton("Effects",buttonStyle)
    , m_homeButton("Home",buttonStyle)
    , m_settingsButton("Settings", buttonStyle)
    , m_renderButton("Render", buttonStyle)
    , m_appState(appstate)
    {
        const auto margin = 7;

        addButton(&m_projectsButton);
        GUIHelpers::setDrawableOnButton(m_projectsButton, BinaryData::projectsButton_svg, m_appState.getProjectsColour());
        m_projectsButton.setTooltip(GUIHelpers::translate("handle projects", m_appState));
        m_projectsButton.setEdgeIndent(margin);

        addButton(&m_instrumentsButton);
        GUIHelpers::setDrawableOnButton(m_instrumentsButton, BinaryData::presetsButton_svg,m_appState.getInstrumentsColour());
        m_instrumentsButton.setTooltip(GUIHelpers::translate("instrument plugins", m_appState));
        m_instrumentsButton.setEdgeIndent(margin);

        addButton(&m_samplesButton);
        GUIHelpers::setDrawableOnButton(m_samplesButton, BinaryData::samplesButton_svg,m_appState.getSamplesColour());
        m_samplesButton.setTooltip(GUIHelpers::translate("samples", m_appState));
        m_samplesButton.setEdgeIndent(margin);

        addButton(&m_effectsButton);
        GUIHelpers::setDrawableOnButton(m_effectsButton, BinaryData::pluginsButton_svg, m_appState.getEffectsColour());
        m_effectsButton.setTooltip(GUIHelpers::translate("effect plugins", m_appState));
        m_effectsButton.setEdgeIndent(margin);

        addButton(&m_homeButton);
        GUIHelpers::setDrawableOnButton(m_homeButton, BinaryData::homeButton_svg, m_appState.getHomeColour());
        m_homeButton.setTooltip(GUIHelpers::translate("home folder file browser", m_appState));
        m_homeButton.setEdgeIndent(margin);

        addButton(&m_settingsButton);
        GUIHelpers::setDrawableOnButton(m_settingsButton, BinaryData::settingsButton_svg, m_appState.getSettingsColour());
        m_settingsButton.setTooltip(GUIHelpers::translate("opens settings", m_appState));
        m_settingsButton.setEdgeIndent(margin);

        addButton(&m_renderButton);
        GUIHelpers::setDrawableOnButton(m_renderButton, BinaryData::renderButton_svg, m_appState.getRenderColour());
        m_renderButton.setTooltip(GUIHelpers::translate("render project", m_appState));
        m_renderButton.setEdgeIndent(margin);

        setButtonGap(15);
    }

private:
    juce::DrawableButton m_projectsButton
                       , m_instrumentsButton
                       , m_samplesButton
                       , m_effectsButton
                       , m_homeButton
                       , m_settingsButton
                       , m_renderButton;
    ApplicationViewState& m_appState;
};

