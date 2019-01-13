@echo off
setlocal enableextensions
chcp 65001

set ARGS=%*

:: Ensure msbuild.exe is in path
where /q msbuild.exe || call :error Cannot find msbuild.exe from PATH
if ERRORLEVEL 1 exit /b 1
if "%1" == "" call :usage
if ERRORLEVEL 1 exit /b 1

:: Setup platform and default Visual Studio version
set PLATFORM=%2
if not defined PLATFORM set PLATFORM=%VSCMD_ARG_TGT_ARCH%

:: Determine current Simulationcraft version
if not defined SIMCVERSION for /F "tokens=3 delims= " %%i in ('findstr /C:"#define SC_MAJOR_VERSION" %SIMCDIR%\engine\simulationcraft.hpp') do set SC_MAJOR_VERSION=%%~i
if not defined SIMCVERSION for /F "tokens=3 delims= " %%i in ('findstr /C:"#define SC_MINOR_VERSION" %SIMCDIR%\engine\simulationcraft.hpp') do set SC_MINOR_VERSION=%%~i
if not defined SIMCVERSION set SIMCVERSION=%SC_MAJOR_VERSION%-%SC_MINOR_VERSION%
if "%SIMCVERSION%" == "-" call :error Unable to determine SIMCVERSION
if ERRORLEVEL 1 goto :enderror

:: Ensure mandatory environment variables are set and reasonable
if not defined INSTALL call :error INSTALL environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined SIMCDIR call :error SIMCDIR environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined QTDIR call :error QTDIR environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not defined CURL_ROOT call :error CURL_ROOT environment variable not defined
if ERRORLEVEL 1 goto :enderror
if not exist %INSTALL% call :error INSTALL environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %SIMCDIR% call :error SIMCDIR environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %QTDIR% call :error QTDIR environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror
if not exist %CURL_ROOT% call :error CURL_ROOT environment variable points to missing directory
if ERRORLEVEL 1 goto :enderror

if defined RELEASE if not defined RELEASE_CREDENTIALS call :error RELEASE builds must set RELEASE_CREDENTIALS
if ERRORLEVEL 1 goto :enderror

if defined RELEASE set SC_DEFAULT_APIKEY=%RELEASE_CREDENTIALS%

:: Setup GIT HEAD commithash for the package name if GIT is found
where /q git.exe
if ERRORLEVEL 0 for /F "delims=" %%i IN ('git --git-dir=%SIMCDIR%\.git\ rev-parse --short HEAD') do set GITREV=-%%i
if defined RELEASE set GITREV=

:: Setup archive name and installation directory
if "%PLATFORM%" == "x64" set PACKAGESUFFIX=win64
if "%PLATFORM%" == "x86" set PACKAGESUFFIX=win32

if not defined PACKAGESUFFIX call :error Unable to determine target architecture
if ERRORLEVEL 1 goto :enderror

set PACKAGENAME=%INSTALL%\simc-%SIMCVERSION%-%PACKAGESUFFIX%%GITREV%
set INSTALLDIR=%INSTALL%\simc-%SIMCVERSION%-%PACKAGESUFFIX%

:: Begin the build process
call :build_release %ARGS%
if ERRORLEVEL 0 goto :end
if ERRORLEVEL 1 goto :enderror

:build_release

:: Setup the target if given, otherwise default is to clean and build all targets in the solution
if "%3" neq "" set TARGET=/t:%3
if "%3" == "" set TARGET=/t:Clean,Build

:: Generate solution before building
del /Q "%SIMCDIR%\.qmake.stash"
%QTDIR%\bin\qmake.exe CURL_ROOT=%CURL_ROOT% -r -tp vc -spec win32-msvc -o "%SIMCDIR%\simulationcraft.sln" "%SIMCDIR%\simulationcraft.pro"

:: Figure out the platform target, Qt creates "Win32" for x86, x64 is intact
set VSPLATFORM=%PLATFORM%
if "%PLATFORM%" == "x86" set VSPLATFORM=Win32

msbuild.exe "%SIMCDIR%\simulationcraft.sln" /p:configuration=%1 /p:platform=%VSPLATFORM% /nr:true /m /p:QTDIR=%QTDIR% %TARGET%
if ERRORLEVEL 1 exit /b 1
:: Start release copying process
md %INSTALLDIR%
call :copy_base

:: Copy release-specific files
call :copy_simc
call :copy_simcgui
:: Build the Inno Setup installer
call :build_installer

:: Aand compress the install directory
call :compress
exit /b 0

:copy_base
robocopy %SIMCDIR%\Profiles\ %INSTALLDIR%\profiles\ *.* /S /NJH /NJS
robocopy %SIMCDIR% %INSTALLDIR%\ README.md COPYING LICENSE LICENSE.BOOST LICENSE.BSD LICENSE.BSD2 LICENSE.LGPL LICENSE.MIT /NJH /NJS
exit /b 0

:copy_simc
robocopy %SIMCDIR% %INSTALLDIR%\ simc.exe /NJH /NJS
exit /b 0

:copy_simcgui
if "%1" == "Release" (
  set WINDEPLOYQTARGS="--release"
) else (
  set WINDDEPLOYQTARGS="--debug"
)

robocopy %SIMCDIR%\ %INSTALLDIR%\ Error.html Welcome.html Welcome.png  /NJH /NJS
robocopy %SIMCDIR%\locale\ %INSTALLDIR%\locale sc_de.qm sc_zh.qm sc_it.qm  /NJH /NJS
robocopy %SIMCDIR%\winreleasescripts\ %INSTALLDIR%\ qt.conf  /NJH /NJS
robocopy %CURL_ROOT%\bin\ %INSTALLDIR%\ libcurl.dll /NJH /NJS
robocopy %SIMCDIR%\ %INSTALLDIR%\ Simulationcraft.exe  /NJH /NJS
%QTDIR%\bin\windeployqt.exe --force --no-translations --compiler-runtime %WINDEPLOYQTARGS% %INSTALLDIR%\Simulationcraft.exe
exit /b 0

:build_installer
if not defined ISCC exit /b 0

:: Setup additional variables for installer package
set SIMCAPPNAME="Simulationcraft(%PLATFORM%)"
if "%PLATFORM%" == "x86" (
	set SIMCSUFFIX=Win32
) else (
	set SIMCSUFFIX=Win64
)
set SIMCAPPFULLVERSION=%SC_MAJOR_VERSION:~0,1%.%SC_MAJOR_VERSION:~1,1%.%SC_MAJOR_VERSION:~2,2%.%SC_MINOR_VERSION%
if "%GITREV%" neq "" set SIMCSUFFIX=%SIMCSUFFIX%%GITREV%

%ISCC%\iscc.exe /DSimcAppFullVersion=%SIMCAPPFULLVERSION% ^
				/DSimcAppName=%SIMCAPPNAME% ^
				/DSimcReleaseSuffix="%SIMCSUFFIX%" ^
				/DSimcReleaseDir="%INSTALLDIR%" ^
				/DSimcAppVersion="%SIMCVERSION%" ^
				/DSimcAppExeName="Simulationcraft.exe" ^
				/DSimcIconFile="%SIMCDIR%\qt\icon\Simcraft2.ico" ^
				/DSimcOutputDir="%INSTALL%" %SIMCDIR%\WinReleaseScripts\SetupSimc.iss
if ERRORLEVEL 1 exit /b 1
exit /b 0

:compress
if not defined SZIP exit /b 0
%SZIP%\7z.exe a -r %PACKAGENAME% %INSTALLDIR% -mx9 -md=32m
exit /b 0

:usage
echo Usage: build-simc.bat VS-Configuration [Architecture] [Target]
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
echo QTDIR      : Root directory of the Qt (5) release, including the platform path (e.g., msvc2017_64)
echo CURL_ROOT  : Root directory for the CURL build output (contains include, bin, lib directories)
echo SZIP       : Directory containing 7-Zip compressor (optional, if omitted no compressed file)
echo ISCC       : Diretory containing Inno Setup (optional, if omitted no setup will be built)
echo INSTALL    : Directory to make an installation package in
echo RELEASE    : Set to build a "release version" (no git commit hash suffix)
echo            : If set, set RELEASE_CREDENTIALS to the Battle.net key for the release
goto :end

:end
endlocal
exit /b 0

:enderror
endlocal
exit /b 1
