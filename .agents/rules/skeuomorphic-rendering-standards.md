---
trigger: always_on
description: "Direct2D Skeuomorphic Rendering Engine standards matching the Kennedarth TEAL CONSOLE hardware benchmark"
---

# DIRECT2D SKEUOMORPHIC RENDERING ENGINE STANDARDS ("KENNEDARTH TEAL CONSOLE" BENCHMARK)

## 1. CHASSIS METAL SHADING & BROKEN-LINE ETCHED SECTION FRAMES
- **Ban Visible Grid Lines on the Faceplate:** Never draw a square graph-paper grid across the metal faceplate. Only show subtle snap dots while actively dragging a widget in the Faceplate Builder; the metal surface itself MUST remain 100% grid-free.
- **High-Density Anodized Brushing + Vignette:**
  - Render fine directional micro-hairline brushing (1px pitch, subtle `±3%` luminance variation) with a radial/edge dark vignette around the outer chassis perimeter and a `2px` 45° specular bevel along the top/left chassis edges.
- **"Broken-Line" Section Frames & Recessed Grooves:**
  - Never use solid filled rectangular header boxes for `SectionFrame`. Render **recessed 2px etched border grooves** (`1px` dark shadow `#0A1214` + `1px` bottom/right specular highlight `#385A62`) that leave a clean horizontal gap at top-center for the section title (`PREAMP · EQ`, `S1 · JFET BUFFER`) silkscreened directly onto the metal in bold uppercase.
  - Separate sub-sections using thin etched horizontal divider lines with centered warm gold/amber sub-headers (`#D98A3C`).

## 2. MULTI-LAYERED 3D KNOB RENDERING (BAN THIN BLUE POINTER STICKS)
Never draw a knob as a simple circle with a `DrawLine()` blue stick. Every rotary knob in Direct2D MUST render **6 physical depth passes**:
1. **Ambient + Cast Drop Shadow:** Soft multi-stop radial shadow offset `(+3px, +5px)` at `45%` opacity grounding the knob onto the faceplate.
2. **Machined Outer Skirt / Knurling:** Metallic ring (`#C8CED4` to `#687078` specular gradient) with **72 fine radial knurl ticks** around the circumference.
3. **Recessed Inner Dish:** Inset circular well with an inverted top-left shadow bevel (`#181C20` to `#2C3238`).
4. **3D Sculpted Pointer Cap (Marconi Wing / Tapered Tear-Drop / Bakelite):**
   - Physical 3D raised cap geometry (tapered tear-drop Marconi wing or sculpted cylindrical cap, `#22262A` matte body with a top-left specular edge highlight and its own cast shadow onto the silver skirt).
   - **High-Contrast Inlay Line:** Crisp, recessed off-white (`#F0F4F8`) or theme-matched cream/amber inlay bar integrated *into* the 3D cap nose—NEVER a neon blue line floating on top of a colored circle.
5. **Radial Scale Ticks & Silkscreen Numbers:**
   - Radial tick marks from `225°` (7 o'clock) to `495°` (5 o'clock), with major ticks (`6px` long, `1.5px` wide) and minor sub-ticks (`3px` long).
   - Color `OFF` / `-∞` / minimum or orange-flagged tick labels in warm amber (`#E07A28`) and standard scale numbers in crisp silkscreen off-white (`#DCE2E6`).

## 3. AUTHENTIC 3D CHROME BAT-HANDLE SWITCHES & JEWEL LEDS
- **Ban Web Pill Sliders & Giant Colored Rectangles:** Never render flat iOS-style pill toggles or giant colored rectangle buttons on hardware faceplates.
- **Machined Bat Switch + Jewel Lamp Assembly:**
  1. **Hex Mounting Nut & Washer:** Brushed steel outer washer ring and 6-sided hexagonal mounting nut (`18px` diameter) with metallic bevel shading.
  2. **Dark Bushing Well:** `7px` dark recessed center hole.
  3. **3D Tapered Chrome Bat Lever:** Conical/rounded metallic lever (`#E8EEF2` highlight to `#505860` shadow) that physically tilts UP (`IN` / `FAST`), CENTER, or DOWN (`OUT` / `SLOW`), casting a drop shadow onto the hex nut.
  4. **Paired 3D Recessed Jewel LED:** `7px` circular chrome bezel next to the switch containing a glass-shaded LED dome (radial glow with white specular hot-spot in warm amber `#FFA033` when `ON`, or dark unlit amber-brown `#2A1C10` when `OFF`).

## 4. MACHINED ALUMINUM NAMEPLATE BADGE
- Render `RivetedNameplate` as a **Machined Brushed-Aluminum Nameplate Plaque**:
  - Thick silver/pewter metallic gradient fill (`#D8DEE4` to `#9BA4AE`) with horizontal micro-brushing.
  - `2px` 3D bevelled outer edge (`#FFFFFF` top/left highlight, `#20282C` bottom/right shadow) + soft chassis drop shadow.
  - **4 Realistic Corner Rivets / Screws** with recessed slot shadows.
  - Crisp dark-engraved uppercase typography (`AVRA AUDIO`, large bold model title with an orange `+` accent, horizontal etched divider rule, and technical subtitle).

## 5. RECESSED BACKLIT ANALOG VU METER & LED BARGRAPH
- **Recessed Backlit Analog VU Meter:**
  - Deep inset rounded-rectangle bezel (`4px` dark inner shadow simulating glass depth).
  - **Warm Backlit Face:** Dark charcoal-brown surface (`#1A1816`) illuminated by a warm amber radial glow (`#D87A18` at `35%` opacity) from the bottom center.
  - **VU Scale Arc:** Curved white/amber scale arc with sub-ticks and a razor-sharp anti-aliased orange/red needle (`#F06418`) with a subtle shadow.
- **Compact Recessed `IN / OUT / OUT-IN` LED Matrix:** Dark recessed bay housing crisp `2px x 8px` unlit/lit LED segments (green `-24..-3 dB`, amber `0 dB`, red `+3 dB`).

## 6. SILKSCREEN TYPOGRAPHY & SMART UNIT FORMATTING
- Render knob titles directly onto the metal chassis in **100% UPPERCASE**, bold geometric sans-serif (`11px–12px`, `#E6ECEF` with `1px` dark drop shadow `#00000088`), with no bounding boxes.
- **Auto-Scale `Hz` to `kHz` and `ms` to `s`:** Format frequencies $\ge 1000\text{ Hz}$ automatically as `24.0 kHz` or `3.8 kHz`, and times $\ge 1000\text{ ms}$ as `1.5 s`. Render the live value readout in warm amber/gold (`#D88B38`, `10.5px`) directly beneath the uppercase white title, with the unit on a clean sub-line.

## 7. HIDE CYAN SELECTION GIZMO OUTSIDE ACTIVE INTERACTION
- Only render the cyan selection gizmo in `+ FACEPLATE UI BUILDER` when actively clicking/dragging a widget, and auto-fade it after 2 seconds of inactivity so the hardware panel can be viewed unobstructed.

## 8. "KENNEDARTH TEAL CONSOLE" MASTER THEME PRESET
- Expose `KENNEDARTH TEAL CONSOLE` at the top of `UNIFIED UI STYLE TEMPLATES`: Deep Petrol British Teal (`#1A4347` to `#0F2A2D`) with vertical micro-brushing, dark vignette, recessed etched section frames, Brushed Silver Fluted Skirts with 3D Black Marconi Tear-Drop Caps, 3D Chrome Bat-Handles + warm amber jewel LEDs, warm amber backlit VU meter, and riveted silver brushed-aluminum nameplate.
