#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;


class LassoSelectionTool : public juce::Component
{
public:
    explicit LassoSelectionTool(EditViewState& evs)
        : m_editViewState(evs) {}
    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;

    void updateSelection(bool add);
private:

    struct LassoRect
    {
        LassoRect ()= default;
        [[maybe_unused]] LassoRect (te::EditTimeRange timeRange, double top, double bottom)
            : m_timeRange(timeRange)
            , m_verticalRange(top, bottom)
            , m_startTime(timeRange.getStart ())
            , m_endTime (timeRange.getEnd ())
            , m_top (top)
            , m_bottom (bottom){}

        juce::Rectangle<int> getRect (EditViewState& evs, int viewWidth) const;
        te::EditTimeRange m_timeRange { 0,0 };
        juce::Range<double> m_verticalRange { 0,0 };

        double m_startTime { 0 };
        double m_endTime { 0 };
        double m_top { 0 };
        double m_bottom { 0 };
    };

    double xToTime(const int x);
    bool                           m_isLassoSelecting {false};
    EditViewState&                 m_editViewState;
    double                         m_clickedTime{};
    double                         m_cachedY{};
    LassoRect                      m_lassoRect;
};