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
the third-party builds (default: the one CMake finds). Uses sccache when it
is on PATH.

Output: the libraries and x2t in <OutputDir>\bin, the tools in
<OutputDir>\tools. Arguments after the named ones go to cmake.

.EXAMPLE
scripts\build.ps1 -Arch arm64 -BuildDir build
#>
param(
    [ValidateSet('x64', 'arm64')]
    [string]$Arch = $(if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') { 'arm64' } else { 'x64' }),
    [string]$BuildDir = 'build',
    [string]$OutputDir = '',
    [switch]$Tests,
    [string]$SdkVersion = '10.0.19041.0',
    [Parameter(ValueFromRemainingArguments)]
    [string[]]$CMakeArgs = @()
)

$ErrorActionPreference = 'Stop'
$Core = Split-Path -Parent $PSScriptRoot

function Assert-LastExit([string]$What) {
    if ($LASTEXITCODE -ne 0) { throw "$What failed with exit code $LASTEXITCODE" }
}

# The environment of vcvars<arch>.bat, imported into this process
function Import-VcVars {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = & $vsWhere -products * -version '[17.0,18.0)' -latest -property installationPath
    if (-not $vs) { throw 'Visual Studio 2022 not found' }
    $bat = Join-Path $vs "VC\Auxiliary\Build\vcvars$(if ($Arch -eq 'arm64') { 'arm64' } else { '64' }).bat"
    $out = & cmd /c "`"$bat`" $SdkVersion 2>&1 && echo __VCVARS_OK__ && set"
    $marker = [Array]::IndexOf([string[]]($out | ForEach-Object { $_.Trim() }), '__VCVARS_OK__')
    if ($marker -lt 0) { $out | Write-Host; throw "$bat failed" }
    foreach ($line in $out[($marker + 1)..($out.Count - 1)]) {
        $i = $line.IndexOf('=')
        if ($i -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0, $i), $line.Substring($i + 1), 'Process') }
    }
    Write-Host "MSVC environment: $bat $SdkVersion (tools $env:VCToolsVersion)"
}

if (-not $env:VCPKG_ROOT) { throw 'VCPKG_ROOT must point at vcpkg' }
if (-not $env:VCINSTALLDIR) { Import-VcVars }

$BuildDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildDir)
if (-not $OutputDir) { $OutputDir = Join-Path $BuildDir 'package' }
$OutputDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDir)

$cmakeArgs = @(
    '-G', 'Ninja', '-S', $Core, '-B', $BuildDir,
    '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake",
    '-DVCPKG_MANIFEST_MODE=ON',
    "-DVCPKG_MANIFEST_DIR=$Core",
    "-DEO_CORE_OUTPUT_DIR=$OutputDir\bin",
    "-DEO_CORE_TOOLS_DIR=$OutputDir\tools",
    "-DEO_BUILD_TESTS=$(if ($Tests) { 'ON' } else { 'OFF' })"
)
if ($Tests) { $cmakeArgs += '-DVCPKG_MANIFEST_FEATURES=tests' }
if ($env:CORE_PYTHON) { $cmakeArgs += "-DPYTHON_BIN=$env:CORE_PYTHON" }
# sccache only caches with debug info embedded in the objects (/Z7)
if (Get-Command sccache -ErrorAction SilentlyContinue) {
    $cmakeArgs += @(
        '-DCMAKE_C_COMPILER_LAUNCHER=sccache',
        '-DCMAKE_CXX_COMPILER_LAUNCHER=sccache',
        '-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded'
    )
}

cmake @cmakeArgs @CMakeArgs
Assert-LastExit 'CMake configure'
cmake --build $BuildDir --parallel
Assert-LastExit 'CMake build'
