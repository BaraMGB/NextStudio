
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
#include "Utilities.h"


class SamplePreviewComponent : public juce::Component
                             , public juce::Slider::Listener
                             , public juce::Timer
{
public:

    explicit SamplePreviewComponent(te::Edit & edit, ApplicationViewState& avs);
    void paint(juce::Graphics &g) override;
    void resized() override;
    void sliderValueChanged(juce::Slider *slider) override;
    void timerCallback() override;

    void play();
    void stop();
    void rewind();

    bool setFile(const juce::File& file);


private:

    void updateButtonColours();

    te::Edit &    m_currentEdit;
    ApplicationViewState & m_avs;
    std::unique_ptr<te::Edit>     m_previewEdit;
    std::unique_ptr<juce::Slider> m_volumeSlider;
    juce::DrawableButton m_playBtn, m_stopBtn, m_syncTempoBtn;
    juce::Label m_fileName;
    std::unique_ptr<SampleView>    m_thumbnail;
    bool m_syncTempo {false};
    juce::File m_file;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePreviewComponent)
};

