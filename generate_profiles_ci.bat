@echo off
if exist simc_cache.dat del /s simc_cache.dat

cd "profiles"
cd "PreRaids"
echo Running PR_Generate.simc in %cd%
"%~dp0builddir\simc.exe" PR_Generate.simc
cd ..\
for %%g in (29 30 31) do (
  cd Tier%%g
  echo Running T%%g_Generate.simc in %cd%
  "%~dp0builddir\simc.exe" T%%g_Generate.simc
  cd ..\
)
goto pause

:exit
echo "This file requires the CLI (simc.exe) to be located in the same folder"

:pause
pause
