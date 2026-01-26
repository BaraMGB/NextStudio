/*
  ==============================================================================

    ArpeggiatorPlugin.cpp
    Created: 26 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#include "ArpeggiatorPlugin.h"
#include "../../Utilities.h"

using namespace tracktion_engine;

ArpeggiatorPlugin::ArpeggiatorPlugin(PluginCreationInfo info)
    : Plugin(info)
{
    auto um = getUndoManager();

    // Mode Param
    modeParam = addParam("mode", "Mode", {0.0f, 3.0f, 1.0f}, [](float v) {
            int mode = juce::roundToInt(v);
            if (mode == up) return "Up";
            if (mode == down) return "Down";
            if (mode == upDown) return "Up/Down";
            if (mode == random) return "Random";
            return "Unknown"; }, [](const juce::String &s) {
            if (s == "Up") return (float)up;
            if (s == "Down") return (float)down;
            if (s == "Up/Down") return (float)upDown;
            if (s == "Random") return (float)random;
            return 0.0f; });

    // Rate Param
    rateParam = addParam("rate", "Rate", {0.0f, 6.0f, 1.0f}, [](float v) {
            int r = juce::roundToInt(v);
            if (r == 0) return "1/1";
            if (r == 1) return "1/2";
            if (r == 2) return "1/4";
            if (r == 3) return "1/8";
            if (r == 4) return "1/16";
            if (r == 5) return "1/32";
            if (r == 6) return "1/64";
            return "1/8"; }, [](const juce::String &s) {
            if (s == "1/1") return 0.0f;
            if (s == "1/2") return 1.0f;
            if (s == "1/4") return 2.0f;
            if (s == "1/8") return 3.0f;
            if (s == "1/16") return 4.0f;
            if (s == "1/32") return 5.0f;
            if (s == "1/64") return 6.0f;
            return 3.0f; });

    // Octave Param
    octaveParam = addParam("octave", "Octave", {0.0f, 3.0f, 1.0f}, [](float v) { return juce::String(juce::roundToInt(v) + 1); }, [](const juce::String &s) { return (float)(s.getIntValue() - 1); });

    gateParam = addParam("gate", "Gate", {0.1f, 1.0f});

    // State linking
    modeValue.referTo(state, "mode", um, 0.0f);
    rateValue.referTo(state, "rate", um, 3.0f);
    octaveValue.referTo(state, "octave", um, 0.0f);
    gateValue.referTo(state, "gate", um, 0.8f);

    modeParam->attachToCurrentValue(modeValue);
    rateParam->attachToCurrentValue(rateValue);
    octaveParam->attachToCurrentValue(octaveValue);
    gateParam->attachToCurrentValue(gateValue);

    state.addListener(this);
    updateAtomics();
}

ArpeggiatorPlugin::~ArpeggiatorPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    modeParam->detachFromCurrentValue();
    rateParam->detachFromCurrentValue();
    octaveParam->detachFromCurrentValue();
    gateParam->detachFromCurrentValue();
}

void ArpeggiatorPlugin::initialise(const PluginInitialisationInfo &info)
{
    updateAtomics();
}

void ArpeggiatorPlugin::deinitialise()
{
}

void ArpeggiatorPlugin::reset()
{
    heldNotes.clear();
    sortedNotes.clear();
    lastNotePlayed = -1;
    currentStep = 0;
    goingUp = true;
}

void ArpeggiatorPlugin::valueTreePropertyChanged(juce::ValueTree &v, const juce::Identifier &i)
{
    if (v == state)
        updateAtomics();
}

void ArpeggiatorPlugin::updateAtomics()
{
    audioParams.mode = modeValue.get();
    audioParams.rate = rateValue.get();
    audioParams.octave = octaveValue.get();
    audioParams.gate = gateValue.get();
}

void ArpeggiatorPlugin::restorePluginStateFromValueTree(const juce::ValueTree &v)
{
    auto restore = [&](te::AutomatableParameter::Ptr &param, const char *name) {
        if (v.hasProperty(name))
            param->setParameter((float)v.getProperty(name), juce::sendNotification);
    };

    restore(modeParam, "mode");
    restore(rateParam, "rate");
    restore(octaveParam, "octave");
    restore(gateParam, "gate");

    updateAtomics();
}

void ArpeggiatorPlugin::midiPanic()
{
    heldNotes.clear();
    sortedNotes.clear();
    lastNotePlayed = -1;
}

double ArpeggiatorPlugin::getRateInBeats(float rateIndex)
{
    int index = std::round(rateIndex);
    return 4.0 / std::pow(2.0, index);
}

void ArpeggiatorPlugin::updateSortedNotes()
{
    sortedNotes.clear();

    if (heldNotes.empty())
        return;

    std::vector<int> baseNotes;
    for (auto &n : heldNotes)
        baseNotes.push_back(n.note);
    std::sort(baseNotes.begin(), baseNotes.end());

    int octaves = std::round(audioParams.octave) + 1;

    for (int oct = 0; oct < octaves; ++oct) {
        for (int note : baseNotes) {
            int newNote = note + (oct * 12);
            if (newNote <= 127)
                sortedNotes.push_back(newNote);
        }
    }
}

int ArpeggiatorPlugin::getNextNote()
{
    if (sortedNotes.empty())
        return -1;

    int mode = std::round(audioParams.mode);

    if (mode == random) {
        juce::Random r;
        return sortedNotes[r.nextInt(sortedNotes.size())];
    }

    if (mode == up) {
        currentStep = (currentStep + 1) % sortedNotes.size();
    }
    else if (mode == down) {
        currentStep--;
        if (currentStep < 0)
            currentStep = sortedNotes.size() - 1;
    }
    else if (mode == upDown) {
        if (goingUp) {
            currentStep++;
            if (currentStep >= sortedNotes.size()) {
                currentStep = sortedNotes.size() - 2;
                goingUp = false;
                if (currentStep < 0)
                    currentStep = 0;
            }
        }
        else {
            currentStep--;
            if (currentStep < 0) {
                currentStep = 1;
                goingUp = true;
                if (currentStep >= sortedNotes.size())
                    currentStep = 0;
            }
        }
    }

    if (currentStep >= 0 && currentStep < sortedNotes.size())
        return sortedNotes[currentStep];

    return sortedNotes[0];
}

void ArpeggiatorPlugin::applyToBuffer(const PluginRenderContext &fc)
{
    auto &midi = *fc.bufferForMidiMessages;
    bool notesChanged = false;

    // 1. Process incoming MIDI
    for (const auto &m : midi) {
        if (m.isNoteOn()) {
            bool found = false;
            for (auto &hn : heldNotes)
                if (hn.note == m.getNoteNumber()) {
                    hn.velocity = m.getVelocity();
                    found = true;
                    break;
                }

            if (!found)
                heldNotes.push_back({m.getNoteNumber(), m.getVelocity()});

            if (heldNotes.size() == 1) // First note pressed, reset pattern
            {
                currentStep = -1;
                goingUp = true;
                if (!fc.isPlaying)
                    stoppedModeBeats = 0.0;
            }
            notesChanged = true;
        }
        else if (m.isNoteOff()) {
            for (auto it = heldNotes.begin(); it != heldNotes.end();) {
                if (it->note == m.getNoteNumber())
                    it = heldNotes.erase(it);
                else
                    ++it;
            }
            notesChanged = true;
        }
    }

    midi.removeNoteOnsAndOffs();

    if (notesChanged)
        updateSortedNotes();

    if (heldNotes.empty()) {
        if (lastNotePlayed != -1) {
            midi.addMidiMessage(juce::MidiMessage::noteOff(1, lastNotePlayed), 0.0, te::MPESourceID{});
            lastNotePlayed = -1;
        }
        return;
    }

    // 2. Timing
    auto startPos = fc.editTime.getStart();
    double startSeconds = startPos.inSeconds();
    auto &ts = edit.tempoSequence;

    double startBeats, endBeats;

    if (fc.isPlaying) {
        startBeats = ts.toBeats(startPos).inBeats();
        endBeats = ts.toBeats(fc.editTime.getEnd()).inBeats();
        stoppedModeBeats = endBeats;
    }
    else {
        double bpm = ts.getTempoAt(startPos).getBpm();
        double beatsInBlock = (fc.bufferNumSamples / sampleRate) * (bpm / 60.0);
        startBeats = stoppedModeBeats;
        endBeats = stoppedModeBeats + beatsInBlock;
        stoppedModeBeats = endBeats;
    }

    double noteDurationBeats = getRateInBeats(audioParams.rate);
    float gate = audioParams.gate;

    // 3. Note Offs
    if (lastNotePlayed != -1) {
        double noteEndBeat = lastNoteStartBeat + (lastNoteDuration * gate);
        if (noteEndBeat > startBeats && noteEndBeat <= endBeats) {
            double offset = 0.0;
            if (fc.isPlaying)
                offset = ts.toTime(tracktion::BeatPosition::fromBeats(noteEndBeat)).inSeconds() - startSeconds;
            else
                offset = (noteEndBeat - startBeats) * (60.0 / ts.getTempoAt(startPos).getBpm());

            midi.addMidiMessage(juce::MidiMessage::noteOff(1, lastNotePlayed), offset, te::MPESourceID{});
            lastNotePlayed = -1;
        }
        else if (noteEndBeat <= startBeats) {
            midi.addMidiMessage(juce::MidiMessage::noteOff(1, lastNotePlayed), 0.0, te::MPESourceID{});
            lastNotePlayed = -1;
        }
    }

    // 4. Note Ons
    double nextGridBeat = std::ceil(startBeats / noteDurationBeats) * noteDurationBeats;
    if (std::abs(startBeats - nextGridBeat) < 0.001)
        nextGridBeat = startBeats;

    while (nextGridBeat < endBeats) {
        double offset = 0.0;
        if (fc.isPlaying)
            offset = ts.toTime(tracktion::BeatPosition::fromBeats(nextGridBeat)).inSeconds() - startSeconds;
        else
            offset = (nextGridBeat - startBeats) * (60.0 / ts.getTempoAt(startPos).getBpm());

        if (lastNotePlayed != -1) {
            midi.addMidiMessage(juce::MidiMessage::noteOff(1, lastNotePlayed), offset, te::MPESourceID{});
            lastNotePlayed = -1;
        }

        int note = getNextNote();
        if (note != -1) {
            midi.addMidiMessage(juce::MidiMessage::noteOn(1, note, (juce::uint8)100), offset, te::MPESourceID{});
            lastNotePlayed = note;
            lastNoteStartBeat = nextGridBeat;
            lastNoteDuration = noteDurationBeats;
        }

        nextGridBeat += noteDurationBeats;
    }

    midi.sortByTimestamp();
}
