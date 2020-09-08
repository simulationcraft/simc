@echo off
if exist simc_cache.dat del /s simc_cache.dat

if exist simc.exe goto 32
goto exit

:32
cd "profiles"
:: PreRaids doesn't match the typical pattern
cd "PreRaids"
echo Running PR_Generate.simc in %cd%
"%~dp0simc.exe" PR_Generate.simc
cd ..\
:: DungeonSlice doesn't match the typical pattern
cd "DungeonSlice"
echo Running DS_Generate.simc in %cd%
"%~dp0simc.exe" DS_Generate.simc
cd ..\
:: TierXX profiles generation
for %%g in (25) do (
  cd Tier%%g
  echo Running T%%g_Generate.simc in %cd%
  "%~dp0simc.exe" T%%g_Generate.simc
  cd ..\
)
goto pause

:exit
echo "This file requires the CLI (simc.exe) to be located in the same folder"

:pause
pause
