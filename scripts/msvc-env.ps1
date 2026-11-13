# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Find-VisualStudio: the Visual Studio to build with. Import-VcVars: loads its
# MSVC environment (vcvars<arch>.bat) into the current process. Dot-source
# this file.

# With a toolset version (e.g. 14.44, the v143 toolset of Visual Studio 2022),
# the newest Visual Studio that has it: Visual Studio 2026 can install the
# v143 toolset, and GitHub's arm64 image has only Visual Studio 2026.
# Without one, Visual Studio 2022.
function Find-VisualStudio {
    param([string]$VcToolsVersion = '')
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if ($VcToolsVersion) {
        $vs = & $vsWhere -products * -version '[17.0,)' -sort -property installationPath |
            Where-Object { Test-Path (Join-Path $_ "VC\Tools\MSVC\$VcToolsVersion.*") } |
            Select-Object -First 1
        if (-not $vs) { throw "No Visual Studio has the MSVC $VcToolsVersion toolset" }
    } else {
        $vs = & $vsWhere -products * -version '[17.0,18.0)' -latest -property installationPath
        if (-not $vs) { throw 'Visual Studio 2022 not found' }
    }
    $vs
}

function Import-VcVars {
    param(
        [ValidateSet('x64', 'arm64')][string]$Arch,
        [string]$SdkVersion = '10.0.22621.0',
        [string]$VcToolsVersion = ''   # e.g. 14.44; empty = newest in Visual Studio 2022
    )
    $vs = Find-VisualStudio -VcToolsVersion $VcToolsVersion
    $bat = Join-Path $vs "VC\Auxiliary\Build\vcvars$(if ($Arch -eq 'arm64') { 'arm64' } else { '64' }).bat"
    $verArg = if ($VcToolsVersion) { "-vcvars_ver=$VcToolsVersion" } else { '' }
    $out = & cmd /c "`"$bat`" $SdkVersion $verArg 2>&1 && echo __VCVARS_OK__ && set"
    $marker = [Array]::IndexOf([string[]]($out | ForEach-Object { $_.Trim() }), '__VCVARS_OK__')
    if ($marker -lt 0) { $out | Write-Host; throw "$bat failed" }
    foreach ($line in $out[($marker + 1)..($out.Count - 1)]) {
        $i = $line.IndexOf('=')
        if ($i -gt 0) { [Environment]::SetEnvironmentVariable($line.Substring(0, $i), $line.Substring($i + 1), 'Process') }
    }
    if (-not $env:VCINSTALLDIR) { throw "$bat did not set up MSVC (VCINSTALLDIR is empty)" }
    Write-Host "MSVC environment: $bat $SdkVersion (tools $env:VCToolsVersion)"
}
