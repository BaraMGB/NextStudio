#include "LowerRangeComponent.h"



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
        if (lastClip->getAudioTrack ())
        {

            showPianoRoll (lastClip);
            resized();
        }
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

            if (m_pianoRoll != nullptr && m_pianoRoll->isVisible ())
            {
                m_pianoRoll->setBounds (area);
            }


}

void LowerRangeComponent::showPluginRack(te::Track *lastClickedTrack)
{
    std::cout << "---show Plugin Rack" << std::endl;
    bool exists = false;
    //hide Piano Rolls

    if (m_pianoRoll)
    {
        m_pianoRoll->setVisible (false);
    }

    for (auto &prc : m_pluginRackComps)
    {
        prc->setVisible(false);
        if (prc->getTrack().get() == lastClickedTrack)
        {
            prc->setVisible(true);
            exists = true;
        }
    }
    if (!exists)
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
    if (midiClip != nullptr)
    {
        //hide all PluginRacks
        for (auto &pluginrack : m_pluginRackComps)
        {
            pluginrack->setVisible (false);
        }
        m_pointedTrack = nullptr;

        m_pianoRoll = std::make_unique<PianoRollComponent>(m_editViewState, *midiClip);
        m_pianoRoll->setAlwaysOnTop(true);
        m_pianoRoll->setVisible(true);
        m_pianoRoll->centerView ();
        addAndMakeVisible(*m_pianoRoll);
    }
}
