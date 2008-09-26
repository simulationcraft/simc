@echo off

echo -----------------------------------------
echo Initializing environment
echo.
rmdir source /s /q
rmdir resources /s /q
erase bin\simcraft.exe
erase raid_wotlk.txt

echo.
echo -----------------------------------------
echo Getting sources
echo.
svn checkout http://simulationcraft.googlecode.com/svn/trunk/ source

echo.
echo -----------------------------------------
echo Preparing resources
echo.
mkdir resources
copy source\rpp\*.* resources\*.*
copy source\raid_wotlk.txt raid_wotlk.txt
svn info source > resources\svn.info

echo.
echo -----------------------------------------
echo Building SimulationCraft
echo.
chdir source
mingw32-make
chdir ..
if exist source\simcraft.exe (goto :build_succeeded)
echo.
echo Build failed. Please, find and fix the reason!
goto :end_of_file

:build_succeeded
copy source\simcraft.exe bin\simcraft.exe
erase source\simcraft.exe

echo.
echo -----------------------------------------
echo Creating release packages

:end_of_file
pause