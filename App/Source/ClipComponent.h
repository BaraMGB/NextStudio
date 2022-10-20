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
    
    void resized () override;
    te::Clip::Ptr getClip () { return m_clip; }


    [[nodiscard]] juce::Rectangle<int> getVisibleBounds();

    [[nodiscard]] double getClickPosTime () const {return m_clickedTime; }
    [[nodiscard]] int getClickPosOffsetX () const
    {
        return m_editViewState.timeToX (m_clickedTime, getParentWidth (), m_editViewState.m_viewX1, m_editViewState.m_viewX2);
    }

    te::Track::Ptr getTrack(const te::Clip::Ptr& clip);

    int getMargin() {return m_margin;}
protected:

    EditViewState& m_editViewState;
    te::Clip::Ptr m_clip;

    void showContextMenu();
    juce::Label m_nameLabel;

private:

    [[nodiscard]] double xToTime(int x) const;

    double m_clickedTime {0.0};
    bool m_isCtrlDown{false};
    bool m_isShiftDown{false};
    bool m_isDragging{false};
    bool m_resizeLeft{false};
    bool m_resizeRight{false};

    const int m_margin {2};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipComponent)
};


