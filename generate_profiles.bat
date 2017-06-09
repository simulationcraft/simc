@echo off
if exist simc_cache.dat del /s simc_cache.dat
:: Preraid doesn't match the typical pattern
::cd profiles/PreRaid
::%~dp0\simc.exe generate_PreRaid.simc
::cd ../

if exist simc.exe goto 32
goto exit
:32
@echo on
forfiles -s -m generate_T*.simc -c "cmd /c echo Running @path && %~dp0simc.exe @file"
goto pause
:exit
echo "This file requires the CLI to be located in the same folder"
:pause
pause