#include "PluginRackComponent.h"
#include "EditComponent.h"

PluginRackComponent::PluginRackComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (t)
{
    buildPlugins();

    track->state.addListener (this);
}

PluginRackComponent::~PluginRackComponent()
{
    for (auto &b : addButtons)
    {
        b->removeListener(this);
    }
    track->state.removeListener (this);
}

void PluginRackComponent::buttonClicked(juce::Button* button)
{
    for (auto &b : addButtons)
    {
        if (b == button)
        {
            if (auto plugin = showMenuAndCreatePlugin (track->edit))
                {
                    track->pluginList.insertPlugin (plugin, addButtons.indexOf(b), 
                                                    &editViewState.selectionManager);
                }
                editViewState.selectionManager.selectOnly (track);

        }
    }
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

void PluginRackComponent::valueTreeChildOrderChanged (juce::ValueTree& c, int, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (updatePlugins);
}

void PluginRackComponent::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour(0x181818));
    g.fillRoundedRectangle(getLocalBounds().withTrimmedLeft (2).toFloat(), 10);
}

void PluginRackComponent::mouseDown (const juce::MouseEvent&)
{
    //editViewState.selectionManager.selectOnly (track.get());
}

void PluginRackComponent::resized()
{
    auto area = getLocalBounds().reduced (5);

    for (auto &b : addButtons)
    {
        b->removeListener(this);
    }
    addButtons.clear();
    auto firstAdder = new AddButton;
    addButtons.add(firstAdder);
    addAndMakeVisible(*firstAdder);
    firstAdder->addListener(this);    
    firstAdder->setButtonText("+");
    firstAdder->setBounds(area.removeFromLeft(15));

    for (auto p : plugins)
    {
        area.removeFromLeft (5);
        p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));

        area.removeFromLeft(5);

        auto adder = new AddButton;
        adder->setPlugin(p->getPlugin());
        addButtons.add(adder);
        addAndMakeVisible(*adder);
        adder->setButtonText("+");
        adder->setBounds(area.removeFromLeft(15));
        adder->addListener(this);
    }
    area.removeFromLeft (5);
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
            auto p = new PluginWindowComponent (editViewState, plugin
                                                , track->getColour ());
            addAndMakeVisible (p);
            plugins.add (p);
        }
    }
    resized();
}

//-----------------------------------------------------------------------------------------------------




LowerRangeComponent::LowerRangeComponent(EditViewState &evs)
    : editViewState(evs)
{
    editViewState.selectionManager.addChangeListener(this);
    m_pluginRackComps.clear(true);
}

LowerRangeComponent::~LowerRangeComponent()
{
    editViewState.selectionManager.removeChangeListener(this);
}

void LowerRangeComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{
    auto lastClickedTrack = editViewState.selectionManager
            .getItemsOfType<tracktion_engine::Track>()
            .getLast();
    if (m_pointedTrack.get() != lastClickedTrack)
    {
        if (lastClickedTrack &&  !(lastClickedTrack->isArrangerTrack()
                                   || lastClickedTrack->isTempoTrack()
                                   || lastClickedTrack->isMarkerTrack()
                                   || lastClickedTrack->isChordTrack()))
        {

            m_pointedTrack = lastClickedTrack;
            bool flag = false;
            for (auto &prc : m_pluginRackComps)
            {
                prc->setVisible(false);
                if (prc->getTrack().getObject() == lastClickedTrack)
                {
                    prc->setVisible(true);
                    flag = true;
                }
            }
            if (!flag)
            {
                PluginRackComponent * pluginRackComp =
                        new PluginRackComponent(editViewState,lastClickedTrack);

                pluginRackComp->setAlwaysOnTop(true);
                pluginRackComp->setVisible(true);
                addAndMakeVisible(pluginRackComp);
                m_pluginRackComps.add(pluginRackComp);
                resized();
            }
        }
    }
    else
    {
       // resized();
    }
}

void LowerRangeComponent::paint(juce::Graphics &g)
{

    auto rect = getLocalBounds();
    g.setColour(juce::Colour(0xff555555));
    g.fillRect(rect);
    g.setColour(juce::Colour(0xff181818));
    auto cornerSize = 10;
    g.fillRoundedRectangle(rect.removeFromBottom(getHeight() - 10).toFloat(), cornerSize);

}

void LowerRangeComponent::resized()
{

        auto pluginRect = getLocalBounds();
        pluginRect.removeFromTop (10);

        for (auto& prc : m_pluginRackComps)
        {
            if (prc->isVisible())
            {
                prc->setBounds(pluginRect);
            }
        }

}
