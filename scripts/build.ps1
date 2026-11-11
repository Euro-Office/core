# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

<#
.SYNOPSIS
Configures and builds core on Windows with MSVC (Visual Studio 2022) and
Ninja. The CI workflows and the downstream repos build core through this
script, so they get the same binaries. Linux, macOS and Android: build.sh.

.DESCRIPTION
Run it from a Developer PowerShell, or let it load the MSVC environment of
Visual Studio 2022 itself (or, with -VcToolsVersion, of the newest Visual
Studio that has that toolset). Needs VCPKG_ROOT, except with -ThirdPartyOnly.
CORE_PYTHON sets the Python for the third-party builds (default: the one CMake
finds). -Ccache compiles through ccache, which it installs
(install-ccache.ps1); otherwise sccache is used when it is on PATH.
-ThirdPartyOnly only builds the third-party libraries (and uploads the ones
missing from the remote cache, given its credentials), not core; add
-DBUILD_DESKTOP=ON for the desktop app's (Qt, CEF).

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
    [switch]$ThirdPartyOnly,
    [string]$SdkVersion = '10.0.19041.0',
    [string]$VcToolsVersion = '',   # e.g. 14.44; empty = newest in Visual Studio 2022
    [Parameter(ValueFromRemainingArguments)]
    [string[]]$CMakeArgs = @()
)

$ErrorActionPreference = 'Stop'
$Core = Split-Path -Parent $PSScriptRoot

function Assert-LastExit([string]$What) {
    if ($LASTEXITCODE -ne 0) { throw "$What failed with exit code $LASTEXITCODE" }
}

. "$PSScriptRoot\msvc-env.ps1"

# The third-party libraries don't use vcpkg's
if (-not $env:VCPKG_ROOT -and -not $ThirdPartyOnly) { throw 'VCPKG_ROOT must point at vcpkg' }
if (-not $env:VCINSTALLDIR) { Import-VcVars -Arch $Arch -SdkVersion $SdkVersion -VcToolsVersion $VcToolsVersion }

$BuildDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDir)
if (-not $OutputDir) { $OutputDir = Join-Path $BuildDir 'package' }
$OutputDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDir)

$configureArgs = @(
    '-G', 'Ninja', '-S', $Core, '-B', $BuildDir,
    '-DCMAKE_BUILD_TYPE=Release',
    "-DEO_CORE_OUTPUT_DIR=$OutputDir\bin",
    "-DEO_CORE_TOOLS_DIR=$OutputDir\tools",
    "-DEO_CORE_LIB_DIR=$OutputDir\lib",
    "-DEO_BUILD_TESTS=$(if ($Tests) { 'ON' } else { 'OFF' })"
)
if (-not $ThirdPartyOnly) {
    $configureArgs += @(
        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake",
        '-DVCPKG_MANIFEST_MODE=ON',
        "-DVCPKG_MANIFEST_DIR=$Core"
    )
}
if ($Tests) { $configureArgs += '-DVCPKG_MANIFEST_FEATURES=tests' }
if ($env:CORE_PYTHON) { $configureArgs += "-DPYTHON_BIN=$env:CORE_PYTHON" }
if ($ThirdPartyOnly) { $configureArgs += '-DEO_THIRD_PARTY_ONLY=ON' }
if ($Ccache) {
    & "$PSScriptRoot\install-ccache.ps1"
    ccache --zero-stats
    Assert-LastExit 'ccache'
    $configureArgs += @(
        '-DCMAKE_C_COMPILER_LAUNCHER=ccache',
        '-DCMAKE_CXX_COMPILER_LAUNCHER=ccache'
    )
    # As build.sh, for the targets with precompiled headers. sccache can't cache
    # those (/Yc, /Yu), and they can't be turned off: msvc_fix.h is one of them.
    $env:CCACHE_SLOPPINESS = 'pch_defines,time_macros,include_file_mtime,include_file_ctime'
    # Some sources are not UTF-8, ccache would not cache them
    $env:CCACHE_NOMSVC_UTF8 = 'true'
    # A new runner installs its compiler again: new file times
    $env:CCACHE_COMPILERCHECK = 'content'
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
# Configuring built the third-party libraries
if ($ThirdPartyOnly) { return }
cmake --build $BuildDir --parallel
Assert-LastExit 'CMake build'
