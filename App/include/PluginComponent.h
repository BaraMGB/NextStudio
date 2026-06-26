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

#include "ModifierViewComponent.h"
#include "PluginChainItemView.h"
#include "PluginViewComponent.h"
#include "PresetManagerComponent.h"
#include "ChorusPluginComponent.h"
#include "CompressorPluginComponent.h"
#include "DelayPluginComponent.h"
#include "DrumSamplerView.h"
#include "EqPluginComponent.h"
#include "FilterPluginComponent.h"
#include "PeakLimiterPluginComponent.h"
#include "PhaserPluginComponent.h"
#include "ReverbPluginComponent.h"
#include "SaturationPluginComponent.h"
#include "SoundFontPluginComponent.h"
#include "SpectrumAnalyzerPluginComponent.h"
#include "VstPluginComponent.h"
#include "VolumePluginComponent.h"
#include "EditViewState.h"

// If there are any shared utility classes left here, they can remain or be moved to a util file.
