/*
  ==============================================================================

    ModifierViewComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>
#include "Utilities/EditViewState.h"
#include "UI/Controls/ParameterComponent.h"
#include "UI/Controls/AutomatableComboBox.h"
#include "UI/Controls/AutomatableParameter.h"

namespace te = tracktion_engine;

class ModifierViewComponent : public juce::Component
{
public:
    ModifierViewComponent(EditViewState &, te::Modifier::Ptr);
    ~ModifierViewComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    virtual int getNeededWidth() { return 2; }
    te::Modifier::Ptr getModifier() { return m_modifier; }

    void removeConnection(int rowIndex);

protected:
    class DragHandle
        : public juce::Component
        , public te::ParameterisableDragDropSource
    {
    public:
        DragHandle();

        void setModifier(te::Modifier::Ptr m) { m_modifier = m; }

        void paint(juce::Graphics &g) override;
        void mouseDown(const juce::MouseEvent &e) override;
        void mouseDrag(const juce::MouseEvent &e) override;
        void mouseUp(const juce::MouseEvent &e) override;

        // ParameterisableDragDropSource implementation
        void draggedOntoAutomatableParameterTargetBeforeParamSelection() override {}
        void draggedOntoAutomatableParameterTarget(const te::AutomatableParameter::Ptr &param) override;

    private:
        te::Modifier::Ptr m_modifier;
    };

    EditViewState &m_editViewState;
    te::Modifier::Ptr m_modifier;
    juce::Viewport m_viewPort;
    juce::Component m_paramListComponent;
    juce::OwnedArray<ParameterComponent> m_parameters;
    DragHandle m_dragHandle;

    struct ConnectedParametersListBoxModel : public juce::TableListBoxModel
    {
        ConnectedParametersListBoxModel(te::Modifier::Ptr m, te::Edit &e, ModifierViewComponent &parent)
            : modifier(m),
              edit(e),
              m_parent(parent)
        {
            update();
        }
        int getNumRows() override;
        void paintRowBackground(juce::Graphics &, int rowNumber, int width, int height, bool rowIsSelected) override;
        void paintCell(juce::Graphics &, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
        void cellClicked(int rowNumber, int columnId, const juce::MouseEvent &e) override;
        void update();

        te::Modifier::Ptr modifier;
        te::Edit &edit;
        ModifierViewComponent &m_parent;
        juce::ReferenceCountedArray<te::AutomatableParameter> cachedParams;
    };

    ConnectedParametersListBoxModel m_listBoxModel;
    juce::TableListBox m_table;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModifierViewComponent)
};

class LFOModifierComponent : public ModifierViewComponent
{
public:
    LFOModifierComponent(EditViewState &evs, te::Modifier::Ptr m);
    ~LFOModifierComponent() override = default;

    void resized() override;
    int getNeededWidth() override { return 4; }
    void paint(juce::Graphics &g) override;

private:
    AutomatableChoiceComponent m_wave, m_sync, m_rateType, m_bipolar;
    AutomatableParameterComponent m_rate, m_depth, m_phase, m_offset;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOModifierComponent)
};

class StepModifierComponent : public ModifierViewComponent
{
public:
    StepModifierComponent(EditViewState &evs, te::Modifier::Ptr m);
    ~StepModifierComponent() override = default;

    void resized() override;
    int getNeededWidth() override { return 6; }
    void paint(juce::Graphics &g) override;

private:
    class StepDisplay
        : public juce::Component
        , public juce::Slider::Listener
        , private juce::Timer
    {
    public:
        StepDisplay(te::StepModifier &m);
        void paint(juce::Graphics &g) override;
        void resized() override;
        void sliderValueChanged(juce::Slider *slider) override;
        void timerCallback() override;
        void updateSteps();

    private:
        te::StepModifier &m_modifier;
        juce::OwnedArray<juce::Slider> m_sliders;
        int m_currentStep{-1};
    };

    AutomatableChoiceComponent m_sync, m_rateType;
    AutomatableParameterComponent m_numSteps, m_rate, m_depth;
    StepDisplay m_stepDisplay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepModifierComponent)
};
