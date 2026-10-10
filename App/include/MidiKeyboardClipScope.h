#pragma once

#include "SelectionIdentity.h"
#include <tracktion_engine/tracktion_engine.h>

// Editor targeting context, not object-selection membership. Note selection may
// replace the shared manager's clips; that must not change the keyboard's scope.
class MidiKeyboardClipScope
{
public:
    void remember(const juce::Array<tracktion::engine::MidiClip *> &explicitlySelectedClips)
    {
        if (explicitlySelectedClips.isEmpty())
            return;
        m_clipStates.clear();
        for (auto *clip : explicitlySelectedClips)
            m_clipStates.addIfNotAlreadyThere(clip->state);
    }

    void clear() { m_clipStates.clear(); }

    juce::Array<tracktion::engine::MidiClip *> resolve(const juce::Array<tracktion::engine::MidiClip *> &liveTrackClips) const
    {
        juce::Array<tracktion::engine::MidiClip *> result;
        SelectionTreeSet remaining(m_clipStates.begin(), m_clipStates.end());
        for (auto *clip : liveTrackClips)
            if (remaining.erase(clip->state) != 0)
                result.add(clip);
        return result;
    }

private:
    juce::Array<juce::ValueTree> m_clipStates;
};
