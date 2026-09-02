/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ProjectWorkflow.h"

namespace ProjectWorkflow
{
bool Operation::isValid() const noexcept
{
    return type != OperationType::none
           && (type != OperationType::load || file != juce::File());
}

bool ExecutionGuard::matches(const void *editIdentity, const juce::var &significantChange) const noexcept
{
    return editIdentity != nullptr
           && editIdentity == expectedEditIdentity
           && significantChange == expectedSignificantChange;
}

void Controller::beginSaveAs(bool preservePendingOperation)
{
    if (!preservePendingOperation)
        clearPending();
    state = State::saveProjectAs;
}

bool Controller::stageOperation(Operation operation, bool hasUnsavedChanges)
{
    pendingOperation = std::move(operation);
    continueAfterSave = false;
    state = hasUnsavedChanges ? State::confirmUnsavedChanges : State::committing;
    return state == State::committing;
}

Operation Controller::confirmDiscard()
{
    if (state != State::confirmUnsavedChanges || !pendingOperation.isValid())
        return {};

    continueAfterSave = false;
    state = State::committing;
    return pendingOperation;
}

void Controller::beginSaveBeforePending(bool saveTargetRequired)
{
    if (!pendingOperation.isValid())
        return;

    continueAfterSave = true;
    state = saveTargetRequired ? State::saveProjectAs : State::saving;
}

void Controller::markSaving()
{
    state = State::saving;
}

Operation Controller::completeSave()
{
    if (continueAfterSave && pendingOperation.isValid())
    {
        continueAfterSave = false;
        state = State::committing;
        return pendingOperation;
    }

    clearPending();
    state = State::normal;
    return {};
}

void Controller::markCommitting()
{
    state = State::committing;
}

void Controller::completeOperation()
{
    clearPending();
    state = State::normal;
}

void Controller::failSave(State retryState)
{
    clearPending();
    state = retryState;
}

void Controller::cancel()
{
    clearPending();
    state = State::normal;
}

void Controller::showError()
{
    if (state != State::operationError)
        stateBeforeError = state;
    state = State::operationError;
}

void Controller::goBackFromError()
{
    if (stateBeforeError == State::saving || stateBeforeError == State::committing)
    {
        clearPending();
        state = State::normal;
        return;
    }

    state = stateBeforeError == State::operationError || stateBeforeError == State::normal
              ? State::normal
              : stateBeforeError;
}

bool Controller::isSavePath() const noexcept
{
    if (state == State::saveProjectAs || state == State::confirmOverwrite || state == State::saving)
        return true;

    return state == State::operationError
           && (stateBeforeError == State::saveProjectAs
               || stateBeforeError == State::confirmOverwrite
               || stateBeforeError == State::saving);
}

bool Controller::locksMainInteraction() const noexcept
{
    if (isSavePath() || state == State::confirmUnsavedChanges || state == State::committing)
        return true;

    return state == State::operationError
           && stateBeforeError == State::confirmUnsavedChanges;
}

void Controller::clearPending()
{
    pendingOperation = {};
    continueAfterSave = false;
}
} // namespace ProjectWorkflow
