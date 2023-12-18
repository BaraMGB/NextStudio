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
#include "Utilities.h"
#include "PreviewComponent.h"
#include "SidebarMenu.h"


class SidebarComponent : public juce::Component
                       , public juce::Button::Listener
{
public:
    SidebarComponent(ApplicationViewState& as) : m_appState(as)
        , m_menu(as)
    {
        addAndMakeVisible(m_menu);
        for (auto b : m_menu.getButtons())
            b->addListener(this);
    }

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;

    void resized() override
    {
        auto area = getLocalBounds();
        m_menu.setBounds(area.removeFromLeft(80));
    }

    void buttonClicked (juce::Button* button) override;

private:
    ApplicationViewState& m_appState;
    SidebarMenu m_menu;
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidebarComponent)
};
