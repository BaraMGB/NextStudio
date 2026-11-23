/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace AudioClipPlayback
{
// Match Edit::createEditForPreviewingFile for MP3s: the buffered decoder can
// play the source directly. Do not make arrangement playback depend on a
// separate cached WAV file which may be missing, unreadable or stale.
inline bool useDirectMp3Playback(tracktion_engine::WaveAudioClip &clip)
{
    if (!clip.getOriginalFile().hasFileExtension("mp3")
        || !static_cast<bool>(clip.state.getProperty(tracktion_engine::IDs::proxyAllowed, true)))
        return false;

    clip.setUsesProxy(false);
    return true;
}

inline void useDirectMp3Playback(tracktion_engine::Edit &edit)
{
    for (auto *track : tracktion_engine::getAudioTracks(edit))
        for (auto *clip : tracktion_engine::getClipsOfTypeRecursive<tracktion_engine::WaveAudioClip>(*track))
            useDirectMp3Playback(*clip);
}
} // namespace AudioClipPlayback
