#include "MidiClipComponent.h"


MidiClipComponent::MidiClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, std::move(c))
{
    setPaintingIsUnclipped(true);

}

MidiClipComponent::~MidiClipComponent()
{
}

void MidiClipComponent::paint (juce::Graphics& g)
{
    auto startX = m_editViewState.timeToX(getClip()->getPosition().getStart().inSeconds(),
                                          getParentComponent()->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    auto endX = m_editViewState.timeToX(getClip()->getPosition().getEnd().inSeconds(),
                                        getParentComponent()->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    if (!(endX < 0 || startX > getParentComponent()->getWidth()))
    {
        ClipComponent::paint(g);
        auto clipHeader = static_cast<int>(m_editViewState.m_clipHeaderHeight);
        if (auto mc = getMidiClip())
        {
            auto& seq = mc->getSequence();
            auto range = seq.getNoteNumberRange();
            auto lines = juce::jmax(20, range.getLength());
            auto noteHeight = juce::jmax(1,((getHeight() - clipHeader) / lines)/2);
            auto noteColor = m_clip->getColour().withLightness(0.6f);
            for (auto n: seq.getNotes())
            {
                double sBeat = n->getStartBeat().inBeats() - mc->getOffsetInBeats().inBeats();
                double eBeat = n->getEndBeat().inBeats() - mc->getOffsetInBeats().inBeats();
                if (auto p = getParentComponent())
                {
                    auto y = (getHeight()/2);
                    if (!range.isEmpty())
                        y = juce::jmap(n->getNoteNumber(), range.getStart() + lines, range.getStart(), clipHeader + 5, getHeight() - 5);

                    int x1 =  m_editViewState.beatsToX(
                                sBeat + m_editViewState.m_viewX1, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
                    int x2 =  m_editViewState.beatsToX(
                                eBeat + m_editViewState.m_viewX1, p->getWidth(), m_editViewState.m_viewX1, m_editViewState.m_viewX2);

                    int gap = 2;
                    x1 = juce::jmin(juce::jmax(gap, x1), getWidth () - gap);
                    x2 = juce::jmin(juce::jmax(gap, x2), getWidth () - gap);

                    auto area = getVisibleBounds();
                    x1 = juce::jmax(area.getX(), x1);
                    x2 = juce::jmin(area.getRight(), x2);
                    auto w = juce::jmax(0, x2 - x1);
                    g.setColour(noteColor);
                    g.fillRect(x1, y, w, noteHeight);
                }
            }
        }
    }
}


