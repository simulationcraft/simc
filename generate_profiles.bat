@echo off
if exist simc_cache.dat del /s simc_cache.dat
:: Preraid doesn't match the typical pattern
::cd profiles/PreRaid
::%~dp0\simc64.exe generate_PreRaid.simc
::cd ../

if exist simc64.exe goto 64
if exist simc.exe goto 32
goto exit
:64
@echo on
forfiles -s -m generate_T1?*.simc -c "cmd /c echo Running @path && %~dp0simc64.exe @file"
pause
exit
:32
@echo on
forfiles -s -m generate_T1?*.simc -c "cmd /c echo Running @path && %~dp0simc.exe @file"
pause
exit
:exit
echo "This file requires the CLI to be located in the same folder"
pause
exit