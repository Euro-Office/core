@echo off
::  Helper script to build BDB products for binary release
::  Assumes current directory is <product>/dist
::  Current version of Visual Studio for binaries is 2010.
::  This is enforced but if testing is desired for other releases
::  it's possible to edit.
::

:: Try to find different versions of Visual Studio
call :TryBat "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" && goto VS100

:: no luck
goto batnotfound

:VS100
@echo Note these build commands will not work with the express 
@echo editions of VS2010 which do not appear to include "devenv."
@echo Express editions can be built with lines like:
@echo "VCExpress Berkeley_DB_vs2010.sln /build" or
@echo "VCSExpress BDB_dotNet_vs2010.sln /build"
::goto :batnotfound
@echo Using Visual Studio format 10.0 (Visual Studio 2010)
@echo "" > winbld.out
@echo devenv /build "Release|x64" ..\build_windows\Berkeley_DB_vs2010.sln
@devenv /build "Release|x64" ..\build_windows\Berkeley_DB_vs2010.sln >> winbld.out
@echo devenv ..\build_windows\Berkeley_DB_vs2010.sln /build "Release|x64" /project VS10\db_java.vcxproj
@devenv ..\build_windows\Berkeley_DB_vs2010.sln /build "Release|x64" /project VS10\db_java.vcxproj >> winbld.out
@echo devenv /build "Release|x64" ..\build_windows\BDB_dotNet_vs2010.sln
@devenv /build "Release|x64" ..\build_windows\BDB_dotNet_vs2010.sln >> winbld.out
goto :eof

:batnotfound
echo *********** ERROR: Visual Studio Config batch file not found *************
exit 3
goto end

:: TryBat(BATPATH)
:: If the BATPATH exists, use it and return 0,
:: otherwise, return 1.

:TryBat
:: Filename = "%1"
if not exist %1 exit /b 1
call %1 x64
exit /b 0
goto :eof

:end
