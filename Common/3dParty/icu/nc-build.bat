@echo off
setlocal

REM ---- defaults ----
set "CYGWIN_BIN=%~1"
if "%CYGWIN_BIN%"=="" if defined CYGWIN_ROOT set "CYGWIN_BIN=%CYGWIN_ROOT%\bin"
if "%CYGWIN_BIN%"=="" set "CYGWIN_BIN=C:\cygwin64\bin"

set "VCVARS=%~2"
if not "%VCVARS%"=="" goto :vcvars_done

REM No vcvars given: locate the newest Visual Studio (any edition:
REM Community/Professional/Enterprise/BuildTools) via vswhere, which
REM ships with every VS 2017+ installation at a fixed location.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsx86_amd64.bat"
)
REM Last-resort fallback for machines without vswhere
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsx86_amd64.bat"
:vcvars_done

REM ---- required args ----
set "ICU_SOURCE=%~3"
set "ICU_INSTALL=%~4"

if "%ICU_SOURCE%"=="" goto :usage
if "%ICU_INSTALL%"=="" goto :usage

REM ---- fail early with explicit errors instead of mysterious error 3 ----
if not exist "%CYGWIN_BIN%\bash.exe" (
  echo ERROR: Cygwin bash not found at "%CYGWIN_BIN%\bash.exe" 1>&2
  exit /b 1
)
if not exist "%VCVARS%" (
  echo ERROR: vcvars script not found at "%VCVARS%" 1>&2
  exit /b 1
)
echo Using Cygwin: %CYGWIN_BIN%
echo Using vcvars: %VCVARS%

REM ---- PATH ORDER: MSVC first, Cygwin second ----
set "PATH=%CYGWIN_BIN%;%PATH%"
call "%VCVARS%" || exit /b 1

echo "------------- DBG 1"

REM ---- run build ----
cd /D "%ICU_SOURCE%" || exit /b 1

echo "------------- DBG 2"

for /f "delims=" %%i in ('cygpath "%~dp0"') do set SCRIPT_DIR=%%i

for /f "delims=" %%i in ('cygpath "%ICU_INSTALL%"') do set INSTALL_DIR=%%i

echo "Script dir : %SCRIPT_DIR%"
echo "Install dir: %INSTALL_DIR%"

"%CYGWIN_BIN%\bash.exe" "%SCRIPT_DIR%nc-build-cygwin.sh" "%INSTALL_DIR%"

exit /b %errorlevel%

:usage
echo.
echo Usage:
echo   %~nx0 [cygwin_bin] [vcvars_bat] ^<icu_source^> ^<icu_install^>
echo.
echo Arguments:
echo   1. cygwin_bin     Optional. Default: %%CYGWIN_ROOT%%\bin, else C:\cygwin64\bin
echo   2. vcvars_bat     Optional. Default: located via vswhere (latest VS, any edition)
echo   3. icu_source     Required. Path of the ICU sources dir, e.g.
echo                      C:\Users\superdev\Repos\icu\source
echo   4. icu_install    Required. Path of the ICU output/install dir, e.g.
echo                      C:\Users\superdev\icu_install
echo.
exit /b 2