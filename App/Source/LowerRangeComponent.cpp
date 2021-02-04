#include "LowerRangeComponent.h"


SplitterComponent::SplitterComponent(EditViewState &evs) : m_editViewState(evs)
{

}

void SplitterComponent::mouseEnter(const juce::MouseEvent &event)
{

}

void SplitterComponent::mouseExit(const juce::MouseEvent &event)
{

}

void SplitterComponent::mouseDown(const juce::MouseEvent &event)
{
    m_mousedownPosYatMousdown = event.mouseDownPosition.y;
    m_pianorollHeightAtMousedown = m_editViewState.m_pianorollHeight;
}

void SplitterComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (m_editViewState.m_isPianoRollVisible)
    {
        auto newHeight = static_cast<int> (m_pianorollHeightAtMousedown
                                        - event.getDistanceFromDragStartY());
        m_editViewState.m_pianorollHeight = std::max(20, newHeight);
    }
}

void SplitterComponent::mouseUp(const juce::MouseEvent &event)
{
}

//------------------------------------------------------------------------------

LowerRangeComponent::LowerRangeComponent(EditViewState &evs)
    : m_editViewState(evs)
    , m_pianoRollEditor (evs)
    , m_splitter (evs)
{
    m_editViewState.m_isPianoRollVisible = false;
    m_pluginRackComps.clear(true);
    addAndMakeVisible (m_splitter);
    addChildComponent (m_pianoRollEditor);
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

        m_splitter.setBounds (area.removeFromTop (m_splitterHeight));

        for (auto& pluginRackComp : m_pluginRackComps)
        {
            if (pluginRackComp->isVisible())
            {
                pluginRackComp->setBounds(area);
            }
        }

        if (m_pianoRollEditor.isVisible ())
        {
            m_pianoRollEditor.setBounds (area);
        }
}

void LowerRangeComponent::showPluginRack(te::Track::Ptr track)
{
    m_pianoRollEditor.setVisible (false);
    m_pianoRollEditor.clearPianoRollClip ();

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
    if (auto midiclip = dynamic_cast<te::MidiClip*>(clip.get ()))
    {
        //hide all PluginRacks
        for (auto &pluginrack : m_pluginRackComps)
        {
            pluginrack->setVisible (false);
        }

        m_pianoRollEditor.setVisible (true);
        m_pianoRollEditor.setPianoRollClip (std::make_unique<PianoRollContentComponent>(m_editViewState, clip));
        resized ();
    }
}

void LowerRangeComponent::hideAll()
{
    m_pianoRollEditor.setVisible (false);
    m_pianoRollEditor.clearPianoRollClip ();

    for (auto &pluginrack : m_pluginRackComps)
    {
        pluginrack->setVisible (false);
    }
}


void LowerRangeComponent::addPluginRackComp(PluginRackComponent *pluginrack)
{
    addAndMakeVisible (pluginrack);
    m_pluginRackComps.add (pluginrack);
}
