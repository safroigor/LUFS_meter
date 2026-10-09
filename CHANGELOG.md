# Changelog

All notable changes to this project are documented in this file.

## [1.0.0] - 2026-10-09

First release.

### Added
- VST3 (Windows x64) Integrated Loudness meter (LUFS), pass-through (never
  modifies audio).
- Accurate measurement via libebur128 v1.2.6 in exact (non-histogram) mode.
- Dedicated measurement thread with a lock-free SPSC ring buffer; the audio
  callback performs no allocation, locking or I/O.
- Compact dark UI with horizontal and vertical orientation, Slim/Compact sizes,
  optional label and scale, and a Reset button.
- Session behaviour: the value freezes on transport stop/pause or a break in the
  audio, and a fresh measurement starts when audio resumes.
- Unit test suite (CTest) covering reference levels, silence, gating, sample
  rates, reset, block-size independence and the threaded pipeline.

### Verified
- 27 automated checks pass.
- Loads and runs in REAPER x64; reading cross-checked against an independent
  meter (MeldaProduction MLoudnessAnalyzer).

### Known limitations
- EBU Loudness Test Set v5.0 not run (download blocked by Cloudflare).
- No True Peak, Momentary/Short-term LUFS, or Loudness Range.
- Channel layouts beyond mono/stereo not verified.
