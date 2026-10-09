# LUFS Meter

A minimal Windows x64 VST3 plugin that measures **Integrated Loudness** according
to ITU-R BS.1770 and EBU R128, and displays the value together with a compact
bar scale.

The plugin is a **measurement device**. It never modifies the incoming audio
signal.

> **Status: development.** Stages 1–3 are implemented: reproducible VST3 build,
> measurement engine with passing unit tests, and the meter UI (horizontal /
> vertical, slim / compact, reset, session freeze/restart). See
> [docs/development.md](docs/development.md) and [docs/testing.md](docs/testing.md)
> for the exact state.

## Features (target)

- Integrated Loudness (LUFS), rounded to 0.1 LU.
- Slim horizontal / vertical bar scale.
- Horizontal / vertical orientation switch.
- Reset button.
- Slim / compact display modes.
- Optional LUFS label and scale marks.
- Explicit "no valid value yet" state (never a fake `0 LUFS`).
- Non-blocking UI updates.

Out of scope for v1: True Peak, Momentary/Short-term LUFS, dynamics processing,
loudness history graphs, target levels, extra DSP.

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
