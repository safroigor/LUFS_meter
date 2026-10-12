# Testing

This document records the test matrix, the reference values, the accepted
tolerances and the observed results.

> **Status: v1.8.0 (visible pass reset, dimmer chrome).** Build and the
> automated test suite pass (416 checks, Release and Debug). On top of v1.7.0 a
> new measurement pass visibly starts the meter over — the reading blanks and the
> bar empties, then fills up to the new value — and the icon buttons are dimmed
> further (24 % idle). Carried over: True Peak turns red at a configurable
> threshold (**−1.0 dBTP** by default, was a hard-coded 0.0); the display scale
> spans **−50 … 0 LUFS** with a mark every 10 LUFS (typical −14 … −16 material
> now sits in the upper third); the Horizontal ruler prints signed labels;
> Reset/Settings/About are redrawn in one shared glyph box so they read at 20 px;
> the popups dismiss on a click anywhere outside them; and the indicator **fills
> up** to the first value of a pass over ~0.55 s instead of jumping to it. The
> transport lifecycle, colour-zone model,
> Momentary/Short-Term/True Peak measurements, the coherent snapshot, settings
> persistence / legacy migration, the geometry and size memory and the fill
> animation are covered by automated tests. The layouts were rendered offscreen
> at their preferred and minimum sizes plus one frame of the fill sweep (see
> below), but the plug-in has **not** been re-verified in REAPER after the
> redesign, and real-monitor DPI scaling remains unverified.

## Tolerance policy

Tolerances are taken from the source that defines them. A universal tolerance is
**not** assumed.

- EBU Tech 3341 minimum-requirement signals: Integrated/Momentary/Short-term
  `±0.1 LUFS` where the document specifies it.
- Expectations synthesized by this project (mono, sample-rate sweep) use a
  documented tolerance, and the source is recorded per test. No single universal
  tolerance is assumed.

## Executed tests

Executable: `build/release/tests/LufsMeterTests_artefacts/Release/LufsMeterTests.exe`
Run via CTest. Result: **416 checks, 0 failures** (Release and Debug).

| # | Test | Signal / action | Expected | Tolerance | Result |
|---|---|---|---|---|---|
| 1 | `testStereoReference` | stereo 1 kHz sine, −23 dBFS peak, in phase, 20 s @ 48 kHz | −23.0 LUFS | ±0.1 (EBU Tech 3341 case 1) | pass |
| 2 | `testLevelScaling` | stereo sine at −33 / −18 dBFS | −33.0 / −18.0 LUFS | ±0.1 | pass |
| 3 | `testSilence` | digital silence, 2 s | no value (not 0 LUFS) | — | pass |
| 4 | `testLowLevelAboveGate` | stereo sine −60 dBFS | −60.0 LUFS | ±0.1 | pass |
| 5 | `testBelowAbsoluteGate` | stereo sine −75 dBFS (below −70 gate) | no value | — | pass |
| 6 | `testMonoReference` | mono sine, −23 dBFS peak | −26.0 LUFS | ±0.15 (half energy of stereo) | pass |
| 7 | `testSampleRates` | stereo −23 dBFS @ 44.1 / 88.2 / 96 / 192 kHz | −23.0 LUFS | ±0.1 | pass |
| 8 | `testBlockSizeIndependence` | same signal fed in 1 / 512 / 65536-frame chunks | −23.0 LUFS | ±0.1 | pass |
| 9 | `testNoValueBeforeOneBlock` | 200 ms of audio (< one 400 ms block) | no value | — | pass |
| 10 | `testReset` | measure −23, reset, measure −33 | reset discards; then −33.0 | ±0.1 | pass |
| 11 | `testRelativeGating` | 10 s −23 LUFS + 10 s −40 LUFS | −23.0 LUFS (quiet part below −10 LU gate) | ±0.1 | pass |
| 12 | `testSilenceThenSignal` | 5 s silence + 20 s −23 dBFS | −23.0 LUFS | ±0.1 | pass |
| 13 | `testEnginePipeline` | MeasurementEngine: 5 s −23 dBFS pushed in 512-frame blocks | I/M/S ≈ −23.0 LUFS, TP ≈ −23.0 dBTP; reset clears all | ±0.1 / ±0.5 | pass |
| 14 | `testSilenceInMiddleKeepsValue` | 20 s −23 + 15 s silence + 20 s −23 | −23.0 LUFS (silence does not reset) | ±0.1 | pass |
| 15 | `testMultipleTakesSeparatedBySilence` | 4 × 10 s −23 takes, 8 s silent gaps | −23.0 LUFS | ±0.1 | pass |
| 16 | `testZoneClassification` | zone boundaries incl. shared green/red edge | normal / green / red as configured | exact | pass |
| 17 | `testZoneSanitisation` | reversed and out-of-scale boundaries | ordered (normal < green < red) and clamped | exact | pass |
| 18 | `testSettingsPersistence` | XML round-trip incl. colours and the True Peak warning threshold; repeated save/load; every display mode; out-of-range mode; a state saved before the threshold existed; the threshold set to +5 / −100 dBTP | restored exactly; mode never changes on reload; the threshold defaults to −1.0 and clamps to −20 … 0 dBTP; clamped | exact | pass |
| 19 | `testLifecycleContinuousPlayback` | advancing position with jitter | no resets after the first block | exact | pass |
| 20 | `testLifecycleSilenceDoesNotReset` | 50 continuous blocks (tracker has no audio input) | no resets | exact | pass |
| 21 | `testLifecycleSeekResets` | position jumps forward while playing | new pass | exact | pass |
| 22 | `testLifecycleLoopWrapResets` | position wraps backwards to loop start | new pass | exact | pass |
| 23 | `testLifecycleStopAndRestart` | stop, then play from the same position; repeated cycles | new pass each time | exact | pass |
| 24 | `testLifecycleGapResets` | >1 s between callbacks while "playing" | new pass | exact | pass |
| 25 | `testLifecycleWithoutPositionStillPlays` | transport playing, no position reported | continuous, no automatic resets | exact | pass |
| 26 | `testZoneBoundaryConsistency` | `greenHigh -14.0`, `redThreshold -13.9`; probes on/around the boundary | `-13.9 → red`, `-14.0 → green`, gap → normal, float-noise → red | exact | pass |
| 27 | `testMomentaryReference` | stereo 1 kHz sine −23 dBFS, 5 s | M ≈ −23.0 LUFS | ±0.1 | pass |
| 28 | `testMomentaryUnavailableBeforeWindow` | 200 ms of audio (< 400 ms) | no Momentary value | — | pass |
| 29 | `testMomentarySilentWindow` | 10 s −23 tone then 0.5 s silence | M unavailable; I ≈ −23.0 keeps its value | ±0.15 | pass |
| 30 | `testShortTermReference` | stereo 1 kHz sine −23 dBFS, 5 s | S ≈ −23.0 LUFS | ±0.1 | pass |
| 31 | `testShortTermUnavailableBeforeWindow` | 2 s then +1.5 s | no S before 3 s; S ≈ −23.0 after | ±0.1 | pass |
| 32 | `testShortTermRollingTransition` | 5 s −23 then 5 s −33 | S rolls to −33.0 | ±0.1 | pass |
| 33 | `testShortTermSilenceAfterActive` | 5 s tone then 1 s silence, then 2.5 s more | S stays valid while the window holds audio; unavailable when silent | — | pass |
| 34 | `testShortTermLowLevelUngated` | stereo sine −75 dBFS, 5 s | S ≈ −75.0 (ungated); I has no value | ±0.1 | pass |
| 35 | `testMomentaryShortTermSampleRates` | −23 dBFS @ 44.1 / 88.2 / 96 / 192 kHz | M and S ≈ −23.0 | ±0.1 | pass |
| 36 | `testMomentaryShortTermBlockSize` | 1 / 512 / 65536-frame chunks | M and S ≈ −23.0 | ±0.1 | pass |
| 37 | `testTruePeakInterSample` | fs/4 sine, 45° phase (sample peak −3.01 dBFS) | dBTP ≈ 0.0, clearly above sample peak | ±0.5 | pass |
| 38 | `testTruePeakAcrossChannels` | left silent, right fs/4 tone at −6.02 dBFS | max across channels ≈ −6.02 dBTP | ±0.5 | pass |
| 39 | `testTruePeakSilenceAndReset` | silence, then a −6 dBFS tone, then reset | unavailable on silence; reset clears the maximum | — | pass |
| 40 | `testSnapshotConcurrentRead` | reader thread while audio is pushed | snapshot always coherent/finite (no torn values) | — | pass |
| 41 | `testMomentaryShortTermMono` | mono 1 kHz sine −23 dBFS, 5 s | M and S ≈ −26.0 (single-channel energy) | ±0.15 | pass |
| 42 | `testTruePeakSampleRates` | fs/4 45° sine @ 44.1 / 96 / 192 kHz | dBTP ≈ 0.0 (4×/2×); ≈ −3.01 at 192 kHz (no oversampling) | ±0.5 / ±0.1 | pass |
| 43 | `testLegacyDisplayModeMigration` | legacy orientation/size pairs, absent orientation, `displayMode` precedence, legacy attrs written for Compact | deterministic mode mapping; old builds see Horizontal for Compact | exact | pass |
| 44 | `testScaleProportion` | `scaleProportion` at min/max, at the quarter/half/three-quarter points of the range (computed from the constants), and outside the scale | 0 / 1 / linear; −16 and −14 land above 0.66 of the range; clamped to [0, 1] | exact | pass |
| 45 | `testEditorLayoutTable` | preferred/minimum of all three modes; overlay rule; `controlsIn` placement in every mode; reserved side room (`rowInsetFor`); Horizontal minimum width vs the readout row and the overlaid buttons | preferred ≥ minimum; Vertical narrower than Horizontal and Horizontal shorter than Vertical; only Horizontal overlays; About top-left and Settings top-right, Reset bottom-left (Vertical/Horizontal) or at half height (Compact) in different rows from the top buttons; Vertical reserves no side room, Horizontal/Compact reserve more than one button; ≥ 290 px left for the TP / LS / LM row (it measures ~260 px) | exact | pass |
| 46 | `testEditorWindowSizes` | per-mode sizes through XML; repeated save/load; unset modes; a state with no `<windowSizes>`; clamping below/above the mode's range; zero/negative sizes; reset; a damaged node (missing attributes, −40); out-of-range mode indices | each mode restored exactly and independently; repeated round trips stable; unset stays unset and writes no node; clamped to the mode's limits and to `maxWindowSize`; a degenerate size never erases a memory; unknown modes are unset | exact | pass |
| 47 | `testValueAnimation` | a fill with 10 even steps; the same duration in one step vs 50 steps; a target that moves mid-fill; `clear` (no value) after a reset; `advance` while cleared; the fill that follows a clear | starts at its starting point, monotonic, reaches the target exactly at the end of the duration, result independent of the step breakdown (±1e−9), eased towards a moving target and landed on it exactly, cleared stays cleared, and the next fill starts from the bottom of the scale rather than from where it was | exact | pass |

### Notes

- Tolerances for the −23 dBFS reference follow EBU Tech 3341 (±0.1 LU).
  Expectations synthesized by this project (mono, sample-rate sweep) use the
  same or a documented tolerance; the source is stated per row.
- A first version of `testSilenceThenSignal` used a 5 s tone and read
  −23.13 LUFS. This is correct BS.1770 behaviour: gating blocks that span the
  silence→tone transition are above the absolute gate and are included. The test
  now uses a longer tone so this expected boundary contribution is negligible,
  and the ±0.1 LU tolerance is meaningful.
- The exact (non-histogram) mode is used, so no 0.1 LU quantisation applies.
- Momentary/Short-Term are ungated, so `testShortTermLowLevelUngated` reads
  −75 LUFS for a signal the Integrated gate excludes. M/S are held unavailable
  until a full window (400 ms / 3 s) has been processed, and again once the
  window is essentially silent (the documented −120 LUFS display-validity floor);
  this floor exists because the K-weighting filter's decaying tail otherwise
  leaves a tiny finite energy in an all-silent window.
- True-peak tests use a synthetic `fs/4`, 45°-phase sine whose samples are
  ±1/√2 (−3.01 dBFS) while the reconstructed peak is 1.0 (0 dBTP), which
  demonstrates the inter-sample distinction. libebur128's true-peak algorithm is
  implementation-defined, so a ±0.5 dB tolerance is used rather than claiming
  reference-meter equivalence.

## Toolchain used for these results

CMake 4.4.4, Ninja 1.13.2, MSVC 19.44.35229 (toolset 14.44.35207),
Windows SDK 10.0.26100.0, JUCE 8.0.15, libebur128 v1.2.6.

## UI previews (offscreen)

### Compact layouts, remembered window size and About (current)

Every display mode was rendered offscreen through JUCE (painting the editor's
composition — `MeterComponent` plus the three `IconButton`s at their shared
positions — into an image, no host involved) with the `LufsMeterPreview` tool
(`tools/preview`, built only with `-DLUFSMETER_BUILD_PREVIEW=ON`). The sizes come
from `EditorLayout`, so a render can never show a size the editor does not use:

| Preview | File |
|---|---|
| Vertical, preferred (176 × 360) | `docs/images/ui-mode-vertical.png` |
| Vertical, minimum (156 × 300) | `docs/images/ui-mode-vertical-min.png` |
| Vertical, no label / no scale | `docs/images/ui-mode-vertical-plain.png` |
| Vertical, one frame of the fill sweep (0.15 s) | `docs/images/ui-mode-vertical-filling.png` |
| Horizontal, preferred (430 × 120) | `docs/images/ui-mode-horizontal.png` |
| Horizontal, minimum (360 × 112) | `docs/images/ui-mode-horizontal-min.png` |
| Horizontal, no label / no scale | `docs/images/ui-mode-horizontal-plain.png` |
| Horizontal, one frame of the fill sweep (0.15 s) | `docs/images/ui-mode-horizontal-filling.png` |
| Horizontal, True Peak above its warning threshold (−0.5 dBFS tone) | `docs/images/ui-mode-horizontal-tp-red.png` |
| Compact, preferred (200 × 176) | `docs/images/ui-compact.png` |
| Compact, minimum (176 × 160) | `docs/images/ui-compact-min.png` |
| Compact, no label / no scale | `docs/images/ui-compact-plain.png` |
| Compact, default zone / low fill (−27 LUFS) | `docs/images/ui-compact-normal.png` |
| Compact, green zone (−18 LUFS) | `docs/images/ui-compact-green.png` |
| Compact, red zone (−8 LUFS) | `docs/images/ui-compact-red.png` |
| Vertical / Horizontal / Compact, no signal | `...-empty.png` |
| Settings panel (with Reset window size) | `docs/images/ui-settings.png` |
| About popup | `docs/images/ui-about.png` |

Rendered checks (preferred and minimum size of every mode, plus the no-label /
no-scale variants; the buttons are drawn, so a render shows what a host window
shows):

- **Control placement:** About in the top-left corner and Settings in the
  top-right one in every mode; Reset in the bottom-left corner in Vertical and
  Horizontal (clear of the `LUFS-I` label and of the `-50` ruler label) and on
  the left edge at half height in Compact, where it sits beside — not on — the
  reading capsule. All three glyphs share one glyph box and one stroke weight,
  so the circular arrow, the six-tooth gear and the About ring are all legible at
  the 20 px button size — the reset and settings glyphs previously overlapped
  into a blob roughly half this size — and all three draw in a translucent white
  (24 % idle) so they read as chrome next to the meter rather than competing
  with it.
- **True Peak red:** with the default −1.0 dBTP threshold, a −0.5 dBFS tone
  renders `TP -0.5` in red while `LS` and `LM` stay in the readout colour; below
  the threshold nothing changes.
- **Scale:** −50 … 0 LUFS with marks every 10 LU (0, −10, −20, −30, −40, −50),
  labelled from the top so the maximum is always present. The Horizontal ruler
  prints the sign (`-50 … 0`); the vertical column prints magnitudes only, and
  the Horizontal minimum width automatically shows the sparser 15 LU step
  (0, −15, −30, −45) without collisions.
- **Vertical (176 × 360, min 156 × 300):** the segmented bar (≈44 px thick, from
  58 px), the TP / LS / LM column beside it (values at 0.9 scale, ≈20 px), the
  scale labels to the right, the focal reading (46 px, the same as Horizontal)
  and the `LUFS-I` label all render without clipping at both sizes; neither the
  reading nor the readout column needs squeezing at the preferred size.
- **Horizontal (430 × 120, min 360 × 112):** there is no reserved strip, the
  meter keeps the whole area, and the TP / LS / LM row is laid out inside what
  the top corner buttons leave free. The row (≈260 px at 0.9 scale) keeps its
  clear space to About and Settings at the 360 px minimum width and much more at
  the preferred width, and it never meets the scale or the reading. Reset in the
  bottom-left corner stays clear of the leftmost ruler label. The bar (≈30 px
  thick, ≈223 px long) and its ticks fit at both sizes.
- **Compact (200 × 176, min 176 × 160):** the continuous fill, the large centred
  reading and the three label-over-value columns stay within their columns; the
  readout type scales down (≈0.94 / ≈0.76 of the base size) as the window gets
  smaller, and nothing clips.
- **Compact backdrop and capsule:** no grey panel is painted any more — the area
  is the window background, with the measurement's own fill as the only tint.
  The reading sits on a capsule sized to a full `-00.0` (≈136 px at the
  preferred size, clamped to what the left-hand Reset button leaves free), so
  the fill's moving edge is masked behind the digits instead of cutting them;
  the capsule keeps its width as the digits change, and in the no-signal state
  only the placeholder dash is visible inside it.
- **Compact fill:** its top edge still follows the Integrated value through the
  shared `MeterSettings::scaleProportion` mapping (on the −50 … 0 scale, −27/−23/
  −18/−8 map to 0.46 / 0.54 / 0.64 / 0.84), the colour follows the configured
  zones (neutral at −27, green at −23/−18, red at −8), and there is no fill when
  the value is unavailable — the readouts then show the em-dash placeholder `—`.
- **Fill sweep:** the 0.15 s frames show the meter part-way to the same −23 dBFS
  tone (reading ≈ −33.4 LUFS, bar filled to about a third of the scale) in both
  segmented modes, i.e. the value arrives by filling up from the bottom with the
  reading travelling with it, rather than by jumping. A still cannot show the
  motion itself; `testValueAnimation` covers the timing, monotonicity and the
  final value, and the host check below covers the effect in a real transport.
- **Settings panel:** the mode selector, toggles, zone sliders, colour swatches
  and both buttons (Reset, Reset window size) fit the popup without clipping.
- **About popup:** plugin metadata, the four measurements and the third-party
  attributions.

> These renders are at the code's logical sizes and verify geometry, ordering,
> colour and non-clipping, **not** host behaviour or real-monitor DPI crispness.

### v1.2.0 secondary readouts (superseded)

> The v1.2.0 `ui-v1.2-*.png` renders remain on disk for history; the readouts now
> follow the three display modes above. In v1.2.0 the horizontal row read TP, LS,
> LM above the scale and the vertical column stacked three two-line groups to the
> left of the meter, with unavailable values shown as `—` and True Peak red at
> `0.0 dBTP`.


### v1.1.0 (historical)

> These renders predate the v1.2.0 secondary readouts.

The layout, typography, icons and controls were rendered offscreen through JUCE
(painting the components into an image, no host involved) and inspected at the
compact, minimum and larger sizes:

| Preview | File |
|---|---|
| Horizontal, green zone | `docs/images/ui-horizontal.png` |
| Horizontal at the minimum width (210 x 96) | `docs/images/ui-horizontal-min.png` |
| Vertical, green zone | `docs/images/ui-vertical.png` |
| Vertical, red zone | `docs/images/ui-vertical-red.png` |
| Vertical, larger window | `docs/images/ui-vertical-large.png` |

> The `docs/images/ui-settings.png` render is regenerated for the current panel
> (see the section above) rather than kept as history.

Rendered checks: the horizontal label, meter and reading share one visual
vertical centreline while the ruler stays independently below; the vertical
meter is centred with the ruler to its right and the reading/label form a compact
group on the meter's centreline; the numeric reading uses the same zone colour as
the meter (green reading in the green zone, red reading in the red zone); the
scale labels are larger and legible with consistent tick alignment; the settings
type is larger with no clipping or crowding; no scale labels or readings clip at
the sizes above.


> Offscreen rendering verifies geometry and colours, **not** host behaviour,
> DPI-dependent crispness on real display scaling, or interaction (hover,
> pressed, focus). Those still require a running host (see below).

## Host verification (performed by the user)

- The v1.0.0 plugin loaded and ran in **REAPER x64**; the reading cross-checked
  against an independent meter (MeldaProduction MLoudnessAnalyzer).

> **Not re-verified after the redesign (v1.1.0), for v1.2.0 or for v1.4.0.** The
> following are covered only by automated tests and offscreen renders (see
> above) and must be re-checked in a host before release:

- Transport-driven pass lifecycle: play/stop/seek/loop with a real REAPER
  transport, including the pause-vs-stop caveat (ADR 0007).
- The redesigned UI: segmented bar, zone/colour settings, the colour picker,
  and the relocated Reset button in all three display modes (Vertical,
  Horizontal, Compact).
- The three display modes and the Compact continuous fill: legibility of the
  reading and the TP / LS / LM readouts over the fill at the minimum and
  preferred sizes, correct fill height against the scale and zone colours, the
  Info-left / Reset-Settings-right control row, the About/info popup, and DPI
  scaling. Offscreen renders pass at the logical sizes; a real host and
  real-monitor check is still required.
- The v1.2.0 secondary readouts (TP / LS / LM): legibility at the minimum sizes,
  no overlap with the meter/ruler/reading/controls, True Peak shown in red at
  ≥ 0.0 dBTP, and DPI scaling.
- **Remembered window size (v1.4.0):** drag the window in each display mode,
  save the project, then close and reopen the plug-in window and reload the
  project — each mode must reopen at the size it was dragged to; switching modes
  must return that mode's own size; **Reset window size** must return the
  preferred size; dragging below the minimum must stop at the minimum. Without a
  project save the size is not expected to survive a restart (the size travels
  with the plug-in state, ADR 0009).
- **Compact layouts in a host (v1.4.0):** the overlaid corner buttons must not
  meet the TP / LS / LM row or the scale at the Horizontal minimum width
  (360 × 112), the slim Vertical (176 × 360, min 156 × 300) must not clip its
  readout column or scale labels, and both must stay legible at 100 %, 125 % and
  150 % display scaling.
- **v1.5.0:** the three icons must be legible in a host at 100 % and 150 %
  scaling; the −50 … 0 scale must put the user's −14 … −16 material in the upper
  third with the green/red zones where they were configured; the Horizontal
  ruler must show signed labels without clipping at the minimum width; and the
  indicator must fill up (not jump) when playback starts after a stop, after a
  seek/loop and when the window is opened in the middle of a pass.
- **v1.6.0:** Reset must be reachable and clear of the meter in all three modes
  (bottom-left, and left-of-centre in Compact), the Compact area must read as a
  flat window background with only the fill as a tint, and the capsule must keep
  the digits readable as the fill's edge passes them while the measurement
  changes.
- **v1.7.0:** the settings and About popups must close on a click anywhere
  outside them — in the plug-in window, elsewhere in the host's window, and in
  another application — without the dismissing click hitting the meter or
  re-opening the popup; True Peak must turn red at the configured threshold and
  the setting must survive a project save/load.
- **v1.8.0:** repositioning the cursor (a click in the timeline, a drag, a loop
  jump) must visibly start the meter over — the reading blanks and the bar
  empties before it fills up to the new value — and the Reset button must clear
  the displayed value at once.
- Reference-meter comparison of Short-Term, Momentary and True Peak against a
  trusted analyzer, and true-peak comparison against suitable test signals.

## Known limitations / not verified

- **EBU Loudness Test Set v5.0** could not be downloaded automatically: the EBU
  site returns a Cloudflare "Attention Required" block. The ±0.1 LU reference is
  therefore covered by the synthesized EBU Tech 3341 case-1 test (see above)
  rather than the official files.
- Long-duration accumulation stability (multiple hours) was not measured.
- Channel layouts beyond mono and stereo (e.g. 5.1) were not verified.
- Pause and stop are not distinguishable through JUCE's play-head API; both
  start a new measurement pass (documented in ADR 0007).
- Hosts that do not report transport/logic position accumulate continuously and
  can only be cleared with the Reset button.
- At sample rates ≥ 192 kHz libebur128 performs no true-peak oversampling, so
  the reported dBTP equals the sample peak. libebur128's true-peak algorithm is
  implementation-defined; small differences against other meters are expected.
- The M/S silence floor (`-120 LUFS`) is a display-validity choice, not part of
  the standards.
