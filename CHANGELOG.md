# Changelog

All notable changes to this project are documented in this file.

## [1.8.0] - 2026-10-12

### Changed
- **A new pass visibly starts the meter over again.** v1.7.0 held the last
  reading through a pass with nothing measured yet, which removed the jerk but
  also hid the fact that a new measurement had begun. The indicator now drops
  back to the start of the scale again — the reading blanks and the bar empties
  — and fills up to the first value of the new pass, so repositioning the cursor
  is unmistakable. `ValueAnimation::hold` was removed with it.
- **Icons dimmer again:** the icon buttons now draw in translucent white at
  24 % (was 36 %) idle, 45 % hovered and 78 % pressed, with the hover/press discs
  and the focus ring softened to match.
- Version 1.8.0.

### Tests
- `testValueAnimation` covers the reset that has to be visible: a cleared
  animation has no value, and the fill that follows starts from the bottom of the
  scale rather than from wherever it was: **416 checks, 0 failures** (Release and
  Debug).
- Preview renders regenerated with the dimmer icons.

## [1.7.0] - 2026-10-12

### Added
- **Configurable True Peak warning threshold.** True Peak now turns red at or
  above a threshold that is set in the settings popup — **−1.0 dBTP by default**
  (was hard-coded at 0.0 dBTP) — and is persisted with the rest of the settings
  (`MeterSettings::truePeakRedThresholdDbtp`, clamped to −20 … 0 dBTP).

### Changed
- **Icon buttons are dimmer.** They are chrome for the meter, so they now draw in
  a translucent white (36 % idle, 60 % hover, 88 % pressed) instead of the
  secondary text colour, and the hover/press discs and focus ring were softened
  to match.
- **The meter holds its reading through a new pass instead of dropping it.** A
  seek (or playback restarting) starts a new measurement pass, which used to
  clear the display: the indicator fell to the bottom and then filled up again,
  and repositioning the cursor therefore read as two sharp jerks. The last
  reading is now held — the same way it already was when the transport stops —
  and the fill eases from there to the new value, so a reposition is one smooth
  move. The Reset button still clears the display immediately
  (`MeterComponent::resetDisplay`), and a meter that has never had a value still
  shows the placeholder.
- **Vertical's focal reading is the same size as Horizontal's** (46 px, was
  36 px).
- **Popups close on any click outside them.** The settings and About boxes are
  still desktop windows (so the tall settings panel is not clipped by a small
  plug-in window), but they now also dismiss on a click that lands outside every
  JUCE window — the host's own UI, or another application — which JUCE's modal
  handling cannot see; and the click that dismisses them is consumed, so it
  cannot also hit the meter or immediately re-open the popup through its own
  button.
- Version 1.7.0.

### Tests
- Extended `testValueAnimation` for the hold: a pass with no value keeps the
  indicator where it is, a hold does not drift over time, and the next fill
  starts from the held value and lands on the new one: **420 checks, 0 failures**
  (Release and Debug).
- Extended `testSettingsPersistence` for the True Peak threshold (round trip,
  default −1.0, clamping to −20 … 0 dBTP, and a state saved before the setting
  existed).
- New preview render for True Peak above the warning threshold.

## [1.6.0] - 2026-10-12

### Changed
- **Reset moved out of the corner it shared with Settings.** In Vertical and
  Horizontal it now sits in the bottom-left corner, under the scale; in Compact
  it sits on the left edge, vertically centred beside the reading. About stays
  in the top-left corner and Settings in the top-right one in every mode, so no
  corner holds two buttons any more. The meter is told how much room the
  overlaid buttons take (`EditorLayout::rowInsetFor`), which keeps the
  Horizontal TP / LS / LM row and the Compact reading clear of them: the
  Horizontal row gains the space the second right-hand button used to occupy.
- **Compact: no grey backdrop.** The meter used to paint a `panelBackground`
  rectangle over the whole area, which read as a dirty wash behind everything.
  It now draws straight onto the window background, so the only thing tinting
  the area is the measurement's own fill.
- **Compact: the focal reading sits on a capsule.** The fill's moving edge used
  to slice through the digits as it passed them. The reading is now drawn on a
  window-coloured capsule that masks the edge behind it, sized to a full
  "-00.0" so it keeps its width as the value changes, and its type shrinks a
  little only if the digits would not fit inside it at the smallest window
  sizes.
- Version 1.6.0.

### Tests
- Extended `testEditorLayoutTable` for the new placement: About top-left and
  Settings top-right in every mode, Reset bottom-left in Vertical/Horizontal and
  at half height in Compact, no two buttons overlapping, no reserved inset in
  Vertical, and the Horizontal minimum width still leaving the TP / LS / LM row
  its ~260 px between the overlaid buttons: **406 checks, 0 failures** (Release
  and Debug).
- Preview renders regenerated (they show the new control placement, the removed
  Compact backdrop and the reading capsule).

## [1.5.0] - 2026-10-12

### Added
- **The meter fills in instead of jumping.** When a measurement pass starts —
  and when the editor opens in the middle of one — the indicator sweeps up from
  the bottom of the scale to the first Integrated value over ~0.55 s (ease-out),
  with the large reading travelling with it, so the bar, the marker, the reading
  and its zone colour can never disagree. After the sweep the meter follows the
  measurement directly, so it never lags the value it reports. `ValueAnimation`
  (header-only, unit-tested) is driven by real elapsed time and paints nothing
  itself, so a render is reproducible; the editor's existing timer supplies the
  frame time.

### Changed
- **The scale now spans −50 … 0 LUFS** (was −30 … 0), so typical programme
  material (−14 … −16 LUFS) sits in the upper third of the meter instead of
  around its middle. The default zone boundaries (−24 … −14, red at ≥ −14) are
  unchanged, and the settings sliders follow the new range automatically.
- **Scale marks are every 10 LUFS** — 0, −10, −20, −30, −40, −50 — and are
  counted down from the top, so the maximum is always labelled and the marks
  stay on round values. A narrow window still shows a sparser ruler
  automatically (down to 15 LUFS at the Horizontal minimum width).
- **Horizontal ruler labels carry a minus sign** (`-50 … 0`); the vertical
  column keeps magnitudes only, where a repeated minus beside the meter reads as
  noise.
- **Icons redrawn at one optical size.** Reset, Settings and About used to be
  drawn at three different sizes (a wide ring, a 3 px arc, a 10-tooth gear at
  half the size), which left Reset and Settings unreadable at a 20 px button.
  All three now share one glyph box and one stroke weight: a wider reset arc
  with a solid arrow head, a six-tooth gear with a visible centre hole, and the
  About ring in the same box. Button size, hover/pressed/focus treatment and
  click areas are unchanged.
- Version 1.5.0.

### Tests
- Added `testValueAnimation` (starts at its starting point, monotonic, reaches
  the target exactly after the duration, result independent of the step
  breakdown, follows a target that moves mid-fill, clears when the value
  disappears): **376 checks, 0 failures** (Release and Debug).
- `testScaleProportion` is expressed against the scale constants instead of the
  old −30 … 0 numbers (quarter/half/three-quarter points, clamp outside the
  range), plus a check that typical programme material lands in the upper third.
- Preview renders regenerated for the wider scale, the signed Horizontal ruler
  and the new icons, plus one frame of the fill sweep for Vertical and
  Horizontal so the animation is documented even though a still cannot show
  motion.

## [1.4.0] - 2026-10-12

### Added
- **Window-size memory.** The editor remembers the size the window was dragged
  to, **per display mode**, and writes it into the plug-in state (a
  `<windowSizes>` child of the state XML). A host destroys the editor when its
  window closes and asks the newly created one what size it wants, so without
  this the window always reopened at the default — which is why REAPER appeared
  to forget the resize. Restored states are also followed by an editor that is
  already open, so loading a project or a preset updates the display mode and
  the window size instead of leaving a stale layout (ADR 0009).
- **Reset window size** button in the settings popup: forgets the current
  mode's remembered size and returns to the size that mode is designed for.

### Changed
- **Vertical is slim:** 224 × 360 → **176 × 360** (minimum 190 × 300 →
  156 × 300). The segmented bar is thinner (58 → 44 px cap), the focal reading
  is 36 px and the TP / LS / LM column beside it is drawn at 0.9 scale so the
  column keeps its room in the narrower window. The height is unchanged.
- **Horizontal is short:** 430 × 144 → **430 × 120** (minimum 360 × 136 →
  360 × 112). The reserved control strip is gone: About sits in the top-left
  corner and Reset / Settings in the top-right, drawn over the meter's empty top
  corners, and the meter is told how much room those buttons reserve so the
  TP / LS / LM row can never run under them. The row uses a 32 px band instead
  of 40 and is drawn at 0.9 scale; the trimmed label and reading areas make the
  segmented scale slightly longer rather than shorter.
- **Compact is a little smaller:** 220 × 190 → **200 × 176** (minimum
  190 × 170 → 176 × 160).
- **One control arrangement in every mode:** About on the left, Reset and
  Settings on the right. Previously all three buttons sat in the top-right
  corner, except in Compact where About was already on the left.
- Icon buttons are 20 px (was 22 px), matching the smaller chrome.
- The editor chrome (padding, strip height, button size) and the per-mode
  preferred/minimum sizes now live in one place, `src/EditorLayout.h`, read by
  the editor, the window-size clamp and the offscreen preview tool; the preview
  tool renders the full chrome (meter and icon buttons) at the same bounds.
- Version 1.4.0.

### Tests
- Added `testEditorLayoutTable` (preferred ≥ minimum for every mode, the
  cross-axis compaction, the overlay rule, the button placement, and the
  guarantee that the Horizontal minimum width leaves the TP / LS / LM row clear
  of the overlaid buttons) and `testEditorWindowSizes` (per-mode XML round-trip,
  repeated save/load, clamping to each mode's limits, damaged or absent values,
  out-of-range mode indices, and that a degenerate size never erases a valid
  memory): **332 checks, 0 failures** (Release and Debug).
- Preview renders regenerated at the new sizes, extended with the minimum size
  of every mode, a no-label/no-scale variant and the settings panel.

## [1.3.0] - 2026-10-10

### Added
- **Three mutually exclusive display modes**, selected in one settings selector:
  **Vertical** and **Horizontal** (the existing segmented meters, unchanged) and
  **Compact**, a nearly square digital loudness meter (preferred 220 × 190,
  minimum 190 × 170). Compact's main display area is one **continuous**
  bottom-anchored fill whose top edge moves vertically with the Integrated value
  (shared scale mapping), taking the configured zone colour at low alpha; there
  is no segmented bar, ruler, tick labels, marker or standalone meter. The large
  centred LUFS-I reading and the TP / LS / LM row are drawn on top. Controls sit
  in a minimal top row: Info on the left, Reset and Settings on the right.
- **About / information** vector `IconButton` in every display mode, opening an
  `AboutContent` popup with the plugin metadata, the four measurements and the
  third-party attributions (JUCE, libebur128).

### Changed
- `MeterSettings` now stores a single authoritative `MeterDisplayMode`
  (`vertical` / `horizontal` / `compact`). The legacy `MeterOrientation` /
  `MeterSizeMode` enums remain only as the saved-state vocabulary. The settings
  panel shows one three-option display-mode selector instead of the orientation
  radios and the size dropdown.
- Settings persistence writes `displayMode` plus the derived legacy
  `orientation` / `size` attributes. On load `displayMode` takes precedence, so
  repeated save/load never changes the selected mode. Legacy states migrate
  deterministically: Vertical (slim or compact) → Vertical, Horizontal (slim or
  compact) → Horizontal, absent → Horizontal. An older build cannot know
  Compact, so a Compact state is written as Horizontal and opens as the
  Horizontal layout.
- Compact mode redesigned from the initial wide, short segmented strip
  (340 × 104) into a nearly square meter (220 × 190) with a continuous moving
  fill, a substantially larger LUFS-I reading (~65% larger), TP / LS / LM
  arranged as three label-over-value columns (matching Vertical mode) whose
  values are ~35% larger than the previous inline row, and Info moved to the
  top-left with Reset / Settings at the top-right. `MeterSettings::scaleProportion`
  is exposed as the shared, unit-tested scale mapping used by both the segmented
  bar and the Compact fill.

### Tests
- Added `testLegacyDisplayModeMigration`, `testScaleProportion` (the shared Compact
  fill mapping at the scale minimum/maximum, intermediate values and clamping) and
  extended `testSettingsPersistence` (repeated save/load stability, all three
  modes, out-of-range clamping): **284 checks, 0 failures** (Release and Debug).
  New offscreen previews for the three modes, the Compact fill at several LUFS-I
  levels, and the About popup.

## [1.2.0] - 2026-10-09

### Added
- **Momentary Loudness (M)** (400 ms) and **Short-Term Loudness (S)** (3 s)
  numeric readouts, computed by libebur128 from the same K-weighting pass as
  Integrated. Both are ungated, as the standards require.
- **True Peak (TP)** in dBTP, using libebur128's oversampling interpolator and
  reporting the maximum across all channels for the current pass.
- Compact numeric-only secondary readouts, in the display order **TP, LS, LM**
  (True Peak, Short-Term, Momentary): a single row above the scale in horizontal
  orientation and three stacked two-line groups to the left of the scale in
  vertical orientation. Numeric values are larger and heavier than their
  abbreviations. True Peak is shown in red at or above `0.0 dBTP`; unavailable
  values use the em-dash placeholder `—`.
- Coherent reading snapshot published from the measurement thread to the UI via
  a seqlock over atomic fields (ADR 0008); the UI never sees a mix of two
  updates.
- Tests for M/S/TP: reference levels, warm-up before the window is full, silent
  windows, rolling transitions, ungated low-level signals, inter-sample true
  peak, cross-channel maximum, reset, sample rates, block sizes, and concurrent
  snapshot reads.

### Changed
- `LoudnessMeter` now enables `EBUR128_MODE_I | EBUR128_MODE_S |
  EBUR128_MODE_TRUE_PEAK` and exposes M/S/TP getters, with warm-up gating and a
  documented `-120 LUFS` silence floor for M/S.
- Horizontal preferred/minimum height and width increased, and vertical
  preferred/minimum width increased, to fit the larger secondary readouts; the
  primary composition is otherwise unchanged.
- Secondary readout polish: labels are now TP, LS, LM. The horizontal row hugs
  the top of its band (removing dead space above), is centred on the segmented
  scale, and keeps a clear gap above it. Each vertical label sits directly above
  its value, with a wider gap between the three groups. The large LUFS-I reading
  is aligned to the meter's visual centre using the digit glyph ink bounds.
- Plugin version bumped to `1.2.0`.

### Documentation
- Updated measurement, architecture, testing, README and ADR 0002; added ADR
  0008 for the coherent snapshot. Removed the "out of scope" statements for
  Momentary, Short-Term and True Peak.

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
