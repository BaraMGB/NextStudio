/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "EditViewState.h"

#include <functional>
#include <vector>

namespace ClipEditing
{
enum class PlacementMode
{
    move,
    copy,
    insertState,
    insertMidi,
    insertWaveFile
};

struct Placement
{
    PlacementMode mode = PlacementMode::move;
    te::Clip::Ptr source;
    juce::ValueTree state;
    te::ClipTrack *destination = nullptr;
    te::ClipPosition finalPosition;
    te::TrackItem::Type type = te::TrackItem::Type::midi;
    juce::String name;
    juce::File sourceFile;
    std::function<bool(te::Clip &)> finalize;
};

struct SourceRemoval
{
    te::Clip::Ptr clip;
    tracktion::TimeRange range;
};

struct DestinationRemoval
{
    te::ClipTrack *track = nullptr;
    tracktion::TimeRange range;
};

struct Options
{
    juce::String undoName = "Edit clips";
    bool moveAutomation = false;
    bool copyAutomation = false;
    tracktion::TimeDuration automationOffset;
    std::vector<SourceRemoval> sourceRemovals;
    std::vector<DestinationRemoval> destinationRemovals;
};

struct Result
{
    bool succeeded = false;
    juce::String error;
    juce::Array<te::Clip::Ptr> clips;
};

Result applyOverwrite(te::Edit &, te::SelectionManager &, std::vector<Placement>, const Options & = {});
Result applyOverwrite(EditViewState &, std::vector<Placement>, const Options & = {});

bool hasOverlaps(const te::ClipTrack &);
juce::String describeOverlaps(const te::ClipTrack &);
void configureArrangementRecordingDevice(te::InputDevice &);

} // namespace ClipEditing
