
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "Utilities.h"


class TrackListView  : public juce::Component
                     , public juce::DragAndDropTarget
                     , private juce::ChangeListener
{
public:
    TrackListView(EditViewState& evs)
        : m_editViewState (evs)
    {
        m_editViewState.m_selectionManager.addChangeListener(this);
    }
    ~TrackListView()
    {
        m_editViewState.m_selectionManager.removeChangeListener(this);
    }

    void resized() override;

    void mouseDown (const juce::MouseEvent &) override;

    inline bool isInterestedInDragSource (const SourceDetails&) override{return true;}
    void itemDragMove (const SourceDetails& dragSourceDetails) override{}
    void itemDragExit (const SourceDetails&) override{}
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;

    int getTrackHeight(TrackHeaderComponent* header) const;
    void addHeaderViews(TrackHeaderComponent& th);
    void updateViews();

    void clear();

private:

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    te::AudioTrack::Ptr addTrack(bool isMidiTrack, bool isFolderTrack, juce::Colour trackColour);

    EditViewState& m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_trackHeaders;
    const int getPopupResult();
    void collapseTracks(bool minimize);
    TrackHeaderComponent* getTrackHeaderView(tracktion_engine::Track::Ptr track);
};