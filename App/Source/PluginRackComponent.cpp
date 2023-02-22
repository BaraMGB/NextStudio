#include "PluginRackComponent.h"

#include <utility>

PluginRackComponent::PluginRackComponent (EditViewState& evs, te::Track::Ptr t)
    : editViewState (evs), track (std::move(t))
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
                EngineHelpers::insertPlugin (track, plugin, addButtons.indexOf (b));

            editViewState.m_selectionManager.selectOnly (track);
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
    if (m_isOver)
    {
        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds(), 2);
    }
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
            auto p = new RackWindowComponent (editViewState, plugin);
            addAndMakeVisible (p);
            plugins.add (p);
        }
    }
    resized();
}
bool PluginRackComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        return true;
    }
    return false;
}

void PluginRackComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if(dragSourceDetails.description == "PluginListEntry")
        if (auto listbox = dynamic_cast<juce::ListBox*>(
            dragSourceDetails.sourceComponent.get ()))
            if (auto lbm = dynamic_cast<PluginListBoxComponent*>(listbox->getModel()))
                EngineHelpers::insertPlugin (getTrack(), lbm->getSelectedPlugin());

    m_isOver = false;
    repaint();
}
