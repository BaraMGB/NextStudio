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

#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "Utilities/EditViewState.h"

PluginViewComponent::PluginViewComponent(EditViewState &evs, te::Plugin::Ptr p)
    : m_editViewState(evs),
      m_plugin(p)
{
}

te::Plugin::Ptr PluginViewComponent::getPlugin() const { return m_plugin; }

void PluginViewComponent::setPlugin(const te::Plugin::Ptr &plugin) { m_plugin = plugin; }

bool PluginViewComponent::getInitialPresetLoaded()
{
    auto state = m_editViewState.getPresetManagerUIStateForPlugin(*m_plugin);
    return state.getProperty("initialPresetLoaded", false);
}

void PluginViewComponent::setInitialPresetLoaded(bool loaded)
{
    auto state = m_editViewState.getPresetManagerUIStateForPlugin(*m_plugin);
    state.setProperty("initialPresetLoaded", loaded, nullptr);
}

juce::String PluginViewComponent::getLastLoadedPresetName()
{
    auto state = m_editViewState.getPresetManagerUIStateForPlugin(*m_plugin);
    return state.getProperty("lastLoadedPreset", juce::String());
}

void PluginViewComponent::setLastLoadedPresetName(const juce::String &name)
{
    auto state = m_editViewState.getPresetManagerUIStateForPlugin(*m_plugin);
    state.setProperty("lastLoadedPreset", name, nullptr);
}
