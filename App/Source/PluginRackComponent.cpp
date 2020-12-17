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
                                                    &editViewState.m_selectionManager);
                }
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
    : m_editViewState(evs)
{
    m_editViewState.m_selectionManager.addChangeListener(this);
    m_pluginRackComps.clear(true);
}

LowerRangeComponent::~LowerRangeComponent()
{
    m_editViewState.m_selectionManager.removeChangeListener(this);
}

void LowerRangeComponent::changeListenerCallback(juce::ChangeBroadcaster * /*source*/)
{
    auto lastClickedTrack = m_editViewState.m_selectionManager
            .getItemsOfType<tracktion_engine::Track>()
            .getLast();
    auto lastClip = m_editViewState.m_selectionManager
            .getItemsOfType<te::MidiClip>()
            .getLast ();
    if (lastClip == nullptr && m_pointedTrack.get() != lastClickedTrack)
    {
        if (lastClickedTrack &&     !(lastClickedTrack->isArrangerTrack()
                                   || lastClickedTrack->isTempoTrack()
                                   || lastClickedTrack->isMarkerTrack()
                                   || lastClickedTrack->isChordTrack()))
        {

            m_pointedTrack = lastClickedTrack;
            m_pointedClip = nullptr;
            showPluginRack(lastClickedTrack);
            resized();
        }
    }
    else if(lastClip)
    {
        if (lastClip != m_pointedClip)
        {
            m_pointedClip = lastClip;
        }

        showPianoRoll (lastClip);
        resized();
    }
}

void LowerRangeComponent::paint(juce::Graphics &g)
{
    auto rect = getLocalBounds();
    g.setColour(juce::Colour(0xff555555));
    g.fillRect(rect);
    g.setColour(juce::Colour(0xff181818));
    auto cornerSize = 10;
    g.fillRoundedRectangle(rect.removeFromBottom(getHeight() - m_splitterHeight)
                           .toFloat(), cornerSize);
}

void LowerRangeComponent::resized()
{
        auto area = getLocalBounds();
        // todo: we add a splitter later for this
        area.removeFromTop (m_splitterHeight);

        for (auto& pluginRackComp : m_pluginRackComps)
        {
            if (pluginRackComp->isVisible())
            {
                pluginRackComp->setBounds(area);
            }
        }
        for (auto& pianoRolls : m_pianoRollComps)
        {
            if (pianoRolls->isVisible ())
            {
                pianoRolls->setBounds (area);
            }
        }

}

void LowerRangeComponent::showPluginRack(te::Track *lastClickedTrack)
{
    std::cout << "---show Plugin Rack" << std::endl;
    bool flag = false;
    //hide Piano Rolls
    for (auto& pianoRolls : m_pianoRollComps)
    {
        pianoRolls->setVisible (false);
    }
    for (auto &prc : m_pluginRackComps)
    {
        prc->setVisible(false);
        if (prc->getTrack().get() == lastClickedTrack)
        {
            prc->setVisible(true);
            flag = true;
        }
    }
    if (!flag)
    {
        PluginRackComponent * pluginRackComp =
                new PluginRackComponent(m_editViewState,lastClickedTrack);

        pluginRackComp->setAlwaysOnTop(true);
        pluginRackComp->setVisible(true);
        addAndMakeVisible(pluginRackComp);
        m_pluginRackComps.add(pluginRackComp);
    }
}

void LowerRangeComponent::showPianoRoll(te::MidiClip * midiClip)
{
    std::cout << "---show Piano Roll" << std::endl;
    //hide all PluginRacks
    for (auto &pluginrack : m_pluginRackComps)
    {
        pluginrack->setVisible (false);
    }
    m_pointedTrack = nullptr;
    bool pianoRollExists = false;
    for (auto pianoRoll : m_pianoRollComps)
    {
        pianoRoll->setVisible (false);
        if (pianoRoll->getMidiClip () == midiClip)
        {
            pianoRollExists = true;
            pianoRoll->setVisible (true);
            pianoRoll->centerView();
        }
    }
    if (!pianoRollExists)
    {
        auto pianoRollComp = new PianoRollComponent(m_editViewState, *midiClip);
        pianoRollComp->setAlwaysOnTop(true);
        pianoRollComp->setVisible(true);
        pianoRollComp->centerView ();
        addAndMakeVisible(pianoRollComp);
        m_pianoRollComps.add(pianoRollComp);

    }
}
