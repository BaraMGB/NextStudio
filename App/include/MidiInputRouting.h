/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#pragma once

#include <tracktion_engine/tracktion_engine.h>

namespace MidiInputRouting
{
namespace te = tracktion::engine;

struct UpdateResult
{
    bool routingChanged{};
    bool metadataChanged{};
    juce::String error;

    void merge(const UpdateResult &other)
    {
        routingChanged = routingChanged || other.routingChanged;
        metadataChanged = metadataChanged || other.metadataChanged;

        if (other.error.isNotEmpty())
            error = error.isEmpty() ? other.error : error + "\n" + other.error;
    }
};

bool isAutomaticFocusTarget(const te::InputDeviceInstance &instance, te::EditItemID targetID);
bool isManualTarget(const te::InputDeviceInstance &instance, te::EditItemID targetID);

/** Resolves one focus target from the current selection. Directly selected MIDI
    tracks take precedence; selected clips provide the fallback used when a clip
    click replaced the track selection.
*/
te::EditItemID findMidiFocusTarget(const juce::Array<te::Track *> &selectedTracks,
                                   const juce::Array<te::Clip *> &selectedClips,
                                   const juce::Identifier &midiTrackProperty);

/** Adds/removes a persistent, user-selected input target.
    Enabling an existing automatic target promotes it to a manual target without
    changing the playback graph. Disabling never removes an automatic target.
*/
UpdateResult setManualTarget(te::InputDeviceInstance &instance,
                             te::EditItemID targetID,
                             bool shouldBeManual,
                             juce::UndoManager *undoManager);

/** Reconciles only targets created by automatic MIDI focus.
    Manual targets on all devices are preserved. Devices in
    devicesYieldingToManualTargets receive no automatic target while another
    enabled input is manually assigned to the focused track. Pass the same
    UndoManager as a related manual edit to make the complete routing transition
    one atomic undo transaction; automatic focus updates otherwise pass nullptr.
*/
UpdateResult reconcileAutomaticFocus(te::Edit &edit,
                                     const juce::Array<te::InputDevice *> &focusDevices,
                                     te::EditItemID focusedTrackID = {},
                                     const juce::Array<te::InputDevice *> &devicesYieldingToManualTargets = {},
                                     juce::UndoManager *undoManager = nullptr);

/** Removes all targets carrying the automatic-focus marker. */
UpdateResult clearAutomaticFocus(te::Edit &edit);

/** One-time migration for projects written before automatic and manual targets
    were distinguishable. Existing targets of the current focus devices are
    cleared; all other device assignments remain manual and untouched.
*/
UpdateResult migrateLegacyFocusTargets(te::Edit &edit,
                                       const juce::Array<te::InputDevice *> &focusDevices);

} // namespace MidiInputRouting
