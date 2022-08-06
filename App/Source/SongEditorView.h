#include "../JuceLibraryCode/JuceHeader.h"
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
    const TrackComponent& getTrackView(te::Track::Ptr track);

    void clear();
    int getSize();
    LassoSelectionTool& getLasso();
    LassoSelectionTool::LassoRect getCurrentLassoRect();

    void startLasso(const juce::MouseEvent& e);

    void updateLasso(const juce::MouseEvent& e);
    void duplicateSelectedClips();
    void moveSelectedClips(ClipComponent *draggedClip, int verticalOffset);
private:

    int timeToX (double time);
    int getSnapedX(int x, bool down=false) const;

    TrackComponent *getTrackCompForTrack(const tracktion_engine::Track::Ptr& track);
    EditViewState& m_editViewState;
    LassoSelectionTool m_lassoComponent;
    ClipComponent *getClipComponentForClip (const te::Clip::Ptr& clip);
    juce::OwnedArray<TrackComponent>    m_trackViews;
    juce::Array<te::Clip*>              m_cachedSelectedClips;
    double                              m_cachedY;

    void updateSelection(bool add);
    void updateClipCache();
    juce::Range<double> getVerticalRangeOfTrack(
        double scrollY, tracktion_engine::Track* track) const;
    void selectCatchedClips(const tracktion_engine::Track* track);

    double getPasteTime(double dropTime, ClipComponent* draggedClip) const;
    bool trackWantsClip(const te::Clip* clip, const TrackComponent* track) const;
    double m_draggedTimeDelta;

    int getVerticalOffset(const SourceDetails& dragSourceDetails,
                          const juce::Point<int>& dropPos);

    TrackComponent *getTrackForClip(int verticalOffset, const te::Clip *clip);

    double xtoTime(int x);

    double m_cachedEditLength {0.0};

    double getSnapedTime(double time, bool downwards=false);

    void addWaveFileToNewTrack(const SourceDetails &dragSourceDetails, double dropTime) const;



    void resizeSelectedClips(bool snap, bool fromLeftEdge=false);
    void drawResizingOverlays (const ClipComponent *draggedClip);
    void drawDraggingOverlays (const ClipComponent *draggedClip, const juce::Point<int> &dropPos, int verticalOffset);



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongEditorView)
};

