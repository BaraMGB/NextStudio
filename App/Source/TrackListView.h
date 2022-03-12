
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "Utilities.h"


class TrackListView  : public juce::Component
                     , public juce::DragAndDropTarget
                     , private juce::ChangeListener
{
public:
    TrackListView(EditViewState& evs, juce::Array<juce::Colour> tc)
        : m_editViewState (evs)
        , m_trackColours(std::move(tc))
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

    int getTrackHeight(const TrackHeaderComponent* header) const;
    void addHeaderViews(TrackHeaderComponent& th);
    void updateViews();

    void clear();

private:

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    te::AudioTrack::Ptr addTrack(bool isMidiTrack, juce::Colour trackColour);

    EditViewState& m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_trackHeaders;
    juce::Array<juce::Colour>               m_trackColours;
    const int getPopupResult();
    void collapseTracks(bool minimize);
};