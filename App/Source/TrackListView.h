
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "Utilities.h"


class TrackListView  : public juce::Component
                     , public juce::DragAndDropTarget
{
public:
    TrackListView(EditViewState& evs)
        : m_editViewState (evs)
    {
        setInterceptsMouseClicks(false, true);
    }
    ~TrackListView(){}

    void resized() override;

    inline bool isInterestedInDragSource (const SourceDetails&) override{return true;}
    void itemDragMove (const SourceDetails& dragSourceDetails) override{}
    void itemDragExit (const SourceDetails&) override{}
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;

    int getTrackHeight(const TrackHeaderComponent* header) const;
    void addHeaderViews(TrackHeaderComponent& th);
    void updateViews();

    void clear();
    int getSize();

private:

    EditViewState& m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_views;
};