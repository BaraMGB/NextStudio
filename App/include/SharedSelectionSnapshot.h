#pragma once

#include "MidiSelectionSnapshot.h"
#include "SelectableAutomationPoint.h"
#include <vector>

// Cancellation restores the shared manager, not just the active editor's objects.
// Safe references reject destroyed/recreated objects; automation guards retain
// otherwise disposable proxies, and MIDI membership uses event-tree identity.
class SharedSelectionSnapshot
{
public:
    void capture(tracktion::engine::SelectionManager& manager)
    {
        clear();
        for (auto* object : manager.getSelectedObjects())
        {
            m_objects.emplace_back(object);
            if (auto* point = dynamic_cast<SelectableAutomationPoint*>(object)) m_points.emplace_back(point);
            if (auto* events = dynamic_cast<tracktion::engine::SelectedMidiEvents*>(object))
            {
                MidiEntry entry;
                entry.owner = events;
                for (auto* clip : events->getClips()) entry.clips.emplace_back(clip);
                for (auto* note : events->getSelectedNotes()) entry.notes.add(note->state);
                for (auto* sysex : events->getSelectedSysexes()) entry.sysexes.add(sysex->state);
                for (auto* controller : events->getSelectedControllers()) entry.controllers.add(controller->state);
                m_midi.push_back(std::move(entry));
            }
        }
    }

    void restore(tracktion::engine::Edit& edit, tracktion::engine::SelectionManager& manager) const
    {
        namespace te = tracktion::engine;
        manager.deselectAll();
        const auto parameters = edit.getAllAutomatableParams(true);
        const std::unordered_set<te::AutomatableParameter*> liveParameters(parameters.begin(), parameters.end());
        std::unordered_map<te::AutomationCurve*, SelectionTreeIndex> curves;
        for (const auto& entry : m_midi)
            if (auto* events = entry.owner.get())
            {
                juce::Array<te::MidiClip*> clips;
                for (const auto& ref : entry.clips)
                    if (auto* clip = ref.get(); clip && clip->state.isAChildOf(edit.state)) clips.add(clip);
                // Clear any unowned/stale membership before changing clip sources:
                // Tracktion's setClips() otherwise calls clipForEvent on it.
                te::SelectionManager hydration(edit.engine);
                events->setSelected(hydration, juce::Array<te::MidiNote*>{}, false);
                events->setClips(clips);
                const auto notes = MidiSelectionSnapshot::resolve(entry.notes, clips);
                juce::Array<te::MidiSysexEvent*> sysexes;
                juce::Array<te::MidiControllerEvent*> controllers;
                const SelectionTreeSet sysexStates(entry.sysexes.begin(), entry.sysexes.end());
                const SelectionTreeSet controllerStates(entry.controllers.begin(), entry.controllers.end());
                for (auto* clip : clips)
                {
                    for (auto* event : clip->getSequence().getSysexEvents())
                        if (sysexStates.contains(event->state)) sysexes.add(event);
                    for (auto* event : clip->getSequence().getControllerEvents())
                        if (controllerStates.contains(event->state)) controllers.add(event);
                }
                // Hydrate through Tracktion's batch API without selectOnly()
                // destroying another captured MIDI owner or the shared objects.
                // SelectionManager's destructor removes listeners, not membership.
                events->setSelected(hydration, notes, false, true);
                events->setSelected(hydration, sysexes, false, true);
                events->setSelected(hydration, controllers, false, true);
            }
        te::SelectableList restored;
        for (const auto& ref : m_objects)
            if (auto* object = ref.get())
            {
                if (auto* track = dynamic_cast<te::Track*>(object); track && !track->state.isAChildOf(edit.state)) continue;
                if (auto* clip = dynamic_cast<te::Clip*>(object); clip && !clip->state.isAChildOf(edit.state)) continue;
                if (auto* plugin = dynamic_cast<te::Plugin*>(object); plugin && !plugin->state.isAChildOf(edit.state)) continue;
                if (auto* point = dynamic_cast<SelectableAutomationPoint*>(object))
                {
                    if (!liveParameters.contains(point->parameter.get())) continue;
                    auto [curve, inserted] = curves.try_emplace(&point->m_curve);
                    if (inserted) curve->second = indexSelectionChildren(point->m_curve.state);
                    const auto found = curve->second.find(point->pointState);
                    point->index = found == curve->second.end() ? -1 : found->second;
                    if (point->index < 0) continue;
                }
                if (auto* events = dynamic_cast<te::SelectedMidiEvents*>(object); events && events->getNumSelected() == 0) continue;
                restored.add(object);
            }
        manager.select(restored);
    }

    void clear() { m_objects.clear(); m_midi.clear(); m_points.clear(); }
private:
    struct MidiEntry
    {
        tracktion::engine::SafeSelectable<tracktion::engine::SelectedMidiEvents> owner;
        std::vector<tracktion::engine::SafeSelectable<tracktion::engine::MidiClip>> clips;
        MidiSelectionSnapshot::Items notes, sysexes, controllers;
    };
    std::vector<tracktion::engine::SafeSelectable<tracktion::engine::Selectable>> m_objects;
    std::vector<MidiEntry> m_midi;
    std::vector<SelectableAutomationPoint::Ptr> m_points;
};
