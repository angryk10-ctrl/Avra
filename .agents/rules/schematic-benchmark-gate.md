---
trigger: always_on
description: "Schematic-level hardware benchmark, R/L/C component-value physics, DAFx equations, and granular circuit verification gate"
---

# SCHEMATIC-LEVEL HARDWARE BENCHMARK & GRANULAR CIRCUIT VERIFICATION GATE

Whenever a module in Avra Studio is modeled after a real-world hardware circuit (e.g., Pultec EQP-1A3, Neve 1073, UREI 1176 Rev A/E, Teletronix LA-2A, API 550A, Fairchild 670, Roland RE-201, EMT 140, Studer A800, etc.), the C++20 DSP code MUST be grounded in **actual schematic component values ($R, L, C$, transformer turns ratios, tube/transistor part numbers) and published DAFx physical modeling equations**, NOT generic textbook filters.

Note: User-facing UI strings (`SchematicSpec.h`, `UiBuilderSpec.h`, `AvraStudio.h`) must remain 100% free of third-party registered trademarks for commercial legal compliance, while internal C++ DSP source headers (`include/avra/dsp/circuits/CircuitLibrary.h` / `src/dsp/`) document the exact `@schematic_benchmark` circuit topology and physical component constants.

---

## 1. MANDATORY `@schematic_benchmark` C++ HEADER SPEC BLOCK
Every DSP stage header/class in `include/avra/dsp/circuits/` (`src/dsp/`) MUST begin with a formal `@schematic_benchmark` specification block documenting the exact physical circuit being modeled before any code is written:

```cpp
/**
 * @schematic_benchmark
 * - Hardware Inspiration: [or - Software Inspiration: e.g., Passive Offset-RC Program Equalizer + 12AX7/12AU7 Push-Pull]
 * - Active Devices / Iron: [Exact part numbers, e.g., 2N5457 JFET, 2N3055 NPN @ 75mA DC bias, Marinair LO1166]
 * - Passive R/L/C Values: [Exact schematic values, e.g., R_boost=10k, R_atten=100k, C_boost={2uF, 1uF, 0.47uF, 0.22uF}]
 * - Non-Ideal Circuit Quirks Modeled:
 *   1. [Quirk 1: e.g., Simultaneous Boost/Atten resonant under-dip via 10:1 R/C impedance offset]
 *   2. [Quirk 2: e.g., -16dB passive insertion loss restored via high-inductance interstage + push-pull tube stage]
 *   3. [Quirk 3: e.g., Frequency-dependent transformer hysteresis (B ~ V/f) on sub-100Hz fundamentals]
 */
```

---

## 2. COMPONENT-VALUE-DERIVED PHYSICS & PUBLISHED DAFX EQUATIONS
DSP kernels must compute their operating behavior from real physical constants (`constexpr float kR_...`, `kC_...`, `kL_...`, `kTurnsRatio_...`) and published DAFx/IEEE audio engineering equations:
1. **Passive & Inductor EQ Networks (`EQ1`–`EQ5`):**
   - Derive corner frequencies from schematic $R, L, C$ values ($f_c = \frac{1}{2\pi R C}$ and $f_0 = \frac{1}{2\pi \sqrt{L C}}$), model multi-tap toroidal inductor saturation inside the Cytomic TPT ZDF bandpass state ($s_1 \leftarrow s_1 - \alpha s_1^3$), and enforce the `-16 dB` (`0.1585x`) passive bridge insertion loss feeding the `+16 dB` (`6.31x`) tube/Class-A makeup amplifier.
2. **Vacuum Tubes, JFETs, BJTs & Discrete Op-Amps (`PR1`–`PR16`):**
   - Ground transfer curves in Koren's triode/pentode equations ($I_p \propto (V_{gk} + V_{pk}/\mu)^{3/2}$), Shockley JFET square-law transconductance ($I_D = I_{DSS}(1 - V_{GS}/V_P)^2$), Ebers-Moll BJT collector curves with RC emitter/cathode bypass shelf and B+ power-supply sag ($\tau_{\text{charge}}, \tau_{\text{discharge}}$), and explicit op-amp slew-rate clamping ($|dV/dt| \le I_{\text{tail}} / C_{\text{comp}}$).
3. **Transformers, Tape & Hysteresis (`IR1`–`IR5`, `TP1`–`TP3`):**
   - Model Faraday/Jiles-Atherton magnetic flux integration ($\Phi[n] = \alpha \Phi[n-1] + (1-\alpha) V_{\text{in}}[n]$ so $B_{\max} \propto V/f$), primary inductance + inter-winding capacitance resonance, and gap-dependent asymmetric vs. symmetric core pinning.
4. **Feedback/Feed-Forward Compressors & Gain Cells (`CP1`–`CP15`):**
   - Model 1-sample zero-delay feedback FET voltage-divider attenuation with drain-source resistance harmonic cancellation ($R_{DS}(V_{GS})$), dual-time-constant T4B electroluminescent/CdS photocell memory ($65\text{ ms}$ fast + $0.5\text{–}4.0\text{ s}$ program-dependent tail), remote-cutoff Vari-Mu transconductance ($g_m(V_{\text{bias}})$), and padded 4-diode bridge conduction ($I = I_S \sinh(V / \eta V_T)$).
5. **BBD, Tape Delay & Reverb Tanks (`DL1`–`DL10`, `RV1`–`RV9`):**
   - Model 4096-stage bucket-brigade clock-frequency anti-aliasing ($f_{\text{clk}} = N_{\text{stages}} / (2 T_{\text{delay}})$) wrapped in a 2:1 RMS compander, 4-point Hermite cubic fractional delay interpolation, dispersive all-pass spring chirp cascades, and quadrature-modulated Hadamard/Householder FDN tanks.

---

## 3. AUTOMATED SCHEMATIC BENCHMARK AUDIT GATE
`avra_forensic_dsp_audit` and `Avra_Tests` must programmatically inspect `include/avra/dsp/circuits/CircuitLibrary.h` to verify that:
- Every single circuit stage class in the 150-module studio library is preceded by a complete `@schematic_benchmark` specification block (`Hardware Inspiration` / `Software Inspiration`, `Active Devices / Iron`, `Passive R/L/C Values`, and `Non-Ideal Circuit Quirks Modeled`).
- Explicit physical schematic component constants ($R, L, C$, bias points, time constants) and all forensic DSP checks pass with zero failures.
