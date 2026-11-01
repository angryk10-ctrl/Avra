---
trigger: always_on
description: "Mandatory DSP physics, 4-tier UI parameter density, auto-plumbing rules, and anti-placebo verification standards"
---

# MANDATORY ARCHITECTURE, UI FACEPLATE & DSP QUALITY LAWS

## LAW 1: THE 4-TIER FACEPLATE PARAMETER GRAMMAR (NO 2-KNOB MODULES)
Every module added to the Faceplate Builder MUST expose **6 to 10 interactive controls** plus **1 live visual telemetry meter**, structured across 4 mandatory tiers:
- **Tier 1 — Input & Conditioning:** `Input Drive/Trim` (`dB`), `HPF/LPF Scope Bounds` (`Hz`), `Pre-Delay` (`ms`), or `Impedance/Pad Switch`.
- **Tier 2 — Core Physics Engine:** Primary controls (`Threshold/Ratio/Attack/Release`, `Multi-band Gain/Freq/Q`, `Decay RT60`, `Recover/Tame`).
  - *Multi-Band EQ Law:* NEVER hardcode frequencies into static text labels. Every EQ band MUST render its own sweepable or stepped `Frequency (Hz/kHz)` control, `Gain (dB)` knob, `Q/Bandwidth` knob, and `Bell/Shelf` switch.
- **Tier 3 — Circuit Voice & Nonlinearity:** Hardware character controls (`Bias/Asymmetry`, `Core Material Switch [Nickel/Iron/Steel]`, `Triode/Pentode`, `HF Damping`, `Wow/Flutter`, `Detector Topology`).
- **Tier 4 — Output, Blending & Mandatory Telemetry:** `Output Trim (dB)`, `Link IO` toggle (requires BOTH Input and Output knobs to exist on the panel and inversely couples them), `Dry/Wet Mix (%)`, and **1 Live Telemetry Widget** (`Ballistic VU Gain Reduction`, `3-LED Headroom Ladder`, `Gate Open/Closed LED`, `Mini EQ Curve`, or `Spectral Delta Bar`).

## LAW 2: STRICT BAN ON FAKE `%` UNITS & VISUAL HIERARCHY
Never use `0.0 % to 100.0 %` for physical acoustic quantities:
- **Time** (`Attack`, `Release`, `Hold`, `Pre-Delay`, `Delay Time`): MUST use **`ms`** with logarithmic skew (`0.35–0.5`).
- **Reverb Decay (`RT60`)**: MUST use **`s`** (`0.2 s` to `25.0 s`).
- **Frequency** (`Cutoff`, `Band Freq`, `Damping`, `Focus Bounds`): MUST use **`Hz` / `kHz`** with logarithmic octave scaling (`0.25`).
- **Level / Drive / Threshold / Ceiling / Makeup**: MUST use **`dB`**.
- **Widget Variety:** Never render a uniform wall of identical knobs. Use `LARGE_KNOB` for Tier 2 core controls, `SMALL_KNOB` for Tier 1/3 trims/filters, and `SWITCH_3WAY`, `PILL_TOGGLE`, or `STEPPED_ROTARY` for circuit modes.

## LAW 3: SMART SCHEMATIC AUTO-PLUMBING
When a module is instantiated in the schematic graph, automatically wire these conditioning behaviors in the DSP engine:
1. **Asymmetrical DC Blocker:** Any asymmetrical wave-shaper, tube, or Class-A stage (`PR1`–`PR15`, `ST1`–`ST7`) MUST pass through a 1st-order `5 Hz` DC Blocker (`y[n] = x[n] - x[n-1] + 0.9995 * y[n-1]`) at its output.
2. **Analog Tolerance & Thermal Floor:** Vintage hardware modules automatically apply a seeded `1.5%` Left/Right component tolerance offset and `-90 dBFS` thermal bias dither.
3. **Passive Insertion Loss Makeup:** Passive EQ networks (Pultec, Massive Passive) automatically include unity gain restoration (`+16 dB`) feeding their tube/iron makeup stage.
4. **Parallel Latency Sync:** Parallel splits automatically delay-compensate branches so summing is sample-accurate and phase-coherent.

## LAW 4: ANTI-PLACEBO DSP LAWS (NO FAKE OR DUPLICATE MATH)
You are STRICTLY FORBIDDEN from writing stub, placeholder, or duplicate DSP math across modules:
1. **100% Parameter-to-DSP Wiring (Zero Dead Knobs):** Every single parameter in a module's UI manifest MUST directly modify the sample-by-sample math inside the audio processing loop.
2. **Per-Sample Parameter Smoothing:** All continuous parameters (`gain`, `freq`, `delay`, `drive`, `mix`) MUST be smoothed per sample (`z += coeff * (target - z)`) to prevent zipper noise.
3. **Zero-Delay Feedback (ZDF) Filters:** Do NOT use uncompensated RBJ Biquads that cramp near Nyquist. Use Topology-Preserving Transform (TPT) / Cytomic ZDF State-Variable Filters (`SVF`).
   - *Inductor EQs (`EQ2`, `EQ3`, `EQ4`):* Apply odd-order coil saturation (`s1 - a * s1^3`) inside the ZDF bandpass state.
   - *Proportional-Q EQs (`EQ5`, `EQ7`):* Dynamically narrow $Q$ as gain increases: `Q_actual = Q_base * pow(2.0, abs(gainDb) / 5.0)`.
   - *Pultec Low Trick (`EQ1`):* Use independent Boost and Cut low shelves where the Attenuation corner frequency is offset by `1.75x` the Boost corner frequency to produce the authentic resonant mid-scoop.
4. **Frequency-Dependent Iron, Tape & Tube Physics:**
   - *Transformers (`IR1`–`IR5`):* Core saturation scales inversely with frequency ($B \propto V/f$). Pre-emphasize lows (`-6 dB/oct` integrator + `25 Hz` resonant bump) before hysteresis wave-shaping and de-emphasize after, so sub-bass saturates the core while highs stay clean.
   - *Tubes & Class-A Transistors (`PR1`–`PR15`, `ST2`):* Implement asymmetrical positive/negative half-wave transfer curves (generating strong 2nd-order even harmonics), dynamic Miller-capacitance HF roll-off under drive, and an RC envelope follower simulating B+ power supply rail sag under sustained bass load.
5. **Authentic Compressor Detector & Gain-Cell Physics (`CP1`–`CP14`):**
   - *Feedback vs. Feed-Forward:* Feedback compressors (1176, LA-2A, Vari-Mu, Diode Bridge) MUST feed the previous output sample `y[n-1]` into the detector, NOT `x[n]`.
   - *T4B Optical Memory (`CP2`):* Blend a fast release integrator (`60 ms`) with a slow program-dependent memory integrator (`0.5 s` to `4.0 s`) weighted by cumulative gain-reduction history.
   - *FET Distortion (`CP1`):* Inject level-dependent 2nd/3rd harmonic saturation proportional to instantaneous gain reduction + support "All-Buttons-In" bias shift.
6. **Time, Delay & Reverb Tank Physics (`DL1`–`DL10`, `RV1`–`RV9`):**
   - Use **4-point Hermite cubic interpolation** on all modulated fractional delay lines (never linear interpolation).
   - Place Head-Bump resonance (`120 Hz`), HF Damping LPF, wow/flutter LFOs, and soft saturation *inside* the feedback loop of Tape/BBD/Drum delays.
   - Wrap BBD (`DL3`) and 12-Bit Digital (`DL6`) delays in a `2:1` RMS input compressor and `1:2` RMS output expander (NE570 compander model).
   - For Spring Reverb (`RV3`), cascade 8–16 all-pass dispersion filters (`800 Hz – 3.5 kHz`) before the tank to generate mechanical chirp. For Plates/Halls (`RV1`, `RV2`, `RV5`, `RV8`), use 4 series Schroeder input diffusers feeding a quadrature-modulated Hadamard/Householder Feedback Delay Network (FDN).
