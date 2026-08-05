/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#include "ProjectLifecycle.h"

namespace ProjectLifecycle
{
bool shouldProceedAfterUnsavedChoice(UnsavedChoice choice, SaveResult saveResult)
{
    switch (choice)
    {
    case UnsavedChoice::save:
        return saveResult == SaveResult::saved;
    case UnsavedChoice::discard:
        return true;
    case UnsavedChoice::cancel:
    default:
        return false;
    }
}

juce::File withProjectExtension(const juce::File &file)
{
    return file.withFileExtension(".tracktionedit");
}

bool isPersistentProjectFile(const juce::File &file)
{
    return file.getFileExtension().equalsIgnoreCase(".tracktionedit");
}

LoadFileStatus inspectLoadFile(const juce::File &file, bool allowRecoveryFile)
{
    if (!file.existsAsFile())
        return LoadFileStatus::missing;

    const bool supportedExtension = isPersistentProjectFile(file)
                                 || (allowRecoveryFile && file.getFileExtension().equalsIgnoreCase(".nextTemp"));
    if (!supportedExtension)
        return LoadFileStatus::unsupportedExtension;

    if (file.getSize() == 0)
        return LoadFileStatus::empty;

    if (auto xml = juce::parseXML(file))
        return xml->hasTagName("EDIT") ? LoadFileStatus::valid : LoadFileStatus::invalidData;

    if (juce::FileInputStream input(file); input.openedOk())
    {
        const auto state = juce::ValueTree::readFromStream(input);
        if (state.hasType(juce::Identifier("EDIT")))
            return LoadFileStatus::valid;
    }

    return LoadFileStatus::invalidData;
}

void ProjectRequestState::clear()
{
    request = {};
}

void ProjectRequestState::requestNewProject()
{
    request = {ProjectAction::newProject, {}};
}

bool ProjectRequestState::requestLoadProject(const juce::File &file)
{
    if (!file.existsAsFile() || !isPersistentProjectFile(file))
    {
        clear();
        return false;
    }

    request = {ProjectAction::loadProject, file};
    return true;
}

ProjectRequest ProjectRequestState::take()
{
    const auto result = request;
    clear();
    return result;
}

ProjectRequest ProjectRequestState::peek() const
{
    return request;
}
} // namespace ProjectLifecycle
