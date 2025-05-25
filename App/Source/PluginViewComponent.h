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

// #include "PluginComponent.h"

#include "../JuceLibraryCode/JuceHeader.h"

#include "Utilities.h"

namespace te = tracktion_engine;

class PluginViewComponent : public juce::Component
{
public:
    PluginViewComponent (EditViewState&, te::Plugin::Ptr);


    [[nodiscard]] te::Plugin::Ptr getPlugin() const;

    void setPlugin(const te::Plugin::Ptr &getPlugin);
    virtual int getNeededWidth() {return 1;}

    juce::Colour getTrackColour() {return m_plugin->getOwnerTrack()->getColour();}
protected:
    te::Plugin::Ptr    m_plugin;
    EditViewState&     m_editViewState;
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginViewComponent)
};

