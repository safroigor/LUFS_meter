# Development

This document describes how to prepare the environment and produce a
reproducible Windows x64 build of the LUFS Meter VST3 plugin.

## Verified environment (as of Stage 1)

| Tool | Version | Notes |
|---|---|---|
| OS | Windows 11 `10.0.26100`, AMD64 | native Windows, not WSL |
| Git | 2.55.0.windows.2 | submodules used |
| CMake | 4.4.4 | |
| Ninja | 1.13.2 | |
| MSVC | Visual Studio 2022 Build Tools 17.14.41 (v143) | installed to `D:\Tools\VSBuildTools` |
| Windows SDK | installed with the VCTools workload | |
| Host | REAPER x64 | `C:\Program Files\REAPER (x64)` |

## Dependencies

Both dependencies are git submodules pinned to exact tags:

| Dependency | Tag | Commit | License |
|---|---|---|---|
| JUCE | `8.0.15` | `91ad83ae34a81e0833b1a2b0866f54846370ae53` | AGPLv3 / commercial |
| libebur128 | `v1.2.6` | `67b33abe1558160ed76ada1322329b0e9e058b02` | MIT |

libebur128 is compiled directly from `third_party/libebur128/ebur128/ebur128.c`
rather than through its own CMake project, because that project declares
`cmake_minimum_required(VERSION 2.8.12)`, which modern CMake (4.x) rejects. The
bundled `ebur128/queue/sys/queue.h` provides `<sys/queue.h>` on MSVC.

## Installing the toolchain

```powershell
winget install --id Kitware.CMake -e
winget install --id Ninja-build.Ninja -e
winget install --id Microsoft.VisualStudio.2022.BuildTools -e `
  --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --installPath D:\Tools\VSBuildTools"
```

## Configure, build, test

Ninja needs the MSVC environment, so use the wrapper script. It locates
`vcvars64.bat`, imports the MSVC environment, then drives the CMake presets.

```powershell
# Configure + build (Release)
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset release

# Configure + build (Debug)
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset debug

# Configure + build + run tests
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset release -Test
```

Equivalent manual steps from an *x64 Native Tools Command Prompt for VS 2022*:

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release
```

## Build artefacts

The JUCE VST3 bundle is written to:

```
build/<preset>/LUFSMeter_artefacts/<Config>/VST3/LUFS Meter.vst3
```

`<Config>` is `Release` or `Debug` depending on the preset.

## Installing into REAPER

REAPER's configured VST paths (`vstpath64` in `%APPDATA%\REAPER\REAPER.ini`)
already include:

```
%COMMONPROGRAMFILES%\VST3          -> C:\Program Files\Common Files\VST3
%LOCALAPPDATA%\Programs\Common\VST3 -> C:\Users\<user>\AppData\Local\Programs\Common\VST3
```

The user-local location requires no administrator rights and is already scanned,
so it is the recommended deployment target for testing:

```powershell
$dest = "$env:LOCALAPPDATA\Programs\Common\VST3"
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item -Recurse -Force "build\release\LUFSMeter_artefacts\Release\VST3\LUFS Meter.vst3" $dest
```

Then in REAPER: *Options > Preferences > Plug-ins > VST* and *Re-scan*.

The concrete path and successful load are recorded in `docs/testing.md` once
verified.

## Release

Current release: **v1.0.0** (VST3, Windows x64). Plugin identity (manufacturer
code, plugin code, bundle identifier, version) is listed in the
[README](../README.md). The verified environment and test results are in
[docs/testing.md](testing.md).

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Preset release -Test
```

## Notes / limitations

- The plugin currently has no installer; deployment is a manual copy of the
  bundle.
- No code signing is performed.
