#include "ClipGestureLimits.h"
#include <algorithm>
#include <cmath>

namespace ClipGestureLimits
{
double constrain(const juce::Array<tracktion_engine::Clip*>& selection, Kind kind, double requested)
{
    if (!std::isfinite(requested))
        return 0;
    juce::Array<tracktion_engine::Clip*> clips;
    for (auto* clip : selection)
        if (clip != nullptr && (kind != Kind::stretch || dynamic_cast<tracktion_engine::WaveAudioClip*>(clip) != nullptr))
            clips.add(clip);
    if (clips.isEmpty())
        return 0;
    const auto minimum = tracktion::TimeDuration::fromSeconds(0.000001);
    auto delta = tracktion::TimeDuration::fromSeconds(requested);
    for (auto* clip : clips)
    {
        const auto p = clip->getPosition();
        if (kind == Kind::move)
            delta = std::max(delta, tracktion::TimePosition() - p.getStart());
        else if (kind == Kind::resizeLeft)
        {
            delta = std::max(delta, std::max(tracktion::TimePosition(), p.getStart() - p.getOffset()) - p.getStart());
            delta = std::min(delta, p.getLength() - minimum);
        }
        else
            delta = std::max(delta, minimum - p.getLength());
    }
    if (kind != Kind::move)
        for (auto* clip : clips)
            for (auto* other : clips)
            {
                if (clip == other || clip->getClipTrack() != other->getClipTrack())
                    continue;
                const auto p = clip->getPosition();
                const auto o = other->getPosition();
                if (kind == Kind::resizeLeft && delta < tracktion::TimeDuration() && o.getEnd() <= p.getStart())
                    delta = std::max(delta, o.getEnd() - p.getStart());
                else if (kind != Kind::resizeLeft && delta > tracktion::TimeDuration() && o.getStart() >= p.getEnd())
                    delta = std::min(delta, o.getStart() - p.getEnd());
            }
    return delta.inSeconds();
}
}
