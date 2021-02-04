#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TimeLineComponent.h"
#include "PlayHeadComponent.h"
#include "PianoRollContentComponent.h"

namespace te = tracktion_engine;

class TimelineOverlayComponent : public juce::Component
{
public:
    TimelineOverlayComponent(EditViewState& evs);
    void paint (juce::Graphics& g) override;
private:
    bool hitTest(int,int) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    int timeToX(double time);
    EditViewState& m_editViewState;
    double m_loop1AtMousedown
         , m_loop2AtMousedown;
};


class PianoRollComponent : public juce::Component
                         , public te::ValueTreeAllEventListener
                         , public juce::MidiKeyboardStateListener
{
public:
    PianoRollComponent (EditViewState&);
    ~PianoRollComponent();

    void focusLost (juce::Component::FocusChangeType cause) override;
    void focusGained (juce::Component::FocusChangeType cause) override;
    void resized () override;

    void valueTreeChanged() override {}
    void valueTreePropertyChanged (juce::ValueTree&
                                   , const juce::Identifier&) override;

    void handleNoteOn(juce::MidiKeyboardState*, int, int, float) override;
    void handleNoteOff(juce::MidiKeyboardState*, int, int, float) override;

    te::Clip::Ptr getClip();
    te::MidiClip* getMidiClip();

    void centerView();
    void setPianoRollClip(std::unique_ptr<PianoRollContentComponent> pianoRollClip);
    void clearPianoRollClip();


private:
    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;
    juce::MidiKeyboardState m_keybordstate;
    juce::MidiKeyboardComponent m_keyboard;
    TimeLineComponent m_timeline;
    std::unique_ptr<PianoRollContentComponent> m_pianoRollClip{nullptr};
    TimelineOverlayComponent m_timelineOverlay;
    PlayheadComponent m_playhead;

};
