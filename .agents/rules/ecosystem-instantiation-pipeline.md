---
trigger: always_on
description: "Mandatory 5-Step Ecosystem Instantiation & Auto-Plumbing Pipeline for all 150 Studio Modules"
---

# MANDATORY 5-STEP ECOSYSTEM INSTANTIATION & AUTO-PLUMBING PIPELINE

Every single component in the 150-module Studio Engineering Catalog MUST be a first-class citizen of the **Avra Studio Ecosystem**. You are STRICTLY FORBIDDEN from registering any module as a bare DSP class without its complete UI auto-generation, parameter binding, under-the-hood protection plumbing, and exporter wiring.

Enforce this **5-Step Ecosystem Contract** across all 150 modules:

---

## 1. STEP 1 — SCHEMATIC NODE & ELECTRONIC SYMBOL REGISTRATION
When a user clicks `[+]` or drags any module from the 7 library tabs onto the `+ SCHEMATIC BUILDER` canvas:
- Render the node with its category-accurate **electronic schematic vector symbol** (`Transistor`, `Dual-Triode Valve`, `Transformer Coils`, `Inductor/Capacitor Tank`, `Diode-Bridge/VCA`, `Tape/Delay Head`, or `Crossover Matrix`).
- Populate the **Stage Component Inspector** at the bottom of the Schematic Builder with all 6–10 stage parameters, series/parallel routing controls, and the live C++20 `stageN_.setParameters(...)` / `stageN_.processStereo(l, r)` preview.

---

## 2. STEP 2 — UNDER-THE-HOOD "SMART PLUMBING" & CIRCUIT PROTECTION AUTO-INJECTION
Every module manifest MUST declare its `SmartPlumbingFlags`, and the DSP graph compiler MUST automatically inject and display these protection stages in green (`SMART PLUMBING (AUTO-INJECTED): [...]`):
1. **`[DC-GUARD 5Hz]` (Asymmetrical DC Blocker):** Automatically injected at the output of every asymmetrical tube, single-ended Class-A transistor, JFET, diode clipper, and rectifier stage (`y[n] = x[n] - x[n-1] + 0.9995 * y[n-1]`) so DC offset never corrupts downstream compressors or reverbs.
2. **`[OVERSAMPLE 4x/8x ADAA]` (Local Anti-Aliasing Island):** Automatically wraps high-harmonic waveshapers, hard clippers, and fast FET/diode limiters in a zero-aliasing oversampling or ADAA island so ultrasonic harmonics never fold back into the audible band.
3. **`[PASSIVE MAKEUP +14dB..+18dB]` (Insertion Loss Restoration):** Automatically restores unity gain after passive RLC filter networks before hitting their interstage transformer and tube/Class-A makeup amplifier.
4. **`[DRIFT 1.5%–2.5% + THERMAL -95dB]` (Analog Tolerance & Bias Floor):** Automatically applies seeded Left/Right component tolerance offsets and subtle thermal bias dither so stereo buses exhibit realistic analog width and zero denormal CPU spikes.
5. **`[PHASE / LATENCY SYNC]` (Parallel Branch Delay Compensation):** Whenever a module is placed in `PARALLEL` mode or inside a multi-band/spectral splitter (`SP_A`–`SP_H`, `SP1`–`SP13`), automatically measure the branch's exact group/FIR sample latency and insert a matching sample-accurate delay line on parallel dry/companion paths so summing is 100% phase-coherent.
6. **`[SAFETY SUB-SONIC & NAN CLAMP]` (Runaway Feedback Protection):** Inside self-oscillating ladder filters (`EQ10`) and high-feedback tape/BBD/reverb tanks (`DL1`–`DL12`, `RV1`–`RV15`), automatically place a `15 Hz` infrasonic guard and a `+18 dBFS` safety soft-limiter inside the feedback loop so runaway resonance can never blow the user's speakers or produce `NaN`/`Inf`.

---

## 3. STEP 3 — AUTOMATIC FACEPLATE UI GENERATION & WIDGET HIERARCHY
Adding or hot-swapping a module in the Schematic Builder MUST immediately synchronize with `+ FACEPLATE UI BUILDER` and `LIVE TEST RACK`:
- Automatically construct a framed hardware sub-panel (`S1 · [MODULE TITLE]`) on the faceplate containing all **6 to 10 interactive controls** + **1 live telemetry meter** defined in that module's 4-Tier manifest:
  - **Tier 1 (Input/Conditioning):** Small 7-layer rotary knobs (`Input Trim dB`, `HPF/LPF Hz`, `Pre-Delay ms`).
  - **Tier 2 (Core Physics Engine):** Large 7-layer primary rotary knobs (`Threshold/Ratio/Attack/Release`, `Multi-Band Freq/Gain/Q`, `RT60 Decay s`) with dead-center 12 o'clock `0 dB` alignment and proper `-∞` symbols.
  - **Tier 3 (Circuit Voice):** 3D Chrome Bat-Handle switches + Recessed Jewel LEDs or Stepped Rotaries (`Core Material`, `Triode/Pentode`, `LN Mode`, `Head Select`).
  - **Tier 4 (Output & Telemetry):** `Output Trim (dB)`, `Link IO` switch (inversely coupling Input Drive and Output Trim), `Dry/Wet Mix (%)`, and a **Module-Specific Live Telemetry Widget** (Backlit VU Needle for compressors, 3-LED Headroom Bar for preamps/iron, or Spectral Delta Bar for EQs/unmaskers).
- Applying any `UNIFIED UI STYLE TEMPLATE` must cleanly re-skin all auto-generated widgets on that sub-panel while preserving the user's custom stickers, tape locks, and layout tweaks.

---

## 4. STEP 4 — TWO-WAY LOCK-FREE PARAMETER BINDING
- **Zero Unbound Widgets:** Every auto-generated faceplate control MUST have its `AUTO-BOUND: S<N> <ParameterName>` binding pre-wired to the real-time audio thread via our lock-free SPSC queue, and appear in the `BIND TO DSP PARAMETER ▾` dropdown in the Widget Inspector.

---

## 5. STEP 5 — AUTOMATED ECOSYSTEM AUDIT CHECK (`CHECK 6` IN FORENSIC HARNESS)
`tests/avra_forensic_dsp_audit.cpp` MUST run **`CHECK 6: Full Ecosystem Integration Audit`**. For all 150 studio modules in the catalog, programmatically instantiate the module through the application ecosystem controller and assert that:
1. `node.getSmartPlumbingFlags() != 0` (appropriate DC-Guard, Drift, Makeup, Oversampling, Phase/Latency Sync, or Feedback-Safety protection is active).
2. `faceplate.getWidgetsForStage(stageIndex).size() >= 7` (at least 6 interactive controls + 1 live telemetry meter are generated on the faceplate).
3. Every generated faceplate widget has a valid, non-null `parameterBindingId` that mutates the underlying C++20 DSP state when turned.
4. `cppExporter.generateStageCode(stageIndex)` outputs valid, compilable C++20 code that includes both the core C++20 DSP stage class and its Smart Plumbing protection wrapper.
