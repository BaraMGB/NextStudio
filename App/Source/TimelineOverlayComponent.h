#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"

namespace te = tracktion_engine;

class TimelineOverlayComponent : public juce::Component
{
public:
    TimelineOverlayComponent(EditViewState& evs, tracktion_engine::Clip::Ptr clip);
    void paint (juce::Graphics& g) override;
private:
    bool hitTest(int,int) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    std::vector<te::MidiClip*> getMidiClipsOfTrack();
    tracktion_engine::MidiClip *getMidiclipByPos(int y);

    int timeToX(double time);
    double xToBeats(int x);
    void updateClipRects();
    EditViewState& m_editViewState;
    tracktion_engine::Clip::Ptr m_defaultClip;
    double m_loop1AtMousedown
         , m_loop2AtMousedown;
    bool m_leftResized {false};
    bool m_rightResized{false};
    juce::Point<float> m_posAtMousedown;
    juce::Array<juce::Rectangle<int>> m_clipRects;
};
