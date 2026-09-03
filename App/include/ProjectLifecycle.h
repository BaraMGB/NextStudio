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

#include <vector>

namespace ProjectLifecycle
{
enum class SaveResult
{
    saved,
    cancelled,
    failed
};

/** Exact snapshots of ValueTree properties changed temporarily during Save As.
    Snapshots restore on scope exit unless dismiss() commits the changes.
*/
class PropertyRollback
{
public:
    PropertyRollback() = default;
    ~PropertyRollback() { restore(); }

    PropertyRollback(const PropertyRollback &) = delete;
    PropertyRollback &operator=(const PropertyRollback &) = delete;

    void capture(const juce::ValueTree &tree, const juce::Identifier &property);
    void restore();
    void dismiss() noexcept { snapshots.clear(); }
    bool isEmpty() const noexcept { return snapshots.empty(); }

private:
    struct Snapshot
    {
        juce::ValueTree tree;
        juce::Identifier property;
        juce::var value;
        bool existed{false};
    };

    std::vector<Snapshot> snapshots;
};

juce::File withProjectExtension(const juce::File &file);
juce::File normaliseSaveTarget(const juce::File &requestedFile, const juce::File &currentFile = {});
juce::String projectNameWithoutExtension(const juce::String &name);
bool isValidProjectName(const juce::String &name);
bool isPersistentProjectFile(const juce::File &file);
bool isProjectBrowserEntry(const juce::File &file);
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
} // namespace ProjectLifecycle
