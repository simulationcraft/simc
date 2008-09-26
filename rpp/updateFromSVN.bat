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
echo.
echo source\rpp\updateFromSVN.bat > excludeList.txt
xcopy source\rpp . /t /EXCLUDE:excludeList.txt /y
xcopy source\rpp . /e /EXCLUDE:excludeList.txt /y
erase excludeList.txt
svn info source > resources\svn.info

echo.
echo -----------------------------------------
echo Building SimulationCraft
echo.
chdir source
mingw32-make
chdir ..
ping 1.1.1.1 -n 1 -w 1000 > nul
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
echo.

:ask_releases
set /p choice= ========Wanna create Windows and Linux releases? Choose: Y for "Yes", and anything else for "No". 
if "%choice%" == "Y" (goto :do_releases) else (goto :seems_no_releases)
:seems_no_releases
if "%choice%" == "y" (goto :do_releases) else (goto :no_releases)

:do_releases
echo.
mkdir releases
echo Oops. This functionality is not supported yet. Sorry! > releases\simcraft-{$Current_SimCraftRevision}-windows.zip
echo Oops. This functionality is not supported yet. Sorry! > releases\simcraft-{$Current_SimCraftRevision}.tar.gz
echo Oops. This functionality is not supported yet. Sorry!
copy bin\scratmini.templator.exe scratmini.templator.exe
scratmini.templator template=releases\simcraft-{$Current_SimCraftRevision}-windows.zip
scratmini.templator template=releases\simcraft-{$Current_SimCraftRevision}.tar.gz
erase scratmini.templator.exe
erase releases\simcraft-{$Current_SimCraftRevision}* /q

:no_releases
:end_of_file
echo.
pause