@echo off
setlocal enableextensions
set ARGS=%*

:: Ensure msbuild.exe is in path
where /q msbuild.exe || call :error Cannot find msbuild.exe from PATH
if ERRORLEVEL 1 exit /b 1
if "%2" == "" call :usage
if ERRORLEVEL 1 exit /b 1

:: Setup platform and default Visual Studio version
set PLATFORM=%2
if "%PLATFORM%" == "x64" set SUFFIX=64
if "%VSVERSION%" == "" set VSVERSION=2013

:: Determine current Simulationcraft version
if "%SIMCVERSION%" == "" for /F "tokens=3 delims= " %%i in ('findstr /C:"#define SC_MAJOR_VERSION" %SIMCDIR%\engine\simulationcraft.hpp') do set SC_MAJOR_VERSION=%%~i
if "%SIMCVERSION%" == "" for /F "tokens=3 delims= " %%i in ('findstr /C:"#define SC_MINOR_VERSION" %SIMCDIR%\engine\simulationcraft.hpp') do set SC_MINOR_VERSION=%%~i
if "%SIMCVERSION%" == "" set SIMCVERSION=%SC_MAJOR_VERSION%-%SC_MINOR_VERSION%
if "%SIMCVERSION%" == "-" call :error Unable to determine SIMCVERSION
if ERRORLEVEL 1 exit /b 1

:: Ensure mandatory environment variables are set and reasonable
if not defined INSTALL call :error INSTALL environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined SIMCDIR call :error SIMCDIR environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined QTDIR call :error QTDIR environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined REDIST call :error REDIST environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined SZIP call :error SZIP environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined ISCC call :error ISCC environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not exist %INSTALL% call :error INSTALL environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %SIMCDIR% call :error SIMCDIR environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %QTDIR% call :error QTDIR environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %REDIST% call :error REDIST environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %SZIP% call :error SZIP environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %ISCC% call :error ISCC environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror

:: Setup GIT HEAD commithash for the package name if GIT is found
where /q git.exe
if ERRORLEVEL 0 for /F "delims=" %%i IN ('git --git-dir=%SIMCDIR%\.git\ rev-parse --short HEAD') do set GITREV=-%%i
if defined RELEASE set GITREV=

:: Setup archive name and installation directory
set PACKAGENAME=%INSTALL%\simulationcraft-%SIMCVERSION%-%PLATFORM%%GITREV%
set INSTALLDIR=%INSTALL%\simulationcraft-%SIMCVERSION%-%PLATFORM%

:: Begin the build process
call :build_release %ARGS%
if ERRORLEVEL 0 goto :end
if ERRORLEVEL 1 goto :enderror

:build_release
:: Setup the target if given, otherwise default is to build all targets in the solution
if "%3" neq "" set TARGET=/t:%3
if not defined SOLUTION set SOLUTION=simc_vs%VSVERSION%.sln

msbuild.exe "%SIMCDIR%\%SOLUTION%" /p:configuration=%1 /p:platform=%2 /nr:true /m /t:Clean
msbuild.exe "%SIMCDIR%\%SOLUTION%" /p:configuration=%1 /p:platform=%2 /nr:true /m %TARGET%
if ERRORLEVEL 1 exit /b 1
:: Start release copying process
md %INSTALLDIR%
call :copy_base

if "%3" == "" call :copy_simc
if "%3" == "" call :copy_simcgui
if "%3" == "SimcGUI" call :copy_simcgui
if "%3" == "simc" call :copy_simc
:: Aand compress the install directory
call :compress
exit /b 0

:copy_base
robocopy %SIMCDIR%\Profiles\ %INSTALLDIR%\profiles\ *.* /S /NJH /NJS
robocopy %SIMCDIR% %INSTALLDIR%\ README.md COPYING /NJH /NJS
exit /b 0

:copy_simc
robocopy %SIMCDIR% %INSTALLDIR%\ simc%SUFFIX%.exe /NJH /NJS
exit /b 0

:copy_simcgui
if "%PLATFORM%" == "win32" (
	set REDISTPLATFORM=x86
) else (
	set REDISTPLATFORM=%PLATFORM%
)

robocopy %REDIST%\%REDISTPLATFORM%\Microsoft.VC120.CRT %INSTALLDIR%\ msvcp120.dll msvcr120.dll vccorlib120.dll /NJH /NJS
robocopy %SIMCDIR%\ %INSTALLDIR%\ Error.html Welcome.html Welcome.png  /NJH /NJS
robocopy %SIMCDIR%\locale\ %INSTALLDIR%\locale sc_de.qm sc_zh.qm sc_it.qm  /NJH /NJS
robocopy %SIMCDIR%\winreleasescripts\ %INSTALLDIR%\ qt.conf  /NJH /NJS
robocopy %SIMCDIR%\ %INSTALLDIR%\ Simulationcraft%SUFFIX%.exe  /NJH /NJS
if "%SUFFIX%" == "64" %QTDIR%\msvc%VSVERSION%_64\bin\windeployqt.exe --no-translations %INSTALLDIR%\Simulationcraft%SUFFIX%.exe
if "%SUFFIX%" == "" %QTDIR%\msvc%VSVERSION%\bin\windeployqt.exe --no-translations %INSTALLDIR%\Simulationcraft%SUFFIX%.exe
exit /b 0

:compress
%SZIP%\7z.exe a -r %PACKAGENAME% %INSTALLDIR% -mx9 -md=32m
exit /b 0

:usage
echo Usage: build-simc.bat VS-Configuration Platform [Target]
call :help
goto :enderror

:error
echo Error: %*
call :help
goto :enderror

:help
echo.
echo To build, set the following environment variables:
echo ============================================================
echo SIMCVERSION: Simulationcraft release version (default from %SIMCDIR%\engine\simulationcraft.hpp)
echo SIMCDIR    : Simulationcraft source directory root
echo QTDIR      : Root directory of the Qt (5) release
echo REDIST     : Directory containing Windows Runtime Environment redistributables
echo SZIP       : Directory containing 7-Zip compressor
echo ISCC       : Diretory containing Inno Setup (5)
echo INSTALL    : Directory to make an installation package in
echo VSVERSION  : Visual studio version (default 2013)
echo RELEASE    : Set to build a "release version" (no git commit hash suffix)
goto :end

:end
endlocal
exit /b 0

:enderror
endlocal
exit /b 1