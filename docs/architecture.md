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
- Exposes a reset request as an atomic flag.
- Publishes the latest measured value to the UI through an atomic snapshot.
- Never modifies the audio signal.

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

### PluginProcessor
- Receives audio buffers from the host.
- Delegates to `MeasurementEngine::pushBlock` in `processBlock`.
- Exposes an atomic snapshot and a reset request to the editor.
- Never modifies the audio signal.

### PluginEditor
- Displays the numeric Integrated LUFS value rounded to 0.1 LU.
- Draws a slim bar scale (horizontal or vertical).
- Provides Reset and display options (orientation, size, label, scale).
- Reads only the atomic snapshot; performs no DSP.

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
