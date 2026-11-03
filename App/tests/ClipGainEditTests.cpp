#include "ClipGainEdit.h"
#include "ClipGainHandle.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace te = tracktion_engine;
namespace
{
void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}
void near(float actual, float expected) { require(std::abs(actual - expected) < 0.0001f, "unexpected gain"); }

struct Fixture
{
    Fixture() : engine("NextStudioClipGainTests"), edit(te::Edit::createSingleTrackEdit(engine))
    {
        file = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("clip-gain-test", ".wav");
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(juce::WavAudioFormat().createWriterFor(
            file.createOutputStream().release(), 44100.0, 1, 16, {}, 0));
        require(writer != nullptr, "wave writer");
        juce::AudioBuffer<float> buffer(1, 4410);
        buffer.clear();
        require(writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()), "wave write");
        writer.reset();
        track = te::getFirstAudioTrack(*edit);
        first = track->insertWaveClip("first", file, {{tracktion::TimePosition::fromSeconds(0), tracktion::TimePosition::fromSeconds(1)}, {}}, false);
        second = track->insertWaveClip("second", file, {{tracktion::TimePosition::fromSeconds(2), tracktion::TimePosition::fromSeconds(3)}, {}}, false);
        midi = track->insertMIDIClip("midi", {tracktion::TimePosition::fromSeconds(4), tracktion::TimePosition::fromSeconds(5)}, nullptr);
        require(first != nullptr && second != nullptr && midi != nullptr, "clips");
        first->setGainDB(-6.0f);
        second->setGainDB(-12.0f);
        edit->getUndoManager().clearUndoHistory();
    }
    ~Fixture() { first = nullptr; second = nullptr; midi = nullptr; edit.reset(); file.deleteFile(); }
    juce::File file;
    te::Engine engine;
    std::unique_ptr<te::Edit> edit;
    te::AudioTrack *track{};
    te::WaveAudioClip::Ptr first, second;
    te::MidiClip::Ptr midi;
    juce::Array<te::Clip *> selection() { return {first.get(), second.get()}; }
};

void parsing()
{
    for (auto text : {"", "-", ".", "nan", "inf", "1e2", "2 dB junk", "3..5", "-100.01", "24.01", "999999999999999999999999"})
        require(!ClipGainEdit::parse(text), "invalid gain accepted");
    near(*ClipGainEdit::parse(" -6.5 dB "), -6.5f);
    near(*ClipGainEdit::parse("+24"), 24.0f);
    near(*ClipGainEdit::parse("-100"), -100.0f);
    require(ClipGainEdit::format(-0.001f) == "0.0 dB", "negative zero");
}

void editing()
{
    Fixture f;
    auto &undo = f.edit->getUndoManager();
    require(ClipGainEdit::audioOnly(f.selection()), "audio selection rejected");
    require(!ClipGainEdit::audioOnly({f.first.get(), f.midi.get()}), "mixed field enabled");
    require(!ClipGainEdit::audioOnly({}), "empty field enabled");
    const auto midiBefore = f.midi->state.createCopy();
    {
        ClipGainEdit gain(*f.edit, {f.first.get(), f.second.get(), f.midi.get()});
        for (int i = 0; i < 40; ++i) require(gain.previewRelative(float(i) * 0.5f), "live update");
        near(f.first->getGainDB(), 13.5f);
        near(f.second->getGainDB(), 7.5f);
        require(!undo.canUndo(), "preview entered undo");
        require(gain.commit(), "commit");
    }
    require(f.midi->state.isEquivalentTo(midiBefore), "MIDI changed");
    require(undo.undo(), "undo");
    near(f.first->getGainDB(), -6); near(f.second->getGainDB(), -12);
    require(!undo.canUndo(), "drag created multiple transactions");
    require(undo.redo(), "redo");
    near(f.first->getGainDB(), 13.5f); near(f.second->getGainDB(), 7.5f);
    {
        ClipGainEdit gain(*f.edit, f.selection());
        require(gain.previewAbsolute(-8.25f) && gain.commit(), "absolute");
    }
    near(f.first->getGainDB(), -8.25f); near(f.second->getGainDB(), -8.25f);
    auto xml = f.edit->state.createXml();
    auto restored = juce::ValueTree::fromXml(*xml);
    auto loaded = te::Edit::createEditForExamining(f.engine, restored);
    require(loaded != nullptr, "restore edit");
    auto *restoredTrack = te::getFirstAudioTrack(*loaded);
    auto *restoredAudio = dynamic_cast<te::AudioClipBase *>(restoredTrack->getClips()[0]);
    require(restoredAudio != nullptr, "restore audio");
    near(restoredAudio->getGainDB(), -8.25f);
}

void cancellationAndLimits()
{
    Fixture f;
    auto &undo = f.edit->getUndoManager();
    {
        ClipGainEdit gain(*f.edit, f.selection());
        require(gain.previewRelative(999), "upper limit");
        near(f.first->getGainDB(), 24); near(f.second->getGainDB(), 18);
        require(gain.previewRelative(-999), "lower limit");
        near(f.first->getGainDB(), -94); near(f.second->getGainDB(), -100);
        require(!gain.previewRelative(std::numeric_limits<float>::infinity()), "infinity accepted");
        require(!gain.previewAbsolute(25), "out of bounds accepted");
        require(!gain.matches({f.first.get()}), "selection change missed");
    }
    near(f.first->getGainDB(), -6); near(f.second->getGainDB(), -12);
    require(!undo.canUndo(), "cancel entered undo");
    {
        ClipGainEdit gain(*f.edit, f.selection());
        gain.previewRelative(3); gain.previewRelative(0);
        require(gain.commit(), "no-op");
    }
    require(!undo.canUndo(), "no-op entered undo");
    f.first->state.removeProperty(te::IDs::gain, nullptr);
    {
        ClipGainEdit gain(*f.edit, {f.first.get()});
        gain.previewRelative(2);
    }
    require(!f.first->state.hasProperty(te::IDs::gain), "cancel did not restore missing property");
    {
        ClipGainEdit gain(*f.edit, f.selection());
        gain.previewRelative(3);
        f.second->removeFromParent();
        require(!gain.commit(), "deleted clip committed");
    }
    near(f.first->getGainDB(), 0);
    {
        ClipGainEdit gain(*f.edit, {f.first.get()});
        gain.previewRelative(3);
        f.first->setGainDB(-20);
        require(!gain.commit(), "competing change accepted");
        near(f.first->getGainDB(), -20);
    }
}

void geometry()
{
    require(!ClipGainHandle::geometry({0, 0, 39, 100}, 20, 0), "narrow handle");
    require(!ClipGainHandle::geometry({0, 0, 100, 43}, 20, 0), "short handle");
    require(!ClipGainHandle::geometry({0, 0, 100, 30}, 20, 0), "minimized handle");
    for (float height : {44.0f, 50.0f})
        for (float db : {-100.0f, 0.0f, 24.0f})
        {
            const auto handle = ClipGainHandle::geometry({0, 0, 100, height}, 20, db);
            require(handle.has_value(), "minimum/default height missing handle");
            require(handle->circle().getY() >= 33.0f, "handle overlaps header/fade strip");
            require(handle->circle().getBottom() <= height - 3.0f, "handle exceeds bottom padding");
            require(handle->hit(handle->centre), "minimum/default centre miss");
        }
    for (float scale : {1.0f, 1.25f, 1.5f, 2.0f})
        for (float db : {-100.0f, -6.0f, 0.0f, 24.0f})
        {
            const auto handle = ClipGainHandle::geometry({-20.25f, 3.5f, 200, 100}, 20, db);
            require(handle.has_value(), "missing geometry");
            near(handle->centre.x, 79.75f);
            require(handle->circle().getY() > 23.5f, "handle overlaps header");
            require(handle->hit(handle->centre), "centre miss");
            require(!handle->hit(handle->centre + juce::Point<float>(3, 3)), "square corner hit");
            require(!handle->hit({handle->left, handle->centre.y}), "line hit");
            require(!handle->hit(handle->centre + juce::Point<float>(3.1f, 0)), "invisible hit padding");
            // Native scaling changes the raster, never logical hit geometry.
            const auto physical = handle->centre * scale;
            require(handle->hit(physical / scale), "scaled centre miss");
        }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    try { parsing(); editing(); cancellationAndLimits(); geometry(); }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
    std::cout << "Clip gain tests passed\n";
    return 0;
}
