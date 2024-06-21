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
#include "SampleBrowser.h"
#include "FileBrowser.h"
#include "ProjectsBrowser.h"
#include "Utilities.h"
#include "PreviewComponent.h"
#include "SidebarMenu.h"
#include "PluginBrowser.h"
#include "InstrumentEffectChooser.h"

class SidebarComponent : public juce::Component
                       , public juce::Button::Listener
                    
{
public:
    SidebarComponent(EditViewState& evs, juce::ApplicationCommandManager& commandManager);
    ~SidebarComponent() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;

    void resized() override; 
    void buttonClicked (juce::Button* button) override;

    void updateParentsListener();

private:
    void setAllVisibleOff();
    int m_cachedSidebarWidth {0};
    juce::String m_lastClickedButton;
    EditViewState& m_evs;
    ApplicationViewState& m_appState;
    te::Engine& m_engine;
    te::Edit& m_edit;
    juce::ApplicationCommandManager& m_commandManager;
    SidebarMenu m_menu;
    SettingsView m_settingsView;
    InstrumentEffectChooser m_instrumentList;
    InstrumentEffectChooser m_effectList;
    std::unique_ptr<juce::Component> m_renderComponent;

    FileBrowserComponent           m_fileListBrowser;

    SamplePreviewComponent m_samplePreview;
    SampleBrowserComponent m_sampleBrowser;
    ProjectsBrowserComponent m_projectsBrowser;
    const int CONTENT_HEADER_HEIGHT {30};
    juce::String m_headerName;
    juce::Colour m_headerColour;

JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarComponent)
};
