#include "ProjectWorkflow.h"

#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

void testCleanLoadExecutesImmediately()
{
    ProjectWorkflow::Controller workflow;
    const ProjectWorkflow::Operation load{ProjectWorkflow::OperationType::load, juce::File("/tmp/Song.tracktionedit")};

    REQUIRE(workflow.stageOperation(load, false));
    REQUIRE(workflow.getState() == ProjectWorkflow::State::committing);
    REQUIRE(workflow.locksMainInteraction());
    REQUIRE(workflow.getPendingOperation() == load);
}

void testDiscardContinuation()
{
    ProjectWorkflow::Controller workflow;
    const ProjectWorkflow::Operation createNew{ProjectWorkflow::OperationType::createNew, {}};

    REQUIRE(!workflow.stageOperation(createNew, true));
    REQUIRE(workflow.getState() == ProjectWorkflow::State::confirmUnsavedChanges);
    REQUIRE(workflow.locksMainInteraction());
    REQUIRE(workflow.confirmDiscard() == createNew);
    REQUIRE(workflow.getState() == ProjectWorkflow::State::committing);
}

void testSaveAsContinuation()
{
    const ProjectWorkflow::Operation operations[]{
        {ProjectWorkflow::OperationType::createNew, {}},
        {ProjectWorkflow::OperationType::load, juce::File("/tmp/Song.tracktionedit")},
        {ProjectWorkflow::OperationType::quit, {}}};

    for (const auto &operation : operations)
    {
        ProjectWorkflow::Controller workflow;
        REQUIRE(!workflow.stageOperation(operation, true));
        workflow.beginSaveBeforePending(true);
        REQUIRE(workflow.getState() == ProjectWorkflow::State::saveProjectAs);
        REQUIRE(workflow.shouldContinueAfterSave());
        REQUIRE(workflow.locksMainInteraction());

        REQUIRE(workflow.completeSave() == operation);
        REQUIRE(workflow.getState() == ProjectWorkflow::State::committing);
        REQUIRE(!workflow.shouldContinueAfterSave());
    }
}

void testStandaloneSaveAsCompletesNormally()
{
    ProjectWorkflow::Controller workflow;
    workflow.beginSaveAs();
    workflow.markSaving();

    REQUIRE(workflow.completeSave() == ProjectWorkflow::Operation{});
    REQUIRE(workflow.getState() == ProjectWorkflow::State::normal);
    REQUIRE(!workflow.hasPendingOperation());
}

void testCancelClearsIntent()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::load, juce::File("/tmp/Song.tracktionedit")}, true);
    workflow.beginSaveBeforePending(true);
    workflow.cancel();

    REQUIRE(workflow.getState() == ProjectWorkflow::State::normal);
    REQUIRE(!workflow.hasPendingOperation());
    REQUIRE(!workflow.locksMainInteraction());
}

void testErrorReturnsToCorrectState()
{
    ProjectWorkflow::Controller workflow;
    workflow.beginSaveAs();
    workflow.showError();

    REQUIRE(workflow.getState() == ProjectWorkflow::State::operationError);
    REQUIRE(workflow.isSavePath());
    REQUIRE(workflow.locksMainInteraction());

    workflow.goBackFromError();
    REQUIRE(workflow.getState() == ProjectWorkflow::State::saveProjectAs);
}

void testUnsavedConfirmationErrorRemainsModal()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::createNew, {}}, true);
    workflow.showError();

    REQUIRE(workflow.getState() == ProjectWorkflow::State::operationError);
    REQUIRE(workflow.getStateBeforeError() == ProjectWorkflow::State::confirmUnsavedChanges);
    REQUIRE(workflow.locksMainInteraction());

    workflow.goBackFromError();
    REQUIRE(workflow.getState() == ProjectWorkflow::State::confirmUnsavedChanges);
    REQUIRE(workflow.locksMainInteraction());
}

void testTransientErrorsCannotReturnToBusyStates()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::load, juce::File("/tmp/Song.tracktionedit")}, false);
    workflow.showError();
    workflow.goBackFromError();

    REQUIRE(workflow.getState() == ProjectWorkflow::State::normal);
    REQUIRE(!workflow.hasPendingOperation());
    REQUIRE(!workflow.locksMainInteraction());

    workflow.beginSaveAs();
    workflow.markSaving();
    workflow.showError();
    workflow.goBackFromError();

    REQUIRE(workflow.getState() == ProjectWorkflow::State::normal);
    REQUIRE(!workflow.locksMainInteraction());
}

void testSaveAsFailureAbortsContinuationBeforeRetry()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::quit, {}}, true);
    workflow.beginSaveBeforePending(true);
    REQUIRE(workflow.shouldContinueAfterSave());

    workflow.markSaving();
    workflow.failSave(ProjectWorkflow::State::saveProjectAs);
    workflow.showError();

    REQUIRE(!workflow.hasPendingOperation());
    REQUIRE(!workflow.shouldContinueAfterSave());
    workflow.goBackFromError();
    REQUIRE(workflow.getState() == ProjectWorkflow::State::saveProjectAs);

    workflow.markSaving();
    REQUIRE(workflow.completeSave() == ProjectWorkflow::Operation{});
    REQUIRE(workflow.getState() == ProjectWorkflow::State::normal);
}

void testDirectSaveFailureAbortsContinuation()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::load, juce::File("/tmp/Song.tracktionedit")}, true);
    workflow.beginSaveBeforePending(false);
    workflow.failSave(ProjectWorkflow::State::normal);
    workflow.showError();

    REQUIRE(!workflow.hasPendingOperation());
    REQUIRE(!workflow.shouldContinueAfterSave());
    REQUIRE(!workflow.locksMainInteraction());
}

void testExecutionGuardRejectsStaleEditOrChange()
{
    int firstEdit = 0;
    int replacementEdit = 0;
    const ProjectWorkflow::ExecutionGuard guard(&firstEdit, juce::var("change-1"));

    REQUIRE(guard.matches(&firstEdit, juce::var("change-1")));
    REQUIRE(!guard.matches(&replacementEdit, juce::var("change-1")));
    REQUIRE(!guard.matches(&firstEdit, juce::var("change-2")));
    REQUIRE(!guard.matches(nullptr, juce::var("change-1")));
}

void testDiscardClearsSaveContinuation()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::quit, {}}, true);
    workflow.beginSaveBeforePending(true);
    workflow.transitionTo(ProjectWorkflow::State::confirmUnsavedChanges);
    REQUIRE(workflow.confirmDiscard().type == ProjectWorkflow::OperationType::quit);
    REQUIRE(!workflow.shouldContinueAfterSave());
}

void testCompletingOperationClearsPendingIntent()
{
    ProjectWorkflow::Controller workflow;
    workflow.stageOperation({ProjectWorkflow::OperationType::load, juce::File("/tmp/Song.tracktionedit")}, false);
    workflow.completeOperation();

    REQUIRE(workflow.getState() == ProjectWorkflow::State::normal);
    REQUIRE(!workflow.hasPendingOperation());
    REQUIRE(!workflow.locksMainInteraction());
}
} // namespace

int main()
{
    testCleanLoadExecutesImmediately();
    testDiscardContinuation();
    testSaveAsContinuation();
    testStandaloneSaveAsCompletesNormally();
    testCancelClearsIntent();
    testErrorReturnsToCorrectState();
    testUnsavedConfirmationErrorRemainsModal();
    testTransientErrorsCannotReturnToBusyStates();
    testSaveAsFailureAbortsContinuationBeforeRetry();
    testDirectSaveFailureAbortsContinuation();
    testExecutionGuardRejectsStaleEditOrChange();
    testDiscardClearsSaveContinuation();
    testCompletingOperationClearsPendingIntent();

    if (failures != 0)
    {
        std::cerr << failures << " project workflow test(s) failed.\n";
        return 1;
    }

    std::cout << "All project workflow tests passed.\n";
    return 0;
}
