/*
  ==============================================================================

    PluginComponent.h
    Created: 31 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <tracktion_engine/tracktion_engine.h>

// This file is now mostly a forwarder or aggregator, as components have been moved to their own files.
// We keep it for now to avoid breaking too many includes if they rely on it,
// but ideally, usage should be replaced by including specific headers.

#include "LowerRange/PluginChain/ModifierViewComponent.h"
#include "LowerRange/PluginChain/PluginViewComponent.h"
#include "LowerRange/PluginChain/PresetManagerComponent.h"
#include "LowerRange/PluginChain/RackItemView.h"
#include "Plugins/Chorus/ChorusPluginComponent.h"
#include "Plugins/Compressor/CompressorPluginComponent.h"
#include "Plugins/Delay/DelayPluginComponent.h"
#include "Plugins/DrumSampler/DrumSamplerView.h"
#include "Plugins/EQ/EqPluginComponent.h"
#include "Plugins/Filter/FilterPluginComponent.h"
#include "Plugins/FourOscPlugin/FourOscPluginComponent.h"
#include "Plugins/Reverb/ReverbPluginComponent.h"
#include "Plugins/SpectrumAnalyzer/SpectrumAnalyzerPluginComponent.h"
#include "Plugins/VST/VstPluginComponent.h"
#include "Plugins/Volume/VolumePluginComponent.h"
#include "Utilities/EditViewState.h"

// If there are any shared utility classes left here, they can remain or be moved to a util file.
