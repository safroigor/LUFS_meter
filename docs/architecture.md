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
- Displays a compact, numeric-only secondary block for True Peak, Short-Term and
  Momentary, labelled in the display order **TP, LS, LM**, subordinate to the
  Integrated reading: a single row above the segmented scale in horizontal
  orientation, and three stacked two-line groups (label above a larger value) to
  the left of the scale in vertical orientation. Numeric values are larger and
  heavier than their abbreviations. No separate meters are added. Unavailable
  values use the em-dash placeholder `—`; True Peak turns red at or above
  `0.0 dBTP`.
- Draws a segmented bar (horizontal or vertical) via `MeterComponent`; segment
  and numeric-reading colour both come from the configured zones via the shared
  `MeterSettings::zoneForValue`, not from drawing code.
- Uses a shared visual system: `MeterTheme` (palette and typography) and
  `MeterLookAndFeel` (control styling); Reset and Settings are vector
  `IconButton`s in a thin upper-right control strip.
- Provides a settings panel: orientation (Horizontal/Vertical), size
  (Slim/Compact), Show label, Show scale, green low/high, red threshold, and the
  default/green/red colours.
- Settings are persisted in the plugin state (saved/restored with the host
  project).
- Reads only the coherent snapshot; performs no DSP. Changing orientation or size
  never resets the measurement.

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
