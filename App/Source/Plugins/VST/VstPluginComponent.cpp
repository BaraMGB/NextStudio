/*
  ==============================================================================

    VstPluginComponent.cpp
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "Plugins/VST/VstPluginComponent.h"
#include "LowerRange/PluginChain/PresetHelpers.h"

VstPluginComponent::VstPluginComponent(EditViewState &evs, te::Plugin::Ptr p)
    : PluginViewComponent(evs, p),
      m_lastChangedParameterComponent(nullptr),
      m_plugin(p)
{
    if (p)
    {
        for (auto &param : p->getAutomatableParameters())
        {
            if (param)
            {
                param->addListener(this);
                auto parameterComp = std::make_unique<ParameterComponent>(*param, m_editViewState.m_applicationState);
                m_pluginListComponent.addAndMakeVisible(parameterComp.get());
                m_parameters.add(std::move(parameterComp));
            }
        }
    }

    addAndMakeVisible(m_viewPort);

    if (p->getAutomatableParameter(0))
    {
        m_lastChangedParameterComponent = std::make_unique<ParameterComponent>(*(p->getAutomatableParameter(0)), m_editViewState.m_applicationState);
        addAndMakeVisible(*m_lastChangedParameterComponent);
    }
    m_viewPort.setViewedComponent(&m_pluginListComponent, true);
    m_viewPort.setScrollBarsShown(true, false, true, false);
}

VstPluginComponent::~VstPluginComponent()
{
    m_lastChangedParameterComponent.reset();
    if (m_plugin)
    {
        for (auto &param : m_plugin->getAutomatableParameters())
        {
            if (param)
            {
                param->removeListener(this);
            }
        }
    }
}

void VstPluginComponent::paint(juce::Graphics &g)
{
    g.setColour(m_editViewState.m_applicationState.getBackgroundColour2());
    g.fillAll();
    if (m_lastChangedParameterComponent)
    {
        auto h = m_lastChangedParameterComponent->getHeight();
        g.setColour(juce::Colours::black);
        g.drawLine(0, h, getWidth(), h);
    }
}

void VstPluginComponent::resized()
{
    int scrollPos = m_viewPort.getVerticalScrollBar().getCurrentRangeStart();
    auto area = getLocalBounds();
    const auto widgetHeight = 30;
    if (m_lastChangedParameterComponent)
    {
        m_lastChangedParameterComponent->setBounds(area.removeFromTop(widgetHeight));
    }
    m_viewPort.setBounds(area);
    m_pluginListComponent.setBounds(area.getX(), area.getY(), area.getWidth(), m_pluginListComponent.getChildren().size() * widgetHeight);

    auto pcb = m_pluginListComponent.getBounds();
    for (auto &pc : m_parameters)
    {
        pc->setBounds(pcb.removeFromTop(widgetHeight));
    }
    m_viewPort.getVerticalScrollBar().setCurrentRangeStart(scrollPos);
}

void VstPluginComponent::parameterChanged(te::AutomatableParameter &param, float)
{
    if (!m_lastChangedParameterComponent->isDragged() && &m_lastChangedParameterComponent->getParameter() != &param)
    {
        removeChildComponent(m_lastChangedParameterComponent.get());
        m_lastChangedParameterComponent = std::make_unique<ParameterComponent>(param, m_editViewState.m_applicationState);
        addAndMakeVisible(m_lastChangedParameterComponent.get());
        resized();
    }
}

ApplicationViewState &VstPluginComponent::getApplicationViewState() { return m_editViewState.m_applicationState; }

juce::ValueTree VstPluginComponent::getPluginState()
{
    auto state = m_plugin->state.createCopy();
    state.setProperty("type", getPluginTypeName(), nullptr);
    return state;
}

juce::ValueTree VstPluginComponent::getFactoryDefaultState()
{
    if (auto *ep = dynamic_cast<te::ExternalPlugin *>(m_plugin.get()))
    {
        juce::ValueTree state("PLUGIN");
        state.setProperty("type", ep->desc.pluginFormatName + juce::String::toHexString(ep->desc.deprecatedUid).toUpperCase(), nullptr);
        return state;
    }

    return {};
}

void VstPluginComponent::restorePluginState(const juce::ValueTree &state) { m_plugin->restorePluginStateFromValueTree(state); }

juce::String VstPluginComponent::getPresetSubfolder() const { return PresetHelpers::getPluginPresetFolder(*m_plugin); }

juce::String VstPluginComponent::getPluginTypeName() const
{
    if (auto *ep = dynamic_cast<te::ExternalPlugin *>(m_plugin.get()))
        return ep->desc.pluginFormatName + juce::String::toHexString(ep->desc.deprecatedUid).toUpperCase();

    return "unknown";
}
