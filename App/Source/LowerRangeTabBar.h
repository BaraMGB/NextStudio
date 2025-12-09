/*
  ==============================================================================

    LowerRangeTabBar.h
    Created: 7 Dec 2025 4:06:24pm
    Author:  Steffen

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

//==============================================================================
/*
*/
class LowerRangeTabBar  : public juce::Component,
                            public juce::ValueTree::Listener
{
public:
    LowerRangeTabBar(EditViewState& evs);
    ~LowerRangeTabBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override;

    std::function<void(LowerRangeView)> onTabSelected;

private:
    void updateTabButtons();

    EditViewState& m_evs;
    juce::TextButton m_mixerButton {"Mixer"};
    juce::TextButton m_midiEditorButton {"MIDI Editor"};
    juce::TextButton m_pluginsButton {"Plugins"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowerRangeTabBar)
};
