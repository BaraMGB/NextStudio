# Playback graph reallocation inhibition

## Purpose

`tracktion_engine::TransportControl::ReallocationInhibitor` is a small RAII guard that temporarily prevents an active Tracktion playback graph from being regenerated.

It is intended for compound edit operations such as moving, copying, or inserting many clips. These operations can produce many model notifications that each invalidate the playback graph, although only the final model state needs a usable graph.

NextStudio currently uses the guard in:

- `App/src/ClipOverwriteCommand.cpp` — `ClipEditing::applyOverwrite()`

The implementation belongs to Tracktion Engine:

- declaration: `modules/tracktion_engine/modules/tracktion_engine/playback/tracktion_TransportControl.h`
- implementation: `modules/tracktion_engine/modules/tracktion_engine/playback/tracktion_TransportControl.cpp`

## Background: how an edit triggers graph regeneration

Track and clip mutations are stored in Tracktion's `juce::ValueTree` model. `Edit::TreeWatcher` observes changes that affect playback, including:

- adding or removing clips;
- changing clip position, length, source, speed, looping, or enabled state;
- adding or removing plug-ins;
- changing routing and side-chain properties;
- changing relevant MIDI events and automation data.

For these changes, the watcher calls:

```cpp
edit.restartPlayback();
```

`Edit::restartPlayback()` does not rebuild the graph directly. It sets `shouldRestartPlayback` and starts a short timer. In `Edit::timerCallback()`, the request is forwarded to:

```cpp
edit.getTransport().editHasChanged();
```

Without an inhibitor, `TransportControl::editHasChanged()` calls:

```cpp
ensureContextAllocated(true);
```

If a playback context exists, this forces `EditPlaybackContext::createPlayAudioNodes()` to create and install a new graph. Graph creation ultimately runs through `createNodeForEdit()` and reconstructs the applicable track, clip, routing, and plug-in nodes.

This work is synchronous and can be expensive for edits containing many clips or complex external plug-ins.

## Guard semantics

Constructing a `ReallocationInhibitor` increments an internal inhibitor counter in the transport state:

```cpp
te::TransportControl::ReallocationInhibitor inhibitor(edit.getTransport());
```

Destroying the guard decrements that counter. Guards are therefore nestable: graph regeneration remains inhibited until the last active guard is destroyed.

While the counter is greater than zero, `TransportControl::editHasChanged()` does not regenerate the graph. Instead, it records a delayed change:

```cpp
isDelayedChangePending = true;
```

`TransportControl` already has a periodic timer. While a delayed change is pending, that timer retries `editHasChanged()`. Once no inhibitor remains, the pending request is processed and the graph is regenerated from the final edit state.

The resulting sequence is:

1. construct the inhibitor;
2. perform all related model mutations;
3. model changes request playback restarts as usual;
4. attempted graph reallocations are marked as delayed;
5. destroy the inhibitor at the end of the scope;
6. the transport processes the delayed request and creates one graph from the final state.

## Current NextStudio usage

`ClipEditing::applyOverwrite()` owns the guard for the complete commit of a
planned incoming-wins edit. It covers selective victim trimming and splitting,
direct identity-preserving moves/resizes, copy or insert creation, automation,
selection updates, invariant validation, and rollback. No clip is staged at a
temporary position beyond the end of the edit.

Previously this path temporarily removed every plug-in from the affected tracks and recreated the plug-ins after editing the clips. That avoided some plug-in-related work but destroyed the live plug-in instances. For external VST3 plug-ins, a stale serialized state could then recreate the plug-in at its factory or `Init` state.

The inhibitor keeps the plug-in objects and their live processor state intact while preventing graph regeneration during the compound clip operation.

## What the inhibitor does

The inhibitor:

- delays playback graph reallocation;
- supports nested scopes through a counter;
- preserves the existing edit model while mutations are applied;
- preserves live track plug-in objects and VST3 instances;
- allows the final graph to reflect all changes made inside the scope;
- protects compound operations even if a nested message loop gives timers an opportunity to run.

The final point is important. `Edit::restartPlayback()` already coalesces ordinary synchronous changes through its timer. However, plug-in calls, modal operations, or other nested message dispatch can allow pending timers to execute before a long operation has completed. The inhibitor explicitly prevents graph creation in that situation.

## What the inhibitor does not do

`ReallocationInhibitor` is not a general-purpose edit transaction. It does **not**:

- batch `ValueTree::addChild()` calls into one atomic insertion;
- suppress `ValueTree` listener notifications;
- suppress clip creation or destruction;
- defer clip-list sorting;
- combine undo actions automatically;
- suppress selection-manager notifications;
- optimize automation searches;
- serialize or restore plug-in state;
- lock the edit model for thread-safe background mutation;
- stop the audio device or transport;
- guarantee that only one graph request is emitted.

It only prevents `TransportControl::editHasChanged()` from reallocating the graph while the guard is active. Other costs in a bulk operation remain and must be optimized independently.

## Lifetime requirements

The guard must outlive every synchronous mutation that belongs to the compound operation. Prefer a local stack object whose lexical scope clearly encloses the entire edit:

```cpp
{
    te::TransportControl::ReallocationInhibitor inhibitor(edit.getTransport());

    performFirstMutation();
    performSecondMutation();
    performFinalMutation();
}
```

Do not create it inside a per-clip loop, because each iteration would release inhibition independently:

```cpp
// Wrong for a bulk operation.
for (auto* clip : clips)
{
    te::TransportControl::ReallocationInhibitor inhibitor(edit.getTransport());
    duplicateClip(*clip);
}
```

Do not allow the guard to end before final position, routing, or selection changes have been applied.

The relevant edit operations and transport access are message-thread-oriented. The inhibitor is not permission to mutate Tracktion `ValueTree` state from a worker thread.

## Asynchronous operations

A stack guard is suitable for the synchronous `moveSelectedClips()` implementation. If a compound operation is split across asynchronous callbacks, a guard cannot simply be created in the initiating function and allowed to go out of scope before the callbacks run.

Keeping an inhibitor alive across arbitrary asynchronous work is also risky because it can prevent necessary playback updates for unrelated user actions. Prefer preparing detached data first and applying all model mutations in one short message-thread scope.

## Playback-context behaviour

If no playback context exists, `TransportControl::editHasChanged()` has no graph to reallocate and returns after clearing the delayed state. The inhibitor does not allocate a context by itself.

If a context exists, delayed regeneration uses the normal `ensureContextAllocated(true)` path. Plug-ins and tracks are represented in the new graph according to the final edit state.

The inhibitor does not stop playback. The old graph may continue to service the audio thread until Tracktion installs the replacement graph. Code inside the guarded scope must therefore avoid destroying live resources that the old graph still requires. Keeping plug-in instances alive, as the current clip-copy implementation does, is safer than clearing and recreating the plug-in list.

## Relationship to undo

`ReallocationInhibitor` is independent of `Edit::UndoTransactionInhibitor` and `UndoManager` transactions.

- `ReallocationInhibitor` controls playback graph regeneration.
- `UndoTransactionInhibitor` controls creation of new undo transaction boundaries.
- `UndoManager` records the actual model mutations.

Use the appropriate mechanism for each concern. Adding a graph inhibitor does not automatically turn multiple clip changes into one undo action.

## Performance expectations

The guard can remove repeated or premature graph builds, but it cannot make all bulk-edit costs disappear. Remaining costs include:

- copying clip `ValueTree` data;
- remapping edit item IDs;
- constructing Tracktion clip objects;
- scanning and deleting overlapping destination material;
- processing automation sections and plug-in parameters;
- updating selection state;
- building the one final playback graph.

Consequently, performance should be measured separately for:

1. automation copy/move;
2. clip state duplication;
3. destination-region deletion;
4. clip insertion and repositioning;
5. final graph creation.

The inhibitor addresses item 5 when it occurs repeatedly or before the compound operation is complete.

## Verification checklist

When changing guarded clip operations, verify that:

- duplicating one clip still creates the expected copy;
- duplicating many clips produces correct positions and selection;
- moving clips between tracks retains routing constraints;
- destination overlap deletion remains correct;
- automation follows copied or moved clips as intended;
- playback updates after the guard is destroyed;
- playback can continue during the operation without a crash;
- external VST3 plug-ins keep their current preset and parameter state;
- plug-in windows remain associated with the same live plug-in instance;
- undo and redo still restore the expected edit state.

## Relevant source paths

| Responsibility | Source |
|---|---|
| NextStudio guarded clip operation | `App/src/ClipOverwriteCommand.cpp` |
| Inhibitor declaration | `modules/tracktion_engine/modules/tracktion_engine/playback/tracktion_TransportControl.h` |
| Inhibitor counter and delayed request | `modules/tracktion_engine/modules/tracktion_engine/playback/tracktion_TransportControl.cpp` |
| Edit restart scheduling | `modules/tracktion_engine/modules/tracktion_engine/model/edit/tracktion_Edit.cpp` |
| Playback-context graph creation | `modules/tracktion_engine/modules/tracktion_engine/playback/tracktion_EditPlaybackContext.cpp` |
| Track/clip/plug-in node construction | `modules/tracktion_engine/modules/tracktion_engine/playback/graph/tracktion_EditNodeBuilder.cpp` |
