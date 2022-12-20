#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "EditViewState.h"
#include "Utilities.h"
#include "ClipComponent.h"

namespace te = tracktion_engine;

class AudioClipComponent : public ClipComponent
{
public:
    AudioClipComponent (EditViewState&, te::Clip::Ptr);
    te::WaveAudioClip* getWaveAudioClip();

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioClipComponent)
};
