#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

struct ClipPropertyEdit
{
    te::Clip *clip{};
    te::ClipPosition position;
};
