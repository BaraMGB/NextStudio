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

    void addTrackView(TrackComponent& tc);

    void updateViews();


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
    LassoSelectionTool& getLasso();
    LassoSelectionTool::LassoRect getCurrentLassoRect();

    void startLasso(const juce::MouseEvent& e)
    {
        m_lassoComponent.startLasso(e.getEventRelativeTo (&m_lassoComponent));
        m_cachedY = m_editViewState.m_viewY;
        updateClipCache ();
    }

    void updateLasso(const juce::MouseEvent& e)
    {
        if (m_lassoComponent.isVisible ())
        {
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
            m_lassoComponent.updateLasso(e.getEventRelativeTo (&m_lassoComponent));
            updateSelection(e.mods.isShiftDown ());
        }
    }

private:

    TrackComponent *getTrackCompForTrack(const tracktion_engine::Track::Ptr& track);
    EditViewState& m_editViewState;
    LassoSelectionTool m_lassoComponent;
    ClipComponent *getClipComponentForClip (const te::Clip::Ptr& clip);
    juce::OwnedArray<TrackComponent>    m_views;
    juce::Array<te::Clip*>              m_cachedSelectedClips;
    double                              m_cachedY;

    void updateSelection(bool add);
    void updateClipCache();
    juce::Range<double> getVerticalRangeOfTrack(
        double trackPosY, tracktion_engine::AudioTrack* track) const;
    void selectCatchedClips(const tracktion_engine::AudioTrack* track);

};

