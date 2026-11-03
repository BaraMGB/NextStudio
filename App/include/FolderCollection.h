#pragma once

#include <tracktion_engine/tracktion_engine.h>

// CollectionClips are transient TrackItems, never persistent project clips.
namespace FolderCollection
{
namespace te = tracktion::engine;
inline juce::Array<te::Clip*> clips(const te::SelectionManager& selection)
{
    return te::getClipSelectionWithCollectionClipContents(selection.getSelectedObjects()).getItemsOfType<te::Clip>();
}

inline bool valid(const te::CollectionClip& collection)
{
    auto* folder = collection.getFolderTrack();
    if (!folder || !folder->state.isAChildOf(folder->edit.state) || collection.getNumClips() == 0)
        return false;
    for (auto* clip : collection.getClips())
        if (!clip->state.isAChildOf(folder->state)) return false;
    return true;
}

inline bool selected(const te::CollectionClip& collection, const te::SelectionManager& selection,
                     const juce::Array<te::Clip*>& contents)
{
    if (selection.isSelected(&collection)) return true;
    for (auto* clip : collection.getClips())
        if (!contents.contains(clip)) return false;
    return collection.getNumClips() > 0;
}

inline void deselectContents(const te::CollectionClip& collection, te::SelectionManager& selection)
{
    te::SelectableList remaining;
    for (auto* item : selection.getSelectedObjects())
        if (dynamic_cast<te::Clip*>(item) == nullptr && dynamic_cast<te::CollectionClip*>(item) == nullptr)
            remaining.add(item);
    for (auto* clip : clips(selection))
        if (!collection.containsClip(clip)) remaining.add(clip);
    selection.select(remaining);
}

inline bool canSplitAt(const te::CollectionClip& collection, tracktion::TimePosition time)
{
    for (auto* clip : collection.getClips())
        if (time > clip->getPosition().getStart() && time < clip->getPosition().getEnd()) return true;
    return false;
}

inline te::SelectableList splitAt(te::CollectionClip& collection, tracktion::TimePosition time)
{
    te::SelectableList target;
    target.add(&collection);
    return te::splitClips(target, time);
}

// Keep the engine from replacing a grabbed collection while it is in use.
// Retain the owning tracks as well: CollectionClip itself only stores a raw Track*.
class DragGuard
{
public:
    ~DragGuard() { clear(); }
    void add(te::CollectionClip* collection)
    {
        if (!collection || m_collections.contains(collection)) return;
        m_tracks.addIfNotAlreadyThere(collection->getTrack());
        m_collections.add(collection);
        collection->setDragging(true);
        for (auto* clip : collection->getClips())
            m_sources.push_back({clip, clip->getTrack(), clip->getPosition()});
    }
    bool matches() const
    {
        for (auto* collection : m_collections)
            if (!valid(*collection)) return false;
        for (const auto& source : m_sources)
        {
            const auto position = source.clip->getPosition();
            if (source.clip->getTrack() != source.track || position.time != source.position.time
                || position.offset != source.position.offset) return false;
        }
        return true;
    }
    void clear()
    {
        for (auto* collection : m_collections)
        {
            collection->setDragging(false);
            if (auto* folder = collection->getFolderTrack()) folder->setDirtyClips();
        }
        m_sources.clear();
        m_collections.clear();
        m_tracks.clear();
    }
private:
    struct Source { te::Clip::Ptr clip; te::Track* track; te::ClipPosition position; };
    std::vector<Source> m_sources;
    te::CollectionClip::Array m_collections;
    juce::ReferenceCountedArray<te::Track> m_tracks;
};
}
