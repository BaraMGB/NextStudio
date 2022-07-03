#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "EditViewState.h"
#include "Utilities.h"

namespace te = tracktion_engine;


class LassoSelectionTool : public juce::Component
{
public:
    struct LassoRect
    {
        LassoRect ()= default;
        [[maybe_unused]] LassoRect (tracktion::core::TimeRange timeRange, double top, double bottom)
            : m_timeRange(timeRange)
            , m_verticalRange(top, bottom)
            , m_startTime(timeRange.getStart ().inSeconds())
            , m_endTime (timeRange.getEnd ().inSeconds())
            , m_top (top)
            , m_bottom (bottom){}

        juce::Rectangle<int> getRect (EditViewState& evs, double viewX1, double viewX2, int viewWidth) const;
        tracktion::core::TimeRange m_timeRange;
        juce::Range<double> m_verticalRange { 0,0 };

        double m_startTime { 0 };
        double m_endTime { 0 };
        double m_top { 0 };
        double m_bottom { 0 };
    };

    explicit LassoSelectionTool(EditViewState& evs
                                , juce::CachedValue<double>& x1
                                , juce::CachedValue<double>& x2)
        : m_editViewState(evs)
        , m_X1(x1)
        , m_X2(x2)

    {}
    void paint(juce::Graphics &g) override;

    void startLasso(const juce::MouseEvent& e);
    void updateLasso(const juce::MouseEvent& e, int newTop);
    void stopLasso();
    LassoSelectionTool::LassoRect getLassoRect() const;

private:

    double xToTime(const int x);
    bool                           m_isLassoSelecting {false};
    bool                           m_isSongEditor;

    EditViewState&                 m_editViewState;
    LassoRect                      m_lassoRect;
    juce::CachedValue<double>&     m_X1;
    juce::CachedValue<double>&     m_X2;

    double                         m_clickedTime{};

    tracktion::core::TimeRange getDraggedTimeRange(const juce::MouseEvent& e);
    void updateClipCache();
    juce::Range<double>
        getVerticalRangeOfTrack(double trackPosY,
                                tracktion_engine::AudioTrack* track) const;


};
