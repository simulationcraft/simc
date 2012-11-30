
# Small SimC release script created by philoptik@gmail.com
# It sits in my /git folder, above the simulationcraft folder
# After tagging for a release, I edit the variables 'version'
# and 'rev' and execute it.

REM This script is used for packaging a release built using MSVC 2008
REM Update the "dir" and "rev" variables appropriately

set version=510-1
set rev=r14772

set target_dir=simc-%version%
set tag=release-%version%
set executable_source_dir=simulationcraft

call git svn clone -%rev% https://simulationcraft.googlecode.com/svn/tags/%tag% %target_dir%/source

del /F /Q /S %target_dir%\source\.git
rd /Q /S %target_dir%\source\.git\
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


xcopy %executable_source_dir%\engine\simc.exe %target_dir%\
xcopy %executable_source_dir%\release\SimulationCraft.exe %target_dir%\
xcopy Win32OpenSSL_Light-1_0_1c.exe %target_dir%\