#pragma once

#include <JuceHeader.h>
#include "tracktion_engine/tracktion_engine.h"

#include "DrumPadComponent.h"
#include "EditViewState.h"
#include "PluginViewComponent.h"
#include "PluginPresetInterface.h"
#include "SoundEditorPanel.h"
#include "Utilities.h" 

namespace te = tracktion_engine;
class DrumSamplerView : public PluginViewComponent,
                        public PluginPresetInterface

{
public:
    DrumSamplerView(EditViewState& evs, te::Plugin::Ptr p);
    ~DrumSamplerView() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    int getNeededWidth() override;

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    void restorePluginState(const juce::ValueTree& state) override;
    juce::String getPresetSubfolder() override;
    juce::String getPluginTypeName() override;
    ApplicationViewState& getApplicationViewState() override;
private:
    te::Edit& m_edit;
    te::Plugin::Ptr m_plugin;

    std::unique_ptr<DrumPadComponent> m_drumPadComponent;
    std::unique_ptr<SoundEditorPanel> m_soundEditorPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSamplerView)
};
