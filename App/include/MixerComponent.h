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

namespace te = tracktion_engine;

class MixerChannelStripComponent; // Forward declaration

class MixerComponent
    : public juce::Component
    , public te::ValueTreeAllEventListener
    , private FlaggedAsyncUpdater
{
public:
    MixerComponent(EditViewState &evs);
    ~MixerComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;

private:
    void handleAsyncUpdate() override;

    void buildChannelStrips();

    EditViewState &m_evs;

    juce::Viewport m_viewport;
    juce::Component m_channelStripContainer;
    juce::OwnedArray<MixerChannelStripComponent> m_channelStrips;

    bool m_updateChannelStrips{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
