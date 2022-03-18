#include "MidiClipComponent.h"

#include <utility>

MidiClipComponent::MidiClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, std::move(c))
{
    setPaintingIsUnclipped(true);

}

MidiClipComponent::~MidiClipComponent()
{
    removeAllChangeListeners ();
}

void MidiClipComponent::paint (juce::Graphics& g)
{
    auto startX = m_editViewState.timeToX(getClip()->getPosition().getStart(),
                                          getParentComponent()->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    auto endX = m_editViewState.timeToX(getClip()->getPosition().getEnd(),
                                        getParentComponent()->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    if (!(endX < 0 || startX > getParentComponent()->getWidth()))
    {
        ClipComponent::paint(g);
        auto clipHeader = (float) m_editViewState.m_trackHeightMinimized/2;
        if (auto mc = getMidiClip())
        {
            auto& seq = mc->getSequence();
            for (auto n: seq.getNotes())
            {
                double sBeat = n->getStartBeat() - mc->getOffsetInBeats();
                double eBeat = n->getEndBeat() - mc->getOffsetInBeats();
                if (auto p = getParentComponent())
                {
                    auto y =  (1.f - (float)(n->getNoteNumber()) / 127.f)
                                * ((float)getHeight() - clipHeader)
                                + clipHeader;
                    auto x1 = (float) m_editViewState.beatsToX(
                                sBeat + m_editViewState.m_viewX1, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
                    auto x2 = (float) m_editViewState.beatsToX(
                                eBeat + m_editViewState.m_viewX1, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);

                    x1 = juce::jmin(juce::jmax(2.f, x1), (float) getWidth () - 2.f);
                    x2 = juce::jmin(juce::jmax(2.f, x2), (float) getWidth () - 2.f);

                    g.setColour(juce::Colours::white);

                    g.drawLine(x1, y, x2, y);
                }
            }
        }
    }
}

void MidiClipComponent::mouseExit(const juce::MouseEvent &/*e*/)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void MidiClipComponent::mouseDown(const juce::MouseEvent &e)
{
    ClipComponent::mouseDown(e);
    if ((!isResizeLeft() && !isResizeRight()) && m_editViewState.m_isPianoRollVisible)
        GUIHelpers::centerMidiEditorToClip(m_editViewState, m_clip);
    
    if (e.getNumberOfClicks () > 1 || m_editViewState.m_isPianoRollVisible)
    {
        m_editViewState.m_isPianoRollVisible = true;
        sendChangeMessage ();
    }
}


