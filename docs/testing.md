# Testing

This document records the test matrix, the reference values, the accepted
tolerances and the observed results.

> **Status: v1.3.0 (three display modes + About).** Build and the automated
> test suite pass (284 checks, Release and Debug). On top of v1.2.0 this adds a
> single three-option display-mode selector (Vertical / Horizontal / Compact),
> the Compact continuous background fill and secondary readout row, and an
> About/info popup. The transport lifecycle, colour-zone model,
> Momentary/Short-Term/True Peak measurements, the coherent snapshot and the new
> settings persistence / legacy-migration are covered by automated tests. The
> three modes, the Compact meter and the About popup were rendered offscreen (see
> below) but have **not** been verified in REAPER, and real-monitor DPI scaling
> remains unverified.

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
Run via CTest. Result: **284 checks, 0 failures** (Release and Debug).

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
| 18 | `testSettingsPersistence` | XML round-trip incl. colours; repeated save/load; every display mode; out-of-range mode | restored exactly; mode never changes on reload; clamped | exact | pass |
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
| 44 | `testScaleProportion` | `scaleProportion` at min/max, −15, −24, −6, and outside the scale | 0 / 1 / linear; clamped to [0, 1] | exact | pass |

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

### Three display modes, Compact indicator and About (current)

The three display modes were rendered offscreen through JUCE (painting
`MeterComponent` into an image, no host involved) with the `LufsMeterPreview`
tool (`tools/preview`, built only with `-DLUFSMETER_BUILD_PREVIEW=ON`):

| Preview | File |
|---|---|
| Vertical mode (224 x 360) | `docs/images/ui-mode-vertical.png` |
| Horizontal mode (430 x 144) | `docs/images/ui-mode-horizontal.png` |
| Compact mode, preferred size (220 x 190) | `docs/images/ui-compact.png` |
| Compact mode, minimum size (190 x 170) | `docs/images/ui-compact-min.png` |
| Compact, default zone / low fill (−27 LUFS) | `docs/images/ui-compact-normal.png` |
| Compact, green zone (−18 LUFS) | `docs/images/ui-compact-green.png` |
| Compact, red zone (−8 LUFS) | `docs/images/ui-compact-red.png` |
| Compact, no signal (placeholders) | `docs/images/ui-compact-empty.png` |
| Vertical / Horizontal / Compact, no signal | `...-empty.png` |
| About popup | `docs/images/ui-about.png` |

Rendered checks: Vertical and Horizontal are unchanged from v1.2.0;
Compact is a nearly square meter (204 x 152 main area at the preferred size) with
a large, centred LUFS-I reading (about 65% larger than the previous Compact
strip) and the TP, LS, LM row beneath it, over one **continuous** bottom-anchored
fill — no segments, ruler, ticks or marker. The secondary row is three
label-over-value columns (label directly above its value, as in Vertical mode)
with a 24 px value type (about 35% larger than the previous inline row); the
three groups stay within their columns with clear gaps at both sizes (value
groups separated by ~29 px at the preferred size and ~19 px at the minimum). The
fill's top edge moves with the Integrated value: measured fill proportion 0.105
at −27, 0.237 at −23, 0.401 at −18 and 0.737 at −8, matching the
`scaleProportion` mapping within rounding, and the fill colour follows the
configured zones (neutral at −27, green at −23/−18, red at −8); there is no fill
when the value is unavailable; the reading and the readouts stay legible over the
fill at both the preferred and the minimum size (174 x 132 main area), and the
readouts show the em-dash placeholder `—` with no value; the About popup shows
the plugin metadata, the four measurements and the third-party attributions.

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
| Settings panel | `docs/images/ui-settings.png` |

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

> **Not re-verified after the redesign (v1.1.0) or for v1.2.0.** The following
> are covered only by automated tests (see above) and must be re-checked in a
> host before release:

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
