#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"

namespace te = tracktion_engine;

class TimelineOverlayComponent : public juce::Component
{
public:
    TimelineOverlayComponent(
            EditViewState& evs
          , te::Track::Ptr track
          , TimeLineComponent& tlc);
    void paint (juce::Graphics& g) override;
private:
    bool hitTest(int,int) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    std::vector<te::MidiClip*> getMidiClipsOfTrack();
    tracktion_engine::MidiClip * getMidiClipByPos(int x);

    int timeToX(double time);
    double xToBeats(int x);
    void updateClipRects();
    EditViewState& m_editViewState;
    tracktion_engine::Track::Ptr m_track;
    [[maybe_unused]] double m_loop1AtMousedown{}
         , m_loop2AtMousedown{};
    bool m_leftResized {false};
    bool m_rightResized{false};
    te::ClipPosition m_cachedPos;
    te::MidiClip * m_cachedClip{};
    TimeLineComponent & m_timelineComponent;
    juce::Array<juce::Rectangle<int>> m_clipRects;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineOverlayComponent)
};
