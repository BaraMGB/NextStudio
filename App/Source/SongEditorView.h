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
    void resized();

    void mouseDown (const juce::MouseEvent &) override;
    void mouseDrag (const juce::MouseEvent &) override;
    void mouseUp (const juce::MouseEvent &) override;

    inline bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails& dragSourceDetails) override;
    void itemDropped (const SourceDetails& dragSourceDetails) override;
    void itemDragExit (const SourceDetails&) override;

    juce::OwnedArray<TrackComponent>& getTrackViews();
    TrackComponent * getTrackComponent (int y);

    void addTrackViews(TrackComponent& tc);

    void updateViews();

    //drop stuff//

    void turnoffAllTrackOverlays()
    {
        for (auto &tc : getTrackComps ())
        {
            tc->getTrackOverlay ().setVisible (false);
        }
    }
    juce::OwnedArray<TrackComponent>& getTrackComps ()
    {
        return m_views;
    }
    const TrackComponent& getTrackView(te::Track::Ptr track);

    void clear();
    int getSize();
private:

    LassoSelectionTool m_lassoComponent;
    ClipComponent *getClipComponentForClip (const te::Clip::Ptr& clip);

public:
    LassoSelectionTool& getLasso();

private:
    TrackComponent *getTrackCompForTrack(const tracktion_engine::Track::Ptr& track);
    EditViewState& m_editViewState;
    juce::OwnedArray<TrackComponent> m_views;

};

