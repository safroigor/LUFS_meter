# Architecture

## Overview

The plugin separates three concerns:

```
      audio thread                    measurement thread            UI thread
  +-------------------+   SPSC    +---------------------+      +-----------------+
  | PluginProcessor   |  ring     | LoudnessMeter       |      | PluginEditor    |
  |  - copy channels  | --------> |  - ebur128_state    |      |  - numeric value |
  |  - reset flag     |  buffer   |  - add_frames       | ---> |  - bar scale     |
  +-------------------+           |  - loudness_global  | atom |  - controls      |
          |                       +---------------------+      +-----------------+
          |  pass-through (audio is never modified)                    ^
          v                                                            |
        host output                                          std::atomic snapshot
```

## Components

### PluginProcessor
- Receives audio buffers from the host.
- Copies the channels into a lock-free single-producer/single-consumer (SPSC)
  ring buffer. The audio callback performs no allocation, no locking, no I/O and
  no logging.
- Asks `TransportTracker` whether the block starts a new measurement pass and,
  if so, raises the atomic reset request.
- Publishes the latest measured value to the UI through an atomic snapshot.
- Never modifies the audio signal.

### TransportTracker
- Pure, dependency-free policy object queried once per block by the audio
  thread. It decides when a new measurement pass starts from the host transport
  state and playback position, with no knowledge of the audio signal.
- No JUCE dependency, so the lifecycle is unit-tested directly.

### LoudnessMeter
- Core wrapper around libebur128; owns the `ebur128_state`.
- Single-threaded by contract; has no JUCE/GUI dependency, so it is unit-tested
  directly (see `tests/TestMain.cpp`).
- Uses the exact (non-histogram) mode to avoid 0.1 LU quantisation.
- Enables all four measurements on one state
  (`EBUR128_MODE_I | EBUR128_MODE_S | EBUR128_MODE_TRUE_PEAK`): Integrated,
  Momentary (400 ms), Short-Term (3 s) and True Peak, sharing one K-weighting
  pass.
- Tracks frames fed since the last prepare/reset to keep M unavailable until a
  full 400 ms window exists and S until a full 3 s window exists (libebur128
  zero-fills the not-yet-filled part of its rolling buffer).
- Maps libebur128's `-HUGE_VAL` (no block above the absolute gate, or a fully
  silent M/S window) to an explicit "no valid value" result instead of a fake
  `0 LUFS`, and applies a documented `-120 LUFS` silence floor to M/S so the
  filter's decaying tail is not shown as a nonsense number.

### MeasurementEngine
- Owns a `LoudnessMeter`, a lock-free SPSC ring buffer (`juce::AbstractFifo`)
  and a worker thread.
- Audio thread: copies interleaved samples into the ring buffer and sets an
  atomic reset flag. No allocation, no locking, no library calls.
- Worker thread: drains the buffer, feeds the meter, computes all four values at
  a bounded rate (20 ms period) and publishes them as one coherent `Reading`
  snapshot.
- Reset (destroy + re-init) happens entirely on the worker thread and publishes
  an all-default snapshot.

### PluginProcessor (processBlock)
- Receives audio buffers from the host.
- Queries `TransportTracker` to decide whether a new measurement pass begins;
  if so, it raises the atomic reset request.
- Delegates to `MeasurementEngine::pushBlock` in `processBlock`.
- Exposes an atomic snapshot and a reset request to the editor.
- Never modifies the audio signal.

### PluginEditor
- Displays the numeric Integrated LUFS value rounded to 0.1 LU.
- Has three mutually exclusive display modes (`MeterDisplayMode`):
  - **Vertical** — the segmented vertical meter, slim (176 × 360) with the
    TP / LS / LM column beside it.
  - **Horizontal** — the segmented horizontal meter, short (430 × 120) with the
    TP / LS / LM row above the scale and the icon buttons overlaid on the
    corners.
  - **Compact** — a nearly square digital meter (200 × 176) whose main display
    area is one continuous, bottom-anchored fill
    (`MeterComponent::paintCompact`). The fill's top edge moves vertically with
    the Integrated value on the shared `MeterSettings::scaleProportion` mapping
    and takes the configured zone colour at low alpha (subtler than the other
    modes, so it stays a background element). There is no panel backdrop: the
    meter draws on the window background, so the fill is the only thing tinting
    the area. There is no segmented bar, ruler, tick labels, marker or standalone
    meter; the large centred reading sits on a window-coloured capsule that masks
    the fill's moving edge, so the digits are never sliced by it, and the
    TP / LS / LM row is drawn on top of the fill. The secondary row is three
    label-over-value columns (each label directly above its larger value), the
    same hierarchy as Vertical mode.
- Displays a compact, numeric-only secondary block for True Peak, Short-Term and
  Momentary, labelled in the display order **TP, LS, LM**, subordinate to the
  Integrated reading. Numeric values are larger and heavier than their
  abbreviations. No separate meters are added. Unavailable values use the
  em-dash placeholder `—`; True Peak turns red at or above
  `MeterSettings::truePeakRedThresholdDbtp` (−1.0 dBTP by default, set in the
  settings popup and clamped to −20 … 0 dBTP).
- Draws the segmented bar (horizontal or vertical) or the Compact background
  indicator via `MeterComponent`; segment and numeric-reading colour both come
  from the configured zones via the shared `MeterSettings::zoneForValue`, not
  from drawing code.
- The display scale spans `MeterSettings::scaleMinLufs` (-50) to
  `scaleMaxLufs` (0) LUFS, so typical programme material sits in the upper third
  of the meter. Marks are every 10 LUFS, counted down from the maximum so the top
  of the scale is always labelled; a narrow window automatically shows a sparser
  ruler. The Horizontal ruler prints signed labels (`-50 … 0`), the vertical
  column magnitudes only.
- Uses a shared visual system: `MeterTheme` (palette and typography) and
  `MeterLookAndFeel` (control styling); Reset, Settings and About are vector
  `IconButton`s drawn in one shared glyph box with one stroke weight, so all
  three read at the 20 px button size. About sits in the top-left corner and
  Settings in the top-right one in every display mode, and Reset keeps out of
  their way: the bottom-left corner in Vertical and Horizontal, the left edge at
  half height in Compact. Horizontal overlays them on the meter's own empty
  corners (no reserved strip) so its window can stay short; the meter is told
  how much room they take (`EditorLayout::rowInsetFor` →
  `MeterComponent::setControlInsets`) so the Horizontal TP / LS / LM row and the
  Compact reading capsule stay clear of them. The About/info button opens
  `AboutContent` (plugin metadata, measurement descriptions and third-party
  attributions) in a `CallOutBox`. Both popups are desktop windows — the
  settings panel is taller than the smallest plug-in windows, so it must not be
  clipped — and dismiss on a click outside them, whether that click lands in the
  plug-in window (JUCE's modal handling) or in the host's own UI, which JUCE's
  modal handling cannot see: the editor watches the real OS mouse state for it.
- Provides a settings panel: a three-option display-mode selector
  (Vertical/Horizontal/Compact), Show label, Show scale, green low/high, red
  threshold, the True Peak warning threshold, and the default/green/red
  colours.
- Settings are persisted in the plugin state (saved/restored with the host
  project).
- Reads only the coherent snapshot; performs no DSP. Changing the display mode
  never resets the measurement.

### Filling in the first value

An Integrated value only exists once a full gating block has been measured, so a
pass that starts by displaying a number would jump from empty to it. Instead the
indicator sweeps up from the bottom of the scale to that first value over
`ValueAnimation::defaultDurationSeconds` (0.55 s, ease-out), and the large
reading travels with it. `MeterComponent::advanceAnimation` drives the sweep from
the editor's 30 Hz timer using real elapsed time, so the duration is independent
of the frame rate; the component's `paint` substitutes the animated value into a
copy of the engine's `Reading`, which is why the bar, the Compact fill, the
marker, the reading and its zone colour can never disagree. Once the sweep ends
the meter follows the measurement directly.

When a pass has no value yet — a seek, playback restarting, or the first block —
the meter drops back to the start of the scale: the reading blanks and the bar
empties (`ValueAnimation::clear`), so a new measurement is unmistakable, and the
first value of the pass then fills up from the bottom. The Reset button clears
the display at once, without waiting for the engine to publish the empty reading
(`MeterComponent::resetDisplay`).

The animation holds no clock of its own and paints nothing itself: `ValueAnimation`
is a pure, unit-tested state machine, and `MeterComponent::settleAnimation` puts
an offscreen render (or a test) straight onto the settled value.

### Editor geometry and the remembered window size

`EditorLayout` (header-only, juce_core only) is the single source of truth for
the chrome (`outerPadding` 6, `controlStripHeight` 22, 20 px icon buttons) and
for each mode's preferred and minimum window size, plus the shared control
placement. The editor, the window-size clamp and the offscreen preview tool all
read it, so a render and a host window cannot drift apart.

| mode | preferred | minimum |
|---|---|---|
| Vertical | 176 × 360 | 156 × 300 |
| Horizontal | 430 × 120 | 360 × 112 |
| Compact | 200 × 176 | 176 × 160 |

The editor remembers the window size the user drags each mode to, so a host can
restore it: hosts destroy the editor when its window closes and ask the newly
created one for its size, which is why an unpersisted size always came back at
the default. `EditorWindowSizes` (header-only, unit-tested) holds one size per
mode, clamps it to the mode's limits, and writes it as a `<windowSizes>` child of
the state XML; the processor stores the sizes in atomics because
`getStateInformation` may run off the message thread. Only sizes the editor did
*not* apply itself are remembered (a host resize or a dragged corner), so
"never resized" stays distinct from "resized to exactly the default", and
restoring a state broadcasts a change so an open editor follows the restored
settings, mode and size. See `docs/decisions/0009-editor-window-size-persistence.md`.

### Display modes and settings persistence

`MeterSettings` stores a single authoritative `MeterDisplayMode`
(`vertical`, `horizontal`, `compact`). The legacy `MeterOrientation` and
`MeterSizeMode` enums remain only as the saved-state vocabulary:

- `toXml()` writes `displayMode` **and** the derived legacy `orientation`/`size`
  attributes, so an older build still loads a sensible layout.
- `fromXml()` uses `displayMode` whenever it is present (so repeated save/load
  never changes the selected mode). Otherwise it migrates from the legacy pair:

  | legacy orientation | legacy size | restored mode |
  |---|---|---|
  | vertical | slim | Vertical |
  | vertical | compact | Vertical |
  | horizontal | slim | Horizontal |
  | horizontal | compact | Horizontal |
  | absent | absent | Horizontal (default) |

  Slim and Compact both collapse to their orientation's single display mode; the
  legacy `size` attribute is intentionally ignored in the migration so the result
  is deterministic. Because an older build cannot know about Compact, a Compact
  state is written with `orientation = horizontal`, so old builds show the
  Horizontal layout (the closest legacy layout) and the meter keeps working.


## Measurement lifecycle (transport-based)

The meter measures one playback pass at a time:

- While the transport is playing, the Integrated value is measured
  continuously, **including silent passages**.
- A new pass — which clears the accumulated value — starts only on a
  transport/position event:
  - play after the transport has stopped (including from the same position);
  - seeking to another position while playing;
  - a loop wrapping back to its start;
  - a gap of more than `1000 ms` between callbacks while the host claims to be
    playing.
- While the transport is not playing the meter is not fed, so the last value
  stays frozen; the next pass clears it and shows the "no value" placeholder
  until a fresh value is available.

Signal amplitude is never used as a reset condition. The thresholds, the
pause-vs-stop limitation and the no-transport fallback are recorded in
`docs/decisions/0007-transport-lifecycle.md`.

## Threading and synchronisation

- **Audio thread**: writes into the SPSC ring buffer, sets/clears an atomic reset
  flag. No allocation, no locks, no blocking calls.
- **Measurement thread**: the sole owner of `ebur128_state`. libebur128 is not
  thread-safe, so a single thread must own each state.
- **UI thread**: reads a coherent `Reading` snapshot (Integrated, Momentary,
  Short-Term, True Peak) at timer intervals. It never touches the measurement
  state.

Reset is requested through an atomic flag and executed on the measurement
thread, so it can neither race with `add_frames` nor corrupt the accumulator.
After a reset, the published snapshot is all-default; Integrated stays
unavailable until at least one full gating block of new audio has been measured,
and Momentary/Short-Term stay unavailable until their windows are full again, so
a stale value is never shown as a new result.

The snapshot is published with a seqlock over atomic payload fields, so the UI
always observes a complete, self-consistent set of the four values without
locking. See `docs/decisions/0008-coherent-reading-snapshot.md`.

## Data flow

1. Host calls `processBlock`.
2. Processor copies input channels into the ring buffer (interleaved layout).
3. Measurement thread drains the ring buffer and feeds libebur128.
4. Measurement thread periodically computes all four values and publishes them as
   one coherent snapshot.
5. Editor timer reads the snapshot and repaints.

## Non-goals

- No Loudness Range (LRA), history graphs or target levels in v1.2.
- No audio processing; the signal is passed through unchanged.
