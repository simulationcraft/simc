:: Used to automate everything for Collision so he can be lazy.
:: Requirements - Everything needs to be located in your windows path, except MSVC2017
:: 7Zip
:: WINSCP - Optional, it will just not upload the file automatically if you don't have it.
::  - "open downloads" is the command that selects the downloads alias in winscp, which for me is the simulationcraft server. Change downloads to whatever suits you.
:: MSVC 2017 - Fully updated
:: Git
:: QT > 5.6 
:: Inno Setup - http://www.jrsoftware.org/isinfo.php - Used to make the installer, optional if you just want a compressed file.

set simcversion=810-02
set SIMCAPPFULLVERSION=8.1.0.02
:: For bumping the minor version, just change the above lines. 

set qt_dir=C:\Qt\5.10.1\
set visualstudio=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\
set MSBUILDDIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\
cd ..
git clean -f -x -d
set currdir=%cd%
for /F "delims=" %%i IN ('git --git-dir=%currdir%\.git\ rev-parse --short HEAD') do set GITREV=-%%i

call %qt_dir%msvc2017_64\bin\qtenv2.bat
call "%visualstudio%VC\Auxiliary\Build\vcvars64.bat"
cd /D %currdir%
qmake -r -tp vc -spec win32-msvc simulationcraft.pro PGO=1

cd>bla.txt
set /p download=<bla.txt
del bla.txt

::WebEngine compilation.
set install=simc-%simcversion%-win64

"%MSBUILDDIR%msbuild.exe" %currdir%\simulationcraft.sln /p:configuration=Release /nr:true

robocopy "%visualstudio%VC\Redist\MSVC\14.13.26020\x64\Microsoft.VC141.CRT" %install%\ msvcp140.dll msvcp140_1.dll concrt140.dll vccorlib140.dll vcruntime140.dll
robocopy locale\ %install%\locale sc_de.qm sc_zh.qm sc_it.qm
robocopy winreleasescripts\ %install%\ qt.conf
robocopy . %install%\ Welcome.html Welcome.png Simulationcraft.exe simc.exe readme.txt Error.html COPYING LICENSE.BOOST LICENSE.BSD LICENSE.LGPL LICENSE.MIT
robocopy Profiles\ %install%\profiles\ *.* /S
cd %install%
call %qt_dir%\msvc2017_64\bin\windeployqt.exe --no-translations simulationcraft.exe
del vcredist_x64.exe
cd ..

cd winreleasescripts 
iscc.exe /DMyAppVersion=%simcversion% ^
		 /DSimcAppFullVersion="%SIMCAPPFULLVERSION%" "setup64.iss"
				
cd ..
call start winscp /command "open downloads" "put %download%\SimcSetup-%simcversion%-win64.exe -nopreservetime -nopermissions -transfer=binary" "exit"
7z a -r %install%%GITREV% %install% -mx9 -md=32m
call start winscp /command "open downloads" "put %download%\%install%%GITREV%.7z -nopreservetime -nopermissions -transfer=binary" "exit"