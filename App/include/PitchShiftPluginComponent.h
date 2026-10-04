#pragma once

#include "AutomatableParameter.h"
#include "EffectEditorLayout.h"
#include "PluginViewComponent.h"

class PitchShiftPluginComponent : public PluginViewComponent
{
public:
    PitchShiftPluginComponent(EditViewState &, te::Plugin::Ptr);
    ~PitchShiftPluginComponent() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    int getNeededWidth() override { return EffectEditorLayout::pitchShifterWidthFactor; }

    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    class PitchMapComponent;
    std::unique_ptr<PitchMapComponent> m_graph;
    std::unique_ptr<AutomatableParameterComponent> m_semitones;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchShiftPluginComponent)
};
