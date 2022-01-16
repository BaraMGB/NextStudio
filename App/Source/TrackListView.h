
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "TrackHeadComponent.h"
#include "Utilities.h"


class TrackListView  : public juce::Component
                     , public juce::DragAndDropTarget
{
public:
    TrackListView(EditViewState& evs)
        : m_editViewState (evs){}
    ~TrackListView(){}

    int getTrackHeight(const TrackHeaderComponent* header) const
    {
        return GUIHelpers::getTrackHeight(
            EngineHelpers::getAudioTrack(
                header->getTrack(), m_editViewState)
                , m_editViewState, true);
    }
    void resized()
    {
        int y = juce::roundToInt (m_editViewState.m_viewY.get());
        int allTracksHeight = 0;
        for (auto header : m_views)
        {
            auto trackHeaderHeight = getTrackHeight(header);

            header->setBounds (0, y, getWidth(), trackHeaderHeight);

            y += trackHeaderHeight;
            allTracksHeight += trackHeaderHeight;
        }
    }
    inline bool isInterestedInDragSource (const SourceDetails&) override{return true;}
    void itemDragMove (const SourceDetails& dragSourceDetails) override{}
    void itemDragExit (const SourceDetails&) override{}
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
    {
        if (dragSourceDetails.description == "Track")
        {
            if (auto tc = dynamic_cast<TrackHeaderComponent*>(dragSourceDetails.sourceComponent.get ()))
            {

                m_editViewState.m_edit.moveTrack (
                    tc->getTrack ()
                        , { nullptr
                     , m_editViewState.m_edit.getTrackList ().at
                     (m_editViewState.m_edit.getTrackList ().size ()-1)});
            }
        }
    }

    void addHeaderViews(TrackHeaderComponent& th)
    {
        m_views.add(&th);
    }

    void updateViews()
    {
        for (auto v : m_views)
        {
            addAndMakeVisible(v);
        }
        resized();
    }

    const TrackHeaderComponent& getHeaderView(te::Track::Ptr track)
    {
        for (auto header : m_views)
        {
            if (header->getTrack() == track)
            {
                return *header;
            }
        }
    }

    void clear()
    {
        m_views.clear(true);
        resized();
    }
    int getSize()
    {
        return m_views.size();
    }
private:
    EditViewState& m_editViewState;
    juce::OwnedArray<TrackHeaderComponent> m_views;
};