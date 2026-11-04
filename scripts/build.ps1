# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

<#
.SYNOPSIS
Configures and builds core on Windows with MSVC (Visual Studio 2022) and
Ninja. The CI workflows and the downstream repos build core through this
script, so they get the same binaries. Linux, macOS and Android: build.sh.

.DESCRIPTION
Run it from a Developer PowerShell, or let it load the MSVC environment of
Visual Studio 2022 itself. Needs VCPKG_ROOT. CORE_PYTHON sets the Python for
the third-party builds (default: the one CMake finds). -Ccache compiles through
ccache; otherwise sccache is used when it is on PATH.

Output, a core package (see cmake\prebuilt-core.cmake): the libraries and
x2t in <OutputDir>\bin, the tools in <OutputDir>\tools, the static and
import libraries in <OutputDir>\lib. Arguments after the named ones go to
cmake.

.EXAMPLE
scripts\build.ps1 -Arch arm64 -BuildDir build
#>
# Not positional: arguments without a name go to cmake
[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet('x64', 'arm64')]
    [string]$Arch = $(if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { 'arm64' } else { 'x64' }),
    [string]$BuildDir = 'build',
    [string]$OutputDir = '',
    [switch]$Tests,
    [switch]$Ccache,
    [string]$SdkVersion = '10.0.19041.0',
    [Parameter(ValueFromRemainingArguments)]
    [string[]]$CMakeArgs = @()
)

$ErrorActionPreference = 'Stop'
$Core = Split-Path -Parent $PSScriptRoot

function Assert-LastExit([string]$What) {
    if ($LASTEXITCODE -ne 0) { throw "$What failed with exit code $LASTEXITCODE" }
}

. "$PSScriptRoot\msvc-env.ps1"

if (-not $env:VCPKG_ROOT) { throw 'VCPKG_ROOT must point at vcpkg' }
if (-not $env:VCINSTALLDIR) { Import-VcVars -Arch $Arch -SdkVersion $SdkVersion }

$BuildDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDir)
if (-not $OutputDir) { $OutputDir = Join-Path $BuildDir 'package' }
$OutputDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDir)

$configureArgs = @(
    '-G', 'Ninja', '-S', $Core, '-B', $BuildDir,
    '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake",
    '-DVCPKG_MANIFEST_MODE=ON',
    "-DVCPKG_MANIFEST_DIR=$Core",
    "-DEO_CORE_OUTPUT_DIR=$OutputDir\bin",
    "-DEO_CORE_TOOLS_DIR=$OutputDir\tools",
    "-DEO_CORE_LIB_DIR=$OutputDir\lib",
    "-DEO_BUILD_TESTS=$(if ($Tests) { 'ON' } else { 'OFF' })"
)
if ($Tests) { $configureArgs += '-DVCPKG_MANIFEST_FEATURES=tests' }
if ($env:CORE_PYTHON) { $configureArgs += "-DPYTHON_BIN=$env:CORE_PYTHON" }
if ($Ccache) {
    $configureArgs += @(
        '-DCMAKE_C_COMPILER_LAUNCHER=ccache',
        '-DCMAKE_CXX_COMPILER_LAUNCHER=ccache'
    )
    # As build.sh, for the targets with precompiled headers. sccache can't cache
    # those (/Yc, /Yu), and they can't be turned off: msvc_fix.h is one of them.
    $env:CCACHE_SLOPPINESS = 'pch_defines,time_macros,include_file_mtime,include_file_ctime'
    # Some sources are not UTF-8, ccache would not cache them
    $env:CCACHE_MSVC_UTF8 = 'false'
}
# sccache only caches with debug info embedded in the objects (/Z7)
elseif (Get-Command sccache -ErrorAction SilentlyContinue) {
    $configureArgs += @(
        '-DCMAKE_C_COMPILER_LAUNCHER=sccache',
        '-DCMAKE_CXX_COMPILER_LAUNCHER=sccache',
        '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded'
    )
}

cmake @configureArgs @CMakeArgs
Assert-LastExit 'CMake configure'
cmake --build $BuildDir --parallel
Assert-LastExit 'CMake build'
