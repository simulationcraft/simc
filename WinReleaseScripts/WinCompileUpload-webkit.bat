:: Used to automate everything for Alex so he can be lazy.
:: Requirements - Everything needs to be located in your windows path, except MSVC2013
:: 7Zip
:: WINSCP - Optional, it will just not upload the file automatically if you don't have it.
::  - "open downloads" is the command that selects the downloads alias in winscp, which for me is the simulationcraft server. Change downloads to whatever suits you.
:: MSVC 2013 - Fully updated
:: Git
:: QT 5.5.1, or whatever version we are currently using
:: Inno Setup - http://www.jrsoftware.org/isinfo.php - Used to make the installer, optional if you just want a compressed file.

@echo off
:: Building with PGO data will add 10-15 minutes to compile.
set /p ask=Build with PGO data? Only applies to 64-bit installation.  (y/n)
@echo on

set simcversion=623-03
:: For bumping the minor version, just change the above line. Make sure to also change setup32.iss and setup64.iss as well. 
set simcfiles=C:\Simulationcraft\
:: Location of source files
set qt_dir=C:\Qt\Qt5.5.1\5.5\
:: Location of QT
set redist=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\redist\
:: This is a really standard location for VS2013, but change it if you installed it somewhere else.
set ssllocation64=C:\OpenSSL-Win64\bin
:: Location of openssl32/64

cd ..
git clean -f -x -d

cd>bla.txt
set /p download=<bla.txt
del bla.txt

::Webkit compilation.
set install=simc-%simcversion%-win64

for /f "skip=2 tokens=2,*" %%A in ('reg.exe query "HKLM\SOFTWARE\Microsoft\MSBuild\ToolsVersions\12.0" /v MSBuildToolsPath') do SET MSBUILDDIR=%%B

if %ask%==y "%MSBUILDDIR%msbuild.exe" %simcfiles%\simc_vs2013.sln /p:configuration=WebKit-PGO /p:platform=x64 /nr:true
if %ask%==n "%MSBUILDDIR%msbuild.exe" %simcfiles%\simc_vs2013.sln /p:configuration=WebKit /p:platform=x64 /nr:true

robocopy "%redist%x64\Microsoft.VC120.CRT" %install%\ msvcp120.dll msvcr120.dll vccorlib120.dll
robocopy locale\ %install%\locale sc_de.qm sc_zh.qm sc_it.qm
robocopy winreleasescripts\ %install%\ qt.conf
robocopy . %install%\ Welcome.html Welcome.png Simulationcraft64.exe simc64.exe readme.txt Error.html COPYING
robocopy %ssllocation64% %install%\ libeay32.dll ssleay32.dll 
robocopy Profiles\ %install%\profiles\ *.* /S
cd %install%
%qt_dir%\msvc2013_64\bin\windeployqt.exe --no-translations simulationcraft64.exe
cd ..

cd winreleasescripts
iscc.exe "setup64.iss"
cd ..
call start winscp /command "open downloads" "put %download%\SimcSetup-%simcversion%-win64.exe -nopreservetime -nopermissions -transfer=binary" "exit"
7z a -r %install% %install% -mx9 -md=32m
call start winscp /command "open downloads" "put %download%\%install%.7z -nopreservetime -nopermissions -transfer=binary" "exit"
pause