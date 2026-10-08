#pragma once
#include "TimelineSnapResolver.h"
#include <functional>
#include <cmath>

// UI-only value snapshots: no model pointers, persistence or commit callbacks.
struct ClipTimingPreview
{
    tracktion::TimeRange range;
    int selectionCount = 0;
};
struct NoteTimingPreview
{
    double globalStartBeat = 0, lengthBeats = 0;
    int pitch = 60, velocity = 100, selectionCount = 0;
    bool creation = false;
};
using ClipTimingPreviewHandler = std::function<void(std::optional<ClipTimingPreview>)>;
using NoteTimingPreviewHandler = std::function<void(std::optional<NoteTimingPreview>)>;

enum class TimelineFeedbackOwner { arrangement, notes, overlay, loop };
struct TimelineInteractionFeedback
{
    TimelineSnapResult snap;
    TimelineFeedbackOwner owner;
    juce::Range<float> verticalRange;
    float markerY = 0;
};

namespace TimelineInteractionPreview
{
// Finish even after JUCE has transferred focus but before its queued focus-loss
// notification arrives. Reuse the field's normal commit/reject policy; that
// callback marks it read-only so the later notification cannot commit twice.
inline void finishTextEdit(juce::TextEditor& editor)
{
    if (!editor.isReadOnly() && editor.onFocusLost)
        editor.onFocusLost();
    if (editor.hasKeyboardFocus(true))
        editor.giveAwayKeyboardFocus();
}

// Piano Roll clip headers are later-painted siblings of its ruler.
inline bool rulerFeedbackUsesForeground(bool pianoRoll) { return pianoRoll; }
inline juce::Colour textColour(juce::Colour normal, juce::Colour accent)
{
    // Keep normal contrast rather than dimming/disabling the controls.
    const auto tinted = normal.interpolatedWith(accent, 0.22f);
    return tinted == normal ? normal.interpolatedWith(normal.contrasting(), 0.08f) : tinted;
}
inline juce::String snapLabel(const std::optional<TimelineInteractionFeedback>& feedback)
{
    if (!feedback)
        return "SNAP";
    switch (feedback->snap.state)
    {
        case TimelineSnapResult::State::held: return "SNAP\nheld";
        case TimelineSnapResult::State::free: return "SNAP\nfree";
        case TimelineSnapResult::State::bypassed: return "SNAP\nShift";
        case TimelineSnapResult::State::disabled: return "SNAP\noff";
        case TimelineSnapResult::State::limited: return "SNAP\nlimit";
        case TimelineSnapResult::State::invalid: return "SNAP\ninvalid";
    }
    return "SNAP";
}
inline void drawGuide(juce::Graphics& g, float x, juce::Range<float> vertical,
                      float markerY, juce::Colour accent, juce::Colour outline, float scale)
{
    if (!std::isfinite(x) || vertical.isEmpty() || !std::isfinite(markerY))
        return;
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(juce::Rectangle<float>(x - 6, vertical.getStart(), 12, vertical.getLength()).getSmallestIntegerContainer());
    g.setColour(accent.withAlpha(0.7f));
    g.drawLine(x, vertical.getStart(), x, vertical.getEnd(), 1.0f);
    const float radius = 4.0f / juce::jmax(1.0f, scale);
    juce::Path diamond;
    diamond.startNewSubPath(x, markerY - radius);
    diamond.lineTo(x + radius, markerY);
    diamond.lineTo(x, markerY + radius);
    diamond.lineTo(x - radius, markerY);
    diamond.closeSubPath();
    g.setColour(accent);
    g.fillPath(diamond);
    g.setColour(outline);
    g.strokePath(diamond, juce::PathStrokeType(1.0f / juce::jmax(1.0f, scale)));
}
inline void drawRulerGuide(juce::Graphics& g, const std::optional<TimelineInteractionFeedback>& feedback,
                          float x, juce::Rectangle<float> ruler, juce::Colour accent, juce::Colour outline, float scale)
{
    if (!feedback || !feedback->snap.held() || ruler.isEmpty())
        return;
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(ruler.getSmallestIntegerContainer());
    g.addTransform(juce::AffineTransform::translation(ruler.getX(), ruler.getY()));
    drawGuide(g, x, {0, ruler.getHeight()}, ruler.getHeight() - 4, accent, outline, scale);
}
}
