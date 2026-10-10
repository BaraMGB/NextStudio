#pragma once

#include <tracktion_engine/tracktion_engine.h>
#include "SelectionIdentity.h"

namespace MidiSelectionSnapshot
{
// ValueTree identity survives pointer destruction without selecting recreated notes.
using Items = juce::Array<juce::ValueTree>;
inline Items capture(const tracktion::engine::SelectedMidiEvents& selected,
                     const juce::Array<tracktion::engine::MidiClip*>& clips)
{
    Items result;
    const std::unordered_set<tracktion::engine::MidiNote*> notes(selected.getSelectedNotes().begin(), selected.getSelectedNotes().end());
    for (auto* clip : clips)
        for (auto* note : clip->getSequence().getNotes())
            if (notes.contains(note)) result.add(note->state);
    return result;
}
inline juce::Array<tracktion::engine::MidiNote*> resolve(const Items& items,
                         const juce::Array<tracktion::engine::MidiClip*>& clips)
{
    juce::Array<tracktion::engine::MidiNote*> result;
    SelectionTreeSet remaining(items.begin(), items.end());
    for (auto* clip : clips)
        for (auto* note : clip->getSequence().getNotes())
            if (remaining.erase(note->state) != 0) result.add(note);
    return result;
}
inline void apply(const Items& items, tracktion::engine::SelectedMidiEvents& selected,
                  tracktion::engine::SelectionManager& manager,
                  const juce::Array<tracktion::engine::MidiClip*>& clips)
{
    const auto notes = resolve(items, clips);
    if (selected.getClips() != clips)
    {
        selected.setSelected(manager, juce::Array<tracktion::engine::MidiNote*>{}, false);
        selected.setClips(clips);
    }
    const auto& old = selected.getSelectedNotes();
    if (notes.size() == old.size())
    {
        const std::unordered_set<tracktion::engine::MidiNote*> oldSet(old.begin(), old.end());
        bool same = true;
        for (auto* note : notes) same = same && oldSet.contains(note);
        if (same && (notes.isEmpty() || manager.isSelected(selected))) return;
    }
    selected.setSelected(manager, notes, false);
}
}
