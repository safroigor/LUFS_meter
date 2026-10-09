# Changelog

All notable changes to this project are documented in this file.

## [1.1.0] - 2026-10-09

### Added
- Segmented meter bar (uniform rounded segments) with configurable colour
  zones: green low/high, red threshold, and default/green/red colours.
- Zone/colour settings in the settings popup, with a live colour picker.
- Reset control moved into the main plugin interface with a tooltip and a
  clear, accessible hit area.
- `TransportTracker`: a pure, unit-tested measurement-pass lifecycle driven by
  transport state and playback position.
- Tests for silence continuity, multiple takes, transport lifecycle (start,
  stop, seek, loop, gap, jitter), colour-zone boundaries, and state
  persistence.
- ADR 0007 documenting the transport-based lifecycle.

### Changed
- Silence no longer resets the measurement (fixes measurement restarting during
  silent passages). A new pass now starts only on a transport/position event
  (play after stop, seek, loop wrap, or a processing gap).
- The measurement-mode label is now `LUFS-I`; the unit remains `LUFS` (the
  duplicated label is gone).
- Settings, buttons, checkbox, dropdown and slider styling unified into the dark
  visual system; the meter is wider and visually more substantial.
- `MeterSettings` gained zone/colour fields (persisted in the plugin state;
  legacy states load with sensible defaults).

### UI polish (layout, typography, icons)
- Vertical meter is substantially wider (adaptive, ~2.3x) with the `LUFS-I`
  label moved below the reading; the scale no longer collides with the value.
- Horizontal layout drops the trailing `LUFS` unit; Reset and Settings moved to
  a thin upper-right control strip so they never overlap the scale or reading.
- The numeric reading uses the same zone classification (and colour) as the
  segmented bar, with a shared, rounded, flicker-stable boundary rule.
- New vector Reset/Settings icons (`IconButton`) with hover/pressed/focus
  states; no more text glyphs.
- New `MeterTheme` + `MeterLookAndFeel`: consistent typography (Segoe UI,
  semibold focal value) and styling for buttons, checkboxes, dropdowns,
  sliders and colour swatches.
- Settings checkboxes replaced with compact themed `ToggleButton`s using the
  configured accent colour.

### Layout & HiDPI pass
- Horizontal: `LUFS-I` label, segmented meter and numeric reading now share one
  visual vertical centreline (text centred via its drawing rect, not baselines);
  the scale ruler is an independent group below the meter.
- Horizontal: the meter is wider and the gap before the reading is smaller — the
  reading's width is measured from its own text and the meter takes the rest.
- Vertical: the meter is centred in the content area with the ruler drawn
  independently to its right; the reading and `LUFS-I` label share the meter's
  centreline and sit as a compact group close beneath it.
- Shared layout constants and a small `rowAtCentreY`/text-measure helper replace
  duplicated magic numbers; minimum sizes are enforced per orientation.
- Settings typography increased ~2 logical points across title, headings,
  labels, checkboxes, sliders, dropdowns, swatches and buttons, with row heights,
  control sizes, padding and panel dimensions enlarged to match (panel is now
  sized from the layout itself).
- JUCE logical-coordinate/DPI scaling is left to JUCE (no manual pixel scaling).

### Zone-colour consistency & scale typography
- Fixed the numeric reading sometimes disagreeing with the meter at a zone
  boundary: the reading is now classified from the **same canonical 0.1 LU
  rounded value** that is displayed, using the shared
  `MeterSettings::zoneForValue`, so reading and meter can never disagree.
- `zoneForValue` gained a tiny epsilon so values sitting exactly on a boundary
  classify deterministically (float-noise tolerant).
- Zone/threshold/colour changes already flow to the meter immediately; confirmed
  no cached colour is used.
- Scale-label font increased ~2 logical points (horizontal 11→13, vertical
  11→13 for compact; 10→12 slim), with adaptive tick spacing so larger labels
  never overlap; ruler reserved area and label boxes enlarged to match.
- Added `testZoneBoundaryConsistency` covering the reported case
  (`greenHigh -14.0`, `redThreshold -13.9`, reading `-13.9`) and the gap.

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
