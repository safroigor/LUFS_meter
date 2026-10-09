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
- Maps libebur128's `-HUGE_VAL` (no block above the absolute gate) to an explicit
  "no valid value" result instead of a fake `0 LUFS`.

### MeasurementEngine
- Owns a `LoudnessMeter`, a lock-free SPSC ring buffer (`juce::AbstractFifo`)
  and a worker thread.
- Audio thread: copies interleaved samples into the ring buffer and sets an
  atomic reset flag. No allocation, no locking, no library calls.
- Worker thread: drains the buffer, feeds the meter, computes Integrated LUFS at
  a bounded rate (20 ms period) and publishes `{hasValue, value}` atomically.
- Reset (destroy + re-init) happens entirely on the worker thread.

### PluginProcessor (processBlock)
- Receives audio buffers from the host.
- Queries `TransportTracker` to decide whether a new measurement pass begins;
  if so, it raises the atomic reset request.
- Delegates to `MeasurementEngine::pushBlock` in `processBlock`.
- Exposes an atomic snapshot and a reset request to the editor.
- Never modifies the audio signal.

### PluginEditor
- Displays the numeric Integrated LUFS value rounded to 0.1 LU.
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
- Reads only the atomic snapshot; performs no DSP. Changing orientation or size
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
- **UI thread**: reads an atomic snapshot of the value at timer intervals. It
  never touches the measurement state.

Reset is requested through an atomic flag and executed on the measurement
thread, so it can neither race with `add_frames` nor corrupt the accumulator.
After a reset, `hasValue()` is false until at least one full gating block of new
audio has been measured, so a stale value is never shown as a new result.

## Data flow

1. Host calls `processBlock`.
2. Processor copies input channels into the ring buffer (interleaved layout).
3. Measurement thread drains the ring buffer and feeds libebur128.
4. Measurement thread periodically computes Integrated LUFS and stores it
   atomically.
5. Editor timer reads the atomic value and repaints.

## Non-goals

- No True Peak, Momentary/Short-term, LRA, history graphs or target levels in v1.
- No audio processing; the signal is passed through unchanged.
