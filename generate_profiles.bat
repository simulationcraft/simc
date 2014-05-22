del /s simc_cache.dat

:: Preraid doesn't match the typical pattern
cd profiles/PreRaid
%~dp0\simc.exe generate_PreRaid.simc
cd ../

forfiles -s -m generate_????.simc -c "cmd /c echo Running @path && %~dp0simc.exe @file"

pause