/*
  ==============================================================================

    VolumePluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "UI/Controls/AutomatableParameter.h"
#include "Utilities/EditViewState.h"
#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class VolumePluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    VolumePluginComponent(EditViewState &, te::Plugin::Ptr);
    ~VolumePluginComponent() override { m_plugin->state.removeListener(this); }

    void paint(juce::Graphics &) override;
    void resized() override;
    int getNeededWidth() override { return 1; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override
    {
        if (i == te::IDs::pan)
            m_panParComp->updateLabel();
        if (i == te::IDs::volume)
            m_volParComp->updateLabel();
    }

    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}

    std::unique_ptr<AutomatableParameterComponent> m_volParComp, m_panParComp;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VolumePluginComponent)
};
