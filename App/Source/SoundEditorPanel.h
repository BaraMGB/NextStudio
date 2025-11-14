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

#include "Utilities.h"

namespace te = tracktion_engine;
class SampleDisplay;

class SoundEditorPanel : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener
{
public:
    SoundEditorPanel(te::Edit& edit);
    ~SoundEditorPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSound(te::SamplerPlugin* plugin, int soundIndex);

    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;

private:
    te::Edit& m_edit;
    te::SamplerPlugin* samplerPlugin = nullptr;
    int soundIndex = -1;

    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::Slider startSlider;
    juce::Slider endSlider;
    juce::ToggleButton openEndedButton;
    juce::Label openEndedLabel;

    std::unique_ptr<SampleDisplay> m_thumbnail;
};
