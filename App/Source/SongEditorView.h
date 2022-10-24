#include "../JuceLibraryCode/JuceHeader.h"
#include "AutomationLaneComponent.h"
#include "ClipComponent.h"
#include "EditViewState.h"
#include "TrackComponent.h"
#include "LassoSelectionTool.h"
#include "Utilities.h"
#include "tracktion_core/utilities/tracktion_Time.h"

class SongEditorView : public juce::Component
{
public:
    SongEditorView(EditViewState& evs, LowerRangeComponent& lr);
    ~SongEditorView(){}

    int getTrackHeight(const TrackComponent* tc) const;
	void paint(juce::Graphics& g) override;
	void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent &) override;
    void mouseDown (const juce::MouseEvent &) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;

    juce::OwnedArray<TrackComponent>& getTrackViews();
    TrackComponent * getTrackComponent (int y);
    void addTrackView(TrackComponent& tc);
    void updateViews();

    void turnoffAllTrackOverlays();
    juce::OwnedArray<TrackComponent>& getTrackComps ();
    void clearTracks();
    int countTracks();

    LassoSelectionTool& getLasso();
    LassoSelectionTool::LassoRect getCurrentLassoRect();
    void startLasso(const juce::MouseEvent& e, bool fromAutomation, bool selectRange);
    void updateLasso(const juce::MouseEvent& e);
    void stopLasso();
    void duplicateSelectedClips();

private:

    void moveSelectedClips(double sourceTime, bool copy, bool snap, double delta, int verticalOffset);  
    int timeToX (double time);
    double xToSnapedBeat (int x);
    int getSnapedX(int x, bool down=false) const;
    double xtoTime(int x);
    double getSnapedTime(double time, bool downwards=false);

    ClipComponent *getClipComponentForClip (const te::Clip::Ptr& clip);
    TrackComponent *getTrackForClip(int verticalOffset, const te::Clip *clip);
    TrackComponent *getTrackCompForTrack(tracktion_engine::Track::Ptr track);
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
    juce::Array<juce::Image> getSelectedClipImages();

    juce::Range<int> getVerticalRangeOfTrack(tracktion_engine::Track::Ptr track, bool withAutomation) ;
    void selectCatchedClips(const tracktion_engine::Track* track);

    // void addWaveFileToNewTrack(const SourceDetails &dragSourceDetails, double dropTime) const;

    void resizeSelectedClips(bool snap, bool fromLeftEdge=false);

    EditViewState&                      m_editViewState;
    LowerRangeComponent&                m_lowerRange;
    LassoSelectionTool                  m_lassoComponent;
    juce::OwnedArray<TrackComponent>    m_trackViews;
    juce::Array<te::Clip*>              m_cachedSelectedClips;
    juce::Array<AutomationPoint*>       m_cachedSelectedAutomation;
    bool                                m_automationClicked{false};
    bool                                m_selectTimerange{false};

    te::Track::Ptr                      m_hoveredTrack {nullptr};
    te::AutomatableParameter::Ptr       m_hoveredAutamatableParam{nullptr};
    int                                 m_hoveredAutomationPoint {-1};
    tracktion::TimePosition             m_timeOfHoveredAutomationPoint;
    te::Clip::Ptr                       m_hoveredClip {nullptr};
    int                                 m_hoveredCurve {-1};
    bool                                m_leftBorderHovered{false};
    bool                                m_rightBorderHovered;

    juce::Array<AutomationLaneComponent::CurvePoint*>
                                        m_selPointsAtMousedown;

    double                              m_curveAtMousedown{0.f};
    ClipComponent*                      m_draggedClipComponent;
    double                              m_draggedTimeDelta{0.0};
    double                              m_clipPosAtMouseDown;
    bool                                m_isDragging {false};
    juce::Array<juce::Image>            m_selectedClipsImages; 
    int                                 m_draggedVerticalOffset{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorView)
};

