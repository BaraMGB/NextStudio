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

    void setFile(const juce::File& file);


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

