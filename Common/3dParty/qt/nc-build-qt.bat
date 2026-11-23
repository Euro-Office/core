@echo off
setlocal

rem Usage: nc-build-qt.bat <qt-source-dir> [configure flags...]
rem Sets up the MSVC x64 environment, then runs configure.bat, builds
rem with jom (if available) or nmake, and installs.

set "QT_SRC=%~1"
shift

if "%QT_SRC%"=="" (
    echo ERROR: Qt source directory not given
    exit /b 1
)

rem --- Toolset selection ---
rem Qt 5.9 does not compile with the native VS2019/VS2022 toolsets (v142/v143).
rem It supports v141 (VS2017, 14.1x) and v140 (VS2015, 14.0), both of which
rem can be installed as components inside a newer VS via the installer:
rem   "MSVC v141 - VS 2017 C++ x64/x86 build tools"
rem   "MSVC v140 - VS 2015 C++ build tools (v14.00)"
rem Set QT_VCVARS_VER explicitly to skip auto-detection.

rem --- Locate Visual Studio via vswhere ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found, is Visual Studio installed?
    exit /b 1
)

set "VSINSTALL="

if not "%QT_VCVARS_VER%"=="" (
    rem Explicit toolset requested: just find the latest VS with C++ tools
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
    goto found_vs
)

rem Prefer v141, fall back to v140
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.v141.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not "%VSINSTALL%"=="" (
    set "QT_VCVARS_VER=14.16"
    goto found_vs
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.140 -property installationPath`) do set "VSINSTALL=%%i"
if not "%VSINSTALL%"=="" (
    set "QT_VCVARS_VER=14.0"
    goto found_vs
)

:found_vs
if "%VSINSTALL%"=="" (
    echo ERROR: No Visual Studio installation with a Qt 5.9 compatible toolset found.
    echo Install "MSVC v141 - VS 2017 C++ x64/x86 build tools" or
    echo "MSVC v140 - VS 2015 C++ build tools" via the VS installer.
    exit /b 1
)

echo Using Visual Studio at %VSINSTALL% with toolset %QT_VCVARS_VER%

rem --- Neutralize any inherited VS environment ---
rem The orchestrator may have already activated a different VS/toolset
rem in the parent process. With VSCMD_VER set, VsDevCmd does an
rem "incremental" init that mixes the two installs and fails to resolve
rem the toolset. Clear the activation state to force a clean init.
if defined VSCMD_VER (
    echo Clearing inherited Visual Studio environment for clean init...
    set "VSCMD_VER="
    set "VSINSTALLDIR="
    set "VCINSTALLDIR="
    set "VCIDEInstallDir="
    set "VCToolsInstallDir="
    set "VCToolsRedistDir="
    set "VCToolsVersion="
    set "VisualStudioVersion="
    set "DevEnvDir="
    set "WindowsSdkDir="
    set "WindowsSdkBinPath="
    set "WindowsSdkVerBinPath="
    set "WindowsSDKLibVersion="
    set "WindowsSDKVersion="
    set "UCRTVersion="
    set "UniversalCRTSdkDir="
    set "ExtensionSdkDir="
    set "Platform="
    set "INCLUDE="
    set "LIB="
    set "LIBPATH="
)

if "%QT_VCVARS_VER%"=="14.0" goto setup_v140

call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x64 -vcvars_ver=%QT_VCVARS_VER%
if errorlevel 1 (
    echo ERROR: vcvarsall.bat failed for toolset %QT_VCVARS_VER%
    exit /b 1
)
goto env_ready

:setup_v140
rem The modern vcvarsall delegates -vcvars_ver=14.0 through the
rem VS140COMNTOOLS env var, which is often not set. Call the VS2015
rem vcvarsall directly instead. Note: the 2015 script uses 'amd64'.
set "VC140_VCVARSALL=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
if defined VS140COMNTOOLS set "VC140_VCVARSALL=%VS140COMNTOOLS%..\..\VC\vcvarsall.bat"

if not exist "%VC140_VCVARSALL%" (
    echo ERROR: v140 vcvarsall.bat not found at "%VC140_VCVARSALL%"
    echo Is the "MSVC v140 - VS 2015 C++ build tools" component fully installed?
    exit /b 1
)

call "%VC140_VCVARSALL%" amd64
if errorlevel 1 (
    echo ERROR: v140 vcvarsall.bat failed
    exit /b 1
)

rem --- Optional: pin an older Windows SDK / UCRT version ---
rem Newer SDKs (e.g. 10.0.26100) use compiler intrinsics that the v140
rem compiler does not have, breaking ALL compilation. Install an older
rem SDK (e.g. 10.0.19041.0) and set QT_WINSDK_VER to select it.
if not defined QT_WINSDK_VER goto env_ready
if not defined UCRTVersion (
    echo ERROR: UCRTVersion not set by vcvarsall, cannot pin SDK version
    exit /b 1
)

if not exist "%UniversalCRTSdkDir%Include\%QT_WINSDK_VER%\ucrt" (
    echo ERROR: requested SDK %QT_WINSDK_VER% not found under "%UniversalCRTSdkDir%Include"
    echo Installed SDK versions:
    dir /b "%UniversalCRTSdkDir%Include"
    exit /b 1
)

echo Pinning Windows SDK/UCRT version %QT_WINSDK_VER% [was %UCRTVersion%]
call set "INCLUDE=%%INCLUDE:%UCRTVersion%=%QT_WINSDK_VER%%%"
call set "LIB=%%LIB:%UCRTVersion%=%QT_WINSDK_VER%%%"
call set "LIBPATH=%%LIBPATH:%UCRTVersion%=%QT_WINSDK_VER%%%"
call set "PATH=%%PATH:%UCRTVersion%=%QT_WINSDK_VER%%%"
set "WindowsSDKVersion=%QT_WINSDK_VER%\"
set "UCRTVersion=%QT_WINSDK_VER%"

:env_ready
where nmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: nmake not on PATH after environment setup
    exit /b 1
)

cd /d "%QT_SRC%"

rem --- Collect remaining args as configure flags ---
set "CFG_FLAGS="
:collect
if "%~1"=="" goto run
set "CFG_FLAGS=%CFG_FLAGS% %1"
shift
goto collect

:run
rem If the tree is already configured, resume the build instead of
rem reconfiguring (Qt requires a clean tree for reconfiguration; use
rem force-redo on the dep to reconfigure from scratch).
if exist "%QT_SRC%\Makefile" (
    echo Existing configuration detected, resuming build
    goto build
)

echo Running: configure.bat %CFG_FLAGS%
call configure.bat %CFG_FLAGS%
if errorlevel 1 (
    echo ERROR: configure failed
    exit /b 1
)

:build

rem --- Build: prefer jom (parallel) over nmake ---
rem Set QT_JOM to the full path of jom.exe if it is not on PATH.
if defined QT_JOM (
    if exist "%QT_JOM%" (
        "%QT_JOM%"
        goto build_done
    )
    echo WARNING: QT_JOM set but not found: "%QT_JOM%", falling back
)
where jom >nul 2>nul
if %errorlevel%==0 (
    jom
) else (
    echo WARNING: jom not found, building single-threaded with nmake
    nmake
)
:build_done
if errorlevel 1 (
    echo ERROR: build failed
    exit /b 1
)

nmake install
if errorlevel 1 (
    echo ERROR: install failed
    exit /b 1
)

exit /b 0