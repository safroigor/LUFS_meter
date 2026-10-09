<#
.SYNOPSIS
    Reproducible Windows x64 build helper for the LUFS Meter VST3 plugin.

.DESCRIPTION
    Locates the Visual Studio 2022 C++ toolchain (vcvars64.bat), imports the
    MSVC environment, and then runs CMake configure/build/test against the
    presets defined in CMakePresets.json.

    Ninja requires the MSVC environment variables (INCLUDE, LIB, PATH), which
    is why this wrapper exists instead of calling cmake directly.

.EXAMPLE
    pwsh -File scripts/build.ps1 -Preset release
    pwsh -File scripts/build.ps1 -Preset debug -Test
#>
#requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release')]
    [string] $Preset = 'release',

    [switch] $Test
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Resolve-Vcvars64 {
    $candidates = [System.Collections.Generic.List[string]]::new()

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $installs = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        foreach ($install in $installs) {
            if ($install) {
                $candidates.Add((Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'))
            }
        }
    }

    $candidates.Add('C:\Tools\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat')
    $candidates.Add('D:\Tools\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat')
    $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
    $candidates.Add('C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat')

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }

    throw "vcvars64.bat not found. Install Visual Studio 2022 Build Tools with the C++ workload."
}

$vcvars = Resolve-Vcvars64
Write-Host "Using MSVC environment: $vcvars"
Write-Host "Preset: $Preset"

$commands = @(
    "call `"$vcvars`" >nul",
    "cmake --preset $Preset",
    "cmake --build --preset $Preset"
)

if ($Test) {
    $commands += "ctest --preset $Preset"
}

$commandLine = $commands -join ' && '
& cmd.exe /c $commandLine
exit $LASTEXITCODE
