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

#include "LowerRangeTabBar.h"
#include "NextLookAndFeel.h"

LowerRangeTabBar::LowerRangeTabBar(EditViewState &evs)
    : MenuBar(Alignment::Bottom, true),
      m_evs(evs),
      m_mixerButton("Mixer", juce::DrawableButton::ButtonStyle::ImageAboveTextLabel),
      m_midiEditorButton("MIDI Editor", juce::DrawableButton::ButtonStyle::ImageAboveTextLabel),
      m_pluginsButton("Plugins", juce::DrawableButton::ButtonStyle::ImageAboveTextLabel)
{
    const auto margin = 7;

    auto color = m_evs.m_applicationState.getButtonTextColour();
    GUIHelpers::setDrawableOnButton(m_mixerButton, BinaryData::headphonessettings_svg, color);
    GUIHelpers::setDrawableOnButton(m_midiEditorButton, BinaryData::piano_svg, color);
    GUIHelpers::setDrawableOnButton(m_pluginsButton, BinaryData::powerplug_svg, color);

    addButton(&m_mixerButton, 1);
    m_mixerButton.setEdgeIndent(margin);

    addButton(&m_midiEditorButton, 1);
    m_midiEditorButton.setEdgeIndent(margin);

    addButton(&m_pluginsButton, 1);
    m_pluginsButton.setEdgeIndent(margin);

    m_mixerButton.onClick = [this]
    {
        if (onTabSelected)
            onTabSelected(LowerRangeView::mixer);
    };

    m_midiEditorButton.onClick = [this]
    {
        if (onTabSelected)
            onTabSelected(LowerRangeView::midiEditor);
    };

    m_pluginsButton.onClick = [this]
    {
        if (onTabSelected)
            onTabSelected(LowerRangeView::pluginRack);
    };

    setButtonGap(15);

    m_evs.m_state.addListener(this);
    updateTabButtons();
}

LowerRangeTabBar::~LowerRangeTabBar() { m_evs.m_state.removeListener(this); }

void LowerRangeTabBar::valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &i)
{
    if (i == IDs::lowerRangeView)
        updateTabButtons();
}

void LowerRangeTabBar::updateTabButtons()
{
    auto currentView = m_evs.getLowerRangeView();
    m_mixerButton.setToggleState(currentView == LowerRangeView::mixer, juce::dontSendNotification);
    m_midiEditorButton.setToggleState(currentView == LowerRangeView::midiEditor, juce::dontSendNotification);
    m_pluginsButton.setToggleState(currentView == LowerRangeView::pluginRack, juce::dontSendNotification);

    // The MIDI editor should only be enabled if a MIDI clip is selected
    bool midiEditorEnabled = false;
    if (auto clip = dynamic_cast<te::Clip *>(m_evs.m_selectionManager.getSelectedObject(0)))
        if (clip->isMidi())
            midiEditorEnabled = true;

    m_midiEditorButton.setEnabled(midiEditorEnabled);
}
