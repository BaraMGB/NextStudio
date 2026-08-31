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
#include <juce_data_structures/juce_data_structures.h>

namespace ProjectLifecycle
{
enum class SaveResult
{
    saved,
    cancelled,
    failed
};

enum class UnsavedChoice
{
    save,
    discard,
    cancel
};

bool shouldProceedAfterUnsavedChoice(UnsavedChoice choice, SaveResult saveResult);

juce::File withProjectExtension(const juce::File &file);
juce::String projectNameWithoutExtension(const juce::String &name);
bool isValidProjectName(const juce::String &name);
bool isPersistentProjectFile(const juce::File &file);
bool isValidProjectTarget(const juce::File &file);
bool shouldChooseSaveTarget(const juce::File &currentFile, bool forceSaveAs);

enum class LoadFileStatus
{
    valid,
    missing,
    unsupportedExtension,
    empty,
    invalidData
};

LoadFileStatus inspectLoadFile(const juce::File &file, bool allowRecoveryFile);

enum class ProjectAction
{
    none,
    newProject,
    loadProject
};

struct ProjectRequest
{
    ProjectAction action{ProjectAction::none};
    juce::File file;
    bool unsavedChangesHandled{false};
};

class ProjectRequestState
{
public:
    void clear();
    void requestNewProject();
    bool requestLoadProject(const juce::File &file, bool unsavedChangesHandled = false);
    ProjectRequest take();
    ProjectRequest peek() const;

private:
    ProjectRequest request;
};
} // namespace ProjectLifecycle
