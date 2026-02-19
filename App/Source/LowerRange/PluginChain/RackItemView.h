/*
  ==============================================================================

    RackItemView.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include "LowerRange/PluginChain/ModifierViewComponent.h"
#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "LowerRange/PluginChain/PresetManagerComponent.h"
#include "Utilities/EditViewState.h"
#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion_engine;

class BorderlessButton : public juce::DrawableButton
{
public:
    BorderlessButton(const juce::String &buttonName, juce::DrawableButton::ButtonStyle buttonStyle)
        : juce::DrawableButton(buttonName, buttonStyle)
    {
    }

    void paint(juce::Graphics &g) override {}
};

class RackItemView
    : public juce::Component
    , public juce::Button::Listener
    , public te::ParameterisableDragDropSource
{
public:
    RackItemView(EditViewState &, te::Track::Ptr, te::Plugin::Ptr);
    RackItemView(EditViewState &, te::Track::Ptr, te::Modifier::Ptr);
    ~RackItemView() override;

    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseDoubleClick(const juce::MouseEvent &) override;

    void buttonClicked(juce::Button *button) override;

    // ParameterisableDragDropSource implementation
    void draggedOntoAutomatableParameterTargetBeforeParamSelection() override {}
    void draggedOntoAutomatableParameterTarget(const te::AutomatableParameter::Ptr &param) override;

    int getNeededWidthFactor();

    [[maybe_unused]] void setNeededWidthFactor(int wf) { m_neededWidthFactor = wf; }

    bool isCollapsed() const { return m_collapsed; }
    int getHeaderWidth() const { return m_headerWidth; }

    te::Plugin::Ptr getPlugin() { return m_plugin; }

    te::Modifier::Ptr getModifier() { return m_modifier; }

private:
    juce::Colour getTrackColour();

    juce::Label name;

    [[maybe_unused]] int m_neededWidthFactor{1};
    EditViewState &m_evs;
    te::Track::Ptr m_track;
    te::Plugin::Ptr m_plugin;
    te::Modifier::Ptr m_modifier;

    std::unique_ptr<PluginViewComponent> m_pluginComponent;
    std::unique_ptr<ModifierViewComponent> m_modifierComponent;

    std::unique_ptr<PresetManagerComponent> m_presetManager;
    BorderlessButton m_showPluginBtn;
    bool m_clickOnHeader{false};
    bool m_collapsed{false};
    int m_headerWidth{20};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackItemView)
};
