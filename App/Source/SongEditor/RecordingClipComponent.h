
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
#include "SongEditor/TimeLineComponent.h"
#include "Utilities/EditViewState.h"
#include "Utilities/Utilities.h"

namespace te = tracktion_engine;

class RecordingClipComponent
    : public juce::Component
    , private juce::Timer
{
public:
    RecordingClipComponent(te::Track::Ptr t, EditViewState &, TimeLineComponent &timeLine);

    void paint(juce::Graphics &g) override;
    te::Track::Ptr getTrack() { return m_track; }

private:
    void timerCallback() override;
    void updatePosition();
    void initialiseThumbnailAndPunchTime();
    void drawThumbnail(juce::Graphics &g, juce::Colour waveformColour) const;
    bool getBoundsAndTime(juce::Rectangle<int> &bounds, tracktion::TimeRange &times) const;

    int m_ClipHeaderHeight{10};

    te::Track::Ptr m_track;
    EditViewState &m_editViewState;
    TimeLineComponent &m_timeLine;

    te::RecordingThumbnailManager::Thumbnail::Ptr m_thumbnail;

    tracktion::TimePosition m_punchInTime = tracktion::TimePosition::fromSeconds(-1.0);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingClipComponent)
};
