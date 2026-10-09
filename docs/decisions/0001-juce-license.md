# ADR 0001 — Use JUCE under AGPLv3

- **Status:** accepted
- **Date:** 2026-10-09

## Context

The plugin needs a framework that provides VST3 packaging and a GUI on Windows.
JUCE is dual-licensed: AGPLv3 or a commercial license.

## Decision

Use **JUCE 8.0.15** under the **AGPLv3** option. The project is open source and
is distributed under AGPLv3 (see `LICENSE`).

## Consequences

- All source of the plugin must be available under AGPLv3-compatible terms.
- Network use of a modified version triggers source-disclosure obligations under
  AGPLv3.
- libebur128 (MIT) is compatible and is included as a submodule with its license
  retained.
- If the license model ever changes to closed source, a commercial JUCE license
  would be required.

## Alternatives considered

- Commercial JUCE license (rejected: project is open source).
- Raw Steinberg VST3 SDK (rejected for v1: more manual GUI/packaging work).
