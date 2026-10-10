# LUFS Meter

A minimal Windows x64 VST3 plugin that measures **Integrated**, **Momentary** and
**Short-Term Loudness** and **True Peak** according to ITU-R BS.1770 and
EBU R 128, and displays them together with a compact bar scale.

The plugin is a **measurement device**. It never modifies the incoming audio
signal.

> **Status: v1.3.0.** Adds three mutually exclusive display modes — Vertical,
> Horizontal and Compact (a nearly square digital meter with a continuous
> background fill) — plus an About/info popup. See
> [docs/testing.md](docs/testing.md) for the exact verified state.

## Features

- Integrated Loudness (LUFS-I), rounded to 0.1 LU, with a large focal reading
  coloured by the active zone.
- True Peak (TP), Short-Term (LS) and Momentary (LM) numeric readouts,
  subordinate to the Integrated value: a row above the scale in horizontal
  orientation, and stacked two-line groups to the left of the scale in vertical
  orientation.
- Segmented horizontal / vertical scale with configurable green and red zones.
- Three mutually exclusive display modes, chosen in one selector: **Vertical**
  and **Horizontal** (the existing segmented meter) and **Compact**, a nearly
  square digital meter whose background is one continuous fill that rises and
  falls with the Integrated value using the configured zone colours.
- About / information popup with the plugin metadata, the measurements and the
  third-party attributions.
- Transport-driven measurement pass: silence during playback never clears the
  value; play-after-stop, seek and loop start a new pass.
- Reset button in the main interface.
- Optional `LUFS-I` label and scale marks.
- Explicit "no valid value yet" state (never a fake `0 LUFS` / `0 dBTP`).
- Non-blocking UI updates.

Out of scope: Loudness Range (LRA), dynamics processing, loudness history
graphs, target levels, extra DSP.

## Plugin identity

| Field | Value |
|---|---|
| Manufacturer | SafroAudio |
| Manufacturer code | `Safr` |
| Plugin code | `Lufs` |
| Bundle identifier | `com.SafroAudio.LUFSMeter` |
| Product version | 1.3.0 |
| Format | VST3, Windows x64 |

## Requirements

- Windows 10/11 x64.
- Visual Studio 2022 Build Tools with the **Desktop development with C++**
  workload (MSVC v143 + Windows SDK).
- CMake >= 3.25, Ninja.
- Git (with submodules).

## Dependencies (pinned)

| Dependency | Version | License | How |
|---|---|---|---|
| [JUCE](https://github.com/juce-framework/JUCE) | 8.0.15 | AGPLv3 / commercial | git submodule `third_party/JUCE` |
| [libebur128](https://github.com/jiixyj/libebur128) | v1.2.6 | MIT | git submodule `third_party/libebur128` |

Exact commits are recorded by the git submodules.

## Getting the sources

```powershell
git clone --recurse-submodules https://github.com/safroigor/LUFS_meter.git
cd LUFS_meter
```

If you already cloned without submodules:

```powershell
git submodule update --init --recursive
```

## Building

The build wrapper imports the MSVC environment (required by Ninja) and then runs
CMake configure + build:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset release
```

Debug build:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset debug
```

Run the tests:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset release -Test
```

Build artefacts:

```
build/release/LUFSMeter_artefacts/Release/VST3/LUFS Meter.vst3
```

## Installing for a host (REAPER)

Copy the produced `LUFS Meter.vst3` bundle to a VST3 folder that your host scans,
for example:

```powershell
Copy-Item -Recurse -Force "build\release\LUFSMeter_artefacts\Release\VST3\LUFS Meter.vst3" `
    "$env:APPDATA\REAPER\..."
```

The exact host scan path is documented and verified in
[docs/development.md](docs/development.md).

## License

The project is released under the **GNU Affero General Public License v3.0**
(see `LICENSE`), which is compatible with the JUCE AGPLv3 option and with
libebur128's MIT license.

## Documentation

- [docs/development.md](docs/development.md) — environment, build, install.
- [docs/architecture.md](docs/architecture.md) — components, data flow, sync.
- [docs/measurement.md](docs/measurement.md) — standard, algorithm, limits.
- [docs/testing.md](docs/testing.md) — test matrix, tolerances, results.
- [docs/decisions/](docs/decisions/) — Architecture Decision Records.
