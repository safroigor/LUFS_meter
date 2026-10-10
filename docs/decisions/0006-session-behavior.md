# ADR 0006 — Session behaviour: freeze on interruption, restart on resume

- **Status:** superseded by [0007](0007-transport-lifecycle.md)
- **Date:** 2026-10-09

## Context

The meter is used to audition material in a host. The requirement is:

- Any interruption of the signal (transport stop/pause, or a break in the audio)
  must **freeze** the displayed value.
- As soon as playback/audio resumes, a **new measurement** must start that does
  **not** include anything accumulated before the interruption.

## Decision

The processor tracks a measurement "session":

- **Interruption** is detected when *any* of the following holds:
  - the host reports the transport as not playing (`AudioPlayHead::getIsPlaying()`);
  - the audio callback is not called for more than `1000 ms` (host stopped
    processing while stopped), checked on the next callback;
  - the input is below `-100 dBFS` (essentially digital silence) for more than
    `500 ms`.
- While interrupted, no audio is fed to the meter and the last published value
  remains visible (frozen).
- When audio resumes, a reset is requested and the measurement restarts from
  zero. The display shows the "no value" placeholder until a new value is
  available.
- Changing the UI display mode (Vertical / Horizontal / Compact) never resets
  the measurement.
- The UI Reset button performs the same restart on demand.

## Consequences

- Stated behaviour is met: value freezes on stop/pause/breaks and restarts on
  playback.
- Because a genuine break in the audio (>= 0.5 s of near-silence) also triggers
  a restart, material containing such gaps is measured per contiguous section
  rather than end-to-end. This is intended by the requirement, but it is a
  deliberate deviation from a single continuous Integrated measurement of a
  whole programme.
- The thresholds (`-100 dBFS`, `500 ms`, `1000 ms`) are compile-time constants
  and can be tuned if the practical behaviour needs adjustment.

## Alternatives considered

- **Transport state only**: cleaner, but would not react when a host keeps
  playing digital silence.
- **Signal level only**: would react to silence but not to a host that stops
  processing entirely.
- Continuous single measurement of the whole programme (standard Integrated
  behaviour): rejected because the requirement explicitly wants per-session
  measurement.
