#include "LassoSelectionComponent.h"
#include "MidiKeyboardClipScope.h"
#include "MidiSelectionSnapshot.h"
#include "MouseGestureInput.h"
#include "SelectionGestures.h"
#include "SharedSelectionSnapshot.h"
#include "VelocityMarkerGeometry.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace te = tracktion::engine;
namespace
{
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void near(double a, double b) { require(std::abs(a - b) < 1.0e-5, "projection mismatch"); }

void geometry()
{
    LassoGesture lasso;
    TimeRangeGesture range;
    require(!lasso.active() && !range.active(), "initial active state");
    lasso.begin({8.25, 60.5});
    lasso.update({3.5, 55.25});
    require(lasso.bounds() == juce::Rectangle<double>(3.5, 55.25, 4.75, 5.25), "reverse rectangle");
    // Floating musical anchor survives a changed view mapping, not a rebased MouseEvent.
    double startBeat = 0, beatsPerPixel = 0.02, pitchStart = 50, pitchHeight = 20;
    auto project = [&](juce::Point<double> p)
    { return juce::Point<float>{float((p.x - startBeat) / beatsPerPixel), float(300 - (p.y - pitchStart) * pitchHeight)}; };
    auto old = lasso.viewBounds(project);
    near(old.getRight(), 412.5);
    startBeat = 2; beatsPerPixel = 0.025; pitchStart = 52; pitchHeight = 25;
    auto changed = lasso.viewBounds(project);
    near(changed.getRight(), 250);
    near(lasso.anchor().x, 8.25);
    near(lasso.anchor().y, 60.5);
    // Standing pointer: only the endpoint is inverted using the new view.
    lasso.update({startBeat + 200 * beatsPerPixel, pitchStart + (300 - 150) / pitchHeight});
    changed = lasso.viewBounds(project);
    near(changed.getX(), 200);
    near(changed.getRight(), 250);
    near(changed.getBottom(), 150);

    for (float scale : {1.0f, 1.25f, 1.5f, 2.0f})
    {
        const juce::Rectangle<float> rect{10.25f * scale, 20.5f * scale, 30.75f * scale, 40.25f * scale};
        require(LassoGesture::containsCentre(rect, rect.getTopLeft()), "top/left boundary");
        require(LassoGesture::containsCentre(rect, rect.getBottomRight()), "bottom/right boundary");
        require(!LassoGesture::containsCentre(rect, rect.getBottomRight() + juce::Point<float>{0.01f, 0}), "outside boundary");
    }
    require(!LassoGesture::containsCentre({}, {0, 0}), "empty point rectangle");
    range.begin(12.125, 2.25);
    range.update(8.5, 0.75);
    require(range.interval() == juce::Range<double>(8.5, 12.125), "resolved range lost precision");
    require(range.lanes() == juce::Range<double>(0.75, 2.25), "range lane bounds");
    lasso.end();
    require(range.active(), "ending lasso ended range");
    lasso.update({999,999});
    require(!lasso.active(), "late update restarted lasso");
    range.end();
    range.update(999,999);
    require(!range.active(), "late update restarted range");

    LassoSelectionComponent component;
    component.setViewBounds({0, 0, 500, 300});
    component.setProjection(project);
    component.begin({8.25,60.5});
    component.update({7,58});
    const auto rect = component.viewBounds();
    near(rect.getRight(),250);
    component.end();
    require(!component.active(), "component did not end");
}

void velocityGeometry()
{
    using namespace VelocityMarkerGeometry;
    for (int height : {0, 8, 60, 100, 125, 150, 200})
        for (int velocity : {0, 32, 80, 112, 127})
        {
            const auto y = markerY(velocity, height);
            near(projectVelocity(velocityAt(y, height), height), y);
            near(markerY(0, height), height - 4);
            near(markerY(127, height), height - 4 - span(height));
        }
    LassoGesture lasso;
    lasso.begin({5.25, 112.5});
    lasso.update({1.25, 31.5});
    int height = 100;
    auto project = [&](juce::Point<double> p) { return juce::Point<float>{float(p.x * 100), projectVelocity(p.y, height)}; };
    require(LassoGesture::containsCentre(lasso.viewBounds(project), {300, markerY(80, height)}), "velocity head missed");
    require(!LassoGesture::containsCentre(lasso.viewBounds(project), {300, markerY(10, height)}), "stem selected without head");
    height = 200;
    require(LassoGesture::containsCentre(lasso.viewBounds(project), {300, markerY(80, height)}), "velocity resize lost anchor");
    lasso.update({1.5, 100});
    require(!LassoGesture::containsCentre(lasso.viewBounds(project), {300, markerY(80, height)}), "velocity shrink retained head");
}

void selectionPolicy()
{
    const juce::Array<int> initial{1,2};
    const juce::Array<int> expanded{2,3,3,4}, shrunk{3};
    auto add = combineLassoSelection(initial, expanded, LassoSelectionMode::add);
    require(add.size() == 4, "union contains duplicates");
    add = combineLassoSelection(initial, shrunk, LassoSelectionMode::add);
    require(add.size() == 3 && !add.contains(4), "shrink retained transient hit");
    const auto toggle = combineLassoSelection(initial, expanded, LassoSelectionMode::toggle);
    require(toggle.size() == 3 && toggle.contains(1) && !toggle.contains(2) && toggle.contains(3), "xor policy");
    require(combineLassoSelection(initial, shrunk, LassoSelectionMode::replace) == shrunk, "replace policy");
    require(combineLassoSelection(initial, juce::Array<int>{}, LassoSelectionMode::toggle) == initial, "empty toggle");
    require(lassoSelectionMode({}) == LassoSelectionMode::replace, "default modifier");
    require(lassoSelectionMode(juce::ModifierKeys::shiftModifier) == LassoSelectionMode::add, "Shift policy");
    require(lassoSelectionMode(juce::ModifierKeys::ctrlModifier) == LassoSelectionMode::toggle, "Ctrl policy");
    require(lassoSelectionMode(juce::ModifierKeys::commandModifier) == LassoSelectionMode::toggle, "Command policy");
    require(lassoSelectionMode(juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier) == LassoSelectionMode::add, "Shift precedence");
}

void inputOwnership()
{
    juce::Component owner, lane;
    owner.addChildComponent(lane);
    lane.setBounds(20, 30, 100, 100);
    auto event = [&](juce::int64 downTime, bool dragged)
    {
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), {40.5f, 50.5f},
            juce::ModifierKeys::leftButtonModifier, 0.5f, 0, 0, 0, 0, &lane, &lane,
            juce::Time(downTime + 10), {10.25f, 20.5f}, juce::Time(downTime), 1, dragged);
    };
    const auto selection = event(1000, true), edit = event(2000, true);
    MouseGestureInput input;
    MouseGestureCancellation canceled;
    require(!input.belongsToGesture(selection), "unowned input accepted");
    input.remember(selection);
    require(input.belongsToGesture(selection.getEventRelativeTo(&owner)), "relative event lost gesture ownership");
    require(input.belongsToGesture(*input.withModifiers(juce::ModifierKeys::shiftModifier)), "modifier replay lost ownership");
    require(input.belongsToGesture(*input.forContext(owner, {})), "context replay lost ownership");
    canceled.cancel(input);
    // An old selection abort must not consume the subsequent clip/range release.
    require(!canceled.consume(edit), "selection cancellation swallowed edit commit");
    input.remember(edit);
    require(!input.belongsToGesture(selection), "late selection event accepted by new edit");
    require(input.belongsToGesture(edit), "new edit lost ownership");
    require(canceled.consume(selection), "canceled selection release was not suppressed");
    require(!canceled.consume(edit), "consumed cancellation affected new edit");
    input.reset();
    require(!input.belongsToGesture(edit), "completed input accepted late release");
    require(!input.withModifiers({}), "completed input replayed");
    input.remember(edit);
    canceled.cancel(input);
    require(canceled.consume(edit), "second gesture could not be canceled");
    require(!canceled.consume(edit), "cancellation consumed twice");

    // MIDI keeps ownership separate from its hover/replay cache: hover after an
    // abort must not make that canceled press eligible for a drag or release.
    MouseGestureInput press, hover;
    press.remember(selection);
    hover.remember(selection);
    press.reset();
    hover.reset();
    hover.remember(selection);
    require(!press.belongsToGesture(selection), "hover revived a canceled MIDI press");
    press.remember(edit);
    require(!press.belongsToGesture(selection) && press.belongsToGesture(edit), "superseded MIDI press was accepted");
    press.reset();
    require(!press.belongsToGesture(edit), "duplicate MIDI release remained eligible");
}

void realSelection()
{
    te::Engine engine("NextStudioSelectionGestureTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    te::SelectionManager manager(engine);
    auto* track = te::getAudioTracks(*edit)[0];
    auto clip = track->insertMIDIClip("Selection", {tracktion::TimePosition::fromSeconds(2), tracktion::TimePosition::fromSeconds(6)}, nullptr);
    auto second = track->insertMIDIClip("Second", {tracktion::TimePosition::fromSeconds(7), tracktion::TimePosition::fromSeconds(10)}, nullptr);
    auto* a = clip->getSequence().addNote(60, tracktion::BeatPosition::fromBeats(1), tracktion::BeatDuration::fromBeats(1), 32, 0, nullptr);
    auto* b = second->getSequence().addNote(64, tracktion::BeatPosition::fromBeats(2), tracktion::BeatDuration::fromBeats(1), 80, 0, nullptr);
    const juce::Array<te::MidiClip*> clips{clip.get(), second.get()};
    te::SelectedMidiEvents selected(clips);
    selected.setSelected(manager, juce::Array<te::MidiNote*>{a}, false);
    auto original = MidiSelectionSnapshot::capture(selected, clips);
    auto aState = a->state, bState = b->state;
    auto model = track->state.createCopy();
    edit->getUndoManager().clearUndoHistory();
    auto combined = combineLassoSelection(original, MidiSelectionSnapshot::Items{bState}, LassoSelectionMode::add);
    MidiSelectionSnapshot::apply(combined, selected, manager, clips);
    require(selected.getSelectedNotes().size() == 2 && selected.isSelected(a) && selected.isSelected(b), "shared batch selection");
    MidiSelectionSnapshot::apply(original, selected, manager, clips);
    require(selected.getSelectedNotes().size() == 1 && selected.isSelected(a), "cancel snapshot restore");
    require(track->state.isEquivalentTo(model), "selection changed musical state");
    require(!edit->getUndoManager().canUndo(), "selection polluted undo");

    // Removed and identical-looking replacement notes are not the original identity.
    selected.setSelected(manager, juce::Array<te::MidiNote*>{}, false);
    clip->getSequence().removeNote(*a, nullptr);
    auto* replacement = clip->getSequence().addNote(60, tracktion::BeatPosition::fromBeats(1), tracktion::BeatDuration::fromBeats(1), 32, 0, nullptr);
    require(replacement->state != aState, "replacement reused state");
    MidiSelectionSnapshot::apply(original, selected, manager, clips);
    require(selected.getSelectedNotes().isEmpty(), "restoration selected recreated note");
    MidiSelectionSnapshot::apply({bState,bState}, selected, manager, clips);
    require(selected.getSelectedNotes().size() == 1, "snapshot duplicate selected twice");
    // A changed clip set must not pass stale notes to Tracktion's clipForEvent assertion.
    MidiSelectionSnapshot::apply({bState}, selected, manager, {clip.get()});
    require(selected.getSelectedNotes().isEmpty(), "removed clip survived resolution");
    selected.deselect();
}

void keyboardClipScope()
{
    te::Engine engine("NextStudioKeyboardClipScopeTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    te::SelectionManager manager(engine);
    auto *track = te::getAudioTracks(*edit)[0];
    auto first = track->insertMIDIClip("First", {tracktion::TimePosition::fromSeconds(2), tracktion::TimePosition::fromSeconds(6)}, nullptr);
    auto second = track->insertMIDIClip("Second", {tracktion::TimePosition::fromSeconds(8), tracktion::TimePosition::fromSeconds(10)}, nullptr);
    auto *note = first->getSequence().addNote(53, tracktion::BeatPosition::fromBeats(1), tracktion::BeatDuration::fromBeats(1), 80, 0, nullptr);
    const juce::Array<te::MidiClip *> live{first.get(), second.get()};
    MidiKeyboardClipScope scope;
    require(scope.resolve(live).isEmpty(), "keyboard invented an explicit clip target");
    scope.remember({first.get()});
    te::SelectedMidiEvents selected(live);
    auto model = track->state.createCopy();
    edit->getUndoManager().clearUndoHistory();
    manager.selectOnly(first.get());
    MidiSelectionSnapshot::apply({note->state}, selected, manager, live);
    require(manager.getItemsOfType<te::MidiClip>().isEmpty(), "fixture retained shared clip selection");
    scope.remember({});
    require(scope.resolve(live) == juce::Array<te::MidiClip *>{first.get()}, "note selection lost keyboard scope or expanded to siblings");
    MidiSelectionSnapshot::apply({}, selected, manager, live);
    scope.remember({});
    require(scope.resolve(live) == juce::Array<te::MidiClip *>{first.get()}, "empty membership lost keyboard scope");
    scope.remember({second.get()});
    require(scope.resolve(live) == juce::Array<te::MidiClip *>{second.get()}, "explicit clip did not replace keyboard scope");
    scope.remember({first.get(), second.get(), first.get()});
    require(scope.resolve(live) == live, "multi-clip keyboard scope duplicated or omitted a clip");
    require(scope.resolve({second.get()}) == juce::Array<te::MidiClip *>{second.get()}, "keyboard retained detached/wrong-track clip");
    require(track->state.isEquivalentTo(model) && !edit->getUndoManager().canUndo(), "keyboard scope changed model/undo");
    selected.deselect();
    scope.remember({first.get()});
    first->removeFromParent();
    auto replacement = track->insertMIDIClip("First", {tracktion::TimePosition::fromSeconds(2), tracktion::TimePosition::fromSeconds(6)}, nullptr);
    require(scope.resolve({replacement.get(), second.get()}).isEmpty(), "keyboard targeted a recreated clip");
    scope.remember({second.get()});
    scope.clear();
    require(scope.resolve({second.get()}).isEmpty(), "track teardown retained keyboard scope");
}

void sharedSelection()
{
    te::Engine engine("NextStudioSharedSelectionTests");
    auto edit = te::Edit::createSingleTrackEdit(engine);
    te::SelectionManager manager(engine);
    auto* track = te::getAudioTracks(*edit)[0];
    auto clip = track->insertMIDIClip("Shared", {tracktion::TimePosition::fromSeconds(2), tracktion::TimePosition::fromSeconds(6)}, nullptr);
    auto* note = clip->getSequence().addNote(60, tracktion::BeatPosition::fromBeats(1), tracktion::BeatDuration::fromBeats(1), 80, 0, nullptr);
    auto* controller = clip->getSequence().addControllerEvent(tracktion::BeatPosition::fromBeats(1), 1, 64, nullptr);
    const juce::Array<te::MidiClip*> sourceClips{clip.get()};
    te::SelectedMidiEvents first(sourceClips), second(sourceClips);
    // Two event owners plus a clip exercise batch hydration without selectOnly
    // clearing a previously hydrated owner or the editor-external objects.
    first.addSelectedEvent(note, false);
    second.addSelectedEvent(controller, false);
    manager.select({&first, &second, clip.get()});
    SharedSelectionSnapshot snapshot;
    snapshot.capture(manager);
    auto model = track->state.createCopy();
    edit->getUndoManager().clearUndoHistory();
    manager.selectOnly(track);
    snapshot.restore(*edit, manager);
    require(manager.isSelected(first) && manager.isSelected(second) && manager.isSelected(*clip), "shared objects were not restored");
    require(first.isSelected(note) && second.getSelectedControllers().contains(controller), "MIDI membership was lost on restore");
    require(track->state.isEquivalentTo(model) && !edit->getUndoManager().canUndo(), "shared restoration changed model/undo");

    // Never substitute an identical-looking note for a removed identity.
    manager.deselectAll();
    clip->getSequence().removeNote(*note, nullptr);
    clip->getSequence().addNote(60, tracktion::BeatPosition::fromBeats(1), tracktion::BeatDuration::fromBeats(1), 80, 0, nullptr);
    snapshot.restore(*edit, manager);
    require(!manager.isSelected(first) && first.getSelectedNotes().isEmpty(), "restored a replacement MIDI note");
    require(manager.isSelected(second) && manager.isSelected(*clip), "valid shared objects were discarded");
    snapshot.clear();

    auto plugin = track->pluginList.insertPlugin(te::VolumeAndPanPlugin::create(), 0);
    auto* volume = dynamic_cast<te::VolumeAndPanPlugin*>(plugin.get());
    require(volume != nullptr, "missing volume fixture");
    auto& curve = volume->volParam->getCurve();
    const auto index = curve.addPoint(tracktion::TimePosition::fromSeconds(1), .5f, 0);
    juce::ReferenceCountedArray<SelectableAutomationPoint> lane;
    lane.add(new SelectableAutomationPoint(index, curve));
    auto* proxy = lane[0].get();
    manager.selectOnly(proxy);
    snapshot.capture(manager);
    manager.selectOnly(track);
    require(proxy->getReferenceCount() > 1, "snapshot did not retain automation identity");
    snapshot.restore(*edit, manager);
    require(manager.isSelected(proxy), "automation selection was not restored");
    snapshot.clear();
    require(manager.isSelected(proxy) && proxy->getReferenceCount() == 1, "restored proxy lifetime depends on snapshot");
    snapshot.capture(manager);
    manager.deselectAll();
    curve.removePoint(index);
    snapshot.restore(*edit, manager);
    require(manager.getSelectedObjects().isEmpty(), "removed automation point was restored");
    snapshot.clear();

    // Safe references reject a destroyed owner, not just its removed notes.
    auto transient = std::make_unique<te::SelectedMidiEvents>(juce::Array<te::MidiClip*>{clip.get()});
    transient->addSelectedEvent(clip->getSequence().getNotes()[0], false);
    manager.selectOnly(*transient);
    snapshot.capture(manager);
    transient.reset();
    snapshot.restore(*edit, manager);
    require(manager.getSelectedObjects().isEmpty(), "destroyed owner survived shared restoration");
    snapshot.clear();

    manager.selectOnly(*clip);
    snapshot.capture(manager);
    clip->removeFromParent(); // Clip::Ptr deliberately keeps the removed object alive.
    snapshot.restore(*edit, manager);
    require(manager.getSelectedObjects().isEmpty(), "removed but retained clip was restored");
    snapshot.clear();
}

void indexedSelection()
{
    // Bucket keys are never identity: equal musical data remain distinct.
    juce::ValueTree original("NOTE");
    original.setProperty("b", 1.0, nullptr);
    const auto replacement = original.createCopy();
    require(combineLassoSelection(juce::Array<juce::ValueTree>{original},
        juce::Array<juce::ValueTree>{replacement, replacement}, LassoSelectionMode::add).size() == 2,
        "lookup merged distinct identities");
    original.removeAllProperties(nullptr);
    original.setProperty("b", 2.0, nullptr);
    require(combineLassoSelection(juce::Array<juce::ValueTree>{original},
        juce::Array<juce::ValueTree>{original}, LassoSelectionMode::toggle).isEmpty(), "index retained a stale property anchor");

    juce::ValueTree empty("NOTE");
    require(combineLassoSelection(juce::Array<juce::ValueTree>{empty},
        juce::Array<juce::ValueTree>{empty.createCopy()}, LassoSelectionMode::add).size() == 2,
        "propertyless hash collision merged identities");

    for (int size : {2000, 20000})
    {
        juce::Array<juce::ValueTree> initial, hits;
        for (int i = 0; i < size; ++i)
        {
            juce::ValueTree state("NOTE");
            // Deliberately identical contents defeat content-based hashes.
            state.setProperty("b", 1.0, nullptr);
            state.setProperty("p", 60, nullptr);
            initial.add(state);
            if (i % 2 == 0) hits.add(state);
        }
        const auto start = std::chrono::steady_clock::now();
        for (int repeat = 0; repeat < 3; ++repeat)
        {
            const auto result = combineLassoSelection(initial, hits, LassoSelectionMode::toggle);
            require(result.size() == size / 2 && result[0] == initial[1], "large indexed selection lost membership/order");
        }
        const auto milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        // Evidence, not a machine-dependent timing assertion.
        std::cout << "Indexed selection " << size << " identities, 3 updates: " << milliseconds << " ms\n";
    }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        geometry();
        velocityGeometry();
        selectionPolicy();
        inputOwnership();
        realSelection();
        keyboardClipScope();
        sharedSelection();
        indexedSelection();
        std::cout << "Selection gesture regressions passed\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
