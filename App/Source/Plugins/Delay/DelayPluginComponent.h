/*
  ==============================================================================

    DelayPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Utilities/EditViewState.h"
#include "UI/Controls/AutomatableParameter.h"
#include "UI/Controls/NonAutomatableParameter.h"
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class DelayPluginComponent
    : public PluginViewComponent
    , private te::ValueTreeAllEventListener
{
public:
    DelayPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~DelayPluginComponent() override { m_plugin->state.removeListener(this); }

    void resized() override;
    int getNeededWidth() override { return 2; }

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
        if (i == te::IDs::feedback)
            m_fbParCom->updateLabel();
        if (i == te::IDs::mix)
            m_mix->updateLabel();
    }
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}

    std::unique_ptr<AutomatableParameterComponent> m_mix, m_fbParCom;
    std::unique_ptr<NonAutomatableParameterComponent> m_time;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayPluginComponent)
};
