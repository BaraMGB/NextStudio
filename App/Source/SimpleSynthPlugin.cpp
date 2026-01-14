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
    tuneValue.referTo(state, "tune", um, 0.0f);
    waveValue.referTo(state, "wave", um, 0.0f);
    attackValue.referTo(state, "attack", um, 0.1f);
    decayValue.referTo(state, "decay", um, 0.1f);
    sustainValue.referTo(state, "sustain", um, 0.8f);
    releaseValue.referTo(state, "release", um, 0.2f);

    // Create and expose automatable parameters
    levelParam = addParam("level", "Level", {-100.0f, 0.0f});
    tuneParam = addParam("tune", "Tune", {-24.0f, 24.0f});
    waveParam = addParam("wave", "Wave", {0.0f, 4.0f, 1.0f}); 
    attackParam = addParam("attack", "Attack", {0.001f, 5.0f});
    decayParam = addParam("decay", "Decay", {0.001f, 5.0f});
    sustainParam = addParam("sustain", "Sustain", {0.0f, 1.0f});
    releaseParam = addParam("release", "Release", {0.001f, 5.0f});
    
    // Attach parameters to cached values for automatic bi-directional updates
    levelParam->attachToCurrentValue(levelValue);
    tuneParam->attachToCurrentValue(tuneValue);
    waveParam->attachToCurrentValue(waveValue);
    attackParam->attachToCurrentValue(attackValue);
    decayParam->attachToCurrentValue(decayValue);
    sustainParam->attachToCurrentValue(sustainValue);
    releaseParam->attachToCurrentValue(releaseValue);
}

SimpleSynthPlugin::~SimpleSynthPlugin()
{
    notifyListenersOfDeletion();
    
    levelParam->detachFromCurrentValue();
    tuneParam->detachFromCurrentValue();
    waveParam->detachFromCurrentValue();
    attackParam->detachFromCurrentValue();
    decayParam->detachFromCurrentValue();
    sustainParam->detachFromCurrentValue();
    releaseParam->detachFromCurrentValue();
}

void SimpleSynthPlugin::initialise(const te::PluginInitialisationInfo& info)
{
    // Initialize voices with the correct sample rate
    for (auto& v : voices)
    {
        v.sampleRate = (float)info.sampleRate;
        v.adsr.setSampleRate(info.sampleRate);
        v.random.setSeedRandomly();
    }
        
    masterLevelSmoother.reset(info.sampleRate, 0.02); // 20ms smoothing to prevent clicks on volume changes
}

void SimpleSynthPlugin::deinitialise()
{
}

void SimpleSynthPlugin::applyToBuffer(const te::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;

    fc.destBuffer->clear();

    // Fetch current ADSR settings for all voices
    juce::ADSR::Parameters adsrParams;
    adsrParams.attack = attackParam->getCurrentValue();
    adsrParams.decay = decayParam->getCurrentValue();
    adsrParams.sustain = sustainParam->getCurrentValue();
    adsrParams.release = releaseParam->getCurrentValue();

    // Process MIDI events
    if (fc.bufferForMidiMessages != nullptr)
    {
        for (auto m : *fc.bufferForMidiMessages)
        {
            if (m.isNoteOn())
            {
                // Find a free voice to play the note
                for (auto& v : voices)
                {
                    if (!v.active)
                    {
                        v.start(m.getNoteNumber(), m.getFloatVelocity(), (float)sampleRate, adsrParams);
                        break;
                    }
                }
            }
            else if (m.isNoteOff())
            {
                // Find the active voice playing this note and trigger release
                for (auto& v : voices)
                {
                    // Check isKeyDown to ensure we only release voices currently held by a key.
                    // This prevents cutting off a voice that is already in its release phase
                    // if the same note was pressed again quickly (voice stealing overlap).
                    if (v.active && v.currentNote == m.getNoteNumber() && v.isKeyDown)
                    {
                        v.stop();
                        break; // Stop only one voice per NoteOff event
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
    float tuneSemitones = tuneParam->getCurrentValue();
    int waveShape = (int)waveParam->getCurrentValue();

    // Update voice frequencies based on current tuning
    for (auto& v : voices)
    {
        if (v.active)
        {
            float baseFreq = 440.0f * std::pow(2.0f, (v.currentNote - 69 + tuneSemitones) / 12.0f);
            v.targetFrequency = baseFreq;
            v.phaseDelta = v.targetFrequency * juce::MathConstants<float>::twoPi / v.sampleRate;
        }
    }

    // Audio Generation Loop
    for (int i = 0; i < numSamples; ++i)
    {
        float l = 0.0f;
        float r = 0.0f;
        float gain = masterLevelSmoother.getNextValue();

        for (auto& v : voices)
        {
            if (v.active)
            {
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
                            // Triangle waves have naturally low harmonic content (harmonics drop off at 1/n^2),
                            // so aliasing is much less audible than with Saw/Square. 
                            // PolyBLEP is omitted here to save CPU.
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

                // Apply ADSR envelope
                float adsrGain = v.adsr.getNextSample();
                
                // If the envelope has finished, deactivate the voice to save CPU
                if (!v.adsr.isActive())
                    v.active = false;
                
                float currentGain = adsrGain * v.currentVelocity;
                
                // Mix into stereo buffer
                l += sample * currentGain * 0.5f; 
                r += sample * currentGain * 0.5f;
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
    if (v.hasProperty("tune")) tuneValue = v.getProperty("tune");
    if (v.hasProperty("wave")) waveValue = v.getProperty("wave");
    if (v.hasProperty("attack")) attackValue = v.getProperty("attack");
    if (v.hasProperty("decay")) decayValue = v.getProperty("decay");
    if (v.hasProperty("sustain")) sustainValue = v.getProperty("sustain");
    if (v.hasProperty("release")) releaseValue = v.getProperty("release");
}

void SimpleSynthPlugin::Voice::start(int note, float velocity, float sr, const juce::ADSR::Parameters& params)
{
    active = true;
    isKeyDown = true;
    currentNote = note;
    currentVelocity = velocity;
    sampleRate = sr;
    phase = 0.0f; // Reset phase for consistent attack
    targetFrequency = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    phaseDelta = targetFrequency * juce::MathConstants<float>::twoPi / sampleRate;
    
    // Configure and trigger the envelope
    adsr.setSampleRate(sampleRate);
    adsr.setParameters(params);
    adsr.noteOn();
}

void SimpleSynthPlugin::Voice::stop()
{
    isKeyDown = false;
    // Trigger the release phase of the envelope
    adsr.noteOff();
}