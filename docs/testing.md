# Testing

This document records the test matrix, the reference values, the accepted
tolerances and the observed results.

> **Status: v1.0.0 (release).** Build, measurement engine, unit tests and the
> meter UI are implemented and verified in REAPER by the user. Automated tests
> cover measurement only.

## Tolerance policy

Tolerances are taken from the source that defines them. A universal tolerance is
**not** assumed.

- EBU Tech 3341 minimum-requirement signals: Integrated/Momentary/Short-term
  `±0.1 LUFS` where the document specifies it.
- Expectations synthesized by this project (mono, sample-rate sweep) use a
  documented tolerance, and the source is recorded per test. No single universal
  tolerance is assumed.

## Executed tests (Stage 2)

Executable: `build/release/tests/LufsMeterTests_artefacts/Release/LufsMeterTests.exe`
Run via CTest. Result: **27 checks, 0 failures**.

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

## Host verification (performed by the user)

- The plugin loads and runs in **REAPER x64**; the reading updates during
  playback and freezes on interruption.
- The reading was cross-checked against an **independent meter**
  (MeldaProduction MLoudnessAnalyzer) on identical material and matched.
- The UI was verified in both orientations: layout, settings popup,
  Slim/Compact, Show label/scale, Reset, and resize.

## Known limitations / not verified

- **EBU Loudness Test Set v5.0** could not be downloaded automatically: the EBU
  site returns a Cloudflare "Attention Required" block. The ±0.1 LU reference is
  therefore covered by the synthesized EBU Tech 3341 case-1 test (see above)
  rather than the official files.
- Long-duration accumulation stability (multiple hours) was not measured.
- Channel layouts beyond mono and stereo (e.g. 5.1) were not verified.
- The session freeze/restart thresholds (`-100 dBFS`, `500 ms`, `1000 ms`) are
  compile-time constants; the practical behaviour was confirmed by ear/host use
  but not exhaustively quantified.
