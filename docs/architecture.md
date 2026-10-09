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
- Wraps libebur128 and owns its `ebur128_state`.
- Runs on a dedicated measurement thread (see decisions) that drains the SPSC
  ring buffer and calls `ebur128_add_frames_float`.
- Computes `ebur128_loudness_global` at a bounded rate and publishes the result.
- Handles reset (destroy + re-init) entirely on the measurement thread.
- Does not depend on the GUI or on VST3, so it can be unit-tested outside a DAW.
- Maps libebur128's `-HUGE_VAL` (no block above the absolute gate) to an explicit
  "no valid value" state instead of a fake `0 LUFS`.

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

> Stage 1 contains only the buildable skeleton. The ring buffer, measurement
> thread and atomics are implemented in Stage 2.

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
