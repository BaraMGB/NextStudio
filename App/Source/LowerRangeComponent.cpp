#include "LowerRangeComponent.h"
#include <algorithm>


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
    : m_evs(evs)
    , m_pianoRollEditor (evs)
    , m_splitter (evs)
{
    m_evs.m_isPianoRollVisible = false;
    m_rackViews.clear(true);
    addAndMakeVisible (m_splitter);
    addChildComponent (m_pianoRollEditor);
    m_evs.m_edit.state.addListener (this);
}

LowerRangeComponent::~LowerRangeComponent()
{
    m_evs.m_edit.state.removeListener (this);
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

        for (auto& pluginRackComp : m_rackViews)
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
void LowerRangeComponent::showRackView(const te::Track::Ptr track)
{
    m_pianoRollEditor.setVisible (false);
    m_pianoRollEditor.clearTrack();


    for (auto &rackView : m_rackViews)
    {
        rackView->setVisible(false);

        if (rackView->getTrack() == track)
            rackView->setVisible(true);
    }

    m_evs.m_isPianoRollVisible = false;
    resized();
    repaint();
}

void LowerRangeComponent::showPianoRoll(const tracktion_engine::Track::Ptr track)
{
    if (track->state.getProperty (IDs::isMidiTrack))
    {
        for (auto &pr : m_rackViews)
           pr->setVisible (false);

        m_pianoRollEditor.setVisible (true);
        m_pianoRollEditor.setTrack (track);
        m_evs.m_isPianoRollVisible = true;
        resized();
        repaint();
    }

}

void LowerRangeComponent::addRackView(std::unique_ptr<RackView> pluginrack)
{
    addChildComponent(pluginrack.get());
    m_rackViews.add (std::move(pluginrack));
}

void LowerRangeComponent::clearPluginRacks()
{
    std::cout << "cleat racks vector" << std::endl;
    m_rackViews.clear ();
}
const RackView* LowerRangeComponent::getVisibleRackView()
{
    for (auto rv : m_rackViews)
        if (rv->isVisible())
            return rv;

    return nullptr;
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
