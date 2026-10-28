/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

namespace te = tracktion_engine;

namespace ClipContextActions
{
struct RenderPlan
{
    tracktion::TimeRange range;
    juce::BigInteger tracksToRender;
    juce::Array<te::Clip *> clipsToRender;

    [[nodiscard]] bool isValid() const
    {
        return range.getLength().inSeconds() > 0.0
               && !tracksToRender.isZero()
               && !clipsToRender.isEmpty();
    }
};

[[nodiscard]] RenderPlan createRenderPlan(te::Edit &edit, te::Clip::Ptr clip);
[[nodiscard]] bool renameClip(te::Edit &edit, te::Clip::Ptr clip, const juce::String &newName);
[[nodiscard]] bool deleteClip(te::Edit &edit, te::Clip::Ptr clip);
} // namespace ClipContextActions
