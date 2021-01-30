#include "LowerRangeComponent.h"



LowerRangeComponent::LowerRangeComponent(EditViewState &evs)
    : m_editViewState(evs)
{
    //m_editViewState.m_selectionManager.addChangeListener(this);
    m_pluginRackComps.clear(true);

}

LowerRangeComponent::~LowerRangeComponent()
{
    m_editViewState.m_selectionManager.removeChangeListener(this);
}

void LowerRangeComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{

    if (auto trackHeaderComp = dynamic_cast<TrackHeaderComponent*>(source))
    {
        showPluginRack (trackHeaderComp->getTrack ());
        resized ();
        repaint ();
    }

    if (auto midiClipComp = dynamic_cast<MidiClipComponent*>(source))
    {
        showPianoRoll (midiClipComp->getClip ());
        resized();
        repaint ();
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

        for (auto pr : m_pianoRolls)
            if (pr != nullptr && pr->isVisible ())
            {
                pr->setBounds (area);
            }
}

void LowerRangeComponent::showPluginRack(te::Track::Ptr track)
{
    std::cout << "---show Plugin Rack" << std::endl;


    //hide Piano Rolls
    for (auto pr : m_pianoRolls)
    {
        if (pr)
        {
            m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState.removeListener (pr);
            pr->setVisible (false);
        }
    }

    for (auto &prc : m_pluginRackComps)
    {
        prc->setVisible(false);
        if (prc->getTrack() == track)
        {
            prc->setVisible(true);
        }
    }
}

void LowerRangeComponent::showPianoRoll(tracktion_engine::Clip::Ptr clip)
{
    //if (auto midiclip = dynamic_cast<te::MidiClip*>(clip.get ()))
    //{
    //    //hide all PluginRacks
    //    for (auto &pluginrack : m_pluginRackComps)
    //    {
    //        pluginrack->setVisible (false);
    //    }
    //    m_pointedTrack = nullptr;

    //    for (auto &pianoroll : m_pianoRolls)
    //    {
    //        std::cout << "---show Piano Roll" << std::endl;

    //        pianoroll->setVisible (false);
    //        m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState.removeListener (pianoroll);
    //        if (pianoroll->getClip () == clip)
    //        {
    //            pianoroll->setVisible (true);
    //            m_editViewState.m_edit.engine.getDeviceManager ().getDefaultMidiInDevice ()->keyboardState.addListener (pianoroll);
    //            pianoroll->centerView ();
    //            //m_editViewState.m_isPianoRollVisible = true;
    //        }
    //    }
    //}
}

void LowerRangeComponent::addPianoRollEditor(PianoRollComponent *pr)
{
    addAndMakeVisible (pr);
    m_pianoRolls.add (pr);
}

void LowerRangeComponent::addPluginRackComp(PluginRackComponent *pluginrack)
{
    addAndMakeVisible (pluginrack);
    m_pluginRackComps.add (pluginrack);
}


