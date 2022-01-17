#include "LowerRangeComponent.h"


SplitterComponent::SplitterComponent(EditViewState &evs) : m_editViewState(evs)
{

}

void SplitterComponent::mouseMove(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    m_isHovering = true;
    repaint ();
}

void SplitterComponent::mouseEnter(const juce::MouseEvent &)
{
}

void SplitterComponent::mouseExit(const juce::MouseEvent &)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
    m_isHovering = false;
    repaint ();
}

void SplitterComponent::mouseDown(const juce::MouseEvent &)
{
    m_pianorollHeightAtMousedown = m_editViewState.m_midiEditorHeight;
    m_cachedPianoNoteNum = (double) m_editViewState.m_pianoStartKey;
}

void SplitterComponent::mouseDrag(const juce::MouseEvent &event)
{
    if (m_editViewState.m_isPianoRollVisible)
    {
        auto newHeight = static_cast<int> (m_pianorollHeightAtMousedown
                                        - event.getDistanceFromDragStartY());
        auto noteHeight = (double) m_editViewState.m_pianoKeyWidth;
        auto noteDist = event.getDistanceFromDragStartY () / noteHeight;

        m_editViewState.m_pianoStartKey =
                juce::jlimit(0.0
                           , 127.0 - (getHeight ()
                                / m_editViewState.m_pianoKeyWidth)
                           , m_cachedPianoNoteNum + noteDist);
        m_editViewState.m_midiEditorHeight = std::max(20, newHeight);
    }
}

void SplitterComponent::mouseUp(const juce::MouseEvent &)
{
}

void SplitterComponent::paint(juce::Graphics &g)
{
    if (m_isHovering)
    {
        g.setColour(juce::Colours::navy);
        g.fillRect (getLocalBounds ());
    }
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
    m_editViewState.m_edit.state.addListener (this);
}

LowerRangeComponent::~LowerRangeComponent()
{
    m_editViewState.m_selectionManager.removeChangeListener(this);
    m_editViewState.m_edit.state.removeListener (this);
}

void LowerRangeComponent::changeListenerCallback(juce::ChangeBroadcaster * source)
{

    if (auto trackHeaderComp = dynamic_cast<TrackHeaderComponent*>(source))
    {
        std::cout << "th" << std::endl;
        showPluginRack (trackHeaderComp->getTrack ());
        resized ();
        repaint ();
    }

    if (auto midiClipComp = dynamic_cast<MidiClipComponent*>(source))
    {
        showPianoRoll (midiClipComp->getClip()->getTrack());
        resized();
        repaint ();
    }
}

void LowerRangeComponent::paint(juce::Graphics &g)
{
    auto rect = getLocalBounds();
    g.setColour(juce::Colour(0xff181818));
    g.fillRect(rect.removeFromBottom(getHeight() - (int) m_splitterHeight).toFloat());
}

void LowerRangeComponent::paintOverChildren(juce::Graphics &g)
{
    float size = 20;
    auto area = getLocalBounds ().toFloat();
    g.setColour(juce::Colour(0xff555555));
    juce::Path topLeft;

    topLeft.addArc (area.getX(),area.getY() + m_splitterHeight, size, size
              , juce::MathConstants<float>::pi * 1.5
              , juce::MathConstants<float>::pi * 2
              , true);
    topLeft.lineTo (area.getX(),area.getY() + m_splitterHeight);
    topLeft.closeSubPath ();
    g.fillPath (topLeft);

    juce::Path topRight;
    topRight.addArc (
                area.getWidth () - size
              , area.getY () + m_splitterHeight
              , size, size
              , juce::MathConstants<float>::pi * 2
              , juce::MathConstants<float>::pi * 2.5
              , true);
    topRight.lineTo (area.getWidth (), area.getY () - m_splitterHeight);
    topRight.closeSubPath ();
    g.fillPath (topRight);

    juce::Path bottomRight;
    bottomRight.addArc (area.getWidth () - size, area.getHeight () - size, size, size
              , juce::MathConstants<float>::pi * 2.5
              , juce::MathConstants<float>::pi * 3
              , true);
    bottomRight.lineTo (area.getWidth () , area.getHeight () );
    bottomRight.closeSubPath ();
    g.fillPath (bottomRight);

    juce::Path bottomLeft;
    bottomLeft.addArc (area.getX (), area.getHeight () - size, size, size
              , juce::MathConstants<float>::pi * 3
              , juce::MathConstants<float>::pi * 3.5
              , true);
    bottomLeft.lineTo (area.getX (), area.getHeight ());
    bottomLeft.closeSubPath ();
    g.fillPath (bottomLeft);
}

void LowerRangeComponent::resized()
{
        auto area = getLocalBounds();

        m_splitter.setBounds (area.removeFromTop ((int) m_splitterHeight));

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
void LowerRangeComponent::removePluginRackwithTrack(const te::Track::Ptr& track)
{
    for (auto &prc : m_pluginRackComps)
    {
        if (prc->getTrack() == track)
        {
            prc->setVisible (false);
            m_pluginRackComps.removeObject (prc);
            if (!m_pluginRackComps.isEmpty ())
            {
                m_pluginRackComps.getFirst ()->setVisible (true);
            }
        }
    }
}
void LowerRangeComponent::showPluginRack(const te::Track::Ptr& track)
{
    m_pianoRollEditor.setVisible (false);
    m_pianoRollEditor.clearTrack();


    for (auto &prc : m_pluginRackComps)
    {
        prc->setVisible(false);
        if (prc->getTrack() == track)
        {
            prc->setVisible(true);
        }
    }
}

void LowerRangeComponent::showPianoRoll(const tracktion_engine::Track::Ptr& track)
{
    if (track->state.getProperty (IDs::isMidiTrack))
    {
        //hide all PluginRacks
        for (auto &pluginrack : m_pluginRackComps)
        {
            pluginrack->setVisible (false);
        }

        m_pianoRollEditor.setVisible (true);
        m_pianoRollEditor.setTrack (track);

        resized ();
    }
}

void LowerRangeComponent::addPluginRackComp(PluginRackComponent *pluginrack)
{
    addAndMakeVisible (pluginrack);
    m_pluginRackComps.add (pluginrack);
}

void LowerRangeComponent::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
        if (v.hasType (tracktion_engine::IDs::MIDICLIP))
        {
            resized ();
            repaint ();
        }
        if (v.hasType (IDs::EDITVIEWSTATE))
        {
            if (i == IDs::pianoX1
                || i == IDs::pianoX2
                || i == IDs::pianoY1
                || i == IDs::pianorollNoteWidth)
            {
                resized ();
                repaint ();
            }
        }
}

void LowerRangeComponent::valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &)
{
    resized ();
    repaint ();
}

void LowerRangeComponent::valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int)
{
    resized ();
    repaint ();
}

void LowerRangeComponent::valueTreeChildOrderChanged(juce::ValueTree &, int, int)
{
    resized ();
    repaint ();
}
