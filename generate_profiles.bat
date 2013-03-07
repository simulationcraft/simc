del /s simc_cache.dat

:: Preraid doesn't match the typical pattern
%~dp0\simc.exe generate_PreRaid.simc

forfiles -s -m generate_????.simc -c "cmd /c echo Running @path && %~dp0simc.exe @file"

pause