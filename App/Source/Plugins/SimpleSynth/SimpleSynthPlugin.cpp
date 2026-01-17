#include "SimpleSynthPlugin.h"
#include <map>

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

    // Link cached values to the ValueTree state
    levelValue.referTo(state, "level", um, 0.0f);
    coarseTuneValue.referTo(state, "coarseTune", um, 0.0f);
    fineTuneValue.referTo(state, "fineTune", um, 0.0f);
    waveValue.referTo(state, "wave", um, 2.0f);
    attackValue.referTo(state, "attack", um, 0.001f);
    decayValue.referTo(state, "decay", um, 0.001f);
    sustainValue.referTo(state, "sustain", um, 1.0f);
    releaseValue.referTo(state, "release", um, 0.001f);
    unisonOrderValue.referTo(state, "unisonOrder", um, 1.0f);
    unisonDetuneValue.referTo(state, "unisonDetune", um, 0.0f);
    unisonSpreadValue.referTo(state, "unisonSpread", um, 0.0f);
    retriggerValue.referTo(state, "retrigger", um, 0.0f);
    filterTypeValue.referTo(state, "filterType", um, 0.0f);
    filterCutoffValue.referTo(state, "cutoff", um, 20000.0f);
    filterResValue.referTo(state, "resonance", um, 0.0f);
    filterEnvAmountValue.referTo(state, "filterEnvAmount", um, 0.0f);
    filterAttackValue.referTo(state, "filterAttack", um, 0.001f);
    filterDecayValue.referTo(state, "filterDecay", um, 0.001f);
    filterSustainValue.referTo(state, "filterSustain", um, 1.0f);
    filterReleaseValue.referTo(state, "filterRelease", um, 0.001f);

    // Create and expose automatable parameters
    levelParam = addParam("level", "Level", {-100.0f, 0.0f});
    coarseTuneParam = addParam("coarseTune", "Coarse Tune", {-24.0f, 24.0f, 1.0f});
    fineTuneParam = addParam("fineTune", "Fine Tune", {-100.0f, 100.0f}); // Cents
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
    attackParam = addParam("attack", "Attack", {0.0f, 5.0f});
    decayParam = addParam("decay", "Decay", {0.0f, 5.0f});
    sustainParam = addParam("sustain", "Sustain", {0.0f, 1.0f});
    releaseParam = addParam("release", "Release", {0.0f, 5.0f});
    unisonOrderParam = addParam("unisonOrder", "Unison Voices", {1.0f, 5.0f, 1.0f});
    unisonDetuneParam = addParam("unisonDetune", "Unison Detune", {0.0f, 100.0f}); // Cents
    unisonSpreadParam = addParam("unisonSpread", "Unison Spread", {0.0f, 100.0f}); // Percent
    retriggerParam = addParam("retrigger", "Retrigger", {0.0f, 1.0f, 1.0f}); // Default Off (0.0) -> Random Phase
    filterTypeParam = addParam("filterType", "Filter Type", {0.0f, 1.0f, 1.0f},
                               [](float v) { return v > 0.5f ? "SVF (12dB)" : "Ladder (24dB)"; },
                               [](const juce::String& s) { return s.contains("SVF") ? 1.0f : 0.0f; });
    
    // Filter Parameters
    // Use a skewed range for Frequency to make the knob feel natural (logarithmic)
    // Removed 4th argument (default value override) to match function signature
    filterCutoffParam = addParam("cutoff", "Cutoff", {20.0f, 20000.0f, 0.0f, 0.3f});
    filterResParam = addParam("resonance", "Resonance", {0.0f, 1.0f});
    filterEnvAmountParam = addParam("filterEnvAmount", "Env Amount", {-100.0f, 100.0f});
    filterAttackParam = addParam("filterAttack", "Filter Attack", {0.0f, 5.0f});
    filterDecayParam = addParam("filterDecay", "Filter Decay", {0.0f, 5.0f});
    filterSustainParam = addParam("filterSustain", "Filter Sustain", {0.0f, 1.0f});
    filterReleaseParam = addParam("filterRelease", "Filter Release", {0.0f, 5.0f});

    // Attach parameters to cached values for automatic bi-directional updates
    levelParam->attachToCurrentValue(levelValue);
    coarseTuneParam->attachToCurrentValue(coarseTuneValue);
    fineTuneParam->attachToCurrentValue(fineTuneValue);
    waveParam->attachToCurrentValue(waveValue);
    attackParam->attachToCurrentValue(attackValue);
    decayParam->attachToCurrentValue(decayValue);
    sustainParam->attachToCurrentValue(sustainValue);
    releaseParam->attachToCurrentValue(releaseValue);
    unisonOrderParam->attachToCurrentValue(unisonOrderValue);
    unisonDetuneParam->attachToCurrentValue(unisonDetuneValue);
    unisonSpreadParam->attachToCurrentValue(unisonSpreadValue);
    retriggerParam->attachToCurrentValue(retriggerValue);
    filterTypeParam->attachToCurrentValue(filterTypeValue);
    filterCutoffParam->attachToCurrentValue(filterCutoffValue);
    filterResParam->attachToCurrentValue(filterResValue);
    filterEnvAmountParam->attachToCurrentValue(filterEnvAmountValue);
    filterAttackParam->attachToCurrentValue(filterAttackValue);
    filterDecayParam->attachToCurrentValue(filterDecayValue);
    filterSustainParam->attachToCurrentValue(filterSustainValue);
    filterReleaseParam->attachToCurrentValue(filterReleaseValue);

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

    // CRITICAL: Clamp unisonOrder to safe range [1, 5] to prevent loop overflows
    int unisonOrder = juce::jlimit(1, 5, (int)audioParams.unisonOrder.load());
    
    float unisonDetuneCents = audioParams.unisonDetune.load();
    float unisonSpread = juce::jlimit(0.0f, 1.0f, audioParams.unisonSpread.load() / 100.0f);
    
    // Safety clamping for filter to ensure stability
    int filterType = juce::jlimit(0, (int)FilterType::numFilterTypes - 1, (int)audioParams.filterType.load());
    float baseCutoff = juce::jlimit(20.0f, 20000.0f, audioParams.filterCutoff.load());
    float resonance = juce::jlimit(0.0f, 1.0f, audioParams.filterRes.load());
    float filterEnvAmount = juce::jlimit(-1.0f, 1.0f, audioParams.filterEnvAmount.load() / 100.0f);

    float coarseTune = audioParams.coarseTune.load();
    float fineTuneCents = audioParams.fineTune.load();
    int waveShape = (int)audioParams.wave.load();
    
    // Ensure waveShape is within valid enum range
    if (waveShape < 0 || waveShape >= Waveform::numWaveforms)
        waveShape = Waveform::saw;

    // 1. MIDI Processing
    processMidiMessages(fc.bufferForMidiMessages, adsrParams, filterAdsrParams);

    // 2. Update Voice Parameters (Control Rate)
    updateVoiceParameters(unisonOrder, unisonDetuneCents, unisonSpread, resonance, coarseTune, fineTuneCents, adsrParams, filterAdsrParams);

    // 3. Audio Generation & Mixing
    renderAudio(fc, baseCutoff, filterEnvAmount, waveShape, unisonOrder);
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

    for (auto m : *midiMessages)
    {
        // Sanitize MIDI data
        if (m.isNoteOn())
        {
            int note = juce::jlimit(0, 127, m.getNoteNumber());
            float velocity = juce::jlimit(0.0f, 1.0f, m.getFloatVelocity());
            
            // Increment global note counter for LRU tracking
            // Wraparound is fine, uint32_t is large enough for years of playing
            noteCounter++;

            // Unison Logic: Trigger multiple voices
            triggerNote(note, velocity, unisonOrder, retrigger, startCutoff, adsrParams, filterAdsrParams);
        }
        else if (m.isNoteOff())
        {
            int note = m.getNoteNumber(); // No limit needed for comparison, but good practice
            
            for (auto& v : voices)
            {
                if (v.active && v.currentNote == note && v.isKeyDown)
                    v.stop();
            }
        }
        else if (m.isAllNotesOff())
        {
            for (auto& v : voices)
                v.stop();
        }
    }
}

void SimpleSynthPlugin::updateVoiceParameters(int unisonOrder, float unisonDetuneCents, float unisonSpread, float resonance, float coarseTune, float fineTuneCents, const juce::ADSR::Parameters& ampAdsr, const juce::ADSR::Parameters& filterAdsr)
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
             triggerNote(note, vel, unisonOrder, retrigger, startCutoff, ampAdsr, filterAdsr);
        }
        
        lastUnisonOrder = unisonOrder;
    }

    float totalTuneSemitones = coarseTune + (fineTuneCents / 100.0f);

    for (auto& v : voices)
    {
        if (v.active)
        {
            v.adsr.setParameters(ampAdsr);
            v.filterAdsr.setParameters(filterAdsr);

            v.currentPan = juce::jlimit(0.0f, 1.0f, 0.5f + (v.unisonBias * 0.5f * unisonSpread));
            
            float cents = v.unisonBias * unisonDetuneCents;
            v.currentDetuneMultiplier = std::exp2f(cents / 1200.0f);

            float baseFreq = 440.0f * std::exp2f((v.currentNote - 69 + totalTuneSemitones) / 12.0f);
            v.targetFrequency = baseFreq * v.currentDetuneMultiplier;
            v.phaseDelta = v.targetFrequency * juce::MathConstants<float>::twoPi / v.sampleRate;

            v.filter.setResonance(resonance);
            
            // Map 0.0-1.0 to 0.707-10.0 for SVF Q
            // SVF expects a Q factor where 0.707 is flat (Butterworth)
            float svfQ = svfBaseQ + (resonance * 9.0f);
            v.svfFilter.setResonance(svfQ);
        }
    }
}

void SimpleSynthPlugin::renderAudio(const te::PluginRenderContext& fc, float baseCutoff, float filterEnvAmount, int waveShape, int unisonOrder)
{
    float* left = fc.destBuffer->getWritePointer(0);
    float* right = fc.destBuffer->getWritePointer(1);
    const int numSamples = fc.bufferNumSamples;

    masterLevelSmoother.setTargetValue(juce::Decibels::decibelsToGain(audioParams.level.load()));
    cutoffSmoother.setTargetValue(baseCutoff);
    const int filterType = (int)audioParams.filterType.load();

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
                // exp2f is much faster than std::pow(2.0, x)
                const float modSemitones = filterEnv * filterEnvAmount * maxFilterSweepSemitones;
                const float freqMultiplier = std::exp2f(modSemitones / 12.0f);
                
                const float modulatedCutoff = juce::jlimit(20.0f, 20000.0f, smoothedCutoff * freqMultiplier);
                
                float sample = 0.0f;
                const float t = v.phase / juce::MathConstants<float>::twoPi; 
                const float dt = v.phaseDelta / juce::MathConstants<float>::twoPi;

                switch (waveShape)
                {
                    case Waveform::sine:
                        sample = sineTable.getUnchecked(v.phase * sineTableScaler);
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
                        sample = (v.phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
                        sample += poly_blep(t, dt);
                        {
                            float t_shifted = t + 0.5f; 
                            if (t_shifted >= 1.0f) t_shifted -= 1.0f;
                            sample -= poly_blep(t_shifted, dt);
                        }
                        break;
                    case Waveform::noise:
                        sample = v.random.nextFloat() * 2.0f - 1.0f;
                        break;
                }

                v.phase += v.phaseDelta;
                if (v.phase >= juce::MathConstants<float>::twoPi)
                    v.phase -= juce::MathConstants<float>::twoPi;

                if (filterType == FilterType::ladder)
                {
                    float* channels[] = { &sample };
                    juce::dsp::AudioBlock<float> block (channels, 1, 1);
                    juce::dsp::ProcessContextReplacing<float> context (block);
                    v.filter.setCutoffFrequencyHz(modulatedCutoff);
                    v.filter.process(context);
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
    // Helper to restore properties from ValueTree to CachedValues
    auto restore = [&](juce::CachedValue<float>& cv, const char* name)
    {
        if (v.hasProperty(name))
            cv = v.getProperty(name);
    };

    restore(levelValue, "level");
    restore(coarseTuneValue, "coarseTune");
    restore(fineTuneValue, "fineTune");
    restore(waveValue, "wave");
    restore(attackValue, "attack");
    restore(decayValue, "decay");
    restore(sustainValue, "sustain");
    restore(releaseValue, "release");
    restore(unisonOrderValue, "unisonOrder");
    restore(unisonDetuneValue, "unisonDetune");
    restore(unisonSpreadValue, "unisonSpread");
    restore(retriggerValue, "retrigger");
    restore(filterTypeValue, "filterType");
    restore(filterCutoffValue, "cutoff");
    restore(filterResValue, "resonance");
    restore(filterEnvAmountValue, "filterEnvAmount");
    restore(filterAttackValue, "filterAttack");
    restore(filterDecayValue, "filterDecay");
    restore(filterSustainValue, "filterSustain");
    restore(filterReleaseValue, "filterRelease");

    updateAtomics();
}

void SimpleSynthPlugin::triggerNote(int note, float velocity, int unisonOrder, bool retrigger, float startCutoff, const juce::ADSR::Parameters& ampParams, const juce::ADSR::Parameters& filterParams)
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
            
            voiceToUse->start(note, velocity, (float)sampleRate, startCutoff, ampParams, filterParams, bias, retrigger, noteCounter);
        }
    }
}

void SimpleSynthPlugin::Voice::start(int note, float velocity, float sr, float startCutoff, const juce::ADSR::Parameters& ampParams, const juce::ADSR::Parameters& filterParams, float bias, bool retrigger, uint32_t timestamp)
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
        filter.reset(); // Force smoothers to snap to startCutoff
        filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    }
    
    // If Retrigger is On: Reset phase to 0 for punchy attack
    // If Retrigger is Off: Randomize phase for analog feel / less phasing in unison
    if (retrigger)
        phase = 0.0f;
    else
        phase = random.nextFloat() * juce::MathConstants<float>::twoPi;
        
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
