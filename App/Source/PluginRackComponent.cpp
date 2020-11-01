#include "PluginRackComponent.h"

PluginRackComponent::PluginRackComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (t)
{
    addAndMakeVisible (addButton);

    buildPlugins();

    track->state.addListener (this);

    addButton.onClick = [this]
    {
        if (auto plugin = showMenuAndCreatePlugin (track->edit))
            track->pluginList.insertPlugin (plugin, track->pluginList.size() - 2, &editViewState.selectionManager);
        editViewState.selectionManager.selectOnly (track);
    };
}

PluginRackComponent::~PluginRackComponent()
{
    track->state.removeListener (this);
}

void PluginRackComponent::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void PluginRackComponent::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void PluginRackComponent::valueTreeChildOrderChanged (juce::ValueTree&, int, int)
{
    markAndUpdate (updatePlugins);
}

void PluginRackComponent::paint (Graphics& g)
{
    g.setColour (Colour(0x181818));
    g.fillRect (getLocalBounds().withTrimmedLeft (2));
}

void PluginRackComponent::mouseDown (const MouseEvent&)
{
    //editViewState.selectionManager.selectOnly (track.get());
}

void PluginRackComponent::resized()
{
    auto area = getLocalBounds().reduced (5);

    for (auto p : plugins)
    {
        area.removeFromLeft (5);
        p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));
    }
    area.removeFromLeft (5);
    addButton.setBounds (area.removeFromLeft(15));
}

void PluginRackComponent::handleAsyncUpdate()
{
    if (compareAndReset (updatePlugins))
        buildPlugins();
}

void PluginRackComponent::buildPlugins()
{
    
    plugins.clear();
    for (auto plugin : track->pluginList)
    {
        //don't show the default volume and levelmeter plugin
        if (track->pluginList.indexOf(plugin)  < track->pluginList.size() - 2 )
        {
            auto p = new PluginComponent (editViewState, plugin);
            addAndMakeVisible (p);
            plugins.add (p);
        }
    }
    resized();
}

//-----------------------------------------------------------------------------------------------------




LowerRangeComponent::LowerRangeComponent(EditViewState &evs) : editViewState(evs)
{
    editViewState.selectionManager.addChangeListener(this);

}

LowerRangeComponent::~LowerRangeComponent()
{
    editViewState.selectionManager.removeChangeListener(this);
}

void LowerRangeComponent::changeListenerCallback(ChangeBroadcaster *)
{
    auto lastClickedTrack = editViewState.selectionManager.getItemsOfType<tracktion_engine::Track>().getLast();
    if (lastClickedTrack &&  !(lastClickedTrack->isArrangerTrack()
                               || lastClickedTrack->isTempoTrack()
                               || lastClickedTrack->isMarkerTrack()
                               || lastClickedTrack->isChordTrack()))
    {
        m_pointedTrack = lastClickedTrack;
        m_pluginRackComp.clear();
        auto tfc = new PluginRackComponent(editViewState, m_pointedTrack);
        m_pluginRackComp.add(tfc);
        addAndMakeVisible(tfc);
        tfc->setAlwaysOnTop(true);
        resized();
    }
    else
    {
        m_pluginRackComp.clear();
        resized();
    }
}

void LowerRangeComponent::paint(Graphics &g)
{

    auto rect = getLocalBounds();
    g.setColour(Colour(0xff181818));
    g.fillRect(rect);
    g.setColour(Colour(0xff555555));
    g.fillRect(rect.removeFromTop (10));

}

void LowerRangeComponent::resized()
{
    if (m_pluginRackComp.getFirst())
    {
        auto pluginRect = getLocalBounds();
        pluginRect.removeFromTop (10);
        m_pluginRackComp.getFirst()->setBounds(pluginRect);
    }
}
