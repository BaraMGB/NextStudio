/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#pragma once

#include "Components/AutomatableParameter.h"
#include "Components/AutomatableSlider.h"
#include "Components/NonAutomatableParameter.h"
#include "PluginViewComponent.h"
#include "PresetManagerComponent.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class OscComponent : public juce::Component
{
public:
    OscComponent(te::FourOscPlugin::OscParams &params, juce::Colour colorToUse);
    ~OscComponent() override = default;

    void updateUI();

    void resized() override;

private:
    te::FourOscPlugin::OscParams &m_params;
    juce::Colour m_colour;
    std::unique_ptr<juce::ComboBox> m_waveShapeCombo;

    std::unique_ptr<AutomatableParameterComponent> m_tuneParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_fineTuneParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_levelParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_pulseWidthParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_detuneParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_spreadParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_panParamComp;
    std::unique_ptr<NonAutomatableParameterComponent> m_voicesParamComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscComponent)
};

//==============================================================================

class EnvelopeComponent : public juce::Component
{
public:
    EnvelopeComponent(ApplicationViewState &appState, const juce::String &name, te::Plugin &plugin, te::AutomatableParameter::Ptr attackParam, te::AutomatableParameter::Ptr decayParam, te::AutomatableParameter::Ptr sustainParam, te::AutomatableParameter::Ptr releaseParam);
    ~EnvelopeComponent() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;

private:
    ApplicationViewState &m_appstate;
    juce::Label m_name;
    te::Plugin &m_plugin;

    std::unique_ptr<AutomatableParameterComponent> m_attackParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_decayParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_sustainParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_releaseParamComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)
};

//==============================================================================
class FilterComponent : public juce::Component
{
public:
    FilterComponent(te::FourOscPlugin &plugin, ApplicationViewState &appstate);
    ~FilterComponent() override = default;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void updateUI();

private:
    te::FourOscPlugin &m_plugin;
    ApplicationViewState &m_appstate;

    std::unique_ptr<juce::ComboBox> m_typeCombo;
    std::unique_ptr<juce::ComboBox> m_slopeCombo;

    juce::Label m_name;

    std::unique_ptr<AutomatableParameterComponent> m_freqParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_resParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_amountParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_keyParamComp;
    std::unique_ptr<AutomatableParameterComponent> m_velocityParamComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterComponent)
};

//==============================================================================
class FourOscPluginComponent
    : public PluginViewComponent
    , private juce::ValueTree::Listener
{
public:
    FourOscPluginComponent(EditViewState &evs, te::Plugin::Ptr p);
    ~FourOscPluginComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    int getNeededWidth() override { return 6; }

    // PluginPresetInterface implementation
    juce::ValueTree getPluginState() override;
    juce::ValueTree getFactoryDefaultState() override;
    void restorePluginState(const juce::ValueTree &state) override;
    juce::String getPresetSubfolder() const override;
    juce::String getPluginTypeName() const override;
    ApplicationViewState &getApplicationViewState() override;

private:
    void updateOscComponentVisibility();
    int getActiveOscComponent();

    te::FourOscPlugin *m_fourOscPlugin = nullptr;

    std::unique_ptr<juce::TabbedComponent> m_tabComponent;

    std::unique_ptr<juce::ComboBox> m_voiceModeCombo;

    juce::OwnedArray<juce::TextButton> m_oscSelectButtons;
    int m_currentOscIndex = 0;

    std::unique_ptr<AutomatableSliderComponent> m_masterLevelSlider;
    std::unique_ptr<juce::Label> m_masterLevelLabel;

    juce::OwnedArray<OscComponent> m_oscComponents;

    std::unique_ptr<EnvelopeComponent> m_ampEnvComponent;
    std::unique_ptr<FilterComponent> m_filterComponent;
    std::unique_ptr<EnvelopeComponent> m_filterEnvComp;

    std::unique_ptr<juce::ToggleButton> m_distortionToggle;
    std::unique_ptr<AutomatableSliderComponent> m_distortionSlider;

    std::unique_ptr<juce::ToggleButton> m_reverbToggle;
    std::unique_ptr<AutomatableSliderComponent> m_reverbSizeSlider;
    std::unique_ptr<AutomatableSliderComponent> m_reverbMixSlider;
    std::unique_ptr<juce::Label> m_reverbSizeLabel;
    std::unique_ptr<juce::Label> m_reverbMixLabel;

    std::unique_ptr<juce::ToggleButton> m_delayToggle;
    std::unique_ptr<AutomatableSliderComponent> m_delayFeedbackSlider;
    std::unique_ptr<AutomatableSliderComponent> m_delayMixSlider;
    std::unique_ptr<juce::Label> m_delayFeedbackLabel;
    std::unique_ptr<juce::Label> m_delayMixLabel;

    std::unique_ptr<juce::ToggleButton> m_chorusToggle;
    std::unique_ptr<AutomatableSliderComponent> m_chorusDepthSlider;
    std::unique_ptr<AutomatableSliderComponent> m_chorusMixSlider;
    std::unique_ptr<juce::Label> m_chorusDepthLabel;
    std::unique_ptr<juce::Label> m_chorusMixLabel;

    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}

    juce::Array<juce::Rectangle<int>> m_rectsToPaint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FourOscPluginComponent)
};
