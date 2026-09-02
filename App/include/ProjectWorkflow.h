/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/
#pragma once

#include <juce_core/juce_core.h>

#include <utility>

namespace ProjectWorkflow
{
enum class State
{
    normal,
    saveProjectAs,
    confirmOverwrite,
    saving,
    committing,
    operationError,
    confirmUnsavedChanges
};

enum class OperationType
{
    none,
    createNew,
    load,
    quit
};

enum class UnsavedResolution
{
    clean,
    saved,
    discarded
};

struct Operation
{
    OperationType type{OperationType::none};
    juce::File file;

    bool isValid() const noexcept;
    bool operator==(const Operation &) const = default;
};

/** Captures the edit identity and significant-change marker for deferred execution. */
class ExecutionGuard
{
public:
    ExecutionGuard(const void *editIdentity, juce::var significantChange)
        : expectedEditIdentity(editIdentity),
          expectedSignificantChange(std::move(significantChange))
    {
    }

    bool matches(const void *editIdentity, const juce::var &significantChange) const noexcept;

private:
    const void *expectedEditIdentity{};
    juce::var expectedSignificantChange;
};

/** Pure project-workflow state model.

    The controller owns pending intent and continuation state. It deliberately
    has no Component or Edit dependencies so transitions can be unit tested.
*/
class Controller
{
public:
    State getState() const noexcept { return state; }
    State getStateBeforeError() const noexcept { return stateBeforeError; }
    const Operation &getPendingOperation() const noexcept { return pendingOperation; }
    bool hasPendingOperation() const noexcept { return pendingOperation.isValid(); }
    bool shouldContinueAfterSave() const noexcept { return continueAfterSave; }

    void beginSaveAs(bool preservePendingOperation = false);
    void transitionTo(State newState) noexcept { state = newState; }

    /** Stages an operation. Returns true when it can execute immediately. */
    bool stageOperation(Operation operation, bool hasUnsavedChanges);
    Operation confirmDiscard();

    /** Starts saving before the pending operation. */
    void beginSaveBeforePending(bool saveTargetRequired);
    void markSaving();

    /** Completes saving and returns an operation that must now execute. */
    Operation completeSave();

    void markCommitting();
    void completeOperation();
    /** Clears queued intent after a failed write and selects the safe retry state. */
    void failSave(State retryState);
    void cancel();
    void showError();
    void goBackFromError();

    bool isSavePath() const noexcept;
    bool locksMainInteraction() const noexcept;

private:
    void clearPending();

    State state{State::normal};
    State stateBeforeError{State::normal};
    Operation pendingOperation;
    bool continueAfterSave{false};
};
} // namespace ProjectWorkflow
