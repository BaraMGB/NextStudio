/*
  ==============================================================================

    ArpeggiatorPlugin.h
    Created: 26 Jan 2026
    Author:  NextStudio

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace te = tracktion_engine;

class ArpeggiatorPlugin : public te::Plugin
{
  public:
    ArpeggiatorPlugin(te::PluginCreationInfo info);
    ~ArpeggiatorPlugin() override;

    //==============================================================================
    static constexpr const char *xmlTypeName = "arpeggiator";
    static const char *getPluginName() { return "Arpeggiator"; }

    juce::String getName() const override { return getPluginName(); }
    juce::String getPluginType() override { return xmlTypeName; }
    juce::String getSelectableDescription() override { return getPluginName(); }

    bool isSynth() override { return false; }
    bool takesMidiInput() override { return true; }
    bool takesAudioInput() override { return false; }
    bool producesAudioWhenNoAudioInput() override { return false; }

    void initialise(const te::PluginInitialisationInfo &) override;
    void deinitialise() override;
    void reset() override;
    void applyToBuffer(const te::PluginRenderContext &) override;
    void midiPanic() override;

    //==============================================================================
    void restorePluginStateFromValueTree(const juce::ValueTree &v) override;
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;

    enum Mode {
        up = 0,
        down,
        upDown,
        random,
        numModes
    };

    // Parameters
    te::AutomatableParameter::Ptr modeParam;
    te::AutomatableParameter::Ptr rateParam;
    te::AutomatableParameter::Ptr octaveParam;
    te::AutomatableParameter::Ptr gateParam;

    // State
    juce::CachedValue<float> modeValue;
    juce::CachedValue<float> rateValue;
    juce::CachedValue<float> octaveValue;
    juce::CachedValue<float> gateValue;

  private:
    void updateAtomics();

    // Audio Thread Params
    struct AudioParams
    {
        std::atomic<float> mode{0.0f};
        std::atomic<float> rate{2.0f}; // Index for 1/8
        std::atomic<float> octave{0.0f};
        std::atomic<float> gate{0.8f};
    } audioParams;

    // Arpeggiator State
    struct NoteInfo
    {
        int note;
        int velocity;
    };

    std::vector<NoteInfo> heldNotes;
    std::vector<int> sortedNotes;

    int currentStep = 0;
    bool goingUp = true;           // State for Up/Down mode
    double stoppedModeBeats = 0.0; // Internal beat clock for stopped mode

    int lastNotePlayed = -1;
    double lastNoteStartBeat = -1.0;
    double lastNoteDuration = 0.0;

    void updateSortedNotes();
    int getNextNote();
    double getRateInBeats(float rateIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorPlugin)
};
