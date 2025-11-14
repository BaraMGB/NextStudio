#pragma once

#include <JuceHeader.h>

#include "DrumPadComponent.h"
#include "EditViewState.h"
#include "PluginViewComponent.h"
#include "PluginPresetInterface.h"
#include "SoundEditorPanel.h"
#include "Utilities.h" 

namespace te = tracktion_engine;
class DrumSamplerView : public PluginViewComponent
{
public:
    DrumSamplerView(EditViewState& evs, te::SamplerPlugin& sampler);
    ~DrumSamplerView() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    int getNeededWidth() override;

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree& state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState& getApplicationViewState() override;
private:

    te::Edit& m_edit;
    te::SamplerPlugin& m_sampler;

    DrumPadComponent m_drumPadComponent;
    SoundEditorPanel m_soundEditorPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSamplerView)
};
