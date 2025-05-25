
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
#include "Utilities.h"
#include "AutomationLaneComponent.h"

namespace te = tracktion_engine;

class TrackLaneComponent : public juce::Component
{
public:
    TrackLaneComponent(EditViewState& evs, te::Track::Ptr track, juce::String timelineID);

    ~TrackLaneComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    te::Track::Ptr getTrack() const { return m_track; }

    void buildAutomationLanes();
    AutomationLaneComponent * getAutomationLane(tracktion::AutomatableParameter::Ptr ap);

private:
    EditViewState& m_editViewState;
    juce::String m_timeLineID;
    te::Track::Ptr m_track;
    juce::OwnedArray<AutomationLaneComponent> m_automationLanes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLaneComponent)
};
