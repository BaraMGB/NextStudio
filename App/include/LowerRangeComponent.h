
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

#include "LowerRangeTabBar.h"
#include "MixerComponent.h"
#include "PianoRollEditor.h"
#include "PluginChainView.h"
#include "SplitterCollapseController.h"
#include "SplitterComponent.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class LowerRangeCollapsedButton;

class LowerRangeComponent
    : public juce::Component
    , public te::ValueTreeAllEventListener
    , private juce::ChangeListener
{
public:
    explicit LowerRangeComponent(EditViewState &evs);
    ~LowerRangeComponent() override;

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

    PianoRollEditor &getPianoRollEditor() { return m_pianoRollEditor; }

    static constexpr int collapsedHeight = 38;
    static constexpr int defaultExpandedHeight = 350;

private:
    void updateView();
    void updateCollapsedButtons();
    void syncActiveTrack(bool forceRefresh);
    te::Track::Ptr getTrackMarkedForLowerRange() const;
    te::Track::Ptr getSelectedTrackForLowerRange() const;

    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;
    void changeListenerCallback(juce::ChangeBroadcaster *source) override;

    void handleSplitterMouseDown();
    void handleSplitterDrag(int dragDistance);

    EditViewState &m_evs;

    PluginChainView m_pluginChainView;
    PianoRollEditor m_pianoRollEditor;
    MixerComponent m_mixer;
    LowerRangeTabBar m_tabBar;
    SplitterComponent m_splitter;
    std::unique_ptr<LowerRangeCollapsedButton> m_collapsedMixerButton;
    std::unique_ptr<LowerRangeCollapsedButton> m_collapsedMidiEditorButton;
    std::unique_ptr<LowerRangeCollapsedButton> m_collapsedPluginChainButton;

    static constexpr int splitterHeight = 10;
    static constexpr int collapsedBarHeight = collapsedHeight - splitterHeight;
    static constexpr int menuBarWidth = 100;

    SplitterCollapseController m_splitterCollapseController;
    int m_pianorollHeightAtMousedown{};
    double m_cachedPianoNoteNum{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowerRangeComponent)
};
