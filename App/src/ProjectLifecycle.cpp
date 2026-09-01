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
juce::File withProjectExtension(const juce::File &file)
{
    if (file == juce::File())
        return {};

    const auto baseName = projectNameWithoutExtension(file.getFileNameWithoutExtension());
    return file.getSiblingFile(baseName + ".tracktionedit");
}

juce::File normaliseSaveTarget(const juce::File &requestedFile, const juce::File &currentFile)
{
    if (requestedFile == currentFile && isPersistentProjectFile(currentFile))
        return currentFile;
    return withProjectExtension(requestedFile);
}

juce::String projectNameWithoutExtension(const juce::String &name)
{
    auto result = name.trim();
    constexpr auto extension = ".tracktionedit";
    while (result.endsWithIgnoreCase(extension))
        result = result.dropLastCharacters(juce::String(extension).length()).trimEnd();
    return result;
}

bool isValidProjectName(const juce::String &name)
{
    const auto normalised = projectNameWithoutExtension(name);
    if (normalised.isEmpty() || normalised == "." || normalised == ".."
        || normalised.endsWithChar('.') || normalised.endsWithChar(' '))
        return false;

    for (const auto character : normalised)
        if (character < 32 || juce::String("<>:\"/\\|?*").containsChar(character))
            return false;

    return true;
}

bool isPersistentProjectFile(const juce::File &file)
{
    return file.getFileExtension().equalsIgnoreCase(".tracktionedit");
}

bool isProjectBrowserEntry(const juce::File &file)
{
    return file.isDirectory() || isPersistentProjectFile(file);
}

bool isValidProjectTarget(const juce::File &file)
{
    if (file == juce::File() || file.isDirectory() || !isPersistentProjectFile(file)
        || !isValidProjectName(file.getFileNameWithoutExtension()))
        return false;

    const auto parent = file.getParentDirectory();
    return parent.isDirectory() && parent.hasWriteAccess()
           && (!file.existsAsFile() || file.hasWriteAccess());
}

bool shouldChooseSaveTarget(const juce::File &currentFile, bool forceSaveAs)
{
    return forceSaveAs || !isPersistentProjectFile(currentFile);
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

} // namespace ProjectLifecycle
