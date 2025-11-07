
#pragma once

#include "Utilities.h"

namespace te = tracktion_engine;
class SampleDisplay;

class SoundEditorPanel : public juce::Component, public juce::Slider::Listener
{
public:
    SoundEditorPanel(te::Edit& edit);
    ~SoundEditorPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setSound(te::SamplerPlugin* plugin, int soundIndex);

    void sliderValueChanged(juce::Slider* slider) override;

private:
    te::Edit& m_edit;
    te::SamplerPlugin* samplerPlugin = nullptr;
    int soundIndex = -1;

    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::Slider startSlider;
    juce::Slider endSlider;

    std::unique_ptr<SampleDisplay> m_thumbnail;
};
