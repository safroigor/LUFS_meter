# ADR 0002 — Use libebur128 for loudness measurement

- **Status:** accepted
- **Date:** 2026-10-09

## Context

Integrated Loudness per ITU-R BS.1770 / EBU R128 must be measured accurately.
A well-tested implementation is preferable to a hand-rolled one.

## Decision

Use **libebur128 v1.2.6** (MIT), pinned as a submodule.

Rationale, verified from its source and documentation:

- Implements M, S and I modes per BS.1770.
- Recalculates K-weighting coefficients for any sample rate.
- Applies the −70 LUFS absolute gate and −10 LU relative gate, with the −0.691
  offset.
- Supports ITU channel maps and channel weighting.
- Returns `-HUGE_VAL` for silence, which maps cleanly to an explicit "no value"
  state.
- MIT license, compatible with the project's AGPLv3.

## Consequences

- The library is not thread-safe and allocates per gating block in its default
  mode; it must run on a single dedicated thread, not in the audio callback
  (see ADR 0003).
- It is compiled directly from source, bypassing its outdated CMake project.

## Alternatives considered

- Hand-written BS.1770 implementation (rejected: higher risk of subtle errors).
- Other libraries (rejected: libebur128 is established and license-clean).

## Notes

As of v1.2.0 the library's **true-peak** feature (`EBUR128_MODE_TRUE_PEAK`) and
**Momentary/Short-Term** modes (`EBUR128_MODE_S`) are used as well as Integrated;
the three measurements share one `ebur128_state` and one K-weighting pass. LRA
remains unused. See `docs/measurement.md` and
`docs/decisions/0008-coherent-reading-snapshot.md`.
