
REM Small SimC release script created by philoptik@gmail.com
REM It sits in my /git folder, above the simulationcraft folder
REM After tagging for a release, I edit the variables 'version'
REM and 'rev' and execute it.

IF (%1) == () exit /b

set version=%1

REM REQUIRES: qmake, make, g++ in your PATH, as well as a archiver

set target_dir=simc-%version%
set build_dir=simc-%version%-build
set tag=release-%version%
set MAKE=mingw32-make
set QMAKE=qmake
set ARCHIVER=7z a


call svn checkout https://simulationcraft.googlecode.com/svn/tags/%tag% %target_dir%/source

REM copy things over to the temporary build dir, and build cli + gui there
xcopy /E /I /H %target_dir%\source %build_dir%
cd %build_dir%
cd engine
%MAKE% -j4
cd ..
%QMAKE% simcqt.pro
%MAKE% -j4 release
cd ..


del /F /Q /S %target_dir%\source\.svn
rd /Q /S %target_dir%\source\.svn\
xcopy /E /I /H %target_dir%\source\profiles %target_dir%\profiles
rd /Q /S %target_dir%\source\profiles\

xcopy /E /I /H %target_dir%\source\locale %target_dir%\locale
rd /Q /S %target_dir%\source\locale\

move %target_dir%\source\Examples.simc %target_dir%\
move %target_dir%\source\Legend.html %target_dir%\
move %target_dir%\source\READ_ME_FIRST.txt %target_dir%\
move %target_dir%\source\Welcome.html %target_dir%\
move %target_dir%\source\Welcome.png %target_dir%\
move %target_dir%\source\windows_env_path.vbs %target_dir%\


xcopy %target_dir%\source\mingw_qt_dll_setup.bat %target_dir%\
cd %target_dir%
call mingw_qt_dll_setup.bat
cd ..
del %target_dir%\mingw_qt_dll_setup.bat


xcopy %build_dir%\engine\simc.exe %target_dir%\
xcopy %build_dir%\release\SimulationCraft.exe %target_dir%\
xcopy Win32OpenSSL_Light-1_0_1c.exe %target_dir%\

REM remove tmp build dir
rd /Q /S %build_dir%


call %ARCHIVER% simc-%version%-win32.zip %target_dir%
