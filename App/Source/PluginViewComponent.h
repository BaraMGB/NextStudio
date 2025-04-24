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

