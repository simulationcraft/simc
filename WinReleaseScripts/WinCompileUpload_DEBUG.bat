:: Used to automate everything for Collision so he can be lazy.
:: Requirements - Everything needs to be located in your windows path, except MSVC2015
:: 7Zip
:: WINSCP - Optional, it will just not upload the file automatically if you don't have it.
::  - "open downloads" is the command that selects the downloads alias in winscp, which for me is the simulationcraft server. Change downloads to whatever suits you.
:: MSVC 2015 - Fully updated
:: Git
:: QT 5.6.0, or whatever version we are currently using
:: Inno Setup - http://www.jrsoftware.org/isinfo.php - Used to make the installer, optional if you just want a compressed file.

cd ..
git clean -f -x -d
set currdir=%cd%
call C:\Qt\Qt5.6.0\5.6\msvc2015_64\bin\qtenv2.bat
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
cd %currdir%
qmake -r -tp vc -spec win32-msvc2015 simulationcraft.pro

set simcversion=720-01
set SIMCAPPFULLVERSION=7.2.0.01
:: For bumping the minor version, just change the above line.
:: Location of source files
set qt_dir=C:\Qt\Qt5.8.0\5.8\
:: Location of QT
set redist=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\
:: This is a really standard location for VS2015, but change it if you installed it somewhere else.

for /F "delims=" %%i IN ('git --git-dir=%currdir%\.git\ rev-parse --short HEAD') do set GITREV=-%%i

cd>bla.txt
set /p download=<bla.txt
del bla.txt

::WebEngine compilation.
set install=simc-%simcversion%-SLOW_DEBUGGING_BUILD

for /f "skip=2 tokens=2,*" %%A in ('reg.exe query "HKLM\SOFTWARE\Microsoft\MSBuild\ToolsVersions\14.0" /v MSBuildToolsPath') do SET MSBUILDDIR=%%B

"%MSBUILDDIR%msbuild.exe" %currdir%\simulationcraft.sln /p:configuration=Debug /nr:true /m

robocopy "%redist%x64\Microsoft.VC140.CRT" %install%\ msvcp140d.dll vccorlib140d.dll vcruntime140d.dll
robocopy locale\ %install%\locale sc_de.qm sc_zh.qm sc_it.qm
robocopy winreleasescripts\ %install%\ qt.conf
robocopy . %install%\ Welcome.html Welcome.png Simulationcraft.exe simc.exe readme.txt Error.html COPYING LICENSE.BOOST LICENSE.BSD LICENSE.LGPL LICENSE.MIT
robocopy Profiles\ %install%\profiles\ *.* /S
cd %install%
%qt_dir%\msvc2015_64\bin\windeployqt.exe --no-translations simulationcraft.exe
del vcredist_x64.exe
cd ..

7z a -r %install%%GITREV% %install% -mx9 -md=32m
call start winscp /command "open downloads" "put %download%\%install%%GITREV%.7z -nopreservetime -nopermissions -transfer=binary" "exit"