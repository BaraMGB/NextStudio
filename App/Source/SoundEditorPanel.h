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

#include "SampleDisplay.h"
#include "AutomatableSliderComponent.h"

namespace te = tracktion_engine;
class ApplicationViewState;

class SoundEditorPanel : public juce::Component, public juce::Button::Listener, public juce::Value::Listener
{
public:
    SoundEditorPanel(tracktion::SamplerPlugin& plugin, te::Edit& edit, ApplicationViewState& appViewState);
    ~SoundEditorPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSound(int soundIndex);

    void buttonClicked(juce::Button* button) override;
    void valueChanged(juce::Value& value) override;


private:

    te::Edit& m_edit;
    ApplicationViewState& m_appViewState;
    te::SamplerPlugin& m_samplerPlugin;
    int soundIndex = -1;
    bool m_markersNeedUpdate = false;

    juce::Value gainValue;
    juce::Value panValue;
    std::unique_ptr<NonAutomatableParameterComponent> gainSlider;
    std::unique_ptr<NonAutomatableParameterComponent> panSlider;
    juce::ToggleButton openEndedButton;
    juce::Label openEndedLabel;

    std::unique_ptr<te::AudioFile> m_audioFile;
    std::unique_ptr<SampleDisplay> m_thumbnail;
};
