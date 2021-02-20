#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;

class ClipComponent : public juce::Component
{
public:
    ClipComponent (EditViewState&, te::Clip::Ptr);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;

    te::Clip::Ptr getClip () { return m_clip; }

    bool isCtrlDown () const { return m_isCtrlDown; }
    bool isShiftDown () const { return m_isShiftDown; }

    double getClickPosTime () const { return m_clickPosTime; }
    int getClipPosOffsetX () const
    {
        return m_editViewState.timeToX (m_clickPosTime, getParentWidth ());
    }
    void setClickPosTime (double clickPosTime) { m_clickPosTime = clickPosTime; }
    te::Track::Ptr getTrack(te::Clip::Ptr clip);

protected:
    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;

    void showContextMenu();
private:
    double m_clipPosAtMouseDown{};
    double m_clickPosTime{0.0};
    bool m_isCtrlDown{false};
    bool m_isShiftDown{false};
    bool m_isDragging{false};
};


