/*
  ==============================================================================

    LowerRangeTabBar.cpp
    Created: 7 Dec 2025 4:06:24pm
    Author:  Steffen

  ==============================================================================
*/

#include <JuceHeader.h>
#include "LowerRangeTabBar.h"

//==============================================================================
LowerRangeTabBar::LowerRangeTabBar(EditViewState& evs) : m_evs(evs)
{
    addAndMakeVisible(m_mixerButton);
    addAndMakeVisible(m_midiEditorButton);
    addAndMakeVisible(m_pluginsButton);

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
    
    m_evs.m_state.addListener(this);
    updateTabButtons();
}

LowerRangeTabBar::~LowerRangeTabBar()
{
    m_evs.m_state.removeListener(this);
}

void LowerRangeTabBar::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background
}

void LowerRangeTabBar::resized()
{
    auto area = getLocalBounds();
    auto buttonWidth = area.getWidth() / 3;
    m_mixerButton.setBounds(area.removeFromLeft(buttonWidth));
    m_midiEditorButton.setBounds(area.removeFromLeft(buttonWidth));
    m_pluginsButton.setBounds(area.removeFromLeft(buttonWidth));
}

void LowerRangeTabBar::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& i)
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
    if (auto clip = dynamic_cast<te::Clip*>(m_evs.m_selectionManager.getSelectedObject(0)))
        if (clip->isMidi())
            midiEditorEnabled = true;
    
    m_midiEditorButton.setEnabled(midiEditorEnabled);
}
