
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

#include "EditViewState.h"
#include "LowerRangeTabBar.h"
#include "MixerComponent.h"
#include "PianoRollEditor.h"
#include "PresetManagerComponent.h"
#include "RackView.h"
#include "SplitterComponent.h"
#include "TrackPresetAdapter.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class LowerRangeComponent
    : public juce::Component
    , public te::ValueTreeAllEventListener
{
public:
    explicit LowerRangeComponent(EditViewState &evs);
    ~LowerRangeComponent() override;

    void paint(juce::Graphics &g) override;
    void paintOverChildren(juce::Graphics &g) override;
    void resized() override;

    PianoRollEditor &getPianoRollEditor() { return m_pianoRollEditor; }

private:
    void updateView();
    void updatePresetManager(te::Track *track);

    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

    void handleSplitterMouseDown();
    void handleSplitterDrag(int dragDistance);

    EditViewState &m_evs;

    RackView m_rackView;
    PianoRollEditor m_pianoRollEditor;
    MixerComponent m_mixer;
    LowerRangeTabBar m_tabBar;
    SplitterComponent m_splitter;

    std::unique_ptr<TrackPresetAdapter> m_presetAdapter;
    std::unique_ptr<PresetManagerComponent> m_presetManager;

    const float m_splitterHeight{10.f};

    int m_pianorollHeightAtMousedown{};
    double m_cachedPianoNoteNum{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowerRangeComponent)
};
