21. **Bugfix: Rack Container Audio Routing (Channel Capabilities)**
    *   **Issue:** When used inside a Rack Container, `SimpleSynthPlugin` was sometimes connected incorrectly (e.g., Mono/Right channel only) or caused routing failures.
    *   **Cause:** The plugin relied on default `te::Plugin` channel reporting, which was ambiguous for the Rack's auto-patching algorithm.
    *   **Fix:** Explicitly overrode `getChannelNames` and `getNumOutputChannelsGivenInputs` to declare **0 Inputs** and **2 Outputs (Stereo)**.
        *   **Result:** The Rack Container now correctly identifies the synth as a Stereo source and wires its outputs to the Rack's Left (Pin 1) and Right (Pin 2) channels.
    
    22. **Design Philosophy: High-Quality Filtering**
        *   **Ladder Filter (Luxury Model):** Unlike many implementations that update filter coefficients at control rates (e.g., every 32/64 samples), the `SimpleSynth`'s Ladder Filter calculates and updates its coefficients for every single sample. 
            *   **Benefit:** This provides maximum precision and prevents "zipper noise" or artifacts during extremely fast cutoff modulations (e.g., fast envelopes or audio-rate FM).
            *   **Trade-off:** This is CPU-intensive but intentional to maintain "Luxury" audio quality.
        
        23. **Sound Design & Safety Tweaks (Feb 2026)**
            *   **Resonance Boost:** 
                *   SVF Q-factor range increased from ~10 to ~40 for modern, sharp resonance peaks (Vital-style).
                *   Ladder Filter resonance range extended to 115% to drive internal saturation into self-oscillation territory.
            *   **Gain Staging:** Oscillator raw levels reduced by 6dB (0.5x) to provide headroom for high resonance and polyphonic summing.
            *   **Output Protection:** Implemented a lightweight algebraic Soft Clipper (`x / (1 + |x|)`) at the final output stage. This prevents harsh digital clipping and adds musical saturation when driving the synth hard.
        