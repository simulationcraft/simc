@echo off

echo -----------------------------------------
echo Initializing environment
echo.

rmdir bin /s /q
rmdir source /s /q
rmdir resources /s /q
rmdir intermediate /s /q
rmdir results /s /q
rmdir wiki /s /q
attrib +s updateFromSVN.bat
erase *.* /a-s /q
attrib -s updateFromSVN.bat

echo.
echo -----------------------------------------
echo Updating from SVN repository
echo.
svn checkout http://simulationcraft.googlecode.com/svn/trunk/ source

echo.
echo -----------------------------------------
echo Deploying automation software
echo source\rpp\updateFromSVN.bat > excludeList.txt
xcopy source\rpp . /t /EXCLUDE:excludeList.txt /y
xcopy source\rpp . /e /EXCLUDE:excludeList.txt /y
pause
erase excludeList.txt
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