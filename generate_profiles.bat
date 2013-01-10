del /s simc_cache.dat

forfiles -s -m generate*.simc -c "cmd /c %~dp0\simc.exe @file"

pause