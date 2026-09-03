/*

This file is part of NextStudio.
Copyright (c) Steffen Baranowsky 2019-2026.

*/

#include "MidiInputRouting.h"

#include <chrono>
#include <iostream>

namespace
{
namespace te = tracktion::engine;
int failures = 0;

#define REQUIRE(cond) \
    do { if (!(cond)) { std::cerr << "FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++failures; } } while (0)

void pumpMessages()
{
    juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
}

struct Fixture
{
    Fixture()
        : engine("NextStudioMidiInputRoutingTests-" + juce::String(juce::Time::currentTimeMillis()))
    {
        const auto suffix = juce::String(juce::Time::currentTimeMillis());
        firstDeviceName = "RoutingTestA-" + suffix;
        secondDeviceName = "RoutingTestB-" + suffix;

        REQUIRE(engine.getDeviceManager().createVirtualMidiDevice(firstDeviceName).wasOk());
        REQUIRE(engine.getDeviceManager().createVirtualMidiDevice(secondDeviceName).wasOk());

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while ((firstDevice == nullptr || secondDevice == nullptr) && std::chrono::steady_clock::now() < deadline)
        {
            pumpMessages();
            engine.getDeviceManager().dispatchPendingUpdates();

            for (auto candidate : engine.getDeviceManager().getMidiInDevices())
            {
                if (candidate->getName() == firstDeviceName)
                    firstDevice = candidate;
                else if (candidate->getName() == secondDeviceName)
                    secondDevice = candidate;
            }
        }

        REQUIRE(firstDevice != nullptr);
        REQUIRE(secondDevice != nullptr);
        if (firstDevice == nullptr || secondDevice == nullptr)
            return;

        firstDevice->setEnabled(true);
        secondDevice->setEnabled(true);

        edit = te::Edit::createSingleTrackEdit(engine);
        edit->ensureNumberOfAudioTracks(2);
        firstTrack = te::getAudioTracks(*edit)[0];
        secondTrack = te::getAudioTracks(*edit)[1];
        edit->getTransport().ensureContextAllocated();
        edit->dispatchPendingUpdatesSynchronously();

        firstInstance = edit->getCurrentInstanceForInputDevice(firstDevice.get());
        secondInstance = edit->getCurrentInstanceForInputDevice(secondDevice.get());
        REQUIRE(firstInstance != nullptr);
        REQUIRE(secondInstance != nullptr);
    }

    ~Fixture()
    {
        edit.reset();

        if (auto *device = dynamic_cast<te::VirtualMidiInputDevice *>(firstDevice.get()))
            engine.getDeviceManager().deleteVirtualMidiDevice(*device);
        if (auto *device = dynamic_cast<te::VirtualMidiInputDevice *>(secondDevice.get()))
            engine.getDeviceManager().deleteVirtualMidiDevice(*device);
    }

    te::Engine engine;
    std::unique_ptr<te::Edit> edit;
    std::shared_ptr<te::MidiInputDevice> firstDevice, secondDevice;
    te::InputDeviceInstance *firstInstance{}, *secondInstance{};
    te::AudioTrack *firstTrack{}, *secondTrack{};
    juce::String firstDeviceName, secondDeviceName;
};

void testSelectionResolutionPrefersTracksAndFallsBackToClips()
{
    Fixture f;
    if (f.firstTrack == nullptr || f.secondTrack == nullptr)
        return;

    const juce::Identifier midiTrackProperty{"testMidiTrack"};
    f.firstTrack->state.setProperty(midiTrackProperty, true, nullptr);
    f.secondTrack->state.setProperty(midiTrackProperty, true, nullptr);

    auto firstClip = f.firstTrack->insertMIDIClip("first",
                                                  {tracktion::TimePosition(),
                                                   tracktion::TimePosition::fromSeconds(1.0)},
                                                  nullptr);
    auto secondClip = f.secondTrack->insertMIDIClip("second",
                                                    {tracktion::TimePosition(),
                                                     tracktion::TimePosition::fromSeconds(1.0)},
                                                    nullptr);
    REQUIRE(firstClip != nullptr);
    REQUIRE(secondClip != nullptr);
    if (firstClip == nullptr || secondClip == nullptr)
        return;

    REQUIRE(MidiInputRouting::findMidiFocusTarget({f.firstTrack},
                                                   {secondClip.get()},
                                                   midiTrackProperty)
            == f.firstTrack->itemID);
    REQUIRE(MidiInputRouting::findMidiFocusTarget({},
                                                   {firstClip.get(), secondClip.get()},
                                                   midiTrackProperty)
            == f.secondTrack->itemID);

    f.firstTrack->state.setProperty(midiTrackProperty, false, nullptr);
    REQUIRE(MidiInputRouting::findMidiFocusTarget({f.firstTrack},
                                                   {secondClip.get()},
                                                   midiTrackProperty)
            == f.secondTrack->itemID);
    REQUIRE(!MidiInputRouting::findMidiFocusTarget({}, {firstClip.get()}, midiTrackProperty).isValid());
    REQUIRE(!MidiInputRouting::findMidiFocusTarget({}, {}, midiTrackProperty).isValid());
}

void testManualTargetToggleIsIdempotent()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    REQUIRE(!MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(!MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));

    auto update = MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, nullptr);
    REQUIRE(update.error.isEmpty());
    REQUIRE(update.routingChanged);
    REQUIRE(!update.metadataChanged);
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));

    update = MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, nullptr);
    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(!update.metadataChanged);
    REQUIRE(f.firstInstance->getTargets().size() == 1);

    update = MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, false, nullptr);
    REQUIRE(update.error.isEmpty());
    REQUIRE(update.routingChanged);
    REQUIRE(f.firstInstance->getTargets().isEmpty());

    update = MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, false, nullptr);
    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(!update.metadataChanged);
}

void testAutomaticReconciliationIsIdempotentAndEnablesMonitoring()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    auto &undoManager = f.edit->getUndoManager();
    undoManager.clearUndoHistory();
    f.firstDevice->setMonitorMode(te::InputDevice::MonitorMode::off);

    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                             {f.firstDevice.get()},
                                                             f.firstTrack->itemID);
    REQUIRE(update.error.isEmpty());
    REQUIRE(update.routingChanged);
    REQUIRE(update.metadataChanged);
    REQUIRE(f.firstDevice->getMonitorMode() == te::InputDevice::MonitorMode::on);
    REQUIRE(!undoManager.canUndo());

    update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                        {f.firstDevice.get()},
                                                        f.firstTrack->itemID);
    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(!update.metadataChanged);
    REQUIRE(f.firstInstance->getTargets().size() == 1);
}

void testAutomaticTargetCannotBeDisabledAsManual()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    MidiInputRouting::reconcileAutomaticFocus(*f.edit, {f.firstDevice.get()}, f.firstTrack->itemID);
    auto update = MidiInputRouting::setManualTarget(*f.firstInstance,
                                                     f.firstTrack->itemID,
                                                     false,
                                                     &f.edit->getUndoManager());

    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(!update.metadataChanged);
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testExistingManualTargetOnFocusDevicePreventsDuplicate()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, nullptr);
    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                             {f.firstDevice.get()},
                                                             f.firstTrack->itemID);

    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(!update.metadataChanged);
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testNonYieldingFocusDeviceRemainsAlongsideManualInput()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, true, nullptr);
    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                             {f.firstDevice.get()},
                                                             f.firstTrack->itemID);

    REQUIRE(update.error.isEmpty());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));
}

void testDisabledManualInputDoesNotSuppressYieldingDefault()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, true, nullptr);
    f.secondDevice->setEnabled(false);

    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                             {f.firstDevice.get()},
                                                             f.firstTrack->itemID,
                                                             {f.firstDevice.get()});
    REQUIRE(update.error.isEmpty());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testClearingAutomaticFocusIsIdempotentAndPreservesManualTargets()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    MidiInputRouting::reconcileAutomaticFocus(*f.edit, {f.firstDevice.get()}, f.firstTrack->itemID);
    MidiInputRouting::setManualTarget(*f.firstInstance, f.secondTrack->itemID, true, nullptr);
    MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, true, nullptr);

    auto update = MidiInputRouting::clearAutomaticFocus(*f.edit);
    REQUIRE(update.error.isEmpty());
    REQUIRE(update.routingChanged);
    REQUIRE(!MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.secondTrack->itemID));
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));

    update = MidiInputRouting::clearAutomaticFocus(*f.edit);
    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(!update.metadataChanged);
}

void testManualPromotionIsUndoableWithoutChangingTopology()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    auto &undoManager = f.edit->getUndoManager();
    MidiInputRouting::reconcileAutomaticFocus(*f.edit, {f.firstDevice.get()}, f.firstTrack->itemID);
    undoManager.clearUndoHistory();
    undoManager.beginNewTransaction("Pin MIDI input");

    auto update = MidiInputRouting::setManualTarget(*f.firstInstance,
                                                     f.firstTrack->itemID,
                                                     true,
                                                     &undoManager);
    REQUIRE(update.error.isEmpty());
    REQUIRE(!update.routingChanged);
    REQUIRE(update.metadataChanged);
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));

    REQUIRE(undoManager.undo());
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));

    REQUIRE(undoManager.redo());
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testMigrationWithoutFocusDevicesPreservesRoutesAndRunsOnce()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, nullptr);
    auto migration = MidiInputRouting::migrateLegacyFocusTargets(*f.edit, {});
    REQUIRE(migration.error.isEmpty());
    REQUIRE(!migration.routingChanged);
    REQUIRE(migration.metadataChanged);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));

    migration = MidiInputRouting::migrateLegacyFocusTargets(*f.edit, {f.firstDevice.get()});
    REQUIRE(migration.error.isEmpty());
    REQUIRE(!migration.routingChanged);
    REQUIRE(!migration.metadataChanged);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testUpdateResultMergeCombinesFlagsAndErrors()
{
    MidiInputRouting::UpdateResult first;
    first.routingChanged = true;
    first.error = "first";

    MidiInputRouting::UpdateResult second;
    second.metadataChanged = true;
    second.error = "second";

    first.merge(second);
    REQUIRE(first.routingChanged);
    REQUIRE(first.metadataChanged);
    REQUIRE(first.error == "first\nsecond");

    MidiInputRouting::UpdateResult empty;
    first.merge(empty);
    REQUIRE(first.error == "first\nsecond");
}

void testAutomaticFocusMovesWithoutTouchingManualTargets()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    juce::Array<te::InputDevice *> focusDevices{f.firstDevice.get()};
    f.edit->getUndoManager().clearUndoHistory();
    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit, focusDevices, f.firstTrack->itemID);
    REQUIRE(update.error.isEmpty());
    REQUIRE(update.routingChanged);
    REQUIRE(!f.edit->getUndoManager().canUndo());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(!MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));

    auto manual = MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, true, &f.edit->getUndoManager());
    REQUIRE(manual.error.isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));

    update = MidiInputRouting::reconcileAutomaticFocus(*f.edit, focusDevices, f.secondTrack->itemID);
    REQUIRE(update.error.isEmpty());
    REQUIRE(!f.firstInstance->getTargets().contains(f.firstTrack->itemID));
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.secondTrack->itemID));
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));
}

void testDefaultDeviceCanBePinnedManually()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    juce::Array<te::InputDevice *> focusDevices{f.firstDevice.get()};
    MidiInputRouting::reconcileAutomaticFocus(*f.edit, focusDevices, f.firstTrack->itemID);

    auto promotion = MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, &f.edit->getUndoManager());
    REQUIRE(promotion.error.isEmpty());
    REQUIRE(!promotion.routingChanged);
    REQUIRE(promotion.metadataChanged);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));

    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit, focusDevices, f.secondTrack->itemID);
    REQUIRE(update.error.isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.secondTrack->itemID));

    update = MidiInputRouting::clearAutomaticFocus(*f.edit);
    REQUIRE(update.error.isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(!f.firstInstance->getTargets().contains(f.secondTrack->itemID));
}

void testUnpinningFocusedDefaultIsAtomicAcrossUndoAndRedo()
{
    Fixture f;
    if (f.firstInstance == nullptr)
        return;

    auto &undoManager = f.edit->getUndoManager();
    const juce::Array<te::InputDevice *> focusDevices{f.firstDevice.get()};
    MidiInputRouting::reconcileAutomaticFocus(*f.edit, focusDevices, f.firstTrack->itemID);
    MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, nullptr);

    undoManager.clearUndoHistory();
    undoManager.beginNewTransaction("Unpin MIDI input");
    auto update = MidiInputRouting::setManualTarget(*f.firstInstance,
                                                     f.firstTrack->itemID,
                                                     false,
                                                     &undoManager);
    update.merge(MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                            focusDevices,
                                                            f.firstTrack->itemID,
                                                            {},
                                                            &undoManager));

    REQUIRE(update.error.isEmpty());
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));

    REQUIRE(undoManager.undo());
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.firstTrack->itemID));

    REQUIRE(undoManager.redo());
    REQUIRE(f.firstInstance->getTargets().size() == 1);
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testChangingFocusDeviceCleansOldAutomaticTarget()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    MidiInputRouting::reconcileAutomaticFocus(*f.edit, {f.firstDevice.get()}, f.firstTrack->itemID);
    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit, {f.secondDevice.get()}, f.secondTrack->itemID);

    REQUIRE(update.error.isEmpty());
    REQUIRE(f.firstInstance->getTargets().isEmpty());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.secondInstance, f.secondTrack->itemID));
}

void testDefaultFocusYieldsToAnotherManualInput()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    auto manual = MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, true, nullptr);
    REQUIRE(manual.error.isEmpty());

    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                             {f.firstDevice.get()},
                                                             f.firstTrack->itemID,
                                                             {f.firstDevice.get()});
    REQUIRE(update.error.isEmpty());
    REQUIRE(f.firstInstance->getTargets().isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));

    manual = MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, false, nullptr);
    REQUIRE(manual.error.isEmpty());

    update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                        {f.firstDevice.get()},
                                                        f.firstTrack->itemID,
                                                        {f.firstDevice.get()});
    REQUIRE(update.error.isEmpty());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
}

void testYieldingDefaultAndManualInputUndoTogether()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    auto &undoManager = f.edit->getUndoManager();
    const juce::Array<te::InputDevice *> focusDevices{f.firstDevice.get()};
    const juce::Array<te::InputDevice *> yieldingDevices{f.firstDevice.get()};
    MidiInputRouting::reconcileAutomaticFocus(*f.edit, focusDevices, f.firstTrack->itemID);

    undoManager.clearUndoHistory();
    undoManager.beginNewTransaction("Assign specific MIDI input");
    auto update = MidiInputRouting::setManualTarget(*f.secondInstance,
                                                     f.firstTrack->itemID,
                                                     true,
                                                     &undoManager);
    update.merge(MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                            focusDevices,
                                                            f.firstTrack->itemID,
                                                            yieldingDevices,
                                                            &undoManager));

    REQUIRE(update.error.isEmpty());
    REQUIRE(f.firstInstance->getTargets().isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));

    REQUIRE(undoManager.undo());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(f.secondInstance->getTargets().isEmpty());

    REQUIRE(undoManager.redo());
    REQUIRE(f.firstInstance->getTargets().isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));
}

void testAlwaysFocusedVirtualDeviceSurvivesDefaultFocusDisable()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    // The first device represents the optional default hardware input; the
    // second represents the virtual PC keyboard, which remains focus-managed.
    MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                               {f.firstDevice.get(), f.secondDevice.get()},
                                               f.firstTrack->itemID);
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.firstInstance, f.firstTrack->itemID));
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.secondInstance, f.firstTrack->itemID));

    auto update = MidiInputRouting::reconcileAutomaticFocus(*f.edit,
                                                             {f.secondDevice.get()},
                                                             f.secondTrack->itemID);
    REQUIRE(update.error.isEmpty());
    REQUIRE(f.firstInstance->getTargets().isEmpty());
    REQUIRE(MidiInputRouting::isAutomaticFocusTarget(*f.secondInstance, f.secondTrack->itemID));
    REQUIRE(!f.secondInstance->getTargets().contains(f.firstTrack->itemID));
}

void testLegacyMigrationPreservesOtherDevices()
{
    Fixture f;
    if (f.firstInstance == nullptr || f.secondInstance == nullptr)
        return;

    MidiInputRouting::setManualTarget(*f.firstInstance, f.firstTrack->itemID, true, nullptr);
    MidiInputRouting::setManualTarget(*f.firstInstance, f.secondTrack->itemID, true, nullptr);
    MidiInputRouting::setManualTarget(*f.secondInstance, f.firstTrack->itemID, true, nullptr);

    auto migration = MidiInputRouting::migrateLegacyFocusTargets(*f.edit, {f.firstDevice.get()});
    REQUIRE(migration.error.isEmpty());
    REQUIRE(f.firstInstance->getTargets().isEmpty());
    REQUIRE(MidiInputRouting::isManualTarget(*f.secondInstance, f.firstTrack->itemID));

    MidiInputRouting::setManualTarget(*f.firstInstance, f.secondTrack->itemID, true, nullptr);
    migration = MidiInputRouting::migrateLegacyFocusTargets(*f.edit, {f.firstDevice.get()});
    REQUIRE(!migration.routingChanged);
    REQUIRE(MidiInputRouting::isManualTarget(*f.firstInstance, f.secondTrack->itemID));
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    testSelectionResolutionPrefersTracksAndFallsBackToClips();
    testManualTargetToggleIsIdempotent();
    testAutomaticReconciliationIsIdempotentAndEnablesMonitoring();
    testAutomaticTargetCannotBeDisabledAsManual();
    testExistingManualTargetOnFocusDevicePreventsDuplicate();
    testNonYieldingFocusDeviceRemainsAlongsideManualInput();
    testDisabledManualInputDoesNotSuppressYieldingDefault();
    testClearingAutomaticFocusIsIdempotentAndPreservesManualTargets();
    testManualPromotionIsUndoableWithoutChangingTopology();
    testMigrationWithoutFocusDevicesPreservesRoutesAndRunsOnce();
    testUpdateResultMergeCombinesFlagsAndErrors();
    testAutomaticFocusMovesWithoutTouchingManualTargets();
    testDefaultDeviceCanBePinnedManually();
    testUnpinningFocusedDefaultIsAtomicAcrossUndoAndRedo();
    testChangingFocusDeviceCleansOldAutomaticTarget();
    testDefaultFocusYieldsToAnotherManualInput();
    testYieldingDefaultAndManualInputUndoTogether();
    testAlwaysFocusedVirtualDeviceSurvivesDefaultFocusDisable();
    testLegacyMigrationPreservesOtherDevices();

    if (failures != 0)
        return 1;

    std::cout << "MIDI input routing tests passed\n";
    return 0;
}
