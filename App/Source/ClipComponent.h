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
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;
    void mouseExit(const juce::MouseEvent&) override;
    
    te::Clip::Ptr getClip () { return m_clip; }

    [[nodiscard]] bool isCtrlDown () const { return m_isCtrlDown; }
    [[nodiscard]] bool isShiftDown () const { return m_isShiftDown; }
    [[nodiscard]] bool isResizeLeft() const;
    [[nodiscard]] bool isResizeRight() const;

    void setResizeLeft(bool mResizeLeft){ m_resizeLeft = mResizeLeft; }
    void setResizeRight(bool mResizeRight){ m_resizeRight = mResizeRight; }

    [[nodiscard]] double getClickPosTime () const { return m_clickedTime; }
    [[nodiscard]] int getClipPosOffsetX () const
    {
        return m_editViewState.timeToX (m_clickedTime, getParentWidth (), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    }

    te::Track::Ptr getTrack(const te::Clip::Ptr& clip);


protected:

    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;
    bool m_updateRegion{false};

    void showContextMenu();

private:

    [[nodiscard]] double xToTime(int x) const;

    double m_clickedTime {0.0};
    bool m_isCtrlDown{false};
    bool m_isShiftDown{false};
    bool m_isDragging{false};
    bool m_resizeLeft{false};
    bool m_resizeRight{false};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
};


