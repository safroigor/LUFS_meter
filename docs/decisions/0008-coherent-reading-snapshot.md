# ADR 0008 — Coherent reading snapshot between the worker and the UI

- **Status:** accepted
- **Date:** 2026-10-09

## Context

The meter now publishes four related measurements from the measurement thread to
the UI thread: Integrated, Momentary, Short-Term and True Peak. They are produced
together in one pass and must be shown as one coherent set — the UI must never
render, say, a new Integrated value beside a Short-Term value from a different
update, and it must never observe a torn value.

The audio callback is not involved in this: it only copies samples into the SPSC
ring buffer. The writer is the measurement worker thread and the reader is the UI
timer thread, so the existing real-time constraints (ADR 0003) are unaffected.

## Decision

Publish the four measurements as a single `MeasurementEngine::Reading` through a
**seqlock** over atomic payload fields:

- Every payload field is a `std::atomic` (so concurrent access is free of data
  races / undefined behaviour, even though the sequence protocol "protects" the
  combination).
- A `std::atomic<unsigned long long>` sequence is incremented to an odd value
  before a write and to the next even value after it, with release fences around
  the payload stores.
- The reader loads the sequence with acquire semantics, retries if it is odd,
  reads the payload with relaxed loads, then re-checks the sequence; if it
  changed, it retries.

The payload fields are atomic specifically so the C++ memory model is respected:
a version counter alone does **not** make concurrent non-atomic access safe.

## Consequences

- The UI always sees a complete, self-consistent set of the four values.
- No locks are taken on any thread; the audio callback remains allocation- and
  lock-free.
- The worker thread remains the sole owner of the `ebur128_state`; publication is
  the only cross-thread channel.
- A reset publishes an all-default snapshot, so the UI clears all four values at
  once.

## Alternatives considered

- **Independent atomics per value**: rejected — allows the UI to show a mix of
  updates.
- **`juce::SpinLock`/`std::mutex` around the snapshot**: correct, but a seqlock
  keeps the publication lock-free and matches the existing design's preference.
- **`std::atomic<std::shared_ptr<Reading>>`**: would allocate on every publish
  and is not guaranteed lock-free.
