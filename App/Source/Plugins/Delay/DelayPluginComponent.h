/*
  ==============================================================================

    DelayPluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Plugins/Delay/NextDelayPlugin.h"
#include "UI/Controls/AutomatableComboBox.h"
#include "UI/Controls/AutomatableParameter.h"
#include "UI/Controls/NonAutomatableParameter.h"
#include "Utilities/EditViewState.h"
#include <JuceHeader.h>
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
    int getNeededWidth() override { return 3; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    bool isNextDelay() const { return m_plugin->getPluginType() == NextDelayPlugin::xmlTypeName; }

    void valueTreeChanged() override {}
    void valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i) override
    {
        if (!isNextDelay() && i == te::IDs::feedback)
            m_fbParCom->updateLabel();
        if (!isNextDelay() && i == te::IDs::mix)
            m_mix->updateLabel();
    }
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}

    std::unique_ptr<AutomatableParameterComponent> m_mix, m_fbParCom, m_time, m_stereoOffset, m_pingPongAmount, m_hpCutoff, m_lpCutoff;
    std::unique_ptr<AutomatableChoiceComponent> m_mode, m_syncEnabled, m_syncDivision;
    std::unique_ptr<NonAutomatableParameterComponent> m_legacyTime;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayPluginComponent)
};
