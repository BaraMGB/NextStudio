#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"

namespace te = tracktion_engine;

class MidiClipComponent : public ClipComponent
                        , public juce::ChangeBroadcaster
{
public:
    MidiClipComponent (EditViewState&, te::Clip::Ptr);
    ~MidiClipComponent();

    te::MidiClip* getMidiClip()
    {
        return dynamic_cast<te::MidiClip*> (m_clip.get());
    }

    void paint (juce::Graphics& g) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent &) override;

private:
    int m_mouseDownX {0};
    int m_clipWidthMouseDown;
    double m_oldDistTime{0.0};
    tracktion_engine::ClipPosition m_posAtMouseDown;
};
