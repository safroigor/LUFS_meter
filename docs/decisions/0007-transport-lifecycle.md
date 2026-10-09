# ADR 0007 — Transport-based measurement lifecycle

- **Status:** accepted
- **Date:** 2026-10-09
- **Supersedes:** [0006](0006-session-behavior.md)

## Context

The original session behaviour (ADR 0006) treated a genuine break in the audio —
`-100 dBFS` for more than `500 ms` — as an interruption and restarted the
measurement when audio returned. That is wrong for spoken-word and programme
material: silence between takes is part of the programme and must **not** clear
the accumulated Integrated Loudness, and the amplitude of the signal is not a
transport event.

The requirement is now explicit:

- silent passages during continuous playback must not reset the measurement;
- a new playback pass resets it;
- restarting after a stop, and seeking, start a new pass;
- signal amplitude / a silence threshold must **not** be used as a reset
  condition.

## Decision

The measurement lifecycle is driven exclusively by the host transport and the
playback position, in a small pure class (`TransportTracker`) that the audio
thread queries once per block. A **new pass** (which clears the accumulated
value) starts when:

1. the transport transitions from not-playing to playing. This covers
   stop -> play, including resuming from the identical position, where position
   continuity alone could not detect the restart;
2. while playing, the playback position jumps by more than a small tolerance
   (a seek, or a loop wrapping back to its start). The tolerance
   (`lastBlock + currentBlock + 20 ms`, min `50 ms`) absorbs normal block
   boundaries and jitter so minor fluctuations never cause a false reset;
3. the host claims to be playing but does not call `processBlock` for more than
   `1000 ms` (a host that stops processing without reporting it).

While the transport is not playing the meter is not fed, so the last value stays
frozen on screen. While it is playing, **every** block is fed to the meter,
including digital silence.

### Pause vs stop

JUCE's `AudioPlayHead::PositionInfo` exposes a single `getIsPlaying()` flag and
(reliably) a playback position; it does **not** distinguish pause from stop.
Because the requirement demands that play-after-stop from the same position
resets, and that case is indistinguishable from pause/resume, both are treated
as the start of a new pass. This is documented behaviour, not an accident.

### Hosts without transport

If `getPlayHead()` returns nothing, or the position info is unavailable, the
transport is treated as continuously playing and no automatic resets occur. The
measurement then behaves like a standalone integrated meter and is only cleared
by the manual Reset button. This is the most robust behaviour available without
transport information.

## Manual reset

The Reset button (now in the main UI) requests a reset on the measurement
thread, discarding any queued audio so no stale samples leak into the new
measurement. Playback is not interrupted: the displayed and accumulated value is
cleared, and a fresh measurement begins from the current point.

## Consequences

- Silence is measured like any other audio: it contributes nothing above the
  gates but never clears history.
- Take-based material (e.g. spoken word with gaps) reads as one integrated
  value per playback pass.
- Pause/resume is a pass boundary; users who want a single pass across a pause
  should not pause, or should treat the pause as a deliberate split.
- Reset (destroy + re-init of the libebur128 state) still happens only on the
  measurement thread, so the audio callback remains real-time safe.

## Alternatives considered

- **Signal-level detection** (ADR 0006): rejected — it resets on silence, which
  is exactly the reported defect.
- **Position continuity only**: cannot detect stop -> play from the same
  position, which the requirement requires to reset.
- **Pause-aware state machine**: not implementable reliably, because the host API
  does not distinguish pause from stop.
