# ADR 0004 — Pin dependencies as git submodules

- **Status:** accepted
- **Date:** 2026-10-09

## Context

Reproducible builds require exact dependency versions. Floating references to
`master`/`main` are not acceptable as versions.

## Decision

Pin both dependencies as git submodules at exact tags:

| Dependency | Tag | Commit |
|---|---|---|
| JUCE | `8.0.15` | `91ad83ae34a81e0833b1a2b0866f54846370ae53` |
| libebur128 | `v1.2.6` | `67b33abe1558160ed76ada1322329b0e9e058b02` |

The gitlink commit recorded by the superproject fixes the exact revision a
clean checkout will use.

## Consequences

- A clean checkout with `--recurse-submodules` reproduces the build.
- Updating a dependency is an explicit, reviewable commit that changes the
  pinned commit and the documented version.

## Alternatives considered

- CMake `FetchContent` with `GIT_TAG` (viable; submodules chosen for
  transparency and offline re-use).
- System-installed packages (rejected: not reproducible).
