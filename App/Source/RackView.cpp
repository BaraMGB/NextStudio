#include "RackView.h"
#include "PluginMenu.h"
#include "Utilities.h"


RackView::RackView (EditViewState& evs)
    : m_evs (evs)
{
    rebuildView();
}

RackView::~RackView()
{
    for (auto &b : m_addButtons)
    {
        b->removeListener(this);
    }

    if (m_track != nullptr)
        m_track->state.removeListener(this);
}

void RackView::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour(0x181818));
    g.fillRoundedRectangle(getLocalBounds().withTrimmedLeft (2).toFloat(), 10);
    g.setColour(juce::Colours::white);
        
    if (m_isOver)
        g.drawRect(getLocalBounds(), 2);
    
    if (m_track == nullptr)
        g.drawText("select a track for showing rack", getLocalBounds(), juce::Justification::centred);
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

    if (m_track != nullptr)
    {
        auto firstAdder = new AddButton(m_track);
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

            auto adder = new AddButton(m_track);
            adder->setPlugin(p->getPlugin());
            m_addButtons.add(adder);
            addAndMakeVisible(*adder);
            adder->setButtonText("+");
            adder->setBounds(area.removeFromLeft(15));
            adder->addListener(this);
        }
        area.removeFromLeft (5);
    }
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

void RackView::setTrack(te::Track::Ptr track)
{
    m_track = track;
    m_track->state.addListener(this);
    m_trackID = m_track->itemID.toString();
    rebuildView();
}

void RackView::clearTrack()
{
    if (m_track != nullptr)
        m_track->state.removeListener(this);
    m_track = nullptr;
    m_trackID = "";
    rebuildView();
}

juce::String RackView::getCurrentTrackID()
{
    return m_trackID;
}

juce::OwnedArray<AddButton> & RackView::getAddButtons()
{
    return m_addButtons;
}

juce::OwnedArray<RackItemView> & RackView::getPluginComponents()
{
    return m_rackItems;
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

void RackView::handleAsyncUpdate()
{
    if (compareAndReset (m_updatePlugins))
        rebuildView();
}

void RackView::rebuildView()
{
    m_rackItems.clear();

    if (m_track != nullptr)
    {
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
    }
    GUIHelpers::log("plugins changed");
    resized();
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

void RackView::itemDragMove(const SourceDetails& dragSourceDetails) 
{
    if (dragSourceDetails.description == "PluginComp"
        || dragSourceDetails.description == "PluginListEntry")
    {
        m_isOver = true;
    }
    repaint();
}

void RackView::itemDragExit (const SourceDetails& /*dragSourceDetails*/) 
{
    m_isOver = false;
    repaint();
}

void RackView::itemDropped(
    const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    if(dragSourceDetails.description == "PluginListEntry")
        if (auto listbox = dynamic_cast<juce::ListBox*>(
            dragSourceDetails.sourceComponent.get ()))
            if (auto lbm = dynamic_cast<PluginListBoxComponent*>(listbox->getModel()))
                EngineHelpers::insertPlugin (m_track, lbm->getSelectedPlugin());

    m_isOver = false;
    repaint();
}
