#include "../JuceLibraryCode/JuceHeader.h"
#include "ClipComponent.h"
#include "EditViewState.h"
#include "TrackComponent.h"
#include "LassoSelectionTool.h"
#include "Utilities.h"

class SongEditorView : public juce::Component
                     , public juce::DragAndDropTarget
{
public:
    SongEditorView(EditViewState& evs);
    ~SongEditorView(){}

    int getTrackHeight(const TrackComponent* tc) const;
	void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent &) override;
    void mouseDown (const juce::MouseEvent &) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;

    inline bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails&) override;

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
    int getSnapedX(int x, bool down=false) const;
    double xtoTime(int x);
    double getSnapedTime(double time, bool downwards=false);

    ClipComponent *getClipComponentForClip (const te::Clip::Ptr& clip);
    TrackComponent *getTrackForClip(int verticalOffset, const te::Clip *clip);
    TrackComponent *getTrackCompForTrack(const tracktion_engine::Track::Ptr& track);

    void updateClipSelection(bool add);
    void updateClipCache();
    void updateAutomationSelection(bool add);
    void updateAutomationCache();
    void updateRangeSelection(); 
    void clearSelectedTimeRange();

    juce::Range<double> getVerticalRangeOfTrack(double scrollY, tracktion_engine::Track* track, bool withAutomation) const;
    void selectCatchedClips(const tracktion_engine::Track* track);

    double getPasteTime(double dropTime, ClipComponent* draggedClip) const;
    int getVerticalOffset(const SourceDetails& dragSourceDetails, const juce::Point<int>& dropPos);

    void addWaveFileToNewTrack(const SourceDetails &dragSourceDetails, double dropTime) const;

    void resizeSelectedClips(bool snap, bool fromLeftEdge=false);
    void drawResizingOverlays (const ClipComponent *draggedClip);
    void drawDraggingOverlays (const ClipComponent *draggedClip, const juce::Point<int> &dropPos, int verticalOffset);

    EditViewState&                      m_editViewState;
    LassoSelectionTool                  m_lassoComponent;
    juce::OwnedArray<TrackComponent>    m_trackViews;
    juce::Array<te::Clip*>              m_cachedSelectedClips;
    juce::Array<AutomationPoint*>        m_cachedSelectedAutomation;
    double                              m_draggedTimeDelta;
    bool                                m_automationClicked{false};
    bool                                m_selectTimerange{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorView)
};

