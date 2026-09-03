/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/
#pragma once

/** Combines independent workflows that block interaction with the main UI. */
class MainInteractionState
{
public:
    enum class Foreground
    {
        none,
        projectWorkflow,
        setupWizard
    };

    void setProjectWorkflowActive(bool active) noexcept { projectWorkflowActive = active; }
    void setSetupWizardActive(bool active) noexcept { setupWizardActive = active; }

    bool isProjectWorkflowActive() const noexcept { return projectWorkflowActive; }
    bool isSetupWizardActive() const noexcept { return setupWizardActive; }
    bool isLocked() const noexcept { return projectWorkflowActive || setupWizardActive; }

    Foreground getForeground() const noexcept
    {
        if (setupWizardActive)
            return Foreground::setupWizard;
        if (projectWorkflowActive)
            return Foreground::projectWorkflow;
        return Foreground::none;
    }

private:
    bool projectWorkflowActive{false};
    bool setupWizardActive{false};
};
