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
#include "AudioMidiSettings.h"
#include "EditViewState.h"
#include "ApplicationViewState.h"
#include "Utilities.h"
#include "PreviewComponent.h"
#include "SidebarMenu.h"
#include "PluginBrowser.h"
#include "InstrumentEffectChooser.h"



class SidebarComponent : public juce::Component
                       , public juce::Button::Listener
{
public:
    SidebarComponent(ApplicationViewState& as, te::Engine& engine, juce::ApplicationCommandManager& commandManager) : m_appState(as)
        , m_engine(engine)
        , m_commandManager(commandManager)
        , m_menu(as)
        , m_settingsView(m_engine, m_commandManager)
        , m_instrumentList(engine, true)
        , m_effectList(engine, false)
    {
        addAndMakeVisible(m_menu);
        addChildComponent(m_settingsView);
        addChildComponent(m_instrumentList);
        addChildComponent(m_effectList);
        addAndMakeVisible(m_fileBrowser);
        for (auto b : m_menu.getButtons())
            b->addListener(this);
    }

    ~SidebarComponent() override
    {
        for (auto b : m_menu.getButtons())
            b->removeListener(this);
    }

    void paintOverChildren(juce::Graphics& g) override;

    void resized() override
    {
        auto area = getLocalBounds();
        m_menu.setBounds(area.removeFromLeft(80));
        if (m_settingsView.isVisible())
            m_settingsView.setBounds(area);
        else if (m_instrumentList.isVisible())
            m_instrumentList.setBounds(area);
        else if (m_effectList.isVisible())
            m_effectList.setBounds(area);
        else if (m_fileBrowser.isVisible())
            m_fileBrowser.setBounds(area);
    }

    void buttonClicked (juce::Button* button) override;

private:

    void setAllVisibleOff();

    ApplicationViewState& m_appState;
    te::Engine& m_engine;
    juce::ApplicationCommandManager& m_commandManager;
    SidebarMenu m_menu;
    SettingsView m_settingsView;
    InstrumentEffectChooser m_instrumentList;
    InstrumentEffectChooser m_effectList;
    
    juce::FileBrowserComponent m_fileBrowser{1+4+8+64, juce::File(m_appState.m_workDir), nullptr, nullptr};

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarComponent)
};
