# Testing

This document records the test matrix, the reference values, the accepted
tolerances and the observed results.

> **Status: v1.1.0.** Build and the automated test suite pass. The transport
> lifecycle and colour-zone model are covered by automated tests; the redesigned
> UI is covered by offscreen renders. Host-specific transport behaviour and
> real-monitor DPI scaling have **not** been verified in REAPER for this build.

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
Run via CTest. Result: **153 checks, 0 failures**.

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
| 13 | `testEnginePipeline` | MeasurementEngine: 5 s −23 dBFS pushed in 512-frame blocks | −23.0 LUFS | ±0.1 | pass |
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

## Toolchain used for these results

CMake 4.4.4, Ninja 1.13.2, MSVC 19.44.35229 (toolset 14.44.35207),
Windows SDK 10.0.26100.0, JUCE 8.0.15, libebur128 v1.2.6.

## UI previews (offscreen)

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

> **Not re-verified after the redesign.** The following are covered only by
> automated tests (see above) and must be re-checked in a host before release:

- Transport-driven pass lifecycle: play/stop/seek/loop with a real REAPER
  transport, including the pause-vs-stop caveat (ADR 0007).
- The redesigned UI: segmented bar, zone/colour settings, the colour picker,
  and the relocated Reset button in both orientations and both size presets.

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
