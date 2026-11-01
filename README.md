<div align="center">

# AVRA STUDIO
### Modular C++20 Audio Plugin Development Environment & Circuit Designer
**Design Analog & Spectral DSP Chains • Customize Hardware Faceplates • Test Live at 60 FPS • Compile Native VST3, CLAP & Standalone Applications**

![C++20](https://img.shields.io/badge/DSP_Engine-C%2B%2B20_SIMD-00d2b8?style=for-the-badge&logo=cplusplus)
![GUI](https://img.shields.io/badge/GUI-Direct2D_1.1_%40_60FPS-f5a623?style=for-the-badge)
![Formats](https://img.shields.io/badge/Exports-VST3_%7C_CLAP_%7C_EXE-9b59b6?style=for-the-badge)
[![Buy Me a Coffee](https://img.shields.io/badge/Support-Buy_Me_a_Coffee-FFDD00?style=for-the-badge&logo=buy-me-a-coffee&logoColor=black)](https://buymeacoffee.com/Kennedarth)

</div>

---

## Overview

**Avra Studio** is a free, open-source visual development environment and real-time audio workbench for designing, testing, and compiling audio plugins for personal and free community use.

Rather than chaining generic textbook biquad filters, Avra Studio provides **150 C++20 studio circuit modules** across 7 categories modeled around physical circuit behaviors—including gapped Class-A transformers, remote-cutoff vari-mu stages, dual-time-constant T4B optical cells, topology-preserving zero-delay feedback (ZDF) inductor EQs, automatic level riders, and STFT spectral processors.

When a design is ready to export, Avra Studio generates standalone **C++20 source code** and invokes your local MSVC (`cl.exe`) and CMake toolchain to compile native 64-bit **`.vst3`**, **`.clap`**, and **Standalone `.exe`** binaries with zero external SDK downloads required.

---

## Studio Architecture (4 Workspace Views)

### 1. `+ SCHEMATIC BUILDER` (`F1`)
* **Series & Parallel Signal Graph:** Chain up to 8 series or parallel processing stages across **7 categories** (`PRE / TUBE`, `IRON / SAT`, `EQ / SPEC`, `COMP / CLIP`, `TIME / VERB`, `SPLIT / MOD`, and `AUTO / SMART`). Any stage can be switched between series routing and a parallel Y-split (`Y` / `Σ`) with independent Wet/Dry blend and `180°` polarity inversion.
* **Source & Sub-Category Filtering:** Filter the component library by source material (**`All`**, **`Bass`**, **`Guitar`**, **`Drums`**, **`Vocals`**, **`Keys`**), sub-category pills, or text search.
* **6 Pre-Wired Studio Blueprints:** Load complete multi-stage signal chains with one click:
  * **`BLUEPRINT: DRUM BUS`** (5 Stages) — Class-A Transistor Preamp (`PR2`) → Proportional-Q Gyrator EQ (`EQ5`) → Quad-VCA Bus Compressor (`CP5`) → Parallel Fast Feedback FET Limiter (`CP1`, `25%`) → 16x Variable-Knee Clipper (`CL1`).
  * **`BLUEPRINT: BASS GTR`** (6 Stages) — Dual-Triode Tube Pre (`PR4`) → Auto Bass Leveller (`AU2`) → Program-Dependent T4B Optical (`CP2`) → Sub-Harmonic Synthesizer (`ST6`) → Parallel Dark Bass Grit Bus (`ST4`, `25%`) → Passive Offset-RC Low EQ (`EQ1`).
  * **`BLUEPRINT: CLEAN GTR`** (5 Stages) — Dual-Triode Console Pre (`PR13`) → Fast Feedback FET Limiter (`CP1`) → Proportional-Q Gyrator EQ (`EQ5`) → Parallel Tri-Stereo BBD Chorus (`MD2`, `28%`) → Parallel Triple-Helix Studio Spring Reverb (`RV3`, `22%`).
  * **`BLUEPRINT: DIST GTR`** (5 Stages) — Hysteresis Noise Gate (`CP13`) → EF86 Pentode Line Preamp (`PR5`) → 6AS6 Starved Valve Drive (`ST2`) → 3-Band Inductor Console EQ (`EQ4`) → 2-Inch Multitrack Tape (`TP1`).
  * **`BLUEPRINT: VOCAL`** (7 Stages) — Class-A Transistor Preamp (`PR2`) → Auto Vocal Leveller (`AU1`) → Split-Band De-Esser (`CP12`) → Program-Dependent T4B Optical (`CP2`) → 40 kHz Air & Tilt EQ (`EQ8`) → Parallel Varispeed Tape ADT Doubler (`DL10`, `26%`) → Parallel Gold-Foil Plate Reverb (`RV1`, `22%`).
  * **`BLUEPRINT: SYNTH / KEYS`** (6 Stages) — Pure JFET Line Buffer (`PR1`) → Proportional-Q Gyrator EQ (`EQ5`) → Quad-VCA Bus Compressor (`CP5`) → Parallel 4-Mode BBD Chorus (`MD1`, `20%`) → Parallel Ping-Pong Stereo Delay (`DL2`, `15%`) → Parallel Concert Hall FDN Reverb (`RV2`, `18%`).
* **Automatic Signal Conditioning:** The DSP graph compiler automatically applies **5 Hz DC-Guard high-pass blockers** after asymmetrical saturation stages, **seeded L/R component tolerance offsets**, **passive filter insertion-loss makeup (`+16 dB`)**, **polyphase oversampling**, **15 Hz infrasonic and `+18 dBFS` feedback safety clamps**, and **parallel branch latency compensation**.

### 2. `+ FACEPLATE UI BUILDER` (`F2`)
* **Automatic Control Mapping:** Adding a module to the schematic populates its corresponding sub-panel on the faceplate with physical units (`dB`, `Hz`/`kHz`, `ms`, `s`), logarithmic skew laws, `Link IO` inverse gain coupling, top `S1 → S2 → ...` stage selector pills, and live metering.
* **51 Unified UI Style Templates:** Apply complete visual themes across a 3-column selector—including *Kennedarth Teal Console*, *British Console Slate*, *FET Blackface Studio*, *Passive Hammertone Blue*, *German Broadcast Lab*, *Vintage Tube Bakelite*, *Crimson Mastering Lab*, *Swiss Mastering Transfer*, *Nordic Spectral Glass*, *Modern Minimalist*, and custom artwork backdrops. Custom themes can also be saved to the user theme list.
* **Widget & Chassis Customization:** Customize **34 rotary knob and fader styles**, **10 switch styles**, **18 meters and scopes**, **32 studio decals/tape strips** (plus custom `.PNG` decal imports), chassis finishes, mounting hardware, label plate colors (`PLATE COL`), and riveted nameplates.
* **Console Wax-Pencil Marks & Tape Locks:** `Shift + Right-Click` any rotary knob skirt to place a sweet-spot reference mark, or place gaff/masking tape decals over controls to lock them against accidental edits.

### 3. `LIVE TEST RACK` (`F3`)
* **Built-In 48 kHz Test Signal Generator:** Audition chains in real time using synthesized 120 BPM studio test stems—**`DRUMS`** (4/4 acoustic kit), **`SYNTH`** (polyphonic chord progression), **`CLEAN GTR`** (A–E–D–A open chord progression), **`DIST GTR`** (A–E–D–A driven power chords), **`BASS DI`** (A–E–D–A bassline), **`VOCAL`** (A–E–D–A sustained harmonic progression), or **`WAV...`** (load any custom `.wav` file)—plus **`PINK`** noise and **`HARMONICS`** (`1 kHz` reference sine at `-12 dBFS`) with **`SILENT`** analyzer mode.
* **Live Telemetry Scopes & Lab Bench:** Inspect pre/post signals on a **Dual-Trace FFT Spectrum Analyzer (`20 Hz – 20 kHz`)** with even/odd harmonic markers and live `THD %` readout, a switchable **`Waveform | Transfer Knee | Phase Scope`** display, a **`LAB BENCH`** analyzer mode (Bode Magnitude/Phase, Harmonic THD Spectrum, and Dynamic Transfer curves), and **In / Out / Out-In Peak & RMS meters**.
* **Critical Auditioning Tools:** Includes **`Δ DELTA`** (wet-minus-dry null difference listen), **`AUTO-LEVEL`** (loudness-matched output compensation), **`A | B`** comparison banks, **`PROBE`** (click to overlay or solo any inter-stage signal tap), and live **`PDC`** latency reporting.

### 4. `BUILD & EXPORT` (`F4`)
* **Native Binary Compilation:** Toggle **`.vst3`**, **`.clap`**, and **Standalone `.exe`** targets and click **`BUILD VST3 + CLAP + APP`** (`F7` or `BUILD` menu). A Windows File Explorer dialog lets you choose the output directory and plugin name before compiling via CMake and MSVC.
* **Menu Bar & Keyboard Shortcuts:** Full workstation menu bar (`FILE`, `EDIT`, `VIEW`, `BUILD`, `HELP`) with 50-step Undo/Redo (`Ctrl+Z` / `Ctrl+Y`), Auto-Save backups (`.avra.bak`), view switching (`F1`–`F4`), one-click compilation (`F7`), forensic DSP audit (`F8`), IDE handoff (`F9` / `Ctrl+Shift+O`), and shortcuts reference (`F12`).

---

## Component Library (7 Categories, 150 Studio Modules)

Every module in Avra Studio is backed by a C++20 DSP class in [`include/avra/dsp/circuits/CircuitLibrary.h`](include/avra/dsp/circuits/CircuitLibrary.h) with an explicit `@schematic_benchmark` specification block. The Schematic Builder library provides **150 studio modules** across 7 categories (`24 + 25 + 25 + 26 + 24 + 21 + 5 = 150`):

| Category | Modules | Representative Topologies |
| :--- | :--- | :--- |
| **`PRE / TUBE`** | 24 | Discrete JFET Buffer (`PR1`), British 73 Class-A Pre (`PR2`), American Discrete Op-Amp Pre (`PR3`), Dual-Triode 12AX7/12BH7 Pre (`PR4`), EF86 Pentode Line Pre (`PR5`), German Broadcast V76 Valve (`PR6`), PNP Germanium Pre (`PR7`), 120V High-Rail Pre (`PR9`), Mastering Transfer Console (`PR21`), Current-Mode Transimpedance Amp (`PR24`). |
| **`IRON / SAT`** | 25 | British Gapped Class-A Iron (`IR1`), 80% Nickel Bridging Iron (`IR2`), Trifilar Punch Iron (`IR3`), Multi-Core Matrix (`IR4`), Amorphous Core (`IR8`), 2-Inch Mastering Tape (`TP1`), 1/2-Inch Two-Track Tape (`TP4`), Dual-Band Tube/MOSFET Saturator (`ST1`), 6AS6 Starved Pentode (`ST2`), Polynomial Loudness Inflator (`ST3`), Vinyl Cutting Lathe (`ST8`). |
| **`EQ / SPEC`** | 25 | Passive Offset-RC Program EQ (`EQ1`), 3-Band Multi-Tap Inductor EQ (`EQ3`), Proportional-Q Gyrator EQ (`EQ5`), Console 4K Parametric (`EQ6`), 4-Band Parallel Passive LC (`EQ8`), 7-Band Toroidal Graphic EQ (`EQ13`), Reciprocal Mastering Shelf (`EQ15`), Elliptical M/S EQ (`EQ19`), Bark Auditory Unmasker (`SP1`), 32-Band Spectral Carver (`SP2`). |
| **`COMP / CLIP`** | 26 | Fast Feedback FET Limiter (`CP1`), Program-Dependent T4B Optical (`CP2`), Vari-Mu 6386 Push-Pull (`CP3`), 4-Diode Bridge Limiter (`CP4`), Quad-VCA Bus Compressor (`CP5`), Multi-Ratio Stress VCA (`CP6`), Split-Band De-Esser (`CP10`), Hysteresis Gate (`CP11`), Zener Limiter (`CP13`), Oversampled Mastering Clippers & Limiters (`CL1`–`CL5`). |
| **`TIME / VERB`** | 24 | Multi-Head Magnetic Tape Echo (`DL1`), Ping-Pong Stereo Delay (`DL2`), Companded MN3005 BBD Delay (`DL3`), Magnetic Drum Echo (`DL5`), 12-Bit Companded Rack Delay (`DL6`), Grain Cloud Delay (`DL9`), Octave Shimmer FDN (`SH1`), Modulated Hall & Plate FDN (`RV1` / `RV2`), Counter-Wound Spring Tank (`RV3`), Gated Drum Room (`RV4`). |
| **`SPLIT / MOD`** | 21 | Linkwitz-Riley Crossovers (`SP_A` / `SP_B`), Elliptical M/S Matrix (`SP_C`), Dynamic Ducking Router (`SP_E`), 4-Mode Spatial Dimension Chorus (`MD1`), Tri-Stereo BBD Chorus (`MD2`), Optical Vibe Phaser (`MD4`), Dual-Rotor Cabinet (`MD7`), Through-Zero Rack Flanger (`MD9`), Binaural 3D Panner (`MD11`), Macro Hub & Slope Generators (`CV1` / `CV2`). |
| **`AUTO / SMART`** | 5 | Vocal Rider (`AU1`), Fundamental-Isolated Auto Bass Rider (`AU2`), 4-Band Dynamic EQ (`AU3`), Dynamic Bleed Suppressor (`AU4`), K-Weighted Auto LUFS Gain-Stager (`AU5`). |

---

## System & Toolchain Prerequisites

1. **OS:** Windows 10 / 11 (x64) with Direct2D 1.1 support.
2. **C++20 Compiler (for plugin export):** Visual Studio 2022+ or Visual Studio Build Tools (`cl.exe` with the *Desktop development with C++* workload).
3. **CMake (for plugin export):** Version `3.20+` available in `PATH`.
4. **IDE (Optional):** Visual Studio Code (`code.exe`) or Visual Studio (`devenv.exe`).

*(No external audio plugin SDKs are required; Avra Studio includes self-contained VST3, CLAP, and Win32 WASAPI/Direct2D headers under [`include/avra/`](include/avra/).)*

---

## Quick Start Guide

### Option 1: Pre-Built Windows Installer (`Avra_Studio_Setup_Win64.exe`) or Portable ZIP (`Avra_Studio_Win64.zip`)
Both a self-contained Windows x64 GUI installer and a portable archive containing `Avra.exe`, all UI/audio assets, and the C++20 plugin headers are available on the **[GitHub Releases](https://github.com/angryk10-ctrl/Avra/releases/latest)** page:

* **Windows Setup Installer (Recommended):** Download and run **[`Avra_Studio_Setup_Win64.exe`](https://github.com/angryk10-ctrl/Avra/releases/latest/download/Avra_Studio_Setup_Win64.exe)** to choose an installation directory (default `C:\Avra`), create Desktop and Start Menu shortcuts, and launch Avra Studio.
* **Portable ZIP Archive:** Download **[`Avra_Studio_Win64.zip`](https://github.com/angryk10-ctrl/Avra/releases/latest/download/Avra_Studio_Win64.zip)**, extract it to any local directory, and run **`Avra.exe`**.

*(Designing signal chains, customizing faceplates, and auditioning audio in the Live Test Rack work immediately without a compiler installed. Compiling `.vst3`, `.clap`, or `.exe` binaries via **`BUILD VST3 + CLAP + APP`** requires MSVC `cl.exe` and CMake `3.20+`.)*

### Option 2: Build from Source
```bash
# 1. Clone the repository
git clone https://github.com/angryk10-ctrl/Avra.git
cd Avra

# 2. Configure and build Avra Studio (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Avra_Studio

# 3. Launch Avra Studio
./build/Release/Avra.exe
```

### Workflow Summary
1. Launch **`Avra.exe`**.
2. In **`+ SCHEMATIC BUILDER`** (`F1`), select one of the **6 Studio Blueprints** across the top bar or click **`[+]`** on any module card in the left library to assemble a chain.
3. Switch to **`+ FACEPLATE UI BUILDER`** (`F2`) to choose from the **51 Unified UI Style Templates** or customize individual knob, switch, meter, and decal widgets.
4. Switch to **`LIVE TEST RACK`** (`F3`) and click any test signal button (`DRUMS`, `SYNTH`, `CLEAN GTR`, `DIST GTR`, `BASS DI`, `VOCAL`, `WAV...`, `PINK`, or `HARMONICS`) to audition the circuit and inspect the live FFT spectrum, waveform/knee/phase scope, or `LAB BENCH` plots.
5. Click **`BUILD VST3 + CLAP + APP`** (`F7`), choose a destination folder and plugin name in the File Explorer dialog, and compile your `.vst3`, `.clap`, and `.exe` binaries.

---

## DSP Architecture & Verification Suite

All modules in `CircuitLibrary.h` are checked by automated test binaries (`Avra_Tests`, `avra_forensic_dsp_audit`, and `avra_control_spec_audit`):
* **Parameter-to-DSP Wiring:** Every exposed UI parameter is verified to alter sample-by-sample DSP output across its range (`> -75 dBFS` RMS delta).
* **Topology-Preserving ZDF Filters:** Equalizer and crossover stages use Cytomic TPT/ZDF State-Variable Filters (`SVF`) with inductor state saturation ($s_1 - \alpha s_1^3$) and proportional-$Q$ scaling.
* **Frequency-Dependent Magnetic Hysteresis:** Transformer and tape stages integrate flux ($\Phi \propto \int V\,dt$, scaling core saturation as $B \propto V/f$) so low frequencies drive core nonlinearity while high frequencies remain clean.
* **Feedback Detectors & Optical Memory:** Feedback compressors (`CP1`–`CP4`) route `y[n-1]` into the detector path, and T4B optical stages model dual-time-constant electroluminescent recovery.
* **Per-Sample Parameter Smoothing:** Continuous parameters use per-sample exponential smoothing (`z += coeff * (target - z)`) to prevent zipper noise during automation and CLAP parameter modulation (`CLAP_EVENT_PARAM_MOD`).

To run the test suite from source:
```bash
cmake --build build --config Release --target avra_forensic_dsp_audit avra_control_spec_audit Avra_Tests
./build/Release/avra_forensic_dsp_audit.exe
./build/Release/avra_control_spec_audit.exe
./build/Release/Avra_Tests.exe
```

---

## Real-World DAW Stress-Test Benchmarks

The following real-time playback benchmarks were recorded in **Steinberg Cubase** (64-bit Windows, 24-bit session format) using native `.vst3` multi-stage plugins compiled by Avra Studio.

### Reference Test Hardware
* **Processor:** 12th Gen Intel Core i5-12400F (2.50 GHz base, 6 cores / 12 threads)
* **Memory:** 32.0 GB RAM
* **Graphics:** NVIDIA GeForce RTX 3050 (8 GB)
* **Audio Interface:** Antelope Audio Discrete 4 Synergy Core

### Measured Session Stability

| Test Scenario | Track Count | Per-Track Signal Load | Sample Rate / Bit Depth | Buffer Size | Result |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Test 1A — Heavy Instrument + Multi-Stage VST3** | 22 Tracks | Superior Drummer 3 + Avra Bass Guitar Blueprint `.vst3` | `192.0 kHz` / 24-bit | `4096 samples` | **Stable** |
| **Test 1B — Heavy Instrument + Multi-Stage VST3** | 22 Tracks | Superior Drummer 3 + Avra Bass Guitar Blueprint `.vst3` | `192.0 kHz` / 24-bit | `2048 samples` | **Stable** |
| **Test 1C — Heavy Instrument + Multi-Stage VST3** | 22 Tracks | Superior Drummer 3 + Avra Bass Guitar Blueprint `.vst3` | `44.1 kHz` / 24-bit | `1024 samples` | **Stable** |
| **Test 2A — High-Rate Audio Mix Session** | 22 Audio Tracks | 24-bit Audio Stem + Avra Bass Guitar Blueprint `.vst3` (6 stages / track = 132 active DSP stages) | `96.0 kHz` / 24-bit | `512 samples` | **Stable** |
| **Test 2B — Multi-Track Audio Mix Session** | 22 Audio Tracks | 24-bit Audio Stem + Avra Bass Guitar Blueprint `.vst3` (6 stages / track = 132 active DSP stages) | `44.1 kHz` / 24-bit | `256 samples` | **Stable** |

---

## Support the Project

If you find **Avra Studio** useful and would like to support ongoing development:

* **Buy Me a Coffee:** [buymeacoffee.com/Kennedarth](https://buymeacoffee.com/Kennedarth)

---

## License & Non-Commercial Terms

Distributed under the **[Avra Studio Non-Commercial Open-Source & Plugin Output License](./LICENSE)**:

* **Free Personal & Community Use:** You may download, modify, compile, and use **Avra Studio** and the audio plugins (`.vst3`, `.clap`, `.exe`) built with it for personal, educational, and non-commercial production use, and distribute your generated plugins free of charge.
* **No Commercial Sale of Avra Studio:** You may not sell, rent, re-license for a fee, or bundle **Avra Studio** (or derivatives of its source code, DSP library, or UI assets) into paid products or subscriptions.
* **No Commercial Sale of Generated Plugins:** You may not sell, monetize, or charge fees for any audio plugin, executable, preset pack, or C++ source code exported by **Avra Studio**.
* **Attribution & Integrity:** Removing or bypassing license notices or attribution metadata in **Avra Studio** or its exported plugins is prohibited.
