/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2025.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

==============================================================================
*/

#include "TrackInsertion.h"

namespace TrackInsertion
{
tracktion_engine::TrackInsertPoint atEndOfEdit(tracktion_engine::Edit &edit)
{
    return tracktion_engine::TrackInsertPoint::getEndOfTracks(edit);
}

tracktion_engine::TrackInsertPoint atEndOfEditExcluding(
    tracktion_engine::Edit &edit, const juce::Array<tracktion_engine::Track *> &excludedTracks)
{
    auto topLevelTracks = tracktion_engine::getTopLevelTracks(edit);
    for (int i = topLevelTracks.size(); --i >= 0;)
        if (auto *track = topLevelTracks.getUnchecked(i); track != nullptr && !excludedTracks.contains(track))
            return {nullptr, track};

    return {nullptr, nullptr};
}

tracktion_engine::TrackInsertPoint beforeTrack(tracktion_engine::Track &track)
{
    return {track, true};
}

tracktion_engine::TrackInsertPoint afterTrack(tracktion_engine::Track &track)
{
    return {track, false};
}

tracktion_engine::TrackInsertPoint atEndOfFolder(tracktion_engine::FolderTrack &folder)
{
    return {&folder, folder.getAllSubTracks(false).getLast()};
}
} // namespace TrackInsertion
