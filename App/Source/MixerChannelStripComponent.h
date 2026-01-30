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
#include "Components/AutomatableParameter.h"
#include "Components/AutomatableSlider.h"
#include "EditViewState.h"
#include "LevelMeterComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace te = tracktion_engine;

class MixerChannelStripComponent
    : public juce::Component
    , public juce::ValueTree::Listener
{
public:
    MixerChannelStripComponent(EditViewState &evs, te::AudioTrack::Ptr at);
    ~MixerChannelStripComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void updateComponentsFromTrack();

    // ValueTree::Listener overrides
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override {}
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override;
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override;
    void valueTreeParentChanged(juce::ValueTree &) override {}

private:
    EditViewState &m_evs;
    te::AudioTrack::Ptr m_track;

    juce::Label m_trackName;
    AutomatableSliderComponent m_volumeSlider;
    AutomatableSliderComponent m_panSlider;
    std::unique_ptr<LevelMeterComponent> m_levelMeterLeft;
    std::unique_ptr<LevelMeterComponent> m_levelMeterRight;
    juce::TextButton m_muteButton, m_soloButton, m_armButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelStripComponent)
};
