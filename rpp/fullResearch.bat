@echo off

echo =========Checking batch parameters=========

:check_first_param
if "%1" == "" (goto :check_second_param)
if "%1" == "quick" (goto :check_second_param)
if "%1" == "full" (goto :check_second_param)
goto :bad_params

:check_second_param
if "%2" == "" (goto :params_fine)
if "%2" == "nowiki" (goto :params_fine)
if "%2" == "wiki" (goto :params_fine)
goto :bad_params

:bad_params
echo.
echo Params you're using to run this batch are invalid. Check several first lines of batch file contents to learn full list of supported params.
pause
goto :end_of_file

:params_fine
echo.
echo Params are fine. Check several first lines of batch file contents to learn full list of supported params.

echo.
echo =========Initializing environment=========
echo.

del _RUN_ME.bat
del *.exe /q
rmdir intermediate /s /q
rmdir results /s /q
rmdir wiki /s /q
mkdir intermediate\input
mkdir intermediate\output
mkdir results

echo.
echo =========Preparing binaries=========
echo.

copy bin\scratmini.input.exe scratmini.input.exe
copy bin\scratmini.output.exe scratmini.output.exe
copy bin\simcraft.exe simcraft.exe
copy source\raid_wotlk.txt raid_wotlk.txt

echo.
echo =========Generating sustainability, dps and manaregen data=========

scratmini.input baseline=raid_wotlk.txt scaling=false exclude=shaman=Shaman_Enhancement
@echo on
@call _RUN_ME.bat
@echo off
scratmini.output

echo.
if "%1" == "" (goto :ask_scaling)
if "%1" == "quick" (goto :no_scaling)
echo =========Generating scaling dataz (it's gonna take loads of time btw!)=========
goto :do_scaling

:ask_scaling
set /p choice= ========Wanna generate scaling dataz? (it's gonna take loads of time!) Choose: Y for "Yes", and anything else for "No". 
if "%choice%" == "Y" (goto :do_scaling) else (goto :seems_no_scaling)
:seems_no_scaling
if "%choice%" == "y" (goto :do_scaling) else (goto :no_scaling)

:do_scaling
echo.
scratmini.input baseline=raid_wotlk.txt scaling=true exclude=shaman=Shaman_Enhancement
@echo on
@call _RUN_ME.bat
@echo off
scratmini.output

:no_scaling

echo.
if "%2" == "" (goto :ask_wiki)
if "%2" == "nowiki" (goto :no_wiki)
echo =========Generating report in wiki format=========
goto :do_wiki

:ask_wiki
set /p choice= ========Wanna generate report in wiki format? Choose: Y for "Yes", and anything else for "No". 
if "%choice%" == "Y" (goto :do_wiki) else (goto :seems_no_wiki)
:seems_no_wiki
if "%choice%" == "y" (goto :do_wiki) else (goto :no_wiki)

:do_wiki
echo.
mkdir wiki
copy bin\scratmini.templator.exe scratmini.templator.exe
copy resources\{$Page_WikiName}.wiki wiki\{$Page_WikiName}.wiki
scratmini.templator template=wiki\{$Page_WikiName}.wiki strings=resources\template.strings
erase wiki\{$Page_WikiName}.wiki
copy results\scaling.csv wiki\scaling.csv
copy results\shadow_manaregen.csv wiki\shadow_manaregen.csv
del scratmini.templator.exe

:no_wiki

echo.
echo =========Press any key to remove intermediate files
echo.
pause

erase _RUN_ME.bat
erase *.exe /q
erase raid_wotlk.txt
rmdir intermediate /s /q

:end_of_file