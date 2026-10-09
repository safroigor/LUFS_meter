# Testing

This document records the test matrix, the reference values, the accepted
tolerances and the observed results.

> **Stage 1 status:** only the buildable plugin skeleton exists. No measurement
> tests have been run yet, and no build has been verified in a host. This file
> will be populated as tests are added in Stage 2 and validated in Stage 4.

## Tolerance policy

Tolerances are taken from the source that defines them. A universal tolerance is
**not** assumed.

- EBU Tech 3341 minimum-requirement signals: Integrated/Momentary/Short-term
  `±0.1 LUFS` where the document specifies it.
- Values synthesised by this project (not from a standard) use a tolerance of
  `±0.05 LU`, chosen to be tighter than the standard's meter tolerance; the
  source of the expected value is recorded per test.

## Planned test matrix

| # | Signal | Sample rate | Channels | Expected | Tolerance | Status |
|---|---|---|---|---|---|---|
| 1 | 1 kHz stereo sine, −23 dBFS in phase, 20 s | 48 kHz | stereo | I = −23.0 LUFS | ±0.1 (Tech 3341) | pending |
| 2 | −33 dBFS stereo sine | 48 kHz | stereo | I = −33.0 LUFS | ±0.1 | pending |
| 3 | Digital silence | 48 kHz | stereo | no valid value | — | pending |
| 4 | Very low level (−60 dBFS) | 48 kHz | stereo | gated / no value | per standard | pending |
| 5 | Absolute gate boundary (−70 LUFS) | 48 kHz | stereo | gated correctly | per standard | pending |
| 6 | Relative gate behaviour (two-level material) | 48 kHz | stereo | per BS.1770 | ±0.1 | pending |
| 7 | 1 kHz sine | 44.1 kHz | stereo | see #1 | ±0.1 | pending |
| 8 | 1 kHz sine | 96 kHz | stereo | see #1 | ±0.1 | pending |
| 9 | 1 kHz sine | 192 kHz | stereo | see #1 | ±0.1 | pending |
| 10 | 1 kHz sine | 48 kHz | mono | see #1 | ±0.1 | pending |
| 11 | Reset mid-stream starts a new measurement | 48 kHz | stereo | new value independent | exact | pending |
| 12 | Long accumulation (>= 20 min) stability | 48 kHz | stereo | stable | ±0.05 | pending |
| 13 | EBU Loudness Test Set v5.0 signals | 48 kHz | mono/stereo | per Tech 3341 | ±0.1 | pending |
| 14 | Cross-check vs independent meter (Melda MLoudnessAnalyzer) | 48 kHz | stereo | matches | ±0.1 | pending |

## Test infrastructure

- Tests are built with CTest.
- The measurement core is testable without a DAW.

## Build verification (Stage 1)

| Check | Result | Evidence |
|---|---|---|
| CMake configure (Release, Ninja, MSVC) | passed | `cmake --preset release` |
| Compilation of `ebur128.c` (C) | passed | object built, `ebur128.lib` linked |
| VST3 bundle produced | passed | `build/release/LUFSMeter_artefacts/Release/VST3/LUFS Meter.vst3` |
| VST3 entry points exported | passed | `dumpbin /exports` shows `GetPluginFactory`, `InitDll`, `ExitDll` |
| Plugin copied to host scan path | passed | `%LOCALAPPDATA%\Programs\Common\VST3\LUFS Meter.vst3` |
| Load in REAPER | **not yet verified** | requires a host scan; not automated |
| Measurement tests | **not run** | measurement engine not implemented yet |

Toolchain actually used: CMake 4.4.4, Ninja 1.13.2, MSVC 19.44.35229
(v143 toolset 14.44.35207), Windows SDK 10.0.26100.0, JUCE 8.0.15,
libebur128 v1.2.6.

## Not yet verified

- Plugin load in REAPER.
- EBU test set comparison.
- Any loudness measurement (Stage 2).
