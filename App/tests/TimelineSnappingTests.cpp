#include "TimelineSnapResolver.h"
#include "PianoRollDrawGesture.h"
#include "PianoRollNoteLength.h"
#include "MidiNoteGesture.h"
#include "MidiNoteCreation.h"
#include "ClipGestureLimits.h"
#include "AutomationGestureLimits.h"
#include "MouseGestureInput.h"
#include "KnifeTool.h"
#include "TimelineViewGeometry.h"
#include "TimeUtils.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace te = tracktion_engine;
namespace
{
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void near(double a, double b, double tolerance = 1.0e-8) { require(std::abs(a - b) < tolerance, "position mismatch"); }
tracktion::BeatPosition beat(double b) { return tracktion::BeatPosition::fromBeats(b); }
tracktion::TimePosition time(double t) { return tracktion::TimePosition::fromSeconds(t); }
TimelineSnapResolver fixed(const te::TempoSequence& tempo, double interval = 1, double scale = 1, double bpp = .01)
{
    return {tempo, {true, interval, {}, bpp, scale, 0}};
}
void testFixedGrid(const te::TempoSequence& tempo)
{
    for (int denominator : {1, 2, 4, 8, 16, 32, 64, 128})
        for (double raster : {1.0, 1.25, 1.5, 2.0})
        {
            const double interval = 4.0 / denominator;
            const auto resolver = fixed(tempo, interval, raster);
            const double radius = std::min(.18 / raster, .3 * interval);
            for (double target : {-2 * interval, 0.0, 2 * interval, 10000 * interval})
            {
                near(resolver.snapBeatForMouse(target), target);
                near(resolver.snapBeatForMouse(target - radius * .9), target);
                near(resolver.snapBeatForMouse(target + radius * .9), target);
                near(resolver.startAtOrBefore(target), target);
                near(resolver.startAtOrBefore(target + interval * .37), target);
                near(resolver.endAtOrAfter(target, target), target);
                near(resolver.endAtOrAfter(target + interval * .1, target), target + interval);
                require(resolver.snapBeatForMouse(target + radius * 1.1) > target, "cannot leave detent");
                require(resolver.snapBeatForMouse(target - radius * 1.1) < target, "cannot leave detent left");
            }
            double previous = -interval;
            for (int i = 0; i <= 10000; ++i)
            {
                const double raw = -interval + i * 3 * interval / 10000;
                const double output = resolver.snapBeatForMouse(raw);
                require(output >= previous - 1.0e-8, "fixed grid non-monotonic");
                near(resolver.snapBeatForMouse(resolver.rawAnchorForMouse(output)), output);
                previous = output;
            }
        }
    auto offContext = fixed(tempo).context(); offContext.enabled = false;
    const TimelineSnapResolver off(tempo, offContext);
    near(off.snapBeatForMouse(2.031), 2.031);
    near(off.startAtOrBefore(2.031), 2.031);
    near(off.endAtOrAfter(2.031, 2), 2.031);
    auto invalidContext = offContext; invalidContext.beatsPerPixel = 0;
    require(!TimelineSnapResolver(tempo, invalidContext).valid(), "zero viewport accepted");
}
void testKnifePreviewRaster(const te::TempoSequence& tempo)
{
    // Exercise the coordinate type exposed by the production MIDI Knife, not
    // just the resolver: integer storage used to truncate fractional grid X.
    using PreviewX = decltype(std::declval<const KnifeTool&>().getSplitLineX());
    for (double scale : {1.0, 1.25, 1.5, 2.0})
        for (double phase : {.137, .5, .83})
        {
            const double bpp = .01;
            const double start = phase * bpp / scale;
            const auto resolver = fixed(tempo, 1, scale, bpp);
            const double splitBeat = resolver.snapBeatForMouse(1.03);
            near(splitBeat, 1);
            const auto grid = TimelineViewGeometry::lines(start, start + 300 * bpp, 300, 0, 9, 4);
            require(!grid.empty() && grid.front().beat == splitBeat, "missing Knife grid target");
            const float gridX = static_cast<float>(grid.front().x);
            const PreviewX previewX = static_cast<PreviewX>(TimelineViewGeometry::beatToX(splitBeat, start, bpp));
            require(std::abs(previewX - gridX) < 1.0e-5, "Knife preview discarded fractional grid X");

            juce::Image image(juce::Image::RGB, 600, 40, true);
            juce::Graphics g(image);
            g.addTransform(juce::AffineTransform::scale(static_cast<float>(scale), 1));
            g.setColour(juce::Colours::white);
            g.drawLine(gridX, 0, gridX, 15, 1.0f);
            g.drawLine(static_cast<float>(previewX), 20, static_cast<float>(previewX), 35, 1.0f);
            for (int x = 0; x < image.getWidth(); ++x)
                require(image.getPixelAt(x, 10) == image.getPixelAt(x, 25), "Knife preview differs from grid pixel coverage");
        }
}
void testEditorProfiles(const te::TempoSequence& tempo)
{
    for (bool pianoRoll : {true, false})
        for (double raster : {1.0, 1.25, 1.5, 2.0})
            for (double bpp : {.1, .01})
                for (double interval : {.25, 1.0, 4.0})
                {
                    auto context = fixed(tempo, interval, raster, bpp).context();
                    context.attraction = TimelineSoftSnap::profileForEditor(pianoRoll);
                    const TimelineSnapResolver resolver(tempo, context);
                    const double radius = std::min(context.attraction.radiusPixels * bpp / raster,
                                                   context.attraction.intervalFraction * interval);
                    near(resolver.snapBeatForMouse(radius * .95), 0);
                    near(resolver.snapBeatForMouse(-radius * .95), 0);
                    require(resolver.snapBeatForMouse(radius * 1.05) > 0, "profile cannot leave detent");
                    double previous = -interval;
                    for (int i = 0; i <= 2000; ++i)
                    {
                        const double raw = -interval + 3 * interval * i / 2000;
                        const double displayed = resolver.snapBeatForMouse(raw);
                        require(displayed >= previous - 1e-8, "profile grid reversed");
                        near(resolver.snapBeatForMouse(resolver.rawAnchorForMouse(displayed)), displayed);
                        previous = displayed;
                    }
                    TimelineMouseGesture gesture;
                    gesture.begin(interval * .37, 50, resolver);
                    near(gesture.update(50, resolver, false), interval * .37); // inverse uses the same profile
                    near(gesture.update(55, resolver, true), interval * .37 + 5 * bpp);
                    auto offContext = context; offContext.enabled = false; offContext.fixedInterval = 0;
                    near(gesture.update(55, {tempo, offContext}, false), interval * .37 + 5 * bpp);
                }
    auto midiContext = fixed(tempo, 1, 1, .1).context();
    midiContext.attraction = TimelineSoftSnap::profileForEditor(true);
    auto songContext = midiContext; songContext.attraction = TimelineSoftSnap::profileForEditor(false);
    const TimelineSnapResolver midi(tempo, midiContext), song(tempo, songContext);
    near(midi.snapBeatForMouse(.35), .125); near(song.snapBeatForMouse(.35), 0);
    require(!(midiContext == songContext), "profile missing from context identity");
    TimelineMouseGesture changing;
    changing.begin(.37, 50, midi);
    const double displayed = changing.update(55, midi, false);
    near(changing.update(55, song, false), displayed); // strength/context changes re-anchor, no jump
    auto invalid = songContext; invalid.attraction.intervalFraction = .5;
    require(!TimelineSnapResolver(tempo, invalid).valid(), "overlapping detents accepted");
}
void testGesture(const te::TempoSequence& tempo)
{
    auto resolver = fixed(tempo);
    TimelineMouseGesture gesture;
    gesture.begin(2.37, 50, resolver);
    near(gesture.update(50, resolver, false), 2.37); // no off-grid grab jump
    const double forward = gesture.update(67, resolver, false);
    near(gesture.update(50, resolver, false), 2.37);
    near(gesture.update(67, resolver, false), forward);
    near(gesture.update(67, resolver, true), 2.54); // raw bypass
    near(gesture.update(67, resolver, false), forward);
    for (int i = 0; i <= 100; ++i) gesture.update(50 + i * .17, resolver, false);
    near(gesture.update(67, resolver, false), forward); // event-count independence
    auto bypassContext = resolver.context(); bypassContext.enabled = false; bypassContext.fixedInterval = 0;
    TimelineSnapResolver off(tempo, bypassContext);
    near(gesture.update(67, off, false), 2.54); // Off clears grid metadata, but NOT the raw origin
    near(gesture.update(67, resolver, false), forward);
    TimelineMouseGesture startedOff;
    startedOff.begin(2.37, 50, off); near(startedOff.update(67, off, false), 2.54);
    near(startedOff.update(67, resolver, false), forward); // inverse from original edge, no lost displacement
    TimelineMouseGesture changedGrid;
    changedGrid.begin(2.37, 50, resolver);
    near(changedGrid.update(67, off, false), 2.54);
    near(changedGrid.update(67, fixed(tempo, .25), false), 2.54); // a different grid re-anchors at current edge
    require(changedGrid.update(75, fixed(tempo, .25), false) > 2.54, "motion lost after Off -> new grid");
    auto newContext = resolver.context(); newContext.beatsPerPixel = .02; ++newContext.revision;
    TimelineSnapResolver zoomed(tempo, newContext);
    near(gesture.update(67, zoomed, false), forward); // re-anchor, not replay
    require(gesture.update(80, zoomed, false) > forward, "lost motion after zoom");
    gesture.reset(); require(!gesture.active(), "cancel retained anchor");

    PianoRollDrawGesture draw;
    require(draw.begin(2, .25, 100, resolver, false), "draw not initialized");
    near(draw.startBeat(), 2); near(draw.endBeat(), 3); // coarse snap/fine insert
    draw.update(104, resolver, false, false); near(draw.endBeat(), 3); // click jitter
    draw.update(104, resolver, false, true); near(draw.endBeat(), 3); // initial detent
    draw.update(20, resolver, false, true);
    require(draw.endBeat() < 2.25 && draw.endBeat() > 2, "insert length still acts as minimum");
    draw.update(-1000, resolver, false, true); near(draw.endBeat() - draw.startBeat(), 1.0 / 960);
    draw.update(100, resolver, false, true); near(draw.endBeat(), 3); // reverse from floor
    draw.update(220, resolver, false, true); require(draw.endBeat() > 4, "cannot continue through detents");
    draw.update(30, resolver, true, true); near(draw.endBeat(), 2.3);
    draw.update(30, resolver, false, true);
    const double magneticEnd = draw.endBeat();
    draw.update(30, off, false, true); near(draw.endBeat(), 2.3);
    draw.update(30, resolver, false, true); near(draw.endBeat(), magneticEnd);
    draw.reset(); require(!draw.active(), "draw cancel retained state");
    require(draw.begin(2, .25, 100, resolver, true), "bypassed draw failed"); near(draw.endBeat(), 2.25);
    require(draw.begin(2, .37, 100, fixed(tempo, .25), false), "fractional default failed"); near(draw.endBeat(), 2.5);
    require(draw.begin(2, 1, 100, fixed(tempo, .25), false), "fine snap failed"); near(draw.endBeat(), 3);
    auto offContext = resolver.context(); offContext.enabled = false;
    require(draw.begin(2, .37, 100, {tempo, offContext}, false), "off draw failed"); near(draw.endBeat(), 2.37);
    require(!draw.begin(NAN, .25, 100, resolver, false), "non-finite draw accepted");
}
void testAdaptive(te::Edit& edit)
{
    auto& tempo = edit.tempoSequence;
    tempo.getTempo(0)->setBpm(90);
    tempo.getTempo(0)->setCurve(.5f);
    tempo.insertTempo(beat(8), 180, 0);
    tempo.insertTempo(beat(20), 110, 0);
    auto sig = tempo.insertTimeSig(beat(12)); sig->numerator = 3; sig->denominator = 4; sig->triplets = true;
    auto sig2 = tempo.insertTimeSig(beat(24)); sig2->numerator = 7; sig2->denominator = 8; sig2->triplets = false;
    tempo.updateTempoData();
    for (int level : {4, 7, 9, 10})
    for (const auto attraction : {TimelineSoftSnap::profileForEditor(true), TimelineSoftSnap::profileForEditor(false)})
    {
        const TimelineSnapResolver resolver(tempo, {true, 0, {te::TimecodeType::barsBeats, level}, .01, 1.25, 0, 0, attraction});
        const double maxStep = .001 / (1 - 2 * attraction.intervalFraction) + 1e-6;
        double previous = resolver.snapBeatForMouse(0);
        for (int i = 0; i <= 32000; ++i)
        {
            const double raw = i / 1000.0;
            const auto targets = resolver.adjacentTargets(raw);
            require(targets.upper > targets.lower, "adaptive targets did not progress");
            const auto output = resolver.snapBeatForMouse(raw);
            require(output >= previous - 1.0e-6, "adaptive boundary discontinuity/reversal");
            if (output - previous >= maxStep)
                std::cerr << "level=" << level << " raw=" << raw << " lower=" << targets.lower << " upper=" << targets.upper
                          << " output=" << output << " previous=" << previous << '\n';
            require(output - previous < maxStep, "adaptive boundary jump");
            near(resolver.snapBeatForMouse(targets.lower), targets.lower, 1.0e-6);
            near(resolver.snapBeatForMouse(resolver.rawAnchorForMouse(output)), output, 1.0e-6);
            const auto requestedTime = time(resolver.beatToTime(raw));
            const auto expectedLower = resolver.context().snapType.roundTimeDown(requestedTime, tempo);
            if (level < 10)
                near(targets.lower, resolver.timeToBeat(expectedLower.inSeconds()), 1.0e-6);
            require(targets.lower <= raw + 1.0e-6 && targets.upper >= raw - 1.0e-6, "adaptive targets do not bracket pointer");
            const auto ceiling = resolver.endAtOrAfter(raw, raw);
            require(ceiling >= raw - 1.0e-8, "default endpoint rounded backwards");
            previous = output;
        }
    }
    const TimelineSnapResolver bars(tempo, {true, 0, {te::TimecodeType::barsBeats, 10}, .01, 1, 0});
    near(bars.adjacentTargets(27).lower, 24); // engine hard-round bug returned 31
    near(bars.adjacentTargets(27).upper, 31);
    near(bars.startAtOrBefore(27), 24);
    PianoRollDrawGesture creation;
    require(creation.begin(bars.startAtOrBefore(27), .25, 100, bars, false), "meter-change creation failed");
    near(creation.startBeat(), 24); near(creation.endBeat(), 31);
    auto irregular = tempo.insertTimeSig(beat(14.137));
    irregular->startBeatNumber = beat(14.137); // insertTimeSig initially rounds to a whole beat
    irregular->numerator = 5; irregular->triplets = false;
    tempo.updateTempoData();
    for (int level : {4, 7, 10})
    for (const auto attraction : {TimelineSoftSnap::profileForEditor(true), TimelineSoftSnap::profileForEditor(false)})
    {
        const TimelineSnapResolver resolver(tempo, {true, 0, {te::TimecodeType::barsBeats, level}, .01, 1, 0, 0, attraction});
        const double maxStep = .0001 / (1 - 2 * attraction.intervalFraction) + 1e-6;
        double previous = resolver.snapBeatForMouse(13.9);
        for (int i = 0; i <= 5000; ++i)
        {
            const double raw = 13.9 + i / 10000.0;
            const double output = resolver.snapBeatForMouse(raw);
            if (!(output >= previous - 1.0e-6 && output - previous < maxStep))
            {
                const auto targets = resolver.adjacentTargets(raw);
                std::cerr << "irregular level=" << level << " raw=" << raw << " lower=" << targets.lower << " upper=" << targets.upper
                          << " output=" << output << " previous=" << previous << '\n';
            }
            require(output >= previous - 1.0e-6 && output - previous < maxStep, "irregular meter/triplet boundary jump");
            previous = output;
        }
        near(resolver.snapBeatForMouse(14.137), 14.137, 1.0e-6);
    }
    // Non-musical adaptive time formats are projected into the beat view too.
    for (auto type : {te::TimecodeType::millisecs, te::TimecodeType::fps25})
    {
        const TimelineSnapResolver resolver(tempo, {true, 0, {type, 2}, .01, 1, 0});
        auto targets = resolver.adjacentTargets(8.123);
        require(targets.upper > targets.lower, "time-based adaptive lookup failed");
        near(resolver.snapBeatForMouse(targets.upper), targets.upper, 1.0e-6);
    }
}
void testTimeRangeProjection(const te::TempoSequence& tempo)
{
    // Exercise the production range/drop projection through tempo ramps and
    // changes, including panned and partly off-screen previews. Do not replace
    // endpoint projection with an average seconds-per-pixel or fixed width.
    for (double scale : {1.0, 1.25, 1.5, 2.0})
        for (double viewStart : {0.0, 8.137, 25.0})
        {
            const int width = static_cast<int>(1000 * scale);
            const double viewEnd = viewStart + 32;
            const double bpp = 32.0 / width;
            for (double startBeat : {0.0, 7.0, 8.0, 19.0, 24.0})
            {
                const auto start = tempo.toTime(beat(startBeat));
                const tracktion::TimeRange source(start, start + tracktion::TimeDuration::fromSeconds(1));
                for (double seconds : {0.0, .5, 2.0})
                {
                    const auto target = source + tracktion::TimeDuration::fromSeconds(seconds);
                    const auto pixels = TimeUtils::timeRangeToX(target, tempo, viewStart, viewEnd, width);
                    near(pixels.getStart(), TimelineViewGeometry::beatToX(tempo.toBeats(target.getStart()).inBeats(), viewStart, bpp), 2e-4);
                    near(pixels.getEnd(), TimelineViewGeometry::beatToX(tempo.toBeats(target.getEnd()).inBeats(), viewStart, bpp), 2e-4);
                }
            }
        }
    const auto start = tempo.toTime(beat(7));
    const tracktion::TimeRange source(start, start + tracktion::TimeDuration::fromSeconds(1));
    const auto before = TimeUtils::timeRangeToX(source, tempo, 0, 32, 1000);
    const auto after = TimeUtils::timeRangeToX(source + tracktion::TimeDuration::fromSeconds(2), tempo, 0, 32, 1000);
    require(std::abs(before.getLength() - after.getLength()) > 1, "fixture did not expose range width changes");
    const double averagedShift = 2 * 1000 / (tempo.toTime(beat(32)) - tempo.toTime(beat(0))).inSeconds();
    require(std::abs(after.getStart() - before.getStart() - averagedShift) > 1, "fixture did not expose averaged shift error");
    require(TimeUtils::timeRangeToX(source, tempo, 0, 32, 0).isEmpty(), "zero-width view accepted");
}
void testNoteModel(te::Edit& edit, te::MidiClip& clip)
{
    auto& undo = edit.getUndoManager();
    auto& sequence = clip.getSequence();
    auto* original = sequence.addNote(60, beat(0), tracktion::BeatDuration::fromBeats(4), 87, 5, nullptr);
    original->setMute(true, nullptr);
    original->state.setProperty("customAttribute", 123, nullptr);
    const auto before = sequence.state.createCopy();
    undo.clearUndoHistory();
    double lastInserted = .25;
    PianoRollDrawGesture draw;
    const auto resolver = fixed(edit.tempoSequence);
    const double base = clip.getStartBeat().inBeats() - clip.getOffsetInBeats().inBeats();
    draw.begin(base + 1, .25, 100, resolver, false);
    draw.update(30, resolver, false, true);
    require(!undo.canUndo() && sequence.state.isEquivalentTo(before), "preview mutated musical state");
    const auto previewStart = draw.startBeat() - base;
    const auto previewLength = draw.endBeat() - draw.startBeat();
    auto* created = MidiNoteCreation::add(&clip, 60, previewStart, previewLength, 99, {},
                                        [&](double actual) { lastInserted = actual; });
    require(created != nullptr, "creation failed");
    near(created->getStartBeat().inBeats(), previewStart);
    near(created->getLengthBeats().inBeats(), previewLength);
    near(lastInserted, previewLength);
    require(sequence.getNotes().size() == 3, "overlap did not split original");
    int originalPieces = 0;
    for (auto* note : sequence.getNotes())
        if (note != created)
        {
            require(note->isMute() && note->getVelocity() == 87 && int(note->state.getProperty("customAttribute")) == 123,
                    "overlap lost attributes");
            ++originalPieces;
        }
    require(originalPieces == 2, "original pieces missing");
    const auto after = sequence.state.createCopy();
    require(undo.undo(), "draw undo failed"); require(sequence.state.isEquivalentTo(before), "draw required more than one undo");
    require(!undo.canUndo(), "more than one draw transaction");
    require(undo.redo(), "draw redo failed"); require(sequence.state.isEquivalentTo(after), "fractional redo mismatch");
    undo.clearUndoHistory();
    draw.reset();
    require(!undo.canUndo() && sequence.state.isEquivalentTo(after), "cancel mutated sequence");
    require(MidiNoteCreation::add(&clip, 60, NAN, .25, 99, {}, {}) == nullptr && !undo.canUndo(), "invalid creation mutated model");
    const auto persisted = juce::ValueTree::fromXml(*sequence.state.createXml());
    require(persisted.isEquivalentTo(after), "fractional persistence mismatch");

    auto* note = sequence.addNote(67, beat(3), tracktion::BeatDuration::fromBeats(1), 78, 9, nullptr);
    juce::Array<MidiNoteGesture::Item> notes; notes.add({&clip, note});
    for (auto kind : {MidiNoteGesture::Kind::move, MidiNoteGesture::Kind::resizeLeft, MidiNoteGesture::Kind::resizeRight})
    {
        const auto delta = MidiNoteGesture::constrain(notes, kind, .07123);
        const auto timing = MidiNoteGesture::resolve(notes[0], kind, delta);
        const double originalStart = base + note->getStartBeat().inBeats();
        const double originalEnd = base + note->getEndBeat().inBeats();
        const double actualStart = base + timing.startBeat;
        const double actualEnd = actualStart + timing.lengthBeats;
        if (kind == MidiNoteGesture::Kind::resizeRight)
            near(resolver.beatToTime(actualEnd) - resolver.beatToTime(originalEnd), delta);
        else
            near(resolver.beatToTime(actualStart) - resolver.beatToTime(originalStart), delta);
        if (kind == MidiNoteGesture::Kind::move) near(timing.lengthBeats, 1);
        if (kind == MidiNoteGesture::Kind::resizeLeft) near(actualEnd, originalEnd);
    }
    auto* shortNote = sequence.addNote(68, beat(5), tracktion::BeatDuration::fromBeats(.03), 78, 9, nullptr);
    notes.add({&clip, shortNote});
    const auto floorDelta = MidiNoteGesture::constrain(notes, MidiNoteGesture::Kind::resizeRight, -100);
    near(MidiNoteGesture::resolve(notes[1], MidiNoteGesture::Kind::resizeRight, floorDelta).lengthBeats, 1.0 / 960);
    require(MidiNoteGesture::resolve(notes[0], MidiNoteGesture::Kind::resizeRight, floorDelta).lengthBeats > .9,
            "multi-resize did not use common feasible delta");
}
void testClipLimits(te::Edit& edit)
{
    auto* track = te::getAudioTracks(edit)[0];
    auto first = track->insertMIDIClip("first", {time(1), time(3)}, nullptr);
    auto second = track->insertMIDIClip("second", {time(4), time(5)}, nullptr);
    first->setOffset(tracktion::TimeDuration::fromSeconds(.5));
    juce::Array<te::Clip*> clips; clips.add(first.get()); clips.add(second.get());
    const auto before = first->state.createCopy();
    edit.getUndoManager().clearUndoHistory();
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::move, -20), -1);
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::resizeLeft, -20), 0); // second has no source offset
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::resizeRight, 20), 1); // selected-clip collision
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::resizeRight, -20), -.999999);
    require(first->state.isEquivalentTo(before) && !edit.getUndoManager().canUndo(), "clip preview mutated model");
    clips.remove(1);
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::resizeLeft, -20), -.5);
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::stretch, 1), 0); // MIDI is not stretchable
    near(ClipGestureLimits::constrain(clips, ClipGestureLimits::Kind::move, NAN), 0);
}
void testAutomationLimits(te::Edit& edit)
{
    auto* track = te::getAudioTracks(edit)[0];
    auto plugin = track->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0);
    auto* volume = dynamic_cast<te::VolumeAndPanPlugin*>(plugin.get()); require(volume != nullptr, "volume missing");
    auto* parameter = volume->volParam.get();
    auto& curve = parameter->getCurve();
    curve.addPoint(time(1), .3f, 0); curve.addPoint(time(2), .4f, 0); curve.addPoint(time(3), .5f, 0); curve.addPoint(time(5), .6f, 0);
    juce::Array<AutomationGestureLimits::Point> selected; selected.add({parameter, 1, time(2)}); selected.add({parameter, 2, time(3)});
    near(AutomationGestureLimits::constrain(selected, 100), 2);
    near(AutomationGestureLimits::constrain(selected, -100), -1);
    near(AutomationGestureLimits::constrain(selected, .07123), .07123);
    // The production lane uses this order so engine neighbour clamps do not collapse spacing.
    curve.movePoint(2, time(4), .5f, false); curve.movePoint(1, time(3), .4f, false);
    near((curve.getPointTime(2) - curve.getPointTime(1)).inSeconds(), 1);
    curve.movePoint(1, time(1), .4f, false); curve.movePoint(2, time(2), .5f, false);
    near((curve.getPointTime(2) - curve.getPointTime(1)).inSeconds(), 1);
}
void testSplitLimits()
{
    near(*MidiNoteGesture::validSplitBeat(3, 5, 3.37), 3.37);
    const double tick = PianoRollNoteLength::minimumLengthBeats;
    require(MidiNoteGesture::validSplitBeat(3, 3 + 2 * tick, 3 + tick).has_value(), "exact one-tick split rejected");
    require(!MidiNoteGesture::validSplitBeat(3, 5, 3 + tick * .5), "sub-tick left fragment");
    require(!MidiNoteGesture::validSplitBeat(3, 5, 5 - tick * .5), "sub-tick right fragment");
    require(!MidiNoteGesture::validSplitBeat(3, 5, 3) && !MidiNoteGesture::validSplitBeat(3, 5, 5), "edge split accepted");
    require(!MidiNoteGesture::validSplitBeat(5, 3, 4), "reversed split accepted");
    require(!MidiNoteGesture::validSplitBeat(3, 5, std::numeric_limits<double>::quiet_NaN()), "nonfinite split accepted");
}
void testModifierInput()
{
    juce::Component component;
    const auto source = juce::Desktop::getInstance().getMainMouseSource();
    const auto downTime = juce::Time(123456);
    const juce::MouseEvent event(source, {12.37f, 25.125f}, {juce::ModifierKeys::leftButtonModifier},
        .75f, .1f, .2f, .3f, .4f, &component, &component, juce::Time(123999), {10.125f, 25.125f}, downTime, 1, true);
    MouseGestureInput input;
    require(!input.withModifiers({juce::ModifierKeys::shiftModifier}), "inactive event replayed");
    input.remember(event);
    const auto shifted = input.withModifiers({juce::ModifierKeys::shiftModifier});
    require(shifted && shifted->mods.isShiftDown() && shifted->mods.isLeftButtonDown(), "modifier update lost button state");
    near(shifted->position.x, event.position.x); near(shifted->mouseDownPosition.x, event.mouseDownPosition.x);
    near(shifted->pressure, .75); require(shifted->mouseDownTime == downTime && shifted->mouseWasDraggedSinceMouseDown(), "modifier update lost gesture origin");
    const auto released = input.withModifiers({});
    require(released && !released->mods.isShiftDown() && released->mods.isLeftButtonDown(), "modifier offset accumulated");
    input.reset(); require(!input.withModifiers({}), "finished gesture replayed");
}
void run()
{
    te::Engine engine("NextStudioTimelineSnappingTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    testModifierInput();
    testSplitLimits();
    testFixedGrid(edit->tempoSequence);
    testKnifePreviewRaster(edit->tempoSequence);
    testEditorProfiles(edit->tempoSequence);
    testGesture(edit->tempoSequence);
    testAdaptive(*edit);
    testTimeRangeProjection(edit->tempoSequence);
    auto* track = te::getAudioTracks(*edit)[0];
    auto clip = track->insertMIDIClip("offset", {time(3), time(20)}, nullptr);
    clip->setOffset(tracktion::TimeDuration::fromSeconds(.25));
    testNoteModel(*edit, *clip);
    testClipLimits(*edit);
    testAutomationLimits(*edit);
}
}
int main()
{
    try { run(); std::cout << "All TimelineSnapping tests passed.\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
