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

    workflow.beginLoadBrowser();
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
    REQUIRE(workflow.confirmDiscard() == createNew);
    REQUIRE(workflow.getState() == ProjectWorkflow::State::committing);
}

void testSaveAsContinuation()
{
    ProjectWorkflow::Controller workflow;
    const ProjectWorkflow::Operation quit{ProjectWorkflow::OperationType::quit, {}};

    REQUIRE(!workflow.stageOperation(quit, true));
    workflow.beginSaveBeforePending(true);
    REQUIRE(workflow.getState() == ProjectWorkflow::State::saveProjectAs);
    REQUIRE(workflow.shouldContinueAfterSave());
    REQUIRE(workflow.locksMainInteraction());

    REQUIRE(workflow.completeSave() == quit);
    REQUIRE(workflow.getState() == ProjectWorkflow::State::committing);
    REQUIRE(!workflow.shouldContinueAfterSave());
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
} // namespace

int main()
{
    testCleanLoadExecutesImmediately();
    testDiscardContinuation();
    testSaveAsContinuation();
    testCancelClearsIntent();
    testErrorReturnsToCorrectState();

    if (failures != 0)
    {
        std::cerr << failures << " project workflow test(s) failed.\n";
        return 1;
    }

    std::cout << "All project workflow tests passed.\n";
    return 0;
}
