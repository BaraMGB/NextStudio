#include "../JuceLibraryCode/JuceHeader.h"
#include "AutomationLaneComponent.h"
#include "ClipComponent.h"
#include "EditViewState.h"
#include "TrackComponent.h"
#include "LassoSelectionTool.h"

class SongEditorView : public juce::Component
{
public:
    SongEditorView(EditViewState& evs, LowerRangeComponent& lr);
    ~SongEditorView(){}

	void paint(juce::Graphics& g) override;
	void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent &) override;
    void mouseDown (const juce::MouseEvent &) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;

    juce::OwnedArray<TrackComponent>& getTrackViews();
    void addTrackView(TrackComponent& tc);
    void updateViews();

    void clearTrackViews();

    void startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange);
    void updateLasso(const juce::MouseEvent& e);
    void stopLasso();
    void duplicateSelectedClips();
    
    void deleteSelectedTimeRange();
    juce::Array<te::Track*> getTracksWithSelectedTimeRange();
    tracktion::TimeRange getSelectedTimeRange();

private:

    void moveSelectedClips(double sourceTime, bool copy, double delta, int verticalOffset);  

    int timeToX (double time);
    double xtoTime(int x);
    double xToSnapedBeat (int x);
    double getSnapedTime(double time, bool downwards=false);

    ClipComponent *getClipViewForClip (const te::Clip::Ptr& clip);
    TrackComponent *getTrackViewForTrack(tracktion_engine::Track::Ptr track);
    AutomationLaneComponent *getAutomationLaneForAutomatableParameter(te::AutomatableParameter::Ptr ap);

    int getVerticalOffset(TrackComponent* sourceTrackComp, const juce::Point<int>& dropPos);
    te::Track::Ptr getTrackAt(int y);
    int getYForTrack (te::Track* track);
    te::AutomatableParameter::Ptr getAutomatableParamAt(int y);

    void updateClipSelection(bool add);
    void updateClipCache();
    void updateAutomationSelection(bool add);
    void updateAutomationCache();
    void updateRangeSelection(); 
    void clearSelectedTimeRange();
    void setSelectedTimeRange(tracktion::TimeRange tr);

    juce::Range<int> getVerticalRangeOfTrack(tracktion_engine::Track::Ptr track, bool withAutomation) ;
    void selectCatchedClips(const tracktion_engine::Track* track);

    struct SelectedTimeRange 
    {
        juce::Array<te::Track*> selectedTracks;
        tracktion::TimeRange timeRange;

        tracktion::TimePosition getStart() { return timeRange.getStart(); }
        tracktion::TimeDuration getLength() { return timeRange.getLength(); }
        tracktion::TimePosition getEnd() { return timeRange.getEnd(); }
    };

    void moveSelectedTimeRanges(tracktion::TimeDuration td, bool copy);
    void moveSelectedRangeOfTrack(te::Track::Ptr,tracktion::TimeDuration td, bool copy);

    void constrainClipInRange(te::Clip* c, tracktion::TimeRange r);
    // void addWaveFileToNewTrack(const SourceDetails &dragSourceDetails, double dropTime) const;

    void resizeSelectedClips(bool snap, bool fromLeftEdge=false);

    //essentials
    EditViewState&                      m_editViewState;
    LowerRangeComponent&                m_lowerRange;
    LassoSelectionTool                  m_lassoComponent;
    juce::OwnedArray<TrackComponent>    m_trackViews;

    //flags
    bool                                m_isDragging {false};
    bool                                m_lassoStartsInAutomation{false};
    bool                                m_isSelectingTimeRange{false};
    bool                                m_isDraggingSelectedTimeRange{false};
    bool                                m_leftBorderHovered{false};
    bool                                m_rightBorderHovered{false};
    bool                                m_hoveredTimeRange{false};

    //caches
    juce::Array<te::Clip*>              m_cachedSelectedClips;
    juce::Array<AutomationPoint*>       m_cachedSelectedAutomation;

    SelectedTimeRange                   m_selectedRange;
    juce::Image                         m_timeRangeImage;

    juce::Array<AutomationLaneComponent::CurvePoint*>
                                        m_selPointsAtMousedown;
    double                              m_curveAtMousedown{0.f};
    double                              m_clipPosAtMouseDown;

    ClipComponent*                      m_draggedClipComponent;
    double                              m_draggedTimeDelta{0.0};
    int                                 m_draggedVerticalOffset{0};

    te::Track::Ptr                      m_hoveredTrack {nullptr};
    te::AutomatableParameter::Ptr       m_hoveredAutamatableParam{nullptr};
    int                                 m_hoveredAutomationPoint {-1};
    tracktion::TimePosition             m_timeOfHoveredAutomationPoint;
    te::Clip::Ptr                       m_hoveredClip {nullptr};
    int                                 m_hoveredCurve {-1};


    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorView)
};

