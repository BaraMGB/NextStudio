#include "EditViewState.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace te = tracktion_engine;
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
void near(double a, double b, double tolerance = 0.0002) { require(std::abs(a - b) < tolerance, "coordinate mismatch"); }
void run()
{
    juce::TemporaryFile settings;
    ApplicationViewState avs(settings.getFile());
    te::Engine engine("NextStudioTimelineViewStateTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    te::SelectionManager selection(engine);
    EditViewState evs(*edit, selection, avs);
    auto *track = te::getAudioTracks(*edit)[0];
    auto clip = track->insertMIDIClip("Alignment", {tracktion::TimePosition::fromSeconds(0), tracktion::TimePosition::fromSeconds(4)}, nullptr);
    require(clip != nullptr, "clip missing");
    auto *note = clip->getSequence().addNote(60, tracktion::BeatPosition::fromBeats(0.25), tracktion::BeatDuration::fromBeats(0.25), 100, 0, nullptr);
    require(note != nullptr, "note missing");
    auto clipBefore = clip->state.createCopy();
    auto notesBefore = clip->getSequence().state.createCopy();
    const int snapMode = evs.m_pianoRollSnapMode;
    const int snapDenominator = evs.m_pianoRollSnapDenominator;
    edit->getUndoManager().clearUndoHistory();

    for (double raster : {1.0, 1.25, 1.5, 2.0})
    {
        evs.configureTimelineViewport("SongEditor", 1200, raster);
        evs.configureTimelineViewport("PianoRollTest", 1200, raster);
        evs.applyTimelineZoom("SongEditor", {1.0 / 218.5, 2, 300});
        evs.applyTimelineZoom("PianoRollTest", {1.0 / 218.5, 2, 300});
        auto range = evs.getVisibleBeatRange("SongEditor", 1200);
        const double b = range.getLength().inBeats() / 1200;
        near(evs.beatsToX(2, "SongEditor", 1200), 300);
        auto grid = TimelineViewGeometry::lines(range.getStart().inBeats(), range.getEnd().inBeats(), 1200, 0, TimelineViewGeometry::gridLevel(b, 4), 4);
        for (const auto &line : grid)
        {
            near(evs.beatsToX(line.beat, "SongEditor", 1200), line.x);
            near(evs.timeToX(evs.beatToTime(line.beat), "SongEditor", 1200), line.x);
            near(evs.beatsToX(line.beat, "PianoRollTest", 1200), line.x);
        }
        // Real clip-relative note/clip positions through both production coordinate APIs.
        const auto noteStart = clip->getStartBeat().inBeats() + note->getStartBeat().inBeats();
        const auto noteEnd = clip->getStartBeat().inBeats() + note->getEndBeat().inBeats();
        for (double beat : {clip->getStartBeat().inBeats(), clip->getEndBeat().inBeats(), noteStart, noteEnd, 0.37, 0.78})
        {
            auto x = evs.beatsToX(beat, "SongEditor", 1200);
            near(evs.timeToX(evs.beatToTime(beat), "PianoRollTest", 1200), x);
            near(evs.xToBeats(x, 1200.0, range.getStart().inBeats(), range.getEnd().inBeats()), beat);
            near(evs.xToTime(x, 1200.0, range.getStart().inBeats(), range.getEnd().inBeats()), evs.beatToTime(beat));
        }
        // Noninteger clipped/drag-preview bounds inherit the full view's transform.
        const double left = range.getStart().inBeats() + 73.37 * b;
        near(evs.beatsToX(2, 250.75, left, left + 250.75 * b) + 73.37, evs.beatsToX(2, "SongEditor", 1200));
        auto node = evs.m_viewDataTree.getChildWithName("SongEditor");
        const double storedB = node.getProperty(IDs::beatsPerPixel);
        evs.setNewStartAndZoom("SongEditor", 0.137);
        near(double(node.getProperty(IDs::beatsPerPixel)), storedB, 1.0e-14);
        near(evs.getVisibleBeatRange("PianoRollTest", 1200).getStart().inBeats(), range.getStart().inBeats());
        const auto revision = evs.getTimelineRevision("SongEditor");
        evs.configureTimelineViewport("SongEditor", 1200, raster);
        require(evs.getTimelineRevision("SongEditor") == revision, "redundant context update");
        evs.configureTimelineViewport("SongEditor", 800, raster);
        near(double(node.getProperty(IDs::viewX)), 0.137);
        evs.fitTimelineToClip("SongEditor", 3, 8, 800);
        auto fitted = evs.getVisibleBeatRange("SongEditor", 800);
        require(fitted.getStart().inBeats() <= 3 && fitted.getEnd().inBeats() >= 11, "fit crops clip");
        evs.configureTimelineViewport("SongEditor", 800, raster); // Settle the explicit fit before restoring another view.
        // Old persisted values are normalized once the owner configures its context.
        node.setProperty(IDs::beatsPerPixel, 1.0 / 206.5, nullptr);
        node.setProperty(IDs::viewX, 0.137, nullptr);
        evs.configureTimelineViewport("SongEditor", 800, raster);
        near(double(node.getProperty(IDs::viewX)), 0.137);
        const double restoredB = node.getProperty(IDs::beatsPerPixel);
        const double spacing = TimelineViewGeometry::intervalBeats(TimelineViewGeometry::gridLevel(restoredB, 4), 4) * raster / restoredB;
        near(spacing, std::round(spacing));
        evs.setNewBeatRange("SongEditor", {tracktion::BeatPosition::fromBeats(0), tracktion::BeatPosition::fromBeats(12)}, 800);
        require(evs.getVisibleBeatRange("SongEditor", 800).getEnd().inBeats() >= 12 - 1.0e-8, "range fit crops");
        const auto previous = node.createCopy();
        evs.setNewBeatRange("SongEditor", {}, 0);
        evs.setNewStartAndZoom("SongEditor", NAN, 0);
        require(node.isEquivalentTo(previous), "invalid request changed state");
    }
    require(clip->state.isEquivalentTo(clipBefore), "clip changed by view operation");
    require(clip->getSequence().state.isEquivalentTo(notesBefore), "notes changed by view operation");
    require(int(evs.m_pianoRollSnapMode) == snapMode && int(evs.m_pianoRollSnapDenominator) == snapDenominator, "snap settings changed");
    require(!edit->getUndoManager().canUndo(), "view change polluted undo history");
    // A fit requested before the Piano Roll exists must use its eventual viewport.
    evs.fitTimelineToClip("DeferredFit", 12, 8, 880);
    evs.configureTimelineViewport("DeferredFit", 1360, 1.25);
    auto deferredRange = evs.getVisibleBeatRange("DeferredFit", 1360);
    near(deferredRange.getCentre().inBeats(), 16.0);
    require(deferredRange.getStart().inBeats() <= 12 && deferredRange.getEnd().inBeats() >= 20, "deferred fit crops");
    evs.fitTimelineToClip("CancelledFit", 12, 8, 880);
    evs.setNewStartAndZoom("CancelledFit", 2.0);
    evs.configureTimelineViewport("CancelledFit", 1360, 1.25);
    near(evs.getVisibleBeatRange("CancelledFit", 1360).getStart().inBeats(), 2.0);
    // Reopening a previously visited track must not trust its cached width.
    for (double raster : {1.0, 1.25, 1.5, 2.0})
    {
        evs.configureTimelineViewport("ReopenedFit", 1200, 1);
        evs.fitTimelineToClip("ReopenedFit", 12, 8, 600);
        evs.configureTimelineViewport("ReopenedFit", 600, raster);
        auto reopened = evs.getVisibleBeatRange("ReopenedFit", 600);
        near(reopened.getCentre().inBeats(), 16);
        require(reopened.getStart().inBeats() <= 12 && reopened.getEnd().inBeats() >= 20, "reopened fit crops clip");
        // Once the fit is settled, ordinary resizing must preserve the left beat.
        const auto revision = evs.getTimelineRevision("ReopenedFit");
        evs.configureTimelineViewport("ReopenedFit", 600, raster);
        require(evs.getTimelineRevision("ReopenedFit") == revision, "settled fit renotifies");
        evs.configureTimelineViewport("ReopenedFit", 800, raster);
        near(evs.getVisibleBeatRange("ReopenedFit", 800).getStart().inBeats(), reopened.getStart().inBeats());
    }
    // Multiple requests before layout keep only the latest target, even when
    // the first request populated a provisional context.
    evs.fitTimelineToClip("LatestFit", 12, 8, 880);
    evs.fitTimelineToClip("LatestFit", 30, 4, 880);
    evs.configureTimelineViewport("LatestFit", 600, 1.25);
    near(evs.getVisibleBeatRange("LatestFit", 600).getCentre().inBeats(), 32);
    evs.configureTimelineViewport("CancelledReopenedFit", 1200, 1);
    evs.fitTimelineToClip("CancelledReopenedFit", 12, 8, 600);
    evs.setNewStartAndZoom("CancelledReopenedFit", 2);
    evs.configureTimelineViewport("CancelledReopenedFit", 600, 1.25);
    near(evs.getVisibleBeatRange("CancelledReopenedFit", 600).getStart().inBeats(), 2);
    evs.fitTimelineToClip("ZoomCancelledFit", 12, 8, 880);
    evs.applyTimelineZoom("ZoomCancelledFit", {0.02, 3, 0});
    evs.configureTimelineViewport("ZoomCancelledFit", 600, 1.25);
    near(evs.getVisibleBeatRange("ZoomCancelledFit", 600).getStart().inBeats(), 3);
    // Range/clip fits at and beyond the user zoom limit remain visible after
    // the owner's asynchronous context refresh and a project restore.
    evs.configureTimelineViewport("LargeFit", 1200, 1);
    for (double length : {94524.0, 100240.0, 200000.0})
    {
        evs.setNewBeatRange("LargeFit", {tracktion::BeatPosition::fromBeats(0), tracktion::BeatPosition::fromBeats(length)}, 1200);
        evs.configureTimelineViewport("LargeFit", 1200, 1);
        require(evs.getVisibleBeatRange("LargeFit", 1200).getEnd().inBeats() >= length - 1.0e-8, "large range fit crops");
        evs.fitTimelineToClip("LargeFit", 12, length, 1200);
        evs.configureTimelineViewport("LargeFit", 1200, 1);
        require(evs.getVisibleBeatRange("LargeFit", 1200).getEnd().inBeats() >= 12 + length - 1.0e-8, "large clip fit crops");
        auto source = evs.m_viewDataTree.getChildWithName("LargeFit");
        auto restored = evs.m_viewDataTree.getOrCreateChildWithName("RestoredLargeFit", nullptr);
        restored.setProperty(IDs::viewX, source.getProperty(IDs::viewX), nullptr);
        restored.setProperty(IDs::beatsPerPixel, source.getProperty(IDs::beatsPerPixel), nullptr);
        evs.configureTimelineViewport("RestoredLargeFit", 1200, 1);
        require(evs.getVisibleBeatRange("RestoredLargeFit", 1200).getEnd().inBeats() >= 12 + length - 1.0e-8, "restore crops large fit");
    }
    require(clip->state.isEquivalentTo(clipBefore), "fit regression changed clip");
    require(clip->getSequence().state.isEquivalentTo(notesBefore), "fit regression changed notes");
    require(!edit->getUndoManager().canUndo(), "fit regression polluted undo history");
    // Tempo-dependent time paths still coincide with the beat grid.
    edit->tempoSequence.getTempo(0)->setBpm(93);
    edit->tempoSequence.insertTempo(tracktion::BeatPosition::fromBeats(4), 157, 0);
    for (double beat : {0.25, 4.0, 7.5})
        near(evs.timeToX(evs.beatToTime(beat), "SongEditor", 800), evs.beatsToX(beat, "SongEditor", 800));
    // Tempo changes are engine actions; isolate subsequent view updates from them.
    edit->getUndoManager().clearUndoHistory();
    evs.applyTimelineZoom("SongEditor", {1.0 / 176.5, 5, 100});
    require(!edit->getUndoManager().canUndo(), "view change polluted undo history");
}
} // namespace
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        run();
        std::cout << "Timeline state/coordinate tests passed\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
