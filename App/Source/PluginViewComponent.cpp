#include "PluginViewComponent.h"

PluginViewComponent::PluginViewComponent
    (EditViewState& evs, te::Plugin::Ptr p)
    : m_editViewState (evs), m_plugin (p)
{
}

te::Plugin::Ptr PluginViewComponent::getPlugin() const
{
    return m_plugin;
}

void PluginViewComponent::setPlugin(const te::Plugin::Ptr &plugin)
{
    m_plugin = plugin;
}

