
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "Utilities.h"


class TrackListView  : public juce::Component
                     , public juce::DragAndDropTarget
                     , private juce::ChangeListener
                     , public juce::ApplicationCommandTarget
{
public:
    TrackListView(EditViewState& evs, LowerRangeComponent& lr)
        : m_editViewState (evs)
        , m_lowerRange(lr)
    {
        m_editViewState.m_selectionManager.addChangeListener(this);
        setWantsKeyboardFocus(true);
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

    
    ApplicationCommandTarget* getNextCommandTarget() override   { return findFirstTargetParentComponent(); }
    void getAllCommands (juce::Array<juce::CommandID>& commands) override;

    void getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    
    bool perform (const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    int getTrackHeight(TrackHeaderComponent* header) const;
    void addHeaderViews(std::unique_ptr<TrackHeaderComponent> header);
    void updateViews();

    void clear();
    LowerRangeComponent& getLowerRange()
    {
        return m_lowerRange;
    }

    void collapseTracks(bool minimize);

private:

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    te::AudioTrack::Ptr addTrack(bool isMidiTrack, bool isFolderTrack, juce::Colour trackColour);

    EditViewState& m_editViewState;
    LowerRangeComponent& m_lowerRange;
    juce::OwnedArray<TrackHeaderComponent> m_trackHeaders;
    const int getPopupResult();
    TrackHeaderComponent* getTrackHeaderView(tracktion_engine::Track::Ptr track);
};
