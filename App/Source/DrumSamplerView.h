#pragma once

#include <JuceHeader.h>
#include "tracktion_engine/tracktion_engine.h"

#include "DrumPadComponent.h"
#include "SoundEditorPanel.h"
#include "Utilities.h" 

namespace te = tracktion_engine;

class DrumSamplerView : public juce::Component
{
public:
    DrumSamplerView(te::SamplerPlugin& plugin);
    ~DrumSamplerView() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    int getNeededWidth() const;

private:
    te::Edit& m_edit;
    te::SamplerPlugin& m_plugin;

    std::unique_ptr<DrumPadComponent> m_drumPadComponent;
    std::unique_ptr<SoundEditorPanel> m_soundEditorPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSamplerView)
};
