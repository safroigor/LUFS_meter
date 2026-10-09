# ADR 0005 — Standard revisions and host

- **Status:** accepted
- **Date:** 2026-10-09

## Context

Measurement must reference current, available standards, and be verifiable in a
real host.

## Decision

- Measure per **ITU-R BS.1770-5 (11/2023)** (current in-force revision; BS.1770-4
  is superseded) and **EBU R 128 v5.0**.
- Use **EBU Tech 3341** tolerances and its minimum-requirement test signals for
  verification.
- Use **REAPER x64** as the primary test host (installed on the development
  machine).
- Use **MeldaProduction MLoudnessAnalyzer** as an independent cross-check, not as
  the sole correctness proof.

## Consequences

- BS.1770-5 and R 128 v5.0 PDFs are publicly reachable; EBU Tech 3341 PDF is
  reachable as well.
- The EBU Loudness Test Set v5.0 (48 kHz) is an external asset and is **not**
  committed to the repository (size, EBU Terms of Use). It is downloaded locally
  for testing.
- Test coverage at 44.1/96/192 kHz uses project-synthesised signals, since the
  EBU set is 48 kHz.

## Alternatives considered

- BS.1770-4 (rejected: superseded).
- Other hosts (REAPER chosen because it is installed).
