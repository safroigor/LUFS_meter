# ADR 0003 — Dedicated measurement thread in exact mode

- **Status:** accepted
- **Date:** 2026-10-09

## Context

The audio callback must be real-time safe: no allocation, no locks, no blocking
calls. libebur128 v1.2.6, in its default mode, calls `malloc` for every 100 ms
gating block, so `ebur128_add_frames_*` cannot run in the audio callback.
`EBUR128_MODE_HISTOGRAM` avoids allocation but quantises block energies to
0.1 LU bins.

## Decision

Run the loudness measurement on a **dedicated measurement thread** in the
**default (exact, non-histogram)** mode.

- The audio thread only copies interleaved samples into a lock-free SPSC ring
  buffer and sets an atomic reset flag.
- The measurement thread owns the `ebur128_state`, drains the ring buffer and
  computes Integrated LUFS at a bounded rate.
- Reset (destroy + re-init) happens on the measurement thread.
- The result is published to the UI via an atomic snapshot.

## Consequences

- The audio callback performs no allocation, no locking and no library work
  beyond the copy, giving strong real-time safety.
- Full measurement precision is preserved (no histogram quantisation).
- Additional complexity: an SPSC ring buffer and a worker thread, plus handling
  of ring-buffer overflow.

## Alternatives considered

- **Histogram mode on the audio thread** — simpler, but quantised results and a
  harder reset path.
- **Exact mode on the audio thread** — rejected: allocates in the audio callback.
