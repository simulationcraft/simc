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
:: TWWX profiles generation
for %%g in (TWW1, TWW2) do (
  cd %%g
  echo Running %%g_Generate.simc in %cd%
  "%~dp0simc.exe" %%g_Generate.simc
  cd ..\
)
goto pause

:exit
echo "This file requires the CLI (simc.exe) to be located in the same folder"

:pause
pause
