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
    waveParam = addParam("wave", "Wave", {0.0f, 4.0f, 1.0f}); 
    attackParam = addParam("attack", "Attack", {0.0f, 5.0f});
    decayParam = addParam("decay", "Decay", {0.0f, 5.0f});
    sustainParam = addParam("sustain", "Sustain", {0.0f, 1.0f});
    releaseParam = addParam("release", "Release", {0.0f, 5.0f});
    unisonOrderParam = addParam("unisonOrder", "Unison Voices", {1.0f, 5.0f, 1.0f});
    unisonDetuneParam = addParam("unisonDetune", "Unison Detune", {0.0f, 100.0f}); // Cents
    unisonSpreadParam = addParam("unisonSpread", "Unison Spread", {0.0f, 100.0f}); // Percent
    retriggerParam = addParam("retrigger", "Retrigger", {0.0f, 1.0f, 1.0f}); // Default Off (0.0) -> Random Phase
    
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
    filterCutoffParam->attachToCurrentValue(filterCutoffValue);
    filterResParam->attachToCurrentValue(filterResValue);
    filterEnvAmountParam->attachToCurrentValue(filterEnvAmountValue);
    filterAttackParam->attachToCurrentValue(filterAttackValue);
    filterDecayParam->attachToCurrentValue(filterDecayValue);
    filterSustainParam->attachToCurrentValue(filterSustainValue);
    filterReleaseParam->attachToCurrentValue(filterReleaseValue);
}

SimpleSynthPlugin::~SimpleSynthPlugin()
{
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
    filterCutoffParam->detachFromCurrentValue();
    filterResParam->detachFromCurrentValue();
    filterEnvAmountParam->detachFromCurrentValue();
    filterAttackParam->detachFromCurrentValue();
    filterDecayParam->detachFromCurrentValue();
    filterSustainParam->detachFromCurrentValue();
    filterReleaseParam->detachFromCurrentValue();
}

void SimpleSynthPlugin::initialise(const te::PluginInitialisationInfo& info)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = info.sampleRate > 0.0 ? info.sampleRate : 44100.0;
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
        v.random.setSeedRandomly();
    }
        
    masterLevelSmoother.reset(spec.sampleRate, 0.02); // 20ms smoothing to prevent clicks on volume changes
}

void SimpleSynthPlugin::deinitialise()
{
}

void SimpleSynthPlugin::applyToBuffer(const te::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;

    fc.destBuffer->clear();

    // Ensure valid sample rate from Engine
    if (sampleRate <= 0.0)
        return;

    // Fetch current ADSR settings for all voices
    juce::ADSR::Parameters adsrParams;
    adsrParams.attack = attackParam->getCurrentValue();
    adsrParams.decay = decayParam->getCurrentValue();
    adsrParams.sustain = sustainParam->getCurrentValue();
    adsrParams.release = releaseParam->getCurrentValue();

    juce::ADSR::Parameters filterAdsrParams;
    filterAdsrParams.attack = filterAttackParam->getCurrentValue();
    filterAdsrParams.decay = filterDecayParam->getCurrentValue();
    filterAdsrParams.sustain = filterSustainParam->getCurrentValue();
    filterAdsrParams.release = filterReleaseParam->getCurrentValue();

    int unisonOrder = (int)unisonOrderParam->getCurrentValue();
    // Fetch live unison parameters
    float unisonDetuneCents = unisonDetuneParam->getCurrentValue();
    float unisonSpread = unisonSpreadParam->getCurrentValue() / 100.0f;
    bool retrigger = retriggerParam->getCurrentValue() > 0.5f;

    // Filter Parameters
    // CLAMP Cutoff to safe range to prevent filter instability (blowups/silence)
    float baseCutoff = juce::jmax(20.0f, filterCutoffParam->getCurrentValue());
    float resonance = filterResParam->getCurrentValue();
    float filterEnvAmount = filterEnvAmountParam->getCurrentValue() / 100.0f; // -1.0 to 1.0

    // Process MIDI events
    if (fc.bufferForMidiMessages != nullptr)
    {
        for (auto m : *fc.bufferForMidiMessages)
        {
            if (m.isNoteOn())
            {
                // Unison Logic: Trigger multiple voices
                for (int u = 0; u < unisonOrder; ++u)
                {
                    // Find a free voice to play the note
                    for (auto& v : voices)
                    {
                        if (!v.active)
                        {
                            float bias = 0.0f;
                            if (unisonOrder > 1)
                            {
                                // Map u (0 to order-1) to -1.0 to +1.0
                                // e.g. Order=3: u=0 -> -1, u=1 -> 0, u=2 -> +1
                                float spreadAmount = (float)u / (float)(unisonOrder - 1); 
                                bias = (spreadAmount - 0.5f) * 2.0f;
                            }
                            
                            v.start(m.getNoteNumber(), m.getFloatVelocity(), (float)sampleRate, adsrParams, filterAdsrParams, bias, retrigger);
                            break;
                        }
                    }
                }
            }
            else if (m.isNoteOff())
            {
                // Find the active voice playing this note and trigger release
                for (auto& v : voices)
                {
                    // Check isKeyDown to ensure we only release voices currently held by a key.
                    if (v.active && v.currentNote == m.getNoteNumber() && v.isKeyDown)
                    {
                        v.stop();
                        // Do NOT break here because in unison mode multiple voices have the same note number!
                    }
                }
            }
            else if (m.isAllNotesOff())
            {
                for (auto& v : voices)
                    v.stop();
            }
        }
    }

    // Prepare audio processing
    float* left = fc.destBuffer->getWritePointer(0);
    float* right = fc.destBuffer->getWritePointer(1);
    int numSamples = fc.bufferNumSamples;

    // Update master volume smoothing target
    masterLevelSmoother.setTargetValue(juce::Decibels::decibelsToGain(levelParam->getCurrentValue()));

    // Cache parameter values for this block
    float coarseTune = coarseTuneParam->getCurrentValue();
    float fineTuneCents = fineTuneParam->getCurrentValue();
    float totalTuneSemitones = coarseTune + (fineTuneCents / 100.0f);
    int waveShape = (int)waveParam->getCurrentValue();

    // UPDATE VOICE PARAMETERS (Control Rate)
    // Recalculate Pan and Detune based on current global parameters and the voice's unison bias
    // Also update filter parameters per voice
    for (auto& v : voices)
    {
        if (v.active)
        {
            // Calculate real-time Pan
            float spreadWidth = unisonSpread;
            v.currentPan = 0.5f + (v.unisonBias * 0.5f * spreadWidth);
            v.currentPan = juce::jlimit(0.0f, 1.0f, v.currentPan);

            // Calculate real-time Detune
            float detuneAmount = v.unisonBias; // -1 to 1
            float cents = detuneAmount * unisonDetuneCents;
            v.currentDetuneMultiplier = std::pow(2.0f, cents / 1200.0f);

            // Update Frequency
            float baseFreq = 440.0f * std::pow(2.0f, (v.currentNote - 69 + totalTuneSemitones) / 12.0f);
            v.targetFrequency = baseFreq * v.currentDetuneMultiplier;
            v.phaseDelta = v.targetFrequency * juce::MathConstants<float>::twoPi / v.sampleRate;

            // Update Filter - Base parameters only, modulation happens per sample
            v.filter.setResonance(resonance);
        }
    }

    // Audio Generation Loop
    for (int i = 0; i < numSamples; ++i)
    {
        float l = 0.0f;
        float r = 0.0f;
        float gain = masterLevelSmoother.getNextValue();

        // Reduce gain slightly when using many unison voices to prevent clipping
        if (unisonOrder > 1)
            gain /= std::sqrt((float)unisonOrder);

        for (auto& v : voices)
        {
            if (v.active)
            {
                // Calculate Filter Envelope
                float filterEnv = v.filterAdsr.getNextSample();
                
                // Modulate Cutoff
                // Use a snappy linear-in-frequency addition.
                // At 100% Env Amount, the envelope can sweep up to 18kHz above/below the base cutoff.
                float maxEnvSweepHz = 18000.0f;
                float modulatedCutoff = baseCutoff + (filterEnv * filterEnvAmount * maxEnvSweepHz);
                modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);
                
                v.filter.setCutoffFrequencyHz(modulatedCutoff);

                float sample = 0.0f;
                // Normalize phase and increment to 0..1 range for PolyBLEP calculations
                float t = v.phase / juce::MathConstants<float>::twoPi; 
                float dt = v.phaseDelta / juce::MathConstants<float>::twoPi;

                switch (waveShape)
                {
                    case sine:
                        sample = std::sin(v.phase);
                        break;
                    case triangle:
                        {
                            // Standard naive triangle wave.
                            sample = 2.0f * std::abs(2.0f * t - 1.0f) - 1.0f; 
                        }
                        break;
                    case saw:
                        // Naive Sawtooth: 2 * t - 1
                        sample = (2.0f * t) - 1.0f;
                        // Apply PolyBLEP to subtract the aliasing artifact at the discontinuity
                        sample -= poly_blep(t, dt);
                        break;
                    case square:
                        // Naive Square Wave
                        sample = (v.phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
                        // PolyBLEP: Correct the rising edge at t=0
                        sample += poly_blep(t, dt);
                        // PolyBLEP: Correct the falling edge at t=0.5
                        // We shift t by 0.5 to reuse the same poly_blep logic for the midpoint
                        {
                            float t_shifted = t + 0.5f; 
                            if (t_shifted >= 1.0f) t_shifted -= 1.0f;
                            sample -= poly_blep(t_shifted, dt);
                        }
                        break;
                    case noise:
                        sample = v.random.nextFloat() * 2.0f - 1.0f;
                        break;
                }

                // Advance phase
                v.phase += v.phaseDelta;
                if (v.phase >= juce::MathConstants<float>::twoPi)
                    v.phase -= juce::MathConstants<float>::twoPi;

                // Process Filter
                // Use standard DSP module processing pattern
                float* channels[] = { &sample };
                juce::dsp::AudioBlock<float> block (channels, 1, 1);
                juce::dsp::ProcessContextReplacing<float> context (block);
                v.filter.process(context);
                
                // Prevent denormals which can cause CPU spikes or silence in IIR filters
                JUCE_SNAP_TO_ZERO(sample);

                // Apply ADSR envelope
                float adsrGain = v.adsr.getNextSample();
                
                // If the envelope has finished, deactivate the voice to save CPU
                if (!v.adsr.isActive())
                    v.active = false;
                
                float currentGain = adsrGain * v.currentVelocity;
                
                // Mix into stereo buffer using calculated currentPan
                // Linear Panning
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
    // Restore state from XML/ValueTree. 
    // CachedValues will automatically update their attached parameters.
    if (v.hasProperty("level")) levelValue = v.getProperty("level");
    if (v.hasProperty("coarseTune")) coarseTuneValue = v.getProperty("coarseTune");
    if (v.hasProperty("fineTune")) fineTuneValue = v.getProperty("fineTune");
    if (v.hasProperty("wave")) waveValue = v.getProperty("wave");
    if (v.hasProperty("attack")) attackValue = v.getProperty("attack");
    if (v.hasProperty("decay")) decayValue = v.getProperty("decay");
    if (v.hasProperty("sustain")) sustainValue = v.getProperty("sustain");
    if (v.hasProperty("release")) releaseValue = v.getProperty("release");
    if (v.hasProperty("unisonOrder")) unisonOrderValue = v.getProperty("unisonOrder");
    if (v.hasProperty("unisonDetune")) unisonDetuneValue = v.getProperty("unisonDetune");
    if (v.hasProperty("unisonSpread")) unisonSpreadValue = v.getProperty("unisonSpread");
    if (v.hasProperty("retrigger")) retriggerValue = v.getProperty("retrigger");
    if (v.hasProperty("cutoff")) filterCutoffValue = v.getProperty("cutoff");
    if (v.hasProperty("resonance")) filterResValue = v.getProperty("resonance");
    if (v.hasProperty("filterEnvAmount")) filterEnvAmountValue = v.getProperty("filterEnvAmount");
    if (v.hasProperty("filterAttack")) filterAttackValue = v.getProperty("filterAttack");
    if (v.hasProperty("filterDecay")) filterDecayValue = v.getProperty("filterDecay");
    if (v.hasProperty("filterSustain")) filterSustainValue = v.getProperty("filterSustain");
    if (v.hasProperty("filterRelease")) filterReleaseValue = v.getProperty("filterRelease");
}

void SimpleSynthPlugin::Voice::start(int note, float velocity, float sr, const juce::ADSR::Parameters& ampParams, const juce::ADSR::Parameters& filterParams, float bias, bool retrigger)
{
    active = true;
    isKeyDown = true;
    currentNote = note;
    currentVelocity = velocity;
    
    // Check if sample rate has changed significantly or was uninitialized
    // Re-prepare DSP objects if necessary
    if (sampleRate <= 0.0f || std::abs(sampleRate - sr) > 1.0f)
    {
        sampleRate = sr;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 4096;
        spec.numChannels = 1;
        
        filter.prepare(spec);
        filter.setMode(juce::dsp::LadderFilterMode::LPF24);
        adsr.setSampleRate(sampleRate);
        filterAdsr.setSampleRate(sampleRate);
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