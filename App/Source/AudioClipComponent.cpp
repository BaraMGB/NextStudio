#include "AudioClipComponent.h"

AudioClipComponent::AudioClipComponent (EditViewState& evs, te::Clip::Ptr c)
    : ClipComponent (evs, std::move(c))
{
}


tracktion_engine::WaveAudioClip *AudioClipComponent::getWaveAudioClip()
{
    return dynamic_cast<te::WaveAudioClip*> (m_clip.get());
}


