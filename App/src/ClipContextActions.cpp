/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "ClipContextActions.h"

namespace ClipContextActions
{
RenderPlan createRenderPlan(te::Edit &edit, te::Clip::Ptr clip)
{
    RenderPlan plan;
    if (clip == nullptr || !clip->state.isAChildOf(edit.state))
        return plan;

    plan.range = clip->getEditTimeRange();
    if (plan.range.getLength().inSeconds() <= 0.0)
        return {};

    const auto trackIndex = te::getAllTracks(edit).indexOf(clip->getTrack());
    if (trackIndex < 0)
        return {};

    plan.tracksToRender.setBit(trackIndex);
    plan.clipsToRender.add(clip.get());
    return plan;
}

bool renameClip(te::Edit &edit, te::Clip::Ptr clip, const juce::String &newName)
{
    const auto trimmedName = newName.trim();
    if (clip == nullptr || trimmedName.isEmpty() || !clip->state.isAChildOf(edit.state))
        return false;

    edit.getUndoManager().beginNewTransaction("Rename Clip");
    clip->setName(trimmedName);
    return true;
}

bool deleteClip(te::Edit &edit, te::Clip::Ptr clip)
{
    if (clip == nullptr || !clip->state.isAChildOf(edit.state))
        return false;

    auto *track = clip->getTrack();
    if (track == nullptr)
        return false;

    edit.getUndoManager().beginNewTransaction("Delete Clip");
    for (auto parameter : track->getAllAutomatableParams())
        parameter->getCurve().removePointsInRegion(clip->getEditTimeRange());
    clip->removeFromParent();
    return true;
}
} // namespace ClipContextActions
