#include "MainInteractionState.h"

#include <iostream>

namespace
{
int failures = 0;

#define REQUIRE(condition) \
    do { if (!(condition)) { std::cerr << "FAIL: " << #condition << " (line " << __LINE__ << ")\n"; ++failures; } } while (false)

void testSetupWizardLocksMainInteraction()
{
    MainInteractionState state;
    state.setSetupWizardActive(true);

    REQUIRE(state.isLocked());
    REQUIRE(state.isSetupWizardActive());
    REQUIRE(!state.isProjectWorkflowActive());
    REQUIRE(state.getForeground() == MainInteractionState::Foreground::setupWizard);

    state.setSetupWizardActive(false);
    REQUIRE(!state.isLocked());
    REQUIRE(state.getForeground() == MainInteractionState::Foreground::none);
}

void testSetupWizardHasForegroundPriority()
{
    MainInteractionState state;
    state.setProjectWorkflowActive(true);
    REQUIRE(state.getForeground() == MainInteractionState::Foreground::projectWorkflow);

    state.setSetupWizardActive(true);
    REQUIRE(state.isLocked());
    REQUIRE(state.getForeground() == MainInteractionState::Foreground::setupWizard);

    state.setProjectWorkflowActive(false);
    REQUIRE(state.isLocked());
    REQUIRE(state.getForeground() == MainInteractionState::Foreground::setupWizard);

    state.setSetupWizardActive(false);
    REQUIRE(!state.isLocked());
    REQUIRE(state.getForeground() == MainInteractionState::Foreground::none);
}
} // namespace

int main()
{
    testSetupWizardLocksMainInteraction();
    testSetupWizardHasForegroundPriority();

    if (failures != 0)
    {
        std::cerr << failures << " main interaction state test(s) failed.\n";
        return 1;
    }

    std::cout << "All main interaction state tests passed.\n";
    return 0;
}
