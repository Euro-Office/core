# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

<#
.SYNOPSIS
Puts ccache on PATH for build.ps1 -Ccache, and for the later steps of a GitHub
Actions job. Caching MSVC needs a newer version than the package managers
have, and arm64 a native build, so it comes from ccache's releases.
#>
$ErrorActionPreference = 'Stop'

$version = '4.14.2'
if ($env:PROCESSOR_ARCHITECTURE -eq 'ARM64') {
    $name = "ccache-$version-windows-aarch64"
    $sha256 = '47ea5a0ed661bea53962d8fb8ee9a1769092d979a6f7c73d3081499b3305b2ab'
} else {
    $name = "ccache-$version-windows-x86_64"
    $sha256 = '6993ce645e53192cec592ee23186200b574ac4c59097ecde8d9cfd681b47ae84'
}

$temp = [IO.Path]::GetTempPath()
$dir = Join-Path $temp $name
if (-not (Test-Path (Join-Path $dir 'ccache.exe'))) {
    $zip = Join-Path $temp "$name.zip"
    Invoke-WebRequest "https://github.com/ccache/ccache/releases/download/v$version/$name.zip" -OutFile $zip
    if ((Get-FileHash $zip -Algorithm SHA256).Hash -ne $sha256) { throw "$name.zip: wrong checksum" }
    Expand-Archive $zip -DestinationPath $temp -Force
}

$env:PATH = "$dir;$env:PATH"
if ($env:GITHUB_PATH) { $dir | Out-File -FilePath $env:GITHUB_PATH -Encoding utf8 -Append }
