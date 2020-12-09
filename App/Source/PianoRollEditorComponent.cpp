#include "PianoRollEditorComponent.h"

PianoRollEditorComponent::PianoRollEditorComponent(EditViewState & evs
                                                   , te::MidiClip& clip)
    : m_editViewState(evs)
    , m_clip(clip)
{

}

void PianoRollEditorComponent::paint(juce::Graphics &g)
{
    g.setColour (juce::Colours::black);
    g.fillAll ();
    auto clipHeader = 10;
    if (auto mc = getMidiClip ())
    {
        auto& seq = mc->getSequence();
        for (auto n : seq.getNotes())
        {
            double sBeat = n->getStartBeat() - mc->getOffsetInBeats();
            double eBeat = n->getEndBeat() - mc->getOffsetInBeats();
            if (auto p = getParentComponent())
            {
                double y = ((1.0 - double (n->getNoteNumber()) / 127.0)
                            * (getHeight() - clipHeader) + clipHeader);

                auto x1 =  m_editViewState.beatsToX (
                            sBeat + m_editViewState.m_viewX1, p->getWidth ());
                auto x2 =  m_editViewState.beatsToX (
                            eBeat + m_editViewState.m_viewX1, p->getWidth ());

                g.setColour (juce::Colours::white);
                g.drawLine (float (x1), float (y), float (x2), float (y));
            }
        }
    }

}

void PianoRollEditorComponent::mouseDown(const juce::MouseEvent &)
{

}

void PianoRollEditorComponent::mouseDrag(const juce::MouseEvent &)
{

}

void PianoRollEditorComponent::mouseUp(const juce::MouseEvent &)
{

}

void PianoRollEditorComponent::showContextMenu()
{

}
