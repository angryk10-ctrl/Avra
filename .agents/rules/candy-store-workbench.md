---
trigger: always_on
description: "Live Test Rack Calibration, Tactile Hardware Widgets, Studio Blueprints, and 51 Unified UI Style Templates"
---

# LIVE TEST RACK, TACTILE HARDWARE WIDGETS & STUDIO BLUEPRINT STANDARDS

## PHASE 1: LIVE TEST RACK CALIBRATION & TELEMETRY SCOPES
1. **Unity Gain Calibration (`OUT-IN` Delta = `0.0 dB` at Unity):**
   - When `INPUT GAIN` and `MASTER OUTPUT` are at `0.0 dB` (unity), the meter must display an exact `0.0 dB` `OUT-IN` delta (no uncompensated pan-law or hidden stage boost mismatch).
2. **Stereo Balance / Pan Skirt Labels:**
   - `STEREO BAL` uses studio-standard `L100 — L50 — C — R50 — R100` radial skirt labels with `CENTER` at 12 o'clock.
3. **Top Test Bar Monitoring & Signal Generators:**
   - `Δ DELTA` (Null Listen) latch button outputs `(Wet - Dry)` so the user hears strictly what the circuit is adding or removing.
   - `AUTO-LEVEL` (`LUFS MATCH`) toggle automatically offsets the output monitor level by the negative of the live `OUT-IN` RMS delta so toggling `BYPASS` is loudness-matched.
   - Test Signal Generator Bar includes `DRUMS`, `SYNTH`, `CLEAN GTR`, `DIST GTR`, `BASS DI`, `VOCAL`, `WAV...`, `PINK`, and `HARMONICS`.
4. **`HARMONICS` "Plugin Doctor" THD Analyzer & Silent Analyzer Mode:**
   - When `HARMONICS` (1 kHz test sine at `-12 dBFS`) is active, expose a `SILENT` analyzer mode toggle that mutes Windows speaker output while keeping the DSP and 60fps FFT Spectrum running.
   - In `POST-PLUGIN FFT SPECTRUM`, draw color-coded harmonic markers at Even Harmonics (`2 kHz`, `4 kHz`, `6 kHz` in Gold/Cyan) and Odd Harmonics (`3 kHz`, `5 kHz`, `7 kHz` in Orange/Red), plus a live `THD: X.XX% (Even vs. Odd Ratio)` badge.
5. **Dual-Trace Pre/Post Overlay & Transfer Knee / Phase Scope:**
   - Both `POST-PLUGIN FFT SPECTRUM` and `OUTPUT WAVEFORM / TRANSIENT SCOPE` render the Pre-Plugin Input (`IN`) as a translucent grey reference shadow behind the Post-Plugin Output (`OUT`) trace.
   - The middle telemetry box includes a toggleable `WAVEFORM | TRANSFER KNEE | PHASE SCOPE` mode switch.

## PHASE 2: TACTILE HARDWARE EYE-CANDY WIDGETS (FACEPLATE UI BUILDER)
1. **Interactive Spinning Tape Reel / Magnetic Drum Viewport** (`7.5`, `15`, `30 IPS` + mouse drag braking/flanging).
2. **Glowing Vacuum Tube Porthole Window** (dynamic heater & blue ionization glow driven by live THD & drive).
3. **6E5 "Magic Eye" Green Phosphor Tube Meter** (dynamic shadow iris closing with GR / Peak).
4. **Cold-Cathode Nixie Tube Readout** (glowing orange numerals for `ms`, `Hz`, `dB`).
5. **2D XY Vector Chaos / Morph Pad** (4-corner parameter morph pad).
6. **Console Scribble-Tape, Gaff Tape & Dymo Label Strips**.

## PHASE 3: SCHEMATIC "HOT-WIRES" & 6 STUDIO BLUEPRINTS
1. **Live Signal-Glowing Wires & CPU/Latency Node Badges** (`0.1% | 0 smpl`).
2. **6 Multi-Stage Studio Blueprints** (`DRUM BUS`, `BASS GTR`, `CLEAN GTR`, `DIST GTR`, `VOCAL`, `SYNTH / KEYS`).
3. **Per-Module Series & Parallel (`Y / Σ`) Routing with Wet/Dry Blend & `180°` Phase Polarity**.

## PHASE 4: 51 UNIFIED UI STYLE TEMPLATES
- Full 6-layer procedural surface, SVG knob silhouette, switch archetype, meter skin, label plaque, custom backdrop artwork, and corner fastener switching across 51 studio themes.
