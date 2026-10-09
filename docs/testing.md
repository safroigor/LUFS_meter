# Testing

This document records the test matrix, the reference values, the accepted
tolerances and the observed results.

> **Status: v1.2.0.** Build and the automated test suite pass. The transport
> lifecycle, colour-zone model, Momentary/Short-Term/True Peak measurements and
> the coherent snapshot are covered by automated tests. The secondary readouts
> were rendered offscreen in both orientations (see below) but have **not** been
> verified in REAPER, and real-monitor DPI scaling remains unverified.

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
Run via CTest. Result: **259 checks, 0 failures**.

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
| 18 | `testSettingsPersistence` | XML round-trip incl. colours; legacy 4-attribute state | restored exactly; legacy defaults safely | exact | pass |
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

### v1.2.0 secondary readouts

The secondary readouts were rendered offscreen through JUCE (painting
`MeterComponent` into an image, no host involved) with the `LufsMeterPreview`
tool (`tools/preview`, built only with `-DLUFSMETER_BUILD_PREVIEW=ON`):

| Preview | File |
|---|---|
| Horizontal compact | `docs/images/ui-v1.2-horizontal-compact.png` |
| Horizontal slim | `docs/images/ui-v1.2-horizontal-slim.png` |
| Horizontal minimum (360 x 136) | `docs/images/ui-v1.2-horizontal-min.png` |
| Horizontal, no signal (placeholders) | `docs/images/ui-v1.2-horizontal-empty.png` |
| Horizontal, True Peak in the red state | `docs/images/ui-v1.2-horizontal-tp-red.png` |
| Vertical compact | `docs/images/ui-v1.2-vertical-compact.png` |
| Vertical slim | `docs/images/ui-v1.2-vertical-slim.png` |
| Vertical minimum (190 x 300) | `docs/images/ui-v1.2-vertical-min.png` |
| Vertical, no signal (placeholders) | `docs/images/ui-v1.2-vertical-empty.png` |
| Vertical, True Peak in the red state | `docs/images/ui-v1.2-vertical-tp-red.png` |

Rendered checks: the horizontal row reads TP, LS, LM in order above the scale,
centred on the segmented scale's own horizontal bounds, with numeric values
larger and heavier than their abbreviations and no clipping at the minimum
360 x 136; the LUFS-I reading is aligned to the meter's visual centre (measured
within ~1 px); the vertical column shows three two-line groups (label directly
above a larger value, widely spaced between groups) to the left of the centred
meter with the ruler still on the right, with no clipping or overlap at the
minimum 190 x 300; unavailable measurements show the em-dash placeholder `—`;
True Peak is red at `0.0 dBTP`.

> These renders are at the code's logical sizes and verify geometry, ordering,
> colour and non-clipping, **not** host behaviour or real-monitor DPI crispness.

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
  and the relocated Reset button in both orientations and both size presets.
- The v1.2.0 secondary readouts (TP / LS / LM): legibility at the minimum sizes,
  no overlap with the meter/ruler/reading/controls, True Peak shown in red at
  ≥ 0.0 dBTP, and DPI scaling. Offscreen renders pass at the logical sizes; a
  real host and real-monitor check is still required.
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
