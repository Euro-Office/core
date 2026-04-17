@REM @echo off
@REM set PATH=C:\cygwin64\bin;%PATH%
@REM call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsx86_amd64.bat"

@REM cd /D C:\Users\adaor\Repos\eo-core\build\icu\work\icu\source

@REM bash /cygdrive/c/Users/adaor/Repos/eo-core/core/Common/3dParty/icu/nc-cygwin.sh 


@echo off
setlocal

REM ---- defaults ----
set "CYGWIN_BIN=%~1"
if "%CYGWIN_BIN%"=="" set "CYGWIN_BIN=C:\cygwin64\bin"

set "VCVARS=%~2"
if "%VCVARS%"=="" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsx86_amd64.bat"

REM ---- required args ----
set "ICU_SOURCE=%~3"
set "ICU_INSTALL=%~4"

if "%ICU_SOURCE%"=="" goto :usage
if "%ICU_INSTALL%"=="" goto :usage

REM ---- PATH ORDER: MSVC first, Cygwin second ----
set "PATH=%CYGWIN_BIN%;%PATH%"
call "%VCVARS%" || exit /b 1

echo "------------- DBG 1"

REM ---- run build ----
cd /D "%ICU_SOURCE%" || exit /b 1

echo "------------- DBG 2"

for /f %%i in ('cygpath "%~dp0"') do set SCRIPT_DIR=%%i

for /f %%i in ('cygpath "%ICU_INSTALL%"') do set INSTALL_DIR=%%i

echo "Script dir : %SCRIPT_DIR%"
echo "Install dir: %INSTALL_DIR%"
@REM exit 0

%CYGWIN_BIN%\bash.exe "%SCRIPT_DIR%nc-build-cygwin.sh" "%INSTALL_DIR%"

exit /b %errorlevel%

:usage
echo.
echo Usage:
echo   %~nx0 [cygwin_bin] [vcvars_bat] ^<icu_source^> ^<icu_install^>
echo.
echo Arguments:
echo   1. cygwin_bin     Optional. Default: C:\cygwin64\bin
echo   2. vcvars_bat     Optional. Default: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsx86_amd64.bat
echo   3. icu_source     Required. Path of the ICU sources dir, e.g.
echo                      C:\Users\superdev\Repos\icu\source
echo   4. icu_install    Required. Path of the ICU output/install dir, e.g.
echo                      C:\Users\superdev\icu_install
echo.
echo Example:
echo   %~nx0 "" "" ^
echo     C:\Users\superdev\Repos\icu\source ^
echo     C:\Users\superdev\icu_install
echo.
exit /b 2
