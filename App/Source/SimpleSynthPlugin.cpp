#include "SimpleSynthPlugin.h"

SimpleSynthPlugin::SimpleSynthPlugin(te::PluginCreationInfo info) 
    : te::Plugin(info)
{
    auto um = getUndoManager();

    levelValue.referTo(state, "level", um, 0.0f);
    tuneValue.referTo(state, "tune", um, 0.0f);
    waveValue.referTo(state, "wave", um, 0.0f);

    levelParam = addParam("level", "Level", {-100.0f, 0.0f});
    tuneParam = addParam("tune", "Tune", {-24.0f, 24.0f});
    
    // Waveform parameter: 0=Sine, 1=Triangle, 2=Saw, 3=Square, 4=Noise
    waveParam = addParam("wave", "Wave", {0.0f, 4.0f, 1.0f}); 
    
    levelParam->attachToCurrentValue(levelValue);
    tuneParam->attachToCurrentValue(tuneValue);
    waveParam->attachToCurrentValue(waveValue);
}

SimpleSynthPlugin::~SimpleSynthPlugin()
{
    notifyListenersOfDeletion();
    
    levelParam->detachFromCurrentValue();
    tuneParam->detachFromCurrentValue();
    waveParam->detachFromCurrentValue();
}

void SimpleSynthPlugin::initialise(const te::PluginInitialisationInfo& info)
{
    for (auto& v : voices)
    {
        v.sampleRate = (float)info.sampleRate;
        v.random.setSeedRandomly();
    }
        
    masterLevelSmoother.reset(info.sampleRate, 0.02); // 20ms smoothing
}

void SimpleSynthPlugin::deinitialise()
{
}

void SimpleSynthPlugin::applyToBuffer(const te::PluginRenderContext& fc)
{
    if (fc.destBuffer == nullptr)
        return;

    fc.destBuffer->clear();

    // Handle MIDI
    if (fc.bufferForMidiMessages != nullptr)
    {
        for (auto m : *fc.bufferForMidiMessages)
        {
            if (m.isNoteOn())
            {
                for (auto& v : voices)
                {
                    if (!v.active)
                    {
                        v.start(m.getNoteNumber(), m.getFloatVelocity(), (float)sampleRate);
                        break;
                    }
                }
            }
            else if (m.isNoteOff())
            {
                for (auto& v : voices)
                {
                    if (v.active && v.currentNote == m.getNoteNumber())
                    {
                        v.stop();
                        break; // Stop one voice per note off
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

    // Process Audio
    float* left = fc.destBuffer->getWritePointer(0);
    float* right = fc.destBuffer->getWritePointer(1);
    int numSamples = fc.bufferNumSamples;

    // Update smoothing target
    masterLevelSmoother.setTargetValue(juce::Decibels::decibelsToGain(levelParam->getCurrentValue()));

    // Parameters
    float tuneSemitones = tuneParam->getCurrentValue();
    int waveShape = (int)waveParam->getCurrentValue();

    // Pre-calculate phase deltas
    for (auto& v : voices)
    {
        if (v.active)
        {
            float baseFreq = 440.0f * std::pow(2.0f, (v.currentNote - 69 + tuneSemitones) / 12.0f);
            v.targetFrequency = baseFreq;
            v.phaseDelta = v.targetFrequency * juce::MathConstants<float>::twoPi / v.sampleRate;
        }
    }

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
                
                switch (waveShape)
                {
                    case sine:
                        sample = std::sin(v.phase);
                        break;
                    case triangle:
                        {
                            float t = v.phase / juce::MathConstants<float>::twoPi;
                            sample = 2.0f * std::abs(2.0f * t - 1.0f) - 1.0f; 
                        }
                        break;
                    case saw:
                        sample = 2.0f * (v.phase / juce::MathConstants<float>::twoPi) - 1.0f;
                        break;
                    case square:
                        sample = (v.phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
                        break;
                    case noise:
                        sample = v.random.nextFloat() * 2.0f - 1.0f;
                        break;
                }

                v.phase += v.phaseDelta;
                if (v.phase >= juce::MathConstants<float>::twoPi)
                    v.phase -= juce::MathConstants<float>::twoPi;

                // Simple release envelope
                float currentGain = v.currentLevel;
                if (v.currentNote == -1) // Release phase
                {
                    v.currentLevel *= 0.99f; // Simple exponential decay
                    if (v.currentLevel < 0.001f)
                        v.active = false;
                }
                
                l += sample * currentGain * 0.5f; // Split to stereo
                r += sample * currentGain * 0.5f;
            }
        }

        left[i] = l * gain;
        right[i] = r * gain;
    }
}

void SimpleSynthPlugin::restorePluginStateFromValueTree(const juce::ValueTree& v)
{
    if (v.hasProperty("level"))
        levelValue = v.getProperty("level");
    if (v.hasProperty("tune"))
        tuneValue = v.getProperty("tune");
    if (v.hasProperty("wave"))
        waveValue = v.getProperty("wave");
}

void SimpleSynthPlugin::Voice::start(int note, float velocity, float sr)
{
    active = true;
    currentNote = note;
    currentVelocity = velocity;
    sampleRate = sr;
    currentLevel = velocity;
    phase = 0.0f;
    targetFrequency = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    phaseDelta = targetFrequency * juce::MathConstants<float>::twoPi / sampleRate;
}

void SimpleSynthPlugin::Voice::stop()
{
    currentNote = -1; // Signal release
}