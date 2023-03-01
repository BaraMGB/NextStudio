#include "PluginRackComponent.h"


RackView::RackView (EditViewState& evs, te::Track::Ptr t)
    : m_evs (evs), m_track (std::move(t))
{
    rebuildView();

    m_track->state.addListener (this);
}

RackView::~RackView()
{
    for (auto &b : m_addButtons)
    {
        b->removeListener(this);
    }

    m_track->state.removeListener (this);
}

void RackView::buttonClicked(juce::Button* button)
{
    for (auto &b : m_addButtons)
    {
        if (b == button)
        {
            if (auto plugin = showMenuAndCreatePlugin (m_track->edit))
                EngineHelpers::insertPlugin (m_track, plugin, m_addButtons.indexOf (b));

            m_evs.m_selectionManager.selectOnly (m_track);
        }
    }
}

void RackView::valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& c)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (m_updatePlugins);
}

void RackView::valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& c, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (m_updatePlugins);
}

void RackView::valueTreeChildOrderChanged (juce::ValueTree& c, int, int)
{
    if (c.hasType (te::IDs::PLUGIN))
        markAndUpdate (m_updatePlugins);
}

void RackView::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour(0x181818));
    g.fillRoundedRectangle(getLocalBounds().withTrimmedLeft (2).toFloat(), 10);
    if (m_isOver)
    {
        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds(), 2);
    }
}

void RackView::mouseDown (const juce::MouseEvent&)
{
    //editViewState.selectionManager.selectOnly (track.get());
}

void RackView::resized()
{
    auto area = getLocalBounds().reduced (5);

    for (auto &b : m_addButtons)
    {
        b->removeListener(this);
    }
    m_addButtons.clear();
    auto firstAdder = new AddButton;
    m_addButtons.add(firstAdder);
    addAndMakeVisible(*firstAdder);
    firstAdder->addListener(this);
    firstAdder->setButtonText("+");
    firstAdder->setBounds(area.removeFromLeft(15));

    for (auto p : m_rackItems)
    {
        area.removeFromLeft (5);
        p->setBounds (area.removeFromLeft((area.getHeight() * p->getNeededWidthFactor()) / 2 ));

        area.removeFromLeft(5);

        auto adder = new AddButton;
        adder->setPlugin(p->getPlugin());
        m_addButtons.add(adder);
        addAndMakeVisible(*adder);
        adder->setButtonText("+");
        adder->setBounds(area.removeFromLeft(15));
        adder->addListener(this);
    }
    area.removeFromLeft (5);
}

void RackView::handleAsyncUpdate()
{
    if (compareAndReset (m_updatePlugins))
        rebuildView();
}

void RackView::rebuildView()
{
    m_rackItems.clear();

    for (auto plugin : m_track->pluginList)
    {
        //don't show the default volume and levelmeter plugin
        if (m_track->pluginList.indexOf(plugin)  < m_track->pluginList.size() - 2 )
        {
            auto p = new RackItemView (m_evs, plugin);
            addAndMakeVisible (p);
            m_rackItems.add (p);
        }
    }
    resized();
}

bool RackView::isIdValid()
{
    return te::findTrackForID(m_evs.m_edit, m_id) != nullptr;
}

bool RackView::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if (dragSourceDetails.description == "PluginListEntry")
    {
        return true;
    }
    return false;
}

void RackView::itemDropped(
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
