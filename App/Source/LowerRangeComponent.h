
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
#include "Utilities.h"
#include "PluginComponent.h"
#include "PluginMenu.h"
#include "PianoRollEditor.h"
#include "RackView.h"
#include "TrackHeadComponent.h"
#include "TimelineOverlayComponent.h"
#include "SplitterComponent.h"

namespace te = tracktion_engine;

class LowerRangeComponent : public juce::Component
                          , public te::ValueTreeAllEventListener
{
public:
    explicit LowerRangeComponent (EditViewState& evs);
     ~LowerRangeComponent() override;

    void paint (juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized () override;

    PianoRollEditor& getPianoRollEditor() {return m_pianoRollEditor;}

private:

    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;
    void valueTreeChildAdded (juce::ValueTree&
                              , juce::ValueTree&) override;
    void valueTreeChildRemoved (juce::ValueTree&
                                , juce::ValueTree&
                                , int) override;
    void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override;

    void handleSplitterMouseDown();
    void handleSplitterDrag(int dragDistance);

    EditViewState& m_evs;

    RackView m_rackView;
    PianoRollEditor m_pianoRollEditor;
    SplitterComponent m_splitter;
    const float m_splitterHeight {10.f};

    int m_pianorollHeightAtMousedown{};
    double m_cachedPianoNoteNum{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowerRangeComponent)
};
