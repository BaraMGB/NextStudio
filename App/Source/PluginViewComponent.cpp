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

