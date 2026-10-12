# ADR 0009 — Remember the editor window size per display mode

- **Status:** accepted
- **Date:** 2026-10-12

## Context

The editor is resizable, and in REAPER the size the user drags the window to was
lost: reopening the plug-in window (or reloading the project) brought the window
back at the default size.

The cause is not the host. A host destroys the editor when the window closes and
creates a new one when it reopens, then sizes its window to whatever that new
editor reports. The editor sized itself to a hard-coded default in its
constructor and nothing about the user's choice was ever written to the state
chunk — so there was nothing for the host to restore.

The three display modes have very different shapes (a tall column, a short
strip, a small square), so a single remembered size would be wrong for two of
them, and switching modes has always reset the window to the mode's default.

## Decision

Store the window size in the **plug-in state**, one entry per display mode.

- The editor remembers a size only when the window was sized by someone else:
  a host resize request, or the user dragging the corner. Sizes the editor
  applies itself (opening, switching mode, restoring a state, Reset window size)
  are not recorded, so "never resized" stays distinguishable from "resized to
  exactly the default".
- `EditorWindowSizes` (header-only, unit-tested) owns the three sizes, writes
  them as a `<windowSizes>` child of the state XML and clamps whatever it reads
  to the mode's limits, so a chunk from another build cannot restore an
  unusable window.
- The processor keeps the sizes as atomics: the editor writes them on the
  message thread, while `getStateInformation` may be called from any thread.
- Restoring a state broadcasts a change; an editor that is already open follows
  the restored settings, mode and size instead of leaving a stale layout.
- The geometry itself — preferred size, minimum size and chrome — lives in
  `EditorLayout.h`, shared by the editor, the size clamp and the preview tool.

## Consequences

- In REAPER the window reopens at the size the user chose for that display mode,
  as soon as the project (or a preset) carries the state; each mode keeps its
  own size, and switching modes returns to that mode's size.
- The size travels with the project and with presets, which is the behaviour the
  host's own "save state" model implies. A size is therefore only remembered if
  the host saves the state — with REAPER, on project save. Sizes are logical
  pixels, so the layout is the same on every monitor.
- Sizes are clamped on both write and read, so a mode can never be restored
  below its minimum even if a state was written by a build with different
  limits.
- Changing the preferred sizes later only affects modes that were never resized
  (or where the user pressed **Reset window size**).

## Alternatives considered

- **Let the host remember it**: not available — a VST3 host has no view size of
  its own to restore; it asks the fresh editor what size it wants.
- **A global settings file (`juce::PropertiesFile`)**: would survive even
  unsaved projects, but adds file I/O and a second source of truth (project vs
  machine) to a plug-in that currently touches no files. Rejected for v1.4.
- **One remembered size for all modes**: simplest, but a tall column and a short
  strip cannot share a sensible size, and the mode switch already resets the
  window.
- **Aspect-ratio lock per mode**: restricts the user (and the host) more than
  the minimum-size limits already do, without solving the persistence problem.
