#include "SimpleSynthPlugin.h"

// Helper function for PolyBLEP (Polynomial Band-Limited Step)
// This technique reduces aliasing by smoothing the waveform discontinuities
// (like the jump in a sawtooth or square wave) using a polynomial curve.
// t:  Normalized phase position relative to the discontinuity (0.0 to 1.0)
// dt: Phase increment per sample (also normalized 0.0 to 1.0)
static float poly_blep(float t, float dt)
{
    // If we are close to the start of the period (t < dt), we are just after the jump.
    // We smooth the transition up from the reset point.
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    // If we are close to the end of the period (t > 1 - dt), we are just before the jump.
    // We smooth the transition down towards the reset point.
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    // Otherwise, we are in the linear part of the waveform, no correction needed.
    return 0.0f;
}

// Helper function for PolyBLAMP (Polynomial Band-Limited Ramp)
// Used to smooth out slope discontinuities (like the peaks of a triangle wave)
static float poly_blamp(float t, float dt)
{
    if (t < dt)
    {
        t = t / dt - 1.0f;
        return -1.0f / 15.0f * (t * t * t * t * t);
    }
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt + 1.0f;
        return 1.0f / 15.0f * (t * t * t * t * t);
    }

    return 0.0f;
}

SimpleSynthPlugin::SimpleSynthPlugin(te::PluginCreationInfo info) 
    : te::Plugin(info)
{
    auto um = getUndoManager();

    // Helper lambda to reduce boilerplate for standard parameters
    auto setupParam = [&](te::AutomatableParameter::Ptr& param, juce::CachedValue<float>& cv, 
                          const juce::String& id, const juce::String& name, 
                          juce::NormalisableRange<float> range, float defaultVal)
    {
        cv.referTo(state, id, um, defaultVal);
        param = addParam(id, name, range);
        param->attachToCurrentValue(cv);
    };

    setupParam(levelParam, levelValue, "level", "Level", {-100.0f, 0.0f}, 0.0f);
    setupParam(coarseTuneParam, coarseTuneValue, "coarseTune", "Coarse Tune", {-24.0f, 24.0f, 1.0f}, 0.0f);
    setupParam(fineTuneParam, fineTuneValue, "fineTune", "Fine Tune", {-100.0f, 100.0f}, 0.0f);

    // Osc 2 Params
    setupParam(osc2EnabledParam, osc2EnabledValue, "osc2Enabled", "Osc 2 On", {0.0f, 1.0f}, 0.0f);

    osc2WaveValue.referTo(state, "osc2Wave", um, 2.0f);
    osc2WaveParam = addParam("osc2Wave", "Osc 2 Wave", {0.0f, 4.0f, 1.0f},
                         [](float v) {
                             int type = juce::roundToInt(v);
                             if (type == Waveform::sine) return "Sine";
                             if (type == Waveform::triangle) return "Triangle";
                             if (type == Waveform::saw) return "Saw";
                             if (type == Waveform::square) return "Square";
                             if (type == Waveform::noise) return "Noise";
                             return "Unknown";
                         },
                         [](const juce::String& s) {
                             if (s == "Sine") return (float)Waveform::sine;
                             if (s == "Triangle") return (float)Waveform::triangle;
                             if (s == "Saw") return (float)Waveform::saw;
                             if (s == "Square") return (float)Waveform::square;
                             if (s == "Noise") return (float)Waveform::noise;
                             return 0.0f;
                         });
    osc2WaveParam->attachToCurrentValue(osc2WaveValue);

    setupParam(osc2CoarseParam, osc2CoarseValue, "osc2Coarse", "Osc 2 Coarse", {-24.0f, 24.0f, 1.0f}, 0.0f);
    setupParam(osc2FineParam, osc2FineValue, "osc2Fine", "Osc 2 Fine", {-100.0f, 100.0f}, 0.0f);
    setupParam(osc2LevelParam, osc2LevelValue, "osc2Level", "Osc 2 Level", {0.0f, 1.0f}, 0.5f);

    // Mix Mode Params
    mixModeValue.referTo(state, "mixMode", um, 0.0f);
    mixModeParam = addParam("mixMode", "Mix Mode", {0.0f, 3.0f, 1.0f},
                            [](float v) {
                                int mode = juce::roundToInt(v);
                                if (mode == MixMode::mix) return "Mix";
                                if (mode == MixMode::ringMod) return "RingMod";
                                if (mode == MixMode::fm) return "FM";
                                if (mode == MixMode::hardSync) return "HardSync";
                                return "Unknown";
                            },
                            [](const juce::String& s) {
                                if (s == "Mix") return (float)MixMode::mix;
                                if (s == "RingMod") return (float)MixMode::ringMod;
                                if (s == "FM") return (float)MixMode::fm;
                                if (s == "HardSync") return (float)MixMode::hardSync;
                                return 0.0f;
                            });
    mixModeParam->attachToCurrentValue(mixModeValue);

    setupParam(crossModAmountParam, crossModAmountValue, "crossModAmount", "Cross Mod", {0.0f, 1.0f}, 0.0f);

    // Wave Param (Custom String Conversion)
    waveValue.referTo(state, "wave", um, 2.0f);
    waveParam = addParam("wave", "Wave", {0.0f, 4.0f, 1.0f},
                         [](float v) {
                             int type = juce::roundToInt(v);
                             if (type == Waveform::sine) return "Sine";
                             if (type == Waveform::triangle) return "Triangle";
                             if (type == Waveform::saw) return "Saw";
                             if (type == Waveform::square) return "Square";
                             if (type == Waveform::noise) return "Noise";
                             return "Unknown";
                         },
                         [](const juce::String& s) {
                             if (s == "Sine") return (float)Waveform::sine;
                             if (s == "Triangle") return (float)Waveform::triangle;
                             if (s == "Saw") return (float)Waveform::saw;
                             if (s == "Square") return (float)Waveform::square;
                             if (s == "Noise") return (float)Waveform::noise;
                             return 0.0f;
                         });
    waveParam->attachToCurrentValue(waveValue);

    setupParam(attackParam, attackValue, "attack", "Attack", {0.0f, 5.0f}, 0.001f);
    setupParam(decayParam, decayValue, "decay", "Decay", {0.0f, 5.0f}, 0.001f);
    setupParam(sustainParam, sustainValue, "sustain", "Sustain", {0.0f, 1.0f}, 1.0f);
    setupParam(releaseParam, releaseValue, "release", "Release", {0.0f, 5.0f}, 0.001f);
    setupParam(unisonOrderParam, unisonOrderValue, "unisonOrder", "Unison Voices", {1.0f, 5.0f, 1.0f}, 1.0f);
    setupParam(unisonDetuneParam, unisonDetuneValue, "unisonDetune", "Unison Detune", {0.0f, 100.0f}, 0.0f);
    setupParam(unisonSpreadParam, unisonSpreadValue, "unisonSpread", "Unison Spread", {0.0f, 100.0f}, 0.0f);
    setupParam(retriggerParam, retriggerValue, "retrigger", "Retrigger", {0.0f, 1.0f, 1.0f}, 0.0f);

    // Filter Type Param (Custom String Conversion)
    filterTypeValue.referTo(state, "filterType", um, 0.0f);
    filterTypeParam = addParam("filterType", "Filter Type", {0.0f, 1.0f, 1.0f},
                               [](float v) { return v > 0.5f ? "SVF (12dB)" : "Ladder (24dB)"; },
                               [](const juce::String& s) { return s.contains("SVF") ? 1.0f : 0.0f; });
    filterTypeParam->attachToCurrentValue(filterTypeValue);

    // Filter Params
    setupParam(filterCutoffParam, filterCutoffValue, "cutoff", "Cutoff", {20.0f, 20000.0f, 0.0f, 0.3f}, 20000.0f);
    setupParam(filterResParam, filterResValue, "resonance", "Resonance", {0.0f, 1.0f}, 0.0f);
    setupParam(filterDriveParam, filterDriveValue, "drive", "Drive", {1.0f, 10.0f}, 1.0f);
    setupParam(filterEnvAmountParam, filterEnvAmountValue, "filterEnvAmount", "Env Amount", {-100.0f, 100.0f}, 0.0f);
    setupParam(filterAttackParam, filterAttackValue, "filterAttack", "Filter Attack", {0.0f, 5.0f}, 0.001f);
    setupParam(filterDecayParam, filterDecayValue, "filterDecay", "Filter Decay", {0.0f, 5.0f}, 0.001f);
    setupParam(filterSustainParam, filterSustainValue, "filterSustain", "Filter Sustain", {0.0f, 1.0f}, 1.0f);
    setupParam(filterReleaseParam, filterReleaseValue, "filterRelease", "Filter Release", {0.0f, 5.0f}, 0.001f);

    state.addListener(this);
    updateAtomics();
}

SimpleSynthPlugin::~SimpleSynthPlugin()
{
    state.removeListener(this);
    notifyListenersOfDeletion();

    levelParam->detachFromCurrentValue();
    coarseTuneParam->detachFromCurrentValue();
    fineTuneParam->detachFromCurrentValue();
    osc2EnabledParam->detachFromCurrentValue();
    osc2WaveParam->detachFromCurrentValue();
    osc2CoarseParam->detachFromCurrentValue();
    osc2FineParam->detachFromCurrentValue();
    osc2LevelParam->detachFromCurrentValue();
    mixModeParam->detachFromCurrentValue();
    crossModAmountParam->detachFromCurrentValue();
    waveParam->detachFromCurrentValue();
    attackParam->detachFromCurrentValue();
    decayParam->detachFromCurrentValue();
    sustainParam->detachFromCurrentValue();
    releaseParam->detachFromCurrentValue();
    unisonOrderParam->detachFromCurrentValue();
    unisonDetuneParam->detachFromCurrentValue();
    unisonSpreadParam->detachFromCurrentValue();
    retriggerParam->detachFromCurrentValue();
    filterTypeParam->detachFromCurrentValue();
    filterCutoffParam->detachFromCurrentValue();
    filterResParam->detachFromCurrentValue();
    filterEnvAmountParam->detachFromCurrentValue();
    filterAttackParam->detachFromCurrentValue();
    filterDecayParam->detachFromCurrentValue();
    filterSustainParam->detachFromCurrentValue();
    filterReleaseParam->detachFromCurrentValue();
}

void SimpleSynthPlugin::valueTreePropertyChanged(juce::ValueTree& v, const juce::Identifier&)
{
    if (v == state)
        updateAtomics();
}

void SimpleSynthPlugin::updateAtomics()
{
    audioParams.level = levelValue.get();
    audioParams.coarseTune = coarseTuneValue.get();
    audioParams.fineTune = fineTuneValue.get();

    audioParams.osc2Enabled = osc2EnabledValue.get();
    audioParams.osc2Wave = osc2WaveValue.get();
    audioParams.osc2Coarse = osc2CoarseValue.get();
    audioParams.osc2Fine = osc2FineValue.get();
    audioParams.osc2Level = osc2LevelValue.get();
    audioParams.mixMode = mixModeValue.get();
    audioParams.crossModAmount = crossModAmountValue.get();

    audioParams.wave = waveValue.get();
    audioParams.attack = attackValue.get();
    audioParams.decay = decayValue.get();
    audioParams.sustain = sustainValue.get();
    audioParams.release = releaseValue.get();
    audioParams.unisonOrder = unisonOrderValue.get();
    audioParams.unisonDetune = unisonDetuneValue.get();
    audioParams.unisonSpread = unisonSpreadValue.get();
    audioParams.retrigger = retriggerValue.get();
    audioParams.filterType = filterTypeValue.get();
    audioParams.filterCutoff = filterCutoffValue.get();
    audioParams.filterRes = filterResValue.get();
    audioParams.filterDrive = filterDriveValue.get();
    audioParams.filterEnvAmount = filterEnvAmountValue.get();
    audioParams.filterAttack = filterAttackValue.get();
    audioParams.filterDecay = filterDecayValue.get();
    audioParams.filterSustain = filterSustainValue.get();
    audioParams.filterRelease = filterReleaseValue.get();
}

void SimpleSynthPlugin::initialise(const te::PluginInitialisationInfo& info)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = info.sampleRate > 0.0 ? info.sampleRate : (double)defaultSampleRate;
    spec.maximumBlockSize = 4096; // Use a safe large block size as actual size isn't guaranteed here
    spec.numChannels = 1; // Mono processing per voice

    // Initialize voices with the correct sample rate
    for (auto& v : voices)
    {
        v.sampleRate = (float)spec.sampleRate;
        v.adsr.setSampleRate(spec.sampleRate);
        v.filterAdsr.setSampleRate(spec.sampleRate);

        v.filter.prepare(spec);
        v.filter.setMode(juce::dsp::LadderFilterMode::LPF24); // 24dB Low Pass

        v.svfFilter.prepare(spec);
        v.svfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

        v.random.setSeedRandomly();
    }

    masterLevelSmoother.reset(spec.sampleRate, (double)levelSmoothingTime);
    cutoffSmoother.reset(spec.sampleRate, (double)cutoffSmoothingTime);

    // Prepare sine lookup table (2048 points is plenty for audio)
    sineTable.initialise([](size_t i) { 
                             return std::sin((float)i / 2048.0f * juce::MathConstants<float>::twoPi); 
                         }, 2048);
    sineTableScaler = 2048.0f / juce::MathConstants<float>::twoPi;
}

void SimpleSynthPlugin::deinitialise()
{
}

void SimpleSynthPlugin::reset()
{
    midiPanic();
}

void SimpleSynthPlugin::applyToBuffer(const te::PluginRenderContext& fc)
{
    // 1. Basic Buffer Validation
    if (fc.destBuffer == nullptr || fc.bufferNumSamples == 0)
        return;

    if (!isEnabled())
    {
        fc.destBuffer->clear();
        return;
    }

    // 2. Sample Rate Validation
    if (sampleRate <= 0.0)
        return;

    fc.destBuffer->clear();

    // Snapshot parameters at the start of the block for consistency
    // Sanitize inputs immediately to prevent DSP blowups
    juce::ADSR::Parameters adsrParams;
    adsrParams.attack = juce::jmax(0.0f, audioParams.attack.load());
    adsrParams.decay = juce::jmax(0.0f, audioParams.decay.load());
    adsrParams.sustain = juce::jlimit(0.0f, 1.0f, audioParams.sustain.load());
    adsrParams.release = juce::jmax(0.0f, audioParams.release.load());

    juce::ADSR::Parameters filterAdsrParams;
    filterAdsrParams.attack = juce::jmax(0.0f, audioParams.filterAttack.load());
    filterAdsrParams.decay = juce::jmax(0.0f, audioParams.filterDecay.load());
    filterAdsrParams.sustain = juce::jlimit(0.0f, 1.0f, audioParams.filterSustain.load());
    filterAdsrParams.release = juce::jmax(0.0f, audioParams.filterRelease.load());

    // If the transport is not playing and we are not rendering,
    // ensure any ringing notes are silenced (notes that are active but no longer have keys pressed).
    if (!fc.isPlaying && !fc.isRendering)
    {
        for (auto& v : voices)
        {
            if (v.active && !v.isKeyDown)
                v.stop();
        }
    }

    // Safety clamping unisonOrder to safe range [1, 5] to prevent loop overflows
    int unisonOrder = juce::jlimit(1, 5, (int)audioParams.unisonOrder.load());

    float unisonDetuneCents = audioParams.unisonDetune.load();
    float unisonSpread = juce::jlimit(0.0f, 1.0f, audioParams.unisonSpread.load() / 100.0f);

    // Safety clamping for filter to ensure stability
    int filterType = juce::jlimit(0, (int)FilterType::numFilterTypes - 1, (int)audioParams.filterType.load());
    float baseCutoff = juce::jlimit(20.0f, 20000.0f, audioParams.filterCutoff.load());
    float resonance = juce::jlimit(0.0f, 1.0f, audioParams.filterRes.load());
    float drive = juce::jlimit(1.0f, 10.0f, audioParams.filterDrive.load());
    float filterEnvAmount = juce::jlimit(-1.0f, 1.0f, audioParams.filterEnvAmount.load() / 100.0f);

    float coarseTune = audioParams.coarseTune.load();
    float fineTuneCents = audioParams.fineTune.load();
    float osc2Coarse = audioParams.osc2Coarse.load();
    float osc2FineCents = audioParams.osc2Fine.load();
    int waveShape = (int)audioParams.wave.load();

    // Ensure waveShape is within valid enum range
    if (waveShape < 0 || waveShape >= Waveform::numWaveforms)
        waveShape = Waveform::saw;

    // 1. MIDI Processing
    processMidiMessages(fc.bufferForMidiMessages, adsrParams, filterAdsrParams);

    if (panicTriggered.exchange(false))
    {
        for (auto& v : voices)
            v.kill();
    }

    // 2. Update Voice Parameters (Control Rate)
    updateVoiceParameters(unisonOrder, unisonDetuneCents, unisonSpread, resonance, drive, coarseTune, fineTuneCents, osc2Coarse, osc2FineCents, adsrParams, filterAdsrParams);

    // 3. Audio Generation & Mixing
    renderAudio(fc, baseCutoff, filterEnvAmount, waveShape, unisonOrder, drive);
}

void SimpleSynthPlugin::midiPanic()
{
    panicTriggered = true;
}

SimpleSynthPlugin::Voice* SimpleSynthPlugin::findVoiceToSteal()
{
    Voice* oldestReleaseVoice = nullptr;
    uint32_t oldestReleaseTime = std::numeric_limits<uint32_t>::max();

    Voice* oldestVoice = nullptr;
    uint32_t oldestTime = std::numeric_limits<uint32_t>::max();

    for (auto& v : voices)
    {
        // Candidate 1: Voice in Release phase (key up)
        if (!v.isKeyDown)
        {
            if (v.noteOnTime < oldestReleaseTime)
            {
                oldestReleaseTime = v.noteOnTime;
                oldestReleaseVoice = &v;
            }
        }

        // Candidate 2: Any voice (LRU fallback)
        if (v.noteOnTime < oldestTime)
        {
            oldestTime = v.noteOnTime;
            oldestVoice = &v;
        }
    }

    // Prefer stealing a releasing voice over a held voice
    if (oldestReleaseVoice != nullptr)
        return oldestReleaseVoice;

    return oldestVoice;
}

void SimpleSynthPlugin::processMidiMessages(te::MidiMessageArray* midiMessages, const juce::ADSR::Parameters& adsrParams, const juce::ADSR::Parameters& filterAdsrParams)
{
    if (midiMessages == nullptr)
        return;

    int unisonOrder = juce::jlimit(1, 5, (int)audioParams.unisonOrder.load());
    bool retrigger = audioParams.retrigger.load() > 0.5f;
    float startCutoff = audioParams.filterCutoff.load();
    float drive = juce::jlimit(1.0f, 10.0f, audioParams.filterDrive.load());

    for (auto m : *midiMessages)
    {
        // Sanitize MIDI data
        if (m.isNoteOff())
        {
            int note = m.getNoteNumber();

            for (auto& v : voices)
            {
                if (v.active && v.currentNote == note && v.isKeyDown)
                    v.stop();
            }
        }
        else if (m.isNoteOn())
        {
            int note = juce::jlimit(0, 127, m.getNoteNumber());
            float velocity = juce::jlimit(0.0f, 1.0f, m.getFloatVelocity());

            if (velocity > 0.0f)
            {
                // Increment global note counter for LRU tracking
                // Wraparound is fine, uint32_t is large enough for years of playing
                noteCounter++;

                // Unison Logic: Trigger multiple voices
                triggerNote(note, velocity, unisonOrder, retrigger, startCutoff, drive, adsrParams, filterAdsrParams);
            }
            else
            {
                // NoteOn with velocity 0 is treated as NoteOff
                for (auto& v : voices)
                {
                    if (v.active && v.currentNote == note && v.isKeyDown)
                        v.stop();
                }
            }
        }
        else if (m.isAllNotesOff())
        {
            for (auto& v : voices)
                v.stop();
        }
        else if (m.isAllSoundOff())
        {
            for (auto& v : voices)
                v.kill();
        }
    }
}

void SimpleSynthPlugin::updateVoiceParameters(int unisonOrder, float unisonDetuneCents, float unisonSpread, float resonance, float drive, float coarseTune, float fineTuneCents, float osc2Coarse, float osc2FineCents, const juce::ADSR::Parameters& ampAdsr, const juce::ADSR::Parameters& filterAdsr)
{
    // Check for Unison Order change
    if (unisonOrder != lastUnisonOrder)
    {
        std::map<int, float> notesToRetrigger;

        for (auto& v : voices)
        {
            if (v.active && v.isKeyDown)
            {
                notesToRetrigger[v.currentNote] = v.currentVelocity;
            }
            // Stop all voices to allow clean re-allocation with new unison count
            if (v.active)
                v.stop();
        }

        float startCutoff = audioParams.filterCutoff.load();
        bool retrigger = audioParams.retrigger.load() > 0.5f;
        for (auto const& [note, vel] : notesToRetrigger)
        {
             triggerNote(note, vel, unisonOrder, retrigger, startCutoff, drive, ampAdsr, filterAdsr);
        }

        lastUnisonOrder = unisonOrder;
    }

    float tuneSemitones1 = coarseTune + (fineTuneCents / 100.0f);
    // Decoupled tuning: Osc 2 is relative to Note, not Osc 1.
    // This allows sweeping Osc 1 independently for Hard Sync effects.
    float tuneSemitones2 = osc2Coarse + (osc2FineCents / 100.0f);

    for (auto& v : voices)
    {
        if (v.active)
        {
            // Only update ADSR parameters if voice is NOT in release phase
            // This prevents release timing issues when parameters change during playback
            if (v.isKeyDown)
            {
                v.adsr.setParameters(ampAdsr);
                v.filterAdsr.setParameters(filterAdsr);
            }

            v.currentPan = juce::jlimit(0.0f, 1.0f, 0.5f + (v.unisonBias * 0.5f * unisonSpread));

            float cents = v.unisonBias * unisonDetuneCents;
            v.currentDetuneMultiplier = std::exp2f(cents / 1200.0f);

            // OSC 1 Frequency
            float baseFreq = SimpleSynthPlugin::referenceFrequency * std::exp2f((v.currentNote - SimpleSynthPlugin::midiNoteA4 + tuneSemitones1) / 12.0f);
            v.targetFrequency = baseFreq * v.currentDetuneMultiplier;
            v.phaseDelta = v.targetFrequency * juce::MathConstants<float>::twoPi / v.sampleRate;

            // OSC 2 Frequency
            float baseFreq2 = SimpleSynthPlugin::referenceFrequency * std::exp2f((v.currentNote - SimpleSynthPlugin::midiNoteA4 + tuneSemitones2) / 12.0f);
            v.targetFrequency2 = baseFreq2 * v.currentDetuneMultiplier;
            v.phaseDelta2 = v.targetFrequency2 * juce::MathConstants<float>::twoPi / v.sampleRate;

            v.filter.setResonance(resonance);
            v.filter.setDrive(drive);

            // Map 0.0-1.0 to 0.707-10.0 for SVF Q
            // SVF expects a Q factor where 0.707 is flat (Butterworth)
            float svfQ = svfBaseQ + (resonance * 9.0f);
            v.svfFilter.setResonance(svfQ);
        }
    }
}

inline float SimpleSynthPlugin::generateWaveSample(int waveShape, float phase, float phaseDelta, juce::Random& random)
{
    float sample = 0.0f;
    const float t = phase / juce::MathConstants<float>::twoPi; 
    const float dt = phaseDelta / juce::MathConstants<float>::twoPi;

    switch (waveShape)
    {
        case Waveform::sine:
            sample = sineTable.getUnchecked(phase * sineTableScaler);
            break;
        case Waveform::triangle:
            {
                // Naive triangle wave
                sample = 2.0f * std::abs(2.0f * t - 1.0f) - 1.0f; 

                // Apply PolyBLAMP to smooth the slope changes at t=0.0 and t=0.5
                sample += poly_blamp(t, dt) * 4.0f;

                float t2 = t + 0.5f;
                if (t2 >= 1.0f) t2 -= 1.0f;
                sample -= poly_blamp(t2, dt) * 4.0f;
            }
            break;
        case Waveform::saw:
            sample = (2.0f * t) - 1.0f;
            sample -= poly_blep(t, dt);
            break;
        case Waveform::square:
            sample = (phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
            sample += poly_blep(t, dt);
            {
                float t_shifted = t + 0.5f; 
                if (t_shifted >= 1.0f) t_shifted -= 1.0f;
                sample -= poly_blep(t_shifted, dt);
            }
            break;
        case Waveform::noise:
            sample = random.nextFloat() * 2.0f - 1.0f;
            break;
    }
    return sample;
}

void SimpleSynthPlugin::renderAudio(const te::PluginRenderContext& fc, float baseCutoff, float filterEnvAmount, int waveShape, int unisonOrder, float drive)
{
    float* left = fc.destBuffer->getWritePointer(0);
    float* right = fc.destBuffer->getWritePointer(1);
    const int numSamples = fc.bufferNumSamples;

    masterLevelSmoother.setTargetValue(juce::Decibels::decibelsToGain(audioParams.level.load()));
    cutoffSmoother.setTargetValue(baseCutoff);
    const int filterType = (int)audioParams.filterType.load();

    // New Params
    bool osc2On = audioParams.osc2Enabled.load() > 0.5f;
    int osc2WaveShape = (int)audioParams.osc2Wave.load();
    if (osc2WaveShape < 0 || osc2WaveShape >= Waveform::numWaveforms) osc2WaveShape = Waveform::saw;

    float osc2Level = audioParams.osc2Level.load();
    int mixMode = (int)audioParams.mixMode.load();
    float crossMod = audioParams.crossModAmount.load();

    float unisonGainCorrection = 1.0f;
    if (unisonOrder > 1)
        unisonGainCorrection = 1.0f / std::sqrt((float)unisonOrder);

    for (int i = 0; i < numSamples; ++i)
    {
        float l = 0.0f;
        float r = 0.0f;
        float gain = masterLevelSmoother.getNextValue() * unisonGainCorrection;
        float smoothedCutoff = cutoffSmoother.getNextValue();

        for (auto& v : voices)
        {
            if (v.active)
            {
                const float filterEnv = v.filterAdsr.getNextSample();

                // Logarithmic Modulation (Pitch-based)
                const float modSemitones = filterEnv * filterEnvAmount * maxFilterSweepSemitones;
                const float freqMultiplier = std::exp2f(modSemitones / 12.0f);

                const float modulatedCutoff = juce::jlimit(20.0f, 20000.0f, smoothedCutoff * freqMultiplier);

                // --- OSC 2 Generation (Modulator / Second Voice) ---
                float s2 = 0.0f;
                if (osc2On)
                {
                    s2 = generateWaveSample(osc2WaveShape, v.phase2, v.phaseDelta2, v.random);
                }

                // --- Phase Modifications (Sync / FM) ---
                float effectivePhase1 = v.phase;

                if (osc2On)
                {
                    // Hard Sync: Reset Phase 1 if Phase 2 wraps in this step
                    if (mixMode == MixMode::hardSync) 
                    {
                        if (v.phase2 + v.phaseDelta2 >= juce::MathConstants<float>::twoPi) 
                        {
                             v.phase = 0.0f;
                             effectivePhase1 = 0.0f;
                        }
                    }
                    // FM: Modulate Phase 1 with Osc 2 Output
                    else if (mixMode == MixMode::fm) 
                    {
                        // Map crossMod to a reasonable modulation index range (0.0 to 4.0 radians approx)
                        effectivePhase1 += (s2 * crossMod * 4.0f); 

                        // Wrap effective phase for lookup correctness
                        while (effectivePhase1 >= juce::MathConstants<float>::twoPi) effectivePhase1 -= juce::MathConstants<float>::twoPi;
                        while (effectivePhase1 < 0.0f) effectivePhase1 += juce::MathConstants<float>::twoPi;
                    }
                }

                // --- OSC 1 Generation ---
                float s1 = generateWaveSample(waveShape, effectivePhase1, v.phaseDelta, v.random);

                // --- Mixing ---
                float mixedSample = 0.0f;

                // If Osc 2 is Off, we bypass complex mixing logic and just output s1
                if (!osc2On)
                {
                     mixedSample = s1;
                }
                else
                {
                    switch (mixMode) 
                    {
                        case MixMode::ringMod:
                            // Blend between Clean Mix and RingMod (S1 * S2)
                            // Base: S1 + S2*Lev
                            // Ring: S1 * S2
                            {
                                float clean = s1 + (s2 * osc2Level);
                                float ring = s1 * s2; // Pure Ring Mod
                                mixedSample = clean * (1.0f - crossMod) + ring * crossMod;
                            }
                            break;

                        case MixMode::mix:
                        case MixMode::fm: // For FM, we usually just hear the Carrier (Osc 1), but let's allow mixing Osc 2
                        case MixMode::hardSync:
                        default:
                            mixedSample = s1 + (s2 * osc2Level);
                            break;
                    }
                }

                // --- Advance Phases ---
                v.phase += v.phaseDelta;
                if (v.phase >= juce::MathConstants<float>::twoPi)
                    v.phase -= juce::MathConstants<float>::twoPi;

                if (osc2On)
                {
                    v.phase2 += v.phaseDelta2;
                    if (v.phase2 >= juce::MathConstants<float>::twoPi)
                        v.phase2 -= juce::MathConstants<float>::twoPi;
                }

                // --- Filtering ---
                float sample = mixedSample; // Input to filter

                if (filterType == FilterType::ladder)
                {
                    float* channels[] = { &sample };
                    juce::dsp::AudioBlock<float> block (channels, 1, 1);
                    juce::dsp::ProcessContextReplacing<float> context (block);
                    v.filter.setCutoffFrequencyHz(modulatedCutoff);
                    v.filter.process(context);

                    block.multiplyBy(std::sqrt(drive));
                }
                else
                {
                    v.svfFilter.setCutoffFrequency(modulatedCutoff);
                    sample = v.svfFilter.processSample(0, sample);
                }

                JUCE_SNAP_TO_ZERO(sample);

                float adsrGain = v.adsr.getNextSample();
                if (!v.adsr.isActive())
                    v.active = false;

                float currentGain = adsrGain * v.currentVelocity;
                l += sample * currentGain * (1.0f - v.currentPan); 
                r += sample * currentGain * v.currentPan;
            }
        }

        left[i] = l * gain;
        right[i] = r * gain;
    }
}

void SimpleSynthPlugin::restorePluginStateFromValueTree(const juce::ValueTree& v)
{
    // Helper to restore properties from ValueTree to AutomatableParameters
    // We use setParameter to ensure that listeners (GUI) are notified of the change.
    // Setting CachedValues directly does not always trigger the parameter listeners.
    auto restore = [&](te::AutomatableParameter::Ptr& param, const char* name)
    {
        if (v.hasProperty(name))
        {
            float val = (float)v.getProperty(name);
            param->setParameter(val, juce::sendNotification);
        }
    };

    restore(levelParam, "level");
    restore(coarseTuneParam, "coarseTune");
    restore(fineTuneParam, "fineTune");

    restore(osc2EnabledParam, "osc2Enabled");
    restore(osc2WaveParam, "osc2Wave");
    restore(osc2CoarseParam, "osc2Coarse");
    restore(osc2FineParam, "osc2Fine");
    restore(osc2LevelParam, "osc2Level");
    restore(mixModeParam, "mixMode");
    restore(crossModAmountParam, "crossModAmount");

    restore(waveParam, "wave");
    restore(attackParam, "attack");
    restore(decayParam, "decay");
    restore(sustainParam, "sustain");
    restore(releaseParam, "release");
    restore(unisonOrderParam, "unisonOrder");
    restore(unisonDetuneParam, "unisonDetune");
    restore(unisonSpreadParam, "unisonSpread");
    restore(retriggerParam, "retrigger");
    restore(filterTypeParam, "filterType");
    restore(filterCutoffParam, "cutoff");
    restore(filterResParam, "resonance");
    restore(filterDriveParam, "drive");
    restore(filterEnvAmountParam, "filterEnvAmount");
    restore(filterAttackParam, "filterAttack");
    restore(filterDecayParam, "filterDecay");
    restore(filterSustainParam, "filterSustain");
    restore(filterReleaseParam, "filterRelease");

    // updateAtomics will be called via valueTreePropertyChanged when parameters update the state
    updateAtomics();
}

void SimpleSynthPlugin::triggerNote(int note, float velocity, int unisonOrder, bool retrigger, float startCutoff, float drive, const juce::ADSR::Parameters& ampParams, const juce::ADSR::Parameters& filterParams)
{
    // Unison Logic: Trigger multiple voices
    for (int u = 0; u < unisonOrder; ++u)
    {
        Voice* voiceToUse = nullptr;

        // 1. Try to find a free voice
        for (auto& v : voices)
        {
            if (!v.active)
            {
                voiceToUse = &v;
                break;
            }
        }

        // 2. If no free voice, steal one!
        if (voiceToUse == nullptr)
        {
            voiceToUse = findVoiceToSteal();
            // Ideally fade out here, but for now we hard steal
            if (voiceToUse) voiceToUse->stop();
        }

        if (voiceToUse != nullptr)
        {
            float bias = 0.0f;
            if (unisonOrder > 1)
            {
                float spreadAmount = (float)u / (float)(unisonOrder - 1); 
                bias = (spreadAmount - 0.5f) * 2.0f;
            }

            voiceToUse->start(note, velocity, (float)sampleRate, startCutoff, drive, ampParams, filterParams, bias, retrigger, noteCounter);
        }
    }
}

void SimpleSynthPlugin::Voice::start(int note, float velocity, float sr, float startCutoff, float drive, const juce::ADSR::Parameters& ampParams, const juce::ADSR::Parameters& filterParams, float bias, bool retrigger, uint32_t timestamp)
{
    active = true;
    isKeyDown = true;
    currentNote = note;
    currentVelocity = velocity;
    noteOnTime = timestamp;

    // Check if sample rate has changed significantly or was uninitialized
    // Re-prepare DSP objects if necessary
    if (sampleRate <= 0.0f || std::abs(sampleRate - sr) > 1.0f)
    {
        sampleRate = sr;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 4096;
        spec.numChannels = 1;

        svfFilter.prepare(spec);
        svfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

        adsr.setSampleRate(sampleRate);
        filterAdsr.setSampleRate(sampleRate);
    }

    // Fix for Ladder Filter Snapping:
    // Always re-prepare the Ladder Filter to reset its internal parameter smoothers.
    // Otherwise, it interpolates from the last used cutoff of this voice.
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 4096;
        spec.numChannels = 1;
        filter.prepare(spec);
        filter.setCutoffFrequencyHz(juce::jlimit(20.0f, 20000.0f, startCutoff));
        filter.setDrive(drive);
        filter.reset(); // Force smoothers to snap to startCutoff
        filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    }

    // If Retrigger is On: Reset phase to 0 for punchy attack
    // If Retrigger is Off: Randomize phase for analog feel / less phasing in unison
    if (retrigger)
    {
        phase = 0.0f;
        phase2 = 0.0f;
    }
    else
    {
        phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
        phase2 = random.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    unisonBias = bias;

    // Reset Filter State
    filter.reset();
    svfFilter.reset();

    // Configure and trigger the envelope
    // Note: setSampleRate is already handled above if needed, or in initialise
    adsr.setParameters(ampParams);
    adsr.noteOn();

    filterAdsr.setParameters(filterParams);
    filterAdsr.noteOn();
}

void SimpleSynthPlugin::Voice::stop()
{
    isKeyDown = false;
    // Trigger the release phase of the envelope
    adsr.noteOff();
    filterAdsr.noteOff();
}

void SimpleSynthPlugin::Voice::kill()
{
    active = false;
    isKeyDown = false;
    adsr.reset();
    filterAdsr.reset();
}
